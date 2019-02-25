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

#include "vectors/gimpvectors.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimperror.h"
#include "gimpgrouplayer.h"
#include "gimpimage.h"
#include "gimpimage-merge.h"
#include "gimpimage-undo.h"
#include "gimpitemstack.h"
#include "gimplayer-floating-selection.h"
#include "gimplayer-new.h"
#include "gimplayermask.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"
#include "gimppickable.h"
#include "gimpprogress.h"
#include "gimpprojectable.h"
#include "gimpundostack.h"

#include "gimp-intl.h"


static GimpLayer * gimp_image_merge_layers (GimpImage     *image,
                                            GimpContainer *container,
                                            GSList        *merge_list,
                                            GimpContext   *context,
                                            GimpMergeType  merge_type,
                                            const gchar   *undo_desc,
                                            GimpProgress  *progress);


/*  public functions  */

GimpLayer *
gimp_image_merge_visible_layers (GimpImage     *image,
                                 GimpContext   *context,
                                 GimpMergeType  merge_type,
                                 gboolean       merge_active_group,
                                 gboolean       discard_invisible,
                                 GimpProgress  *progress)
{
  GimpContainer *container;
  GList         *list;
  GSList        *merge_list     = NULL;
  GSList        *invisible_list = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  if (merge_active_group)
    {
      GimpLayer *active_layer = gimp_image_get_active_layer (image);

      /*  if the active layer is the floating selection, get the
       *  underlying drawable, but only if it is a layer
       */
      if (active_layer && gimp_layer_is_floating_sel (active_layer))
        {
          GimpDrawable *fs_drawable;

          fs_drawable = gimp_layer_get_floating_sel_drawable (active_layer);

          if (GIMP_IS_LAYER (fs_drawable))
            active_layer = GIMP_LAYER (fs_drawable);
        }

      if (active_layer)
        container = gimp_item_get_container (GIMP_ITEM (active_layer));
      else
        container = gimp_image_get_layers (image);
    }
  else
    {
      container = gimp_image_get_layers (image);
    }

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (container));
       list;
       list = g_list_next (list))
    {
      GimpLayer *layer = list->data;

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
      GimpLayer   *layer;
      const gchar *undo_desc = C_("undo-type", "Merge Visible Layers");

      gimp_set_busy (image->gimp);

      gimp_image_undo_group_start (image,
                                   GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                                   undo_desc);

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

      gimp_image_undo_group_end (image);

      gimp_unset_busy (image->gimp);

      return layer;
    }

  return gimp_image_get_active_layer (image);
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

GimpLayer *
gimp_image_merge_down (GimpImage      *image,
                       GimpLayer      *current_layer,
                       GimpContext    *context,
                       GimpMergeType   merge_type,
                       GimpProgress   *progress,
                       GError        **error)
{
  GimpLayer   *layer;
  GList       *list;
  GList       *layer_list = NULL;
  GSList      *merge_list = NULL;
  const gchar *undo_desc;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (current_layer), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (current_layer)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (gimp_layer_is_floating_sel (current_layer))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Cannot merge down a floating selection."));
      return NULL;
    }

  if (! gimp_item_get_visible (GIMP_ITEM (current_layer)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Cannot merge down an invisible layer."));
      return NULL;
    }

  for (list = gimp_item_get_container_iter (GIMP_ITEM (current_layer));
       list;
       list = g_list_next (list))
    {
      layer = list->data;

      if (layer == current_layer)
        break;
    }

  for (layer_list = g_list_next (list);
       layer_list;
       layer_list = g_list_next (layer_list))
    {
      layer = layer_list->data;

      if (gimp_item_get_visible (GIMP_ITEM (layer)))
        {
          if (gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
            {
              g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                   _("Cannot merge down to a layer group."));
              return NULL;
            }

          if (gimp_item_is_content_locked (GIMP_ITEM (layer)))
            {
              g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                   _("The layer to merge down to is locked."));
              return NULL;
            }

          merge_list = g_slist_append (NULL, layer);
          break;
        }
    }

  if (! merge_list)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("There is no visible layer to merge down to."));
      return NULL;
    }

  merge_list = g_slist_prepend (merge_list, current_layer);

  undo_desc = C_("undo-type", "Merge Down");

  gimp_set_busy (image->gimp);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               undo_desc);

  layer = gimp_image_merge_layers (image,
                                   gimp_item_get_container (GIMP_ITEM (current_layer)),
                                   merge_list, context, merge_type,
                                   undo_desc, progress);
  g_slist_free (merge_list);

  gimp_image_undo_group_end (image);

  gimp_unset_busy (image->gimp);

  return layer;
}

