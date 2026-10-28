# Single-Cycle & Pipelined RISC-V Processor Simulator

A small C++17 simulator for a tiny RV32I-style subset:

- `add`, `sub`, `addi`
- `lw`, `sw`
- `beq`, `bne`, `jal`
- `halt`

It includes:

- a single-cycle execution mode
- a 5-stage pipeline model
- forwarding for data hazards
- one-cycle stall for load-use hazards
- branch flush in the EX stage

## Build

```bash
make
```

## Run

```bash
./riscvsim
```

Or pass a program file:

```bash
./riscvsim programs/demo.asm
```

## What to explain in interview

- Why pipelining improves throughput
- What a data hazard is
- Why load-use needs a stall
- What forwarding solves
- Why branches cause flushes
