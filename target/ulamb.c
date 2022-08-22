#include <ir/ir.h>
#include <target/util.h>
#include <target/ulambcore.h>

// #define DEBUG


// (cons x y) = (lambda (f) (f x y))
// = 00010110[x][y]
static const char CONS_HEAD[] = "00010110";
static const char CONS_COMMA[] = "";

// (cons4 x1 x2 x3 x4) = (lambda (f) (f x1 x2 x3 x4))
// = 000101010110[x1][x2][x3][x4]
static const char CONS4_HEAD[] = "000101010110";

static const char T[] = "0000110";
static const char NIL[] = "000010";
static const char ULAMB_REG_A[] = "000101100000100001011000001000010110000010000010";
static const char ULAMB_REG_B[] = "0001011000001100001011000001000010110000010000010";
static const char ULAMB_REG_C[] = "0001011000001000010110000011000010110000010000010";
static const char ULAMB_REG_D[] = "00010110000011000010110000011000010110000010000010";
static const char ULAMB_REG_SP[] = "0001011000001000010110000010000101100000110000010";
static const char ULAMB_REG_BP[] = "00010110000011000010110000010000101100000110000010";
static const char INST_ADD[] = "00000000000000000010";
static const char INST_STORE[] = "000000000000000000110";
static const char INST_MOV[] = "0000000000000000001110";
static const char INST_JMP[] = "00000000000000000011110";
static const char INST_JUMPCMP[] = "000000000000000000111110";
static const char INST_LOAD[] = "0000000000000000001111110";
static const char INST_CMP[] = "00000000000000000011111110";
static const char INST_SUB[] = "000000000000000000111111110";
static const char INST_IO_INT[] = "0000000000000000001111111110";
static const char CMP_EQ[] = "00000000000010";
static const char CMP_NE[] = "000000000000110";
static const char CMP_LT[] = "0000000000001110";
static const char CMP_GT[] = "00000000000011110";
static const char CMP_LE[] = "000000000000111110";
static const char CMP_GE[] = "0000000000001111110";
static const char IO_INT_PUTC[] = "00000010";
static const char IO_INT_GETC[] = "000000110";
static const char IO_INT_EXIT[] = "0000001110";

static void ulamb_debug(const char* fmt, ...) {
  if (fmt[0]) {
    #ifndef DEBUG
      return;
    #endif
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  }
}

static const char* ulamb_reg(Reg r) {
  switch (r) {
  case A: return ULAMB_REG_A;
  case B: return ULAMB_REG_B;
  case C: return ULAMB_REG_C;
  case D: return ULAMB_REG_D;
  case BP: return ULAMB_REG_BP;
  case SP: return ULAMB_REG_SP;
  default:
    error("unknown register: %d", r);
  }
}

static void ulamb_emit_int(int n) {
  ulamb_debug("\n# int %d (%0d)\n", n, n);
  for (int i = 0; i < 24; i++) {
#ifndef __eir__
    n &= ((1 << 24) - 1);
#endif
    fputs(CONS_HEAD, stdout);
    ulamb_debug("    ");
    fputs((n & 1) ? T : NIL, stdout);
    ulamb_debug("    ");
    fputs(CONS_COMMA, stdout);
    ulamb_debug("\n");
    n = n >> 1;
  }
  fputs(NIL, stdout);
}

static void emit_ulamb_isimm(Value* v) {
  if (v->type == REG) {
    fputs(NIL, stdout);
  } else if (v->type == IMM) {
    fputs(T, stdout);
  } else {
    error("invalid value");
  }
}

static void emit_ulamb_value_str(Value* v) {
  if (v->type == REG) {
    fputs(ulamb_reg(v->reg), stdout);
  } else if (v->type == IMM) {
    ulamb_emit_int(v->imm);
  } else {
    error("invalid value");
  }
}

static void ulamb_emit_data(Data* data) {
  ulamb_debug("# Data\n");
  for (Data* d = data; d; d = d->next) {
    ulamb_debug("\n# data\n");
    fputs(CONS_HEAD, stdout);
    ulamb_emit_int(d->v);
    fputs(CONS_COMMA, stdout);
  }
  ulamb_debug("\n# Data end\n");
  fputs(NIL, stdout);
}

static void ulamb_emit_func_prologue(int func_id) {
  ulamb_debug("\n# Func_prologue\n");
  // fputs(CONS_HEAD, stdout);

  // Placeholder code that does nothing, to suppress compilation errors
  if (func_id) {
    return;
  }
}

