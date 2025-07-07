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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gegl/gimp-babl-compat.h"
#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-nodes.h"
#include "gegl/gimp-gegl-utils.h"

#include "path/gimppath.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdrawable-filters.h"
#include "gimperror.h"
#include "gimpgrouplayer.h"
#include "gimpimage.h"
#include "gimpimage-merge.h"
#include "gimpimage-undo.h"
#include "gimpitemstack.h"
#include "gimplayer-floating-selection.h"
#include "gimplayer-new.h"
#include "gimplayermask.h"
#include "gimpparasitelist.h"
#include "gimppickable.h"
#include "gimpprogress.h"
#include "gimpprojectable.h"
#include "gimpundostack.h"

#include "gimp-intl.h"


static GSList    * gimp_image_trim_merge_list (GimpImage     *image,
                                               GSList        *merge_list);
static GimpLayer * gimp_image_merge_layers    (GimpImage     *image,
                                               GimpContainer *container,
                                               GSList        *merge_list,
                                               GimpContext   *context,
                                               GimpMergeType  merge_type,
                                               const gchar   *undo_desc,
                                               GimpProgress  *progress);


/*  public functions  */

GList *
gimp_image_merge_visible_layers (GimpImage     *image,
                                 GimpContext   *context,
                                 GimpMergeType  merge_type,
                                 gboolean       merge_active_group,
                                 gboolean       discard_invisible,
                                 GimpProgress  *progress)
{
  const gchar *undo_desc  = C_("undo-type", "Merge Visible Layers");
  GList       *containers = NULL;
  GList       *new_layers = NULL;
  GList       *iter;
  GList       *iter2;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  if (merge_active_group)
    {
      GList *selected_layers = gimp_image_get_selected_layers (image);

      /*  if the active layer is the floating selection, get the
       *  underlying drawable, but only if it is a layer
       */
      if (g_list_length (selected_layers) == 1 && gimp_layer_is_floating_sel (selected_layers->data))
        {
          GimpDrawable *fs_drawable;

          fs_drawable = gimp_layer_get_floating_sel_drawable (selected_layers->data);

          if (GIMP_IS_LAYER (fs_drawable))
            containers = g_list_prepend (containers,
                                         gimp_item_get_container (GIMP_ITEM (fs_drawable)));
        }
      else
        {
          for (iter = selected_layers; iter; iter = iter->next)
            if (! gimp_item_get_parent (iter->data))
              break;

          /* No need to list selected groups if any selected layer is
           * top-level.
           */
          if (iter == NULL)
            {
              for (iter = selected_layers; iter; iter = iter->next)
                {
                  for (iter2 = selected_layers; iter2; iter2 = iter2->next)
                    {
                      /* Only retain a selected layer's container if no
                       * other selected layers are its parents.
                       */
                      if (iter->data != iter2->data &&
                          gimp_viewable_is_ancestor (iter2->data, iter->data))
                        break;
                    }
                  if (iter2 == NULL &&
                      ! g_list_find (containers, gimp_item_get_container (GIMP_ITEM (iter->data))))
                    containers = g_list_prepend (containers,
                                                 gimp_item_get_container (GIMP_ITEM (iter->data)));
                }
            }
        }
    }
  if (! containers)
    containers = g_list_prepend (NULL, gimp_image_get_layers (image));

  gimp_set_busy (image->gimp);
  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               undo_desc);

  for (iter = containers; iter; iter = iter->next)
    {
      GimpContainer *container      = iter->data;
      GSList        *merge_list     = NULL;
      GSList        *invisible_list = NULL;

      for (iter2 = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (container));
           iter2;
           iter2 = g_list_next (iter2))
        {
          GimpLayer *layer = iter2->data;

          if (gimp_layer_is_floating_sel (layer))
            continue;

          if (gimp_item_get_visible (GIMP_ITEM (layer)))
            {
              merge_list = g_slist_append (merge_list, layer);
            }
          else if (discard_invisible)
            {
              invisible_list = g_slist_append (invisible_list, layer);
            }
        }

      if (merge_list)
        {
          GimpLayer *layer;

          /* if there's a floating selection, anchor it */
          if (gimp_image_get_floating_selection (image))
            floating_sel_anchor (gimp_image_get_floating_selection (image));

          layer = gimp_image_merge_layers (image,
                                           container,
                                           merge_list, context, merge_type,
                                           undo_desc, progress);
          g_slist_free (merge_list);

          if (invisible_list)
            {
              GSList *list;

              for (list = invisible_list; list; list = g_slist_next (list))
                gimp_image_remove_layer (image, list->data, TRUE, NULL);

              g_slist_free (invisible_list);
            }

          new_layers = g_list_prepend (new_layers, layer);
        }
    }

  gimp_image_set_selected_layers (image, new_layers);
  gimp_image_undo_group_end (image);
  gimp_unset_busy (image->gimp);

  g_list_free (new_layers);
  g_list_free (containers);

  return gimp_image_get_selected_layers (image);
}

