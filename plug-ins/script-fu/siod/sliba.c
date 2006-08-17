

/*
 *                   COPYRIGHT (c) 1988-1994 BY                             *
 *        PARADIGM ASSOCIATES INCORPORATED, CAMBRIDGE, MASSACHUSETTS.       *
 *        See the source file SLIB.C for more information.                  *

 Array-hacking code moved to another source file.

 */

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include <glib.h>

#include "siod.h"
#include "siodp.h"

static void
init_sliba_version (void)
{
  setvar (cintern ("*sliba-version*"),
          cintern ("$Id$"),
          NIL);
}

static LISP sym_plists = NIL;
static LISP bashnum = NIL;
static LISP sym_e = NIL;
static LISP sym_f = NIL;

void
init_storage_a1 (long type)
{
  long j;
  struct user_type_hooks *p;
  set_gc_hooks (type,
                array_gc_relocate,
                array_gc_mark,
                array_gc_scan,
                array_gc_free,
                &j);
  set_print_hooks (type, array_prin1);
  p = get_user_type_hooks (type);
  p->fast_print = array_fast_print;
  p->fast_read = array_fast_read;
  p->equal = array_equal;
  p->c_sxhash = array_sxhash;
}

void
init_storage_a (void)
{
  gc_protect (&bashnum);
  bashnum = newcell (tc_flonum);
  init_storage_a1 (tc_string);
  init_storage_a1 (tc_double_array);
  init_storage_a1 (tc_long_array);
  init_storage_a1 (tc_lisp_array);
  init_storage_a1 (tc_byte_array);
}

LISP
array_gc_relocate (LISP ptr)
{
  LISP nw;
  if ((nw = heap) >= heap_end)
    gc_fatal_error ();
  heap = nw + 1;
  memcpy (nw, ptr, sizeof (struct obj));
  return (nw);
}

void
array_gc_scan (LISP ptr)
{
  long j;
  if TYPEP
    (ptr, tc_lisp_array)
      for (j = 0; j < ptr->storage_as.lisp_array.dim; ++j)
      ptr->storage_as.lisp_array.data[j] =
        gc_relocate (ptr->storage_as.lisp_array.data[j]);
}

LISP
array_gc_mark (LISP ptr)
{
  long j;
  if TYPEP
    (ptr, tc_lisp_array)
      for (j = 0; j < ptr->storage_as.lisp_array.dim; ++j)
      gc_mark (ptr->storage_as.lisp_array.data[j]);
  return (NIL);
}

void
array_gc_free (LISP ptr)
{
  switch (ptr->type)
    {
    case tc_string:
    case tc_byte_array:
      free (ptr->storage_as.string.data);
      break;
    case tc_double_array:
      free (ptr->storage_as.double_array.data);
      break;
    case tc_long_array:
      free (ptr->storage_as.long_array.data);
      break;
    case tc_lisp_array:
      free (ptr->storage_as.lisp_array.data);
      break;
    }
}

void
array_prin1 (LISP ptr, struct gen_printio *f)
{
  int i, j;
  switch (ptr->type)
    {
    case tc_string:
      gput_st (f, "\"");
      if (strcspn (ptr->storage_as.string.data, "\"\\\n\r\t") ==
          strlen (ptr->storage_as.string.data))
        gput_st (f, ptr->storage_as.string.data);
      else
        {
          int n, c;
          char cbuff[3];
          n = strlen (ptr->storage_as.string.data);
          for (j = 0; j < n; ++j)
            switch (c = ptr->storage_as.string.data[j])
              {
              case '\\':
              case '"':
                cbuff[0] = '\\';
                cbuff[1] = c;
                cbuff[2] = 0;
                gput_st (f, cbuff);
                break;
              case '\n':
                gput_st (f, "\\n");
                break;
              case '\r':
                gput_st (f, "\\r");
                break;
              case '\t':
                gput_st (f, "\\t");
                break;
              default:
                cbuff[0] = c;
                cbuff[1] = 0;
                gput_st (f, cbuff);
                break;
              }
        }
      gput_st (f, "\"");
      break;
    case tc_double_array:
      gput_st (f, "#(");
      for (j = 0; j < ptr->storage_as.double_array.dim; ++j)
        {
          g_ascii_formatd (tkbuffer, TKBUFFERN, "%g",
                           ptr->storage_as.double_array.data[j]);
          gput_st (f, tkbuffer);
          if ((j + 1) < ptr->storage_as.double_array.dim)
            gput_st (f, " ");
        }
      gput_st (f, ")");
      break;
    case tc_long_array:
      gput_st (f, "#(");
      for (j = 0; j < ptr->storage_as.long_array.dim; ++j)
        {
          sprintf (tkbuffer, "%ld", ptr->storage_as.long_array.data[j]);
          gput_st (f, tkbuffer);
          if ((j + 1) < ptr->storage_as.long_array.dim)
            gput_st (f, " ");
        }
      gput_st (f, ")");
      break;
    case tc_byte_array:
      sprintf (tkbuffer, "#%ld\"", ptr->storage_as.string.dim);
      gput_st (f, tkbuffer);
      for (j = 0, i = 0; j < ptr->storage_as.string.dim; j++)
        {
          sprintf (tkbuffer + i, "%02x",
                   ptr->storage_as.string.data[j] & 0xFF);
          i += 2;
          if (i % TKBUFFERN == 0)
            {
              gput_st (f, tkbuffer);
              i = 0;
            }
        }
      if (i)
        gput_st (f, tkbuffer);
      gput_st (f, "\"");
      break;
    case tc_lisp_array:
      gput_st (f, "#(");
      for (j = 0; j < ptr->storage_as.lisp_array.dim; ++j)
        {
          lprin1g (ptr->storage_as.lisp_array.data[j], f);
          if ((j + 1) < ptr->storage_as.lisp_array.dim)
            gput_st (f, " ");
        }
      gput_st (f, ")");
      break;
    }
}

LISP
strcons (long length, char *data)
{
  long flag;
  LISP s;
  flag = no_interrupt (1);
  s = cons (NIL, NIL);
  s->type = tc_string;
  if (length == -1)
    length = strlen (data);
  s->storage_as.string.data = must_malloc (length + 1);
  s->storage_as.string.dim = length;
  if (data)
    memcpy (s->storage_as.string.data, data, length);
  s->storage_as.string.data[length] = 0;
  no_interrupt (flag);
  return (s);
}

int
rfs_getc (unsigned char **p)
{
  int i;
  i = **p;
  if (!i)
    return (EOF);
  *p = *p + 1;
  return (i);
}

void
rfs_ungetc (unsigned char c, unsigned char **p)
{
  *p = *p - 1;
}

LISP
read_from_string (LISP x)
{
  char *p;
  struct gen_readio s;
  p = get_c_string (x);
  s.getc_fcn = (int (*)(void *)) rfs_getc;
  s.ungetc_fcn = (void (*)(int, void *)) rfs_ungetc;
  s.cb_argument = (char *) &p;
  return (readtl (&s));
}

int
pts_puts (char *from, void *cb)
{
  LISP into;
  size_t fromlen, intolen, intosize, fitsize;
  into = (LISP) cb;
  fromlen = strlen (from);
  intolen = strlen (into->storage_as.string.data);
  intosize = into->storage_as.string.dim - intolen;
  fitsize = (fromlen < intosize) ? fromlen : intosize;
  memcpy (&into->storage_as.string.data[intolen], from, fitsize);
  into->storage_as.string.data[intolen + fitsize] = 0;
  if (fitsize < fromlen)
    my_err ("print to string overflow", NIL);
  return (1);
}

LISP
err_wta_str (LISP exp)
{
  return (my_err ("not a string", exp));
}

LISP
print_to_string (LISP exp, LISP str, LISP nostart)
{
  struct gen_printio s;
  if NTYPEP
    (str, tc_string) err_wta_str (str);
  s.putc_fcn = NULL;
  s.puts_fcn = pts_puts;
  s.cb_argument = str;
  if NULLP
    (nostart)
      str->storage_as.string.data[0] = 0;
  lprin1g (exp, &s);
  return (str);
}

LISP
aref1 (LISP a, LISP i)
{
  long k;
  if NFLONUMP
    (i) my_err ("bad index to aref", i);
  k = (long) FLONM (i);
  if (k < 0)
    my_err ("negative index to aref", i);
  switch TYPE
    (a)
    {
    case tc_string:
    case tc_byte_array:
      if (k >= a->storage_as.string.dim)
        my_err ("index too large", i);
      return (flocons ((guint8) a->storage_as.string.data[k]));
    case tc_double_array:
      if (k >= a->storage_as.double_array.dim)
        my_err ("index too large", i);
      return (flocons (a->storage_as.double_array.data[k]));
    case tc_long_array:
      if (k >= a->storage_as.long_array.dim)
        my_err ("index too large", i);
      return (flocons (a->storage_as.long_array.data[k]));
    case tc_lisp_array:
      if (k >= a->storage_as.lisp_array.dim)
        my_err ("index too large", i);
      return (a->storage_as.lisp_array.data[k]);
    default:
      return (my_err ("invalid argument to aref", a));
    }
}

void
err1_aset1 (LISP i)
{
  my_err ("index to aset too large", i);
}

void
err2_aset1 (LISP v)
{
  my_err ("bad value to store in array", v);
}

LISP
aset1 (LISP a, LISP i, LISP v)
{
  long k;
  if NFLONUMP
    (i) my_err ("bad index to aset", i);
  k = (long) FLONM (i);
  if (k < 0)
    my_err ("negative index to aset", i);
  switch TYPE
    (a)
    {
    case tc_string:
    case tc_byte_array:
      if NFLONUMP
        (v) err2_aset1 (v);
      if (k >= a->storage_as.string.dim)
        err1_aset1 (i);
      a->storage_as.string.data[k] = (char) FLONM (v);
      return (v);
    case tc_double_array:
      if NFLONUMP
        (v) err2_aset1 (v);
      if (k >= a->storage_as.double_array.dim)
        err1_aset1 (i);
      a->storage_as.double_array.data[k] = FLONM (v);
      return (v);
    case tc_long_array:
      if NFLONUMP
        (v) err2_aset1 (v);
      if (k >= a->storage_as.long_array.dim)
        err1_aset1 (i);
      a->storage_as.long_array.data[k] = (long) FLONM (v);
      return (v);
    case tc_lisp_array:
      if (k >= a->storage_as.lisp_array.dim)
        err1_aset1 (i);
      a->storage_as.lisp_array.data[k] = v;
      return (v);
    default:
      return (my_err ("invalid argument to aset", a));
    }
}

