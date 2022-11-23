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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-edit.h"
#include "ligmabuffer.h"
#include "ligmacontext.h"
#include "ligmadrawable-edit.h"
#include "ligmagrouplayer.h"
#include "ligmaimage.h"
#include "ligmaimage-duplicate.h"
#include "ligmaimage-merge.h"
#include "ligmaimage-new.h"
#include "ligmaimage-undo.h"
#include "ligmalayer-floating-selection.h"
#include "ligmalayer-new.h"
#include "ligmalayermask.h"
#include "ligmalist.h"
#include "ligmapickable.h"
#include "ligmaselection.h"

#include "ligma-intl.h"


/*  local function protypes  */

static LigmaBuffer   * ligma_edit_extract            (LigmaImage     *image,
                                                    GList         *pickables,
                                                    LigmaContext   *context,
                                                    gboolean       cut_pixels,
                                                    GError       **error);
static LigmaDrawable * ligma_edit_paste_get_top_item (GList         *drawables);


/*  public functions  */

LigmaObject *
ligma_edit_cut (LigmaImage     *image,
               GList         *drawables,
               LigmaContext   *context,
               GError       **error)
{
  GList    *iter;
  gboolean  cut_layers = FALSE;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (ligma_channel_is_empty (ligma_image_get_mask (image)))
    {
      cut_layers = TRUE;

      for (iter = drawables; iter; iter = iter->next)
        if (! LIGMA_IS_LAYER (iter->data))
          {
            cut_layers = FALSE;
            break;
          }
    }

  if (cut_layers)
    {
      GList     *remove = NULL;
      LigmaImage *clip_image;
      gchar     *undo_label;

      /* Let's work on a copy because we will edit the list to remove
       * layers whose ancestor is also cut.
       */
      drawables = g_list_copy (drawables);
      for (iter = drawables; iter; iter = iter->next)
        {
          GList *iter2;

          for (iter2 = drawables; iter2; iter2 = iter2->next)
            {
              if (iter2 == iter)
                continue;

              if (ligma_viewable_is_ancestor (iter2->data, iter->data))
                {
                  /* When cutting a layer group, all its children come
                   * with anyway.
                   */
                  remove = g_list_prepend (remove, iter);
                  break;
                }
            }
        }
      for (iter = remove; iter; iter = iter->next)
        drawables = g_list_delete_link (drawables, iter->data);

      g_list_free (remove);

      /* Now copy all layers into the clipboard image. */
      clip_image = ligma_image_new_from_drawables (image->ligma, drawables, FALSE, TRUE);
      ligma_container_remove (image->ligma->images, LIGMA_OBJECT (clip_image));
      ligma_set_clipboard_image (image->ligma, clip_image);
      g_object_unref (clip_image);

      undo_label = g_strdup_printf (ngettext ("Cut Layer", "Cut %d Layers",
                                              g_list_length (drawables)),
                                    g_list_length (drawables));
      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_EDIT_CUT,
                                   undo_label);
      g_free (undo_label);

      /* Remove layers from source image. */
      for (iter = drawables; iter; iter = iter->next)
        ligma_image_remove_layer (image, LIGMA_LAYER (iter->data),
                                 TRUE, NULL);

      ligma_image_undo_group_end (image);
      g_list_free (drawables);

      return LIGMA_OBJECT (ligma_get_clipboard_image (image->ligma));
    }
  else
    {
      LigmaBuffer *buffer;

      buffer = ligma_edit_extract (image, drawables, context, TRUE, error);

      if (buffer)
        {
          ligma_set_clipboard_buffer (image->ligma, buffer);
          g_object_unref (buffer);

          return LIGMA_OBJECT (ligma_get_clipboard_buffer (image->ligma));
        }
    }

  return NULL;
}

