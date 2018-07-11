/* global.h: extend the standard programming environment a little.  This
 *    is included from config.h, which everyone includes.
 *
 * Copyright (C) 1992 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdlib.h>

#include "types.h"

/* Define common sorts of messages.  */

/* This should be called only after a system call fails.  */
#define FATAL_PERROR(s) do { perror (s); exit (errno); } while (0)


#define START_FATAL() do { fputs ("fatal: ", stderr)
#define END_FATAL() fputs (".\n", stderr); exit (1); } while (0)

#define FATAL(x)							\
  START_FATAL (); fprintf (stderr, "%s", x); END_FATAL ()
#define FATAL1(s, e1)							\
  START_FATAL (); fprintf (stderr, s, e1); END_FATAL ()
#define FATAL2(s, e1, e2)						\
  START_FATAL (); fprintf (stderr, s, e1, e2); END_FATAL ()
#define FATAL3(s, e1, e2, e3)						\
  START_FATAL (); fprintf (stderr, s, e1, e2, e3); END_FATAL ()
#define FATAL4(s, e1, e2, e3, e4)					\
  START_FATAL (); fprintf (stderr, s, e1, e2, e3, e4); END_FATAL ()


#define START_WARNING() do { fputs ("warning: ", stderr)
#define END_WARNING() fputs (".\n", stderr); fflush (stderr); } while (0)

#define WARNING(x)							\
  START_WARNING (); fprintf (stderr, "%s", x); END_WARNING ()
#define WARNING1(s, e1)							\
  START_WARNING (); fprintf (stderr, s, e1); END_WARNING ()
#define WARNING2(s, e1, e2)						\
  START_WARNING (); fprintf (stderr, s, e1, e2); END_WARNING ()
#define WARNING3(s, e1, e2, e3)						\
  START_WARNING (); fprintf (stderr, s, e1, e2, e3); END_WARNING ()
#define WARNING4(s, e1, e2, e3, e4)					\
  START_WARNING (); fprintf (stderr, s, e1, e2, e3, e4); END_WARNING ()

/* Define useful abbreviations.  */

/* This is the maximum number of numerals that result when a 64-bit
   integer is converted to a string, plus one for a trailing null byte,
   plus one for a sign.  */
#define MAX_INT_LENGTH 21

/* Printer's points, as defined by TeX (and good typesetters everywhere).  */
#define POINTS_PER_INCH  72.27

/* Convert a number V in pixels to printer's points, and vice versa,
   assuming a resolution of DPI pixels per inch.  */
#define PIXELS_TO_POINTS(v, dpi) (POINTS_PER_INCH * (v) / (dpi))
#define POINTS_TO_REAL_PIXELS(v, dpi) ((v) * (dpi) / POINTS_PER_INCH)
#define POINTS_TO_PIXELS(v, dpi) ((int) (POINTS_TO_REAL_PIXELS (v, dpi) + .5))

/* Some simple numeric operations.  It is possible to define these much
   more cleanly in GNU C, but we haven't done that (yet).  */
#define SQUARE(x) ((x) * (x))
#define CUBE(x) ((x) * (x) * (x))
#define	SAME_SIGN(u,v) ((u) >= 0 && (v) >= 0 || (u) < 0 && (v) < 0)
#define SIGN(x) ((x) > 0 ? 1 : (x) < 0 ? -1 : 0)
#define SROUND(x) ((int) ((int) (x) + .5 * SIGN (x)))

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Too bad C doesn't define operators for these.  */
#define MAX_EQUALS(var, expr) if ((expr) > (var)) (var) = (expr)
#define MIN_EQUALS(var, expr) if ((expr) < (var)) (var) = (expr)

#define STREQ(s1, s2) (strcmp (s1, s2) == 0)

/* Declarations for commonly-used routines we provide ourselves.  The
   ones here are only needed by us, so we do not provide them in
   unprototyped form.  Others are declared both ways in lib.h.  */

