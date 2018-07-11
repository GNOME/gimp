/* types.h: general types.
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

#ifndef TYPES_H
#define TYPES_H

/* Booleans.  */
typedef enum { false = 0, true = 1 } boolean;

/* The X11 library defines `FALSE' and `TRUE', and so we only want to
   define them if necessary.  */
#ifndef FALSE
#define FALSE false
#define TRUE true
#endif /* FALSE */

/* The usual null-terminated string.  */
typedef char *string;

/* A generic pointer in ANSI C.  */
typedef void *address;

/* We use `real' for our floating-point variables.  */
typedef double real;

/* A character code.  Perhaps someday we will allow for 16-bit
   character codes, but for now we are restricted to 256 characters per
   font (like TeX and PostScript).  */
typedef unsigned char charcode_type;


/* Used in file formats.  */
typedef unsigned char one_byte;
typedef signed char signed_byte;
typedef unsigned short two_bytes;
typedef short signed_2_bytes;
typedef unsigned int four_bytes;
typedef int signed_4_bytes;
typedef int byte_count_type;

/* These are intended to be used for output in file formats where a
   ``byte'' is defined to be eight bits, regardless of the hardware.  */
#define ONE_BYTE_BIG  (1 << 8)
#define TWO_BYTES_BIG  (1 << 16)
#define THREE_BYTES_BIG (1 << 24)


/* Complex numbers.  */
typedef struct
{
  real real;
  real imag;
} complex;
typedef enum { first_complex_part, second_complex_part} complex_part_type;
typedef enum { polar_rep, rectangular_rep} complex_rep_type;


/* Dimensions of a rectangle.  */
typedef struct
{
  unsigned height, width;
} dimensions_type;

#define DIMENSIONS_HEIGHT(d) ((d).height)
#define DIMENSIONS_WIDTH(d) ((d).width)


/* Cartesian points.  */
typedef struct
{
  int x, y;
} coordinate_type;

typedef struct
{
  double x, y;
} real_coordinate_type;

#if 0
typedef struct
{
  double       align_threshold;
  double       corner_always_threshold;
  unsigned int corner_surround;
  double       corner_threshold;
  double       error_threshold;
  unsigned int filter_alternative_surround;
  double       filter_epsilon;
  unsigned int filter_iteration_count;
  double       filter_percent;
  unsigned int filter_secondary_surround;
  unsigned int filter_surround;
  boolean      keep_knees;
  double       line_reversion_threshold;
  double       line_threshold;
  double       reparameterize_improvement;
  double       reparameterize_threshold;
  double       subdivide_search;
  unsigned int subdivide_surround;
  double       subdivide_threshold;
  unsigned int tangent_surround;
} SELVALS;

#else

typedef struct
{
  double       align_threshold;
  double       corner_always_threshold;
  double       corner_surround;
  double       corner_threshold;
  double       error_threshold;
  double       filter_alternative_surround;
  double       filter_epsilon;
  double       filter_iteration_count;
  double       filter_percent;
  double       filter_secondary_surround;
  double       filter_surround;
  boolean      keep_knees;
  double       line_reversion_threshold;
  double       line_threshold;
  double       reparameterize_improvement;
  double       reparameterize_threshold;
  double       subdivide_search;
  double       subdivide_surround;
  double       subdivide_threshold;
  double       tangent_surround;
} SELVALS;

#endif /* 1 */

#endif /* not TYPES_H */
