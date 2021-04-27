#include <ir/ir.h>
#include <target/util.h>

#include <ir/table.h>

//=============================================================
// Configurations
//=============================================================

#define QFTASM_RAM_AS_STDIN_BUFFER
#define QFTASM_RAM_AS_STDOUT_BUFFER

#define QFTASM_ABSOLUTE_RAM_ADDRESS

#ifdef QFTASM_RAM_AS_STDIN_BUFFER
// static const int QFTASM_RAMSTDIN_BUF_STARTPOSITION = 7167;
// static const int QFTASM_RAMSTDIN_BUF_STARTPOSITION = 3475;
static const int QFTASM_RAMSTDIN_BUF_STARTPOSITION = 1310;
// static const int QFTASM_RAMSTDIN_BUF_STARTPOSITION = 125-64;
#endif

#ifdef QFTASM_RAM_AS_STDOUT_BUFFER
// static const int QFTASM_RAMSTDOUT_BUF_STARTPOSITION = 8191;
// static const int QFTASM_RAMSTDOUT_BUF_STARTPOSITION = 3975;
static const int QFTASM_RAMSTDOUT_BUF_STARTPOSITION = 1846;
// static const int QFTASM_RAMSTDOUT_BUF_STARTPOSITION = 127-64;
#endif

// RAM pointer offset to prevent negative-value RAM addresses
// from underflowing into the register regions
// static const int QFTASM_MEM_OFFSET = 2048;
// static const int QFTASM_MEM_OFFSET = 1024;
static const int QFTASM_MEM_OFFSET = 0;
// This is required to run tests on ELVM,
// since the Makefile compiles all files at first
#define QFTASM_SUPPRESS_MEMORY_INIT_OVERFLOW_ERROR

//=============================================================
static const int QFTASM_PC = 0;
static const int QFTASM_STDIN = 1;
static const int QFTASM_STDOUT = 2;
static const int QFTASM_A = 3;
static const int QFTASM_B = 4;
static const int QFTASM_C = 5;
static const int QFTASM_D = 6;
static const int QFTASM_BP = 7;
static const int QFTASM_SP = 8;
static const int QFTASM_TEMP = 9;
static const int QFTASM_TEMP_2 = 10;

// #ifdef QFTASM_JMPTABLE_IN_ROM
// // static const int QFTASM_JMPTABLE_OFFSET = 0;
// // static const int QFTASM_JMPTABLE_OFFSET = 4;
// #else
// static const int QFTASM_JMPTABLE_OFFSET = 11;
// #endif

#define QFTASM_STDIO_OPEN (1 << 8)
#define QFTASM_STDIO_CLOSED (1 << 9)

static void qftasm_emit_line(const char* fmt, ...) {
  static int qftasm_lineno_counter_ = 0;
  if (qftasm_lineno_counter_ > 0) {
    printf("\n");
  }
  printf("%d. ", qftasm_lineno_counter_);
  qftasm_lineno_counter_++;

  if (fmt[0]) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  }
}

static int qftasm_int24_to_int16(int x) {
  // Interpret x as a 24-bit signed integer.
  // If it is negative, then reinterpret x as a 16-bit signed integer.
  if (x < 0) {
    x += (1 << 16);
  } else if (x > (1 << 23)) {
    x = x - (1 << 24) + (1 << 16);
  }
  x = x & ((1 << 16) - 1);
  return x;
}

static void qftasm_emit_memory_initialization(Data* data) {
  // RAM initialization
  int written_memory_table = 0;
  for (int mp = 0; data; data = data->next, mp++) {
    if (data->v) {
#ifndef QFTASM_SUPPRESS_MEMORY_INIT_OVERFLOW_ERROR
      if (mp + QFTASM_MEM_OFFSET>= (1 << 16)) {
        error("Memory pointer overflow occured at memory initialization: Address %d", mp + QFTASM_MEM_OFFSET);
      }
#endif
      qftasm_emit_line("MNZ 32768 %d %d;%s",
                       qftasm_int24_to_int16(data->v), mp + QFTASM_MEM_OFFSET,
                       !written_memory_table ? " Memory table" : "");
      written_memory_table = 1;
    }
  }
}

