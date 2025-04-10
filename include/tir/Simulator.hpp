#pragma once

#include "tir/TIR.hpp"
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// For c-style RNG
#include <stdlib.h>
#include <time.h>

class RandomGenerator {
public:
  RandomGenerator() { srand(time(NULL)); }

  int getRandomNumber() { return rand() % INT_MAX; }
};

namespace tir {

/**
 * Constants for interpreting intermediate code.
 * Based on /u/cs444/pub/tir/src/ir/interpret/Configuration.java
 */
struct Configuration {
  /** Some special stack-related names that are used in the IR,
   *
   * Made these into functions since vscode was throwing an error about string
   * field initializers not being allowed
   */
  /** Prefix for argument registers */
  static std::string ABSTRACT_ARG_PREFIX() { return "_ARG"; }
  static std::string ABSTRACT_RET() { return "_RET"; }

  /** Word size; assumes a 32-bit architecture */
  static const int WORD_SIZE = 4;
};

/** Based on /u/cs444/pub/tir/src/ir/interpret/Simulator.java */
class Simulator {
  /** Forward Declarations */
private:
  class ExecutionFrame;
  class ExprStack;

protected:
  /** map from address to instruction */
  std::unordered_map<int, std::shared_ptr<Node>> indexToInsn;

  /** a random number generator for initializing garbage */
  RandomGenerator r;

  void leave(std::shared_ptr<ExecutionFrame> frame);

  /** End Forward Declarations */
private:
  /** compilation unit to be interpreted */
  std::shared_ptr<CompUnit> compUnit;

  /** map from labeled name to address */
  std::unordered_map<std::string, int> nameToIndex;

  /** heap */
  std::vector<int> mem;

  /** heap size maximum **/
  int heapSizeMax;

  std::shared_ptr<ExprStack> exprStack;
  // BufferedReader inReader;

  std::unordered_set<std::string> libraryFunctions;

  /**
   *
   * @param name name of the label
   * @return the IR node at the named label
   */
  int findLabel(std::string name) {
    if (!nameToIndex.count(name))
      throw std::invalid_argument("Could not find label '" + name + "'!");
    return nameToIndex[name];
  }

protected:
  static int debugLevel;

  int getMemoryIndex(int addr) {
    if (addr % Configuration::WORD_SIZE != 0)
      throw std::invalid_argument(
          "Unaligned memory access: " + std::to_string(addr) +
          " (word size=" + std::to_string(Configuration::WORD_SIZE) + ")");
    return addr / Configuration::WORD_SIZE;
  }
  /**
   * Simulate a library function call, returning the list of returned values
   * @param name name of the function call
   * @param args arguments to the function call, which may include
   *          the pointer to the location of multiple results
   */
  int libraryCall(const std::string &name, const std::vector<int> &args) {
    int ws = Configuration::WORD_SIZE;
    int ret;
    if (name == "NATIVEjava.io.OutputStream.nativeWrite") {
      std::cout << ((char)(std::byte)args[0]) << std::endl;
      ret = 0;
    } else if (name == "__malloc") {
      ret = malloc(args[0]);
    } else if (name == "__debexit") {
      ret = args[0];
      exit(args[0]);
    } else if (name == "__exception") {
      ret = 13;
      exit(13);
    } else {
      throw std::runtime_error("Unsupported library function: " + name);
    }
    return ret;
  }

public:
  const int DEFAULT_HEAP_SIZE = 2048;
  /**
   * Construct an IR interpreter with a default heap size
   * @param compUnit the compilation unit to be interpreted
   */
  Simulator(std::shared_ptr<CompUnit> compUnit) : compUnit(compUnit) {}

  /**
   * Construct an IR interpreter
   * @param compUnit the compilation unit to be interpreted
   * @param heapSize the heap size
   */
  Simulator(std::shared_ptr<CompUnit> compUnit, int heapSize)
      : compUnit(compUnit), heapSizeMax(heapSize) {
    // this.compUnit = compUnit;
    // this.heapSizeMax = heapSize;

    // initialize random seed:
    r = RandomGenerator();

    mem = std::vector<int>();

    exprStack = std::make_shared<ExprStack>();
    // Don't think we need this
    // inReader = new BufferedReader(new InputStreamReader(System.in));

    // TODO: check this
    libraryFunctions = std::unordered_set<std::string>();
    libraryFunctions.insert("__malloc");
    libraryFunctions.insert("__debexit");
    libraryFunctions.insert("__exception");
    libraryFunctions.insert("NATIVEjava.io.OutputStream.nativeWrite");

    InsnMapsBuilder imb = InsnMapsBuilder();
    imb.visit(compUnit);
    indexToInsn = imb.getIndexToInsn();
    nameToIndex = imb.getNameToIndex();
    if (debugLevel > 1) {
      std::cout << "indexToInsn size: " << indexToInsn.size() << "\n";
      for (auto &[index, insn] : indexToInsn) {
        std::cout << index << " " << typeid(*insn).name() << "\n";
      }
      std::cout << "nameToIndex size: " << nameToIndex.size() << "\n";
      for (auto &[name, index] : nameToIndex) {
        std::cout << name << " " << index << "\n";
      }
    }
  }