LigmaObject *
ligma_edit_copy (LigmaImage     *image,
                GList         *drawables,
                LigmaContext   *context,
                GError       **error)
{
  GList    *iter;
  gboolean  drawables_are_layers = TRUE;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (drawables != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (LIGMA_IS_DRAWABLE (iter->data), NULL);
      g_return_val_if_fail (ligma_item_is_attached (iter->data), NULL);

      if (! LIGMA_IS_LAYER (iter->data))
        drawables_are_layers = FALSE;
    }

  /* Only accept multiple drawables for layers. */
  g_return_val_if_fail (g_list_length (drawables) == 1 || drawables_are_layers, NULL);

  if (drawables_are_layers)
    {
      /* Special-casing the 1 layer with no selection case.
       * It allows us to save the whole layer with all pixels as stored,
       * not the rendered version of it.
       */
      LigmaImage   *clip_image;
      LigmaChannel *clip_selection;

      clip_image = ligma_image_new_from_drawables (image->ligma, drawables, TRUE, TRUE);
      ligma_container_remove (image->ligma->images, LIGMA_OBJECT (clip_image));
      ligma_set_clipboard_image (image->ligma, clip_image);
      g_object_unref (clip_image);

      clip_selection = ligma_image_get_mask (clip_image);
      if (! ligma_channel_is_empty (clip_selection))
        {
          GList         *all_items;
          GeglRectangle  selection_bounds;

          ligma_item_bounds (LIGMA_ITEM (clip_selection),
                            &selection_bounds.x, &selection_bounds.y,
                            &selection_bounds.width, &selection_bounds.height);

          /* Invert the selection. */
          ligma_channel_invert (clip_selection, FALSE);
          all_items = ligma_image_get_layer_list (clip_image);

          for (iter = all_items; iter; iter = g_list_next (iter))
            {
              gint item_x;
              gint item_y;

              ligma_item_get_offset (LIGMA_ITEM (iter->data), &item_x, &item_y);

              /* Even if the original layer may not have an alpha channel, the
               * selected data must always have one. First because a selection
               * is in some way an alpha channel when we copy (we may copy part
               * of a pixel, i.e. with transparency). Second because the
               * selection is not necessary rectangular, unlike layers. So when
               * we will clear, if we hadn't added an alpha channel, we'd end up
               * with background color all over the place.
               */
              ligma_layer_add_alpha (LIGMA_LAYER (iter->data));
              ligma_drawable_edit_clear (LIGMA_DRAWABLE (iter->data), context);

              /* Finally shrink the copied layer to selection bounds. */
              ligma_item_resize (iter->data, context, LIGMA_FILL_TRANSPARENT,
                                selection_bounds.width, selection_bounds.height,
                                item_x - selection_bounds.x, item_y - selection_bounds.y);
            }
          g_list_free (all_items);
        }
      /* Remove selection from the clipboard image. */
      ligma_channel_clear (clip_selection, NULL, FALSE);

      return LIGMA_OBJECT (ligma_get_clipboard_image (image->ligma));
    }
  else
    {
      LigmaBuffer *buffer;

      buffer = ligma_edit_extract (image, drawables, context, FALSE, error);

      if (buffer)
        {
          ligma_set_clipboard_buffer (image->ligma, buffer);
          g_object_unref (buffer);

          return LIGMA_OBJECT (ligma_get_clipboard_buffer (image->ligma));
        }
    }

  return NULL;
}

LigmaBuffer *
ligma_edit_copy_visible (LigmaImage    *image,
                        LigmaContext  *context,
                        GError      **error)
{
  LigmaBuffer *buffer;
  GList      *pickables;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  pickables = g_list_prepend (NULL, image);
  buffer = ligma_edit_extract (image, pickables, context, FALSE, error);
  g_list_free (pickables);

  if (buffer)
    {
      ligma_set_clipboard_buffer (image->ligma, buffer);
      g_object_unref (buffer);

      return ligma_get_clipboard_buffer (image->ligma);
    }

  return NULL;
}

static gboolean
ligma_edit_paste_is_in_place (LigmaPasteType paste_type)
{
  switch (paste_type)
    {
    case LIGMA_PASTE_TYPE_FLOATING:
    case LIGMA_PASTE_TYPE_FLOATING_INTO:
    case LIGMA_PASTE_TYPE_NEW_LAYER:
    case LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
    case LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
      return FALSE;

    case LIGMA_PASTE_TYPE_FLOATING_IN_PLACE:
    case LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
    case LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE:
    case LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
    case LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
      return TRUE;
    }

  g_return_val_if_reached (FALSE);
}

