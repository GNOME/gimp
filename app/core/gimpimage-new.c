/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-utils.h"

#include "ligma.h"
#include "ligmabuffer.h"
#include "ligmachannel.h"
#include "ligmacontext.h"
#include "ligmadrawable-fill.h"
#include "ligmagrouplayer.h"
#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmaimage-colormap.h"
#include "ligmaimage-new.h"
#include "ligmaimage-undo.h"
#include "ligmalayer.h"
#include "ligmalayer-new.h"
#include "ligmalist.h"
#include "ligmatemplate.h"

#include "ligma-intl.h"


LigmaTemplate *
ligma_image_new_get_last_template (Ligma      *ligma,
                                  LigmaImage *image)
{
  LigmaTemplate *template;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (image == NULL || LIGMA_IS_IMAGE (image), NULL);

  template = ligma_template_new ("image new values");

  if (image)
    {
      ligma_config_sync (G_OBJECT (ligma->config->default_image),
                        G_OBJECT (template), 0);
      ligma_template_set_from_image (template, image);
    }
  else
    {
      ligma_config_sync (G_OBJECT (ligma->image_new_last_template),
                        G_OBJECT (template), 0);
    }

  return template;
}

void
ligma_image_new_set_last_template (Ligma         *ligma,
                                  LigmaTemplate *template)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_TEMPLATE (template));

  ligma_config_sync (G_OBJECT (template),
                    G_OBJECT (ligma->image_new_last_template), 0);
}

LigmaImage *
ligma_image_new_from_template (Ligma         *ligma,
                              LigmaTemplate *template,
                              LigmaContext  *context)
{
  LigmaImage               *image;
  LigmaLayer               *layer;
  LigmaColorProfile        *profile;
  LigmaColorRenderingIntent intent;
  gboolean                 bpc;
  gint                     width, height;
  gboolean                 has_alpha;
  const gchar             *comment;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_TEMPLATE (template), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  image = ligma_create_image (ligma,
                             ligma_template_get_width (template),
                             ligma_template_get_height (template),
                             ligma_template_get_base_type (template),
                             ligma_template_get_precision (template),
                             FALSE);

  ligma_image_undo_disable (image);

  comment = ligma_template_get_comment (template);

  if (comment)
    {
      LigmaParasite *parasite;

      parasite = ligma_parasite_new ("ligma-comment",
                                    LIGMA_PARASITE_PERSISTENT,
                                    strlen (comment) + 1,
                                    comment);
      ligma_image_parasite_attach (image, parasite, FALSE);
      ligma_parasite_free (parasite);
    }

  ligma_image_set_resolution (image,
                             ligma_template_get_resolution_x (template),
                             ligma_template_get_resolution_y (template));
  ligma_image_set_unit (image, ligma_template_get_resolution_unit (template));

  profile = ligma_template_get_color_profile (template);
  ligma_image_set_color_profile (image, profile, NULL);
  if (profile)
    g_object_unref (profile);

  profile = ligma_template_get_simulation_profile (template);
  ligma_image_set_simulation_profile (image, profile);
  if (profile)
    g_object_unref (profile);

  intent = ligma_template_get_simulation_intent (template);
  ligma_image_set_simulation_intent (image, intent);
  bpc = ligma_template_get_simulation_bpc (template);
  ligma_image_set_simulation_bpc (image, bpc);

  width  = ligma_image_get_width (image);
  height = ligma_image_get_height (image);

  if (ligma_template_get_fill_type (template) == LIGMA_FILL_TRANSPARENT)
    has_alpha = TRUE;
  else
    has_alpha = FALSE;

  layer = ligma_layer_new (image, width, height,
                          ligma_image_get_layer_format (image, has_alpha),
                          _("Background"),
                          LIGMA_OPACITY_OPAQUE,
                          ligma_image_get_default_new_layer_mode (image));

  ligma_drawable_fill (LIGMA_DRAWABLE (layer),
                      context, ligma_template_get_fill_type (template));

  ligma_image_add_layer (image, layer, NULL, 0, FALSE);

  ligma_image_undo_enable (image);
  ligma_image_clean_all (image);

  return image;
}

