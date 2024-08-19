/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpexportoptions.c
 * Copyright (C) 2024 Alx Sa.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

#include "gimpexportoptions.h"

#include "libgimp-intl.h"


/* export helper functions */

typedef void (* ExportFunc) (GimpImage    *image,
                             GList       **drawables);

/* the export action structure */
typedef struct
{
  ExportFunc   default_action;
  ExportFunc   alt_action;
  const gchar *reason;
  const gchar *possibilities[2];
  gint         choice;
} ExportAction;


/* the functions that do the actual export */

static void
export_merge (GimpImage  *image,
              GList     **drawables)
{
  GList  *layers;
  GList  *iter;
  gint32  nvisible = 0;

  layers = gimp_image_list_layers (image);

  for (iter = layers; iter; iter = g_list_next (iter))
    {
      if (gimp_item_get_visible (GIMP_ITEM (iter->data)))
        nvisible++;
    }

  if (nvisible <= 1)
    {
      GimpLayer     *transp;
      GimpImageType  layer_type;

      /* if there is only one (or zero) visible layer, add a new
       * transparent layer that has the same size as the canvas.  The
       * merge that follows will ensure that the offset, opacity and
       * size are correct
       */
      switch (gimp_image_get_base_type (image))
        {
        case GIMP_RGB:
          layer_type = GIMP_RGBA_IMAGE;
          break;
        case GIMP_GRAY:
          layer_type = GIMP_GRAYA_IMAGE;
          break;
        case GIMP_INDEXED:
          layer_type = GIMP_INDEXEDA_IMAGE;
          break;
        default:
          /* In case we add a new type in future. */
          g_return_if_reached ();
        }
      transp = gimp_layer_new (image, "-",
                               gimp_image_get_width (image),
                               gimp_image_get_height (image),
                               layer_type,
                               100.0, GIMP_LAYER_MODE_NORMAL);
      gimp_image_insert_layer (image, transp, NULL, 1);
      gimp_selection_none (image);
      gimp_drawable_edit_clear (GIMP_DRAWABLE (transp));
      nvisible++;
    }

  if (nvisible > 1)
    {
      GimpLayer *merged;

      merged = gimp_image_merge_visible_layers (image, GIMP_CLIP_TO_IMAGE);

      g_return_if_fail (merged != NULL);

      *drawables = g_list_prepend (NULL, merged);

      g_list_free (layers);

      layers = gimp_image_list_layers (image);

      /*  make sure that the merged drawable matches the image size  */
      if (gimp_drawable_get_width   (GIMP_DRAWABLE (merged)) !=
          gimp_image_get_width  (image) ||
          gimp_drawable_get_height  (GIMP_DRAWABLE (merged)) !=
          gimp_image_get_height (image))
        {
          gint off_x, off_y;

          gimp_drawable_get_offsets (GIMP_DRAWABLE (merged), &off_x, &off_y);
          gimp_layer_resize (merged,
                             gimp_image_get_width (image),
                             gimp_image_get_height (image),
                             off_x, off_y);
        }
    }

  /* remove any remaining (invisible) layers */
  for (iter = layers; iter; iter = iter->next)
    {
      if (! g_list_find (*drawables, iter->data))
        gimp_image_remove_layer (image, iter->data);
    }

  g_list_free (layers);
}

static void
export_flatten (GimpImage  *image,
                GList     **drawables)
{
  GimpLayer *flattened;

  flattened = gimp_image_flatten (image);

  if (flattened != NULL)
    *drawables = g_list_prepend (NULL, flattened);
}

static void
export_merge_layer_effects_rec (GList *layers)
{
  GList *iter;

  for (iter = layers; iter; iter = g_list_next (iter))
    if (gimp_item_is_group (iter->data))
      {
        GList *children = gimp_item_list_children (iter->data);

        export_merge_layer_effects_rec (children);

        g_list_free (children);
      }
    else
      {
        gimp_drawable_merge_filters (GIMP_DRAWABLE (iter->data));
      }
}

static void
export_merge_layer_effects (GimpImage  *image,
                            GList     **drawables)
{
  GList *layers;

  layers = gimp_image_list_layers (image);
  export_merge_layer_effects_rec (layers);
  g_list_free (layers);
}

static void
export_remove_alpha (GimpImage  *image,
                     GList     **drawables)
{
  GList  *layers;
  GList  *iter;

  layers = gimp_image_list_layers (image);

  for (iter = layers; iter; iter = iter->next)
    {
      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (iter->data)))
        gimp_layer_flatten (iter->data);
    }

  g_list_free (layers);
}

