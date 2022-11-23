/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaForegroundSelectTool
 * Copyright (C) 2005  Sven Neumann <sven@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaguiconfig.h"

#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-mask.h"
#include "gegl/ligma-gegl-utils.h"

#include "core/ligma.h"
#include "core/ligmachannel-select.h"
#include "core/ligmadrawable-foreground-extract.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"
#include "core/ligmaprogress.h"
#include "core/ligmascanconvert.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvasitem.h"
#include "display/ligmacanvasbufferpreview.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmatoolgui.h"

#include "ligmaforegroundselecttool.h"
#include "ligmaforegroundselectoptions.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


#define FAR_OUTSIDE -10000


typedef struct _StrokeUndo StrokeUndo;

struct _StrokeUndo
{
  GeglBuffer          *saved_trimap;
  gint                 trimap_x;
  gint                 trimap_y;
  LigmaMattingDrawMode  draw_mode;
  gint                 stroke_width;
};


static void   ligma_foreground_select_tool_finalize       (GObject          *object);

static gboolean  ligma_foreground_select_tool_initialize  (LigmaTool         *tool,
                                                          LigmaDisplay      *display,
                                                          GError          **error);
static void   ligma_foreground_select_tool_control        (LigmaTool         *tool,
                                                          LigmaToolAction    action,
                                                          LigmaDisplay      *display);
static void   ligma_foreground_select_tool_button_press   (LigmaTool         *tool,
                                                          const LigmaCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          LigmaButtonPressType press_type,
                                                          LigmaDisplay      *display);
static void   ligma_foreground_select_tool_button_release (LigmaTool         *tool,
                                                          const LigmaCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          LigmaButtonReleaseType release_type,
                                                          LigmaDisplay      *display);
static void   ligma_foreground_select_tool_motion         (LigmaTool         *tool,
                                                          const LigmaCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          LigmaDisplay      *display);
static gboolean  ligma_foreground_select_tool_key_press   (LigmaTool         *tool,
                                                          GdkEventKey      *kevent,
                                                          LigmaDisplay      *display);
static void   ligma_foreground_select_tool_modifier_key   (LigmaTool         *tool,
                                                          GdkModifierType   key,
                                                          gboolean          press,
                                                          GdkModifierType   state,
                                                          LigmaDisplay      *display);
static void   ligma_foreground_select_tool_active_modifier_key
                                                         (LigmaTool         *tool,
                                                          GdkModifierType   key,
                                                          gboolean          press,
                                                          GdkModifierType   state,
                                                          LigmaDisplay      *display);
static void   ligma_foreground_select_tool_oper_update    (LigmaTool         *tool,
                                                          const LigmaCoords *coords,
                                                          GdkModifierType   state,
                                                          gboolean          proximity,
                                                          LigmaDisplay      *display);
static void   ligma_foreground_select_tool_cursor_update  (LigmaTool         *tool,
                                                          const LigmaCoords *coords,
                                                          GdkModifierType   state,
                                                          LigmaDisplay      *display);
static const gchar * ligma_foreground_select_tool_can_undo
                                                         (LigmaTool         *tool,
                                                          LigmaDisplay      *display);
static const gchar * ligma_foreground_select_tool_can_redo
                                                         (LigmaTool         *tool,
                                                          LigmaDisplay      *display);
static gboolean   ligma_foreground_select_tool_undo       (LigmaTool         *tool,
                                                          LigmaDisplay      *display);
static gboolean   ligma_foreground_select_tool_redo       (LigmaTool         *tool,
                                                          LigmaDisplay      *display);
static void   ligma_foreground_select_tool_options_notify (LigmaTool         *tool,
                                                          LigmaToolOptions  *options,
                                                          const GParamSpec *pspec);

static void   ligma_foreground_select_tool_draw           (LigmaDrawTool     *draw_tool);

static void   ligma_foreground_select_tool_confirm        (LigmaPolygonSelectTool *poly_sel,
                                                          LigmaDisplay           *display);

static void   ligma_foreground_select_tool_halt           (LigmaForegroundSelectTool *fg_select);
static void   ligma_foreground_select_tool_commit         (LigmaForegroundSelectTool *fg_select);

static void   ligma_foreground_select_tool_set_trimap     (LigmaForegroundSelectTool *fg_select);
static void   ligma_foreground_select_tool_set_preview    (LigmaForegroundSelectTool *fg_select);
static void   ligma_foreground_select_tool_preview        (LigmaForegroundSelectTool *fg_select);

static void   ligma_foreground_select_tool_stroke_paint   (LigmaForegroundSelectTool *fg_select);
static void   ligma_foreground_select_tool_cancel_paint   (LigmaForegroundSelectTool *fg_select);

static void   ligma_foreground_select_tool_response       (LigmaToolGui              *gui,
                                                          gint                      response_id,
                                                          LigmaForegroundSelectTool *fg_select);
static void   ligma_foreground_select_tool_preview_toggled(GtkToggleButton          *button,
                                                          LigmaForegroundSelectTool *fg_select);

static void   ligma_foreground_select_tool_update_gui     (LigmaForegroundSelectTool *fg_select);

static StrokeUndo * ligma_foreground_select_undo_new      (GeglBuffer               *trimap,
                                                          GArray                   *stroke,
                                                          LigmaMattingDrawMode       draw_mode,
                                                          gint                      stroke_width);
