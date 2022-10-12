#include <ir/ir.h>
#include <target/util.h>
#include <target/wcore.h>



static const int GRASS_N_BITS = 24;


static const int GRASS_INST_MOV = 8;
static const int GRASS_INST_ADDSUB = 7;
static const int GRASS_INST_STORE = 6;
static const int GRASS_INST_LOAD = 5;
static const int GRASS_INST_JMP = 4;
static const int GRASS_INST_CMP = 3;
static const int GRASS_INST_JMPCMP = 2;
static const int GRASS_INST_IO = 1;

static const int GRASS_IO_GETC = 1;
static const int GRASS_IO_PUTC = 2;
static const int GRASS_IO_EXIT = 3;

static const char GRASS_REG_A[]  = "wvwwWWWwwvwwvwWwwwWwwwv";
static const char GRASS_REG_B[]  = "wwvwvwwWWWwwvwWwwWwwwwwvwWwwwWWWWWwwwWWwvwWwwwwwwWWWWWWwwwWWwv";
static const char GRASS_REG_C[]  = "wwvwWwwWwwwvwvwWwwwwWWWwwwwWWwvwWwwwwwWWWWwwwWWwvwWwwwwwwWWWWWwwwWWwv";
static const char GRASS_REG_D[]  = "wwvwvwwWWWwwvwWwwWwwwwwvwWwwwwwWWWWWwwwWWwvwWwwwwwwWWWWWWwwwWWwv";
static const char GRASS_REG_SP[] = "wwvwvwwWWWwwvwWwwwwWwwwwwvwWwwwWWWWWwwwWWwvwWwwwwwwWWWWWWwwwWWwv";
static const char GRASS_REG_BP[] = "wwvwvwwWWWwwvwWwwWwwwwwvwWwwwwwWWWWWwwwWWwvwWwwwwwwWWWWWWwwwWWwvwWwwwwwwwWWWWWWWwwwWWwv";
static const int GRASS_REG_A_WEIGHT = 4;
static const int GRASS_REG_B_WEIGHT = 6;
static const int GRASS_REG_C_WEIGHT = 6;
static const int GRASS_REG_D_WEIGHT = 6;
static const int GRASS_REG_SP_WEIGHT = 6;
static const int GRASS_REG_BP_WEIGHT = 7;

static const char GRASS_CMP_GT[] = "wwvwvwwWWWwwvwWwwwwWwwwwwWwwwwv";
static const char GRASS_CMP_LT[] = "wwvwvwwWWWwwvwWwwwwWwwwWwwwwwwv";
static const char GRASS_CMP_EQ[] = "wvwwWWWwwvwwvwWwwwWwwwWwwwwv";
static const char GRASS_CMP_LE[] = "wvwwWWWwwvwwvwWwwwWwwwwWwwwwv";
static const char GRASS_CMP_GE[] = "wvwwWWWwwvwwvwWwwwWwwwWwwwwwv";
static const char GRASS_CMP_NE[] = "wwvwvwwWWWwwvwWwwwwWwwwWwwwwv";
static const int GRASS_CMP_WEIGHT = 4;


static int GRASS_BP = 1;

static void grass_emit_reg_helper(const char* s, const int i) {
  fputs(s, stdout);
  GRASS_BP += i;
}

static void grass_emit_reg(Reg r) {
  switch (r) {
  case A: grass_emit_reg_helper(GRASS_REG_A, GRASS_REG_A_WEIGHT); return;
  case B: grass_emit_reg_helper(GRASS_REG_B, GRASS_REG_B_WEIGHT); return;
  case C: grass_emit_reg_helper(GRASS_REG_C, GRASS_REG_C_WEIGHT); return;
  case D: grass_emit_reg_helper(GRASS_REG_D, GRASS_REG_D_WEIGHT); return;
  case BP: grass_emit_reg_helper(GRASS_REG_BP, GRASS_REG_BP_WEIGHT); return;
  case SP: grass_emit_reg_helper(GRASS_REG_SP, GRASS_REG_SP_WEIGHT); return;
  default:
    error("unknown register: %d", r);
  }
}

static void grass_apply(int W, int w) {
  for (; W; W--) putchar('W');
  for (; w; w--) putchar('w');
}

static int grass_put_t_nil(int t_nil) {
  if (t_nil) {
    // \x.x
    fputs("wv", stdout);
    // \x.\y.(3 2)
    fputs("wwWWWwwv", stdout);
    GRASS_BP += 2;
  } else {
    // \x.\y.y
    fputs("wwv", stdout);
    GRASS_BP++;
  }
  return GRASS_BP;
}

