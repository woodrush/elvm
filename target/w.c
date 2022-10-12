#include <ir/ir.h>
#include <target/util.h>
#include <target/wcore.h>



static const int GRASS_N_BITS = 24;


static const int GRASS_INST_MOV = 8;
// static const int GRASS_INST_ADDSUB = 7;
// static const int GRASS_INST_STORE = 6;
// static const int GRASS_INST_LOAD = 5;
static const int GRASS_INST_JMP = 4;
// static const int GRASS_INST_CMP = 3;
// static const int GRASS_INST_JMPCMP = 2;
static const int GRASS_INST_IO = 1;
// static const int GRASS_IO_GETC = 1;
static const int GRASS_IO_PUTC = 2;
static const int GRASS_IO_EXIT = 3;


// static const char* grass_reg(Reg r) {
//   switch (r) {
//   case A: return GRASS_REG_A;
//   case B: return GRASS_REG_B;
//   case C: return GRASS_REG_C;
//   case D: return GRASS_REG_D;
//   case BP: return GRASS_REG_BP;
//   case SP: return GRASS_REG_SP;
//   default:
//     error("unknown register: %d", r);
//   }
// }

// static void grass_emit_int(int n) {
// #ifndef __eir__
//   n &= ((1 << GRASS_N_BITS) - 1);
// #endif
//   fputs(GRASS_INT_HEADER, stdout);
//   for (int checkbit = 1 << (GRASS_N_BITS - 1); checkbit; checkbit >>= 1) {
//     fputs((n & checkbit) ? GRASS_INT_BIT1 : GRASS_INT_BIT0, stdout);
//   }
//   fputs(GRASS_INT_FOOTER, stdout);
// }

static int GRASS_BP = 1;

static void grass_apply(int W, int w) {
  for (; W; W--) putchar('W');
  for (; w; w--) putchar('w');
}

static int grass_put_t_nil(int t_nil) {
  if (t_nil) {
    // \x.x
    fputs("t: ", stdout);
    fputs("wv", stdout);
    // \x.\y.(3 2)
    fputs("wwWWWwwv", stdout);
    GRASS_BP += 2;
  } else {
    // \x.\y.y
    fputs("nil: ", stdout);
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
    fputs("\n ", stdout);
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
    // fputs(grass_reg(v->reg), stdout);
  } else if (v->type == IMM) {
    grass_emit_int_inst(v->imm);
  } else {
    error("invalid value");
  }
  return GRASS_BP;
}

static void grass_emit_data_list(Data* data) {
  if (data) {
    fputs("wwv", stdout);
    fputs("WWwv", stdout);
  }
  // fputs(GRASS_NIL, stdout);
  // fputs("WWwv ; Integers\n", stdout);
  // if (!data) {
  //   for (; data; data = data->next){
  //     fputs(GRASS_CONS_HEAD, stdout);
  //     grass_emit_int(data->v);
  //   }
  // }
}

// static void grass_emit_inst_header(const char* inst_tag, Value* v) {
//   fputs(GRASS_CONS4_HEAD, stdout);
//   fputs(inst_tag, stdout);
//   emit_grass_isimm(v);
//   emit_grass_value_str(v);
// }

// static void grass_emit_basic_inst(Inst* inst, const char* inst_tag) {
//   grass_emit_inst_header(inst_tag, &inst->src);
//   emit_grass_value_str(&inst->dst);
// }

// static void grass_emit_addsub_inst(Inst* inst, const char* is_add) {
//   grass_emit_inst_header(GRASS_INST_ADDSUB, &inst->src);
//   fputs(GRASS_CONS_HEAD, stdout);
//   emit_grass_value_str(&inst->dst);
//   fputs(is_add, stdout);
// }

// static void grass_emit_jmpcmp_inst(Inst* inst, const char* cmp_tag) {
//   grass_emit_inst_header(GRASS_INST_JMPCMP, &inst->src);
//   grass_emit_inst_header(cmp_tag, &inst->jmp);
//   emit_grass_value_str(&inst->dst);
// }