Data* initdata;
static void init_state_qftasm(Data* data, Inst* init_inst) {
  initdata = data;
    // qftasm_emit_line("MNZ 0 0 0; pc == %d:", pc);
  // stdin, stdout
#ifdef QFTASM_RAM_AS_STDIN_BUFFER
  // qftasm_emit_line("MNZ 32768 %d %d; Register initialization (stdin buffer pointer)", QFTASM_RAMSTDIN_BUF_STARTPOSITION*2+1, QFTASM_STDIN);
  qftasm_emit_line("MNZ 32768 %d %d; Register initialization (stdin buffer pointer)", QFTASM_RAMSTDIN_BUF_STARTPOSITION*2, QFTASM_STDIN);
#else
  qftasm_emit_line("MNZ 32768 %d %d; Register initialization (stdin)", QFTASM_STDIO_CLOSED, QFTASM_STDIN);
#endif
#ifdef QFTASM_RAM_AS_STDOUT_BUFFER
  qftasm_emit_line("MNZ 32768 %d %d; Register initialization (stdout buffer pointer)", QFTASM_RAMSTDOUT_BUF_STARTPOSITION, QFTASM_STDOUT);
#else
  qftasm_emit_line("MNZ 32768 %d %d; Register initialization (stdout)", QFTASM_STDIO_CLOSED, QFTASM_STDOUT);
#endif
  qftasm_emit_memory_initialization(initdata);

  if (init_inst) {
    return;
  }
}

static int qftasm_reg2addr(Reg reg) {
  switch (reg) {
    case A: return QFTASM_A;
    case B: return QFTASM_B;
    case C: return QFTASM_C;
    case D: return QFTASM_D;
    case BP: return QFTASM_BP;
    case SP: return QFTASM_SP;
    default:
      error("Undefined register name in qftasm_reg2addr: %d", reg);
  }
}

static const char* qftasm_value_str(Value* v) {
  if (v->type == REG) {
    return format("A%d", qftasm_reg2addr(v->reg));
  } else if (v->type == IMM) {
    return format("%d", qftasm_int24_to_int16(v->imm));
  } else {
    return format("{%s}", v->tmp);
    // error("invalid value");
  }
}

static const char* qftasm_src_str(Inst* inst) {
  return qftasm_value_str(&inst->src);
}

static const char* qftasm_dst_str(Inst* inst) {
  return qftasm_value_str(&inst->dst);
}

static void qftasm_emit_conditional_jmp_inst(Inst* inst) {
  Value* v = &inst->jmp;
  if (v->type == REG) {
    qftasm_emit_line("MNZ A%d A%d %d; (reg)", QFTASM_TEMP, qftasm_reg2addr(v->reg), QFTASM_PC);

  } else if (v->type == IMM) {
    qftasm_emit_line("MNZ A%d {pc%d} %d; (imm)", QFTASM_TEMP, v->imm, QFTASM_PC);
  } else {
    qftasm_emit_line("MNZ A%d {%s} %d; (imm)", QFTASM_TEMP, v->tmp, QFTASM_PC);
  //  error("Invalid value at conditional jump");
  }
}

static void qftasm_emit_unconditional_jmp_inst(Inst* inst) {
  Value* v = &inst->jmp;
  if (v->type == REG) {
    qftasm_emit_line("MNZ 32768 A%d %d; JMP (reg)", qftasm_reg2addr(v->reg), QFTASM_PC);
  } else if (v->type == IMM) {
    qftasm_emit_line("MNZ 32768 {pc%d} %d; JMP (imm)", v->imm, QFTASM_PC);
  } else {
    qftasm_emit_line("MNZ 32768 {%s} %d; JMP (imm)", v->tmp, QFTASM_PC);
    // error("Invalid value at JMP");
  }
}

