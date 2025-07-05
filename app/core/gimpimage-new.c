/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimpbuffer.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-fill.h"
#include "gimpgrouplayer.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-colormap.h"
#include "gimpimage-metadata.h"
#include "gimpimage-new.h"
#include "gimpimage-undo.h"
#include "gimplayer.h"
#include "gimplayer-new.h"
#include "gimplist.h"
#include "gimptemplate.h"

#include "gimp-intl.h"


static void   gimp_image_new_add_creation_metadata (GimpImage *image);


/*  public functions  */

GimpTemplate *
gimp_image_new_get_last_template (Gimp      *gimp,
                                  GimpImage *image)
{
  GimpTemplate *template;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (image == NULL || GIMP_IS_IMAGE (image), NULL);

  template = gimp_template_new ("image new values");

  if (image)
    {
      const gchar *comment;

      comment = gimp_template_get_comment (gimp->config->default_image);

      gimp_config_sync (G_OBJECT (gimp->config->default_image),
                        G_OBJECT (template), 0);
      gimp_template_set_from_image (template, image);

      /* Do not pass around the comment from the current active comment. Only
       * pass comments stored in actual templates. This can be even considered
       * as data leak otherwise (creating 2 images in a row and not realizing
       * the second will have the metadata comment from the first). See #11384.
       */
      g_object_set (template, "comment", comment, NULL);
    }
  else
    {
      GimpTemplate *last_template;

      last_template = gimp_get_last_template (gimp);

      gimp_config_sync (G_OBJECT (last_template),
                        G_OBJECT (template), 0);
    }

  return template;
}

void
gimp_image_new_set_last_template (Gimp         *gimp,
                                  GimpTemplate *template)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_TEMPLATE (template));

  gimp_set_last_template (gimp, template);
}

GimpImage *
gimp_image_new_from_template (Gimp         *gimp,
                              GimpTemplate *template,
                              GimpContext  *context)
{
  GimpImage               *image;
  GimpLayer               *layer;
  GimpColorProfile        *profile;
  GimpColorRenderingIntent intent;
  gboolean                 bpc;
  gint                     width, height;
  gboolean                 has_alpha;
  const gchar             *comment;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  image = gimp_create_image (gimp,
                             gimp_template_get_width (template),
                             gimp_template_get_height (template),
                             gimp_template_get_base_type (template),
                             gimp_template_get_precision (template),
                             FALSE);
  gimp_context_set_image (context, image);

  gimp_image_undo_disable (image);

  comment = gimp_template_get_comment (template);

  if (comment)
    {
      GimpParasite *parasite;

      parasite = gimp_parasite_new ("gimp-comment",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (comment) + 1,
                                    comment);
      gimp_image_parasite_attach (image, parasite, FALSE);
      gimp_parasite_free (parasite);
    }

  gimp_image_set_resolution (image,
                             gimp_template_get_resolution_x (template),
                             gimp_template_get_resolution_y (template));
  gimp_image_set_unit (image, gimp_template_get_resolution_unit (template));

  profile = gimp_template_get_color_profile (template);
  gimp_image_set_color_profile (image, profile, NULL);
  if (profile)
    g_object_unref (profile);

  profile = gimp_template_get_simulation_profile (template);
  gimp_image_set_simulation_profile (image, profile);
  if (profile)
    g_object_unref (profile);

  intent = gimp_template_get_simulation_intent (template);
  gimp_image_set_simulation_intent (image, intent);
  bpc = gimp_template_get_simulation_bpc (template);
  gimp_image_set_simulation_bpc (image, bpc);

  width  = gimp_image_get_width (image);
  height = gimp_image_get_height (image);

  if (gimp_template_get_fill_type (template) == GIMP_FILL_TRANSPARENT)
    has_alpha = TRUE;
  else
    has_alpha = FALSE;

  layer = gimp_layer_new (image, width, height,
                          gimp_image_get_layer_format (image, has_alpha),
                          _("Background"),
                          GIMP_OPACITY_OPAQUE,
                          gimp_image_get_default_new_layer_mode (image));

  gimp_drawable_fill (GIMP_DRAWABLE (layer),
                      context, gimp_template_get_fill_type (template));

  gimp_image_add_layer (image, layer, NULL, 0, FALSE);

  gimp_image_new_add_creation_metadata (image);

  gimp_image_undo_enable (image);
  gimp_image_clean_all (image);

  return image;
}