static void         ligma_foreground_select_undo_pop      (StrokeUndo               *undo,
                                                          GeglBuffer               *trimap);
static void         ligma_foreground_select_undo_free     (StrokeUndo               *undo);


G_DEFINE_TYPE (LigmaForegroundSelectTool, ligma_foreground_select_tool,
               LIGMA_TYPE_POLYGON_SELECT_TOOL)

#define parent_class ligma_foreground_select_tool_parent_class


void
ligma_foreground_select_tool_register (LigmaToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (LIGMA_TYPE_FOREGROUND_SELECT_TOOL,
                LIGMA_TYPE_FOREGROUND_SELECT_OPTIONS,
                ligma_foreground_select_options_gui,
                LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND,
                "ligma-foreground-select-tool",
                _("Foreground Select"),
                _("Foreground Select Tool: Select a region containing foreground objects"),
                N_("F_oreground Select"), NULL,
                NULL, LIGMA_HELP_TOOL_FOREGROUND_SELECT,
                LIGMA_ICON_TOOL_FOREGROUND_SELECT,
                data);
}

static void
ligma_foreground_select_tool_class_init (LigmaForegroundSelectToolClass *klass)
{
  GObjectClass               *object_class    = G_OBJECT_CLASS (klass);
  LigmaToolClass              *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass          *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);
  LigmaPolygonSelectToolClass *polygon_select_tool_class;

  polygon_select_tool_class = LIGMA_POLYGON_SELECT_TOOL_CLASS (klass);

  object_class->finalize             = ligma_foreground_select_tool_finalize;

  tool_class->initialize             = ligma_foreground_select_tool_initialize;
  tool_class->control                = ligma_foreground_select_tool_control;
  tool_class->button_press           = ligma_foreground_select_tool_button_press;
  tool_class->button_release         = ligma_foreground_select_tool_button_release;
  tool_class->motion                 = ligma_foreground_select_tool_motion;
  tool_class->key_press              = ligma_foreground_select_tool_key_press;
  tool_class->modifier_key           = ligma_foreground_select_tool_modifier_key;
  tool_class->active_modifier_key    = ligma_foreground_select_tool_active_modifier_key;
  tool_class->oper_update            = ligma_foreground_select_tool_oper_update;
  tool_class->cursor_update          = ligma_foreground_select_tool_cursor_update;
  tool_class->can_undo               = ligma_foreground_select_tool_can_undo;
  tool_class->can_redo               = ligma_foreground_select_tool_can_redo;
  tool_class->undo                   = ligma_foreground_select_tool_undo;
  tool_class->redo                   = ligma_foreground_select_tool_redo;
  tool_class->options_notify         = ligma_foreground_select_tool_options_notify;

  draw_tool_class->draw              = ligma_foreground_select_tool_draw;

  polygon_select_tool_class->confirm = ligma_foreground_select_tool_confirm;
}

static void
ligma_foreground_select_tool_init (LigmaForegroundSelectTool *fg_select)
{
  LigmaTool *tool = LIGMA_TOOL (fg_select);

  ligma_tool_control_set_motion_mode (tool->control, LIGMA_MOTION_MODE_EXACT);
  ligma_tool_control_set_scroll_lock (tool->control, FALSE);
  ligma_tool_control_set_preserve    (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask  (tool->control,
                                     LIGMA_DIRTY_IMAGE_SIZE |
                                     LIGMA_DIRTY_ACTIVE_DRAWABLE);
  ligma_tool_control_set_precision   (tool->control,
                                     LIGMA_CURSOR_PRECISION_SUBPIXEL);
  ligma_tool_control_set_tool_cursor (tool->control,
                                     LIGMA_TOOL_CURSOR_FREE_SELECT);

  ligma_tool_control_set_action_size (tool->control,
                                     "tools/tools-foreground-select-brush-size-set");

  fg_select->state = MATTING_STATE_FREE_SELECT;
  fg_select->grayscale_preview = NULL;
}

static void
ligma_foreground_select_tool_finalize (GObject *object)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (object);

  g_clear_object (&fg_select->gui);
  fg_select->preview_toggle = NULL;

  if (fg_select->stroke)
    g_warning ("%s: stroke should be NULL at this point", G_STRLOC);

  if (fg_select->mask)
    g_warning ("%s: mask should be NULL at this point", G_STRLOC);

  if (fg_select->trimap)
    g_warning ("%s: mask should be NULL at this point", G_STRLOC);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
ligma_foreground_select_tool_initialize (LigmaTool     *tool,
                                        LigmaDisplay  *display,
                                        GError      **error)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);
  LigmaGuiConfig            *config    = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaImage                *image     = ligma_display_get_image (display);
  LigmaDisplayShell         *shell     = ligma_display_get_shell (display);
  GList                    *drawables = ligma_image_get_selected_drawables (image);
  LigmaDrawable             *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                             _("Cannot select from multiple layers."));
      else
        g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED, _("No selected drawables."));

      g_list_free (drawables);
      return FALSE;
    }
  drawable = drawables->data;
  g_list_free (drawables);

  if (! ligma_item_is_visible (LIGMA_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("The active layer is not visible."));
      return FALSE;
    }

  tool->display = display;

  /*  enable double click for the FreeSelectTool, because it may have been
   *  disabled if the tool has switched to MATTING_STATE_PAINT_TRIMAP,
   *  in ligma_foreground_select_tool_set_trimap().
   */
  ligma_tool_control_set_wants_double_click (tool->control, TRUE);

  fg_select->state = MATTING_STATE_FREE_SELECT;

  if (! fg_select->gui)
    {
      fg_select->gui =
        ligma_tool_gui_new (tool->tool_info,
                           NULL,
                           _("Dialog for foreground select"),
                           NULL, NULL,
                           ligma_widget_get_monitor (GTK_WIDGET (shell)),
                           TRUE,

                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                           _("_Select"), GTK_RESPONSE_APPLY,

                           NULL);

      ligma_tool_gui_set_auto_overlay (fg_select->gui, TRUE);

      g_signal_connect (fg_select->gui, "response",
                        G_CALLBACK (ligma_foreground_select_tool_response),
                        fg_select);

      fg_select->preview_toggle =
        gtk_check_button_new_with_mnemonic (_("_Preview mask"));
      gtk_box_pack_start (GTK_BOX (ligma_tool_gui_get_vbox (fg_select->gui)),
                          fg_select->preview_toggle, FALSE, FALSE, 0);
      gtk_widget_show (fg_select->preview_toggle);

      g_signal_connect (fg_select->preview_toggle, "toggled",
                        G_CALLBACK (ligma_foreground_select_tool_preview_toggled),
                        fg_select);
    }

  ligma_tool_gui_set_description (fg_select->gui,
                                 _("Select foreground pixels"));

  ligma_tool_gui_set_response_sensitive (fg_select->gui, GTK_RESPONSE_APPLY,
                                        FALSE);
  gtk_widget_set_sensitive (fg_select->preview_toggle, FALSE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fg_select->preview_toggle),
                                FALSE);

  ligma_tool_gui_set_shell (fg_select->gui, shell);
  ligma_tool_gui_set_viewable (fg_select->gui, LIGMA_VIEWABLE (drawable));

  ligma_tool_gui_show (fg_select->gui);

  return TRUE;
}