static void qftasm_emit_load_inst(Inst* inst) {
#ifdef QFTASM_ABSOLUTE_RAM_ADDRESS
  if (inst->src.type == REG) {
    qftasm_emit_line("MNZ 32768 B%d %d; LOAD (reg)",
                      qftasm_reg2addr(inst->src.reg), qftasm_reg2addr(inst->dst.reg));
  } else if (inst->src.type == IMM) {
    qftasm_emit_line("MNZ 32768 A%d %d; LOAD (imm)",
                      qftasm_int24_to_int16(inst->src.imm), qftasm_reg2addr(inst->dst.reg));
  } else {
    qftasm_emit_line("MNZ 32768 {%s} %d; LOAD (imm)",
                      inst->src.tmp, qftasm_reg2addr(inst->dst.reg));
    // error("Invalid value at LOAD");
  }
#else
  if (inst->src.type == REG) {
    qftasm_emit_line("ADD A%d %d %d; LOAD (reg)",
                      qftasm_reg2addr(inst->src.reg), QFTASM_MEM_OFFSET, QFTASM_TEMP);
    qftasm_emit_line("MNZ 32768 B%d %d;",
                      QFTASM_TEMP, qftasm_reg2addr(inst->dst.reg));
  } else if (inst->src.type == IMM) {
    qftasm_emit_line("MNZ 32768 A%d %d; LOAD (imm)",
                      qftasm_int24_to_int16(inst->src.imm) + QFTASM_MEM_OFFSET, qftasm_reg2addr(inst->dst.reg));
  } else {
    qftasm_emit_line("MNZ 32768 {%s} %d; LOAD (imm)",
                      inst->src.tmp, qftasm_reg2addr(inst->dst.reg));
    // error("Invalid value at LOAD");
  }
#endif
}

static void qftasm_emit_jne(Inst* inst, Value* v) {
  if (inst->src.type == IMM & inst->src.imm == 0) {
    // Value* v = &inst->jmp;
    if (v->type == REG) {
      qftasm_emit_line("MNZ A%d A%d %d; JNE (with 0, reg)", qftasm_reg2addr(inst->dst.reg), qftasm_reg2addr(v->reg), QFTASM_PC);
    } else if (v->type == IMM) {
      qftasm_emit_line("MNZ A%d {pc%d} %d; JNE (with 0, imm)", qftasm_reg2addr(inst->dst.reg), v->imm, QFTASM_PC);
    } else {
      qftasm_emit_line("MNZ A%d {%s} %d; JNE (with 0, imm)", qftasm_reg2addr(inst->dst.reg), v->tmp, QFTASM_PC);
    //  error("Invalid value at conditional jump");
    }
  } else {
    qftasm_emit_line("XOR %s A%d %d; JNE",
                    qftasm_src_str(inst), qftasm_reg2addr(inst->dst.reg), QFTASM_TEMP);
    qftasm_emit_conditional_jmp_inst(inst);
  }
}

static void qftasm_emit_store_inst(Inst* inst) {
#ifdef QFTASM_ABSOLUTE_RAM_ADDRESS
  // Here, "src" and "dst" have opposite meanings from their names
  if (inst->src.type == REG) {
    qftasm_emit_line("MNZ 32768 A%d A%d; STORE (reg)",
                      qftasm_reg2addr(inst->dst.reg), qftasm_reg2addr(inst->src.reg));
  } else if (inst->src.type == IMM) {
    qftasm_emit_line("MNZ 32768 A%d %d; STORE (imm)",
                      qftasm_reg2addr(inst->dst.reg),
                      qftasm_int24_to_int16(inst->src.imm));
  } else {
    qftasm_emit_line("MNZ 32768 {%s} %d; STORE (imm)",
                      inst->dst.tmp,
                      qftasm_int24_to_int16(inst->src.imm));
    // error("Invalid value at STORE");
  }
#else
  // Here, "src" and "dst" have opposite meanings from their names
  if (inst->src.type == REG) {
    qftasm_emit_line("ADD A%d %d %d; STORE (reg)",
                      qftasm_reg2addr(inst->src.reg), QFTASM_MEM_OFFSET, QFTASM_TEMP);
    qftasm_emit_line("MNZ 32768 A%d A%d;",
                      qftasm_reg2addr(inst->dst.reg), QFTASM_TEMP);
  } else if (inst->src.type == IMM) {
    // qftasm_emit_line("MNZ 32768 A%d %d; STORE (imm)",
    //                   qftasm_reg2addr(inst->dst.reg),
    //                   qftasm_int24_to_int16(inst->src.imm) + QFTASM_MEM_OFFSET);
  } else {
    // TODO: fix
    qftasm_emit_line("MNZ 32768 {%s} %d; STORE (imm)",
                      inst->dst.tmp,
                      qftasm_int24_to_int16(inst->src.imm) + QFTASM_MEM_OFFSET);
    // error("Invalid value at STORE");
  }
#endif
}

