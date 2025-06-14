#include "codeGen/exprIRConverter.hpp"
#include "codeGen/dispatchVector.hpp"

namespace codegen {

// Taken from HierarchyCheck
static bool isClass(std::shared_ptr<parsetree::ast::AstNode> decl) {
  return !!std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(decl);
}

static bool isSuperClass(std::shared_ptr<parsetree::ast::AstNode> super,
                         std::shared_ptr<parsetree::ast::AstNode> child) {
  if (!child || !super) {
    return false;
  }
  if (!isClass(child)) {
    return false;
  }
  if (!isClass(super)) {
    return false;
  }
  auto childDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(child);
  auto superDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(super);

  for (auto &superClass : childDecl->getSuperClasses()) {
    if (!superClass || !superClass->getResolvedDecl().getAstNode())
      continue;
    // Cast to class
    auto superClassDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
        superClass->getResolvedDecl().getAstNode());

    if (superClassDecl == superDecl)
      return true;

    if (isSuperClass(super, std::dynamic_pointer_cast<parsetree::ast::AstNode>(
                                superClassDecl)))
      return true;
  }
  return false;
}

std::shared_ptr<tir::Expr>
ExprIRConverter::mapValue(std::shared_ptr<parsetree::ast::ExprValue> &value) {
  // std::cout << "mapValue:\n";
  // value->print(std::cout);
  /*
  MemberName, MethodName
  TypeNode
  ThisNode done
  Literal
  */
  if (auto thisNode =
          std::dynamic_pointer_cast<parsetree::ast::ThisNode>(value)) {
    return std::make_shared<tir::Temp>("this", thisNode);
  } else if (auto methodName =
                 std::dynamic_pointer_cast<parsetree::ast::MethodName>(value)) {
    // TODO: This is a hack, delay the eval to method invocation
    return std::make_shared<tir::TempTIR>(value,
                                          tir::TempTIR::Type::MethodName);
  } else if (auto typeNode =
                 std::dynamic_pointer_cast<parsetree::ast::TypeNode>(value)) {
    // TODO: This is a hack, delay the eval to cast, class creation, array *
    return std::make_shared<tir::TempTIR>(value, tir::TempTIR::Type::TypeNode);
  }

  // left case Literal and MemberName

  if (auto literal =
          std::dynamic_pointer_cast<parsetree::ast::Literal>(value)) {
    // int64_t, bool, char, std::string, std::nullptr_t
    if (literal->getLiteralType() == parsetree::ast::Literal::Type::Integer) {
      int64_t val = literal->getAsInt();
      return std::make_shared<tir::Const>(val);
    } else if (literal->getLiteralType() ==
               parsetree::ast::Literal::Type::Boolean) {
      int64_t val = literal->getAsInt();
      if (val != 0 && val != 1)
        throw std::runtime_error("Invalid boolean literal in codegen");
      return std::make_shared<tir::Const>(val);
    } else if (literal->getLiteralType() ==
               parsetree::ast::Literal::Type::Character) {
      int64_t val = literal->getAsInt();
      if (val < 0 || val > 255)
        throw std::runtime_error("Invalid character literal in codegen");
      return std::make_shared<tir::Const>(val);
    } else if (literal->getLiteralType() ==
               parsetree::ast::Literal::Type::String) {

      /*
      essentially doing:
      String s = new String();
      s.chars = new char[] { 'h', 'e', 'l', 'l', 'o' };

      ESEQ {
        SeqIR {
            t1 = malloc(12)
            Mem[t1] = @DV_String
            Mem[t1 + 4] = 0
            Mem[t1 + 8] = 0
            CALL @String::<init>(), this = t1
            t2 = t1 + 8
            Mem[t2] = malloc(28)
            Mem[Mem[t2]]     = 5
            Mem[Mem[t2] + 4] = @DV_Array
            Mem[Mem[t2] + 8]  = 104  // 'h'
            Mem[Mem[t2] + 12] = 101  // 'e'
            Mem[Mem[t2] + 16] = 108  // 'l'
            Mem[Mem[t2] + 20] = 108  // 'l'
            Mem[Mem[t2] + 24] = 111  // 'o'
        }
        Result: t1
    }
      */

      // get the string value
      auto val = literal->getAsString();
      // std::cout << "val: " << val << "." << std::endl;
      std::vector<std::shared_ptr<tir::Stmt>> seqVec;

      auto stringClass = astManager->java_lang.String;
      auto stringDV = codegen::DispatchVectorBuilder::getDV(stringClass);
      int numFields = stringDV->fieldVector.size();

      // create string reference
      std::string stringRefName = tir::Temp::generateName("string_ref");

      // allocate space for class
      seqVec.push_back(std::make_shared<tir::Move>(
          std::make_shared<tir::Temp>(stringRefName),
          tir::Call::makeMalloc(
              std::make_shared<tir::Const>((numFields + 1) * 4))));

      // first location is DV
      seqVec.push_back(std::make_shared<tir::Move>(
          std::make_shared<tir::Mem>(
              std::make_shared<tir::Temp>(stringRefName)),
          // location
          std::make_shared<tir::Temp>(codeGenLabels->getClassLabel(stringClass),
                                      nullptr, true)));

      // initialize all fields
      int count = 0;
      for (auto &field : stringDV->fieldVector) {
        auto initializer = field->hasInit()
                               ? innerExprConverter->evaluateList(
                                     field->getInitializer()->getExprNodes())
                               : std::make_shared<tir::Const>(0);
        seqVec.push_back(std::make_shared<tir::Move>(
            std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
                tir::BinOp::OpType::ADD,
                std::make_shared<tir::Temp>(stringRefName),
                std::make_shared<tir::Const>((count + 1) * 4))),
            initializer));
        count++;
      }

      // arg vector
      std::vector<std::shared_ptr<tir::Expr>> args;

      // find the zero arg constructor for string
      std::shared_ptr<parsetree::ast::MethodDecl> constructor = nullptr;
      for (auto &method : stringClass->getConstructors()) {
        if (method->getParams().size() == 0) {
          constructor = method;
          break;
        }
      }
      if (constructor == nullptr) {
        throw std::runtime_error("No zero arg constructor for String found");
      }

      // call the constructor
      seqVec.push_back(std::make_shared<tir::Exp>(std::make_shared<tir::Call>(
          std::make_shared<tir::Name>(
              codeGenLabels->getMethodLabel(constructor)),
          std::make_shared<tir::Temp>(stringRefName), args)));

      // get chars field
      std::shared_ptr<parsetree::ast::FieldDecl> charsField =
          stringClass->getField("chars");
      if (charsField == nullptr) {
        throw std::runtime_error("Chars field not found in String class");
      }
      int charsFieldIndex = stringDV->getFieldOffset(charsField) + 1;

      // create chars ref
      std::string charsRefName = tir::Temp::generateName("chars_ref");

      // move mem location of chars to chars ref
      seqVec.push_back(std::make_shared<tir::Move>(
          std::make_shared<tir::Temp>(charsRefName),
          std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD,
              std::make_shared<tir::Temp>(stringRefName),
              std::make_shared<tir::Const>(charsFieldIndex * 4))));

      // resize the chars field to 4 * sizeof(value) + 8
      seqVec.push_back(std::make_shared<tir::Move>(
          std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(charsRefName)),
          tir::Call::makeMalloc(
              std::make_shared<tir::Const>(4 * val.length() + 8))));

      // write size
      seqVec.push_back(std::make_shared<tir::Move>(
          std::make_shared<tir::Mem>(std::make_shared<tir::Mem>(
              std::make_shared<tir::Temp>(charsRefName))),
          std::make_shared<tir::Const>(val.length())));

      // attach array DV
      auto arrayClass = astManager->java_lang.Arrays;
      if (arrayClass == nullptr) {
        throw std::runtime_error("java.util.Arrays class not found");
      }
      seqVec.push_back(std::make_shared<tir::Move>(
          // MEM(arr + 4) = DV for arrays
          std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD,
              std::make_shared<tir::Mem>(
                  std::make_shared<tir::Temp>(charsRefName)),
              std::make_shared<tir::Const>(4))),
          std::make_shared<tir::Temp>(codeGenLabels->getClassLabel(arrayClass),
                                      nullptr, true)));

      // initialize the new chars array, starting at MEM[arr + 8]
      for (int i = 0; i < val.length(); i++) {
        char c = val[i];
        seqVec.push_back(std::make_shared<tir::Move>(
            std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
                tir::BinOp::OpType::ADD,
                std::make_shared<tir::Mem>(
                    std::make_shared<tir::Temp>(charsRefName)),
                std::make_shared<tir::Const>(8 + i * 4))),
            std::make_shared<tir::Const>(c)));
      }
      seqVec.push_back(std::make_shared<tir::Move>(
          std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(charsRefName)),
          std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD,
              std::make_shared<tir::Mem>(
                  std::make_shared<tir::Temp>(charsRefName)),
              std::make_shared<tir::Const>(4))));

      return std::make_shared<tir::ESeq>(
          std::make_shared<tir::Seq>(seqVec),
          std::make_shared<tir::Temp>(stringRefName));

    } else if (literal->getLiteralType() ==
               parsetree::ast::Literal::Type::Null) {
      return std::make_shared<tir::Const>(0);
    } else {
      throw std::runtime_error("Invalid literal in codegen");
    }
  }

  if (auto memberName =
          std::dynamic_pointer_cast<parsetree::ast::MemberName>(value)) {

    // Todo: If array length, special case handled in field access
    // static field
    std::shared_ptr<parsetree::ast::Type> realType = nullptr;
    if (auto fieldDecl = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(
            memberName->getResolvedDecl())) {
      if (fieldDecl->getRealType() &&
          (fieldDecl->getRealType() != fieldDecl->getType())) {
        realType = fieldDecl->getRealType();
      }
      if (fieldDecl->isStatic()) {
        auto retExpr = std::make_shared<tir::Temp>(
            codeGenLabels->getStaticFieldLabel(fieldDecl), fieldDecl, true);
        if (realType) {
          realTypeMap[retExpr] = realType;
        }
        return retExpr;
      }
    } else if (auto varDecl =
                   std::dynamic_pointer_cast<parsetree::ast::VarDecl>(
                       memberName->getResolvedDecl())) {
      if (varDecl->getRealType() &&
          (varDecl->getRealType() != varDecl->getType())) {
        realType = varDecl->getRealType();
      }
      // parameter and local variable
      if (varDecl->isInParam()) {
        auto retExpr = std::make_shared<tir::Temp>(
            codeGenLabels->getParameterLabel(varDecl), varDecl);
        if (realType) {
          realTypeMap[retExpr] = realType;
        }
        return retExpr;
      }
      auto retExpr = std::make_shared<tir::Temp>(
          codeGenLabels->getLocalVariableLabel(varDecl), varDecl);

      if (realType) {
        realTypeMap[retExpr] = realType;
      }
      return retExpr;
    }
    // instance field
    if (auto fieldDecl = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(
            memberName->getResolvedDecl())) {
      // local (accessed by this or by itself)
      if (memberName->isAccessedByThis() || !(memberName->isNotAsBase())) {
        auto dv = codegen::DispatchVectorBuilder::getDV(currentClass);
        int offset = dv->getFieldOffset(fieldDecl);
        if (offset == -1)
          throw std::runtime_error("cannot find field " + fieldDecl->getName() +
                                   " in current class");

        auto retExpr = std::make_shared<tir::Mem>(
            std::make_shared<tir::BinOp>(
                tir::BinOp::OpType::ADD,
                std::make_shared<tir::Temp>("this", currentClass),
                std::make_shared<tir::Const>(4 * (offset + 1)) // +1 for header
                ),
            fieldDecl);
        if (realType) {
          realTypeMap[retExpr] = realType;
        }
        return retExpr;
      }
      // obj.field
      // left for field access to handle
      auto retExpr = std::make_shared<tir::TempTIR>(
          memberName, tir::TempTIR::Type::FieldAccess);
      if (realType) {
        realTypeMap[retExpr] = realType;
      }
      return retExpr;
    }
  }
  throw std::runtime_error("Invalid value in mapValue in codegen");
}