static int grass_emit_int_inst(int n) {
#ifndef __eir__
  n &= ((1 << GRASS_N_BITS) - 1);
#endif
  // \x.x
  fputs("wv", stdout);
  // \x.\y.x : Initial index: 2
  fputs("wwWWWwwv", stdout);
  // \x.\y.y : Initial index: 1
  fputs("wwv", stdout);
  GRASS_BP += 3;
  for (int i = 0; i < GRASS_N_BITS; i++) {
    const int checkbit = 1 << i;
    const int t_index = i + 1 + 2;
    const int nil_index = i + 1 + 1;
    putchar('w');
    grass_apply(1, (n & checkbit) ? nil_index : t_index);
    grass_apply(1, 3);
    putchar('v');
    GRASS_BP++;
  }
  return GRASS_BP;
}

static int emit_grass_isimm(Value* v) {
  if (v->type == REG) {
    grass_put_t_nil(0);
  } else if (v->type == IMM) {
    grass_put_t_nil(1);
  } else {
    error("invalid value");
  }
  return GRASS_BP;
}

static int emit_grass_value_str(Value* v) {
  if (v->type == REG) {
    grass_emit_reg(v->reg);
  } else if (v->type == IMM) {
    grass_emit_int_inst(v->imm);
  } else {
    error("invalid value");
  }
  return GRASS_BP;
}

static int grass_emit_int_data(int n) {
#ifndef __eir__
  n &= ((1 << GRASS_N_BITS) - 1);
#endif
  // \x.x
  fputs("wv", stdout);
  // \x.\y.y
  fputs("wwv", stdout);
  // \x.\y.x
  fputs("wwWWWWwwv", stdout);
  GRASS_BP += 3;

  // Apply t to initialize data insertion
  grass_apply(GRASS_BP, 1);
  putchar('v');
  GRASS_BP = 1;

  // Apply integers
  for (int i = 0; i < GRASS_N_BITS; i++) {
    const int checkbit = 1 << (GRASS_N_BITS - 1 - i);
    const int nil_index = i + 3;
    const int t_index = i + 2;
    grass_apply(1, (n & checkbit) ? nil_index : t_index);
  }
  putchar('v');
  GRASS_BP = 1;
  putchar('\n');
  return GRASS_BP;
}

static Data* grass_reverse_data(Data* data) {
  Data* prev = NULL;
  while (data) {
    Data* next = data->next;
    data->next = prev;
    prev = data;
    data = next;
  }
  return prev;
}

static void grass_emit_data_list(Data* data) {
  data = grass_reverse_data(data);
  for (; data; data = data->next){
    grass_emit_int_data(data->v);
  }
  // \x.\y.y
  fputs("wwv", stdout);
  GRASS_BP++;
  // Apply nil to end data insertion
  grass_apply(GRASS_BP, 1);
  putchar('v');
  GRASS_BP = 1;
}

static int grass_put_inst_tag(int i_tag) {
  if (i_tag == GRASS_INST_MOV) {
    // \x.\y.\z.\a.\b.\c.\d.\e.e
    fputs("wwwwwwwwv", stdout);
    GRASS_BP++;
  } else {
    // \x.x
    fputs("wv", stdout);
    // (\x.\y.\z.\a.\b.\c.\d.\e. (2 i_tag))
    fputs("wwwwwwww", stdout);
    grass_apply(9, 9 - i_tag);
    putchar('v');
    GRASS_BP += 2;
  }
  return GRASS_BP;
}

static int grass_put_io_tag(int io_tag) {
  if (io_tag == GRASS_IO_EXIT) {
    // \x.\y.\z.z
    fputs("wwwv", stdout);
    GRASS_BP++;
  } else {
    // \x.x
    fputs("wv", stdout);
    // (\x.\y.\z. (2 io_tag))
    fputs("www", stdout);
    grass_apply(4, 4 - io_tag);
    putchar('v');
    GRASS_BP += 2;
  }
  return GRASS_BP;
}

static void grass_emit_cmp_inst(const char* enum_cmp, Inst* inst, int* cons4_1, int* cons4_2, int* cons4_3, int* cons4_4) {
  *cons4_1 = grass_put_inst_tag(GRASS_INST_CMP);
  *cons4_2 = emit_grass_isimm(&inst->src);
  *cons4_3 = emit_grass_value_str(&inst->src);
  fputs(enum_cmp, stdout);
  GRASS_BP += GRASS_CMP_WEIGHT;
  const int cmp_cons_1 = GRASS_BP;
  const int cmp_cons_2 = emit_grass_value_str(&inst->dst);
  putchar('w');
  grass_apply(1, 0 + 1 + GRASS_BP - (cmp_cons_1 - 1));
  grass_apply(1, 1 + 1 + GRASS_BP - (cmp_cons_2 - 1));
  putchar('v');
  GRASS_BP++;
  fputs("\n", stdout);
  *cons4_4 = GRASS_BP;
}

