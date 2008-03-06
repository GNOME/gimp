/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "pdb-types.h"

#include "core/gimp.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpitem.h"

#include "vectors/gimpvectors.h"

#include "gimppdb-utils.h"
#include "gimppdberror.h"

#include "gimp-intl.h"


GimpBrush *
gimp_pdb_get_brush (Gimp         *gimp,
                    const gchar  *name,
                    gboolean      writable,
                    GError      **error)
{
  GimpBrush *brush;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Invalid empty brush name"));
      return NULL;
    }

  brush = (GimpBrush *)
    gimp_container_get_child_by_name (gimp->brush_factory->container, name);

  if (! brush)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Brush '%s' not found"), name);
    }
  else if (writable && ! GIMP_DATA (brush)->writable)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Brush '%s' is not editable"), name);
      return NULL;
    }

  return brush;
}

GimpBrush *
gimp_pdb_get_generated_brush (Gimp         *gimp,
                              const gchar  *name,
                              gboolean      writable,
                              GError      **error)
{
  GimpBrush *brush;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  brush = gimp_pdb_get_brush (gimp, name, writable, error);

  if (! brush)
    return NULL;

  if (! GIMP_IS_BRUSH_GENERATED (brush))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Brush '%s' is not a generated brush"), name);
      return NULL;
    }

  return brush;
}

GimpPattern *
gimp_pdb_get_pattern (Gimp         *gimp,
                      const gchar  *name,
                      GError      **error)
{
  GimpPattern *pattern;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Invalid empty pattern name"));
      return NULL;
    }

  pattern = (GimpPattern *)
    gimp_container_get_child_by_name (gimp->pattern_factory->container, name);

  if (! pattern)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Pattern '%s' not found"), name);
    }

  return pattern;
}

GimpGradient *
gimp_pdb_get_gradient (Gimp         *gimp,
                       const gchar  *name,
                       gboolean      writable,
                       GError      **error)
{
  GimpGradient *gradient;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Invalid empty gradient name"));
      return NULL;
    }

  gradient = (GimpGradient *)
    gimp_container_get_child_by_name (gimp->gradient_factory->container, name);

  if (! gradient)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Gradient '%s' not found"), name);
    }
  else if (writable && ! GIMP_DATA (gradient)->writable)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Gradient '%s' is not editable"), name);
      return NULL;
    }

  return gradient;
}

GimpPalette *
gimp_pdb_get_palette (Gimp         *gimp,
                      const gchar  *name,
                      gboolean      writable,
                      GError      **error)
{
  GimpPalette *palette;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Invalid empty palette name"));
      return NULL;
    }

  palette = (GimpPalette *)
    gimp_container_get_child_by_name (gimp->palette_factory->container, name);

  if (! palette)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Palette '%s' not found"), name);
    }
  else if (writable && ! GIMP_DATA (palette)->writable)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Palette '%s' is not editable"), name);
      return NULL;
    }

  return palette;
}

GimpFont *
gimp_pdb_get_font (Gimp         *gimp,
                   const gchar  *name,
                   GError      **error)
{
  GimpFont *font;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Invalid empty font name"));
      return NULL;
    }

  font = (GimpFont *)
    gimp_container_get_child_by_name (gimp->fonts, name);

  if (! font)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Font '%s' not found"), name);
    }

  return font;
}

GimpBuffer *
gimp_pdb_get_buffer (Gimp         *gimp,
                     const gchar  *name,
                     GError      **error)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Invalid empty buffer name"));
      return NULL;
    }

  buffer = (GimpBuffer *)
    gimp_container_get_child_by_name (gimp->named_buffers, name);

  if (! buffer)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Named buffer '%s' not found"), name);
    }

  return buffer;
}

GimpPaintInfo *
gimp_pdb_get_paint_info (Gimp         *gimp,
                         const gchar  *name,
                         GError      **error)
{
  GimpPaintInfo *paint_info;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Invalid empty paint method name"));
      return NULL;
    }

  paint_info = (GimpPaintInfo *)
    gimp_container_get_child_by_name (gimp->paint_info_list, name);

  if (! paint_info)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Paint method '%s' does not exist"), name);
    }

  return paint_info;
}

gboolean
gimp_pdb_item_is_attached (GimpItem  *item,
                           GError   **error)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_item_is_attached (item))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Item '%s' (%d) cannot be used because it has not "
                     "been added to an image"),
                   gimp_object_get_name (GIMP_OBJECT (item)),
                   gimp_item_get_ID (item));

      return FALSE;
    }

  return TRUE;
}

GimpStroke *
gimp_pdb_get_vectors_stroke (GimpVectors  *vectors,
                             gint          stroke_ID,
                             GError      **error)
{
  GimpStroke *stroke;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  stroke = gimp_vectors_stroke_get_by_ID (vectors, stroke_ID);

  if (! stroke)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_CALLING_ERROR,
                   _("Vectors object %d does not contain stroke with ID %d"),
                   gimp_item_get_ID (GIMP_ITEM (vectors)), stroke_ID);
    }

  return stroke;
}

