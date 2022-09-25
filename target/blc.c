#include <ir/ir.h>
#include <target/util.h>


static char blc_vm[] =
"000000000001000100010001000100010001011111111100101000101100101111111000000101"
"100000000001011011111001111111011101011111111100101101111111111001111111000000"
"000000000010001000100010101010101111111000000000110001000000011000001001011111"
"100000000101010101011111111111111011111111111101111111111101111111111011101101"
"111111011111001111111000000100011110000000010101000000010111100110110010111111"
"111110110101101000000101010101010101111110010101100100010111111111111111110000"
"000010111110010111111111111111111111111111111111100000010110000011011011101100"
"101101111111111111111111111111101111111111111111000000101011111111111110111110"
"110010101010111111111111111111111011111111111111111110111111111111111111010111"
"111111011111111111111100001011001011111111111111111111111111111110000110000010"
"111001111111101111111111111011110011000000000010101000000010111100110110010111"
"111111111111110110101110110010111111111111110100000010101111111001011111111111"
"111111111111111111101101111111100111111111111111111010011111111111110111111111"
"111111111001011000000100010101010111111111111111111111111110000010011110010101"
"111111111111011011111111111111111111111111011111010100001111111111111110111011"
"111111111111111111111111101111110011111111111101100101010111111111111111111111"
"011111111111111111011001111111111101011111100101010101010101111111111010010111"
"111111111111111111101111111111111111101101111111111111111110111111111111111101"
"111111111111110111111101111111111111011111111111100101100000010101011111111111"
"101100101111111111111111111111111010101111000011111111111111011101111110010101"
"111111111101011011111100101010101111111111111011111111111011111111110111111111"
"010111111100111111111111011100111111111111011000010101111111111101111110100001"
"011001010111111111101111111101111111011111101110000001010101111111100000000001"
"011110000000010111111000000001010111111111110111111111011000000101011111111111"
"110101111000011111111111000010110111101100111111101111001011011101110110111111"
"110000001111011011111110000010011111000000000010111100000000101111110000000010"
"101011110011111111111011111100111111111110111110110111111100111110111111001101"
"111111110011110000000000001011110000000010001011111111100000000101111111100111"
"111111111011110010111110110111001011000001000001000010101011111111110101110111"
"111000000101111111001111111100001011011101100111111110000101101101110011011001"
"110000000000000010111100000000111111000000101010101111111111110111111111101111"
"111110111101000000100010001000100010001010101111011110111111100110000011001100"
"000100101010111011111101111001100000110011000001000000101111111111111101000010"
"110111011111110000001011111111111100101111011010010111101011001011111111111110"
"111110010111111000001000001100101110000010000011001011011111011100110000000010"
"111000000001111100000010001011111110010111101000000011001011110000000101001011"
"111111101111010000000111000010001110011010000111001101001000101111110100101111"
"11101000001000000101100000110110";


static const int BLC_N_BITS = 24;
static const char BLC_16[] = "010001011010100000011100111010";
static const char BLC_8[] = "0000011100111001110011100111001110011100111010";

static const char BLC_APPLY[] = "01";

// (cons x y) = (lambda (f) (f x y)) = 00010110[x][y]
static const char BLC_CONS_HEAD[] = "00010110";

// (cons4 x1 x2 x3 x4) = (lambda (f) (f x1 x2 x3 x4)) = 000101010110[x1][x2][x3][x4]
static const char BLC_CONS4_HEAD[] = "000101010110";

// ((lambda (cons-t cons-nil) [A]) (lambda (x f) (f t x)) (lambda (x f) (f nil x)))
//   = 01000100[A]000001011000001011000000101100000110110
// Where [A] is 
// (F3 (F2 (F1 BLC_NIL)))
//   = 01[F3]01[F2]01[F1]000010
// Where F1, F2, ... is in { 10, 110 }
static const char BLC_INT_HEADER[] = "01000100";
static const char BLC_INT_BIT1[] = "10";
static const char BLC_INT_BIT0[] = "110";
static const char BLC_INT_FOOTER[] = "000010000001011000001011000000101100000110110";

