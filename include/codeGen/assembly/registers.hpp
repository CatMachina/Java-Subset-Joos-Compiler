#pragma once

namespace codegen::assembly {

// 8-bit registers

// Accumulator (low or high byte)
inline const std::string R8_AL = "al";
inline const std::string R8_AH = "ah";
// Base register (low or high byte)
inline const std::string R8_BL = "bl";
inline const std::string R8_BH = "bh";
// Counter (low or high byte), often used in shifts/loops
inline const std::string R8_CL = "cl";
inline const std::string R8_CH = "ch";
// Data register (low or high byte)
inline const std::string R8_DL = "dl";
inline const std::string R8_DH = "dh";
// Stack pointer (low)
inline const std::string R8_SPL = "spl";
// Base pointer (low)
inline const std::string R8_BPL = "bpl";
// Source index register (low)
inline const std::string R8_SIL = "sil";
// Destination index register (low)
inline const std::string R8_DIL = "dil";

// 16-bit general-purpose registers

// Accumulator register
inline const std::string R16_AX = "ax";
// Base register (often used for memory addressing)
inline const std::string R16_BX = "bx";
// Counter register (used in loops, shifts)
inline const std::string R16_CX = "cx";
// Data register (used in division, I/O)
inline const std::string R16_DX = "dx";
// Stack pointer
inline const std::string R16_SP = "sp";
// Base pointer (used for stack frame access)
inline const std::string R16_BP = "bp";
// Source index (used in string/memory operations)
inline const std::string R16_SI = "si";
// Destination index (used in string/memory operations)
inline const std::string R16_DI = "di";

// 32-bit general-purpose registers (extended 16-bit registers)

// Extended Accumulator
inline const std::string R32_EAX = "eax";
// Extended Base
inline const std::string R32_EBX = "ebx";
// Extended Counter
inline const std::string R32_ECX = "ecx";
// Extended Data
inline const std::string R32_EDX = "edx";
// Extended Stack Pointer
inline const std::string R32_ESP = "esp";
// Extended Base Pointer
inline const std::string R32_EBP = "ebp";
// Extended Source Index
inline const std::string R32_ESI = "esi";
// Extended Destination Index
inline const std::string R32_EDI = "edi";

// Holds address of next instruction to execute
inline const std::string R32_EIP = "eip";

inline const std::unordered_set<std::string> ALL_GPR = {
    // 8-bit
    "al",
    "ah",
    "bl",
    "bh",
    "cl",
    "ch",
    "dl",
    "dh",
    "spl",
    "bpl",
    "sil",
    "dil",

    // 16-bit
    "ax",
    "bx",
    "cx",
    "dx",
    "sp",
    "bp",
    "si",
    "di",

    // 32-bit
    "eax",
    "ebx",
    "ecx",
    "edx",
    "esp",
    "ebp",
    "esi",
    "edi",
};

inline bool isGPR(const std::string &reg) { return ALL_GPR.contains(reg); }

// Groups of overlapping registers by logical register family (e.g., EAX
// includes AX, AL, AH)
inline const std::unordered_map<std::string, std::unordered_set<std::string>>
    REG_ALIAS_GROUPS = {
        {"eax", {"eax", "ax", "al", "ah"}}, {"ebx", {"ebx", "bx", "bl", "bh"}},
        {"ecx", {"ecx", "cx", "cl", "ch"}}, {"edx", {"edx", "dx", "dl", "dh"}},
        {"esi", {"esi", "si", "sil"}},      {"edi", {"edi", "di", "dil"}},
        {"esp", {"esp", "sp", "spl"}},      {"ebp", {"ebp", "bp", "bpl"}},
};

/*
auto it = REG_TO_ALIAS_GROUP.find("al");
if (it != REG_TO_ALIAS_GROUP.end()) {
    for (const auto& alias : *it->second) {
        std::cout << alias << "\n";
    }
}
*/
inline std::unordered_map<std::string, const std::unordered_set<std::string> *>
    REG_TO_ALIAS_GROUP = [] {
      std::unordered_map<std::string, const std::unordered_set<std::string> *>
          map;
      for (const auto &[group_leader, group] : REG_ALIAS_GROUPS) {
        for (const auto &reg : group) {
          map[reg] = &group;
        }
      }
      return map;
    }();

} // namespace codegen::assembly
