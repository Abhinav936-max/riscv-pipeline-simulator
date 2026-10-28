#include "simulator.h"
#include "parser.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace riscvsim {

Simulator::Simulator(Program program)
    : program_(std::move(program)), regs_(32, 0) {}

int Simulator::readReg(int r) const {
    if (r == 0) return 0;
    return regs_[r];
}

void Simulator::writeReg(int r, int v) {
    if (r != 0) regs_[r] = v;
}

int Simulator::loadMem(int addr) const {
    auto it = mem_.find(addr);
    return it == mem_.end() ? 0 : it->second;
}

void Simulator::storeMem(int addr, int v) {
    mem_[addr] = v;
}

void Simulator::printStep(const std::string& mode, int cycle, int pc, const Instr& ins) const {
    std::cout << '[' << mode << "] cycle " << cycle << " pc=" << pc << " -> " << ins.text << '\n';
}

void Simulator::runSingleCycle(int maxSteps) {
    halted_ = false;
    pc_ = 0;
    int steps = 0;
    while (!halted_ && pc_ >= 0 && pc_ < static_cast<int>(program_.code.size()) && steps < maxSteps) {
        const auto& ins = program_.code[pc_];
        printStep("single", steps + 1, pc_, ins);
        switch (ins.op) {
            case Op::Add: writeReg(ins.rd, readReg(ins.rs1) + readReg(ins.rs2)); ++pc_; break;
            case Op::Sub: writeReg(ins.rd, readReg(ins.rs1) - readReg(ins.rs2)); ++pc_; break;
            case Op::Addi: writeReg(ins.rd, readReg(ins.rs1) + ins.imm); ++pc_; break;
            case Op::Lw: writeReg(ins.rd, loadMem(readReg(ins.rs1) + ins.imm)); ++pc_; break;
            case Op::Sw: storeMem(readReg(ins.rs1) + ins.imm, readReg(ins.rs2)); ++pc_; break;
            case Op::Beq: pc_ = (readReg(ins.rs1) == readReg(ins.rs2)) ? ins.imm : pc_ + 1; break;
            case Op::Bne: pc_ = (readReg(ins.rs1) != readReg(ins.rs2)) ? ins.imm : pc_ + 1; break;
            case Op::Jal: writeReg(ins.rd, pc_ + 1); pc_ = ins.imm; break;
            case Op::Halt: halted_ = true; break;
            case Op::Nop: ++pc_; break;
        }
        ++steps;
    }
}

int Simulator::getForwarded(int reg, const PipeReg& exmem, const PipeReg& memwb) const {
    if (reg == 0) return 0;
    if (exmem.valid && exmem.instr.rd == reg && exmem.instr.op != Op::Sw && exmem.instr.op != Op::Beq && exmem.instr.op != Op::Bne)
        return exmem.alu;
    if (memwb.valid && memwb.instr.rd == reg && memwb.instr.op != Op::Sw && memwb.instr.op != Op::Beq && memwb.instr.op != Op::Bne)
        return memwb.mem;
    return regs_[reg];
}

bool Simulator::isPipelineEmpty() const { return false; }

