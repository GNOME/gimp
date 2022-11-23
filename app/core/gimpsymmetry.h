/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasymmetry.h
 * Copyright (C) 2015 Jehan <jehan@ligma.org>
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

#ifndef __LIGMA_SYMMETRY_H__
#define __LIGMA_SYMMETRY_H__


#include "ligmaobject.h"


/* shift one more than LIGMA_CONFIG_PARAM_DONT_COMPARE */
#define LIGMA_SYMMETRY_PARAM_GUI (1 << (7 + G_PARAM_USER_SHIFT))


#define LIGMA_TYPE_SYMMETRY            (ligma_symmetry_get_type ())
#define LIGMA_SYMMETRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SYMMETRY, LigmaSymmetry))
#define LIGMA_SYMMETRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SYMMETRY, LigmaSymmetryClass))
#define LIGMA_IS_SYMMETRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SYMMETRY))
#define LIGMA_IS_SYMMETRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SYMMETRY))
#define LIGMA_SYMMETRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SYMMETRY, LigmaSymmetryClass))


typedef struct _LigmaSymmetryClass   LigmaSymmetryClass;

struct _LigmaSymmetry
{
  LigmaObject    parent_instance;

  LigmaImage    *image;
  LigmaDrawable *drawable;
  LigmaCoords   *origin;
  gboolean      active;
  gint          version;

  GList        *strokes;
  gboolean      stateful;
};

struct _LigmaSymmetryClass
{
  LigmaObjectClass  parent_class;

  const gchar * label;

  /* Virtual functions */
  void       (* update_strokes)             (LigmaSymmetry       *symmetry,
                                             LigmaDrawable       *drawable,
                                             LigmaCoords         *origin);
  void       (* get_transform)              (LigmaSymmetry       *symmetry,
                                             gint                stroke,
                                             gdouble            *angle,
                                             gboolean           *reflect);
  void       (* active_changed)             (LigmaSymmetry       *symmetry);

  gboolean   (* update_version)             (LigmaSymmetry       *symmetry);
};


GType          ligma_symmetry_get_type       (void) G_GNUC_CONST;

void           ligma_symmetry_set_stateful   (LigmaSymmetry       *symmetry,
                                             gboolean            stateful);
void           ligma_symmetry_set_origin     (LigmaSymmetry       *symmetry,
                                             LigmaDrawable       *drawable,
                                             LigmaCoords         *origin);
void           ligma_symmetry_clear_origin   (LigmaSymmetry       *symmetry);

LigmaCoords   * ligma_symmetry_get_origin     (LigmaSymmetry       *symmetry);
gint           ligma_symmetry_get_size       (LigmaSymmetry       *symmetry);
LigmaCoords   * ligma_symmetry_get_coords     (LigmaSymmetry       *symmetry,
                                             gint                stroke);
void           ligma_symmetry_get_transform  (LigmaSymmetry       *symmetry,
                                             gint                stroke,
                                             gdouble            *angle,
                                             gboolean           *reflect);
void           ligma_symmetry_get_matrix     (LigmaSymmetry       *symmetry,
                                             gint                stroke,
                                             LigmaMatrix3        *matrix);
GeglNode     * ligma_symmetry_get_operation  (LigmaSymmetry       *symmetry,
                                             gint                stroke);

gchar        * ligma_symmetry_parasite_name  (GType               type);
LigmaParasite * ligma_symmetry_to_parasite    (const LigmaSymmetry *symmetry);
LigmaSymmetry * ligma_symmetry_from_parasite  (const LigmaParasite *parasite,
                                             LigmaImage          *image,
                                             GType               type);


#endif  /*  __LIGMA_SYMMETRY_H__  */