  /**
   * Allocate a specified amount of bytes on the heap
   * @param size the number of bytes to be allocated
   * @return the starting address of the allocated region on the heap
   */
  int malloc(int size) {
    if (size < 0)
      throw std::invalid_argument("Invalid size");
    if (size % Configuration::WORD_SIZE != 0)
      throw std::invalid_argument("Can only allocate in chunks of " +
                                  std::to_string(Configuration::WORD_SIZE) +
                                  " bytes!");

    int retval = mem.size();
    if (retval + size > heapSizeMax)
      throw std::out_of_range("Out of heap!");
    for (int i = 0; i < size; i++) {
      mem.push_back(r.getRandomNumber());
    }
    return retval;
  }

  /**
   * Allocate a specified amount of bytes on the heap and initialize them to 0.
   * @param size the number of bytes to be allocated
   * @return the starting address of the allocated region on the heap
   */
  int calloc(int size) {
    int retval = malloc(size);
    for (int i = (int)retval; i < retval + size; i++) {
      mem[i] = 0;
    }
    return retval;
  }

  /**
   * Read a value at the specified location on the heap
   * @param addr the address to be read
   * @return the value at {@code addr}
   */
  int read(int addr) {
    int i = getMemoryIndex(addr);
    if (i >= mem.size())
      throw std::out_of_range("Attempting to read past end of heap");
    return mem[i];
  }

  /**
   * Write a value at the specified location on the heap
   * @param addr the address to be written
   * @param value the value to be written
   */
  void store(int addr, int value) {
    int i = getMemoryIndex(addr);
    if (i >= mem.size())
      throw std::out_of_range("Attempting to store past end of heap");
    mem[i] = value;
  }

  /**
   * Simulate a function call
   * All arguments to the function call are passed via registers with prefix
   * {@link Configuration#ABSTRACT_ARG_PREFIX} and indices starting from 0.
   * @param name name of the function call
   * @param args arguments to the function call
   * @return the value that would be in register
   *          {@link Configuration#ABSTRACT_RET}
   */
  int call(const std::string &name, const std::vector<int> &args) {
    return call(std::make_shared<ExecutionFrame>(this, -1), name, args);
  }

  /**
   * Simulate a function call.
   * All arguments to the function call are passed via registers with prefix
   * {@link Configuration#ABSTRACT_ARG_PREFIX} and indices starting from 0.
   * The function call should return the result via register
   * {@link Configuration#ABSTRACT_RET}.
   * @param parent parent call frame to write _RET values to
   * @param name name of the function call
   * @param args arguments to the function call
   * @return the value of register
   *          {@link Configuration#ABSTRACT_RET}
   */
  int call(std::shared_ptr<ExecutionFrame> parent, const std::string &name,
           const std::vector<int> &args) {
    int ret;
    // Catch standard library calls.
    if (libraryFunctions.contains(name)) {
      ret = libraryCall(name, args);
    } else {
      std::shared_ptr<FuncDecl> fDecl = compUnit->getFunction(name);
      if (fDecl == nullptr)
        throw std::runtime_error("Tried to call an unknown function: '" + name +
                                 "'");
      if (debugLevel > 1) {
        std::cout << "Calling function " << fDecl->getName() << "\n";
      }
      // Create a new stack frame.
      int ip = findLabel(name);
      std::shared_ptr<ExecutionFrame> frame =
          std::make_shared<ExecutionFrame>(this, ip);

      // Pass the remaining arguments into registers.
      for (int i = 0; i < args.size(); ++i)
        frame->put(Configuration::ABSTRACT_ARG_PREFIX() + std::to_string(i),
                   args[i]);

      // Simulate!
      while (frame->advance())
        ;

      ret = frame->ret;
    }

    parent->put(Configuration::ABSTRACT_RET(), ret);
    return ret;
  }

  /** Private classes */
private:
  /**
   * Holds the instruction pointer and temporary registers
   * within an execution frame.
   */
  class ExecutionFrame : public std::enable_shared_from_this<ExecutionFrame> {
    friend class Simulator;
    Simulator *simulator;
    /** local registers (register name -> value) */
    std::unordered_map<std::string, int> regs;