LISP
arcons (long typecode, long n, long initp)
{
  LISP a;
  long flag, j;
  flag = no_interrupt (1);
  a = cons (NIL, NIL);
  switch (typecode)
    {
    case tc_double_array:
      a->storage_as.double_array.dim = n;
      a->storage_as.double_array.data = (double *) must_malloc (n *
                                                           sizeof (double));
      if (initp)
        for (j = 0; j < n; ++j)
          a->storage_as.double_array.data[j] = 0.0;
      break;
    case tc_long_array:
      a->storage_as.long_array.dim = n;
      a->storage_as.long_array.data = (long *) must_malloc (n * sizeof (long));
      if (initp)
        for (j = 0; j < n; ++j)
          a->storage_as.long_array.data[j] = 0;
      break;
    case tc_string:
      a->storage_as.string.dim = n;
      a->storage_as.string.data = (char *) must_malloc (n + 1);
      a->storage_as.string.data[n] = 0;
      if (initp)
        for (j = 0; j < n; ++j)
          a->storage_as.string.data[j] = ' ';
    case tc_byte_array:
      a->storage_as.string.dim = n;
      a->storage_as.string.data = (char *) must_malloc (n);
      if (initp)
        for (j = 0; j < n; ++j)
          a->storage_as.string.data[j] = 0;
      break;
    case tc_lisp_array:
      a->storage_as.lisp_array.dim = n;
      a->storage_as.lisp_array.data = (LISP *) must_malloc (n * sizeof (LISP));
      for (j = 0; j < n; ++j)
        a->storage_as.lisp_array.data[j] = NIL;
      break;
    default:
      errswitch ();
    }
  a->type = typecode;
  no_interrupt (flag);
  return (a);
}

LISP
mallocl (void *place, long size)
{
  long n, r;
  LISP retval;
  n = size / sizeof (long);
  r = size % sizeof (long);
  if (r)
    ++n;
  retval = arcons (tc_long_array, n, 0);
  *(long **) place = retval->storage_as.long_array.data;
  return (retval);
}

LISP
cons_array (LISP dim, LISP kind)
{
  LISP a;
  long flag, n, j;
  if (NFLONUMP (dim) || (FLONM (dim) < 0))
    return (my_err ("bad dimension to cons-array", dim));
  else
    n = (long) FLONM (dim);
  flag = no_interrupt (1);
  a = cons (NIL, NIL);
  if EQ
    (cintern ("double"), kind)
    {
      a->type = tc_double_array;
      a->storage_as.double_array.dim = n;
      a->storage_as.double_array.data = (double *) must_malloc (n *
                                                           sizeof (double));
      for (j = 0; j < n; ++j)
        a->storage_as.double_array.data[j] = 0.0;
    }
  else if EQ
    (cintern ("long"), kind)
    {
      a->type = tc_long_array;
      a->storage_as.long_array.dim = n;
      a->storage_as.long_array.data = (long *) must_malloc (n * sizeof (long));
      for (j = 0; j < n; ++j)
        a->storage_as.long_array.data[j] = 0;
    }
  else if EQ
    (cintern ("string"), kind)
    {
      a->type = tc_string;
      a->storage_as.string.dim = n;
      a->storage_as.string.data = (char *) must_malloc (n + 1);
      a->storage_as.string.data[n] = 0;
      for (j = 0; j < n; ++j)
        a->storage_as.string.data[j] = ' ';
    }
  else if EQ
    (cintern ("byte"), kind)
    {
      a->type = tc_byte_array;
      a->storage_as.string.dim = n;
      a->storage_as.string.data = (char *) must_malloc (n);
      for (j = 0; j < n; ++j)
        a->storage_as.string.data[j] = 0;
    }
  else if (EQ (cintern ("lisp"), kind) || NULLP (kind))
    {
      a->type = tc_lisp_array;
      a->storage_as.lisp_array.dim = n;
      a->storage_as.lisp_array.data = (LISP *) must_malloc (n * sizeof (LISP));
      for (j = 0; j < n; ++j)
        a->storage_as.lisp_array.data[j] = NIL;
    }
  else
    my_err ("bad type of array", kind);
  no_interrupt (flag);
  return (a);
}

LISP
string_append (LISP args)
{
  long size;
  LISP l, s;
  char *data;
  size = 0;
  for (l = args; NNULLP (l); l = cdr (l))
    size += strlen (get_c_string (car (l)));
  s = strcons (size, NULL);
  data = s->storage_as.string.data;
  data[0] = 0;
  for (l = args; NNULLP (l); l = cdr (l))
    strcat (data, get_c_string (car (l)));
  return (s);
}

LISP
bytes_append (LISP args)
{
  long size, n, j;
  LISP l, s;
  char *data, *ptr;
  size = 0;
  for (l = args; NNULLP (l); l = cdr (l))
    {
      get_c_string_dim (car (l), &n);
      size += n;
    }
  s = arcons (tc_byte_array, size, 0);
  data = s->storage_as.string.data;
  for (j = 0, l = args; NNULLP (l); l = cdr (l))
    {
      ptr = get_c_string_dim (car (l), &n);
      memcpy (&data[j], ptr, n);
      j += n;
    }
  return (s);
}

LISP
substring (LISP str, LISP start, LISP end)
{
  long s, e, n;
  char *data;
  data = get_c_string_dim (str, &n);
  s = get_c_long (start);
  if NULLP
    (end)
      e = n;
  else
    e = get_c_long (end);
  if ((s < 0) || (s > e))
    my_err ("bad start index", start);
  if ((e < 0) || (e > n))
    my_err ("bad end index", end);
  return (strcons (e - s, &data[s]));
}

LISP
string_search (LISP token, LISP str)
{
  char *s1, *s2, *ptr;
  s1 = get_c_string (str);
  s2 = get_c_string (token);
  ptr = strstr (s1, s2);
  if (ptr)
    return (flocons (ptr - s1));
  else
    return (NIL);
}

#define IS_TRIM_SPACE(_x) (strchr(" \t\r\n",(_x)))

LISP
string_trim (LISP str)
{
  char *start, *end; /*, *sp = " \t\r\n";*/
  start = get_c_string (str);
  while (*start && IS_TRIM_SPACE (*start))
    ++start;
  end = &start[strlen (start)];
  while ((end > start) && IS_TRIM_SPACE (*(end - 1)))
    --end;
  return (strcons (end - start, start));
}

LISP
string_trim_left (LISP str)
{
  char *start, *end;
  start = get_c_string (str);
  while (*start && IS_TRIM_SPACE (*start))
    ++start;
  end = &start[strlen (start)];
  return (strcons (end - start, start));
}

LISP
string_trim_right (LISP str)
{
  char *start, *end;
  start = get_c_string (str);
  end = &start[strlen (start)];
  while ((end > start) && IS_TRIM_SPACE (*(end - 1)))
    --end;
  return (strcons (end - start, start));
}

LISP
string_upcase (LISP str)
{
  LISP result;
  char *s1, *s2;
  long j, n;
  s1 = get_c_string (str);
  n = strlen (s1);
  result = strcons (n, s1);
  s2 = get_c_string (result);
  for (j = 0; j < n; ++j)
    s2[j] = g_ascii_toupper (s2[j]);
  return (result);
}

LISP
string_downcase (LISP str)
{
  LISP result;
  char *s1, *s2;
  long j, n;
  s1 = get_c_string (str);
  n = strlen (s1);
  result = strcons (n, s1);
  s2 = get_c_string (result);
  for (j = 0; j < n; ++j)
    s2[j] = g_ascii_tolower (s2[j]);
  return (result);
}

LISP
lreadstring (struct gen_readio * f)
{
  int j, c, n, ndigits;
  char *p;
  j = 0;
  p = tkbuffer;
  while (((c = GETC_FCN (f)) != '"') && (c != EOF))
    {
      if (c == '\\')
        {
          c = GETC_FCN (f);
          if (c == EOF)
            my_err ("eof after \\", NIL);
          switch (c)
            {
            case '\\':
              c = '\\';
              break;
            case 'n':
              c = '\n';
              break;
            case 't':
              c = '\t';
              break;
            case 'r':
              c = '\r';
              break;
            case 'd':
              c = 0x04;
              break;
            case 'N':
              c = 0;
              break;
            case 's':
              c = ' ';
              break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
              n = c - '0';
              ndigits = 1;
              while (ndigits < 3)
                {
                  c = GETC_FCN (f);
                  if (c == EOF)
                    my_err ("eof after \\0", NIL);
                  if (c >= '0' && c <= '7')
                    {
                      n = n * 8 + c - '0';
                      ndigits++;
                    }
                  else
                    {
                      UNGETC_FCN (c, f);
                      break;
                    }
                }
              c = n;
            }
        }
      if ((j + 1) >= TKBUFFERN)
        my_err ("read string overflow", NIL);
      ++j;
      *p++ = c;
    }
  *p = 0;
  return (strcons (j, tkbuffer));
}


LISP
lreadsharp (struct gen_readio * f)
{
  LISP obj, l, result;
  long j, n;
  int c;
  c = GETC_FCN (f);
  switch (c)
    {
    case '(':
      UNGETC_FCN (c, f);
      obj = lreadr (f);
      n = nlength (obj);
      result = arcons (tc_lisp_array, n, 1);
      for (l = obj, j = 0; j < n; l = cdr (l), ++j)
        result->storage_as.lisp_array.data[j] = car (l);
      return (result);
    case '.':
      obj = lreadr (f);
      return (leval (obj, NIL));
    case 'f':
      return (NIL);
    case 't':
      return (flocons (1));
    default:
      return (my_err ("readsharp syntax not handled", NIL));
    }
}

