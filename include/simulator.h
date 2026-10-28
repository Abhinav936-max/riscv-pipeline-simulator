#pragma once
#include "instruction.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace riscvsim {

class Simulator {
public:
    explicit Simulator(Program program);
    void runSingleCycle(int maxSteps = 1000);
    void runPipeline(int maxCycles = 1000);
    void dumpState() const;

private:
    struct PipeReg {
        bool valid = false;
        Instr instr;
        int pc = 0;
        int rs1Val = 0;
        int rs2Val = 0;
        int alu = 0;
        int mem = 0;
    };

    int readReg(int r) const;
    void writeReg(int r, int v);
    int loadMem(int addr) const;
    void storeMem(int addr, int v);
    int getForwarded(int reg, const PipeReg& exmem, const PipeReg& memwb) const;
    bool isPipelineEmpty() const;
    void printStep(const std::string& mode, int cycle, int pc, const Instr& ins) const;

    Program program_;
    std::vector<int> regs_;
    std::map<int, int> mem_;
    int pc_ = 0;
    bool halted_ = false;
};

} // namespace riscvsim
