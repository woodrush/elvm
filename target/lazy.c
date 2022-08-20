#include <ir/ir.h>
#include <target/util.h>


// static void lazy_emit_line(const char* fmt, ...) {
//   static int lazy_lineno_counter_ = 0;
//   printf("%d. ", lazy_lineno_counter_);
//   lazy_lineno_counter_++;

//   if (fmt[0]) {
//     va_list ap;
//     va_start(ap, fmt);
//     vprintf(fmt, ap);
//     va_end(ap);
//   }
//   putchar('\n');
// }

static void lazy_emit_func_prologue(int func_id) {
  // Placeholder code that does nothing, to suppress compilation errors
  if (func_id) {
    return;
  }
}

static void lazy_emit_func_epilogue(void) {
}


static void lazy_emit_pc_change(int pc) {
  // Placeholder code that does nothing, to suppress compilation errors
  if (pc) {
    return;
  }
}

static void lazy_emit_inst(Inst* inst) {
  switch (inst->op) {
  case MOV:
    break;

  case ADD:
    break;

  case SUB:
    break;

  case LOAD:
    break;

  case STORE:
    break;

  case PUTC:
    break;

  case GETC:
    break;

  case EXIT:
    break;

  case DUMP:
    break;

  case EQ:
    break;
  case NE:
    break;
  case LT:
    break;
  case GT:
    break;
  case LE:
    break;
  case GE:
    break;

  case JEQ:
    break;
  case JNE:
    break;
  case JLT:
    break;
  case JGT:
    break;
  case JLE:
    break;
  case JGE:
    break;
  case JMP:
    break;

  default:
    error("oops");
  }
}

void target_lazy(Module* module) {
  emit_chunked_main_loop(module->text,
                         lazy_emit_func_prologue,
                         lazy_emit_func_epilogue,
                         lazy_emit_pc_change,
                         lazy_emit_inst);
}
