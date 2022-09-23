#include <ir/ir.h>
#include <target/util.h>
#include <target/lamcore.h>


static const int LAM_N_BITS = 24;
static const char LAM_16[] = "((\\x.((x x) x)) (\\x.(\\y.(x (x y)))))";
static const char LAM_8[] = "(\\x.(\\y.(((\\x.(\\y.(x (x y)))) (((\\y.(y y)) (\\x.(\\y.(x (x y))))) x)) y)))";

static const char CONS_HEAD[] = "(\\f.(f ";
static const char CONS_FOOTER[] = "))";

static const char INT_HEADER[] = "(((\\x.(\\y.";
static const char INT_BIT1[] = "y ";
static const char INT_BIT0[] = "x ";
static const char INT_FOOTER[] = ")) (\\x.(\\y.((y (\\x.(\\a.x))) x)))) (\\x.(\\y.((y (\\x.(\\a.a))) x))))";

static const char T[] = "(\\x.\\y.x)";
static const char NIL[] = "(\\x.\\y.y)";

static const char LAM_REG_A[] = "(\\x.((x (\\y.(\\z.y))) (\\y.(\\z.z))))";
static const char LAM_REG_B[] = "(\\x.((x (\\y.(\\z.z))) (\\x.((x (\\z.(\\a.z))) (\\x.((x (\\a.(\\b.a))) (\\a.(\\b.b))))))))";
static const char LAM_REG_C[] = "(\\x.((x (\\y.(\\z.z))) (\\x.((x (\\z.(\\a.a))) (\\x.((x (\\a.(\\b.b))) (\\x.((x (\\b.(\\c.c))) (\\b.(\\c.c))))))))))";
static const char LAM_REG_D[] = "(\\x.((x (\\y.(\\z.z))) (\\x.((x (\\z.(\\a.a))) (\\x.((x (\\a.(\\b.a))) (\\a.(\\b.b))))))))";
static const char LAM_REG_SP[] = "(\\x.((x (\\y.(\\z.z))) (\\x.((x (\\z.(\\a.z))) (\\x.((x (\\a.(\\b.b))) (\\a.(\\b.b))))))))";
static const char LAM_REG_BP[] = "(\\x.((x (\\y.(\\z.z))) (\\x.((x (\\z.(\\a.a))) (\\x.((x (\\a.(\\b.b))) (\\x.((x (\\b.(\\c.b))) (\\b.(\\c.c))))))))))";
static const char INST_IO[] = "(\\x.(\\y.(\\z.(\\a.(\\b.(\\c.(\\d.(\\e.x))))))))";
static const char INST_JMPCMP[] = "(\\x.(\\y.(\\z.(\\a.(\\b.(\\c.(\\d.(\\e.y))))))))";
static const char INST_CMP[] = "(\\x.(\\y.(\\z.(\\a.(\\b.(\\c.(\\d.(\\e.z))))))))";
static const char INST_JMP[] = "(\\x.(\\y.(\\z.(\\a.(\\b.(\\c.(\\d.(\\e.a))))))))";
static const char INST_LOAD[] = "(\\x.(\\y.(\\z.(\\a.(\\b.(\\c.(\\d.(\\e.b))))))))";
static const char INST_STORE[] = "(\\x.(\\y.(\\z.(\\a.(\\b.(\\c.(\\d.(\\e.c))))))))";
static const char INST_ADDSUB[] = "(\\x.(\\y.(\\z.(\\a.(\\b.(\\c.(\\d.(\\e.d))))))))";
static const char INST_MOV[] = "(\\x.(\\y.(\\z.(\\a.(\\b.(\\c.(\\d.(\\e.e))))))))";
static const char CMP_GT[] = "(\\x.(((x (\\y.(\\z.z))) (\\y.(\\z.z))) (\\y.(\\z.y))))";
static const char CMP_LT[] = "(\\x.(((x (\\y.(\\z.z))) (\\y.(\\z.y))) (\\y.(\\z.z))))";
static const char CMP_EQ[] = "(\\x.(((x (\\y.(\\z.y))) (\\y.(\\z.z))) (\\y.(\\z.z))))";
static const char CMP_LE[] = "(\\x.(((x (\\y.(\\z.y))) (\\y.(\\z.y))) (\\y.(\\z.z))))";
static const char CMP_GE[] = "(\\x.(((x (\\y.(\\z.y))) (\\y.(\\z.z))) (\\y.(\\z.y))))";
static const char CMP_NE[] = "(\\x.(((x (\\y.(\\z.z))) (\\y.(\\z.y))) (\\y.(\\z.y))))";
static const char IO_GETC[] = "(\\x.(\\y.(\\z.x)))";
static const char IO_PUTC[] = "(\\x.(\\y.(\\z.y)))";
static const char IO_EXIT[] = "(\\x.(\\y.(\\z.z)))";
static const char PLACEHOLDER[] = "(\\x.x)";