GimpLayer *
gimp_image_flatten (GimpImage     *image,
                    GimpContext   *context,
                    GimpProgress  *progress,
                    GError       **error)
{
  GList     *list;
  GSList    *merge_list = NULL;
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (list = gimp_image_get_layer_iter (image);
       list;
       list = g_list_next (list))
    {
      layer = list->data;

      if (gimp_layer_is_floating_sel (layer))
        continue;

      if (gimp_item_get_visible (GIMP_ITEM (layer)))
        merge_list = g_slist_append (merge_list, layer);
    }

  if (merge_list)
    {
      const gchar *undo_desc = C_("undo-type", "Flatten Image");

      gimp_set_busy (image->gimp);

      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                                   undo_desc);

      /* if there's a floating selection, anchor it */
      if (gimp_image_get_floating_selection (image))
        floating_sel_anchor (gimp_image_get_floating_selection (image));

      layer = gimp_image_merge_layers (image,
                                       gimp_image_get_layers (image),
                                       merge_list, context,
                                       GIMP_FLATTEN_IMAGE,
                                       undo_desc, progress);
      g_slist_free (merge_list);

      gimp_image_alpha_changed (image);

      gimp_image_undo_group_end (image);

      gimp_unset_busy (image->gimp);

      return layer;
    }

  g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                       _("Cannot flatten an image without any visible layer."));
  return NULL;
}

GList *
gimp_image_merge_down (GimpImage      *image,
                       GList          *layers,
                       GimpContext    *context,
                       GimpMergeType   merge_type,
                       GimpProgress   *progress,
                       GError        **error)
{
  GList       *merged_layers = NULL;
  GList       *merge_lists   = NULL;
  GSList      *merge_list    = NULL;
  GimpLayer   *layer;
  GList       *list;
  const gchar *undo_desc;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  layers = g_list_copy (layers);
  while ((list = layers))
    {
      GList *list2;

      g_return_val_if_fail (GIMP_IS_LAYER (list->data), NULL);
      g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (list->data)), NULL);

      if (gimp_layer_is_floating_sel (list->data))
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("Cannot merge down a floating selection."));
          g_list_free (layers);
          g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
          return NULL;
        }

      if (! gimp_item_get_visible (GIMP_ITEM (list->data)))
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("Cannot merge down an invisible layer."));
          g_list_free (layers);
          g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
          return NULL;
        }

      for (list2 = gimp_item_get_container_iter (GIMP_ITEM (list->data));
           list2;
           list2 = g_list_next (list2))
        {
          if (list2->data == list->data)
            break;
        }

      layer = NULL;
      for (list2 = g_list_next (list2);
           list2;
           list2 = g_list_next (list2))
        {
          layer = list2->data;

          if (gimp_item_get_visible (GIMP_ITEM (layer)))
            {
              if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
                {
                  g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                       _("Cannot merge down to a layer group."));
                  g_list_free (layers);
                  g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
                  return NULL;
                }

              if (gimp_item_is_content_locked (GIMP_ITEM (layer), NULL))
                {
                  g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                       _("The layer to merge down to is locked."));
                  g_list_free (layers);
                  g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
                  return NULL;
                }

              break;
            }

          layer = NULL;
        }

      if (! layer)
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("There is no visible layer to merge down to."));
          g_list_free (layers);
          g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
          return NULL;
        }

      merge_list = g_slist_append (merge_list, list->data);
      layers     = g_list_delete_link (layers, layers);

      if ((list = g_list_find (layers, layer)))
        {
          /* The next item is itself in the merge down list. Continue
           * filling the same merge list instead of starting a new one.
           */
          layers = g_list_delete_link (layers, list);
          layers = g_list_prepend (layers, layer);

          continue;
        }

      merge_list  = g_slist_append (merge_list, layer);
      merge_lists = g_list_prepend (merge_lists, merge_list);
      merge_list  = NULL;
    }

  undo_desc = C_("undo-type", "Merge Down");

  gimp_set_busy (image->gimp);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               undo_desc);

  for (list = merge_lists; list; list = list->next)
    {
      merge_list = list->data;
      layer = gimp_image_merge_layers (image,
                                       gimp_item_get_container (merge_list->data),
                                       merge_list, context, merge_type,
                                       undo_desc, progress);
      merged_layers = g_list_prepend (merged_layers, layer);
    }

  g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);

  gimp_image_undo_group_end (image);

  gimp_unset_busy (image->gimp);

  return merged_layers;
}

