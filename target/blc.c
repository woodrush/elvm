#include <ir/ir.h>
#include <target/util.h>
#include <target/blccore.h>

// #define DEBUG


// (cons x y) = (lambda (f) (f x y))
// = 00010110[x][y]
static const int BLC_N_BITS = 24;
static const char CONS_HEAD[] = "00010110";

// (cons4 x1 x2 x3 x4) = (lambda (f) (f x1 x2 x3 x4))
// = 000101010110[x1][x2][x3][x4]
static const char CONS4_HEAD[] = "000101010110";

static const char T[] = "0000110";
static const char NIL[] = "000010";
static const char BLC_REG_A[] = "0001010110000010000010000010";
static const char BLC_REG_B[] = "00010101100000110000010000010";
static const char BLC_REG_C[] = "00010101100000100000110000010";
static const char BLC_REG_D[] = "000101011000001100000110000010";
static const char BLC_REG_SP[] = "00010101100000100000100000110";
static const char BLC_REG_BP[] = "000101011000001100000100000110";
static const char INST_EXIT[] = "000010";
static const char INST_MOV[] = "000000000000000010";
static const char INST_ADDSUB[] = "0000000000000000110";
static const char INST_STORE[] = "00000000000000001110";
static const char INST_LOAD[] = "000000000000000011110";
static const char INST_JMP[] = "0000000000000000111110";
static const char INST_CMP[] = "00000000000000001111110";
static const char INST_JUMPCMP[] = "000000000000000011111110";
static const char INST_IO[] = "0000000000000000111111110";
static const char CMP_GT[] = "00010101100000100000100000110";
static const char CMP_LT[] = "00010101100000100000110000010";
static const char CMP_EQ[] = "00010101100000110000010000010";
static const char CMP_LE[] = "000101011000001100000110000010";
static const char CMP_GE[] = "000101011000001100000100000110";
static const char CMP_NE[] = "000101011000001000001100000110";
static const char IO_PUTC[] = "000010";
static const char IO_GETC[] = "0000110";
static const char PLACEHOLDER[] = "10";


