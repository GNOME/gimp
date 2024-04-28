/* T I N Y S C H E M E    1 . 4 2
 *   Dimitrios Souflis (dsouflis@acm.org)
 *   Based on MiniScheme (original credits follow)
 * (MINISCM)               coded by Atsushi Moriwaki (11/5/1989)
 * (MINISCM)           E-MAIL :  moriwaki@kurims.kurims.kyoto-u.ac.jp
 * (MINISCM) This version has been modified by R.C. Secrist.
 * (MINISCM)
 * (MINISCM) Mini-Scheme is now maintained by Akira KIDA.
 * (MINISCM)
 * (MINISCM) This is a revised and modified version by Akira KIDA.
 * (MINISCM)   current version is 0.85k4 (15 May 1994)
 *
 */

/* ******** READ THE FOLLOWING BEFORE MODIFYING THIS FILE! ******** */
/* This copy of TinyScheme has been modified to support UTF-8 coded */
/* character strings. As a result, the length of a string in bytes  */
/* may not be the same as the length of a string in characters. You */
/* must keep this in mind at all times while making any changes to  */
/* the routines in this file and when adding new features.          */
/*                                                                  */
/* UTF-8 modifications made by Kevin Cozens (kcozens@interlog.com)  */
/* **************************************************************** */

#include "config.h"

#define _SCHEME_SOURCE
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if USE_DL
# include "dynload.h"
#endif
#if USE_MATH
# include <math.h>
#endif

#include <limits.h>
#include <float.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "../script-fu-intl.h"

#include "scheme-private.h"
#include "string-port.h"
#include "error-port.h"

#if !STANDALONE
static ts_output_func   ts_output_handler = NULL;
static gpointer         ts_output_data = NULL;

/* Register an output func from a wrapping interpeter.
 * Typically the output func writes to a string, or to stdout.
 */
void
ts_register_output_func (ts_output_func  func,
                         gpointer        user_data)
{
  ts_output_handler = func;
  ts_output_data    = user_data;
}

/* len is length of 'string' in bytes or -1 for null terminated strings */
void
ts_output_string (TsOutputType  type,
                  const char   *string,
                  int           len)
{
  if (len < 0)
    len = strlen (string);

  if (ts_output_handler && len > 0)
    (* ts_output_handler) (type, string, len, ts_output_data);
}

static gboolean
ts_is_output_redirected (void)
{
  return ts_output_handler != NULL;
}

/* Returns string of errors declared by interpreter or script.
 *
 * You must call when a script has returned an error flag.
 * Side effect is to clear: you should only call once.
 *
 * When called when the script did not return an error flag,
 * returns "Unknown error"
 *
 * Returned string is transfered, owned by the caller and must be freed.
 */
const gchar*
ts_get_error_string (scheme *sc)
{
  return error_port_take_string_and_close (sc);
}
#endif

/* Used for documentation purposes, to signal functions in 'interface' */
#define INTERFACE

#define TOK_EOF     (-1)
#define TOK_LPAREN  0
#define TOK_RPAREN  1
#define TOK_DOT     2
#define TOK_ATOM    3
#define TOK_QUOTE   4
#define TOK_COMMENT 5
#define TOK_DQUOTE  6
#define TOK_BQUOTE  7
#define TOK_COMMA   8
#define TOK_ATMARK  9
#define TOK_SHARP   10
#define TOK_SHARP_CONST 11
#define TOK_VEC     12
#define TOK_USCORE  13

#define BACKQUOTE '`'
#define DELIMITERS  "()\";\f\t\v\n\r "

/*
 *  Basic memory allocation units
 */

#define banner "TinyScheme 1.42 (with UTF-8 support)"

#include <string.h>
#include <stdlib.h>

/* Set current outport with checks for validity. */
void
set_outport (scheme * sc, pointer arg)
{
  if (! is_port (arg))
    g_warning ("%s arg not a port", G_STRFUNC);

  if ( ! is_outport (arg) )
    g_warning ("%s port not an output port, or closed", G_STRFUNC);

  sc->outport = arg;
}

#define stricmp utf8_stricmp

static int utf8_stricmp(const char *s1, const char *s2)
{
  char *s1a, *s2a;
  int result;

  s1a = g_utf8_casefold(s1, -1);
  s2a = g_utf8_casefold(s2, -1);

  result = g_utf8_collate(s1a, s2a);

  g_free(s1a);
  g_free(s2a);
  return result;
}

#define min(a, b)  ((a) <= (b) ? (a) : (b))

#if USE_STRLWR
/*
#error FIXME: Can't just use g_utf8_strdown since it allocates a new string
#define strlwr(s)  g_utf8_strdown(s, -1)
*/
#else
#define strlwr(s)  s
#endif

#ifndef prompt
# define prompt "ts> "
#endif

#ifndef InitFile
# define InitFile "init.scm"
#endif

#ifndef FIRST_CELLSEGS
# define FIRST_CELLSEGS 3
#endif

enum scheme_types {
  T_STRING=1,
  T_NUMBER=2,
  T_SYMBOL=3,
  T_PROC=4,
  T_PAIR=5,
  T_CLOSURE=6,
  T_CONTINUATION=7,
  T_FOREIGN=8,
  T_CHARACTER=9,
  T_PORT=10,
  T_VECTOR=11,
  T_MACRO=12,
  T_PROMISE=13,
  T_ENVIRONMENT=14,
  T_BYTE=15,
  T_LAST_SYSTEM_TYPE=15
};

/* ADJ is enough slack to align cells in a TYPE_BITS-bit boundary */
#define ADJ 32
#define TYPE_BITS 5
#define T_MASKTYPE      31    /* 0000000000011111 */
#define T_SYNTAX      4096    /* 0001000000000000 */
#define T_IMMUTABLE   8192    /* 0010000000000000 */
#define T_ATOM       16384    /* 0100000000000000 */   /* only for gc */
#define CLRATOM      49151    /* 1011111111111111 */   /* only for gc */
#define MARK         32768    /* 1000000000000000 */
#define UNMARK       32767    /* 0111111111111111 */

static num num_add(num a, num b);
static num num_mul(num a, num b);
static num num_div(num a, num b);
static num num_intdiv(num a, num b);
static num num_sub(num a, num b);
static num num_rem(num a, num b);
static num num_mod(num a, num b);
static int num_eq(num a, num b);
static int num_gt(num a, num b);
static int num_ge(num a, num b);
static int num_lt(num a, num b);
static int num_le(num a, num b);

#if USE_MATH
static double round_per_R5RS(double x);
#endif
static int is_zero_double(double x);
static INLINE int num_is_integer(pointer p) {
  return ((p)->_object._number.is_fixnum);
}

static num num_zero;
static num num_one;

/* macros for cell operations */
#define typeflag(p)      ((p)->_flag)
#define type(p)          (typeflag(p)&T_MASKTYPE)

INTERFACE INLINE int is_string(pointer p)     { return (type(p)==T_STRING); }
#define strvalue(p)      ((p)->_object._string._svalue)
#define strlength(p)     ((p)->_object._string._length)

INTERFACE static int is_list(scheme *sc, pointer a);
INTERFACE INLINE int is_vector(pointer p)    { return (type(p)==T_VECTOR); }
INTERFACE static void fill_vector(pointer vec, pointer obj);
INTERFACE static pointer vector_elem(pointer vec, int ielem);
INTERFACE static pointer set_vector_elem(pointer vec, int ielem, pointer a);
INTERFACE INLINE int is_number(pointer p)    { return (type(p)==T_NUMBER); }
INTERFACE INLINE int is_integer(pointer p) {
  if (!is_number(p))
      return 0;
  if (num_is_integer(p) || (double)ivalue(p) == rvalue(p))
      return 1;
  return 0;
}

INTERFACE INLINE int is_real(pointer p) {
  return is_number(p) && (!(p)->_object._number.is_fixnum);
}

INTERFACE INLINE int is_byte (pointer p) { return type (p) == T_BYTE; }
INTERFACE INLINE int is_character(pointer p) { return (type(p)==T_CHARACTER); }
INTERFACE INLINE int string_length(pointer p) { return strlength(p); }
INTERFACE INLINE char *string_value(pointer p) { return strvalue(p); }
INLINE num nvalue(pointer p)       { return ((p)->_object._number); }
INTERFACE long ivalue(pointer p)      { return (num_is_integer(p)?(p)->_object._number.value.ivalue:(long)(p)->_object._number.value.rvalue); }
INTERFACE double rvalue(pointer p)    { return (!num_is_integer(p)?(p)->_object._number.value.rvalue:(double)(p)->_object._number.value.ivalue); }
#define ivalue_unchecked(p)       ((p)->_object._number.value.ivalue)
#define rvalue_unchecked(p)       ((p)->_object._number.value.rvalue)
#define set_num_integer(p)   (p)->_object._number.is_fixnum=1;
#define set_num_real(p)      (p)->_object._number.is_fixnum=0;
INTERFACE guint8 bytevalue (pointer p) { return (guint8)ivalue_unchecked (p); }
INTERFACE  gunichar charvalue(pointer p)  { return (gunichar)ivalue_unchecked(p); }

INTERFACE INLINE int is_port(pointer p)     { return (type(p)==T_PORT); }
INTERFACE INLINE int is_inport(pointer p)  { return is_port(p) && p->_object._port->kind & port_input; }
INTERFACE INLINE int is_outport(pointer p) { return is_port(p) && p->_object._port->kind & port_output; }

INTERFACE INLINE int is_pair(pointer p)     { return (type(p)==T_PAIR); }
#define car(p)           ((p)->_object._cons._car)
#define cdr(p)           ((p)->_object._cons._cdr)
INTERFACE pointer pair_car(pointer p)   { return car(p); }
INTERFACE pointer pair_cdr(pointer p)   { return cdr(p); }
INTERFACE pointer set_car(pointer p, pointer q) { return car(p)=q; }
INTERFACE pointer set_cdr(pointer p, pointer q) { return cdr(p)=q; }

INTERFACE INLINE int is_symbol(pointer p)   { return (type(p)==T_SYMBOL); }
INTERFACE INLINE char *symname(pointer p)   { return strvalue(car(p)); }
#if USE_PLIST
SCHEME_EXPORT INLINE int hasprop(pointer p)     { return (typeflag(p)&T_SYMBOL); }
#define symprop(p)       cdr(p)
#endif

INTERFACE INLINE int is_syntax(pointer p)   { return (typeflag(p)&T_SYNTAX); }
INTERFACE INLINE int is_proc(pointer p)     { return (type(p)==T_PROC); }
INTERFACE INLINE int is_foreign(pointer p)  { return (type(p)==T_FOREIGN); }
INTERFACE INLINE char *syntaxname(pointer p) { return strvalue(car(p)); }
#define procnum(p)       ivalue(p)
static const char *procname(pointer x);

INTERFACE INLINE int is_closure(pointer p)  { return (type(p)==T_CLOSURE); }
INTERFACE INLINE int is_macro(pointer p)    { return (type(p)==T_MACRO); }
INTERFACE INLINE pointer closure_code(pointer p)   { return car(p); }
INTERFACE INLINE pointer closure_env(pointer p)    { return cdr(p); }

INTERFACE INLINE int is_continuation(pointer p)    { return (type(p)==T_CONTINUATION); }
#define cont_dump(p)     cdr(p)

/* To do: promise should be forced ONCE only */
INTERFACE INLINE int is_promise(pointer p)  { return (type(p)==T_PROMISE); }

INTERFACE INLINE int is_environment(pointer p) { return (type(p)==T_ENVIRONMENT); }
#define setenvironment(p)    typeflag(p) = T_ENVIRONMENT

#define is_atom(p)       (typeflag(p)&T_ATOM)
#define setatom(p)       typeflag(p) |= T_ATOM
#define clratom(p)       typeflag(p) &= CLRATOM

#define is_mark(p)       (typeflag(p)&MARK)
#define setmark(p)       typeflag(p) |= MARK
#define clrmark(p)       typeflag(p) &= UNMARK

INTERFACE INLINE int is_immutable(pointer p) { return (typeflag(p)&T_IMMUTABLE); }
/*#define setimmutable(p)  typeflag(p) |= T_IMMUTABLE*/
INTERFACE INLINE void setimmutable(pointer p) { typeflag(p) |= T_IMMUTABLE; }

#define caar(p)          car(car(p))
#define cadr(p)          car(cdr(p))
#define cdar(p)          cdr(car(p))
#define cddr(p)          cdr(cdr(p))
#define cadar(p)         car(cdr(car(p)))
#define caddr(p)         car(cdr(cdr(p)))
#define cdaar(p)         cdr(car(car(p)))
#define cadaar(p)        car(cdr(car(car(p))))
#define cadddr(p)        car(cdr(cdr(cdr(p))))
#define cddddr(p)        cdr(cdr(cdr(cdr(p))))

#if USE_CHAR_CLASSIFIERS
static INLINE int Cisalpha(gunichar c) { return g_unichar_isalpha(c); }
static INLINE int Cisdigit(gunichar c) { return g_unichar_isdigit(c); }
static INLINE int Cisspace(gunichar c) { return g_unichar_isspace(c); }
static INLINE int Cisupper(gunichar c) { return g_unichar_isupper(c); }
static INLINE int Cislower(gunichar c) { return g_unichar_islower(c); }
#endif

#if USE_ASCII_NAMES
static const char *charnames[32]={
 "nul",
 "soh",
 "stx",
 "etx",
 "eot",
 "enq",
 "ack",
 "bel",
 "bs",
 "ht",
 "lf",
 "vt",
 "ff",
 "cr",
 "so",
 "si",
 "dle",
 "dc1",
 "dc2",
 "dc3",
 "dc4",
 "nak",
 "syn",
 "etb",
 "can",
 "em",
 "sub",
 "esc",
 "fs",
 "gs",
 "rs",
 "us"
};

static int is_ascii_name(const char *name, int *pc) {
  int i;
  for(i=0; i<32; i++) {
     if(stricmp(name,charnames[i])==0) {
          *pc=i;
          return 1;
     }
  }
  if(stricmp(name,"del")==0) {
     *pc=127;
     return 1;
  }
  return 0;
}

#endif

/* Number of bytes expected AFTER lead byte of UTF-8 character. */
static const char utf8_length[64] =
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0xc0-0xcf */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0xd0-0xdf */
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 0xe0-0xef */
    3,3,3,3,3,3,3,3,4,4,4,4,5,5,0,0  /* 0xf0-0xff */
};

static int file_push(scheme *sc, const char *fname);
static void file_pop(scheme *sc);
static int file_interactive(scheme *sc);
static INLINE int is_one_of(char *s, gunichar c);
static int alloc_cellseg(scheme *sc, int n);
static long binary_decode(const char *s);
static INLINE pointer get_cell(scheme *sc, pointer a, pointer b);
static pointer _get_cell(scheme *sc, pointer a, pointer b);
static pointer reserve_cells(scheme *sc, int n);
static pointer get_consecutive_cells(scheme *sc, int n);
static pointer find_consecutive_cells(scheme *sc, int n);
static void finalize_cell(scheme *sc, pointer a);
static int count_consecutive_cells(pointer x, int needed);
static pointer find_slot_in_env(scheme *sc, pointer env, pointer sym, int all);
static pointer mk_number(scheme *sc, num n);
static char *store_string(scheme *sc, int len, const char *str, gunichar fill);
static pointer mk_vector(scheme *sc, int len);
static pointer mk_atom(scheme *sc, char *q);
static pointer mk_sharp_const(scheme *sc, char *name);
static pointer port_from_filename(scheme *sc, const char *fn, int prop);
static pointer port_from_file(scheme *sc, FILE *, int prop);
static port *port_rep_from_filename(scheme *sc, const char *fn, int prop);
static port *port_rep_from_file(scheme *sc, FILE *, int prop);
static void port_close(scheme *sc, pointer p, int flag);
static void mark(pointer a);
static void gc(scheme *sc, pointer a, pointer b);
static gint basic_inbyte (port *pt);
static gint inbyte (scheme *sc);
static void backbyte (scheme *sc, gint b);
static gunichar inchar(scheme *sc);
static void backchar(scheme *sc, gunichar c);
static char *readstr_upto(scheme *sc, char *delim);
static pointer readstrexp(scheme *sc);
static INLINE int skipspace(scheme *sc);
static int token(scheme *sc);
static void printslashstring(scheme *sc, char *s, int len);
static void atom2str(scheme *sc, pointer l, int f, char **pp, int *plen);
static void printatom(scheme *sc, pointer l, int f);
static pointer mk_proc(scheme *sc, enum scheme_opcodes op);
static pointer mk_closure(scheme *sc, pointer c, pointer e);
static pointer mk_continuation(scheme *sc, pointer d);
static pointer reverse(scheme *sc, pointer a);
static pointer reverse_in_place(scheme *sc, pointer term, pointer list);
static pointer revappend(scheme *sc, pointer a, pointer b);
int list_length(scheme *sc, pointer a);
int eqv(pointer a, pointer b);

static INLINE void dump_stack_mark(scheme *);
static pointer opexe_0(scheme *sc, enum scheme_opcodes op);
static pointer opexe_1(scheme *sc, enum scheme_opcodes op);
static pointer opexe_2(scheme *sc, enum scheme_opcodes op);
static pointer opexe_3(scheme *sc, enum scheme_opcodes op);
static pointer opexe_4(scheme *sc, enum scheme_opcodes op);
static pointer opexe_5(scheme *sc, enum scheme_opcodes op);
static pointer opexe_6(scheme *sc, enum scheme_opcodes op);
static void Eval_Cycle(scheme *sc, enum scheme_opcodes op);
static void assign_syntax(scheme *sc, char *name);
static int syntaxnum(pointer p);
static void assign_proc(scheme *sc, enum scheme_opcodes, char *name);
scheme *scheme_init_new(void);

#define num_ivalue(n)       (n.is_fixnum?(n).value.ivalue:(long)(n).value.rvalue)
#define num_rvalue(n)       (!n.is_fixnum?(n).value.rvalue:(double)(n).value.ivalue)

static num num_add(num a, num b) {
 num ret;
 ret.is_fixnum=a.is_fixnum && b.is_fixnum;
 if(ret.is_fixnum) {
     ret.value.ivalue= a.value.ivalue+b.value.ivalue;
 } else {
     ret.value.rvalue=num_rvalue(a)+num_rvalue(b);
 }
 return ret;
}

static num num_mul(num a, num b) {
 num ret;
 ret.is_fixnum=a.is_fixnum && b.is_fixnum;
 if(ret.is_fixnum) {
     ret.value.ivalue= a.value.ivalue*b.value.ivalue;
 } else {
     ret.value.rvalue=num_rvalue(a)*num_rvalue(b);
 }
 return ret;
}

static num num_div(num a, num b) {
 num ret;
 ret.is_fixnum=a.is_fixnum && b.is_fixnum && a.value.ivalue%b.value.ivalue==0;
 if(ret.is_fixnum) {
     ret.value.ivalue= a.value.ivalue/b.value.ivalue;
 } else {
     ret.value.rvalue=num_rvalue(a)/num_rvalue(b);
 }
 return ret;
}

static num num_intdiv(num a, num b) {
 num ret;
 ret.is_fixnum=a.is_fixnum && b.is_fixnum;
 if(ret.is_fixnum) {
     ret.value.ivalue= a.value.ivalue/b.value.ivalue;
 } else {
     ret.value.rvalue=num_rvalue(a)/num_rvalue(b);
 }
 return ret;
}

static num num_sub(num a, num b) {
 num ret;
 ret.is_fixnum=a.is_fixnum && b.is_fixnum;
 if(ret.is_fixnum) {
     ret.value.ivalue= a.value.ivalue-b.value.ivalue;
 } else {
     ret.value.rvalue=num_rvalue(a)-num_rvalue(b);
 }
 return ret;
}

static num num_rem(num a, num b) {
 num ret;
 long e1, e2, res;
 ret.is_fixnum=a.is_fixnum && b.is_fixnum;
 e1=num_ivalue(a);
 e2=num_ivalue(b);
 res=e1%e2;
 /* remainder should have same sign as first operand */
 if (res > 0) {
     if (e1 < 0) {
        res -= labs(e2);
     }
 } else if (res < 0) {
     if (e1 > 0) {
         res += labs(e2);
     }
 }
 if (ret.is_fixnum) {
     ret.value.ivalue = res;
 } else {
     ret.value.rvalue = res;
 }
 return ret;
}

static num num_mod(num a, num b) {
 num ret;
 long e1, e2, res;
 ret.is_fixnum=a.is_fixnum && b.is_fixnum;
 e1=num_ivalue(a);
 e2=num_ivalue(b);
 res=e1%e2;
 /* modulo should have same sign as second operand */
 if (res * e2 < 0) {
    res += e2;
 }
 if (ret.is_fixnum) {
     ret.value.ivalue = res;
 } else {
     ret.value.rvalue = res;
 }
 return ret;
}

static int num_eq(num a, num b) {
 int ret;
 int is_fixnum=a.is_fixnum && b.is_fixnum;
 if(is_fixnum) {
     ret= a.value.ivalue==b.value.ivalue;
 } else {
     ret=num_rvalue(a)==num_rvalue(b);
 }
 return ret;
}


static int num_gt(num a, num b) {
 int ret;
 int is_fixnum=a.is_fixnum && b.is_fixnum;
 if(is_fixnum) {
     ret= a.value.ivalue>b.value.ivalue;
 } else {
     ret=num_rvalue(a)>num_rvalue(b);
 }
 return ret;
}

static int num_ge(num a, num b) {
 return !num_lt(a,b);
}

static int num_lt(num a, num b) {
 int ret;
 int is_fixnum=a.is_fixnum && b.is_fixnum;
 if(is_fixnum) {
     ret= a.value.ivalue<b.value.ivalue;
 } else {
     ret=num_rvalue(a)<num_rvalue(b);
 }
 return ret;
}