GimpLayer *
gimp_image_merge_group_layer (GimpImage      *image,
                              GimpGroupLayer *group)
{
  GimpLayer *parent;
  GimpLayer *layer;
  gint       index;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);
  g_return_val_if_fail (gimp_item_get_image (GIMP_ITEM (group)) == image, NULL);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                               C_("undo-type", "Merge Layer Group"));

  parent = gimp_layer_get_parent (GIMP_LAYER (group));
  index  = gimp_item_get_index (GIMP_ITEM (group));

  /* if this is a pass-through group, change its mode to NORMAL *before*
   * duplicating it, since PASS_THROUGH mode is invalid for regular layers.
   * see bug #793714.
   */
  if (gimp_layer_get_mode (GIMP_LAYER (group)) == GIMP_LAYER_MODE_PASS_THROUGH)
    {
      GimpLayerColorSpace    blend_space;
      GimpLayerColorSpace    composite_space;
      GimpLayerCompositeMode composite_mode;

      /* keep the group's current blend space, composite space, and composite
       * mode.
       */
      blend_space     = gimp_layer_get_blend_space     (GIMP_LAYER (group));
      composite_space = gimp_layer_get_composite_space (GIMP_LAYER (group));
      composite_mode  = gimp_layer_get_composite_mode  (GIMP_LAYER (group));

      gimp_layer_set_mode            (GIMP_LAYER (group), GIMP_LAYER_MODE_NORMAL, TRUE);
      gimp_layer_set_blend_space     (GIMP_LAYER (group), blend_space,            TRUE);
      gimp_layer_set_composite_space (GIMP_LAYER (group), composite_space,        TRUE);
      gimp_layer_set_composite_mode  (GIMP_LAYER (group), composite_mode,         TRUE);
    }

  layer = GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (group),
                                           GIMP_TYPE_LAYER));

  gimp_object_set_name (GIMP_OBJECT (layer), gimp_object_get_name (group));

  gimp_image_remove_layer (image, GIMP_LAYER (group), TRUE, NULL);
  gimp_image_add_layer (image, layer, parent, index, TRUE);

  gimp_image_undo_group_end (image);

  return layer;
}


/* merging vectors */

GimpVectors *
gimp_image_merge_visible_vectors (GimpImage  *image,
                                  GError    **error)
{
  GList       *list;
  GList       *merge_list = NULL;
  GimpVectors *vectors;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (list = gimp_image_get_vectors_iter (image);
       list;
       list = g_list_next (list))
    {
      vectors = list->data;

      if (gimp_item_get_visible (GIMP_ITEM (vectors)))
        merge_list = g_list_prepend (merge_list, vectors);
    }

  merge_list = g_list_reverse (merge_list);

  if (merge_list && merge_list->next)
    {
      GimpVectors *target_vectors;
      gchar       *name;
      gint         pos;

      gimp_set_busy (image->gimp);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE,
                                   C_("undo-type", "Merge Visible Paths"));

      vectors = GIMP_VECTORS (merge_list->data);

      name = g_strdup (gimp_object_get_name (vectors));
      pos = gimp_item_get_index (GIMP_ITEM (vectors));

      target_vectors = GIMP_VECTORS (gimp_item_duplicate (GIMP_ITEM (vectors),
                                                          GIMP_TYPE_VECTORS));
      gimp_image_remove_vectors (image, vectors, TRUE, NULL);

      for (list = g_list_next (merge_list);
           list;
           list = g_list_next (list))
        {
          vectors = list->data;

          gimp_vectors_add_strokes (vectors, target_vectors);
          gimp_image_remove_vectors (image, vectors, TRUE, NULL);
        }

      gimp_object_take_name (GIMP_OBJECT (target_vectors), name);

      g_list_free (merge_list);

      /* FIXME tree */
      gimp_image_add_vectors (image, target_vectors, NULL, pos, TRUE);
      gimp_unset_busy (image->gimp);

      gimp_image_undo_group_end (image);

      return target_vectors;
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
  GeglNode         *node;
  GeglNode         *source_node;
  GeglNode         *flatten_node;
  GeglNode         *offset_node;
  GeglNode         *last_node;
  GeglNode         *last_node_source;
  GimpParasiteList *parasites;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  top_layer = merge_list->data;
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
  for (layers = merge_list; layers; layers = g_slist_next (layers))
    {
      gint off_x, off_y;

      layer = layers->data;

      gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

      switch (merge_type)
        {
        case GIMP_EXPAND_AS_NECESSARY:
        case GIMP_CLIP_TO_IMAGE:
          if (layers == merge_list)
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
    return NULL;

  bottom_layer = layer;

  flatten_node = NULL;

  if (merge_type == GIMP_FLATTEN_IMAGE ||
      (gimp_drawable_is_indexed (GIMP_DRAWABLE (layer)) &&
       ! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer))))
    {
      GimpRGB bg;

      merge_layer = gimp_layer_new (image, (x2 - x1), (y2 - y1),
                                    gimp_image_get_layer_format (image, FALSE),
                                    gimp_object_get_name (bottom_layer),
                                    GIMP_OPACITY_OPAQUE,
                                    gimp_image_get_default_new_layer_mode (image));

      if (! merge_layer)
        {
          g_warning ("%s: could not allocate merge layer", G_STRFUNC);

          return NULL;
        }

      /*  get the background for compositing  */
      gimp_context_get_background (context, &bg);
      gimp_pickable_srgb_to_image_color (GIMP_PICKABLE (layer),
                                         &bg, &bg);

      flatten_node = gimp_gegl_create_flatten_node (
        &bg, gimp_layer_get_real_composite_space (bottom_layer));

      position = 0;
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

          return NULL;
        }

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

  /*  Disconnect the bottom-layer node's input  */
  last_node        = gimp_filter_get_node (GIMP_FILTER (bottom_layer));
  last_node_source = gegl_node_get_producer (last_node, "input", NULL);

  gegl_node_disconnect (last_node, "input");

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

  /*  Remove the merged layers from the image  */
  for (layers = merge_list; layers; layers = g_slist_next (layers))
    gimp_image_remove_layer (image, layers->data, TRUE, NULL);

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
                            gimp_container_get_n_children (container) -
                            position + 1,
                            TRUE);
    }

  gimp_drawable_update (GIMP_DRAWABLE (merge_layer), 0, 0, -1, -1);

  return merge_layer;
}