GimpLayer *
gimp_image_merge_group_layer (GimpImage      *image,
                              GimpGroupLayer *group)
{
  GimpLayer     *parent;
  GimpLayer     *layer;
  GeglBuffer    *pass_through_buffer = NULL;
  gboolean       is_pass_through     = FALSE;
  gint           index;
  GeglRectangle  rect;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);
  g_return_val_if_fail (gimp_item_get_image (GIMP_ITEM (group)) == image, NULL);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               C_("undo-type", "Merge Layer Group"));

  parent = gimp_layer_get_parent (GIMP_LAYER (group));
  index  = gimp_item_get_index (GIMP_ITEM (group));

  is_pass_through = (gimp_layer_get_mode (GIMP_LAYER (group)) == GIMP_LAYER_MODE_PASS_THROUGH &&
                     gimp_item_get_visible (GIMP_ITEM (group)));
  if (is_pass_through &&
      (gimp_layer_get_opacity (GIMP_LAYER (group)) < 1.0 ||
       ! gimp_drawable_has_visible_filters (GIMP_DRAWABLE (group))))
    {
      GimpDrawable  *drawable  = GIMP_DRAWABLE (group);
      GeglNode      *mode_node = gimp_drawable_get_mode_node (drawable);

      rect = gegl_node_get_bounding_box (mode_node);
      pass_through_buffer = gegl_buffer_new (&rect, gimp_drawable_get_format (drawable));
      gegl_node_blit_buffer (mode_node, pass_through_buffer, &rect, 0, GEGL_ABYSS_NONE);
    }

  /* Merge down filter effects */
  gimp_drawable_merge_filters (GIMP_DRAWABLE (group));
  gimp_drawable_clear_filters (GIMP_DRAWABLE (group));

  layer = GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (group),
                                           GIMP_TYPE_LAYER));

  gimp_object_set_name (GIMP_OBJECT (layer), gimp_object_get_name (group));

  gimp_image_remove_layer (image, GIMP_LAYER (group), TRUE, NULL);
  gimp_image_add_layer (image, layer, parent, index, TRUE);

  /* Pass-through groups are a very special case. The duplicate works
   * only if both these points are true:
   * - The group is at full opacity: with lower opacity, what we want is
   *   in fact the output of the "gimp:pass-through" mode (similar to
   *   "gimp:replace") because we can't reproduce the same render
   *   otherwise. This works well, since anyway the merged layer is
   *   ensured to be the bottomest one on its own level.
   * - The group has filters: gimp_drawable_merge_filters() will
   *   actually set the end-rendering to the drawable (kinda rasterizing
   *   the group layer).
   */
   if (pass_through_buffer)
    {
      if (rect.x != 0 || rect.y != 0)
        {
          GeglBuffer *buffer;

          buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, rect.width, rect.height),
                                    gimp_drawable_get_format (GIMP_DRAWABLE (layer)));
          gegl_buffer_copy (pass_through_buffer, &rect, GEGL_ABYSS_NONE,
                            buffer, GEGL_RECTANGLE (0, 0, rect.width, rect.height));
          g_object_unref (pass_through_buffer);
          pass_through_buffer = buffer;
        }
      gimp_drawable_set_buffer_full (GIMP_DRAWABLE (layer), FALSE, NULL,
                                     pass_through_buffer, &rect, TRUE);
      g_object_unref (pass_through_buffer);
    }

  /* For pass-through group layers, we must remove all "big sister"
   * layers, i.e. all layers on the same level below the newly merged
   * layer, because their render is already integrated in the merged
   * layer. Therefore keeping them would change the whole image's
   * rendering.
   */
  if (is_pass_through)
    {
      GimpContainer *stack;
      GList         *iter;
      GList         *new_selected = g_list_prepend (NULL, layer);
      GList         *to_remove    = NULL;
      gboolean       remove       = FALSE;

      if (parent)
        stack = gimp_viewable_get_children (GIMP_VIEWABLE (parent));
      else
        stack = gimp_image_get_layers (image);

      for (iter = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (stack)); iter; iter = iter->next)
        {
          if (iter->data == layer)
            remove = TRUE;
          else if (remove && gimp_item_get_visible (iter->data))
            to_remove = g_list_prepend (to_remove, iter->data);
        }

      for (iter = to_remove; iter; iter = iter->next)
        gimp_image_remove_layer (image, GIMP_LAYER (iter->data), TRUE, new_selected);

      g_list_free (new_selected);
    }

  gimp_image_undo_group_end (image);

  return layer;
}


