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

#include "libgimpbase/gimpbase.h"

#include "pdb-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpbrushgenerated.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimpitem.h"
#include "core/gimpmybrush.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"

#include "text/gimptextlayer.h"
#include "text/gimpfont.h"

#include "vectors/gimppath.h"

#include "gimppdb-utils.h"
#include "gimppdberror.h"

#include "gimp-intl.h"


typedef struct
{
  const gchar *name;
  const gchar *collection;
  gboolean     is_internal;
} SearchData;


static GimpResource * gimp_pdb_get_data_factory_item    (Gimp         *gimp,
                                                         GType         data_type,
                                                         const gchar  *name,
                                                         const gchar  *collection,
                                                         gboolean      is_internal);
const gchar *         gimp_pdb_get_data_label           (GType         data_type);

static gboolean       gimp_pdb_search_in_data_container (GimpData     *data,
                                                         SearchData   *search_data);;


GimpDataFactory *
gimp_pdb_get_data_factory (Gimp  *gimp,
                           GType  data_type)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (data_type, GIMP_TYPE_DATA), NULL);

  if (g_type_is_a (data_type, GIMP_TYPE_BRUSH_GENERATED))
    return gimp->brush_factory;
  else if (g_type_is_a (data_type, GIMP_TYPE_BRUSH))
    return gimp->brush_factory;
  else if (g_type_is_a (data_type, GIMP_TYPE_PATTERN))
    return gimp->pattern_factory;
  else if (g_type_is_a (data_type, GIMP_TYPE_GRADIENT))
    return gimp->gradient_factory;
  else if (g_type_is_a (data_type, GIMP_TYPE_PALETTE))
    return gimp->palette_factory;
  else if (g_type_is_a (data_type, GIMP_TYPE_FONT))
    return gimp->font_factory;
  else if (g_type_is_a (data_type, GIMP_TYPE_DYNAMICS))
    return gimp->dynamics_factory;
  else if (g_type_is_a (data_type, GIMP_TYPE_MYBRUSH))
    return gimp->mybrush_factory;

  /* If we reach this, it means we forgot a data factory in our list! */
  g_return_val_if_reached (NULL);
}

GList *
gimp_pdb_get_resources (Gimp               *gimp,
                        GType               data_type,
                        const gchar        *name,
                        GimpPDBDataAccess   access,
                        GError            **error)
{
  GList           *resources;
  GimpDataFactory *factory;
  GimpContainer   *container;
  const gchar     *label;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  label = gimp_pdb_get_data_label (data_type);

  if (! name || ! strlen (name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: %s is a data label from the
                    * PDB-error-data-label context.
                    */
                   C_("PDB-error-message", "%s name cannot be empty"),
                   g_type_name (data_type));
      return NULL;
    }

  factory = gimp_pdb_get_data_factory (gimp, data_type);
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  container = gimp_data_factory_get_container (factory);
  resources = gimp_container_get_children_by_name (container, name);

  if (! resources && ! strcmp (name, "Standard"))
    {
      GimpData *resource;

      resource = gimp_data_factory_data_get_standard (factory, gimp_get_user_context (gimp));
      g_return_val_if_fail (resource != NULL, NULL);
      resources = g_list_prepend (NULL, resource);
    }

  if (! resources)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' not found"), label, name);
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_WRITE) ||
           (access & GIMP_PDB_DATA_ACCESS_RENAME))
    {
      for (GList *iter = resources; iter; iter = iter->next)
        {
          if ((access & GIMP_PDB_DATA_ACCESS_WRITE) &&
              ! gimp_data_is_writable (GIMP_DATA (iter->data)))
            {
              g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           /* TRANSLATOR: the first %s is a data label from the
                            * PDB-error-data-label context. The second %s is a data
                            * name.
                            */
                           C_("PDB-error-message", "%s '%s' is not editable"), label, name);
              return NULL;
            }
          else if ((access & GIMP_PDB_DATA_ACCESS_RENAME) &&
                   ! gimp_viewable_is_name_editable (GIMP_VIEWABLE (iter->data)))
            {
              g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           /* TRANSLATOR: the first %s is a data label from the
                            * PDB-error-data-label context. The second %s is a data
                            * name.
                            */
                           C_("PDB-error-message", "%s '%s' is not renamable"), label, name);
              return NULL;
            }
        }
    }

  return resources;
}

