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

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "gegl/ligma-babl-compat.h"
#include "gegl/ligma-gegl-apply-operation.h"
#include "gegl/ligma-gegl-nodes.h"
#include "gegl/ligma-gegl-utils.h"

#include "vectors/ligmavectors.h"

#include "ligma.h"
#include "ligmacontext.h"
#include "ligmaerror.h"
#include "ligmagrouplayer.h"
#include "ligmaimage.h"
#include "ligmaimage-merge.h"
#include "ligmaimage-undo.h"
#include "ligmaitemstack.h"
#include "ligmalayer-floating-selection.h"
#include "ligmalayer-new.h"
#include "ligmalayermask.h"
#include "ligmaparasitelist.h"
#include "ligmapickable.h"
#include "ligmaprogress.h"
#include "ligmaprojectable.h"
#include "ligmaundostack.h"

#include "ligma-intl.h"


static LigmaLayer * ligma_image_merge_layers (LigmaImage     *image,
                                            LigmaContainer *container,
                                            GSList        *merge_list,
                                            LigmaContext   *context,
                                            LigmaMergeType  merge_type,
                                            const gchar   *undo_desc,
                                            LigmaProgress  *progress);


/*  public functions  */

GList *
ligma_image_merge_visible_layers (LigmaImage     *image,
                                 LigmaContext   *context,
                                 LigmaMergeType  merge_type,
                                 gboolean       merge_active_group,
                                 gboolean       discard_invisible,
                                 LigmaProgress  *progress)
{
  const gchar *undo_desc  = C_("undo-type", "Merge Visible Layers");
  GList       *containers = NULL;
  GList       *new_layers = NULL;
  GList       *iter;
  GList       *iter2;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);

  if (merge_active_group)
    {
      GList *selected_layers = ligma_image_get_selected_layers (image);

      /*  if the active layer is the floating selection, get the
       *  underlying drawable, but only if it is a layer
       */
      if (g_list_length (selected_layers) == 1 && ligma_layer_is_floating_sel (selected_layers->data))
        {
          LigmaDrawable *fs_drawable;

          fs_drawable = ligma_layer_get_floating_sel_drawable (selected_layers->data);

          if (LIGMA_IS_LAYER (fs_drawable))
            containers = g_list_prepend (containers,
                                         ligma_item_get_container (LIGMA_ITEM (fs_drawable)));
        }
      else
        {
          for (iter = selected_layers; iter; iter = iter->next)
            if (! ligma_item_get_parent (iter->data))
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
                          ligma_viewable_is_ancestor (iter2->data, iter->data))
                        break;
                    }
                  if (iter2 == NULL &&
                      ! g_list_find (containers, ligma_item_get_container (LIGMA_ITEM (iter->data))))
                    containers = g_list_prepend (containers,
                                                 ligma_item_get_container (LIGMA_ITEM (iter->data)));
                }
            }
        }
    }
  if (! containers)
    containers = g_list_prepend (NULL, ligma_image_get_layers (image));

  ligma_set_busy (image->ligma);
  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               undo_desc);

  for (iter = containers; iter; iter = iter->next)
    {
      LigmaContainer *container      = iter->data;
      GSList        *merge_list     = NULL;
      GSList        *invisible_list = NULL;

      for (iter2 = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (container));
           iter2;
           iter2 = g_list_next (iter2))
        {
          LigmaLayer *layer = iter2->data;

          if (ligma_layer_is_floating_sel (layer))
            continue;

          if (ligma_item_get_visible (LIGMA_ITEM (layer)))
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
          LigmaLayer   *layer;
          /* if there's a floating selection, anchor it */
          if (ligma_image_get_floating_selection (image))
            floating_sel_anchor (ligma_image_get_floating_selection (image));

          layer = ligma_image_merge_layers (image,
                                           container,
                                           merge_list, context, merge_type,
                                           undo_desc, progress);
          g_slist_free (merge_list);

          if (invisible_list)
            {
              GSList *list;

              for (list = invisible_list; list; list = g_slist_next (list))
                ligma_image_remove_layer (image, list->data, TRUE, NULL);

              g_slist_free (invisible_list);
            }

          new_layers = g_list_prepend (new_layers, layer);
        }
    }

  ligma_image_set_selected_layers (image, new_layers);
  ligma_image_undo_group_end (image);
  ligma_unset_busy (image->ligma);

  g_list_free (new_layers);
  g_list_free (containers);

  return ligma_image_get_selected_layers (image);
}

