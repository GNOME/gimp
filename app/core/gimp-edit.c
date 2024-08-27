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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-edit.h"
#include "gimpbuffer.h"
#include "gimpcontext.h"
#include "gimpdrawable-edit.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawablefilter.h"
#include "gimperror.h"
#include "gimpgrouplayer.h"
#include "gimpimage.h"
#include "gimpimage-duplicate.h"
#include "gimpimage-merge.h"
#include "gimpimage-new.h"
#include "gimpimage-resize.h"
#include "gimpimage-undo.h"
#include "gimplayer-floating-selection.h"
#include "gimplayer-new.h"
#include "gimplayermask.h"
#include "gimplist.h"
#include "gimppickable.h"
#include "gimpselection.h"

#include "gimp-intl.h"


/*  local function protypes  */

static GimpBuffer   * gimp_edit_extract            (GimpImage     *image,
                                                    GList         *pickables,
                                                    GimpContext   *context,
                                                    gboolean       cut_pixels,
                                                    GError       **error);
static GimpDrawable * gimp_edit_paste_get_top_item (GList         *drawables);


/*  public functions  */

GimpObject *
gimp_edit_cut (GimpImage     *image,
               GList         *drawables,
               GimpContext   *context,
               GError       **error)
{
  GList    *iter;
  gboolean  layers_only = TRUE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    if (! GIMP_IS_LAYER (iter->data))
      {
        layers_only = FALSE;
        break;
      }

  if (layers_only)
    {
      gchar    *undo_label;
      gboolean  success = TRUE;

      undo_label = g_strdup_printf (ngettext ("Cut Layer", "Cut %d Layers",
                                              g_list_length (drawables)),
                                    g_list_length (drawables));
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_CUT,
                                   undo_label);
      g_free (undo_label);

      if (gimp_channel_is_empty (gimp_image_get_mask (image)))
        {
          GList     *remove = NULL;
          GimpImage *clip_image;

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

                  if (gimp_viewable_is_ancestor (iter2->data, iter->data))
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
          clip_image = gimp_image_new_from_drawables (image->gimp, drawables, FALSE, TRUE);
          gimp_container_remove (image->gimp->images, GIMP_OBJECT (clip_image));
          gimp_set_clipboard_image (image->gimp, clip_image);
          g_object_unref (clip_image);

          /* Remove layers from source image. */
          for (iter = drawables; iter; iter = iter->next)
            gimp_image_remove_layer (image, GIMP_LAYER (iter->data),
                                     TRUE, NULL);

          g_list_free (drawables);
        }
      else
        {
          /* With selection, a cut is similar to a copy followed by a clear. */
          if (gimp_edit_copy (image, drawables, context, TRUE, error))
            {
              for (iter = drawables; iter; iter = iter->next)
                if (! GIMP_IS_GROUP_LAYER (iter->data))
                  gimp_drawable_edit_clear (GIMP_DRAWABLE (iter->data), context);
            }
          else
            {
              success = FALSE;
            }
        }

      gimp_image_undo_group_end (image);

      return success ? GIMP_OBJECT (gimp_get_clipboard_image (image->gimp)) : NULL;
    }
  else
    {
      GimpBuffer *buffer;

      buffer = gimp_edit_extract (image, drawables, context, TRUE, error);

      if (buffer)
        {
          gimp_set_clipboard_buffer (image->gimp, buffer);
          g_object_unref (buffer);

          return GIMP_OBJECT (gimp_get_clipboard_buffer (image->gimp));
        }
    }

  return NULL;
}