static gboolean
ligma_edit_paste_is_floating (LigmaPasteType  paste_type,
                             LigmaDrawable  *drawable)
{
  switch (paste_type)
    {
    case LIGMA_PASTE_TYPE_FLOATING:
    case LIGMA_PASTE_TYPE_FLOATING_INTO:
    case LIGMA_PASTE_TYPE_FLOATING_IN_PLACE:
    case LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
      return TRUE;

    case LIGMA_PASTE_TYPE_NEW_LAYER:
    case LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE:
      return FALSE;

    case LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
    case LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
    case LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
    case LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
      if (LIGMA_IS_LAYER_MASK (drawable))
        return TRUE;
      else
        return FALSE;
    }

  g_return_val_if_reached (FALSE);
}

static GList *
ligma_edit_paste_get_tagged_layers (LigmaImage         *image,
                                   GList             *layers,
                                   GList             *returned_layers,
                                   const Babl        *floating_format,
                                   LigmaImageBaseType  base_type,
                                   LigmaPrecision      precision,
                                   LigmaPasteType      paste_type)
{
  GList *iter;

  for (iter = layers; iter; iter = iter->next)
    {
      LigmaLayer *layer;
      GType      layer_type;
      gboolean   copied = TRUE;

      switch (paste_type)
        {
        case LIGMA_PASTE_TYPE_FLOATING:
        case LIGMA_PASTE_TYPE_FLOATING_IN_PLACE:
        case LIGMA_PASTE_TYPE_FLOATING_INTO:
        case LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
          /*  when pasting as floating make sure ligma_item_convert()
           *  will turn group layers into normal layers, otherwise use
           *  the same layer type so e.g. text information gets
           *  preserved. See issue #2667.
           */
          if (LIGMA_IS_GROUP_LAYER (iter->data))
            layer_type = LIGMA_TYPE_LAYER;
          else
            layer_type = G_TYPE_FROM_INSTANCE (iter->data);
          break;

        case LIGMA_PASTE_TYPE_NEW_LAYER:
        case LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE:
          layer_type = G_TYPE_FROM_INSTANCE (iter->data);
          break;

        default:
          g_return_val_if_reached (NULL);
        }

      if (LIGMA_IS_GROUP_LAYER (iter->data))
        copied = (gboolean) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (iter->data),
                                                                "ligma-image-copied-layer"));
      if (copied)
        {
          layer = LIGMA_LAYER (ligma_item_convert (LIGMA_ITEM (iter->data),
                                                 image, layer_type));
          returned_layers = g_list_prepend (returned_layers, layer);

          switch (paste_type)
            {
            case LIGMA_PASTE_TYPE_FLOATING:
            case LIGMA_PASTE_TYPE_FLOATING_IN_PLACE:
            case LIGMA_PASTE_TYPE_FLOATING_INTO:
            case LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
              /*  when pasting as floating selection, get rid of the layer mask,
               *  and make sure the layer has the right format
               */
              if (ligma_layer_get_mask (iter->data))
                ligma_layer_apply_mask (iter->data, LIGMA_MASK_DISCARD, FALSE);

              if (ligma_drawable_get_format (LIGMA_DRAWABLE (iter->data)) !=
                  floating_format)
                {
                  ligma_drawable_convert_type (LIGMA_DRAWABLE (iter->data), image,
                                              base_type, precision,
                                              TRUE, NULL, NULL,
                                              GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                              FALSE, NULL);
                }
              break;

            default:
              break;
            }
        }
      else
        {
          LigmaContainer *container;

          container = ligma_viewable_get_children (iter->data);
          returned_layers = ligma_edit_paste_get_tagged_layers (image,
                                                               LIGMA_LIST (container)->queue->head,
                                                               returned_layers,
                                                               floating_format,
                                                               base_type, precision, paste_type);
        }
    }

  return returned_layers;
}