// Get earliest toString method
std::shared_ptr<parsetree::ast::MethodDecl>
getToStringMethod(std::shared_ptr<parsetree::ast::ClassDecl> classDecl,
                  std::shared_ptr<parsetree::ast::ClassDecl> object) {
  for (auto func : classDecl->getAllMethods()) {
    if (func->getName() == "toString" && func->getParams().size() == 0) {
      return func;
    }
  }
  int numSuperClasses = classDecl->getSuperClasses().size();
  for (auto &super : classDecl->getSuperClasses()) {
    if (!super || !super->isResolved())
      continue;
    auto superDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
        super->getResolvedDecl().getAstNode());
    if (!superDecl)
      continue;
    // Since Object is always a parent, only traverse to object if it's the only
    // parent
    if (numSuperClasses != 1 && superDecl == object)
      continue;
    auto method = getToStringMethod(superDecl, object);
    if (method)
      return method;
  }
  return nullptr;
}

std::shared_ptr<tir::Expr> ExprIRConverter::evalStringConcatenation(
    std::shared_ptr<parsetree::ast::BinOp> &op,
    const std::shared_ptr<tir::Expr> lhs,
    const std::shared_ptr<tir::Expr> rhs) {
  // If both, then make new string entry and add both chars
  const std::string stringConcatPrefix = "string_concat_";
  std::cout << "String Concatentation: " << std::endl;
  lhs->print(std::cout);
  rhs->print(std::cout);
  std::cout << "Done SC Print!" << std::endl;

  if (op->getLhsType()->isString() && op->getRhsType()->isString()) {
    {
      /** Old implementation (manually construct new string) */
      // std::cout << "String Concatenation: both sides string" << std::endl;
      // Initiliazing metadata for string instances
      // auto stringClass = astManager->java_lang.String;
      // auto stringDV = codegen::DispatchVectorBuilder::getDV(stringClass);
      // int numFields = stringDV->fieldVector.size();
      // // get chars field
      // std::shared_ptr<parsetree::ast::FieldDecl> charsField =
      //     stringClass->getField("chars");
      // if (charsField == nullptr) {
      //   throw std::runtime_error("Chars field not found in String class");
      // }
      // int charsFieldIndex = stringDV->getFieldOffset(charsField) + 1;
      // std::cout << "charsFieldIndex (stringConcat): " << charsFieldIndex <<
      // std::endl;

      // Evaluating a + b where a,b are strings
      // std::vector<std::shared_ptr<tir::Stmt>> seqVec;
      // std::cout << "String Concatenation: initialized" << std::endl;
      // Keep a and b in registers to avoid redundancy
      // std::string strA = tir::Temp::generateName(stringConcatPrefix +
      // "str_a"); std::string strB = tir::Temp::generateName(stringConcatPrefix
      // + "str_b"); seqVec.push_back(
      //     std::make_shared<tir::Move>(std::make_shared<tir::Temp>(strA),
      //     lhs));
      // seqVec.push_back(
      //     std::make_shared<tir::Move>(std::make_shared<tir::Temp>(strB),
      //     rhs));
      // // Get position of a's characters
      // std::string charsRefA =
      //     tir::Temp::generateName(stringConcatPrefix + "chars_ref_a");
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(charsRefA),
      //     std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
      //         tir::BinOp::OpType::ADD, std::make_shared<tir::Temp>(strA),
      //         std::make_shared<tir::Const>(charsFieldIndex * 4)))));
      // // Get string a length
      // std::string lenA = tir::Temp::generateName(stringConcatPrefix +
      // "len_a"); seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(lenA),
      //     std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(charsRefA))));

      // // Get position of b's characters
      // std::string charsRefB =
      //     tir::Temp::generateName(stringConcatPrefix + "chars_ref_b");
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(charsRefB),
      //     std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
      //         tir::BinOp::OpType::ADD, std::make_shared<tir::Temp>(strB),
      //         std::make_shared<tir::Const>(charsFieldIndex * 4)))));
      // // Get string b length
      // std::string lenB = tir::Temp::generateName(stringConcatPrefix +
      // "len_b"); seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(lenB),
      //     std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(charsRefB))));

      // std::cout << "String Concatenation: get lens and chars" << std::endl;
      // // Get total size + malloc size
      // std::string totalLen =
      //     tir::Temp::generateName(stringConcatPrefix + "total_len");
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(totalLen),
      //     std::make_shared<tir::BinOp>(tir::BinOp::OpType::ADD,
      //                                  std::make_shared<tir::Temp>(lenA),
      //                                  std::make_shared<tir::Temp>(lenB))));
      // std::string newCharsRef =
      //     tir::Temp::generateName(stringConcatPrefix + "new_chars_ref");
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(newCharsRef),
      //     tir::Call::makeMalloc(std::make_shared<tir::BinOp>(
      //         tir::BinOp::OpType::ADD,
      //         std::make_shared<tir::BinOp>(tir::BinOp::OpType::MUL,
      //                                      std::make_shared<tir::Temp>(totalLen),
      //                                      std::make_shared<tir::Const>(4)),
      //         std::make_shared<tir::Const>(8)))));

      // // Fill in new string's character array metadata
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(newCharsRef)),
      //     std::make_shared<tir::Temp>(totalLen)));

      // // attach array DV
      // auto arrayClass = astManager->java_lang.Arrays;
      // if (arrayClass == nullptr) {
      //   throw std::runtime_error("java.util.Arrays class not found");
      // }
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     // MEM(arr + 4) = DV for arrays
      //     std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
      //         tir::BinOp::OpType::ADD,
      //         std::make_shared<tir::Mem>(
      //             std::make_shared<tir::Temp>(newCharsRef)),
      //         std::make_shared<tir::Const>(4))),
      //     std::make_shared<tir::Temp>(codeGenLabels->getClassLabel(arrayClass),
      //                                 nullptr, true)));
      // std::cout << "String Concatenation: done new chars" << std::endl;
      // // Copy a_chars to new chars
      // // Loop A Condition
      // std::string iA = tir::Temp::generateName(stringConcatPrefix + "i_a");
      // std::string loopA =
      //     tir::Label::generateName(stringConcatPrefix + "loop_copy_a");
      // std::string loopB =
      //     tir::Label::generateName(stringConcatPrefix + "loop_copy_b");
      // std::string condA = tir::Temp::generateName(stringConcatPrefix +
      // "cond_a"); seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(iA), std::make_shared<tir::Const>(0)));
      // seqVec.push_back(std::make_shared<tir::Label>(loopA));
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(condA),
      //     std::make_shared<tir::BinOp>(tir::BinOp::OpType::GEQ,
      //                                  std::make_shared<tir::Temp>(iA),
      //                                  std::make_shared<tir::Temp>(lenA))));
      // seqVec.push_back(std::make_shared<tir::CJump>(
      //     std::make_shared<tir::Temp>(condA), loopB));
      // // Loop A Body
      // std::string srcA = tir::Temp::generateName(stringConcatPrefix +
      // "src_a"); std::string dstA = tir::Temp::generateName(stringConcatPrefix
      // + "dst_a"); seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(srcA),
      //     std::make_shared<tir::BinOp>(
      //         tir::BinOp::OpType::ADD,
      //         std::make_shared<tir::Temp>(charsRefA),
      //         std::make_shared<tir::BinOp>(
      //             tir::BinOp::OpType::ADD, std::make_shared<tir::Const>(8),
      //             std::make_shared<tir::BinOp>(
      //                 tir::BinOp::OpType::MUL,
      //                 std::make_shared<tir::Temp>(iA),
      //                 std::make_shared<tir::Const>(4))))));
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(dstA),
      //     std::make_shared<tir::BinOp>(
      //         tir::BinOp::OpType::ADD,
      //         std::make_shared<tir::Temp>(newCharsRef),
      //         std::make_shared<tir::BinOp>(
      //             tir::BinOp::OpType::ADD, std::make_shared<tir::Const>(8),
      //             std::make_shared<tir::BinOp>(
      //                 tir::BinOp::OpType::MUL,
      //                 std::make_shared<tir::Temp>(iA),
      //                 std::make_shared<tir::Const>(4))))));
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(dstA)),
      //     std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(srcA))));
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(iA),
      //     std::make_shared<tir::BinOp>(tir::BinOp::OpType::ADD,
      //                                  std::make_shared<tir::Temp>(iA),
      //                                  std::make_shared<tir::Const>(1))));
      // seqVec.push_back(
      //     std::make_shared<tir::Jump>(std::make_shared<tir::Name>(loopA)));
      // std::cout << "String Concatenation: done Loop A" << std::endl;
      // // Copy b_chars to new chars
      // // Loop B Condition
      // std::string iB = tir::Temp::generateName(stringConcatPrefix + "i_b");
      // std::string buildString =
      //     tir::Label::generateName(stringConcatPrefix + "build_string");
      // std::string condB = tir::Temp::generateName(stringConcatPrefix +
      // "cond_b"); seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(iB), std::make_shared<tir::Const>(0)));
      // seqVec.push_back(std::make_shared<tir::Label>(loopB));
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(condB),
      //     std::make_shared<tir::BinOp>(tir::BinOp::OpType::GEQ,
      //                                  std::make_shared<tir::Temp>(iB),
      //                                  std::make_shared<tir::Temp>(lenB))));
      // seqVec.push_back(std::make_shared<tir::CJump>(
      //     std::make_shared<tir::Temp>(condB), buildString));
      // // Loop B Body
      // std::string srcB = tir::Temp::generateName(stringConcatPrefix +
      // "src_b"); std::string dstB = tir::Temp::generateName(stringConcatPrefix
      // + "dst_b"); seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(srcB),
      //     std::make_shared<tir::BinOp>(
      //         tir::BinOp::OpType::ADD,
      //         std::make_shared<tir::Temp>(charsRefB),
      //         std::make_shared<tir::BinOp>(
      //             tir::BinOp::OpType::ADD, std::make_shared<tir::Const>(8),
      //             std::make_shared<tir::BinOp>(
      //                 tir::BinOp::OpType::MUL,
      //                 std::make_shared<tir::Temp>(iB),
      //                 std::make_shared<tir::Const>(4))))));
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(dstB),
      //     std::make_shared<tir::BinOp>(
      //         tir::BinOp::OpType::ADD,
      //         std::make_shared<tir::Temp>(newCharsRef),
      //         std::make_shared<tir::BinOp>(
      //             tir::BinOp::OpType::ADD, std::make_shared<tir::Const>(8),
      //             std::make_shared<tir::BinOp>(
      //                 tir::BinOp::OpType::MUL,
      //                 std::make_shared<tir::BinOp>(
      //                     tir::BinOp::OpType::ADD,
      //                     std::make_shared<tir::Temp>(lenA),
      //                     std::make_shared<tir::Temp>(iB)),
      //                 std::make_shared<tir::Const>(4))))));
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(dstB)),
      //     std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(srcB))));
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(iB),
      //     std::make_shared<tir::BinOp>(tir::BinOp::OpType::ADD,
      //                                  std::make_shared<tir::Temp>(iB),
      //                                  std::make_shared<tir::Const>(1))));
      // seqVec.push_back(
      //     std::make_shared<tir::Jump>(std::make_shared<tir::Name>(loopB)));
      // std::cout << "String Concatenation: done Loop B" << std::endl;
      // seqVec.push_back(std::make_shared<tir::Label>(buildString));

      // Finalize building the string
      // std::string stringRefName = tir::Temp::generateName("string_ref");

      // // allocate space for class
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Temp>(stringRefName),
      //     tir::Call::makeMalloc(
      //         std::make_shared<tir::Const>((numFields + 1) * 4))));

      // // first location is DV
      // seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(stringRefName)),
      //     std::make_shared<tir::Temp>(codeGenLabels->getClassLabel(stringClass),
      //                                 nullptr, true)));

      // // initialize all fields
      // int count = 0;
      // for (auto &field : stringDV->fieldVector)
      // {
      //   auto initializer = field->hasInit()
      //                          ? innerExprConverter->evaluateList(
      //                                field->getInitializer()->getExprNodes())
      //                          : std::make_shared<tir::Const>(0);
      //   seqVec.push_back(std::make_shared<tir::Move>(
      //       std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
      //           tir::BinOp::OpType::ADD,
      //           std::make_shared<tir::Temp>(stringRefName),
      //           std::make_shared<tir::Const>((count + 1) * 4))),
      //       initializer));
      //   count++;
      // }
      // // find the zero arg constructor for string
      // std::shared_ptr<parsetree::ast::MethodDecl> constructor = nullptr;
      // for (auto &method : stringClass->getConstructors())
      // {
      //   if (method->getParams().size() == 0)
      //   {
      //     constructor = method;
      //     break;
      //   }
      // }
      // if (constructor == nullptr)
      // {
      //   throw std::runtime_error("No zero arg constructor for String found");
      // }
      // // call the constructor
      // seqVec.push_back(std::make_shared<tir::Exp>(std::make_shared<tir::Call>(
      //     std::make_shared<tir::Name>(codeGenLabels->getMethodLabel(constructor)),
      //     std::make_shared<tir::Temp>(stringRefName),
      //     std::vector<std::shared_ptr<tir::Expr>>{})));

      // std::cout << "String Concatenation: done build new string" <<
      // std::endl; seqVec.push_back(std::make_shared<tir::Move>(
      //     std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
      //         tir::BinOp::OpType::ADD,
      //         std::make_shared<tir::Temp>(stringRefName),
      //         std::make_shared<tir::Const>(charsFieldIndex * 4))),
      //     std::make_shared<tir::Temp>(newCharsRef)));
    }
    /** New implementation, leverage existing concat call on string */
    auto concatMethod = astManager->java_lang.String->getMethod("concat");
    if (!concatMethod) {
      throw std::runtime_error("String object has no concat method");
    }
    std::vector<std::shared_ptr<tir::Expr>> args{rhs};

    // Leverage concat method
    auto methodName = std::make_shared<parsetree::ast::MethodName>(
        concatMethod->getFullName(), source::SourceRange());
    methodName->setResolvedDecl(concatMethod);
    auto methodNameTIR = std::make_shared<tir::TempTIR>(
        methodName, tir::TempTIR::Type::MethodName);
    auto methodTIR = std::make_shared<tir::TempTIR>(
        std::pair<std::shared_ptr<tir::Expr>, std::shared_ptr<tir::Expr>>{
            lhs, methodNameTIR},
        tir::TempTIR::Type::MethodCall);
    std::vector<std::shared_ptr<parsetree::ast::ExprNode>> methodInvocationArgs;
    auto methodInvocation = std::make_shared<parsetree::ast::MethodInvocation>(
        1, methodInvocationArgs);
    auto newString = evalMethodInvocation(methodInvocation, methodTIR, args);

    return newString;
  }
  std::shared_ptr<parsetree::ast::Type> type = nullptr;
  std::shared_ptr<tir::Expr> oldExpr = nullptr;
  if (!op->getLhsType()->isString()) {
    type = op->getLhsType();
    oldExpr = lhs;
  } else {
    type = op->getRhsType();
    oldExpr = rhs;
  }
  /** Old implementation */
  {
    // std::shared_ptr<parsetree::ast::ExprValue> nullLiteral =
    //     std::make_shared<parsetree::ast::Literal>(
    //         std::make_shared<parsetree::Literal>(parsetree::Literal::Type::String,
    //                                              "null"),
    //         std::make_shared<parsetree::ast::BasicType>(
    //             parsetree::ast::BasicType::Type::String));
    // auto nullExpr = mapValue(nullLiteral);
    // std::shared_ptr<parsetree::ast::ClassDecl> classDecl = nullptr;
    // std::shared_ptr<parsetree::ast::MethodDecl> classConstructor = nullptr;
    // if (type->isCharacter()) {
    //   classDecl = astManager->java_lang.Character;
    //   for (auto constructor : classDecl->getConstructors()) {
    //     auto params = constructor->getParams();
    //     if (params.size() == 1 && params[0]->getType()->isCharacter()) {
    //       classConstructor = constructor;
    //       break;
    //     }
    //   }
    // } else if (type->isBoolean()) {
    //   classDecl = astManager->java_lang.Boolean;
    //   for (auto constructor : classDecl->getConstructors()) {
    //     auto params = constructor->getParams();
    //     if (params.size() == 1 && params[0]->getType()->isBoolean()) {
    //       classConstructor = constructor;
    //       break;
    //     }
    //   }
    // } else if (type->isNumeric()) {
    //   classDecl = astManager->java_lang.Integer;
    //   for (auto constructor : classDecl->getConstructors()) {
    //     auto params = constructor->getParams();
    //     if (params.size() == 1 && params[0]->getType()->isNumeric()) {
    //       classConstructor = constructor;
    //       break;
    //     }
    //   }
    // } else if (type->isNull()) {
    //   auto newOp = std::make_shared<parsetree::ast::BinOp>(
    //       parsetree::ast::BinOp::OpType::Add);
    //   if (!op->getLhsType()->isString()) {
    //     newOp->setLhsType(std::make_shared<parsetree::ast::BasicType>(
    //         parsetree::ast::BasicType::Type::String));
    //     newOp->setRhsType(op->getRhsType());
    //     return evalStringConcatenation(newOp, nullExpr, rhs);
    //   }
    //   newOp->setLhsType(op->getLhsType());
    //   newOp->setRhsType(std::make_shared<parsetree::ast::BasicType>(
    //       parsetree::ast::BasicType::Type::String));
    //   return evalStringConcatenation(newOp, lhs, nullExpr);
    // } else {
    //   throw std::runtime_error(
    //       "Performing string concatenation on an invalid type");
    // }
    // // Call evalNewObject
    // // ReferenceType should be the casted class
    // auto referenceType =
    //     std::make_shared<parsetree::ast::ReferenceType>(classDecl);
    // referenceType->setResolvedDecl(static_check::Decl(classDecl));
    // // TypeNode decl should resolve to class constructor taking in its own
    // // primitive type
    // auto typeNode =
    // std::make_shared<parsetree::ast::TypeNode>(referenceType);
    // typeNode->setResolvedDecl(classConstructor);
    // // Create the arguments to make new object call;
    // auto obj =
    // std::make_shared<tir::TempTIR>(typeNode, tir::TempTIR::Type::TypeNode);
    // auto objOp = std::make_shared<parsetree::ast::ClassCreation>(1);
    // auto newObj = evalNewObject(objOp, obj, {oldExpr});
    // // Find the appropriate toString method
    // auto toString = getToStringMethod(classDecl,
    // astManager->java_lang.Object); if (!toString) {
    //   for (auto func : astManager->java_lang.Object->getAllMethods()) {
    //     if (!func->isConstructor() && func->getName() == "toString" &&
    //         func->getParams().size() == 0) {
    //       toString = func;
    //       break;
    //     }
    //   }
    // }
    // // Call toString on the newly created object
    // auto methodName = std::make_shared<parsetree::ast::MethodName>(
    //     toString->getFullName(), source::SourceRange());
    // methodName->setResolvedDecl(toString);
    // auto methodNameTIR = std::make_shared<tir::TempTIR>(
    //     methodName, tir::TempTIR::Type::MethodName);
    // auto methodTIR = std::make_shared<tir::TempTIR>(
    //     std::pair<std::shared_ptr<tir::Expr>, std::shared_ptr<tir::Expr>>{
    //         newObj, methodNameTIR},
    //     tir::TempTIR::Type::MethodCall);
    // std::vector<std::shared_ptr<parsetree::ast::ExprNode>>
    // methodInvocationArgs; auto methodInvocation =
    // std::make_shared<parsetree::ast::MethodInvocation>(
    //     0, methodInvocationArgs);
    // auto newString = evalMethodInvocation(methodInvocation, methodTIR, {});

    // // Perform a check if the result is null
    // std::vector<std::shared_ptr<tir::Stmt>> checkSeqVec;
    // std::string input = tir::Temp::generateName(stringConcatPrefix +
    // "input"); checkSeqVec.push_back(std::make_shared<tir::Move>(
    //     std::make_shared<tir::Temp>(input), newString));
    // std::string stringNotNull =
    //     tir::Label::generateName(stringConcatPrefix + "string_not_null");
    // std::string doneCheck =
    //     tir::Label::generateName(stringConcatPrefix + "done_null_check");
    // checkSeqVec.push_back(std::make_shared<tir::CJump>(
    //     std::make_shared<tir::BinOp>(tir::BinOp::OpType::NEQ,
    //                                  std::make_shared<tir::Temp>(input),
    //                                  std::make_shared<tir::Const>(0)),
    //     stringNotNull));
    // checkSeqVec.push_back(std::make_shared<tir::Move>(
    //     std::make_shared<tir::Temp>(input), nullExpr));
    // checkSeqVec.push_back(std::make_shared<tir::Label>(stringNotNull));
    // auto finalExpr =
    //     std::make_shared<tir::ESeq>(std::make_shared<tir::Seq>(checkSeqVec),
    //                                 std::make_shared<tir::Temp>(input));
  }

  std::shared_ptr<parsetree::ast::MethodDecl> valueOfMethod = nullptr;
  for (auto method : astManager->java_lang.String->getAllMethods()) {
    auto params = method->getParams();
    std::cout << method->getName() << std::endl;
    method->print(std::cout);
    if (method->getName() == "valueOf" && method->isStatic() &&
        params.size() == 1 && *params[0]->getType() == *type) {
      valueOfMethod = method;
      break;
    }
  }
  if (!valueOfMethod) {
    throw std::runtime_error(
        "String object has no corresponding valueOf method");
  }

  // Leverage valueOf method
  auto methodName = std::make_shared<parsetree::ast::MethodName>(
      valueOfMethod->getFullName(), source::SourceRange());
  methodName->setResolvedDecl(valueOfMethod);
  auto methodNameTIR = std::make_shared<tir::TempTIR>(
      methodName, tir::TempTIR::Type::MethodName);
  std::vector<std::shared_ptr<parsetree::ast::ExprNode>> methodInvocationArgs;
  auto methodInvocation = std::make_shared<parsetree::ast::MethodInvocation>(
      1, methodInvocationArgs);
  std::vector<std::shared_ptr<tir::Expr>> args{oldExpr};
  auto newString = evalMethodInvocation(methodInvocation, methodNameTIR, args);

  // Now recurse once to evaluate the string concatenation!
  auto newOp = std::make_shared<parsetree::ast::BinOp>(
      parsetree::ast::BinOp::OpType::Add);
  if (!op->getLhsType()->isString()) {
    newOp->setLhsType(std::make_shared<parsetree::ast::BasicType>(
        parsetree::ast::BasicType::Type::String));
    newOp->setRhsType(op->getRhsType());
    return evalStringConcatenation(newOp, newString, rhs);
  }
  newOp->setLhsType(op->getLhsType());
  newOp->setRhsType(std::make_shared<parsetree::ast::BasicType>(
      parsetree::ast::BasicType::Type::String));
  return evalStringConcatenation(newOp, lhs, newString);
}