GimpObject *
gimp_edit_copy (GimpImage     *image,
                GList         *drawables,
                GimpContext   *context,
                gboolean       copy_for_cut,
                GError       **error)
{
  GList    *iter;
  gboolean  drawables_are_layers = TRUE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (drawables != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (GIMP_IS_DRAWABLE (iter->data), NULL);
      g_return_val_if_fail (gimp_item_is_attached (iter->data), NULL);

      if (! GIMP_IS_LAYER (iter->data))
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
      GimpImage     *clip_image;
      GimpChannel   *clip_selection;
      GList         *remove = NULL;
      GeglRectangle  selection_bounds;
      gboolean       has_selection = FALSE;

      for (iter = drawables; iter; iter = iter->next)
        {
          gint x1;
          gint y1;
          gint x2;
          gint y2;

          if (! gimp_item_mask_bounds (iter->data, &x1, &y1, &x2, &y2) ||
              (x1 != x2 && y1 != y2))
            break;
        }

      if (iter == NULL)
        {
          if (error)
            {
              if (copy_for_cut)
                g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                     _("Cannot cut because the selected region is empty."));
              else
                g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                     _("Cannot copy because the selected region is empty."));
            }
          return NULL;
        }

      drawables = g_list_copy (drawables);
      for (iter = drawables; iter; iter = iter->next)
        {
          GList *iter2;

          for (iter2 = drawables; iter2; iter2 = iter2->next)
            {
              if (iter2 == iter)
                continue;

              if (gimp_viewable_is_ancestor (iter2->data, iter->data))
                {
                  /* When copying a layer group, all its children come
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

      clip_image = gimp_image_new_from_drawables (image->gimp, drawables, TRUE, TRUE);
      gimp_container_remove (image->gimp->images, GIMP_OBJECT (clip_image));
      gimp_set_clipboard_image (image->gimp, clip_image);
      g_object_unref (clip_image);
      g_list_free (drawables);

      clip_selection = gimp_image_get_mask (clip_image);
      if (! gimp_channel_is_empty (clip_selection))
        {
          gimp_item_bounds (GIMP_ITEM (clip_selection),
                            &selection_bounds.x, &selection_bounds.y,
                            &selection_bounds.width, &selection_bounds.height);

          /* Invert the selection. */
          gimp_channel_invert (clip_selection, FALSE);

          /* Empty selection work the same as full selection for the
           * gimp_drawable_edit_*() APIs. So as a special case, we check again
           * after inverting the selection (if the inverted selection is empty,
           * it meant we were in "all" selection case).
           * Otherwise we were ending up clearing everything with
           * gimp_drawable_edit_clear() on the inverted "all" selection.
           */
          has_selection = (! gimp_channel_is_empty (clip_selection));
        }

      if (has_selection)
        {
          GList *all_items;

          all_items = gimp_image_get_layer_list (clip_image);

          for (iter = all_items; iter; iter = g_list_next (iter))
            {
              gint item_x;
              gint item_y;

              gimp_item_get_offset (GIMP_ITEM (iter->data), &item_x, &item_y);

              /* Even if the original layer may not have an alpha channel, the
               * selected data must always have one. First because a selection
               * is in some way an alpha channel when we copy (we may copy part
               * of a pixel, i.e. with transparency). Second because the
               * selection is not necessary rectangular, unlike layers. So when
               * we will clear, if we hadn't added an alpha channel, we'd end up
               * with background color all over the place.
               */
              gimp_layer_add_alpha (GIMP_LAYER (iter->data));
              gimp_drawable_edit_clear (GIMP_DRAWABLE (iter->data), context);

              /* Finally shrink the copied layer to selection bounds. */
              gimp_item_resize (iter->data, context, GIMP_FILL_TRANSPARENT,
                                selection_bounds.width, selection_bounds.height,
                                item_x - selection_bounds.x, item_y - selection_bounds.y);
            }
          g_list_free (all_items);

          /* We need to keep the image size as-is, because even after cropping
           * layers to selection, their offset stay important for in-place paste
           * variants. Yet we also need to store the dimensions where we'd have
           * cropped the image for the "Paste as New Image" action.
           */
          g_object_set_data (G_OBJECT (clip_image),
                             "gimp-edit-new-image-x",
                             GINT_TO_POINTER (selection_bounds.x));
          g_object_set_data (G_OBJECT (clip_image),
                             "gimp-edit-new-image-y",
                             GINT_TO_POINTER (selection_bounds.y));
          g_object_set_data (G_OBJECT (clip_image),
                             "gimp-edit-new-image-width",
                             GINT_TO_POINTER (selection_bounds.width));
          g_object_set_data (G_OBJECT (clip_image),
                             "gimp-edit-new-image-height",
                             GINT_TO_POINTER (selection_bounds.height));
        }
      /* Remove selection from the clipboard image. */
      gimp_channel_clear (clip_selection, NULL, FALSE);

      return GIMP_OBJECT (gimp_get_clipboard_image (image->gimp));
    }
  else
    {
      GimpBuffer *buffer;

      buffer = gimp_edit_extract (image, drawables, context, FALSE, error);

      if (buffer)
        {
          gimp_set_clipboard_buffer (image->gimp, buffer);
          g_object_unref (buffer);

          return GIMP_OBJECT (gimp_get_clipboard_buffer (image->gimp));
        }
    }

  return NULL;
}