LigmaImage *
ligma_image_new_from_drawable (Ligma         *ligma,
                              LigmaDrawable *drawable)
{
  LigmaItem          *item;
  LigmaImage         *image;
  LigmaImage         *new_image;
  LigmaLayer         *new_layer;
  GType              new_type;
  gint               off_x, off_y;
  LigmaImageBaseType  type;
  gdouble            xres;
  gdouble            yres;
  LigmaColorProfile  *profile;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  item  = LIGMA_ITEM (drawable);
  image = ligma_item_get_image (item);

  type = ligma_drawable_get_base_type (drawable);

  new_image = ligma_create_image (ligma,
                                 ligma_item_get_width  (item),
                                 ligma_item_get_height (item),
                                 type,
                                 ligma_drawable_get_precision (drawable),
                                 TRUE);
  ligma_image_undo_disable (new_image);

  if (type == LIGMA_INDEXED)
    ligma_image_set_colormap_palette (new_image,
                                     ligma_image_get_colormap_palette (image),
                                     FALSE);

  ligma_image_get_resolution (image, &xres, &yres);
  ligma_image_set_resolution (new_image, xres, yres);
  ligma_image_set_unit (new_image, ligma_image_get_unit (image));

  profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (drawable));
  ligma_image_set_color_profile (new_image, profile, NULL);

  if (LIGMA_IS_LAYER (drawable))
    new_type = G_TYPE_FROM_INSTANCE (drawable);
  else
    new_type = LIGMA_TYPE_LAYER;

  new_layer = LIGMA_LAYER (ligma_item_convert (LIGMA_ITEM (drawable),
                                             new_image, new_type));

  ligma_object_set_name (LIGMA_OBJECT (new_layer),
                        ligma_object_get_name (drawable));

  ligma_item_get_offset (LIGMA_ITEM (new_layer), &off_x, &off_y);
  ligma_item_translate (LIGMA_ITEM (new_layer), -off_x, -off_y, FALSE);
  ligma_item_set_visible (LIGMA_ITEM (new_layer), TRUE, FALSE);
  ligma_layer_set_mode (new_layer,
                       ligma_image_get_default_new_layer_mode (new_image),
                       FALSE);
  ligma_layer_set_opacity (new_layer, LIGMA_OPACITY_OPAQUE, FALSE);
  if (ligma_layer_can_lock_alpha (new_layer))
    ligma_layer_set_lock_alpha (new_layer, FALSE, FALSE);

  ligma_image_add_layer (new_image, new_layer, NULL, 0, TRUE);

  ligma_image_undo_enable (new_image);

  return new_image;
}

/**
 * ligma_image_new_copy_drawables:
 * @image: the image where @drawables belong to.
 * @drawables: the drawables to copy into @new_image.
 * @new_image: the image to insert to.
 * @tag_copies: tag copies of @drawable with "ligma-image-copied-layer".
 * @copied_drawables:
 * @tagged_drawables:
 * @parent:
 * @new_parent:
 *
 * This recursive function will create copies of all @drawables
 * belonging to the same @image, and will insert them into @new_image
 * with the same layer order and hierarchy (adding layer groups when
 * needed).
 * If a single drawable is selected, it will be copied visible, with
 * full opacity and default layer mode. Otherwise, visibility, opacity
 * and layer mode will be copied as-is, allowing proper compositing.
 *
 * The @copied_drawables, @tagged_drawables, @parent and @new_parent arguments
 * are only used internally for recursive calls and must be set to NULL for the
 * initial call.
 */