static int num_le(num a, num b) {
 return !num_gt(a,b);
}

#if USE_MATH
/* Round to nearest. Round to even if midway */
static double round_per_R5RS(double x) {
 double fl=floor(x);
 double ce=ceil(x);
 double dfl=x-fl;
 double dce=ce-x;
 if(dfl>dce) {
     return ce;
 } else if(dfl<dce) {
     return fl;
 } else {
     if(fmod(fl,2.0)==0.0) {       /* I imagine this holds */
          return fl;
     } else {
          return ce;
     }
 }
}
#endif

static int is_zero_double(double x) {
 return x<DBL_MIN && x>-DBL_MIN;
}

static long binary_decode(const char *s) {
 long x=0;

 while(*s!=0 && (*s=='1' || *s=='0')) {
     x<<=1;
     x+=*s-'0';
     s++;
 }

 return x;
}

/* allocate new cell segment */
static int alloc_cellseg(scheme *sc, int n) {
     pointer newp;
     pointer last;
     pointer p;
     char *cp;
     long i;
     int k;
     int adj=ADJ;

     if(adj<sizeof(struct cell)) {
       adj=sizeof(struct cell);
     }

     for (k = 0; k < n; k++) {
          if (sc->last_cell_seg >= CELL_NSEGMENT - 1)
               return k;
          cp = (char*) sc->malloc(CELL_SEGSIZE * sizeof(struct cell)+adj);
          if (cp == 0)
               return k;
          i = ++sc->last_cell_seg ;
          sc->alloc_seg[i] = cp;
          /* adjust in TYPE_BITS-bit boundary */
          if(((uintptr_t)cp)%adj!=0) {
            cp=(char*)(adj*((uintptr_t)cp/adj+1));
          }
        /* insert new segment in address order */
          newp=(pointer)cp;
        sc->cell_seg[i] = newp;
        while (i > 0 && sc->cell_seg[i - 1] > sc->cell_seg[i]) {
              p = sc->cell_seg[i];
            sc->cell_seg[i] = sc->cell_seg[i - 1];
            sc->cell_seg[--i] = p;
        }
          sc->fcells += CELL_SEGSIZE;
        last = newp + CELL_SEGSIZE - 1;
          for (p = newp; p <= last; p++) {
               typeflag(p) = 0;
               cdr(p) = p + 1;
               car(p) = sc->NIL;
          }
        /* insert new cells in address order on free list */
        if (sc->free_cell == sc->NIL || p < sc->free_cell) {
             cdr(last) = sc->free_cell;
             sc->free_cell = newp;
        } else {
              p = sc->free_cell;
              while (cdr(p) != sc->NIL && newp > cdr(p))
                   p = cdr(p);
              cdr(last) = cdr(p);
              cdr(p) = newp;
        }
     }
     return n;
}

static INLINE pointer get_cell_x(scheme *sc, pointer a, pointer b) {
  if (sc->free_cell != sc->NIL) {
    pointer x = sc->free_cell;
    sc->free_cell = cdr(x);
    --sc->fcells;
    return (x);
  }
  return _get_cell (sc, a, b);
}


/* get new cell.  parameter a, b is marked by gc. */
static pointer _get_cell(scheme *sc, pointer a, pointer b) {
  pointer x;

  if(sc->no_memory) {
    return sc->sink;
  }

  if (sc->free_cell == sc->NIL) {
    const int min_to_be_recovered = sc->last_cell_seg*8;
    gc(sc,a, b);
    if (sc->fcells < min_to_be_recovered
        || sc->free_cell == sc->NIL) {
      /* if only a few recovered, get more to avoid fruitless gc's */
      if (!alloc_cellseg(sc,1) && sc->free_cell == sc->NIL) {
        g_warning ("%s", G_STRFUNC);
        sc->no_memory=1;
        return sc->sink;
      }
    }
  }
  x = sc->free_cell;
  sc->free_cell = cdr(x);
  --sc->fcells;
  return (x);
}

/* make sure that there is a given number of cells free */
static pointer reserve_cells(scheme *sc, int n) {
       if(sc->no_memory) {
               return sc->NIL;
       }

       /* Are there enough cells available? */
       if (sc->fcells < n) {
               /* If not, try gc'ing some */
               gc(sc, sc->NIL, sc->NIL);
               if (sc->fcells < n) {
                       /* If there still aren't, try getting more heap */
                       if (!alloc_cellseg(sc,1)) {
                               g_warning ("%s", G_STRFUNC);
                               sc->no_memory=1;
                               return sc->NIL;
                       }
               }
               if (sc->fcells < n) {
                       /* If all fail, report failure */
                       g_warning ("%s", G_STRFUNC);
                       sc->no_memory=1;
                       return sc->NIL;
               }
       }
       return (sc->T);
}

static pointer get_consecutive_cells(scheme *sc, int n) {
  pointer x;

  if(sc->no_memory) { return sc->sink; }

  /* Are there any cells available? */
  x=find_consecutive_cells(sc,n);
  if (x != sc->NIL) { return x; }

  /* If not, try gc'ing some */
  gc(sc, sc->NIL, sc->NIL);
  x=find_consecutive_cells(sc,n);
  if (x != sc->NIL) { return x; }

  /* If there still aren't, try getting more heap */
  if (!alloc_cellseg(sc,1))
    {
      g_warning ("%s", G_STRFUNC);
      sc->no_memory=1;
      return sc->sink;
    }

  x=find_consecutive_cells(sc,n);
  if (x != sc->NIL) { return x; }

  /* If all fail, report failure */
  g_warning ("%s", G_STRFUNC);
  sc->no_memory=1;
  return sc->sink;
}

static int count_consecutive_cells(pointer x, int needed) {
 int n=1;
 while(cdr(x)==x+1) {
     x=cdr(x);
     n++;
     if(n>needed) return n;
 }
 return n;
}

static pointer find_consecutive_cells(scheme *sc, int n) {
  pointer *pp;
  int cnt;

  pp=&sc->free_cell;
  while(*pp!=sc->NIL) {
    cnt=count_consecutive_cells(*pp,n);
    if(cnt>=n) {
      pointer x=*pp;
      *pp=cdr(*pp+n-1);
      sc->fcells -= n;
      return x;
    }
    pp=&cdr(*pp+cnt-1);
  }
  return sc->NIL;
}

/* To retain recent allocs before interpreter knows about them -
   Tehom */

static void push_recent_alloc(scheme *sc, pointer recent, pointer extra)
{
  pointer holder = get_cell_x(sc, recent, extra);
  typeflag(holder) = T_PAIR | T_IMMUTABLE;
  car(holder) = recent;
  cdr(holder) = car(sc->sink);
  car(sc->sink) = holder;
}


static pointer get_cell(scheme *sc, pointer a, pointer b)
{
  pointer cell   = get_cell_x(sc, a, b);
  /* For right now, include "a" and "b" in "cell" so that gc doesn't
     think they are garbage. */
  /* Tentatively record it as a pair so gc understands it. */
  typeflag(cell) = T_PAIR;
  car(cell) = a;
  cdr(cell) = b;
  push_recent_alloc(sc, cell, sc->NIL);
  return cell;
}

static pointer get_vector_object(scheme *sc, int len, pointer init)
{
  pointer cells = get_consecutive_cells(sc,len/2+len%2+1);
  if(sc->no_memory) { return sc->sink; }
  /* Record it as a vector so that gc understands it. */
  typeflag(cells) = (T_VECTOR | T_ATOM);
  ivalue_unchecked(cells)=len;
  set_num_integer(cells);
  fill_vector(cells,init);
  push_recent_alloc(sc, cells, sc->NIL);
  return cells;
}

static INLINE void ok_to_freely_gc(scheme *sc)
{
  car(sc->sink) = sc->NIL;
}


#if defined TSGRIND
static void check_cell_alloced(pointer p, int expect_alloced)
{
  /* Can't use putstr(sc,str) because callers have no access to
     sc.  */
  if(typeflag(p) & !expect_alloced)
    {
      fprintf(stderr,"Cell is already allocated!\n");
    }
  if(!(typeflag(p)) & expect_alloced)
    {
      fprintf(stderr,"Cell is not allocated!\n");
    }
}
static void check_range_alloced(pointer p, int n, int expect_alloced)
{
  int i;
  for(i = 0;i<n;i++)
    { (void)check_cell_alloced(p+i,expect_alloced); }
}

#endif

/* Medium level cell allocation */

/* get new cons cell */
pointer _cons(scheme *sc, pointer a, pointer b, int immutable) {
  pointer x = get_cell(sc,a, b);

  typeflag(x) = T_PAIR;
  if(immutable) {
    setimmutable(x);
  }
  car(x) = a;
  cdr(x) = b;
  return (x);
}

/* ========== oblist implementation  ========== */

#ifndef USE_OBJECT_LIST

static int hash_fn(const char *key, int table_size);

static pointer oblist_initial_value(scheme *sc)
{
  return mk_vector(sc, 461); /* probably should be bigger */
}

/* returns the new symbol */
static pointer oblist_add_by_name(scheme *sc, const char *name)
{
  pointer x;
  int location;

  x = immutable_cons(sc, mk_string(sc, name), sc->NIL);
  typeflag(x) = T_SYMBOL;
  setimmutable(car(x));

  location = hash_fn(name, ivalue_unchecked(sc->oblist));
  set_vector_elem(sc->oblist, location,
                  immutable_cons(sc, x, vector_elem(sc->oblist, location)));
  return x;
}

static INLINE pointer oblist_find_by_name(scheme *sc, const char *name)
{
  int location;
  pointer x;
  char *s;

  location = hash_fn(name, ivalue_unchecked(sc->oblist));
  for (x = vector_elem(sc->oblist, location); x != sc->NIL; x = cdr(x)) {
    s = symname(car(x));
    /* case-insensitive, per R5RS section 2. */
    if(stricmp(name, s) == 0) {
      return car(x);
    }
  }
  return sc->NIL;
}

static pointer oblist_all_symbols(scheme *sc)
{
  int i;
  pointer x;
  pointer ob_list = sc->NIL;

  for (i = 0; i < ivalue_unchecked(sc->oblist); i++) {
    for (x  = vector_elem(sc->oblist, i); x != sc->NIL; x = cdr(x)) {
      ob_list = cons(sc, x, ob_list);
    }
  }
  return ob_list;
}

#else

static pointer oblist_initial_value(scheme *sc)
{
  return sc->NIL;
}

static INLINE pointer oblist_find_by_name(scheme *sc, const char *name)
{
     pointer x;
     char    *s;

     for (x = sc->oblist; x != sc->NIL; x = cdr(x)) {
        s = symname(car(x));
        /* case-insensitive, per R5RS section 2. */
        if(stricmp(name, s) == 0) {
          return car(x);
        }
     }
     return sc->NIL;
}

/* returns the new symbol */
static pointer oblist_add_by_name(scheme *sc, const char *name)
{
  pointer x;

  x = immutable_cons(sc, mk_string(sc, name), sc->NIL);
  typeflag(x) = T_SYMBOL;
  setimmutable(car(x));
  sc->oblist = immutable_cons(sc, x, sc->oblist);
  return x;
}

static pointer oblist_all_symbols(scheme *sc)
{
  return sc->oblist;
}

#endif

/* Declare p as void* but require port* */
pointer mk_port(scheme *sc, void *p) {
  pointer x = get_cell(sc, sc->NIL, sc->NIL);

  typeflag(x) = T_PORT|T_ATOM;
  x->_object._port=p;
  return (x);
}

pointer mk_foreign_func(scheme *sc, foreign_func f) {
  pointer x = get_cell(sc, sc->NIL, sc->NIL);

  typeflag(x) = (T_FOREIGN | T_ATOM);
  x->_object._ff=f;
  return (x);
}

INTERFACE pointer
mk_byte (scheme *sc, guint8 b)
{
  pointer x = get_cell (sc, sc->NIL, sc->NIL);

  typeflag (x) = (T_BYTE | T_ATOM);
  ivalue_unchecked (x) = b;
  set_num_integer (x);
  return x;
}

/* Returns atom of type char from 32-bit codepoint.
 * Returns nil when codepoint is not valid UTF-8 encoding.
 */
INTERFACE pointer mk_character(scheme *sc, gunichar c) {
  pointer x;

  /* Reject invalid codepoints in UTF-8. */
  if (!g_unichar_validate (c))
    {
      g_warning ("Failed make character from invalid codepoint.");
      return sc->NIL;
    }

  x = get_cell(sc,sc->NIL, sc->NIL);

  /* g_debug ("%s %i", G_STRFUNC, c); */

  typeflag(x) = (T_CHARACTER | T_ATOM);
  ivalue_unchecked(x)= c;
  set_num_integer(x);
  return (x);
}

/* get number atom (integer) */
INTERFACE pointer mk_integer(scheme *sc, long num) {
  pointer x = get_cell(sc,sc->NIL, sc->NIL);

  typeflag(x) = (T_NUMBER | T_ATOM);
  ivalue_unchecked(x)= num;
  set_num_integer(x);
  return (x);
}

INTERFACE pointer mk_real(scheme *sc, double n) {
  pointer x = get_cell(sc,sc->NIL, sc->NIL);

  typeflag(x) = (T_NUMBER | T_ATOM);
  rvalue_unchecked(x)= n;
  set_num_real(x);
  return (x);
}

static pointer mk_number(scheme *sc, num n) {
 if(n.is_fixnum) {
     return mk_integer(sc,n.value.ivalue);
 } else {
     return mk_real(sc,n.value.rvalue);
 }
}

pointer foreign_error (scheme *sc, const char *s, pointer a) {
  sc->foreign_error = cons (sc, mk_string (sc, s), a);
  return sc->T;
}

/* char_cnt is length of string in chars. */
/* str points to a NUL terminated string. */
/* Only uses fill_char if str is NULL.    */
/* This routine automatically adds 1 byte */
/* to allow space for terminating NUL.    */
static char *store_string(scheme *sc, int char_cnt,
                          const char *str, gunichar fill) {
     int  len;
     int  i;
     gchar utf8[7];
     gchar *q;
     gchar *q2;

     if(str!=0) {
       q2 = g_utf8_offset_to_pointer(str, (long)char_cnt);
       (void)g_utf8_validate(str, -1, (const gchar **)&q);
       if (q <= q2)
          len = q - str;
       else
          len = q2 - str;
       q=(gchar*)sc->malloc(len+1);
     } else {
       len = g_unichar_to_utf8(fill, utf8);
       q=(gchar*)sc->malloc(char_cnt*len+1);
     }

     if(q==0) {
       g_warning ("%s", G_STRFUNC);
       sc->no_memory=1;
       return sc->strbuff;
     }
     if(str!=0) {
       memcpy(q, str, len);
       q[len]=0;
     } else {
       q2 = q;
       for (i = 0; i < char_cnt; ++i)
       {
         memcpy(q2, utf8, len);
         q2 += len;
       }
       *q2=0;
     }
     return (q);
}

/* get new string */
INTERFACE pointer mk_string(scheme *sc, const char *str) {
     return mk_counted_string(sc,str,g_utf8_strlen(str, -1));
}

/* str points to a NUL terminated string. */
/* len is the length of str in characters */
INTERFACE pointer mk_counted_string(scheme *sc, const char *str, int len) {
     pointer x = get_cell(sc, sc->NIL, sc->NIL);

     typeflag(x) = (T_STRING | T_ATOM);
     strvalue(x) = store_string(sc,len,str,0);
     strlength(x) = len;
     return (x);
}

/* len is the length for the empty string in characters */
INTERFACE pointer mk_empty_string(scheme *sc, int len, gunichar fill) {
     pointer x = get_cell(sc, sc->NIL, sc->NIL);

     typeflag(x) = (T_STRING | T_ATOM);
     strvalue(x) = store_string(sc,len,0,fill);
     strlength(x) = len;
     return (x);
}

INTERFACE static pointer mk_vector(scheme *sc, int len)
{ return get_vector_object(sc,len,sc->NIL); }

INTERFACE static void fill_vector(pointer vec, pointer obj) {
     int i;
     int num=ivalue(vec)/2+ivalue(vec)%2;
     for(i=0; i<num; i++) {
          typeflag(vec+1+i) = T_PAIR;
          setimmutable(vec+1+i);
          car(vec+1+i)=obj;
          cdr(vec+1+i)=obj;
     }
}

INTERFACE static pointer vector_elem(pointer vec, int ielem) {
     int n=ielem/2;
     if(ielem%2==0) {
          return car(vec+1+n);
     } else {
          return cdr(vec+1+n);
     }
}

INTERFACE static pointer set_vector_elem(pointer vec, int ielem, pointer a) {
     int n=ielem/2;
     if(ielem%2==0) {
          return car(vec+1+n)=a;
     } else {
          return cdr(vec+1+n)=a;
     }
}

/* get new symbol */
INTERFACE pointer mk_symbol(scheme *sc, const char *name) {
     pointer x;

     /* first check oblist */
     x = oblist_find_by_name(sc, name);
     if (x != sc->NIL) {
          return (x);
     } else {
          x = oblist_add_by_name(sc, name);
          return (x);
     }
}

INTERFACE pointer gensym(scheme *sc) {
     pointer x;
     char name[40];

     for(; sc->gensym_cnt<LONG_MAX; sc->gensym_cnt++) {
          snprintf(name,40,"gensym-%ld",sc->gensym_cnt);

          /* first check oblist */
          x = oblist_find_by_name(sc, name);

          if (x != sc->NIL) {
               continue;
          } else {
               x = oblist_add_by_name(sc, name);
               return (x);
          }
     }

     return sc->NIL;
}

/* make symbol or number atom from string */
static pointer mk_atom(scheme *sc, char *q) {
     char    c, *p;
     int has_dec_point=0;
     int has_fp_exp = 0;

#if USE_COLON_HOOK
     if((p=strstr(q,"::"))!=0) {
          *p=0;
          return cons(sc, sc->COLON_HOOK,
                          cons(sc,
                              cons(sc,
                                   sc->QUOTE,
                                   cons(sc, mk_atom(sc,p+2), sc->NIL)),
                              cons(sc, mk_symbol(sc,strlwr(q)), sc->NIL)));
     }
#endif

     p = q;
     c = *p++;
     if ((c == '+') || (c == '-')) {
       c = *p++;
       if (c == '.') {
         has_dec_point=1;
         c = *p++;
       }
       if (!isdigit(c)) {
         return (mk_symbol(sc, strlwr(q)));
       }
     } else if (c == '.') {
       has_dec_point=1;
       c = *p++;
       if (!isdigit(c)) {
         return (mk_symbol(sc, strlwr(q)));
       }
     } else if (!isdigit(c)) {
       return (mk_symbol(sc, strlwr(q)));
     }

     for ( ; (c = *p) != 0; ++p) {
          if (!isdigit(c)) {
               if(c=='.') {
                    if(!has_dec_point) {
                         has_dec_point=1;
                         continue;
                    }
               }
               else if ((c == 'e') || (c == 'E')) {
                       if(!has_fp_exp) {
                          has_fp_exp = 1;
                          has_dec_point = 1; /* decimal point illegal
                                                from now on */
                          p++;
                          if ((*p == '-') || (*p == '+') || isdigit(*p)) {
                             continue;
                          }
                       }
               }
               return (mk_symbol(sc, strlwr(q)));
          }
     }
     if(has_dec_point) {
       return mk_real(sc,g_ascii_strtod(q,NULL));
     }
     return (mk_integer(sc, atol(q)));
}

/* make atom from sharp expr representing constant.
 * Returns atom of type integer, char, or boolean.
 * Returns nil for certain errors in sharp char expr,
 * including invalid UTF-8 encoding
 * or sharp hex char expr that is invalid codepoint.
 */
static pointer mk_sharp_const(scheme *sc, char *name) {
     long    x;
     char    tmp[STRBUFFSIZE];

     if (!strcmp(name, "t"))
          return (sc->T);
     else if (!strcmp(name, "f"))
          return (sc->F);
     else if (*name == 'o') {/* #o (octal) */
          snprintf(tmp, STRBUFFSIZE, "0%s", name+1);
          sscanf(tmp, "%lo", (long unsigned *)&x);
          return (mk_integer(sc, x));
     } else if (*name == 'd') {    /* #d (decimal) */
          sscanf(name+1, "%ld", (long int *)&x);
          return (mk_integer(sc, x));
     } else if (*name == 'x') {    /* #x (hex) */
          snprintf(tmp, STRBUFFSIZE, "0x%s", name+1);
          sscanf(tmp, "%lx", (long unsigned *)&x);
          return (mk_integer(sc, x));
     } else if (*name == 'b') {    /* #b (binary) */
          x = binary_decode(name+1);
          return (mk_integer(sc, x));
     } else if (*name == '\\') {
          /* #\<foo>  or #\x<foo> (character) */
          gunichar codepoint;
          if(stricmp(name+1,"space")==0) {
               codepoint=' ';
          } else if(stricmp(name+1,"newline")==0) {
               codepoint='\n';
          } else if(stricmp(name+1,"return")==0) {
               codepoint='\r';
          } else if(stricmp(name+1,"tab")==0) {
               codepoint='\t';
          } else if(name[1]=='x' && name[2]!=0) {
            /* Hex literal in ASCII bytes longer than
             * \xffffffff won't fit in codepoint.
             */
            if (strlen (name) > 10) {
              g_warning ("Hex literal larger than 32-bit");
              return sc->NIL;
            }
            else {
              /* #\x<[0-f]*> Convert hex literal to codepoint. */
              if(sscanf(name+2,"%x",(unsigned int *)&codepoint)!=1) {
                g_warning ("Hex literal has invalid digits");
                return sc->NIL;
              }
            }
#if USE_ASCII_NAMES
          } else if(is_ascii_name(name+1,&codepoint)) {
               /* nothing, side effect is fill codepoint.  */
#endif
          } else {
               /* #\<UTF-8 encoding>
                * i.e. one to four bytes.
                */
               codepoint = g_utf8_get_char_validated (&name[1], -1);
               if (codepoint == -1) {
                  g_warning ("Invalid UTF-8 encoding");
                  return sc->NIL;
               }
          }
          /* Not assert is valid codepoint yet.
           * mk_character does not require but will return NIL
           * when not valid.
           */
          return mk_character(sc,codepoint);
     } else
          return (sc->NIL);
}

