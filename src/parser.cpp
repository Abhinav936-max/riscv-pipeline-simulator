#include "parser.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace riscvsim {

namespace {
std::string trim(const std::string& s) {
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    std::istringstream in(line);
    while (in >> cur) out.push_back(cur);
    return out;
}

int parseReg(const std::string& s) {
    if (s.size() < 2 || s[0] != 'x') throw std::runtime_error("bad register: " + s);
    return std::stoi(s.substr(1));
}

int parseImm(const std::string& s) { return std::stoi(s); }

Instr make(Op op, const std::string& txt) {
    Instr i;
    i.op = op;
    i.text = txt;
    return i;
}
} // namespace

std::string regName(int r) {
    return "x" + std::to_string(r);
}

Program parseProgram(const std::string& source) {
    std::istringstream input(source);
    std::string line;
    std::unordered_map<std::string, int> labels;
    std::vector<std::pair<int, std::vector<std::string>>> raw;

    int pc = 0;
    while (std::getline(input, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        line = trim(line);
        if (line.empty()) continue;
        if (line.back() == ':') {
            labels[line.substr(0, line.size() - 1)] = pc;
            continue;
        }
        raw.push_back({pc++, tokenize(line)});
    }

    Program p;
    p.code.reserve(raw.size());
    for (const auto& [idx, t] : raw) {
        if (t.empty()) continue;
        auto join = [&](size_t from) {
            std::string s;
            for (size_t i = from; i < t.size(); ++i) {
                if (i > from) s += ' ';
                s += t[i];
            }
            return s;
        };
        Instr ins = make(Op::Nop, join(0));
        const std::string& op = t[0];
        if (op == "add") { ins.op = Op::Add; ins.rd = parseReg(t[1]); ins.rs1 = parseReg(t[2]); ins.rs2 = parseReg(t[3]); }
        else if (op == "sub") { ins.op = Op::Sub; ins.rd = parseReg(t[1]); ins.rs1 = parseReg(t[2]); ins.rs2 = parseReg(t[3]); }
        else if (op == "addi") { ins.op = Op::Addi; ins.rd = parseReg(t[1]); ins.rs1 = parseReg(t[2]); ins.imm = parseImm(t[3]); }
        else if (op == "lw") {
            ins.op = Op::Lw; ins.rd = parseReg(t[1]);
            auto pos1 = t[2].find('('); auto pos2 = t[2].find(')');
            ins.imm = parseImm(t[2].substr(0, pos1));
            ins.rs1 = parseReg(t[2].substr(pos1 + 1, pos2 - pos1 - 1));
        }
        else if (op == "sw") {
            ins.op = Op::Sw; ins.rs2 = parseReg(t[1]);
            auto pos1 = t[2].find('('); auto pos2 = t[2].find(')');
            ins.imm = parseImm(t[2].substr(0, pos1));
            ins.rs1 = parseReg(t[2].substr(pos1 + 1, pos2 - pos1 - 1));
        }
        else if (op == "beq") { ins.op = Op::Beq; ins.rs1 = parseReg(t[1]); ins.rs2 = parseReg(t[2]); ins.label = t[3]; }
        else if (op == "bne") { ins.op = Op::Bne; ins.rs1 = parseReg(t[1]); ins.rs2 = parseReg(t[2]); ins.label = t[3]; }
        else if (op == "jal") { ins.op = Op::Jal; ins.rd = parseReg(t[1]); ins.label = t[2]; }
        else if (op == "halt") { ins.op = Op::Halt; }
        else throw std::runtime_error("unknown opcode: " + op);
        p.code.push_back(ins);
    }

    for (auto& ins : p.code) {
        if (!ins.label.empty()) {
            auto it = labels.find(ins.label);
            if (it == labels.end()) throw std::runtime_error("unknown label: " + ins.label);
            ins.imm = it->second;
        }
    }
    return p;
}

} // namespace riscvsim