#define HASH_COMBINE(_h1,_h2,_mod) ((((_h1) * 17 + 1) ^ (_h2)) % (_mod))

long
c_sxhash (LISP obj, long n)
{
  long hash;
  unsigned char *s;
  LISP tmp;
  struct user_type_hooks *p;
  STACK_CHECK (&obj);
  INTERRUPT_CHECK ();
  switch TYPE
    (obj)
    {
    case tc_nil:
      return (0);
    case tc_cons:
      hash = c_sxhash (CAR (obj), n);
      for (tmp = CDR (obj); CONSP (tmp); tmp = CDR (tmp))
        hash = HASH_COMBINE (hash, c_sxhash (CAR (tmp), n), n);
      hash = HASH_COMBINE (hash, c_sxhash (tmp, n), n);
      return (hash);
    case tc_symbol:
      for (hash = 0, s = (unsigned char *) PNAME (obj); *s; ++s)
        hash = HASH_COMBINE (hash, *s, n);
      return (hash);
    case tc_subr_0:
    case tc_subr_1:
    case tc_subr_2:
    case tc_subr_3:
    case tc_subr_4:
    case tc_subr_5:
    case tc_lsubr:
    case tc_fsubr:
    case tc_msubr:
      for (hash = 0, s = (unsigned char *) obj->storage_as.subr.name; *s; ++s)
        hash = HASH_COMBINE (hash, *s, n);
      return (hash);
    case tc_flonum:
      return (((unsigned long) FLONM (obj)) % n);
    default:
      p = get_user_type_hooks (TYPE (obj));
      if (p->c_sxhash)
        return ((*p->c_sxhash) (obj, n));
      else
        return (0);
    }
}

LISP
sxhash (LISP obj, LISP n)
{
  return (flocons (c_sxhash (obj, FLONUMP (n) ? (long) FLONM (n) : 10000)));
}

LISP
equal (LISP a, LISP b)
{
  struct user_type_hooks *p;
  long atype;
  STACK_CHECK (&a);
loop:
  INTERRUPT_CHECK ();
  if EQ
    (a, b) return (sym_t);
  atype = TYPE (a);
  if (atype != TYPE (b))
    return (NIL);
  switch (atype)
    {
    case tc_cons:
      if NULLP
        (equal (car (a), car (b))) return (NIL);
      a = cdr (a);
      b = cdr (b);
      goto loop;
    case tc_flonum:
      return ((FLONM (a) == FLONM (b)) ? sym_t : NIL);
    case tc_symbol:
      return (NIL);
    default:
      p = get_user_type_hooks (atype);
      if (p->equal)
        return ((*p->equal) (a, b));
      else
        return (NIL);
    }
}

LISP
array_equal (LISP a, LISP b)
{
  long j, len;
  switch (TYPE (a))
    {
    case tc_string:
    case tc_byte_array:
      len = a->storage_as.string.dim;
      if (len != b->storage_as.string.dim)
        return (NIL);
      if (memcmp (a->storage_as.string.data, b->storage_as.string.data, len) == 0)
        return (sym_t);
      else
        return (NIL);
    case tc_long_array:
      len = a->storage_as.long_array.dim;
      if (len != b->storage_as.long_array.dim)
        return (NIL);
      if (memcmp (a->storage_as.long_array.data,
                  b->storage_as.long_array.data,
                  len * sizeof (long)) == 0)
          return (sym_t);
      else
        return (NIL);
    case tc_double_array:
      len = a->storage_as.double_array.dim;
      if (len != b->storage_as.double_array.dim)
        return (NIL);
      for (j = 0; j < len; ++j)
        if (a->storage_as.double_array.data[j] !=
            b->storage_as.double_array.data[j])
          return (NIL);
      return (sym_t);
    case tc_lisp_array:
      len = a->storage_as.lisp_array.dim;
      if (len != b->storage_as.lisp_array.dim)
        return (NIL);
      for (j = 0; j < len; ++j)
        if NULLP
          (equal (a->storage_as.lisp_array.data[j],
                  b->storage_as.lisp_array.data[j]))
            return (NIL);
      return (sym_t);
    default:
      return (errswitch ());
    }
}

long
array_sxhash (LISP a, long n)
{
  long j, len, hash;
  unsigned char *char_data;
  unsigned long *long_data;
  double *double_data;
  switch (TYPE (a))
    {
    case tc_string:
    case tc_byte_array:
      len = a->storage_as.string.dim;
      for (j = 0, hash = 0, char_data = (unsigned char *) a->storage_as.string.data;
           j < len;
           ++j, ++char_data)
        hash = HASH_COMBINE (hash, *char_data, n);
      return (hash);
    case tc_long_array:
      len = a->storage_as.long_array.dim;
      for (j = 0, hash = 0, long_data = (unsigned long *) a->storage_as.long_array.data;
           j < len;
           ++j, ++long_data)
        hash = HASH_COMBINE (hash, *long_data % n, n);
      return (hash);
    case tc_double_array:
      len = a->storage_as.double_array.dim;
      for (j = 0, hash = 0, double_data = a->storage_as.double_array.data;
           j < len;
           ++j, ++double_data)
        hash = HASH_COMBINE (hash, (unsigned long) *double_data % n, n);
      return (hash);
    case tc_lisp_array:
      len = a->storage_as.lisp_array.dim;
      for (j = 0, hash = 0; j < len; ++j)
        hash = HASH_COMBINE (hash,
                             c_sxhash (a->storage_as.lisp_array.data[j], n),
                             n);
      return (hash);
    default:
      errswitch ();
      return (0);
    }
}

long
href_index (LISP table, LISP key)
{
  long index;
  if NTYPEP
    (table, tc_lisp_array) my_err ("not a hash table", table);
  index = c_sxhash (key, table->storage_as.lisp_array.dim);
  if ((index < 0) || (index >= table->storage_as.lisp_array.dim))
    {
      my_err ("sxhash inconsistency", table);
      return (0);
    }
  else
    return (index);
}

LISP
href (LISP table, LISP key)
{
  return (cdr (assoc (key,
              table->storage_as.lisp_array.data[href_index (table, key)])));
}

LISP
hset (LISP table, LISP key, LISP value)
{
  long index;
  LISP cell, l;
  index = href_index (table, key);
  l = table->storage_as.lisp_array.data[index];
  if NNULLP
    (cell = assoc (key, l))
      return (setcdr (cell, value));
  cell = cons (key, value);
  table->storage_as.lisp_array.data[index] = cons (cell, l);
  return (value);
}

LISP
assoc (LISP x, LISP alist)
{
  LISP l, tmp;
  for (l = alist; CONSP (l); l = CDR (l))
    {
      tmp = CAR (l);
      if (CONSP (tmp) && equal (CAR (tmp), x))
        return (tmp);
      INTERRUPT_CHECK ();
    }
  if EQ
    (l, NIL) return (NIL);
  return (my_err ("improper list to assoc", alist));
}

LISP
assv (LISP x, LISP alist)
{
  LISP l, tmp;
  for (l = alist; CONSP (l); l = CDR (l))
    {
      tmp = CAR (l);
      if (CONSP (tmp) && NNULLP (eql (CAR (tmp), x)))
        return (tmp);
      INTERRUPT_CHECK ();
    }
  if EQ
    (l, NIL) return (NIL);
  return (my_err ("improper list to assv", alist));
}

void
put_long (long i, FILE * f)
{
  fwrite (&i, sizeof (long), 1, f);
}

long
get_long (FILE * f)
{
  long i;
  fread (&i, sizeof (long), 1, f);
  return (i);
}

long
fast_print_table (LISP obj, LISP table)
{
  FILE *f;
  LISP ht, index;
  f = get_c_file (car (table), (FILE *) NULL);
  if NULLP
    (ht = car (cdr (table)))
      return (1);
  index = href (ht, obj);
  if NNULLP
    (index)
    {
      putc (FO_fetch, f);
      put_long (get_c_long (index), f);
      return (0);
    }
  if NULLP
    (index = car (cdr (cdr (table))))
      return (1);
  hset (ht, obj, index);
  FLONM (bashnum) = 1.0;
  setcar (cdr (cdr (table)), plus (index, bashnum));
  putc (FO_store, f);
  put_long (get_c_long (index), f);
  return (1);
}

LISP
fast_print (LISP obj, LISP table)
{
  FILE *f;
  long len;
  LISP tmp;
  struct user_type_hooks *p;
  STACK_CHECK (&obj);
  f = get_c_file (car (table), (FILE *) NULL);
  switch (TYPE (obj))
    {
    case tc_nil:
      putc (tc_nil, f);
      return (NIL);
    case tc_cons:
      for (len = 0, tmp = obj; CONSP (tmp); tmp = CDR (tmp))
        {
          INTERRUPT_CHECK ();
          ++len;
        }
      if (len == 1)
        {
          putc (tc_cons, f);
          fast_print (car (obj), table);
          fast_print (cdr (obj), table);
        }
      else if NULLP
        (tmp)
        {
          putc (FO_list, f);
          put_long (len, f);
          for (tmp = obj; CONSP (tmp); tmp = CDR (tmp))
            fast_print (CAR (tmp), table);
        }
      else
        {
          putc (FO_listd, f);
          put_long (len, f);
          for (tmp = obj; CONSP (tmp); tmp = CDR (tmp))
            fast_print (CAR (tmp), table);
          fast_print (tmp, table);
        }
      return (NIL);
    case tc_flonum:
      putc (tc_flonum, f);
      fwrite (&obj->storage_as.flonum.data,
              sizeof (obj->storage_as.flonum.data),
              1,
              f);
      return (NIL);
    case tc_symbol:
      if (fast_print_table (obj, table))
        {
          putc (tc_symbol, f);
          len = strlen (PNAME (obj));
          if (len >= TKBUFFERN)
            my_err ("symbol name too long", obj);
          put_long (len, f);
          fwrite (PNAME (obj), len, 1, f);
          return (sym_t);
        }
      else
        return (NIL);
    default:
      p = get_user_type_hooks (TYPE (obj));
      if (p->fast_print)
        return ((*p->fast_print) (obj, table));
      else
        return (my_err ("cannot fast-print", obj));
    }
}