GimpResource *
gimp_pdb_get_resource (Gimp               *gimp,
                       GType               data_type,
                       const gchar        *name,
                       GimpPDBDataAccess   access,
                       GError            **error)
{
  GimpResource *resource;
  const gchar  *label;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  label = gimp_pdb_get_data_label (data_type);

  if (! name || ! strlen (name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: %s is a data label from the
                    * PDB-error-data-label context.
                    */
                   C_("PDB-error-message", "%s name cannot be empty"),
                   g_type_name (data_type));
      return NULL;
    }

  resource = gimp_pdb_get_data_factory_item (gimp, data_type, name, NULL, TRUE);

  if (! resource)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' not found"), label, name);
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_WRITE) &&
           ! gimp_data_is_writable (GIMP_DATA (resource)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' is not editable"), label, name);
      return NULL;
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_RENAME) &&
           ! gimp_viewable_is_name_editable (GIMP_VIEWABLE (resource)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' is not renamable"), label, name);
      return NULL;
    }

  return resource;
}

GimpResource *
gimp_pdb_get_resource_by_id (Gimp               *gimp,
                             GType               data_type,
                             const gchar        *name,
                             const gchar        *collection,
                             gboolean            is_internal,
                             GimpPDBDataAccess   access,
                             GError            **error)
{
  GimpResource *resource;
  const gchar  *label;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  label = gimp_pdb_get_data_label (data_type);

  if (! name || ! strlen (name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: %s is a data label from the
                    * PDB-error-data-label context.
                    */
                   C_("PDB-error-message", "%s name cannot be empty"),
                   g_type_name (data_type));
      return NULL;
    }

  resource = gimp_pdb_get_data_factory_item (gimp, data_type, name, collection, is_internal);

  if (! resource)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' not found"), label, name);
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_WRITE) &&
           ! gimp_data_is_writable (GIMP_DATA (resource)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' is not editable"), label, name);
      return NULL;
    }
  else if ((access & GIMP_PDB_DATA_ACCESS_RENAME) &&
           ! gimp_viewable_is_name_editable (GIMP_VIEWABLE (resource)))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   /* TRANSLATOR: the first %s is a data label from the
                    * PDB-error-data-label context. The second %s is a data
                    * name.
                    */
                   C_("PDB-error-message", "%s '%s' is not renamable"), label, name);
      return NULL;
    }

  return resource;
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

  brush = GIMP_BRUSH (gimp_pdb_get_resource (gimp, GIMP_TYPE_BRUSH_GENERATED, name, access, error));

  if (brush != NULL && ! GIMP_IS_BRUSH_GENERATED (brush))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Brush '%s' is not a generated brush"), name);
      return NULL;
    }

  return brush;
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
                   gimp_item_get_id (item));
      return FALSE;
    }

  if (image && image != gimp_item_get_image (item))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be used because it is "
                     "attached to another image"),
                   gimp_object_get_name (item),
                   gimp_item_get_id (item));
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
                   gimp_item_get_id (item));
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
                   gimp_item_get_id (item),
                   gimp_object_get_name (item2),
                   gimp_item_get_id (item2));
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
                   gimp_item_get_id (item),
                   gimp_object_get_name (not_descendant),
                   gimp_item_get_id (not_descendant));
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
                   gimp_item_get_id (item));
      return FALSE;
    }
  else if (gimp_item_get_image (item) != dest_image)
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Trying to add item '%s' (%d) to wrong image"),
                   gimp_object_get_name (item),
                   gimp_item_get_id (item));
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

  if ((modify & GIMP_PDB_ITEM_CONTENT) && gimp_item_is_content_locked (item, NULL))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because its "
                     "contents are locked"),
                   gimp_object_get_name (item),
                   gimp_item_get_id (item));
      return FALSE;
    }

  if ((modify & GIMP_PDB_ITEM_POSITION) && gimp_item_is_position_locked (item, NULL))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Item '%s' (%d) cannot be modified because its "
                     "position and size are locked"),
                   gimp_object_get_name (item),
                   gimp_item_get_id (item));
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
                   gimp_item_get_id (item));
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
                   gimp_item_get_id (item));
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
                   gimp_item_get_id (GIMP_ITEM (layer)));

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
               gimp_image_get_id (image),
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
               gimp_image_get_id (image),
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
               gimp_image_get_id (image),
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
               gimp_image_get_id (image),
               gimp_pdb_enum_value_get_nick (GIMP_TYPE_PRECISION, precision));

  return FALSE;
}

GimpGuide *
gimp_pdb_image_get_guide (GimpImage  *image,
                          gint        guide_id,
                          GError    **error)
{
  GimpGuide *guide;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  guide = gimp_image_get_guide (image, guide_id);

  if (guide)
    return guide;

  g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) does not contain guide with ID %d"),
               gimp_image_get_display_name (image),
               gimp_image_get_id (image),
               guide_id);
  return NULL;
}