void Simulator::runPipeline(int maxCycles) {
    halted_ = false;
    pc_ = 0;

    PipeReg ifid, idex, exmem, memwb;
    int cycles = 0;
    bool fetchDone = false;

    auto fetchInstr = [&](int pc) -> PipeReg {
        PipeReg p;
        if (pc >= 0 && pc < static_cast<int>(program_.code.size())) {
            p.valid = true;
            p.instr = program_.code[pc];
            p.pc = pc;
        }
        return p;
    };

    auto pipeEmpty = [&]() {
        return !ifid.valid && !idex.valid && !exmem.valid && !memwb.valid && (fetchDone || pc_ >= static_cast<int>(program_.code.size()));
    };

    while (cycles < maxCycles && !pipeEmpty()) {
        ++cycles;

        // Write-back stage
        if (memwb.valid) {
            const auto& ins = memwb.instr;
            if (ins.op == Op::Add || ins.op == Op::Sub || ins.op == Op::Addi || ins.op == Op::Lw || ins.op == Op::Jal) {
                writeReg(ins.rd, memwb.mem);
            }
        }

        // Memory stage -> next WB
        PipeReg nextMemwb;
        if (exmem.valid) {
            nextMemwb = exmem;
            const auto& ins = exmem.instr;
            if (ins.op == Op::Lw) nextMemwb.mem = loadMem(exmem.alu);
            else if (ins.op == Op::Sw) storeMem(exmem.alu, exmem.rs2Val);
            else nextMemwb.mem = exmem.alu;
        }

        // Execute stage -> next MEM
        PipeReg nextExmem;
        bool branchTaken = false;
        int branchTarget = -1;
        if (idex.valid) {
            nextExmem = idex;
            const auto& ins = idex.instr;
            nextExmem.rs1Val = getForwarded(ins.rs1, exmem, memwb);
            nextExmem.rs2Val = getForwarded(ins.rs2, exmem, memwb);
            switch (ins.op) {
                case Op::Add: nextExmem.alu = nextExmem.rs1Val + nextExmem.rs2Val; break;
                case Op::Sub: nextExmem.alu = nextExmem.rs1Val - nextExmem.rs2Val; break;
                case Op::Addi: nextExmem.alu = nextExmem.rs1Val + ins.imm; break;
                case Op::Lw:
                case Op::Sw: nextExmem.alu = nextExmem.rs1Val + ins.imm; break;
                case Op::Beq:
                    if (nextExmem.rs1Val == nextExmem.rs2Val) { branchTaken = true; branchTarget = ins.imm; }
                    break;
                case Op::Bne:
                    if (nextExmem.rs1Val != nextExmem.rs2Val) { branchTaken = true; branchTarget = ins.imm; }
                    break;
                case Op::Jal:
                    nextExmem.alu = nextExmem.pc + 1;
                    branchTaken = true;
                    branchTarget = ins.imm;
                    break;
                case Op::Halt:
                    halted_ = true;
                    break;
                case Op::Nop:
                    break;
            }
        }

        // Hazard detection: load-use stall
        bool stall = false;
        if (ifid.valid && idex.valid && idex.instr.op == Op::Lw) {
            int rd = idex.instr.rd;
            const auto& ins = ifid.instr;
            if (rd != 0 && (ins.rs1 == rd || ins.rs2 == rd)) stall = true;
        }

        // Decode/fetch -> next ID, next IF
        PipeReg nextIdex;
        PipeReg nextIfid;
        if (branchTaken) {
            nextIdex = {};            // flush younger instruction in ID
            nextIfid = {};            // flush younger instruction in IF
            pc_ = branchTarget;
            fetchDone = false;
        } else if (stall) {
            nextIdex = {};            // bubble in EX next cycle
            nextIfid = ifid;          // hold IF/ID
            fetchDone = false;
        } else {
            nextIdex = ifid;
            if (pc_ < static_cast<int>(program_.code.size())) {
                nextIfid = fetchInstr(pc_++);
            } else {
                fetchDone = true;
            }
        }

        if (branchTaken) {
            if (pc_ < static_cast<int>(program_.code.size())) nextIfid = fetchInstr(pc_);
        }

        // If we stalled, don't fetch a new instruction
        if (stall) {
            nextIfid = ifid;
        }

        if (halted_ && !ifid.valid && !idex.valid && !exmem.valid && !memwb.valid) break;

        memwb = nextMemwb;
        exmem = nextExmem;
        idex = nextIdex;
        ifid = nextIfid;

        printStep("pipe", cycles, pc_, exmem.valid ? exmem.instr : Instr{});
    }
}
void Simulator::dumpState() const {
    std::cout << "registers:\n";
    for (int i = 0; i < 32; ++i) {
        std::cout << "x" << i << "=" << regs_[i] << ((i % 8 == 7) ? '\n' : ' ');
    }
    std::cout << "memory:\n";
    for (const auto& [addr, val] : mem_) std::cout << addr << " -> " << val << '\n';
}

} // namespace riscvsim