LISP
fast_read (LISP table)
{
  FILE *f;
  LISP tmp, l;
  struct user_type_hooks *p;
  int c;
  long len;
  f = get_c_file (car (table), (FILE *) NULL);
  c = getc (f);
  if (c == EOF)
    return (table);
  switch (c)
    {
    case FO_comment:
      while ((c = getc (f)))
        switch (c)
          {
          case EOF:
            return (table);
          case '\n':
            return (fast_read (table));
          }
    case FO_fetch:
      len = get_long (f);
      FLONM (bashnum) = len;
      return (href (car (cdr (table)), bashnum));
    case FO_store:
      len = get_long (f);
      tmp = fast_read (table);
      hset (car (cdr (table)), flocons (len), tmp);
      return (tmp);
    case tc_nil:
      return (NIL);
    case tc_cons:
      tmp = fast_read (table);
      return (cons (tmp, fast_read (table)));
    case FO_list:
    case FO_listd:
      len = get_long (f);
      FLONM (bashnum) = len;
      l = make_list (bashnum, NIL);
      tmp = l;
      while (len > 1)
        {
          CAR (tmp) = fast_read (table);
          tmp = CDR (tmp);
          --len;
        }
      CAR (tmp) = fast_read (table);
      if (c == FO_listd)
        CDR (tmp) = fast_read (table);
      return (l);
    case tc_flonum:
      tmp = newcell (tc_flonum);
      fread (&tmp->storage_as.flonum.data,
             sizeof (tmp->storage_as.flonum.data),
             1,
             f);
      return (tmp);
    case tc_symbol:
      len = get_long (f);
      if (len >= TKBUFFERN)
        my_err ("symbol name too long", NIL);
      fread (tkbuffer, len, 1, f);
      tkbuffer[len] = 0;
      return (rintern (tkbuffer));
    default:
      p = get_user_type_hooks (c);
      if (p->fast_read)
        return (*p->fast_read) (c, table);
      else
        return (my_err ("unknown fast-read opcode", flocons (c)));
    }
}

LISP
array_fast_print (LISP ptr, LISP table)
{
  int j, len;
  FILE *f;
  f = get_c_file (car (table), (FILE *) NULL);
  switch (ptr->type)
    {
    case tc_string:
    case tc_byte_array:
      putc (ptr->type, f);
      len = ptr->storage_as.string.dim;
      put_long (len, f);
      fwrite (ptr->storage_as.string.data, len, 1, f);
      return (NIL);
    case tc_double_array:
      putc (tc_double_array, f);
      len = ptr->storage_as.double_array.dim * sizeof (double);
      put_long (len, f);
      fwrite (ptr->storage_as.double_array.data, len, 1, f);
      return (NIL);
    case tc_long_array:
      putc (tc_long_array, f);
      len = ptr->storage_as.long_array.dim * sizeof (long);
      put_long (len, f);
      fwrite (ptr->storage_as.long_array.data, len, 1, f);
      return (NIL);
    case tc_lisp_array:
      putc (tc_lisp_array, f);
      len = ptr->storage_as.lisp_array.dim;
      put_long (len, f);
      for (j = 0; j < len; ++j)
        fast_print (ptr->storage_as.lisp_array.data[j], table);
      return (NIL);
    default:
      return (errswitch ());
    }
}

LISP
array_fast_read (int code, LISP table)
{
  long j, len, iflag;
  FILE *f;
  LISP ptr;
  f = get_c_file (car (table), (FILE *) NULL);
  switch (code)
    {
    case tc_string:
      len = get_long (f);
      ptr = strcons (len, NULL);
      fread (ptr->storage_as.string.data, len, 1, f);
      ptr->storage_as.string.data[len] = 0;
      return (ptr);
    case tc_byte_array:
      len = get_long (f);
      iflag = no_interrupt (1);
      ptr = newcell (tc_byte_array);
      ptr->storage_as.string.dim = len;
      ptr->storage_as.string.data =
        (char *) must_malloc (len);
      fread (ptr->storage_as.string.data, len, 1, f);
      no_interrupt (iflag);
      return (ptr);
    case tc_double_array:
      len = get_long (f);
      iflag = no_interrupt (1);
      ptr = newcell (tc_double_array);
      ptr->storage_as.double_array.dim = len;
      ptr->storage_as.double_array.data =
        (double *) must_malloc (len * sizeof (double));
      fread (ptr->storage_as.double_array.data, sizeof (double), len, f);
      no_interrupt (iflag);
      return (ptr);
    case tc_long_array:
      len = get_long (f);
      iflag = no_interrupt (1);
      ptr = newcell (tc_long_array);
      ptr->storage_as.long_array.dim = len;
      ptr->storage_as.long_array.data =
        (long *) must_malloc (len * sizeof (long));
      fread (ptr->storage_as.long_array.data, sizeof (long), len, f);
      no_interrupt (iflag);
      return (ptr);
    case tc_lisp_array:
      len = get_long (f);
      FLONM (bashnum) = len;
      ptr = cons_array (bashnum, NIL);
      for (j = 0; j < len; ++j)
        ptr->storage_as.lisp_array.data[j] = fast_read (table);
      return (ptr);
    default:
      return (errswitch ());
    }
}

long
get_c_long (LISP x)
{
  if NFLONUMP
    (x) my_err ("not a number", x);
  return ((long) FLONM (x));
}

double
get_c_double (LISP x)
{
  if NFLONUMP
    (x) my_err ("not a number", x);
  return (FLONM (x));
}

LISP
make_list (LISP x, LISP v)
{
  long n;
  LISP l;
  n = get_c_long (x);
  l = NIL;
  while (n > 0)
    {
      l = cons (v, l);
      --n;
    }
  return (l);
}

LISP
lfread (LISP size, LISP file)
{
  long flag, n, ret, m;
  char *buffer;
  LISP s;
  FILE *f;
  f = get_c_file (file, stdin);
  flag = no_interrupt (1);
  switch (TYPE (size))
    {
    case tc_string:
    case tc_byte_array:
      s = size;
      buffer = s->storage_as.string.data;
      n = s->storage_as.string.dim;
      m = 0;
      break;
    default:
      n = get_c_long (size);
      buffer = (char *) must_malloc (n + 1);
      buffer[n] = 0;
      m = 1;
    }
  ret = fread (buffer, 1, n, f);
  if (ret == 0)
    {
      if (m)
        free (buffer);
      no_interrupt (flag);
      return (NIL);
    }
  if (m)
    {
      if (ret == n)
        {
          s = cons (NIL, NIL);
          s->type = tc_string;
          s->storage_as.string.data = buffer;
          s->storage_as.string.dim = n;
        }
      else
        {
          s = strcons (ret, NULL);
          memcpy (s->storage_as.string.data, buffer, ret);
          free (buffer);
        }
      no_interrupt (flag);
      return (s);
    }
  no_interrupt (flag);
  return (flocons ((double) ret));
}

LISP
lfwrite (LISP string, LISP file)
{
  FILE *f;
  long flag;
  char *data;
  long dim, len;
  f = get_c_file (file, stdout);
  data = get_c_string_dim (CONSP (string) ? car (string) : string, &dim);
  len = CONSP (string) ? get_c_long (cadr (string)) : dim;
  if (len <= 0)
    return (NIL);
  if (len > dim)
    my_err ("write length too long", string);
  flag = no_interrupt (1);
  fwrite (data, 1, len, f);
  no_interrupt (flag);
  return (NIL);
}

LISP
lfflush (LISP file)
{
  FILE *f;
  long flag;
  f = get_c_file (file, stdout);
  flag = no_interrupt (1);
  fflush (f);
  no_interrupt (flag);
  return (NIL);
}

LISP
string_length (LISP string)
{
  if NTYPEP
    (string, tc_string) err_wta_str (string);
  return (flocons (strlen (string->storage_as.string.data)));
}

LISP
string_dim (LISP string)
{
  if NTYPEP
    (string, tc_string) err_wta_str (string);
  return (flocons ((double) string->storage_as.string.dim));
}

long
nlength (LISP obj)
{
  LISP l;
  long n;
  switch TYPE
    (obj)
    {
    case tc_string:
      return (strlen (obj->storage_as.string.data));
    case tc_byte_array:
      return (obj->storage_as.string.dim);
    case tc_double_array:
      return (obj->storage_as.double_array.dim);
    case tc_long_array:
      return (obj->storage_as.long_array.dim);
    case tc_lisp_array:
      return (obj->storage_as.lisp_array.dim);
    case tc_nil:
      return (0);
    case tc_cons:
      for (l = obj, n = 0; CONSP (l); l = CDR (l), ++n)
        INTERRUPT_CHECK ();
      if NNULLP
        (l) my_err ("improper list to length", obj);
      return (n);
    default:
      my_err ("wta to length", obj);
      return (0);
    }
}

LISP
llength (LISP obj)
{
  return (flocons (nlength (obj)));
}