static const char* lam_reg(Reg r) {
  switch (r) {
  case A: return LAM_REG_A;
  case B: return LAM_REG_B;
  case C: return LAM_REG_C;
  case D: return LAM_REG_D;
  case BP: return LAM_REG_BP;
  case SP: return LAM_REG_SP;
  default:
    error("unknown register: %d", r);
  }
}

static void lam_print_n (int n, const char* s) {
  for (int i=0; i<n; i++) {
    fputs(s, stdout);
  }
}

static void lam_emit_int(int n) {
#ifndef __eir__
  n &= ((1 << LAM_N_BITS) - 1);
#endif
  fputs(INT_HEADER, stdout);
  for (int checkbit = 1 << (LAM_N_BITS - 1); checkbit; checkbit >>= 1) {
    fputs("(", stdout);
    fputs((n & checkbit) ? INT_BIT1 : INT_BIT0, stdout);
  }
  fputs(NIL, stdout);
  lam_print_n(LAM_N_BITS, ")");
  fputs(INT_FOOTER, stdout);
}

static void emit_lam_isimm(Value* v) {
  if (v->type == REG) {
    fputs(NIL, stdout);
  } else if (v->type == IMM) {
    fputs(T, stdout);
  } else {
    error("invalid value");
  }
}

static void emit_lam_value_str(Value* v) {
  if (v->type == REG) {
    fputs(lam_reg(v->reg), stdout);
  } else if (v->type == IMM) {
    lam_emit_int(v->imm);
  } else {
    error("invalid value");
  }
}

static void lam_emit_data_list(Data* data) {
  int n_data = 0;
  for (; data; data = data->next){
    fputs(CONS_HEAD, stdout);
    lam_emit_int(data->v);
    n_data++;
  }
  fputs(NIL, stdout);
  lam_print_n(n_data, CONS_FOOTER);
}

static void lam_emit_inst_header(const char* inst_tag, Value* v) {
  fputs(CONS_HEAD, stdout);
  fputs(inst_tag, stdout);
  emit_lam_isimm(v);
  emit_lam_value_str(v);
}

static void lam_emit_basic_inst(Inst* inst, const char* inst_tag) {
  lam_emit_inst_header(inst_tag, &inst->src);
  emit_lam_value_str(&inst->dst);
  fputs(CONS_FOOTER, stdout);
}

static void lam_emit_addsub_inst(Inst* inst, const char* is_add) {
  lam_emit_inst_header(INST_ADDSUB, &inst->src);
  fputs(CONS_HEAD, stdout);
  emit_lam_value_str(&inst->dst);
  fputs(is_add, stdout);
  fputs(CONS_FOOTER, stdout);
  fputs(CONS_FOOTER, stdout);
}

static void lam_emit_jmpcmp_inst(Inst* inst, const char* cmp_tag) {
  lam_emit_inst_header(INST_JMPCMP, &inst->src);
  lam_emit_inst_header(cmp_tag, &inst->jmp);
  emit_lam_value_str(&inst->dst);
  fputs(CONS_FOOTER, stdout);
  fputs(CONS_FOOTER, stdout);
}

static void lam_emit_cmp_inst(Inst* inst, const char* cmp_tag) {
  lam_emit_inst_header(INST_CMP, &inst->src);
  fputs(CONS_HEAD, stdout);
  fputs(cmp_tag, stdout);
  emit_lam_value_str(&inst->dst);
  fputs(CONS_FOOTER, stdout);
  fputs(CONS_FOOTER, stdout);
}

