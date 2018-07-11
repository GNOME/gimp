/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrendererbutton.h
 * Copyright (C) 2016 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CELL_RENDERER_BUTTON_H__
#define __GIMP_CELL_RENDERER_BUTTON_H__


#define GIMP_TYPE_CELL_RENDERER_BUTTON            (gimp_cell_renderer_button_get_type ())
#define GIMP_CELL_RENDERER_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CELL_RENDERER_BUTTON, GimpCellRendererButton))
#define GIMP_CELL_RENDERER_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CELL_RENDERER_BUTTON, GimpCellRendererButtonClass))
#define GIMP_IS_CELL_RENDERER_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CELL_RENDERER_BUTTON))
#define GIMP_IS_CELL_RENDERER_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CELL_RENDERER_BUTTON))
#define GIMP_CELL_RENDERER_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CELL_RENDERER_BUTTON, GimpCellRendererButtonClass))


typedef struct _GimpCellRendererButtonClass GimpCellRendererButtonClass;

struct _GimpCellRendererButton
{
  GtkCellRendererPixbuf  parent_instance;
};

struct _GimpCellRendererButtonClass
{
  GtkCellRendererPixbufClass  parent_class;

  void (* clicked) (GimpCellRendererButton *cell,
                    const gchar            *path,
                    GdkModifierType         state);
};


GType             gimp_cell_renderer_button_get_type (void) G_GNUC_CONST;

GtkCellRenderer * gimp_cell_renderer_button_new      (void);

void              gimp_cell_renderer_button_clicked  (GimpCellRendererButton *cell,
                                                      const gchar            *path,
                                                      GdkModifierType         state);


#endif /* __GIMP_CELL_RENDERER_BUTTON_H__ */
