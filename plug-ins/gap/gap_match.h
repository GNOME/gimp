/*  gap_match.h
 *
 * GAP ... Gimp Animation Plugins
 *
 */
/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* revision history:
 * version 0.97.00  1998.10.14  hof: - created module 
 */
 

#ifndef _GAP_MATCH_H
#define _GAP_MATCH_H

#include "libgimp/gimp.h"

#define MTCH_EQUAL       0
#define MTCH_START       1
#define MTCH_END         2
#define MTCH_ANYWHERE    3
#define MTCH_NUMBERLIST  4
#define MTCH_INV_NUMBERLIST  5
#define MTCH_ALL_VISIBLE  6

int  p_is_empty (char *str);
void p_substitute_framenr (char *buffer, int buff_len, char *new_layername, long curr);

void str_toupper(char *str);

int  p_match_number(gint32 layer_id, char *pattern);
int  p_match_name(char *layername, char *pattern, gint32 mode, gint32 case_sensitive);
int  p_match_layer(gint32 layer_idx, char *layername, char *pattern,
                  gint32 mode, gint32 case_sensitive, gint32 invert,
                  gint nlayers, gint32 layer_id);

#endif