static GList *
ligma_edit_paste_get_layers (LigmaImage     *image,
                            GList         *drawables,
                            LigmaObject    *paste,
                            LigmaPasteType *paste_type)
{
  GList      *layers = NULL;
  const Babl *floating_format;

  /*  change paste type to NEW_LAYER for cases where we can't attach a
   *  floating selection
   */
  if (g_list_length (drawables) != 1               ||
      ligma_viewable_get_children (drawables->data) ||
      ligma_item_is_content_locked (drawables->data, NULL))
    {
      if (ligma_edit_paste_is_in_place (*paste_type))
        *paste_type = LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE;
      else
        *paste_type = LIGMA_PASTE_TYPE_NEW_LAYER;
    }

  /*  floating pastes always have the pasted-to drawable's format with
   *  alpha; if drawable == NULL, user is pasting into an empty image
   */
  if (drawables && ligma_edit_paste_is_floating (*paste_type, drawables->data))
    floating_format = ligma_drawable_get_format_with_alpha (drawables->data);
  else
    floating_format = ligma_image_get_layer_format (image, TRUE);

  if (LIGMA_IS_IMAGE (paste))
    {
      layers = ligma_image_get_layer_iter (LIGMA_IMAGE (paste));

      if (g_list_length (layers) > 1)
        {
          if (ligma_edit_paste_is_in_place (*paste_type))
            *paste_type = LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE;
          else
            *paste_type = LIGMA_PASTE_TYPE_NEW_LAYER;
        }

      layers = ligma_edit_paste_get_tagged_layers (image, layers, NULL, floating_format,
                                                  ligma_drawable_get_base_type (drawables->data),
                                                  ligma_drawable_get_precision (drawables->data),
                                                  *paste_type);
      layers = g_list_reverse (layers);
    }
  else if (LIGMA_IS_BUFFER (paste))
    {
      LigmaLayer *layer;

      layer = ligma_layer_new_from_buffer (LIGMA_BUFFER (paste), image,
                                          floating_format,
                                          _("Pasted Layer"),
                                          LIGMA_OPACITY_OPAQUE,
                                          ligma_image_get_default_new_layer_mode (image));

      layers = g_list_prepend (layers, layer);
    }

  return layers;
}

