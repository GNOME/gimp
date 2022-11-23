/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"

#include "pdb-types.h"

#include "core/ligma.h"
#include "core/ligmabrushgenerated.h"
#include "core/ligmachannel.h"
#include "core/ligmacontainer.h"
#include "core/ligmadatafactory.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-guides.h"
#include "core/ligmaimage-sample-points.h"
#include "core/ligmaitem.h"

#include "text/ligmatextlayer.h"

#include "vectors/ligmavectors.h"

#include "ligmapdb-utils.h"
#include "ligmapdberror.h"

#include "ligma-intl.h"


static LigmaObject *
ligma_pdb_get_data_factory_item (LigmaDataFactory *factory,
                                const gchar     *name)
{
  LigmaObject *object;

  object = ligma_container_get_child_by_name (ligma_data_factory_get_container (factory), name);

  if (! object)
    object = ligma_container_get_child_by_name (ligma_data_factory_get_container_obsolete (factory), name);

  if (! object && ! strcmp (name, "Standard"))
    {
      Ligma *ligma = ligma_data_factory_get_ligma (factory);

      object = (LigmaObject *)
        ligma_data_factory_data_get_standard (factory,
                                             ligma_get_user_context (ligma));
    }

  return object;
}


LigmaBrush *
ligma_pdb_get_brush (Ligma               *ligma,
                    const gchar        *name,
                    LigmaPDBDataAccess   access,
                    GError            **error)
{
  LigmaBrush *brush;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty brush name"));
      return NULL;
    }

  brush = (LigmaBrush *) ligma_pdb_get_data_factory_item (ligma->brush_factory, name);

  if (! brush)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Brush '%s' not found"), name);
    }
  else if ((access & LIGMA_PDB_DATA_ACCESS_WRITE) &&
           ! ligma_data_is_writable (LIGMA_DATA (brush)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Brush '%s' is not editable"), name);
      return NULL;
    }
  else if ((access & LIGMA_PDB_DATA_ACCESS_RENAME) &&
           ! ligma_viewable_is_name_editable (LIGMA_VIEWABLE (brush)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Brush '%s' is not renamable"), name);
      return NULL;
    }

  return brush;
}

LigmaBrush *
ligma_pdb_get_generated_brush (Ligma               *ligma,
                              const gchar        *name,
                              LigmaPDBDataAccess   access,
                              GError            **error)
{
  LigmaBrush *brush;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  brush = ligma_pdb_get_brush (ligma, name, access, error);

  if (! brush)
    return NULL;

  if (! LIGMA_IS_BRUSH_GENERATED (brush))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Brush '%s' is not a generated brush"), name);
      return NULL;
    }

  return brush;
}

LigmaDynamics *
ligma_pdb_get_dynamics (Ligma               *ligma,
                       const gchar        *name,
                       LigmaPDBDataAccess   access,
                       GError            **error)
{
  LigmaDynamics *dynamics;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty paint dynamics name"));
      return NULL;
    }

  dynamics = (LigmaDynamics *) ligma_pdb_get_data_factory_item (ligma->dynamics_factory, name);

  if (! dynamics)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Paint dynamics '%s' not found"), name);
    }
  else if ((access & LIGMA_PDB_DATA_ACCESS_WRITE) &&
           ! ligma_data_is_writable (LIGMA_DATA (dynamics)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Paint dynamics '%s' is not editable"), name);
      return NULL;
    }
  else if ((access & LIGMA_PDB_DATA_ACCESS_RENAME) &&
           ! ligma_viewable_is_name_editable (LIGMA_VIEWABLE (dynamics)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Paint dynamics '%s' is not renamable"), name);
      return NULL;
    }

  return dynamics;
}

LigmaMybrush *
ligma_pdb_get_mybrush (Ligma               *ligma,
                      const gchar        *name,
                      LigmaPDBDataAccess   access,
                      GError            **error)
{
  LigmaMybrush *brush;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty MyPaint brush name"));
      return NULL;
    }

  brush = (LigmaMybrush *) ligma_pdb_get_data_factory_item (ligma->mybrush_factory, name);

  if (! brush)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("MyPaint brush '%s' not found"), name);
    }
  else if ((access & LIGMA_PDB_DATA_ACCESS_WRITE) &&
           ! ligma_data_is_writable (LIGMA_DATA (brush)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("MyPaint brush '%s' is not editable"), name);
      return NULL;
    }
  else if ((access & LIGMA_PDB_DATA_ACCESS_RENAME) &&
           ! ligma_viewable_is_name_editable (LIGMA_VIEWABLE (brush)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("MyPaint brush '%s' is not renamable"), name);
      return NULL;
    }

  return brush;
}

