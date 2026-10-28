#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace riscvsim {

enum class Op { Add, Sub, Addi, Lw, Sw, Beq, Bne, Jal, Halt, Nop };

struct Instr {
    Op op = Op::Nop;
    int rd = 0;
    int rs1 = 0;
    int rs2 = 0;
    int imm = 0;
    std::string label;
    std::string text;
};

struct Program {
    std::vector<Instr> code;
};

} // namespace riscvsim