/* ========== garbage collector ========== */

/*--
 *  We use algorithm E (Knuth, The Art of Computer Programming Vol.1,
 *  sec. 2.3.5), the Schorr-Deutsch-Waite link-inversion algorithm,
 *  for marking.
 */
static void mark(pointer a) {
     pointer t, q, p;

     t = (pointer) 0;
     p = a;
E2:  setmark(p);
     if(is_vector(p)) {
          int i;
          int num=ivalue_unchecked(p)/2+ivalue_unchecked(p)%2;
          for(i=0; i<num; i++) {
               /* Vector cells will be treated like ordinary cells */
               mark(p+1+i);
          }
     }
     if (is_atom(p))
          goto E6;
     /* E4: down car */
     q = car(p);
     if (q && !is_mark(q)) {
          setatom(p);  /* a note that we have moved car */
          car(p) = t;
          t = p;
          p = q;
          goto E2;
     }
 E5:  q = cdr(p); /* down cdr */
     if (q && !is_mark(q)) {
          cdr(p) = t;
          t = p;
          p = q;
          goto E2;
     }
E6:   /* up.  Undo the link switching from steps E4 and E5. */
     if (!t)
          return;
     q = t;
     if (is_atom(q)) {
          clratom(q);
          t = car(q);
          car(q) = p;
          p = q;
          goto E5;
     } else {
          t = cdr(q);
          cdr(q) = p;
          p = q;
          goto E6;
     }
}

/* garbage collection. parameter a, b is marked. */
static void gc(scheme *sc, pointer a, pointer b) {
  pointer p;
  int i;

  if(sc->gc_verbose) {
    putstr(sc, "gc...");
  }

  /* mark system globals */
  mark(sc->oblist);
  mark(sc->global_env);

  /* mark current registers */
  mark(sc->args);
  mark(sc->envir);
  mark(sc->code);
  dump_stack_mark(sc);
  mark(sc->value);
  mark(sc->inport);
  mark(sc->save_inport);
  mark(sc->outport);
  mark(sc->loadport);

  /* Mark recent objects the interpreter doesn't know about yet. */
  mark(car(sc->sink));
  /* Mark any older stuff above nested C calls */
  mark(sc->c_nest);

  /* mark variables a, b */
  mark(a);
  mark(b);

  /* garbage collect */
  clrmark(sc->NIL);
  sc->fcells = 0;
  sc->free_cell = sc->NIL;
  /* free-list is kept sorted by address so as to maintain consecutive
     ranges, if possible, for use with vectors. Here we scan the cells
     (which are also kept sorted by address) downwards to build the
     free-list in sorted order.
  */
  for (i = sc->last_cell_seg; i >= 0; i--) {
    p = sc->cell_seg[i] + CELL_SEGSIZE;
    while (--p >= sc->cell_seg[i]) {
      if (is_mark(p)) {
        clrmark(p);
      } else {
        /* reclaim cell */
        if (typeflag(p) != 0) {
          finalize_cell(sc, p);
          typeflag(p) = 0;
          car(p) = sc->NIL;
        }
        ++sc->fcells;
        cdr(p) = sc->free_cell;
        sc->free_cell = p;
      }
    }
  }

  if (sc->gc_verbose) {
    char msg[80];
    snprintf(msg,80,"done: %ld cells were recovered.\n", sc->fcells);
    putstr(sc,msg);
  }
}

static void finalize_cell(scheme *sc, pointer a) {
  if (is_string(a))
    {
      sc->free(strvalue(a));
    }
  else if(is_port(a))
    {
      /* Scheme does not require a script to close a port.
       * Close the file on the system and/or free memory resources.
       */
      if(a->_object._port->kind & port_file)
        {
          if (a->_object._port->rep.stdio.closeit)
            /* Safe to call port_close when already closed. */
            port_close(sc,a,port_input|port_output);
          sc->free(a->_object._port);
        }
      else if (a->_object._port->kind & port_string)
        {
          string_port_dispose (sc, a);
        }
      else
        {
          g_warning ("%s Unknown port kind.", G_STRFUNC);
        }
    }
    /* Else object has no allocation. */
}

/* ========== Routines for Reading ========== */

static int file_push(scheme *sc, const char *fname) {
 FILE *fin = NULL;
 if (sc->file_i == MAXFIL-1)
    return 0;

  fin=g_fopen(fname,"rb");
  if(fin!=0) {
    sc->file_i++;
    sc->load_stack[sc->file_i].kind=port_file|port_input;
    sc->load_stack[sc->file_i].rep.stdio.file=fin;
    sc->load_stack[sc->file_i].rep.stdio.closeit=1;
    sc->nesting_stack[sc->file_i]=0;
    sc->loadport->_object._port=sc->load_stack+sc->file_i;

#if SHOW_ERROR_LINE
    sc->load_stack[sc->file_i].rep.stdio.curr_line = 0;
    if(fname)
      sc->load_stack[sc->file_i].rep.stdio.filename = store_string(sc, strlen(fname), fname, 0);
#endif
  }
  return fin!=0;
}

static void file_pop(scheme *sc) {
 if(sc->file_i != 0) {
     sc->nesting=sc->nesting_stack[sc->file_i];
     port_close(sc,sc->loadport,port_input);
     /* Pop load stack, discarding port soon to be gc. */
     sc->file_i--;
     /* Top of stack into current load port. */
     sc->loadport->_object._port = sc->load_stack + sc->file_i;
   }
}

static int file_interactive(scheme *sc) {
 return sc->file_i==0 && sc->load_stack[0].rep.stdio.file==stdin
     && sc->inport->_object._port->kind&port_file;
}

static port *port_rep_from_filename(scheme *sc, const char *fn, int prop) {
  FILE *f;
  char *rw;
  port *pt;
  if(prop==(port_input|port_output)) {
    rw="a+b";
  } else if(prop==port_output) {
    rw="wb";
  } else {
    rw="rb";
  }
  f=g_fopen(fn,rw);
  if(f==0) {
    return 0;
  }
  pt=port_rep_from_file(sc,f,prop);
  pt->rep.stdio.closeit=1;

#if SHOW_ERROR_LINE
  if(fn)
    pt->rep.stdio.filename = store_string(sc, strlen(fn), fn, 0);

  pt->rep.stdio.curr_line = 0;
#endif
  return pt;
}

static pointer port_from_filename(scheme *sc, const char *fn, int prop) {
  port *pt;
  pt=port_rep_from_filename(sc,fn,prop);
  if(pt==0) {
    return sc->NIL;
  }
  return mk_port(sc,pt);
}

static port *port_rep_from_file(scheme *sc, FILE *f, int prop)
{
    port *pt;

    pt = (port *)sc->malloc(sizeof *pt);
    if (pt == NULL) {
        return NULL;
    }
    pt->kind = port_file | prop;
    pt->rep.stdio.file = f;
    pt->rep.stdio.closeit = 0;
    return pt;
}

static pointer port_from_file(scheme *sc, FILE *f, int prop) {
  port *pt;
  pt=port_rep_from_file(sc,f,prop);
  if(pt==0) {
    return sc->NIL;
  }
  return mk_port(sc,pt);
}


/* Close one or more directions of the port.
 *
 * When there are no directions remaining, the port becomes fully closed.
 *
 * When the port is already fully closed, this does nothing.
 *
 * When a port becomes fully closed, release OS resources (close the file),
 * for a file-port. A string-port has no system resources.
 *
 * The port remains an object to be gc'd later
 * but a script cannot call some port methods on the port.
 */
static void port_close(scheme *sc, pointer p, int directions) {
  port *pt=p->_object._port;

  /* Fully closed already? */
  if((pt->kind & (port_input|port_output))==0)
    return;

  /* Clear directions that are closing. */
  pt->kind &= ~directions;

  /* Fully closed? */
  if((pt->kind & (port_input|port_output))==0) {
    if(pt->kind&port_file) {

#if SHOW_ERROR_LINE
      /* Cleanup is here so (close-*-port) functions could work too */
      pt->rep.stdio.curr_line = 0;

      if(pt->rep.stdio.filename)
        sc->free(pt->rep.stdio.filename);
#endif

      fclose(pt->rep.stdio.file);
    }
    /* Closing does not lose the kind of port nor the saw_EOF flag.
     * The port struct still has attributes, until it is destroyed.
     */
  }
}

/* Read utf8 character from the active port. */
static gunichar
inchar (scheme *sc)
{
  gint b = inbyte (sc);

  while (TRUE)
    {
      if (b == EOF)
        return EOF;

      if (b <= 0x7F)
        return b;

      /* Is this byte an invalid lead per RFC-3629? */
      if (b < 0xC2 || b > 0xF4)
        {
          /* Ignore invalid lead byte and get the next character */
          b = inbyte (sc);
        }
      else
        {
          /* Byte is valid lead */
          guint8 utf8[7];
          gint   len;
          gint   i;
          /* Save the lead byte */
          utf8[0] = b;
          len     = utf8_length[b & 0x3F];
          for (i = 1; i <= len; i++)
            {
              b = inbyte (sc);
              /* Stop reading if this is not a continuation character */
              if ((b & 0xC0) != 0x80)
                break;
              utf8[i] = b;
            }
          /* Read the expected number of bytes? */
          if (i > len)
            {
              return g_utf8_get_char_validated ((char *) utf8,
                                                sizeof (utf8));
            }
          /* Not enough continuation characters so ignore and restart */
        }
    } /* end of while (TRUE) */
}

/* Read a single unsigned byte from the active port. */
static gint
basic_inbyte (port *pt)
{
  if (pt->kind & port_file)
    return fgetc (pt->rep.stdio.file);
  else
    return string_port_inbyte (pt);
}

/* Read a single unsigned byte from the active port. */
static gint
inbyte (scheme *sc)
{
  gint  result;
  port *pt;

  pt = sc->inport->_object._port;
  if (pt->kind & port_saw_EOF)
    return EOF;

  result = basic_inbyte (pt);
  if (result == EOF && sc->inport == sc->loadport)
    pt->kind |= port_saw_EOF;

  return result;
}

/* Write utf8 character back to the active port. */
static void
backchar (scheme *sc, gunichar c)
{
  gchar utf8buffer[6];
  gint  byte_count;
  gint  i;

  if (c == EOF)
    return;
  byte_count = g_unichar_to_utf8 (c, utf8buffer);
  /* Write bytes of the utf8 character back in reverse. */
  for (i = byte_count - 1; i > -1; i--)
    {
      backbyte (sc, utf8buffer[i]);
    }
}

/* Write byte back to the active port. */
static void
backbyte (scheme *sc, gint b)
{
  port *pt;
  if (b == EOF)
    return;
  pt = sc->inport->_object._port;
  if (pt->kind & port_file)
    ungetc (b, pt->rep.stdio.file);
  else
    string_port_backbyte (pt);
}


static void
putbytes (scheme *sc, const char *bytes, int byte_count)
{
  port *pt;

  if (error_port_is_redirect_output (sc))
    pt = error_port_get_port_rep (sc);
  else
    pt = sc->outport->_object._port;

  if(pt->kind&port_file) {
#if STANDALONE
      fwrite (bytes, 1, byte_count, pt->rep.stdio.file);
      fflush(pt->rep.stdio.file);
#else
      /* If output is still directed to stdout (the default) try
       * redirect it to any registered output routine.
       * Currently, we require outer wrapper to set_output_func.
       */
      if (pt->rep.stdio.file == stdout)
        {
          if (ts_is_output_redirected ())
            ts_output_string (TS_OUTPUT_NORMAL, bytes, byte_count);
          else
            g_warning ("%s Output disappears since outer wrapper did not redirect.", G_STRFUNC);
        }
      else
        {
          /* Otherwise, the script has set the output port, write to it. */
          fwrite (bytes, 1, byte_count, pt->rep.stdio.file);
          fflush (pt->rep.stdio.file);
        }
#endif
  }
  else if (pt->kind & port_string)
    {
      string_port_put_bytes (sc, pt, bytes, byte_count);
    }
  else
    {
      g_warning ("%s closed or unknown port kind", G_STRFUNC);
    }
}

static void
putchars (scheme *sc, const char *chars, int char_count)
{
  gint byte_count;

  if (char_count <= 0)
    return;

  byte_count = g_utf8_offset_to_pointer (chars, char_count) - chars;
  putbytes (sc, chars, byte_count);
}

INTERFACE void putcharacter(scheme *sc, gunichar c) {
  char utf8[7];

  (void)g_unichar_to_utf8(c, utf8);
  putchars(sc, utf8, 1);
}

INTERFACE void putstr(scheme *sc, const char *s) {
  putchars(sc, s, g_utf8_strlen(s, -1));
}

/* read characters up to delimiter, but cater to character constants */
static char *readstr_upto(scheme *sc, char *delim) {
  char *p = sc->strbuff;
  gunichar c = 0;
  gunichar c_prev = 0;
  int len = 0;

  do {
    c_prev = c;
    c = inchar(sc);
    len = g_unichar_to_utf8(c, p);
    p += len;
  } while ((p - sc->strbuff < sizeof(sc->strbuff)) &&
           (c && !is_one_of(delim, c)));

  if(p == sc->strbuff+2 && c_prev == '\\')
    *p = '\0';
  else
  {
    backchar(sc,c);    /* put back the delimiter */
    p[-len] = '\0';
  }
  return sc->strbuff;
}

/* read string expression "xxx...xxx" */
static pointer readstrexp(scheme *sc) {
  char *p = sc->strbuff;
  gunichar c;
  int c1=0;
  int len;
  enum { st_ok, st_bsl, st_x1, st_x2, st_oct1, st_oct2 } state=st_ok;

  for (;;) {
    c=inchar(sc);
    if(c == EOF || p-sc->strbuff > sizeof(sc->strbuff)-1) {
      return sc->F;
    }
    switch(state) {
    case st_ok:
      switch(c) {
      case '\\':
        state=st_bsl;
        break;
      case '"':
        *p=0;
        return mk_counted_string(sc,sc->strbuff,
                                 g_utf8_strlen(sc->strbuff, sizeof(sc->strbuff)));
      default:
        len = g_unichar_to_utf8(c, p);
        p += len;
        break;
      }
      break;
    case st_bsl:
      switch(c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
        state=st_oct1;
        c1=g_unichar_digit_value(c);
        break;
      case 'x':
      case 'X':
        state=st_x1;
        c1=0;
        break;
      case 'n':
        *p++='\n';
        state=st_ok;
        break;
      case 't':
        *p++='\t';
        state=st_ok;
        break;
      case 'r':
        *p++='\r';
        state=st_ok;
        break;
      case '"':
        *p++='"';
        state=st_ok;
        break;
      default:
        len = g_unichar_to_utf8(c, p);
        p += len;
        state=st_ok;
        break;
      }
      break;
    case st_x1:
    case st_x2:
      if (!g_unichar_isxdigit(c))
         return sc->F;
      c1=(c1<<4)+g_unichar_xdigit_value(c);
      if(state==st_x1)
        state=st_x2;
      else {
        *p++=c1;
        state=st_ok;
      }
      break;
    case st_oct1:   /* State when handling second octal digit */
    case st_oct2:   /* State when handling third octal digit */
      if (!g_unichar_isdigit(c) || g_unichar_digit_value(c) > 7)
      {
        *p++=c1;
        backchar(sc, c);
        state=st_ok;
      }
      else
      {
        /* Is value of three character octal too big for a byte? */
        if (state==st_oct2 && c1 >= 32)
          return sc->F;

        c1=(c1<<3)+g_unichar_digit_value(c);

        if (state == st_oct1)
          state=st_oct2;
        else
        {
          *p++=c1;
          state=st_ok;
        }
      }
      break;
    }
  }
}

/* check c is in chars */
static INLINE int is_one_of(char *s, gunichar c) {
  if (c==EOF)
     return 1;

  if (g_utf8_strchr(s, -1, c) != NULL)
     return (1);

  return (0);
}

/* skip white characters */
static INLINE int skipspace(scheme *sc) {
     gunichar c;
#if SHOW_ERROR_LINE
     int curr_line = 0;
#endif
     do {
         c=inchar(sc);
#if SHOW_ERROR_LINE
         if(c=='\n')
           curr_line++;
#endif
     } while (g_unichar_isspace(c));

/* record it */
#if SHOW_ERROR_LINE
     if (sc->load_stack[sc->file_i].kind & port_file)
       sc->load_stack[sc->file_i].rep.stdio.curr_line += curr_line;
#endif

     if(c!=EOF) {
          backchar(sc,c);
          return 1;
     }
     else
       { return EOF; }
}

/* get token */
static int token(scheme *sc) {
     gunichar c;
     c = skipspace(sc);
     if(c == EOF) { return (TOK_EOF); }
     switch (c=inchar(sc)) {
     case EOF:
          return (TOK_EOF);
     case '(':
          return (TOK_LPAREN);
     case ')':
          return (TOK_RPAREN);
     case '.':
          c=inchar(sc);
          if(is_one_of(" \n\t",c)) {
               return (TOK_DOT);
          } else {
               backchar(sc,c);
               backchar(sc,'.');
               return TOK_ATOM;
          }
     case '\'':
          return (TOK_QUOTE);
     case ';':
          while ((c=inchar(sc)) != '\n' && c!=EOF)
            ;

#if SHOW_ERROR_LINE
           if(c == '\n' && sc->load_stack[sc->file_i].kind & port_file)
             sc->load_stack[sc->file_i].rep.stdio.curr_line++;
#endif

       if(c == EOF)
         { return (TOK_EOF); }
       else
         { return (token(sc));}
     case '"':
          return (TOK_DQUOTE);
     case '_':
          if ((c=inchar(sc)) == '"')
               return (TOK_USCORE);
          backchar(sc,c);
          return (TOK_ATOM);
     case BACKQUOTE:
          return (TOK_BQUOTE);
     case ',':
          if ((c=inchar(sc)) == '@') {
               return (TOK_ATMARK);
          } else {
               backchar(sc,c);
               return (TOK_COMMA);
          }
     case '#':
          c=inchar(sc);
          if (c == '(') {
               return (TOK_VEC);
          } else if(c == '!') {
               while ((c=inchar(sc)) != '\n' && c!=EOF)
                   ;

#if SHOW_ERROR_LINE
           if(c == '\n' && sc->load_stack[sc->file_i].kind & port_file)
             sc->load_stack[sc->file_i].rep.stdio.curr_line++;
#endif

           if(c == EOF)
             { return (TOK_EOF); }
           else
             { return (token(sc));}
          } else {
               backchar(sc,c);
               if(is_one_of(" tfodxb\\",c)) {
                    return TOK_SHARP_CONST;
               } else {
                    return (TOK_SHARP);
               }
          }
     default:
          backchar(sc,c);
          return (TOK_ATOM);
     }
}

/* ========== Routines for Printing ========== */
#define   ok_abbrev(x)   (is_pair(x) && cdr(x) == sc->NIL)

static void printslashstring(scheme *sc, char *p, int len) {
  int i;
  gunichar c;
  char *s=(char*)p;

  putcharacter(sc,'"');
  for (i=0; i<len; i++) {
    c = g_utf8_get_char(s);
    /* Is a check for a value of 0xff still valid in UTF8?? ~~~~~ */
    if(c==0xff || c=='"' || c<' ' || c=='\\') {
      putcharacter(sc,'\\');
      switch(c) {
      case '"':
        putcharacter(sc,'"');
        break;
      case '\n':
        putcharacter(sc,'n');
        break;
      case '\t':
        putcharacter(sc,'t');
        break;
      case '\r':
        putcharacter(sc,'r');
        break;
      case '\\':
        putcharacter(sc,'\\');
        break;
      default: {
          /* This still needs work ~~~~~ */
          int d=c/16;
          putcharacter(sc,'x');
          if(d<10) {
            putcharacter(sc,d+'0');
          } else {
            putcharacter(sc,d-10+'A');
          }
          d=c%16;
          if(d<10) {
            putcharacter(sc,d+'0');
          } else {
            putcharacter(sc,d-10+'A');
          }
        }
      }
    } else {
      putcharacter(sc,c);
    }
    s = g_utf8_next_char(s);
  }
  putcharacter(sc,'"');
}


/* print atoms */
static void printatom(scheme *sc, pointer l, int f) {
  char *p;
  int len;
  atom2str(sc,l,f,&p,&len);
  putbytes (sc, p, len);
}

/* Encode 32-bit codepoint into null-terminated byte sequence in buffer.
 * Require buffer >6 bytes, per GTK docs.
 * (Not four since the algorithm temporarily uses more.)
 *
 * !!! Does not ensure the sequence is valid,
 * just that the encoding algorithm was followed.
 *
 * Ensures buffer is null-terminated.
 *
 * This encapsulates call to g_unichar_to_utf8
 * to ensure null termination.
 */
static void
get_utf8_from_codepoint (gunichar codepoint, gchar *buffer)
{
  guint len;

  len = g_unichar_to_utf8 (codepoint, buffer);
  /* assert len is ALWAYS [1,6]. */

  /* Despite what GTK docs say, g_unichar_to_utf8 does NOT null-terminate. */
  buffer[len] = 0;
}

