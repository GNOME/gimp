/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpForegroundSelectTool
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-mask.h"
#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimpchannel-select.h"
#include "core/gimpdrawable-foreground-extract.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpprogress.h"
#include "core/gimpscanconvert.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasitem.h"
#include "display/gimpcanvasbufferpreview.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimptoolgui.h"

#include "gimpforegroundselecttool.h"
#include "gimpforegroundselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define FAR_OUTSIDE -10000


typedef struct _StrokeUndo StrokeUndo;

struct _StrokeUndo
{
  GeglBuffer          *saved_trimap;
  gint                 trimap_x;
  gint                 trimap_y;
  GimpMattingDrawMode  draw_mode;
  gint                 stroke_width;
};


static void   gimp_foreground_select_tool_finalize       (GObject          *object);

static gboolean  gimp_foreground_select_tool_initialize  (GimpTool         *tool,
                                                          GimpDisplay      *display,
                                                          GError          **error);
static void   gimp_foreground_select_tool_control        (GimpTool         *tool,
                                                          GimpToolAction    action,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_button_press   (GimpTool         *tool,
                                                          const GimpCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          GimpButtonPressType press_type,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_button_release (GimpTool         *tool,
                                                          const GimpCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          GimpButtonReleaseType release_type,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_motion         (GimpTool         *tool,
                                                          const GimpCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          GimpDisplay      *display);
static gboolean  gimp_foreground_select_tool_key_press   (GimpTool         *tool,
                                                          GdkEventKey      *kevent,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_modifier_key   (GimpTool         *tool,
                                                          GdkModifierType   key,
                                                          gboolean          press,
                                                          GdkModifierType   state,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_active_modifier_key
                                                         (GimpTool         *tool,
                                                          GdkModifierType   key,
                                                          gboolean          press,
                                                          GdkModifierType   state,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_oper_update    (GimpTool         *tool,
                                                          const GimpCoords *coords,
                                                          GdkModifierType   state,
                                                          gboolean          proximity,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_cursor_update  (GimpTool         *tool,
                                                          const GimpCoords *coords,
                                                          GdkModifierType   state,
                                                          GimpDisplay      *display);
static const gchar * gimp_foreground_select_tool_can_undo
                                                         (GimpTool         *tool,
                                                          GimpDisplay      *display);
static const gchar * gimp_foreground_select_tool_can_redo
                                                         (GimpTool         *tool,
                                                          GimpDisplay      *display);
static gboolean   gimp_foreground_select_tool_undo       (GimpTool         *tool,
                                                          GimpDisplay      *display);
static gboolean   gimp_foreground_select_tool_redo       (GimpTool         *tool,
                                                          GimpDisplay      *display);
static void   gimp_foreground_select_tool_options_notify (GimpTool         *tool,
                                                          GimpToolOptions  *options,
                                                          const GParamSpec *pspec);

static void   gimp_foreground_select_tool_draw           (GimpDrawTool     *draw_tool);

static void   gimp_foreground_select_tool_confirm        (GimpPolygonSelectTool *poly_sel,
                                                          GimpDisplay           *display);

static void   gimp_foreground_select_tool_halt           (GimpForegroundSelectTool *fg_select);
static void   gimp_foreground_select_tool_commit         (GimpForegroundSelectTool *fg_select);

static void   gimp_foreground_select_tool_set_trimap     (GimpForegroundSelectTool *fg_select);
static void   gimp_foreground_select_tool_set_preview    (GimpForegroundSelectTool *fg_select);
static void   gimp_foreground_select_tool_preview        (GimpForegroundSelectTool *fg_select);

static void   gimp_foreground_select_tool_stroke_paint   (GimpForegroundSelectTool *fg_select);
static void   gimp_foreground_select_tool_cancel_paint   (GimpForegroundSelectTool *fg_select);

static void   gimp_foreground_select_tool_response       (GimpToolGui              *gui,
                                                          gint                      response_id,
                                                          GimpForegroundSelectTool *fg_select);
static void   gimp_foreground_select_tool_preview_toggled(GtkToggleButton          *button,
                                                          GimpForegroundSelectTool *fg_select);

static void   gimp_foreground_select_tool_update_gui     (GimpForegroundSelectTool *fg_select);

static gboolean  gimp_foreground_select_tool_has_strokes (GimpForegroundSelectTool *fg_select);

static StrokeUndo * gimp_foreground_select_undo_new      (GeglBuffer               *trimap,
                                                          GArray                   *stroke,
                                                          GimpMattingDrawMode       draw_mode,
                                                          gint                      stroke_width);
static void         gimp_foreground_select_undo_pop      (StrokeUndo               *undo,
                                                          GeglBuffer               *trimap);
static void         gimp_foreground_select_undo_free     (StrokeUndo               *undo);


G_DEFINE_TYPE (GimpForegroundSelectTool, gimp_foreground_select_tool,
               GIMP_TYPE_POLYGON_SELECT_TOOL)

#define parent_class gimp_foreground_select_tool_parent_class


void
gimp_foreground_select_tool_register (GimpToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (GIMP_TYPE_FOREGROUND_SELECT_TOOL,
                GIMP_TYPE_FOREGROUND_SELECT_OPTIONS,
                gimp_foreground_select_options_gui,
                GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                "gimp-foreground-select-tool",
                _("Foreground Select"),
                _("Foreground Select Tool: Select a region containing foreground objects"),
                N_("F_oreground Select"), NULL,
                NULL, GIMP_HELP_TOOL_FOREGROUND_SELECT,
                GIMP_ICON_TOOL_FOREGROUND_SELECT,
                data);
}

static void
gimp_foreground_select_tool_class_init (GimpForegroundSelectToolClass *klass)
{
  GObjectClass               *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass              *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass          *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  GimpPolygonSelectToolClass *polygon_select_tool_class;

  polygon_select_tool_class = GIMP_POLYGON_SELECT_TOOL_CLASS (klass);

  object_class->finalize             = gimp_foreground_select_tool_finalize;

  tool_class->initialize             = gimp_foreground_select_tool_initialize;
  tool_class->control                = gimp_foreground_select_tool_control;
  tool_class->button_press           = gimp_foreground_select_tool_button_press;
  tool_class->button_release         = gimp_foreground_select_tool_button_release;
  tool_class->motion                 = gimp_foreground_select_tool_motion;
  tool_class->key_press              = gimp_foreground_select_tool_key_press;
  tool_class->modifier_key           = gimp_foreground_select_tool_modifier_key;
  tool_class->active_modifier_key    = gimp_foreground_select_tool_active_modifier_key;
  tool_class->oper_update            = gimp_foreground_select_tool_oper_update;
  tool_class->cursor_update          = gimp_foreground_select_tool_cursor_update;
  tool_class->can_undo               = gimp_foreground_select_tool_can_undo;
  tool_class->can_redo               = gimp_foreground_select_tool_can_redo;
  tool_class->undo                   = gimp_foreground_select_tool_undo;
  tool_class->redo                   = gimp_foreground_select_tool_redo;
  tool_class->options_notify         = gimp_foreground_select_tool_options_notify;

  draw_tool_class->draw              = gimp_foreground_select_tool_draw;

  polygon_select_tool_class->confirm = gimp_foreground_select_tool_confirm;
}

static void
gimp_foreground_select_tool_init (GimpForegroundSelectTool *fg_select)
{
  GimpTool *tool = GIMP_TOOL (fg_select);

  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_scroll_lock (tool->control, FALSE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE_SIZE |
                                     GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_FREE_SELECT);

  gimp_tool_control_set_action_size (tool->control,
                                     "tools-foreground-select-brush-size-set");

  fg_select->state = MATTING_STATE_FREE_SELECT;
  fg_select->grayscale_preview = NULL;
}

static void
gimp_foreground_select_tool_finalize (GObject *object)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (object);

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
gimp_foreground_select_tool_initialize (GimpTool     *tool,
                                        GimpDisplay  *display,
                                        GError      **error)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  GimpGuiConfig            *config    = GIMP_GUI_CONFIG (display->gimp->config);
  GimpImage                *image     = gimp_display_get_image (display);
  GimpDisplayShell         *shell     = gimp_display_get_shell (display);
  GList                    *drawables = gimp_image_get_selected_drawables (image);
  GimpDrawable             *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                             _("Cannot select from multiple layers."));
      else
        g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED, _("No selected drawables."));

      g_list_free (drawables);
      return FALSE;
    }
  drawable = drawables->data;
  g_list_free (drawables);

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The active layer is not visible."));
      return FALSE;
    }

  tool->display = display;

  /*  enable double click for the FreeSelectTool, because it may have been
   *  disabled if the tool has switched to MATTING_STATE_PAINT_TRIMAP,
   *  in gimp_foreground_select_tool_set_trimap().
   */
  gimp_tool_control_set_wants_double_click (tool->control, TRUE);

  fg_select->state = MATTING_STATE_FREE_SELECT;

  if (! fg_select->gui)
    {
      fg_select->gui =
        gimp_tool_gui_new (tool->tool_info,
                           NULL,
                           _("Dialog for foreground select"),
                           NULL, NULL,
                           gimp_widget_get_monitor (GTK_WIDGET (shell)),
                           TRUE,

                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                           _("_Select"), GTK_RESPONSE_APPLY,

                           NULL);

      gimp_tool_gui_set_auto_overlay (fg_select->gui, TRUE);

      g_signal_connect (fg_select->gui, "response",
                        G_CALLBACK (gimp_foreground_select_tool_response),
                        fg_select);

      fg_select->preview_toggle =
        gtk_check_button_new_with_mnemonic (_("_Preview mask"));
      gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (fg_select->gui)),
                          fg_select->preview_toggle, FALSE, FALSE, 0);
      gtk_widget_show (fg_select->preview_toggle);

      g_signal_connect (fg_select->preview_toggle, "toggled",
                        G_CALLBACK (gimp_foreground_select_tool_preview_toggled),
                        fg_select);
    }

  gimp_tool_gui_set_description (fg_select->gui,
                                 _("Select foreground pixels"));

  gimp_tool_gui_set_response_sensitive (fg_select->gui, GTK_RESPONSE_APPLY,
                                        FALSE);
  gtk_widget_set_sensitive (fg_select->preview_toggle, FALSE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fg_select->preview_toggle),
                                FALSE);

  gimp_tool_gui_set_shell (fg_select->gui, shell);
  gimp_tool_gui_set_viewable (fg_select->gui, GIMP_VIEWABLE (drawable));

  gimp_tool_gui_show (fg_select->gui);

  return TRUE;
}