static void qftasm_emit_func_prologue(int func_id) {
  // Placeholder code that does nothing, to suppress compilation errors
  if (func_id) {
    return;
  }
}

static void qftasm_emit_func_epilogue(void) {
}


static Inst* qftasm_prev_inst;

static void qftasm_emit_pc_change(int pc) {
  // The comments in this line are required for post-processing step in ./tools/qftasm_pp.py
  // qftasm_emit_line("MNZ 0 0 0; pc == %d:", pc);

  // qftasm_emit_line("MNZ 0 0 0; pc == %d:", pc);
  if (pc > 0 && qftasm_prev_inst) {
    switch (qftasm_prev_inst->op) {
      case JEQ:
      case JNE:
      case JLT:
      case JGT:
      case JLE:
      case JGE:
      case JMP:
        qftasm_emit_line("MNZ 0 0 0;");
        break;
      default:
        break;
    }
  }
  printf(" pc == %d:", pc);
}


int qftasm_opt_skip_count = 0;
static int qftasm_init_optimization (Inst* inst) {
  if (
    inst->op == MOV
    && inst->next && inst->next->op == ADD
    && inst->dst.reg == inst->next->dst.reg
  ) {
    qftasm_emit_line("ADD %s %s %d; MOV-ADD",
      qftasm_src_str(inst), qftasm_src_str(inst->next), qftasm_reg2addr(inst->next->dst.reg));
    qftasm_opt_skip_count = 1;
    return 1;
  // } else if (
  //   inst->op == MOV
  //   && inst->next && inst->next->op == SUB
  //   && inst->dst.reg == inst->next->dst.reg
  // ) {
  //   qftasm_emit_line("SUB %s %s %d; MOV-SUB",
  //     qftasm_src_str(inst), qftasm_src_str(inst->next), qftasm_reg2addr(inst->next->dst.reg));
  //   qftasm_opt_skip_count = 1;
  //   return 1;
  } else if (
    inst->op == LOAD
    && inst->next && inst->next->op == MOV
    && inst->next->next && inst->next->next->op == LOAD
    && inst->src.type == REG
    && inst->next->next->src.type == REG
    && inst->src.reg == inst->next->dst.reg
    && inst->dst.reg == inst->next->src.reg
    && inst->src.reg == inst->next->next->src.reg
    && inst->dst.reg == inst->next->next->dst.reg
  ) {
    qftasm_emit_line("MNZ 32768 B%d %d; LOAD-MOV-LOAD (triplet)",
                     qftasm_reg2addr(inst->src.reg),
                     qftasm_reg2addr(inst->src.reg));
    qftasm_emit_line("MNZ 32768 B%d %d;",
                     qftasm_reg2addr(inst->src.reg),
                     qftasm_reg2addr(inst->dst.reg));
    qftasm_opt_skip_count = 2;
    return 1;
  } else if (
    inst->op == LOAD
    && inst->next && inst->next->op == MOV
    && inst->next->next && inst->next->next->op == LOAD
    && inst->src.type == REG
    && inst->next->next->src.type == REG
    && inst->src.reg == inst->next->dst.reg
    && inst->dst.reg == inst->next->src.reg
    // && inst->src.reg == inst->next->next->src.reg
    && inst->dst.reg == inst->next->next->dst.reg
  ) {
    qftasm_emit_line("MNZ 32768 B%d %d; LOAD-MOV-LOAD",
                     qftasm_reg2addr(inst->src.reg),
                     qftasm_reg2addr(inst->src.reg));
    qftasm_opt_skip_count = 1;
    return 1;
  }
  return 0;
}

