#pragma once
#include "instruction.h"

#include <string>

namespace riscvsim {

Program parseProgram(const std::string& source);
std::string regName(int r);

} // namespace riscvsim