std::shared_ptr<tir::Expr>
ExprIRConverter::evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
                           const std::shared_ptr<tir::Expr> lhs,
                           const std::shared_ptr<tir::Expr> rhs) {

  // std::cout << "evalBinOp: " << std::endl;
  // op->print(std::cout);
  if (!lhs || !rhs)
    throw std::runtime_error("BinOp operands are null");

  switch (op->getOp()) {
  // short circuit and
  case parsetree::ast::BinOp::OpType::And: {
    // Move(t_and, lhs)
    auto temp_string = tir::Temp::generateName("t_and");
    auto move1 = std::make_shared<tir::Move>(
        std::make_shared<tir::Temp>(temp_string, nullptr), lhs);

    // CJump(lhs == 0, false, true)
    auto true_name = tir::Label::generateName("sc_true");
    auto false_name = tir::Label::generateName("sc_false");
    auto cjump1 = std::make_shared<tir::CJump>(
        tir::BinOp::makeNegate(
            std::make_shared<tir::Temp>(temp_string, nullptr)),
        false_name, true_name);

    // sc_true:
    auto true_label = std::make_shared<tir::Label>(true_name);

    // Evaluate rhs
    // Move(t_and, rhs)
    auto move2 = std::make_shared<tir::Move>(
        std::make_shared<tir::Temp>(temp_string, nullptr), rhs);

    // sc_false:
    auto false_label = std::make_shared<tir::Label>(false_name);

    // ESEQ({...}, t_and)
    return std::make_shared<tir::ESeq>(
        std::make_shared<tir::Seq>(std::vector<std::shared_ptr<tir::Stmt>>{
            move1, cjump1, true_label, move2, false_label}),
        std::make_shared<tir::Temp>(temp_string, nullptr));
  }

  // short circuit or
  case parsetree::ast::BinOp::OpType::Or: {
    auto temp_string = tir::Temp::generateName("t_or");

    // Move(t_or, lhs)
    auto move1 = std::make_shared<tir::Move>(
        std::make_shared<tir::Temp>(temp_string, nullptr), lhs);

    // CJump(t_or, true, false)
    auto true_name = tir::Label::generateName("sc_true");
    auto false_name = tir::Label::generateName("sc_false");
    auto cjump1 = std::make_shared<tir::CJump>(
        std::make_shared<tir::Temp>(temp_string, nullptr), true_name,
        false_name);

    // sc_false:
    auto false_label = std::make_shared<tir::Label>(false_name);

    // Evaluate rhs
    // Move(t_or, rhs)
    auto move2 = std::make_shared<tir::Move>(
        std::make_shared<tir::Temp>(temp_string, nullptr), rhs);

    // sc_true:
    auto true_label = std::make_shared<tir::Label>(true_name);

    // ESEQ({...}, t_or)
    return std::make_shared<tir::ESeq>(
        std::make_shared<tir::Seq>(std::vector<std::shared_ptr<tir::Stmt>>{
            move1, cjump1, false_label, move2, true_label}),
        std::make_shared<tir::Temp>(temp_string, nullptr));
  }

  // eager and
  case parsetree::ast::BinOp::OpType::BitWiseAnd:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::AND, lhs, rhs);

  // eager or
  case parsetree::ast::BinOp::OpType::BitWiseOr:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::OR, lhs, rhs);

  case parsetree::ast::BinOp::OpType::GreaterThan:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::GT, lhs, rhs);

  case parsetree::ast::BinOp::OpType::GreaterThanOrEqual:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::GEQ, lhs, rhs);

  case parsetree::ast::BinOp::OpType::LessThan:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::LT, lhs, rhs);

  case parsetree::ast::BinOp::OpType::LessThanOrEqual:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::LEQ, lhs, rhs);

  case parsetree::ast::BinOp::OpType::Equal:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::EQ, lhs, rhs);

  case parsetree::ast::BinOp::OpType::NotEqual:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::NEQ, lhs, rhs);

  case parsetree::ast::BinOp::OpType::Plus:
  case parsetree::ast::BinOp::OpType::Add: {
    // std::cout << "Printing Op:" << std::endl;
    // std::cout << op << "" << std::endl;
    // std::cout << op->getLhsType()->isString() << "" << std::endl;
    // std::cout << op->getRhsType()->isString() << "" << std::endl;
    if (op->getLhsType()->isString() || op->getRhsType()->isString()) {
      return evalStringConcatenation(op, lhs, rhs);
    }
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::ADD, lhs, rhs);
  }

  case parsetree::ast::BinOp::OpType::Minus:
  case parsetree::ast::BinOp::OpType::Subtract:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::SUB, lhs, rhs);

  case parsetree::ast::BinOp::OpType::Multiply:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::MUL, lhs, rhs);

  case parsetree::ast::BinOp::OpType::Divide:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::DIV, lhs, rhs);

  case parsetree::ast::BinOp::OpType::Modulo:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::MOD, lhs, rhs);

  // need double check
  case parsetree::ast::BinOp::OpType::InstanceOf: {
    // std::cout << "ExprIRConverter::evalBinOp::InstanceOf" << std::endl;
    if (op->getLhsType() && op->getRhsType()) {
      auto lhsType = op->getLhsType();
      auto it = realTypeMap.find(lhs);
      // std::cout << "from op, lhs: ";
      // lhsType->print(std::cout);
      if (it != realTypeMap.end()) {
        // std::cout << "update lhs type from ";
        // lhsType->print(std::cout);
        // std::cout << " to ";
        // it->second->print(std::cout);
        // std::cout << std::endl;
        lhsType = it->second;
        realTypeMap.erase(it);
      }
      auto rhsType = op->getRhsType();
      // std::cout << "from op, rhs: ";
      // rhsType->print(std::cout);

      // special case: array type
      if (auto lhsArrayType =
              std::dynamic_pointer_cast<parsetree::ast::ArrayType>(lhsType)) {
        // true if rhs is object, cloneable or serializable
        if (auto rhsRefType =
                std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
                    rhsType)) {
          if (!(rhsRefType->isResolved())) {
            throw std::runtime_error("rhs type in instanceof not resolved");
          }
          auto rhsRefTypeAst = rhsRefType->getResolvedDecl().getAstNode();
          if (rhsRefTypeAst == astManager->java_lang.Object ||
              rhsRefTypeAst == astManager->java_lang.Cloneable ||
              rhsRefTypeAst == astManager->java_lang.Serializable) {
            return std::make_shared<tir::Const>(1);
          }
        }

        // true if rhs is array type of B and A is assignable to B
        if (auto rhsArrayType =
                std::dynamic_pointer_cast<parsetree::ast::ArrayType>(rhsType)) {
          auto lhsElemType = lhsArrayType->getElementType();
          auto rhsElemType = rhsArrayType->getElementType();
          auto lhsElemRef =
              std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
                  lhsElemType);
          auto rhsElemRef =
              std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
                  rhsElemType);
          if (!lhsElemRef || !rhsElemRef) {
            throw std::runtime_error(
                "InstanceOf array operands cannot be non-reference types");
          }
          if (!(lhsElemRef->isResolved() && rhsElemRef->isResolved())) {
            throw std::runtime_error(
                "InstanceOf array operands are not resolved");
          }
          auto lhsElemDecl =
              std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
                  lhsElemRef->getResolvedDecl().getAstNode());
          auto rhsElemDecl =
              std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
                  rhsElemRef->getResolvedDecl().getAstNode());
          if (!lhsElemDecl || !rhsElemDecl) {
            throw std::runtime_error(
                "InstanceOf array operands cannot be non-class types");
          }
          if (lhsElemDecl == rhsElemDecl ||
              isSuperClass(rhsElemDecl, lhsElemDecl)) {
            return std::make_shared<tir::Const>(1);
          }
        }
        return std::make_shared<tir::Const>(0);

        // lhs not array type and rhs array type -> false
      } else if (auto rhsArrayType =
                     std::dynamic_pointer_cast<parsetree::ast::ArrayType>(
                         rhsType)) {
        return std::make_shared<tir::Const>(0);
      }

      // lhs and rhs both are reference types
      if (auto lhsRefType =
              std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
                  lhsType)) {
        if (auto rhsRefType =
                std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
                    rhsType)) {
          if (!(lhsRefType->isResolved() && rhsRefType->isResolved())) {
            throw std::runtime_error("InstanceOf operands are not resolved");
          }
          auto lhsAstDecl = lhsRefType->getResolvedDecl().getAstNode();
          auto rhsAstDecl = rhsRefType->getResolvedDecl().getAstNode();
          auto lhsDecl =
              std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(lhsAstDecl);
          auto rhsDecl =
              std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(rhsAstDecl);
          if (!lhsDecl || !rhsDecl) {
            throw std::runtime_error("InstanceOf operands are not classes");
          }
          // std::cout << "checking instanceof with lhs: " << lhsDecl->getName()
          // << ", rhs: " << rhsDecl->getName() << "\n";
          if (lhsDecl == rhsDecl || isSuperClass(rhsDecl, lhsDecl)) {
            // return std::make_shared<tir::Const>(1);
            return std::make_shared<tir::BinOp>(
                tir::BinOp::OpType::NEQ, lhs, std::make_shared<tir::Const>(0));
          }
        }
      }
    }
    return std::make_shared<tir::Const>(0);
  }

  default:
    throw std::runtime_error("Invalid binary operator");
  }
}