static void qftasm_emit_inst(Inst* inst) {
  if (qftasm_opt_skip_count > 0) {
    qftasm_opt_skip_count--;
    return;
  }
  int optimized = qftasm_init_optimization(inst);
  if (!optimized) {
    switch (inst->op) {
    case MOV:
      qftasm_emit_line("MNZ 32768 %s %d; MOV",
                      qftasm_src_str(inst), qftasm_reg2addr(inst->dst.reg));
      break;

    case ADD:
      qftasm_emit_line("ADD A%d %s %d; ADD",
                      qftasm_reg2addr(inst->dst.reg),
                      qftasm_src_str(inst),
                      qftasm_reg2addr(inst->dst.reg));
      break;

    case SUB:
      qftasm_emit_line("SUB A%d %s %d; SUB",
                      qftasm_reg2addr(inst->dst.reg),
                      qftasm_src_str(inst),
                      qftasm_reg2addr(inst->dst.reg));
      // qftasm_emit_line("XOR %s 65535 %d; SUB", qftasm_src_str(inst), QFTASM_TEMP);
      // qftasm_emit_line("ADD 1 A%d %d;", QFTASM_TEMP, QFTASM_TEMP);
      // qftasm_emit_line("ADD A%d A%d %d;", QFTASM_TEMP, qftasm_reg2addr(inst->dst.reg), qftasm_reg2addr(inst->dst.reg));
      break;

    case LOAD:
      qftasm_emit_load_inst(inst);
      break;

    case STORE:
      qftasm_emit_store_inst(inst);
      break;

    case PUTC:
  #ifdef QFTASM_RAM_AS_STDOUT_BUFFER
      qftasm_emit_line("MNZ 32768 %s A%d; PUTC", qftasm_src_str(inst), QFTASM_STDOUT);
      qftasm_emit_line("SUB A%d 1 %d;", QFTASM_STDOUT, QFTASM_STDOUT);
      qftasm_emit_line("MNZ 32768 0 A%d;", QFTASM_STDOUT);
  #else
      qftasm_emit_line("MNZ 32768 %s %d; PUTC", qftasm_src_str(inst), QFTASM_STDOUT);
      qftasm_emit_line("MNZ 32768 %d %d;", QFTASM_STDIO_CLOSED, QFTASM_STDOUT);
  #endif
      break;

    case GETC:
  #ifdef QFTASM_RAM_AS_STDIN_BUFFER
      qftasm_emit_line("ANT A%d %d %d; GETC", QFTASM_STDIN, 0b1111111111111110, QFTASM_TEMP);
      // qftasm_emit_line("SRL A%d 1 %d;", QFTASM_STDIN, QFTASM_TEMP_2);

      qftasm_emit_line("ANT A%d 1 %d; GETC", QFTASM_STDIN, QFTASM_TEMP_2);
      qftasm_emit_line("SRU 0 A%d %d;", QFTASM_TEMP_2, QFTASM_TEMP_2);
      qftasm_emit_line("MNZ 0 0 0;");

      // If the stdin buffer pointer is odd, skip the following
      qftasm_emit_line("MNZ A%d 4 %d;", QFTASM_TEMP, QFTASM_TEMP);
      qftasm_emit_line("ADD A%d A%d %d;", QFTASM_TEMP, QFTASM_PC, QFTASM_PC);
      qftasm_emit_line("MNZ 0 0 0;");

      // qftasm_emit_line("SRL B%d 8 %d;", QFTASM_TEMP_2, qftasm_reg2addr(inst->dst.reg));
      qftasm_emit_line("ANT B%d %d %d;", QFTASM_TEMP_2, 0b1111111100000000, qftasm_reg2addr(inst->dst.reg));

      qftasm_emit_line("ADD A%d 4 %d;", QFTASM_PC, QFTASM_PC); // Local jump
      qftasm_emit_line("MNZ 0 0 0;");

      qftasm_emit_line("ANT B%d %d %d; GETC", QFTASM_TEMP_2, 0b11111111, QFTASM_TEMP_2);
      qftasm_emit_line("SRE 0 A%d %d;", QFTASM_TEMP_2, qftasm_reg2addr(inst->dst.reg));
      qftasm_emit_line("MNZ 0 0 0;");


      qftasm_emit_line("ADD A%d 1 %d;", QFTASM_STDIN, QFTASM_STDIN);
  #else
      qftasm_emit_line("MNZ 32768 %d %d; GETC", QFTASM_STDIO_OPEN, QFTASM_STDIN);
      qftasm_emit_line("MNZ 0 0 0;"); // Required due to the delay between memory writing and instruction execution
      qftasm_emit_line("MNZ 32768 A%d %d;", QFTASM_STDIN, qftasm_reg2addr(inst->dst.reg));
      qftasm_emit_line("MNZ 32768 %d %d;", QFTASM_STDIO_CLOSED, QFTASM_STDIN);
  #endif
      break;

    case EXIT:
      qftasm_emit_line("MNZ 32768 65534 0; EXIT");
      qftasm_emit_line("MNZ 0 0 0;");
      break;

    case DUMP:
      break;

    case EQ:
      if (inst->src.type == IMM && inst->src.imm == 0) {
        qftasm_emit_line("MNZ A%d 1 %d; EQ (with 0)", qftasm_reg2addr(inst->dst.reg), qftasm_reg2addr(inst->dst.reg));
      } else {
        qftasm_emit_line("XOR %s A%d %d; EQ",
                        qftasm_src_str(inst),
                        qftasm_reg2addr(inst->dst.reg),
                        qftasm_reg2addr(inst->dst.reg));
        qftasm_emit_line("MNZ A%d 1 %d;", qftasm_reg2addr(inst->dst.reg), qftasm_reg2addr(inst->dst.reg));
      }
      qftasm_emit_line("XOR 1 A%d %d;", qftasm_reg2addr(inst->dst.reg), qftasm_reg2addr(inst->dst.reg));
      break;
    case NE:
      qftasm_emit_line("XOR %s A%d %d; NE",
                      qftasm_src_str(inst), qftasm_reg2addr(inst->dst.reg), qftasm_reg2addr(inst->dst.reg));
      qftasm_emit_line("MNZ A%d 1 %d;", qftasm_reg2addr(inst->dst.reg), qftasm_reg2addr(inst->dst.reg));
      break;
    case LT: // dst < src
      if (inst->src.type == IMM && inst->src.imm == 0) {
        qftasm_emit_line("MNZ 32768 0 %d; LT (with 0)", QFTASM_TEMP);
      } else {
        qftasm_emit_line("MNZ 32768 0 %d; LT", QFTASM_TEMP);
        qftasm_emit_line("SUB A%d %s %d;",
                        qftasm_reg2addr(inst->dst.reg), qftasm_src_str(inst), qftasm_reg2addr(inst->dst.reg));
      }
      qftasm_emit_line("MLZ A%d 1 %d;", qftasm_reg2addr(inst->dst.reg), QFTASM_TEMP);
      qftasm_emit_line("MNZ 32768 A%d %d;", QFTASM_TEMP, qftasm_reg2addr(inst->dst.reg));
      break;
    case GT: // dst > src
      qftasm_emit_line("MNZ 32768 0 %d; GT", QFTASM_TEMP);
      qftasm_emit_line("SUB %s %s %d;",
                      qftasm_src_str(inst), qftasm_dst_str(inst), qftasm_reg2addr(inst->dst.reg));
      qftasm_emit_line("MLZ A%d 1 %d;", qftasm_reg2addr(inst->dst.reg), QFTASM_TEMP);
      qftasm_emit_line("MNZ 32768 A%d %d;", QFTASM_TEMP, qftasm_reg2addr(inst->dst.reg));
      break;
    case LE: // dst <= src, i.e. !(dst > src)
      qftasm_emit_line("MNZ 32768 1 %d; LE", QFTASM_TEMP);
      qftasm_emit_line("SUB %s %s %d;",
                      qftasm_src_str(inst), qftasm_dst_str(inst), qftasm_reg2addr(inst->dst.reg));
      qftasm_emit_line("MLZ A%d 0 %d;", qftasm_reg2addr(inst->dst.reg), QFTASM_TEMP);
      qftasm_emit_line("MNZ 32768 A%d %d;", QFTASM_TEMP, qftasm_reg2addr(inst->dst.reg));
      break;
    case GE: // dst >= src, i.e. !(dst < src)
      if (inst->src.type == IMM && inst->src.imm == 0) {
        qftasm_emit_line("MNZ 32768 1 %d; GE (with 0)", QFTASM_TEMP);
      } else {
        qftasm_emit_line("MNZ 32768 1 %d; GE", QFTASM_TEMP);
        qftasm_emit_line("SUB %s %s %d;",
                        qftasm_dst_str(inst), qftasm_src_str(inst), qftasm_reg2addr(inst->dst.reg));
      }
      qftasm_emit_line("MLZ A%d 0 %d;", qftasm_reg2addr(inst->dst.reg), QFTASM_TEMP);
      qftasm_emit_line("MNZ 32768 A%d %d;", QFTASM_TEMP, qftasm_reg2addr(inst->dst.reg));
      break;

    case JEQ:
      if (inst->src.type == IMM & inst->src.imm == 0) {
        qftasm_emit_line("MNZ 32768 1 %d; JEQ (with 0)", QFTASM_TEMP);
        qftasm_emit_line("MNZ A%d 0 %d;", qftasm_reg2addr(inst->dst.reg), QFTASM_TEMP);
      } else {
        qftasm_emit_line("XOR %s A%d %d; JEQ",
                        qftasm_src_str(inst), qftasm_reg2addr(inst->dst.reg), QFTASM_TEMP);
        qftasm_emit_line("MNZ A%d 1 %d;", QFTASM_TEMP, QFTASM_TEMP);
        qftasm_emit_line("XOR 1 A%d %d;", QFTASM_TEMP, QFTASM_TEMP);
      }
      qftasm_emit_conditional_jmp_inst(inst);
      break;
    case JNE:
      qftasm_emit_jne(inst, &inst->jmp);
      break;
    case JLT:
      if (inst->src.type == IMM && inst->src.imm == 0) {
        qftasm_emit_line("MNZ 32768 0 %d; JLT (with 0)", QFTASM_TEMP_2);
      } else {
        qftasm_emit_line("MNZ 32768 0 %d; JLT", QFTASM_TEMP_2);
        qftasm_emit_line("SUB %s %s %d;",
                        qftasm_dst_str(inst), qftasm_src_str(inst), QFTASM_TEMP);
      }
      qftasm_emit_line("MLZ A%d 1 %d;", QFTASM_TEMP, QFTASM_TEMP_2);
      qftasm_emit_line("MNZ 32768 A%d %d;", QFTASM_TEMP_2, QFTASM_TEMP);
      qftasm_emit_conditional_jmp_inst(inst);
      break;
    case JGT:
      qftasm_emit_line("MNZ 32768 0 %d; JGT", QFTASM_TEMP_2);
      qftasm_emit_line("SUB %s %s %d;",
                      qftasm_src_str(inst), qftasm_dst_str(inst), QFTASM_TEMP);
      qftasm_emit_line("MLZ A%d 1 %d;", QFTASM_TEMP, QFTASM_TEMP_2);
      qftasm_emit_line("MNZ 32768 A%d %d;", QFTASM_TEMP_2, QFTASM_TEMP);
      qftasm_emit_conditional_jmp_inst(inst);
      break;
    case JLE:
      qftasm_emit_line("MNZ 32768 1 %d; JLE", QFTASM_TEMP_2);
      qftasm_emit_line("SUB %s %s %d;",
                      qftasm_src_str(inst), qftasm_dst_str(inst), QFTASM_TEMP);
      qftasm_emit_line("MLZ A%d 0 %d;", QFTASM_TEMP, QFTASM_TEMP_2);
      qftasm_emit_line("MNZ 32768 A%d %d;", QFTASM_TEMP_2, QFTASM_TEMP);
      qftasm_emit_conditional_jmp_inst(inst);
      break;
    case JGE:
      if (inst->src.type == IMM && inst->src.imm == 0) {
        qftasm_emit_line("MNZ 32768 1 %d; JGE (with 0)", QFTASM_TEMP_2);
      } else {
        qftasm_emit_line("MNZ 32768 1 %d; JGE", QFTASM_TEMP_2);
        qftasm_emit_line("SUB %s %s %d;",
                        qftasm_dst_str(inst), qftasm_src_str(inst), QFTASM_TEMP);
      }
      qftasm_emit_line("MLZ A%d 0 %d;", QFTASM_TEMP, QFTASM_TEMP_2);
      qftasm_emit_line("MNZ 32768 A%d %d;", QFTASM_TEMP_2, QFTASM_TEMP);
      qftasm_emit_conditional_jmp_inst(inst);
      break;
    case JMP:
      qftasm_emit_unconditional_jmp_inst(inst);
      break;

    default:
      error("oops");
    }
  }
  qftasm_prev_inst = inst;
}

void target_qftasm(Module* module) {
  // for (Table* table = module->table; table; table = table->next) {
  //   printf("%s : %ld\n", table->key, (long)table->value);
  // }
  init_state_qftasm(module->data, module->text);

  emit_chunked_main_loop(module->text,
                         qftasm_emit_func_prologue,
                         qftasm_emit_func_epilogue,
                         qftasm_emit_pc_change,
                         qftasm_emit_inst);
}