static void
ligma_foreground_select_tool_control (LigmaTool       *tool,
                                     LigmaToolAction  action,
                                     LigmaDisplay    *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_foreground_select_tool_halt (fg_select);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_foreground_select_tool_commit (fg_select);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_foreground_select_tool_button_press (LigmaTool            *tool,
                                          const LigmaCoords    *coords,
                                          guint32              time,
                                          GdkModifierType      state,
                                          LigmaButtonPressType  press_type,
                                          LigmaDisplay         *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);
  LigmaDrawTool             *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else
    {
      LigmaVector2 point = ligma_vector2_new (coords->x, coords->y);

      ligma_draw_tool_pause (draw_tool);

      if (ligma_draw_tool_is_active (draw_tool) && draw_tool->display != display)
        ligma_draw_tool_stop (draw_tool);

      ligma_tool_control_activate (tool->control);

      fg_select->last_coords = *coords;

      g_return_if_fail (fg_select->stroke == NULL);
      fg_select->stroke = g_array_new (FALSE, FALSE, sizeof (LigmaVector2));

      g_array_append_val (fg_select->stroke, point);

      if (! ligma_draw_tool_is_active (draw_tool))
        ligma_draw_tool_start (draw_tool, display);

      ligma_draw_tool_resume (draw_tool);
    }
}

static void
ligma_foreground_select_tool_button_release (LigmaTool              *tool,
                                            const LigmaCoords      *coords,
                                            guint32                time,
                                            GdkModifierType        state,
                                            LigmaButtonReleaseType  release_type,
                                            LigmaDisplay           *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
  else
    {
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

      ligma_tool_control_halt (tool->control);

      if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
        {
          ligma_foreground_select_tool_cancel_paint (fg_select);
        }
      else
        {
          ligma_foreground_select_tool_stroke_paint (fg_select);

          if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
            ligma_foreground_select_tool_preview (fg_select);
          else
            ligma_foreground_select_tool_set_trimap (fg_select);
        }

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
    }
}

static void
ligma_foreground_select_tool_motion (LigmaTool         *tool,
                                    const LigmaCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state,
                                    LigmaDisplay      *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      LIGMA_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
  else
    {
      LigmaVector2 *last = &g_array_index (fg_select->stroke,
                                          LigmaVector2,
                                          fg_select->stroke->len - 1);

      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

      fg_select->last_coords = *coords;

      if (last->x != (gint) coords->x || last->y != (gint) coords->y)
        {
          LigmaVector2 point = ligma_vector2_new (coords->x, coords->y);

          g_array_append_val (fg_select->stroke, point);
        }

      ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
    }
}

static gboolean
ligma_foreground_select_tool_key_press (LigmaTool    *tool,
                                       GdkEventKey *kevent,
                                       LigmaDisplay *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      return LIGMA_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
    }
  else
    {
      if (display != tool->display)
        return FALSE;

      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fg_select->preview_toggle),
                                          TRUE);
          else
            ligma_foreground_select_tool_response (fg_select->gui,
                                                  GTK_RESPONSE_APPLY, fg_select);
          return TRUE;

        case GDK_KEY_Escape:
          if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
            ligma_foreground_select_tool_response (fg_select->gui,
                                                  GTK_RESPONSE_CANCEL, fg_select);
          else
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fg_select->preview_toggle),
                                          FALSE);
          return TRUE;

        default:
          return FALSE;
        }
    }
}