/* Return a string representing an atom of type char
 * whose value i.e. codepoint is given.
 * String is a sharp char expression.
 *
 * Returned string is static.
 *
 * Translates whitespace to sharp expression like #\tab.
 *
 * Translate all other non-whitespace codepoints to sharp char expr
 * of form #\* where * is a utf8 encoding (a glyph)
 * The reverse of mk_sharp_constant.
 *
 * Note TinyScheme differs when USE_ASCII_NAMES,
 * e.g. returns #\del .
 *
 * ScriptFu returns sharp char expr with suffix
 * one glyph in utf8 for all other non-whitespace codepoints
 * (even c<32 i.e. control chars and c=127).
 * I.E. a sharp constant #\* where * is a unichar.
 * The unichar may have a box-w-hex glyph.
 *
 * Also does not return sharp char expr hex
 * of form #\x* where * is a hex literal.
 * ScriptFu may return that format elsewhere,
 * for other atom types for certain <base> codes.
 */
static gchar *
get_sharp_char_expr (gunichar codepoint)
{
  gchar *result;

  /* g_debug ("%s %i", G_STRFUNC, codepoint); */

  /* codepoint is unsigned, insure char constants do not sign extend. */
  switch (codepoint)
    {
    case ' ':
      result = "#\\space";
      break;
    case '\n':
      result = "#\\newline";
      break;
    case '\r':
      result = "#\\return";
      break;
    case '\t':
      result = "#\\tab";
      break;
    default:
      {
        /* g_unichar_to_utf8 requires >6 byte buffer. */
        gchar        utf8_bytes[7];
        /* Plus two bytes for "#\" */
        static gchar sharp_constant[9];

        get_utf8_from_codepoint (codepoint, utf8_bytes);

        /* Cat null terminated sequence of bytes (UTF-8 encoding) to "#\" */
        snprintf (sharp_constant, 9, "#\\%s", utf8_bytes);
        result = sharp_constant;
      }
    } /* end switch codepoint */
  /* g_debug ("%s result %s", G_STRFUNC, result); */
  return result;
}

/* Uses internal buffer unless string pointer is already available */
static void atom2str(scheme *sc, pointer l, int f, char **pp, int *plen) {
     char *p;

     if (l == sc->NIL) {
          p = "()";
     } else if (l == sc->T) {
          p = "#t";
     } else if (l == sc->F) {
          p = "#f";
     } else if (l == sc->EOF_OBJ) {
          p = "#<EOF>";
     } else if (is_port(l)) {
          p = "#<PORT>";
     } else if (is_number(l)) {
          p = sc->strbuff;
          if (f <= 1 || f == 10) /* f is the base for numbers if > 1 */ {
              if(num_is_integer(l)) {
                   snprintf(p, STRBUFFSIZE, "%ld", ivalue_unchecked(l));
              } else {
                   snprintf(p, STRBUFFSIZE, "%.10g", rvalue_unchecked(l));
                   /* r5rs says there must be a '.' (unless 'e'?) */
                   f = strcspn(p, ".e");
                   if (p[f] == 0) {
                        p[f] = '.'; /* not found, so add '.0' at the end */
                        p[f+1] = '0';
                        p[f+2] = 0;
                   }
              }
          } else {
              long v = ivalue(l);
              if (f == 16) {
                  if (v >= 0)
                    snprintf(p, STRBUFFSIZE, "%lx", v);
                  else
                    snprintf(p, STRBUFFSIZE, "-%lx", -v);
              } else if (f == 8) {
                  if (v >= 0)
                    snprintf(p, STRBUFFSIZE, "%lo", v);
                  else
                    snprintf(p, STRBUFFSIZE, "-%lo", -v);
              } else if (f == 2) {
                  unsigned long b = (v < 0) ? -v : v;
                  p = &p[STRBUFFSIZE-1];
                  *p = 0;
                  do { *--p = (b&1) ? '1' : '0'; b >>= 1; } while (b != 0);
                  if (v < 0) *--p = '-';
              }
          }
     } else if (is_string(l)) {
          if (!f) {
               p = strvalue(l);
          } else { /* Hack, uses the fact that printing is needed */
               *pp=sc->strbuff;
               *plen=0;
               printslashstring(sc, strvalue(l),
                                g_utf8_strlen(strvalue(l), -1));
               return;
          }
       }
     else if (is_byte (l))
       {
         guint8 b = bytevalue (l);
         p        = sc->strbuff;
         if (! f)
           {
             p[0]  = b;
             p[1]  = 0;
             *pp   = p;
             *plen = 1;
             return;
           }
         else
           {
             snprintf (p, STRBUFFSIZE, "%lu", ivalue_unchecked (l));
           }
     } else if (is_character(l)) {
          gunichar c=charvalue(l);
          if (!f) {
               /* Return just the utf8 encoding, the glyphs themselves.
                * versus a sharp char expr.
                */
               /* Use strbuff, which is >6 bytes */
               p = sc->strbuff;
               get_utf8_from_codepoint (c, p);
          } else {
               p = get_sharp_char_expr (c);
          } /* end if format code. */
     } else if (is_symbol(l)) {
          p = symname(l);
     } else if (is_proc(l)) {
          p = sc->strbuff;
          snprintf(p,STRBUFFSIZE,"#<%s PROCEDURE %ld>",
                   procname(l),procnum(l));
     } else if (is_macro(l)) {
          p = "#<MACRO>";
     } else if (is_closure(l)) {
          p = "#<CLOSURE>";
     } else if (is_promise(l)) {
          p = "#<PROMISE>";
     } else if (is_foreign(l)) {
          p = sc->strbuff;
          snprintf(p,STRBUFFSIZE,"#<FOREIGN PROCEDURE %ld>", procnum(l));
     } else if (is_continuation(l)) {
          p = "#<CONTINUATION>";
     } else {
          p = "#<ERROR>";
     }
     *pp=p;
     *plen = strlen (p);
}
/* ========== Routines for Evaluation Cycle ========== */

/* make closure. c is code. e is environment */
static pointer mk_closure(scheme *sc, pointer c, pointer e) {
     pointer x = get_cell(sc, c, e);

     typeflag(x) = T_CLOSURE;
     car(x) = c;
     cdr(x) = e;
     return (x);
}

/* make continuation. */
static pointer mk_continuation(scheme *sc, pointer d) {
     pointer x = get_cell(sc, sc->NIL, d);

     typeflag(x) = T_CONTINUATION;
     cont_dump(x) = d;
     return (x);
}

static pointer list_star(scheme *sc, pointer d) {
  pointer p, q;
  if(cdr(d)==sc->NIL) {
    return car(d);
  }
  p=cons(sc,car(d),cdr(d));
  q=p;
  while(cdr(cdr(p))!=sc->NIL) {
    d=cons(sc,car(p),cdr(p));
    if(cdr(cdr(p))!=sc->NIL) {
      p=cdr(d);
    }
  }
  cdr(p)=car(cdr(p));
  return q;
}

/* reverse list -- produce new list */
static pointer reverse(scheme *sc, pointer a) {
/* a must be checked by gc */
     pointer p = sc->NIL;

     for ( ; is_pair(a); a = cdr(a)) {
          p = cons(sc, car(a), p);
     }
     return (p);
}

/* reverse list --- in-place */
static pointer reverse_in_place(scheme *sc, pointer term, pointer list) {
     pointer p = list, result = term, q;

     while (p != sc->NIL) {
          q = cdr(p);
          cdr(p) = result;
          result = p;
          p = q;
     }
     return (result);
}

/* append list -- produce new list */
static pointer revappend(scheme *sc, pointer a, pointer b) {
    pointer result = a;
    pointer p = b;

    while (is_pair(p)) {
        result = cons(sc, car(p), result);
        p = cdr(p);
    }

    if (p == sc->NIL) {
        return result;
    }

    return sc->F;   /* signal an error */
}

/* equivalence of atoms */
int eqv(pointer a, pointer b) {
     if (is_string(a)) {
          if (is_string(b))
               return (strvalue(a) == strvalue(b));
          else
               return (0);
     } else if (is_number(a)) {
          if (is_number(b)) {
               if (num_is_integer(a) == num_is_integer(b))
                    return num_eq(nvalue(a),nvalue(b));
          }
          return (0);
       }
     else if (is_byte (a))
       {
         if (is_byte (b))
           return bytevalue (a) == bytevalue (b);
         else
           return 0;
     } else if (is_character(a)) {
          if (is_character(b))
               return charvalue(a)==charvalue(b);
          else
               return (0);
     } else if (is_port(a)) {
          if (is_port(b))
               return a==b;
          else
               return (0);
     } else if (is_proc(a)) {
          if (is_proc(b))
               return procnum(a)==procnum(b);
          else
               return (0);
     } else {
          return (a == b);
     }
}

/* true or false value macro */
/* () is #t in R5RS */
#define is_true(p)       ((p) != sc->F)
#define is_false(p)      ((p) == sc->F)

/* ========== Environment implementation  ========== */

#if !defined(USE_ALIST_ENV) || !defined(USE_OBJECT_LIST)

static int hash_fn(const char *key, int table_size)
{
  unsigned int hashed = 0;
  const char *c;
  int bits_per_int = sizeof(unsigned int)*8;

  for (c = key; *c; c++) {
    /* letters have about 5 bits in them */
    hashed = (hashed<<5) | (hashed>>(bits_per_int-5));
    hashed ^= *c;
  }
  return hashed % table_size;
}
#endif

#ifndef USE_ALIST_ENV

/*
 * In this implementation, each frame of the environment may be
 * a hash table: a vector of alists hashed by variable name.
 * In practice, we use a vector only for the initial frame;
 * subsequent frames are too small and transient for the lookup
 * speed to out-weigh the cost of making a new vector.
 */

static void new_frame_in_env(scheme *sc, pointer old_env)
{
  pointer new_frame;

  /* The interaction-environment has about 300 variables in it. */
  if (old_env == sc->NIL) {
    new_frame = mk_vector(sc, 461);
  } else {
    new_frame = sc->NIL;
  }

  sc->envir = immutable_cons(sc, new_frame, old_env);
  setenvironment(sc->envir);
}

static INLINE void new_slot_spec_in_env(scheme *sc, pointer env,
                                        pointer variable, pointer value)
{
  pointer slot = immutable_cons(sc, variable, value);

  if (is_vector(car(env))) {
    int location = hash_fn(symname(variable), ivalue_unchecked(car(env)));

    set_vector_elem(car(env), location,
                    immutable_cons(sc, slot, vector_elem(car(env), location)));
  } else {
    car(env) = immutable_cons(sc, slot, car(env));
  }
}

static pointer find_slot_in_env(scheme *sc, pointer env, pointer hdl, int all)
{
  pointer x,y;
  int location;

  for (x = env; x != sc->NIL; x = cdr(x)) {
    if (is_vector(car(x))) {
      location = hash_fn(symname(hdl), ivalue_unchecked(car(x)));
      y = vector_elem(car(x), location);
    } else {
      y = car(x);
    }
    for ( ; y != sc->NIL; y = cdr(y)) {
              if (caar(y) == hdl) {
                   break;
              }
         }
         if (y != sc->NIL) {
              break;
         }
         if(!all) {
           return sc->NIL;
         }
    }
    if (x != sc->NIL) {
          return car(y);
    }
    return sc->NIL;
}

#else /* USE_ALIST_ENV */

static INLINE void new_frame_in_env(scheme *sc, pointer old_env)
{
  sc->envir = immutable_cons(sc, sc->NIL, old_env);
  setenvironment(sc->envir);
}

static INLINE void new_slot_spec_in_env(scheme *sc, pointer env,
                                        pointer variable, pointer value)
{
  car(env) = immutable_cons(sc, immutable_cons(sc, variable, value), car(env));
}

static pointer find_slot_in_env(scheme *sc, pointer env, pointer hdl, int all)
{
    pointer x,y;
    for (x = env; x != sc->NIL; x = cdr(x)) {
         for (y = car(x); y != sc->NIL; y = cdr(y)) {
              if (caar(y) == hdl) {
                   break;
              }
         }
         if (y != sc->NIL) {
              break;
         }
         if(!all) {
           return sc->NIL;
         }
    }
    if (x != sc->NIL) {
          return car(y);
    }
    return sc->NIL;
}

#endif /* USE_ALIST_ENV else */

static INLINE void new_slot_in_env(scheme *sc, pointer variable, pointer value)
{
  new_slot_spec_in_env(sc, sc->envir, variable, value);
}

static INLINE void set_slot_in_env(scheme *sc, pointer slot, pointer value)
{
  cdr(slot) = value;
}

static INLINE pointer slot_value_in_env(pointer slot)
{
  return cdr(slot);
}

/* ========== Evaluation Cycle ========== */


static pointer _Error_1(scheme *sc, const char *s, pointer a) {
     const char *str = s;
#if USE_ERROR_HOOK
     pointer x;
     pointer hdl=sc->ERROR_HOOK;
#endif

#if SHOW_ERROR_LINE
     char sbuf[STRBUFFSIZE];

     /* make sure error is not in REPL */
     if (sc->load_stack[sc->file_i].kind & port_file &&
         sc->load_stack[sc->file_i].rep.stdio.file != stdin) {
       int ln = sc->load_stack[sc->file_i].rep.stdio.curr_line;
       const char *fname = sc->load_stack[sc->file_i].rep.stdio.filename;

       /* should never happen */
       if(!fname) fname = "<unknown>";

       /* we started from 0 */
       ln++;
       /* Err kind s is first, to have a stable prefix for testing. */
       snprintf(sbuf, STRBUFFSIZE, "%s (%s : %i) ", s, fname, ln);

       str = (const char*)sbuf;
     }
#endif

#if USE_ERROR_HOOK
     x=find_slot_in_env(sc,sc->envir,hdl,1);
    if (x != sc->NIL) {
         if(a!=0) {
               sc->code = cons(sc, cons(sc, sc->QUOTE, cons(sc,(a), sc->NIL)), sc->NIL);
         } else {
               sc->code = sc->NIL;
         }
         sc->code = cons(sc, mk_string(sc, str), sc->code);
         setimmutable(car(sc->code));
         sc->code = cons(sc, slot_value_in_env(x), sc->code);
         sc->op = (int)OP_EVAL;
         return sc->T;
    }
#endif

    if(a!=0) {
          sc->args = cons(sc, (a), sc->NIL);
    } else {
          sc->args = sc->NIL;
    }
    sc->args = cons(sc, mk_string(sc, str), sc->args);
    setimmutable(car(sc->args));
    sc->op = (int)OP_ERR0;
    return sc->T;
}
#define Error_1(sc,s,a)  return _Error_1(sc,s,a)
#define Error_0(sc,s)    return _Error_1(sc,s,0)

/* Too small to turn into function */
# define  BEGIN     do {
# define  END  } while (0)
#define s_goto(sc,a) BEGIN                                  \
    sc->op = (int)(a);                                      \
    return sc->T; END

#define s_return(sc,a) return _s_return(sc,a)

#ifndef USE_SCHEME_STACK

/* this structure holds all the interpreter's registers */
struct dump_stack_frame {
  enum scheme_opcodes op;
  pointer args;
  pointer envir;
  pointer code;
};

#define STACK_GROWTH 3

static void s_save(scheme *sc, enum scheme_opcodes op, pointer args, pointer code)
{
  int nframes = (int)sc->dump;
  struct dump_stack_frame *next_frame;

  /* enough room for the next frame? */
  if (nframes >= sc->dump_size) {
    sc->dump_size += STACK_GROWTH;
    /* alas there is no sc->realloc */
    sc->dump_base = realloc(sc->dump_base,
                            sizeof(struct dump_stack_frame) * sc->dump_size);
  }
  next_frame = (struct dump_stack_frame *)sc->dump_base + nframes;
  next_frame->op = op;
  next_frame->args = args;
  next_frame->envir = sc->envir;
  next_frame->code = code;
  sc->dump = (pointer)(nframes+1);
}

static pointer _s_return(scheme *sc, pointer a)
{
  int nframes = (int)sc->dump;
  struct dump_stack_frame *frame;

  sc->value = (a);
  if (nframes <= 0) {
    return sc->NIL;
  }
  nframes--;
  frame = (struct dump_stack_frame *)sc->dump_base + nframes;
  sc->op = frame->op;
  sc->args = frame->args;
  sc->envir = frame->envir;
  sc->code = frame->code;
  sc->dump = (pointer)nframes;
  return sc->T;
}

static INLINE void dump_stack_reset(scheme *sc)
{
  /* in this implementation, sc->dump is the number of frames on the stack */
  sc->dump = (pointer)0;
}

static INLINE void dump_stack_initialize(scheme *sc)
{
  sc->dump_size = 0;
  sc->dump_base = NULL;
  dump_stack_reset(sc);
}

static void dump_stack_free(scheme *sc)
{
  free(sc->dump_base);
  sc->dump_base = NULL;
  sc->dump = (pointer)0;
  sc->dump_size = 0;
}

static INLINE void dump_stack_mark(scheme *sc)
{
  int nframes = (int)sc->dump;
  int i;
  for(i=0; i<nframes; i++) {
    struct dump_stack_frame *frame;
    frame = (struct dump_stack_frame *)sc->dump_base + i;
    mark(frame->args);
    mark(frame->envir);
    mark(frame->code);
  }
}

#else

static INLINE void dump_stack_reset(scheme *sc)
{
  sc->dump = sc->NIL;
}

static INLINE void dump_stack_initialize(scheme *sc)
{
  dump_stack_reset(sc);
}

static void dump_stack_free(scheme *sc)
{
  sc->dump = sc->NIL;
}

static pointer _s_return(scheme *sc, pointer a) {
    sc->value = (a);
    if(sc->dump==sc->NIL) return sc->NIL;
    sc->op = ivalue(car(sc->dump));
    sc->args = cadr(sc->dump);
    sc->envir = caddr(sc->dump);
    sc->code = cadddr(sc->dump);
    sc->dump = cddddr(sc->dump);
    return sc->T;
}

static void s_save(scheme *sc, enum scheme_opcodes op, pointer args, pointer code) {
    sc->dump = cons(sc, sc->envir, cons(sc, (code), sc->dump));
    sc->dump = cons(sc, (args), sc->dump);
    sc->dump = cons(sc, mk_integer(sc, (long)(op)), sc->dump);
}

static INLINE void dump_stack_mark(scheme *sc)
{
  mark(sc->dump);
}
#endif

#define s_retbool(tf)    s_return(sc,(tf) ? sc->T : sc->F)

