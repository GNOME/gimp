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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-item-list.h"
#include "core/gimpimage-transform.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitem-linked.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpprogress.h"
#include "core/gimp-transform-resize.h"

#include "vectors/gimpvectors.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "widgets/gimpmessagedialog.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimptoolcontrol.h"
#include "gimptools-utils.h"
#include "gimptransformoptions.h"
#include "gimptransformtool.h"

#include "gimp-intl.h"


/* the minimal ratio between the transformed item size and the image size,
 * above which confirmation is required.
 */
#define MIN_CONFIRMATION_RATIO 10


/*  local function prototypes  */

static void                     gimp_transform_tool_control            (GimpTool           *tool,
                                                                        GimpToolAction      action,
                                                                        GimpDisplay        *display);

static gchar                  * gimp_transform_tool_real_get_undo_desc (GimpTransformTool  *tr_tool);
static GimpTransformDirection   gimp_transform_tool_real_get_direction (GimpTransformTool  *tr_tool);
static GeglBuffer             * gimp_transform_tool_real_transform     (GimpTransformTool  *tr_tool,
                                                                        GimpObject         *object,
                                                                        GeglBuffer         *orig_buffer,
                                                                        gint                orig_offset_x,
                                                                        gint                orig_offset_y,
                                                                        GimpColorProfile  **buffer_profile,
                                                                        gint               *new_offset_x,
                                                                        gint               *new_offset_y);

static void                     gimp_transform_tool_halt               (GimpTransformTool  *tr_tool);

static gboolean                 gimp_transform_tool_confirm            (GimpTransformTool  *tr_tool,
                                                                        GimpDisplay        *display);


G_DEFINE_TYPE (GimpTransformTool, gimp_transform_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_transform_tool_parent_class


/*  private functions  */


static void
gimp_transform_tool_class_init (GimpTransformToolClass *klass)
{
  GimpToolClass *tool_class = GIMP_TOOL_CLASS (klass);

  tool_class->control  = gimp_transform_tool_control;

  klass->recalc_matrix = NULL;
  klass->get_undo_desc = gimp_transform_tool_real_get_undo_desc;
  klass->get_direction = gimp_transform_tool_real_get_direction;
  klass->transform     = gimp_transform_tool_real_transform;

  klass->undo_desc     = _("Transform");
  klass->progress_text = _("Transforming");
}

static void
gimp_transform_tool_init (GimpTransformTool *tr_tool)
{
  gimp_matrix3_identity (&tr_tool->transform);
  tr_tool->transform_valid = TRUE;

  tr_tool->restore_type = FALSE;
}

static void
gimp_transform_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_transform_tool_halt (tr_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gchar *
gimp_transform_tool_real_get_undo_desc (GimpTransformTool *tr_tool)
{
  return g_strdup (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->undo_desc);
}

static GimpTransformDirection
gimp_transform_tool_real_get_direction (GimpTransformTool *tr_tool)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  return options->direction;
}

static GeglBuffer *
gimp_transform_tool_real_transform (GimpTransformTool *tr_tool,
                                    GimpObject        *object,
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
  GimpTransformDirection  direction;
  GimpProgress           *progress;

  direction = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_direction (tr_tool);

  progress = gimp_progress_start (GIMP_PROGRESS (tool), FALSE,
                                  "%s", klass->progress_text);

  if (orig_buffer)
    {
      /*  this happens when transforming a selection cut out of a
       *  normal drawable
       */

      g_return_val_if_fail (GIMP_IS_DRAWABLE (object), NULL);

      ret = gimp_drawable_transform_buffer_affine (GIMP_DRAWABLE (object),
                                                   context,
                                                   orig_buffer,
                                                   orig_offset_x,
                                                   orig_offset_y,
                                                   &tr_tool->transform,
                                                   direction,
                                                   options->interpolation,
                                                   clip,
                                                   buffer_profile,
                                                   new_offset_x,
                                                   new_offset_y,
                                                   progress);
    }
  else if (GIMP_IS_ITEM (object))
    {
      /*  this happens for entire drawables, paths and layer groups  */

      GimpItem *item = GIMP_ITEM (object);

      if (gimp_item_get_linked (item))
        {
          gimp_item_linked_transform (item, context,
                                      &tr_tool->transform,
                                      direction,
                                      options->interpolation,
                                      clip,
                                      progress);
        }
      else
        {
          clip = gimp_item_get_clip (item, clip);

          gimp_item_transform (item, context,
                               &tr_tool->transform,
                               direction,
                               options->interpolation,
                               clip,
                               progress);
        }
    }
  else
    {
      /*  this happens for images  */

      g_return_val_if_fail (GIMP_IS_IMAGE (object), NULL);

      gimp_image_transform (GIMP_IMAGE (object), context,
                            &tr_tool->transform,
                            direction,
                            options->interpolation,
                            clip,
                            progress);
    }

  if (progress)
    gimp_progress_end (progress);

  return ret;
}

static void
gimp_transform_tool_halt (GimpTransformTool *tr_tool)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  tr_tool->x1 = 0;
  tr_tool->y1 = 0;
  tr_tool->x2 = 0;
  tr_tool->y2 = 0;

  if (tr_tool->restore_type)
    {
      g_object_set (options,
                    "type", tr_tool->saved_type,
                    NULL);

      tr_tool->restore_type = FALSE;
    }
}

