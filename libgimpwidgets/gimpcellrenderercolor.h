/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcellrenderercolor.h
 * Copyright (C) 2004  Sven Neuman <sven1@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_CELL_RENDERER_COLOR_H__
#define __GIMP_CELL_RENDERER_COLOR_H__

G_BEGIN_DECLS


#define GIMP_TYPE_CELL_RENDERER_COLOR            (gimp_cell_renderer_color_get_type ())
#define GIMP_CELL_RENDERER_COLOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CELL_RENDERER_COLOR, GimpCellRendererColor))
#define GIMP_CELL_RENDERER_COLOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CELL_RENDERER_COLOR, GimpCellRendererColorClass))
#define GIMP_IS_CELL_RENDERER_COLOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CELL_RENDERER_COLOR))
#define GIMP_IS_CELL_RENDERER_COLOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CELL_RENDERER_COLOR))
#define GIMP_CELL_RENDERER_COLOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CELL_RENDERER_COLOR, GimpCellRendererColorClass))


typedef struct _GimpCellRendererColorClass GimpCellRendererColorClass;

struct _GimpCellRendererColor
{
  GtkCellRenderer  parent_instance;
};

struct _GimpCellRendererColorClass
{
  GtkCellRendererClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType             gimp_cell_renderer_color_get_type (void) G_GNUC_CONST;

GtkCellRenderer * gimp_cell_renderer_color_new      (void);


G_END_DECLS

#endif /* __GIMP_CELL_RENDERER_COLOR_H__ */