GimpBuffer *
gimp_edit_copy_visible (GimpImage    *image,
                        GimpContext  *context,
                        GError      **error)
{
  GimpBuffer *buffer;
  GList      *pickables;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  pickables = g_list_prepend (NULL, image);
  buffer = gimp_edit_extract (image, pickables, context, FALSE, error);
  g_list_free (pickables);

  if (buffer)
    {
      gimp_set_clipboard_buffer (image->gimp, buffer);
      g_object_unref (buffer);

      return gimp_get_clipboard_buffer (image->gimp);
    }

  return NULL;
}

static gboolean
gimp_edit_paste_is_in_place (GimpPasteType paste_type)
{
  switch (paste_type)
    {
    case GIMP_PASTE_TYPE_FLOATING:
    case GIMP_PASTE_TYPE_FLOATING_INTO:
    case GIMP_PASTE_TYPE_NEW_LAYER:
    case GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
    case GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
      return FALSE;

    case GIMP_PASTE_TYPE_FLOATING_IN_PLACE:
    case GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
    case GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE:
    case GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
    case GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
      return TRUE;
    }

  g_return_val_if_reached (FALSE);
}

static gboolean
gimp_edit_paste_is_floating (GimpPasteType  paste_type,
                             GimpDrawable  *drawable)
{
  switch (paste_type)
    {
    case GIMP_PASTE_TYPE_FLOATING:
    case GIMP_PASTE_TYPE_FLOATING_INTO:
    case GIMP_PASTE_TYPE_FLOATING_IN_PLACE:
    case GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
      return TRUE;

    case GIMP_PASTE_TYPE_NEW_LAYER:
    case GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE:
      return FALSE;

    case GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
    case GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
    case GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
    case GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
      if (GIMP_IS_LAYER_MASK (drawable))
        return TRUE;
      else
        return FALSE;
    }

  g_return_val_if_reached (FALSE);
}

