#include "codeGen/exprIRConverter.hpp"

namespace codegen {

std::shared_ptr<tir::Expr>
mapValue(std::shared_ptr<parsetree::ast::ExprValue> &value) {
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
    return std::make_shared<tir::TempTIR>(methodName,
                                          tir::TempTIR::Type::MethodName);
  } else if (auto typeNode =
                 std::dynamic_pointer_cast<parsetree::ast::TypeNode>(value)) {
    // TODO: This is a hack, delay the eval to cast, class creation, array *
    return std::make_shared<tir::TempTIR>(typeNode,
                                          tir::TempTIR::Type::TypeNode);
  }

  // left case Literal and MemberName

  if (auto literal =
          std::dynamic_pointer_cast<parsetree::ast::Literal>(value)) {
    // int64_t, bool, char, std::string, std::nullptr_t
    if (value->getLiteralType() == parsetree::ast::Literal::Type::Integer) {
      int64_t val = literal->getAsInt();
      return std::make_shared<tir::Const>(val);
    } else if (value->getLiteralType() ==
               parsetree::ast::Literal::Type::Boolean) {
      int64_t val = literal->getAsInt();
      if (val != 0 && val != 1)
        throw std::runtime_error("Invalid boolean literal in codegen");
      return std::make_shared<tir::Const>(val);
    } else if (value->getLiteralType() ==
               parsetree::ast::Literal::Type::Character) {
      int64_t val = literal->getAsInt();
      if (val < 0 || val > 255)
        throw std::runtime_error("Invalid character literal in codegen");
      return std::make_shared<tir::Const>(val);
    } else if (value->getLiteralType() ==
               parsetree::ast::Literal::Type::String) {

      // TODO!
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
      auto val = value->getAsString();

      // constants
      auto zero = std::make_shared<tir::Const>(0);
      auto four = std::make_shared<tir::Const>(4);
      auto eight = std::make_shared<tir::Const>(8);

      // temps
      auto t1 = std::make_shared<tir::Temp>("t1");
      auto t2 = std::make_shared<tir::Temp>("t2");

      // t1 = malloc(12)
      auto mallocString =
          tir::Call::makeMalloc(std::make_shared<tir::Const>(12));
      auto moveT1 = std::make_shared<tir::Move>(t1, mallocString);

      // Mem[t1] = @DV_String
      auto dvString =
          std::make_shared<tir::Name>("@DV_String", /* isGlobal */ true);
      auto memT1 = std::make_shared<tir::Mem>(t1);
      auto moveDvString = std::make_shared<tir::Move>(memT1, dvString);

      // Mem[t1 + 4] = 0
      auto t1Plus4 = std::make_shared<tir::BinOp>(ADD, t1, four);
      auto memT1Plus4 = std::make_shared<tir::Mem>(t1Plus4);
      auto moveMemT1Plus4 = std::make_shared<tir::Move>(memT1Plus4, zero);

      // Mem[t1 + 8] = 0
      auto t1Plus8 = std::make_shared<tir::BinOp>(ADD, t1, eight);
      auto memT1Plus8 = std::make_shared<tir::Mem>(t1Plus8);
      auto moveMemT1Plus8 = std::make_shared<tir::Move>(memT1Plus8, zero);

      // CALL @String::<init>(), this = t1
      auto init =
          std::make_shared<tir::Name>("@String::<init>", /* isGlobal */ true);
      std::vector<std::shared_ptr<tir::Expr>> args = {t1};
      auto call = std::make_shared<tir::Call>(init, args);
      auto callStmt = std::make_shared<tir::Exp>(call);

      // t2 = t1 + 8
      auto moveT2 = std::make_shared<tir::Move>(t2, t1Plus8);

      // allocate char array
      // Mem[t2] = malloc(28)
      auto mallocCharArray = tir::Call::makeMalloc(
          std::make_shared<tir::Const>(8 + 4 * val.size()));
      auto memT2 = std::make_shared<tir::Mem>(t2);
      auto moveMemT2 = std::make_shared<tir::Move>(memT2, mallocCharArray);

      // char array length
      // Mem[Mem[t2]]     = 5
      auto memMemT2 = std::make_shared<tir::Mem>(memT2);
      auto moveLength = std::make_shared<tir::Move>(
          memMemT2, std::make_shared<tir::Const>(val.size()));

      // Mem[Mem[t2] + 4] = @DV_Array
      auto dvArray =
          std::make_shared<tir::Name>("@DV_Array", /* isGlobal */ true);
      auto memT2Plus4 =
          std::make_shared<tir::BinOp>(tir::BinOp::OpType::ADD, memT2, four);
      auto moveDvArray = std::make_shared<tir::Move>(memT2Plus4, dvArray);

      // individual characters, for example:
      // Mem[Mem[t2] + 8]  = 104  // 'h'
      // Mem[Mem[t2] + 12] = 101  // 'e'
      // Mem[Mem[t2] + 16] = 108  // 'l'
      // Mem[Mem[t2] + 20] = 108  // 'l'
      // Mem[Mem[t2] + 24] = 111  // 'o'
      std::vector<std::shared_ptr<tir::Stmt>> moveChars;
      auto addr = memT2Plus4;
      for (const char c : val) {
        addr =
            std::make_shared<tir::BinOp>(tir::BinOp::OpType::ADD, addr, four);
        auto memAddr = std::make_shared<tir::Mem>(addr);
        auto moveChar =
            std::make_shared<tir::Move>(addr, std::make_shared<tir::Const>(c));
        moveChars.push_back(moveChar);
      }

      std::vector<std::shared_ptr<tir::Stmt>> stmts = {
          moveT1, moveDvString, moveMemT1Plus4, moveMemT1Plus8, callStmt,
          moveT2, moveMemT2,    moveLength,     moveDvArray};

      stmts.insert(stmts.end(), moveChars.begin(), moveChars.end());

      auto seq = std::make_shared<tir::Seq>(stmts);

      return std::maked_shared<tir::ESeq>(seq, t1);

    } else if (value->getLiteralType() == parsetree::ast::Literal::Type::Null) {
      return std::make_shared<tir::Const>(0);
    } else {
      throw std::runtime_error("Invalid literal in codegen");
    }
  }

