/* T I N Y S C H E M E    1 . 3 8
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
/* the routines in this file, or when adding new features.          */
/*                                                                  */
/* UTF-8 modifications made by Kevin Cozens (kcozens@interlog.com)  */
/* **************************************************************** */

#include "config.h"

#define _SCHEME_SOURCE
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef WIN32
# include <io.h>
# define access(f,a) _access(f,a)
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
#include <string.h>

#include <libintl.h>

#include "scheme-private.h"

#if !STANDALONE
static ts_output_func   ts_output_handler = NULL;
static gpointer         ts_output_data = NULL;

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

# define BACKQUOTE '`'

/*
 *  Basic memory allocation units
 */

#define banner "TinyScheme 1.38 (with UTF-8 support)"

#include <string.h>
#include <stdlib.h>

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
# define prompt "> "
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
  T_LAST_SYSTEM_TYPE=14
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

static num num_zero;
static num num_one;

/* macros for cell operations */
#define typeflag(p)      ((p)->_flag)
#define type(p)          (typeflag(p)&T_MASKTYPE)

INTERFACE INLINE int is_string(pointer p)     { return (type(p)==T_STRING); }
#define strvalue(p)      ((p)->_object._string._svalue)
#define strlength(p)     ((p)->_object._string._length)

INTERFACE static int is_list(scheme *sc, pointer p);
INTERFACE INLINE int is_vector(pointer p)    { return (type(p)==T_VECTOR); }
INTERFACE static void fill_vector(pointer vec, pointer obj);
INTERFACE static pointer vector_elem(pointer vec, int ielem);
INTERFACE static pointer set_vector_elem(pointer vec, int ielem, pointer a);
INTERFACE INLINE int is_number(pointer p)    { return (type(p)==T_NUMBER); }
INTERFACE INLINE int is_integer(pointer p) {
  return ((p)->_object._number.is_fixnum);
}
INTERFACE INLINE int is_real(pointer p) {
  return (!(p)->_object._number.is_fixnum);
}

INTERFACE INLINE int is_character(pointer p) { return (type(p)==T_CHARACTER); }
INTERFACE INLINE int string_length(pointer p) { return strlength(p); }
INTERFACE INLINE char *string_value(pointer p) { return strvalue(p); }
INLINE num nvalue(pointer p)       { return ((p)->_object._number); }
INTERFACE long ivalue(pointer p)      { return (is_integer(p)?(p)->_object._number.value.ivalue:(long)(p)->_object._number.value.rvalue); }
INTERFACE double rvalue(pointer p)    { return (!is_integer(p)?(p)->_object._number.value.rvalue:(double)(p)->_object._number.value.ivalue); }
#define ivalue_unchecked(p)       ((p)->_object._number.value.ivalue)
#define rvalue_unchecked(p)       ((p)->_object._number.value.rvalue)
#define set_integer(p)   (p)->_object._number.is_fixnum=1;
#define set_real(p)      (p)->_object._number.is_fixnum=0;
INTERFACE  gunichar charvalue(pointer p)  { return (gunichar)ivalue_unchecked(p); }

INTERFACE INLINE int is_port(pointer p)     { return (type(p)==T_PORT); }
#define is_inport(p) (type(p)==T_PORT && p->_object._port->kind&port_input)
#define is_outport(p) (type(p)==T_PORT && p->_object._port->kind&port_output)

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

static const char utf8_length[128] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x80-0x8f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x90-0x9f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xa0-0xaf */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xb0-0xbf */
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
static pointer mk_empty_string(scheme *sc, int len, gunichar fill);
static char *store_string(scheme *sc, int len, const char *str, gunichar fill);
static pointer mk_vector(scheme *sc, int len);
static pointer mk_atom(scheme *sc, char *q);
static pointer mk_sharp_const(scheme *sc, char *name);
static pointer mk_port(scheme *sc, port *p);
static pointer port_from_filename(scheme *sc, const char *fn, int prop);
static pointer port_from_file(scheme *sc, FILE *, int prop);
static pointer port_from_string(scheme *sc, char *start, char *past_the_end, int prop);
static port *port_rep_from_filename(scheme *sc, const char *fn, int prop);
static port *port_rep_from_file(scheme *sc, FILE *, int prop);
static port *port_rep_from_string(scheme *sc, char *start, char *past_the_end, int prop);
static void port_close(scheme *sc, pointer p, int flag);
static void mark(pointer a);
static void gc(scheme *sc, pointer a, pointer b);
static gunichar inchar(scheme *sc);
static void backchar(scheme *sc, gunichar c);
static char *readstr_upto(scheme *sc, char *delim);
static pointer readstrexp(scheme *sc);
static INLINE void skipspace(scheme *sc);
static int token(scheme *sc);
static void printslashstring(scheme *sc, char *s, int len);
static void atom2str(scheme *sc, pointer l, int f, char **pp, int *plen);
static void printatom(scheme *sc, pointer l, int f);
static pointer mk_proc(scheme *sc, enum scheme_opcodes op);
static pointer mk_closure(scheme *sc, pointer c, pointer e);
static pointer mk_continuation(scheme *sc, pointer d);
static pointer reverse(scheme *sc, pointer a);
static pointer reverse_in_place(scheme *sc, pointer term, pointer list);
static pointer append(scheme *sc, pointer a, pointer b);
static int list_length(scheme *sc, pointer a);
static int eqv(pointer a, pointer b);
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
#if !STANDALONE
void scheme_call(scheme *sc, pointer func, pointer args);
#endif

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
 /* modulo should have same sign as second operand */
 if (res > 0) {
     if (e1 < 0) {
        res -= labs(e2);
     }
 } else if (res < 0) {
     if (e1 > 0) {
        res += labs(e2);
     }
 }
 ret.value.ivalue=res;
 return ret;
}

