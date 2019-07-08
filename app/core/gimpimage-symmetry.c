/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimage-symmetry.c
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

#include "config.h"

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "gimpsymmetry.h"
#include "gimpimage.h"
#include "gimpimage-private.h"
#include "gimpimage-symmetry.h"
#include "gimpsymmetry-mandala.h"
#include "gimpsymmetry-mirror.h"
#include "gimpsymmetry-tiling.h"


/**
 * gimp_image_symmetry_list:
 *
 * Returns a list of #GType of all existing symmetries.
 **/
GList *
gimp_image_symmetry_list (void)
{
  GList *list = NULL;

  list = g_list_prepend (list, GINT_TO_POINTER (GIMP_TYPE_MIRROR));
  list = g_list_prepend (list, GINT_TO_POINTER (GIMP_TYPE_TILING));
  list = g_list_prepend (list, GINT_TO_POINTER (GIMP_TYPE_MANDALA));

  return list;
}

/**
 * gimp_image_symmetry_new:
 * @image: the #GimpImage
 * @type:  the #GType of the symmetry
 *
 * Creates a new #GimpSymmetry of @type attached to @image.
 * @type must be a subtype of `GIMP_TYPE_SYMMETRY`.
 * Note that using the base @type `GIMP_TYPE_SYMMETRY` creates an
 * identity transformation.
 *
 * Returns: the new #GimpSymmetry.
 **/
GimpSymmetry *
gimp_image_symmetry_new (GimpImage *image,
                         GType      type)
{
  GimpSymmetry *sym = NULL;

  g_return_val_if_fail (g_type_is_a (type, GIMP_TYPE_SYMMETRY), NULL);

  sym = g_object_new (type,
                      "image", image,
                      NULL);

  return sym;
}

/**
 * gimp_image_symmetry_add:
 * @image: the #GimpImage
 * @type:  the #GType of the symmetry
 *
 * Add a symmetry of type @type to @image and make it the
 * active transformation.
 **/
void
gimp_image_symmetry_add (GimpImage    *image,
                         GimpSymmetry *sym)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_SYMMETRY (sym));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  private->symmetries = g_list_prepend (private->symmetries,
                                        g_object_ref (sym));
}

/**
 * gimp_image_symmetry_remove:
 * @image:   the #GimpImage
 * @sym: the #GimpSymmetry
 *
 * Remove @sym from the list of symmetries of @image.
 * If it was the active transformation, unselect it first.
 **/
void
gimp_image_symmetry_remove (GimpImage    *image,
                            GimpSymmetry *sym)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_SYMMETRY (sym));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->active_symmetry == sym)
    gimp_image_set_active_symmetry (image, GIMP_TYPE_SYMMETRY);

  private->symmetries = g_list_remove (private->symmetries, sym);
  g_object_unref (sym);
}

/**
 * gimp_image_symmetry_get:
 * @image: the #GimpImage
 *
 * Returns: the list of #GimpSymmetry set on @image.
 * The returned list belongs to @image and should not be freed.
 **/
GList *
gimp_image_symmetry_get (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return private->symmetries;
}

/**
 * gimp_image_set_active_symmetry:
 * @image: the #GimpImage
 * @type:  the #GType of the symmetry
 *
 * Select the symmetry of type @type.
 * Using the GType allows to select a transformation without
 * knowing whether one of the same @type was already created.
 *
 * Returns TRUE on success, FALSE if no such symmetry was found.
 **/
gboolean
gimp_image_set_active_symmetry (GimpImage *image,
                                GType      type)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  g_object_set (image,
                "symmetry", type,
                NULL);

  return TRUE;
}

/**
 * gimp_image_get_active_symmetry:
 * @image: the #GimpImage
 *
 * Returns the #GimpSymmetry transformation active on @image.
 **/
GimpSymmetry *
gimp_image_get_active_symmetry (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return private->active_symmetry;
}