static GList *
gimp_edit_paste_get_tagged_layers (GimpImage         *image,
                                   GList             *layers,
                                   GList             *returned_layers,
                                   const Babl        *floating_format,
                                   GimpImageBaseType  base_type,
                                   GimpPrecision      precision,
                                   GimpPasteType      paste_type)
{
  GList *iter;

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayer *layer;
      GType      layer_type;
      gboolean   copied = TRUE;

      switch (paste_type)
        {
        case GIMP_PASTE_TYPE_FLOATING:
        case GIMP_PASTE_TYPE_FLOATING_IN_PLACE:
        case GIMP_PASTE_TYPE_FLOATING_INTO:
        case GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
          /*  when pasting as floating make sure gimp_item_convert()
           *  will turn group layers into normal layers, otherwise use
           *  the same layer type so e.g. text information gets
           *  preserved. See issue #2667.
           */
          if (GIMP_IS_GROUP_LAYER (iter->data))
            layer_type = GIMP_TYPE_LAYER;
          else
            layer_type = G_TYPE_FROM_INSTANCE (iter->data);
          break;

        case GIMP_PASTE_TYPE_NEW_LAYER:
        case GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE:
          layer_type = G_TYPE_FROM_INSTANCE (iter->data);
          break;

        default:
          g_return_val_if_reached (NULL);
        }

      if (GIMP_IS_GROUP_LAYER (iter->data))
        copied = (gboolean) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (iter->data),
                                                                "gimp-image-copied-layer"));
      if (copied)
        {
          layer = GIMP_LAYER (gimp_item_convert (GIMP_ITEM (iter->data),
                                                 image, layer_type));

          if (gimp_drawable_has_filters (GIMP_DRAWABLE (iter->data)))
            {
              GList         *filter_list;
              GimpContainer *filters;

              filters = gimp_drawable_get_filters (GIMP_DRAWABLE (iter->data));

              for (filter_list = GIMP_LIST (filters)->queue->tail;
                   filter_list;
                   filter_list = g_list_previous (filter_list))
                {
                  if (GIMP_IS_DRAWABLE_FILTER (filter_list->data))
                    {
                      GimpDrawableFilter *old_filter = filter_list->data;
                      GimpDrawableFilter *filter;

                      filter = gimp_drawable_filter_duplicate (GIMP_DRAWABLE (layer), old_filter);

                      if (filter != NULL)
                        {
                          gimp_drawable_filter_apply (filter, NULL);
                          gimp_drawable_filter_commit (filter, TRUE, NULL, FALSE);

                          gimp_drawable_filter_layer_mask_freeze (filter);
                          g_object_unref (filter);
                        }
                    }
                }
            }
          returned_layers = g_list_prepend (returned_layers, layer);

          switch (paste_type)
            {
            case GIMP_PASTE_TYPE_FLOATING:
            case GIMP_PASTE_TYPE_FLOATING_IN_PLACE:
            case GIMP_PASTE_TYPE_FLOATING_INTO:
            case GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
              /*  when pasting as floating selection, get rid of the layer mask,
               *  and make sure the layer has the right format
               */
              if (gimp_layer_get_mask (iter->data))
                gimp_layer_apply_mask (iter->data, GIMP_MASK_DISCARD, FALSE);

              if (gimp_drawable_get_format (GIMP_DRAWABLE (iter->data)) !=
                  floating_format)
                {
                  gimp_drawable_convert_type (GIMP_DRAWABLE (iter->data), image,
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
          GimpContainer *container;

          container = gimp_viewable_get_children (iter->data);
          returned_layers = gimp_edit_paste_get_tagged_layers (image,
                                                               GIMP_LIST (container)->queue->head,
                                                               returned_layers,
                                                               floating_format,
                                                               base_type, precision, paste_type);
        }
    }

  return returned_layers;
}

static GList *
gimp_edit_paste_get_layers (GimpImage     *image,
                            GList         *drawables,
                            GimpObject    *paste,
                            GimpPasteType *paste_type)
{
  GList      *layers = NULL;
  const Babl *floating_format;

  /*  change paste type to NEW_LAYER for cases where we can't attach a
   *  floating selection
   */
  if (g_list_length (drawables) != 1               ||
      gimp_viewable_get_children (drawables->data) ||
      gimp_item_is_content_locked (drawables->data, NULL))
    {
      if (gimp_edit_paste_is_in_place (*paste_type))
        *paste_type = GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE;
      else
        *paste_type = GIMP_PASTE_TYPE_NEW_LAYER;
    }

  /*  floating pastes always have the pasted-to drawable's format with
   *  alpha; if drawable == NULL, user is pasting into an empty image
   */
  if (drawables && gimp_edit_paste_is_floating (*paste_type, drawables->data))
    floating_format = gimp_drawable_get_format_with_alpha (drawables->data);
  else
    floating_format = gimp_image_get_layer_format (image, TRUE);

  if (GIMP_IS_IMAGE (paste))
    {
      GimpImageBaseType base_type;
      GimpPrecision     precision;

      layers = gimp_image_get_layer_iter (GIMP_IMAGE (paste));

      /* If pasting into an empty image, use the image information.
       * Otherwise, get it from the selected drawables */
      if (drawables && drawables->data)
        {
          base_type = gimp_drawable_get_base_type (drawables->data);
          precision = gimp_drawable_get_precision (drawables->data);
        }
      else
        {
          base_type = gimp_image_get_base_type (image);
          precision = gimp_image_get_precision (image);
        }

      if (g_list_length (layers) > 1)
        {
          if (gimp_edit_paste_is_in_place (*paste_type))
            *paste_type = GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE;
          else
            *paste_type = GIMP_PASTE_TYPE_NEW_LAYER;
        }

      layers = gimp_edit_paste_get_tagged_layers (image, layers, NULL, floating_format,
                                                  base_type, precision,
                                                  *paste_type);
      layers = g_list_reverse (layers);
    }
  else if (GIMP_IS_BUFFER (paste))
    {
      GimpLayer *layer;

      layer = gimp_layer_new_from_buffer (GIMP_BUFFER (paste), image,
                                          floating_format,
                                          _("Pasted Layer"),
                                          GIMP_OPACITY_OPAQUE,
                                          gimp_image_get_default_new_layer_mode (image));

      layers = g_list_prepend (layers, layer);
    }

  return layers;
}

static void
gimp_edit_paste_get_viewport_offset (GimpImage *image,
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

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (pasted_layers != NULL);
  g_return_if_fail (offset_x != NULL);
  g_return_if_fail (offset_y != NULL);

  image_width  = gimp_image_get_width  (image);
  image_height = gimp_image_get_height (image);

  for (iter = pasted_layers; iter; iter = iter->next)
    {
      gint layer_off_x;
      gint layer_off_y;
      gint layer_width;
      gint layer_height;

      g_return_if_fail (GIMP_IS_VIEWABLE (iter->data));

      gimp_item_get_offset (GIMP_ITEM (iter->data), &layer_off_x, &layer_off_y);
      gimp_viewable_get_size (iter->data, &layer_width, &layer_height);

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

      have_mask = ! gimp_channel_is_empty (gimp_image_get_mask (image));

      for (iter = drawables; iter; iter = iter->next)
        {
          GimpContainer *children;

          children = gimp_viewable_get_children (iter->data);
          if (! children || gimp_container_get_n_children (children) > 0)
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
          gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
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

              gimp_item_get_offset (GIMP_ITEM (iter->data), &layer_off_x, &layer_off_y);
              /* This is the bounds relatively to the drawable. */
              gimp_item_mask_intersect (GIMP_ITEM (iter->data),
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
          gimp_rectangle_intersect (viewport_x, viewport_y,
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
gimp_edit_paste_paste (GimpImage     *image,
                       GList         *drawables,
                       GList         *layers,
                       GimpPasteType  paste_type,
                       gboolean       use_offset,
                       gint           layers_bbox_x,
                       gint           layers_bbox_y,
                       gint           offset_x,
                       gint           offset_y)
{
  GList *iter;

  g_return_val_if_fail (paste_type > GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE ||
                        g_list_length (drawables) == 1, NULL);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                               C_("undo-type", "Paste"));

  for (iter = layers; iter; iter = iter->next)
    {
      /* Layers for in-place paste are already translated. */
      if (use_offset)
        gimp_item_translate (GIMP_ITEM (iter->data),
                             offset_x - layers_bbox_x,
                             offset_y - layers_bbox_y, FALSE);

      switch (paste_type)
        {
        case GIMP_PASTE_TYPE_FLOATING:
        case GIMP_PASTE_TYPE_FLOATING_IN_PLACE:
          /*  if there is a selection mask clear it - this might not
           *  always be desired, but in general, it seems like the correct
           *  behavior
           */
          if (! gimp_channel_is_empty (gimp_image_get_mask (image)))
            gimp_channel_clear (gimp_image_get_mask (image), NULL, TRUE);

          /* fall thru */

        case GIMP_PASTE_TYPE_FLOATING_INTO:
        case GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
          floating_sel_attach (iter->data, drawables->data);
          break;

        case GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
        case GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
          if (g_list_length (drawables) == 1 && GIMP_IS_LAYER_MASK (drawables->data))
            {
              if (! gimp_channel_is_empty (gimp_image_get_mask (image)))
                gimp_channel_clear (gimp_image_get_mask (image), NULL, TRUE);
              floating_sel_attach (iter->data, drawables->data);
              break;
            }

          /* fall thru if not a layer mask */

        case GIMP_PASTE_TYPE_NEW_LAYER:
        case GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE:
            {
              GimpLayer *parent   = NULL;
              gint       position = 0;

              /*  always add on top of a passed layer, where we would attach
               *  a floating selection
               */
              if (g_list_length (drawables) > 0 && GIMP_IS_LAYER (drawables->data))
                {
                  GimpDrawable *top_drawable;

                  top_drawable = gimp_edit_paste_get_top_item (drawables);
                  parent   = gimp_layer_get_parent (GIMP_LAYER (top_drawable));
                  position = gimp_item_get_index (GIMP_ITEM (top_drawable));
                }

              gimp_image_add_layer (image, iter->data, parent, position, TRUE);
            }
          break;

        case GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
        case GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
          g_return_val_if_reached (NULL);
        }
    }

  gimp_image_undo_group_end (image);

  return layers;
}

GList *
gimp_edit_paste (GimpImage     *image,
                 GList         *drawables,
                 GimpObject    *paste,
                 GimpPasteType  paste_type,
                 GimpContext   *context,
                 gboolean       merged,
                 gint           viewport_x,
                 gint           viewport_y,
                 gint           viewport_width,
                 gint           viewport_height)
{
  GList    *layers;
  gboolean  use_offset     = FALSE;
  gint      layers_bbox_x  = 0;
  gint      layers_bbox_y  = 0;
  gint      offset_y       = 0;
  gint      offset_x       = 0;
  GType     drawables_type = G_TYPE_NONE;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (paste) || GIMP_IS_BUFFER (paste), NULL);

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (GIMP_IS_DRAWABLE (iter->data), NULL);
      g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (iter->data)), NULL);
      g_return_val_if_fail (gimp_item_get_image (GIMP_ITEM (iter->data)) == image, NULL);
      g_return_val_if_fail (drawables_type == G_TYPE_NONE || G_OBJECT_TYPE (iter->data) == drawables_type, NULL);

      drawables_type = G_OBJECT_TYPE (iter->data);

      /* Currently we can only paste to channels (named channels, layer
       * masks, selection, etc.) as floating.
       */
      if (GIMP_IS_CHANNEL (iter->data))
        {
          switch (paste_type)
            {
            case GIMP_PASTE_TYPE_NEW_LAYER:
            case GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
            case GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
              paste_type = GIMP_PASTE_TYPE_FLOATING;
              break;
            case GIMP_PASTE_TYPE_NEW_LAYER_IN_PLACE:
            case GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
            case GIMP_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
              paste_type = GIMP_PASTE_TYPE_FLOATING_IN_PLACE;
              break;
            default:
              break;
            }
        }
    }

  if (merged && GIMP_IS_IMAGE (paste))
    {
      GimpImage *tmp_image;

      tmp_image = gimp_image_duplicate (GIMP_IMAGE (paste));
      gimp_container_remove (image->gimp->images, GIMP_OBJECT (tmp_image));
      gimp_image_merge_visible_layers (tmp_image, context, GIMP_EXPAND_AS_NECESSARY,
                                       FALSE, FALSE, NULL);
      layers = g_list_copy (gimp_image_get_layer_iter (tmp_image));

      /* The merge process should ensure that we get a single non-group and
       * no-mask layer.
       */
      g_return_val_if_fail (g_list_length (layers) == 1, NULL);

      layers->data = gimp_item_convert (GIMP_ITEM (layers->data), image,
                                        G_TYPE_FROM_INSTANCE (layers->data));

      switch (paste_type)
        {
        case GIMP_PASTE_TYPE_FLOATING:
        case GIMP_PASTE_TYPE_FLOATING_IN_PLACE:
        case GIMP_PASTE_TYPE_FLOATING_INTO:
        case GIMP_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
          if (gimp_drawable_get_format (GIMP_DRAWABLE (layers->data)) !=
              gimp_drawable_get_format_with_alpha (GIMP_DRAWABLE (layers->data)))
            {
              gimp_drawable_convert_type (GIMP_DRAWABLE (layers->data), image,
                                          gimp_drawable_get_base_type (layers->data),
                                          gimp_drawable_get_precision (layers->data),
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
      layers = gimp_edit_paste_get_layers (image, drawables, paste, &paste_type);
    }

  if (! layers)
    return NULL;

  if (gimp_edit_paste_is_in_place (paste_type))
    {
      if (GIMP_IS_BUFFER (paste))
        {
          GimpBuffer *buffer = GIMP_BUFFER (paste);

          use_offset = TRUE;
          offset_x   = buffer->offset_x;
          offset_y   = buffer->offset_y;
        }
    }
  else
    {
      use_offset = TRUE;
      gimp_edit_paste_get_viewport_offset (image, drawables, layers,
                                           viewport_x,
                                           viewport_y,
                                           viewport_width,
                                           viewport_height,
                                           &layers_bbox_x,
                                           &layers_bbox_y,
                                           &offset_x,
                                           &offset_y);
    }

  return gimp_edit_paste_paste (image, drawables, layers, paste_type,
                                use_offset, layers_bbox_x, layers_bbox_y,
                                offset_x, offset_y);
}

GimpImage *
gimp_edit_paste_as_new_image (Gimp        *gimp,
                              GimpObject  *paste,
                              GimpContext *context)
{
  GimpImage *image = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (paste) || GIMP_IS_BUFFER (paste), NULL);

  if (GIMP_IS_IMAGE (paste))
    {
      gint offset_x;
      gint offset_y;
      gint new_width;
      gint new_height;

      offset_x   = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (paste),
                                                       "gimp-edit-new-image-x"));
      offset_y   = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (paste),
                                                       "gimp-edit-new-image-y"));
      new_width  = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (paste),
                                                       "gimp-edit-new-image-width"));
      new_height = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (paste),
                                                       "gimp-edit-new-image-height"));
      image = gimp_image_duplicate (GIMP_IMAGE (paste));
      if (new_width > 0 && new_height > 0)
        {
          gimp_image_undo_disable (image);
          gimp_image_resize (image, context,
                             new_width, new_height,
                             -offset_x, -offset_y,
                             NULL);
          gimp_image_undo_enable (image);
        }
    }
  else if (GIMP_IS_BUFFER (paste))
    {
      image = gimp_image_new_from_buffer (gimp, GIMP_BUFFER (paste));
    }

  return image;
}