static void
ligma_foreground_select_tool_modifier_key (LigmaTool        *tool,
                                          GdkModifierType  key,
                                          gboolean         press,
                                          GdkModifierType  state,
                                          LigmaDisplay     *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      LIGMA_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                    display);
    }
  else
    {
#if 0
      if (key == ligma_get_toggle_behavior_mask ())
        {
          LigmaForegroundSelectOptions *options;

          options = LIGMA_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

          g_object_set (options,
                        "background", ! options->background,
                        NULL);
        }
#endif
    }
}

static void
ligma_foreground_select_tool_active_modifier_key (LigmaTool        *tool,
                                                 GdkModifierType  key,
                                                 gboolean         press,
                                                 GdkModifierType  state,
                                                 LigmaDisplay     *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      LIGMA_TOOL_CLASS (parent_class)->active_modifier_key (tool, key, press,
                                                           state, display);
    }
}

static void
ligma_foreground_select_tool_oper_update (LigmaTool         *tool,
                                         const LigmaCoords *coords,
                                         GdkModifierType   state,
                                         gboolean          proximity,
                                         LigmaDisplay      *display)
{
  LigmaForegroundSelectTool    *fg_select    = LIGMA_FOREGROUND_SELECT_TOOL (tool);
  LigmaForegroundSelectOptions *options;
  const gchar                 *status_stage = NULL;
  const gchar                 *status_mode  = NULL;

  options = LIGMA_FOREGROUND_SELECT_TOOL_GET_OPTIONS (fg_select);

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      if (LIGMA_SELECTION_TOOL (tool)->function == SELECTION_SELECT)
        {
          gint n_points;

          ligma_polygon_select_tool_get_points (LIGMA_POLYGON_SELECT_TOOL (tool),
                                               NULL, &n_points);

          if (n_points > 2)
            {
              status_mode = _("Roughly outline the object to extract");
              status_stage = _("press Enter to refine.");
            }
          else
            {
              status_stage = _("Roughly outline the object to extract");
            }
        }
    }
  else
    {
      LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

      ligma_draw_tool_pause (draw_tool);

      if (proximity)
        {
          fg_select->last_coords = *coords;
        }
      else
        {
          fg_select->last_coords.x = FAR_OUTSIDE;
          fg_select->last_coords.y = FAR_OUTSIDE;
        }

      ligma_draw_tool_resume (draw_tool);

      if (options->draw_mode == LIGMA_MATTING_DRAW_MODE_FOREGROUND)
        status_mode = _("Selecting foreground");
      else if (options->draw_mode == LIGMA_MATTING_DRAW_MODE_BACKGROUND)
        status_mode = _("Selecting background");
      else
        status_mode = _("Selecting unknown");

      if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
        status_stage = _("press Enter to preview.");
      else
        status_stage = _("press Escape to exit preview or Enter to apply.");
    }

  if (proximity && status_stage)
    {
      if (status_mode)
        ligma_tool_replace_status (tool, display, "%s, %s", status_mode, status_stage);
      else
        ligma_tool_replace_status (tool, display, "%s", status_stage);
    }
}

static void
ligma_foreground_select_tool_cursor_update (LigmaTool         *tool,
                                           const LigmaCoords *coords,
                                           GdkModifierType   state,
                                           LigmaDisplay      *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      switch (LIGMA_SELECTION_TOOL (tool)->function)
        {
        case SELECTION_MOVE_MASK:
        case SELECTION_MOVE:
        case SELECTION_MOVE_COPY:
        case SELECTION_ANCHOR:
          return;
        default:
          break;
        }
    }

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static const gchar *
ligma_foreground_select_tool_can_undo (LigmaTool    *tool,
                                      LigmaDisplay *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->undo_stack)
    {
      StrokeUndo  *undo = fg_select->undo_stack->data;
      const gchar *desc;

      if (ligma_enum_get_value (LIGMA_TYPE_MATTING_DRAW_MODE, undo->draw_mode,
                               NULL, NULL, &desc, NULL))
        {
          return desc;
        }
    }

  return NULL;
}

static const gchar *
ligma_foreground_select_tool_can_redo (LigmaTool    *tool,
                                      LigmaDisplay *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->redo_stack)
    {
      StrokeUndo  *undo = fg_select->redo_stack->data;
      const gchar *desc;

      if (ligma_enum_get_value (LIGMA_TYPE_MATTING_DRAW_MODE, undo->draw_mode,
                               NULL, NULL, &desc, NULL))
        {
          return desc;
        }
    }

  return NULL;
}

static gboolean
ligma_foreground_select_tool_undo (LigmaTool    *tool,
                                  LigmaDisplay *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);
  StrokeUndo               *undo      = fg_select->undo_stack->data;

  ligma_foreground_select_undo_pop (undo, fg_select->trimap);

  fg_select->undo_stack = g_list_remove (fg_select->undo_stack, undo);
  fg_select->redo_stack = g_list_prepend (fg_select->redo_stack, undo);

  if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
    ligma_foreground_select_tool_preview (fg_select);
  else
    ligma_foreground_select_tool_set_trimap (fg_select);

  return TRUE;
}