static void lam_emit_io_inst(const char* io_tag, Value* v) {
  lam_emit_inst_header(INST_IO, v);
  fputs(io_tag, stdout);
  fputs(CONS_FOOTER, stdout);
}

static void lam_emit_exit_inst() {
  fputs(CONS_HEAD, stdout);
  fputs(INST_IO, stdout);
  fputs(NIL, stdout);
  fputs(NIL, stdout);
  fputs(IO_EXIT, stdout);
  fputs(CONS_FOOTER, stdout);
}

static void lam_emit_jmp_inst(Inst* inst) {
  lam_emit_inst_header(INST_JMP, &inst->jmp);
  fputs(PLACEHOLDER, stdout);
  fputs(CONS_FOOTER, stdout);
}

static void lam_emit_dump_inst(void) {
  fputs(CONS_HEAD, stdout);
  fputs(INST_MOV, stdout);
  fputs(NIL, stdout);
  fputs(LAM_REG_A, stdout);
  fputs(LAM_REG_A, stdout);
  fputs(CONS_FOOTER, stdout);
}

static void lam_emit_inst(Inst* inst) {
  switch (inst->op) {
  case MOV: lam_emit_basic_inst(inst, INST_MOV); break;
  case LOAD: lam_emit_basic_inst(inst, INST_LOAD); break;
  case STORE: lam_emit_basic_inst(inst, INST_STORE); break;

  case ADD: lam_emit_addsub_inst(inst, T); break;
  case SUB: lam_emit_addsub_inst(inst, NIL); break;

  case EQ: lam_emit_cmp_inst(inst, CMP_EQ); break;
  case NE: lam_emit_cmp_inst(inst, CMP_NE); break;
  case LT: lam_emit_cmp_inst(inst, CMP_LT); break;
  case GT: lam_emit_cmp_inst(inst, CMP_GT); break;
  case LE: lam_emit_cmp_inst(inst, CMP_LE); break;
  case GE: lam_emit_cmp_inst(inst, CMP_GE); break;

  case JEQ: lam_emit_jmpcmp_inst(inst, CMP_EQ); break;
  case JNE: lam_emit_jmpcmp_inst(inst, CMP_NE); break;
  case JLT: lam_emit_jmpcmp_inst(inst, CMP_LT); break;
  case JGT: lam_emit_jmpcmp_inst(inst, CMP_GT); break;
  case JLE: lam_emit_jmpcmp_inst(inst, CMP_LE); break;
  case JGE: lam_emit_jmpcmp_inst(inst, CMP_GE); break;

  case JMP: lam_emit_jmp_inst(inst); break;

  case PUTC: lam_emit_io_inst(IO_PUTC, &inst->src); break;
  case GETC: lam_emit_io_inst(IO_GETC, &inst->dst); break;
  
  case EXIT: lam_emit_exit_inst(); break;
  case DUMP: lam_emit_dump_inst(); break;

  default:
    error("oops");
  }
}

static Inst* lam_emit_chunk(Inst* inst) {
  const int init_pc = inst->pc;
  int n_insts = 0;
  for (; inst && (inst->pc == init_pc); inst = inst->next) {
    fputs(CONS_HEAD, stdout);
    lam_emit_inst(inst);
    n_insts++;
  }
  fputs(NIL, stdout);
  lam_print_n(n_insts, CONS_FOOTER);
  return inst;
}

static void lam_emit_text_list(Inst* inst) {
  int n_chunks = 0;
  while (inst) {
    fputs(CONS_HEAD, stdout);
    inst = lam_emit_chunk(inst);
    n_chunks++;
  }
  fputs(NIL, stdout);
  lam_print_n(n_chunks, CONS_FOOTER);
}

void target_lam(Module* module) {
  fputs("(", stdout);
  fputs(lam_core, stdout);
  fputs(LAM_8, stdout);
  fputs(LAM_16, stdout);
  lam_emit_data_list(module->data);
  lam_emit_text_list(module->text);
  fputs(")", stdout);
}
