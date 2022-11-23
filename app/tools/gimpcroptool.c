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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-crop.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaitem.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmatoolrectangle.h"

#include "ligmacropoptions.h"
#include "ligmacroptool.h"
#include "ligmarectangleoptions.h"
#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"

#include "ligma-intl.h"


static void      ligma_crop_tool_constructed                (GObject              *object);
static void      ligma_crop_tool_dispose                    (GObject              *object);

static void      ligma_crop_tool_control                    (LigmaTool             *tool,
                                                            LigmaToolAction        action,
                                                            LigmaDisplay          *display);
static void      ligma_crop_tool_button_press               (LigmaTool             *tool,
                                                            const LigmaCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            LigmaButtonPressType   press_type,
                                                            LigmaDisplay          *display);
static void      ligma_crop_tool_button_release             (LigmaTool             *tool,
                                                            const LigmaCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            LigmaButtonReleaseType release_type,
                                                            LigmaDisplay          *display);
static void     ligma_crop_tool_motion                      (LigmaTool             *tool,
                                                            const LigmaCoords     *coords,
                                                            guint32               time,
                                                            GdkModifierType       state,
                                                            LigmaDisplay          *display);
static void      ligma_crop_tool_options_notify             (LigmaTool             *tool,
                                                            LigmaToolOptions      *options,
                                                            const GParamSpec     *pspec);

static void      ligma_crop_tool_rectangle_changed          (LigmaToolWidget       *rectangle,
                                                            LigmaCropTool         *crop_tool);
static void      ligma_crop_tool_rectangle_response         (LigmaToolWidget       *rectangle,
                                                            gint                  response_id,
                                                            LigmaCropTool         *crop_tool);
static void      ligma_crop_tool_rectangle_change_complete  (LigmaToolRectangle    *rectangle,
                                                            LigmaCropTool         *crop_tool);

static void      ligma_crop_tool_start                      (LigmaCropTool         *crop_tool,
                                                            LigmaDisplay          *display);
static void      ligma_crop_tool_commit                     (LigmaCropTool         *crop_tool);
static void      ligma_crop_tool_halt                       (LigmaCropTool         *crop_tool);

static void      ligma_crop_tool_update_option_defaults     (LigmaCropTool         *crop_tool,
                                                            gboolean              ignore_pending);
static LigmaRectangleConstraint
                 ligma_crop_tool_get_constraint             (LigmaCropTool         *crop_tool);

static void      ligma_crop_tool_image_changed              (LigmaCropTool         *crop_tool,
                                                            LigmaImage            *image,
                                                            LigmaContext          *context);
static void      ligma_crop_tool_image_size_changed         (LigmaCropTool         *crop_tool);
static void   ligma_crop_tool_image_selected_layers_changed (LigmaCropTool         *crop_tool);
static void      ligma_crop_tool_layer_size_changed         (LigmaCropTool         *crop_tool);

static void      ligma_crop_tool_auto_shrink                (LigmaCropTool         *crop_tool);