static void ulamb_emit_pc_change(int pc) {
  if (pc) {
    ulamb_debug("\n# PC change\n");
    fputs(NIL, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(CONS_HEAD, stdout);
  }
}

static void ulamb_emit_func_epilogue(void) {
  ulamb_debug("\n# Func_epilogue\n");
  // fputs(NIL, stdout);
  // fputs(CONS_COMMA, stdout);
  // fputs(NIL, stdout);
}

static void ulamb_emit_basic_inst(Inst* inst, const char* inst_tag) {
    fputs(CONS4_HEAD, stdout);
    fputs(inst_tag, stdout);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_isimm(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_value_str(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_value_str(&inst->dst);
}

static void ulamb_emit_jumpcmp_inst(Inst* inst, const char* cmp_tag) {
    fputs(CONS4_HEAD, stdout);
    fputs(INST_JUMPCMP, stdout);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_isimm(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_value_str(&inst->src);
    fputs(CONS_COMMA, stdout);
    fputs(CONS4_HEAD, stdout);
    fputs(cmp_tag, stdout);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_value_str(&inst->dst);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_isimm(&inst->jmp);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_value_str(&inst->jmp);
}

static void ulamb_emit_cmp_inst(Inst* inst, const char* cmp_tag) {
    fputs(CONS4_HEAD, stdout);
    fputs(INST_CMP, stdout);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_isimm(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_value_str(&inst->src);
    fputs(CONS_COMMA, stdout);
    fputs(CONS_HEAD, stdout);
    fputs(cmp_tag, stdout);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_value_str(&inst->dst);
}


static void ulamb_emit_inst(Inst* inst) {
  ulamb_debug("\n# Inst\n");
  fputs(CONS_HEAD, stdout);
  ulamb_debug("\n# Inst-body (%d)\n", inst->op);
  switch (inst->op) {
  case MOV:
    ulamb_emit_basic_inst(inst, INST_MOV);
    break;
  case ADD:
    ulamb_emit_basic_inst(inst, INST_ADD);
    break;
  case SUB:
    ulamb_emit_basic_inst(inst, INST_SUB);
    break;
  case LOAD:
    ulamb_emit_basic_inst(inst, INST_LOAD);
    break;
  case STORE:
    ulamb_emit_basic_inst(inst, INST_STORE);
    break;

  case PUTC: //5
    // (cons4 inst-io-int _ [src] io-int-putc)
    fputs(CONS4_HEAD, stdout);
    fputs(INST_IO_INT, stdout);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_isimm(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_value_str(&inst->src);
    fputs(CONS_COMMA, stdout);
    fputs(IO_INT_PUTC, stdout);
    break;

  case GETC:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_IO_INT, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(NIL, stdout);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_value_str(&inst->dst);
    fputs(CONS_COMMA, stdout);
    fputs(IO_INT_GETC, stdout);
    break;

  case EXIT:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_IO_INT, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(T, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(T, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(IO_INT_EXIT, stdout);
    break;

  case DUMP:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_MOV, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(NIL, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(ULAMB_REG_A, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(ULAMB_REG_A, stdout);
    break;

  // (cons4 inst-cmp [src-isimm] [src] (cons [emum-cmp] [dst]))
  case EQ:
    ulamb_emit_cmp_inst(inst, CMP_EQ);
    break;
  case NE: //10
    ulamb_emit_cmp_inst(inst, CMP_NE);
    break;
  case LT:
    ulamb_emit_cmp_inst(inst, CMP_LT);
    break;
  case GT:
    ulamb_emit_cmp_inst(inst, CMP_GT);
    break;
  case LE:
    ulamb_emit_cmp_inst(inst, CMP_LE);
    break;
  case GE: //14
    ulamb_emit_cmp_inst(inst, CMP_GE);
    break;

  case JEQ:
    ulamb_emit_jumpcmp_inst(inst, CMP_EQ);
    break;
  case JNE:
    ulamb_emit_jumpcmp_inst(inst, CMP_NE);
    break;
  case JLT:
    ulamb_emit_jumpcmp_inst(inst, CMP_LT);
    break;
  case JGT:
    ulamb_emit_jumpcmp_inst(inst, CMP_GT);
    break;
  case JLE:
    ulamb_emit_jumpcmp_inst(inst, CMP_LE);
    break;
  case JGE:
    ulamb_emit_jumpcmp_inst(inst, CMP_GE);
    break;

  case JMP:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_JMP, stdout);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_isimm(&inst->jmp);
    fputs(CONS_COMMA, stdout);
    emit_ulamb_value_str(&inst->jmp);
    fputs(CONS_COMMA, stdout);
    fputs(NIL, stdout);
    break;

  default:
    error("oops");
  }
  ulamb_debug("\n# Inst-end\n");
  fputs(CONS_COMMA, stdout);
}

void target_ulamb(Module* module) {
  #ifndef DEBUG
    printf("0101");
    fputs(ulamb_core, stdout);
  #endif

  ulamb_emit_data(module->data);
  fputs(CONS_HEAD, stdout);
  emit_chunked_main_loop(module->text,
                        ulamb_emit_func_prologue,
                        ulamb_emit_func_epilogue,
                        ulamb_emit_pc_change,
                        ulamb_emit_inst);
  fputs(NIL, stdout);
  fputs(CONS_COMMA, stdout);
  fputs(NIL, stdout);
}