GimpImage *
gimp_image_new_from_drawable (Gimp         *gimp,
                              GimpDrawable *drawable)
{
  GimpItem          *item;
  GimpImage         *image;
  GimpImage         *new_image;
  GimpLayer         *new_layer;
  GType              new_type;
  gint               off_x, off_y;
  GimpImageBaseType  type;
  gdouble            xres;
  gdouble            yres;
  GimpColorProfile  *profile;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  item  = GIMP_ITEM (drawable);
  image = gimp_item_get_image (item);

  type = gimp_drawable_get_base_type (drawable);

  new_image = gimp_create_image (gimp,
                                 gimp_item_get_width  (item),
                                 gimp_item_get_height (item),
                                 type,
                                 gimp_drawable_get_precision (drawable),
                                 TRUE);
  gimp_image_undo_disable (new_image);

  if (type == GIMP_INDEXED)
    gimp_image_set_colormap_palette (new_image,
                                     gimp_image_get_colormap_palette (image),
                                     FALSE);

  gimp_image_get_resolution (image, &xres, &yres);
  gimp_image_set_resolution (new_image, xres, yres);
  gimp_image_set_unit (new_image, gimp_image_get_unit (image));

  profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (drawable));
  gimp_image_set_color_profile (new_image, profile, NULL);

  if (GIMP_IS_LAYER (drawable))
    new_type = G_TYPE_FROM_INSTANCE (drawable);
  else
    new_type = GIMP_TYPE_LAYER;

  new_layer = GIMP_LAYER (gimp_item_convert (GIMP_ITEM (drawable),
                                             new_image, new_type));

  gimp_object_set_name (GIMP_OBJECT (new_layer),
                        gimp_object_get_name (drawable));

  gimp_item_get_offset (GIMP_ITEM (new_layer), &off_x, &off_y);
  gimp_item_translate (GIMP_ITEM (new_layer), -off_x, -off_y, FALSE);
  gimp_item_set_visible (GIMP_ITEM (new_layer), TRUE, FALSE);
  gimp_layer_set_mode (new_layer,
                       gimp_image_get_default_new_layer_mode (new_image),
                       FALSE);
  gimp_layer_set_opacity (new_layer, GIMP_OPACITY_OPAQUE, FALSE);
  if (gimp_layer_can_lock_alpha (new_layer))
    gimp_layer_set_lock_alpha (new_layer, FALSE, FALSE);

  gimp_image_add_layer (new_image, new_layer, NULL, 0, TRUE);

  gimp_image_new_add_creation_metadata (new_image);

  gimp_image_undo_enable (new_image);

  return new_image;
}