static pointer opexe_0(scheme *sc, enum scheme_opcodes op) {
     pointer x, y;

     switch (op) {
     case OP_LOAD:       /* load */
          if(file_interactive(sc)) {
               fprintf(sc->outport->_object._port->rep.stdio.file,
                       "Loading %s\n", strvalue(car(sc->args)));
          }
          if (!file_push(sc,strvalue(car(sc->args)))) {
               Error_1(sc,"unable to open", car(sc->args));
          }
          else
          {
            sc->args = mk_integer(sc,sc->file_i);
            s_goto(sc,OP_T0LVL);
          }

     case OP_T0LVL: /* top level */
       /* If we reached the end of file, this loop is done. */
       if(sc->loadport->_object._port->kind & port_saw_EOF)
         {
           if(sc->file_i == 0)
             {
               sc->args=sc->NIL;
               s_goto(sc,OP_QUIT);
             }
           else
             {
               file_pop(sc);
               s_return(sc,sc->value);
             }
           /* NOTREACHED */
         }

       /* If interactive, be nice to user. */
       if(file_interactive(sc))
         {
           sc->envir = sc->global_env;
           dump_stack_reset(sc);
           putstr(sc,"\n");
           putstr(sc,prompt);
         }

       /* Set up another iteration of REPL */
       sc->nesting=0;
       sc->save_inport=sc->inport;
       sc->inport = sc->loadport;
       s_save(sc,OP_T0LVL, sc->NIL, sc->NIL);
       s_save(sc,OP_VALUEPRINT, sc->NIL, sc->NIL);
       s_save(sc,OP_T1LVL, sc->NIL, sc->NIL);
       s_goto(sc,OP_READ_INTERNAL);

     case OP_T1LVL: /* top level */
          sc->code = sc->value;
          sc->inport=sc->save_inport;
          s_goto(sc,OP_EVAL);

     case OP_READ_INTERNAL:       /* internal read */
          sc->tok = token(sc);
          if(sc->tok==TOK_EOF)
            { s_return(sc,sc->EOF_OBJ); }
          s_goto(sc,OP_RDSEXPR);

     case OP_GENSYM:
          s_return(sc, gensym(sc));

     case OP_VALUEPRINT: /* print evaluation result */
          /* OP_VALUEPRINT is always pushed, because when changing from
             non-interactive to interactive mode, it needs to be
             already on the stack */
       if(sc->tracing) {
         putstr(sc,"\nGives: ");
       }
       if(file_interactive(sc) || sc->print_output) {
         sc->print_flag = 1;
         sc->args = sc->value;
         s_goto(sc,OP_P0LIST);
       } else {
         s_return(sc,sc->value);
       }

     case OP_EVAL:       /* main part of evaluation */
#if USE_TRACING
       if(sc->tracing) {
         /*s_save(sc,OP_VALUEPRINT,sc->NIL,sc->NIL);*/
         s_save(sc,OP_REAL_EVAL,sc->args,sc->code);
         sc->args=sc->code;
         putstr(sc,"\nEval: ");
         s_goto(sc,OP_P0LIST);
       }
       /* fall through */
     case OP_REAL_EVAL:
#endif
          if (is_symbol(sc->code)) {    /* symbol */
               x=find_slot_in_env(sc,sc->envir,sc->code,1);
               if (x != sc->NIL) {
                    s_return(sc,slot_value_in_env(x));
               } else {
                    Error_1(sc,"eval: unbound variable:", sc->code);
               }
          } else if (is_pair(sc->code)) {
               if (is_syntax(x = car(sc->code))) {     /* SYNTAX */
                    sc->code = cdr(sc->code);
                    s_goto(sc,syntaxnum(x));
               } else {/* first, eval top element and eval arguments */
                    s_save(sc,OP_E0ARGS, sc->NIL, sc->code);
                    /* If no macros => s_save(sc,OP_E1ARGS, sc->NIL, cdr(sc->code));*/
                    sc->code = car(sc->code);
                    s_goto(sc,OP_EVAL);
               }
          } else {
               s_return(sc,sc->code);
          }

     case OP_E0ARGS:     /* eval arguments */
          if (is_macro(sc->value)) {    /* macro expansion */
               s_save(sc,OP_DOMACRO, sc->NIL, sc->NIL);
               sc->args = cons(sc,sc->code, sc->NIL);
               sc->code = sc->value;
               s_goto(sc,OP_APPLY);
          } else {
               sc->code = cdr(sc->code);
               s_goto(sc,OP_E1ARGS);
          }

     case OP_E1ARGS:     /* eval arguments */
          sc->args = cons(sc, sc->value, sc->args);
          if (is_pair(sc->code)) { /* continue */
               s_save(sc,OP_E1ARGS, sc->args, cdr(sc->code));
               sc->code = car(sc->code);
               sc->args = sc->NIL;
               s_goto(sc,OP_EVAL);
          } else {  /* end */
               sc->args = reverse_in_place(sc, sc->NIL, sc->args);
               sc->code = car(sc->args);
               sc->args = cdr(sc->args);
               s_goto(sc,OP_APPLY);
          }

#if USE_TRACING
     case OP_TRACING: {
       int tr=sc->tracing;
       sc->tracing=ivalue(car(sc->args));
       s_return(sc,mk_integer(sc,tr));
     }
#endif

     case OP_APPLY:      /* apply 'code' to 'args' */
#if USE_TRACING
       if(sc->tracing) {
         s_save(sc,OP_REAL_APPLY,sc->args,sc->code);
         sc->print_flag = 1;
         /*  sc->args=cons(sc,sc->code,sc->args);*/
         putstr(sc,"\nApply to: ");
         s_goto(sc,OP_P0LIST);
       }
       /* fall through */
     case OP_REAL_APPLY:
#endif
          if (is_proc(sc->code)) {
               s_goto(sc,procnum(sc->code));   /* PROCEDURE */
          } else if (is_foreign(sc->code))
          {
               /* Keep nested calls from GC'ing the arglist */
               push_recent_alloc(sc,sc->args,sc->NIL);
               sc->foreign_error = sc->NIL;
               x=sc->code->_object._ff(sc,sc->args);
               if (sc->foreign_error == sc->NIL) {
                   s_return(sc,x);
               } else {
                   x = sc->foreign_error;
                   sc->foreign_error = sc->NIL;
                   Error_1 (sc, string_value (car (x)), cdr (x));
               }
          } else if (is_closure(sc->code) || is_macro(sc->code)
                     || is_promise(sc->code)) { /* CLOSURE */
            /* Should not accept promise */
               /* make environment */
               new_frame_in_env(sc, closure_env(sc->code));
               for (x = car(closure_code(sc->code)), y = sc->args;
                    is_pair(x); x = cdr(x), y = cdr(y)) {
                    if (y == sc->NIL) {
                         Error_0(sc,"not enough arguments");
                    } else {
                         new_slot_in_env(sc, car(x), car(y));
                    }
               }
               if (x == sc->NIL) {
                    /*--
                     * if (y != sc->NIL) {
                     *   Error_0(sc,"too many arguments");
                     * }
                     */
               } else if (is_symbol(x))
                    new_slot_in_env(sc, x, y);
               else {
                    Error_1(sc,"syntax error in closure: not a symbol:", x);
               }
               sc->code = cdr(closure_code(sc->code));
               sc->args = sc->NIL;
               s_goto(sc,OP_BEGIN);
          } else if (is_continuation(sc->code)) { /* CONTINUATION */
               sc->dump = cont_dump(sc->code);
               s_return(sc,sc->args != sc->NIL ? car(sc->args) : sc->NIL);
          } else {
               Error_0(sc,"illegal function");
          }

     case OP_DOMACRO:    /* do macro */
          sc->code = sc->value;
          s_goto(sc,OP_EVAL);

#if 1
     case OP_LAMBDA:     /* lambda */
          /* If the hook is defined, apply it to sc->code, otherwise
             set sc->value fall thru */
          {
               pointer f=find_slot_in_env(sc,sc->envir,sc->COMPILE_HOOK,1);
               if(f==sc->NIL) {
                    sc->value = sc->code;
                    /* Fallthru */
               } else {
                    s_save(sc,OP_LAMBDA1,sc->args,sc->code);
                    sc->args=cons(sc,sc->code,sc->NIL);
                    sc->code=slot_value_in_env(f);
                    s_goto(sc,OP_APPLY);
               }
          }

     case OP_LAMBDA1:
          s_return(sc,mk_closure(sc, sc->value, sc->envir));

#else
     case OP_LAMBDA:     /* lambda */
          s_return(sc,mk_closure(sc, sc->code, sc->envir));

#endif

     case OP_MKCLOSURE: /* make-closure */
       x=car(sc->args);
       if(car(x)==sc->LAMBDA) {
         x=cdr(x);
       }
       if(cdr(sc->args)==sc->NIL) {
         y=sc->envir;
       } else {
         y=cadr(sc->args);
       }
       s_return(sc,mk_closure(sc, x, y));

     case OP_QUOTE:      /* quote */
          s_return(sc,car(sc->code));

     case OP_DEF0:  /* define */
          if(is_immutable(car(sc->code)))
                Error_1(sc,"define: unable to alter immutable", car(sc->code));

          if (is_pair(car(sc->code))) {
               x = caar(sc->code);
               sc->code = cons(sc, sc->LAMBDA, cons(sc, cdar(sc->code), cdr(sc->code)));
          } else {
               x = car(sc->code);
               sc->code = cadr(sc->code);
          }
          if (!is_symbol(x)) {
               Error_0(sc,"variable is not a symbol");
          }
          s_save(sc,OP_DEF1, sc->NIL, x);
          s_goto(sc,OP_EVAL);

     case OP_DEF1:  /* define */
          x=find_slot_in_env(sc,sc->envir,sc->code,0);
          if (x != sc->NIL) {
               set_slot_in_env(sc, x, sc->value);
          } else {
               new_slot_in_env(sc, sc->code, sc->value);
          }
          s_return(sc,sc->code);


     case OP_DEFP:  /* defined? */
          x=sc->envir;
          if(cdr(sc->args)!=sc->NIL) {
               x=cadr(sc->args);
          }
          s_retbool(find_slot_in_env(sc,x,car(sc->args),1)!=sc->NIL);

     case OP_SET0:       /* set! */
          if(is_immutable(car(sc->code)))
                Error_1(sc,"set!: unable to alter immutable variable",car(sc->code));
          s_save(sc,OP_SET1, sc->NIL, car(sc->code));
          sc->code = cadr(sc->code);
          s_goto(sc,OP_EVAL);

     case OP_SET1:       /* set! */
          y=find_slot_in_env(sc,sc->envir,sc->code,1);
          if (y != sc->NIL) {
             set_slot_in_env(sc, y, sc->value);
             s_return(sc,sc->value);
          } else {
             Error_1(sc,"set!: unbound variable:", sc->code);
          }

     case OP_BEGIN:      /* begin */
          if (!is_pair(sc->code)) {
               s_return(sc,sc->code);
          }
          if (cdr(sc->code) != sc->NIL) {
               s_save(sc,OP_BEGIN, sc->NIL, cdr(sc->code));
          }
          sc->code = car(sc->code);
          s_goto(sc,OP_EVAL);

     case OP_IF0:        /* if */
          s_save(sc,OP_IF1, sc->NIL, cdr(sc->code));
          sc->code = car(sc->code);
          s_goto(sc,OP_EVAL);

     case OP_IF1:        /* if */
          if (is_true(sc->value))
               sc->code = car(sc->code);
          else
               sc->code = cadr(sc->code);  /* (if #f 1) ==> () because
                                            * car(sc->NIL) = sc->NIL */
          s_goto(sc,OP_EVAL);

     case OP_LET0:       /* let */
          sc->args = sc->NIL;
          sc->value = sc->code;
          sc->code = is_symbol(car(sc->code)) ? cadr(sc->code) : car(sc->code);
          s_goto(sc,OP_LET1);

     case OP_LET1:       /* let (calculate parameters) */
          sc->args = cons(sc, sc->value, sc->args);
          if (is_pair(sc->code)) { /* continue */
               if (!is_pair(car(sc->code)) || !is_pair(cdar(sc->code))) {
                    Error_1(sc, "Bad syntax of binding spec in let :", car(sc->code));
               }
               s_save(sc,OP_LET1, sc->args, cdr(sc->code));
               sc->code = cadar(sc->code);
               sc->args = sc->NIL;
               s_goto(sc,OP_EVAL);
          } else {  /* end */
               sc->args = reverse_in_place(sc, sc->NIL, sc->args);
               sc->code = car(sc->args);
               sc->args = cdr(sc->args);
               s_goto(sc,OP_LET2);
          }

     case OP_LET2:       /* let */
          new_frame_in_env(sc, sc->envir);
          for (x = is_symbol(car(sc->code)) ? cadr(sc->code) : car(sc->code), y = sc->args;
               y != sc->NIL; x = cdr(x), y = cdr(y)) {
               new_slot_in_env(sc, caar(x), car(y));
          }
          if (is_symbol(car(sc->code))) {    /* named let */
               for (x = cadr(sc->code), sc->args = sc->NIL; x != sc->NIL; x = cdr(x)) {
                    if (!is_pair(x))
                        Error_1(sc, "Bad syntax of binding in let :", x);
                    if (!is_list(sc, car(x)))
                        Error_1(sc, "Bad syntax of binding in let :", car(x));
                    sc->args = cons(sc, caar(x), sc->args);
               }
               x = mk_closure(sc, cons(sc, reverse_in_place(sc, sc->NIL, sc->args), cddr(sc->code)), sc->envir);
               new_slot_in_env(sc, car(sc->code), x);
               sc->code = cddr(sc->code);
               sc->args = sc->NIL;
          } else {
               sc->code = cdr(sc->code);
               sc->args = sc->NIL;
          }
          s_goto(sc,OP_BEGIN);

     case OP_LET0AST:    /* let* */
          if (car(sc->code) == sc->NIL) {
               new_frame_in_env(sc, sc->envir);
               sc->code = cdr(sc->code);
               s_goto(sc,OP_BEGIN);
          }
          if(!is_pair(car(sc->code)) || !is_pair(caar(sc->code)) || !is_pair(cdaar(sc->code))) {
               Error_1(sc,"Bad syntax of binding spec in let* :",car(sc->code));
          }
          s_save(sc,OP_LET1AST, cdr(sc->code), car(sc->code));
          sc->code = cadaar(sc->code);
          s_goto(sc,OP_EVAL);

     case OP_LET1AST:    /* let* (make new frame) */
          new_frame_in_env(sc, sc->envir);
          s_goto(sc,OP_LET2AST);

     case OP_LET2AST:    /* let* (calculate parameters) */
          new_slot_in_env(sc, caar(sc->code), sc->value);
          sc->code = cdr(sc->code);
          if (is_pair(sc->code)) { /* continue */
               s_save(sc,OP_LET2AST, sc->args, sc->code);
               sc->code = cadar(sc->code);
               sc->args = sc->NIL;
               s_goto(sc,OP_EVAL);
          } else {  /* end */
               sc->code = sc->args;
               sc->args = sc->NIL;
               s_goto(sc,OP_BEGIN);
          }
     default:
          snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
          Error_0(sc,sc->strbuff);
     }
     return sc->T;
}

static pointer opexe_1(scheme *sc, enum scheme_opcodes op) {
     pointer x, y;

     switch (op) {
     case OP_LET0REC:    /* letrec */
          new_frame_in_env(sc, sc->envir);
          sc->args = sc->NIL;
          sc->value = sc->code;
          sc->code = car(sc->code);
          s_goto(sc,OP_LET1REC);

     case OP_LET1REC:    /* letrec (calculate parameters) */
          sc->args = cons(sc, sc->value, sc->args);
          if (is_pair(sc->code)) { /* continue */
               if (!is_pair(car(sc->code)) || !is_pair(cdar(sc->code))) {
                    Error_1(sc,"Bad syntax of binding spec in letrec :",car(sc->code));
               }
               s_save(sc,OP_LET1REC, sc->args, cdr(sc->code));
               sc->code = cadar(sc->code);
               sc->args = sc->NIL;
               s_goto(sc,OP_EVAL);
          } else {  /* end */
               sc->args = reverse_in_place(sc, sc->NIL, sc->args);
               sc->code = car(sc->args);
               sc->args = cdr(sc->args);
               s_goto(sc,OP_LET2REC);
          }

     case OP_LET2REC:    /* letrec */
          for (x = car(sc->code), y = sc->args; y != sc->NIL; x = cdr(x), y = cdr(y)) {
               new_slot_in_env(sc, caar(x), car(y));
          }
          sc->code = cdr(sc->code);
          sc->args = sc->NIL;
          s_goto(sc,OP_BEGIN);

     case OP_COND0:      /* cond */
          if (!is_pair(sc->code)) {
               Error_0(sc,"syntax error in cond");
          }
          s_save(sc,OP_COND1, sc->NIL, sc->code);
          sc->code = caar(sc->code);
          s_goto(sc,OP_EVAL);

     case OP_COND1:      /* cond */
          if (is_true(sc->value)) {
               if ((sc->code = cdar(sc->code)) == sc->NIL) {
                    s_return(sc,sc->value);
               }
               if(!sc->code) {
                    Error_0(sc,"syntax error in cond");
               }
               if(!sc->code || car(sc->code)==sc->FEED_TO) {
                    if(!is_pair(cdr(sc->code))) {
                         Error_0(sc,"syntax error in cond");
                    }
                    x=cons(sc, sc->QUOTE, cons(sc, sc->value, sc->NIL));
                    sc->code=cons(sc,cadr(sc->code),cons(sc,x,sc->NIL));
                    s_goto(sc,OP_EVAL);
               }
               s_goto(sc,OP_BEGIN);
          } else {
               if ((sc->code = cdr(sc->code)) == sc->NIL) {
                    s_return(sc,sc->NIL);
               } else {
                    s_save(sc,OP_COND1, sc->NIL, sc->code);
                    sc->code = caar(sc->code);
                    s_goto(sc,OP_EVAL);
               }
          }

     case OP_DELAY:      /* delay */
          x = mk_closure(sc, cons(sc, sc->NIL, sc->code), sc->envir);
          typeflag(x)=T_PROMISE;
          s_return(sc,x);

     case OP_AND0:       /* and */
          if (sc->code == sc->NIL) {
               s_return(sc,sc->T);
          }
          s_save(sc,OP_AND1, sc->NIL, cdr(sc->code));
          sc->code = car(sc->code);
          s_goto(sc,OP_EVAL);

     case OP_AND1:       /* and */
          if (is_false(sc->value)) {
               s_return(sc,sc->value);
          } else if (sc->code == sc->NIL) {
               s_return(sc,sc->value);
          } else {
               s_save(sc,OP_AND1, sc->NIL, cdr(sc->code));
               sc->code = car(sc->code);
               s_goto(sc,OP_EVAL);
          }

     case OP_OR0:        /* or */
          if (sc->code == sc->NIL) {
               s_return(sc,sc->F);
          }
          s_save(sc,OP_OR1, sc->NIL, cdr(sc->code));
          sc->code = car(sc->code);
          s_goto(sc,OP_EVAL);

     case OP_OR1:        /* or */
          if (is_true(sc->value)) {
               s_return(sc,sc->value);
          } else if (sc->code == sc->NIL) {
               s_return(sc,sc->value);
          } else {
               s_save(sc,OP_OR1, sc->NIL, cdr(sc->code));
               sc->code = car(sc->code);
               s_goto(sc,OP_EVAL);
          }

     case OP_C0STREAM:   /* cons-stream */
          s_save(sc,OP_C1STREAM, sc->NIL, cdr(sc->code));
          sc->code = car(sc->code);
          s_goto(sc,OP_EVAL);

     case OP_C1STREAM:   /* cons-stream */
          sc->args = sc->value;  /* save sc->value to register sc->args for gc */
          x = mk_closure(sc, cons(sc, sc->NIL, sc->code), sc->envir);
          typeflag(x)=T_PROMISE;
          s_return(sc,cons(sc, sc->args, x));

     case OP_MACRO0:     /* macro */
          if (is_pair(car(sc->code))) {
               x = caar(sc->code);
               sc->code = cons(sc, sc->LAMBDA, cons(sc, cdar(sc->code), cdr(sc->code)));
          } else {
               x = car(sc->code);
               sc->code = cadr(sc->code);
          }
          if (!is_symbol(x)) {
               Error_0(sc,"variable is not a symbol");
          }
          s_save(sc,OP_MACRO1, sc->NIL, x);
          s_goto(sc,OP_EVAL);

     case OP_MACRO1:     /* macro */
          typeflag(sc->value) = T_MACRO;
          x = find_slot_in_env(sc, sc->envir, sc->code, 0);
          if (x != sc->NIL) {
               set_slot_in_env(sc, x, sc->value);
          } else {
               new_slot_in_env(sc, sc->code, sc->value);
          }
          s_return(sc,sc->code);

     case OP_CASE0:      /* case */
          s_save(sc,OP_CASE1, sc->NIL, cdr(sc->code));
          sc->code = car(sc->code);
          s_goto(sc,OP_EVAL);

     case OP_CASE1:      /* case */
          for (x = sc->code; x != sc->NIL; x = cdr(x)) {
               if (!is_pair(y = caar(x))) {
                    break;
               }
               for ( ; y != sc->NIL; y = cdr(y)) {
                    if (eqv(car(y), sc->value)) {
                         break;
                    }
               }
               if (y != sc->NIL) {
                    break;
               }
          }
          if (x != sc->NIL) {
               if (is_pair(caar(x))) {
                    sc->code = cdar(x);
                    s_goto(sc,OP_BEGIN);
               } else {/* else */
                    s_save(sc,OP_CASE2, sc->NIL, cdar(x));
                    sc->code = caar(x);
                    s_goto(sc,OP_EVAL);
               }
          } else {
               s_return(sc,sc->NIL);
          }

     case OP_CASE2:      /* case */
          if (is_true(sc->value)) {
               s_goto(sc,OP_BEGIN);
          } else {
               s_return(sc,sc->NIL);
          }

     case OP_PAPPLY:     /* apply */
          sc->code = car(sc->args);
          sc->args = list_star(sc,cdr(sc->args));
          /*sc->args = cadr(sc->args);*/
          s_goto(sc,OP_APPLY);

     case OP_PEVAL: /* eval */
          if(cdr(sc->args)!=sc->NIL) {
               sc->envir=cadr(sc->args);
          }
          sc->code = car(sc->args);
          s_goto(sc,OP_EVAL);

     case OP_CONTINUATION:    /* call-with-current-continuation */
          sc->code = car(sc->args);
          sc->args = cons(sc, mk_continuation(sc, sc->dump), sc->NIL);
          s_goto(sc,OP_APPLY);

     default:
          snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
          Error_0(sc,sc->strbuff);
     }
     return sc->T;
}

