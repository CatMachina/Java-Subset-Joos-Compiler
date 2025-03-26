#pragma once

#include "ast/ast.hpp"
#include "tir/TIR.hpp"

namespace codeGen {

class CodeGenLabels {
public:
  const std::string kGlobalPrefix = "_##";
  const std::string kAbstractArgPrefix = "_ARG";
  const std::string kAbstractReturn = "_RET";

  std::string
  getMethodLabel(std::shared_ptr<parsetree::ast::MethodDecl> method) {
    if (method->getModifiers()->isNative()) {
      return "NATIVE" + method->getFullName();
    }
    return getUniqueLabel(method, method->getFullName(), "_METHOD",
                          method_id_counter_, method_labels_);
  }

  std::string
  getStaticMethodLabel(std::shared_ptr<parsetree::ast::MethodDecl> method) {
    if (method->ast_reference->hasModifier(Modifier::NATIVE)) {
      return "NATIVE" + method->getFullName();
    }
    return getUniqueLabel(method, method->getFullName(), "_STATIC_METHOD",
                          method_id_counter_, method_labels_);
  }

  std::string getFieldLabel(std::shared_ptr<parsetree::ast::FieldDecl> field) {
    return getUniqueLabel(field, field->getFullName(), "_FIELD",
                          field_id_counter_, field_labels_);
  }

  std::string
  getStaticFieldLabel(std::shared_ptr<parsetree::ast::FieldDecl> field) {
    return getUniqueLabel(field, field->getFullName(), "_STATIC_FIELD",
                          field_id_counter_, field_labels_);
  }

  std::string
  getLocalVariableLabel(std::shared_ptr<parsetree::ast::VarDecl> variable) {
    return getUniqueLabel(variable, variable->getFullName(), "_LOCAL_VARIABLE",
                          local_var_id_counter_, local_var_labels_);
  }

  std::string
  getParameterLabel(std::shared_ptr<parsetree::ast::VarDecl> parameter) {
    return getUniqueLabel(parameter, parameter->getFullName(), "_PARAMETER",
                          parameter_id_counter_, parameter_labels_);
  }

  std::string
  getClassLabel(std::shared_ptr<parsetree::ast::ClassDecl> class_obj) {
    return getUniqueLabel(class_obj, class_obj->getFullName(), "_CLASS",
                          class_id_counter_, class_labels_);
  }

private:
  template <typename DeclPtr>
  std::string
  getUniqueLabel(DeclPtr obj, const std::string &actual_name,
                 const std::string &prefix, size_t &counter,
                 std::unordered_map<DeclPtr, std::string> &label_map) {
    auto it = label_map.find(obj);
    if (it != label_map.end()) {
      return it->second;
    }

    std::string label = kGlobalPrefix + prefix + "_ID_" +
                        std::to_string(counter++) + "_#" + actual_name;
    label_map[obj] = label;
    return label;
  }

  size_t method_id_counter_ = 0;
  std::unordered_map<std::shared_ptr<parsetree::ast::MethodDecl>, std::string>
      method_labels_;

  size_t field_id_counter_ = 0;
  std::unordered_map<std::shared_ptr<parsetree::ast::FieldDecl>, std::string>
      field_labels_;

  size_t local_var_id_counter_ = 0;
  std::unordered_map<std::shared_ptr<parsetree::ast::VarDecl>, std::string>
      local_var_labels_;

  size_t parameter_id_counter_ = 0;
  std::unordered_map<std::shared_ptr<parsetree::ast::VarDecl>, std::string>
      parameter_labels_;

  size_t class_id_counter_ = 0;
  std::unordered_map<std::shared_ptr<parsetree::ast::ClassDecl>, std::string>
      class_labels_;
};

} // namespace codeGen
