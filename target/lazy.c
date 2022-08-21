#include <ir/ir.h>
#include <target/util.h>
#include <target/lazycore.h>

// #define DEBUG


// (cons x y) = (lambda (f) (f x y))
// = ``s``si`k[x]`k[y]
static const char CONS_HEAD[] = "``s``si`k";
static const char CONS_COMMA[] = "`k";

// (cons4 x1 x2 x3 x4) = (lambda (f) (f x1 x2 x3 x4))
// = ``s``s``s``si`k[x1]`k[x2]`k[x3]`k[x4]
static const char CONS4_HEAD[] = "``s``s``s``si`k";

static const char T[] = "k";
static const char NIL[] = "`ki";
static const char REG_A[] = "``s`kk``s`kk``s`kk``s`kkk";
static const char REG_B[] = "`k``s`kk``s`kk``s`kkk";
static const char REG_C[] = "`k`k``s`kk``s`kkk";
static const char REG_D[] = "`k`k`k``s`kkk";
static const char REG_SP[] = "`k`k`k`kk";
static const char REG_BP[] = "`k`k`k`k`ki";
static const char INST_IO_INT[] = "``s`kk``s`kk``s`kk``s`kk``s`kk``s`kk``s`kkk";
static const char INST_SUB[] = "`k``s`kk``s`kk``s`kk``s`kk``s`kk``s`kkk";
static const char INST_CMP[] = "`k`k``s`kk``s`kk``s`kk``s`kk``s`kkk";
static const char INST_LOAD[] = "`k`k`k``s`kk``s`kk``s`kk``s`kkk";
static const char INST_JUMPCMP[] = "`k`k`k`k``s`kk``s`kk``s`kkk";
static const char INST_JMP[] = "`k`k`k`k`k``s`kk``s`kkk";
static const char INST_MOV[] = "`k`k`k`k`k`k``s`kkk";
static const char INST_STORE[] = "`k`k`k`k`k`k`kk";
static const char INST_ADD[] = "`k`k`k`k`k`k`k`ki";
static const char CMP_EQ[] = "``s`kk``s`kk``s`kk``s`kkk";
static const char CMP_NE[] = "`k``s`kk``s`kk``s`kkk";
static const char CMP_LT[] = "`k`k``s`kk``s`kkk";
static const char CMP_GT[] = "`k`k`k``s`kkk";
static const char CMP_LE[] = "`k`k`k`kk";
static const char CMP_GE[] = "`k`k`k`k`ki";
static const char IO_INT_EXIT[] = "``s`kkk";
static const char IO_INT_GETC[] = "`kk";
static const char IO_INT_PUTC[] = "`k`ki";

static void lazy_debug(const char* fmt, ...) {
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
  case A: return REG_A;
  case B: return REG_B;
  case C: return REG_C;
  case D: return REG_D;
  case BP: return REG_BP;
  case SP: return REG_SP;
  default:
    error("unknown register: %d", r);
  }
}

static void lazy_emit_int(int n) {
  lazy_debug("\n# int %d (%0d)\n", n, n);
  for (int i = 0; i < 24; i++) {
    fputs(CONS_HEAD, stdout);
    lazy_debug("    ");
    fputs((n & 1) ? T : NIL, stdout);
    lazy_debug("    ");
    fputs(CONS_COMMA, stdout);
    lazy_debug("\n");
    n = n >> 1;
  }
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

static void lazy_emit_data(Data* data) {
  lazy_debug("# Data\n");
  for (Data* d = data; d; d = d->next) {
    if (d->v) {
      lazy_debug("\n# data\n");
      fputs(CONS_HEAD, stdout);
      lazy_emit_int(d->v);
      fputs(CONS_COMMA, stdout);
    }
  }
  lazy_debug("\n# Data end\n");
  fputs(NIL, stdout);
}

static void lazy_emit_func_prologue(int func_id) {
  lazy_debug("\n# Func_prologue\n");
  fputs(CONS_HEAD, stdout);

  // Placeholder code that does nothing, to suppress compilation errors
  if (func_id) {
    return;
  }
}

static void lazy_emit_pc_change(int pc) {
  if (pc) {
    lazy_debug("\n# PC change\n");
    fputs(NIL, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(CONS_HEAD, stdout);
  }
}

static void lazy_emit_func_epilogue(void) {
  lazy_debug("\n# Func_epilogue\n");
  fputs(NIL, stdout);
  fputs(CONS_COMMA, stdout);
  fputs(NIL, stdout);
}

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

static void lazy_emit_jumpcmp_inst(Inst* inst, const char* cmp_tag) {
    fputs(CONS4_HEAD, stdout);
    fputs(INST_JUMPCMP, stdout);
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
  lazy_debug("\n# Inst\n");
  fputs(CONS_HEAD, stdout);
  lazy_debug("\n# Inst-body (%d)\n", inst->op);
  switch (inst->op) {
  case MOV:
    lazy_emit_basic_inst(inst, INST_MOV);
    break;
  case ADD:
    lazy_emit_basic_inst(inst, INST_ADD);
    break;
  case SUB:
    lazy_emit_basic_inst(inst, INST_SUB);
    break;
  case LOAD:
    lazy_emit_basic_inst(inst, INST_LOAD);
    break;
  case STORE:
    lazy_emit_basic_inst(inst, INST_STORE);
    break;

  case PUTC: //5
    // (cons4 inst-io-int _ [src] io-int-putc)
    fputs(CONS4_HEAD, stdout);
    fputs(INST_IO_INT, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_isimm(&inst->src);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->src);
    fputs(CONS_COMMA, stdout);
    fputs(IO_INT_PUTC, stdout);
    break;

  case GETC:
    fputs(CONS4_HEAD, stdout);
    fputs(INST_IO_INT, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(NIL, stdout);
    fputs(CONS_COMMA, stdout);
    emit_lazy_value_str(&inst->dst);
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
    fputs(REG_A, stdout);
    fputs(CONS_COMMA, stdout);
    fputs(REG_A, stdout);
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
  lazy_debug("\n# Inst-end\n");
  fputs(CONS_COMMA, stdout);
}

void target_lazy(Module* module) {
  #ifndef DEBUG
    printf("``");
    fputs(lazy_core, stdout);
  #endif

  lazy_emit_data(module->data);
  emit_chunked_main_loop(module->text,
                        lazy_emit_func_prologue,
                        lazy_emit_func_epilogue,
                        lazy_emit_pc_change,
                        lazy_emit_inst);
}