static pointer opexe_2(scheme *sc, enum scheme_opcodes op) {
     pointer x, y;
     num v;
#if USE_MATH
     double dd;
#endif

     switch (op) {
#if USE_MATH
     case OP_INEX2EX:    /* inexact->exact */
          x=car(sc->args);
          if(num_is_integer(x)) {
               s_return(sc,x);
          } else if(modf(rvalue_unchecked(x),&dd)==0.0) {
               s_return(sc,mk_integer(sc,ivalue(x)));
          } else {
               Error_1(sc,"inexact->exact: not integral:",x);
          }

     case OP_EXP:
          x=car(sc->args);
          s_return(sc, mk_real(sc, exp(rvalue(x))));

     case OP_LOG:
          x=car(sc->args);
          s_return(sc, mk_real(sc, log(rvalue(x))));

     case OP_SIN:
          x=car(sc->args);
          s_return(sc, mk_real(sc, sin(rvalue(x))));

     case OP_COS:
          x=car(sc->args);
          s_return(sc, mk_real(sc, cos(rvalue(x))));

     case OP_TAN:
          x=car(sc->args);
          s_return(sc, mk_real(sc, tan(rvalue(x))));

     case OP_ASIN:
          x=car(sc->args);
          s_return(sc, mk_real(sc, asin(rvalue(x))));

     case OP_ACOS:
          x=car(sc->args);
          s_return(sc, mk_real(sc, acos(rvalue(x))));

     case OP_ATAN:
          x=car(sc->args);
          if(cdr(sc->args)==sc->NIL) {
               s_return(sc, mk_real(sc, atan(rvalue(x))));
          } else {
               y=cadr(sc->args);
               s_return(sc, mk_real(sc, atan2(rvalue(x),rvalue(y))));
          }

     case OP_SQRT:
          x=car(sc->args);
          s_return(sc, mk_real(sc, sqrt(rvalue(x))));

     case OP_EXPT: {
          double result;
          int real_result=1;
          x=car(sc->args);
          y=cadr(sc->args);
          if (num_is_integer(x) && num_is_integer(y))
             real_result=0;
          /* This 'if' is an R5RS compatibility fix. */
          /* NOTE: Remove this 'if' fix for R6RS.    */
          if (rvalue(x) == 0 && rvalue(y) < 0) {
             result = 0.0;
          } else {
             result = pow(rvalue(x),rvalue(y));
          }
          /* Before returning integer result make sure we can. */
          /* If the test fails, result is too big for integer. */
          if (!real_result)
          {
            long result_as_long = (long)result;
            if (result != (double)result_as_long)
              real_result = 1;
          }
          if (real_result) {
             s_return(sc, mk_real(sc, result));
          } else {
             s_return(sc, mk_integer(sc, result));
          }
     }

     case OP_FLOOR:
          x=car(sc->args);
          s_return(sc, mk_real(sc, floor(rvalue(x))));

     case OP_CEILING:
          x=car(sc->args);
          s_return(sc, mk_real(sc, ceil(rvalue(x))));

     case OP_TRUNCATE:
          x=car(sc->args);
          s_return(sc, mk_real(sc, trunc(rvalue(x))));

     case OP_ROUND:
        x=car(sc->args);
        if (num_is_integer(x))
            s_return(sc, x);
        s_return(sc, mk_real(sc, round_per_R5RS(rvalue(x))));
#endif

     case OP_ADD:        /* + */
       v=num_zero;
       for (x = sc->args; x != sc->NIL; x = cdr(x)) {
         v=num_add(v,nvalue(car(x)));
       }
       s_return(sc,mk_number(sc, v));

     case OP_MUL:        /* * */
       v=num_one;
       for (x = sc->args; x != sc->NIL; x = cdr(x)) {
         v=num_mul(v,nvalue(car(x)));
       }
       s_return(sc,mk_number(sc, v));

     case OP_SUB:        /* - */
       if(cdr(sc->args)==sc->NIL) {
         x=sc->args;
         v=num_zero;
       } else {
         x = cdr(sc->args);
         v = nvalue(car(sc->args));
       }
       for (; x != sc->NIL; x = cdr(x)) {
         v=num_sub(v,nvalue(car(x)));
       }
       s_return(sc,mk_number(sc, v));

     case OP_DIV:        /* / */
       if(cdr(sc->args)==sc->NIL) {
         x=sc->args;
         v=num_one;
       } else {
         x = cdr(sc->args);
         v = nvalue(car(sc->args));
       }
       for (; x != sc->NIL; x = cdr(x)) {
         if (!is_zero_double(rvalue(car(x))))
           v=num_div(v,nvalue(car(x)));
         else {
           Error_0(sc,"/: division by zero");
         }
       }
       s_return(sc,mk_number(sc, v));

     case OP_INTDIV:        /* quotient */
          v = nvalue(car(sc->args));
          x = cadr(sc->args);
          if (ivalue(x) != 0)
               v=num_intdiv(v,nvalue(x));
          else {
               Error_0(sc,"quotient: division by zero");
          }
          s_return(sc,mk_number(sc, v));

     case OP_REM:        /* remainder */
          v = nvalue(car(sc->args));
          x = cadr(sc->args);
          if (ivalue(x) != 0)
               v=num_rem(v,nvalue(x));
          else {
               Error_0(sc,"remainder: division by zero");
          }
          s_return(sc,mk_number(sc, v));

     case OP_MOD:        /* modulo */
          v = nvalue(car(sc->args));
          x = cadr(sc->args);
          if (ivalue(x) != 0)
               v=num_mod(v,nvalue(x));
          else {
               Error_0(sc,"modulo: division by zero");
          }
          s_return(sc,mk_number(sc, v));

     case OP_CAR:        /* car */
       s_return(sc,caar(sc->args));

     case OP_CDR:        /* cdr */
       s_return(sc,cdar(sc->args));

     case OP_CONS:       /* cons */
          cdr(sc->args) = cadr(sc->args);
          s_return(sc,sc->args);

     case OP_SETCAR:     /* set-car! */
       if(!is_immutable(car(sc->args))) {
         caar(sc->args) = cadr(sc->args);
         s_return(sc,car(sc->args));
       } else {
         Error_0(sc,"set-car!: unable to alter immutable pair");
       }

     case OP_SETCDR:     /* set-cdr! */
       if(!is_immutable(car(sc->args))) {
         cdar(sc->args) = cadr(sc->args);
         s_return(sc,car(sc->args));
       } else {
         Error_0(sc,"set-cdr!: unable to alter immutable pair");
       }

    case OP_BYTE2INT: /* byte->integer */
      {
        guint8 b;
        b = ivalue (car (sc->args));
        s_return (sc, mk_integer (sc, b));
      }

    case OP_INT2BYTE: /* integer->byte */
      {
        guint8 b;
        b = ivalue (car (sc->args));
        s_return (sc, mk_byte (sc, b));
      }

     case OP_CHAR2INT: { /* char->integer */
          gunichar c;
          c=ivalue(car(sc->args));
          s_return(sc,mk_integer(sc,c));
     }

     case OP_INT2CHAR: { /* integer->char */
          gunichar c;
          c=(gunichar)ivalue(car(sc->args));
          s_return(sc,mk_character(sc,c));
     }

     case OP_CHARUPCASE: {
          gunichar c;
          c=(gunichar)ivalue(car(sc->args));
          c=g_unichar_toupper(c);
          s_return(sc,mk_character(sc,c));
     }

     case OP_CHARDNCASE: {
          gunichar c;
          c=(gunichar)ivalue(car(sc->args));
          c=g_unichar_tolower(c);
          s_return(sc,mk_character(sc,c));
     }

     case OP_STR2SYM:  /* string->symbol */
          s_return(sc,mk_symbol(sc,strvalue(car(sc->args))));

     case OP_STR2ATOM: /* string->atom */ {
          char *s=strvalue(car(sc->args));
          long pf = 0;
          if(cdr(sc->args)!=sc->NIL) {
            /* we know cadr(sc->args) is a natural number */
            /* see if it is 2, 8, 10, or 16, or error */
            pf = ivalue_unchecked(cadr(sc->args));
            if(pf == 16 || pf == 10 || pf == 8 || pf == 2) {
               /* base is OK */
            }
            else {
              pf = -1;
            }
          }
          if (pf < 0) {
            Error_1(sc, "string->atom: bad base:", cadr(sc->args));
          } else if(*s=='#') /* no use of base! */ {
            s_return(sc, mk_sharp_const(sc, s+1));
          } else {
            if (pf == 0 || pf == 10) {
              s_return(sc, mk_atom(sc, s));
            }
            else {
              char *ep;
              long iv = strtol(s,&ep,(int )pf);
              if (*ep == 0) {
                s_return(sc, mk_integer(sc, iv));
              }
              else {
                s_return(sc, sc->F);
              }
            }
          }
        }

     case OP_SYM2STR: /* symbol->string */
          x=mk_string(sc,symname(car(sc->args)));
          setimmutable(x);
          s_return(sc,x);

     case OP_ATOM2STR: /* atom->string */ {
          long pf = 0;
          x=car(sc->args);
          y=cadr(sc->args);
          if(y!=sc->NIL) {
            /* we know cadr(sc->args) is a natural number */
            /* see if it is 2, 8, 10, or 16, or error */
            pf = ivalue_unchecked(y);
            if(is_number(x) && (pf == 16 || pf == 10 || pf == 8 || pf == 2)) {
              /* base is OK */
            }
            else {
              pf = -1;
            }
          }
          if (pf < 0) {
            Error_1(sc, "atom->string: bad base:", y);
          }
        else if (is_number (x)    || is_byte (x)   ||
                 is_character (x) || is_string (x) ||
                 is_symbol (x))
          {
            char *p;
            int   len;
            atom2str (sc, x, (int)pf, &p, &len);
            s_return (sc, mk_counted_string (sc, p, len));
          } else {
            Error_1(sc, "atom->string: not an atom:", x);
          }
        }

     case OP_MKSTRING: { /* make-string */
          gunichar fill=' ';
          int len;

          len=ivalue(car(sc->args));

          if(cdr(sc->args)!=sc->NIL) {
               fill=charvalue(cadr(sc->args));
          }
          s_return(sc,mk_empty_string(sc,len,fill));
     }

     case OP_STRLEN:  /* string-length */
          s_return(sc,mk_integer(sc,g_utf8_strlen(strvalue(car(sc->args)), -1)));

     case OP_STRREF: { /* string-ref */
          char *str;
          int index;

          str=strvalue(car(sc->args));

          x=cadr(sc->args);
          if (!is_integer(x)) {
               Error_1(sc,"string-ref: index must be exact:",x);
          }

          index=ivalue(x);
          if(index>=g_utf8_strlen(strvalue(car(sc->args)), -1)) {
               Error_1(sc,"string-ref: out of bounds:",x);
          }

          str = g_utf8_offset_to_pointer(str, (long)index);
          s_return(sc,mk_character(sc, g_utf8_get_char(str)));
     }

     case OP_STRSET: { /* string-set! */
          char *str;
          int   index;
          gunichar c;
          char  utf8[7];
          int   utf8_len;
          int   newlen;
          char *p1, *p2;
          int   p1_len;
          int   p2_len;
          char *newstr;

          x=car(sc->args);
          if(is_immutable(x)) {
               Error_1(sc,"string-set!: unable to alter immutable string:",x);
          }

          str=strvalue(x);

          y=cadr(sc->args);
          if (!is_integer(y)) {
               Error_1(sc,"string-set!: index must be exact:",y);
          }

          index=ivalue(y);
          if(index>=strlength(x)) {
               Error_1(sc,"string-set!: out of bounds:",y);
          }

          c=charvalue(caddr(sc->args));
          utf8_len = g_unichar_to_utf8(c, utf8);

          p1 = g_utf8_offset_to_pointer(str, (long)index);
          p2 = g_utf8_offset_to_pointer(str, (long)index+1);
          p1_len = p1-str;
          p2_len = strlen(p2);

          newlen = p1_len+utf8_len+p2_len;
          newstr = (char *)sc->malloc(newlen+1);
          if (newstr == NULL) {
             g_warning ("%s string-set", G_STRFUNC);
             sc->no_memory=1;
             Error_1(sc,"string-set!: No memory to alter string:",x);
          }

          if (p1_len > 0)
             memcpy(newstr, str, p1_len);
          memcpy(newstr+p1_len, utf8, utf8_len);
          if (p2_len > 0)
             memcpy(newstr+p1_len+utf8_len, p2, p2_len);
          newstr[newlen] = '\0';

          free(strvalue(x));
          strvalue(x)=newstr;
          strlength(x)=g_utf8_strlen(newstr, -1);

          s_return(sc,x);
     }

     case OP_STRAPPEND: { /* string-append */
       /* in 1.29 string-append was in Scheme in init.scm but was too slow */
       int len = 0;
       pointer car_x;
       char *newstr;
       char *pos;
       char *end;

       /* compute needed length for new string */
       for (x = sc->args; x != sc->NIL; x = cdr(x)) {
          car_x = car(x);
          end = g_utf8_offset_to_pointer(strvalue(car_x), (long)strlength(car_x));
          len += end - strvalue(car_x);
       }

       newstr = (char *)sc->malloc(len+1);
       if (newstr == NULL) {
          g_warning ("%s string-append", G_STRFUNC);
          sc->no_memory=1;
          Error_1(sc,"string-set!: No memory to append strings:",car(sc->args));
       }

       /* store the contents of the argument strings into the new string */
       pos = newstr;
       for (x = sc->args; x != sc->NIL; x = cdr(x)) {
           car_x = car(x);
           end = g_utf8_offset_to_pointer(strvalue(car_x), (long)strlength(car_x));
           len = end - strvalue(car_x);
           memcpy(pos, strvalue(car_x), len);
           pos += len;
       }
       *pos = '\0';

       car_x = mk_string(sc, newstr);
       g_free(newstr);

       s_return(sc, car_x);
     }

     case OP_SUBSTR: { /* substring */
          char *str;
          char *beg;
          char *end;
          int index0;
          int index1;
          int len;
          pointer x;

          str=strvalue(car(sc->args));

          index0=ivalue(cadr(sc->args));

          if(index0>g_utf8_strlen(str, -1)) {
               Error_1(sc,"substring: start out of bounds:",cadr(sc->args));
          }

          if(cddr(sc->args)!=sc->NIL) {
               index1=ivalue(caddr(sc->args));
               if(index1>g_utf8_strlen(str, -1) || index1<index0) {
                    Error_1(sc,"substring: end out of bounds:",caddr(sc->args));
               }
          } else {
               index1=g_utf8_strlen(str, -1);
          }

          /* store the contents of the argument strings into the new string */
          beg = g_utf8_offset_to_pointer(str, (long)index0);
          end = g_utf8_offset_to_pointer(str, (long)index1);
          len=end-beg;

          str = (char *)sc->malloc(len+1);
          if (str == NULL) {
             g_warning ("%s substring", G_STRFUNC);
             sc->no_memory=1;
             Error_1(sc,"string-set!: No memory to extract substring:",car(sc->args));
          }

          memcpy(str, beg, len);
          str[len] = '\0';

          x = mk_string(sc, str);
          g_free(str);

          s_return(sc,x);
     }

     case OP_VECTOR: {   /* vector */
          int i;
          pointer vec;
          int len=list_length(sc,sc->args);
          if(len<0) {
               Error_1(sc,"vector: not a proper list:",sc->args);
          }
          vec=mk_vector(sc,len);
          if(sc->no_memory) { s_return(sc, sc->sink); }
          for (x = sc->args, i = 0; is_pair(x); x = cdr(x), i++) {
               set_vector_elem(vec,i,car(x));
          }
          s_return(sc,vec);
     }

     case OP_MKVECTOR: { /* make-vector */
          pointer fill=sc->NIL;
          int len;
          pointer vec;

          len=ivalue(car(sc->args));

          if(cdr(sc->args)!=sc->NIL) {
               fill=cadr(sc->args);
          }
          vec=mk_vector(sc,len);
          if(sc->no_memory) { s_return(sc, sc->sink); }
          if(fill!=sc->NIL) {
               fill_vector(vec,fill);
          }
          s_return(sc,vec);
     }

     case OP_VECLEN:  /* vector-length */
          s_return(sc,mk_integer(sc,ivalue(car(sc->args))));

     case OP_VECREF: { /* vector-ref */
          int index;

          x=cadr(sc->args);
          if (!is_integer(x)) {
               Error_1(sc,"vector-ref: index must be exact:",x);
          }
          index=ivalue(x);

          if(index>=ivalue(car(sc->args))) {
               Error_1(sc,"vector-ref: out of bounds:",x);
          }

          s_return(sc,vector_elem(car(sc->args),index));
     }

     case OP_VECSET: {   /* vector-set! */
          int index;

          if(is_immutable(car(sc->args))) {
               Error_1(sc,"vector-set!: unable to alter immutable vector:",car(sc->args));
          }

          x=cadr(sc->args);
          if (!is_integer(x)) {
               Error_1(sc,"vector-set!: index must be exact:",x);
          }

          index=ivalue(x);
          if(index>=ivalue(car(sc->args))) {
               Error_1(sc,"vector-set!: out of bounds:",x);
          }

          set_vector_elem(car(sc->args),index,caddr(sc->args));
          s_return(sc,car(sc->args));
     }

     default:
          snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
          Error_0(sc,sc->strbuff);
     }
     return sc->T;
}

static int is_list(scheme *sc, pointer a)
{ return list_length(sc,a) >= 0; }

/* Result is:
   proper list: length
   circular list: -1
   not even a pair: -2
   dotted list: -2 minus length before dot
*/
int list_length(scheme *sc, pointer p) {
    int i=0;
    pointer slow, fast;

    slow = fast = p;
    while (1)
    {
        if (fast == sc->NIL)
                return i;
        if (!is_pair(fast))
                return -2 - i;
        fast = cdr(fast);
        ++i;
        if (fast == sc->NIL)
                return i;
        if (!is_pair(fast))
                return -2 - i;
        ++i;
        fast = cdr(fast);

       /* Safe because we would have already returned if `fast'
          encountered a non-pair. */
        slow = cdr(slow);
        if (fast == slow)
        {
            /* the fast pointer has looped back around and caught up
               with the slow pointer, hence the structure is circular,
               not of finite length, and therefore not a list */
            return -1;
        }
    }
}

static pointer opexe_3(scheme *sc, enum scheme_opcodes op) {
     pointer x;
     num v;
     int (*comp_func)(num,num)=0;

     switch (op) {
     case OP_NOT:        /* not */
          s_retbool(is_false(car(sc->args)));
     case OP_BOOLP:       /* boolean? */
          s_retbool(car(sc->args) == sc->F || car(sc->args) == sc->T);
     case OP_EOFOBJP:       /* boolean? */
          s_retbool(car(sc->args) == sc->EOF_OBJ);
     case OP_NULLP:       /* null? */
          s_retbool(car(sc->args) == sc->NIL);
     case OP_NUMEQ:      /* = */
     case OP_LESS:       /* < */
     case OP_GRE:        /* > */
     case OP_LEQ:        /* <= */
     case OP_GEQ:        /* >= */
          switch(op) {
               case OP_NUMEQ: comp_func=num_eq; break;
               case OP_LESS:  comp_func=num_lt; break;
               case OP_GRE:   comp_func=num_gt; break;
               case OP_LEQ:   comp_func=num_le; break;
               case OP_GEQ:   comp_func=num_ge; break;
               default:       break;  /* Quiet the compiler */
          }
          x=sc->args;
          v=nvalue(car(x));
          x=cdr(x);

          for (; x != sc->NIL; x = cdr(x)) {
               if(!comp_func(v,nvalue(car(x)))) {
                    s_retbool(0);
               }
               v=nvalue(car(x));
          }
          s_retbool(1);
     case OP_SYMBOLP:     /* symbol? */
          s_retbool(is_symbol(car(sc->args)));
     case OP_NUMBERP:     /* number? */
          s_retbool(is_number(car(sc->args)));
     case OP_STRINGP:     /* string? */
          s_retbool(is_string(car(sc->args)));
     case OP_INTEGERP:     /* integer? */
          s_retbool(is_integer(car(sc->args)));
     case OP_REALP:     /* real? */
          s_retbool(is_number(car(sc->args))); /* All numbers are real */
    case OP_BYTEP:      /* byte? */
      s_retbool (is_byte (car (sc->args)));
     case OP_CHARP:     /* char? */
          s_retbool(is_character(car(sc->args)));
#if USE_CHAR_CLASSIFIERS
     case OP_CHARAP:     /* char-alphabetic? */
          s_retbool(Cisalpha(ivalue(car(sc->args))));
     case OP_CHARNP:     /* char-numeric? */
          s_retbool(Cisdigit(ivalue(car(sc->args))));
     case OP_CHARWP:     /* char-whitespace? */
          s_retbool(Cisspace(ivalue(car(sc->args))));
     case OP_CHARUP:     /* char-upper-case? */
          s_retbool(Cisupper(ivalue(car(sc->args))));
     case OP_CHARLP:     /* char-lower-case? */
          s_retbool(Cislower(ivalue(car(sc->args))));
#endif
     case OP_PORTP:     /* port? */
          s_retbool(is_port(car(sc->args)));
     case OP_INPORTP:     /* input-port? */
          s_retbool(is_inport(car(sc->args)));
     case OP_OUTPORTP:     /* output-port? */
          s_retbool(is_outport(car(sc->args)));
     case OP_PROCP:       /* procedure? */
          /*--
              * continuation should be procedure by the example
              * (call-with-current-continuation procedure?) ==> #t
                 * in R^3 report sec. 6.9
              */
          s_retbool(is_proc(car(sc->args)) || is_closure(car(sc->args))
                 || is_continuation(car(sc->args)) || is_foreign(car(sc->args)));
     case OP_PAIRP:       /* pair? */
          s_retbool(is_pair(car(sc->args)));
     case OP_LISTP:       /* list? */
          s_retbool(list_length(sc,car(sc->args)) >= 0);
     case OP_ENVP:        /* environment? */
          s_retbool(is_environment(car(sc->args)));
     case OP_VECTORP:     /* vector? */
          s_retbool(is_vector(car(sc->args)));
     case OP_EQ:         /* eq? */
          s_retbool(car(sc->args) == cadr(sc->args));
     case OP_EQV:        /* eqv? */
          s_retbool(eqv(car(sc->args), cadr(sc->args)));
     default:
          snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
          Error_0(sc,sc->strbuff);
     }
     return sc->T;
}

