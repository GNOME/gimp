/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "gegl/ligma-gegl-loops.h"

#include "vectors/ligmavectors.h"

#include "ligma.h"
#include "ligmachannel.h"
#include "ligmaguide.h"
#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmaimage-colormap.h"
#include "ligmaimage-duplicate.h"
#include "ligmaimage-grid.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-metadata.h"
#include "ligmaimage-private.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-sample-points.h"
#include "ligmaitemstack.h"
#include "ligmalayer.h"
#include "ligmalayermask.h"
#include "ligmalayer-floating-selection.h"
#include "ligmaparasitelist.h"
#include "ligmasamplepoint.h"


static void          ligma_image_duplicate_resolution       (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_save_source_file (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_colormap         (LigmaImage *image,
                                                            LigmaImage *new_image);
static LigmaItem    * ligma_image_duplicate_item             (LigmaItem  *item,
                                                            LigmaImage *new_image);
static GList       * ligma_image_duplicate_layers           (LigmaImage *image,
                                                            LigmaImage *new_image);
static GList       * ligma_image_duplicate_channels         (LigmaImage *image,
                                                            LigmaImage *new_image);
static GList       * ligma_image_duplicate_vectors          (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_floating_sel     (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_mask             (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_components       (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_guides           (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_sample_points    (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_grid             (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_metadata         (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_quick_mask       (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_parasites        (LigmaImage *image,
                                                            LigmaImage *new_image);
static void          ligma_image_duplicate_color_profile    (LigmaImage *image,
                                                            LigmaImage *new_image);


LigmaImage *
ligma_image_duplicate (LigmaImage *image)
{
  LigmaImage    *new_image;
  GList        *active_layers;
  GList        *active_channels;
  GList        *active_vectors;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  ligma_set_busy_until_idle (image->ligma);

  /*  Create a new image  */
  new_image = ligma_create_image (image->ligma,
                                 ligma_image_get_width  (image),
                                 ligma_image_get_height (image),
                                 ligma_image_get_base_type (image),
                                 ligma_image_get_precision (image),
                                 FALSE);
  ligma_image_undo_disable (new_image);

  /*  Store the source uri to be used by the save dialog  */
  ligma_image_duplicate_save_source_file (image, new_image);

  /*  Copy resolution information  */
  ligma_image_duplicate_resolution (image, new_image);

  /*  Copy parasites first so we have a color profile  */
  ligma_image_duplicate_parasites (image, new_image);
  ligma_image_duplicate_color_profile (image, new_image);

  /*  Copy the colormap if necessary  */
  ligma_image_duplicate_colormap (image, new_image);

  /*  Copy the layers  */
  active_layers = ligma_image_duplicate_layers (image, new_image);

  /*  Copy the channels  */
  active_channels = ligma_image_duplicate_channels (image, new_image);

  /*  Copy any vectors  */
  active_vectors = ligma_image_duplicate_vectors (image, new_image);

  /*  Copy floating layer  */
  ligma_image_duplicate_floating_sel (image, new_image);

  /*  Copy the selection mask  */
  ligma_image_duplicate_mask (image, new_image);

  /*  Set active layer, active channel, active vectors  */
  if (active_layers)
    ligma_image_set_selected_layers (new_image, active_layers);

  if (active_channels)
    ligma_image_set_selected_channels (new_image, active_channels);

  if (active_vectors)
    ligma_image_set_selected_vectors (new_image, active_vectors);

  /*  Copy state of all color components  */
  ligma_image_duplicate_components (image, new_image);

  /*  Copy any guides  */
  ligma_image_duplicate_guides (image, new_image);

  /*  Copy any sample points  */
  ligma_image_duplicate_sample_points (image, new_image);

  /*  Copy the grid  */
  ligma_image_duplicate_grid (image, new_image);

  /*  Copy the metadata  */
  ligma_image_duplicate_metadata (image, new_image);

  /*  Copy the quick mask info  */
  ligma_image_duplicate_quick_mask (image, new_image);

  ligma_image_undo_enable (new_image);

  /*  Explicitly mark image as dirty, so that its dirty time is set  */
  ligma_image_dirty (new_image, LIGMA_DIRTY_ALL);

  return new_image;
}

static void
ligma_image_duplicate_resolution (LigmaImage *image,
                                 LigmaImage *new_image)
{
  gdouble xres;
  gdouble yres;

  ligma_image_get_resolution (image, &xres, &yres);
  ligma_image_set_resolution (new_image, xres, yres);
  ligma_image_set_unit (new_image, ligma_image_get_unit (image));
}

static void
ligma_image_duplicate_save_source_file (LigmaImage *image,
                                       LigmaImage *new_image)
{
  GFile *file = ligma_image_get_file (image);

  if (file)
    g_object_set_data_full (G_OBJECT (new_image), "ligma-image-source-file",
                            g_object_ref (file),
                            (GDestroyNotify) g_object_unref);
}

static void
ligma_image_duplicate_colormap (LigmaImage *image,
                               LigmaImage *new_image)
{
  if (ligma_image_get_base_type (new_image) == LIGMA_INDEXED)
    ligma_image_set_colormap_palette (new_image,
                                     ligma_image_get_colormap_palette (image),
                                     FALSE);
}

static LigmaItem *
ligma_image_duplicate_item (LigmaItem  *item,
                           LigmaImage *new_image)
{
  LigmaItem *new_item;

  new_item = ligma_item_convert (item, new_image,
                                G_TYPE_FROM_INSTANCE (item));

  /*  Make sure the copied item doesn't say: "<old item> copy"  */
  ligma_object_set_name (LIGMA_OBJECT (new_item),
                        ligma_object_get_name (item));

  return new_item;
}

static GList *
ligma_image_duplicate_layers (LigmaImage *image,
                             LigmaImage *new_image)
{
  GList         *new_selected_layers = NULL;
  GList         *selected_paths      = NULL;
  GList         *selected_layers;
  LigmaItemStack *new_item_stack;
  GList         *list;
  gint           count;

  selected_layers = ligma_image_get_selected_layers (image);

  for (list = selected_layers; list; list = list->next)
    selected_paths = g_list_prepend (selected_paths,
                                     ligma_item_get_path (list->data));

  for (list = ligma_image_get_layer_iter (image), count = 0;
       list;
       list = g_list_next (list))
    {
      LigmaLayer *layer = list->data;
      LigmaLayer *new_layer;

      if (ligma_layer_is_floating_sel (layer))
        continue;

      new_layer = LIGMA_LAYER (ligma_image_duplicate_item (LIGMA_ITEM (layer),
                                                         new_image));

      /*  Make sure that if the layer has a layer mask,
       *  its name isn't screwed up
       */
      if (new_layer->mask)
        ligma_object_set_name (LIGMA_OBJECT (new_layer->mask),
                              ligma_object_get_name (layer->mask));

      ligma_image_add_layer (new_image, new_layer,
                            NULL, count++, FALSE);
    }

  new_item_stack = LIGMA_ITEM_STACK (ligma_image_get_layers (new_image));
  for (list = selected_paths; list; list = list->next)
    new_selected_layers = g_list_prepend (new_selected_layers,
                                          ligma_item_stack_get_item_by_path (new_item_stack, list->data));

  g_list_free_full (selected_paths, (GDestroyNotify) g_list_free);

  return new_selected_layers;
}

static GList *
ligma_image_duplicate_channels (LigmaImage *image,
                               LigmaImage *new_image)
{
  GList *new_selected_channels = NULL;
  GList *selected_channels;
  GList *list;
  gint   count;

  selected_channels = ligma_image_get_selected_channels (image);

  for (list = ligma_image_get_channel_iter (image), count = 0;
       list;
       list = g_list_next (list))
    {
      LigmaChannel  *channel = list->data;
      LigmaChannel  *new_channel;

      new_channel = LIGMA_CHANNEL (ligma_image_duplicate_item (LIGMA_ITEM (channel),
                                                             new_image));

      if (g_list_find (selected_channels, channel))
        new_selected_channels = g_list_prepend (new_selected_channels, new_channel);

      ligma_image_add_channel (new_image, new_channel,
                              NULL, count++, FALSE);
    }

  return new_selected_channels;
}

static GList *
ligma_image_duplicate_vectors (LigmaImage *image,
                              LigmaImage *new_image)
{
  GList *new_selected_vectors = NULL;
  GList *selected_vectors;
  GList *list;
  gint   count;

  selected_vectors = ligma_image_get_selected_vectors (image);

  for (list = ligma_image_get_vectors_iter (image), count = 0;
       list;
       list = g_list_next (list))
    {
      LigmaVectors  *vectors = list->data;
      LigmaVectors  *new_vectors;

      new_vectors = LIGMA_VECTORS (ligma_image_duplicate_item (LIGMA_ITEM (vectors),
                                                             new_image));

      if (g_list_find (selected_vectors, vectors))
        new_selected_vectors = g_list_prepend (new_selected_vectors, new_vectors);


      ligma_image_add_vectors (new_image, new_vectors,
                              NULL, count++, FALSE);
    }

  return new_selected_vectors;
}

static void
ligma_image_duplicate_floating_sel (LigmaImage *image,
                                   LigmaImage *new_image)
{
  LigmaLayer     *floating_sel;
  LigmaDrawable  *floating_sel_drawable;
  GList         *floating_sel_path;
  LigmaItemStack *new_item_stack;
  LigmaLayer     *new_floating_sel;
  LigmaDrawable  *new_floating_sel_drawable;

  floating_sel = ligma_image_get_floating_selection (image);

  if (! floating_sel)
    return;

  floating_sel_drawable = ligma_layer_get_floating_sel_drawable (floating_sel);

  if (LIGMA_IS_LAYER_MASK (floating_sel_drawable))
    {
      LigmaLayer *layer;

      layer = ligma_layer_mask_get_layer (LIGMA_LAYER_MASK (floating_sel_drawable));

      floating_sel_path = ligma_item_get_path (LIGMA_ITEM (layer));

      new_item_stack = LIGMA_ITEM_STACK (ligma_image_get_layers (new_image));
    }
  else
    {
      floating_sel_path = ligma_item_get_path (LIGMA_ITEM (floating_sel_drawable));

      if (LIGMA_IS_LAYER (floating_sel_drawable))
        new_item_stack = LIGMA_ITEM_STACK (ligma_image_get_layers (new_image));
      else
        new_item_stack = LIGMA_ITEM_STACK (ligma_image_get_channels (new_image));
    }

  /*  adjust path[0] for the floating layer missing in new_image  */
  floating_sel_path->data =
    GUINT_TO_POINTER (GPOINTER_TO_UINT (floating_sel_path->data) - 1);

  if (LIGMA_IS_LAYER (floating_sel_drawable))
    {
      new_floating_sel =
        LIGMA_LAYER (ligma_image_duplicate_item (LIGMA_ITEM (floating_sel),
                                               new_image));
    }
  else
    {
      /*  can't use ligma_item_convert() for floating selections of channels
       *  or layer masks because they maybe don't have a normal layer's type
       */
      new_floating_sel =
        LIGMA_LAYER (ligma_item_duplicate (LIGMA_ITEM (floating_sel),
                                         G_TYPE_FROM_INSTANCE (floating_sel)));
      ligma_item_set_image (LIGMA_ITEM (new_floating_sel), new_image);

      ligma_object_set_name (LIGMA_OBJECT (new_floating_sel),
                            ligma_object_get_name (floating_sel));
    }

  /*  Make sure the copied layer doesn't say: "<old layer> copy"  */
  ligma_object_set_name (LIGMA_OBJECT (new_floating_sel),
                        ligma_object_get_name (floating_sel));

  new_floating_sel_drawable =
    LIGMA_DRAWABLE (ligma_item_stack_get_item_by_path (new_item_stack,
                                                     floating_sel_path));

  if (LIGMA_IS_LAYER_MASK (floating_sel_drawable))
    new_floating_sel_drawable =
      LIGMA_DRAWABLE (ligma_layer_get_mask (LIGMA_LAYER (new_floating_sel_drawable)));

  floating_sel_attach (new_floating_sel, new_floating_sel_drawable);

  g_list_free (floating_sel_path);
}

static void
ligma_image_duplicate_mask (LigmaImage *image,
                           LigmaImage *new_image)
{
  LigmaDrawable *mask;
  LigmaDrawable *new_mask;

  mask     = LIGMA_DRAWABLE (ligma_image_get_mask (image));
  new_mask = LIGMA_DRAWABLE (ligma_image_get_mask (new_image));

  ligma_gegl_buffer_copy (ligma_drawable_get_buffer (mask), NULL, GEGL_ABYSS_NONE,
                         ligma_drawable_get_buffer (new_mask), NULL);

  LIGMA_CHANNEL (new_mask)->bounds_known   = FALSE;
  LIGMA_CHANNEL (new_mask)->boundary_known = FALSE;
}

static void
ligma_image_duplicate_components (LigmaImage *image,
                                 LigmaImage *new_image)
{
  LigmaImagePrivate *private     = LIGMA_IMAGE_GET_PRIVATE (image);
  LigmaImagePrivate *new_private = LIGMA_IMAGE_GET_PRIVATE (new_image);
  gint              count;

  for (count = 0; count < MAX_CHANNELS; count++)
    {
      new_private->visible[count] = private->visible[count];
      new_private->active[count]  = private->active[count];
    }
}

static void
ligma_image_duplicate_guides (LigmaImage *image,
                             LigmaImage *new_image)
{
  GList *list;

  for (list = ligma_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      LigmaGuide *guide    = list->data;
      gint       position = ligma_guide_get_position (guide);

      switch (ligma_guide_get_orientation (guide))
        {
        case LIGMA_ORIENTATION_HORIZONTAL:
          ligma_image_add_hguide (new_image, position, FALSE);
          break;

        case LIGMA_ORIENTATION_VERTICAL:
          ligma_image_add_vguide (new_image, position, FALSE);
          break;

        default:
          g_error ("Unknown guide orientation.\n");
        }
    }
}

static void
ligma_image_duplicate_sample_points (LigmaImage *image,
                                    LigmaImage *new_image)
{
  GList *list;

  for (list = ligma_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      LigmaSamplePoint *sample_point = list->data;
      gint             x;
      gint             y;

      ligma_sample_point_get_position (sample_point, &x, &y);

      ligma_image_add_sample_point_at_pos (new_image, x, y, FALSE);
    }
}

static void
ligma_image_duplicate_grid (LigmaImage *image,
                           LigmaImage *new_image)
{
  if (ligma_image_get_grid (image))
    ligma_image_set_grid (new_image, ligma_image_get_grid (image), FALSE);
}

static void
ligma_image_duplicate_metadata (LigmaImage *image,
                               LigmaImage *new_image)
{
  LigmaMetadata *metadata = ligma_image_get_metadata (image);

  if (metadata)
    {
      metadata = ligma_metadata_duplicate (metadata);
      ligma_image_set_metadata (new_image, metadata, FALSE);
      g_object_unref (metadata);
    }
}

static void
ligma_image_duplicate_quick_mask (LigmaImage *image,
                                 LigmaImage *new_image)
{
  LigmaImagePrivate *private     = LIGMA_IMAGE_GET_PRIVATE (image);
  LigmaImagePrivate *new_private = LIGMA_IMAGE_GET_PRIVATE (new_image);

  new_private->quick_mask_state    = private->quick_mask_state;
  new_private->quick_mask_inverted = private->quick_mask_inverted;
  new_private->quick_mask_color    = private->quick_mask_color;
}

static void
ligma_image_duplicate_parasites (LigmaImage *image,
                                LigmaImage *new_image)
{
  LigmaImagePrivate *private     = LIGMA_IMAGE_GET_PRIVATE (image);
  LigmaImagePrivate *new_private = LIGMA_IMAGE_GET_PRIVATE (new_image);

  if (private->parasites)
    {
      g_object_unref (new_private->parasites);
      new_private->parasites = ligma_parasite_list_copy (private->parasites);
    }
}

static void
ligma_image_duplicate_color_profile (LigmaImage *image,
                                    LigmaImage *new_image)
{
  LigmaColorProfile *profile = ligma_image_get_color_profile (image);
  LigmaColorProfile *hidden  = _ligma_image_get_hidden_profile (image);

  ligma_image_set_color_profile (new_image, profile, NULL);
  _ligma_image_set_hidden_profile (new_image, hidden, FALSE);
}