static void
gimp_foreground_select_tool_control (GimpTool       *tool,
                                     GimpToolAction  action,
                                     GimpDisplay    *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_foreground_select_tool_halt (fg_select);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_foreground_select_tool_commit (fg_select);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_foreground_select_tool_button_press (GimpTool            *tool,
                                          const GimpCoords    *coords,
                                          guint32              time,
                                          GdkModifierType      state,
                                          GimpButtonPressType  press_type,
                                          GimpDisplay         *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  GimpDrawTool             *draw_tool = GIMP_DRAW_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else
    {
      GimpVector2 point = gimp_vector2_new (coords->x, coords->y);

      gimp_draw_tool_pause (draw_tool);

      if (gimp_draw_tool_is_active (draw_tool) && draw_tool->display != display)
        gimp_draw_tool_stop (draw_tool);

      gimp_tool_control_activate (tool->control);

      fg_select->last_coords = *coords;

      g_return_if_fail (fg_select->stroke == NULL);
      fg_select->stroke = g_array_new (FALSE, FALSE, sizeof (GimpVector2));

      g_array_append_val (fg_select->stroke, point);

      if (! gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_start (draw_tool, display);

      gimp_draw_tool_resume (draw_tool);
    }
}

static void
gimp_foreground_select_tool_button_release (GimpTool              *tool,
                                            const GimpCoords      *coords,
                                            guint32                time,
                                            GdkModifierType        state,
                                            GimpButtonReleaseType  release_type,
                                            GimpDisplay           *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
  else
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      gimp_tool_control_halt (tool->control);

      if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
        {
          gimp_foreground_select_tool_cancel_paint (fg_select);
        }
      else
        {
          gimp_foreground_select_tool_stroke_paint (fg_select);

          if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
            gimp_foreground_select_tool_preview (fg_select);
          else
            gimp_foreground_select_tool_set_trimap (fg_select);
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_foreground_select_tool_motion (GimpTool         *tool,
                                    const GimpCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state,
                                    GimpDisplay      *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
  else
    {
      GimpVector2 *last = &g_array_index (fg_select->stroke,
                                          GimpVector2,
                                          fg_select->stroke->len - 1);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      fg_select->last_coords = *coords;

      if (last->x != (gint) coords->x || last->y != (gint) coords->y)
        {
          GimpVector2 point = gimp_vector2_new (coords->x, coords->y);

          g_array_append_val (fg_select->stroke, point);
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static gboolean
gimp_foreground_select_tool_key_press (GimpTool    *tool,
                                       GdkEventKey *kevent,
                                       GimpDisplay *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      return GIMP_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
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
            gimp_foreground_select_tool_response (fg_select->gui,
                                                  GTK_RESPONSE_APPLY, fg_select);
          return TRUE;

        case GDK_KEY_Escape:
          if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
            gimp_foreground_select_tool_response (fg_select->gui,
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
gimp_foreground_select_tool_modifier_key (GimpTool        *tool,
                                          GdkModifierType  key,
                                          gboolean         press,
                                          GdkModifierType  state,
                                          GimpDisplay     *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                    display);
    }
  else
    {
#if 0
      if (key == gimp_get_toggle_behavior_mask ())
        {
          GimpForegroundSelectOptions *options;

          options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

          g_object_set (options,
                        "background", ! options->background,
                        NULL);
        }
#endif
    }
}

static void
gimp_foreground_select_tool_active_modifier_key (GimpTool        *tool,
                                                 GdkModifierType  key,
                                                 gboolean         press,
                                                 GdkModifierType  state,
                                                 GimpDisplay     *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GIMP_TOOL_CLASS (parent_class)->active_modifier_key (tool, key, press,
                                                           state, display);
    }
}

static void
gimp_foreground_select_tool_oper_update (GimpTool         *tool,
                                         const GimpCoords *coords,
                                         GdkModifierType   state,
                                         gboolean          proximity,
                                         GimpDisplay      *display)
{
  GimpForegroundSelectTool    *fg_select    = GIMP_FOREGROUND_SELECT_TOOL (tool);
  GimpForegroundSelectOptions *options;
  const gchar                 *status_stage = NULL;
  const gchar                 *status_mode  = NULL;

  options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (fg_select);

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      if (GIMP_SELECTION_TOOL (tool)->function == SELECTION_SELECT)
        {
          gint n_points;

          gimp_polygon_select_tool_get_points (GIMP_POLYGON_SELECT_TOOL (tool),
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
      GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

      gimp_draw_tool_pause (draw_tool);

      if (proximity)
        {
          fg_select->last_coords = *coords;
        }
      else
        {
          fg_select->last_coords.x = FAR_OUTSIDE;
          fg_select->last_coords.y = FAR_OUTSIDE;
        }

      gimp_draw_tool_resume (draw_tool);

      if (options->draw_mode == GIMP_MATTING_DRAW_MODE_FOREGROUND)
        status_mode = _("Selecting foreground");
      else if (options->draw_mode == GIMP_MATTING_DRAW_MODE_BACKGROUND)
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
        gimp_tool_replace_status (tool, display, "%s, %s", status_mode, status_stage);
      else
        gimp_tool_replace_status (tool, display, "%s", status_stage);
    }
}

static void
gimp_foreground_select_tool_cursor_update (GimpTool         *tool,
                                           const GimpCoords *coords,
                                           GdkModifierType   state,
                                           GimpDisplay      *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      switch (GIMP_SELECTION_TOOL (tool)->function)
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

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static const gchar *
gimp_foreground_select_tool_can_undo (GimpTool    *tool,
                                      GimpDisplay *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->undo_stack)
    {
      StrokeUndo  *undo = fg_select->undo_stack->data;
      const gchar *desc;

      if (gimp_enum_get_value (GIMP_TYPE_MATTING_DRAW_MODE, undo->draw_mode,
                               NULL, NULL, &desc, NULL))
        {
          return desc;
        }
    }

  return NULL;
}

static const gchar *
gimp_foreground_select_tool_can_redo (GimpTool    *tool,
                                      GimpDisplay *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->redo_stack)
    {
      StrokeUndo  *undo = fg_select->redo_stack->data;
      const gchar *desc;

      if (gimp_enum_get_value (GIMP_TYPE_MATTING_DRAW_MODE, undo->draw_mode,
                               NULL, NULL, &desc, NULL))
        {
          return desc;
        }
    }

  return NULL;
}

static gboolean
gimp_foreground_select_tool_undo (GimpTool    *tool,
                                  GimpDisplay *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  StrokeUndo               *undo      = fg_select->undo_stack->data;

  gimp_foreground_select_undo_pop (undo, fg_select->trimap);

  fg_select->undo_stack = g_list_remove (fg_select->undo_stack, undo);
  fg_select->redo_stack = g_list_prepend (fg_select->redo_stack, undo);

  if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
    gimp_foreground_select_tool_preview (fg_select);
  else
    gimp_foreground_select_tool_set_trimap (fg_select);

  return TRUE;
}

static gboolean
gimp_foreground_select_tool_redo (GimpTool    *tool,
                                  GimpDisplay *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  StrokeUndo               *undo      = fg_select->redo_stack->data;

  gimp_foreground_select_undo_pop (undo, fg_select->trimap);

  fg_select->redo_stack = g_list_remove (fg_select->redo_stack, undo);
  fg_select->undo_stack = g_list_prepend (fg_select->undo_stack, undo);

  if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
    gimp_foreground_select_tool_preview (fg_select);
  else
    gimp_foreground_select_tool_set_trimap (fg_select);

  return TRUE;
}

static void
gimp_foreground_select_tool_options_notify (GimpTool         *tool,
                                            GimpToolOptions  *options,
                                            const GParamSpec *pspec)
{
  GimpForegroundSelectTool    *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  GimpForegroundSelectOptions *fg_options;

  fg_options = GIMP_FOREGROUND_SELECT_OPTIONS (options);

  if (! tool->display)
    return;

  if (! strcmp (pspec->name, "mask-color") ||
      ! strcmp (pspec->name, "preview-mode"))
    {
      if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
        {
          gimp_foreground_select_tool_set_trimap (fg_select);
        }
      else if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
        {
          gimp_foreground_select_tool_set_preview (fg_select);
        }
    }
  else if (! strcmp (pspec->name, "engine"))
    {
      if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
        {
          gimp_foreground_select_tool_preview (fg_select);
        }
    }
  else if (! strcmp (pspec->name, "iterations"))
    {
      if (fg_options->engine == GIMP_MATTING_ENGINE_GLOBAL &&
          fg_select->state   == MATTING_STATE_PREVIEW_MASK)
        {
          gimp_foreground_select_tool_preview (fg_select);
        }
    }
  else if (! strcmp (pspec->name, "levels") ||
           ! strcmp (pspec->name, "active-levels"))
    {
      if (fg_options->engine == GIMP_MATTING_ENGINE_LEVIN &&
          fg_select->state   == MATTING_STATE_PREVIEW_MASK)
        {
          gimp_foreground_select_tool_preview (fg_select);
        }
    }
}

static void
gimp_foreground_select_tool_get_area (GeglBuffer *mask,
                                      gint       *x1,
                                      gint       *y1,
                                      gint       *x2,
                                      gint       *y2)
{
  gint width;
  gint height;

  gimp_gegl_mask_bounds (mask, x1, y1, x2, y2);

  width  = *x2 - *x1;
  height = *y2 - *y1;

  *x1 = MAX (*x1 - width  / 2, 0);
  *y1 = MAX (*y1 - height / 2, 0);
  *x2 = MIN (*x2 + width  / 2, gimp_item_get_width  (GIMP_ITEM (mask)));
  *y2 = MIN (*y2 + height / 2, gimp_item_get_height (GIMP_ITEM (mask)));
}

static void
gimp_foreground_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool                    *tool      = GIMP_TOOL (draw_tool);
  GimpForegroundSelectTool    *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  GimpForegroundSelectOptions *options;

  options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
      return;
    }
  else
    {
      gint    x      = fg_select->last_coords.x;
      gint    y      = fg_select->last_coords.y;
      gdouble radius = options->stroke_width / 2.0f;

      if (fg_select->stroke)
        {
          GimpDisplayShell *shell = gimp_display_get_shell (draw_tool->display);

          gimp_draw_tool_add_pen (draw_tool,
                                  (const GimpVector2 *) fg_select->stroke->data,
                                  fg_select->stroke->len,
                                  GIMP_CONTEXT (options),
                                  GIMP_ACTIVE_COLOR_FOREGROUND,
                                  options->stroke_width * shell->scale_y);
        }

      /*  warn if the user is drawing outside of the working area  */
      if (FALSE)
        {
          gint x1, y1;
          gint x2, y2;

          gimp_foreground_select_tool_get_area (fg_select->mask,
                                                &x1, &y1, &x2, &y2);

          if (x < x1 + radius || x > x2 - radius ||
              y < y1 + radius || y > y2 - radius)
            {
              gimp_draw_tool_add_rectangle (draw_tool, FALSE,
                                            x1, y1,
                                            x2 - x1, y2 - y1);
            }
        }

      if (x > FAR_OUTSIDE && y > FAR_OUTSIDE)
        gimp_draw_tool_add_arc (draw_tool, FALSE,
                                x - radius, y - radius,
                                2 * radius, 2 * radius,
                                0.0, 2.0 * G_PI);

      if (fg_select->grayscale_preview)
        gimp_draw_tool_add_preview (draw_tool, fg_select->grayscale_preview);
    }
}

static void
gimp_foreground_select_tool_confirm (GimpPolygonSelectTool *poly_sel,
                                     GimpDisplay           *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (poly_sel);
  GimpImage                *image     = gimp_display_get_image (display);
  GList                    *drawables = gimp_image_get_selected_drawables (image);
  GimpDrawable             *drawable;
  GimpItem                 *item;

  g_return_if_fail (g_list_length (drawables) == 1);

  drawable = drawables->data;
  item     = GIMP_ITEM (drawable);
  g_list_free (drawables);

  if (drawable && fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      GimpScanConvert   *scan_convert = gimp_scan_convert_new ();
      const GimpVector2 *points;
      gint               n_points;

      gimp_polygon_select_tool_get_points (poly_sel, &points, &n_points);

      gimp_scan_convert_add_polyline (scan_convert, n_points, points, TRUE);

      fg_select->trimap =
        gegl_buffer_new (GEGL_RECTANGLE (gimp_item_get_offset_x (item),
                                         gimp_item_get_offset_y (item),
                                         gimp_item_get_width    (item),
                                         gimp_item_get_height   (item)),
                         gimp_image_get_mask_format (image));

      gimp_scan_convert_render_value (scan_convert, fg_select->trimap,
                                      0, 0, 0.5);
      gimp_scan_convert_free (scan_convert);

      fg_select->grayscale_preview =
          gimp_canvas_buffer_preview_new (gimp_display_get_shell (display),
                                          fg_select->trimap);

      gimp_foreground_select_tool_set_trimap (fg_select);
    }
}

static void
gimp_foreground_select_tool_halt (GimpForegroundSelectTool *fg_select)
{
  GimpTool     *tool      = GIMP_TOOL (fg_select);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (fg_select);

  if (draw_tool->preview)
    gimp_draw_tool_remove_preview (draw_tool, fg_select->grayscale_preview);

  g_clear_object (&fg_select->grayscale_preview);
  g_clear_object (&fg_select->trimap);
  g_clear_object (&fg_select->mask);

  if (fg_select->undo_stack)
    {
      g_list_free_full (fg_select->undo_stack,
                        (GDestroyNotify) gimp_foreground_select_undo_free);
      fg_select->undo_stack = NULL;
    }

  if (fg_select->redo_stack)
    {
      g_list_free_full (fg_select->redo_stack,
                        (GDestroyNotify) gimp_foreground_select_undo_free);
      fg_select->redo_stack = NULL;
    }

  if (tool->display)
    gimp_display_shell_set_mask (gimp_display_get_shell (tool->display),
                                 NULL, 0, 0, NULL, FALSE);

  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_FREE_SELECT);
  gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                            GIMP_TOOL_CURSOR_FREE_SELECT);

  gimp_tool_control_set_toggled (tool->control, FALSE);

  /*  set precision to SUBPIXEL, because it may have been changed to
   *  PIXEL_CENTER if the tool has switched to MATTING_STATE_PAINT_TRIMAP,
   *  in gimp_foreground_select_tool_set_trimap().
   */
  gimp_tool_control_set_precision (tool->control,
                                   GIMP_CURSOR_PRECISION_SUBPIXEL);

  fg_select->state = MATTING_STATE_FREE_SELECT;

  /*  update the undo actions / menu items  */
  if (tool->display)
    gimp_image_flush (gimp_display_get_image (tool->display));

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (fg_select->gui)
    gimp_tool_gui_hide (fg_select->gui);
}

static void
gimp_foreground_select_tool_commit (GimpForegroundSelectTool *fg_select)
{
  GimpTool             *tool    = GIMP_TOOL (fg_select);
  GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (fg_select);

  if (tool->display                                 &&
      fg_select->state != MATTING_STATE_FREE_SELECT &&
      gimp_foreground_select_tool_has_strokes (fg_select))
    {
      GimpImage *image = gimp_display_get_image (tool->display);

      if (fg_select->state != MATTING_STATE_PREVIEW_MASK)
        gimp_foreground_select_tool_preview (fg_select);

      gimp_channel_select_buffer (gimp_image_get_mask (image),
                                  C_("command", "Foreground Select"),
                                  fg_select->mask,
                                  0, /* x offset */
                                  0, /* y offset */
                                  options->operation,
                                  options->feather,
                                  options->feather_radius,
                                  options->feather_radius);

      gimp_image_flush (image);
    }
}

static void
gimp_foreground_select_tool_set_trimap (GimpForegroundSelectTool *fg_select)
{
  GimpTool                    *tool = GIMP_TOOL (fg_select);
  GimpForegroundSelectOptions *options;

  g_return_if_fail (fg_select->trimap != NULL);

  options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  gimp_polygon_select_tool_halt (GIMP_POLYGON_SELECT_TOOL (fg_select));

  if (options->preview_mode == GIMP_MATTING_PREVIEW_MODE_ON_COLOR)
    {
      if (fg_select->grayscale_preview)
        gimp_canvas_item_set_visible (fg_select->grayscale_preview, FALSE);

      gimp_display_shell_set_mask (gimp_display_get_shell (tool->display),
                                   fg_select->trimap, 0, 0,
                                   options->mask_color, TRUE);
    }
  else
    {
      gimp_display_shell_set_mask (gimp_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);

      if (fg_select->grayscale_preview)
        {
          g_object_set (fg_select->grayscale_preview, "buffer",
                        fg_select->trimap, NULL);

          gimp_canvas_item_set_visible (fg_select->grayscale_preview, TRUE);
        }
    }

  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_PAINTBRUSH);
  gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                            GIMP_TOOL_CURSOR_PAINTBRUSH);

  gimp_tool_control_set_toggled (tool->control, FALSE);

  /* disable double click in paint trimap state */
  gimp_tool_control_set_wants_double_click (tool->control, FALSE);

  /* set precision to PIXEL_CENTER in paint trimap state */
  gimp_tool_control_set_precision (tool->control,
                                   GIMP_CURSOR_PRECISION_PIXEL_CENTER);

  fg_select->state = MATTING_STATE_PAINT_TRIMAP;

  gimp_foreground_select_tool_update_gui (fg_select);
}

