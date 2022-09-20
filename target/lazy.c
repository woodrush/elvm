#include <ir/ir.h>
#include <target/util.h>
#include <target/lazycore.h>

// #define DEBUG



// (cons x y) = (lambda (f) (f x y))
// = ``s``si`k[x]`k[y]

static const char LAZY_APPLY[] = "`";
static const char CONS_HEAD[] = "``s``si`k";
static const char CONS_COMMA[] = "`k";

// (cons4 x1 x2 x3 x4) = (lambda (f) (f x1 x2 x3 x4))
// = ``s``s``s``si`k[x1]`k[x2]`k[x3]`k[x4]
static const char CONS4_HEAD[] = "``s``s``s``si`k";



static const char T[] = "k";
static const char NIL[] = "`ki";
static const char LAZY_REG_A[]  = "``s``si`kk`k`ki";
static const char LAZY_REG_B[]  = "``s``si`k`ki`k``s``si`kk`k``s``si`kk`k`ki";
static const char LAZY_REG_C[]  = "``s``si`k`ki`k``s``si`k`ki`k``s``si`k`ki`k``s``si`k`ki`k`ki";
static const char LAZY_REG_D[]  = "``s``si`k`ki`k``s``si`k`ki`k``s``si`kk`k`ki";
static const char LAZY_REG_SP[] = "``s``si`k`ki`k``s``si`kk`k``s``si`k`ki`k`ki";
static const char LAZY_REG_BP[] = "``s``si`k`ki`k``s``si`k`ki`k``s``si`k`ki`k``s``si`kk`k`ki";

static const char INST_IO[] = "``s`kk``s`kk``s`kk``s`kk``s`kk``s`kkk";
static const char INST_JMPCMP[] = "`k``s`kk``s`kk``s`kk``s`kk``s`kkk";
static const char INST_CMP[] = "`k`k``s`kk``s`kk``s`kk``s`kkk";
static const char INST_JMP[] = "`k`k`k``s`kk``s`kk``s`kkk";
static const char INST_LOAD[] = "`k`k`k`k``s`kk``s`kkk";
static const char INST_STORE[] = "`k`k`k`k`k``s`kkk";
static const char INST_ADDSUB[] = "`k`k`k`k`k`kk";
static const char INST_MOV[] = "`k`k`k`k`k`k`ki";

// static const char INST_ADD[] = "`k`k`k`k`k`k`k`ki";
// static const char INST_STORE[] = "`k`k`k`k`k`k`kk";
// static const char INST_MOV[] = "`k`k`k`k`k`k``s`kkk";
// static const char INST_JMP[] = "`k`k`k`k`k``s`kk``s`kkk";
// static const char INST_JUMPCMP[] = "`k`k`k`k``s`kk``s`kk``s`kkk";
// static const char INST_LOAD[] = "`k`k`k``s`kk``s`kk``s`kk``s`kkk";
// static const char INST_CMP[] = "`k`k``s`kk``s`kk``s`kk``s`kk``s`kkk";
// static const char INST_SUB[] = "`k``s`kk``s`kk``s`kk``s`kk``s`kk``s`kkk";
// static const char INST_IO_INT[] = "``s`kk``s`kk``s`kk``s`kk``s`kk``s`kk``s`kkk";

static const char CMP_GT[] = "``s``s``si`k`ki`k`ki`kk";
static const char CMP_LT[] = "``s``s``si`k`ki`kk`k`ki";
static const char CMP_EQ[] = "``s``s``si`kk`k`ki`k`ki";
static const char CMP_LE[] = "``s``s``si`kk`kk`k`ki";
static const char CMP_GE[] = "``s``s``si`kk`k`ki`kk";
static const char CMP_NE[] = "``s``s``si`k`ki`kk`kk";


// static const char CMP_GT[] = "`k`k`k`k`ki";
// static const char CMP_LT[] = "`k`k`k`kk";
// static const char CMP_EQ[] = "`k`k`k``s`kkk";
// static const char CMP_LE[] = "`k`k``s`kk``s`kkk";
// static const char CMP_GE[] = "`k``s`kk``s`kk``s`kkk";
// static const char CMP_NE[] = "``s`kk``s`kk``s`kk``s`kkk";

static const char IO_GETC[] = "``s`kkk";
static const char IO_PUTC[] = "`kk";
static const char IO_EXIT[] = "`k`ki";

// static const char IO_INT_PUTC[] = "`k`ki";
// static const char IO_INT_GETC[] = "`kk";
// static const char IO_INT_EXIT[] = "``s`kkk";

