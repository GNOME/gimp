/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsizebox.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_SIZE_BOX_H__
#define __GIMP_SIZE_BOX_H__

G_BEGIN_DECLS


#define GIMP_TYPE_SIZE_BOX            (gimp_size_box_get_type ())
#define GIMP_SIZE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SIZE_BOX, GimpSizeBox))
#define GIMP_SIZE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SIZE_BOX, GimpSizeBoxClass))
#define GIMP_IS_SIZE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SIZE_BOX))
#define GIMP_IS_SIZE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SIZE_BOX))
#define GIMP_SIZE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SIZE_BOX, GimpSizeBoxClass))


typedef struct _GimpSizeBoxClass  GimpSizeBoxClass;

struct _GimpSizeBox
{
  GtkBox        parent_instance;

  GtkSizeGroup *size_group;

  gint          width;
  gint          height;
  GimpUnit     *unit;
  gdouble       xresolution;
  gdouble       yresolution;
  GimpUnit     *resolution_unit;

  gboolean      edit_resolution;
};

struct _GimpSizeBoxClass
{
  GtkBoxClass  parent_class;
};


GType       gimp_size_box_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GIMP_SIZE_BOX_H__ */