LigmaLayer *
ligma_image_flatten (LigmaImage     *image,
                    LigmaContext   *context,
                    LigmaProgress  *progress,
                    GError       **error)
{
  GList     *list;
  GSList    *merge_list = NULL;
  LigmaLayer *layer;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (list = ligma_image_get_layer_iter (image);
       list;
       list = g_list_next (list))
    {
      layer = list->data;

      if (ligma_layer_is_floating_sel (layer))
        continue;

      if (ligma_item_get_visible (LIGMA_ITEM (layer)))
        merge_list = g_slist_append (merge_list, layer);
    }

  if (merge_list)
    {
      const gchar *undo_desc = C_("undo-type", "Flatten Image");

      ligma_set_busy (image->ligma);

      ligma_image_undo_group_start (image,
                                   LIGMA_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                                   undo_desc);

      /* if there's a floating selection, anchor it */
      if (ligma_image_get_floating_selection (image))
        floating_sel_anchor (ligma_image_get_floating_selection (image));

      layer = ligma_image_merge_layers (image,
                                       ligma_image_get_layers (image),
                                       merge_list, context,
                                       LIGMA_FLATTEN_IMAGE,
                                       undo_desc, progress);
      g_slist_free (merge_list);

      ligma_image_alpha_changed (image);

      ligma_image_undo_group_end (image);

      ligma_unset_busy (image->ligma);

      return layer;
    }

  g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                       _("Cannot flatten an image without any visible layer."));
  return NULL;
}

GList *
ligma_image_merge_down (LigmaImage      *image,
                       GList          *layers,
                       LigmaContext    *context,
                       LigmaMergeType   merge_type,
                       LigmaProgress   *progress,
                       GError        **error)
{
  GList       *merged_layers = NULL;
  GList       *merge_lists   = NULL;
  GSList      *merge_list    = NULL;
  LigmaLayer   *layer;
  GList       *list;
  const gchar *undo_desc;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  layers = g_list_copy (layers);
  while ((list = layers))
    {
      GList *list2;

      g_return_val_if_fail (LIGMA_IS_LAYER (list->data), NULL);
      g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (list->data)), NULL);

      if (ligma_layer_is_floating_sel (list->data))
        {
          g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                               _("Cannot merge down a floating selection."));
          g_list_free (layers);
          g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
          return NULL;
        }

      if (! ligma_item_get_visible (LIGMA_ITEM (list->data)))
        {
          g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                               _("Cannot merge down an invisible layer."));
          g_list_free (layers);
          g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
          return NULL;
        }

      for (list2 = ligma_item_get_container_iter (LIGMA_ITEM (list->data));
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

          if (ligma_item_get_visible (LIGMA_ITEM (layer)))
            {
              if (ligma_viewable_get_children (LIGMA_VIEWABLE (layer)))
                {
                  g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                                       _("Cannot merge down to a layer group."));
                  g_list_free (layers);
                  g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);
                  return NULL;
                }

              if (ligma_item_is_content_locked (LIGMA_ITEM (layer), NULL))
                {
                  g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
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
          g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
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

  ligma_set_busy (image->ligma);

  ligma_image_undo_group_start (image,
                               LIGMA_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               undo_desc);

  for (list = merge_lists; list; list = list->next)
    {
      merge_list = list->data;
      layer = ligma_image_merge_layers (image,
                                       ligma_item_get_container (merge_list->data),
                                       merge_list, context, merge_type,
                                       undo_desc, progress);
      merged_layers = g_list_prepend (merged_layers, layer);
    }

  g_list_free_full (merge_lists, (GDestroyNotify) g_slist_free);

  ligma_image_undo_group_end (image);

  ligma_unset_busy (image->ligma);

  return merged_layers;
}

