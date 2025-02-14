#include "staticCheck/globalEnvironment.hpp"

namespace static_check {

void GlobalEnvironment::registerDecl(const std::string &name,
                                     std::shared_ptr<Decl> decl) {
  namesToDecls.insert({name, decl});
}

std::shared_ptr<Decl>
GlobalEnvironment::getDecl(const std::string &name) const {
  auto result = namesToDecls.find(name);
  if (result == namesToDecls.end()) {
    return nullptr;
  }
  return result->second;
}

void GlobalEnvironment::registerTypeToPackage(const std::string &typeName,
                                              std::string &packageName) {
  typeNamesToPackageNames.insert({typeName, packageName});
}

std::string
GlobalEnvironment::getPackageName(const std::string &typeName) const {
  auto result = typeNamesToPackageNames.find(typeName);
  if (result == typeNamesToPackageNames.end()) {
    return nullptr;
  }
  return result->second;
}

} // namespace static_check
