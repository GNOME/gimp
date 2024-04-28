/* scheme-private.h */

#ifndef _SCHEME_PRIVATE_H
#define _SCHEME_PRIVATE_H

#include "scheme.h"
/*------------------ Ugly internals -----------------------------------*/
/*------------------ Of interest only to FFI users --------------------*/

/* FUTURE: should have bit fields.
 * With separate enums for each field.
 */
/* Direction bit field:
 *    00 fully closed
 *    01 open for input
 *    10 open for output
 *    11 open for input or output
 */
/* !!! A struct port retains whether the port
 * is-a string or file port, even when closed.
 */
enum PortData {
  /* Kind */
  port_file=1,
  port_string=2,
  /* Direction. */
  port_input=4,
  port_output=8,
  /* A state, for all kinds. */
  port_saw_EOF=16
};

typedef struct port {
  unsigned int kind;
  union {
    struct {
      FILE *file;
      int closeit;
#if SHOW_ERROR_LINE
      int curr_line;
      char *filename;
#endif
    } stdio;
    struct {
      char *start;
      char *past_the_end;
      char *curr;
    } string;
  } rep;
} port;

/* cell structure */
struct cell {
  unsigned int _flag;
  union {
    struct {
      char   *_svalue;
      int   _length;
    } _string;
    num _number;
    port *_port;
    foreign_func _ff;
    struct {
      struct cell *_car;
      struct cell *_cdr;
    } _cons;
  } _object;
};

struct scheme {
/* arrays for segments */
func_alloc malloc;
func_dealloc free;

/* return code */
int retcode;
int tracing;


#ifndef CELL_SEGSIZE
#define CELL_SEGSIZE    25000 /* # of cells in one segment */
#endif
#ifndef CELL_NSEGMENT
#define CELL_NSEGMENT   50    /* # of segments for cells */
#endif
char *alloc_seg[CELL_NSEGMENT];
pointer cell_seg[CELL_NSEGMENT];
int     last_cell_seg;

/* We use 5 registers. */
pointer args;            /* register for arguments of function */
pointer envir;           /* stack register for current environment */
pointer code;            /* register for current code */
pointer dump;            /* stack register for next evaluation */
pointer foreign_error;   /* used for foreign functions to signal an error */

int interactive_repl;    /* are we in an interactive REPL? */
int print_output;        /* set to 1 to print results and error messages */

struct cell _sink;
pointer sink;            /* when mem. alloc. fails */
struct cell _NIL;
pointer NIL;             /* special cell representing empty cell */
struct cell _HASHT;
pointer T;               /* special cell representing #t */
struct cell _HASHF;
pointer F;               /* special cell representing #f */
struct cell _EOF_OBJ;
pointer EOF_OBJ;         /* special cell representing end-of-file object */
pointer oblist;          /* pointer to symbol table */
pointer global_env;      /* pointer to global environment */

pointer c_nest;          /* stack for nested calls from C */

/* global pointers to special symbols */
pointer LAMBDA;          /* pointer to syntax lambda */
pointer QUOTE;           /* pointer to syntax quote */

pointer QQUOTE;          /* pointer to symbol quasiquote */
pointer UNQUOTE;         /* pointer to symbol unquote */
pointer UNQUOTESP;       /* pointer to symbol unquote-splicing */
pointer FEED_TO;         /* => */
pointer COLON_HOOK;      /* *colon-hook* */
pointer ERROR_HOOK;      /* *error-hook* */
pointer SHARP_HOOK;      /* *sharp-hook* */
pointer COMPILE_HOOK;    /* *compile-hook* */

pointer free_cell;       /* pointer to top of free cells */
long    fcells;          /* # of free cells */

pointer inport;
pointer outport;
pointer save_inport;
pointer loadport;
port   *errport;          /* Not a cell, a port struct. */

#ifndef MAXFIL
#define MAXFIL 64
#endif
port load_stack[MAXFIL]; /* Stack of open files for port -1 (LOADing) */
int nesting_stack[MAXFIL];
int file_i;
int nesting;

char    gc_verbose;      /* if gc_verbose is not zero, print gc status */
char    no_memory;       /* Whether mem. alloc. has failed */

#ifndef LINESIZE
#define LINESIZE 1024
#endif
char    linebuff[LINESIZE];
#ifndef STRBUFFSIZE
#define STRBUFFSIZE 1024
#endif
char    strbuff[STRBUFFSIZE];

FILE *tmpfp;
int tok;
int print_flag;
pointer value;
int op;

void *ext_data;      /* For the benefit of foreign functions */
long gensym_cnt;

struct scheme_interface *vptr;
void *dump_base;     /* pointer to base of allocated dump stack */
int dump_size;       /* number of frames allocated for dump stack */
};

/* operator code */
enum scheme_opcodes {
#define _OP_DEF(A,B,C,D,E,OP) OP,
#include "opdefines.h"
  OP_MAXDEFINED
};

#ifdef __cplusplus
extern "C" {
#endif

#define cons(sc,a,b) _cons(sc,a,b,0)
#define immutable_cons(sc,a,b) _cons(sc,a,b,1)

int is_string(pointer p);
char *string_value(pointer p);
int is_number(pointer p);
num nvalue(pointer p);
long ivalue(pointer p);
double rvalue(pointer p);
int is_integer(pointer p);
int is_real(pointer p);
int is_byte (pointer p);
int is_character(pointer p);
int string_length(pointer p);
guint8 bytevalue (pointer p);
gunichar charvalue(pointer p);
int is_vector(pointer p);

int is_port(pointer p);
int is_inport(pointer p);
int is_outport(pointer p);

int is_pair(pointer p);
pointer pair_car(pointer p);
pointer pair_cdr(pointer p);
pointer set_car(pointer p, pointer q);
pointer set_cdr(pointer p, pointer q);

int is_symbol(pointer p);
char *symname(pointer p);
char *symkey(pointer p);
int hasprop(pointer p);

int is_syntax(pointer p);
int is_proc(pointer p);
int is_foreign(pointer p);
char *syntaxname(pointer p);
int is_closure(pointer p);
int is_macro(pointer p);
pointer closure_code(pointer p);
pointer closure_env(pointer p);

int is_continuation(pointer p);
int is_promise(pointer p);
int is_environment(pointer p);
int is_immutable(pointer p);
void setimmutable(pointer p);

#ifdef __cplusplus
}
#endif

#endif

/*
Local variables:
c-file-style: "k&r"
End:
*/
