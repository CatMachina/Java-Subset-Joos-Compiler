#ifndef WEEDER_HPP
#define WEEDER_HPP

#include "ast.hpp"
#include <stdexcept>

class Weeder : public ExprVisitor, public StmtVisitor {
public:
    void weed(const ClassDecl& classDecl);

    void visit(const LiteralExpr& expr) override;
    void visit(const VariableExpr& expr) override;
    void visit(const BinaryExpr& expr) override;
    void visit(const UnaryExpr& expr) override;
    void visit(const MethodCallExpr& expr) override;

    void visit(const BlockStmt& stmt) override;
    void visit(const ExprStmt& stmt) override;
    void visit(const IfStmt& stmt) override;
    void visit(const WhileStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;

private:
    void checkClassConstraints(const ClassDecl& classDecl);
    void checkMethodConstraints(const MethodDecl& methodDecl);
    void checkFieldConstraints(const FieldDecl& fieldDecl);
    void checkInterfaceConstraints(const ClassDecl& classDecl);
};

#endif // WEEDER_HPP
