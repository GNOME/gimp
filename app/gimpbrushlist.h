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

#ifndef __GIMP_BRUSH_LIST_H__
#define __GIMP_BRUSH_LIST_H__


#include "gimplist.h"


#define GIMP_TYPE_BRUSH_LIST         (gimp_brush_list_get_type ())
#define GIMP_BRUSH_LIST(obj)         (GTK_CHECK_CAST ((obj), GIMP_TYPE_BRUSH_LIST, GimpBrushList))
#define GIMP_IS_BRUSH_LIST(obj)      (GTK_CHECK_TYPE ((obj), GIMP_TYPE_BRUSH_LIST))
#define GIMP_BRUSH_LIST_CLASS(klass) (GTK_CHECK_CLASS_CAST (klass, gimp_brush_list_get_type(), GimpBrushListClass))

typedef struct _GimpBrushListClass GimpBrushListClass;

struct _GimpBrushList
{
  GimpList gimplist;

  gint     num_brushes;
};

struct _GimpBrushListClass
{
  GimpListClass parent_class;
};


/*  global variables  */
extern GimpBrushList *brush_list;


/*  function declarations  */
GimpBrushList * gimp_brush_list_new                (void);
GtkType         gimp_brush_list_get_type           (void);
gint            gimp_brush_list_length             (GimpBrushList *list);

void            gimp_brush_list_add                (GimpBrushList *list,
						    GimpBrush     *brush);
void            gimp_brush_list_remove             (GimpBrushList *list,
						    GimpBrush     *brush);

GimpBrush     * gimp_brush_list_get_brush          (GimpBrushList *list,
						    gchar         *name);
GimpBrush     * gimp_brush_list_get_brush_by_index (GimpBrushList *list,
						    gint           index);

gint            gimp_brush_list_get_brush_index    (GimpBrushList *list,
						    GimpBrush     *brush);


void            brushes_init                       (gboolean       no_data);
void            brushes_free                       (void);
GimpBrush     * brushes_get_standard_brush         (void);


#endif  /*  __GIMP_BRUSH_LIST_H__  */