static pointer opexe_4(scheme *sc, enum scheme_opcodes op) {
     pointer x, y;

     switch (op) {
     case OP_FORCE:      /* force */
          sc->code = car(sc->args);
          if (is_promise(sc->code)) {
               /* Should change type to closure here */
               s_save(sc, OP_SAVE_FORCED, sc->NIL, sc->code);
               sc->args = sc->NIL;
               s_goto(sc,OP_APPLY);
          } else {
               s_return(sc,sc->code);
          }

     case OP_SAVE_FORCED:     /* Save forced value replacing promise */
          memcpy(sc->code,sc->value,sizeof(struct cell));
          s_return(sc,sc->value);

     case OP_WRITE:      /* write */
     case OP_DISPLAY:    /* display */
     case OP_WRITE_CHAR: /* write-char */
     case OP_WRITE_BYTE: /* write-byte */
          if(is_pair(cdr(sc->args))) {
               if(cadr(sc->args)!=sc->outport) {
                    x=cons(sc,sc->outport,sc->NIL);
                    s_save(sc,OP_SET_OUTPORT, x, sc->NIL);
                    set_outport (sc, cadr (sc->args));
               }
          }
          sc->args = car(sc->args);
          if(op==OP_WRITE) {
               sc->print_flag = 1;
          } else {
               sc->print_flag = 0;
          }
          s_goto(sc,OP_P0LIST);

     case OP_NEWLINE:    /* newline */
          if(is_pair(sc->args)) {
               if(car(sc->args)!=sc->outport) {
                    x=cons(sc,sc->outport,sc->NIL);
                    s_save(sc,OP_SET_OUTPORT, x, sc->NIL);
                    set_outport (sc, car (sc->args));
               }
          }
          putstr(sc, "\n");
          s_return(sc,sc->T);

     case OP_ERR0:  /* error */
          /* Subsequently, the current interpretation will abort. */
          sc->retcode=-1;

          /* Subsequently, putbytes will write to error_port*/
          error_port_redirect_output (sc);

          /* Print prefix: "Error: <reason>" OR "Error: --" */
          if (!is_string(car(sc->args))) {
               sc->args=cons(sc,mk_string(sc," -- "),sc->args);
               setimmutable(car(sc->args));
          }
          putstr(sc, "Error: ");
          putstr(sc, strvalue(car(sc->args)));
          sc->args = cdr(sc->args);
          s_goto(sc,OP_ERR1);

     case OP_ERR1:  /* error */
          putstr(sc, " ");
          if (sc->args != sc->NIL) {
               /* Print other args.*/
               s_save(sc,OP_ERR1, cdr(sc->args), sc->NIL);
               sc->args = car(sc->args);
               sc->print_flag = 1;
               s_goto(sc,OP_P0LIST);
               /* Continues at saved OP_ERR1. */
          } else {
              if (sc->interactive_repl)
                {
                  /* Case is SF Text Console, not SF Console (w GUI).
                   * Simple write to stdout.
                   */
                  gchar *error_message = (gchar*) error_port_take_string_and_close (sc);
                  g_printf ("%s", error_message);
                  g_free (error_message);

                  /* Continue to read stdin and eval. */
                  s_goto (sc, OP_T0LVL);
                }
              else
                {
                  /* ScriptFu wrapper will retrieve error message. */
                  return sc->NIL;
                }
          }

     case OP_REVERSE:   /* reverse */
          s_return(sc,reverse(sc, car(sc->args)));

     case OP_LIST_STAR: /* list* */
          s_return(sc,list_star(sc,sc->args));

     case OP_APPEND:    /* append */
          x = sc->NIL;
          y = sc->args;
          if (y == x) {
              s_return(sc, x);
          }

          /* cdr() in the while condition is not a typo. If car() */
          /* is used (append '() 'a) will return the wrong result.*/
          while (cdr(y) != sc->NIL) {
              x = revappend(sc, x, car(y));
              y = cdr(y);
              if (x == sc->F) {
                  Error_0(sc, "non-list argument to append");
              }
          }

          s_return(sc, reverse_in_place(sc, car(y), x));

#if USE_PLIST
     case OP_PUT:        /* put */
          if (!hasprop(car(sc->args)) || !hasprop(cadr(sc->args))) {
               Error_0(sc,"illegal use of put");
          }
          for (x = symprop(car(sc->args)), y = cadr(sc->args); x != sc->NIL; x = cdr(x)) {
               if (caar(x) == y) {
                    break;
               }
          }
          if (x != sc->NIL)
               cdar(x) = caddr(sc->args);
          else
               symprop(car(sc->args)) = cons(sc, cons(sc, y, caddr(sc->args)),
                                symprop(car(sc->args)));
          s_return(sc,sc->T);

     case OP_GET:        /* get */
          if (!hasprop(car(sc->args)) || !hasprop(cadr(sc->args))) {
               Error_0(sc,"illegal use of get");
          }
          for (x = symprop(car(sc->args)), y = cadr(sc->args); x != sc->NIL; x = cdr(x)) {
               if (caar(x) == y) {
                    break;
               }
          }
          if (x != sc->NIL) {
               s_return(sc,cdar(x));
          } else {
               s_return(sc,sc->NIL);
          }
#endif /* USE_PLIST */
     case OP_QUIT:       /* quit */
          if(is_pair(sc->args)) {
               gint err_code = ivalue (car (sc->args));

               sc->retcode = err_code;

               /* ScriptFu specific.
                * Non-zero code means script as PDB procedure declares failure.
                * Invariant that a SF fail puts message to output port.
                *
                * FIXME: instead of this, which diverges from upstream,
                * ask upstream for QUIT_HOOK OR make script_fu_interpret_string
                * ensure non-empty error message in this case.
                */
               if (err_code != 0)
                 {
                    snprintf (sc->strbuff, STRBUFFSIZE, "script quit with code: %d", err_code);
                    putstr (sc, sc->strbuff);
                 }
          }
          return (sc->NIL);

     case OP_GC:         /* gc */
          gc(sc, sc->NIL, sc->NIL);
          s_return(sc,sc->T);

     case OP_GCVERB:          /* gc-verbose */
     {    int  was = sc->gc_verbose;

          sc->gc_verbose = (car(sc->args) != sc->F);
          s_retbool(was);
     }

     case OP_NEWSEGMENT: /* new-segment */
          if (!is_pair(sc->args) || !is_number(car(sc->args))) {
               Error_0(sc,"new-segment: argument must be a number");
          }
          alloc_cellseg(sc, (int) ivalue(car(sc->args)));
          s_return(sc,sc->T);

     case OP_OBLIST: /* oblist */
          s_return(sc, oblist_all_symbols(sc));

     case OP_CURR_INPORT: /* current-input-port */
          s_return(sc,sc->inport);

     case OP_CURR_OUTPORT: /* current-output-port */
          s_return(sc,sc->outport);

     case OP_OPEN_INFILE: /* open-input-file */
     case OP_OPEN_OUTFILE: /* open-output-file */
     case OP_OPEN_INOUTFILE: /* open-input-output-file */ {
          int prop=0;
          pointer p;
          switch(op) {
               case OP_OPEN_INFILE:     prop=port_input; break;
               case OP_OPEN_OUTFILE:    prop=port_output; break;
               case OP_OPEN_INOUTFILE:  prop=port_input|port_output; break;
               default:                 break;  /* Quiet the compiler */
          }
          p=port_from_filename(sc,strvalue(car(sc->args)),prop);
          if(p==sc->NIL) {
               s_return(sc,sc->F);
          }
          s_return(sc,p);
     }

#if USE_STRING_PORTS
     case OP_OPEN_INSTRING: /* open-input-string */
     case OP_OPEN_INOUTSTRING: /* open-input-output-string */ {
          int prop=0;
          pointer p;
          switch(op) {
               case OP_OPEN_INSTRING:     prop=port_input; break;
               case OP_OPEN_INOUTSTRING:  prop=port_input|port_output; break;
               default:                   break;  /* Quiet the compiler */
          }
          /* !!! open-input-output-string is defined same as open-input-string.
           * The prop argument has no effect.
           * This version's behavior is not the same as upstream.
           *
           * Because no document specifies how input-output should work,
           * we cannot test whatever is implemented upstream.
           * Assume it should be a pipe, a string-port does NOT have
           * both a read and write pointer, and so cannot be a pipe.
           */
          p = string_port_open_input_string (sc, car (sc->args), prop);

          if(p==sc->NIL) {
               s_return(sc,sc->F);
          }
          s_return(sc,p);
     }
     case OP_OPEN_OUTSTRING: /* open-output-string */ {
          pointer p = string_port_open_output_string (sc, car(sc->args));
          if( p == sc->NIL)
              s_return(sc,sc->F);
          else
            s_return(sc,p);
     }
     case OP_GET_OUTSTRING: /* get-output-string */ {
          port *p;

          if ((p=car(sc->args)->_object._port)->kind&port_string)
            s_return (sc, string_port_get_output_string (sc, p));
          else
            s_return(sc, sc->F);
     }
#endif

     case OP_CLOSE_INPORT: /* close-input-port */
          port_close(sc,car(sc->args),port_input);
          s_return(sc,sc->T);

     case OP_CLOSE_OUTPORT: /* close-output-port */
          port_close(sc,car(sc->args),port_output);
          s_return(sc,sc->T);

     case OP_INT_ENV: /* interaction-environment */
          s_return(sc,sc->global_env);

     case OP_CURR_ENV: /* current-environment */
          s_return(sc,sc->envir);

     default:
          sprintf(sc->strbuff, "%d: illegal operator", sc->op);
          Error_0(sc,sc->strbuff);
     }
     return sc->T;
}

static pointer opexe_5(scheme *sc, enum scheme_opcodes op) {
     pointer x;
     char *trans_str;

     if(sc->nesting!=0) {
          int n=sc->nesting;
          sc->nesting=0;
          sc->retcode=-1;
          Error_1(sc,"unmatched parentheses:",mk_integer(sc,n));
     }

     switch (op) {
     /* ========== reading part ========== */
     case OP_READ:
          if(!is_pair(sc->args)) {
               s_goto(sc,OP_READ_INTERNAL);
          }
          if(!is_inport(car(sc->args))) {
               Error_1(sc,"read: not an input port:",car(sc->args));
          }
          if(car(sc->args)==sc->inport) {
               s_goto(sc,OP_READ_INTERNAL);
          }
          x=sc->inport;
          sc->inport=car(sc->args);
          x=cons(sc,x,sc->NIL);
          s_save(sc,OP_SET_INPORT, x, sc->NIL);
          s_goto(sc,OP_READ_INTERNAL);

    case OP_READ_CHAR: /* read-char */
    case OP_PEEK_CHAR: /* peek-char */
      {
        gint value;
        if (is_pair (sc->args))
          {
            if (car (sc->args) != sc->inport)
              {
                x = sc->inport;
                x = cons (sc, x, sc->NIL);
                s_save (sc, OP_SET_INPORT, x, sc->NIL);
                sc->inport = car (sc->args);
              }
          }
        value = inchar (sc);
        if (value == EOF)
          s_return (sc, sc->EOF_OBJ);
        if (op == OP_PEEK_CHAR)
          backchar (sc, value);
        s_return (sc, mk_character (sc, value));
      }

    case OP_READ_BYTE: /* read-byte */
    case OP_PEEK_BYTE: /* peek-byte */
      {
        gint value;
        if (is_pair (sc->args))
          {
            if (car (sc->args) != sc->inport)
              {
                x = sc->inport;
                x = cons (sc, x, sc->NIL);
                s_save(sc, OP_SET_INPORT, x, sc->NIL);
                sc->inport = car (sc->args);
              }
          }
        value = inbyte (sc);
        if (value == EOF)
          s_return (sc, sc->EOF_OBJ);
        if (op == OP_PEEK_BYTE)
          backbyte (sc, value);
        s_return (sc, mk_byte (sc, value));
      }

    case OP_CHAR_READY: /* char-ready? */
    case OP_BYTE_READY: /* byte-ready? */
      {
        pointer p = sc->inport;
        int     res;
        if (is_pair (sc->args))
          {
            p = car (sc->args);
          }
        res = p->_object._port->kind & port_string;
        s_retbool (res);
      }

     case OP_SET_INPORT: /* set-input-port */
          sc->inport=car(sc->args);
          s_return(sc,sc->value);

     case OP_SET_OUTPORT: /* set-output-port */
          set_outport (sc, car (sc->args));
          s_return(sc,sc->value);

     case OP_RDSEXPR:
          switch (sc->tok) {
          case TOK_EOF:
               s_return(sc,sc->EOF_OBJ);
          /* NOTREACHED */
/*
 * Commented out because we now skip comments in the scanner
 *
          case TOK_COMMENT: {
               gunichar c;
               while ((c=inchar(sc)) != '\n' && c!=EOF)
                    ;
               sc->tok = token(sc);
               s_goto(sc,OP_RDSEXPR);
          }
*/
          case TOK_VEC:
               s_save(sc,OP_RDVEC,sc->NIL,sc->NIL);
               /* fall through */
          case TOK_LPAREN:
               sc->tok = token(sc);
               if (sc->tok == TOK_RPAREN) {
                    s_return(sc,sc->NIL);
               } else if (sc->tok == TOK_DOT) {
                    Error_0(sc,"syntax error: illegal dot expression");
               } else {
                    sc->nesting_stack[sc->file_i]++;
                    s_save(sc,OP_RDLIST, sc->NIL, sc->NIL);
                    s_goto(sc,OP_RDSEXPR);
               }
          case TOK_QUOTE:
               s_save(sc,OP_RDQUOTE, sc->NIL, sc->NIL);
               sc->tok = token(sc);
               s_goto(sc,OP_RDSEXPR);
          case TOK_BQUOTE:
               sc->tok = token(sc);
               if(sc->tok==TOK_VEC) {
                 s_save(sc,OP_RDQQUOTEVEC, sc->NIL, sc->NIL);
                 sc->tok=TOK_LPAREN;
                 s_goto(sc,OP_RDSEXPR);
               } else {
                 s_save(sc,OP_RDQQUOTE, sc->NIL, sc->NIL);
               }
               s_goto(sc,OP_RDSEXPR);
          case TOK_COMMA:
               s_save(sc,OP_RDUNQUOTE, sc->NIL, sc->NIL);
               sc->tok = token(sc);
               s_goto(sc,OP_RDSEXPR);
          case TOK_ATMARK:
               s_save(sc,OP_RDUQTSP, sc->NIL, sc->NIL);
               sc->tok = token(sc);
               s_goto(sc,OP_RDSEXPR);
          case TOK_ATOM:
               s_return(sc,mk_atom(sc, readstr_upto(sc, DELIMITERS)));
          case TOK_DQUOTE:
               x=readstrexp(sc);
               if(x==sc->F) {
                 Error_0(sc,"Error reading string");
               }
               setimmutable(x);
               s_return(sc,x);
          case TOK_USCORE:
               x=readstrexp(sc);
               if(x==sc->F) {
                 Error_0(sc,"Error reading string");
               }
               trans_str = gettext (strvalue (x));
               if (trans_str != strvalue(x)) {
                 sc->free(strvalue(x));
                 strlength(x) = g_utf8_strlen(trans_str, -1);
                 strvalue(x) = store_string(sc, strlength(x), trans_str, 0);
               }
               setimmutable(x);
               s_return(sc,x);
          case TOK_SHARP: {
               pointer f=find_slot_in_env(sc,sc->envir,sc->SHARP_HOOK,1);
               if(f==sc->NIL) {
                    Error_0(sc,"syntax: illegal sharp expression");
               } else {
                    sc->code=cons(sc,slot_value_in_env(f),sc->NIL);
                    s_goto(sc,OP_EVAL);
               }
          }
          case TOK_SHARP_CONST:
               if ((x = mk_sharp_const(sc, readstr_upto(sc, DELIMITERS))) == sc->NIL) {
                    Error_0(sc,"syntax: illegal sharp constant expression");
               } else {
                    s_return(sc,x);
               }
          case TOK_RPAREN:
               Error_1 (sc, "syntax error: unexpected right parenthesis", 0);
          case TOK_DOT:
               Error_1 (sc, "syntax error: unexpected dot", 0);
          default:
               g_warning ("%s: unhandled token", G_STRFUNC);
               break;
          }
          break;

     case OP_RDLIST: {
          sc->args = cons(sc, sc->value, sc->args);
          sc->tok = token(sc);
/* We now skip comments in the scanner
          while (sc->tok == TOK_COMMENT) {
               gunichar c;
               while ((c=inchar(sc)) != '\n' && c!=EOF)
                    ;
               sc->tok = token(sc);
          }
*/
          if (sc->tok == TOK_EOF)
               { Error_0 (sc,"syntax error: expected right paren, found EOF"); }
          else if (sc->tok == TOK_RPAREN) {
               gunichar c = inchar(sc);
               if (c != '\n')
                 backchar(sc,c);
#if SHOW_ERROR_LINE
               else if (sc->load_stack[sc->file_i].kind & port_file)
                  sc->load_stack[sc->file_i].rep.stdio.curr_line++;
#endif
               sc->nesting_stack[sc->file_i]--;
               s_return(sc,reverse_in_place(sc, sc->NIL, sc->args));
          } else if (sc->tok == TOK_DOT) {
               s_save(sc,OP_RDDOT, sc->args, sc->NIL);
               sc->tok = token(sc);
               s_goto(sc,OP_RDSEXPR);
          } else {
               s_save(sc,OP_RDLIST, sc->args, sc->NIL);
               s_goto(sc,OP_RDSEXPR);
          }
     }

     case OP_RDDOT:
          if (token(sc) != TOK_RPAREN) {
               Error_0(sc,"syntax error: illegal dot expression");
          } else {
               sc->nesting_stack[sc->file_i]--;
               s_return(sc,reverse_in_place(sc, sc->value, sc->args));
          }

     case OP_RDQUOTE:
          s_return(sc,cons(sc, sc->QUOTE, cons(sc, sc->value, sc->NIL)));

     case OP_RDQQUOTE:
          s_return(sc,cons(sc, sc->QQUOTE, cons(sc, sc->value, sc->NIL)));

     case OP_RDQQUOTEVEC:
          s_return(sc,cons(sc, mk_symbol(sc,"apply"),
                        cons(sc, mk_symbol(sc,"vector"),
                             cons(sc,cons(sc, sc->QQUOTE,
                                  cons(sc,sc->value,sc->NIL)),
                                  sc->NIL))));

     case OP_RDUNQUOTE:
          s_return(sc,cons(sc, sc->UNQUOTE, cons(sc, sc->value, sc->NIL)));

     case OP_RDUQTSP:
          s_return(sc,cons(sc, sc->UNQUOTESP, cons(sc, sc->value, sc->NIL)));

     case OP_RDVEC:
          /*sc->code=cons(sc,mk_proc(sc,OP_VECTOR),sc->value);
          s_goto(sc,OP_EVAL); Cannot be quoted*/
          /*x=cons(sc,mk_proc(sc,OP_VECTOR),sc->value);
          s_return(sc,x); Cannot be part of pairs*/
          /*sc->code=mk_proc(sc,OP_VECTOR);
          sc->args=sc->value;
          s_goto(sc,OP_APPLY);*/
          sc->args=sc->value;
          s_goto(sc,OP_VECTOR);

     /* ========== printing part ========== */
     case OP_P0LIST:
          if(is_vector(sc->args)) {
               putstr(sc,"#(");
               sc->args=cons(sc,sc->args,mk_integer(sc,0));
               s_goto(sc,OP_PVECFROM);
          } else if(is_environment(sc->args)) {
               putstr(sc,"#<ENVIRONMENT>");
               s_return(sc,sc->T);
          } else if (!is_pair(sc->args)) {
               printatom(sc, sc->args, sc->print_flag);
               s_return(sc,sc->T);
          } else if (car(sc->args) == sc->QUOTE && ok_abbrev(cdr(sc->args))) {
               putstr(sc, "'");
               sc->args = cadr(sc->args);
               s_goto(sc,OP_P0LIST);
          } else if (car(sc->args) == sc->QQUOTE && ok_abbrev(cdr(sc->args))) {
               putstr(sc, "`");
               sc->args = cadr(sc->args);
               s_goto(sc,OP_P0LIST);
          } else if (car(sc->args) == sc->UNQUOTE && ok_abbrev(cdr(sc->args))) {
               putstr(sc, ",");
               sc->args = cadr(sc->args);
               s_goto(sc,OP_P0LIST);
          } else if (car(sc->args) == sc->UNQUOTESP && ok_abbrev(cdr(sc->args))) {
               putstr(sc, ",@");
               sc->args = cadr(sc->args);
               s_goto(sc,OP_P0LIST);
          } else {
               putstr(sc, "(");
               s_save(sc,OP_P1LIST, cdr(sc->args), sc->NIL);
               sc->args = car(sc->args);
               s_goto(sc,OP_P0LIST);
          }

     case OP_P1LIST:
          if (is_pair(sc->args)) {
            s_save(sc,OP_P1LIST, cdr(sc->args), sc->NIL);
            putstr(sc, " ");
            sc->args = car(sc->args);
            s_goto(sc,OP_P0LIST);
          } else if(is_vector(sc->args)) {
            s_save(sc,OP_P1LIST,sc->NIL,sc->NIL);
            putstr(sc, " . ");
            s_goto(sc,OP_P0LIST);
          } else {
            if (sc->args != sc->NIL) {
              putstr(sc, " . ");
              printatom(sc, sc->args, sc->print_flag);
            }
            putstr(sc, ")");
            s_return(sc,sc->T);
          }
     case OP_PVECFROM: {
          int i=ivalue_unchecked(cdr(sc->args));
          pointer vec=car(sc->args);
          int len=ivalue_unchecked(vec);
          if(i==len) {
               putstr(sc,")");
               s_return(sc,sc->T);
          } else {
               pointer elem=vector_elem(vec,i);
               ivalue_unchecked(cdr(sc->args))=i+1;
               s_save(sc,OP_PVECFROM, sc->args, sc->NIL);
               sc->args=elem;
               if (i > 0)
                    putstr(sc," ");
               s_goto(sc,OP_P0LIST);
          }
     }

     default:
          snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
          Error_0(sc,sc->strbuff);

     }
     return sc->T;
}

static pointer opexe_6(scheme *sc, enum scheme_opcodes op) {
     pointer x, y;
     long v;

     switch (op) {
     case OP_LIST_LENGTH:     /* length */   /* a.k */
          v=list_length(sc,car(sc->args));
          if(v<0) {
               Error_1(sc,"length: not a list:",car(sc->args));
          }
          s_return(sc,mk_integer(sc, v));

     case OP_ASSQ:       /* assq */     /* a.k */
          x = car(sc->args);
          for (y = cadr(sc->args); is_pair(y); y = cdr(y)) {
               if (!is_pair(car(y))) {
                    Error_0(sc,"unable to handle non pair element");
               }
               if (x == caar(y))
                    break;
          }
          if (is_pair(y)) {
               s_return(sc,car(y));
          } else {
               s_return(sc,sc->F);
          }


     case OP_GET_CLOSURE:     /* get-closure-code */   /* a.k */
          sc->args = car(sc->args);
          if (sc->args == sc->NIL) {
               s_return(sc,sc->F);
          } else if (is_closure(sc->args)) {
               s_return(sc,cons(sc, sc->LAMBDA, closure_code(sc->value)));
          } else if (is_macro(sc->args)) {
               s_return(sc,cons(sc, sc->LAMBDA, closure_code(sc->value)));
          } else {
               s_return(sc,sc->F);
          }
     case OP_CLOSUREP:        /* closure? */
          /*
           * Note, macro object is also a closure.
           * Therefore, (closure? <#MACRO>) ==> #t
           */
          s_retbool(is_closure(car(sc->args)));
     case OP_MACROP:          /* macro? */
          s_retbool(is_macro(car(sc->args)));
     default:
          snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
          Error_0(sc,sc->strbuff);
     }
     return sc->T; /* NOTREACHED */
}