static void grass_emit_jmpcmp_inst(const char* enum_cmp, Inst* inst, int* cons4_1, int* cons4_2, int* cons4_3, int* cons4_4) {
  *cons4_1 = grass_put_inst_tag(GRASS_INST_JMPCMP);
  *cons4_2 = emit_grass_isimm(&inst->src);
  *cons4_3 = emit_grass_value_str(&inst->src);
  fputs(enum_cmp, stdout);
  GRASS_BP += GRASS_CMP_WEIGHT;
  const int jmpcmp_cons_1 = GRASS_BP;
  const int jmpcmp_cons_2 = emit_grass_isimm(&inst->jmp);
  const int jmpcmp_cons_3 = emit_grass_value_str(&inst->jmp);
  const int jmpcmp_cons_4 = emit_grass_value_str(&inst->dst);
  putchar('w');
  grass_apply(1, 0 + 1 + GRASS_BP - (jmpcmp_cons_1 - 1));
  grass_apply(1, 1 + 1 + GRASS_BP - (jmpcmp_cons_2 - 1));
  grass_apply(1, 2 + 1 + GRASS_BP - (jmpcmp_cons_3 - 1));
  grass_apply(1, 3 + 1 + GRASS_BP - (jmpcmp_cons_4 - 1));
  putchar('v');
  GRASS_BP++;
  fputs("\n", stdout);
  *cons4_4 = GRASS_BP;
}

