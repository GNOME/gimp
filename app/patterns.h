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

typedef struct _GPattern  GPattern, * GPatternP;

struct _GPattern
{
  gchar   * filename;     /*  actual filename--pattern's location on disk   */
  gchar   * name;         /*  pattern's name--for pattern selection dialog  */
  gint      index;        /*  pattern's index...                            */
  TempBuf * mask;         /*  the actual mask...                            */
};

/*  function declarations  */
void       patterns_init              (gboolean   no_data);
void       patterns_free              (void);

void       select_pattern             (GPatternP  pattern);
GPatternP  get_pattern_by_index       (gint       index);
GPatternP  get_active_pattern         (void);
GPatternP  pattern_list_get_pattern   (GSList    *list,
				       gchar     *name);

void       create_pattern_dialog      (void);
void       pattern_select_dialog_free (void);
/* this is useful for pixmap brushes etc */
int        load_pattern_pattern       (GPatternP pattern,
				       FILE* fp,
				       gchar* filename);

/*  global variables  */
extern GSList * pattern_list;
extern gint     num_patterns;

#endif  /*  __PATTERNS_H__  */
