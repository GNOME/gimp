/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrendererviewable.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_CELL_RENDERER_VIEWABLE_H__
#define __GIMP_CELL_RENDERER_VIEWABLE_H__


#define GIMP_TYPE_CELL_RENDERER_VIEWABLE            (gimp_cell_renderer_viewable_get_type ())
#define GIMP_CELL_RENDERER_VIEWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CELL_RENDERER_VIEWABLE, GimpCellRendererViewable))
#define GIMP_CELL_RENDERER_VIEWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CELL_RENDERER_VIEWABLE, GimpCellRendererViewableClass))
#define GIMP_IS_CELL_RENDERER_VIEWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CELL_RENDERER_VIEWABLE))
#define GIMP_IS_CELL_RENDERER_VIEWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CELL_RENDERER_VIEWABLE))
#define GIMP_CELL_RENDERER_VIEWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CELL_RENDERER_VIEWABLE, GimpCellRendererViewableClass))


typedef struct _GimpCellRendererViewableClass GimpCellRendererViewableClass;

struct _GimpCellRendererViewable
{
  GtkCellRenderer   parent_instance;

  GimpViewRenderer *renderer;
};

struct _GimpCellRendererViewableClass
{
  GtkCellRendererClass  parent_class;

  gboolean (* pre_clicked) (GimpCellRendererViewable *cell,
                            const gchar              *path,
                            GdkModifierType           state);
  void     (* clicked)     (GimpCellRendererViewable *cell,
                            const gchar              *path,
                            GdkModifierType           state);
};


GType             gimp_cell_renderer_viewable_get_type    (void) G_GNUC_CONST;
GtkCellRenderer * gimp_cell_renderer_viewable_new         (void);
gboolean          gimp_cell_renderer_viewable_pre_clicked (GimpCellRendererViewable *cell,
                                                           const gchar              *path,
                                                           GdkModifierType           state);
void              gimp_cell_renderer_viewable_clicked     (GimpCellRendererViewable *cell,
                                                           const gchar              *path,
                                                           GdkModifierType           state);


#endif /* __GIMP_CELL_RENDERER_VIEWABLE_H__ */