static void grass_emit_inst(Inst* inst) {
  int cons4_1 = 0;
  int cons4_2 = 0;
  int cons4_3 = 0;
  int cons4_4 = 0;

  switch (inst->op) {
  case MOV: {
    cons4_1 = grass_put_inst_tag(GRASS_INST_MOV);
    cons4_2 = emit_grass_isimm(&inst->src);
    cons4_3 = emit_grass_value_str(&inst->src);
    cons4_4 = emit_grass_value_str(&inst->dst);
    break;
  }
  case LOAD: {
    cons4_1 = grass_put_inst_tag(GRASS_INST_LOAD);
    cons4_2 = emit_grass_isimm(&inst->src);
    cons4_3 = emit_grass_value_str(&inst->src);
    cons4_4 = emit_grass_value_str(&inst->dst);
    break;
  }
  case STORE: {
    cons4_1 = grass_put_inst_tag(GRASS_INST_STORE);
    cons4_2 = emit_grass_isimm(&inst->src);
    cons4_3 = emit_grass_value_str(&inst->src);
    cons4_4 = emit_grass_value_str(&inst->dst);
    break;
  }

  case ADD: {
    cons4_1 = grass_put_inst_tag(GRASS_INST_ADDSUB);
    cons4_2 = emit_grass_isimm(&inst->src);
    cons4_3 = emit_grass_value_str(&inst->src);
    const int add_cons_1 = emit_grass_value_str(&inst->dst);
    const int add_cons_2 = grass_put_t_nil(1);
    putchar('w');
    grass_apply(1, 0 + 1 + GRASS_BP - (add_cons_1 - 1));
    grass_apply(1, 1 + 1 + GRASS_BP - (add_cons_2 - 1));
    putchar('v');
    GRASS_BP++;
    cons4_4 = GRASS_BP;
    break;
  }
  case SUB: {
    cons4_1 = grass_put_inst_tag(GRASS_INST_ADDSUB);
    cons4_2 = emit_grass_isimm(&inst->src);
    cons4_3 = emit_grass_value_str(&inst->src);
    const int add_cons_1 = emit_grass_value_str(&inst->dst);
    const int add_cons_2 = grass_put_t_nil(0);
    putchar('w');
    grass_apply(1, 0 + 1 + GRASS_BP - (add_cons_1 - 1));
    grass_apply(1, 1 + 1 + GRASS_BP - (add_cons_2 - 1));
    putchar('v');
    GRASS_BP++;
    cons4_4 = GRASS_BP;
    break;
  }

  case EQ: grass_emit_cmp_inst(GRASS_CMP_EQ, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;
  case NE: grass_emit_cmp_inst(GRASS_CMP_NE, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;
  case LT: grass_emit_cmp_inst(GRASS_CMP_LT, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;
  case GT: grass_emit_cmp_inst(GRASS_CMP_GT, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;
  case LE: grass_emit_cmp_inst(GRASS_CMP_LE, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;
  case GE: grass_emit_cmp_inst(GRASS_CMP_GE, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;

  case JEQ: grass_emit_jmpcmp_inst(GRASS_CMP_EQ, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;
  case JNE: grass_emit_jmpcmp_inst(GRASS_CMP_NE, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;
  case JLT: grass_emit_jmpcmp_inst(GRASS_CMP_LT, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;
  case JGT: grass_emit_jmpcmp_inst(GRASS_CMP_GT, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;
  case JLE: grass_emit_jmpcmp_inst(GRASS_CMP_LE, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;
  case JGE: grass_emit_jmpcmp_inst(GRASS_CMP_GE, inst, &cons4_1, &cons4_2, &cons4_3, &cons4_4); break;

  case JMP: {
    cons4_1 = grass_put_inst_tag(GRASS_INST_JMP);
    cons4_2 = emit_grass_isimm(&inst->jmp);
    cons4_3 = emit_grass_value_str(&inst->jmp);
    cons4_4 = grass_put_t_nil(0);
    break;
  }

  case PUTC: {
    cons4_1 = grass_put_inst_tag(GRASS_INST_IO);
    cons4_2 = emit_grass_isimm(&inst->src);
    cons4_3 = emit_grass_value_str(&inst->src);
    cons4_4 = grass_put_io_tag(GRASS_IO_PUTC);
    break;
  }
  case GETC: {
    cons4_1 = grass_put_inst_tag(GRASS_INST_IO);
    cons4_2 = grass_put_t_nil(0);
    cons4_3 = emit_grass_value_str(&inst->dst);
    cons4_4 = grass_put_io_tag(GRASS_IO_GETC);
    break;
  }

  case EXIT: {
    cons4_1 = grass_put_inst_tag(GRASS_INST_IO);
    cons4_2 = grass_put_t_nil(0);
    cons4_3 = grass_put_t_nil(0);
    cons4_4 = grass_put_io_tag(GRASS_IO_EXIT);
    break;
  }
  case DUMP: {
    cons4_1 = grass_put_inst_tag(GRASS_INST_MOV);
    cons4_2 = grass_put_t_nil(0);
    grass_emit_reg_helper(GRASS_REG_A, GRASS_REG_A_WEIGHT);
    cons4_3 = GRASS_BP;
    grass_emit_reg_helper(GRASS_REG_A, GRASS_REG_A_WEIGHT);
    cons4_4 = GRASS_BP;
    break;
  }

  default:
    error("oops");
  }

  putchar('w');
  grass_apply(1, 0 + 1 + GRASS_BP - (cons4_1 - 1));
  grass_apply(1, 1 + 1 + GRASS_BP - (cons4_2 - 1));
  grass_apply(1, 2 + 1 + GRASS_BP - (cons4_3 - 1));
  grass_apply(1, 3 + 1 + GRASS_BP - (cons4_4 - 1));
  putchar('v');
  GRASS_BP++;

  grass_apply(GRASS_BP, 1);
  putchar('v');
  GRASS_BP = 1;
  fputs("\n", stdout);
}

static Inst* grass_reverse_instructions(Inst* inst) {
  Inst* prev = NULL;
  while (inst) {
    Inst* next = inst->next;
    inst->next = prev;
    prev = inst;
    inst = next;
  }
  return prev;
}

static Inst* grass_emit_chunk(Inst* inst) {
  const int init_pc = inst->pc;
  for (; inst && (inst->pc == init_pc); inst = inst->next) {
    grass_emit_inst(inst);
  }
  grass_put_t_nil(0);
  grass_apply(GRASS_BP, 1);
  putchar('v');
  GRASS_BP = 1;
  putchar('\n');
  return inst;
}

static void grass_emit_text_list(Inst* inst) {
  inst = grass_reverse_instructions(inst);
  
  while (inst) {
    inst = grass_emit_chunk(inst);
  }
  grass_put_t_nil(0);
  grass_apply(GRASS_BP, 1);
  putchar('v');
  GRASS_BP = 1;
}

void target_w(Module* module) {
  fputs(GRASS_VM, stdout);
  putchar('v');
  GRASS_BP = 1;
  putchar('\n');
  grass_emit_data_list(module->data);
  putchar('\n');
  grass_emit_text_list(module->text);
  putchar('\n');
  // main
  fputs("wv", stdout);
  fputs("wWWwww", stdout);
}