std::shared_ptr<tir::Expr>
ExprIRConverter::evalUnOp(std::shared_ptr<parsetree::ast::UnOp> &op,
                          const std::shared_ptr<tir::Expr> rhs) {

  // std::cout << "EvalUnOp\n";
  if (std::dynamic_pointer_cast<tir::Expr>(rhs) == nullptr) {
    throw std::runtime_error("Invalid unary operation, rhs is not an Expr IR");
  }

  switch (op->getOp()) {
  case parsetree::ast::UnOp::OpType::Plus:
    return rhs;

  case parsetree::ast::UnOp::OpType::Minus: {
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::SUB,
                                        std::make_shared<tir::Const>(0), rhs);
  }

  case parsetree::ast::UnOp::OpType::Not:
    return tir::BinOp::makeNegate(rhs);

  default:
    throw std::runtime_error("Invalid unary operator");
  }
}

bool ExprIRConverter::isArrayLength(
    std::shared_ptr<parsetree::ast::FieldDecl> fieldDecl) {
  std::shared_ptr<parsetree::ast::FieldDecl> lengthDecl = nullptr;
  for (auto field : astManager->java_lang.Array->getFields()) {
    if (field->getName() == "length") {
      lengthDecl = field;
    }
  }
  if (!lengthDecl) {
    throw std::runtime_error("Cannot find array length field");
  }
  return fieldDecl == lengthDecl;
}