static gboolean
gimp_transform_tool_confirm (GimpTransformTool *tr_tool,
                             GimpDisplay       *display)
{
  GimpTransformOptions *options          = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpDisplayShell     *shell            = gimp_display_get_shell (display);
  GimpImage            *image            = gimp_display_get_image (display);
  GimpObject           *active_object;
  gdouble               max_ratio        = 0.0;
  GimpObject           *max_ratio_object = NULL;

  active_object = gimp_transform_tool_get_active_object (tr_tool, display);

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix)
    {
      GimpMatrix3             transform;
      GimpTransformDirection  direction;
      GeglRectangle           selection_bounds;
      gboolean                selection_empty = TRUE;
      GList                  *objects;
      GList                  *iter;

      transform = tr_tool->transform;
      direction = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_direction (
        tr_tool);

      if (direction == GIMP_TRANSFORM_BACKWARD)
        gimp_matrix3_invert (&transform);

      if (options->type == GIMP_TRANSFORM_TYPE_LAYER &&
          ! gimp_viewable_get_children (GIMP_VIEWABLE (active_object)))
        {
          selection_empty = ! gimp_item_bounds (
            GIMP_ITEM (gimp_image_get_mask (image)),
            &selection_bounds.x,     &selection_bounds.y,
            &selection_bounds.width, &selection_bounds.height);
        }

      if (selection_empty              &&
          GIMP_IS_ITEM (active_object) &&
          gimp_item_get_linked (GIMP_ITEM (active_object)))
        {
          objects = gimp_image_item_list_get_list (image,
                                                   GIMP_ITEM_TYPE_ALL,
                                                   GIMP_ITEM_SET_LINKED);
        }
      else
        {
          objects = g_list_append (NULL, active_object);
        }

      if (options->type == GIMP_TRANSFORM_TYPE_IMAGE)
        {
          objects = g_list_concat (
            objects,
            gimp_image_item_list_get_list (image,
                                           GIMP_ITEM_TYPE_ALL,
                                           GIMP_ITEM_SET_ALL));
        }

      for (iter = objects; iter; iter = g_list_next (iter))
        {
          GimpObject          *object = iter->data;
          GimpTransformResize  clip   = options->clip;
          GeglRectangle        orig_bounds;
          GeglRectangle        new_bounds;
          gdouble              ratio  = 0.0;

          if (GIMP_IS_DRAWABLE (object))
            {
              if (selection_empty)
                {
                  GimpItem *item = GIMP_ITEM (object);

                  gimp_item_get_offset (item, &orig_bounds.x, &orig_bounds.y);

                  orig_bounds.width  = gimp_item_get_width  (item);
                  orig_bounds.height = gimp_item_get_height (item);

                  clip = gimp_item_get_clip (item, clip);
                }
              else
                {
                  orig_bounds = selection_bounds;
                }
            }
          else if (GIMP_IS_ITEM (object))
            {
              GimpItem *item = GIMP_ITEM (object);

              gimp_item_bounds (item,
                                &orig_bounds.x,     &orig_bounds.y,
                                &orig_bounds.width, &orig_bounds.height);

              clip = gimp_item_get_clip (item, clip);
            }
          else
            {
              GimpImage *image;

              g_return_val_if_fail (GIMP_IS_IMAGE (object), FALSE);

              image = GIMP_IMAGE (object);

              orig_bounds.x      = 0;
              orig_bounds.y      = 0;
              orig_bounds.width  = gimp_image_get_width  (image);
              orig_bounds.height = gimp_image_get_height (image);
            }

          gimp_transform_resize_boundary (&transform, clip,

                                          orig_bounds.x,
                                          orig_bounds.y,
                                          orig_bounds.x + orig_bounds.width,
                                          orig_bounds.y + orig_bounds.height,

                                          &new_bounds.x,
                                          &new_bounds.y,
                                          &new_bounds.width,
                                          &new_bounds.height);

          new_bounds.width  -= new_bounds.x;
          new_bounds.height -= new_bounds.y;

          if (new_bounds.width > orig_bounds.width)
            {
              ratio = MAX (ratio,
                           (gdouble) new_bounds.width /
                           (gdouble) gimp_image_get_width (image));
            }

          if (new_bounds.height > orig_bounds.height)
            {
              ratio = MAX (ratio,
                           (gdouble) new_bounds.height /
                           (gdouble) gimp_image_get_height (image));
            }

          if (ratio > max_ratio)
            {
              max_ratio        = ratio;
              max_ratio_object = object;
            }
        }

      g_list_free (objects);
    }

  if (max_ratio > MIN_CONFIRMATION_RATIO)
    {
      GtkWidget *dialog;
      gint       response;

      dialog = gimp_message_dialog_new (_("Confirm Transformation"),
                                        GIMP_ICON_DIALOG_WARNING,
                                        GTK_WIDGET (shell),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        gimp_standard_help_func, NULL,

                                        _("_Cancel"),    GTK_RESPONSE_CANCEL,
                                        _("_Transform"), GTK_RESPONSE_OK,

                                        NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      if (GIMP_IS_ITEM (max_ratio_object))
        {
          gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                             _("Transformation creates "
                                               "a very large item."));

          gimp_message_box_set_text (
            GIMP_MESSAGE_DIALOG (dialog)->box,
            _("Applying the transformation will result "
              "in an item that is over %g times larger "
              "than the image."),
              floor (max_ratio));
        }
      else
        {
          gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                             _("Transformation creates "
                                               "a very large image."));

          gimp_message_box_set_text (
            GIMP_MESSAGE_DIALOG (dialog)->box,
            _("Applying the transformation will enlarge "
              "the image by a factor of %g."),
              floor (max_ratio));
        }

      response = gtk_dialog_run (GTK_DIALOG (dialog));

      gtk_widget_destroy (dialog);

      if (response != GTK_RESPONSE_OK)
        return FALSE;
    }

  return TRUE;
}