LISP
number2string (LISP x, LISP b, LISP w, LISP p)
{
  char buffer[1000];
  double y;
  long base, width, prec;
  if NFLONUMP
    (x) my_err ("wta", x);
  y = FLONM (x);
  width = NNULLP (w) ? get_c_long (w) : -1;
  if (width > 100)
    my_err ("width too long", w);
  prec = NNULLP (p) ? get_c_long (p) : -1;
  if (prec > 100)
    my_err ("precision too large", p);
  if (NULLP (b) || EQ (sym_e, b) || EQ (sym_f, b))
    {
      char format[32];

      if ((width >= 0) && (prec >= 0))
        sprintf (format,
                 NULLP (b) ? "%%%ld.%ldg" :
                 EQ (sym_e, b) ? "%%%ld.%ldd" : "%%%ld.%ldf",
                 width, prec);
      else if (width >= 0)
        sprintf (format,
                 NULLP (b) ? "%%%ldg" : EQ (sym_e, b) ? "%%%lde" : "%%%ldf",
                 width);
      else if (prec >= 0)
        sprintf (format,
                 NULLP (b) ? "%%.%ldg" : EQ (sym_e, b) ? "%%.%lde" : "%%.%ldf",
                 prec);
      else
        sprintf (format, NULLP (b) ? "%%g" : EQ (sym_e, b) ? "%%e" : "%%f");

      g_ascii_formatd (buffer, sizeof(buffer), format, y);
    }
  else if (((base = get_c_long (b)) == 10) || (base == 8) || (base == 16))
    {
      if (width >= 0)
        sprintf (buffer,
                 (base == 10) ? "%0*ld" : (base == 8) ? "%0*lo" : "%0*lX",
                 (int) width,
                 (long) y);
      else
        sprintf (buffer,
                 (base == 10) ? "%ld" : (base == 8) ? "%lo" : "%lX",
                 (long) y);
    }
  else
    my_err ("number base not handled", b);
  return (strcons (strlen (buffer), buffer));
}

LISP
string2number (LISP x, LISP b)
{
  char *str;
  long base, value = 0;
  double result = 0.0;
  str = get_c_string (x);
  if NULLP
    (b)
      result = g_ascii_strtod (str, NULL);
  else if ((base = get_c_long (b)) == 10)
    {
      sscanf (str, "%ld", &value);
      result = (double) value;
    }
  else if (base == 8)
    {
      sscanf (str, "%lo", &value);
      result = (double) value;
    }
  else if (base == 16)
    {
      sscanf (str, "%lx", &value);
      result = (double) value;
    }
  else if ((base >= 1) && (base <= 16))
    {
      for (result = 0.0; *str; ++str)
        if (g_ascii_isdigit (*str))
          result = result * base + *str - '0';
        else if (g_ascii_isxdigit (*str))
          result = result * base + g_ascii_toupper (*str) - 'A' + 10;
    }
  else
    my_err ("number base not handled", b);
  return (flocons (result));
}

LISP
lstrcmp (LISP s1, LISP s2)
{
  return (flocons (strcmp (get_c_string (s1), get_c_string (s2))));
}

void
chk_string (LISP s, char **data, long *dim)
{
  if TYPEP
    (s, tc_string)
    {
      *data = s->storage_as.string.data;
      *dim = s->storage_as.string.dim;
    }
  else
    err_wta_str (s);
}

LISP
lstrcpy (LISP dest, LISP src)
{
  long ddim, slen;
  char *d, *s;
  chk_string (dest, &d, &ddim);
  s = get_c_string (src);
  slen = strlen (s);
  if (slen > ddim)
    my_err ("string too long", src);
  memcpy (d, s, slen);
  d[slen] = 0;
  return (NIL);
}

LISP
lstrcat (LISP dest, LISP src)
{
  long ddim, dlen, slen;
  char *d, *s;
  chk_string (dest, &d, &ddim);
  s = get_c_string (src);
  slen = strlen (s);
  dlen = strlen (d);
  if ((slen + dlen) > ddim)
    my_err ("string too long", src);
  memcpy (&d[dlen], s, slen);
  d[dlen + slen] = 0;
  return (NIL);
}

LISP
lstrbreakup (LISP str, LISP lmarker)
{
  char *start, *end, *marker;
  size_t k;
  LISP result = NIL;
  start = end = get_c_string (str);
  marker = get_c_string (lmarker);
  k = strlen (marker);
  if (*marker)
    {
      while (*end)
        {
          if (!(end = strstr (start, marker)))
            end = &start[strlen (start)];
          result = cons (strcons (end - start, start), result);
          start = (*end) ? end + k : end;
        }
      return (nreverse (result));
    }
  else
    return (strcons (strlen (start), start));
}

LISP
lstrunbreakup (LISP elems, LISP lmarker)
{
  LISP result, l;
  for (l = elems, result = NIL; NNULLP (l); l = cdr (l))
    if EQ
      (l, elems)
        result = cons (car (l), result);
    else
      result = cons (car (l), cons (lmarker, result));
  return (string_append (nreverse (result)));
}

LISP
stringp (LISP x)
{
  return (TYPEP (x, tc_string) ? sym_t : NIL);
}

static char *base64_encode_table = "\
ABCDEFGHIJKLMNOPQRSTUVWXYZ\
abcdefghijklmnopqrstuvwxyz\
0123456789+/=";

static char *base64_decode_table = NULL;

static void
init_base64_table (void)
{
  int j;
  base64_decode_table = (char *) malloc (256);
  memset (base64_decode_table, -1, 256);
  for (j = 0; j < 65; ++j)
    base64_decode_table[(unsigned char) base64_encode_table[j]] = j;
}

#define BITMSK(N) ((1 << (N)) - 1)

#define ITEM1(X)   (X >> 2) & BITMSK(6)
#define ITEM2(X,Y) ((X & BITMSK(2)) << 4) | ((Y >> 4) & BITMSK(4))
#define ITEM3(X,Y) ((X & BITMSK(4)) << 2) | ((Y >> 6) & BITMSK(2))
#define ITEM4(X)   X & BITMSK(6)

LISP
base64encode (LISP in)
{
  char *s, *t = base64_encode_table;
  unsigned char *p1, *p2;
  LISP out;
  long j, m, n, chunks, leftover;
  s = get_c_string_dim (in, &n);
  chunks = n / 3;
  leftover = n % 3;
  m = (chunks + ((leftover) ? 1 : 0)) * 4;
  out = strcons (m, NULL);
  p2 = (unsigned char *) get_c_string (out);
  for (j = 0, p1 = (unsigned char *) s; j < chunks; ++j, p1 += 3)
    {
      *p2++ = t[ITEM1 (p1[0])];
      *p2++ = t[ITEM2 (p1[0], p1[1])];
      *p2++ = t[ITEM3 (p1[1], p1[2])];
      *p2++ = t[ITEM4 (p1[2])];
    }
  switch (leftover)
    {
    case 0:
      break;
    case 1:
      *p2++ = t[ITEM1 (p1[0])];
      *p2++ = t[ITEM2 (p1[0], 0)];
      *p2++ = base64_encode_table[64];
      *p2++ = base64_encode_table[64];
      break;
    case 2:
      *p2++ = t[ITEM1 (p1[0])];
      *p2++ = t[ITEM2 (p1[0], p1[1])];
      *p2++ = t[ITEM3 (p1[1], 0)];
      *p2++ = base64_encode_table[64];
      break;
    default:
      errswitch ();
    }
  return (out);
}

LISP
base64decode (LISP in)
{
  char *s, *t = base64_decode_table;
  LISP out;
  unsigned char *p1, *p2;
  long j, m, n, chunks, leftover, item1, item2, item3, item4;
  s = get_c_string (in);
  n = strlen (s);
  if (n == 0)
    return (strcons (0, NULL));
  if (n % 4)
    my_err ("illegal base64 data length", in);
  if (s[n - 1] == base64_encode_table[64])
    {
      if (s[n - 2] == base64_encode_table[64])
        leftover = 1;
      else
        leftover = 2;
    }
  else
    leftover = 0;
  chunks = (n / 4) - ((leftover) ? 1 : 0);
  m = (chunks * 3) + leftover;
  out = strcons (m, NULL);
  p2 = (unsigned char *) get_c_string (out);
  for (j = 0, p1 = (unsigned char *) s; j < chunks; ++j, p1 += 4)
    {
      if ((item1 = t[p1[0]]) & ~BITMSK (6))
        return (NIL);
      if ((item2 = t[p1[1]]) & ~BITMSK (6))
        return (NIL);
      if ((item3 = t[p1[2]]) & ~BITMSK (6))
        return (NIL);
      if ((item4 = t[p1[3]]) & ~BITMSK (6))
        return (NIL);
      *p2++ = (item1 << 2) | (item2 >> 4);
      *p2++ = (item2 << 4) | (item3 >> 2);
      *p2++ = (item3 << 6) | item4;
    }
  switch (leftover)
    {
    case 0:
      break;
    case 1:
      if ((item1 = t[p1[0]]) & ~BITMSK (6))
        return (NIL);
      if ((item2 = t[p1[1]]) & ~BITMSK (6))
        return (NIL);
      *p2++ = (item1 << 2) | (item2 >> 4);
      break;
    case 2:
      if ((item1 = t[p1[0]]) & ~BITMSK (6))
        return (NIL);
      if ((item2 = t[p1[1]]) & ~BITMSK (6))
        return (NIL);
      if ((item3 = t[p1[2]]) & ~BITMSK (6))
        return (NIL);
      *p2++ = (item1 << 2) | (item2 >> 4);
      *p2++ = (item2 << 4) | (item3 >> 2);
      break;
    default:
      errswitch ();
    }
  return (out);
}

LISP
memq (LISP x, LISP il)
{
  LISP l, tmp;
  for (l = il; CONSP (l); l = CDR (l))
    {
      tmp = CAR (l);
      if EQ
        (x, tmp) return (l);
      INTERRUPT_CHECK ();
    }
  if EQ
    (l, NIL) return (NIL);
  return (my_err ("improper list to memq", il));
}

LISP
member (LISP x, LISP il)
{
  LISP l, tmp;
  for (l = il; CONSP (l); l = CDR (l))
    {
      tmp = CAR (l);
      if NNULLP
        (equal (x, tmp)) return (l);
      INTERRUPT_CHECK ();
    }
  if EQ
    (l, NIL) return (NIL);
  return (my_err ("improper list to member", il));
}

