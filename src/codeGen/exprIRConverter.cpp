#include "codeGen/exprIRConverter.hpp"

namespace codegen {

std::shared_ptr<tir::Node>
mapValue(std::shared_ptr<parsetree::ast::ExprValue> &value) {
  /*
  MemberName, MethodName
  TypeNode
  ThisNode done
  Literal
  */
  if (auto thisNode =
          std::dynamic_pointer_cast<parsetree::ast::ThisNode>(value)) {
    return std::make_shared<tir::Temp>("this");
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

    } else if (value->getLiteralType() == parsetree::ast::Literal::Type::Null) {
      return std::make_shared<tir::Const>(0);
    } else {
      throw std::runtime_error("Invalid literal in codegen");
    }
  }
}

} // namespace codegen