static void
ligma_edit_paste_get_viewport_offset (LigmaImage *image,
                                     GList     *drawables,
                                     GList     *pasted_layers,
                                     gint       viewport_x,
                                     gint       viewport_y,
                                     gint       viewport_width,
                                     gint       viewport_height,
                                     gint      *pasted_bbox_x,
                                     gint      *pasted_bbox_y,
                                     gint      *offset_x,
                                     gint      *offset_y)
{
  GList   *iter;
  gint     image_width;
  gint     image_height;
  /* Source: pasted layers */
  gint     source_width;
  gint     source_height;

  gint     x1             = G_MAXINT;
  gint     y1             = G_MAXINT;
  gint     x2             = G_MININT;
  gint     y2             = G_MININT;
  gboolean clamp_to_image = TRUE;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (pasted_layers != NULL);
  g_return_if_fail (offset_x != NULL);
  g_return_if_fail (offset_y != NULL);

  image_width  = ligma_image_get_width  (image);
  image_height = ligma_image_get_height (image);

  for (iter = pasted_layers; iter; iter = iter->next)
    {
      gint layer_off_x;
      gint layer_off_y;
      gint layer_width;
      gint layer_height;

      g_return_if_fail (LIGMA_IS_VIEWABLE (iter->data));

      ligma_item_get_offset (LIGMA_ITEM (iter->data), &layer_off_x, &layer_off_y);
      ligma_viewable_get_size (iter->data, &layer_width, &layer_height);

      x1 = MIN (layer_off_x, x1);
      y1 = MIN (layer_off_y, y1);
      x2 = MAX (layer_off_x + layer_width, x2);
      y2 = MAX (layer_off_y + layer_height, y2);
    }

  /* Source offset and dimensions: this is the bounding box for all layers which
   * we want to paste together, keeping their relative position to each others.
   */
  *pasted_bbox_x = x1;
  *pasted_bbox_y = y1;
  source_width = x2 - x1;
  source_height = y2 - y1;

  if (viewport_width  == image_width &&
      viewport_height == image_height)
    {
      /* if the whole image is visible, act as if there was no viewport */

      viewport_x      = 0;
      viewport_y      = 0;
      viewport_width  = 0;
      viewport_height = 0;
    }

  if (drawables)
    {
      /*  if pasting while 1 or more drawables are selected  */
      gboolean empty_target = TRUE;
      gint     target_x;
      gint     target_y;
      gint     target_width;
      gint     target_height;
      gint     paste_x, paste_y;
      gint     paste_width;
      gint     paste_height;
      gboolean have_mask;

      have_mask = ! ligma_channel_is_empty (ligma_image_get_mask (image));

      for (iter = drawables; iter; iter = iter->next)
        {
          LigmaContainer *children;

          children = ligma_viewable_get_children (iter->data);
          if (! children || ligma_container_get_n_children (children) > 0)
            {
              empty_target = FALSE;
              break;
            }
        }

      if (empty_target)
        {
          /* treat empty layer groups as image-sized, use the selection
           * as target
           */
          ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                            &target_x, &target_y,
                            &target_width, &target_height);
        }
      else
        {
          gint x1 = G_MAXINT;
          gint y1 = G_MAXINT;
          gint x2 = G_MININT;
          gint y2 = G_MININT;

          for (iter = drawables; iter; iter = iter->next)
            {
              gint layer_off_x;
              gint layer_off_y;
              gint mask_off_x;
              gint mask_off_y;
              gint mask_width;
              gint mask_height;

              ligma_item_get_offset (LIGMA_ITEM (iter->data), &layer_off_x, &layer_off_y);
              /* This is the bounds relatively to the drawable. */
              ligma_item_mask_intersect (LIGMA_ITEM (iter->data),
                                        &mask_off_x, &mask_off_y,
                                        &mask_width, &mask_height);

              /* The bounds relatively to the image. */
              x1 = MIN (layer_off_x + mask_off_x, x1);
              y1 = MIN (layer_off_y + mask_off_y, y1);
              x2 = MAX (layer_off_x + mask_off_x + mask_width, x2);
              y2 = MAX (layer_off_y + mask_off_y + mask_height, y2);
            }

          target_x      = x1;
          target_y      = y1;
          target_width  = x2 - x1;
          target_height = y2 - y1;
        }

      if (! have_mask         &&    /* if we have no mask */
          viewport_width  > 0 &&    /* and we have a viewport */
          viewport_height > 0 &&
          (source_width  < target_width || /* and the paste is smaller than the target */
           source_height < target_height) &&

          /* and the viewport intersects with the target */
          ligma_rectangle_intersect (viewport_x, viewport_y,
                                    viewport_width, viewport_height,
                                    target_x, target_x,
                                    target_width, target_height,
                                    &paste_x, &paste_y,
                                    &paste_width, &paste_height))
        {
          /*  center on the viewport  */
          *offset_x = viewport_x + (viewport_width - source_width)  / 2;
          *offset_y = viewport_y + (viewport_height- source_height) / 2;
        }
      else
        {
          /*  otherwise center on the target  */
          *offset_x = target_x + (target_width  - source_width)  / 2;
          *offset_y = target_y + (target_height - source_height) / 2;

          /*  and keep it that way  */
          clamp_to_image = FALSE;
        }
    }
  else if (viewport_width  > 0 &&  /* if we have a viewport */
           viewport_height > 0 &&
           (source_width  < image_width || /* and the paste is       */
            source_height < image_height)) /* smaller than the image */
    {
      /*  center on the viewport  */

      *offset_x = viewport_x + (viewport_width  - source_width)  / 2;
      *offset_y = viewport_y + (viewport_height - source_height) / 2;
    }
  else
    {
      /*  otherwise center on the image  */

      *offset_x = (image_width  - source_width)  / 2;
      *offset_y = (image_height - source_height) / 2;

      /*  and keep it that way  */
      clamp_to_image = FALSE;
    }

  if (clamp_to_image)
    {
      /*  Ensure that the pasted layer is always within the image, if it
       *  fits and aligned at top left if it doesn't. (See bug #142944).
       */
      *offset_x = MIN (*offset_x, image_width  - source_width);
      *offset_y = MIN (*offset_y, image_height - source_height);
      *offset_x = MAX (*offset_x, 0);
      *offset_y = MAX (*offset_y, 0);
    }
}