static void
gimp_foreground_select_tool_set_preview (GimpForegroundSelectTool *fg_select)
{

  GimpTool                    *tool = GIMP_TOOL (fg_select);
  GimpForegroundSelectOptions *options;

  g_return_if_fail (fg_select->mask != NULL);

  options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  if (options->preview_mode == GIMP_MATTING_PREVIEW_MODE_ON_COLOR)
    {
      if (fg_select->grayscale_preview)
        gimp_canvas_item_set_visible (fg_select->grayscale_preview, FALSE);

      gimp_display_shell_set_mask (gimp_display_get_shell (tool->display),
                                   fg_select->mask, 0, 0,
                                   options->mask_color, TRUE);
    }
  else
    {
      gimp_display_shell_set_mask (gimp_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);

      if (fg_select->grayscale_preview)
        {
          g_object_set (fg_select->grayscale_preview, "buffer",
                    fg_select->mask, NULL);
          gimp_canvas_item_set_visible (fg_select->grayscale_preview, TRUE);
        }
    }

  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_PAINTBRUSH);
  gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                            GIMP_TOOL_CURSOR_PAINTBRUSH);

  gimp_tool_control_set_toggled (tool->control, FALSE);

  fg_select->state = MATTING_STATE_PREVIEW_MASK;

  gimp_foreground_select_tool_update_gui (fg_select);
}