/**
 * gimp_image_new_copy_drawables:
 * @image: the image where @drawables belong to.
 * @drawables: the drawables to copy into @new_image.
 * @new_image: the image to insert to.
 * @tag_copies: tag copies of @drawable with "gimp-image-copied-layer".
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
gimp_image_new_copy_drawables (GimpImage *image,
                               GList     *drawables,
                               GimpImage *new_image,
                               gboolean   tag_copies,
                               GList     *copied_drawables,
                               GList     *tagged_drawables,
                               GimpLayer *parent,
                               GimpLayer *new_parent)
{
  GList *layers;
  GList *iter;
  gint   n_drawables;
  gint   index;

  n_drawables = g_list_length (drawables);
  if (parent == NULL)
    {
      /* Root layers. */
      layers = gimp_image_get_layer_iter (image);

      copied_drawables = g_list_copy (drawables);
      for (iter = copied_drawables; iter; iter = iter->next)
        {
          /* Tagged drawables are the explicitly copied drawables which have no
           * explicitly copied descendant items.
           */
          GList *iter2;

          for (iter2 = iter; iter2; iter2 = iter2->next)
            if (gimp_viewable_is_ancestor (iter->data, iter2->data))
              break;

          if (iter2 == NULL)
            tagged_drawables = g_list_prepend (tagged_drawables, iter->data);
        }

      /* Add any item parent. */
      for (iter = copied_drawables; iter; iter = iter->next)
        {
          GimpItem *item = iter->data;
          while ((item = gimp_item_get_parent (item)))
            if (! g_list_find (copied_drawables, item))
              copied_drawables = g_list_prepend (copied_drawables, item);
        }
    }
  else
    {
      GimpContainer *container;

      container = gimp_viewable_get_children (GIMP_VIEWABLE (parent));
      layers = GIMP_LIST (container)->queue->head;
    }

  index = 0;
  for (iter = layers; iter; iter = iter->next)
    {
      if (g_list_find (copied_drawables, iter->data))
        {
          GimpLayer *new_layer;
          GType      new_type;
          gboolean   is_group;
          gboolean   is_tagged;

          if (GIMP_IS_LAYER (iter->data))
            new_type = G_TYPE_FROM_INSTANCE (iter->data);
          else
            new_type = GIMP_TYPE_LAYER;

          is_group  = (gimp_viewable_get_children (iter->data) != NULL);
          is_tagged = (g_list_find (tagged_drawables, iter->data) != NULL);

          if (is_group && ! is_tagged)
            new_layer = gimp_group_layer_new (new_image);
          else
            new_layer = GIMP_LAYER (gimp_item_convert (GIMP_ITEM (iter->data),
                                                       new_image, new_type));

          if (tag_copies && is_tagged)
            g_object_set_data (G_OBJECT (new_layer),
                               "gimp-image-copied-layer",
                               GINT_TO_POINTER (TRUE));

          gimp_object_set_name (GIMP_OBJECT (new_layer),
                                gimp_object_get_name (iter->data));

          /* Visibility, mode and opacity mimic the source image if
           * multiple items are copied. Otherwise we just set them to
           * defaults.
           */
          gimp_item_set_visible (GIMP_ITEM (new_layer),
                                 n_drawables > 1 ?
                                 gimp_item_get_visible (iter->data) : TRUE,
                                 FALSE);
          gimp_layer_set_mode (new_layer,
                               n_drawables > 1 && GIMP_IS_LAYER (iter->data) ?
                               gimp_layer_get_mode (iter->data) :
                               gimp_image_get_default_new_layer_mode (new_image),
                               FALSE);
          gimp_layer_set_opacity (new_layer,
                                  n_drawables > 1 && GIMP_IS_LAYER (iter->data) ?
                                  gimp_layer_get_opacity (iter->data) : GIMP_OPACITY_OPAQUE, FALSE);

          if (gimp_layer_can_lock_alpha (new_layer))
            gimp_layer_set_lock_alpha (new_layer, FALSE, FALSE);

          gimp_image_add_layer (new_image, new_layer, new_parent, index++, TRUE);

          /* If a group, loop through children. */
          if (is_group && ! is_tagged)
            gimp_image_new_copy_drawables (image, drawables, new_image, tag_copies,
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

GimpImage *
gimp_image_new_from_drawables (Gimp     *gimp,
                               GList    *drawables,
                               gboolean  copy_selection,
                               gboolean  tag_copies)
{
  GimpImage         *image = NULL;
  GimpImage         *new_image;
  GList             *iter;
  GimpImageBaseType  type;
  GimpPrecision      precision;
  gdouble            xres;
  gdouble            yres;
  GimpColorProfile  *profile = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (drawables != NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (GIMP_IS_DRAWABLE (iter->data), NULL);

      if (iter == drawables)
        image = gimp_item_get_image (iter->data);
      else
        /* We only accept list of drawables for a same origin image. */
        g_return_val_if_fail (gimp_item_get_image (iter->data) == image, NULL);
    }

  type      = gimp_drawable_get_base_type (drawables->data);
  precision = gimp_drawable_get_precision (drawables->data);
  profile   = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (drawables->data));

  new_image = gimp_create_image (gimp,
                                 gimp_image_get_width  (image),
                                 gimp_image_get_height (image),
                                 type, precision,
                                 TRUE);
  gimp_image_undo_disable (new_image);

  if (type == GIMP_INDEXED)
    gimp_image_set_colormap_palette (new_image,
                                     gimp_image_get_colormap_palette (image),
                                     FALSE);

  gimp_image_get_resolution (image, &xres, &yres);
  gimp_image_set_resolution (new_image, xres, yres);
  gimp_image_set_unit (new_image, gimp_image_get_unit (image));
  if (profile)
    gimp_image_set_color_profile (new_image, profile, NULL);

  if (copy_selection)
    {
      GimpChannel *selection;

      selection = gimp_image_get_mask (image);
      if (! gimp_channel_is_empty (selection))
        {
          GimpChannel *new_selection;
          GeglBuffer  *buffer;

          new_selection = gimp_image_get_mask (new_image);
          buffer = gimp_gegl_buffer_dup (gimp_drawable_get_buffer (GIMP_DRAWABLE (selection)));
          gimp_drawable_set_buffer (GIMP_DRAWABLE (new_selection),
                                    FALSE, NULL, buffer);
          g_object_unref (buffer);
        }
    }

  gimp_image_new_copy_drawables (image, drawables, new_image, tag_copies, NULL, NULL, NULL, NULL);

  gimp_image_new_add_creation_metadata (new_image);

  gimp_image_undo_enable (new_image);

  return new_image;
}

GimpImage *
gimp_image_new_from_component (Gimp            *gimp,
                               GimpImage       *image,
                               GimpChannelType  component)
{
  GimpImage   *new_image;
  GimpChannel *channel;
  GimpLayer   *layer;
  const gchar *desc;
  gdouble      xres;
  gdouble      yres;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  new_image = gimp_create_image (gimp,
                                 gimp_image_get_width  (image),
                                 gimp_image_get_height (image),
                                 GIMP_GRAY,
                                 gimp_image_get_precision (image),
                                 TRUE);

  gimp_image_undo_disable (new_image);

  gimp_image_get_resolution (image, &xres, &yres);
  gimp_image_set_resolution (new_image, xres, yres);
  gimp_image_set_unit (new_image, gimp_image_get_unit (image));

  channel = gimp_channel_new_from_component (image, component, NULL, NULL);

  layer = GIMP_LAYER (gimp_item_convert (GIMP_ITEM (channel),
                                         new_image, GIMP_TYPE_LAYER));
  g_object_unref (channel);

  gimp_enum_get_value (GIMP_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  gimp_object_take_name (GIMP_OBJECT (layer),
                         g_strdup_printf (_("%s Channel Copy"), desc));

  gimp_image_add_layer (new_image, layer, NULL, 0, TRUE);

  gimp_image_new_add_creation_metadata (new_image);

  gimp_image_undo_enable (new_image);

  return new_image;
}

GimpImage *
gimp_image_new_from_buffer (Gimp       *gimp,
                            GimpBuffer *buffer)
{
  GimpImage        *image;
  GimpLayer        *layer;
  const Babl       *format;
  gboolean          has_alpha;
  gdouble           res_x;
  gdouble           res_y;
  GimpColorProfile *profile;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_BUFFER (buffer), NULL);

  format    = gimp_buffer_get_format (buffer);
  has_alpha = babl_format_has_alpha (format);

  image = gimp_create_image (gimp,
                             gimp_buffer_get_width  (buffer),
                             gimp_buffer_get_height (buffer),
                             gimp_babl_format_get_base_type (format),
                             gimp_babl_format_get_precision (format),
                             TRUE);
  gimp_image_undo_disable (image);

  if (gimp_buffer_get_resolution (buffer, &res_x, &res_y))
    {
      gimp_image_set_resolution (image, res_x, res_y);
      gimp_image_set_unit (image, gimp_buffer_get_unit (buffer));
    }

  profile = gimp_buffer_get_color_profile (buffer);
  gimp_image_set_color_profile (image, profile, NULL);

  layer = gimp_layer_new_from_buffer (buffer, image,
                                      gimp_image_get_layer_format (image,
                                                                   has_alpha),
                                      _("Pasted Layer"),
                                      GIMP_OPACITY_OPAQUE,
                                      gimp_image_get_default_new_layer_mode (image));

  gimp_image_add_layer (image, layer, NULL, 0, TRUE);

  gimp_image_new_add_creation_metadata (image);

  gimp_image_undo_enable (image);

  return image;
}

GimpImage *
gimp_image_new_from_pixbuf (Gimp        *gimp,
                            GdkPixbuf   *pixbuf,
                            const gchar *layer_name)
{
  GimpImage         *new_image;
  GimpLayer         *layer;
  GimpImageBaseType  base_type;
  gboolean           has_alpha = FALSE;
  guint8            *icc_data;
  gsize              icc_len;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  switch (gdk_pixbuf_get_n_channels (pixbuf))
    {
    case 2: has_alpha = TRUE;
    case 1: base_type = GIMP_GRAY;
      break;

    case 4: has_alpha = TRUE;
    case 3: base_type = GIMP_RGB;
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  new_image = gimp_create_image (gimp,
                                 gdk_pixbuf_get_width  (pixbuf),
                                 gdk_pixbuf_get_height (pixbuf),
                                 base_type,
                                 GIMP_PRECISION_U8_NON_LINEAR,
                                 FALSE);

  gimp_image_undo_disable (new_image);

  icc_data = gimp_pixbuf_get_icc_profile (pixbuf, &icc_len);
  if (icc_data)
    {
      gimp_image_set_icc_profile (new_image, icc_data, icc_len,
                                  GIMP_ICC_PROFILE_PARASITE_NAME,
                                  NULL);
      g_free (icc_data);
    }

  layer = gimp_layer_new_from_pixbuf (pixbuf, new_image,
                                      gimp_image_get_layer_format (new_image,
                                                                   has_alpha),
                                      layer_name,
                                      GIMP_OPACITY_OPAQUE,
                                      gimp_image_get_default_new_layer_mode (new_image));

  gimp_image_add_layer (new_image, layer, NULL, 0, TRUE);

  gimp_image_new_add_creation_metadata (new_image);

  gimp_image_undo_enable (new_image);

  return new_image;
}


/*  private functions  */

static void
gimp_image_new_add_creation_metadata (GimpImage *image)
{
  GimpMetadata *metadata;

  metadata = gimp_image_get_metadata (image);
  if (! metadata)
    {
      g_critical ("Metadata not found. Should not happen!");
    }
  else
    {
      GDateTime *datetime;

      datetime = g_date_time_new_now_local ();
      gimp_metadata_set_creation_date (metadata, datetime);
      g_date_time_unref (datetime);
    }
}