/* merging paths */

GimpPath *
gimp_image_merge_visible_paths (GimpImage  *image,
                                GError    **error)
{
  GList       *list;
  GList       *merge_list = NULL;
  GimpPath    *path;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (list = gimp_image_get_path_iter (image);
       list;
       list = g_list_next (list))
    {
      path = list->data;

      if (gimp_item_get_visible (GIMP_ITEM (path)))
        merge_list = g_list_prepend (merge_list, path);
    }

  merge_list = g_list_reverse (merge_list);

  if (merge_list && merge_list->next)
    {
      GimpPath *target_path;
      gchar    *name;
      gint      pos;

      gimp_set_busy (image->gimp);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_PATHS_MERGE,
                                   C_("undo-type", "Merge Visible Paths"));

      path = GIMP_PATH (merge_list->data);

      name = g_strdup (gimp_object_get_name (path));
      pos = gimp_item_get_index (GIMP_ITEM (path));

      target_path = GIMP_PATH (gimp_item_duplicate (GIMP_ITEM (path),
                                                    GIMP_TYPE_PATH));
      gimp_image_remove_path (image, path, TRUE, NULL);

      for (list = g_list_next (merge_list);
           list;
           list = g_list_next (list))
        {
          path = list->data;

          gimp_path_add_strokes (path, target_path);
          gimp_image_remove_path (image, path, TRUE, NULL);
        }

      gimp_object_take_name (GIMP_OBJECT (target_path), name);

      g_list_free (merge_list);

      /* FIXME tree */
      gimp_image_add_path (image, target_path, NULL, pos, TRUE);
      gimp_unset_busy (image->gimp);

      gimp_image_undo_group_end (image);

      return target_path;
    }
  else
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Not enough visible paths for a merge. "
                             "There must be at least two."));
      return NULL;
    }
}


/*  private functions  */

