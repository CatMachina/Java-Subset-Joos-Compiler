#ifndef AST_HPP
#define AST_HPP

#include <memory>
#include <string>
#include <vector>

// Forward declarations for visitor
class ExprVisitor;
class StmtVisitor;

// Base class for all AST nodes
class ASTNode {
public:
    virtual ~ASTNode() = default;
};

// Base class for expressions
class Expr : public ASTNode {
public:
    virtual void accept(ExprVisitor& visitor) const = 0;
};

// Base class for statements
class Stmt : public ASTNode {
public:
    virtual void accept(StmtVisitor& visitor) const = 0;
};

//////////////////// Expressions ////////////////////

// Literal expressions (int, boolean, char, etc.)
class LiteralExpr : public Expr {
public:
    enum class Type { Int, Boolean, Char, String, Null };

    Type literalType;
    std::string value;

    LiteralExpr(Type type, const std::string& val)
        : literalType(type), value(val) {}

    void accept(ExprVisitor& visitor) const override;
};

// Variable expressions
class VariableExpr : public Expr {
public:
    std::string name;

    explicit VariableExpr(const std::string& name) : name(name) {}

    void accept(ExprVisitor& visitor) const override;
};

// Binary expressions (e.g., x + y, x < y)
class BinaryExpr : public Expr {
public:
    std::string op; // Operator (e.g., "+", "<")
    std::unique_ptr<Expr> lhs, rhs;

    BinaryExpr(const std::string& op, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    void accept(ExprVisitor& visitor) const override;
};

// Unary expressions (e.g., -x, !x)
class UnaryExpr : public Expr {
public:
    std::string op; // Operator (e.g., "-", "!")
    std::unique_ptr<Expr> operand;

    UnaryExpr(const std::string& op, std::unique_ptr<Expr> operand)
        : op(op), operand(std::move(operand)) {}

    void accept(ExprVisitor& visitor) const override;
};

// Method calls
class MethodCallExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    std::string methodName;
    std::vector<std::unique_ptr<Expr>> arguments;

    MethodCallExpr(std::unique_ptr<Expr> object, const std::string& methodName, std::vector<std::unique_ptr<Expr>> args)
        : object(std::move(object)), methodName(methodName), arguments(std::move(args)) {}

    void accept(ExprVisitor& visitor) const override;
};

//////////////////// Statements ////////////////////

// Block statements (e.g., { ... })
class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts)
        : statements(std::move(stmts)) {}

    void accept(StmtVisitor& visitor) const override;
};

// Expression statements (e.g., x = 5;)
class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;

    explicit ExprStmt(std::unique_ptr<Expr> expr)
        : expression(std::move(expr)) {}

    void accept(StmtVisitor& visitor) const override;
};

// If statements
class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;

    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBranch, std::unique_ptr<Stmt> elseBranch = nullptr)
        : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

    void accept(StmtVisitor& visitor) const override;
};

// While statements
class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}

    void accept(StmtVisitor& visitor) const override;
};

// Return statements
class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;

    explicit ReturnStmt(std::unique_ptr<Expr> expr = nullptr)
        : expression(std::move(expr)) {}

    void accept(StmtVisitor& visitor) const override;
};

//////////////////// Declarations ////////////////////

// Field declaration
class FieldDecl : public ASTNode {
public:
    std::string name;
    std::string type;
    std::unique_ptr<Expr> initializer;

    FieldDecl(const std::string& name, const std::string& type, std::unique_ptr<Expr> initializer = nullptr)
        : name(name), type(type), initializer(std::move(initializer)) {}
};

// Method declaration
class MethodDecl : public ASTNode {
public:
    std::string name;
    std::string returnType;
    std::vector<std::pair<std::string, std::string>> parameters; // {param_type, param_name}
    std::unique_ptr<BlockStmt> body;

    MethodDecl(const std::string& name, const std::string& returnType,
               std::vector<std::pair<std::string, std::string>> params, std::unique_ptr<BlockStmt> body)
        : name(name), returnType(returnType), parameters(std::move(params)), body(std::move(body)) {}
};

// Class declaration
class ClassDecl : public ASTNode {
public:
    std::string name;
    std::string superclass;
    std::vector<std::string> interfaces;
    std::vector<std::unique_ptr<FieldDecl>> fields;
    std::vector<std::unique_ptr<MethodDecl>> methods;

    ClassDecl(const std::string& name, const std::string& superclass, std::vector<std::string> interfaces,
              std::vector<std::unique_ptr<FieldDecl>> fields, std::vector<std::unique_ptr<MethodDecl>> methods)
        : name(name), superclass(superclass), interfaces(std::move(interfaces)),
          fields(std::move(fields)), methods(std::move(methods)) {}
};

//////////////////// Visitor Interfaces ////////////////////

// Expression visitor
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;

    virtual void visit(const LiteralExpr& expr) = 0;
    virtual void visit(const VariableExpr& expr) = 0;
    virtual void visit(const BinaryExpr& expr) = 0;
    virtual void visit(const UnaryExpr& expr) = 0;
    virtual void visit(const MethodCallExpr& expr) = 0;
};

// Statement visitor
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;

    virtual void visit(const BlockStmt& stmt) = 0;
    virtual void visit(const ExprStmt& stmt) = 0;
    virtual void visit(const IfStmt& stmt) = 0;
    virtual void visit(const WhileStmt& stmt) = 0;
    virtual void visit(const ReturnStmt& stmt) = 0;
};

#endif // AST_HPP