static const char BLC_T[] = "0000110";
static const char BLC_NIL[] = "000010";

static const char BLC_REG_A[]  = "00010110000010000101100000110000010";
static const char BLC_REG_B[]  = "00010110000011000010110000011000010110000010000010";
static const char BLC_REG_C[]  = "00010110000011000010110000011000010110000011000010110000010000010";
static const char BLC_REG_D[]  = "0001011000001100001011000001000010110000010000010";
static const char BLC_REG_SP[] = "00010110000011000010110000010000101100000110000010";
static const char BLC_REG_BP[] = "01010100011010000001110011101000000101100000110110000010";
static const char BLC_INST_MOV[] = "000000000000000010";
static const char BLC_INST_ADDSUB[] = "0000000000000000110";
static const char BLC_INST_STORE[] = "00000000000000001110";
static const char BLC_INST_LOAD[] = "000000000000000011110";
static const char BLC_INST_JMP[] = "0000000000000000111110";
static const char BLC_INST_CMP[] = "00000000000000001111110";
static const char BLC_INST_JMPCMP[] = "000000000000000011111110";
static const char BLC_INST_IO[] = "0000000000000000111111110";
static const char BLC_CMP_GT[] = "00010101100000100000100000110";
static const char BLC_CMP_LT[] = "00010101100000100000110000010";
static const char BLC_CMP_EQ[] = "00010101100000110000010000010";
static const char BLC_CMP_LE[] = "000101011000001100000110000010";
static const char BLC_CMP_GE[] = "000101011000001100000100000110";
static const char BLC_CMP_NE[] = "000101011000001000001100000110";
static const char BLC_IO_GETC[] = "0000001110";
static const char BLC_IO_PUTC[] = "000000110";
static const char BLC_IO_EXIT[] = "00000010";
static const char BLC_PLACEHOLDER[] = "10";

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
#ifndef __eir__
  n &= ((1 << BLC_N_BITS) - 1);
#endif
  fputs(BLC_INT_HEADER, stdout);
  for (int checkbit = 1 << (BLC_N_BITS - 1); checkbit; checkbit >>= 1) {
    fputs(BLC_APPLY, stdout);
    fputs((n & checkbit) ? BLC_INT_BIT1 : BLC_INT_BIT0, stdout);
  }
  fputs(BLC_INT_FOOTER, stdout);
}

