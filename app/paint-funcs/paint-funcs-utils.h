/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PAINT_FUNCS_UTILS_H__
#define __PAINT_FUNCS_UTILS_H__


#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

/* This version of INT_MULT3 is very fast, but suffers from some
   slight roundoff errors.  It returns the correct result 99.987
   percent of the time */
#define INT_MULT3(a,b,c,t)  ((t) = (a) * (b) * (c) + 0x7F5B, \
                            ((((t) >> 7) + (t)) >> 16))
/*
  This version of INT_MULT3 always gives the correct result, but runs at
  approximatly one third the speed. */
/*  #define INT_MULT3(a,b,c,t) (((a) * (b) * (c) + 32512) / 65025.0)
 */

#define INT_BLEND(a,b,alpha,tmp)  (INT_MULT((a) - (b), alpha, tmp) + (b))

/* A drawable has an alphachannel if contains either 4 or 2 bytes data
 * aka GRAYA and RGBA and thus the macro below works. This will have
 * to change if we support bigger formats. We'll do it so for now because
 * masking is always cheaper than passing parameters over the stack.      */
/* FIXME: Move to a global place */
#define HAS_ALPHA(bytes) (~bytes & 1)


#endif  /*  __PAINT_FUNCS_UTILS_H__  */
