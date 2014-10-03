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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "libgimpbase/gimpbase.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-duplicate.h"
#include "gimpimage-grid.h"
#include "gimpimage-guides.h"
#include "gimpimage-metadata.h"
#include "gimpimage-private.h"
#include "gimpimage-undo.h"
#include "gimpimage-sample-points.h"
#include "gimpitemstack.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimplayer-floating-sel.h"
#include "gimpparasitelist.h"
#include "gimpsamplepoint.h"

#include "vectors/gimpvectors.h"


static void          gimp_image_duplicate_resolution       (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_save_source_file (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_colormap         (GimpImage *image,
                                                            GimpImage *new_image);
static GimpItem    * gimp_image_duplicate_item             (GimpItem  *item,
                                                            GimpImage *new_image);
static GimpLayer   * gimp_image_duplicate_layers           (GimpImage *image,
                                                            GimpImage *new_image);
static GimpChannel * gimp_image_duplicate_channels         (GimpImage *image,
                                                            GimpImage *new_image);
static GimpVectors * gimp_image_duplicate_vectors          (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_floating_sel     (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_mask             (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_components       (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_guides           (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_sample_points    (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_grid             (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_metadata         (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_quick_mask       (GimpImage *image,
                                                            GimpImage *new_image);
static void          gimp_image_duplicate_parasites        (GimpImage *image,
                                                            GimpImage *new_image);


GimpImage *
gimp_image_duplicate (GimpImage *image)
{
  GimpImage    *new_image;
  GimpLayer    *active_layer;
  GimpChannel  *active_channel;
  GimpVectors  *active_vectors;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  gimp_set_busy_until_idle (image->gimp);

  /*  Create a new image  */
  new_image = gimp_create_image (image->gimp,
                                 gimp_image_get_width  (image),
                                 gimp_image_get_height (image),
                                 gimp_image_get_base_type (image),
                                 gimp_image_get_precision (image),
                                 FALSE);
  gimp_image_undo_disable (new_image);

  /*  Store the source uri to be used by the save dialog  */
  gimp_image_duplicate_save_source_file (image, new_image);

  /*  Copy the colormap if necessary  */
  gimp_image_duplicate_colormap (image, new_image);

  /*  Copy resolution information  */
  gimp_image_duplicate_resolution (image, new_image);

  /*  Copy the layers  */
  active_layer = gimp_image_duplicate_layers (image, new_image);

  /*  Copy the channels  */
  active_channel = gimp_image_duplicate_channels (image, new_image);

  /*  Copy any vectors  */
  active_vectors = gimp_image_duplicate_vectors (image, new_image);

  /*  Copy floating layer  */
  gimp_image_duplicate_floating_sel (image, new_image);

  /*  Copy the selection mask  */
  gimp_image_duplicate_mask (image, new_image);

  /*  Set active layer, active channel, active vectors  */
  if (active_layer)
    gimp_image_set_active_layer (new_image, active_layer);

  if (active_channel)
    gimp_image_set_active_channel (new_image, active_channel);

  if (active_vectors)
    gimp_image_set_active_vectors (new_image, active_vectors);

  /*  Copy state of all color components  */
  gimp_image_duplicate_components (image, new_image);

  /*  Copy any guides  */
  gimp_image_duplicate_guides (image, new_image);

  /*  Copy any sample points  */
  gimp_image_duplicate_sample_points (image, new_image);

  /*  Copy the grid  */
  gimp_image_duplicate_grid (image, new_image);

  /*  Copy the metadata  */
  gimp_image_duplicate_metadata (image, new_image);

  /*  Copy the quick mask info  */
  gimp_image_duplicate_quick_mask (image, new_image);

  /*  Copy parasites  */
  gimp_image_duplicate_parasites (image, new_image);

  gimp_image_undo_enable (new_image);

  return new_image;
}

static void
gimp_image_duplicate_resolution (GimpImage *image,
                                 GimpImage *new_image)
{
  gdouble xres;
  gdouble yres;

  gimp_image_get_resolution (image, &xres, &yres);
  gimp_image_set_resolution (new_image, xres, yres);
  gimp_image_set_unit (new_image, gimp_image_get_unit (image));
}

static void
gimp_image_duplicate_save_source_file (GimpImage *image,
                                       GimpImage *new_image)
{
  GFile *file = gimp_image_get_file (image);

  if (file)
    g_object_set_data_full (G_OBJECT (new_image), "gimp-image-source-file",
                            g_object_ref (file),
                            (GDestroyNotify) g_object_unref);
}

static void
gimp_image_duplicate_colormap (GimpImage *image,
                               GimpImage *new_image)
{
  if (gimp_image_get_base_type (new_image) == GIMP_INDEXED)
    gimp_image_set_colormap (new_image,
                             gimp_image_get_colormap (image),
                             gimp_image_get_colormap_size (image),
                             FALSE);
}

static GimpItem *
gimp_image_duplicate_item (GimpItem  *item,
                           GimpImage *new_image)
{
  GimpItem *new_item;

  new_item = gimp_item_convert (item, new_image,
                                G_TYPE_FROM_INSTANCE (item));

  /*  Make sure the copied item doesn't say: "<old item> copy"  */
  gimp_object_set_name (GIMP_OBJECT (new_item),
                        gimp_object_get_name (item));

  return new_item;
}

static GimpLayer *
gimp_image_duplicate_layers (GimpImage *image,
                             GimpImage *new_image)
{
  GimpLayer *active_layer = NULL;
  GList     *list;
  gint       count;

  for (list = gimp_image_get_layer_iter (image), count = 0;
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;
      GimpLayer *new_layer;

      if (gimp_layer_is_floating_sel (layer))
        continue;

      new_layer = GIMP_LAYER (gimp_image_duplicate_item (GIMP_ITEM (layer),
                                                         new_image));

      /*  Make sure that if the layer has a layer mask,
       *  its name isn't screwed up
       */
      if (new_layer->mask)
        gimp_object_set_name (GIMP_OBJECT (new_layer->mask),
                              gimp_object_get_name (layer->mask));

      if (gimp_image_get_active_layer (image) == layer)
        active_layer = new_layer;

      gimp_image_add_layer (new_image, new_layer,
                            NULL, count++, FALSE);
    }

  return active_layer;
}

static GimpChannel *
gimp_image_duplicate_channels (GimpImage *image,
                               GimpImage *new_image)
{
  GimpChannel *active_channel = NULL;
  GList       *list;
  gint         count;

  for (list = gimp_image_get_channel_iter (image), count = 0;
       list;
       list = g_list_next (list))
    {
      GimpChannel  *channel = list->data;
      GimpChannel  *new_channel;

      new_channel = GIMP_CHANNEL (gimp_image_duplicate_item (GIMP_ITEM (channel),
                                                             new_image));

      if (gimp_image_get_active_channel (image) == channel)
        active_channel = new_channel;

      gimp_image_add_channel (new_image, new_channel,
                              NULL, count++, FALSE);
    }

  return active_channel;
}

static GimpVectors *
gimp_image_duplicate_vectors (GimpImage *image,
                              GimpImage *new_image)
{
  GimpVectors *active_vectors = NULL;
  GList       *list;
  gint         count;

  for (list = gimp_image_get_vectors_iter (image), count = 0;
       list;
       list = g_list_next (list))
    {
      GimpVectors  *vectors = list->data;
      GimpVectors  *new_vectors;

      new_vectors = GIMP_VECTORS (gimp_image_duplicate_item (GIMP_ITEM (vectors),
                                                             new_image));

      if (gimp_image_get_active_vectors (image) == vectors)
        active_vectors = new_vectors;

      gimp_image_add_vectors (new_image, new_vectors,
                              NULL, count++, FALSE);
    }

  return active_vectors;
}

static void
gimp_image_duplicate_floating_sel (GimpImage *image,
                                   GimpImage *new_image)
{
  GimpLayer     *floating_sel;
  GimpDrawable  *floating_sel_drawable;
  GList         *floating_sel_path;
  GimpItemStack *new_item_stack;
  GimpLayer     *new_floating_sel;
  GimpDrawable  *new_floating_sel_drawable;

  floating_sel = gimp_image_get_floating_selection (image);

  if (! floating_sel)
    return;

  floating_sel_drawable = gimp_layer_get_floating_sel_drawable (floating_sel);

  if (GIMP_IS_LAYER_MASK (floating_sel_drawable))
    {
      GimpLayer *layer;

      layer = gimp_layer_mask_get_layer (GIMP_LAYER_MASK (floating_sel_drawable));

      floating_sel_path = gimp_item_get_path (GIMP_ITEM (layer));

      new_item_stack = GIMP_ITEM_STACK (gimp_image_get_layers (new_image));
    }
  else
    {
      floating_sel_path = gimp_item_get_path (GIMP_ITEM (floating_sel_drawable));

      if (GIMP_IS_LAYER (floating_sel_drawable))
        new_item_stack = GIMP_ITEM_STACK (gimp_image_get_layers (new_image));
      else
        new_item_stack = GIMP_ITEM_STACK (gimp_image_get_channels (new_image));
    }

  /*  adjust path[0] for the floating layer missing in new_image  */
  floating_sel_path->data =
    GUINT_TO_POINTER (GPOINTER_TO_UINT (floating_sel_path->data) - 1);

  if (GIMP_IS_LAYER (floating_sel_drawable))
    {
      new_floating_sel =
        GIMP_LAYER (gimp_image_duplicate_item (GIMP_ITEM (floating_sel),
                                               new_image));
    }
  else
    {
      /*  can't use gimp_item_convert() for floating selections of channels
       *  or layer masks because they maybe don't have a normal layer's type
       */
      new_floating_sel =
        GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (floating_sel),
                                         G_TYPE_FROM_INSTANCE (floating_sel)));
      gimp_item_set_image (GIMP_ITEM (new_floating_sel), new_image);

      gimp_object_set_name (GIMP_OBJECT (new_floating_sel),
                            gimp_object_get_name (floating_sel));
    }

  /*  Make sure the copied layer doesn't say: "<old layer> copy"  */
  gimp_object_set_name (GIMP_OBJECT (new_floating_sel),
                        gimp_object_get_name (floating_sel));

  new_floating_sel_drawable =
    GIMP_DRAWABLE (gimp_item_stack_get_item_by_path (new_item_stack,
                                                     floating_sel_path));

  if (GIMP_IS_LAYER_MASK (floating_sel_drawable))
    new_floating_sel_drawable =
      GIMP_DRAWABLE (gimp_layer_get_mask (GIMP_LAYER (new_floating_sel_drawable)));

  floating_sel_attach (new_floating_sel, new_floating_sel_drawable);

  g_list_free (floating_sel_path);
}

static void
gimp_image_duplicate_mask (GimpImage *image,
                           GimpImage *new_image)
{
  GimpDrawable *mask;
  GimpDrawable *new_mask;

  mask     = GIMP_DRAWABLE (gimp_image_get_mask (image));
  new_mask = GIMP_DRAWABLE (gimp_image_get_mask (new_image));

  gegl_buffer_copy (gimp_drawable_get_buffer (mask), NULL,
                    gimp_drawable_get_buffer (new_mask), NULL);

  GIMP_CHANNEL (new_mask)->bounds_known   = FALSE;
  GIMP_CHANNEL (new_mask)->boundary_known = FALSE;
}

static void
gimp_image_duplicate_components (GimpImage *image,
                                 GimpImage *new_image)
{
  GimpImagePrivate *private     = GIMP_IMAGE_GET_PRIVATE (image);
  GimpImagePrivate *new_private = GIMP_IMAGE_GET_PRIVATE (new_image);
  gint              count;

  for (count = 0; count < MAX_CHANNELS; count++)
    {
      new_private->visible[count] = private->visible[count];
      new_private->active[count]  = private->active[count];
    }
}

static void
gimp_image_duplicate_guides (GimpImage *image,
                             GimpImage *new_image)
{
  GList *list;

  for (list = gimp_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      GimpGuide *guide    = list->data;
      gint       position = gimp_guide_get_position (guide);

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          gimp_image_add_hguide (new_image, position, FALSE);
          break;

        case GIMP_ORIENTATION_VERTICAL:
          gimp_image_add_vguide (new_image, position, FALSE);
          break;

        default:
          g_error ("Unknown guide orientation.\n");
        }
    }
}

static void
gimp_image_duplicate_sample_points (GimpImage *image,
                                    GimpImage *new_image)
{
  GList *list;

  for (list = gimp_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      GimpSamplePoint *sample_point = list->data;

      gimp_image_add_sample_point_at_pos (new_image,
                                          sample_point->x,
                                          sample_point->y,
                                          FALSE);
    }
}

static void
gimp_image_duplicate_grid (GimpImage *image,
                           GimpImage *new_image)
{
  if (gimp_image_get_grid (image))
    gimp_image_set_grid (new_image, gimp_image_get_grid (image), FALSE);
}

static void
gimp_image_duplicate_metadata (GimpImage *image,
                               GimpImage *new_image)
{
  GimpMetadata *metadata = gimp_image_get_metadata (image);

  if (metadata)
    {
      metadata = gimp_metadata_duplicate (metadata);
      gimp_image_set_metadata (new_image, metadata, FALSE);
      g_object_unref (metadata);
    }
}

static void
gimp_image_duplicate_quick_mask (GimpImage *image,
                                 GimpImage *new_image)
{
  GimpImagePrivate *private     = GIMP_IMAGE_GET_PRIVATE (image);
  GimpImagePrivate *new_private = GIMP_IMAGE_GET_PRIVATE (new_image);

  new_private->quick_mask_state    = private->quick_mask_state;
  new_private->quick_mask_inverted = private->quick_mask_inverted;
  new_private->quick_mask_color    = private->quick_mask_color;
}

static void
gimp_image_duplicate_parasites (GimpImage *image,
                                GimpImage *new_image)
{
  GimpImagePrivate *private     = GIMP_IMAGE_GET_PRIVATE (image);
  GimpImagePrivate *new_private = GIMP_IMAGE_GET_PRIVATE (new_image);

  if (private->parasites)
    {
      g_object_unref (new_private->parasites);
      new_private->parasites = gimp_parasite_list_copy (private->parasites);
    }
}