G_DEFINE_TYPE (LigmaCropTool, ligma_crop_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_crop_tool_parent_class


/*  public functions  */

void
ligma_crop_tool_register (LigmaToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (LIGMA_TYPE_CROP_TOOL,
                LIGMA_TYPE_CROP_OPTIONS,
                ligma_crop_options_gui,
                LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND |
                LIGMA_CONTEXT_PROP_MASK_PATTERN,
                "ligma-crop-tool",
                _("Crop"),
                _("Crop Tool: Remove edge areas from image or layer"),
                N_("_Crop"), "<shift>C",
                NULL, LIGMA_HELP_TOOL_CROP,
                LIGMA_ICON_TOOL_CROP,
                data);
}

static void
ligma_crop_tool_class_init (LigmaCropToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaToolClass *tool_class   = LIGMA_TOOL_CLASS (klass);

  object_class->constructed  = ligma_crop_tool_constructed;
  object_class->dispose      = ligma_crop_tool_dispose;

  tool_class->control        = ligma_crop_tool_control;
  tool_class->button_press   = ligma_crop_tool_button_press;
  tool_class->button_release = ligma_crop_tool_button_release;
  tool_class->motion         = ligma_crop_tool_motion;
  tool_class->options_notify = ligma_crop_tool_options_notify;
}

static void
ligma_crop_tool_init (LigmaCropTool *crop_tool)
{
  LigmaTool *tool = LIGMA_TOOL (crop_tool);

  ligma_tool_control_set_wants_click      (tool->control, TRUE);
  ligma_tool_control_set_active_modifiers (tool->control,
                                          LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  ligma_tool_control_set_precision        (tool->control,
                                          LIGMA_CURSOR_PRECISION_PIXEL_BORDER);
  ligma_tool_control_set_cursor           (tool->control,
                                          LIGMA_CURSOR_CROSSHAIR_SMALL);
  ligma_tool_control_set_tool_cursor      (tool->control,
                                          LIGMA_TOOL_CURSOR_CROP);

  ligma_draw_tool_set_default_status (LIGMA_DRAW_TOOL (tool),
                                     _("Click-Drag to draw a crop rectangle"));
}

static void
ligma_crop_tool_constructed (GObject *object)
{
  LigmaCropTool    *crop_tool = LIGMA_CROP_TOOL (object);
  LigmaContext     *context;
  LigmaToolInfo    *tool_info;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  tool_info = LIGMA_TOOL (crop_tool)->tool_info;

  context = ligma_get_user_context (tool_info->ligma);

  g_signal_connect_object (context, "image-changed",
                           G_CALLBACK (ligma_crop_tool_image_changed),
                           crop_tool,
                           G_CONNECT_SWAPPED);

  /* Make sure we are connected to "size-changed" for the initial
   * image.
   */
  ligma_crop_tool_image_changed (crop_tool,
                                ligma_context_get_image (context),
                                context);
}

static void
ligma_crop_tool_dispose (GObject *object)
{
  LigmaCropTool *crop_tool = LIGMA_CROP_TOOL (object);

  /* Clean up current_image and current_layers. */
  ligma_crop_tool_image_changed (crop_tool, NULL, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_crop_tool_control (LigmaTool       *tool,
                        LigmaToolAction  action,
                        LigmaDisplay    *display)
{
  LigmaCropTool *crop_tool = LIGMA_CROP_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_crop_tool_halt (crop_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_crop_tool_commit (crop_tool);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_crop_tool_button_press (LigmaTool            *tool,
                             const LigmaCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             LigmaButtonPressType  press_type,
                             LigmaDisplay         *display)
{
  LigmaCropTool *crop_tool = LIGMA_CROP_TOOL (tool);

  if (tool->display && display != tool->display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      ligma_crop_tool_start (crop_tool, display);

      ligma_tool_widget_hover (crop_tool->widget, coords, state, TRUE);

      /* HACK: force CREATING on a newly created rectangle; otherwise,
       * property bindings would cause the rectangle to start with the
       * size from tool options.
       */
      ligma_tool_rectangle_set_function (LIGMA_TOOL_RECTANGLE (crop_tool->widget),
                                        LIGMA_TOOL_RECTANGLE_CREATING);
    }

  if (ligma_tool_widget_button_press (crop_tool->widget, coords, time, state,
                                     press_type))
    {
      crop_tool->grab_widget = crop_tool->widget;
    }

  ligma_tool_control_activate (tool->control);
}

static void
ligma_crop_tool_button_release (LigmaTool              *tool,
                               const LigmaCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               LigmaButtonReleaseType  release_type,
                               LigmaDisplay           *display)
{
  LigmaCropTool *crop_tool = LIGMA_CROP_TOOL (tool);

  ligma_tool_control_halt (tool->control);

  if (crop_tool->grab_widget)
    {
      ligma_tool_widget_button_release (crop_tool->grab_widget,
                                       coords, time, state, release_type);
      crop_tool->grab_widget = NULL;
    }

  ligma_tool_push_status (tool, display, _("Click or press Enter to crop"));
}

static void
ligma_crop_tool_motion (LigmaTool         *tool,
                       const LigmaCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       LigmaDisplay      *display)
{
  LigmaCropTool *crop_tool = LIGMA_CROP_TOOL (tool);

  if (crop_tool->grab_widget)
    {
      ligma_tool_widget_motion (crop_tool->grab_widget, coords, time, state);
    }
}

static void
ligma_crop_tool_options_notify (LigmaTool         *tool,
                               LigmaToolOptions  *options,
                               const GParamSpec *pspec)
{
  LigmaCropTool *crop_tool = LIGMA_CROP_TOOL (tool);

  if (! strcmp (pspec->name, "layer-only") ||
      ! strcmp (pspec->name, "allow-growing"))
    {
      if (crop_tool->widget)
        {
          ligma_tool_rectangle_set_constraint (LIGMA_TOOL_RECTANGLE (crop_tool->widget),
                                              ligma_crop_tool_get_constraint (crop_tool));
        }
      else
        {
          ligma_crop_tool_update_option_defaults (crop_tool, FALSE);
        }
    }
}

static void
ligma_crop_tool_rectangle_changed (LigmaToolWidget *rectangle,
                                  LigmaCropTool   *crop_tool)
{
}

static void
ligma_crop_tool_rectangle_response (LigmaToolWidget *rectangle,
                                   gint            response_id,
                                   LigmaCropTool   *crop_tool)
{
  LigmaTool *tool = LIGMA_TOOL (crop_tool);

  switch (response_id)
    {
    case LIGMA_TOOL_WIDGET_RESPONSE_CONFIRM:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, tool->display);
      break;

    case LIGMA_TOOL_WIDGET_RESPONSE_CANCEL:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
ligma_crop_tool_rectangle_change_complete (LigmaToolRectangle *rectangle,
                                          LigmaCropTool      *crop_tool)
{
  ligma_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
ligma_crop_tool_start (LigmaCropTool *crop_tool,
                      LigmaDisplay  *display)
{
  static const gchar *properties[] =
  {
    "highlight",
    "highlight-opacity",
    "guide",
    "x",
    "y",
    "width",
    "height",
    "fixed-rule-active",
    "fixed-rule",
    "desired-fixed-width",
    "desired-fixed-height",
    "desired-fixed-size-width",
    "desired-fixed-size-height",
    "aspect-numerator",
    "aspect-denominator",
    "fixed-center"
  };

  LigmaTool         *tool    = LIGMA_TOOL (crop_tool);
  LigmaDisplayShell *shell   = ligma_display_get_shell (display);
  LigmaCropOptions  *options = LIGMA_CROP_TOOL_GET_OPTIONS (crop_tool);
  LigmaToolWidget   *widget;
  gint              i;

  tool->display = display;

  crop_tool->widget = widget = ligma_tool_rectangle_new (shell);

  g_object_set (widget,
                "status-title", _("Crop to: "),
                NULL);

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), widget);

  for (i = 0; i < G_N_ELEMENTS (properties); i++)
    {
      GBinding *binding =
        g_object_bind_property (G_OBJECT (options), properties[i],
                                G_OBJECT (widget),  properties[i],
                                G_BINDING_SYNC_CREATE |
                                G_BINDING_BIDIRECTIONAL);

      crop_tool->bindings = g_list_prepend (crop_tool->bindings, binding);
    }

  ligma_rectangle_options_connect (LIGMA_RECTANGLE_OPTIONS (options),
                                  ligma_display_get_image (shell->display),
                                  G_CALLBACK (ligma_crop_tool_auto_shrink),
                                  crop_tool);

  ligma_tool_rectangle_set_constraint (LIGMA_TOOL_RECTANGLE (widget),
                                      ligma_crop_tool_get_constraint (crop_tool));

  g_signal_connect (widget, "changed",
                    G_CALLBACK (ligma_crop_tool_rectangle_changed),
                    crop_tool);
  g_signal_connect (widget, "response",
                    G_CALLBACK (ligma_crop_tool_rectangle_response),
                    crop_tool);
  g_signal_connect (widget, "change-complete",
                    G_CALLBACK (ligma_crop_tool_rectangle_change_complete),
                    crop_tool);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);
}

static void
ligma_crop_tool_commit (LigmaCropTool *crop_tool)
{
  LigmaTool *tool = LIGMA_TOOL (crop_tool);

  if (tool->display)
    {
      LigmaCropOptions *options = LIGMA_CROP_TOOL_GET_OPTIONS (tool);
      LigmaImage       *image   = ligma_display_get_image (tool->display);
      gdouble          x, y;
      gdouble          x2, y2;
      gint             w, h;

      ligma_tool_rectangle_get_public_rect (LIGMA_TOOL_RECTANGLE (crop_tool->widget),
                                           &x, &y, &x2, &y2);
      w = x2 - x;
      h = y2 - y;

      ligma_tool_pop_status (tool, tool->display);

      /* if rectangle exists, crop it */
      if (w > 0 && h > 0)
        {
          if (options->layer_only)
            {
              GList *layers = ligma_image_get_selected_layers (image);
              GList *iter;
              gint   off_x, off_y;
              gchar *undo_text;

              if (! layers)
                {
                  ligma_tool_message_literal (tool, tool->display,
                                             _("There are no selected layers to crop."));
                  return;
                }

              for (iter = layers; iter; iter = iter->next)
                if (! ligma_item_is_content_locked (LIGMA_ITEM (iter->data), NULL))
                  break;

              if (iter == NULL)
                {
                  ligma_tool_message_literal (tool, tool->display,
                                             _("All selected layers' pixels are locked."));
                  ligma_tools_blink_lock_box (tool->display->ligma, LIGMA_ITEM (layers->data));
                  return;
                }

              undo_text = ngettext ("Resize Layer", "Resize %d layers",
                                    g_list_length (layers));
              undo_text = g_strdup_printf (undo_text, g_list_length (layers));
              ligma_image_undo_group_start (image,
                                           LIGMA_UNDO_GROUP_IMAGE_CROP,
                                           undo_text);
              g_free (undo_text);
              for (iter = layers; iter; iter = iter->next)
                {
                  ligma_item_get_offset (LIGMA_ITEM (iter->data), &off_x, &off_y);

                  off_x -= x;
                  off_y -= y;

                  ligma_item_resize (LIGMA_ITEM (iter->data),
                                    LIGMA_CONTEXT (options), options->fill_type,
                                    w, h, off_x, off_y);
                }
              ligma_image_undo_group_end (image);
            }
          else
            {
              ligma_image_crop (image,
                               LIGMA_CONTEXT (options), LIGMA_FILL_TRANSPARENT,
                               x, y, w, h, options->delete_pixels);
            }

          ligma_image_flush (image);
        }
    }
}

static void
ligma_crop_tool_halt (LigmaCropTool *crop_tool)
{
  LigmaTool         *tool    = LIGMA_TOOL (crop_tool);
  LigmaCropOptions  *options = LIGMA_CROP_TOOL_GET_OPTIONS (crop_tool);

  if (tool->display)
    {
      LigmaDisplayShell *shell = ligma_display_get_shell (tool->display);

      ligma_display_shell_set_highlight (shell, NULL, 0.0);

      ligma_rectangle_options_disconnect (LIGMA_RECTANGLE_OPTIONS (options),
                                         G_CALLBACK (ligma_crop_tool_auto_shrink),
                                         crop_tool);
    }

  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tool)))
    ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

  /*  disconnect bindings manually so they are really gone *now*, we
   *  might be in the middle of a signal emission that keeps the
   *  widget and its bindings alive.
   */
  g_list_free_full (crop_tool->bindings, (GDestroyNotify) g_object_unref);
  crop_tool->bindings = NULL;

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), NULL);
  g_clear_object (&crop_tool->widget);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  ligma_crop_tool_update_option_defaults (crop_tool, TRUE);
}

