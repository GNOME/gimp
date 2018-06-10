/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitem-linked.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpprogress.h"

#include "vectors/gimpvectors.h"

#include "display/gimpdisplay.h"

#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"
#include "gimptransformtool.h"

#include "gimp-intl.h"


static GeglBuffer * gimp_transform_tool_real_transform (GimpTransformTool  *tr_tool,
                                                        GimpItem           *item,
                                                        GeglBuffer         *orig_buffer,
                                                        gint                orig_offset_x,
                                                        gint                orig_offset_y,
                                                        GimpColorProfile  **buffer_profile,
                                                        gint               *new_offset_x,
                                                        gint               *new_offset_y);


G_DEFINE_TYPE (GimpTransformTool, gimp_transform_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_transform_tool_parent_class


static void
gimp_transform_tool_class_init (GimpTransformToolClass *klass)
{
  klass->recalc_matrix = NULL;
  klass->get_undo_desc = NULL;
  klass->transform     = gimp_transform_tool_real_transform;

  klass->progress_text = _("Transforming");
}

static void
gimp_transform_tool_init (GimpTransformTool *tr_tool)
{
  gimp_matrix3_identity (&tr_tool->transform);
  tr_tool->transform_valid = TRUE;
}

static GeglBuffer *
gimp_transform_tool_real_transform (GimpTransformTool *tr_tool,
                                    GimpItem          *active_item,
                                    GeglBuffer        *orig_buffer,
                                    gint               orig_offset_x,
                                    gint               orig_offset_y,
                                    GimpColorProfile **buffer_profile,
                                    gint              *new_offset_x,
                                    gint              *new_offset_y)
{
  GimpTransformToolClass *klass   = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool);
  GimpTool               *tool    = GIMP_TOOL (tr_tool);
  GimpTransformOptions   *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpContext            *context = GIMP_CONTEXT (options);
  GeglBuffer             *ret     = NULL;
  GimpTransformResize     clip    = options->clip;
  GimpProgress           *progress;

  progress = gimp_progress_start (GIMP_PROGRESS (tool), FALSE,
                                  "%s", klass->progress_text);

  if (orig_buffer)
    {
      /*  this happens when transforming a selection cut out of a
       *  normal drawable, or the selection
       */

      /*  always clip the selection and unfloated channels
       *  so they keep their size
       */
      if (GIMP_IS_CHANNEL (active_item) &&
          ! babl_format_has_alpha (gegl_buffer_get_format (orig_buffer)))
        clip = GIMP_TRANSFORM_RESIZE_CLIP;

      ret = gimp_drawable_transform_buffer_affine (GIMP_DRAWABLE (active_item),
                                                   context,
                                                   orig_buffer,
                                                   orig_offset_x,
                                                   orig_offset_y,
                                                   &tr_tool->transform,
                                                   options->direction,
                                                   options->interpolation,
                                                   clip,
                                                   buffer_profile,
                                                   new_offset_x,
                                                   new_offset_y,
                                                   progress);
    }
  else
    {
      /*  this happens for entire drawables, paths and layer groups  */

      if (gimp_item_get_linked (active_item))
        {
          gimp_item_linked_transform (active_item, context,
                                      &tr_tool->transform,
                                      options->direction,
                                      options->interpolation,
                                      clip,
                                      progress);
        }
      else
        {
          /*  always clip layer masks so they keep their size
           */
          if (GIMP_IS_CHANNEL (active_item))
            clip = GIMP_TRANSFORM_RESIZE_CLIP;

          gimp_item_transform (active_item, context,
                               &tr_tool->transform,
                               options->direction,
                               options->interpolation,
                               clip,
                               progress);
        }
    }

  if (progress)
    gimp_progress_end (progress);

  return ret;
}