LigmaLayer *
ligma_image_merge_group_layer (LigmaImage      *image,
                              LigmaGroupLayer *group)
{
  LigmaLayer *parent;
  LigmaLayer *layer;
  gint       index;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (group)), NULL);
  g_return_val_if_fail (ligma_item_get_image (LIGMA_ITEM (group)) == image, NULL);

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               C_("undo-type", "Merge Layer Group"));

  parent = ligma_layer_get_parent (LIGMA_LAYER (group));
  index  = ligma_item_get_index (LIGMA_ITEM (group));

  /* if this is a pass-through group, change its mode to NORMAL *before*
   * duplicating it, since PASS_THROUGH mode is invalid for regular layers.
   * see bug #793714.
   */
  if (ligma_layer_get_mode (LIGMA_LAYER (group)) == LIGMA_LAYER_MODE_PASS_THROUGH)
    {
      LigmaLayerColorSpace    blend_space;
      LigmaLayerColorSpace    composite_space;
      LigmaLayerCompositeMode composite_mode;

      /* keep the group's current blend space, composite space, and composite
       * mode.
       */
      blend_space     = ligma_layer_get_blend_space     (LIGMA_LAYER (group));
      composite_space = ligma_layer_get_composite_space (LIGMA_LAYER (group));
      composite_mode  = ligma_layer_get_composite_mode  (LIGMA_LAYER (group));

      ligma_layer_set_mode            (LIGMA_LAYER (group), LIGMA_LAYER_MODE_NORMAL, TRUE);
      ligma_layer_set_blend_space     (LIGMA_LAYER (group), blend_space,            TRUE);
      ligma_layer_set_composite_space (LIGMA_LAYER (group), composite_space,        TRUE);
      ligma_layer_set_composite_mode  (LIGMA_LAYER (group), composite_mode,         TRUE);
    }

  layer = LIGMA_LAYER (ligma_item_duplicate (LIGMA_ITEM (group),
                                           LIGMA_TYPE_LAYER));

  ligma_object_set_name (LIGMA_OBJECT (layer), ligma_object_get_name (group));

  ligma_image_remove_layer (image, LIGMA_LAYER (group), TRUE, NULL);
  ligma_image_add_layer (image, layer, parent, index, TRUE);

  ligma_image_undo_group_end (image);

  return layer;
}


/* merging vectors */

LigmaVectors *
ligma_image_merge_visible_vectors (LigmaImage  *image,
                                  GError    **error)
{
  GList       *list;
  GList       *merge_list = NULL;
  LigmaVectors *vectors;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (list = ligma_image_get_vectors_iter (image);
       list;
       list = g_list_next (list))
    {
      vectors = list->data;

      if (ligma_item_get_visible (LIGMA_ITEM (vectors)))
        merge_list = g_list_prepend (merge_list, vectors);
    }

  merge_list = g_list_reverse (merge_list);

  if (merge_list && merge_list->next)
    {
      LigmaVectors *target_vectors;
      gchar       *name;
      gint         pos;

      ligma_set_busy (image->ligma);

      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_VECTORS_MERGE,
                                   C_("undo-type", "Merge Visible Paths"));

      vectors = LIGMA_VECTORS (merge_list->data);

      name = g_strdup (ligma_object_get_name (vectors));
      pos = ligma_item_get_index (LIGMA_ITEM (vectors));

      target_vectors = LIGMA_VECTORS (ligma_item_duplicate (LIGMA_ITEM (vectors),
                                                          LIGMA_TYPE_VECTORS));
      ligma_image_remove_vectors (image, vectors, TRUE, NULL);

      for (list = g_list_next (merge_list);
           list;
           list = g_list_next (list))
        {
          vectors = list->data;

          ligma_vectors_add_strokes (vectors, target_vectors);
          ligma_image_remove_vectors (image, vectors, TRUE, NULL);
        }

      ligma_object_take_name (LIGMA_OBJECT (target_vectors), name);

      g_list_free (merge_list);

      /* FIXME tree */
      ligma_image_add_vectors (image, target_vectors, NULL, pos, TRUE);
      ligma_unset_busy (image->ligma);

      ligma_image_undo_group_end (image);

      return target_vectors;
    }
  else
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Not enough visible paths for a merge. "
                             "There must be at least two."));
      return NULL;
    }
}


/*  private functions  */