const gchar *
gimp_edit_named_cut (GimpImage     *image,
                     const gchar   *name,
                     GList         *drawables,
                     GimpContext   *context,
                     GError       **error)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  buffer = gimp_edit_extract (image, drawables, context, TRUE, error);

  if (buffer)
    {
      gimp_object_set_name (GIMP_OBJECT (buffer), name);
      gimp_container_add (image->gimp->named_buffers, GIMP_OBJECT (buffer));
      g_object_unref (buffer);

      return gimp_object_get_name (buffer);
    }

  return NULL;
}

const gchar *
gimp_edit_named_copy (GimpImage     *image,
                      const gchar   *name,
                      GList         *drawables,
                      GimpContext   *context,
                      GError       **error)
{
  GimpBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  buffer = gimp_edit_extract (image, drawables, context, FALSE, error);

  if (buffer)
    {
      gimp_object_set_name (GIMP_OBJECT (buffer), name);
      gimp_container_add (image->gimp->named_buffers, GIMP_OBJECT (buffer));
      g_object_unref (buffer);

      return gimp_object_get_name (buffer);
    }

  return NULL;
}

const gchar *
gimp_edit_named_copy_visible (GimpImage    *image,
                              const gchar  *name,
                              GimpContext  *context,
                              GError      **error)
{
  GimpBuffer *buffer;
  GList      *pickables;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  pickables = g_list_prepend (NULL, image);
  buffer = gimp_edit_extract (image, pickables, context, FALSE, error);
  g_list_free (pickables);

  if (buffer)
    {
      gimp_object_set_name (GIMP_OBJECT (buffer), name);
      gimp_container_add (image->gimp->named_buffers, GIMP_OBJECT (buffer));
      g_object_unref (buffer);

      return gimp_object_get_name (buffer);
    }

  return NULL;
}