static gboolean
ligma_foreground_select_tool_redo (LigmaTool    *tool,
                                  LigmaDisplay *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);
  StrokeUndo               *undo      = fg_select->redo_stack->data;

  ligma_foreground_select_undo_pop (undo, fg_select->trimap);

  fg_select->redo_stack = g_list_remove (fg_select->redo_stack, undo);
  fg_select->undo_stack = g_list_prepend (fg_select->undo_stack, undo);

  if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
    ligma_foreground_select_tool_preview (fg_select);
  else
    ligma_foreground_select_tool_set_trimap (fg_select);

  return TRUE;
}

static void
ligma_foreground_select_tool_options_notify (LigmaTool         *tool,
                                            LigmaToolOptions  *options,
                                            const GParamSpec *pspec)
{
  LigmaForegroundSelectTool    *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);
  LigmaForegroundSelectOptions *fg_options;

  fg_options = LIGMA_FOREGROUND_SELECT_OPTIONS (options);

  if (! tool->display)
    return;

  if (! strcmp (pspec->name, "mask-color") ||
      ! strcmp (pspec->name, "preview-mode"))
    {
      if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
        {
          ligma_foreground_select_tool_set_trimap (fg_select);
        }
      else if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
        {
          ligma_foreground_select_tool_set_preview (fg_select);
        }
    }
  else if (! strcmp (pspec->name, "engine"))
    {
      if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
        {
          ligma_foreground_select_tool_preview (fg_select);
        }
    }
  else if (! strcmp (pspec->name, "iterations"))
    {
      if (fg_options->engine == LIGMA_MATTING_ENGINE_GLOBAL &&
          fg_select->state   == MATTING_STATE_PREVIEW_MASK)
        {
          ligma_foreground_select_tool_preview (fg_select);
        }
    }
  else if (! strcmp (pspec->name, "levels") ||
           ! strcmp (pspec->name, "active-levels"))
    {
      if (fg_options->engine == LIGMA_MATTING_ENGINE_LEVIN &&
          fg_select->state   == MATTING_STATE_PREVIEW_MASK)
        {
          ligma_foreground_select_tool_preview (fg_select);
        }
    }
}

static void
ligma_foreground_select_tool_get_area (GeglBuffer *mask,
                                      gint       *x1,
                                      gint       *y1,
                                      gint       *x2,
                                      gint       *y2)
{
  gint width;
  gint height;

  ligma_gegl_mask_bounds (mask, x1, y1, x2, y2);

  width  = *x2 - *x1;
  height = *y2 - *y1;

  *x1 = MAX (*x1 - width  / 2, 0);
  *y1 = MAX (*y1 - height / 2, 0);
  *x2 = MIN (*x2 + width  / 2, ligma_item_get_width  (LIGMA_ITEM (mask)));
  *y2 = MIN (*y2 + height / 2, ligma_item_get_height (LIGMA_ITEM (mask)));
}

static void
ligma_foreground_select_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaTool                    *tool      = LIGMA_TOOL (draw_tool);
  LigmaForegroundSelectTool    *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (tool);
  LigmaForegroundSelectOptions *options;

  options = LIGMA_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      LIGMA_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
      return;
    }
  else
    {
      gint    x      = fg_select->last_coords.x;
      gint    y      = fg_select->last_coords.y;
      gdouble radius = options->stroke_width / 2.0f;

      if (fg_select->stroke)
        {
          LigmaDisplayShell *shell = ligma_display_get_shell (draw_tool->display);

          ligma_draw_tool_add_pen (draw_tool,
                                  (const LigmaVector2 *) fg_select->stroke->data,
                                  fg_select->stroke->len,
                                  LIGMA_CONTEXT (options),
                                  LIGMA_ACTIVE_COLOR_FOREGROUND,
                                  options->stroke_width * shell->scale_y);
        }

      /*  warn if the user is drawing outside of the working area  */
      if (FALSE)
        {
          gint x1, y1;
          gint x2, y2;

          ligma_foreground_select_tool_get_area (fg_select->mask,
                                                &x1, &y1, &x2, &y2);

          if (x < x1 + radius || x > x2 - radius ||
              y < y1 + radius || y > y2 - radius)
            {
              ligma_draw_tool_add_rectangle (draw_tool, FALSE,
                                            x1, y1,
                                            x2 - x1, y2 - y1);
            }
        }

      if (x > FAR_OUTSIDE && y > FAR_OUTSIDE)
        ligma_draw_tool_add_arc (draw_tool, FALSE,
                                x - radius, y - radius,
                                2 * radius, 2 * radius,
                                0.0, 2.0 * G_PI);

      if (fg_select->grayscale_preview)
        ligma_draw_tool_add_preview (draw_tool, fg_select->grayscale_preview);
    }
}