/**
 * ligma_crop_tool_update_option_defaults:
 * @crop_tool:
 * @ignore_pending: %TRUE to ignore any pending crop rectangle.
 *
 * Sets the default Fixed: Aspect ratio and Fixed: Size option
 * properties.
 */
static void
ligma_crop_tool_update_option_defaults (LigmaCropTool *crop_tool,
                                       gboolean      ignore_pending)
{
  LigmaTool             *tool      = LIGMA_TOOL (crop_tool);
  LigmaToolRectangle    *rectangle = LIGMA_TOOL_RECTANGLE (crop_tool->widget);
  LigmaRectangleOptions *options;

  options = LIGMA_RECTANGLE_OPTIONS (LIGMA_TOOL_GET_OPTIONS (tool));

  if (rectangle && ! ignore_pending)
    {
      /* There is a pending rectangle and we should not ignore it, so
       * set default Fixed: Aspect ratio to the same as the current
       * pending rectangle width/height.
       */

      ligma_tool_rectangle_pending_size_set (rectangle,
                                            G_OBJECT (options),
                                            "default-aspect-numerator",
                                            "default-aspect-denominator");

      g_object_set (G_OBJECT (options),
                    "use-string-current", TRUE,
                    NULL);
    }
  else
    {
      /* There is no pending rectangle, set default Fixed: Aspect
       * ratio to that of the current image/layer.
       */

      if (! rectangle)
        {
          /* ugly hack: if we don't have a widget, construct a temporary one
           * so that we can use it to call
           * ligma_tool_rectangle_constraint_size_set().
           */

          LigmaContext *context = ligma_get_user_context (tool->tool_info->ligma);
          LigmaDisplay *display = ligma_context_get_display (context);

          if (display)
            {
              LigmaDisplayShell *shell = ligma_display_get_shell (display);

              rectangle = LIGMA_TOOL_RECTANGLE (ligma_tool_rectangle_new (shell));

              ligma_tool_rectangle_set_constraint (
                rectangle, ligma_crop_tool_get_constraint (crop_tool));
            }
        }

      if (rectangle)
        {
          ligma_tool_rectangle_constraint_size_set (rectangle,
                                                   G_OBJECT (options),
                                                   "default-aspect-numerator",
                                                   "default-aspect-denominator");

          if (! crop_tool->widget)
            g_object_unref (rectangle);
        }

      g_object_set (G_OBJECT (options),
                    "use-string-current", FALSE,
                    NULL);
    }
}

