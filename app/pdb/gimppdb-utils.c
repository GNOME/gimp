/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "pdb-types.h"

#include "core/gimp.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimpitem.h"

#include "text/gimptextlayer.h"

#include "vectors/gimpvectors.h"

#include "gimppdb-utils.h"
#include "gimppdberror.h"

#include "gimp-intl.h"


static GimpObject *
gimp_pdb_get_data_factory_item (GimpDataFactory *factory,
                                const gchar     *name)
{
  GimpObject *object;

  object = gimp_container_get_child_by_name (gimp_data_factory_get_container (factory), name);

  if (! object)
    object = gimp_container_get_child_by_name (gimp_data_factory_get_container_obsolete (factory), name);

  if (! object && ! strcmp (name, "Standard"))
    {
      Gimp *gimp = gimp_data_factory_get_gimp (factory);

      object = (GimpObject *)
        gimp_data_factory_data_get_standard (factory,
                                             gimp_get_user_context (gimp));
    }

  return object;
}


GimpBrush *
gimp_pdb_get_brush (Gimp               *gimp,
                    const gchar        *name,
                    GimpPDBDataAccess   access,
                    GError            **error)
{
  GimpBrush *brush;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty brush name"));
      return NULL;
    }

  brush = (GimpBrush *) gimp_pdb_get_data_factory_item (gimp->brush_factory, name);

  if (! brush)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Brush '%s' not found"), name);
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_WRITE) &&
           ! gimp_data_is_writable (GIMP_DATA (brush)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Brush '%s' is not editable"), name);
      return NULL;
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_RENAME) &&
           ! gimp_viewable_is_name_editable (GIMP_VIEWABLE (brush)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Brush '%s' is not renamable"), name);
      return NULL;
    }

  return brush;
}

GimpBrush *
gimp_pdb_get_generated_brush (Gimp               *gimp,
                              const gchar        *name,
                              GimpPDBDataAccess   access,
                              GError            **error)
{
  GimpBrush *brush;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  brush = gimp_pdb_get_brush (gimp, name, access, error);

  if (! brush)
    return NULL;

  if (! GIMP_IS_BRUSH_GENERATED (brush))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Brush '%s' is not a generated brush"), name);
      return NULL;
    }

  return brush;
}

GimpDynamics *
gimp_pdb_get_dynamics (Gimp               *gimp,
                       const gchar        *name,
                       GimpPDBDataAccess   access,
                       GError            **error)
{
  GimpDynamics *dynamics;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty paint dynamics name"));
      return NULL;
    }

  dynamics = (GimpDynamics *) gimp_pdb_get_data_factory_item (gimp->dynamics_factory, name);

  if (! dynamics)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Paint dynamics '%s' not found"), name);
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_WRITE) &&
           ! gimp_data_is_writable (GIMP_DATA (dynamics)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Paint dynamics '%s' is not editable"), name);
      return NULL;
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_RENAME) &&
           ! gimp_viewable_is_name_editable (GIMP_VIEWABLE (dynamics)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Paint dynamics '%s' is not renamable"), name);
      return NULL;
    }

  return dynamics;
}

GimpMybrush *
gimp_pdb_get_mybrush (Gimp               *gimp,
                      const gchar        *name,
                      GimpPDBDataAccess   access,
                      GError            **error)
{
  GimpMybrush *brush;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty MyPaint brush name"));
      return NULL;
    }

  brush = (GimpMybrush *) gimp_pdb_get_data_factory_item (gimp->mybrush_factory, name);

  if (! brush)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("MyPaint brush '%s' not found"), name);
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_WRITE) &&
           ! gimp_data_is_writable (GIMP_DATA (brush)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("MyPaint brush '%s' is not editable"), name);
      return NULL;
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_RENAME) &&
           ! gimp_viewable_is_name_editable (GIMP_VIEWABLE (brush)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("MyPaint brush '%s' is not renamable"), name);
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
      g_set_error_literal (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty pattern name"));
      return NULL;
    }

  pattern = (GimpPattern *) gimp_pdb_get_data_factory_item (gimp->pattern_factory, name);

  if (! pattern)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Pattern '%s' not found"), name);
    }

  return pattern;
}

GimpGradient *
gimp_pdb_get_gradient (Gimp               *gimp,
                       const gchar        *name,
                       GimpPDBDataAccess   access,
                       GError            **error)
{
  GimpGradient *gradient;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty gradient name"));
      return NULL;
    }

  gradient = (GimpGradient *) gimp_pdb_get_data_factory_item (gimp->gradient_factory, name);

  if (! gradient)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Gradient '%s' not found"), name);
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_WRITE) &&
           ! gimp_data_is_writable (GIMP_DATA (gradient)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Gradient '%s' is not editable"), name);
      return NULL;
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_RENAME) &&
           ! gimp_viewable_is_name_editable (GIMP_VIEWABLE (gradient)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Gradient '%s' is not renamable"), name);
      return NULL;
    }

  return gradient;
}