static void
ligma_foreground_select_tool_confirm (LigmaPolygonSelectTool *poly_sel,
                                     LigmaDisplay           *display)
{
  LigmaForegroundSelectTool *fg_select = LIGMA_FOREGROUND_SELECT_TOOL (poly_sel);
  LigmaImage                *image     = ligma_display_get_image (display);
  GList                    *drawables = ligma_image_get_selected_drawables (image);
  LigmaDrawable             *drawable;
  LigmaItem                 *item;

  g_return_if_fail (g_list_length (drawables) == 1);

  drawable = drawables->data;
  item     = LIGMA_ITEM (drawable);
  g_list_free (drawables);

  if (drawable && fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      LigmaScanConvert   *scan_convert = ligma_scan_convert_new ();
      const LigmaVector2 *points;
      gint               n_points;

      ligma_polygon_select_tool_get_points (poly_sel, &points, &n_points);

      ligma_scan_convert_add_polyline (scan_convert, n_points, points, TRUE);

      fg_select->trimap =
        gegl_buffer_new (GEGL_RECTANGLE (ligma_item_get_offset_x (item),
                                         ligma_item_get_offset_y (item),
                                         ligma_item_get_width    (item),
                                         ligma_item_get_height   (item)),
                         ligma_image_get_mask_format (image));

      ligma_scan_convert_render_value (scan_convert, fg_select->trimap,
                                      0, 0, 0.5);
      ligma_scan_convert_free (scan_convert);

      fg_select->grayscale_preview =
          ligma_canvas_buffer_preview_new (ligma_display_get_shell (display),
                                          fg_select->trimap);

      ligma_foreground_select_tool_set_trimap (fg_select);
    }
}

static void
ligma_foreground_select_tool_halt (LigmaForegroundSelectTool *fg_select)
{
  LigmaTool     *tool = LIGMA_TOOL (fg_select);
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (fg_select);

  if (draw_tool->preview)
    {
      ligma_draw_tool_remove_preview (draw_tool, fg_select->grayscale_preview);
    }

  g_clear_object (&fg_select->grayscale_preview);
  g_clear_object (&fg_select->trimap);
  g_clear_object (&fg_select->mask);

  if (fg_select->undo_stack)
    {
      g_list_free_full (fg_select->undo_stack,
                        (GDestroyNotify) ligma_foreground_select_undo_free);
      fg_select->undo_stack = NULL;
    }

  if (fg_select->redo_stack)
    {
      g_list_free_full (fg_select->redo_stack,
                        (GDestroyNotify) ligma_foreground_select_undo_free);
      fg_select->redo_stack = NULL;
    }

  if (tool->display)
    ligma_display_shell_set_mask (ligma_display_get_shell (tool->display),
                                 NULL, 0, 0, NULL, FALSE);

  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_FREE_SELECT);
  ligma_tool_control_set_toggle_tool_cursor (tool->control,
                                            LIGMA_TOOL_CURSOR_FREE_SELECT);

  ligma_tool_control_set_toggled (tool->control, FALSE);

  /*  set precision to SUBPIXEL, because it may have been changed to
   *  PIXEL_CENTER if the tool has switched to MATTING_STATE_PAINT_TRIMAP,
   *  in ligma_foreground_select_tool_set_trimap().
   */
  ligma_tool_control_set_precision (tool->control,
                                   LIGMA_CURSOR_PRECISION_SUBPIXEL);

  fg_select->state = MATTING_STATE_FREE_SELECT;

  /*  update the undo actions / menu items  */
  if (tool->display)
    ligma_image_flush (ligma_display_get_image (tool->display));

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (fg_select->gui)
    ligma_tool_gui_hide (fg_select->gui);
}

static void
ligma_foreground_select_tool_commit (LigmaForegroundSelectTool *fg_select)
{
  LigmaTool             *tool    = LIGMA_TOOL (fg_select);
  LigmaSelectionOptions *options = LIGMA_SELECTION_TOOL_GET_OPTIONS (fg_select);

  if (tool->display && fg_select->state != MATTING_STATE_FREE_SELECT)
    {
      LigmaImage *image = ligma_display_get_image (tool->display);

      if (fg_select->state != MATTING_STATE_PREVIEW_MASK)
        ligma_foreground_select_tool_preview (fg_select);

      ligma_channel_select_buffer (ligma_image_get_mask (image),
                                  C_("command", "Foreground Select"),
                                  fg_select->mask,
                                  0, /* x offset */
                                  0, /* y offset */
                                  options->operation,
                                  options->feather,
                                  options->feather_radius,
                                  options->feather_radius);

      ligma_image_flush (image);
    }
}

static void
ligma_foreground_select_tool_set_trimap (LigmaForegroundSelectTool *fg_select)
{
  LigmaTool                    *tool = LIGMA_TOOL (fg_select);
  LigmaForegroundSelectOptions *options;

  g_return_if_fail (fg_select->trimap != NULL);

  options = LIGMA_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  ligma_polygon_select_tool_halt (LIGMA_POLYGON_SELECT_TOOL (fg_select));

  if (options->preview_mode == LIGMA_MATTING_PREVIEW_MODE_ON_COLOR)
    {
      if (fg_select->grayscale_preview)
        ligma_canvas_item_set_visible (fg_select->grayscale_preview, FALSE);

      ligma_display_shell_set_mask (ligma_display_get_shell (tool->display),
                                   fg_select->trimap, 0, 0,
                                   &options->mask_color, TRUE);
    }
  else
    {
      ligma_display_shell_set_mask (ligma_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);

      if (fg_select->grayscale_preview)
        {
          g_object_set (fg_select->grayscale_preview, "buffer",
                        fg_select->trimap, NULL);

          ligma_canvas_item_set_visible (fg_select->grayscale_preview, TRUE);
        }
    }

  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_PAINTBRUSH);
  ligma_tool_control_set_toggle_tool_cursor (tool->control,
                                            LIGMA_TOOL_CURSOR_PAINTBRUSH);

  ligma_tool_control_set_toggled (tool->control, FALSE);

  /* disable double click in paint trimap state */
  ligma_tool_control_set_wants_double_click (tool->control, FALSE);

  /* set precision to PIXEL_CENTER in paint trimap state */
  ligma_tool_control_set_precision (tool->control,
                                   LIGMA_CURSOR_PRECISION_PIXEL_CENTER);

  fg_select->state = MATTING_STATE_PAINT_TRIMAP;

  ligma_foreground_select_tool_update_gui (fg_select);
}