LigmaPattern *
ligma_pdb_get_pattern (Ligma         *ligma,
                      const gchar  *name,
                      GError      **error)
{
  LigmaPattern *pattern;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty pattern name"));
      return NULL;
    }

  pattern = (LigmaPattern *) ligma_pdb_get_data_factory_item (ligma->pattern_factory, name);

  if (! pattern)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Pattern '%s' not found"), name);
    }

  return pattern;
}

LigmaGradient *
ligma_pdb_get_gradient (Ligma               *ligma,
                       const gchar        *name,
                       LigmaPDBDataAccess   access,
                       GError            **error)
{
  LigmaGradient *gradient;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty gradient name"));
      return NULL;
    }

  gradient = (LigmaGradient *) ligma_pdb_get_data_factory_item (ligma->gradient_factory, name);

  if (! gradient)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Gradient '%s' not found"), name);
    }
  else if ((access & LIGMA_PDB_DATA_ACCESS_WRITE) &&
           ! ligma_data_is_writable (LIGMA_DATA (gradient)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Gradient '%s' is not editable"), name);
      return NULL;
    }
  else if ((access & LIGMA_PDB_DATA_ACCESS_RENAME) &&
           ! ligma_viewable_is_name_editable (LIGMA_VIEWABLE (gradient)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Gradient '%s' is not renamable"), name);
      return NULL;
    }

  return gradient;
}

LigmaPalette *
ligma_pdb_get_palette (Ligma               *ligma,
                      const gchar        *name,
                      LigmaPDBDataAccess   access,
                      GError            **error)
{
  LigmaPalette *palette;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty palette name"));
      return NULL;
    }

  palette = (LigmaPalette *) ligma_pdb_get_data_factory_item (ligma->palette_factory, name);

  if (! palette)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Palette '%s' not found"), name);
    }
  else if ((access & LIGMA_PDB_DATA_ACCESS_WRITE) &&
           ! ligma_data_is_writable (LIGMA_DATA (palette)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Palette '%s' is not editable"), name);
      return NULL;
    }
  else if ((access & LIGMA_PDB_DATA_ACCESS_RENAME) &&
           ! ligma_viewable_is_name_editable (LIGMA_VIEWABLE (palette)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Palette '%s' is not renamable"), name);
      return NULL;
    }

  return palette;
}

LigmaFont *
ligma_pdb_get_font (Ligma         *ligma,
                   const gchar  *name,
                   GError      **error)
{
  LigmaFont *font;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty font name"));
      return NULL;
    }

  font = (LigmaFont *) ligma_pdb_get_data_factory_item (ligma->font_factory, name);

  if (! font)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Font '%s' not found"), name);
    }

  return font;
}

LigmaBuffer *
ligma_pdb_get_buffer (Ligma         *ligma,
                     const gchar  *name,
                     GError      **error)
{
  LigmaBuffer *buffer;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty buffer name"));
      return NULL;
    }

  buffer = (LigmaBuffer *)
    ligma_container_get_child_by_name (ligma->named_buffers, name);

  if (! buffer)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Named buffer '%s' not found"), name);
    }

  return buffer;
}

LigmaPaintInfo *
ligma_pdb_get_paint_info (Ligma         *ligma,
                         const gchar  *name,
                         GError      **error)
{
  LigmaPaintInfo *paint_info;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! name || ! strlen (name))
    {
      g_set_error_literal (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                           _("Invalid empty paint method name"));
      return NULL;
    }

  paint_info = (LigmaPaintInfo *)
    ligma_container_get_child_by_name (ligma->paint_info_list, name);

  if (! paint_info)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Paint method '%s' does not exist"), name);
    }

  return paint_info;
}

gboolean
ligma_pdb_item_is_attached (LigmaItem           *item,
                           LigmaImage          *image,
                           LigmaPDBItemModify   modify,
                           GError            **error)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (image == NULL || LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! ligma_item_is_attached (item))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it has not "
                     "been added to an image"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item));
      return FALSE;
    }

  if (image && image != ligma_item_get_image (item))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it is "
                     "attached to another image"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item));
      return FALSE;
    }

  return ligma_pdb_item_is_modifiable (item, modify, error);
}

