// #pragma once

// #include "tir/TIR.hpp"
// #include <vector>
// #include <unordered_map>

// namespace tir {

// class NodeFactory
// {

//   std::shared_ptr<BinOp> IRBinOp(BinOp::OpType type, std::shared_ptr<Expr>
//   left, std::shared_ptr < Expr> right)
//   {
//     return std::make_shared<BinOp>(type, left, right);
//   }

//   std::shared_ptr<Call> IRCall(std::shared_ptr<Expr> target,
//   std::vector<std::shared_ptr<Expr>> args)
//   {
//     return new Call(target, args);
//   }

//   @Call IRCall(Expr target, List<Expr> args)
//   {
//     return new Call(target, args);
//   }

//   @Override public CJump IRCJump(Expr expr, String trueLabel)
//   {
//     return new CJump(expr, trueLabel);
//   }

//   @Override public CJump IRCJump(Expr expr, String trueLabel, String
//   falseLabel)
//   {
//     return new CJump(expr, trueLabel, falseLabel);
//   }

//   @Override public CompUnit IRCompUnit(String name)
//   {
//     return new CompUnit(name);
//   }

//   @Override public CompUnit IRCompUnit(String name,
//                                        Map<String, FuncDecl> functions)
//   {
//     return new CompUnit(name, functions);
//   }

//   @Override public Const IRConst(int value)
//   {
//     return new Const(value);
//   }

//   @Override public ESeq IRESeq(Stmt stmt, Expr expr)
//   {
//     return new ESeq(stmt, expr);
//   }

//   @Override public Exp IRExp(Expr expr)
//   {
//     return new Exp(expr);
//   }

//   @Override public FuncDecl IRFuncDecl(String name, int numParams, Stmt stmt)
//   {
//     return new FuncDecl(name, numParams, stmt);
//   }

//   @Override public Jump IRJump(Expr expr)
//   {
//     return new Jump(expr);
//   }

//   @Override public Label IRLabel(String name)
//   {
//     return new Label(name);
//   }

//   @Override public Mem IRMem(Expr expr)
//   {
//     return new Mem(expr);
//   }

//   @Override public Move IRMove(Expr target, Expr expr)
//   {
//     return new Move(target, expr);
//   }

//   @Override public Name IRName(String name)
//   {
//     return new Name(name);
//   }

//   @Override public Return IRReturn(Expr ret)
//   {
//     return new Return(ret);
//   }

//   @Override public Seq IRSeq(Stmt... stmts)
//   {
//     return new Seq(stmts);
//   }

//   @Override public Seq IRSeq(List<Stmt> stmts)
//   {
//     return new Seq(stmts);
//   }

//   @Override public Temp IRTemp(String name)
//   {
//     return new Temp(name);
//   }
// }

// } // namespace tir