GimpPalette *
gimp_pdb_get_palette (Gimp               *gimp,
                      const gchar        *name,
                      GimpPDBDataAccess   access,
                      GError            **error)
{
  GimpPalette *palette;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty palette name"));
      return NULL;
    }

  palette = (GimpPalette *) gimp_pdb_get_data_factory_item (gimp->palette_factory, name);

  if (! palette)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Palette '%s' not found"), name);
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_WRITE) &&
           ! gimp_data_is_writable (GIMP_DATA (palette)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Palette '%s' is not editable"), name);
      return NULL;
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_RENAME) &&
           ! gimp_viewable_is_name_editable (GIMP_VIEWABLE (palette)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Palette '%s' is not renamable"), name);
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
      g_set_error_literal (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty font name"));
      return NULL;
    }

  font = (GimpFont *) gimp_pdb_get_data_factory_item (gimp->font_factory, name);

  if (! font)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
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
      g_set_error_literal (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty buffer name"));
      return NULL;
    }

  buffer = (GimpBuffer *)
    gimp_container_get_child_by_name (gimp->named_buffers, name);

  if (! buffer)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
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
      g_set_error_literal (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty paint method name"));
      return NULL;
    }

  paint_info = (GimpPaintInfo *)
    gimp_container_get_child_by_name (gimp->paint_info_list, name);

  if (! paint_info)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Paint method '%s' does not exist"), name);
    }

  return paint_info;
}

gboolean
gimp_pdb_item_is_attached (GimpItem           *item,
                           GimpImage          *image,
                           GimpPDBItemModify   modify,
                           GError            **error)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (image == NULL || GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_item_is_attached (item))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it has not "
                     "been added to an image"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item));
      return FALSE;
    }

  if (image && image != gimp_item_get_image (item))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it is "
                     "attached to another image"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item));
      return FALSE;
    }

  return gimp_pdb_item_is_modifiable (item, modify, error);
}

gboolean
gimp_pdb_item_is_in_tree (GimpItem           *item,
                          GimpImage          *image,
                          GimpPDBItemModify   modify,
                          GError            **error)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (image == NULL || GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_pdb_item_is_attached (item, image, modify, error))
    return FALSE;

  if (! gimp_item_get_tree (item))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it is not "
                     "a direct child of an item tree"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
gimp_pdb_item_is_in_same_tree (GimpItem   *item,
                               GimpItem   *item2,
                               GimpImage  *image,
                               GError    **error)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (item2), FALSE);
  g_return_val_if_fail (image == NULL || GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_pdb_item_is_in_tree (item, image, FALSE, error) ||
      ! gimp_pdb_item_is_in_tree (item2, image, FALSE, error))
    return FALSE;

  if (gimp_item_get_tree (item) != gimp_item_get_tree (item2))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Items '%s' (%d) and '%s' (%d) cannot be used "
                     "because they are not part of the same item tree"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item),
                   gimp_object_get_name (item2),
                   gimp_item_get_ID (item2));
      return FALSE;
    }

  return TRUE;
}

gboolean
gimp_pdb_item_is_not_ancestor (GimpItem  *item,
                               GimpItem  *not_descendant,
                               GError   **error)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (GIMP_IS_ITEM (not_descendant), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (gimp_viewable_is_ancestor (GIMP_VIEWABLE (item),
                                 GIMP_VIEWABLE (not_descendant)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) must not be an ancestor of "
                     "'%s' (%d)"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item),
                   gimp_object_get_name (not_descendant),
                   gimp_item_get_ID (not_descendant));
      return FALSE;
    }

  return TRUE;
}

gboolean
gimp_pdb_item_is_floating (GimpItem  *item,
                           GimpImage *dest_image,
                           GError   **error)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! g_object_is_floating (item))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) has already been added to an image"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item));
      return FALSE;
    }
  else if (gimp_item_get_image (item) != dest_image)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Trying to add item '%s' (%d) to wrong image"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
gimp_pdb_item_is_modifiable (GimpItem           *item,
                             GimpPDBItemModify   modify,
                             GError            **error)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /*  When a channel is position-locked, it is also implicitly
   *  content-locked because we translate channels by modifying their
   *  pixels.
   */
  if ((modify & GIMP_PDB_ITEM_POSITION) && GIMP_IS_CHANNEL (item))
    modify |= GIMP_PDB_ITEM_CONTENT;

  if ((modify & GIMP_PDB_ITEM_CONTENT) && gimp_item_is_content_locked (item))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because its "
                     "contents are locked"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item));
      return FALSE;
    }

  if ((modify & GIMP_PDB_ITEM_POSITION) && gimp_item_is_position_locked (item))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because its "
                     "position and size are locked"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