/*  public functions  */


gboolean
gimp_transform_tool_bounds (GimpTransformTool *tr_tool,
                            GimpDisplay       *display)
{
  GimpTransformOptions *options;
  GimpDisplayShell     *shell;
  GimpImage            *image;
  gboolean              non_empty = TRUE;

  g_return_val_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool), FALSE);

  options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  image   = gimp_display_get_image (display);
  shell   = gimp_display_get_shell (display);

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

    case GIMP_TRANSFORM_TYPE_IMAGE:
      if (! shell->show_all)
        {
          tr_tool->x1 = 0;
          tr_tool->y1 = 0;
          tr_tool->x2 = gimp_image_get_width  (image);
          tr_tool->y2 = gimp_image_get_height (image);
        }
      else
        {
          GeglRectangle bounding_box;

          bounding_box = gimp_display_shell_get_bounding_box (shell);

          tr_tool->x1 = bounding_box.x;
          tr_tool->y1 = bounding_box.y;
          tr_tool->x2 = bounding_box.x + bounding_box.width;
          tr_tool->y2 = bounding_box.y + bounding_box.height;
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

  if (tr_tool->x1 == tr_tool->x2 && tr_tool->y1 == tr_tool->y2)
    gimp_transform_tool_bounds (tr_tool, display);

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix)
    GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix (tr_tool);
}

GimpObject *
gimp_transform_tool_get_active_object (GimpTransformTool  *tr_tool,
                                       GimpDisplay        *display)
{
  GimpTransformOptions *options;
  GimpImage            *image;
  GimpObject           *object = NULL;

  g_return_val_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  image = gimp_display_get_image (display);

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  if (tr_tool->object)
    return tr_tool->object;

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      object = GIMP_OBJECT (gimp_image_get_active_drawable (image));
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      object = GIMP_OBJECT (gimp_image_get_mask (image));

      if (gimp_channel_is_empty (GIMP_CHANNEL (object)))
        object = NULL;
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      object = GIMP_OBJECT (gimp_image_get_active_vectors (image));
      break;

    case GIMP_TRANSFORM_TYPE_IMAGE:
      object = GIMP_OBJECT (image);
      break;
    }

  return object;
}