std::shared_ptr<tir::Expr> ExprIRConverter::evalFieldAccess(
    std::shared_ptr<parsetree::ast::FieldAccess> &op,
    const std::shared_ptr<tir::Expr> lhs,
    const std::shared_ptr<tir::Expr> field) {

  // std::cout << "evalFieldAccess, lhs: \n";
  // lhs->print(std::cout);
  // std::cout << "field: \n";
  // field->print(std::cout);
  // op->print(std::cout);
  if (!(op->getResultType())) {
    throw std::runtime_error(
        "Invalid field access, field type is not resolved");
  }

  auto fieldType = op->getResultType();

  std::shared_ptr<parsetree::ast::FieldDecl> fieldDecl = nullptr;
  std::shared_ptr<parsetree::ast::Type> realType = nullptr;

  if (std::dynamic_pointer_cast<tir::TempTIR>(field)) {

    auto fieldTIR = std::dynamic_pointer_cast<tir::TempTIR>(field);
    if (fieldTIR->type == tir::TempTIR::Type::MethodName) {
      // special case: field is method, defer
      return std::make_shared<tir::TempTIR>(std::make_pair(lhs, field),
                                            tir::TempTIR::Type::MethodCall);
    }
    if (fieldTIR->type != tir::TempTIR::Type::FieldAccess) {
      throw std::runtime_error(
          "Invalid field access, field is not a field access");
    }

    auto fieldMember = std::dynamic_pointer_cast<parsetree::ast::MemberName>(
        fieldTIR->astNode);
    if (!fieldMember) {
      throw std::runtime_error(
          "Invalid field access, field is not a MemberName");
    }

    fieldDecl = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(
        fieldMember->getResolvedDecl());
    if (!fieldDecl) {
      throw std::runtime_error(
          "Invalid field access, field is not a FieldDecl");
    }

    auto it = realTypeMap.find(fieldTIR);
    if (it != realTypeMap.end()) {
      realType = it->second;
      realTypeMap.erase(it);
    }

  } else if (auto memIR = std::dynamic_pointer_cast<tir::Mem>(field)) {

    // this.field
    fieldDecl = memIR->getField();
    if (!fieldDecl) {
      throw std::runtime_error(
          "Invalid field access, field mem is not a FieldDecl");
    }
  } else {
    throw std::runtime_error(
        "Invalid field access, field is not a TempTIR or Mem");
  }

  // Special case: array length field
  if (isArrayLength(fieldDecl)) {
    // Get array
    std::string array_name = tir::Temp::generateName("array");
    auto get_array = std::make_shared<tir::Move>(
        std::make_shared<tir::Temp>(array_name, nullptr), lhs);

    // not null check
    std::string error_name = tir::Label::generateName("error");
    std::string not_null_name = tir::Label::generateName("not_null");
    auto not_null_check = std::make_shared<tir::CJump>(
        // NEQ(array, 0)
        std::make_shared<tir::BinOp>(
            tir::BinOp::OpType::NEQ,
            std::make_shared<tir::Temp>(array_name, nullptr),
            std::make_shared<tir::Const>(0)),
        not_null_name, error_name);

    // exception on null
    auto error_label = std::make_shared<tir::Label>(error_name);
    auto exception_call_stmt =
        std::make_shared<tir::Exp>(tir::Call::makeException());
    auto not_null_label = std::make_shared<tir::Label>(not_null_name);

    return std::make_shared<tir::ESeq>(
        // null check
        std::make_shared<tir::Seq>(std::vector<std::shared_ptr<tir::Stmt>>{
            get_array, not_null_check, error_label, exception_call_stmt,
            not_null_label}),

        // Length stored at MEM[array - 4]
        std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
            tir::BinOp::OpType::SUB,
            std::make_shared<tir::Temp>(array_name, nullptr),
            std::make_shared<tir::Const>(4))));
  }

  // Static field access
  if (auto castedFieldDecl =
          std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(fieldDecl)) {
    if (castedFieldDecl->isStatic()) {
      throw std::runtime_error("Non-static static field access on " +
                               fieldDecl->getName());
    }

    // Instance field access
    auto classDecl = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
        fieldDecl->getParent());

    if (classDecl) {
      int fieldOffset =
          codegen::DispatchVectorBuilder::getDV(classDecl)->getFieldOffset(
              castedFieldDecl);
      auto retExpr = std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
          tir::BinOp::OpType::ADD, lhs,
          std::make_shared<tir::Const>(4 * (fieldOffset + 1))));
      if (realType) {
        realTypeMap[retExpr] = realType;
      }
      return retExpr;
    }
  }

  throw std::runtime_error("field access cannot build IR tree");
}

