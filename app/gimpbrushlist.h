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
#ifndef __GIMPBRUSHELIST_H__
#define __GIMPBRUSHELIST_H__

#include <glib.h>
#include "procedural_db.h"
#include "temp_buf.h"
#include "gimpbrush.h"
#include "gimpbrushlistF.h"


#define GIMP_TYPE_BRUSH_LIST  (gimp_brush_list_get_type ())
#define GIMP_BRUSH_LIST(obj)  (GIMP_CHECK_CAST ((obj), GIMP_TYPE_BRUSH_LIST, GimpBrushList))
#define GIMP_IS_BRUSH_LIST(obj) (GIMP_CHECK_TYPE ((obj), GIMP_TYPE_BRUSH_LIST))

GimpBrushList *gimp_brush_list_new      (void);
GtkType        gimp_brush_list_get_type (void);
void           gimp_brush_list_add      (GimpBrushList *list,
					 GimpBrush *brush);
void           gimp_brush_list_remove   (GimpBrushList *list,
					 GimpBrush *brush);
GimpBrushP     gimp_brush_list_get_brush(GimpBrushList *list, char *name);
int            gimp_brush_list_length   (GimpBrushList *list);


/*  global variables  */
extern GimpBrushList *brush_list;


/*  function declarations  */
void       brushes_init             (int no_data);
void       brushes_free             (void);
void       brush_select_dialog_free (void);
void       select_brush             (GimpBrushP);
GimpBrushP gimp_brush_list_get_brush_by_index (GimpBrushList *, int);
int        gimp_brush_list_get_brush_index (GimpBrushList *, GimpBrush *);
GimpBrushP get_active_brush         (void);

/* TODO: {re}move this function */
void       create_brush_dialog      (void);

/*  Brush procedures  */
extern ProcRecord brushes_get_opacity_proc;
extern ProcRecord brushes_set_opacity_proc;
extern ProcRecord brushes_get_spacing_proc;
extern ProcRecord brushes_set_spacing_proc;
extern ProcRecord brushes_get_paint_mode_proc;
extern ProcRecord brushes_set_paint_mode_proc;
extern ProcRecord brushes_get_brush_proc;
extern ProcRecord brushes_set_brush_proc;
extern ProcRecord brushes_list_proc;
extern ProcRecord brushes_refresh_brush_proc;

#endif  /*  __GIMPBRUSHELIST_H__  */