static void emit_blc_isimm(Value* v) {
  if (v->type == REG) {
    fputs(BLC_NIL, stdout);
  } else if (v->type == IMM) {
    fputs(BLC_T, stdout);
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

static void blc_emit_data_list(Data* data) {
  for (; data; data = data->next){
    fputs(BLC_CONS_HEAD, stdout);
    blc_emit_int(data->v);
  }
  fputs(BLC_NIL, stdout);
}

static void blc_emit_inst_header(const char* inst_tag, Value* v) {
  fputs(BLC_CONS4_HEAD, stdout);
  fputs(inst_tag, stdout);
  emit_blc_isimm(v);
  emit_blc_value_str(v);
}

static void blc_emit_basic_inst(Inst* inst, const char* inst_tag) {
  blc_emit_inst_header(inst_tag, &inst->src);
  emit_blc_value_str(&inst->dst);
}

static void blc_emit_addsub_inst(Inst* inst, const char* is_add) {
  blc_emit_inst_header(BLC_INST_ADDSUB, &inst->src);
  fputs(BLC_CONS_HEAD, stdout);
  emit_blc_value_str(&inst->dst);
  fputs(is_add, stdout);
}

static void blc_emit_jmpcmp_inst(Inst* inst, const char* cmp_tag) {
  blc_emit_inst_header(BLC_INST_JMPCMP, &inst->src);
  blc_emit_inst_header(cmp_tag, &inst->jmp);
  emit_blc_value_str(&inst->dst);
}

static void blc_emit_cmp_inst(Inst* inst, const char* cmp_tag) {
  blc_emit_inst_header(BLC_INST_CMP, &inst->src);
  fputs(BLC_CONS_HEAD, stdout);
  fputs(cmp_tag, stdout);
  emit_blc_value_str(&inst->dst);
}

static void blc_emit_io_inst(const char* io_tag, Value* v) {
  blc_emit_inst_header(BLC_INST_IO, v);
  fputs(io_tag, stdout);
}

static void blc_emit_exit_inst() {
  fputs(BLC_CONS4_HEAD, stdout);
  fputs(BLC_INST_IO, stdout);
  fputs(BLC_NIL, stdout);
  fputs(BLC_NIL, stdout);
  fputs(BLC_IO_EXIT, stdout);
}

static void blc_emit_jmp_inst(Inst* inst) {
  blc_emit_inst_header(BLC_INST_JMP, &inst->jmp);
  fputs(BLC_PLACEHOLDER, stdout);
}

static void blc_emit_dump_inst(void) {
  fputs(BLC_CONS4_HEAD, stdout);
  fputs(BLC_INST_MOV, stdout);
  fputs(BLC_NIL, stdout);
  fputs(BLC_REG_A, stdout);
  fputs(BLC_REG_A, stdout);
}

static void blc_emit_inst(Inst* inst) {
  switch (inst->op) {
  case MOV: blc_emit_basic_inst(inst, BLC_INST_MOV); break;
  case LOAD: blc_emit_basic_inst(inst, BLC_INST_LOAD); break;
  case STORE: blc_emit_basic_inst(inst, BLC_INST_STORE); break;

  case ADD: blc_emit_addsub_inst(inst, BLC_T); break;
  case SUB: blc_emit_addsub_inst(inst, BLC_NIL); break;

  case EQ: blc_emit_cmp_inst(inst, BLC_CMP_EQ); break;
  case NE: blc_emit_cmp_inst(inst, BLC_CMP_NE); break;
  case LT: blc_emit_cmp_inst(inst, BLC_CMP_LT); break;
  case GT: blc_emit_cmp_inst(inst, BLC_CMP_GT); break;
  case LE: blc_emit_cmp_inst(inst, BLC_CMP_LE); break;
  case GE: blc_emit_cmp_inst(inst, BLC_CMP_GE); break;

  case JEQ: blc_emit_jmpcmp_inst(inst, BLC_CMP_EQ); break;
  case JNE: blc_emit_jmpcmp_inst(inst, BLC_CMP_NE); break;
  case JLT: blc_emit_jmpcmp_inst(inst, BLC_CMP_LT); break;
  case JGT: blc_emit_jmpcmp_inst(inst, BLC_CMP_GT); break;
  case JLE: blc_emit_jmpcmp_inst(inst, BLC_CMP_LE); break;
  case JGE: blc_emit_jmpcmp_inst(inst, BLC_CMP_GE); break;

  case JMP: blc_emit_jmp_inst(inst); break;

  case PUTC: blc_emit_io_inst(BLC_IO_PUTC, &inst->src); break;
  case GETC: blc_emit_io_inst(BLC_IO_GETC, &inst->dst); break;
  
  case EXIT: blc_emit_exit_inst(); break;
  case DUMP: blc_emit_dump_inst(); break;

  default:
    error("oops");
  }
}

static Inst* blc_emit_chunk(Inst* inst) {
  const int init_pc = inst->pc;
  for (; inst && (inst->pc == init_pc); inst = inst->next) {
    fputs(BLC_CONS_HEAD, stdout);
    blc_emit_inst(inst);
  }
  fputs(BLC_NIL, stdout);
  return inst;
}

static void blc_emit_text_list(Inst* inst) {
  while (inst) {
    fputs(BLC_CONS_HEAD, stdout);
    inst = blc_emit_chunk(inst);
  }
  fputs(BLC_NIL, stdout);
}

void target_blc(Module* module) {
  fputs(BLC_APPLY, stdout);
  fputs(BLC_APPLY, stdout);
  fputs(BLC_APPLY, stdout);
  fputs(BLC_APPLY, stdout);
  fputs(blc_vm, stdout);
  fputs(BLC_8, stdout);
  fputs(BLC_16, stdout);
  blc_emit_data_list(module->data);
  blc_emit_text_list(module->text);
}