std::shared_ptr<tir::Expr> ExprIRConverter::evalMethodInvocation(
    std::shared_ptr<parsetree::ast::MethodInvocation> &op,
    const std::shared_ptr<tir::Expr> method,
    const std::vector<std::shared_ptr<tir::Expr>> &args) {

  // std::cout << "evalMethodInvocation\n";
  // op->print(std::cout);
  if (!std::dynamic_pointer_cast<tir::TempTIR>(method)) {
    throw std::runtime_error(
        "Invalid method invocation, method is not a TempTIR");
  }
  auto methodTIR = std::dynamic_pointer_cast<tir::TempTIR>(method);
  if (methodTIR->type != tir::TempTIR::Type::MethodName &&
      methodTIR->type != tir::TempTIR::Type::MethodCall) {
    throw std::runtime_error("Invalid method invocation, method is not a "
                             "method name or method call");
  }

  std::shared_ptr<parsetree::ast::MethodDecl> methodDecl = nullptr;
  if (auto methodName = std::dynamic_pointer_cast<parsetree::ast::MethodName>(
          methodTIR->astNode)) {
    methodDecl = std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(
        methodName->getResolvedDecl());
    if (!methodDecl) {
      throw std::runtime_error(
          "Invalid method invocation, method is not a method decl");
    }
  } else if (methodTIR->type == tir::TempTIR::Type::MethodCall) {
    if (auto methodTemp = std::dynamic_pointer_cast<tir::TempTIR>(
            methodTIR->methodCall.second)) {
      auto methodName = std::dynamic_pointer_cast<parsetree::ast::MethodName>(
          methodTemp->astNode);
      if (!methodName) {
        throw std::runtime_error(
            "Invalid method invocation, method is not a method name");
      }
      methodDecl = std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(
          methodName->getResolvedDecl());
      if (!methodDecl) {
        throw std::runtime_error(
            "Invalid method invocation, method is not a method decl");
      }
    }
  } else {
    throw std::runtime_error("should not happen");
  }

  // auto methodName =
  //     std::dynamic_pointer_cast<parsetree::ast::MethodName>(methodTIR->astNode);
  // if (!methodName) {
  //   throw std::runtime_error(
  //       "Invalid method invocation, method is not a method name");
  // }

  // static method
  if (methodDecl->isStatic()) {
    return tir::Call::makeExpr(
        std::make_shared<tir::Name>(
            codeGenLabels->getStaticMethodLabel(methodDecl)),
        (methodDecl->getModifiers()->isNative())
            ? nullptr
            : std::make_shared<tir::Const>(0),
        args);
  }

  // instance method
  std::string thisName = "";
  std::shared_ptr<tir::Stmt> thisStmt = nullptr;

  // If there is a parent expr, define a stmt to move into Temp
  if (methodTIR->type == tir::TempTIR::Type::MethodCall) {
    auto parent =
        std::dynamic_pointer_cast<tir::Expr>(methodTIR->methodCall.first);
    if (!parent) {
      throw std::runtime_error("Invalid method invocation, parent not exprIR");
    }
    thisName = tir::Temp::generateName("this");
    thisStmt = std::make_shared<tir::Move>(
        std::make_shared<tir::Temp>(thisName), parent);
  } else {
    thisName = "this";
  }

  // if (thisStmt) {
  //   std::cout << "thisStmt not null:" << std::endl;
  //   thisStmt->print(std::cout);
  // }

  auto returnExpr = tir::Call::makeExpr(
      // gets method NameIR
      std::make_shared<tir::Mem>(
          // *this + 4*offset
          std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD,
              std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(thisName)),
              // std::make_shared<tir::Const>(
              std::make_shared<tir::BinOp>(
                  tir::BinOp::OpType::MUL,
                  std::make_shared<tir::Const>(
                      codegen::DispatchVectorBuilder::getAssignment(
                          methodDecl)),
                  std::make_shared<tir::Const>(4))
              // 4 *
              // codegen::DispatchVectorBuilder::getAssignment(methodDecl))
              )),
      (methodDecl->getModifiers()->isNative())
          ? nullptr
          : std::make_shared<tir::Temp>(thisName, nullptr),
      args);

  if (thisStmt) {
    return std::make_shared<tir::ESeq>(thisStmt, returnExpr);
  } else {
    return returnExpr;
  }
}

std::shared_ptr<tir::Expr> ExprIRConverter::evalNewObject(
    std::shared_ptr<parsetree::ast::ClassCreation> &op,
    const std::shared_ptr<tir::Expr> object,
    const std::vector<std::shared_ptr<tir::Expr>> &args) {

  // std::cout << "evalNewObject\n";
  // op->print(std::cout);
  if (!std::dynamic_pointer_cast<tir::TempTIR>(object)) {
    throw std::runtime_error("Invalid new object, object is not a TempTIR");
  }
  auto objectTIR = std::dynamic_pointer_cast<tir::TempTIR>(object);
  if (objectTIR->type != tir::TempTIR::Type::TypeNode) {
    throw std::runtime_error(
        "Invalid new object, object is not a TempTIR TypeNode");
  }

  auto typeNode =
      std::dynamic_pointer_cast<parsetree::ast::TypeNode>(objectTIR->astNode);
  if (!typeNode) {
    throw std::runtime_error("Invalid new object, object is not a TypeNode");
  }
  if (!typeNode->isDeclResolved() || typeNode->getResolvedDecl() == nullptr) {
    throw std::runtime_error("Invalid new object, type is not decl resolved");
  }
  if (!typeNode->isTypeResolved()) {
    throw std::runtime_error("Invalid new object, type is not type resolved");
  }
  auto typeRefType = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
      typeNode->getType());
  if (!typeRefType) {
    throw std::runtime_error("Invalid new object, type is not a ReferenceType");
  }
  if (!typeRefType->isResolved()) {
    throw std::runtime_error(
        "Invalid new object, type is not resolved ReferenceType");
  }

  auto constructor = std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(
      typeNode->getResolvedDecl());
  if (!constructor) {
    throw std::runtime_error("type node is not a method decl");
  }
  if (!constructor->isConstructor()) {
    throw std::runtime_error("type node is not a constructor");
  }

  auto typeClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
      typeRefType->getResolvedDecl().getAstNode());

  // std::cout << "Class creation!" << std::endl;
  // std::cout << "Type Class: " << typeClass->getName() << std::endl;

  auto classDV = codegen::DispatchVectorBuilder::getDV(typeClass);
  // std::cout << "Class DV: " << std::endl;
  classDV->print(std::cout);
  int num_fields = classDV->fieldVector.size();
  std::string obj_ref = tir::Temp::generateName("obj_ref");

  // allocate space for class
  auto move1 = std::make_shared<tir::Move>(
      std::make_shared<tir::Temp>(obj_ref),
      tir::Call::makeMalloc(
          std::make_shared<tir::Const>(4 * (num_fields + 1))));

  // write dispatch vector to first location
  auto move2 = std::make_shared<tir::Move>(
      std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(obj_ref)),
      // location of dispatch vector
      std::make_shared<tir::Temp>(codeGenLabels->getClassLabel(typeClass),
                                  nullptr, true));

  std::vector<std::shared_ptr<tir::Stmt>> seq_vec = {move1, move2};

  // initialize all fields
  if (innerExprConverter == nullptr) {
    throw std::runtime_error(
        "innerExprConverter should not recursively resolve");
  }

  int count = 0;
  for (auto &field : classDV->fieldVector) {
    if (!field)
      continue;
    std::shared_ptr<tir::Expr> initExpr = nullptr;
    if (!field->hasInit()) {
      initExpr = std::make_shared<tir::Const>(0);
    } else {
      auto initializer = field->getInitializer();
      initExpr = innerExprConverter->evaluateList(initializer->getExprNodes());
    }
    if (!initExpr) {
      throw std::runtime_error("Could not resolve field at class creation");
    }

    seq_vec.push_back(std::make_shared<tir::Move>(
        std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
            tir::BinOp::OpType::ADD, std::make_shared<tir::Temp>(obj_ref),
            std::make_shared<tir::Const>(4 * (count + 1)))),
        initExpr));
    count++;
  }

  // TODO: super constructor?

  // call constructor
  seq_vec.push_back(std::make_shared<tir::Exp>(std::make_shared<tir::Call>(
      std::make_shared<tir::Name>(codeGenLabels->getMethodLabel(constructor)),
      std::make_shared<tir::Temp>(obj_ref), args)));

  return std::make_shared<tir::ESeq>(std::make_shared<tir::Seq>(seq_vec),
                                     std::make_shared<tir::Temp>(obj_ref));
}

