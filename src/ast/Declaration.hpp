#pragma once

#include "astNode.h"

namespace ast {

class VarDecl;
class FieldDecl;
class MethodDecl;

class TypedDecl : public Decl {
    std::unique_ptr<Type> type;

public:
    TypedDecl(std::unique_ptr<Type> type, std::string name)
        : Decl{std::move(name)}, type{std::move(type)} {}

    [[nodiscard]] const Type* getType() const noexcept { return type.get(); }
};

class VarDecl : public TypedDecl {
    std::unique_ptr<Stmt> init;

public:
    VarDecl(std::unique_ptr<Type> type, std::string name)
        : TypedDecl{std::move(type), std::move(name)} {}

    [[nodiscard]] bool hasInit() const noexcept { return init != nullptr; }
    [[nodiscard]] const Stmt* getInit() const noexcept { return init.get(); }

    std::ostream& print(std::ostream& os, int indentation = 0) const override;
};

class FieldDecl : public VarDecl {
    Modifiers modifiers;

public:
    FieldDecl(Modifiers modifiers, std::unique_ptr<Type> type, std::string name)
        : VarDecl{std::move(type), std::move(name)}, modifiers{modifiers} {
        if (modifiers.isFinal()) {
            throw std::runtime_error("FieldDecl cannot be final");
        }
        if (modifiers.isAbstract()) {
            throw std::runtime_error("FieldDecl cannot be abstract");
        }
        if (modifiers.isNative()) {
            throw std::runtime_error("FieldDecl cannot be native");
        }
        if (modifiers.isPublic() && modifiers.isProtected()) {
            throw std::runtime_error("A method cannot be both public and protected. " + name);
        }
        if (!modifiers.isPublic() && !modifiers.isProtected()) {
            throw std::runtime_error("Field must have a visibility modifier");
        }
    }

    std::ostream& print(std::ostream& os, int indentation = 0) const override;
};

struct ImportDeclaration {
    std::unique_ptr<QualifiedIdentifier> qualifiedIdentifier;
    bool isOnDemand;
};

class CompilationUnit final : public DeclContext {
    std::unique_ptr<QualifiedIdentifier> package;
    std::vector<ImportDeclaration> imports;
    std::unique_ptr<DeclContext> body;

public:
    CompilationUnit(std::unique_ptr<QualifiedIdentifier> package,
                    std::vector<ImportDeclaration> imports,
                    std::unique_ptr<DeclContext> body)
        : package{std::move(package)}, imports{std::move(imports)}, body{std::move(body)} {}

    std::ostream& print(std::ostream& os, int indentation = 0) const override;
    [[nodiscard]] DeclContext* getBody() const noexcept { return body.get(); }
};

class ClassDecl : public DeclContext, public Decl {
    Modifiers modifiers;
    std::unique_ptr<QualifiedIdentifier> superClass;
    std::vector<std::unique_ptr<QualifiedIdentifier>> interfaces;
    std::vector<std::unique_ptr<FieldDecl>> fields;
    std::vector<std::unique_ptr<MethodDecl>> methods;
    std::vector<std::unique_ptr<MethodDecl>> constructors;

public:
    ClassDecl(Modifiers modifiers,
              std::string name,
              std::unique_ptr<QualifiedIdentifier> superClass,
              std::vector<std::unique_ptr<QualifiedIdentifier>> interfaces,
              std::vector<std::unique_ptr<Decl>> classBodyDecls);

    std::ostream& print(std::ostream& os, int indentation = 0) const override;
};

class InterfaceDecl : public DeclContext, public Decl {
    Modifiers modifiers;
    std::vector<std::unique_ptr<QualifiedIdentifier>> extends;
    std::vector<std::unique_ptr<MethodDecl>> methods;

public:
    InterfaceDecl(Modifiers modifiers,
                  std::string name,
                  std::vector<std::unique_ptr<QualifiedIdentifier>> extends,
                  std::vector<std::unique_ptr<Decl>> interfaceBodyDecls);

    std::ostream& print(std::ostream& os, int indentation = 0) const override;
};

class MethodDecl : public DeclContext, public Decl {
    Modifiers modifiers;
    std::unique_ptr<Type> returnType;
    std::vector<std::unique_ptr<VarDecl>> parameters;
    bool isConstructor_;
    std::unique_ptr<Stmt> body;

public:
    MethodDecl(Modifiers modifiers,
               std::string name,
               std::unique_ptr<Type> returnType,
               std::vector<std::unique_ptr<VarDecl>> parameters,
               bool isConstructor,
               std::unique_ptr<Stmt> body);

    std::ostream& print(std::ostream& os, int indentation = 0) const override;
    [[nodiscard]] Modifiers& getModifiers() noexcept { return modifiers; }
    [[nodiscard]] bool isConstructor() const noexcept { return isConstructor_; }
};

} // namespace ast