/*  private functions  */

/**
 * gimp_edit_extract:
 * @image:
 * @pickables:
 * @context:
 * @cut_pixels:
 * @error:
 *
 * Extracts the selected part of @image from the list of @pickables.
 * If @cut_pixels is %TRUE, and there is only one pickable input, and if
 * this pickable is a #GimpDrawable, then the selected pixels will be
 * effectively erased from the input pickable.
 * Otherwise @cut_pixels has no additional effect.
 * Note that all @pickables must belong to the same @image.
 *
 * Returns: a #GimpBuffer of the selected part of @image as if only the
 * selected @pickables were present (composited according to their
 * properties, unless there is only one pickable, in which case direct
 * pixel information is used without composition).
 */
static GimpBuffer *
gimp_edit_extract (GimpImage     *image,
                   GList         *pickables,
                   GimpContext   *context,
                   gboolean       cut_pixels,
                   GError       **error)
{
  GeglBuffer *buffer;
  gint        offset_x;
  gint        offset_y;

  g_return_val_if_fail (g_list_length (pickables) > 0, NULL);

  if (g_list_length (pickables) > 1 ||
      ! GIMP_IS_DRAWABLE (pickables->data))
    cut_pixels = FALSE;

  if (cut_pixels)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_CUT,
                                 C_("undo-type", "Cut"));

  /*  Cut/copy the mask portion from the image  */
  buffer = gimp_selection_extract (GIMP_SELECTION (gimp_image_get_mask (image)),
                                   pickables, context,
                                   cut_pixels, FALSE, FALSE,
                                   &offset_x, &offset_y, error);

  if (cut_pixels)
    gimp_image_undo_group_end (image);

  if (buffer)
    {
      GimpBuffer *gimp_buffer;
      gdouble     res_x;
      gdouble     res_y;

      gimp_buffer = gimp_buffer_new (buffer, _("Global Buffer"),
                                     offset_x, offset_y, FALSE);
      g_object_unref (buffer);

      gimp_image_get_resolution (image, &res_x, &res_y);
      gimp_buffer_set_resolution (gimp_buffer, res_x, res_y);
      gimp_buffer_set_unit (gimp_buffer, gimp_image_get_unit (image));

      if (GIMP_IS_COLOR_MANAGED (pickables->data))
        {
          GimpColorProfile *profile =
            gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (pickables->data));

          if (profile)
            gimp_buffer_set_color_profile (gimp_buffer, profile);
        }

      return gimp_buffer;
    }

  return NULL;
}

/* Return the visually top item. */
static GimpDrawable *
gimp_edit_paste_get_top_item (GList *drawables)
{
  GList        *iter;
  GimpDrawable *top      = NULL;
  GList        *top_path = NULL;

  for (iter = drawables; iter; iter = iter->next)
    {
      GList *path = gimp_item_get_path (iter->data);

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