std::shared_ptr<tir::Expr> ExprIRConverter::evalNewArray(
    std::shared_ptr<parsetree::ast::ArrayCreation> &op,
    const std::shared_ptr<tir::Expr> type,
    const std::shared_ptr<tir::Expr> size) {

  // std::cout << "evalNewArray\n";
  // op->print(std::cout);
  auto array_obj = astManager->java_lang.Arrays;
  if (array_obj == nullptr) {
    throw std::runtime_error("java.lang.Array is not resolved");
  }

  // get inner expression
  std::string size_name = tir::Temp::generateName("size");
  auto move1 =
      std::make_shared<tir::Move>(std::make_shared<tir::Temp>(size_name), size);

  // check not negative
  std::string error_name = tir::Label::generateName("error");
  std::string not_negative_name = tir::Label::generateName("not_negative");
  auto cjump = std::make_shared<tir::CJump>(
      // size >= 0
      std::make_shared<tir::BinOp>(tir::BinOp::OpType::GEQ,
                                   std::make_shared<tir::Temp>(size_name),
                                   std::make_shared<tir::Const>(0)),
      not_negative_name, error_name);

  // error
  auto error_label = std::make_shared<tir::Label>(error_name);
  auto error_call_stmt = std::make_shared<tir::Exp>(tir::Call::makeException());

  auto seq_vec = std::vector<std::shared_ptr<tir::Stmt>>{
      move1, cjump, error_label, error_call_stmt};

  // allocate space
  auto array_name = tir::Temp::generateName("array");
  seq_vec.push_back(std::make_shared<tir::Label>(not_negative_name));
  seq_vec.push_back(std::make_shared<tir::Move>(
      std::make_shared<tir::Temp>(array_name),
      tir::Call::makeMalloc(
          // 4 * size + 8
          std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD,
              std::make_shared<tir::BinOp>(
                  tir::BinOp::OpType::MUL, std::make_shared<tir::Const>(4),
                  std::make_shared<tir::Temp>(size_name)),
              std::make_shared<tir::Const>(8)))));

  // write size
  seq_vec.push_back(std::make_shared<tir::Move>(
      std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(array_name)),
      std::make_shared<tir::Temp>(size_name)));

  // add DV
  seq_vec.push_back(
      // MEM(arr + 4) = DV for arrays
      std::make_shared<tir::Move>(
          std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD, std::make_shared<tir::Temp>(array_name),
              std::make_shared<tir::Const>(4))),
          std::make_shared<tir::Temp>(codeGenLabels->getClassLabel(array_obj),
                                      nullptr, true)));

  // zero initialize array (loop)
  std::string iterator_name = tir::Temp::generateName("iter");
  std::string start_loop = tir::Label::generateName("start_loop");
  std::string exit_loop = tir::Label::generateName("exit_loop");
  std::string dummy_name = tir::Label::generateName("dummy");

  seq_vec.push_back(
      // Move(iterator, 0)
      std::make_shared<tir::Move>(std::make_shared<tir::Temp>(iterator_name),
                                  std::make_shared<tir::Const>(0)));

  seq_vec.push_back(
      // start_loop:
      std::make_shared<tir::Label>(start_loop));

  seq_vec.push_back(
      // MOVE(iterator, iterator + 4)
      std::make_shared<tir::Move>(
          std::make_shared<tir::Temp>(iterator_name),
          std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD,
              std::make_shared<tir::Temp>(iterator_name),
              std::make_shared<tir::Const>(4))));

  seq_vec.push_back(
      // CJump(iterator > 4 * t_size, exit_loop, start_loop)
      std::make_shared<tir::CJump>(
          std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::GT,
              std::make_shared<tir::Temp>(iterator_name),
              std::make_shared<tir::BinOp>(
                  tir::BinOp::OpType::MUL, std::make_shared<tir::Const>(4),
                  std::make_shared<tir::Temp>(size_name))),
          exit_loop, dummy_name));

  seq_vec.push_back(
      // dummy label
      std::make_shared<tir::Label>(dummy_name));

  seq_vec.push_back(
      // MOVE(MEM(iterator + array + 4), 0)
      std::make_shared<tir::Move>(
          std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD,
              // std::make_shared<tir::BinOp>(
              //     tir::BinOp::OpType::ADD,
              //     std::make_shared<tir::Temp>(array_name),
              //     std::make_shared<tir::Temp>(iterator_name)),
              // std::make_shared<tir::Const>(4)
              std::make_shared<tir::Temp>(iterator_name),
              std::make_shared<tir::BinOp>(
                  tir::BinOp::OpType::ADD,
                  std::make_shared<tir::Temp>(array_name),
                  std::make_shared<tir::Const>(4)))),
          std::make_shared<tir::Const>(0)));

  seq_vec.push_back(
      // JUMP(start_loop)
      std::make_shared<tir::Jump>(std::make_shared<tir::Name>(start_loop)));

  seq_vec.push_back(
      // exit_loop:
      std::make_shared<tir::Label>(exit_loop));

  /*
  init iterator
  start_loop:
  iterator = iterator + 4
  CJump to exit if outbounds
  dummy:
  a[iterator] = 0
  Jump to start
  exit_loop:
  */
  return std::make_shared<tir::ESeq>(
      std::make_shared<tir::Seq>(seq_vec),
      // t_array + 4
      std::make_shared<tir::BinOp>(tir::BinOp::OpType::ADD,
                                   std::make_shared<tir::Temp>(array_name),
                                   std::make_shared<tir::Const>(4)));
}

std::shared_ptr<tir::Expr> ExprIRConverter::evalArrayAccess(
    std::shared_ptr<parsetree::ast::ArrayAccess> &op,
    const std::shared_ptr<tir::Expr> array,
    const std::shared_ptr<tir::Expr> index) {

  // std::cout << "evalArrayAccess\n";
  // op->print(std::cout);
  /*
  Temp array
  CJump(NEQ(array, 0), not_null, error)
  error:
  CALL(NAME(__exception))
  not_null:
  Temp index
  CJump(GEQ(index, MEM(array - 4)), error, inbound)
  inbound:
  MEM(array + 4 + 4*index)
  */

  // get array in temp
  std::string array_name = tir::Temp::generateName("array");
  auto get_array = std::make_shared<tir::Move>(
      std::make_shared<tir::Temp>(array_name), array);

  // check not null
  std::string error_name = tir::Label::generateName("error");
  std::string not_null_name = tir::Label::generateName("not_null");
  auto check_not_null = std::make_shared<tir::CJump>(
      // NEQ(array, 0)
      std::make_shared<tir::BinOp>(tir::BinOp::OpType::NEQ,
                                   std::make_shared<tir::Temp>(array_name),
                                   std::make_shared<tir::Const>(0)),
      not_null_name, error_name);

  // throw null exception
  auto error_label = std::make_shared<tir::Label>(error_name);
  auto throw_exception = std::make_shared<tir::Exp>(tir::Call::makeException());

  // not null
  auto not_null_label = std::make_shared<tir::Label>(not_null_name);
  std::string index_name = tir::Temp::generateName("index");
  auto get_index = std::make_shared<tir::Move>(
      std::make_shared<tir::Temp>(index_name), index);

  // check bound
  auto still_in_bounds_name = tir::Label::generateName("still_in_bounds");
  auto check_bound = std::make_shared<tir::CJump>(
      // index < 0 || index >= MEM(array - 4)
      std::make_shared<tir::BinOp>(
          tir::BinOp::OpType::OR,
          // LT(index, 0)
          std::make_shared<tir::BinOp>(tir::BinOp::OpType::LT,
                                       std::make_shared<tir::Temp>(index_name),
                                       std::make_shared<tir::Const>(0)),
          // GEQ(index, MEM(array - 4))
          std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::GEQ, std::make_shared<tir::Temp>(index_name),
              std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
                  tir::BinOp::OpType::SUB,
                  std::make_shared<tir::Temp>(array_name),
                  std::make_shared<tir::Const>(4))))),
      error_name, still_in_bounds_name);

  // still in bound
  auto inbound_label = std::make_shared<tir::Label>(still_in_bounds_name);
  auto inbound_call = std::make_shared<tir::Mem>(
      // array + 4 + (4 * index)
      std::make_shared<tir::BinOp>(
          tir::BinOp::OpType::ADD, std::make_shared<tir::Temp>(array_name),
          std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD, std::make_shared<tir::Const>(4),
              std::make_shared<tir::BinOp>(
                  tir::BinOp::OpType::MUL, std::make_shared<tir::Const>(4),
                  std::make_shared<tir::Temp>(index_name)
                  // std::make_shared<tir::Temp>(index_name),
                  // std::make_shared<tir::Const>(4)
                  ))));

  return std::make_shared<tir::ESeq>(
      std::make_shared<tir::Seq>(std::vector<std::shared_ptr<tir::Stmt>>{
          get_array, check_not_null, error_label, throw_exception,
          not_null_label, get_index, check_bound, inbound_label}),
      inbound_call);
}