gimp_pdb_item_is_group (GimpItem  *item,
                        GError   **error)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_viewable_get_children (GIMP_VIEWABLE (item)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it is "
                     "not a group item"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
gimp_pdb_item_is_not_group (GimpItem  *item,
                            GError   **error)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (gimp_viewable_get_children (GIMP_VIEWABLE (item)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because it "
                     "is a group item"),
                   gimp_object_get_name (item),
                   gimp_item_get_ID (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
gimp_pdb_layer_is_text_layer (GimpLayer          *layer,
                              GimpPDBItemModify   modify,
                              GError            **error)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_item_is_text_layer (GIMP_ITEM (layer)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Layer '%s' (%d) cannot be used because it is not "
                     "a text layer"),
                   gimp_object_get_name (layer),
                   gimp_item_get_ID (GIMP_ITEM (layer)));

      return FALSE;
    }

  return gimp_pdb_item_is_attached (GIMP_ITEM (layer), NULL, modify, error);
}

static const gchar *
gimp_pdb_enum_value_get_nick (GType enum_type,
                              gint  value)
{
  GEnumClass  *enum_class;
  GEnumValue  *enum_value;
  const gchar *nick;

  enum_class = g_type_class_ref (enum_type);
  enum_value = g_enum_get_value (enum_class, value);

  nick = enum_value->value_nick;

  g_type_class_unref (enum_class);

  return nick;
}

gboolean
gimp_pdb_image_is_base_type (GimpImage          *image,
                             GimpImageBaseType   type,
                             GError            **error)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (gimp_image_get_base_type (image) == type)
    return TRUE;

  g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) is of type '%s', "
                 "but an image of type '%s' is expected"),
               gimp_image_get_display_name (image),
               gimp_image_get_ID (image),
               gimp_pdb_enum_value_get_nick (GIMP_TYPE_IMAGE_BASE_TYPE,
                                             gimp_image_get_base_type (image)),
               gimp_pdb_enum_value_get_nick (GIMP_TYPE_IMAGE_BASE_TYPE, type));

  return FALSE;
}

gboolean
gimp_pdb_image_is_not_base_type (GimpImage          *image,
                                 GimpImageBaseType   type,
                                 GError            **error)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (gimp_image_get_base_type (image) != type)
    return TRUE;

  g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) must not be of type '%s'"),
               gimp_image_get_display_name (image),
               gimp_image_get_ID (image),
               gimp_pdb_enum_value_get_nick (GIMP_TYPE_IMAGE_BASE_TYPE, type));

  return FALSE;
}

gboolean
gimp_pdb_image_is_precision (GimpImage      *image,
                             GimpPrecision   precision,
                             GError        **error)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (gimp_image_get_precision (image) == precision)
    return TRUE;

  g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) has precision '%s', "
                 "but an image of precision '%s' is expected"),
               gimp_image_get_display_name (image),
               gimp_image_get_ID (image),
               gimp_pdb_enum_value_get_nick (GIMP_TYPE_PRECISION,
                                             gimp_image_get_precision (image)),
               gimp_pdb_enum_value_get_nick (GIMP_TYPE_PRECISION, precision));

  return FALSE;
}

gboolean
gimp_pdb_image_is_not_precision (GimpImage      *image,
                                 GimpPrecision   precision,
                                 GError        **error)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (gimp_image_get_precision (image) != precision)
    return TRUE;

  g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) must not be of precision '%s'"),
               gimp_image_get_display_name (image),
               gimp_image_get_ID (image),
               gimp_pdb_enum_value_get_nick (GIMP_TYPE_PRECISION, precision));

  return FALSE;
}

GimpGuide *
gimp_pdb_image_get_guide (GimpImage  *image,
                          gint        guide_ID,
                          GError    **error)
{
  GimpGuide *guide;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  guide = gimp_image_get_guide (image, guide_ID);

  if (guide)
    return guide;

  g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) does not contain guide with ID %d"),
               gimp_image_get_display_name (image),
               gimp_image_get_ID (image),
               guide_ID);
  return NULL;
}

GimpSamplePoint *
gimp_pdb_image_get_sample_point (GimpImage  *image,
                                 gint        sample_point_ID,
                                 GError    **error)
{
  GimpSamplePoint *sample_point;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  sample_point = gimp_image_get_sample_point (image, sample_point_ID);

  if (sample_point)
    return sample_point;

  g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) does not contain sample point with ID %d"),
               gimp_image_get_display_name (image),
               gimp_image_get_ID (image),
               sample_point_ID);
  return NULL;
}

GimpStroke *
gimp_pdb_get_vectors_stroke (GimpVectors        *vectors,
                             gint                stroke_ID,
                             GimpPDBItemModify   modify,
                             GError            **error)
{
  GimpStroke *stroke = NULL;

  g_return_val_if_fail (GIMP_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! gimp_pdb_item_is_not_group (GIMP_ITEM (vectors), error))
    return NULL;

  if (! modify || gimp_pdb_item_is_modifiable (GIMP_ITEM (vectors),
                                               modify, error))
    {
      stroke = gimp_vectors_stroke_get_by_ID (vectors, stroke_ID);

      if (! stroke)
        g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                     _("Vectors object %d does not contain stroke with ID %d"),
                     gimp_item_get_ID (GIMP_ITEM (vectors)), stroke_ID);
    }

  return stroke;
}