LISP
memv (LISP x, LISP il)
{
  LISP l, tmp;
  for (l = il; CONSP (l); l = CDR (l))
    {
      tmp = CAR (l);
      if NNULLP
        (eql (x, tmp)) return (l);
      INTERRUPT_CHECK ();
    }
  if EQ
    (l, NIL) return (NIL);
  return (my_err ("improper list to memv", il));
}


LISP
nth (LISP x, LISP li)
{
  LISP l;
  long j, n = get_c_long (x);
  for (j = 0, l = li; (j < n) && CONSP (l); ++j)
    l = CDR (l);
  if CONSP
    (l)
      return (CAR (l));
  else
    return (my_err ("bad arg to nth", x));
}

/* these lxxx_default functions are convenient for manipulating
   command-line argument lists */

LISP
lref_default (LISP li, LISP x, LISP fcn)
{
  LISP l;
  long j, n = get_c_long (x);
  for (j = 0, l = li; (j < n) && CONSP (l); ++j)
    l = CDR (l);
  if CONSP
    (l)
      return (CAR (l));
  else if NNULLP
    (fcn)
      return (lapply (fcn, NIL));
  else
    return (NIL);
}

LISP
larg_default (LISP li, LISP x, LISP dval)
{
  LISP l = li, elem;
  long j = 0, n = get_c_long (x);
  while NNULLP
    (l)
    {
      elem = car (l);
      if (TYPEP (elem, tc_string) && strchr ("-:", *get_c_string (elem)))
        l = cdr (l);
      else if (j == n)
        return (elem);
      else
        {
          l = cdr (l);
          ++j;
        }
    }
  return (dval);
}

LISP
lkey_default (LISP li, LISP key, LISP dval)
{
  LISP l = li, elem;
  char *ckey, *celem;
  long n;
  ckey = get_c_string (key);
  n = strlen (ckey);
  while NNULLP
    (l)
    {
      elem = car (l);
      if (TYPEP (elem, tc_string) && (*(celem = get_c_string (elem)) == ':') &&
          (strncmp (&celem[1], ckey, n) == 0) && (celem[n + 1] == '='))
        return (strcons (strlen (&celem[n + 2]), &celem[n + 2]));
      l = cdr (l);
    }
  return (dval);
}


LISP
llist (LISP l)
{
  return (l);
}

LISP
writes1 (FILE * f, LISP l)
{
  LISP v;
  STACK_CHECK (&v);
  INTERRUPT_CHECK ();
  for (v = l; CONSP (v); v = CDR (v))
    writes1 (f, CAR (v));
  switch TYPE
    (v)
    {
    case tc_nil:
      break;
    case tc_symbol:
    case tc_string:
      fput_st (f, get_c_string (v));
      break;
    default:
      lprin1f (v, f);
      break;
    }
  return (NIL);
}

LISP
writes (LISP args)
{
  return (writes1 (get_c_file (car (args), stdout), cdr (args)));
}

LISP
last (LISP l)
{
  LISP v1, v2;
  v1 = l;
  v2 = CONSP (v1) ? CDR (v1) : my_err ("bad arg to last", l);
  while (CONSP (v2))
    {
      INTERRUPT_CHECK ();
      v1 = v2;
      v2 = CDR (v2);
    }
  return (v1);
}

LISP
butlast (LISP l)
{
  INTERRUPT_CHECK ();
  STACK_CHECK (&l);
  if NULLP
    (l) my_err ("list is empty", l);
  if CONSP (l)
    {
      if NULLP (CDR (l))
        return (NIL);
      else
        return (cons (CAR (l), butlast (CDR (l))));
    }
  return (my_err ("not a list", l));
}

LISP
nconc (LISP a, LISP b)
{
  if NULLP
    (a)
      return (b);
  setcdr (last (a), b);
  return (a);
}

LISP
funcall1 (LISP fcn, LISP a1)
{
  switch TYPE
    (fcn)
    {
    case tc_subr_1:
      STACK_CHECK (&fcn);
      INTERRUPT_CHECK ();
      return (SUBR1 (fcn) (a1));
    case tc_closure:
      if TYPEP
        (fcn->storage_as.closure.code, tc_subr_2)
        {
          STACK_CHECK (&fcn);
          INTERRUPT_CHECK ();
          return (SUBR2 (fcn->storage_as.closure.code)
                  (fcn->storage_as.closure.env, a1));
        }
    default:
      return (lapply (fcn, cons (a1, NIL)));
    }
}

LISP
funcall2 (LISP fcn, LISP a1, LISP a2)
{
  switch TYPE
    (fcn)
    {
    case tc_subr_2:
    case tc_subr_2n:
      STACK_CHECK (&fcn);
      INTERRUPT_CHECK ();
      return (SUBR2 (fcn) (a1, a2));
    default:
      return (lapply (fcn, cons (a1, cons (a2, NIL))));
    }
}

LISP
lqsort (LISP l, LISP f, LISP g)
     /* this is a stupid recursive qsort */
{
  int j, n;
  LISP v, mark, less, notless;
  for (v = l, n = 0; CONSP (v); v = CDR (v), ++n)
    INTERRUPT_CHECK ();
  if NNULLP
    (v) my_err ("bad list to qsort", l);
  if (n == 0)
    return (NIL);
  j = rand () % n;
  for (v = l, n = 0; n < j; ++n)
    v = CDR (v);
  mark = CAR (v);
  for (less = NIL, notless = NIL, v = l, n = 0; NNULLP (v); v = CDR (v), ++n)
    if (j != n)
      {
        if NNULLP
          (funcall2 (f,
                     NULLP (g) ? CAR (v) : funcall1 (g, CAR (v)),
                     NULLP (g) ? mark : funcall1 (g, mark)))
            less = cons (CAR (v), less);
        else
          notless = cons (CAR (v), notless);
      }
  return (nconc (lqsort (less, f, g),
                 cons (mark,
                       lqsort (notless, f, g))));
}

LISP
string_lessp (LISP s1, LISP s2)
{
  if (strcmp (get_c_string (s1), get_c_string (s2)) < 0)
    return (sym_t);
  else
    return (NIL);
}

LISP
benchmark_funcall1 (LISP ln, LISP f, LISP a1)
{
  long j, n;
  LISP value = NIL;
  n = get_c_long (ln);
  for (j = 0; j < n; ++j)
    value = funcall1 (f, a1);
  return (value);
}

LISP
benchmark_funcall2 (LISP l)
{
  long j, n;
  LISP ln = car (l);
  LISP f = car (cdr (l));
  LISP a1 = car (cdr (cdr (l)));
  LISP a2 = car (cdr (cdr (cdr (l))));
  LISP value = NULL;
  n = get_c_long (ln);
  for (j = 0; j < n; ++j)
    value = funcall2 (f, a1, a2);
  return (value);
}

LISP
benchmark_eval (LISP ln, LISP exp, LISP env)
{
  long j, n;
  LISP value = NIL;
  n = get_c_long (ln);
  for (j = 0; j < n; ++j)
    value = leval (exp, env);
  return (value);
}

LISP
mapcar1 (LISP fcn, LISP in)
{
  LISP res, ptr, l;
  if NULLP
    (in) return (NIL);
  res = ptr = cons (funcall1 (fcn, car (in)), NIL);
  for (l = cdr (in); CONSP (l); l = CDR (l))
    ptr = CDR (ptr) = cons (funcall1 (fcn, CAR (l)), CDR (ptr));
  return (res);
}

LISP
mapcar2 (LISP fcn, LISP in1, LISP in2)
{
  LISP res, ptr, l1, l2;
  if (NULLP (in1) || NULLP (in2))
    return (NIL);
  res = ptr = cons (funcall2 (fcn, car (in1), car (in2)), NIL);
  for (l1 = cdr (in1), l2 = cdr (in2); CONSP (l1) && CONSP (l2); l1 = CDR (l1), l2 = CDR (l2))
    ptr = CDR (ptr) = cons (funcall2 (fcn, CAR (l1), CAR (l2)), CDR (ptr));
  return (res);
}

LISP
mapcar (LISP l)
{
  LISP fcn = car (l);
  switch (get_c_long (llength (l)))
    {
    case 2:
      return (mapcar1 (fcn, car (cdr (l))));
    case 3:
      return (mapcar2 (fcn, car (cdr (l)), car (cdr (cdr (l)))));
    default:
      return (my_err ("mapcar case not handled", l));
    }
}

LISP
lfmod (LISP x, LISP y)
{
  if NFLONUMP
    (x) my_err ("wta(1st) to fmod", x);
  if NFLONUMP
    (y) my_err ("wta(2nd) to fmod", y);
  return (flocons (fmod (FLONM (x), FLONM (y))));
}

LISP
lsubset (LISP fcn, LISP l)
{
  LISP result = NIL, v;
  for (v = l; CONSP (v); v = CDR (v))
    if NNULLP
      (funcall1 (fcn, CAR (v)))
        result = cons (CAR (v), result);
  return (nreverse (result));
}

LISP
ass (LISP x, LISP alist, LISP fcn)
{
  LISP l, tmp;
  for (l = alist; CONSP (l); l = CDR (l))
    {
      tmp = CAR (l);
      if (CONSP (tmp) && NNULLP (funcall2 (fcn, CAR (tmp), x)))
        return (tmp);
      INTERRUPT_CHECK ();
    }
  if EQ
    (l, NIL) return (NIL);
  return (my_err ("improper list to ass", alist));
}

LISP
append2 (LISP l1, LISP l2)
{
  long n;
  LISP result = NIL, p1, p2;
  n = nlength (l1) + nlength (l2);
  while (n > 0)
    {
      result = cons (NIL, result);
      --n;
    }
  for (p1 = result, p2 = l1; NNULLP (p2); p1 = cdr (p1), p2 = cdr (p2))
    setcar (p1, car (p2));
  for (p2 = l2; NNULLP (p2); p1 = cdr (p1), p2 = cdr (p2))
    setcar (p1, car (p2));
  return (result);
}

