/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacellrendererviewable.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CELL_RENDERER_VIEWABLE_H__
#define __LIGMA_CELL_RENDERER_VIEWABLE_H__


#define LIGMA_TYPE_CELL_RENDERER_VIEWABLE            (ligma_cell_renderer_viewable_get_type ())
#define LIGMA_CELL_RENDERER_VIEWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CELL_RENDERER_VIEWABLE, LigmaCellRendererViewable))
#define LIGMA_CELL_RENDERER_VIEWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CELL_RENDERER_VIEWABLE, LigmaCellRendererViewableClass))
#define LIGMA_IS_CELL_RENDERER_VIEWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CELL_RENDERER_VIEWABLE))
#define LIGMA_IS_CELL_RENDERER_VIEWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CELL_RENDERER_VIEWABLE))
#define LIGMA_CELL_RENDERER_VIEWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CELL_RENDERER_VIEWABLE, LigmaCellRendererViewableClass))


typedef struct _LigmaCellRendererViewableClass LigmaCellRendererViewableClass;

struct _LigmaCellRendererViewable
{
  GtkCellRenderer   parent_instance;

  LigmaViewRenderer *renderer;
};

struct _LigmaCellRendererViewableClass
{
  GtkCellRendererClass  parent_class;

  gboolean (* pre_clicked) (LigmaCellRendererViewable *cell,
                            const gchar              *path,
                            GdkModifierType           state);
  void     (* clicked)     (LigmaCellRendererViewable *cell,
                            const gchar              *path,
                            GdkModifierType           state);
};


GType             ligma_cell_renderer_viewable_get_type    (void) G_GNUC_CONST;
GtkCellRenderer * ligma_cell_renderer_viewable_new         (void);
gboolean          ligma_cell_renderer_viewable_pre_clicked (LigmaCellRendererViewable *cell,
                                                           const gchar              *path,
                                                           GdkModifierType           state);
void              ligma_cell_renderer_viewable_clicked     (LigmaCellRendererViewable *cell,
                                                           const gchar              *path,
                                                           GdkModifierType           state);


#endif /* __LIGMA_CELL_RENDERER_VIEWABLE_H__ */