static LigmaRectangleConstraint
ligma_crop_tool_get_constraint (LigmaCropTool *crop_tool)
{
  LigmaCropOptions *crop_options = LIGMA_CROP_TOOL_GET_OPTIONS (crop_tool);

  if (crop_options->allow_growing)
    {
      return LIGMA_RECTANGLE_CONSTRAIN_NONE;
    }
  else
    {
      return crop_options->layer_only ? LIGMA_RECTANGLE_CONSTRAIN_DRAWABLE :
                                        LIGMA_RECTANGLE_CONSTRAIN_IMAGE;
    }
}

static void
ligma_crop_tool_image_changed (LigmaCropTool *crop_tool,
                              LigmaImage    *image,
                              LigmaContext  *context)
{
  if (crop_tool->current_image)
    {
      g_signal_handlers_disconnect_by_func (crop_tool->current_image,
                                            ligma_crop_tool_image_size_changed,
                                            NULL);
      g_signal_handlers_disconnect_by_func (crop_tool->current_image,
                                            ligma_crop_tool_image_selected_layers_changed,
                                            NULL);

      g_object_remove_weak_pointer (G_OBJECT (crop_tool->current_image),
                                    (gpointer) &crop_tool->current_image);
    }

  crop_tool->current_image = image;

  if (crop_tool->current_image)
    {
      g_object_add_weak_pointer (G_OBJECT (crop_tool->current_image),
                                 (gpointer) &crop_tool->current_image);

      g_signal_connect_object (crop_tool->current_image, "size-changed",
                               G_CALLBACK (ligma_crop_tool_image_size_changed),
                               crop_tool,
                               G_CONNECT_SWAPPED);
      g_signal_connect_object (crop_tool->current_image, "selected-layers-changed",
                               G_CALLBACK (ligma_crop_tool_image_selected_layers_changed),
                               crop_tool,
                               G_CONNECT_SWAPPED);
    }

  /* Make sure we are connected to "size-changed" for the initial
   * layer.
   */
  ligma_crop_tool_image_selected_layers_changed (crop_tool);

  ligma_crop_tool_update_option_defaults (LIGMA_CROP_TOOL (crop_tool), FALSE);
}