  if (auto memberName =
          std::dynamic_pointer_cast<parsetree::ast::MemberName>(value)) {

    // Todo: If array length, special case handled in field access

    // static field
    if (auto fieldDecl = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(
            memberName->getResolvedDecl())) {
      if (fieldDecl->isStatic()) {
        return std::make_shared<tir::Temp>(fieldDecl->getName(), fieldDecl);
      }
    } else if (auto varDecl =
                   std::dynamic_pointer_cast<parsetree::ast::VarDecl>(
                       memberName->getResolvedDecl())) {
      // parameter and local variable
      return std::make_shared<tir::Temp>(varDecl->getName(), varDecl);
    }

    // instance field
    if (auto fieldDecl = std::dynamic_pointer_cast<parsetree::ast::FieldDecl>(
            memberName->getResolvedDecl())) {
      // local (accessed by this or by itself)
      if (memberName->isAccessedByThis() || !(memberName->isNotAsBase())) {
        int offset = currentClass->getFieldOffset(fieldDecl);
        if (offset == -1)
          throw std::runtime_error("cannot find field " + fieldDecl->getName() +
                                   " in current class");
        return std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
            tir::BinOp::Type::Add,
            std::make_shared<tir::Temp>("this", currentClass),
            std::make_shared<tir::Const>(4 * (offset + 1)) // +1 for header
            ));
      }
      // obj.field
      // left for field access to handle
      return std::make_shared<tir::TempTIR>(memberName,
                                            tir::TempTIR::Type::FieldAccess);
    }
  }
  throw std::runtime_error("Invalid value in mapValue in codegen");
}

std::shared_ptr<tir::Expr>
ExprIRConverter::evalBinOp(std::shared_ptr<parsetree::ast::BinOp> &op,
                           const std::shared_ptr<tir::Expr> lhs,
                           const std::shared_ptr<tir::Expr> rhs) {

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
    auto true_name = LabelIR::generateName("sc_true");
    auto false_name = LabelIR::generateName("sc_false");
    auto cjump1 = std::make_shared<tir::CJump>(
        BinOp::makeNegate(std::make_shared<tir::Temp>(temp_string, nullptr)),
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
        std::make_shared<tir::Seq>(
            {move1, cjump1, true_label, move2, false_label}),
        std::make_shared<tir::Temp>(temp_string, nullptr));
  }

  // short circuit or
  case parsetree::ast::BinOp::OpType::Or: {
    auto temp_string = tir::Temp::generateName("t_or");

    // Move(t_or, lhs)
    auto move1 = std::make_shared<tir::Move>(
        std::make_shared<tir::Temp>(temp_string, nullptr), lhs);

    // CJump(t_or, true, false)
    auto true_name = LabelIR::generateName("sc_true");
    auto false_name = LabelIR::generateName("sc_false");
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
        std::make_shared<tir::Seq>(
            {move1, cjump1, false_label, move2, true_label}),
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
  case parsetree::ast::BinOp::OpType::Add:
    return std::make_shared<tir::BinOp>(tir::BinOp::OpType::ADD, lhs, rhs);

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
    if (op->getResultType() && op->getResultType()->isBoolean()) {
      return std::make_shared<tir::Const>(1);
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

std::shared_ptr<tir::Expr> ExprIRConverter::evalFieldAccess(
    std::shared_ptr<parsetree::ast::FieldAccess> &op,
    const std::shared_ptr<tir::Expr> lhs,
    const std::shared_ptr<tir::Expr> field) {

  if (!std::dynamic_pointer_cast<tir::TempTIR>(field)) {
    throw std::runtime_error("Invalid field access, field is not a TempTIR");
  }
  auto fieldTIR = std::dynamic_pointer_cast<tir::TempTIR>(field);
  if (fieldTIR->type != tir::TempTIR::Type::FieldAccess) {
    throw std::runtime_error(
        "Invalid field access, field is not a field access");
  }

  auto fieldDecl =
      std::dynamic_pointer_cast<parsetree::ast::VarDecl>(fieldTIR->astNode);
  if (!fieldDecl) {
    throw std::runtime_error("Invalid field access, field is not a VarDecl");
  }

  // Special case: array length field
  auto lengthDecl = nullptr;
  for (auto field : astManager->java_lang.Array->getFields()) {
    if (field->name() == "length") {
      lengthDecl = field;
    }
  }
  if (fieldDecl == lengthDecl) {
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
        std::make_shared<tir::Seq>({get_array, not_null_check, error_label,
                                    exception_call_stmt, not_null_label}),

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
    if (std::dynamic_pointer_cast<parsetree::ast::ClassDecl>(
            castedFieldDecl->getResolvedDecl()->getAstNode())) {
      return std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
          tir::BinOp::OpType::ADD, lhs,
          std::make_shared<tir::Const>(
              4 * (currentClass->getFieldOffset(castedFieldDecl) + 1))));
    }
  }

  throw std::runtime_error("field access cannot build IR tree");
}