static void
gimp_foreground_select_tool_preview (GimpForegroundSelectTool *fg_select)
{
  GimpTool                    *tool      = GIMP_TOOL (fg_select);
  GimpForegroundSelectOptions *options;
  GimpImage                   *image     = gimp_display_get_image (tool->display);
  GList                       *drawables = gimp_image_get_selected_drawables (image);
  GimpDrawable                *drawable;

  g_return_if_fail (g_list_length (drawables) == 1);

  drawable = drawables->data;
  g_list_free (drawables);

  options  = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  g_clear_object (&fg_select->mask);

  fg_select->mask = gimp_drawable_foreground_extract (drawable,
                                                      options->engine,
                                                      options->iterations,
                                                      options->levels,
                                                      options->active_levels,
                                                      fg_select->trimap,
                                                      GIMP_PROGRESS (fg_select));

  gimp_foreground_select_tool_set_preview (fg_select);
}

static void
gimp_foreground_select_tool_stroke_paint (GimpForegroundSelectTool *fg_select)
{
  GimpForegroundSelectOptions *options;
  GimpScanConvert             *scan_convert;
  StrokeUndo                  *undo;
  gint                         width;
  gdouble                      opacity;

  options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (fg_select);

  g_return_if_fail (fg_select->stroke != NULL);

  width = ROUND ((gdouble) options->stroke_width);

  if (fg_select->redo_stack)
    {
      g_list_free_full (fg_select->redo_stack,
                        (GDestroyNotify) gimp_foreground_select_undo_free);
      fg_select->redo_stack = NULL;
    }

  undo = gimp_foreground_select_undo_new (fg_select->trimap,
                                          fg_select->stroke,
                                          options->draw_mode, width);
  if (! undo)
    {
      g_array_free (fg_select->stroke, TRUE);
      fg_select->stroke = NULL;
      return;
    }

  fg_select->undo_stack = g_list_prepend (fg_select->undo_stack, undo);

  scan_convert = gimp_scan_convert_new ();

  if (fg_select->stroke->len == 1)
    {
      GimpVector2 points[2];

      points[0] = points[1] = ((GimpVector2 *) fg_select->stroke->data)[0];

      points[1].x += 0.01;
      points[1].y += 0.01;

      gimp_scan_convert_add_polyline (scan_convert, 2, points, FALSE);
    }
  else
    {
      gimp_scan_convert_add_polyline (scan_convert,
                                      fg_select->stroke->len,
                                      (GimpVector2 *) fg_select->stroke->data,
                                      FALSE);
    }

  gimp_scan_convert_stroke (scan_convert,
                            width,
                            GIMP_JOIN_ROUND, GIMP_CAP_ROUND, 10.0,
                            0.0, NULL);

  if (options->draw_mode == GIMP_MATTING_DRAW_MODE_FOREGROUND)
    opacity = 1.0;
  else if (options->draw_mode == GIMP_MATTING_DRAW_MODE_BACKGROUND)
    opacity = 0.0;
  else
    opacity = 0.5;

  gimp_scan_convert_compose_value (scan_convert, fg_select->trimap,
                                   0, 0,
                                   opacity);

  gimp_scan_convert_free (scan_convert);

  g_array_free (fg_select->stroke, TRUE);
  fg_select->stroke = NULL;

  /*  update the undo actions / menu items  */
  gimp_image_flush (gimp_display_get_image (GIMP_TOOL (fg_select)->display));
}

