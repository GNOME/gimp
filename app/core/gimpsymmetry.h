/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsymmetry.h
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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

#ifndef __GIMP_SYMMETRY_H__
#define __GIMP_SYMMETRY_H__


#include "gimpobject.h"


/* shift one more than latest GIMP_CONFIG_PARAM_* */
#define GIMP_SYMMETRY_PARAM_GUI (1 << (0 + GIMP_CONFIG_PARAM_FLAG_SHIFT))


#define GIMP_TYPE_SYMMETRY            (gimp_symmetry_get_type ())
#define GIMP_SYMMETRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SYMMETRY, GimpSymmetry))
#define GIMP_SYMMETRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SYMMETRY, GimpSymmetryClass))
#define GIMP_IS_SYMMETRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SYMMETRY))
#define GIMP_IS_SYMMETRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SYMMETRY))
#define GIMP_SYMMETRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SYMMETRY, GimpSymmetryClass))


typedef struct _GimpSymmetryClass   GimpSymmetryClass;

struct _GimpSymmetry
{
  GimpObject    parent_instance;

  GimpImage    *image;
  GimpDrawable *drawable;
  GimpCoords   *origin;
  gboolean      active;
  gint          version;

  GList        *strokes;
  gboolean      stateful;
};

struct _GimpSymmetryClass
{
  GimpObjectClass  parent_class;

  const gchar * label;

  /* Virtual functions */
  void       (* update_strokes)             (GimpSymmetry       *symmetry,
                                             GimpDrawable       *drawable,
                                             GimpCoords         *origin);
  void       (* get_transform)              (GimpSymmetry       *symmetry,
                                             gint                stroke,
                                             gdouble            *angle,
                                             gboolean           *reflect);
  void       (* active_changed)             (GimpSymmetry       *symmetry);

  gboolean   (* update_version)             (GimpSymmetry       *symmetry);
};


GType          gimp_symmetry_get_type       (void) G_GNUC_CONST;

void           gimp_symmetry_set_stateful   (GimpSymmetry       *symmetry,
                                             gboolean            stateful);
void           gimp_symmetry_set_origin     (GimpSymmetry       *symmetry,
                                             GimpDrawable       *drawable,
                                             GimpCoords         *origin);
void           gimp_symmetry_clear_origin   (GimpSymmetry       *symmetry);

GimpCoords   * gimp_symmetry_get_origin     (GimpSymmetry       *symmetry);
gint           gimp_symmetry_get_size       (GimpSymmetry       *symmetry);
GimpCoords   * gimp_symmetry_get_coords     (GimpSymmetry       *symmetry,
                                             gint                stroke);
void           gimp_symmetry_get_transform  (GimpSymmetry       *symmetry,
                                             gint                stroke,
                                             gdouble            *angle,
                                             gboolean           *reflect);
void           gimp_symmetry_get_matrix     (GimpSymmetry       *symmetry,
                                             gint                stroke,
                                             GimpMatrix3        *matrix);
GeglNode     * gimp_symmetry_get_operation  (GimpSymmetry       *symmetry,
                                             gint                stroke);

gchar        * gimp_symmetry_parasite_name  (GType               type);
GimpParasite * gimp_symmetry_to_parasite    (const GimpSymmetry *symmetry);
GimpSymmetry * gimp_symmetry_from_parasite  (const GimpParasite *parasite,
                                             GimpImage          *image,
                                             GType               type);


#endif  /*  __GIMP_SYMMETRY_H__  */