#if 0				/* These aren't actually defined anywhere */
/* Return the current date and time a la date(1).  */
extern string now (void);

/* Check if a string is a valid floating-point or decimal integer.
   Returns false if passed NULL.  */
extern boolean float_ok (string);
extern boolean integer_ok (string);

/* Like `atoi', but disallow negative numbers.  */
extern const unsigned atou (string);

/* The converses of atoi, atou, and atof.  These all return dynamically
   allocated strings.  `dtoa' is so-named because `ftoa' is a library
   function on some systems (the IBM RT), and the loader complains that
   is defined twice, for reasons I don't understand.  */
extern string itoa (int);
extern string utoa (unsigned);
extern string dtoa (double);

#endif

/* Like their stdio counterparts, but abort on error, after calling
   perror(3) with FILENAME as its argument.  */
/* extern FILE *xfopen (string filename, string mode); */
/* extern void xfclose (FILE *, string filename); */
/* extern void xfseek (FILE *, long, int, string filename); */
/* extern four_bytes xftell (FILE *, string filename); */

/* Copies the file FROM to the file TO, then unlinks FROM.  */
extern void xrename (string from, string to);

/* Return NAME with any leading path stripped off.  This returns a
   pointer into NAME.  */
/* ALT extern string basename (string name); */



/* If P or *P is null, abort.  Otherwise, call free(3) on P,
   and then set *P to NULL.  */
extern void safe_free (address *p);


/* Math functions.  */

/* Says whether V1 and V2 are within REAL_EPSILON of each other.
   Fixed-point arithmetic would be better, to guarantee machine
   independence, but it's so much more painful to work with.  The value
   here is smaller than can be represented in either a `fix_word' or a
   `scaled_num', so more precision than this will be lost when we
   output, anyway.  */
#define REAL_EPSILON 0.00001
extern boolean epsilon_equal (real v1, real v2);

/* Arc cosine, in degrees.  */
extern real my_acosd (real);

/* Return the Euclidean distance between the two points.  */
extern real distance (real_coordinate_type, real_coordinate_type);
extern real int_distance (coordinate_type, coordinate_type);

/* Slope between two points (delta y per unit x).  */
extern real slope (real_coordinate_type, real_coordinate_type);

/* Make a real coordinate from an integer one, and vice versa.  */
extern real_coordinate_type int_to_real_coord (coordinate_type);
extern coordinate_type real_to_int_coord (real_coordinate_type);

/* Test if two integer points are adjacent.  */
extern boolean points_adjacent_p (int row1, int col1, int r2, int c2);

/* Find the largest and smallest elements of an array.  */
extern void find_bounds (real values[], unsigned value_count,
	                 /* returned: */ real *the_min, real *the_max);

/* Make all the elements in the array between zero and one.  */
extern real *map_to_unit (real * values, unsigned value_count);


/* String functions.  */

/* Return (a fresh copy of) SOURCE beginning at START and ending at
   LIMIT.  (Or NULL if LIMIT < START.)  */
extern string substring (string source, const unsigned start,
                         const unsigned limit);

/* Change all uppercase letters in S to lowercase.  */
extern string lowercasify (string s);


/* Character code parsing.  */

/* If the string S parses as a character code, this sets *VALID to
   `true' and returns the number.  If it doesn't, it sets *VALID to
   `false' and the return value is garbage.

   We allow any of the following possibilities: a single character, as in
   `a' or `0'; a decimal number, as in `21'; an octal number, as in `03'
   or `0177'; a hexadecimal number, as in `0x3' or `0xff'.  */
extern charcode_type parse_charcode (string s, boolean *valid);

/* Like `parse_charcode', but gives a fatal error if the string isn't a
   valid character code.  */
extern charcode_type xparse_charcode (string s);

/* The environment variable name with which to look up auxiliary files.  */
#ifndef LIB_ENVVAR
#define LIB_ENVVAR "FONTUTIL_LIB"
#endif

#endif /* not GLOBAL_H */