static GList *
ligma_edit_paste_paste (LigmaImage     *image,
                       GList         *drawables,
                       GList         *layers,
                       LigmaPasteType  paste_type,
                       gboolean       use_offset,
                       gint           layers_bbox_x,
                       gint           layers_bbox_y,
                       gint           offset_x,
                       gint           offset_y)
{
  GList *iter;

  g_return_val_if_fail (paste_type > LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE ||
                        g_list_length (drawables) == 1, NULL);

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_EDIT_PASTE,
                               C_("undo-type", "Paste"));

  for (iter = layers; iter; iter = iter->next)
    {
      /* Layers for in-place paste are already translated. */
      if (use_offset)
        ligma_item_translate (LIGMA_ITEM (iter->data),
                             offset_x - layers_bbox_x,
                             offset_y - layers_bbox_y, FALSE);

      switch (paste_type)
        {
        case LIGMA_PASTE_TYPE_FLOATING:
        case LIGMA_PASTE_TYPE_FLOATING_IN_PLACE:
          /*  if there is a selection mask clear it - this might not
           *  always be desired, but in general, it seems like the correct
           *  behavior
           */
          if (! ligma_channel_is_empty (ligma_image_get_mask (image)))
            ligma_channel_clear (ligma_image_get_mask (image), NULL, TRUE);

          /* fall thru */

        case LIGMA_PASTE_TYPE_FLOATING_INTO:
        case LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
          floating_sel_attach (iter->data, drawables->data);
          break;

        case LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
        case LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
          if (g_list_length (drawables) == 1 && LIGMA_IS_LAYER_MASK (drawables->data))
            {
              if (! ligma_channel_is_empty (ligma_image_get_mask (image)))
                ligma_channel_clear (ligma_image_get_mask (image), NULL, TRUE);
              floating_sel_attach (iter->data, drawables->data);
              break;
            }

          /* fall thru if not a layer mask */

        case LIGMA_PASTE_TYPE_NEW_LAYER:
        case LIGMA_PASTE_TYPE_NEW_LAYER_IN_PLACE:
            {
              LigmaLayer *parent   = NULL;
              gint       position = 0;

              /*  always add on top of a passed layer, where we would attach
               *  a floating selection
               */
              if (g_list_length (drawables) > 0 && LIGMA_IS_LAYER (drawables->data))
                {
                  LigmaDrawable *top_drawable;

                  top_drawable = ligma_edit_paste_get_top_item (drawables);
                  parent   = ligma_layer_get_parent (LIGMA_LAYER (top_drawable));
                  position = ligma_item_get_index (LIGMA_ITEM (top_drawable));
                }

              ligma_image_add_layer (image, iter->data, parent, position, TRUE);
            }
          break;

        case LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
        case LIGMA_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
          g_return_val_if_reached (NULL);
        }
    }

  ligma_image_undo_group_end (image);

  return layers;
}