// static void grass_emit_cmp_inst(Inst* inst, const char* cmp_tag) {
//   grass_emit_inst_header(GRASS_INST_CMP, &inst->src);
//   fputs(GRASS_CONS_HEAD, stdout);
//   fputs(cmp_tag, stdout);
//   emit_grass_value_str(&inst->dst);
// }

// static void grass_emit_io_inst(const char* io_tag, Value* v) {
//   grass_emit_inst_header(GRASS_INST_IO, v);
//   fputs(io_tag, stdout);
// }

// static void grass_emit_exit_inst() {
//   fputs(GRASS_CONS4_HEAD, stdout);
//   fputs(GRASS_INST_IO, stdout);
//   fputs(GRASS_NIL, stdout);
//   fputs(GRASS_NIL, stdout);
//   fputs(GRASS_IO_EXIT, stdout);
// }

// static void grass_emit_jmp_inst(Inst* inst) {
//   grass_emit_inst_header(GRASS_INST_JMP, &inst->jmp);
//   fputs(GRASS_PLACEHOLDER, stdout);
// }

// static void grass_emit_dump_inst(void) {
//   fputs(GRASS_CONS4_HEAD, stdout);
//   fputs(GRASS_INST_MOV, stdout);
//   fputs(GRASS_NIL, stdout);
//   fputs(GRASS_REG_A, stdout);
//   fputs(GRASS_REG_A, stdout);
// }

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

static void grass_emit_inst(Inst* inst) {
  int cons4_1 = 0;
  int cons4_2 = 0;
  int cons4_3 = 0;
  int cons4_4 = 0;

  switch (inst->op) {
  case MOV: break;
  case LOAD: break;
  case STORE: break;

  case ADD: break;
  case SUB: break;

  case EQ: break;
  case NE: break;
  case LT: break;
  case GT: break;
  case LE: break;
  case GE: break;

  case JEQ: break;
  case JNE: break;
  case JLT: break;
  case JGT: break;
  case JLE: break;
  case JGE: break;

  case JMP: {
    fputs("\njmp\n", stdout);
    cons4_1 = grass_put_inst_tag(GRASS_INST_JMP); fputs("\n", stdout);
    cons4_2 = emit_grass_isimm(&inst->jmp); fputs("\n", stdout);
    cons4_3 = emit_grass_value_str(&inst->jmp); fputs("\n", stdout);
    cons4_4 = grass_put_t_nil(0); fputs("\n", stdout);
    break;
  }

  case PUTC: {
    fputs("\nputc\n", stdout);
    cons4_1 = grass_put_inst_tag(GRASS_INST_IO); fputs("\n", stdout);
    cons4_2 = emit_grass_isimm(&inst->src); fputs("\n", stdout);
    cons4_3 = emit_grass_value_str(&inst->src); fputs("\n", stdout);
    cons4_4 = grass_put_io_tag(GRASS_IO_PUTC); fputs("\n", stdout);
    break;
  }
  case GETC: break;

  case EXIT: {
    fputs("\nexit\n", stdout);
    cons4_1 = grass_put_inst_tag(GRASS_INST_IO); fputs("\n", stdout);
    cons4_2 = grass_put_t_nil(0); fputs("\n", stdout);
    cons4_3 = grass_put_t_nil(0); fputs("\n", stdout);
    cons4_4 = grass_put_io_tag(GRASS_IO_EXIT); fputs("\n", stdout);    
    break;
  }
  case DUMP: break;

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
  fputs("\n", stdout);

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
  return inst;
}

static void grass_emit_text_list(Inst* inst) {
  inst = grass_reverse_instructions(inst);
  
  while (inst) {
    fputs("\nNext pc\n", stdout);
    inst = grass_emit_chunk(inst);
  }
  fputs("\n", stdout);
  grass_put_t_nil(0);
  fputs("\n", stdout);
  grass_apply(GRASS_BP, 1);
  putchar('v');
  GRASS_BP = 1;
}

void target_w(Module* module) {
  fputs(GRASS_VM, stdout);
  putchar('v');
  putchar('\n');
  grass_emit_data_list(module->data);
  putchar('\n');
  grass_emit_text_list(module->text);
  putchar('\n');

  // main
  fputs("wv", stdout);
  fputs("wWWwww", stdout);
}