static void
ligma_foreground_select_tool_set_preview (LigmaForegroundSelectTool *fg_select)
{

  LigmaTool                    *tool = LIGMA_TOOL (fg_select);
  LigmaForegroundSelectOptions *options;

  g_return_if_fail (fg_select->mask != NULL);

  options = LIGMA_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  if (options->preview_mode == LIGMA_MATTING_PREVIEW_MODE_ON_COLOR)
    {
      if (fg_select->grayscale_preview)
        ligma_canvas_item_set_visible (fg_select->grayscale_preview, FALSE);

      ligma_display_shell_set_mask (ligma_display_get_shell (tool->display),
                                   fg_select->mask, 0, 0,
                                   &options->mask_color, TRUE);
    }
  else
    {
      ligma_display_shell_set_mask (ligma_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);

      if (fg_select->grayscale_preview)
        {
          g_object_set (fg_select->grayscale_preview, "buffer",
                    fg_select->mask, NULL);
          ligma_canvas_item_set_visible (fg_select->grayscale_preview, TRUE);
        }
    }

  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_PAINTBRUSH);
  ligma_tool_control_set_toggle_tool_cursor (tool->control,
                                            LIGMA_TOOL_CURSOR_PAINTBRUSH);

  ligma_tool_control_set_toggled (tool->control, FALSE);

  fg_select->state = MATTING_STATE_PREVIEW_MASK;

  ligma_foreground_select_tool_update_gui (fg_select);
}

static void
ligma_foreground_select_tool_preview (LigmaForegroundSelectTool *fg_select)
{
  LigmaTool                    *tool      = LIGMA_TOOL (fg_select);
  LigmaForegroundSelectOptions *options;
  LigmaImage                   *image     = ligma_display_get_image (tool->display);
  GList                       *drawables = ligma_image_get_selected_drawables (image);
  LigmaDrawable                *drawable;

  g_return_if_fail (g_list_length (drawables) == 1);

  drawable = drawables->data;
  g_list_free (drawables);

  options  = LIGMA_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  g_clear_object (&fg_select->mask);

  fg_select->mask = ligma_drawable_foreground_extract (drawable,
                                                      options->engine,
                                                      options->iterations,
                                                      options->levels,
                                                      options->active_levels,
                                                      fg_select->trimap,
                                                      LIGMA_PROGRESS (fg_select));

  ligma_foreground_select_tool_set_preview (fg_select);
}

static void
ligma_foreground_select_tool_stroke_paint (LigmaForegroundSelectTool *fg_select)
{
  LigmaForegroundSelectOptions *options;
  LigmaScanConvert             *scan_convert;
  StrokeUndo                  *undo;
  gint                         width;
  gdouble                      opacity;

  options = LIGMA_FOREGROUND_SELECT_TOOL_GET_OPTIONS (fg_select);

  g_return_if_fail (fg_select->stroke != NULL);

  width = ROUND ((gdouble) options->stroke_width);

  if (fg_select->redo_stack)
    {
      g_list_free_full (fg_select->redo_stack,
                        (GDestroyNotify) ligma_foreground_select_undo_free);
      fg_select->redo_stack = NULL;
    }

  undo = ligma_foreground_select_undo_new (fg_select->trimap,
                                          fg_select->stroke,
                                          options->draw_mode, width);
  if (! undo)
    {
      g_array_free (fg_select->stroke, TRUE);
      fg_select->stroke = NULL;
      return;
    }

  fg_select->undo_stack = g_list_prepend (fg_select->undo_stack, undo);

  scan_convert = ligma_scan_convert_new ();

  if (fg_select->stroke->len == 1)
    {
      LigmaVector2 points[2];

      points[0] = points[1] = ((LigmaVector2 *) fg_select->stroke->data)[0];

      points[1].x += 0.01;
      points[1].y += 0.01;

      ligma_scan_convert_add_polyline (scan_convert, 2, points, FALSE);
    }
  else
    {
      ligma_scan_convert_add_polyline (scan_convert,
                                      fg_select->stroke->len,
                                      (LigmaVector2 *) fg_select->stroke->data,
                                      FALSE);
    }

  ligma_scan_convert_stroke (scan_convert,
                            width,
                            LIGMA_JOIN_ROUND, LIGMA_CAP_ROUND, 10.0,
                            0.0, NULL);

  if (options->draw_mode == LIGMA_MATTING_DRAW_MODE_FOREGROUND)
    opacity = 1.0;
  else if (options->draw_mode == LIGMA_MATTING_DRAW_MODE_BACKGROUND)
    opacity = 0.0;
  else
    opacity = 0.5;

  ligma_scan_convert_compose_value (scan_convert, fg_select->trimap,
                                   0, 0,
                                   opacity);

  ligma_scan_convert_free (scan_convert);

  g_array_free (fg_select->stroke, TRUE);
  fg_select->stroke = NULL;

  /*  update the undo actions / menu items  */
  ligma_image_flush (ligma_display_get_image (LIGMA_TOOL (fg_select)->display));
}