static void
ligma_crop_tool_image_size_changed (LigmaCropTool *crop_tool)
{
  ligma_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
ligma_crop_tool_image_selected_layers_changed (LigmaCropTool *crop_tool)
{
  GList *iter;

  if (crop_tool->current_layers)
    {
      for (iter = crop_tool->current_layers; iter; iter = iter->next)
        {
          if (iter->data)
            {
              g_signal_handlers_disconnect_by_func (iter->data,
                                                    ligma_crop_tool_layer_size_changed,
                                                    NULL);

              g_object_remove_weak_pointer (G_OBJECT (iter->data),
                                            (gpointer) &iter->data);
            }
        }
      g_list_free (crop_tool->current_layers);
      crop_tool->current_layers = NULL;
    }

  if (crop_tool->current_image)
    {
      crop_tool->current_layers = ligma_image_get_selected_layers (crop_tool->current_image);
      crop_tool->current_layers = g_list_copy (crop_tool->current_layers);
    }
  else
    {
      crop_tool->current_layers = NULL;
    }

  if (crop_tool->current_layers)
    {
      for (iter = crop_tool->current_layers; iter; iter = iter->next)
        {
          g_object_add_weak_pointer (G_OBJECT (iter->data),
                                     (gpointer) &iter->data);

          g_signal_connect_object (iter->data, "size-changed",
                                   G_CALLBACK (ligma_crop_tool_layer_size_changed),
                                   crop_tool,
                                   G_CONNECT_SWAPPED);
        }
    }

  ligma_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
ligma_crop_tool_layer_size_changed (LigmaCropTool *crop_tool)
{
  ligma_crop_tool_update_option_defaults (crop_tool, FALSE);
}

static void
ligma_crop_tool_auto_shrink (LigmaCropTool *crop_tool)
{
  gboolean shrink_merged ;

  g_object_get (ligma_tool_get_options (LIGMA_TOOL (crop_tool)),
                "shrink-merged", &shrink_merged,
                NULL);

  ligma_tool_rectangle_auto_shrink (LIGMA_TOOL_RECTANGLE (crop_tool->widget),
                                   shrink_merged);
}
