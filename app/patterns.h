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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __PATTERNS_H__
#define __PATTERNS_H__

#include "linked.h"
#include "temp_buf.h"
#include "procedural_db.h"

typedef struct _GPattern  GPattern, * GPatternP;

struct _GPattern
{
  char *     filename;     /*  actual filename--pattern's location on disk   */
  char *     name;         /*  pattern's name--for pattern selection dialog  */
  int        index;        /*  pattern's index...                            */
  TempBuf *  mask;         /*  the actual mask...                            */
};

/*  function declarations  */
void                patterns_init              (void);
void                patterns_free              (void);
void                pattern_select_dialog_free (void);
void                select_pattern             (GPatternP);
GPatternP           get_pattern_by_index       (int);
GPatternP           get_active_pattern         (void);
void                create_pattern_dialog      (void);

/*  global variables  */
extern link_ptr     pattern_list;
extern int          num_patterns;

/*  Pattern procedures  */
extern ProcRecord patterns_get_pattern_proc;
extern ProcRecord patterns_set_pattern_proc;
extern ProcRecord patterns_list_proc;

#endif  /*  __PATTERNS_H__  */