static GSList *
gimp_image_trim_merge_list (GimpImage *image,
                            GSList    *merge_list)
{
  GSList *trimmed_list = NULL;
  GSList *pass_through = NULL;

  for (GSList *iter = merge_list; iter; iter = iter->next)
    {
      GimpLayer *layer  = iter->data;
      gboolean   ignore = FALSE;

      for (GSList *iter2 = pass_through; iter2; iter2 = iter2->next)
        {
          GimpLayer *pass_through_parent = gimp_layer_get_parent (iter2->data);
          GimpLayer *cousin              = layer;

          do
            {
              GimpLayer *cousin_parent = gimp_layer_get_parent (cousin);

              if (pass_through_parent == cousin_parent &&
                  gimp_item_get_index (GIMP_ITEM (iter2->data)) < gimp_item_get_index (GIMP_ITEM (cousin)))
                {
                  /* The "cousin" layer is in the same group layer as a
                   * pass-through group and below it. We don't merge it
                   * because it will be rendered already through the
                   * merged pass-through by definition.
                   */
                  ignore = TRUE;
                  break;
                }

              cousin = cousin_parent;
            }
          while (cousin != NULL);

          if (ignore)
            break;
        }

      if (! ignore)
        {
          trimmed_list = g_slist_append (trimmed_list, layer);

          if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)) &&
              gimp_layer_get_mode (layer) == GIMP_LAYER_MODE_PASS_THROUGH)
            pass_through = g_slist_append (pass_through, layer);
        }
    }

  g_slist_free (pass_through);

  return trimmed_list;
}