GimpObject *
gimp_transform_tool_check_active_object (GimpTransformTool  *tr_tool,
                                         GimpDisplay        *display,
                                         GError            **error)
{
  GimpTransformOptions *options;
  GimpObject           *object;
  const gchar          *null_message   = NULL;
  const gchar          *locked_message = NULL;
  GimpGuiConfig        *config         = GIMP_GUI_CONFIG (display->gimp->config);

  g_return_val_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  object = gimp_transform_tool_get_active_object (tr_tool, display);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      null_message = _("There is no layer to transform.");

      if (object)
        {
          GimpItem *item = GIMP_ITEM (object);

          if (gimp_item_is_content_locked (item))
            locked_message = _("The active layer's pixels are locked.");
          else if (gimp_item_is_position_locked (item))
            locked_message = _("The active layer's position and size are locked.");

          if (! gimp_item_is_visible (item) &&
              ! config->edit_non_visible &&
              object != tr_tool->object) /* see bug #759194 */
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

      if (object)
        {
          GimpItem *item = GIMP_ITEM (object);

          /* cannot happen, so don't translate these messages */
          if (gimp_item_is_content_locked (item))
            locked_message = "The selection's pixels are locked.";
          else if (gimp_item_is_position_locked (item))
            locked_message = "The selection's position and size are locked.";
        }
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      null_message = _("There is no path to transform.");

      if (object)
        {
          GimpItem *item = GIMP_ITEM (object);

          if (gimp_item_is_content_locked (item))
            locked_message = _("The active path's strokes are locked.");
          else if (gimp_item_is_position_locked (item))
            locked_message = _("The active path's position is locked.");
          else if (! gimp_vectors_get_n_strokes (GIMP_VECTORS (item)))
            locked_message = _("The active path has no strokes.");
        }
      break;

    case GIMP_TRANSFORM_TYPE_IMAGE:
      /* cannot happen, so don't translate this message */
      null_message = "There is no image to transform.";
      break;
    }

  if (! object)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED, null_message);
      if (error)
        gimp_widget_blink (options->type_box);
      return NULL;
    }

  if (locked_message)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED, locked_message);
      if (error)
        gimp_tools_blink_lock_box (display->gimp, GIMP_ITEM (object));
      return NULL;
    }

  return object;
}

gboolean
gimp_transform_tool_transform (GimpTransformTool *tr_tool,
                               GimpDisplay       *display)
{
  GimpTool             *tool;
  GimpTransformOptions *options;
  GimpContext          *context;
  GimpImage            *image;
  GimpObject           *active_object;
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

  active_object = gimp_transform_tool_check_active_object (tr_tool, display,
                                                           &error);

  if (! active_object)
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

  if (! gimp_transform_tool_confirm (tr_tool, display))
    return FALSE;

  gimp_set_busy (display->gimp);

  /*  We're going to dirty this image, but we want to keep the tool around  */
  gimp_tool_control_push_preserve (tool->control, TRUE);

  undo_desc = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_undo_desc (tr_tool);
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM, undo_desc);
  g_free (undo_desc);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      if (! gimp_viewable_get_children (GIMP_VIEWABLE (active_object)) &&
          ! gimp_channel_is_empty (gimp_image_get_mask (image)))
        {
          orig_buffer = gimp_drawable_transform_cut (
            GIMP_DRAWABLE (active_object),
            context,
            &orig_offset_x,
            &orig_offset_y,
            &new_layer);
        }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
    case GIMP_TRANSFORM_TYPE_PATH:
    case GIMP_TRANSFORM_TYPE_IMAGE:
      break;
    }

  /*  Send the request for the transformation to the tool...
   */
  new_buffer = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->transform (
    tr_tool,
    active_object,
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
          gimp_drawable_transform_paste (GIMP_DRAWABLE (active_object),
                                         new_buffer, buffer_profile,
                                         new_offset_x, new_offset_y,
                                         new_layer);
          g_object_unref (new_buffer);
        }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
    case GIMP_TRANSFORM_TYPE_PATH:
    case GIMP_TRANSFORM_TYPE_IMAGE:
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

void
gimp_transform_tool_set_type (GimpTransformTool *tr_tool,
                              GimpTransformType  type)
{
  GimpTransformOptions *options;

  g_return_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool));

  options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  if (! tr_tool->restore_type)
    tr_tool->saved_type = options->type;

  tr_tool->restore_type = FALSE;

  g_object_set (options,
                "type", type,
                NULL);

  tr_tool->restore_type = TRUE;
}