static void
ligma_image_new_copy_drawables (LigmaImage *image,
                               GList     *drawables,
                               LigmaImage *new_image,
                               gboolean   tag_copies,
                               GList     *copied_drawables,
                               GList     *tagged_drawables,
                               LigmaLayer *parent,
                               LigmaLayer *new_parent)
{
  GList *layers;
  GList *iter;
  gint   n_drawables;
  gint   index;

  n_drawables = g_list_length (drawables);
  if (parent == NULL)
    {
      /* Root layers. */
      layers = ligma_image_get_layer_iter (image);

      copied_drawables = g_list_copy (drawables);
      for (iter = copied_drawables; iter; iter = iter->next)
        {
          /* Tagged drawables are the explicitly copied drawables which have no
           * explicitly copied descendant items.
           */
          GList *iter2;

          for (iter2 = iter; iter2; iter2 = iter2->next)
            if (ligma_viewable_is_ancestor (iter->data, iter2->data))
              break;

          if (iter2 == NULL)
            tagged_drawables = g_list_prepend (tagged_drawables, iter->data);
        }

      /* Add any item parent. */
      for (iter = copied_drawables; iter; iter = iter->next)
        {
          LigmaItem *item = iter->data;
          while ((item = ligma_item_get_parent (item)))
            if (! g_list_find (copied_drawables, item))
              copied_drawables = g_list_prepend (copied_drawables, item);
        }
    }
  else
    {
      LigmaContainer *container;

      container = ligma_viewable_get_children (LIGMA_VIEWABLE (parent));
      layers = LIGMA_LIST (container)->queue->head;
    }

  index = 0;
  for (iter = layers; iter; iter = iter->next)
    {
      if (g_list_find (copied_drawables, iter->data))
        {
          LigmaLayer *new_layer;
          GType      new_type;
          gboolean   is_group;
          gboolean   is_tagged;

          if (LIGMA_IS_LAYER (iter->data))
            new_type = G_TYPE_FROM_INSTANCE (iter->data);
          else
            new_type = LIGMA_TYPE_LAYER;

          is_group  = (ligma_viewable_get_children (iter->data) != NULL);
          is_tagged = (g_list_find (tagged_drawables, iter->data) != NULL);

          if (is_group && ! is_tagged)
            new_layer = ligma_group_layer_new (new_image);
          else
            new_layer = LIGMA_LAYER (ligma_item_convert (LIGMA_ITEM (iter->data),
                                                       new_image, new_type));

          if (tag_copies && is_tagged)
            g_object_set_data (G_OBJECT (new_layer),
                               "ligma-image-copied-layer",
                               GINT_TO_POINTER (TRUE));

          ligma_object_set_name (LIGMA_OBJECT (new_layer),
                                ligma_object_get_name (iter->data));

          /* Visibility, mode and opacity mimic the source image if
           * multiple items are copied. Otherwise we just set them to
           * defaults.
           */
          ligma_item_set_visible (LIGMA_ITEM (new_layer),
                                 n_drawables > 1 ?
                                 ligma_item_get_visible (iter->data) : TRUE,
                                 FALSE);
          ligma_layer_set_mode (new_layer,
                               n_drawables > 1 && LIGMA_IS_LAYER (iter->data) ?
                               ligma_layer_get_mode (iter->data) :
                               ligma_image_get_default_new_layer_mode (new_image),
                               FALSE);
          ligma_layer_set_opacity (new_layer,
                                  n_drawables > 1 && LIGMA_IS_LAYER (iter->data) ?
                                  ligma_layer_get_opacity (iter->data) : LIGMA_OPACITY_OPAQUE, FALSE);

          if (ligma_layer_can_lock_alpha (new_layer))
            ligma_layer_set_lock_alpha (new_layer, FALSE, FALSE);

          ligma_image_add_layer (new_image, new_layer, new_parent, index++, TRUE);

          /* If a group, loop through children. */
          if (is_group && ! is_tagged)
            ligma_image_new_copy_drawables (image, drawables, new_image, tag_copies,
                                           copied_drawables, tagged_drawables,
                                           iter->data, new_layer);
        }
    }

  if (parent == NULL)
    {
      g_list_free (copied_drawables);
      g_list_free (tagged_drawables);
    }
}

LigmaImage *
ligma_image_new_from_drawables (Ligma     *ligma,
                               GList    *drawables,
                               gboolean  copy_selection,
                               gboolean  tag_copies)
{
  LigmaImage         *image = NULL;
  LigmaImage         *new_image;
  GList             *iter;
  LigmaImageBaseType  type;
  LigmaPrecision      precision;
  gdouble            xres;
  gdouble            yres;
  LigmaColorProfile  *profile = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (drawables != NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (LIGMA_IS_DRAWABLE (iter->data), NULL);

      if (iter == drawables)
        image = ligma_item_get_image (iter->data);
      else
        /* We only accept list of drawables for a same origin image. */
        g_return_val_if_fail (ligma_item_get_image (iter->data) == image, NULL);
    }

  type      = ligma_drawable_get_base_type (drawables->data);
  precision = ligma_drawable_get_precision (drawables->data);
  profile   = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (drawables->data));

  new_image = ligma_create_image (ligma,
                                 ligma_image_get_width  (image),
                                 ligma_image_get_height (image),
                                 type, precision,
                                 TRUE);
  ligma_image_undo_disable (new_image);

  if (type == LIGMA_INDEXED)
    ligma_image_set_colormap_palette (new_image,
                                     ligma_image_get_colormap_palette (image),
                                     FALSE);

  ligma_image_get_resolution (image, &xres, &yres);
  ligma_image_set_resolution (new_image, xres, yres);
  ligma_image_set_unit (new_image, ligma_image_get_unit (image));
  if (profile)
    ligma_image_set_color_profile (new_image, profile, NULL);

  if (copy_selection)
    {
      LigmaChannel *selection;

      selection = ligma_image_get_mask (image);
      if (! ligma_channel_is_empty (selection))
        {
          LigmaChannel *new_selection;
          GeglBuffer  *buffer;

          new_selection = ligma_image_get_mask (new_image);
          buffer = ligma_gegl_buffer_dup (ligma_drawable_get_buffer (LIGMA_DRAWABLE (selection)));
          ligma_drawable_set_buffer (LIGMA_DRAWABLE (new_selection),
                                    FALSE, NULL, buffer);
          g_object_unref (buffer);
        }
    }

  ligma_image_new_copy_drawables (image, drawables, new_image, tag_copies, NULL, NULL, NULL, NULL);
  ligma_image_undo_enable (new_image);

  return new_image;
}

