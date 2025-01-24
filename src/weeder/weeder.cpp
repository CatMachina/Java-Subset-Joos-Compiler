#include "weeder.hpp"

void Weeder::weed(const ClassDecl& classDecl) {
    // Check class constraints
    checkClassConstraints(classDecl);

    // Check all fields
    for (const auto& field : classDecl.fields) {
        checkFieldConstraints(*field);
    }

    // Check all methods
    for (const auto& method : classDecl.methods) {
        checkMethodConstraints(*method);
    }

    // If it's an interface, apply interface-specific constraints
    if (!classDecl.interfaces.empty()) {
        checkInterfaceConstraints(classDecl);
    }
}

void Weeder::checkClassConstraints(const ClassDecl& classDecl) {
    // A class cannot be both abstract and final
    if (classDecl.modifiers.contains("abstract") && classDecl.modifiers.contains("final")) {
        throw std::runtime_error("A class cannot be both abstract and final: " + classDecl.name);
    }

    // Every class must contain at least one explicit constructor
    if (classDecl.methods.empty()) {
        throw std::runtime_error("Class must have at least one explicit constructor: " + classDecl.name);
    }
}

void Weeder::checkMethodConstraints(const MethodDecl& methodDecl) {
    // A method has a body if and only if it is neither abstract nor native
    if ((methodDecl.modifiers.contains("abstract") || methodDecl.modifiers.contains("native")) && methodDecl.body) {
        throw std::runtime_error("Abstract or native methods cannot have a body: " + methodDecl.name);
    }

    if (!methodDecl.body &&
        !methodDecl.modifiers.contains("abstract") &&
        !methodDecl.modifiers.contains("native")) {
        throw std::runtime_error("Non-abstract and non-native methods must have a body: " + methodDecl.name);
    }

    // An abstract method cannot be static or final
    if (methodDecl.modifiers.contains("abstract") &&
        (methodDecl.modifiers.contains("static") || methodDecl.modifiers.contains("final"))) {
        throw std::runtime_error("Abstract methods cannot be static or final: " + methodDecl.name);
    }

    // A static method cannot be final
    if (methodDecl.modifiers.contains("static") && methodDecl.modifiers.contains("final")) {
        throw std::runtime_error("Static methods cannot be final: " + methodDecl.name);
    }

    // A native method must be static
    if (methodDecl.modifiers.contains("native") && !methodDecl.modifiers.contains("static")) {
        throw std::runtime_error("Native methods must be static: " + methodDecl.name);
    }

    // The type void may only be used as the return type of a method
    if (methodDecl.returnType == "void" && methodDecl.name != "void") {
        throw std::runtime_error("Only methods can have 'void' as a return type: " + methodDecl.name);
    }

    // A method or constructor must not contain explicit this() or super() calls
    // (Check body for illegal this() or super() calls if needed)
}

void Weeder::checkFieldConstraints(const FieldDecl& fieldDecl) {
    // No field can be final
    if (fieldDecl.modifiers.contains("final")) {
        throw std::runtime_error("Fields cannot be final: " + fieldDecl.name);
    }
}

void Weeder::checkInterfaceConstraints(const ClassDecl& classDecl) {
    // An interface cannot contain fields or constructors
    if (!classDecl.fields.empty()) {
        throw std::runtime_error("Interfaces cannot contain fields: " + classDecl.name);
    }

    // An interface method cannot be static, final, or native
    for (const auto& method : classDecl.methods) {
        if (method->modifiers.contains("static") ||
            method->modifiers.contains("final") ||
            method->modifiers.contains("native")) {
            throw std::runtime_error("Interface methods cannot be static, final, or native: " + method->name);
        }

        // An interface method cannot have a body
        if (method->body) {
            throw std::runtime_error("Interface methods cannot have a body: " + method->name);
        }
    }
}

void Weeder::visit(const LiteralExpr& expr) {
    // All characters must be in the range of 7-bit ASCII (0-127)
    if (expr.literalType == LiteralExpr::Type::String) {
        for (char c : expr.value) {
            if (static_cast<unsigned char>(c) > 127) {
                throw std::runtime_error("String literals must contain only 7-bit ASCII characters.");
            }
        }
    }
}

void Weeder::visit(const VariableExpr& expr) {
    // Nothing specific for variables in weeder
}

void Weeder::visit(const BinaryExpr& expr) {
    // Recursively check left and right operands
    expr.lhs->accept(*this);
    expr.rhs->accept(*this);
}

void Weeder::visit(const UnaryExpr& expr) {
    // Recursively check the operand
    expr.operand->accept(*this);
}

void Weeder::visit(const MethodCallExpr& expr) {
    // Recursively check the object and arguments
    if (expr.object) {
        expr.object->accept(*this);
    }
    for (const auto& arg : expr.arguments) {
        arg->accept(*this);
    }
}

void Weeder::visit(const BlockStmt& stmt) {
    // Recursively check all statements in the block
    for (const auto& subStmt : stmt.statements) {
        subStmt->accept(*this);
    }
}

void Weeder::visit(const ExprStmt& stmt) {
    // Check the expression
    stmt.expression->accept(*this);
}

void Weeder::visit(const IfStmt& stmt) {
    // Check the condition and branches
    stmt.condition->accept(*this);
    stmt.thenBranch->accept(*this);
    if (stmt.elseBranch) {
        stmt.elseBranch->accept(*this);
    }
}

void Weeder::visit(const WhileStmt& stmt) {
    // Check the condition and body
    stmt.condition->accept(*this);
    stmt.body->accept(*this);
}

void Weeder::visit(const ReturnStmt& stmt) {
    // Check the return expression (if any)
    if (stmt.expression) {
        stmt.expression->accept(*this);
    }
}