static void
export_apply_masks (GimpImage  *image,
                    GList     **drawables)
{
  GList  *layers;
  GList  *iter;

  layers = gimp_image_list_layers (image);

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayerMask *mask;

      mask = gimp_layer_get_mask (iter->data);

      if (mask)
        {
          /* we can't apply the mask directly to a layer group, so merge it
           * first
           */
          if (gimp_item_is_group (iter->data))
            iter->data = gimp_group_layer_merge (iter->data);

          gimp_layer_remove_mask (iter->data, GIMP_MASK_APPLY);
        }
    }

  g_list_free (layers);
}

static void
export_convert_rgb (GimpImage  *image,
                    GList     **drawables)
{
  gimp_image_convert_rgb (image);
}

static void
export_convert_grayscale (GimpImage  *image,
                          GList     **drawables)
{
  gimp_image_convert_grayscale (image);
}

static void
export_convert_indexed (GimpImage  *image,
                        GList     **drawables)
{
  GList    *layers;
  GList    *iter;
  gboolean  has_alpha = FALSE;

  /* check alpha */
  layers = gimp_image_list_layers (image);

  for (iter = *drawables; iter; iter = iter->next)
    {
      if (gimp_drawable_has_alpha (iter->data))
        {
          has_alpha = TRUE;
          break;
        }
    }

  if (layers || has_alpha)
    gimp_image_convert_indexed (image,
                                GIMP_CONVERT_DITHER_NONE,
                                GIMP_CONVERT_PALETTE_GENERATE,
                                255, FALSE, FALSE, "");
  else
    gimp_image_convert_indexed (image,
                                GIMP_CONVERT_DITHER_NONE,
                                GIMP_CONVERT_PALETTE_GENERATE,
                                256, FALSE, FALSE, "");
  g_list_free (layers);
}

static void
export_convert_bitmap (GimpImage  *image,
                       GList     **drawables)
{
  if (gimp_image_get_base_type (image) == GIMP_INDEXED)
    gimp_image_convert_rgb (image);

  gimp_image_convert_indexed (image,
                              GIMP_CONVERT_DITHER_FS,
                              GIMP_CONVERT_PALETTE_GENERATE,
                              2, FALSE, FALSE, "");
}

static void
export_add_alpha (GimpImage  *image,
                  GList     **drawables)
{
  GList  *layers;
  GList  *iter;

  layers = gimp_image_list_layers (image);

  for (iter = layers; iter; iter = iter->next)
    {
      if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (iter->data)))
        gimp_layer_add_alpha (GIMP_LAYER (iter->data));
    }

  g_list_free (layers);
}

static void
export_crop_image (GimpImage  *image,
                   GList     **drawables)
{
  gimp_image_crop (image,
                   gimp_image_get_width  (image),
                   gimp_image_get_height (image),
                   0, 0);
}

static void
export_resize_image (GimpImage  *image,
                     GList     **drawables)
{
  gimp_image_resize_to_layers (image);
}

static void
export_void (GimpImage  *image,
             GList     **drawables)
{
  /* do nothing */
}


/* a set of predefined actions */

static ExportAction export_action_merge =
{
  export_merge,
  NULL,
  N_("%s plug-in can't handle layers"),
  { N_("Merge Visible Layers"), NULL },
  0
};

static ExportAction export_action_merge_single =
{
  export_merge,
  NULL,
  N_("%s plug-in can't handle layer offsets, size or opacity"),
  { N_("Merge Visible Layers"), NULL },
  0
};

static ExportAction export_action_animate_or_merge =
{
  NULL,
  export_merge,
  N_("%s plug-in can only handle layers as animation frames"),
  { N_("Save as Animation"), N_("Merge Visible Layers") },
  0
};

static ExportAction export_action_animate_or_flatten =
{
  NULL,
  export_flatten,
  N_("%s plug-in can only handle layers as animation frames"),
  { N_("Save as Animation"), N_("Flatten Image") },
  0
};

static ExportAction export_action_merge_or_flatten =
{
  export_flatten,
  export_merge,
  N_("%s plug-in can't handle layers"),
  { N_("Flatten Image"), N_("Merge Visible Layers") },
  1
};

static ExportAction export_action_flatten =
{
  export_flatten,
  NULL,
  N_("%s plug-in can't handle transparency"),
  { N_("Flatten Image"), NULL },
  0
};