static void lazy_debug(const char* fmt, ...) {
  printf("\n");
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

static const char* lazy_reg(Reg r) {
  switch (r) {
  case A: return LAZY_REG_A;
  case B: return LAZY_REG_B;
  case C: return LAZY_REG_C;
  case D: return LAZY_REG_D;
  case BP: return LAZY_REG_BP;
  case SP: return LAZY_REG_SP;
  default:
    error("unknown register: %d", r);
  }
}

static void lazy_emit_int(int n) {
#ifndef __eir__
    n &= ((1 << 24) - 1);
#endif
  lazy_debug("\n# int %d (%0d)\n", n, n);
  for (int checkbit = 1 << (24 - 1); checkbit; checkbit >>= 1) {
    fputs(CONS_HEAD, stdout);
    fputs((n & checkbit) ? NIL : T, stdout);
    fputs(CONS_COMMA, stdout);
  }
  // for (int i = 0; i < 24; i++) {
  //   fputs(CONS_HEAD, stdout);
  //   lazy_debug("    ");
  //   fputs((n & 1) ? NIL : T, stdout);
  //   lazy_debug("    ");
  //   fputs(CONS_COMMA, stdout);
  //   lazy_debug("\n");
  //   n = n >> 1;
  // }
  fputs(NIL, stdout);
}

static void emit_lazy_isimm(Value* v) {
  if (v->type == REG) {
    fputs(NIL, stdout);
  } else if (v->type == IMM) {
    fputs(T, stdout);
  } else {
    error("invalid value");
  }
}

static void emit_lazy_value_str(Value* v) {
  if (v->type == REG) {
    fputs(lazy_reg(v->reg), stdout);
  } else if (v->type == IMM) {
    lazy_emit_int(v->imm);
  } else {
    error("invalid value");
  }
}

// static void lazy_emit_data(Data* data) {
//   lazy_debug("# Data\n");
//   for (Data* d = data; d; d = d->next) {
//     lazy_debug("\n# data\n");
//     fputs(CONS_HEAD, stdout);
//     lazy_emit_int(d->v);
//     fputs(CONS_COMMA, stdout);
//   }
//   lazy_debug("\n# Data end\n");
//   fputs(NIL, stdout);
// }

// static void lazy_emit_func_prologue(int func_id) {
//   lazy_debug("\n# Func_prologue\n");
//   // fputs(CONS_HEAD, stdout);

//   // Placeholder code that does nothing, to suppress compilation errors
//   if (func_id) {
//     return;
//   }
// }

// static void lazy_emit_pc_change(int pc) {
//   if (pc) {
//     lazy_debug("\n# PC change\n");
//     fputs(NIL, stdout);
//     fputs(CONS_COMMA, stdout);
//     fputs(CONS_HEAD, stdout);
//   }
// }

// static void lazy_emit_func_epilogue(void) {
//   lazy_debug("\n# Func_epilogue\n");
//   // fputs(NIL, stdout);
//   // fputs(CONS_COMMA, stdout);
//   // fputs(NIL, stdout);
// }

static void lazy_emit_basic_inst(Inst* inst, const char* inst_tag) {
    fputs(CONS4_HEAD, stdout);
    fputs(inst_tag, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_isimm(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->dst);
}

static void lazy_emit_addsub_inst(Inst* inst, bool isadd) {
    fputs(CONS4_HEAD, stdout);
    fputs(INST_ADDSUB, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_isimm(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->src);
    fputs(CONS_COMMA, stdout);
    fputs(CONS_HEAD, stdout);
    emit_lazy_value_str(&inst->dst);
    fputs(CONS_COMMA, stdout);
    if (isadd) {
      fputs(T, stdout);
    } else {
      fputs(NIL, stdout);
    }
}

static void lazy_emit_jumpcmp_inst(Inst* inst, const char* cmp_tag) {
    fputs(CONS4_HEAD, stdout);
    fputs(INST_JMPCMP, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_isimm(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->src);
    fputs(CONS_COMMA, stdout);
    fputs(CONS4_HEAD, stdout);
    fputs(cmp_tag, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->dst);
    fputs(CONS_COMMA, stdout);
    emit_lazy_isimm(&inst->jmp);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->jmp);
}

static void lazy_emit_cmp_inst(Inst* inst, const char* cmp_tag) {
    fputs(CONS4_HEAD, stdout);
    fputs(INST_CMP, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_isimm(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->src);
    fputs(CONS_COMMA, stdout);
    fputs(CONS_HEAD, stdout);
    fputs(cmp_tag, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->dst);
}


static void lazy_emit_inst(Inst* inst) {
  // lazy_debug("\n# Inst\n");
  // fputs(CONS_HEAD, stdout);
  // lazy_debug("\n# Inst-body (%d)\n", inst->op);
  switch (inst->op) {
  case MOV:
    lazy_emit_basic_inst(inst, INST_MOV);
    break;
  case ADD:
    lazy_emit_addsub_inst(inst, true);
    break;
  case SUB:
    lazy_emit_addsub_inst(inst, false);
    break;
  case LOAD:
    lazy_emit_basic_inst(inst, INST_LOAD);
    break;
  case STORE:
    lazy_emit_basic_inst(inst, INST_STORE);
    break;

  case PUTC: //5
    // (cons4 inst-io-int _ [src] io-int-putc)
    printf("\n# PUTC\n");
    fputs(CONS4_HEAD, stdout);
    fputs(INST_IO, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_isimm(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->src);
    fputs(CONS_COMMA, stdout);
    fputs(IO_PUTC, stdout);
    break;

  case GETC:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_IO, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(NIL, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->dst);
    fputs(CONS_COMMA, stdout);
    fputs(IO_GETC, stdout);
    break;

  case EXIT:
    printf("# EXT\n");
    fputs(CONS4_HEAD, stdout);
    fputs(INST_IO, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(NIL, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(NIL, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(IO_EXIT, stdout);
    break;

  case DUMP:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_MOV, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(NIL, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(LAZY_REG_A, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(LAZY_REG_A, stdout);
    break;

  // (cons4 inst-cmp [src-isimm] [src] (cons [emum-cmp] [dst]))
  case EQ:
    lazy_emit_cmp_inst(inst, CMP_EQ);
    break;
  case NE: //10
    lazy_emit_cmp_inst(inst, CMP_NE);
    break;
  case LT:
    lazy_emit_cmp_inst(inst, CMP_LT);
    break;
  case GT:
    lazy_emit_cmp_inst(inst, CMP_GT);
    break;
  case LE:
    lazy_emit_cmp_inst(inst, CMP_LE);
    break;
  case GE: //14
    lazy_emit_cmp_inst(inst, CMP_GE);
    break;

  case JEQ:
    lazy_emit_jumpcmp_inst(inst, CMP_EQ);
    break;
  case JNE:
    lazy_emit_jumpcmp_inst(inst, CMP_NE);
    break;
  case JLT:
    lazy_emit_jumpcmp_inst(inst, CMP_LT);
    break;
  case JGT:
    lazy_emit_jumpcmp_inst(inst, CMP_GT);
    break;
  case JLE:
    lazy_emit_jumpcmp_inst(inst, CMP_LE);
    break;
  case JGE:
    lazy_emit_jumpcmp_inst(inst, CMP_GE);
    break;

  case JMP:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_JMP, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_isimm(&inst->jmp);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->jmp);
    fputs(CONS_COMMA, stdout);
    fputs(NIL, stdout);
    break;

  default:
    error("oops");
  }
  // lazy_debug("\n# Inst-end\n");
  // fputs(CONS_COMMA, stdout);
}

static Inst* lazy_emit_chunk(Inst* inst) {
  const int init_pc = inst->pc;
  for (; inst && (inst->pc == init_pc); inst = inst->next) {
    fputs(CONS_HEAD, stdout);
    printf("\n# ST\n");
    lazy_emit_inst(inst);
    printf("\n# ST/\n");
    fputs(CONS_COMMA, stdout);
  }
  fputs(NIL, stdout);
  return inst;
}

static void lazy_emit_data_list(Data* data) {
  for (; data; data = data->next){
    fputs(CONS_HEAD, stdout);
    lazy_emit_int(data->v);
    fputs(CONS_COMMA, stdout);
  }
  fputs(NIL, stdout);
}

static void lazy_emit_text_list(Inst* inst) {
  while (inst) {
    fputs(CONS_HEAD, stdout);
    inst = lazy_emit_chunk(inst);
    printf("\n# newpc \n");
    fputs(CONS_COMMA, stdout);
  }
  fputs(NIL, stdout);
}

void target_lazy(Module* module) {
  // printf("``");
  // fputs(lazy_core, stdout);
  // printf("\n");

  // lazy_emit_data(module->data);
  // fputs(CONS_HEAD, stdout);
  // printf("\n");
  // emit_chunked_main_loop(module->text,
  //                       lazy_emit_func_prologue,
  //                       lazy_emit_func_epilogue,
  //                       lazy_emit_pc_change,
  //                       lazy_emit_inst);
  // fputs(NIL, stdout);
  // fputs(CONS_COMMA, stdout);
  // fputs(NIL, stdout);

  fputs(LAZY_APPLY, stdout);
  fputs(LAZY_APPLY, stdout);
  fputs(lazy_core, stdout);
  printf("\n# data\n");
  lazy_emit_data_list(module->data);
  printf("\n# text\n");
  lazy_emit_text_list(module->text);
}