LigmaImage *
ligma_image_new_from_component (Ligma            *ligma,
                               LigmaImage       *image,
                               LigmaChannelType  component)
{
  LigmaImage   *new_image;
  LigmaChannel *channel;
  LigmaLayer   *layer;
  const gchar *desc;
  gdouble      xres;
  gdouble      yres;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  new_image = ligma_create_image (ligma,
                                 ligma_image_get_width  (image),
                                 ligma_image_get_height (image),
                                 LIGMA_GRAY,
                                 ligma_image_get_precision (image),
                                 TRUE);

  ligma_image_undo_disable (new_image);

  ligma_image_get_resolution (image, &xres, &yres);
  ligma_image_set_resolution (new_image, xres, yres);
  ligma_image_set_unit (new_image, ligma_image_get_unit (image));

  channel = ligma_channel_new_from_component (image, component, NULL, NULL);

  layer = LIGMA_LAYER (ligma_item_convert (LIGMA_ITEM (channel),
                                         new_image, LIGMA_TYPE_LAYER));
  g_object_unref (channel);

  ligma_enum_get_value (LIGMA_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  ligma_object_take_name (LIGMA_OBJECT (layer),
                         g_strdup_printf (_("%s Channel Copy"), desc));

  ligma_image_add_layer (new_image, layer, NULL, 0, TRUE);

  ligma_image_undo_enable (new_image);

  return new_image;
}

LigmaImage *
ligma_image_new_from_buffer (Ligma       *ligma,
                            LigmaBuffer *buffer)
{
  LigmaImage        *image;
  LigmaLayer        *layer;
  const Babl       *format;
  gboolean          has_alpha;
  gdouble           res_x;
  gdouble           res_y;
  LigmaColorProfile *profile;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_BUFFER (buffer), NULL);

  format    = ligma_buffer_get_format (buffer);
  has_alpha = babl_format_has_alpha (format);

  image = ligma_create_image (ligma,
                             ligma_buffer_get_width  (buffer),
                             ligma_buffer_get_height (buffer),
                             ligma_babl_format_get_base_type (format),
                             ligma_babl_format_get_precision (format),
                             TRUE);
  ligma_image_undo_disable (image);

  if (ligma_buffer_get_resolution (buffer, &res_x, &res_y))
    {
      ligma_image_set_resolution (image, res_x, res_y);
      ligma_image_set_unit (image, ligma_buffer_get_unit (buffer));
    }

  profile = ligma_buffer_get_color_profile (buffer);
  ligma_image_set_color_profile (image, profile, NULL);

  layer = ligma_layer_new_from_buffer (buffer, image,
                                      ligma_image_get_layer_format (image,
                                                                   has_alpha),
                                      _("Pasted Layer"),
                                      LIGMA_OPACITY_OPAQUE,
                                      ligma_image_get_default_new_layer_mode (image));

  ligma_image_add_layer (image, layer, NULL, 0, TRUE);

  ligma_image_undo_enable (image);

  return image;
}

LigmaImage *
ligma_image_new_from_pixbuf (Ligma        *ligma,
                            GdkPixbuf   *pixbuf,
                            const gchar *layer_name)
{
  LigmaImage         *new_image;
  LigmaLayer         *layer;
  LigmaImageBaseType  base_type;
  gboolean           has_alpha = FALSE;
  guint8            *icc_data;
  gsize              icc_len;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  switch (gdk_pixbuf_get_n_channels (pixbuf))
    {
    case 2: has_alpha = TRUE;
    case 1: base_type = LIGMA_GRAY;
      break;

    case 4: has_alpha = TRUE;
    case 3: base_type = LIGMA_RGB;
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  new_image = ligma_create_image (ligma,
                                 gdk_pixbuf_get_width  (pixbuf),
                                 gdk_pixbuf_get_height (pixbuf),
                                 base_type,
                                 LIGMA_PRECISION_U8_NON_LINEAR,
                                 FALSE);

  ligma_image_undo_disable (new_image);

  icc_data = ligma_pixbuf_get_icc_profile (pixbuf, &icc_len);
  if (icc_data)
    {
      ligma_image_set_icc_profile (new_image, icc_data, icc_len,
                                  LIGMA_ICC_PROFILE_PARASITE_NAME,
                                  NULL);
      g_free (icc_data);
    }

  layer = ligma_layer_new_from_pixbuf (pixbuf, new_image,
                                      ligma_image_get_layer_format (new_image,
                                                                   has_alpha),
                                      layer_name,
                                      LIGMA_OPACITY_OPAQUE,
                                      ligma_image_get_default_new_layer_mode (new_image));

  ligma_image_add_layer (new_image, layer, NULL, 0, TRUE);

  ligma_image_undo_enable (new_image);

  return new_image;
}