gboolean
gimp_transform_tool_bounds (GimpTransformTool *tr_tool,
                            GimpDisplay       *display)
{
  GimpTransformOptions *options;
  GimpImage            *image;
  gboolean              non_empty = TRUE;

  g_return_val_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool), FALSE);

  options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  image   = gimp_display_get_image (display);

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      {
        GimpDrawable *drawable;
        gint          offset_x;
        gint          offset_y;
        gint          x, y;
        gint          width, height;

        drawable = gimp_image_get_active_drawable (image);

        gimp_item_get_offset (GIMP_ITEM (drawable), &offset_x, &offset_y);

        non_empty = gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                              &x, &y, &width, &height);
        tr_tool->x1 = x + offset_x;
        tr_tool->y1 = y + offset_y;
        tr_tool->x2 = x + width  + offset_x;
        tr_tool->y2 = y + height + offset_y;
      }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      {
        gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                          &tr_tool->x1, &tr_tool->y1,
                          &tr_tool->x2, &tr_tool->y2);
        tr_tool->x2 += tr_tool->x1;
        tr_tool->y2 += tr_tool->y1;
      }
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      {
        GimpChannel *selection = gimp_image_get_mask (image);

        /* if selection is not empty, use its bounds to perform the
         * transformation of the path
         */

        if (! gimp_channel_is_empty (selection))
          {
            gimp_item_bounds (GIMP_ITEM (selection),
                              &tr_tool->x1, &tr_tool->y1,
                              &tr_tool->x2, &tr_tool->y2);
          }
        else
          {
            /* without selection, test the emptiness of the path bounds :
             * if empty, use the canvas bounds
             * else use the path bounds
             */

            if (! gimp_item_bounds (GIMP_ITEM (gimp_image_get_active_vectors (image)),
                                    &tr_tool->x1, &tr_tool->y1,
                                    &tr_tool->x2, &tr_tool->y2))
              {
                tr_tool->x1 = 0;
                tr_tool->y1 = 0;
                tr_tool->x2 = gimp_image_get_width (image);
                tr_tool->y2 = gimp_image_get_height (image);
              }
          }

        tr_tool->x2 += tr_tool->x1;
        tr_tool->y2 += tr_tool->y1;
      }

      break;
    }

  return non_empty;
}

void
gimp_transform_tool_recalc_matrix (GimpTransformTool *tr_tool,
                                   GimpDisplay       *display)
{
  g_return_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  gimp_transform_tool_bounds (tr_tool, display);

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix)
    GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix (tr_tool);
}

GimpItem *
gimp_transform_tool_get_active_item (GimpTransformTool  *tr_tool,
                                     GimpDisplay        *display)
{
  GimpTransformOptions *options;
  GimpImage            *image;
  GimpItem             *item = NULL;

  g_return_val_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  image = gimp_display_get_image (display);

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      item = GIMP_ITEM (gimp_image_get_active_drawable (image));
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      item = GIMP_ITEM (gimp_image_get_mask (image));

      if (gimp_channel_is_empty (GIMP_CHANNEL (item)))
        item = NULL;
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      item = GIMP_ITEM (gimp_image_get_active_vectors (image));
      break;
    }

  return item;
}

GimpItem *
gimp_transform_tool_check_active_item (GimpTransformTool  *tr_tool,
                                       GimpDisplay        *display,
                                       GError            **error)
{
  GimpTransformOptions *options;
  GimpItem             *item;
  const gchar          *null_message   = NULL;
  const gchar          *locked_message = NULL;

  g_return_val_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  item = gimp_transform_tool_get_active_item (tr_tool, display);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      null_message = _("There is no layer to transform.");

      if (item)
        {
          if (gimp_item_is_content_locked (item))
            locked_message = _("The active layer's pixels are locked.");
          else if (gimp_item_is_position_locked (item))
            locked_message = _("The active layer's position and size are locked.");

          if (! gimp_item_is_visible (item))
            {
              g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                   _("The active layer is not visible."));
              return NULL;
            }

          if (! gimp_transform_tool_bounds (tr_tool, display))
            {
              g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                   _("The selection does not intersect with the layer."));
              return NULL;
            }
        }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      null_message = _("There is no selection to transform.");

      if (item)
        {
          /* cannot happen, so don't translate these messages */
          if (gimp_item_is_content_locked (item))
            locked_message = "The selection's pixels are locked.";
          else if (gimp_item_is_position_locked (item))
            locked_message = "The selection's position and size are locked.";
        }
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      null_message = _("There is no path to transform.");

      if (item)
        {
          if (gimp_item_is_content_locked (item))
            locked_message = _("The active path's strokes are locked.");
          else if (gimp_item_is_position_locked (item))
            locked_message = _("The active path's position is locked.");
          else if (! gimp_vectors_get_n_strokes (GIMP_VECTORS (item)))
            locked_message = _("The active path has no strokes.");
        }
      break;
    }

  if (! item)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED, null_message);
      return NULL;
    }

  if (locked_message)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED, locked_message);
      return NULL;
    }

  return item;
}