std::shared_ptr<tir::Expr> ExprIRConverter::evalMethodInvocation(
    std::shared_ptr<parsetree::ast::MethodInvocation> &op,
    const std::shared_ptr<tir::Expr> method,
    const std::vector<std::shared_ptr<tir::Expr>> &args) {
  if (!std::dynamic_pointer_cast<tir::TempTIR>(method)) {
    throw std::runtime_error(
        "Invalid method invocation, method is not a TempTIR");
  }
  auto methodTIR = std::dynamic_pointer_cast<tir::TempTIR>(method);
  if (methodTIR->type != tir::TempTIR::Type::MethodName) {
    throw std::runtime_error(
        "Invalid method invocation, method is not a method name");
  }

  auto methodDecl =
      std::dynamic_pointer_cast<parsetree::ast::MethodDecl>(methodTIR->astNode);
  if (!methodDecl) {
    throw std::runtime_error(
        "Invalid method invocation, method is not a method decl");
  }

  // static method
  if (methodDecl->isStatic()) {
    return tir::Call::makeExpr(
        std::make_shared<tir::Name>(
            Constants::uniqueStaticMethodLabel(methodDecl)),
        (methodDecl->getModifiers()->isNative())
            ? nullptr
            : std::make_shared<tir::Const>(0),
        args);
  }

  // instance method
  // TODO: If there is a parent expr, define a stmt to move into Temp?

  // return expr
  return tir::Call::makeExpr(
      // gets method NameIR
      std::make_shared<tir::Mem>(
          // *this + 4*offset
          std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD,
              std::make_shared<tir::Temp>("this", nullptr),
              std::make_shared<tir::Const>(
                  4 * (currentClass->getMethodOffset(methodDecl) + 1)))),
      (methodDecl->getModifiers()->isNative())
          ? nullptr
          : std::make_shared<tir::Temp>("this", nullptr),
      args, );
}

