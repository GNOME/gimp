/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_IMAGE_UNDO_H__
#define __GIMP_IMAGE_UNDO_H__


#include "gimpundo.h"


#define GIMP_TYPE_IMAGE_UNDO            (gimp_image_undo_get_type ())
#define GIMP_IMAGE_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_UNDO, GimpImageUndo))
#define GIMP_IMAGE_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_UNDO, GimpImageUndoClass))
#define GIMP_IS_IMAGE_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_UNDO))
#define GIMP_IS_IMAGE_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_UNDO))
#define GIMP_IMAGE_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_UNDO, GimpImageUndoClass))


typedef struct _GimpImageUndo      GimpImageUndo;
typedef struct _GimpImageUndoClass GimpImageUndoClass;

struct _GimpImageUndo
{
  GimpUndo           parent_instance;

  GimpImageBaseType  base_type;
  GimpPrecision      precision;
  gint               width;
  gint               height;
  gint               previous_origin_x;
  gint               previous_origin_y;
  gint               previous_width;
  gint               previous_height;
  gdouble            xresolution;
  gdouble            yresolution;
  GimpUnit          *resolution_unit;
  GimpGrid          *grid;
  gint               num_colors;
  guchar            *colormap;
  GimpColorProfile  *hidden_profile;
  GimpMetadata      *metadata;
  gchar             *parasite_name;
  GimpParasite      *parasite;
};

struct _GimpImageUndoClass
{
  GimpUndoClass  parent_class;
};


GType   gimp_image_undo_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_IMAGE_UNDO_H__ */