static LigmaLayer *
ligma_image_merge_layers (LigmaImage     *image,
                         LigmaContainer *container,
                         GSList        *merge_list,
                         LigmaContext   *context,
                         LigmaMergeType  merge_type,
                         const gchar   *undo_desc,
                         LigmaProgress  *progress)
{
  LigmaLayer        *parent;
  gint              x1, y1;
  gint              x2, y2;
  GSList           *layers;
  LigmaLayer        *layer;
  LigmaLayer        *top_layer;
  LigmaLayer        *bottom_layer;
  LigmaLayer        *merge_layer;
  gint              position;
  GeglNode         *node;
  GeglNode         *source_node;
  GeglNode         *flatten_node;
  GeglNode         *offset_node;
  GeglNode         *last_node;
  GeglNode         *last_node_source;
  LigmaParasiteList *parasites;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);

  top_layer = merge_list->data;
  parent    = ligma_layer_get_parent (top_layer);

  /*  Make sure the image's graph is constructed, so that top-level layers have
   *  a parent node.
   */
  (void) ligma_projectable_get_graph (LIGMA_PROJECTABLE (image));

  /*  Make sure the parent's graph is constructed, so that the top layer has a
   *  parent node, even if it is the child of a group layer (in particular, of
   *  an invisible group layer, whose graph may not have been constructed as a
   *  result of the above call.  see issue #2095.)
   */
  if (parent)
    (void) ligma_filter_get_node (LIGMA_FILTER (parent));

  /*  Build our graph inside the top-layer's parent node  */
  source_node = ligma_filter_get_node (LIGMA_FILTER (top_layer));
  node = gegl_node_get_parent (source_node);

  g_return_val_if_fail (node, NULL);

  /*  Get the layer extents  */
  x1 = y1 = 0;
  x2 = y2 = 0;
  for (layers = merge_list; layers; layers = g_slist_next (layers))
    {
      gint off_x, off_y;

      layer = layers->data;

      ligma_item_get_offset (LIGMA_ITEM (layer), &off_x, &off_y);

      switch (merge_type)
        {
        case LIGMA_EXPAND_AS_NECESSARY:
        case LIGMA_CLIP_TO_IMAGE:
          if (layers == merge_list)
            {
              x1 = off_x;
              y1 = off_y;
              x2 = off_x + ligma_item_get_width  (LIGMA_ITEM (layer));
              y2 = off_y + ligma_item_get_height (LIGMA_ITEM (layer));
            }
          else
            {
              if (off_x < x1)
                x1 = off_x;
              if (off_y < y1)
                y1 = off_y;
              if ((off_x + ligma_item_get_width (LIGMA_ITEM (layer))) > x2)
                x2 = (off_x + ligma_item_get_width (LIGMA_ITEM (layer)));
              if ((off_y + ligma_item_get_height (LIGMA_ITEM (layer))) > y2)
                y2 = (off_y + ligma_item_get_height (LIGMA_ITEM (layer)));
            }

          if (merge_type == LIGMA_CLIP_TO_IMAGE)
            {
              x1 = CLAMP (x1, 0, ligma_image_get_width  (image));
              y1 = CLAMP (y1, 0, ligma_image_get_height (image));
              x2 = CLAMP (x2, 0, ligma_image_get_width  (image));
              y2 = CLAMP (y2, 0, ligma_image_get_height (image));
            }
          break;

        case LIGMA_CLIP_TO_BOTTOM_LAYER:
          if (layers->next == NULL)
            {
              x1 = off_x;
              y1 = off_y;
              x2 = off_x + ligma_item_get_width  (LIGMA_ITEM (layer));
              y2 = off_y + ligma_item_get_height (LIGMA_ITEM (layer));
            }
          break;

        case LIGMA_FLATTEN_IMAGE:
          if (layers->next == NULL)
            {
              x1 = 0;
              y1 = 0;
              x2 = ligma_image_get_width  (image);
              y2 = ligma_image_get_height (image);
            }
          break;
        }
    }

  if ((x2 - x1) == 0 || (y2 - y1) == 0)
    return NULL;

  bottom_layer = layer;

  flatten_node = NULL;

  if (merge_type == LIGMA_FLATTEN_IMAGE ||
      (ligma_drawable_is_indexed (LIGMA_DRAWABLE (layer)) &&
       ! ligma_drawable_has_alpha (LIGMA_DRAWABLE (layer))))
    {
      LigmaRGB bg;

      merge_layer = ligma_layer_new (image, (x2 - x1), (y2 - y1),
                                    ligma_image_get_layer_format (image, FALSE),
                                    ligma_object_get_name (bottom_layer),
                                    LIGMA_OPACITY_OPAQUE,
                                    ligma_image_get_default_new_layer_mode (image));

      if (! merge_layer)
        {
          g_warning ("%s: could not allocate merge layer", G_STRFUNC);

          return NULL;
        }

      /*  get the background for compositing  */
      ligma_context_get_background (context, &bg);
      ligma_pickable_srgb_to_image_color (LIGMA_PICKABLE (layer),
                                         &bg, &bg);

      flatten_node = ligma_gegl_create_flatten_node
        (&bg,
         ligma_drawable_get_space (LIGMA_DRAWABLE (layer)),
         ligma_layer_get_real_composite_space (bottom_layer));
    }
  else
    {
      /*  The final merged layer inherits the name of the bottom most layer
       *  and the resulting layer has an alpha channel whether or not the
       *  original did. Opacity is set to 100% and the MODE is set to normal.
       */

      merge_layer =
        ligma_layer_new (image, (x2 - x1), (y2 - y1),
                        ligma_drawable_get_format_with_alpha (LIGMA_DRAWABLE (bottom_layer)),
                        ligma_object_get_name (bottom_layer),
                        LIGMA_OPACITY_OPAQUE,
                        ligma_image_get_default_new_layer_mode (image));

      if (! merge_layer)
        {
          g_warning ("%s: could not allocate merge layer", G_STRFUNC);

          return NULL;
        }
    }

  if (merge_type == LIGMA_FLATTEN_IMAGE)
    {
      position = 0;
    }
  else
    {
      /*  Find the index in the layer list of the bottom layer--we need this
       *  in order to add the final, merged layer to the layer list correctly
       */
      position =
        ligma_container_get_n_children (container) -
        ligma_container_get_child_index (container, LIGMA_OBJECT (bottom_layer));
    }

  ligma_item_set_offset (LIGMA_ITEM (merge_layer), x1, y1);

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

  /*  Disconnect the bottom-layer node's input  */
  last_node        = ligma_filter_get_node (LIGMA_FILTER (bottom_layer));
  last_node_source = gegl_node_get_producer (last_node, "input", NULL);

  gegl_node_disconnect (last_node, "input");

  /*  Render the graph into the merge layer  */
  ligma_gegl_apply_operation (NULL, progress, undo_desc, offset_node,
                             ligma_drawable_get_buffer (
                               LIGMA_DRAWABLE (merge_layer)),
                             NULL, FALSE);

  /*  Reconnect the bottom-layer node's input  */
  if (last_node_source)
    gegl_node_link (last_node_source, last_node);

  /*  Clean up the graph  */
  gegl_node_remove_child (node, offset_node);

  if (flatten_node)
    gegl_node_remove_child (node, flatten_node);

  /* Copy the tattoo and parasites of the bottom layer to the new layer */
  ligma_item_set_tattoo (LIGMA_ITEM (merge_layer),
                        ligma_item_get_tattoo (LIGMA_ITEM (bottom_layer)));

  parasites = ligma_item_get_parasites (LIGMA_ITEM (bottom_layer));
  parasites = ligma_parasite_list_copy (parasites);
  ligma_item_set_parasites (LIGMA_ITEM (merge_layer), parasites);
  g_object_unref (parasites);

  /*  Remove the merged layers from the image  */
  for (layers = merge_list; layers; layers = g_slist_next (layers))
    ligma_image_remove_layer (image, layers->data, TRUE, NULL);

  ligma_item_set_visible (LIGMA_ITEM (merge_layer), TRUE, FALSE);

  /*  if the type is flatten, remove all the remaining layers  */
  if (merge_type == LIGMA_FLATTEN_IMAGE)
    {
      GList *list = ligma_image_get_layer_iter (image);

      while (list)
        {
          layer = list->data;

          list = g_list_next (list);
          ligma_image_remove_layer (image, layer, TRUE, NULL);
        }

      ligma_image_add_layer (image, merge_layer, parent,
                            position, TRUE);
    }
  else
    {
      /*  Add the layer to the image  */
      ligma_image_add_layer (image, merge_layer, parent,
                            ligma_container_get_n_children (container) -
                            position + 1,
                            TRUE);
    }

  ligma_drawable_update (LIGMA_DRAWABLE (merge_layer), 0, 0, -1, -1);

  return merge_layer;
}