LISP
append (LISP l)
{
  STACK_CHECK (&l);
  INTERRUPT_CHECK ();
  if NULLP
    (l)
      return (NIL);
  else if NULLP
    (cdr (l))
      return (car (l));
  else if NULLP
    (cddr (l))
      return (append2 (car (l), cadr (l)));
  else
    return (append2 (car (l), append (cdr (l))));
}

LISP
listn (long n,...)
{
  LISP result, ptr;
  long j;
  va_list args;
  for (j = 0, result = NIL; j < n; ++j)
    result = cons (NIL, result);
  va_start (args, n);
  for (j = 0, ptr = result; j < n; ptr = cdr (ptr), ++j)
    setcar (ptr, va_arg (args, LISP));
  va_end (args);
  return (result);
}


LISP
fast_load (LISP lfname, LISP noeval)
{
  char *fname;
  LISP stream;
  LISP result = NIL, form;
  fname = get_c_string (lfname);
  if (siod_verbose_level >= 3)
    {
      put_st ("fast loading ");
      put_st (fname);
      put_st ("\n");
    }
  stream = listn (3,
                  fopen_c (fname, "rb"),
                  cons_array (flocons (100), NIL),
                  flocons (0));
  while (NEQ (stream, form = fast_read (stream)))
    {
      if (siod_verbose_level >= 5)
        lprint (form, NIL);
      if NULLP
        (noeval)
          leval (form, NIL);
      else
        result = cons (form, result);
    }
  fclose_l (car (stream));
  if (siod_verbose_level >= 3)
    put_st ("done.\n");
  return (nreverse (result));
}

static void
shexstr (char *outstr, void *buff, size_t len)
{
  unsigned char *data = buff;
  size_t j;
  for (j = 0; j < len; ++j)
    sprintf (&outstr[j * 2], "%02X", data[j]);
}

LISP
fast_save (LISP fname, LISP forms, LISP nohash, LISP comment)
{
  char *cname, msgbuff[100], databuff[50];
  LISP stream, l;
  FILE *f;
  long l_one = 1;
  double d_one = 1.0;
  cname = get_c_string (fname);
  if (siod_verbose_level >= 3)
    {
      put_st ("fast saving forms to ");
      put_st (cname);
      put_st ("\n");
    }
  stream = listn (3,
                  fopen_c (cname, "wb"),
                  NNULLP (nohash) ? NIL : cons_array (flocons (100), NIL),
                  flocons (0));
  f = get_c_file (car (stream), NULL);
  if NNULLP
    (comment)
      fput_st (f, get_c_string (comment));
  sprintf (msgbuff, "# Siod Binary Object Save File\n");
  fput_st (f, msgbuff);
  sprintf (msgbuff, "# sizeof(long) = %d\n# sizeof(double) = %d\n",
           (int) sizeof (long), (int) sizeof (double));
  fput_st (f, msgbuff);
  shexstr (databuff, &l_one, sizeof (l_one));
  sprintf (msgbuff, "# 1 = %s\n", databuff);
  fput_st (f, msgbuff);
  shexstr (databuff, &d_one, sizeof (d_one));
  sprintf (msgbuff, "# 1.0 = %s\n", databuff);
  fput_st (f, msgbuff);
  for (l = forms; NNULLP (l); l = cdr (l))
    fast_print (car (l), stream);
  fclose_l (car (stream));
  if (siod_verbose_level >= 3)
    put_st ("done.\n");
  return (NIL);
}

void
swrite1 (LISP stream, LISP data)
{
  FILE *f = get_c_file (stream, stdout);
  switch TYPE
    (data)
    {
    case tc_symbol:
    case tc_string:
      fput_st (f, get_c_string (data));
      break;
    default:
      lprin1f (data, f);
      break;
    }
}

LISP
swrite (LISP stream, LISP table, LISP data)
{
  LISP value, key;
  long j, k, m, n;
  switch (TYPE (data))
    {
    case tc_symbol:
      value = href (table, data);
      if CONSP
        (value)
        {
          swrite1 (stream, CAR (value));
          if NNULLP
            (CDR (value))
              hset (table, data, CDR (value));
        }
      else
        swrite1 (stream, value);
      break;
    case tc_lisp_array:
      n = data->storage_as.lisp_array.dim;
      if (n < 1)
        my_err ("no object repeat count", data);
      key = data->storage_as.lisp_array.data[0];
      if NULLP
        (value = href (table, key))
          value = key;
      else if CONSP
        (value)
        {
          if NNULLP
            (CDR (value))
              hset (table, key, CDR (value));
          value = CAR (value);
        }
      m = get_c_long (value);
      for (k = 0; k < m; ++k)
        for (j = 1; j < n; ++j)
          swrite (stream, table, data->storage_as.lisp_array.data[j]);
      break;
    case tc_cons:
      /* this should be handled similar to the array case */
    default:
      swrite1 (stream, data);
    }
  return (NIL);
}

LISP
ltrunc (LISP x)
{
  double cx = get_c_double (x);
  return (flocons (cx < 0.0 ? ceil (cx) : floor (cx)));
}

LISP
lpow (LISP x, LISP y)
{
  if NFLONUMP
    (x) my_err ("wta(1st) to pow", x);
  if NFLONUMP
    (y) my_err ("wta(2nd) to pow", y);
  return (flocons (pow (FLONM (x), FLONM (y))));
}

LISP
lexp (LISP x)
{
  return (flocons (exp (get_c_double (x))));
}

LISP
llog (LISP x)
{
  return (flocons (log (get_c_double (x))));
}

LISP
lsin (LISP x)
{
  return (flocons (sin (get_c_double (x))));
}

LISP
lcos (LISP x)
{
  return (flocons (cos (get_c_double (x))));
}

LISP
ltan (LISP x)
{
  return (flocons (tan (get_c_double (x))));
}

LISP
lasin (LISP x)
{
  return (flocons (asin (get_c_double (x))));
}

LISP
lacos (LISP x)
{
  return (flocons (acos (get_c_double (x))));
}

LISP
latan (LISP x)
{
  return (flocons (atan (get_c_double (x))));
}

LISP
latan2 (LISP x, LISP y)
{
  return (flocons (atan2 (get_c_double (x), get_c_double (y))));
}

LISP
hexstr (LISP a)
{
  unsigned char *in;
  char *out;
  LISP result;
  long j, dim;
  in = (unsigned char *) get_c_string_dim (a, &dim);
  result = strcons (dim * 2, NULL);
  for (out = get_c_string (result), j = 0; j < dim; ++j, out += 2)
    sprintf (out, "%02x", in[j]);
  return (result);
}

static int
xdigitvalue (int c)
{
  if (g_ascii_isdigit (c))
      return (c - '0');
  if (g_ascii_isxdigit (c))
      return (g_ascii_toupper (c) - 'A' + 10);
  return (0);
}

LISP
hexstr2bytes (LISP a)
{
  char *in;
  unsigned char *out;
  LISP result;
  long j, dim;
  in = get_c_string (a);
  dim = strlen (in) / 2;
  result = arcons (tc_byte_array, dim, 0);
  out = (unsigned char *) result->storage_as.string.data;
  for (j = 0; j < dim; ++j)
    out[j] = xdigitvalue (in[j * 2]) * 16 + xdigitvalue (in[j * 2 + 1]);
  return (result);
}

LISP
getprop (LISP plist, LISP key)
{
  LISP l;
  for (l = cdr (plist); NNULLP (l); l = cddr (l))
    if EQ
      (car (l), key)
        return (cadr (l));
    else
      INTERRUPT_CHECK ();
  return (NIL);
}

LISP
setprop (LISP plist, LISP key, LISP value)
{
  my_err ("not implemented", NIL);
  return (NIL);
}

LISP
putprop (LISP plist, LISP value, LISP key)
{
  return (setprop (plist, key, value));
}

LISP
ltypeof (LISP obj)
{
  long x;
  x = TYPE (obj);
  switch (x)
    {
    case tc_nil:
      return (cintern ("tc_nil"));
    case tc_cons:
      return (cintern ("tc_cons"));
    case tc_flonum:
      return (cintern ("tc_flonum"));
    case tc_symbol:
      return (cintern ("tc_symbol"));
    case tc_subr_0:
      return (cintern ("tc_subr_0"));
    case tc_subr_1:
      return (cintern ("tc_subr_1"));
    case tc_subr_2:
      return (cintern ("tc_subr_2"));
    case tc_subr_2n:
      return (cintern ("tc_subr_2n"));
    case tc_subr_3:
      return (cintern ("tc_subr_3"));
    case tc_subr_4:
      return (cintern ("tc_subr_4"));
    case tc_subr_5:
      return (cintern ("tc_subr_5"));
    case tc_lsubr:
      return (cintern ("tc_lsubr"));
    case tc_fsubr:
      return (cintern ("tc_fsubr"));
    case tc_msubr:
      return (cintern ("tc_msubr"));
    case tc_closure:
      return (cintern ("tc_closure"));
    case tc_free_cell:
      return (cintern ("tc_free_cell"));
    case tc_string:
      return (cintern ("tc_string"));
    case tc_byte_array:
      return (cintern ("tc_byte_array"));
    case tc_double_array:
      return (cintern ("tc_double_array"));
    case tc_long_array:
      return (cintern ("tc_long_array"));
    case tc_lisp_array:
      return (cintern ("tc_lisp_array"));
    case tc_c_file:
      return (cintern ("tc_c_file"));
    default:
      return (flocons (x));
    }
}

LISP
caaar (LISP x)
{
  return (car (car (car (x))));
}

LISP
caadr (LISP x)
{
  return (car (car (cdr (x))));
}

LISP
cadar (LISP x)
{
  return (car (cdr (car (x))));
}

LISP
caddr (LISP x)
{
  return (car (cdr (cdr (x))));
}

LISP
cdaar (LISP x)
{
  return (cdr (car (car (x))));
}

LISP
cdadr (LISP x)
{
  return (cdr (car (cdr (x))));
}