GimpSamplePoint *
gimp_pdb_image_get_sample_point (GimpImage  *image,
                                 gint        sample_point_id,
                                 GError    **error)
{
  GimpSamplePoint *sample_point;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  sample_point = gimp_image_get_sample_point (image, sample_point_id);

  if (sample_point)
    return sample_point;

  g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
               _("Image '%s' (%d) does not contain sample point with ID %d"),
               gimp_image_get_display_name (image),
               gimp_image_get_id (image),
               sample_point_id);
  return NULL;
}

GimpStroke *
gimp_pdb_get_path_stroke (GimpPath           *path,
                          gint                stroke_id,
                          GimpPDBItemModify   modify,
                          GError            **error)
{
  GimpStroke *stroke = NULL;

  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! gimp_pdb_item_is_not_group (GIMP_ITEM (path), error))
    return NULL;

  if (! modify || gimp_pdb_item_is_modifiable (GIMP_ITEM (path),
                                               modify, error))
    {
      stroke = gimp_path_stroke_get_by_id (path, stroke_id);

      if (! stroke)
        g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                     _("Path object %d does not contain stroke with ID %d"),
                     gimp_item_get_id (GIMP_ITEM (path)), stroke_id);
    }

  return stroke;
}

gboolean
gimp_pdb_is_canonical_procedure (const gchar  *procedure_name,
                                 GError      **error)
{
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_is_canonical_identifier (procedure_name))
    {
      g_set_error (error, GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                   _("Procedure name '%s' is not a canonical identifier"),
                   procedure_name);
      return FALSE;
    }

  return TRUE;
}


/* Private functions. */

static GimpResource *
gimp_pdb_get_data_factory_item (Gimp        *gimp,
                                GType        data_type,
                                const gchar *name,
                                const gchar *collection,
                                gboolean     is_internal)
{
  GimpDataFactory *factory;
  GimpContainer   *container;
  GimpObject      *resource;

  factory = gimp_pdb_get_data_factory (gimp, data_type);
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  container = gimp_data_factory_get_container (factory);

  if (collection == NULL)
    {
      resource = gimp_container_get_child_by_name (container, name);
    }
  else
    {
      SearchData *data = g_new (SearchData, 1);

      data->name        = name;
      data->collection  = collection;
      data->is_internal = is_internal;
      resource = gimp_container_search (container,
                                        (GimpContainerSearchFunc) gimp_pdb_search_in_data_container,
                                        data);
      g_free (data);
    }

  if (! resource)
    resource = gimp_container_get_child_by_name (gimp_data_factory_get_container_obsolete (factory),
                                                 name);

  if (! resource && ! strcmp (name, "Standard"))
    resource = (GimpObject *) gimp_data_factory_data_get_standard (factory,
                                                                   gimp_get_user_context (gimp));

  return (GimpResource *) resource;
}

const gchar *
gimp_pdb_get_data_label (GType data_type)
{
  g_return_val_if_fail (g_type_is_a (data_type, GIMP_TYPE_DATA), NULL);

  if (g_type_is_a (data_type, GIMP_TYPE_BRUSH_GENERATED))
    return C_("PDB-error-data-label", "Generated brush");
  else if (g_type_is_a (data_type, GIMP_TYPE_BRUSH))
    return C_("PDB-error-data-label", "Brush");
  else if (g_type_is_a (data_type, GIMP_TYPE_PATTERN))
    return C_("PDB-error-data-label", "Pattern");
  else if (g_type_is_a (data_type, GIMP_TYPE_GRADIENT))
    return C_("PDB-error-data-label", "Gradient");
  else if (g_type_is_a (data_type, GIMP_TYPE_PALETTE))
    return C_("PDB-error-data-label", "Palette");
  else if (g_type_is_a (data_type, GIMP_TYPE_FONT))
    return C_("PDB-error-data-label", "Font");
  else if (g_type_is_a (data_type, GIMP_TYPE_DYNAMICS))
    return C_("PDB-error-data-label", "Paint dynamics");
  else if (g_type_is_a (data_type, GIMP_TYPE_MYBRUSH))
    return C_("PDB-error-data-label", "MyPaint brush");

  /* If we reach this, it means we forgot a data type in our list! */
  g_return_val_if_reached (NULL);
}

static gboolean
gimp_pdb_search_in_data_container (GimpData   *data,
                                   SearchData *search_data)
{
  return gimp_data_identify (data, search_data->name, search_data->collection, search_data->is_internal);
}