gboolean
ligma_pdb_item_is_in_tree (LigmaItem           *item,
                          LigmaImage          *image,
                          LigmaPDBItemModify   modify,
                          GError            **error)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (image == NULL || LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! ligma_pdb_item_is_attached (item, image, modify, error))
    return FALSE;

  if (! ligma_item_get_tree (item))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it is not "
                     "a direct child of an item tree"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
ligma_pdb_item_is_in_same_tree (LigmaItem   *item,
                               LigmaItem   *item2,
                               LigmaImage  *image,
                               GError    **error)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (LIGMA_IS_ITEM (item2), FALSE);
  g_return_val_if_fail (image == NULL || LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! ligma_pdb_item_is_in_tree (item, image, FALSE, error) ||
      ! ligma_pdb_item_is_in_tree (item2, image, FALSE, error))
    return FALSE;

  if (ligma_item_get_tree (item) != ligma_item_get_tree (item2))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Items '%s' (%d) and '%s' (%d) cannot be used "
                     "because they are not part of the same item tree"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item),
                   ligma_object_get_name (item2),
                   ligma_item_get_id (item2));
      return FALSE;
    }

  return TRUE;
}

gboolean
ligma_pdb_item_is_not_ancestor (LigmaItem  *item,
                               LigmaItem  *not_descendant,
                               GError   **error)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (LIGMA_IS_ITEM (not_descendant), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (ligma_viewable_is_ancestor (LIGMA_VIEWABLE (item),
                                 LIGMA_VIEWABLE (not_descendant)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) must not be an ancestor of "
                     "'%s' (%d)"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item),
                   ligma_object_get_name (not_descendant),
                   ligma_item_get_id (not_descendant));
      return FALSE;
    }

  return TRUE;
}

gboolean
ligma_pdb_item_is_floating (LigmaItem  *item,
                           LigmaImage *dest_image,
                           GError   **error)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (dest_image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! g_object_is_floating (item))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) has already been added to an image"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item));
      return FALSE;
    }
  else if (ligma_item_get_image (item) != dest_image)
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Trying to add item '%s' (%d) to wrong image"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
ligma_pdb_item_is_modifiable (LigmaItem           *item,
                             LigmaPDBItemModify   modify,
                             GError            **error)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /*  When a channel is position-locked, it is also implicitly
   *  content-locked because we translate channels by modifying their
   *  pixels.
   */
  if ((modify & LIGMA_PDB_ITEM_POSITION) && LIGMA_IS_CHANNEL (item))
    modify |= LIGMA_PDB_ITEM_CONTENT;

  if ((modify & LIGMA_PDB_ITEM_CONTENT) && ligma_item_is_content_locked (item, NULL))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because its "
                     "contents are locked"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item));
      return FALSE;
    }

  if ((modify & LIGMA_PDB_ITEM_POSITION) && ligma_item_is_position_locked (item, NULL))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because its "
                     "position and size are locked"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
ligma_pdb_item_is_group (LigmaItem  *item,
                        GError   **error)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! ligma_viewable_get_children (LIGMA_VIEWABLE (item)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it is "
                     "not a group item"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
ligma_pdb_item_is_not_group (LigmaItem  *item,
                            GError   **error)
{
  g_return_val_if_fail (LIGMA_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (ligma_viewable_get_children (LIGMA_VIEWABLE (item)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because it "
                     "is a group item"),
                   ligma_object_get_name (item),
                   ligma_item_get_id (item));
      return FALSE;
    }

  return TRUE;
}

gboolean
ligma_pdb_layer_is_text_layer (LigmaLayer          *layer,
                              LigmaPDBItemModify   modify,
                              GError            **error)
{
  g_return_val_if_fail (LIGMA_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! ligma_item_is_text_layer (LIGMA_ITEM (layer)))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Layer '%s' (%d) cannot be used because it is not "
                     "a text layer"),
                   ligma_object_get_name (layer),
                   ligma_item_get_id (LIGMA_ITEM (layer)));

      return FALSE;
    }

  return ligma_pdb_item_is_attached (LIGMA_ITEM (layer), NULL, modify, error);
}

