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
#ifndef __PATTERNS_H__
#define __PATTERNS_H__

#include <glib.h>
#include <stdio.h>

#include "temp_buf.h"

typedef struct _GPattern GPattern;

struct _GPattern
{
  gchar   * filename;     /*  actual filename--pattern's location on disk   */
  gchar   * name;         /*  pattern's name--for pattern selection dialog  */
  gint      index;        /*  pattern's index...                            */
  TempBuf * mask;         /*  the actual mask...                            */
};

/*  global variables  */
extern GSList * pattern_list;
extern gint     num_patterns;

void       patterns_init                     (gboolean   no_data);
void       patterns_free                     (void);
GPattern * patterns_get_standard_pattern     (void);

GPattern * pattern_list_get_pattern_by_index (GSList    *list,
					      gint       index);
GPattern * pattern_list_get_pattern          (GSList    *list,
					      gchar     *name);

/*  this is useful for pixmap brushes etc.  */
gboolean   pattern_load                      (GPattern  *pattern,
					      FILE      *fp,
					      gchar     *filename);
void       pattern_free                      (GPattern  *pattern);

#endif  /*  __PATTERNS_H__  */