gboolean
gimp_transform_tool_transform (GimpTransformTool *tr_tool,
                               GimpDisplay       *display)
{
  GimpTool             *tool;
  GimpTransformOptions *options;
  GimpContext          *context;
  GimpImage            *image;
  GimpItem             *active_item;
  GeglBuffer           *orig_buffer   = NULL;
  gint                  orig_offset_x = 0;
  gint                  orig_offset_y = 0;
  GeglBuffer           *new_buffer;
  gint                  new_offset_x;
  gint                  new_offset_y;
  GimpColorProfile     *buffer_profile;
  gchar                *undo_desc     = NULL;
  gboolean              new_layer     = FALSE;
  GError               *error         = NULL;

  g_return_val_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  tool    = GIMP_TOOL (tr_tool);
  options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  context = GIMP_CONTEXT (options);
  image   = gimp_display_get_image (display);

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  active_item = gimp_transform_tool_check_active_item (tr_tool, display,
                                                       &error);

  if (! active_item)
    {
      gimp_tool_message_literal (tool, display, error->message);
      g_clear_error (&error);
      return FALSE;
    }

  gimp_transform_tool_recalc_matrix (tr_tool, display);

  if (! tr_tool->transform_valid)
    {
      gimp_tool_message_literal (tool, display,
                                 _("The current transform is invalid"));
      return FALSE;
    }

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix &&
      gimp_matrix3_is_identity (&tr_tool->transform))
    {
      /* No need to commit an identity transformation! */
      return TRUE;
    }

  gimp_set_busy (display->gimp);

  /*  We're going to dirty this image, but we want to keep the tool around  */
  gimp_tool_control_push_preserve (tool->control, TRUE);

  undo_desc = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_undo_desc (tr_tool);
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM, undo_desc);
  g_free (undo_desc);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      if (! gimp_viewable_get_children (GIMP_VIEWABLE (tool->drawable)) &&
          ! gimp_channel_is_empty (gimp_image_get_mask (image)))
        {
          orig_buffer = gimp_drawable_transform_cut (tool->drawable,
                                                     context,
                                                     &orig_offset_x,
                                                     &orig_offset_y,
                                                     &new_layer);
        }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      orig_buffer = g_object_ref (gimp_drawable_get_buffer (GIMP_DRAWABLE (active_item)));
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      break;
    }

  /*  Send the request for the transformation to the tool...
   */
  new_buffer = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->transform (tr_tool,
                                                                   active_item,
                                                                   orig_buffer,
                                                                   orig_offset_x,
                                                                   orig_offset_y,
                                                                   &buffer_profile,
                                                                   &new_offset_x,
                                                                   &new_offset_y);

  if (orig_buffer)
    g_object_unref (orig_buffer);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      if (new_buffer)
        {
          /*  paste the new transformed image to the image...also implement
           *  undo...
           */
          gimp_drawable_transform_paste (tool->drawable,
                                         new_buffer, buffer_profile,
                                         new_offset_x, new_offset_y,
                                         new_layer);
          g_object_unref (new_buffer);
        }
      break;

     case GIMP_TRANSFORM_TYPE_SELECTION:
      if (new_buffer)
        {
          gimp_channel_push_undo (GIMP_CHANNEL (active_item), NULL);

          gimp_drawable_set_buffer (GIMP_DRAWABLE (active_item),
                                    FALSE, NULL, new_buffer);
          g_object_unref (new_buffer);
        }
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      /*  Nothing to be done  */
      break;
    }

  gimp_image_undo_group_end (image);

  /*  We're done dirtying the image, and would like to be restarted if
   *  the image gets dirty while the tool exists
   */
  gimp_tool_control_pop_preserve (tool->control);

  gimp_unset_busy (display->gimp);

  gimp_image_flush (image);

  return TRUE;
}