static void
gimp_foreground_select_tool_cancel_paint (GimpForegroundSelectTool *fg_select)
{
  g_return_if_fail (fg_select->stroke != NULL);

  g_array_free (fg_select->stroke, TRUE);
  fg_select->stroke = NULL;
}

static void
gimp_foreground_select_tool_response (GimpToolGui              *gui,
                                      gint                      response_id,
                                      GimpForegroundSelectTool *fg_select)
{
  GimpTool *tool = GIMP_TOOL (fg_select);

  switch (response_id)
    {
    case GTK_RESPONSE_APPLY:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);
      break;

    default:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
gimp_foreground_select_tool_preview_toggled (GtkToggleButton          *button,
                                             GimpForegroundSelectTool *fg_select)
{
  if (fg_select->state != MATTING_STATE_FREE_SELECT)
    {
      if (gtk_toggle_button_get_active (button))
        {
          if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
            gimp_foreground_select_tool_preview (fg_select);
        }
      else
        {
          if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
            gimp_foreground_select_tool_set_trimap (fg_select);
        }
    }
}

static void
gimp_foreground_select_tool_update_gui (GimpForegroundSelectTool *fg_select)
{
  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      gimp_tool_gui_set_description (fg_select->gui, _("Paint mask"));
    }
  else if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
    {
      gimp_tool_gui_set_description (fg_select->gui, _("Preview"));
    }

  gimp_tool_gui_set_response_sensitive (fg_select->gui, GTK_RESPONSE_APPLY,
                                        TRUE);
  gtk_widget_set_sensitive (fg_select->preview_toggle, TRUE);
}

