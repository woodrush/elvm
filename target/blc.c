#include <ir/ir.h>
#include <target/util.h>
#include <target/blccore.h>


// (cons x y) = (lambda (f) (f x y))
// = 00010110[x][y]
static const int BLC_N_BITS = 24;
static const char CONS_HEAD[] = "00010110";

// (cons4 x1 x2 x3 x4) = (lambda (f) (f x1 x2 x3 x4))
// = 000101010110[x1][x2][x3][x4]
static const char CONS4_HEAD[] = "000101010110";


static const char T[] = "0000110";
static const char NIL[] = "000010";

static const char INT_HEADER[] = "01000100";
static const char INT_FOOTER[] = "000010000001011000001011000000101100000110110";

// static const char BLC_REG_A[] =  "0001010110000010000010000010";
// static const char BLC_REG_B[] =  "00010101100000110000010000010";
// static const char BLC_REG_C[] =  "000101011000001100000110000010";
// static const char BLC_REG_D[] =  "00010101100000100000110000010";
// static const char BLC_REG_SP[] = "00010101100000100000100000110";
// static const char BLC_REG_BP[] = "000101011000001100000100000110";

static const char BLC_REG_A[]  = "000101100000110000101100000110000010";
static const char BLC_REG_B[]  = "0001011000001000010110000011000010110000010000010";
static const char BLC_REG_C[]  = "000101100000100001011000001000010110000010000101100000110000010";
static const char BLC_REG_D[]  = "00010110000011000010110000010000010";
static const char BLC_REG_SP[] = "00010110000010000101100000110000101100000110000010";
static const char BLC_REG_BP[] = "0001011000001000010110000010000101100000110000010";

// REG-A:  
// REG-B:  
// REG-C:  
// REG-D:  
// REG-SP: 
// REG-BP: 
// REG-PC: 0101010001101000000111001110100000010111000001010000010


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
  fputs(INT_HEADER, stdout);
  int checkbit = 1 << (BLC_N_BITS - 1);
  for (int i = 0; i < BLC_N_BITS; i++) {
#ifndef __eir__
    n &= ((1 << BLC_N_BITS) - 1);
#endif
    fputs("01", stdout);
    fputs((n & checkbit) ? "10" : "110", stdout);
    checkbit = checkbit >> 1;
  }
  fputs(INT_FOOTER, stdout);
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
  } else if (depth == 0) {
    blc_emit_int(data->v);
    return data->next;
  } else {
    fputs(CONS_HEAD, stdout);
    data = blc_emit_data_tree(depth-1, data);
    return blc_emit_data_tree(depth-1, data);
  }
}

static void blc_emit_inst_header(const char* inst_tag, Value* v) {
  fputs(CONS4_HEAD, stdout);
  fputs(inst_tag, stdout);
  emit_blc_isimm(v);
  emit_blc_value_str(v);
}

static void blc_emit_basic_inst(Inst* inst, const char* inst_tag) {
  blc_emit_inst_header(inst_tag, &inst->src);
  emit_blc_value_str(&inst->dst);
}

static void blc_emit_addsub_inst(Inst* inst, const char* is_add) {
  blc_emit_inst_header(INST_ADDSUB, &inst->src);
  fputs(CONS_HEAD, stdout);
  emit_blc_value_str(&inst->dst);
  fputs(is_add, stdout);
}

static void blc_emit_jumpcmp_inst(Inst* inst, const char* cmp_tag) {
  blc_emit_inst_header(INST_JUMPCMP, &inst->src);
  blc_emit_inst_header(cmp_tag, &inst->jmp);
  emit_blc_value_str(&inst->dst);
}

static void blc_emit_cmp_inst(Inst* inst, const char* cmp_tag) {
  blc_emit_inst_header(INST_CMP, &inst->src);
  fputs(CONS_HEAD, stdout);
  fputs(cmp_tag, stdout);
  emit_blc_value_str(&inst->dst);
}

static void blc_emit_io_inst(const char* io_tag, Value* v) {
  blc_emit_inst_header(INST_IO, v);
  fputs(io_tag, stdout);
}

static void blc_emit_jmp_inst(Inst* inst) {
  blc_emit_inst_header(INST_JMP, &inst->jmp);
  fputs(PLACEHOLDER, stdout);
}

static void blc_emit_dump_inst(void) {
  fputs(CONS4_HEAD, stdout);
  fputs(INST_MOV, stdout);
  fputs(NIL, stdout);
  fputs(BLC_REG_A, stdout);
  fputs(BLC_REG_A, stdout);
}


static void blc_emit_inst(Inst* inst) {
  switch (inst->op) {
  case MOV: blc_emit_basic_inst(inst, INST_MOV); break;
  case LOAD: blc_emit_basic_inst(inst, INST_LOAD); break;
  case STORE: blc_emit_basic_inst(inst, INST_STORE); break;

  case ADD: blc_emit_addsub_inst(inst, T); break;
  case SUB: blc_emit_addsub_inst(inst, NIL); break;

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

  case JMP: blc_emit_jmp_inst(inst); break;

  case PUTC: blc_emit_io_inst(IO_PUTC, &inst->src); break;
  case GETC: blc_emit_io_inst(IO_GETC, &inst->dst); break;

  case DUMP: blc_emit_dump_inst(); break;
  case EXIT: fputs(INST_EXIT, stdout); break;

  default:
    error("oops");
  }
}

static Inst* blc_emit_chunk(Inst* inst) {
  const int init_pc = inst->pc;
  for (; inst && (inst->pc == init_pc); inst = inst->next) {
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
  } else if (depth == 0) {
    inst = blc_emit_chunk(inst);
    return inst;
  } else {
    fputs(CONS_HEAD, stdout);
    inst = blc_emit_text_tree(depth-1, inst);
    return blc_emit_text_tree(depth-1, inst);
  }
}

void target_blc(Module* module) {
  printf("0101");
  fputs(blc_core, stdout);
  blc_emit_data_tree(BLC_N_BITS, module->data);
  blc_emit_text_tree(BLC_N_BITS, module->text);
}
