# Java Subset (Joos) Compiler
A compiler for a subset of the Java 1.3 language. This project was a group project for a university course.

## Authors and Acknowledgments
Special thanks to Edward Xu, Angel Zhang, and Vedant Gupta for many hours spent completing this project together.

## Description
The specification of the compiler has been outlined in a supported features list and assignments for the course. The compiler is created for quite a large subset, so the webpages are linked instead of copying the descriptions over.
- [Supported Features](https://student.cs.uwaterloo.ca/~cs444/joos/features/)
- [Scanning, Parsing, Weeding](https://student.cs.uwaterloo.ca/~cs444/assignments/a1/)
- [Static Checking (Part A)](https://student.cs.uwaterloo.ca/~cs444/assignments/a2/)
- [Static Checking (Part B)](https://student.cs.uwaterloo.ca/~cs444/assignments/a3/)
- [Static Checking (Part C)](https://student.cs.uwaterloo.ca/~cs444/assignments/a4/)
- [Code Generation (Part A)](https://student.cs.uwaterloo.ca/~cs444/assignments/a5/)
- [Code Generation (Part B)](https://student.cs.uwaterloo.ca/~cs444/assignments/a6/)

If the above links do not work or the newest offering of the course is too new (making my group's work very outdated), please refer to these alternative links:
- [Supported Features](https://web.archive.org/web/20240223170621/https://student.cs.uwaterloo.ca/~cs444/joos/features/)
- [Scanning, Parsing, Weeding](https://web.archive.org/web/20240229203505/https://student.cs.uwaterloo.ca/~cs444/assignments/a1/)
- [Static Checking (Part A)](https://web.archive.org/web/20240229220741/https://student.cs.uwaterloo.ca/~cs444/assignments/a2/)
- [Static Checking (Part B)](https://web.archive.org/web/20240131093221/https://student.cs.uwaterloo.ca/~cs444/assignments/a3/)
- [Static Checking (Part C)](https://web.archive.org/web/20240131093225/https://student.cs.uwaterloo.ca/~cs444/assignments/a4/)
- [Code Generation (Part A)](https://web.archive.org/web/20240131093229/https://student.cs.uwaterloo.ca/~cs444/assignments/a5/)
- [Code Generation (Part B)](https://web.archive.org/web/20240131093233/https://student.cs.uwaterloo.ca/~cs444/assignments/a6/)

# High Level Overview
As of now, I have not yet implemented a way to demo it or reproduce it locally due to specific school configuration, builds, and tooling versions. Until I figure it out, if you want to learn more, I talk about the components of the compiler here.

Each component contains a brief **bolded summary** of what that component does, and is followed by a description of how it is implemented in the project. Feel free to click on the links to take a look at the code!

## Lexer
**The lexer (lexical analyzer) reads the raw source code and converts it into a stream of tokens—atomic units like keywords, identifiers, symbols, and literals.**

The lexer is implemented using the Flex lexical analyzer generator. The code for the generator is written in [`lexer.l`](src/lexer/lexer.l) and is organized into a few sections. The top is the definitions section where certain helper functions and variables are initialized. The middle section is the rules section which contains all of the regular expressions sorted by the type of token. The final section contains a few more helper functions for debugging and integration with bison.

## Parser
**The parser consumes the token stream from the lexer and builds a parse tree or abstract syntax tree (AST) based on grammar rules. It checks for correct syntax.**

The parser is implemented using the GNU Bison parser generator. The code for the parser is written in [`parser.y`](src/parser/parser.y) and is organized into a few sections. The prologue consists of C++ code copied verbatim into the generated parser, which includes `#include` statements, type definitions, and helper function forward declarations. The declaration section defines parser configurations, namely: tokens, precedence rules, and semantic types. The next section are the grammar rules, which define how tokens should be parsed  and the action taken when using this rule. The last section is the epilogue, which contains additional C++ code to define helper functions, the main function, and to integrate the parser into the rest of the compiler.

## Abstract Syntax Tree (AST)
**The AST is a tree-based representation of the program's structure, abstracting away redundant syntactic details to focus on the program's semantics.**

The [`ParseTreeVisitor`](include/parseTree/parseTreeVisitor.hpp) class uses the visitor design pattern to traverse the parse tree and construct the corresponding AST nodes.  Statements and declarations are represented using a standard tree structure, with each node storing pointers to its children. For example, a [`ClassDecl`](include/ast/astNode.hpp#L604) node stores pointers to [`MethodDecl`](include/ast/astNode.hpp#L507) and [`FieldDecl`](include/ast/astNode.hpp#L490) objects (all located in [`astNode.hpp`](include/ast/astNode.hpp)).  Expressions, however, are represented by a Reverse Polish Notation (RPN) approach. Specifically, the [`Expr`](include/ast/astNode.hpp#L183) class is a wrapper containing a list of [`ExprNode`](include/ast/astNode.hpp#L173) objects (also in [`astNode.hpp`](include/ast/astNode.hpp)), each representing either an operator or an operand. In terms of the algorithm, the AST traversal uses a recursive depth-first traversal to visit nodes for printing, analysis, and evaluation. 

## Type Linker
**The type linker resolves references to types across files or modules, linking together class, interface, or struct definitions to form a coherent type system.**

The [type linker](include/staticCheck/typeLinker.hpp) includes two passes - global symbol table construction and type linking. In the first pass, a hierarchical symbol table is constructed that contains packages, classes and interface declarations in the global environment. A trie structure is used for the [`Package`](include/staticCheck/environment.hpp) class, where each child represents a subpackage or a class/interface declaration. The `std::variant` library is used to represent the different types of package child. To construct the symbol table, the AST nodes are traversed to extract package and class declarations and store them in a hierarchical way. The second pass then iterates over ASTs and recursively resolves types for the AST nodes in the program, using the symbol table from the previous pass. 

## Hierarchy Checker
**The hierarchy checker validates inheritance and interface relationships (e.g., ensuring no cycles in class hierarchies, checking method overrides, etc.).**

Hierarchy checking is implemented in [`hierarchyCheck.hpp`](include/staticCheck/hierarchyCheck.hpp). It validates class and interface relationships based on the inheritance rules. To do these checks, the class interacts with the AST and its AST nodes by performing a tree traversal. A noteworthy algorithm used is DFS when checking for cyclic dependencies.

## Name Disambiguation
**Name disambiguation resolves ambiguous variable or function names based on scope, import, or overloading rules. It ensures identifiers refer to the correct entities.**

Two passes are used for this. The first pass, [`ExprResolver`](include/staticCheck/exprResolver.hpp), establishes bindings between identifiers and their declarations. The second pass, [`ForwardChecker`](include/staticCheck/forwardChecker.hpp), ensures that variable and field references conform to the forward reference rules. 

As [`ExprNode`](include/ast/astNode.hpp#L173) in [`Expr`](include/ast/astNode.hpp#L183) is stored in Reverse Polish Notation (RPN), an [`Evaluator`](include/staticCheck/evaluator.hpp) interface is created that resolves `ExprNodes` using a stack. Ambiguous names are resolved by recursively reducing expressions into their simplest form. This is further split up into two cases:
- **Name resolution algorithm:** To resolve a name, it is first searched in its declared set if currently in a class, or its local declares otherwise. If not found, it is searched instead in the inherited set of the current context. If not found, recursively search in parent context.

- **Method Overload Resolution:** All the valid candidates of the method are searched (same size of parameters, parameter types applicable, etc.). Then the most specific method is chosen. A method `a` is more specific than another method `b` if `a`'s parameter types are all more specific than `b`'s and `a`'s declaring type is more specific than `b`'s. Two types `A` > `B` when `A` converts to `B`. If more than one valid candidate remains then it is ambiguous.

In `ForwardChecker`, forward references are searched by directly looking at [`FieldDecl`](include/ast/astNode.hpp#L490). After `ExprResolver` runs, all `ExprNodes` that could be assigned to a [`Decl`](include/ast/astNode.hpp#L80) is resolved, and then checked if the resolved `Decl` is initialized after the current `FieldDecl` by comparing the location in the file.

## Type Checker
**The type checker ensures expressions and operations are semantically valid with respect to the language's type system. It reports type mismatches and infers types if needed.**

Type checking also consists of two passes through [`TypeResolver`](include/staticCheck/typeResolver.hpp) and [`StaticResolver`](include/staticCheck/staticResolver.hpp). `TypeResolver` handles type inference, checking assignability, and ensuring valid type conversions. `StaticResolver` ensures static context correctness, verifying instance versus static accesses and accessibility of methods and fields. Both of them inherit the [`Evaluator`](include/staticCheck/evaluator.hpp) from the previous section to resolve from RPN expression nodes.

This stage runs with the assumption that all expression nodes have the corresponding [`Decl`](include/ast/astNode.hpp#L80) assigned to it from forward checking.

## Control Flow Graph (CFG) Builder
**The CFG builder constructs a control flow graph (CFG), where nodes represent program statements or blocks and edges represent possible execution paths (e.g., branches, loops).**
There are three classes for the CFG builder. The [`CFGNode`](include/staticCheck/cfg.hpp) class represents individual nodes in a CFG. Each node contains the corresponding statement in the AST, an optional condition (for branching nodes), and pointers to its predecessor and successor nodes. Each node also contains pointers to [`ReachabilityAnalysisInfo`](include/staticCheck/reachabilityAnalysisInfo.hpp) and [`LiveVariableAnalysisInfo`](include/staticCheck/liveVariableAnalysisInfo.hpp) objects for later data analysis passes. The CFG class stores the set of `CFGNode`’s and provides functions for adding edges between them. The [`CFGBuilder`](include/staticCheck/cfgBuilder.hpp) class constructs the corresponding CFG given a method declaration AST node. The constant folding method uses the `std::variant` library to store different possible evaluation results, such as booleans, integers, or null values. A stack-based approach is used to evaluate the [`ExprNode`](include/ast/astNode.hpp#L173) in Reverse Polish Notation (RPN).

## Dataflow Analysis
**Dataflow analysis computes properties like variable liveness, reachability, or constant propagation by analyzing how data moves through the CFG.**

The dataflow analysis is performed by the [`ReachabilityAnalysis`](include/staticCheck/reachabilityAnalysis.hpp) and [`LiveVariableAnalysis`](include/staticCheck/liveVariableAnalysis.hpp) classes. Both analyses use the resulting control flow graph (CFG) from the [`CFGBuilder`](include/staticCheck/cfgBuilder.hpp) to run analysis. The analysis classes also use the [`ReachabilityAnalysisInfo`](include/staticCheck/reachabilityAnalysisInfo.hpp) and [`LiveVariableAnalysis`](include/staticCheck/reachabilityAnalysisInfo.hpp) classes attached to each [`CFGNode`](include/staticCheck/cfg.hpp) to perform analysis. These classes contain a single static method which is used to call for the check. Helper functions are present in their respective `.cpp` implementation files.

The reachability analysis uses the [standard dataflow equations](https://en.wikipedia.org/wiki/Data-flow_analysis#Basic_principles) along with an iterative method to solve for the `in`/`out` of each CFG node. Once `in`/`out` of each node is computed, another pass is performed to check if the `in` of any CFG node is false, in which case, the statement is unreachable. To check for finite-length execution path returns, a sentinel statement is added to the end of each method, so that if this sentinel statement is reached, then it will be known that some finite execution path did not result in a return statement.

The live variable analysis uses its [standard respective equations](https://en.wikipedia.org/wiki/Data-flow_analysis#Bit_vector_problems), but with the worklist algorithm to perform updates. The `use`/`def` sets were precomputed, as they do not need to be reupdated in the algorithm. These computations were done by classifying the statements of each CFG node, and then combing through the statements to add to the set. Once precomputation was finished, the `in` and `out` sets are updated for each node. Lastly, a check is done to see if any assignment statement had an `out` set that did not contain the variable being updated.

## Typed Intermediate Representation (TIR)
**TIR is a lower-level, strongly typed version of the AST or IR used to simplify later transformations and enable type-safe optimizations.**

The compiler's intermediate representation (IR) is based on a tree-structured design.  The IR consists of two categories of nodes - expressions and statements.  Expressions include `BinOp`, `Call`, `Const`, `ESeq`, `Mem`, `Name`, and `Temp`, and statements encompass `CJump`, `CallStmt`, `Exp`, `Jump`, `Label`, `Move`, `Return`, and `Seq`. There are also nodes that belong to neither category, such as `Comment`, `CompUnit`, and `FuncDecl`. Therefore, the implemented class hierarchy is with a `Node` class as the abstract base class. The two classes that inherit from the `Node` are `Expr` and `Stmt`, representing expressions and statements respectively. Each type of TIR node then inherits from one of these two classes. All of these classes can be found under the [`tir` folder](include/tir/).


## AST to TIR Conversion
**This stage transforms the high-level AST into the typed intermediate representation (TIR), maintaining semantic meaning while making it easier to analyze and optimize.**

The [`TIRBuilder`](include/tir/TIRBuilder.hpp) class is the main component responsible for converting an abstract syntax tree (AST) into a typed Intermediate Representation (TIR). This class contains an [`ExprIRConverter`](include/codeGen/exprIRConverter.hpp), which converts AST expressions to IR-level expressions. The `TIRBuilder` manages compilation units ([`CompUnit`](include/tir/CompUnit.hpp)) and supports the conversion of AST statements and declarations.
As mentioned above, the `ExprIRConverter` class is responsible for handling the AST expressions and converting them into TIR. Implemented helper functions like `evalBinOp`, `evalUnOp`, and `evalFieldAccess`, etc., (all found under the [`.cpp`](src/codegen/exprIRConverter.cpp)) are used to modularize the handling of different types of expressions. It also uses external components such as a [Dispatch Vector builder](include/codeGen/dispatchVector.hpp), a [label manager](include/codeGen/codeGenLabels.hpp), and an [AST manager](include/ast/astManager.hpp).

## IR Lowering to Canonical IR Conversion
**This step translates high-level TIR constructs into canonical IR—a simpler, uniform subset of the IR—removing syntactic sugar and complex constructs.**

The [`TIRCanonicalizer`](include/codeGen/canonicalizer.hpp) class simplifies the expressions and statements in the TIR by hoisting side effects and converting to a canonical form. This allows later optimization and code generation phases to run more smoothly. The canonicalizer contains expression-level canonicalization, handled by `canonicalize(std::shared_ptr<tir::Expr>)`, and statement-level canonicalization, handled by `canonicalize(std::shared_ptr<tir::Stmt>)` and helper functions for specific statement types.

## Canonical IR to Tiled Assembly Conversion
**Generates tiled assembly, mapping IR code to low-level instruction-like blocks that correspond to machine-level behavior, often considering cache or register usage patterns.**

Assembly instructions are implemented by inheriting instruction base class Instruction. Assembly instructions are viewed as "tiles" that cover parts of the IR structure. The [`Tile`](include/codeGen/tile.hpp) class manages these instructions and their associated costs. It holds a list of `TileInstruction` variants, which can be assembly instructions or other tiles. It calculates and stores the cost of executing the instructions within the tile. It also handles the assignment of virtual registers to the instructions within the tile.

The tiling process aims to find the most efficient assembly code by selecting the best possible tiles for each part of the IR. This relies on the assumption that the optimal solution for a larger IR structure includes the optimal solutions for its sub-structures. Dynamic programming and memoization are used to optimize the tile selection.

The [`InstructionSelector`](include/codeGen/instructionSelector.hpp) class does the tiling, and uses a cache to store the best tiles for each IR subtree, preventing extra calculations. It assigns virtual registers and generates the final assembly Instruction sequence.

## Register Allocation
**Register allocation assigns variables and temporaries to machine registers (or memory) while minimizing spills and respecting hardware constraints.**

[Register allocation](include/codeGen/registerAllocator/registerAllocator.hpp) is done using virtual `RegisterAllocator->allocateFor` that takes in a vector of assembly instructions and returns the stack size. Different allocators are defined by inheriting and implementing this base class.

The basic allocator is an intial solution for correctness, while the linear scan allocator is an attempt to reduce compile-time by optimizing register usage (and thus reduce the output size).

### Basic Allocator
[`BasicAllocator`](include/codeGen/registerAllocator/basicAllocator.hpp) simply spills everything on the stack. Firstly, free virtual registers are assigned, then replace all the virtual registers to general purpose registers and return the stack offset.

### Linear Scan Allocator
The linear scan allocator consists of three steps. The first step is [live variable analysis](src/codeGen/registerAllocator/linearScanAllocator.cpp#L21), which computes the live intervals for each virtual register. The next step is the linear scan allocation, which assigns hardware registers to virtual registers by sweeping through live intervals, and then substitutes virtual registers in instructions with their allocated physical registers. Lastly, it spills virtual registers to stack in situations where there is no available physical register.

There are a few bookkeeping objects that are used to accomplish this. [`LiveInterval`](include/codeGen/registerAllocator/linearScanAllocator.hpp#L15) objects store the register name, the start (begin), and end (end) indices of the interval. Data structures like `std::unordered_map` and `std::unordered_set` are used for mapping register names to their live intervals, mapping virtual registers to assigned physical registers, tracking available physical registers, and tracking active intervals, etc.

## Code Generation
**The code generation phase emits actual machine code or low-level assembly from the lowered IR, completing the translation from source to executable code.**

The [`AssemblyGenerator`](include/codeGen/assemblyGenerator.hpp) class transforms canonicalized IR trees into final assembly files. It includes a [`RegisterAllocator`](include/codeGen/registerAllocator/), a set of [`CodeGenLabels`](include/codeGen/codeGenLabels.hpp), and the entry point. It processes a vector of IR trees, each corresponding to a compilation unit. For each IR tree, it extracts the static fields from the IR tree.  Each node is visited by [`LinkingResolver`](include/codeGen/linkingResolver.hpp), which collects required external methods and static fields. Next,  each IR node is passed to an [`InstructionSelector`](include/codeGen/instructionSelector.hpp), which uses tiling to convert the tree to a list of [`AssemblyInstruction`](include/codeGen/tile.hpp#L14) objects, grouped by function or static field initializer. Then, for each compilation unit, a `.s` file is generated, and it contains sections for global, extern, and function bodies. 


## Dispatch Vector (Support for OOP)
**The dispatch vector (or virtual table) supports dynamic method dispatch in object-oriented languages by mapping method calls to actual implementations at runtime.**

[`DispatchVector`](include/codeGen/dispatchVector.hpp) is the class that manages the runtime dispatch information for each class. This class maintains vectors of method and field declarations, along with the class name, and enables efficient lookup of field offsets of field declarations using `getFieldOffset`.

The [`DispatchVectorBuilder`](include/codeGen/dispatchVector.hpp#L55) class does the creation of these `DispatchVector` instances of each class. It uses a graph-coloring algorithm to assign unique colors (integers) to non-conflicting methods within a class hierarchy, basically virtual method table construction.

The [`visitAST`](include/codeGen/dispatchVector.hpp#L72) method traverses the abstract syntax tree, identifying class and interface declarations and adding their methods to an internal graph. Methods that could potentially override each other (based on signature) are considered neighbors in this graph. The `assignColours` method then colors this graph, `getAssignment` retrieves the assigned color for a given method, and `getDV` creates and caches `DispatchVector` instances for each class.