typedef pointer (*dispatch_func)(scheme *, enum scheme_opcodes);

typedef int (*test_predicate)(pointer);
static int is_any(pointer p) { return 1;}

static int is_nonneg(pointer p) {
  return ivalue(p)>=0 && is_integer(p);
}

/* Correspond carefully with following defines! */
static struct {
  test_predicate fct;
  const char *kind;
} tests[]={
  {0,0}, /* unused */
  {is_any, 0},
  {is_string, "string"},
  {is_symbol, "symbol"},
  {is_port, "port"},
  {is_inport,"input port"},
  {is_outport,"output port"},
  {is_environment, "environment"},
  {is_pair, "pair"},
  {0, "pair or '()"},
  {is_character, "character"},
  {is_vector, "vector"},
  {is_number, "number"},
  {is_integer, "integer"},
  {is_nonneg, "non-negative integer"},
  {is_byte, "byte"}
};

#define TST_NONE 0
#define TST_ANY "\001"
#define TST_STRING "\002"
#define TST_SYMBOL "\003"
#define TST_PORT "\004"
#define TST_INPORT "\005"
#define TST_OUTPORT "\006"
#define TST_ENVIRONMENT "\007"
#define TST_PAIR "\010"
#define TST_LIST "\011"
#define TST_CHAR "\012"
#define TST_VECTOR "\013"
#define TST_NUMBER "\014"
#define TST_INTEGER "\015"
#define TST_NATURAL "\016"
#define TST_BYTE "\017"

typedef struct {
  dispatch_func func;
  char *name;
  int min_arity;
  int max_arity;
  char *arg_tests_encoding;
} op_code_info;

#define INF_ARG 0xffff

static op_code_info dispatch_table[]= {
#define _OP_DEF(A,B,C,D,E,OP) {A,B,C,D,E},
#include "opdefines.h"
  { 0 }
};

static const char *procname(pointer x) {
 int n=procnum(x);
 const char *name=dispatch_table[n].name;
 if(name==0) {
     name="ILLEGAL!";
 }
 return name;
}

/* kernel of this interpreter */
static void Eval_Cycle(scheme *sc, enum scheme_opcodes op) {
  sc->op = op;
  for (;;) {
    op_code_info *pcd=dispatch_table+sc->op;
    if (pcd->name!=0) { /* if built-in function, check arguments */
      char msg[STRBUFFSIZE];
      int ok=1;
      int n=list_length(sc,sc->args);

      /* Check number of arguments */
      if(n<pcd->min_arity) {
        ok=0;
        snprintf(msg, STRBUFFSIZE, "%s: needs%s %d argument(s)",
                 pcd->name,
                 pcd->min_arity==pcd->max_arity?"":" at least",
                 pcd->min_arity);
      }
      if(ok && n>pcd->max_arity) {
        ok=0;
        snprintf(msg, STRBUFFSIZE, "%s: needs%s %d argument(s)",
                 pcd->name,
                 pcd->min_arity==pcd->max_arity?"":" at most",
                 pcd->max_arity);
      }
      if(ok) {
        if(pcd->arg_tests_encoding!=0) {
          int i=0;
          int j;
          const char *t=pcd->arg_tests_encoding;
          pointer arglist=sc->args;
          do {
            pointer arg=car(arglist);
            j=(int)t[0];
        if(j==TST_LIST[0]) {
              if(arg!=sc->NIL && !is_pair(arg)) break;
            } else {
              if(!tests[j].fct(arg)) break;
            }

            if(t[1]!=0) {/* last test is replicated as necessary */
              t++;
            }
            arglist=cdr(arglist);
            i++;
          } while(i<n);
          if(i<n) {
            ok=0;
            snprintf(msg, STRBUFFSIZE, "%s: argument %d must be: %s",
                     pcd->name,
                     i+1,
                     tests[j].kind);
          }
        }
      }
      if(!ok) {
        if(_Error_1(sc,msg,0)==sc->NIL) {
          return;
        }
        pcd=dispatch_table+sc->op;
      }
    }
    ok_to_freely_gc(sc);
    if (pcd->func(sc, (enum scheme_opcodes)sc->op) == sc->NIL) {
      return;
    }
    if(sc->no_memory) {
      fprintf(stderr,"No memory!\n");
      return;
    }
  }
}

/* ========== Initialization of internal keywords ========== */

static void assign_syntax(scheme *sc, char *name) {
     pointer x;

     x = oblist_add_by_name(sc, name);
     typeflag(x) |= T_SYNTAX;
}

static void assign_proc(scheme *sc, enum scheme_opcodes op, char *name) {
     pointer x, y;

     x = mk_symbol(sc, name);
     y = mk_proc(sc,op);
     new_slot_in_env(sc, x, y);
}

static pointer mk_proc(scheme *sc, enum scheme_opcodes op) {
     pointer y;

     y = get_cell(sc, sc->NIL, sc->NIL);
     typeflag(y) = (T_PROC | T_ATOM);
     ivalue_unchecked(y) = (long) op;
     set_num_integer(y);
     return y;
}

/* Hard-coded for the given keywords. Remember to rewrite if more are added! */
static int syntaxnum(pointer p) {
     const char *s=strvalue(car(p));
     switch(strlength(car(p))) {
     case 2:
          if(s[0]=='i') return OP_IF0;        /* if */
          else return OP_OR0;                 /* or */
     case 3:
          if(s[0]=='a') return OP_AND0;      /* and */
          else return OP_LET0;               /* let */
     case 4:
          switch(s[3]) {
          case 'e': return OP_CASE0;         /* case */
          case 'd': return OP_COND0;         /* cond */
          case '*': return OP_LET0AST;       /* let* */
          default: return OP_SET0;           /* set! */
          }
     case 5:
          switch(s[2]) {
          case 'g': return OP_BEGIN;         /* begin */
          case 'l': return OP_DELAY;         /* delay */
          case 'c': return OP_MACRO0;        /* macro */
          default: return OP_QUOTE;          /* quote */
          }
     case 6:
          switch(s[2]) {
          case 'm': return OP_LAMBDA;        /* lambda */
          case 'f': return OP_DEF0;          /* define */
          default: return OP_LET0REC;        /* letrec */
          }
     default:
          return OP_C0STREAM;                /* cons-stream */
     }
}

/* initialization of TinyScheme */
#if USE_INTERFACE
INTERFACE static pointer s_cons(scheme *sc, pointer a, pointer b) {
 return cons(sc,a,b);
}
INTERFACE static pointer s_immutable_cons(scheme *sc, pointer a, pointer b) {
 return immutable_cons(sc,a,b);
}

static struct scheme_interface vtbl ={
  scheme_define,
  s_cons,
  s_immutable_cons,
  reserve_cells,
  mk_integer,
  mk_real,
  mk_symbol,
  gensym,
  mk_string,
  mk_counted_string,
  mk_byte,
  mk_character,
  mk_vector,
  mk_foreign_func,
  mk_closure,
  putstr,
  putcharacter,

  is_string,
  string_length,
  string_value,
  is_number,
  nvalue,
  ivalue,
  rvalue,
  is_integer,
  is_real,
  is_byte,
  is_character,
  bytevalue,
  charvalue,
  is_list,
  is_vector,
  list_length,
  ivalue,
  fill_vector,
  vector_elem,
  set_vector_elem,
  is_port,
  is_pair,
  pair_car,
  pair_cdr,
  set_car,
  set_cdr,

  is_symbol,
  symname,

  is_syntax,
  is_proc,
  is_foreign,
  syntaxname,
  is_closure,
  is_macro,
  closure_code,
  closure_env,

  is_continuation,
  is_promise,
  is_environment,
  is_immutable,
  setimmutable,

  scheme_load_file,
  scheme_load_string
};
#endif

scheme *scheme_init_new(void) {
  scheme *sc=(scheme*)malloc(sizeof(scheme));
  if(!scheme_init(sc)) {
    free(sc);
    return 0;
  } else {
    return sc;
  }
}

scheme *scheme_init_new_custom_alloc(func_alloc malloc, func_dealloc free) {
  scheme *sc=(scheme*)malloc(sizeof(scheme));
  if(!scheme_init_custom_alloc(sc,malloc,free)) {
    free(sc);
    return 0;
  } else {
    return sc;
  }
}


int scheme_init(scheme *sc) {
 return scheme_init_custom_alloc(sc,malloc,free);
}

int scheme_init_custom_alloc(scheme *sc, func_alloc malloc, func_dealloc free) {
  int i, n=sizeof(dispatch_table)/sizeof(dispatch_table[0]);
  pointer x;

  num_zero.is_fixnum=1;
  num_zero.value.ivalue=0;
  num_one.is_fixnum=1;
  num_one.value.ivalue=1;

#if USE_INTERFACE
  sc->vptr=&vtbl;
#endif
  sc->gensym_cnt=0;
  sc->malloc=malloc;
  sc->free=free;
  sc->last_cell_seg = -1;
  sc->sink = &sc->_sink;
  sc->NIL = &sc->_NIL;
  sc->T = &sc->_HASHT;
  sc->F = &sc->_HASHF;
  sc->EOF_OBJ=&sc->_EOF_OBJ;
  sc->free_cell = &sc->_NIL;
  sc->fcells = 0;
  sc->no_memory=0;
  sc->inport=sc->NIL;
  sc->outport=sc->NIL;
  sc->save_inport=sc->NIL;
  sc->loadport=sc->NIL;
  error_port_init (sc);
  sc->nesting=0;
  sc->interactive_repl=0;
  sc->print_output=0;

  if (alloc_cellseg(sc,FIRST_CELLSEGS) != FIRST_CELLSEGS) {
    sc->no_memory=1;
    return 0;
  }
  sc->gc_verbose = 0;
  dump_stack_initialize(sc);
  sc->code = sc->NIL;
  sc->tracing=0;

  /* init sc->NIL */
  typeflag(sc->NIL) = (T_ATOM | MARK);
  car(sc->NIL) = cdr(sc->NIL) = sc->NIL;
  /* init T */
  typeflag(sc->T) = (T_ATOM | MARK);
  car(sc->T) = cdr(sc->T) = sc->T;
  /* init F */
  typeflag(sc->F) = (T_ATOM | MARK);
  car(sc->F) = cdr(sc->F) = sc->F;
  /* init sink */
  typeflag(sc->sink) = (T_PAIR | MARK);
  car(sc->sink) = sc->NIL;
  /* init c_nest */
  sc->c_nest = sc->NIL;

  sc->oblist = oblist_initial_value(sc);
  /* init global_env */
  new_frame_in_env(sc, sc->NIL);
  sc->global_env = sc->envir;
  /* init else */
  x = mk_symbol(sc,"else");
  new_slot_in_env(sc, x, sc->T);

  assign_syntax(sc, "lambda");
  assign_syntax(sc, "quote");
  assign_syntax(sc, "define");
  assign_syntax(sc, "if");
  assign_syntax(sc, "begin");
  assign_syntax(sc, "set!");
  assign_syntax(sc, "let");
  assign_syntax(sc, "let*");
  assign_syntax(sc, "letrec");
  assign_syntax(sc, "cond");
  assign_syntax(sc, "delay");
  assign_syntax(sc, "and");
  assign_syntax(sc, "or");
  assign_syntax(sc, "cons-stream");
  assign_syntax(sc, "macro");
  assign_syntax(sc, "case");

  for(i=0; i<n; i++) {
    if(dispatch_table[i].name!=0) {
      assign_proc(sc, (enum scheme_opcodes)i, dispatch_table[i].name);
    }
  }

  /* initialization of global pointers to special symbols */
  sc->LAMBDA = mk_symbol(sc, "lambda");
  sc->QUOTE = mk_symbol(sc, "quote");
  sc->QQUOTE = mk_symbol(sc, "quasiquote");
  sc->UNQUOTE = mk_symbol(sc, "unquote");
  sc->UNQUOTESP = mk_symbol(sc, "unquote-splicing");
  sc->FEED_TO = mk_symbol(sc, "=>");
  sc->COLON_HOOK = mk_symbol(sc,"*colon-hook*");
  sc->ERROR_HOOK = mk_symbol(sc, "*error-hook*");
  sc->SHARP_HOOK = mk_symbol(sc, "*sharp-hook*");
  sc->COMPILE_HOOK = mk_symbol(sc, "*compile-hook*");

  return !sc->no_memory;
}

SCHEME_EXPORT void scheme_set_input_port_file(scheme *sc, FILE *fin) {
  sc->inport=port_from_file(sc,fin,port_input);
}

SCHEME_EXPORT void scheme_set_output_port_file(scheme *sc, FILE *fout) {
  sc->outport=port_from_file(sc,fout,port_output);
}

void scheme_set_external_data(scheme *sc, void *p) {
 sc->ext_data=p;
}

void scheme_deinit(scheme *sc) {
  int i;

#if SHOW_ERROR_LINE
  char *fname;
#endif

  sc->oblist=sc->NIL;
  sc->global_env=sc->NIL;
  dump_stack_free(sc);
  sc->envir=sc->NIL;
  sc->code=sc->NIL;
  sc->args=sc->NIL;
  sc->value=sc->NIL;
  if(is_port(sc->inport)) {
    typeflag(sc->inport) = T_ATOM;
  }
  sc->inport=sc->NIL;
  sc->outport=sc->NIL;
  if(is_port(sc->save_inport)) {
    typeflag(sc->save_inport) = T_ATOM;
  }
  sc->save_inport=sc->NIL;
  if(is_port(sc->loadport)) {
    typeflag(sc->loadport) = T_ATOM;
  }
  sc->loadport=sc->NIL;
  error_port_init (sc);
  sc->gc_verbose=0;
  gc(sc,sc->NIL,sc->NIL);

  for(i=0; i<=sc->last_cell_seg; i++) {
    sc->free(sc->alloc_seg[i]);
  }

#if SHOW_ERROR_LINE
  for(i=0; i<sc->file_i; i++) {
    if (sc->load_stack[sc->file_i].kind & port_file) {
      fname = sc->load_stack[i].rep.stdio.filename;
      if(fname)
        sc->free(fname);
    }
  }
#endif
}

void scheme_load_file(scheme *sc, FILE *fin)
{ scheme_load_named_file(sc,fin,0); }

void scheme_load_named_file(scheme *sc, FILE *fin, const char *filename) {
  if (fin == NULL)
  {
    fprintf(stderr,"File pointer can not be NULL when loading a file\n");
    return;
  }
  dump_stack_reset(sc);
  sc->envir = sc->global_env;
  sc->file_i=0;
  sc->load_stack[0].kind=port_input|port_file;
  sc->load_stack[0].rep.stdio.file=fin;
  sc->loadport=mk_port(sc,sc->load_stack);
  sc->retcode=0;
  if(fin==stdin) {
    sc->interactive_repl=1;
  }

#if SHOW_ERROR_LINE
  sc->load_stack[0].rep.stdio.curr_line = 0;
  if(fin!=stdin && filename)
    sc->load_stack[0].rep.stdio.filename = store_string(sc, strlen(filename), filename, 0);
  else
    sc->load_stack[0].rep.stdio.filename = NULL;
#endif

  sc->inport=sc->loadport;
  sc->args = mk_integer(sc,sc->file_i);
  Eval_Cycle(sc, OP_T0LVL);
  typeflag(sc->loadport)=T_ATOM;
  if(sc->retcode==0) {
    sc->retcode=sc->nesting!=0;
  }
}

void scheme_load_string(scheme *sc, const char *cmd) {
  dump_stack_reset(sc);
  sc->envir = sc->global_env;
  sc->file_i=0;
  string_port_init_static_port (sc->load_stack, cmd);
  sc->loadport=mk_port(sc,sc->load_stack);
  sc->retcode=0;
  sc->interactive_repl=0;
  sc->inport=sc->loadport;
  sc->args = mk_integer(sc,sc->file_i);
  Eval_Cycle(sc, OP_T0LVL);
  typeflag(sc->loadport)=T_ATOM;
  if(sc->retcode==0) {
    sc->retcode=sc->nesting!=0;
  }
}

void scheme_define(scheme *sc, pointer envir, pointer symbol, pointer value) {
     pointer x;

     x=find_slot_in_env(sc,envir,symbol,0);
     if (x != sc->NIL) {
          set_slot_in_env(sc, x, value);
     } else {
          new_slot_spec_in_env(sc, envir, symbol, value);
     }
}

#if !STANDALONE
void scheme_register_foreign_func(scheme * sc, scheme_registerable * sr)
{
  scheme_define(sc,
               sc->global_env,
               mk_symbol(sc,sr->name),
               mk_foreign_func(sc, sr->f));
}

void scheme_register_foreign_func_list(scheme * sc,
                                      scheme_registerable * list,
                                      int count)
{
  int i;
  for(i = 0; i < count; i++)
    {
      scheme_register_foreign_func(sc, list + i);
    }
}

pointer scheme_apply0(scheme *sc, const char *procname)
{ return scheme_eval(sc, cons(sc,mk_symbol(sc,procname),sc->NIL)); }

static void save_from_C_call(scheme *sc)
{
  pointer saved_data =
    cons(sc,
        car(sc->sink),
        cons(sc,
             sc->envir,
             sc->dump));
  /* Push */
  sc->c_nest = cons(sc, saved_data, sc->c_nest);
  /* Truncate the dump stack so TS will return here when done, not
     directly resume pre-C-call operations. */
  dump_stack_reset(sc);
}

static void restore_from_C_call(scheme *sc)
{
  car(sc->sink) = caar(sc->c_nest);
  sc->envir = cadar(sc->c_nest);
  sc->dump = cdr(cdar(sc->c_nest));
  /* Pop */
  sc->c_nest = cdr(sc->c_nest);
}

/* "func" and "args" are assumed to be already eval'ed. */
pointer scheme_call(scheme *sc, pointer func, pointer args)
{
  int old_repl = sc->interactive_repl;
  sc->interactive_repl = 0;
  save_from_C_call(sc);
  sc->envir = sc->global_env;
  sc->args = args;
  sc->code = func;
  sc->retcode = 0;
  Eval_Cycle(sc, OP_APPLY);
  sc->interactive_repl = old_repl;
  restore_from_C_call(sc);
  return sc->value;
}

pointer scheme_eval(scheme *sc, pointer obj)
{
  int old_repl = sc->interactive_repl;
  sc->interactive_repl = 0;
  save_from_C_call(sc);
  sc->args = sc->NIL;
  sc->code = obj;
  sc->retcode = 0;
  Eval_Cycle(sc, OP_EVAL);
  sc->interactive_repl = old_repl;
  restore_from_C_call(sc);
  return sc->value;
}


#endif

/* ========== Main ========== */

#if STANDALONE

#if defined(__APPLE__) && !defined (OSX)
int main(int argc, char **argv)
{
     extern MacTS_main(int argc, char **argv);
     char**    argv;
     int argc = ccommand(&argv);
     MacTS_main(argc,argv);
     return 0;
}
int MacTS_main(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif
  scheme sc;
  FILE *fin = NULL;
  char *file_name=InitFile;
  int retcode;
  int isfile=1;

  if(argc==1) {
    printf(banner);
  }
  if(argc==2 && strcmp(argv[1],"-?")==0) {
    printf("Usage: tinyscheme -?\n");
    printf("or:    tinyscheme [<file1> <file2> ...]\n");
    printf("followed by\n");
    printf("          -1 <file> [<arg1> <arg2> ...]\n");
    printf("          -c <Scheme commands> [<arg1> <arg2> ...]\n");
    printf("assuming that the executable is named tinyscheme.\n");
    printf("Use - as filename for stdin.\n");
    return 1;
  }
  if(!scheme_init(&sc)) {
    fprintf(stderr,"Could not initialize!\n");
    return 2;
  }
  scheme_set_input_port_file(&sc, stdin);
  scheme_set_output_port_file(&sc, stdout);
#if USE_DL
  scheme_define(&sc,sc.global_env,mk_symbol(&sc,"load-extension"),mk_foreign_func(&sc, scm_load_ext));
#endif
  argv++;
  if(g_access(file_name,0)!=0) {
    char *p=getenv("TINYSCHEMEINIT");
    if(p!=0) {
      file_name=p;
    }
  }
  do {
    if(strcmp(file_name,"-")==0) {
      fin=stdin;
    } else if(strcmp(file_name,"-1")==0 || strcmp(file_name,"-c")==0) {
      pointer args=sc.NIL;
      isfile=file_name[1]=='1';
      file_name=*argv++;
      if(strcmp(file_name,"-")==0) {
        fin=stdin;
      } else if(isfile) {
        fin=g_fopen(file_name,"r");
      }
      for(;*argv;argv++) {
        pointer value=mk_string(&sc,*argv);
        args=cons(&sc,value,args);
      }
      args=reverse_in_place(&sc,sc.NIL,args);
      scheme_define(&sc,sc.global_env,mk_symbol(&sc,"*args*"),args);

    } else {
      fin=g_fopen(file_name,"r");
    }
    if(isfile && fin==0) {
      fprintf(stderr,"Could not open file %s\n",file_name);
    } else {
      if(isfile) {
        scheme_load_named_file(&sc,fin,file_name);
      } else {
        scheme_load_string(&sc,file_name);
      }
      if(!isfile || fin!=stdin) {
        if(sc.retcode!=0) {
          fprintf(stderr,"Errors encountered reading %s\n",file_name);
        }
        if(isfile) {
          fclose(fin);
        }
      }
    }
    file_name=*argv++;
  } while(file_name!=0);
  if(argc==1) {
    scheme_load_named_file(&sc,stdin,0);
  }
  retcode=sc.retcode;
  scheme_deinit(&sc);

  return retcode;
}

#endif

/*
Local variables:
c-file-style: "k&r"
End:
*/