static void
ligma_foreground_select_tool_cancel_paint (LigmaForegroundSelectTool *fg_select)
{
  g_return_if_fail (fg_select->stroke != NULL);

  g_array_free (fg_select->stroke, TRUE);
  fg_select->stroke = NULL;
}

static void
ligma_foreground_select_tool_response (LigmaToolGui              *gui,
                                      gint                      response_id,
                                      LigmaForegroundSelectTool *fg_select)
{
  LigmaTool *tool = LIGMA_TOOL (fg_select);

  switch (response_id)
    {
    case GTK_RESPONSE_APPLY:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, tool->display);
      break;

    default:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
ligma_foreground_select_tool_preview_toggled (GtkToggleButton          *button,
                                             LigmaForegroundSelectTool *fg_select)
{
  if (fg_select->state != MATTING_STATE_FREE_SELECT)
    {
      if (gtk_toggle_button_get_active (button))
        {
          if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
            ligma_foreground_select_tool_preview (fg_select);
        }
      else
        {
          if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
            ligma_foreground_select_tool_set_trimap (fg_select);
        }
    }
}

static void
ligma_foreground_select_tool_update_gui (LigmaForegroundSelectTool *fg_select)
{
  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      ligma_tool_gui_set_description (fg_select->gui, _("Paint mask"));
    }
  else if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
    {
      ligma_tool_gui_set_description (fg_select->gui, _("Preview"));
    }

  ligma_tool_gui_set_response_sensitive (fg_select->gui, GTK_RESPONSE_APPLY,
                                        TRUE);
  gtk_widget_set_sensitive (fg_select->preview_toggle, TRUE);
}

static StrokeUndo *
ligma_foreground_select_undo_new (GeglBuffer          *trimap,
                                 GArray              *stroke,
                                 LigmaMattingDrawMode draw_mode,
                                 gint                stroke_width)

{
  StrokeUndo          *undo;
  const GeglRectangle *extent;
  gint                 x1, y1, x2, y2;
  gint                 width, height;
  gint                 i;

  extent = gegl_buffer_get_extent (trimap);

  x1 = G_MAXINT;
  y1 = G_MAXINT;
  x2 = G_MININT;
  y2 = G_MININT;

  for (i = 0; i < stroke->len; i++)
    {
      LigmaVector2 *point = &g_array_index (stroke, LigmaVector2, i);

      x1 = MIN (x1, floor (point->x));
      y1 = MIN (y1, floor (point->y));
      x2 = MAX (x2, ceil (point->x));
      y2 = MAX (y2, ceil (point->y));
    }

  x1 -= (stroke_width + 1) / 2;
  y1 -= (stroke_width + 1) / 2;
  x2 += (stroke_width + 1) / 2;
  y2 += (stroke_width + 1) / 2;

  x1 = MAX (x1, extent->x);
  y1 = MAX (y1, extent->y);
  x2 = MIN (x2, extent->x + extent->width);
  y2 = MIN (y2, extent->x + extent->height);

  width  = x2 - x1;
  height = y2 - y1;

  if (width <= 0 || height <= 0)
    return NULL;

  undo = g_slice_new0 (StrokeUndo);
  undo->saved_trimap = gegl_buffer_new (GEGL_RECTANGLE (x1, y1, width, height),
                                        gegl_buffer_get_format (trimap));

  ligma_gegl_buffer_copy (
    trimap,             GEGL_RECTANGLE (x1, y1, width, height),
    GEGL_ABYSS_NONE,
    undo->saved_trimap, NULL);

  undo->trimap_x = x1;
  undo->trimap_y = y1;

  undo->draw_mode    = draw_mode;
  undo->stroke_width = stroke_width;

  return undo;
}

static void
ligma_foreground_select_undo_pop (StrokeUndo *undo,
                                 GeglBuffer *trimap)
{
  GeglBuffer *buffer;
  gint        width, height;

  buffer = ligma_gegl_buffer_dup (undo->saved_trimap);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  ligma_gegl_buffer_copy (trimap,
                         GEGL_RECTANGLE (undo->trimap_x, undo->trimap_y,
                                         width, height),
                         GEGL_ABYSS_NONE,
                         undo->saved_trimap, NULL);

  ligma_gegl_buffer_copy (buffer,
                         GEGL_RECTANGLE (undo->trimap_x, undo->trimap_y,
                                         width, height),
                         GEGL_ABYSS_NONE,
                         trimap, NULL);

  g_object_unref (buffer);
}

static void
ligma_foreground_select_undo_free (StrokeUndo *undo)
{
  g_clear_object (&undo->saved_trimap);

  g_slice_free (StrokeUndo, undo);
}