static gboolean
gimp_foreground_select_tool_has_strokes (GimpForegroundSelectTool *fg_select)
{
  GList *iter;

  if (fg_select->undo_stack == NULL)
    return FALSE;

  /* Check if undo_stack has any foreground strokes, as these are the
     only ones that can create a selection */
  for (iter = fg_select->undo_stack; iter; iter = iter->next)
    {
      StrokeUndo *undo = iter->data;

      if (undo && undo->draw_mode == GIMP_MATTING_DRAW_MODE_FOREGROUND)
        return TRUE;
    }

  return FALSE;
}

static StrokeUndo *
gimp_foreground_select_undo_new (GeglBuffer          *trimap,
                                 GArray              *stroke,
                                 GimpMattingDrawMode draw_mode,
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
      GimpVector2 *point = &g_array_index (stroke, GimpVector2, i);

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

  gimp_gegl_buffer_copy (
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
gimp_foreground_select_undo_pop (StrokeUndo *undo,
                                 GeglBuffer *trimap)
{
  GeglBuffer *buffer;
  gint        width, height;

  buffer = gimp_gegl_buffer_dup (undo->saved_trimap);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  gimp_gegl_buffer_copy (trimap,
                         GEGL_RECTANGLE (undo->trimap_x, undo->trimap_y,
                                         width, height),
                         GEGL_ABYSS_NONE,
                         undo->saved_trimap, NULL);

  gimp_gegl_buffer_copy (buffer,
                         GEGL_RECTANGLE (undo->trimap_x, undo->trimap_y,
                                         width, height),
                         GEGL_ABYSS_NONE,
                         trimap, NULL);

  g_object_unref (buffer);
}

static void
gimp_foreground_select_undo_free (StrokeUndo *undo)
{
  g_clear_object (&undo->saved_trimap);

  g_slice_free (StrokeUndo, undo);
}
