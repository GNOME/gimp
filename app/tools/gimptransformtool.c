/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmadrawable-transform.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-item-list.h"
#include "core/ligmaimage-transform.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"
#include "core/ligmaprogress.h"
#include "core/ligma-transform-resize.h"

#include "vectors/ligmavectors.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "widgets/ligmamessagedialog.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"
#include "ligmatransformoptions.h"
#include "ligmatransformtool.h"

#include "ligma-intl.h"


/* the minimal ratio between the transformed item size and the image size,
 * above which confirmation is required.
 */
#define MIN_CONFIRMATION_RATIO 10


/*  local function prototypes  */

static void                     ligma_transform_tool_control            (LigmaTool           *tool,
                                                                        LigmaToolAction      action,
                                                                        LigmaDisplay        *display);

static gchar                  * ligma_transform_tool_real_get_undo_desc (LigmaTransformTool  *tr_tool);
static LigmaTransformDirection   ligma_transform_tool_real_get_direction (LigmaTransformTool  *tr_tool);
static GeglBuffer             * ligma_transform_tool_real_transform     (LigmaTransformTool  *tr_tool,
                                                                        GList              *objects,
                                                                        GeglBuffer         *orig_buffer,
                                                                        gint                orig_offset_x,
                                                                        gint                orig_offset_y,
                                                                        LigmaColorProfile  **buffer_profile,
                                                                        gint               *new_offset_x,
                                                                        gint               *new_offset_y);

static void                     ligma_transform_tool_halt               (LigmaTransformTool  *tr_tool);

static gboolean                 ligma_transform_tool_confirm            (LigmaTransformTool  *tr_tool,
                                                                        LigmaDisplay        *display);