  protected:
    /** instruction pointer */
    int ip;

    /** return value from this frame */
    int ret;

  public:
    ExecutionFrame(Simulator *simulator, int ip)
        : simulator(simulator), ip(ip) {
      regs = std::unordered_map<std::string, int>();
      ret = simulator->r.getRandomNumber();
    }

    /**
     * Fetch the value at the given register
     * @param tempName name of the register
     * @return the value at the given register
     */
    int get(std::string tempName) {
      if (!regs.count(tempName)) {
        /* Referencing a temp before having written to it - initialize
           with garbage */
        put(tempName, simulator->r.getRandomNumber());
      }
      return regs[tempName];
    }

    /**
     * Store a value into the given register
     * @param tempName name of the register
     * @param value value to be stored
     */
    void put(std::string tempName, int value) { regs[tempName] = value; }

    /**
     * Advance the instruction pointer. Since we're dealing with a tree,
     * this is postorder traversal, one step at a time, modulo jumps.
     */
    bool advance() {
      // TODO: Wut, only add if needed
      // Time out if necessary.
      // if (Thread.currentThread().isInterrupted())
      //   return false;
      if (debugLevel > 1)
        std::cout << "Evaluating " << getCurrentInsn()->label() << std::endl;
      int backupIP = ip;
      simulator->leave(shared_from_this());

      if (ip == -1)
        return false; /* RETURN */

      if (ip != backupIP) /* A jump was performed */
        return true;

      ip++;
      return true;
    }

    void setIP(int ip) {
      this->ip = ip;
      if (debugLevel > 1) {
        if (ip == -1)
          std::cout << "Returning" << std::endl;
        else
          std::cout << "Jumping to " << getCurrentInsn()->label() << std::endl;
      }
    }

    std::shared_ptr<Node> getCurrentInsn() {
      std::shared_ptr<Node> insn = simulator->indexToInsn[ip];
      if (insn == nullptr)
        throw std::runtime_error("No next instruction.  Forgot RETURN?");
      return insn;
    }
  };
  class StackItem {
  public:
    enum Kind { COMPUTED, MEM, TEMP, NAME };

    Kind type;
    int value;
    int addr;
    std::string temp;
    std::string name;

    StackItem(int value) : value(value) {
      type = Kind::COMPUTED;
      // this.value = value;
    }

    StackItem(int value, int addr) : value(value), addr(addr) {
      type = Kind::MEM;
      // this.value = value;
      // this.addr = addr;
    }

    StackItem(Kind type, int value, const std::string str)
        : type(type), value(value) {
      // this.type = type;
      // this.value = value;
      if (type == Kind::TEMP)
        temp = str;
      else if (type == Kind::NAME)
        name = str;
      else
        throw std::invalid_argument("Unknown StackItem Kind");
    }
  };
  /**
   * While traversing the IR tree, we require a stack in order to hold
   * a number of single-word values (e.g., to evaluate binary expressions).
   * This also keeps track of whether a value was created by a TEMP
   * or MEM, or NAME reference, which is useful when executing moves.
   */
  class ExprStack {
    std::stack<std::shared_ptr<StackItem>> st;

  public:
    ExprStack() { st = std::stack<std::shared_ptr<StackItem>>(); }

    int popValue() {
      int value = st.top()->value;
      st.pop();
      if (debugLevel > 1)
        std::cout << "Popping value " << value << std::endl;
      return value;
    }

    std::shared_ptr<StackItem> pop() {
      auto ret = st.top();
      st.pop();
      return ret;
    }

    void pushAddr(int value, int addr) {
      if (debugLevel > 1)
        std::cout << "Pushing MEM " << value << " (" << addr << ")"
                  << std::endl;
      st.push(std::make_shared<StackItem>(value, addr));
    }

    void pushTemp(int value, const std::string &temp) {
      if (debugLevel > 1)
        std::cout << "Pushing TEMP " << value << " (" << temp << ")"
                  << std::endl;
      st.push(std::make_shared<StackItem>(StackItem::Kind::TEMP, value, temp));
    }

    void pushName(int value, const std::string &name) {
      if (debugLevel > 1)
        std::cout << "Pushing NAME " << value << " (" << name << ")"
                  << std::endl;
      st.push(std::make_shared<StackItem>(StackItem::Kind::NAME, value, name));
    }

    void pushValue(int value) {
      if (debugLevel > 1)
        std::cout << "Pushing value " << value << std::endl;
      st.push(std::make_shared<StackItem>(value));
    }
  };
};
} // namespace tir