static ExportAction export_action_merge_layer_effects =
{
  export_merge_layer_effects,
  NULL,
  N_("%s plug-in can't handle layer effects"),
  { N_("Merge Layer Effects"), NULL },
  0
};

static ExportAction export_action_remove_alpha =
{
  export_remove_alpha,
  NULL,
  N_("%s plug-in can't handle transparent layers"),
  { N_("Flatten Image"), NULL },
  0
};

static ExportAction export_action_apply_masks =
{
  export_apply_masks,
  NULL,
  N_("%s plug-in can't handle layer masks"),
  { N_("Apply Layer Masks"), NULL },
  0
};

static ExportAction export_action_convert_rgb =
{
  export_convert_rgb,
  NULL,
  N_("%s plug-in can only handle RGB images"),
  { N_("Convert to RGB"), NULL },
  0
};

static ExportAction export_action_convert_grayscale =
{
  export_convert_grayscale,
  NULL,
  N_("%s plug-in can only handle grayscale images"),
  { N_("Convert to Grayscale"), NULL },
  0
};

static ExportAction export_action_convert_indexed =
{
  export_convert_indexed,
  NULL,
  N_("%s plug-in can only handle indexed images"),
  { N_("Convert to Indexed using default settings\n"
       "(Do it manually to tune the result)"), NULL },
  0
};

static ExportAction export_action_convert_bitmap =
{
  export_convert_bitmap,
  NULL,
  N_("%s plug-in can only handle bitmap (two color) indexed images"),
  { N_("Convert to Indexed using bitmap default settings\n"
       "(Do it manually to tune the result)"), NULL },
  0
};

static ExportAction export_action_convert_rgb_or_grayscale =
{
  export_convert_rgb,
  export_convert_grayscale,
  N_("%s plug-in can only handle RGB or grayscale images"),
  { N_("Convert to RGB"), N_("Convert to Grayscale")},
  0
};

static ExportAction export_action_convert_rgb_or_indexed =
{
  export_convert_rgb,
  export_convert_indexed,
  N_("%s plug-in can only handle RGB or indexed images"),
  { N_("Convert to RGB"), N_("Convert to Indexed using default settings\n"
                             "(Do it manually to tune the result)")},
  0
};

static ExportAction export_action_convert_indexed_or_grayscale =
{
  export_convert_indexed,
  export_convert_grayscale,
  N_("%s plug-in can only handle grayscale or indexed images"),
  { N_("Convert to Indexed using default settings\n"
       "(Do it manually to tune the result)"),
    N_("Convert to Grayscale") },
  0
};

static ExportAction export_action_add_alpha =
{
  export_add_alpha,
  NULL,
  N_("%s plug-in needs an alpha channel"),
  { N_("Add Alpha Channel"), NULL},
  0
};

static ExportAction export_action_crop_or_resize =
{
  export_crop_image,
  export_resize_image,
  N_("%s plug-in needs to crop the layers to the image bounds"),
  { N_("Crop Layers"), N_("Resize Image to Layers")},
  0
};


static ExportFunc
export_action_get_func (const ExportAction *action)
{
  if (action->choice == 0 && action->default_action)
    {
      return action->default_action;
    }

  if (action->choice == 1 && action->alt_action)
    {
      return action->alt_action;
    }

  return export_void;
}

static void
export_action_perform (const ExportAction *action,
                       GimpImage          *image,
                       GList             **drawables)
{
  export_action_get_func (action) (image, drawables);
}

/**
 * gimp_export_options_get_image:
 * @options: (transfer none): The #GimpExportOptions object.
 * @image: (inout) (transfer none): the image.
 *
 * Takes an image to be exported, possibly creating a temporary copy
 * modified according to export settings in @options (such as the
 * capabilities of the export format).
 *
 * If necessary, a copy is created, converted and modified, @image
 * changed to point to the new image and the procedure returns
 * [enum@Gimp.ExportReturn.EXPORT].
 * In this case, you must take care of deleting the created image using
 * [method@Image.delete] once the image has been exported, unless you
 * were planning to display it with [ctor@Display.new], or you will leak
 * memory.
 *
 * If [enum@Gimp.ExportReturn.IGNORE] is returned, then @image is still the
 * original image. You should neither modify it, nor should you delete
 * it in the end. If you wish to temporarily modify the image before
 * export anyway, call [method@Image.duplicate] when
 * [enum@Gimp.ExportReturn.IGNORE] was returned.
 *
 * Returns: An enum of #GimpExportReturn.
 *
 * Since: 3.0
 **/