GList *
ligma_edit_paste (LigmaImage     *image,
                 GList         *drawables,
                 LigmaObject    *paste,
                 LigmaPasteType  paste_type,
                 LigmaContext   *context,
                 gboolean       merged,
                 gint           viewport_x,
                 gint           viewport_y,
                 gint           viewport_width,
                 gint           viewport_height)
{
  GList    *layers;
  gboolean  use_offset    = FALSE;
  gint      layers_bbox_x = 0;
  gint      layers_bbox_y = 0;
  gint      offset_y      = 0;
  gint      offset_x      = 0;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (paste) || LIGMA_IS_BUFFER (paste), NULL);

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (LIGMA_IS_DRAWABLE (iter->data), NULL);
      g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (iter->data)), NULL);
    }

  if (merged && LIGMA_IS_IMAGE (paste))
    {
      LigmaImage *tmp_image;

      tmp_image = ligma_image_duplicate (LIGMA_IMAGE (paste));
      ligma_container_remove (image->ligma->images, LIGMA_OBJECT (tmp_image));
      ligma_image_merge_visible_layers (tmp_image, context, LIGMA_EXPAND_AS_NECESSARY,
                                       FALSE, FALSE, NULL);
      layers = g_list_copy (ligma_image_get_layer_iter (tmp_image));

      /* The merge process should ensure that we get a single non-group and
       * no-mask layer.
       */
      g_return_val_if_fail (g_list_length (layers) == 1, NULL);

      layers->data = ligma_item_convert (LIGMA_ITEM (layers->data), image,
                                        G_TYPE_FROM_INSTANCE (layers->data));

      switch (paste_type)
        {
        case LIGMA_PASTE_TYPE_FLOATING:
        case LIGMA_PASTE_TYPE_FLOATING_IN_PLACE:
        case LIGMA_PASTE_TYPE_FLOATING_INTO:
        case LIGMA_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
          if (ligma_drawable_get_format (LIGMA_DRAWABLE (layers->data)) !=
              ligma_drawable_get_format_with_alpha (LIGMA_DRAWABLE (layers->data)))
            {
              ligma_drawable_convert_type (LIGMA_DRAWABLE (layers->data), image,
                                          ligma_drawable_get_base_type (layers->data),
                                          ligma_drawable_get_precision (layers->data),
                                          TRUE, NULL, NULL,
                                          GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                          FALSE, NULL);
            }
          break;

        default:
          break;
        }
      g_object_unref (tmp_image);
    }
  else
    {
      layers = ligma_edit_paste_get_layers (image, drawables, paste, &paste_type);
    }

  if (! layers)
    return NULL;

  if (ligma_edit_paste_is_in_place (paste_type))
    {
      if (LIGMA_IS_BUFFER (paste))
        {
          LigmaBuffer *buffer = LIGMA_BUFFER (paste);

          use_offset = TRUE;
          offset_x   = buffer->offset_x;
          offset_y   = buffer->offset_y;
        }
    }
  else
    {
      use_offset = TRUE;
      ligma_edit_paste_get_viewport_offset (image, drawables, layers,
                                           viewport_x,
                                           viewport_y,
                                           viewport_width,
                                           viewport_height,
                                           &layers_bbox_x,
                                           &layers_bbox_y,
                                           &offset_x,
                                           &offset_y);
    }

  return ligma_edit_paste_paste (image, drawables, layers, paste_type,
                                use_offset, layers_bbox_x, layers_bbox_y,
                                offset_x, offset_y);
}

LigmaImage *
ligma_edit_paste_as_new_image (Ligma       *ligma,
                              LigmaObject *paste)
{
  LigmaImage *image = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_IMAGE (paste) || LIGMA_IS_BUFFER (paste), NULL);

  if (LIGMA_IS_IMAGE (paste))
    {
      image = ligma_image_duplicate (LIGMA_IMAGE (paste));
    }
  else if (LIGMA_IS_BUFFER (paste))
    {
      image = ligma_image_new_from_buffer (ligma, LIGMA_BUFFER (paste));
    }

  return image;
}

const gchar *
ligma_edit_named_cut (LigmaImage     *image,
                     const gchar   *name,
                     GList         *drawables,
                     LigmaContext   *context,
                     GError       **error)
{
  LigmaBuffer *buffer;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  buffer = ligma_edit_extract (image, drawables, context, TRUE, error);

  if (buffer)
    {
      ligma_object_set_name (LIGMA_OBJECT (buffer), name);
      ligma_container_add (image->ligma->named_buffers, LIGMA_OBJECT (buffer));
      g_object_unref (buffer);

      return ligma_object_get_name (buffer);
    }

  return NULL;
}

const gchar *
ligma_edit_named_copy (LigmaImage     *image,
                      const gchar   *name,
                      GList         *drawables,
                      LigmaContext   *context,
                      GError       **error)
{
  LigmaBuffer *buffer;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  buffer = ligma_edit_extract (image, drawables, context, FALSE, error);

  if (buffer)
    {
      ligma_object_set_name (LIGMA_OBJECT (buffer), name);
      ligma_container_add (image->ligma->named_buffers, LIGMA_OBJECT (buffer));
      g_object_unref (buffer);

      return ligma_object_get_name (buffer);
    }

  return NULL;
}