static num num_mod(num a, num b) {
 num ret;
 long e1, e2, res;
 ret.is_fixnum=a.is_fixnum && b.is_fixnum;
 e1=num_ivalue(a);
 e2=num_ivalue(b);
 res=e1%e2;
 if(res*e2<0) {    /* modulo should have same sign as second operand */
     e2=labs(e2);
     if(res>0) {
          res-=e2;
     } else {
          res+=e2;
     }
 }
 ret.value.ivalue=res;
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
          if (((unsigned long) cp) % adj != 0) {
            cp=(char*)(adj*((unsigned long)cp/adj+1));
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

static INLINE pointer get_cell(scheme *sc, pointer a, pointer b) {
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
    gc(sc,a, b);
    if (sc->fcells < sc->last_cell_seg*8
        || sc->free_cell == sc->NIL) {
      /* if only a few recovered, get more to avoid fruitless gc's */
      if (!alloc_cellseg(sc,1) && sc->free_cell == sc->NIL) {
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
                               sc->no_memory=1;
                               return sc->NIL;
                       }
               }
               if (sc->fcells < n) {
                       /* If all fail, report failure */
                       sc->no_memory=1;
                       return sc->NIL;
               }
       }
       return (sc->T);
}

static pointer get_consecutive_cells(scheme *sc, int n) {
  pointer x;

  if(sc->no_memory) {
    return sc->sink;
  }

  /* Are there any cells available? */
  x=find_consecutive_cells(sc,n);
  if (x == sc->NIL) {
    /* If not, try gc'ing some */
    gc(sc, sc->NIL, sc->NIL);
    x=find_consecutive_cells(sc,n);
    if (x == sc->NIL) {
      /* If there still aren't, try getting more heap */
      if (!alloc_cellseg(sc,1)) {
        sc->no_memory=1;
        return sc->sink;
      }
    }
    x=find_consecutive_cells(sc,n);
    if (x == sc->NIL) {
      /* If all fail, report failure */
      sc->no_memory=1;
      return sc->sink;
    }
  }
  return (x);
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

static pointer mk_port(scheme *sc, port *p) {
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

INTERFACE pointer mk_character(scheme *sc, gunichar c) {
  pointer x = get_cell(sc,sc->NIL, sc->NIL);

  typeflag(x) = (T_CHARACTER | T_ATOM);
  ivalue_unchecked(x)= c;
  set_integer(x);
  return (x);
}

/* get number atom (integer) */
INTERFACE pointer mk_integer(scheme *sc, long num) {
  pointer x = get_cell(sc,sc->NIL, sc->NIL);

  typeflag(x) = (T_NUMBER | T_ATOM);
  ivalue_unchecked(x)= num;
  set_integer(x);
  return (x);
}

INTERFACE pointer mk_real(scheme *sc, double n) {
  pointer x = get_cell(sc,sc->NIL, sc->NIL);

  typeflag(x) = (T_NUMBER | T_ATOM);
  rvalue_unchecked(x)= n;
  set_real(x);
  return (x);
}

static pointer mk_number(scheme *sc, num n) {
 if(n.is_fixnum) {
     return mk_integer(sc,n.value.ivalue);
 } else {
     return mk_real(sc,n.value.rvalue);
 }
}

void set_safe_foreign (scheme *sc, pointer data) {
  if (sc->safe_foreign == sc->NIL) {
    fprintf (stderr, "get_safe_foreign called outside a foreign function\n");
  } else {
    car (sc->safe_foreign) = data;
  }
}


/* char_cnt is length of string in chars. */
/* str points to a NUL terminated string. */
/* Only uses fill_char if str is NULL.    */
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
     }
     else {
       len = g_unichar_to_utf8(fill, utf8);
       q=(gchar*)sc->malloc(char_cnt*len+1);
     }
     if(q==0) {
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

/* len is the length of str in characters */
INTERFACE pointer mk_counted_string(scheme *sc, const char *str, int len) {
     pointer x = get_cell(sc, sc->NIL, sc->NIL);

     strvalue(x) = store_string(sc,len,str,0);
     typeflag(x) = (T_STRING | T_ATOM);
     strlength(x) = len;
     return (x);
}

static pointer mk_empty_string(scheme *sc, int len, gunichar fill) {
     pointer x = get_cell(sc, sc->NIL, sc->NIL);

     strvalue(x) = store_string(sc,len,0,fill);
     typeflag(x) = (T_STRING | T_ATOM);
     strlength(x) = len;
     return (x);
}

INTERFACE static pointer mk_vector(scheme *sc, int len) {
     pointer x=get_consecutive_cells(sc,len/2+len%2+1);
     typeflag(x) = (T_VECTOR | T_ATOM);
     ivalue_unchecked(x)=len;
     set_integer(x);
     fill_vector(x,sc->NIL);
     return x;
}

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
          sprintf(name,"gensym-%ld",sc->gensym_cnt);

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

/* make constant */
static pointer mk_sharp_const(scheme *sc, char *name) {
     long    x;
     char    tmp[256];

     if (!strcmp(name, "t"))
          return (sc->T);
     else if (!strcmp(name, "f"))
          return (sc->F);
     else if (*name == 'o') {/* #o (octal) */
          sprintf(tmp, "0%s", name+1);
          sscanf(tmp, "%lo", &x);
          return (mk_integer(sc, x));
     } else if (*name == 'd') {    /* #d (decimal) */
          sscanf(name+1, "%ld", &x);
          return (mk_integer(sc, x));
     } else if (*name == 'x') {    /* #x (hex) */
          sprintf(tmp, "0x%s", name+1);
          sscanf(tmp, "%lx", &x);
          return (mk_integer(sc, x));
     } else if (*name == 'b') {    /* #b (binary) */
          x = binary_decode(name+1);
          return (mk_integer(sc, x));
     } else if (*name == '\\') { /* #\w (character) */
          gunichar c=0;
          if(stricmp(name+1,"space")==0) {
               c=' ';
          } else if(stricmp(name+1,"newline")==0) {
               c='\n';
          } else if(stricmp(name+1,"return")==0) {
               c='\r';
          } else if(stricmp(name+1,"tab")==0) {
               c='\t';
     } else if(name[1]=='x' && name[2]!=0) {
          int c1=0;
          if(sscanf(name+2,"%x",&c1)==1 && c1<256) {
               c=c1;
          } else {
               return sc->NIL;
     }
#if USE_ASCII_NAMES
          } else if(is_ascii_name(name+1,&c)) {
               /* nothing */
#endif
          } else if(name[2]==0) {
               c=name[1];
          } else {
               return sc->NIL;
          }
          return mk_character(sc,c);
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
  mark(sc->safe_foreign);
  mark(sc->inport);
  mark(sc->save_inport);
  mark(sc->outport);
  mark(sc->loadport);

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
    sprintf(msg,"done: %ld cells were recovered.\n", sc->fcells);
    putstr(sc,msg);
  }
}

static void finalize_cell(scheme *sc, pointer a) {
  if(is_string(a)) {
    sc->free(strvalue(a));
  } else if(is_port(a)) {
    if(a->_object._port->kind&port_file
       && a->_object._port->rep.stdio.closeit) {
      port_close(sc,a,port_input|port_output);
    }
    sc->free(a->_object._port);
  }
}

/* ========== Routines for Reading ========== */

static int file_push(scheme *sc, const char *fname) {
  FILE *fin=fopen(fname,"rb");
  if(fin!=0) {
    sc->file_i++;
    sc->load_stack[sc->file_i].kind=port_file|port_input;
    sc->load_stack[sc->file_i].rep.stdio.file=fin;
    sc->load_stack[sc->file_i].rep.stdio.closeit=1;
    sc->nesting_stack[sc->file_i]=0;
    sc->loadport->_object._port=sc->load_stack+sc->file_i;
  }
  return fin!=0;
}

static void file_pop(scheme *sc) {
 sc->nesting=sc->nesting_stack[sc->file_i];
 if(sc->file_i!=0) {
   port_close(sc,sc->loadport,port_input);
   sc->file_i--;
   sc->loadport->_object._port=sc->load_stack+sc->file_i;
   if(file_interactive(sc)) {
     putstr(sc,prompt);
   }
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
  f=fopen(fn,rw);
  if(f==0) {
    return 0;
  }
  pt=port_rep_from_file(sc,f,prop);
  pt->rep.stdio.closeit=1;
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

static port *port_rep_from_file(scheme *sc, FILE *f, int prop) {
  char *rw;
  port *pt;
  pt=(port*)sc->malloc(sizeof(port));
  if(pt==0) {
    return 0;
  }
  if(prop==(port_input|port_output)) {
    rw="a+";
  } else if(prop==port_output) {
    rw="w";
  } else {
    rw="r";
  }
  pt->kind=port_file|prop;
  pt->rep.stdio.file=f;
  pt->rep.stdio.closeit=0;
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

static port *port_rep_from_string(scheme *sc, char *start, char *past_the_end, int prop) {
  port *pt;
  pt=(port*)sc->malloc(sizeof(port));
  if(pt==0) {
    return 0;
  }
  pt->kind=port_string|prop;
  pt->rep.string.start=start;
  pt->rep.string.curr=start;
  pt->rep.string.past_the_end=past_the_end;
  return pt;
}

static pointer port_from_string(scheme *sc, char *start, char *past_the_end, int prop) {
  port *pt;
  pt=port_rep_from_string(sc,start,past_the_end,prop);
  if(pt==0) {
    return sc->NIL;
  }
  return mk_port(sc,pt);
}

static void port_close(scheme *sc, pointer p, int flag) {
  port *pt=p->_object._port;
  pt->kind&=~flag;
  if((pt->kind & (port_input|port_output))==0) {
    if(pt->kind&port_file) {
      fclose(pt->rep.stdio.file);
    }
    pt->kind=port_free;
  }
}

static gunichar basic_inchar(port *pt) {
  int  len;

  if(pt->kind&port_file) {
    char utf8[7];
    char *s;
    int  i;

    utf8[0] = fgetc(pt->rep.stdio.file);
    if (utf8[0] & 0x80)
    {
       len = utf8_length[ utf8[0]&0x7F ];
       s = &utf8[1];
       for (i = 0; i < len; ++i)
         *s++ = fgetc(pt->rep.stdio.file);
       /* FIXME: Check for bad character and search for next good char. */
       return g_utf8_get_char_validated(utf8, len+1);
    }
    return (gunichar)utf8[0];
  } else {
    if(*pt->rep.string.curr==0
       || pt->rep.string.curr==pt->rep.string.past_the_end) {
      return EOF;
    } else {
      gunichar c;

      len = pt->rep.string.past_the_end - pt->rep.string.curr;
      c = g_utf8_get_char_validated(pt->rep.string.curr, len);

      if (c < 0)
      {
        pt->rep.string.curr = g_utf8_find_next_char(pt->rep.string.curr,
                                                    pt->rep.string.past_the_end);
        if (pt->rep.string.curr == NULL)
            pt->rep.string.curr = pt->rep.string.past_the_end;
        c = ' ';
      }
      else
      {
        len = g_unichar_to_utf8(c, NULL);
        pt->rep.string.curr += len;
      }

      return c;
    }
  }
}

/* get new character from input file */
static gunichar inchar(scheme *sc) {
  gunichar c;
  port *pt;
 again:
  pt=sc->inport->_object._port;
  if(pt->kind&port_file)
  {
    if (sc->bc_flag)
    {
      sc->bc_flag = 0;
      c = sc->backchar;
    }
    else
      c=basic_inchar(pt);
  }
  else
    c=basic_inchar(pt);
  if(c==EOF && sc->inport==sc->loadport && sc->file_i!=0) {
    file_pop(sc);
    if(sc->nesting!=0) {
      return EOF;
    } else {
      return '\n';
    }
    goto again;
  }
  return c;
}

/* back character to input buffer */
static void backchar(scheme *sc, gunichar c) {
  port *pt;
  gint  charlen;

  if(c==EOF) return;
  charlen = g_unichar_to_utf8(c, NULL);
  pt=sc->inport->_object._port;
  if(pt->kind&port_file) {
      sc->backchar = c;
      sc->bc_flag = 1;
  } else {
    if(pt->rep.string.curr!=pt->rep.string.start) {
      if(pt->rep.string.curr-pt->rep.string.start >= charlen)
        pt->rep.string.curr -= charlen;
      else
        pt->rep.string.curr = pt->rep.string.start;
    }
  }
}

/* len is number of UTF-8 characters in string pointed to by chars */
static void putchars(scheme *sc, const char *chars, int char_cnt) {
  int   free_bytes;     /* Space remaining in buffer (in bytes) */
  int   l;
  port *pt=sc->outport->_object._port;

  if (char_cnt <= 0)
      return;

  /* Get length of 'chars' in bytes */
  char_cnt = g_utf8_offset_to_pointer(chars, (long)char_cnt) - chars;

  if(pt->kind&port_file) {
#if STANDALONE
      fwrite(chars,1,char_cnt,pt->rep.stdio.file);
      fflush(pt->rep.stdio.file);
#else
      /* If output is still directed to stdout (the default) it should be    */
      /* safe to redirect it to the registered output routine. */
      if (pt->rep.stdio.file == stdout)
           ts_output_string (TS_OUTPUT_NORMAL, chars, char_cnt);
      else {
        fwrite(chars,1,char_cnt,pt->rep.stdio.file);
        fflush(pt->rep.stdio.file);
      }
#endif
  } else {
    free_bytes = pt->rep.string.past_the_end - pt->rep.string.curr;
    if (free_bytes > 0)
    {
       l = min(char_cnt, free_bytes);
       memcpy(pt->rep.string.curr, chars, l);
       pt->rep.string.curr += l;
    }
  }
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
  int  len = 0;

#if 0
  while (!is_one_of(delim, (*p++ = inchar(sc))))
      ;
  if(p==sc->strbuff+2 && p[-2]=='\\') {
    *p=0;
  } else {
    backchar(sc,p[-1]);
    *--p = '\0';
  }
#else
  do {
    c_prev = c;
    c = inchar(sc);
    len = g_unichar_to_utf8(c, p);
    p += len;
  } while (c && !is_one_of(delim, c));

  if(p==sc->strbuff+2 && c_prev=='\\')
    *p = '\0';
  else
  {
    backchar(sc,c);    /* put back the delimiter */
    p[-len] = '\0';
  }
#endif
  return sc->strbuff;
}

/* read string expression "xxx...xxx" */
static pointer readstrexp(scheme *sc) {
  char *p = sc->strbuff;
  gunichar c;
  int c1=0;
  int len;
  enum { st_ok, st_bsl, st_x1, st_x2, st_oct1, st_oct2, st_oct3 } state=st_ok;

  for (;;) {
    c=inchar(sc);
    if(c==EOF || p-sc->strbuff>sizeof(sc->strbuff)-1) {
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
        return mk_counted_string(sc,sc->strbuff,p-sc->strbuff);
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
    case st_oct1:
    case st_oct2:
    case st_oct3:
      if (!g_unichar_isdigit(c) || g_unichar_digit_value(c) > 7)
      {
        if (state==st_oct1)
           return sc->F;

        *p++=c1;
        backchar(sc, c);
        state=st_ok;
      }
      else
      {
        c1=(c1<<3)+g_unichar_digit_value(c);
        switch (state)
        {
        case st_oct1:
          state=st_oct2;
          break;
        case st_oct2:
          state=st_oct3;
          break;
        default:
          *p++=c1;
          state=st_ok;
          break;
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
static INLINE void skipspace(scheme *sc) {
     gunichar c;
     while (g_unichar_isspace(c=inchar(sc)))
          ;
     if(c!=EOF) {
          backchar(sc,c);
     }
}

/* get token */
static int token(scheme *sc) {
     gunichar c;
     skipspace(sc);
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
          return (token(sc));
     case '"':
          return (TOK_DQUOTE);
     case '_':
          if ((c=inchar(sc)) == '"')
               return (TOK_DQUOTE);
          backchar(sc,c);
          return (TOK_ATOM);
     case BACKQUOTE:
          return (TOK_BQUOTE);
     case ',':
          if ((c=inchar(sc)) == '@')
               return (TOK_ATMARK);
          else {
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
               return (token(sc));
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
  putchars(sc,p,len);
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
          p = sc->strbuff;
          strcpy(p, "#<PORT>");
     } else if (is_number(l)) {
          p = sc->strbuff;
          if(is_integer(l)) {
               sprintf(p, "%ld", ivalue_unchecked(l));
          } else {
               g_ascii_formatd (p, sizeof (sc->strbuff), "%.10g",
                                rvalue_unchecked(l));
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
     } else if (is_character(l)) {
          gunichar c=charvalue(l);
          p = sc->strbuff;
          if (!f) {
               int len = g_unichar_to_utf8(c, p);
               p[len]=0;
          } else {
               switch(c) {
               case ' ':
                    sprintf(p,"#\\space"); break;
               case '\n':
                    sprintf(p,"#\\newline"); break;
               case '\r':
                    sprintf(p,"#\\return"); break;
               case '\t':
                    sprintf(p,"#\\tab"); break;
               default:
#if USE_ASCII_NAMES
                    if(c==127) {
                         strcpy(p,"#\\del"); break;
                    } else if(c<32) {
                         strcpy(p,"#\\"); strcat(p,charnames[c]); break;
                    }
#else
                    if(c<32) {
                      sprintf(p,"#\\x%x",c); break;
                    }
#endif
                    sprintf(p,"#\\%c",c); break;
               }
          }
     } else if (is_symbol(l)) {
          p = symname(l);
     } else if (is_proc(l)) {
          p = sc->strbuff;
          sprintf(p, "#<%s PROCEDURE %ld>", procname(l),procnum(l));
     } else if (is_macro(l)) {
          p = "#<MACRO>";
     } else if (is_closure(l)) {
          p = "#<CLOSURE>";
     } else if (is_promise(l)) {
          p = "#<PROMISE>";
     } else if (is_foreign(l)) {
          p = sc->strbuff;
          sprintf(p, "#<FOREIGN PROCEDURE %ld>", procnum(l));
     } else if (is_continuation(l)) {
          p = "#<CONTINUATION>";
     } else {
          p = "#<ERROR>";
     }
     *pp=p;
     *plen=g_utf8_strlen(p, -1);
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
static pointer append(scheme *sc, pointer a, pointer b) {
     pointer p = b, q;

     if (a != sc->NIL) {
          a = reverse(sc, a);
          while (a != sc->NIL) {
               q = cdr(a);
               cdr(a) = p;
               p = a;
               a = q;
          }
     }
     return (p);
}

/* equivalence of atoms */
static int eqv(pointer a, pointer b) {
     if (is_string(a)) {
          if (is_string(b))
               return (strvalue(a) == strvalue(b));
          else
               return (0);
     } else if (is_number(a)) {
          if (is_number(b))
               return num_eq(nvalue(a),nvalue(b));
          else
               return (0);
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

#ifdef __GNUC__
#warning FIXME: Update hash_fn() to handle UTF-8 coded keys
#endif
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
#if USE_ERROR_HOOK
     pointer x;
     pointer hdl=sc->ERROR_HOOK;

     x=find_slot_in_env(sc,sc->envir,hdl,1);
    if (x != sc->NIL) {
         if(a!=0) {
               sc->code = cons(sc, cons(sc, sc->QUOTE, cons(sc,(a), sc->NIL)), sc->NIL);
         } else {
               sc->code = sc->NIL;
         }
         sc->code = cons(sc, mk_string(sc, (s)), sc->code);
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
    sc->args = cons(sc, mk_string(sc, (s)), sc->args);
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
          s_goto(sc,OP_T0LVL);

     case OP_T0LVL: /* top level */
          if(file_interactive(sc)) {
               putstr(sc,"\n");
          }
          sc->nesting=0;
          dump_stack_reset(sc);
          sc->envir = sc->global_env;
          sc->save_inport=sc->inport;
          sc->inport = sc->loadport;
          s_save(sc,OP_T0LVL, sc->NIL, sc->NIL);
          s_save(sc,OP_VALUEPRINT, sc->NIL, sc->NIL);
          s_save(sc,OP_T1LVL, sc->NIL, sc->NIL);
          if (file_interactive(sc)) {
              putstr(sc,prompt);
          }
          s_goto(sc,OP_READ_INTERNAL);

     case OP_T1LVL: /* top level */
          sc->code = sc->value;
          sc->inport=sc->save_inport;
          s_goto(sc,OP_EVAL);

     case OP_READ_INTERNAL:       /* internal read */
          sc->tok = token(sc);
          if(sc->tok==TOK_EOF) {
               if(sc->inport==sc->loadport) {
                    sc->args=sc->NIL;
                    s_goto(sc,OP_QUIT);
               } else {
                    s_return(sc,sc->EOF_OBJ);
               }
          }
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
         /*         sc->args=cons(sc,sc->code,sc->args);*/
         putstr(sc,"\nApply to: ");
         s_goto(sc,OP_P0LIST);
       }
       /* fall through */
     case OP_REAL_APPLY:
#endif
          if (is_proc(sc->code)) {
               s_goto(sc,procnum(sc->code));   /* PROCEDURE */
          } else if (is_foreign(sc->code)) {
               sc->safe_foreign = cons (sc, sc->NIL, sc->safe_foreign);
               x=sc->code->_object._ff(sc,sc->args);
               sc->safe_foreign = cdr (sc->safe_foreign);
               s_return(sc,x);
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

     case OP_LAMBDA:     /* lambda */
          s_return(sc,mk_closure(sc, sc->code, sc->envir));

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
          x=car(sc->code);
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
                Error_1(sc,"set!: unable to alter immutable variable", car(sc->code));
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
          sprintf(sc->strbuff, "%d: illegal operator", sc->op);
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
               if(car(sc->code)==sc->FEED_TO) {
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
          sprintf(sc->strbuff, "%d: illegal operator", sc->op);
          Error_0(sc,sc->strbuff);
     }
     return sc->T;
}

static pointer opexe_2(scheme *sc, enum scheme_opcodes op) {
     pointer x;
     num v;
#if USE_MATH
     double dd;
#endif

     switch (op) {
#if USE_MATH
     case OP_INEX2EX:    /* inexact->exact */
          x=car(sc->args);
          if(is_integer(x)) {
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
               pointer y=cadr(sc->args);
               s_return(sc, mk_real(sc, atan2(rvalue(x),rvalue(y))));
          }

     case OP_SQRT:
          x=car(sc->args);
          s_return(sc, mk_real(sc, sqrt(rvalue(x))));

     case OP_EXPT:
          x=car(sc->args);
          if(cdr(sc->args)==sc->NIL) {
               Error_0(sc,"expt: needs two arguments");
          } else {
               pointer y=cadr(sc->args);
               s_return(sc, mk_real(sc, pow(rvalue(x),rvalue(y))));
          }

     case OP_FLOOR:
          x=car(sc->args);
          s_return(sc, mk_real(sc, floor(rvalue(x))));

     case OP_CEILING:
          x=car(sc->args);
          s_return(sc, mk_real(sc, ceil(rvalue(x))));

     case OP_TRUNCATE : {
          double rvalue_of_x ;
          x=car(sc->args);
          rvalue_of_x = rvalue(x) ;
          if (rvalue_of_x > 0) {
            s_return(sc, mk_real(sc, floor(rvalue_of_x)));
          } else {
            s_return(sc, mk_real(sc, ceil(rvalue_of_x)));
          }
     }

     case OP_ROUND:
       x=car(sc->args);
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
          if(cdr(sc->args)==sc->NIL) {
               x=sc->args;
               v=num_one;
          } else {
               x = cdr(sc->args);
               v = nvalue(car(sc->args));
          }
          for (; x != sc->NIL; x = cdr(x)) {
               if (ivalue(car(x)) != 0)
                    v=num_intdiv(v,nvalue(car(x)));
               else {
                    Error_0(sc,"quotient: division by zero");
               }
          }
          s_return(sc,mk_number(sc, v));

     case OP_REM:        /* remainder */
          v = nvalue(car(sc->args));
          if (ivalue(cadr(sc->args)) != 0)
               v=num_rem(v,nvalue(cadr(sc->args)));
          else {
               Error_0(sc,"remainder: division by zero");
          }
          s_return(sc,mk_number(sc, v));

     case OP_MOD:        /* modulo */
          v = nvalue(car(sc->args));
          if (ivalue(cadr(sc->args)) != 0)
               v=num_mod(v,nvalue(cadr(sc->args)));
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
       if(*s=='#') {
         s_return(sc, mk_sharp_const(sc, s+1));
       } else {
         s_return(sc, mk_atom(sc, s));
       }
     }

     case OP_SYM2STR: /* symbol->string */
          x=mk_string(sc,symname(car(sc->args)));
          setimmutable(x);
          s_return(sc,x);
     case OP_ATOM2STR: /* atom->string */
       x=car(sc->args);
       if(is_number(x) || is_character(x) || is_string(x) || is_symbol(x)) {
         char *p;
         int len;
         atom2str(sc,x,0,&p,&len);
         s_return(sc,mk_counted_string(sc,p,len));
       } else {
         Error_1(sc, "atom->string: not an atom:", x);
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

          index=ivalue(cadr(sc->args));

          if(index>=g_utf8_strlen(strvalue(car(sc->args)), -1)) {
               Error_1(sc,"string-ref: out of bounds:",cadr(sc->args));
          }

          str = g_utf8_offset_to_pointer(str, (long)index);
          s_return(sc,mk_character(sc, g_utf8_get_char(str)));
     }

     case OP_STRSET: { /* string-set! */
          pointer a;
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

          a=car(sc->args);
          if(is_immutable(a)) {
               Error_1(sc,"string-set!: unable to alter immutable string:",a);
          }

          str=strvalue(a);
          index=ivalue(cadr(sc->args));
          if(index>=g_utf8_strlen(str, -1)) {
              Error_1(sc,"string-set!: out of bounds:",cadr(sc->args));
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
             sc->no_memory=1;
             Error_1(sc,"string-set!: No memory to alter string:",car(sc->args));
          }

          if (p1_len > 0)
             memcpy(newstr, str, p1_len);
          memcpy(newstr+p1_len, utf8, utf8_len);
          if (p2_len > 0)
             memcpy(newstr+p1_len+utf8_len, p2, p2_len);
          newstr[newlen] = '\0';

          free(strvalue(a));
          strvalue(a)=newstr;
          strlength(a)=newlen;

          s_return(sc,a);
     }

     case OP_STRAPPEND: { /* string-append */
       /* in 1.29 string-append was in Scheme in init.scm but was too slow */
       int len = 0;
       pointer newstr;
       pointer car_x;
       char *pos;

       /* compute needed length for new string */
       for (x = sc->args; x != sc->NIL; x = cdr(x)) {
          len += strlength(car(x));
       }
       newstr = mk_empty_string(sc, len, ' ');
       /* store the contents of the argument strings into the new string */
       pos = strvalue(newstr);
       for (x = sc->args; x != sc->NIL; x = cdr(x)) {
           car_x = car(x);
           memcpy(pos, strvalue(car_x), strlength(car_x));
           pos += strlength(car_x);
       }
       *pos = '\0';
       s_return(sc, newstr);
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

          beg = g_utf8_offset_to_pointer(str, (long)index0);
          end = g_utf8_offset_to_pointer(str, (long)index1);
          len=end-beg;
          x=mk_empty_string(sc,len,' ');
          memcpy(strvalue(x),beg,len);
          strvalue(x)[len] = '\0';

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
          if(fill!=sc->NIL) {
               fill_vector(vec,fill);
          }
          s_return(sc,vec);
     }

     case OP_VECLEN:  /* vector-length */
          s_return(sc,mk_integer(sc,ivalue(car(sc->args))));

     case OP_VECREF: { /* vector-ref */
          int index;

          index=ivalue(cadr(sc->args));

          if(index>=ivalue(car(sc->args))) {
               Error_1(sc,"vector-ref: out of bounds:",cadr(sc->args));
          }

          s_return(sc,vector_elem(car(sc->args),index));
     }

     case OP_VECSET: {   /* vector-set! */
          int index;

          if(is_immutable(car(sc->args))) {
               Error_1(sc,"vector-set!: unable to alter immutable vector:",car(sc->args));
          }

          index=ivalue(cadr(sc->args));
          if(index>=ivalue(car(sc->args))) {
               Error_1(sc,"vector-set!: out of bounds:",cadr(sc->args));
          }

          set_vector_elem(car(sc->args),index,caddr(sc->args));
          s_return(sc,car(sc->args));
     }

     default:
          sprintf(sc->strbuff, "%d: illegal operator", sc->op);
          Error_0(sc,sc->strbuff);
     }
     return sc->T;
}

static int is_list(scheme *sc, pointer a) {
        pointer slow, fast;

        slow = fast = a;
        while (1)
        {
                if (fast == sc->NIL)
                        return 1;
                if (!is_pair(fast))
                        return 0;
                fast = cdr(fast);
                if (fast == sc->NIL)
                        return 1;
                if (!is_pair(fast))
                        return 0;
                fast = cdr(fast);

                slow = cdr(slow);
                if (fast == slow)
                {
                        /* the fast pointer has looped back around and caught up
                           with the slow pointer, hence the structure is circular,
                           not of finite length, and therefore not a list */
                        return 0;
                }
        }
}

static int list_length(scheme *sc, pointer a) {
    int i=0;
        pointer slow, fast;

        slow = fast = a;
        while (1)
        {
                if (fast == sc->NIL)
                        return i;
                if (!is_pair(fast))
                        return i;
                fast = cdr(fast);
                ++i;
                if (fast == sc->NIL)
                        return i;
                if (!is_pair(fast))
                        return i;
                ++i;
                fast = cdr(fast);

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
          s_retbool(is_number(car(sc->args)) && is_integer(car(sc->args)));
     case OP_REALP:     /* real? */
          s_retbool(is_number(car(sc->args))); /* All numbers are real */
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
          s_retbool(is_list(sc, car(sc->args)));
     case OP_ENVP:        /* environment? */
          s_retbool(is_environment(car(sc->args)));
     case OP_VECTORP:     /* vector? */
          s_retbool(is_vector(car(sc->args)));
     case OP_EQ:         /* eq? */
          s_retbool(car(sc->args) == cadr(sc->args));
     case OP_EQV:        /* eqv? */
          s_retbool(eqv(car(sc->args), cadr(sc->args)));
     default:
          sprintf(sc->strbuff, "%d: illegal operator", sc->op);
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
          if(is_pair(cdr(sc->args))) {
               if(cadr(sc->args)!=sc->outport) {
                    x=cons(sc,sc->outport,sc->NIL);
                    s_save(sc,OP_SET_OUTPORT, x, sc->NIL);
                    sc->outport=cadr(sc->args);
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
                    sc->outport=car(sc->args);
               }
          }
          putstr(sc, "\n");
          s_return(sc,sc->T);

     case OP_ERR0:  /* error */
          sc->retcode=-1;
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
               s_save(sc,OP_ERR1, cdr(sc->args), sc->NIL);
               sc->args = car(sc->args);
               sc->print_flag = 1;
               s_goto(sc,OP_P0LIST);
          } else {
               putstr(sc, "\n");
               if(sc->interactive_repl) {
                    s_goto(sc,OP_T0LVL);
               } else {
                    return sc->NIL;
               }
          }

     case OP_REVERSE:    /* reverse */
          s_return(sc,reverse(sc, car(sc->args)));

     case OP_LIST_STAR: /* list* */
       s_return(sc,list_star(sc,sc->args));

     case OP_APPEND:     /* append */
          if(sc->args==sc->NIL) {
               s_return(sc,sc->NIL);
          }
          x=car(sc->args);
          if(cdr(sc->args)==sc->NIL) {
            s_return(sc,sc->args);
          }
          for (y = cdr(sc->args); y != sc->NIL; y = cdr(y)) {
               x=append(sc,x,car(y));
          }
          s_return(sc,x);

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
               sc->retcode=ivalue(car(sc->args));
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
     case OP_OPEN_OUTSTRING: /* open-output-string */
     case OP_OPEN_INOUTSTRING: /* open-input-output-string */ {
          int prop=0;
          pointer p;
          switch(op) {
               case OP_OPEN_INSTRING:     prop=port_input; break;
               case OP_OPEN_OUTSTRING:    prop=port_output; break;
               case OP_OPEN_INOUTSTRING:  prop=port_input|port_output; break;
               default:                   break;  /* Quiet the compiler */
          }
          p=port_from_string(sc, strvalue(car(sc->args)),
                     strvalue(car(sc->args))+strlength(car(sc->args)), prop);
          if(p==sc->NIL) {
               s_return(sc,sc->F);
          }
          s_return(sc,p);
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
     case OP_PEEK_CHAR: /* peek-char */ {
          gunichar c;
          if(is_pair(sc->args)) {
               if(car(sc->args)!=sc->inport) {
                    x=sc->inport;
                    x=cons(sc,x,sc->NIL);
                    s_save(sc,OP_SET_INPORT, x, sc->NIL);
                    sc->inport=car(sc->args);
               }
          }
          c=inchar(sc);
          if(c==EOF) {
               s_return(sc,sc->EOF_OBJ);
          }
          if(sc->op==OP_PEEK_CHAR) {
               backchar(sc,c);
          }
          s_return(sc,mk_character(sc,c));
     }

     case OP_CHAR_READY: /* char-ready? */ {
          pointer p=sc->inport;
          int res;
          if(is_pair(sc->args)) {
               p=car(sc->args);
          }
          res=p->_object._port->kind&port_string;
          s_retbool(res);
     }

     case OP_SET_INPORT: /* set-input-port */
          sc->inport=car(sc->args);
          s_return(sc,sc->value);

     case OP_SET_OUTPORT: /* set-output-port */
          sc->outport=car(sc->args);
          s_return(sc,sc->value);

     case OP_RDSEXPR:
          switch (sc->tok) {
          case TOK_EOF:
               if(sc->inport==sc->loadport) {
                    sc->args=sc->NIL;
                    s_goto(sc,OP_QUIT);
               } else {
                    s_return(sc,sc->EOF_OBJ);
               }
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
               s_return(sc,mk_atom(sc, readstr_upto(sc, "();\t\n\r ")));
          case TOK_DQUOTE:
               x=readstrexp(sc);
               if(x==sc->F) {
                 Error_0(sc,"Error reading string");
               }
               setimmutable(x);
               s_return(sc,x);
          case TOK_SHARP: {
               pointer f=find_slot_in_env(sc,sc->envir,sc->SHARP_HOOK,1);
               if(f==sc->NIL) {
                    Error_0(sc,"undefined sharp expression");
               } else {
                    sc->code=cons(sc,slot_value_in_env(f),sc->NIL);
                    s_goto(sc,OP_EVAL);
               }
          }
          case TOK_SHARP_CONST:
               if ((x = mk_sharp_const(sc, readstr_upto(sc, "();\t\n\r "))) == sc->NIL) {
                    Error_0(sc,"undefined sharp expression");
               } else {
                    s_return(sc,x);
               }
          default:
               Error_1(sc, "syntax error: illegal token", mk_integer (sc, sc->tok));
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
          if (sc->tok == TOK_RPAREN) {
               gunichar c = inchar(sc);
               if (c != '\n') backchar(sc,c);
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
               putstr(sc," )");
               s_return(sc,sc->T);
          } else {
               pointer elem=vector_elem(vec,i);
               ivalue_unchecked(cdr(sc->args))=i+1;
               s_save(sc,OP_PVECFROM, sc->args, sc->NIL);
               sc->args=elem;
               putstr(sc," ");
               s_goto(sc,OP_P0LIST);
          }
     }

     default:
          sprintf(sc->strbuff, "%d: illegal operator", sc->op);
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
          sprintf(sc->strbuff, "%d: illegal operator", sc->op);
          Error_0(sc,sc->strbuff);
     }
     return sc->T; /* NOTREACHED */
}

typedef pointer (*dispatch_func)(scheme *, enum scheme_opcodes);

typedef int (*test_predicate)(pointer);
static int is_any(pointer p) { return 1;}
static int is_num_integer(pointer p) {
  return is_number(p) && ((p)->_object._number.is_fixnum);
}
static int is_nonneg(pointer p) {
  return is_num_integer(p) && ivalue(p)>=0;
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
  {0,"input port"},
  {0,"output_port"},
  {is_environment, "environment"},
  {is_pair, "pair"},
  {0, "pair or '()"},
  {is_character, "character"},
  {is_vector, "vector"},
  {is_number, "number"},
  {is_num_integer, "integer"},
  {is_nonneg, "non-negative integer"},
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
  int count=0;
  int old_op;

  sc->op = op;
  for (;;) {
    op_code_info *pcd=dispatch_table+sc->op;
    if (pcd->name!=0) { /* if built-in function, check arguments */
      char msg[512];
      int ok=1;
      int n=list_length(sc,sc->args);

      /* Check number of arguments */
      if(n<pcd->min_arity) {
        ok=0;
        sprintf(msg,"%s: needs%s %d argument(s)",
                pcd->name,
                pcd->min_arity==pcd->max_arity?"":" at least",
                pcd->min_arity);
      }
      if(ok && n>pcd->max_arity) {
        ok=0;
        sprintf(msg,"%s: needs%s %d argument(s)",
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
            if(j==TST_INPORT[0]) {
              if(!is_inport(arg)) break;
            } else if(j==TST_OUTPORT[0]) {
              if(!is_outport(arg)) break;
            } else if(j==TST_LIST[0]) {
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
            sprintf(msg,"%s: argument %d must be: %s",
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
    old_op=sc->op;
    if (pcd->func(sc, (enum scheme_opcodes)sc->op) == sc->NIL) {
      return;
    }
    if(sc->no_memory) {
      fprintf(stderr,"No memory!\n");
      return;
    }
    count++;
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
     set_integer(y);
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
  is_character,
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
  sc->bc_flag = 0;
  sc->safe_foreign = sc->NIL;

  /* init sc->NIL */
  typeflag(sc->NIL) = (T_ATOM | MARK);
  car(sc->NIL) = cdr(sc->NIL) = sc->NIL;
  /* init T */
  typeflag(sc->T) = (T_ATOM | MARK);
  car(sc->T) = cdr(sc->T) = sc->T;
  /* init F */
  typeflag(sc->F) = (T_ATOM | MARK);
  car(sc->F) = cdr(sc->F) = sc->F;
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

  return !sc->no_memory;
}

SCHEME_EXPORT void scheme_set_input_port_file(scheme *sc, FILE *fin) {
  sc->inport=port_from_file(sc,fin,port_input);
}

void scheme_set_input_port_string(scheme *sc, char *start, char *past_the_end) {
  sc->inport=port_from_string(sc,start,past_the_end,port_input);
}

SCHEME_EXPORT void scheme_set_output_port_file(scheme *sc, FILE *fout) {
  sc->outport=port_from_file(sc,fout,port_output);
}

void scheme_set_output_port_string(scheme *sc, char *start, char *past_the_end) {
  sc->outport=port_from_string(sc,start,past_the_end,port_output);
}

void scheme_set_external_data(scheme *sc, void *p) {
 sc->ext_data=p;
}

void scheme_deinit(scheme *sc) {
  int i;

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
  sc->gc_verbose=0;
  gc(sc,sc->NIL,sc->NIL);

  for(i=0; i<=sc->last_cell_seg; i++) {
    sc->free(sc->alloc_seg[i]);
  }
}

void scheme_load_file(scheme *sc, FILE *fin) {
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
  sc->inport=sc->loadport;
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
  sc->load_stack[0].kind=port_input|port_string;
  sc->load_stack[0].rep.string.start=(char*)cmd; /* This func respects const */
  sc->load_stack[0].rep.string.past_the_end=(char*)cmd+strlen(cmd);
  sc->load_stack[0].rep.string.curr=(char*)cmd;
  sc->loadport=mk_port(sc,sc->load_stack);
  sc->retcode=0;
  sc->interactive_repl=0;
  sc->inport=sc->loadport;
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
void scheme_apply0(scheme *sc, const char *procname) {
     pointer carx=mk_symbol(sc,procname);
     pointer cdrx=sc->NIL;

     dump_stack_reset(sc);
     sc->envir = sc->global_env;
     sc->code = cons(sc,carx,cdrx);
     sc->interactive_repl=0;
     sc->retcode=0;
     Eval_Cycle(sc,OP_EVAL);
     }

void scheme_call(scheme *sc, pointer func, pointer args) {
   dump_stack_reset(sc);
   sc->envir = sc->global_env;
   sc->args = args;
   sc->code = func;
   sc->interactive_repl =0;
   sc->retcode = 0;
   Eval_Cycle(sc, OP_APPLY);
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
  FILE *fin;
  char *file_name=InitFile;
  int retcode;
  int isfile=1;

  if(argc==1) {
    printf(banner);
  }
  if(argc==2 && strcmp(argv[1],"-?")==0) {
    printf("Usage: %s [-? | <file1> <file2> ... | -1 <file> <arg1> <arg2> ...]\n\tUse - as filename for stdin.\n",argv[0]);
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
  if(access(file_name,0)!=0) {
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
        fin=fopen(file_name,"r");
      }
      for(;*argv;argv++) {
        pointer value=mk_string(&sc,*argv);
        args=cons(&sc,value,args);
      }
      args=reverse_in_place(&sc,sc.NIL,args);
      scheme_define(&sc,sc.global_env,mk_symbol(&sc,"*args*"),args);

    } else {
      fin=fopen(file_name,"r");
    }
    if(isfile && fin==0) {
      fprintf(stderr,"Could not open file %s\n",file_name);
    } else {
      if(isfile) {
        scheme_load_file(&sc,fin);
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
    scheme_load_file(&sc,stdin);
  }
  retcode=sc.retcode;
  scheme_deinit(&sc);

  return retcode;
}

#endif