GimpExportReturn
gimp_export_options_get_image (GimpExportOptions  *options,
                               GimpImage         **image)
{
  GSList                 *actions = NULL;
  GimpImageBaseType       type;
  GList                  *layers;
  gint                    n_layers;
  GList                  *iter;
  GimpExportCapabilities  capabilities         = 0;
  gboolean                added_flatten        = FALSE;
  gboolean                has_layer_masks      = FALSE;
  gboolean                background_has_alpha = TRUE;
  GimpExportReturn        retval               = GIMP_EXPORT_IGNORE;

  g_return_val_if_fail (image && gimp_image_is_valid (*image), GIMP_EXPORT_IGNORE);
  g_return_val_if_fail (GIMP_IS_EXPORT_OPTIONS (options), GIMP_EXPORT_IGNORE);

  /* Get capabilities from ExportOptions */
  g_object_get (options, "capabilities", &capabilities, NULL);

  g_return_val_if_fail (capabilities & (GIMP_EXPORT_CAN_HANDLE_RGB |
                                        GIMP_EXPORT_CAN_HANDLE_GRAY |
                                        GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                        GIMP_EXPORT_CAN_HANDLE_BITMAP),
                        GIMP_EXPORT_IGNORE);


  /* do some sanity checks */
  if (capabilities & GIMP_EXPORT_NEEDS_ALPHA)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_ALPHA;

  if (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS;

  if (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS;

  /* Merge down layer effects for non-project file formats */
  if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYER_EFFECTS))
    actions = g_slist_prepend (actions, &export_action_merge_layer_effects);

  /* check alpha and layer masks */
  layers   = gimp_image_list_layers (*image);
  n_layers = g_list_length (layers);

  if (n_layers < 1)
    {
      g_list_free (layers);
      return FALSE;
    }

  for (iter = layers; iter; iter = iter->next)
    {
      GimpLayer *layer = GIMP_LAYER (iter->data);

      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
        {
          if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_ALPHA))
            {
              if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS))
                {
                  actions = g_slist_prepend (actions, &export_action_flatten);
                  added_flatten = TRUE;
                  break;
                }
              else
                {
                  actions = g_slist_prepend (actions, &export_action_remove_alpha);
                  break;
                }
            }
        }
      else
        {
          /*  If this is the last layer, it's visible and has no alpha
           *  channel, then the image has a "flat" background
           */
          if (iter->next == NULL && gimp_item_get_visible (GIMP_ITEM (layer)))
            background_has_alpha = FALSE;

          if (capabilities & GIMP_EXPORT_NEEDS_ALPHA)
            {
              actions = g_slist_prepend (actions, &export_action_add_alpha);
              break;
            }
        }
    }

  if (! added_flatten)
    {
      for (iter = layers; iter; iter = iter->next)
        {
          if (gimp_layer_get_mask (iter->data))
            has_layer_masks = TRUE;
        }
    }

  if (! added_flatten)
    {
      GimpLayer *layer = GIMP_LAYER (layers->data);
      GList     *children;

      children = gimp_item_list_children (GIMP_ITEM (layer));

      if ((capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS) &&
          (capabilities & GIMP_EXPORT_NEEDS_CROP))
        {
          GeglRectangle image_bounds;
          gboolean      needs_crop = FALSE;

          image_bounds.x      = 0;
          image_bounds.y      = 0;
          image_bounds.width  = gimp_image_get_width  (*image);
          image_bounds.height = gimp_image_get_height (*image);

          for (iter = layers; iter; iter = iter->next)
            {
              GimpDrawable  *drawable = iter->data;
              GeglRectangle  layer_bounds;

              gimp_drawable_get_offsets (drawable,
                                     &layer_bounds.x, &layer_bounds.y);

              layer_bounds.width  = gimp_drawable_get_width  (drawable);
              layer_bounds.height = gimp_drawable_get_height (drawable);

              if (! gegl_rectangle_contains (&image_bounds, &layer_bounds))
                {
                  needs_crop = TRUE;

                  break;
                }
            }

          if (needs_crop)
            {
              actions = g_slist_prepend (actions,
                                         &export_action_crop_or_resize);
            }
        }

      /* check if layer size != canvas size, opacity != 100%, or offsets != 0 */
      if (g_list_length (layers) == 1       &&
          ! children                        &&
          ! (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS))
        {
          GimpDrawable *drawable = layers->data;
          gint          offset_x;
          gint          offset_y;

          gimp_drawable_get_offsets (drawable, &offset_x, &offset_y);

          if ((gimp_layer_get_opacity (GIMP_LAYER (drawable)) < 100.0) ||
              (gimp_image_get_width (*image) !=
               gimp_drawable_get_width (drawable))            ||
              (gimp_image_get_height (*image) !=
               gimp_drawable_get_height (drawable))           ||
              offset_x || offset_y)
            {
              if (capabilities & GIMP_EXPORT_CAN_HANDLE_ALPHA)
                {
                  actions = g_slist_prepend (actions,
                                             &export_action_merge_single);
                }
              else
                {
                  actions = g_slist_prepend (actions,
                                             &export_action_flatten);
                }
            }
        }
      /* check multiple layers */
      else if (layers && layers->next != NULL)
        {
          if (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION)
            {
              if (background_has_alpha ||
                  capabilities & GIMP_EXPORT_NEEDS_ALPHA)
                actions = g_slist_prepend (actions,
                                           &export_action_animate_or_merge);
              else
                actions = g_slist_prepend (actions,
                                           &export_action_animate_or_flatten);
            }
          else if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS))
            {
              if (capabilities & GIMP_EXPORT_NEEDS_ALPHA)
                actions = g_slist_prepend (actions,
                                           &export_action_merge);
              else
                actions = g_slist_prepend (actions,
                                           &export_action_merge_or_flatten);
            }
        }
      /* check for a single toplevel layer group */
      else if (children)
        {
          if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYERS))
            {
              if (capabilities & GIMP_EXPORT_NEEDS_ALPHA)
                actions = g_slist_prepend (actions,
                                           &export_action_merge);
              else
                actions = g_slist_prepend (actions,
                                           &export_action_merge_or_flatten);
            }
        }

      g_list_free (children);

      /* check layer masks */
      if (has_layer_masks &&
          ! (capabilities & GIMP_EXPORT_CAN_HANDLE_LAYER_MASKS))
        actions = g_slist_prepend (actions, &export_action_apply_masks);
    }

  g_list_free (layers);

  /* check the image type */
  type = gimp_image_get_base_type (*image);
  switch (type)
    {
    case GIMP_RGB:
      if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_RGB))
        {
          if ((capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED) &&
              (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY))
            actions = g_slist_prepend (actions,
                                       &export_action_convert_indexed_or_grayscale);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_indexed);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_grayscale);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_BITMAP)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_bitmap);
        }
      break;

    case GIMP_GRAY:
      if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY))
        {
          if ((capabilities & GIMP_EXPORT_CAN_HANDLE_RGB) &&
              (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED))
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb_or_indexed);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_RGB)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_indexed);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_BITMAP)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_bitmap);
        }
      break;

    case GIMP_INDEXED:
      if (! (capabilities & GIMP_EXPORT_CAN_HANDLE_INDEXED))
        {
          if ((capabilities & GIMP_EXPORT_CAN_HANDLE_RGB) &&
              (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY))
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb_or_grayscale);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_RGB)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_rgb);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_GRAY)
            actions = g_slist_prepend (actions,
                                       &export_action_convert_grayscale);
          else if (capabilities & GIMP_EXPORT_CAN_HANDLE_BITMAP)
            {
              gint n_colors;

              g_free (gimp_image_get_colormap (*image, NULL, &n_colors));

              if (n_colors > 2)
                actions = g_slist_prepend (actions,
                                           &export_action_convert_bitmap);
            }
        }
      break;
    }

  if (actions)
    {
      actions = g_slist_reverse (actions);

      retval = GIMP_EXPORT_EXPORT;
    }
  else
    {
      retval = GIMP_EXPORT_IGNORE;
    }

  if (retval == GIMP_EXPORT_EXPORT)
    {
      GSList *list;
      GList  *drawables_in;
      GList  *drawables_out;

      *image = gimp_image_duplicate (*image);
      drawables_in  = gimp_image_list_selected_layers (*image);
      drawables_out = drawables_in;

      gimp_image_undo_disable (*image);

      for (list = actions; list; list = list->next)
        {
          export_action_perform (list->data, *image, &drawables_out);

          if (drawables_in != drawables_out)
            {
              g_list_free (drawables_in);
              drawables_in = drawables_out;
            }
        }

      g_list_free (drawables_out);
    }

  g_slist_free (actions);

  return retval;
}