static const gchar *
ligma_pdb_enum_value_get_nick (GType enum_type,
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
ligma_pdb_image_is_base_type (LigmaImage          *image,
                             LigmaImageBaseType   type,
                             GError            **error)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (ligma_image_get_base_type (image) == type)
    return TRUE;

  g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) is of type '%s', "
                 "but an image of type '%s' is expected"),
               ligma_image_get_display_name (image),
               ligma_image_get_id (image),
               ligma_pdb_enum_value_get_nick (LIGMA_TYPE_IMAGE_BASE_TYPE,
                                             ligma_image_get_base_type (image)),
               ligma_pdb_enum_value_get_nick (LIGMA_TYPE_IMAGE_BASE_TYPE, type));

  return FALSE;
}

gboolean
ligma_pdb_image_is_not_base_type (LigmaImage          *image,
                                 LigmaImageBaseType   type,
                                 GError            **error)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (ligma_image_get_base_type (image) != type)
    return TRUE;

  g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) must not be of type '%s'"),
               ligma_image_get_display_name (image),
               ligma_image_get_id (image),
               ligma_pdb_enum_value_get_nick (LIGMA_TYPE_IMAGE_BASE_TYPE, type));

  return FALSE;
}

gboolean
ligma_pdb_image_is_precision (LigmaImage      *image,
                             LigmaPrecision   precision,
                             GError        **error)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (ligma_image_get_precision (image) == precision)
    return TRUE;

  g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) has precision '%s', "
                 "but an image of precision '%s' is expected"),
               ligma_image_get_display_name (image),
               ligma_image_get_id (image),
               ligma_pdb_enum_value_get_nick (LIGMA_TYPE_PRECISION,
                                             ligma_image_get_precision (image)),
               ligma_pdb_enum_value_get_nick (LIGMA_TYPE_PRECISION, precision));

  return FALSE;
}

gboolean
ligma_pdb_image_is_not_precision (LigmaImage      *image,
                                 LigmaPrecision   precision,
                                 GError        **error)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (ligma_image_get_precision (image) != precision)
    return TRUE;

  g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) must not be of precision '%s'"),
               ligma_image_get_display_name (image),
               ligma_image_get_id (image),
               ligma_pdb_enum_value_get_nick (LIGMA_TYPE_PRECISION, precision));

  return FALSE;
}

LigmaGuide *
ligma_pdb_image_get_guide (LigmaImage  *image,
                          gint        guide_id,
                          GError    **error)
{
  LigmaGuide *guide;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  guide = ligma_image_get_guide (image, guide_id);

  if (guide)
    return guide;

  g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) does not contain guide with ID %d"),
               ligma_image_get_display_name (image),
               ligma_image_get_id (image),
               guide_id);
  return NULL;
}

LigmaSamplePoint *
ligma_pdb_image_get_sample_point (LigmaImage  *image,
                                 gint        sample_point_id,
                                 GError    **error)
{
  LigmaSamplePoint *sample_point;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  sample_point = ligma_image_get_sample_point (image, sample_point_id);

  if (sample_point)
    return sample_point;

  g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) does not contain sample point with ID %d"),
               ligma_image_get_display_name (image),
               ligma_image_get_id (image),
               sample_point_id);
  return NULL;
}

LigmaStroke *
ligma_pdb_get_vectors_stroke (LigmaVectors        *vectors,
                             gint                stroke_id,
                             LigmaPDBItemModify   modify,
                             GError            **error)
{
  LigmaStroke *stroke = NULL;

  g_return_val_if_fail (LIGMA_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! ligma_pdb_item_is_not_group (LIGMA_ITEM (vectors), error))
    return NULL;

  if (! modify || ligma_pdb_item_is_modifiable (LIGMA_ITEM (vectors),
                                               modify, error))
    {
      stroke = ligma_vectors_stroke_get_by_id (vectors, stroke_id);

      if (! stroke)
        g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                     _("Vectors object %d does not contain stroke with ID %d"),
                     ligma_item_get_id (LIGMA_ITEM (vectors)), stroke_id);
    }

  return stroke;
}

gboolean
ligma_pdb_is_canonical_procedure (const gchar  *procedure_name,
                                 GError      **error)
{
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! ligma_is_canonical_identifier (procedure_name))
    {
      g_set_error (error, LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_INVALID_ARGUMENT,
                   _("Procedure name '%s' is not a canonical identifier"),
                   procedure_name);
      return FALSE;
    }

  return TRUE;
}