const gchar *
ligma_edit_named_copy_visible (LigmaImage    *image,
                              const gchar  *name,
                              LigmaContext  *context,
                              GError      **error)
{
  LigmaBuffer *buffer;
  GList      *pickables;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  pickables = g_list_prepend (NULL, image);
  buffer = ligma_edit_extract (image, pickables, context, FALSE, error);
  g_list_free (pickables);

  if (buffer)
    {
      ligma_object_set_name (LIGMA_OBJECT (buffer), name);
      ligma_container_add (image->ligma->named_buffers, LIGMA_OBJECT (buffer));
      g_object_unref (buffer);

      return ligma_object_get_name (buffer);
    }

  return NULL;
}


/*  private functions  */

/**
 * ligma_edit_extract:
 * @image:
 * @pickables:
 * @context:
 * @cut_pixels:
 * @error:
 *
 * Extracts the selected part of @image from the list of @pickables.
 * If @cut_pixels is %TRUE, and there is only one pickable input, and if
 * this pickable is a #LigmaDrawable, then the selected pixels will be
 * effectively erased from the input pickable.
 * Otherwise @cut_pixels has no additional effect.
 * Note that all @pickables must belong to the same @image.
 *
 * Returns: a #LigmaBuffer of the selected part of @image as if only the
 * selected @pickables were present (composited according to their
 * properties, unless there is only one pickable, in which case direct
 * pixel information is used without composition).
 */
static LigmaBuffer *
ligma_edit_extract (LigmaImage     *image,
                   GList         *pickables,
                   LigmaContext   *context,
                   gboolean       cut_pixels,
                   GError       **error)
{
  GeglBuffer *buffer;
  gint        offset_x;
  gint        offset_y;

  g_return_val_if_fail (g_list_length (pickables) > 0, NULL);

  if (g_list_length (pickables) > 1 ||
      ! LIGMA_IS_DRAWABLE (pickables->data))
    cut_pixels = FALSE;

  if (cut_pixels)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_EDIT_CUT,
                                 C_("undo-type", "Cut"));

  /*  Cut/copy the mask portion from the image  */
  buffer = ligma_selection_extract (LIGMA_SELECTION (ligma_image_get_mask (image)),
                                   pickables, context,
                                   cut_pixels, FALSE, FALSE,
                                   &offset_x, &offset_y, error);

  if (cut_pixels)
    ligma_image_undo_group_end (image);

  if (buffer)
    {
      LigmaBuffer *ligma_buffer;
      gdouble     res_x;
      gdouble     res_y;

      ligma_buffer = ligma_buffer_new (buffer, _("Global Buffer"),
                                     offset_x, offset_y, FALSE);
      g_object_unref (buffer);

      ligma_image_get_resolution (image, &res_x, &res_y);
      ligma_buffer_set_resolution (ligma_buffer, res_x, res_y);
      ligma_buffer_set_unit (ligma_buffer, ligma_image_get_unit (image));

      if (LIGMA_IS_COLOR_MANAGED (pickables->data))
        {
          LigmaColorProfile *profile =
            ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (pickables->data));

          if (profile)
            ligma_buffer_set_color_profile (ligma_buffer, profile);
        }

      return ligma_buffer;
    }

  return NULL;
}

/* Return the visually top item. */
static LigmaDrawable *
ligma_edit_paste_get_top_item (GList *drawables)
{
  GList        *iter;
  LigmaDrawable *top      = NULL;
  GList        *top_path = NULL;

  for (iter = drawables; iter; iter = iter->next)
    {
      GList *path = ligma_item_get_path (iter->data);

      if (top == NULL)
        {
          top      = iter->data;
          top_path = path;
          path     = NULL;
        }
      else
        {
          GList *p_iter;
          GList *tp_iter;

          for (p_iter = path, tp_iter = top_path; p_iter || tp_iter; p_iter = p_iter->next, tp_iter = tp_iter->next)
            {
              if (tp_iter == NULL)
                {
                  break;
                }
              else if (p_iter == NULL ||
                       GPOINTER_TO_UINT (p_iter->data) < GPOINTER_TO_UINT (tp_iter->data))
                {
                  g_list_free (top_path);
                  top_path = path;
                  path = NULL;
                  top = iter->data;
                  break;
                }
              else if (GPOINTER_TO_UINT (p_iter->data) > GPOINTER_TO_UINT (tp_iter->data))
                {
                  break;
                }
            }
        }
      g_list_free (path);
    }

  g_list_free (top_path);
  return top;
}
