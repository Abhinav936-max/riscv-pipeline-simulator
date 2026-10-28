#include "parser.h"
#include "simulator.h"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace riscvsim;

static std::string loadFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open file: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    try {
        std::string source;
        if (argc > 1) source = loadFile(argv[1]);
        else {
            source = R"(
                addi x1 x0 5
                addi x2 x0 1
            loop:
                beq x1 x0 done
                add x2 x2 x1
                addi x1 x1 -1
                jal x0 loop
            done:
                halt
            )";
        }

        auto program = parseProgram(source);
        Simulator sim(program);
        std::cout << "--- single-cycle ---\n";
        sim.runSingleCycle();
        sim.dumpState();

        std::cout << "--- pipelined ---\n";
        Simulator sim2(program);
        sim2.runPipeline();
        sim2.dumpState();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