static GimpLayer *
gimp_image_merge_layers (GimpImage     *image,
                         GimpContainer *container,
                         GSList        *merge_list,
                         GimpContext   *context,
                         GimpMergeType  merge_type,
                         const gchar   *undo_desc,
                         GimpProgress  *progress)
{
  GimpLayer        *parent;
  gint              x1, y1;
  gint              x2, y2;
  GSList           *layers;
  GimpLayer        *layer;
  GimpLayer        *top_layer;
  GimpLayer        *bottom_layer;
  GimpLayer        *merge_layer;
  gint              position;
  gint              n_bottom_removed = 0;
  GeglNode         *node;
  GeglNode         *source_node;
  GeglNode         *flatten_node;
  GeglNode         *offset_node;
  GeglNode         *last_node;
  GeglNode         *last_node_source = NULL;
  GimpParasiteList *parasites;
  GSList           *trimmed_list;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  trimmed_list = gimp_image_trim_merge_list (image, merge_list);

  top_layer = trimmed_list->data;
  parent    = gimp_layer_get_parent (top_layer);

  /*  Make sure the image's graph is constructed, so that top-level layers have
   *  a parent node.
   */
  (void) gimp_projectable_get_graph (GIMP_PROJECTABLE (image));

  /*  Make sure the parent's graph is constructed, so that the top layer has a
   *  parent node, even if it is the child of a group layer (in particular, of
   *  an invisible group layer, whose graph may not have been constructed as a
   *  result of the above call.  see issue #2095.)
   */
  if (parent)
    (void) gimp_filter_get_node (GIMP_FILTER (parent));

  /*  Build our graph inside the top-layer's parent node  */
  source_node = gimp_filter_get_node (GIMP_FILTER (top_layer));
  node = gegl_node_get_parent (source_node);

  g_return_val_if_fail (node, NULL);

  /*  Get the layer extents  */
  x1 = y1 = 0;
  x2 = y2 = 0;
  for (layers = trimmed_list; layers; layers = g_slist_next (layers))
    {
      gint off_x, off_y;

      layer = layers->data;

      /* Merge down filter effects so they can be restored on undo */
      gimp_drawable_merge_filters (GIMP_DRAWABLE (layer));
      gimp_drawable_clear_filters (GIMP_DRAWABLE (layer));

      gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

      switch (merge_type)
        {
        case GIMP_EXPAND_AS_NECESSARY:
        case GIMP_CLIP_TO_IMAGE:
          if (layers == trimmed_list)
            {
              x1 = off_x;
              y1 = off_y;
              x2 = off_x + gimp_item_get_width  (GIMP_ITEM (layer));
              y2 = off_y + gimp_item_get_height (GIMP_ITEM (layer));
            }
          else
            {
              if (off_x < x1)
                x1 = off_x;
              if (off_y < y1)
                y1 = off_y;
              if ((off_x + gimp_item_get_width (GIMP_ITEM (layer))) > x2)
                x2 = (off_x + gimp_item_get_width (GIMP_ITEM (layer)));
              if ((off_y + gimp_item_get_height (GIMP_ITEM (layer))) > y2)
                y2 = (off_y + gimp_item_get_height (GIMP_ITEM (layer)));
            }

          if (merge_type == GIMP_CLIP_TO_IMAGE)
            {
              x1 = CLAMP (x1, 0, gimp_image_get_width  (image));
              y1 = CLAMP (y1, 0, gimp_image_get_height (image));
              x2 = CLAMP (x2, 0, gimp_image_get_width  (image));
              y2 = CLAMP (y2, 0, gimp_image_get_height (image));
            }
          break;

        case GIMP_CLIP_TO_BOTTOM_LAYER:
          if (layers->next == NULL)
            {
              x1 = off_x;
              y1 = off_y;
              x2 = off_x + gimp_item_get_width  (GIMP_ITEM (layer));
              y2 = off_y + gimp_item_get_height (GIMP_ITEM (layer));
            }
          break;

        case GIMP_FLATTEN_IMAGE:
          if (layers->next == NULL)
            {
              x1 = 0;
              y1 = 0;
              x2 = gimp_image_get_width  (image);
              y2 = gimp_image_get_height (image);
            }
          break;
        }
    }

  if ((x2 - x1) == 0 || (y2 - y1) == 0)
    {
      g_slist_free (trimmed_list);
      return NULL;
    }

  bottom_layer = layer;

  flatten_node = NULL;

  if (merge_type == GIMP_FLATTEN_IMAGE ||
      (gimp_drawable_is_indexed (GIMP_DRAWABLE (layer)) &&
       ! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer))))
    {
      merge_layer = gimp_layer_new (image, (x2 - x1), (y2 - y1),
                                    gimp_image_get_layer_format (image, FALSE),
                                    gimp_object_get_name (bottom_layer),
                                    GIMP_OPACITY_OPAQUE,
                                    gimp_image_get_default_new_layer_mode (image));

      if (! merge_layer)
        {
          g_warning ("%s: could not allocate merge layer", G_STRFUNC);

          g_slist_free (trimmed_list);
          return NULL;
        }

      /*  get the background for compositing  */
      flatten_node = gimp_gegl_create_flatten_node
        (gimp_context_get_background (context),
         gimp_layer_get_real_composite_space (bottom_layer));
    }
  else
    {
      /*  The final merged layer inherits the name of the bottom most layer
       *  and the resulting layer has an alpha channel whether or not the
       *  original did. Opacity is set to 100% and the MODE is set to normal.
       */

      merge_layer =
        gimp_layer_new (image, (x2 - x1), (y2 - y1),
                        gimp_drawable_get_format_with_alpha (GIMP_DRAWABLE (bottom_layer)),
                        gimp_object_get_name (bottom_layer),
                        GIMP_OPACITY_OPAQUE,
                        gimp_image_get_default_new_layer_mode (image));

      if (! merge_layer)
        {
          g_warning ("%s: could not allocate merge layer", G_STRFUNC);

          g_slist_free (trimmed_list);
          return NULL;
        }
    }

  if (merge_type == GIMP_FLATTEN_IMAGE)
    {
      position = 0;
    }
  else
    {
      /*  Find the index in the layer list of the bottom layer--we need this
       *  in order to add the final, merged layer to the layer list correctly
       */
      position =
        gimp_container_get_n_children (container) -
        gimp_container_get_child_index (container, GIMP_OBJECT (bottom_layer));
    }

  gimp_item_set_offset (GIMP_ITEM (merge_layer), x1, y1);

  offset_node = gegl_node_new_child (node,
                                     "operation", "gegl:translate",
                                     "x",         (gdouble) -x1,
                                     "y",         (gdouble) -y1,
                                     NULL);

  if (flatten_node)
    {
      gegl_node_add_child (node, flatten_node);
      g_object_unref (flatten_node);

      gegl_node_link_many (source_node, flatten_node, offset_node, NULL);
    }
  else
    {
      gegl_node_link_many (source_node, offset_node, NULL);
    }

  /*  Disconnect the bottom-layer node's input, unless it's a
   *  pass-through group.
   */
  if (gimp_layer_get_mode (bottom_layer) != GIMP_LAYER_MODE_PASS_THROUGH)
    {
      last_node        = gimp_filter_get_node (GIMP_FILTER (bottom_layer));
      last_node_source = gegl_node_get_producer (last_node, "input", NULL);

      gegl_node_disconnect (last_node, "input");
    }

  /*  Render the graph into the merge layer  */
  gimp_gegl_apply_operation (NULL, progress, undo_desc, offset_node,
                             gimp_drawable_get_buffer (
                               GIMP_DRAWABLE (merge_layer)),
                             NULL, FALSE);

  /*  Reconnect the bottom-layer node's input  */
  if (last_node_source)
    gegl_node_link (last_node_source, last_node);

  /*  Clean up the graph  */
  gegl_node_remove_child (node, offset_node);

  if (flatten_node)
    gegl_node_remove_child (node, flatten_node);

  /* Copy the tattoo and parasites of the bottom layer to the new layer */
  gimp_item_set_tattoo (GIMP_ITEM (merge_layer),
                        gimp_item_get_tattoo (GIMP_ITEM (bottom_layer)));

  parasites = gimp_item_get_parasites (GIMP_ITEM (bottom_layer));
  parasites = gimp_parasite_list_copy (parasites);
  gimp_item_set_parasites (GIMP_ITEM (merge_layer), parasites);
  g_object_unref (parasites);

  for (layers = trimmed_list; layers; layers = g_slist_next (layers))
    {
      /* Remove the sisters below merged pass-through group layers. */
      if (gimp_viewable_get_children (GIMP_VIEWABLE (layers->data)) &&
          gimp_layer_get_mode (layers->data) == GIMP_LAYER_MODE_PASS_THROUGH)
        {
          GimpLayer     *parent;
          GimpContainer *stack;
          GList         *iter;
          gboolean       remove    = FALSE;
          GList         *to_remove = NULL;

          parent = gimp_layer_get_parent (layers->data);
          if (parent)
            stack = gimp_viewable_get_children (GIMP_VIEWABLE (parent));
          else
            stack = gimp_image_get_layers (image);

          for (iter = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (stack)); iter; iter = iter->next)
            {
              if (iter->data == layers->data)
                remove = TRUE;
              else if (remove && gimp_item_get_visible (GIMP_ITEM (iter->data)))
                to_remove = g_list_prepend (to_remove, iter->data);
            }

          if (layers->data == bottom_layer)
            n_bottom_removed = g_list_length (to_remove);

          for (iter = to_remove; iter; iter = iter->next)
            gimp_image_remove_layer (image, iter->data, TRUE, NULL);

          g_list_free (to_remove);
        }

      /*  Remove the merged layers from the image  */
      gimp_image_remove_layer (image, layers->data, TRUE, NULL);
    }

  gimp_item_set_visible (GIMP_ITEM (merge_layer), TRUE, FALSE);

  /*  if the type is flatten, remove all the remaining layers  */
  if (merge_type == GIMP_FLATTEN_IMAGE)
    {
      GList *list = gimp_image_get_layer_iter (image);

      while (list)
        {
          layer = list->data;

          list = g_list_next (list);
          gimp_image_remove_layer (image, layer, TRUE, NULL);
        }

      gimp_image_add_layer (image, merge_layer, parent,
                            position, TRUE);
    }
  else
    {
      /*  Add the layer to the image  */
      gimp_image_add_layer (image, merge_layer, parent,
                            gimp_container_get_n_children (container) +
                            n_bottom_removed - position + 1,
                            TRUE);
    }

  gimp_drawable_update (GIMP_DRAWABLE (merge_layer), 0, 0, -1, -1);
  g_slist_free (trimmed_list);

  return merge_layer;
}
