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
// static const char CONS4_HEAD[] = "``s``s``s``si`k";

static const char T[] = "k";
static const char NIL[] = "`ki";
// static const char REG_A[] = "``s`kk``s`kk``s`kk``s`kkk";
// static const char REG_B[] = "`k``s`kk``s`kk``s`kkk";
// static const char REG_C[] = "`k`k``s`kk``s`kkk";
// static const char REG_D[] = "`k`k`k``s`kkk";
// static const char REG_SP[] = "`k`k`k`kk";
// static const char REG_BP[] = "`k`k`k`k`ki";
// static const char INST_IO_INT[] = "``s`kk``s`kk``s`kk``s`kk``s`kk``s`kk``s`kkk";
// static const char INST_SUB[] = "`k``s`kk``s`kk``s`kk``s`kk``s`kk``s`kkk";
// static const char INST_CMP[] = "`k`k``s`kk``s`kk``s`kk``s`kk``s`kkk";
// static const char INST_LOAD[] = "`k`k`k``s`kk``s`kk``s`kk``s`kkk";
// static const char INST_JUMPCMP[] = "`k`k`k`k``s`kk``s`kk``s`kkk";
// static const char INST_JMP[] = "`k`k`k`k`k``s`kk``s`kkk";
// static const char INST_MOV[] = "`k`k`k`k`k`k``s`kkk";
// static const char INST_STORE[] = "`k`k`k`k`k`k`kk";
// static const char INST_ADD[] = "`k`k`k`k`k`k`k`ki";
// static const char CMP_EQ[] = "``s`kk``s`kk``s`kk``s`kkk";
// static const char CMP_NE[] = "`k``s`kk``s`kk``s`kkk";
// static const char CMP_LT[] = "`k`k``s`kk``s`kkk";
// static const char CMP_GT[] = "`k`k`k``s`kkk";
// static const char CMP_LE[] = "`k`k`k`kk";
// static const char CMP_GE[] = "`k`k`k`k`ki";
// static const char IO_INT_EXIT[] = "``s`kkk";
// static const char IO_INT_GETC[] = "`kk";
// static const char IO_INT_PUTC[] = "`k`ki";

static const char STRING_TERM[] = "```s`k``sii``s``s`ks``s`kk``s`ks``s`k`sik`k``s`kk``sii```sii```sii``s``s`ksk``s``s`ksk`ki";

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

static void lazy_emit_int(int n) {
  lazy_debug("\n# int %d (%0d)\n", n, n);
  for (int i = 0; i < 24; i++) {
    fputs(CONS_HEAD, stdout);
    fputs(n % 2 ? T : NIL, stdout);
    fputs(CONS_COMMA, stdout);
    n = n >> 1;
  }
  fputs(NIL, stdout);
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

  fputs(CONS_HEAD, stdout);
  fputs(NIL, stdout);

  // Placeholder code that does nothing, to suppress compilation errors
  if (func_id) {
    return;
  }
}

static void lazy_emit_func_epilogue(void) {
  lazy_debug("\n# Func_epilogue\n");
  fputs(NIL, stdout);
  fputs(CONS_COMMA, stdout);
  fputs(NIL, stdout);
}


static void lazy_emit_pc_change(int pc) {
  lazy_debug("\n# PC change\n");
  fputs(NIL, stdout);
  fputs(CONS_COMMA, stdout);
  fputs(CONS_HEAD, stdout);

  // Placeholder code that does nothing, to suppress compilation errors
  if (pc) {
    return;
  }
}

static void lazy_emit_inst(Inst* inst) {
  lazy_debug("\n# Inst\n");
  fputs(CONS_HEAD, stdout);
  lazy_debug("\n# Inst-body (%d) (%d)\n", inst->op, EXIT);
  switch (inst->op) {
  case MOV:
    fputs(STRING_TERM, stdout);
    break;

  case ADD:
    fputs(STRING_TERM, stdout);
    break;

  case SUB:
    fputs(STRING_TERM, stdout);
    break;

  case LOAD:
    fputs(STRING_TERM, stdout);
    break;

  case STORE:
    fputs(STRING_TERM, stdout);
    break;

  case PUTC:
    fputs(STRING_TERM, stdout);
    break;

  case GETC:
    fputs(STRING_TERM, stdout);
    break;

  case EXIT:
    fputs(STRING_TERM, stdout);
    break;

  case DUMP:
    fputs(STRING_TERM, stdout);
    break;

  case EQ:
    fputs(STRING_TERM, stdout);
    break;
  case NE:
    fputs(STRING_TERM, stdout);
    break;
  case LT:
    fputs(STRING_TERM, stdout);
    break;
  case GT:
    fputs(STRING_TERM, stdout);
    break;
  case LE:
    fputs(STRING_TERM, stdout);
    break;
  case GE:
    fputs(STRING_TERM, stdout);
    break;

  case JEQ:
    fputs(STRING_TERM, stdout);
    break;
  case JNE:
    fputs(STRING_TERM, stdout);
    break;
  case JLT:
    fputs(STRING_TERM, stdout);
    break;
  case JGT:
    fputs(STRING_TERM, stdout);
    break;
  case JLE:
    fputs(STRING_TERM, stdout);
    break;
  case JGE:
    fputs(STRING_TERM, stdout);
    break;
  case JMP:
    fputs(STRING_TERM, stdout);
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

  // fputs(CONS_HEAD, stdout);
  // fputs(NIL, stdout);
  // fputs(CONS_COMMA, stdout);
  // fputs(NIL, stdout);

  // fputs(CONS_HEAD, stdout);
  // fputs(NIL, stdout);
  // fputs(CONS_COMMA, stdout);
  // fputs(NIL, stdout);

  lazy_emit_data(module->data);
  emit_chunked_main_loop(module->text,
                        lazy_emit_func_prologue,
                        lazy_emit_func_epilogue,
                        lazy_emit_pc_change,
                        lazy_emit_inst);
}