std::shared_ptr<tir::Expr> ExprIRConverter::evalNewObject(
    std::shared_ptr<parsetree::ast::ClassCreation> &op,
    const std::shared_ptr<tir::Expr> object,
    const std::vector<std::shared_ptr<tir::Expr>> &args) {
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
  if (!type->isDeclResolved() || type->getResolvedDecl() == nullptr) {
    throw std::runtime_error("Invalid new object, type is not decl resolved");
  }
  if (!type->isTypeResolved()) {
    throw std::runtime_error("Invalid new object, type is not type resolved");
  }
  auto typeRefType =
      std::dynamic_pointer_cast<parsetree::ast::ReferenceType>(type->getType());
  if (!typeRefType) {
    throw std::runtime_error("Invalid new object, type is not a ReferenceType");
  }
  if (!typeRefType->isResolved()) {
    throw std::runtime_error(
        "Invalid new object, type is not resolved ReferenceType");
  }

  auto constructor = type->getResolvedDecl();
  auto typeClass = std::make_shared<parsetree::ast::ClassDecl>(
      typeRefType->getResolvedDecl()->getAstNode());

  int num_fields = typeClass->getFields()->size();
  std::string obj_ref = tir::Temp::generateName("obj_ref");

  // allocate space for class
  auto move1 = std::make_shared<tir::Move>(
      std::make_shared<tir::Temp>(obj_ref),
      tir::Call::makeMalloc(
          std::make_shared<tir::Const>(4 * (num_fields + 1)), ));

  // write dispatch vector to first location
  auto move2 = std::make_shared<tir::Move>(
      std::make_shared<tir::Mem>(std::make_shared<tir::Temp>(obj_ref)),
      // location of dispatch vector
      std::make_shared<tir::Temp>(Constants::uniqueClassLabel(typeClass),
                                  nullptr, true));

  std::vector<std::shared_ptr<tir::Stmt>> seq_vec = {move1, move2};

  // initialize all fields
  int count = 0;
  for (auto &field : typeClass->getFields()) {
    seq_vec.push_back(std::make_shared<tir::Move>(
        std::make_shared<tir::Mem>(std::make_shared<tir::BinOp>(
            tir::BinOp::OpType::ADD, std::make_shared<tir::Temp>(obj_ref),
            std::make_shared<tir::Const>(4 * (count + 1)))),
        std::make_shared<tir::Const>(0))) count++;
  }

  // TODO: super constructor?

  // call constructor
  seq_vec.push_back(std::make_shared<tir::Exp>(std::make_shared<tir::Call>(
      std::make_shared<tir::Name>(Constants::uniqueMethodLabel(constructor)),
      std::make_shared<tir::Temp>(obj_ref), args)));

  return std::make_shared<tir::ESeq>(std::make_shared<tir::Seq>(seq_vec),
                                     std::make_shared<tir::Temp>(obj_ref));
}

std::shared_ptr<tir::Expr> ExprIRConverter::evalNewArray(
    std::shared_ptr<parsetree::ast::ArrayCreation> &op,
    const std::shared_ptr<tir::Expr> type,
    const std::shared_ptr<tir::Expr> size) {
  auto array_obj = astManager->java_lang.Array;

  // get inner expression
  std::string size_name = tir::Temp::generateName("size");
  auto move1 =
      std::make_shared<tir::Move>(std::make_shared<tir::Temp>(size_name), size);

  // check not negative
  std::string error_name = LabelIR::generateName("error");
  std::string not_negative_name = LabelIR::generateName("not_negative");
  auto cjump = std::make_shared<tir::CJump>(
      // size >= 0
      std::make_shared<tir::BinOp>(tir::BinOp::OpType::GTE,
                                   std::make_shared<tir::Temp>(size_name),
                                   std::make_shared<tir::Const>(0)),
      not_negative_name, error_name);

  // error
  auto error_label = std::make_shared<tir::Label>(error_name);
  auto error_call_stmt = std::make_shared<tir::Exp>(tir::Call::makeException());

  std::vector<std::shared_ptr<tir::Stmt>> seq_vec = {move1, cjump, error_label,
                                                     error_call_stmt};

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
          std::make_shared<tir::Temp>(Constants::uniqueClassLabel(array_obj),
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
              std::make_shared<tir::BinOp>(
                  tir::BinOp::OpType::ADD,
                  std::make_shared<tir::Temp>(array_name),
                  std::make_shared<tir::Temp>(iterator_name)),
              std::make_shared<tir::Const>(4))),
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
      std::make_shared<tir::Temp>(array_name), array)

      // check not null
      std::string error_name = tir::Temp::generateName("error");
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
      std::make_shared<tir::Temp>(index_name), index)

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
  auto inbound_call = std::make_shared<tir::Exp>(
      // array + 4 + (4 * index)
      std::make_shared<tir::BinOp>(
          tir::BinOp::OpType::ADD, std::make_shared<tir::Temp>(array_name),
          std::make_shared<tir::BinOp>(std::make_shared<tir::BinOp>(
              tir::BinOp::OpType::ADD, std::make_shared<tir::Const>(4),
              std::make_shared<tir::BinOp>(
                  tir::BinOp::OpType::MUL,
                  std::make_shared<tir::Temp>(index_name),
                  std::make_shared<tir::Const>(4))))));

  return std::make_shared<tir::ESeq>(
      std::make_shared<tir::Seq>({get_array, check_not_null, error_label,
                                  throw_exception, not_null_label, get_index,
                                  check_bound, inbound_label}),
      inbound_call);
}

std::shared_ptr<tir::Expr>
ExprIRConverter::evalCast(std::shared_ptr<parsetree::ast::Cast> &op,
                          const std::shared_ptr<tir::Expr> type,
                          const std::shared_ptr<tir::Expr> value) {
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

  // TODO:
  // if rhs is literal check promotion or narrowing

  return value;
}

std::shared_ptr<tir::Expr>
ExprIRConverter::evalAssignment(std::shared_ptr<parsetree::ast::Assignment> &op,
                                const std::shared_ptr<tir::Expr> lhs,
                                const std::shared_ptr<tir::Expr> rhs) {}

} // namespace codegen