std::shared_ptr<tir::Expr>
ExprIRConverter::evalCast(std::shared_ptr<parsetree::ast::Cast> &op,
                          const std::shared_ptr<tir::Expr> type,
                          const std::shared_ptr<tir::Expr> value) {

  // std::cout << "evalCast\n";
  // op->print(std::cout);
  if (!std::dynamic_pointer_cast<tir::TempTIR>(type)) {
    throw std::runtime_error("Invalid cast, type is not a TempTIR");
  }
  auto typeTempIR = std::dynamic_pointer_cast<tir::TempTIR>(type);
  if (typeTempIR->type != tir::TempTIR::Type::TypeNode) {
    throw std::runtime_error("Invalid cast, type is not a TempTIR TypeNode");
  }
  auto typeNode =
      std::dynamic_pointer_cast<parsetree::ast::TypeNode>(typeTempIR->astNode);
  if (!typeNode) {
    throw std::runtime_error("Invalid cast, type is not a TypeNode");
  }
  // TODO: switch to old version because new version does not work, eg.
  // Boolean max value becomes BinOp 127 && -1

  // auto resultType = op->getResultType();
  // if (!resultType) {
  //   throw std::runtime_error("Invalid cast, result type is not resolved");
  // }
  // auto basicType =
  //     std::dynamic_pointer_cast<parsetree::ast::BasicType>(op->getResultType());
  // if (basicType && basicType->isNumeric()) {
  //   const int32_t int_cast = 0xFFFFFFFF;
  //   const int16_t short_cast = 0xFFFF;
  //   const uint16_t char_cast = 0xFFFF;
  //   const int8_t byte_cast = 0xFF;
  //   switch (basicType->getType()) {
  //   case parsetree::ast::BasicType::Type::Int:
  //     return std::make_shared<tir::BinOp>(
  //         tir::BinOp::AND, value, std::make_shared<tir::Const>(int_cast));
  //   case parsetree::ast::BasicType::Type::Short:
  //     return std::make_shared<tir::BinOp>(
  //         tir::BinOp::AND, value, std::make_shared<tir::Const>(short_cast));
  //   case parsetree::ast::BasicType::Type::Byte:
  //     return std::make_shared<tir::Const>(byte_cast & (int8_t)value);
  //     // return std::make_shared<tir::BinOp>(
  //     //     tir::BinOp::AND, value,
  //     std::make_shared<tir::Const>(byte_cast));
  //   case parsetree::ast::BasicType::Type::Char:
  //     return std::make_shared<tir::BinOp>(
  //         tir::BinOp::AND, value, std::make_shared<tir::Const>(char_cast));
  //   default:
  //     throw std::runtime_error("Invalid cast from integer to non numeric
  //     type");
  //   }
  // } else if (op->hasRhsLiteral()) {
  //   // if rhs is literal check promotion or narrowing
  //   auto literal = op->getRhsLiteral();
  //   // cast result type is basic type
  //   if (basicType) {
  //     switch (literal->getLiteralType()) {

  //       // cast from int to numeric type
  //     case parsetree::ast::Literal::Type::Integer:
  //       throw std::runtime_error("Attempted to specifically cast int
  //       literal.");

  //       // cast from char to numeric type
  //     case parsetree::ast::Literal::Type::Character:
  //       throw std::runtime_error(
  //           "Attempted to specifically cast char literal.");
  // if rhs is literal check promotion or narrowing
  if (op->hasRhsLiteral()) {
    auto literal = op->getRhsLiteral();
    auto resultType = op->getResultType();
    if (!resultType) {
      throw std::runtime_error("Invalid cast, result type is not resolved");
    }

    const int16_t short_cast = 0xFFFF;
    const uint16_t char_cast = 0xFFFF;
    const int8_t byte_cast = 0xFF;

    // cast result type is basic type
    if (auto basicType =
            std::dynamic_pointer_cast<parsetree::ast::BasicType>(resultType)) {
      switch (literal->getLiteralType()) {

        // cast from int to numeric type
      case parsetree::ast::Literal::Type::Integer: {
        if (!(basicType->isNumeric()))
          throw std::runtime_error(
              "Invalid cast from integer to non numeric type");
        auto literal_val = literal->getAsInt();
        switch (basicType->getType()) {
        case parsetree::ast::BasicType::Type::Int:
          return std::make_shared<tir::Const>(literal_val);
        case parsetree::ast::BasicType::Type::Short:
          return std::make_shared<tir::Const>((int16_t)literal_val &
                                              short_cast);
        case parsetree::ast::BasicType::Type::Byte:
          return std::make_shared<tir::Const>((int8_t)literal_val & byte_cast);
        case parsetree::ast::BasicType::Type::Char:
          return std::make_shared<tir::Const>((uint16_t)literal_val &
                                              char_cast);
        default:
          throw std::runtime_error(
              "Invalid cast from integer to non numeric type");
        }
      }

      // cast from char to numeric type
      case parsetree::ast::Literal::Type::Character: {
        if (!(basicType->isNumeric()))
          throw std::runtime_error(
              "Invalid cast from char to non numeric type");
        auto literal_val = literal->getAsInt();
        switch (basicType->getType()) {
        case parsetree::ast::BasicType::Type::Short:
          return std::make_shared<tir::Const>((int16_t)literal_val &
                                              short_cast);
        case parsetree::ast::BasicType::Type::Byte:
          return std::make_shared<tir::Const>((int8_t)literal_val & byte_cast);

        case parsetree::ast::BasicType::Type::Int:
        case parsetree::ast::BasicType::Type::Char:
          return std::make_shared<tir::Const>(literal_val);
        default:
          throw std::runtime_error(
              "Invalid cast from char to non numeric type");
        }
      }

      // cast from string to basic type, error!
      case parsetree::ast::Literal::Type::String:
        throw std::runtime_error("Invalid cast from string to basic type");

      // if source bool, we can only cast to bool
      case parsetree::ast::Literal::Type::Boolean: {
        if (!(basicType->isBoolean()))
          throw std::runtime_error(
              "Invalid cast from boolean to non boolean type");
        auto literal_val = literal->getAsInt();
        if (literal_val != 0 && literal_val != 1) {
          throw std::runtime_error(
              "Invalid cast with wrong boolean literal value");
        }
        return std::make_shared<tir::Const>(literal_val);
      }

      // if source null, we can only cast to null
      case parsetree::ast::Literal::Type::Null: {
        if (!(basicType->isNull()))
          throw std::runtime_error("Invalid cast from null to non null type");
        return std::make_shared<tir::Const>(0);
      }
      }
    }
    // cast result type is not basic type and src type is string
    else if (literal->getLiteralType() ==
             parsetree::ast::Literal::Type::String) {
      auto typeRef = std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(
          typeNode->getType());
      if (!typeRef) {
        throw std::runtime_error(
            "Invalid cast from string to non reference type");
      }
      if (!typeRef->isResolved()) {
        throw std::runtime_error("Invalid cast to non resolved reference type");
      }
      auto resultClass = std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
          typeRef->getResolvedDecl().getAstNode());
      if (!resultClass) {
        throw std::runtime_error(
            "Invalid cast to reference type not resolved to class");
      }

      auto sourceClass = astManager->java_lang.String;

      if (sourceClass == resultClass) {
        // do nothing
        return value;
      } else if (isSuperClass(resultClass, sourceClass)) {
        // upcasting

        // create cast result
        std::string cast_result = tir::Temp::generateName("cast_result");
        auto create_cast_result = std::make_shared<tir::Move>(
            std::make_shared<tir::Temp>(cast_result), value);

        // change the DV
        auto add_DV = std::make_shared<tir::Move>(
            std::make_shared<tir::Mem>(
                std::make_shared<tir::Temp>(cast_result)),
            std::make_shared<tir::Temp>(
                codeGenLabels->getClassLabel(resultClass), nullptr, true));

        return std::make_shared<tir::ESeq>(
            std::make_shared<tir::Seq>(std::vector<std::shared_ptr<tir::Stmt>>{
                create_cast_result, add_DV}),
            std::make_shared<tir::Temp>(cast_result));
      }
    }
    // target not basic type and src is null
    else if (literal->getLiteralType() == parsetree::ast::Literal::Type::Null) {
      return std::make_shared<tir::Const>(0);
    }

    throw std::runtime_error("Invalid cast");
  }

  // TODO: only handled casting from basic type

  return value;
}

std::shared_ptr<tir::Expr>
ExprIRConverter::evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
                                const std::shared_ptr<tir::Expr> lhs,
                                const std::shared_ptr<tir::Expr> rhs) {

  // std::cout << "evalAssignment\n";
  // op->print(std::cout);
  if (std::dynamic_pointer_cast<tir::TempTIR>(lhs) ||
      std::dynamic_pointer_cast<tir::TempTIR>(rhs)) {
    throw std::runtime_error(
        "Invalid assignment, lhs or rhs should not be TempTIR");
  }

  std::vector<std::shared_ptr<tir::Stmt>> seq_vec;
  std::shared_ptr<tir::Expr> dest = lhs;

  // lhs either Temp or Mem, and handle ESeq if needed
  if (auto lhsESeq = std::dynamic_pointer_cast<tir::ESeq>(lhs)) {

    // replace lhs with ESeq expr
    if (auto lhsSeq = std::dynamic_pointer_cast<tir::Seq>(lhsESeq->getStmt())) {
      for (auto stmt : lhsSeq->getStmts()) {
        seq_vec.push_back(stmt);
      }
    } else {
      throw std::runtime_error("Invalid assignment, ESeq not composed of Seq");
    }
    dest = lhsESeq->getExpr();

  } else if (!(std::dynamic_pointer_cast<tir::Temp>(lhs) ||
               std::dynamic_pointer_cast<tir::Mem>(lhs))) {
    throw std::runtime_error(
        "Invalid assignment, lhs should be Temp or Mem or ESeq");
  }

  auto assign_src = tir::Temp::generateName("assign_src");
  seq_vec.push_back(std::make_shared<tir::Move>(
      std::make_shared<tir::Temp>(assign_src), rhs));

  seq_vec.push_back(std::make_shared<tir::Move>(
      dest, std::make_shared<tir::Temp>(assign_src)));

  return std::make_shared<tir::ESeq>(std::make_shared<tir::Seq>(seq_vec),
                                     std::make_shared<tir::Temp>(assign_src));
}

} // namespace codegen
