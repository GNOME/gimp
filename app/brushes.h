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
#ifndef __BRUSHES_H__
#define __BRUSHES_H__

#include <glib.h>
#include "procedural_db.h"

struct _Canvas;
typedef struct _GBrush  GBrush, * GBrushP;

struct _GBrush
{
  char *     filename;    /*  actual filename--brush's location on disk  */
  char *     name;        /*  brush's name--for brush selection dialog   */
  int        spacing;     /*  brush's spacing                            */
  int        index;       /*  brush's index...                           */
  struct _Canvas *  mask;  /*  the actual mask as canvas                  */
};

/*  global variables  */
extern GSList *     brush_list;
extern int          num_brushes;


/*  function declarations  */
void                brushes_init             (int no_data);
void                brushes_free             (void);
void                brush_select_dialog_free (void);
void                select_brush             (GBrushP);
GBrushP             get_brush_by_index       (int);
GBrushP             get_active_brush         (void);
void                create_brush_dialog      (void);

/*  access functions  */
double              get_brush_opacity        (void);
int                 get_brush_spacing        (void);
int                 get_brush_paint_mode     (void);
void                set_brush_opacity        (double);
void                set_brush_spacing        (int);
void                set_brush_paint_mode     (int);

/*  Brush procedures  */
extern ProcRecord brushes_get_brush_proc;
extern ProcRecord brushes_set_brush_proc;
extern ProcRecord brushes_get_opacity_proc;
extern ProcRecord brushes_set_opacity_proc;
extern ProcRecord brushes_get_spacing_proc;
extern ProcRecord brushes_set_spacing_proc;
extern ProcRecord brushes_get_paint_mode_proc;
extern ProcRecord brushes_set_paint_mode_proc;
extern ProcRecord brushes_list_proc;
extern ProcRecord brushes_refresh_brush_proc;

#endif  /*  __BRUSHES_H__  */