G_DEFINE_TYPE (LigmaTransformTool, ligma_transform_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_transform_tool_parent_class


/*  private functions  */


static void
ligma_transform_tool_class_init (LigmaTransformToolClass *klass)
{
  LigmaToolClass *tool_class = LIGMA_TOOL_CLASS (klass);

  tool_class->control  = ligma_transform_tool_control;

  klass->recalc_matrix = NULL;
  klass->get_undo_desc = ligma_transform_tool_real_get_undo_desc;
  klass->get_direction = ligma_transform_tool_real_get_direction;
  klass->transform     = ligma_transform_tool_real_transform;

  klass->undo_desc     = _("Transform");
  klass->progress_text = _("Transforming");
}

static void
ligma_transform_tool_init (LigmaTransformTool *tr_tool)
{
  ligma_matrix3_identity (&tr_tool->transform);
  tr_tool->transform_valid = TRUE;

  tr_tool->restore_type = FALSE;
}

static void
ligma_transform_tool_control (LigmaTool       *tool,
                             LigmaToolAction  action,
                             LigmaDisplay    *display)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_transform_tool_halt (tr_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gchar *
ligma_transform_tool_real_get_undo_desc (LigmaTransformTool *tr_tool)
{
  return g_strdup (LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool)->undo_desc);
}

static LigmaTransformDirection
ligma_transform_tool_real_get_direction (LigmaTransformTool *tr_tool)
{
  LigmaTransformOptions *options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  return options->direction;
}

static GeglBuffer *
ligma_transform_tool_real_transform (LigmaTransformTool *tr_tool,
                                    GList             *objects,
                                    GeglBuffer        *orig_buffer,
                                    gint               orig_offset_x,
                                    gint               orig_offset_y,
                                    LigmaColorProfile **buffer_profile,
                                    gint              *new_offset_x,
                                    gint              *new_offset_y)
{
  LigmaTransformToolClass *klass   = LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool);
  LigmaTool               *tool    = LIGMA_TOOL (tr_tool);
  LigmaTransformOptions   *options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tool);
  LigmaContext            *context = LIGMA_CONTEXT (options);
  GeglBuffer             *ret     = NULL;
  LigmaTransformResize     clip    = options->clip;
  LigmaTransformDirection  direction;
  LigmaProgress           *progress;

  direction = LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_direction (tr_tool);

  progress = ligma_progress_start (LIGMA_PROGRESS (tool), FALSE,
                                  "%s", klass->progress_text);

  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);

  if (orig_buffer)
    {
      /*  this happens when transforming a selection cut out of
       *  normal drawables.
       */

      ret = ligma_drawable_transform_buffer_affine (objects->data,
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
  else if (g_list_length (objects) == 1 && LIGMA_IS_IMAGE (objects->data))
    {
      /*  this happens for images  */

      ligma_image_transform (objects->data, context,
                            &tr_tool->transform,
                            direction,
                            options->interpolation,
                            clip,
                            progress);
    }
  else
    {
      GList *items;

      /*  this happens for entire drawables, paths and layer groups  */
      g_return_val_if_fail (g_list_length (objects) > 0, NULL);

      items = ligma_image_item_list_filter (g_list_copy (objects));

      ligma_image_item_list_transform (ligma_item_get_image (objects->data),
                                      items, context,
                                      &tr_tool->transform,
                                      direction,
                                      options->interpolation,
                                      clip,
                                      progress);
      g_list_free (items);
    }

  if (progress)
    ligma_progress_end (progress);

  return ret;
}

static void
ligma_transform_tool_halt (LigmaTransformTool *tr_tool)
{
  LigmaTransformOptions *options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

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
ligma_transform_tool_confirm (LigmaTransformTool *tr_tool,
                             LigmaDisplay       *display)
{
  LigmaTransformOptions *options          = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  LigmaDisplayShell     *shell            = ligma_display_get_shell (display);
  LigmaImage            *image            = ligma_display_get_image (display);
  GList                *selected_objects;
  gdouble               max_ratio        = 0.0;
  LigmaObject           *max_ratio_object = NULL;

  selected_objects = ligma_transform_tool_get_selected_objects (tr_tool, display);

  if (LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix)
    {
      LigmaMatrix3             transform;
      LigmaTransformDirection  direction;
      GeglRectangle           selection_bounds;
      gboolean                selection_empty = TRUE;
      GList                  *objects         = NULL;
      GList                  *iter;

      transform = tr_tool->transform;
      direction = LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_direction (
        tr_tool);

      if (direction == LIGMA_TRANSFORM_BACKWARD)
        ligma_matrix3_invert (&transform);

      if (options->type == LIGMA_TRANSFORM_TYPE_LAYER)
        {
          for (iter = selected_objects; iter; iter = iter->next)
            if (! ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)))
          {
            if (ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                                  &selection_bounds.x,     &selection_bounds.y,
                                  &selection_bounds.width, &selection_bounds.height))
              {
                selection_empty = FALSE;
                break;
              }
          }
        }

      if (selection_empty  &&
          selected_objects &&
          LIGMA_IS_ITEM (selected_objects->data))
        {
          objects = ligma_image_item_list_filter (g_list_copy (selected_objects));
          g_list_free (selected_objects);
        }
      else
        {
          objects = selected_objects;
        }

      if (options->type == LIGMA_TRANSFORM_TYPE_IMAGE)
        {
          objects = g_list_concat (
            objects,
            ligma_image_item_list_get_list (image,
                                           LIGMA_ITEM_TYPE_ALL,
                                           LIGMA_ITEM_SET_ALL));
        }

      for (iter = objects; iter; iter = g_list_next (iter))
        {
          LigmaObject          *object = iter->data;
          LigmaTransformResize  clip   = options->clip;
          GeglRectangle        orig_bounds;
          GeglRectangle        new_bounds;
          gdouble              ratio  = 0.0;

          if (LIGMA_IS_DRAWABLE (object))
            {
              if (selection_empty)
                {
                  LigmaItem *item = LIGMA_ITEM (object);

                  ligma_item_get_offset (item, &orig_bounds.x, &orig_bounds.y);

                  orig_bounds.width  = ligma_item_get_width  (item);
                  orig_bounds.height = ligma_item_get_height (item);

                  clip = ligma_item_get_clip (item, clip);
                }
              else
                {
                  orig_bounds = selection_bounds;
                }
            }
          else if (LIGMA_IS_ITEM (object))
            {
              LigmaItem *item = LIGMA_ITEM (object);

              ligma_item_bounds (item,
                                &orig_bounds.x,     &orig_bounds.y,
                                &orig_bounds.width, &orig_bounds.height);

              clip = ligma_item_get_clip (item, clip);
            }
          else
            {
              LigmaImage *image;

              g_return_val_if_fail (LIGMA_IS_IMAGE (object), FALSE);

              image = LIGMA_IMAGE (object);

              orig_bounds.x      = 0;
              orig_bounds.y      = 0;
              orig_bounds.width  = ligma_image_get_width  (image);
              orig_bounds.height = ligma_image_get_height (image);
            }

          ligma_transform_resize_boundary (&transform, clip,

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
                           (gdouble) ligma_image_get_width (image));
            }

          if (new_bounds.height > orig_bounds.height)
            {
              ratio = MAX (ratio,
                           (gdouble) new_bounds.height /
                           (gdouble) ligma_image_get_height (image));
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

      dialog = ligma_message_dialog_new (_("Confirm Transformation"),
                                        LIGMA_ICON_DIALOG_WARNING,
                                        GTK_WIDGET (shell),
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        ligma_standard_help_func, NULL,

                                        _("_Cancel"),    GTK_RESPONSE_CANCEL,
                                        _("_Transform"), GTK_RESPONSE_OK,

                                        NULL);

      ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                                GTK_RESPONSE_OK,
                                                GTK_RESPONSE_CANCEL,
                                                -1);

      if (LIGMA_IS_ITEM (max_ratio_object))
        {
          ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                             _("Transformation creates "
                                               "a very large item."));

          ligma_message_box_set_text (
            LIGMA_MESSAGE_DIALOG (dialog)->box,
            _("Applying the transformation will result "
              "in an item that is over %g times larger "
              "than the image."),
              floor (max_ratio));
        }
      else
        {
          ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (dialog)->box,
                                             _("Transformation creates "
                                               "a very large image."));

          ligma_message_box_set_text (
            LIGMA_MESSAGE_DIALOG (dialog)->box,
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
ligma_transform_tool_bounds (LigmaTransformTool *tr_tool,
                            LigmaDisplay       *display)
{
  LigmaTransformOptions *options;
  LigmaDisplayShell     *shell;
  LigmaImage            *image;
  gboolean              non_empty = TRUE;

  g_return_val_if_fail (LIGMA_IS_TRANSFORM_TOOL (tr_tool), FALSE);

  options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  image   = ligma_display_get_image (display);
  shell   = ligma_display_get_shell (display);

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  switch (options->type)
    {
    case LIGMA_TRANSFORM_TYPE_LAYER:
      {
        GList *drawables;
        gint   offset_x;
        gint   offset_y;
        gint   x, y;
        gint   width, height;

        drawables = ligma_image_get_selected_drawables (image);

        ligma_item_get_offset (LIGMA_ITEM (drawables->data), &offset_x, &offset_y);

        non_empty = ligma_item_mask_intersect (LIGMA_ITEM (drawables->data),
                                              &x, &y, &width, &height);
        tr_tool->x1 = x + offset_x;
        tr_tool->y1 = y + offset_y;
        tr_tool->x2 = x + width  + offset_x;
        tr_tool->y2 = y + height + offset_y;

        g_list_free (drawables);
      }
      break;

    case LIGMA_TRANSFORM_TYPE_SELECTION:
      {
        ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                          &tr_tool->x1, &tr_tool->y1,
                          &tr_tool->x2, &tr_tool->y2);
        tr_tool->x2 += tr_tool->x1;
        tr_tool->y2 += tr_tool->y1;
      }
      break;

    case LIGMA_TRANSFORM_TYPE_PATH:
      {
        LigmaChannel *selection = ligma_image_get_mask (image);

        /* if selection is not empty, use its bounds to perform the
         * transformation of the path
         */

        if (! ligma_channel_is_empty (selection))
          {
            ligma_item_bounds (LIGMA_ITEM (selection),
                              &tr_tool->x1, &tr_tool->y1,
                              &tr_tool->x2, &tr_tool->y2);

            tr_tool->x2 += tr_tool->x1;
            tr_tool->y2 += tr_tool->y1;
          }
        else
          {
            GList *iter;

            /* without selection, test the emptiness of the path bounds :
             * if empty, use the canvas bounds
             * else use the path bounds
             */

            tr_tool->x1 = G_MAXINT;
            tr_tool->y1 = G_MAXINT;
            tr_tool->x2 = G_MININT;
            tr_tool->y2 = G_MININT;

            for (iter = ligma_image_get_selected_vectors (image); iter; iter = iter->next)
              {
                LigmaItem *item   = iter->data;
                gint      x;
                gint      y;
                gint      width;
                gint      height;

                if (ligma_item_bounds (item, &x, &y, &width, &height))
                  {
                    tr_tool->x1 = MIN (tr_tool->x1, x);
                    tr_tool->y1 = MIN (tr_tool->y1, y);
                    tr_tool->x2 = MAX (tr_tool->x2, x + width);
                    tr_tool->y2 = MAX (tr_tool->y2, y + height);
                  }
              }

            if (tr_tool->x2 <= tr_tool->x1 || tr_tool->y2 <= tr_tool->y1)
              {
                tr_tool->x1 = 0;
                tr_tool->y1 = 0;
                tr_tool->x2 = ligma_image_get_width (image);
                tr_tool->y2 = ligma_image_get_height (image);
              }
          }
      }

      break;

    case LIGMA_TRANSFORM_TYPE_IMAGE:
      if (! shell->show_all)
        {
          tr_tool->x1 = 0;
          tr_tool->y1 = 0;
          tr_tool->x2 = ligma_image_get_width  (image);
          tr_tool->y2 = ligma_image_get_height (image);
        }
      else
        {
          GeglRectangle bounding_box;

          bounding_box = ligma_display_shell_get_bounding_box (shell);

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
ligma_transform_tool_recalc_matrix (LigmaTransformTool *tr_tool,
                                   LigmaDisplay       *display)
{
  g_return_if_fail (LIGMA_IS_TRANSFORM_TOOL (tr_tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  if (tr_tool->x1 == tr_tool->x2 && tr_tool->y1 == tr_tool->y2)
    ligma_transform_tool_bounds (tr_tool, display);

  if (LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix)
    LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix (tr_tool);
}

GList *
ligma_transform_tool_get_selected_objects (LigmaTransformTool  *tr_tool,
                                          LigmaDisplay        *display)
{
  LigmaTransformOptions *options;
  LigmaImage            *image;
  GList                *objects = NULL;

  g_return_val_if_fail (LIGMA_IS_TRANSFORM_TOOL (tr_tool), NULL);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);

  options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  image = ligma_display_get_image (display);

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  if (tr_tool->objects)
    return g_list_copy (tr_tool->objects);

  switch (options->type)
    {
    case LIGMA_TRANSFORM_TYPE_LAYER:
      objects = ligma_image_get_selected_drawables (image);
      break;

    case LIGMA_TRANSFORM_TYPE_SELECTION:
      if (! ligma_channel_is_empty (ligma_image_get_mask (image)))
        objects = g_list_prepend (NULL, ligma_image_get_mask (image));
      break;

    case LIGMA_TRANSFORM_TYPE_PATH:
      objects = g_list_copy (ligma_image_get_selected_vectors (image));
      break;

    case LIGMA_TRANSFORM_TYPE_IMAGE:
      objects = g_list_prepend (NULL, image);
      break;
    }

  return objects;
}

GList *
ligma_transform_tool_check_selected_objects (LigmaTransformTool  *tr_tool,
                                            LigmaDisplay        *display,
                                            GError            **error)
{
  LigmaTransformOptions *options;
  GList                *objects;
  GList                *iter;
  const gchar          *null_message   = NULL;
  const gchar          *locked_message = NULL;
  LigmaItem             *locked_item    = NULL;
  LigmaGuiConfig        *config         = LIGMA_GUI_CONFIG (display->ligma->config);

  g_return_val_if_fail (LIGMA_IS_TRANSFORM_TOOL (tr_tool), NULL);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  objects = ligma_transform_tool_get_selected_objects (tr_tool, display);

  switch (options->type)
    {
    case LIGMA_TRANSFORM_TYPE_LAYER:
      null_message = _("There is no layer to transform.");

      for (iter = objects; iter; iter = iter->next)
        {
          LigmaItem *item = iter->data;

          if (ligma_item_is_content_locked (item, &locked_item))
            locked_message = _("A selected layer's pixels are locked.");
          else if (ligma_item_is_position_locked (item, &locked_item))
            locked_message = _("A selected layer's position and size are locked.");

          if (! ligma_item_is_visible (item) &&
              ! config->edit_non_visible &&
              ! g_list_find (tr_tool->objects, item)) /* see bug #759194 */
            {
              g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                                   _("A selected layer is not visible."));
              return NULL;
            }

          if (! ligma_transform_tool_bounds (tr_tool, display))
            {
              g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                                   _("The selection does not intersect with a selected layer."));
              return NULL;
            }
        }
      break;

    case LIGMA_TRANSFORM_TYPE_SELECTION:
      null_message = _("There is no selection to transform.");

      for (iter = objects; iter; iter = iter->next)
        {
          LigmaItem *item = iter->data;

          /* cannot happen, so don't translate these messages */
          if (ligma_item_is_content_locked (item, &locked_item))
            locked_message = "The selection's pixels are locked.";
          else if (ligma_item_is_position_locked (item, &locked_item))
            locked_message = "The selection's position and size are locked.";
        }
      break;

    case LIGMA_TRANSFORM_TYPE_PATH:
      null_message = _("There is no path to transform.");

      for (iter = objects; iter; iter = iter->next)
        {
          LigmaItem *item = iter->data;

          if (ligma_item_is_content_locked (item, &locked_item))
            locked_message = _("The selected path's strokes are locked.");
          else if (ligma_item_is_position_locked (item, &locked_item))
            locked_message = _("The selected path's position is locked.");
          else if (! ligma_vectors_get_n_strokes (LIGMA_VECTORS (item)))
            locked_message = _("The selected path has no strokes.");
        }
      break;

    case LIGMA_TRANSFORM_TYPE_IMAGE:
      /* cannot happen, so don't translate this message */
      null_message = "There is no image to transform.";
      break;
    }

  if (! objects)
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED, null_message);
      if (error)
        {
          ligma_tools_show_tool_options (display->ligma);
          ligma_widget_blink (options->type_box);
        }
      return NULL;
    }

  if (locked_message)
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED, locked_message);
      if (error)
        {
          if (locked_item == NULL)
            locked_item = LIGMA_ITEM (objects->data);

          ligma_tools_blink_lock_box (display->ligma, locked_item);
        }
      return NULL;
    }

  return objects;
}