static void blc_debug(const char* fmt, ...) {
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

static const char* blc_reg(Reg r) {
  switch (r) {
  case A: return BLC_REG_A;
  case B: return BLC_REG_B;
  case C: return BLC_REG_C;
  case D: return BLC_REG_D;
  case BP: return BLC_REG_BP;
  case SP: return BLC_REG_SP;
  default:
    error("unknown register: %d", r);
  }
}

static void blc_emit_int(int n) {
  blc_debug("\n# int %d (%0d)\n", n, n);
  int checkbit = 1 << (BLC_N_BITS - 1);
  for (int i = 0; i < BLC_N_BITS; i++) {
#ifndef __eir__
    n &= ((1 << BLC_N_BITS) - 1);
#endif
    fputs(CONS_HEAD, stdout);
    blc_debug("    ");
    fputs((n & checkbit) ? NIL : T, stdout);
    blc_debug("    ");
    blc_debug("\n");
    checkbit = checkbit >> 1;
  }
  fputs(NIL, stdout);
}

static void emit_blc_isimm(Value* v) {
  if (v->type == REG) {
    fputs(NIL, stdout);
  } else if (v->type == IMM) {
    fputs(T, stdout);
  } else {
    error("invalid value");
  }
}

static void emit_blc_value_str(Value* v) {
  if (v->type == REG) {
    fputs(blc_reg(v->reg), stdout);
  } else if (v->type == IMM) {
    blc_emit_int(v->imm);
  } else {
    error("invalid value");
  }
}

static Data* blc_emit_data_tree(int depth, Data* data) {
  if (!data) {
    fputs(NIL, stdout);
    return data;
  } else if (depth > 0) {
    fputs(CONS_HEAD, stdout);
    Data* next = blc_emit_data_tree(depth-1, data);
    next = blc_emit_data_tree(depth-1, next);
    return next;
  } else {
    blc_emit_int(data->v);
    return data->next;
  }
}

static void blc_emit_data(Data* data) {
  blc_emit_data_tree(BLC_N_BITS, data);
}

static void blc_emit_basic_inst(Inst* inst, const char* inst_tag) {
    fputs(CONS4_HEAD, stdout);
    fputs(inst_tag, stdout);
    emit_blc_isimm(&inst->src);
    emit_blc_value_str(&inst->src);
    emit_blc_value_str(&inst->dst);
}

static void blc_emit_addsub_inst(Inst* inst, bool is_add) {
    fputs(CONS4_HEAD, stdout);
    fputs(INST_ADDSUB, stdout);
    emit_blc_isimm(&inst->src);
    emit_blc_value_str(&inst->src);
    fputs(CONS_HEAD, stdout);
    emit_blc_value_str(&inst->dst);
    if (is_add) {
      fputs(T, stdout);
    } else {
      fputs(NIL, stdout);
    }
}

static void blc_emit_jumpcmp_inst(Inst* inst, const char* cmp_tag) {
    fputs(CONS4_HEAD, stdout);
    fputs(INST_JUMPCMP, stdout);
    emit_blc_isimm(&inst->src);
    emit_blc_value_str(&inst->src);
    fputs(CONS4_HEAD, stdout);
    fputs(cmp_tag, stdout);
    emit_blc_value_str(&inst->dst);
    emit_blc_isimm(&inst->jmp);
    emit_blc_value_str(&inst->jmp);
}

static void blc_emit_cmp_inst(Inst* inst, const char* cmp_tag) {
    fputs(CONS4_HEAD, stdout);
    fputs(INST_CMP, stdout);
    emit_blc_isimm(&inst->src);
    emit_blc_value_str(&inst->src);
    fputs(CONS_HEAD, stdout);
    fputs(cmp_tag, stdout);
    emit_blc_value_str(&inst->dst);
}

static void blc_emit_inst(Inst* inst) {
  blc_debug("\n# Inst-body (%d)\n", inst->op);
  switch (inst->op) {
  case MOV: blc_emit_basic_inst(inst, INST_MOV); break;
  case LOAD: blc_emit_basic_inst(inst, INST_LOAD); break;
  case STORE: blc_emit_basic_inst(inst, INST_STORE); break;

  case ADD: blc_emit_addsub_inst(inst, true); break;
  case SUB: blc_emit_addsub_inst(inst, false); break;

  case PUTC:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_IO, stdout);
    emit_blc_isimm(&inst->src);
    emit_blc_value_str(&inst->src);
    fputs(IO_PUTC, stdout);
    break;

  case GETC:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_IO, stdout);
    fputs(NIL, stdout);
    emit_blc_value_str(&inst->dst);
    fputs(IO_GETC, stdout);
    break;

  case EXIT:
    fputs(INST_EXIT, stdout);
    break;

  case DUMP:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_MOV, stdout);
    fputs(NIL, stdout);
    fputs(BLC_REG_A, stdout);
    fputs(BLC_REG_A, stdout);
    break;

  case EQ: blc_emit_cmp_inst(inst, CMP_EQ); break;
  case NE: blc_emit_cmp_inst(inst, CMP_NE); break;
  case LT: blc_emit_cmp_inst(inst, CMP_LT); break;
  case GT: blc_emit_cmp_inst(inst, CMP_GT); break;
  case LE: blc_emit_cmp_inst(inst, CMP_LE); break;
  case GE: blc_emit_cmp_inst(inst, CMP_GE); break;

  case JEQ: blc_emit_jumpcmp_inst(inst, CMP_EQ); break;
  case JNE: blc_emit_jumpcmp_inst(inst, CMP_NE); break;
  case JLT: blc_emit_jumpcmp_inst(inst, CMP_LT); break;
  case JGT: blc_emit_jumpcmp_inst(inst, CMP_GT); break;
  case JLE: blc_emit_jumpcmp_inst(inst, CMP_LE); break;
  case JGE: blc_emit_jumpcmp_inst(inst, CMP_GE); break;

  case JMP:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_JMP, stdout);
    emit_blc_isimm(&inst->jmp);
    emit_blc_value_str(&inst->jmp);
    fputs(PLACEHOLDER, stdout);
    break;

  default:
    error("oops");
  }
  blc_debug("\n# Inst-end\n");
}

static Inst* blc_emit_chunk(Inst* inst_) {
  int pc = inst_->pc;
  Inst* inst;
  for (inst = inst_; inst && (inst->pc == pc); inst = inst->next) {
    fputs(CONS_HEAD, stdout);
    blc_emit_inst(inst);
  }
  fputs(NIL, stdout);
  return inst;
}

static Inst* blc_emit_text_tree(int depth, Inst* inst) {
  if (!inst) {
    fputs(NIL, stdout);
    return inst;
  } else if (depth > 0) {
    fputs(CONS_HEAD, stdout);
    Inst* next = blc_emit_text_tree(depth-1, inst);
    next = blc_emit_text_tree(depth-1, next);
    return next;
  } else {
    inst = blc_emit_chunk(inst);
    return inst;
  }
}

static void blc_emit_text(Inst* inst) {
  blc_emit_text_tree(BLC_N_BITS, inst);
}

void target_blc(Module* module) {
  #ifndef DEBUG
    printf("0101");
    fputs(blc_core, stdout);
  #endif

  blc_emit_data(module->data);
  blc_emit_text(module->text);
}