LISP
cddar (LISP x)
{
  return (cdr (cdr (car (x))));
}

LISP
cdddr (LISP x)
{
  return (cdr (cdr (cdr (x))));
}

LISP
ash (LISP value, LISP n)
{
  long m, k;
  m = get_c_long (value);
  k = get_c_long (n);
  if (k > 0)
    m = m << k;
  else
    m = m >> (-k);
  return (flocons (m));
}

LISP
bitand (LISP a, LISP b)
{
  return (flocons (get_c_long (a) & get_c_long (b)));
}

LISP
bitor (LISP a, LISP b)
{
  return (flocons (get_c_long (a) | get_c_long (b)));
}

LISP
bitxor (LISP a, LISP b)
{
  return (flocons (get_c_long (a) ^ get_c_long (b)));
}

LISP
bitnot (LISP a)
{
  return (flocons (~get_c_long (a)));
}

LISP
leval_prog1 (LISP args, LISP env)
{
  LISP retval, l;
  retval = leval (car (args), env);
  for (l = cdr (args); NNULLP (l); l = cdr (l))
    leval (car (l), env);
  return (retval);
}

LISP
leval_cond (LISP * pform, LISP * penv)
{
  LISP args, env, clause, value, next;
  args = cdr (*pform);
  env = *penv;
  if NULLP
    (args)
    {
      *pform = NIL;
      return (NIL);
    }
  next = cdr (args);
  while NNULLP
    (next)
    {
      clause = car (args);
      value = leval (car (clause), env);
      if NNULLP
        (value)
        {
          clause = cdr (clause);
          if NULLP
            (clause)
            {
              *pform = value;
              return (NIL);
            }
          else
            {
              next = cdr (clause);
              while (NNULLP (next))
                {
                  leval (car (clause), env);
                  clause = next;
                  next = cdr (next);
                }
              *pform = car (clause);
              return (sym_t);
            }
        }
      args = next;
      next = cdr (next);
    }
  clause = car (args);
  next = cdr (clause);
  if NULLP
    (next)
    {
      *pform = car (clause);
      return (sym_t);
    }
  value = leval (car (clause), env);
  if NULLP
    (value)
    {
      *pform = NIL;
      return (NIL);
    }
  clause = next;
  next = cdr (next);
  while (NNULLP (next))
    {
      leval (car (clause), env);
      clause = next;
      next = cdr (next);
    }
  *pform = car (clause);
  return (sym_t);
}

LISP
lstrspn (LISP str1, LISP str2)
{
  return (flocons (strspn (get_c_string (str1), get_c_string (str2))));
}

LISP
lstrcspn (LISP str1, LISP str2)
{
  return (flocons (strcspn (get_c_string (str1), get_c_string (str2))));
}

LISP
substring_equal (LISP str1, LISP str2, LISP start, LISP end)
{
  char *cstr1, *cstr2;
  long len1, n, s, e;
  cstr1 = get_c_string_dim (str1, &len1);
  cstr2 = get_c_string_dim (str2, &n);
  s = NULLP (start) ? 0 : get_c_long (start);
  e = NULLP (end) ? n : get_c_long (end);
  if ((s < 0) || (s > e) || (e < 0) || (e > n) || ((e - s) != len1))
    return (NIL);
  return ((memcmp (cstr1, &cstr2[s], e - s) == 0) ? a_true_value () : NIL);
}

LISP
set_eval_history (LISP len, LISP circ)
{
  LISP data;
  data = NULLP (len) ? len : make_list (len, NIL);
  if NNULLP
    (circ)
      data = nconc (data, data);
  setvar (cintern ("*eval-history-ptr*"), data, NIL);
  setvar (cintern ("*eval-history*"), data, NIL);
  return (len);
}

static LISP
parser_fasl (LISP ignore)
{
  return (closure (listn (3,
                          NIL,
                          cons_array (flocons (100), NIL),
                          flocons (0)),
                   leval (cintern ("parser_fasl_hook"), NIL)));
}

static LISP
parser_fasl_hook (LISP env, LISP f)
{
  LISP result;
  setcar (env, f);
  result = fast_read (env);
  if EQ
    (result, env)
      return (get_eof_val ());
  else
    return (result);
}

void
init_subrs_a (void)
{
  init_subr_2 ("aref", aref1);
  init_subr_3 ("aset", aset1);
  init_lsubr ("string-append", string_append);
  init_lsubr ("bytes-append", bytes_append);
  init_subr_1 ("string-length", string_length);
  init_subr_1 ("string-dimension", string_dim);
  init_subr_1 ("read-from-string", read_from_string);
  init_subr_3 ("print-to-string", print_to_string);
  init_subr_2 ("cons-array", cons_array);
  init_subr_2 ("sxhash", sxhash);
  init_subr_2 ("equal?", equal);
  init_subr_2 ("href", href);
  init_subr_3 ("hset", hset);
  init_subr_2 ("assoc", assoc);
  init_subr_2 ("assv", assv);
  init_subr_1 ("fast-read", fast_read);
  init_subr_2 ("fast-print", fast_print);
  init_subr_2 ("make-list", make_list);
  init_subr_2 ("fread", lfread);
  init_subr_2 ("fwrite", lfwrite);
  init_subr_1 ("fflush", lfflush);
  init_subr_1 ("length", llength);
  init_subr_4 ("number->string", number2string);
  init_subr_2 ("string->number", string2number);
  init_subr_3 ("substring", substring);
  init_subr_2 ("string-search", string_search);
  init_subr_1 ("string-trim", string_trim);
  init_subr_1 ("string-trim-left", string_trim_left);
  init_subr_1 ("string-trim-right", string_trim_right);
  init_subr_1 ("string-upcase", string_upcase);
  init_subr_1 ("string-downcase", string_downcase);
  init_subr_2 ("strcmp", lstrcmp);
  init_subr_2 ("strcat", lstrcat);
  init_subr_2 ("strcpy", lstrcpy);
  init_subr_2 ("strbreakup", lstrbreakup);
  init_subr_2 ("unbreakupstr", lstrunbreakup);
  init_subr_1 ("string?", stringp);
  gc_protect_sym (&sym_e, "e");
  gc_protect_sym (&sym_f, "f");
  gc_protect_sym (&sym_plists, "*plists*");
  setvar (sym_plists, arcons (tc_lisp_array, 100, 1), NIL);
  init_subr_3 ("lref-default", lref_default);
  init_subr_3 ("larg-default", larg_default);
  init_subr_3 ("lkey-default", lkey_default);
  init_lsubr ("list", llist);
  init_lsubr ("writes", writes);
  init_subr_3 ("qsort", lqsort);
  init_subr_2 ("string-lessp", string_lessp);
  init_lsubr ("mapcar", mapcar);
  init_subr_3 ("mapcar2", mapcar2);
  init_subr_2 ("mapcar1", mapcar1);
  init_subr_3 ("benchmark-funcall1", benchmark_funcall1);
  init_lsubr ("benchmark-funcall2", benchmark_funcall2);
  init_subr_3 ("benchmark-eval", benchmark_eval);
  init_subr_2 ("fmod", lfmod);
  init_subr_2 ("subset", lsubset);
  init_subr_1 ("base64encode", base64encode);
  init_subr_1 ("base64decode", base64decode);
  init_subr_3 ("ass", ass);
  init_subr_2 ("append2", append2);
  init_lsubr ("append", append);
  init_subr_4 ("fast-save", fast_save);
  init_subr_2 ("fast-load", fast_load);
  init_subr_3 ("swrite", swrite);
  init_subr_1 ("trunc", ltrunc);
  init_subr_2 ("pow", lpow);
  init_subr_1 ("exp", lexp);
  init_subr_1 ("log", llog);
  init_subr_1 ("sin", lsin);
  init_subr_1 ("cos", lcos);
  init_subr_1 ("tan", ltan);
  init_subr_1 ("asin", lasin);
  init_subr_1 ("acos", lacos);
  init_subr_1 ("atan", latan);
  init_subr_2 ("atan2", latan2);
  init_subr_1 ("typeof", ltypeof);
  init_subr_1 ("caaar", caaar);
  init_subr_1 ("caadr", caadr);
  init_subr_1 ("cadar", cadar);
  init_subr_1 ("caddr", caddr);
  init_subr_1 ("cdaar", cdaar);
  init_subr_1 ("cdadr", cdadr);
  init_subr_1 ("cddar", cddar);
  init_subr_1 ("cdddr", cdddr);
  setvar (cintern ("*pi*"), flocons (atan (1.0) * 4), NIL);
  init_base64_table ();
  init_subr_1 ("array->hexstr", hexstr);
  init_subr_1 ("hexstr->bytes", hexstr2bytes);
  init_subr_3 ("ass", ass);
  init_subr_2 ("bit-and", bitand);
  init_subr_2 ("bit-or", bitor);
  init_subr_2 ("bit-xor", bitxor);
  init_subr_1 ("bit-not", bitnot);
  init_msubr ("cond", leval_cond);
  init_fsubr ("prog1", leval_prog1);
  init_subr_2 ("strspn", lstrspn);
  init_subr_2 ("strcspn", lstrcspn);
  init_subr_4 ("substring-equal?", substring_equal);
  init_subr_1 ("butlast", butlast);
  init_subr_2 ("ash", ash);
  init_subr_2 ("get", getprop);
  init_subr_3 ("setprop", setprop);
  init_subr_3 ("putprop", putprop);
  init_subr_1 ("last", last);
  init_subr_2 ("memq", memq);
  init_subr_2 ("memv", memv);
  init_subr_2 ("member", member);
  init_subr_2 ("nth", nth);
  init_subr_2 ("nconc", nconc);
  init_subr_2 ("set-eval-history", set_eval_history);
  init_subr_1 ("parser_fasl", parser_fasl);
  setvar (cintern ("*parser_fasl.scm-loaded*"), a_true_value (), NIL);
  init_subr_2 ("parser_fasl_hook", parser_fasl_hook);
  init_sliba_version ();
}