gboolean
ligma_transform_tool_transform (LigmaTransformTool *tr_tool,
                               LigmaDisplay       *display)
{
  LigmaTool             *tool;
  LigmaTransformOptions *options;
  LigmaImage            *image;
  GList                *selected_objects;
  GeglBuffer           *orig_buffer   = NULL;
  gint                  orig_offset_x = 0;
  gint                  orig_offset_y = 0;
  GeglBuffer           *new_buffer;
  gint                  new_offset_x;
  gint                  new_offset_y;
  LigmaColorProfile     *buffer_profile;
  gchar                *undo_desc     = NULL;
  gboolean              new_layer     = FALSE;
  GError               *error         = NULL;

  g_return_val_if_fail (LIGMA_IS_TRANSFORM_TOOL (tr_tool), FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);

  tool    = LIGMA_TOOL (tr_tool);
  options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tool);
  image   = ligma_display_get_image (display);

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  selected_objects = ligma_transform_tool_check_selected_objects (tr_tool, display,
                                                                 &error);

  if (! selected_objects)
    {
      ligma_tool_message_literal (tool, display, error->message);
      g_clear_error (&error);
      return FALSE;
    }

  ligma_transform_tool_recalc_matrix (tr_tool, display);

  if (! tr_tool->transform_valid)
    {
      ligma_tool_message_literal (tool, display,
                                 _("The current transform is invalid"));
      return FALSE;
    }

  if (LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix &&
      ligma_matrix3_is_identity (&tr_tool->transform))
    {
      /* No need to commit an identity transformation! */
      return TRUE;
    }

  if (! ligma_transform_tool_confirm (tr_tool, display))
    return FALSE;

  ligma_set_busy (display->ligma);

  /*  We're going to dirty this image, but we want to keep the tool around  */
  ligma_tool_control_push_preserve (tool->control, TRUE);

  undo_desc = LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_undo_desc (tr_tool);
  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_TRANSFORM, undo_desc);
  g_free (undo_desc);

  switch (options->type)
    {
    case LIGMA_TRANSFORM_TYPE_LAYER:
      if (! ligma_channel_is_empty (ligma_image_get_mask (image)))
        {
          orig_buffer = ligma_drawable_transform_cut (
            selected_objects,
            LIGMA_CONTEXT (options),
            &orig_offset_x,
            &orig_offset_y,
            &new_layer);
        }
      break;

    case LIGMA_TRANSFORM_TYPE_SELECTION:
    case LIGMA_TRANSFORM_TYPE_PATH:
    case LIGMA_TRANSFORM_TYPE_IMAGE:
      break;
    }

  /*  Send the request for the transformation to the tool...
   */
  new_buffer = LIGMA_TRANSFORM_TOOL_GET_CLASS (tr_tool)->transform (
    tr_tool,
    selected_objects,
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
    case LIGMA_TRANSFORM_TYPE_LAYER:
      if (new_buffer)
        {
          /*  paste the new transformed image to the image...also implement
           *  undo...
           */
          ligma_drawable_transform_paste (LIGMA_DRAWABLE (selected_objects->data),
                                         new_buffer, buffer_profile,
                                         new_offset_x, new_offset_y,
                                         new_layer);
          g_object_unref (new_buffer);
        }
      break;

    case LIGMA_TRANSFORM_TYPE_SELECTION:
    case LIGMA_TRANSFORM_TYPE_PATH:
    case LIGMA_TRANSFORM_TYPE_IMAGE:
      /*  Nothing to be done  */
      break;
    }

  ligma_image_undo_group_end (image);

  /*  We're done dirtying the image, and would like to be restarted if
   *  the image gets dirty while the tool exists
   */
  ligma_tool_control_pop_preserve (tool->control);

  ligma_unset_busy (display->ligma);

  ligma_image_flush (image);

  g_list_free (selected_objects);

  return TRUE;
}

void
ligma_transform_tool_set_type (LigmaTransformTool *tr_tool,
                              LigmaTransformType  type)
{
  LigmaTransformOptions *options;

  g_return_if_fail (LIGMA_IS_TRANSFORM_TOOL (tr_tool));

  options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  if (! tr_tool->restore_type)
    tr_tool->saved_type = options->type;

  tr_tool->restore_type = FALSE;

  g_object_set (options,
                "type", type,
                NULL);

  tr_tool->restore_type = TRUE;
}
