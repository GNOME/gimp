/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpForegroundSelectTool
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-combine.h"
#include "core/gimpchannel-select.h"
#include "core/gimpdrawable-foreground-extract.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"
#include "core/gimpscanconvert.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-draw.h"

#include "gimpforegroundselecttool.h"
#include "gimpforegroundselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


typedef struct
{
  gint         width;
  gboolean     background;
  gint         num_points;
  GimpVector2 *points;
} FgSelectStroke;


static GObject * gimp_foreground_select_tool_constructor (GType            type,
                                                          guint            n_params,
                                                          GObjectConstructParam *params);
static void   gimp_foreground_select_tool_finalize       (GObject         *object);

static void   gimp_foreground_select_tool_control        (GimpTool        *tool,
                                                          GimpToolAction   action,
                                                          GimpDisplay     *display);
static void   gimp_foreground_select_tool_oper_update    (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          GdkModifierType  state,
                                                          gboolean         proximity,
                                                          GimpDisplay     *display);
static void   gimp_foreground_select_tool_modifier_key   (GimpTool        *tool,
                                                          GdkModifierType  key,
                                                          gboolean         press,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *display);
static void   gimp_foreground_select_tool_cursor_update  (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *display);
static gboolean  gimp_foreground_select_tool_key_press   (GimpTool        *tool,
                                                          GdkEventKey     *kevent,
                                                          GimpDisplay     *display);
static void   gimp_foreground_select_tool_button_press   (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *display);
static void   gimp_foreground_select_tool_button_release (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpButtonReleaseType release_type,
                                                          GimpDisplay     *display);
static void   gimp_foreground_select_tool_motion         (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *display);

static void   gimp_foreground_select_tool_draw           (GimpDrawTool    *draw_tool);

static void   gimp_foreground_select_tool_select   (GimpFreeSelectTool *free_sel,
                                                    GimpDisplay        *display);

static void   gimp_foreground_select_tool_set_mask (GimpForegroundSelectTool *fg_select,
                                                    GimpDisplay              *display,
                                                    GimpChannel              *mask);
static void   gimp_foreground_select_tool_apply    (GimpForegroundSelectTool *fg_select,
                                                    GimpDisplay              *display);

static void   gimp_foreground_select_tool_stroke   (GimpChannel              *mask,
                                                    FgSelectStroke           *stroke);

static void   gimp_foreground_select_tool_push_stroke (GimpForegroundSelectTool    *fg_select,
                                                       GimpDisplay                 *display,
                                                       GimpForegroundSelectOptions *options);

static void   gimp_foreground_select_options_notify (GimpForegroundSelectOptions *options,
                                                     GParamSpec                  *pspec,
                                                     GimpForegroundSelectTool    *fg_select);


G_DEFINE_TYPE (GimpForegroundSelectTool, gimp_foreground_select_tool,
               GIMP_TYPE_FREE_SELECT_TOOL)

#define parent_class gimp_foreground_select_tool_parent_class


void
gimp_foreground_select_tool_register (GimpToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (GIMP_TYPE_FOREGROUND_SELECT_TOOL,
                GIMP_TYPE_FOREGROUND_SELECT_OPTIONS,
                gimp_foreground_select_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK | GIMP_CONTEXT_BACKGROUND_MASK,
                "gimp-foreground-select-tool",
                _("Foreground Select"),
                _("Foreground Select Tool: Select a region containing foreground objects"),
                N_("F_oreground Select"), NULL,
                NULL, GIMP_HELP_TOOL_FOREGROUND_SELECT,
                GIMP_STOCK_TOOL_FOREGROUND_SELECT,
                data);
}

static void
gimp_foreground_select_tool_class_init (GimpForegroundSelectToolClass *klass)
{
  GObjectClass            *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass           *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass       *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  GimpFreeSelectToolClass *free_select_tool_class;

  free_select_tool_class = GIMP_FREE_SELECT_TOOL_CLASS (klass);

  object_class->constructor      = gimp_foreground_select_tool_constructor;
  object_class->finalize         = gimp_foreground_select_tool_finalize;

  tool_class->control            = gimp_foreground_select_tool_control;
  tool_class->oper_update        = gimp_foreground_select_tool_oper_update;
  tool_class->modifier_key       = gimp_foreground_select_tool_modifier_key;
  tool_class->cursor_update      = gimp_foreground_select_tool_cursor_update;
  tool_class->key_press          = gimp_foreground_select_tool_key_press;
  tool_class->button_press       = gimp_foreground_select_tool_button_press;
  tool_class->button_release     = gimp_foreground_select_tool_button_release;
  tool_class->motion             = gimp_foreground_select_tool_motion;

  draw_tool_class->draw          = gimp_foreground_select_tool_draw;

  free_select_tool_class->select = gimp_foreground_select_tool_select;
}

static void
gimp_foreground_select_tool_init (GimpForegroundSelectTool *fg_select)
{
  GimpTool *tool = GIMP_TOOL (fg_select);

  gimp_tool_control_set_scroll_lock (tool->control, FALSE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control, GIMP_DIRTY_IMAGE_SIZE);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_FREE_SELECT);

  gimp_tool_control_set_action_value_2 (tool->control,
                                        "tools/tools-foreground-select-brush-size-set");

  fg_select->idle_id = 0;
  fg_select->stroke  = NULL;
  fg_select->strokes = NULL;
  fg_select->mask    = NULL;
}

static GObject *
gimp_foreground_select_tool_constructor (GType                  type,
                                         guint                  n_params,
                                         GObjectConstructParam *params)
{
  GObject         *object;
  GimpToolOptions *options;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  options = GIMP_TOOL_GET_OPTIONS (object);

  g_signal_connect_object (options, "notify",
                           G_CALLBACK (gimp_foreground_select_options_notify),
                           object, 0);

  return object;
}

static void
gimp_foreground_select_tool_finalize (GObject *object)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (object);

  if (fg_select->stroke)
    g_warning ("%s: stroke should be NULL at this point", G_STRLOC);

  if (fg_select->strokes)
    g_warning ("%s: strokes should be NULL at this point", G_STRLOC);

  if (fg_select->state)
    g_warning ("%s: state should be NULL at this point", G_STRLOC);

  if (fg_select->mask)
    g_warning ("%s: mask should be NULL at this point", G_STRLOC);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
      {
        GList *list;

        gimp_foreground_select_tool_set_mask (fg_select, display, NULL);

        for (list = fg_select->strokes; list; list = list->next)
          {
            FgSelectStroke *stroke = list->data;

            g_free (stroke->points);
            g_slice_free (FgSelectStroke, stroke);
          }

        g_list_free (fg_select->strokes);
        fg_select->strokes = NULL;

        if (fg_select->state)
          {
            gimp_drawable_foreground_extract_siox_done (fg_select->state);
            fg_select->state = NULL;
          }
      }
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_foreground_select_tool_oper_update (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         GdkModifierType  state,
                                         gboolean         proximity,
                                         GimpDisplay     *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  GimpDrawTool             *draw_tool = GIMP_DRAW_TOOL (tool);
  const gchar              *status    = NULL;

  if (fg_select->mask && gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  GIMP_FREE_SELECT_TOOL (tool)->last_coords = *coords;

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  if (fg_select->mask)
    {
      switch (GIMP_SELECTION_TOOL (tool)->function)
        {
        case SELECTION_SELECT:
        case SELECTION_MOVE_MASK:
        case SELECTION_MOVE:
        case SELECTION_MOVE_COPY:
        case SELECTION_ANCHOR:
          if (fg_select->strokes)
            status = _("Add more strokes or press Enter to accept the selection");
          else
            status = _("Mark foreground by painting on the object to extract");
          break;
        default:
          break;
        }
    }
  else
    {
      switch (GIMP_SELECTION_TOOL (tool)->function)
        {
        case SELECTION_SELECT:
          status = _("Draw a rough circle around the object to extract");
          break;
        default:
          break;
        }
    }

  if (proximity)
    {
      if (status)
        gimp_tool_replace_status (tool, display, status);

      gimp_draw_tool_start (draw_tool, display);
    }
}

static void
gimp_foreground_select_tool_modifier_key (GimpTool        *tool,
                                          GdkModifierType  key,
                                          gboolean         press,
                                          GdkModifierType  state,
                                          GimpDisplay     *display)
{
  if (key == GDK_CONTROL_MASK)
    {
      GimpForegroundSelectOptions *options;

      options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

      g_object_set (options,
                    "background", ! options->background,
                    NULL);
    }
}

static void
gimp_foreground_select_tool_cursor_update (GimpTool        *tool,
                                           GimpCoords      *coords,
                                           GdkModifierType  state,
                                           GimpDisplay     *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->mask)
    {
      GimpForegroundSelectOptions *options;

      options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

      gimp_tool_control_set_toggled (tool->control, options->background);

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

static gboolean
gimp_foreground_select_tool_key_press (GimpTool    *tool,
                                       GdkEventKey *kevent,
                                       GimpDisplay *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (display != tool->display)
    return FALSE;

  switch (kevent->keyval)
    {
    case GDK_KP_Enter:
    case GDK_Return:
      gimp_foreground_select_tool_apply (fg_select, display);
      return TRUE;

    case GDK_Escape:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      return FALSE;
    }
}

static void
gimp_foreground_select_tool_button_press (GimpTool        *tool,
                                          GimpCoords      *coords,
                                          guint32          time,
                                          GdkModifierType  state,
                                          GimpDisplay     *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  GimpDrawTool             *draw_tool = GIMP_DRAW_TOOL (tool);

  if (fg_select->mask)
    {
      GimpVector2 point = gimp_vector2_new (coords->x, coords->y);

      gimp_draw_tool_pause (draw_tool);

      if (gimp_draw_tool_is_active (draw_tool) && draw_tool->display != display)
        gimp_draw_tool_stop (draw_tool);

      gimp_tool_control_activate (tool->control);

      GIMP_FREE_SELECT_TOOL (tool)->last_coords = *coords;

      g_return_if_fail (fg_select->stroke == NULL);
      fg_select->stroke = g_array_new (FALSE, FALSE, sizeof (GimpVector2));

      g_array_append_val (fg_select->stroke, point);

      if (! gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_start (draw_tool, display);

      gimp_draw_tool_resume (draw_tool);
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool,
                                                    coords, time, state, display);
    }
}

static void
gimp_foreground_select_tool_button_release (GimpTool              *tool,
                                            GimpCoords            *coords,
                                            guint32                time,
                                            GdkModifierType        state,
                                            GimpButtonReleaseType  release_type,
                                            GimpDisplay           *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->mask)
    {
      GimpForegroundSelectOptions *options;

      options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      gimp_tool_control_halt (tool->control);

      gimp_foreground_select_tool_push_stroke (fg_select, display, options);

      gimp_free_select_tool_select (GIMP_FREE_SELECT_TOOL (tool), display);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool,
                                                      coords, time, state,
                                                      release_type,
                                                      display);
    }
}

static void
gimp_foreground_select_tool_motion (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->mask)
    {
      GimpVector2 *last = &g_array_index (fg_select->stroke,
                                          GimpVector2,
                                          fg_select->stroke->len - 1);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      GIMP_FREE_SELECT_TOOL (tool)->last_coords = *coords;

      if (last->x != (gint) coords->x || last->y != (gint) coords->y)
        {
          GimpVector2 point = gimp_vector2_new (coords->x, coords->y);

          g_array_append_val (fg_select->stroke, point);
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->motion (tool,
                                              coords, time, state, display);
    }
}

static void
gimp_foreground_select_tool_get_area (GimpChannel *mask,
                                      gint        *x1,
                                      gint        *y1,
                                      gint        *x2,
                                      gint        *y2)
{
  gint width;
  gint height;

  gimp_channel_bounds (mask, x1, y1, x2, y2);

  width  = *x2 - *x1;
  height = *y2 - *y1;

  *x1 = MAX (*x1 - width  / 2, 0);
  *y1 = MAX (*y1 - height / 2, 0);
  *x2 = MIN (*x2 + width  / 2, gimp_item_width (GIMP_ITEM (mask)));
  *y2 = MIN (*y2 + height / 2, gimp_item_height (GIMP_ITEM (mask)));
}

static void
gimp_foreground_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpForegroundSelectTool    *fg_select = GIMP_FOREGROUND_SELECT_TOOL (draw_tool);
  GimpTool                    *tool      = GIMP_TOOL (draw_tool);
  GimpForegroundSelectOptions *options;

  options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  if (fg_select->stroke)
    {
      gimp_display_shell_draw_pen (GIMP_DISPLAY_SHELL (draw_tool->display->shell),
                                   (const GimpVector2 *)fg_select->stroke->data,
                                   fg_select->stroke->len,
                                   GIMP_CONTEXT (options),
                                   (options->background ?
                                    GIMP_ACTIVE_COLOR_BACKGROUND :
                                    GIMP_ACTIVE_COLOR_FOREGROUND),
                                   options->stroke_width);
    }

  if (fg_select->mask)
    {
      GimpFreeSelectTool *sel   = GIMP_FREE_SELECT_TOOL (tool);
      GimpDisplayShell   *shell = GIMP_DISPLAY_SHELL (draw_tool->display->shell);
      gint                x     = sel->last_coords.x;
      gint                y     = sel->last_coords.y;
      gdouble             radius;

      radius = (options->stroke_width / shell->scale_y) / 2;

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
              gimp_draw_tool_draw_rectangle (draw_tool, FALSE,
                                             x1, y1, x2 - x1, y2 - y1, FALSE);
            }
        }

      gimp_draw_tool_draw_arc (draw_tool, FALSE,
                               x - radius, y - radius,
                               2 * radius, 2 * radius, 0, 360 * 64,
                               FALSE);
    }
  else
    {
      GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
    }
}

static void
gimp_foreground_select_tool_select (GimpFreeSelectTool *free_sel,
                                    GimpDisplay        *display)
{
  GimpForegroundSelectTool    *fg_select;
  GimpForegroundSelectOptions *options;
  GimpImage                   *image    = display->image;
  GimpDrawable                *drawable = gimp_image_active_drawable (image);
  GimpScanConvert             *scan_convert;
  GimpChannel                 *mask;

  fg_select = GIMP_FOREGROUND_SELECT_TOOL (free_sel);
  options   = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (free_sel);

  if (fg_select->idle_id)
    {
      g_source_remove (fg_select->idle_id);
      fg_select->idle_id = 0;
    }

  if (! drawable)
    return;

  scan_convert = gimp_scan_convert_new ();

  gimp_scan_convert_add_polyline (scan_convert,
                                  free_sel->num_points, free_sel->points, TRUE);

  mask = gimp_channel_new (image,
                           gimp_image_get_width (image),
                           gimp_image_get_height (image),
                           "foreground-extraction", NULL);

  gimp_scan_convert_render_value (scan_convert,
                                  gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                                  0, 0, 128);
  gimp_scan_convert_free (scan_convert);

  if (fg_select->strokes)
    {
      GList *list;

      gimp_set_busy (image->gimp);

      /*  apply foreground and background markers  */
      for (list = fg_select->strokes; list; list = list->next)
        gimp_foreground_select_tool_stroke (mask, list->data);

      if (fg_select->state)
        gimp_drawable_foreground_extract_siox (GIMP_DRAWABLE (mask),
                                               fg_select->state,
                                               fg_select->refinement,
                                               options->smoothness,
                                               options->sensitivity,
                                               ! options->contiguous,
                                               GIMP_PROGRESS (display));

      fg_select->refinement = SIOX_REFINEMENT_NO_CHANGE;

      gimp_unset_busy (image->gimp);
    }
  else
    {
      gint x1, y1;
      gint x2, y2;

      g_object_set (options, "background", FALSE, NULL);

      gimp_foreground_select_tool_get_area (mask, &x1, &y1, &x2, &y2);

      if (fg_select->state)
        g_warning ("state should be NULL here");

      fg_select->state =
        gimp_drawable_foreground_extract_siox_init (drawable,
                                                    x1, y1, x2 - x1, y2 - y1);
    }

  gimp_foreground_select_tool_set_mask (fg_select, display, mask);

  g_object_unref (mask);
}

static void
gimp_foreground_select_tool_set_mask (GimpForegroundSelectTool *fg_select,
                                      GimpDisplay              *display,
                                      GimpChannel              *mask)
{
  GimpTool                    *tool = GIMP_TOOL (fg_select);
  GimpForegroundSelectOptions *options;

  options = GIMP_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  if (fg_select->mask == mask)
    return;

  if (fg_select->mask)
    {
      g_object_unref (fg_select->mask);
      fg_select->mask = NULL;
    }

  if (mask)
    fg_select->mask = g_object_ref (mask);

  gimp_display_shell_set_mask (GIMP_DISPLAY_SHELL (display->shell),
                               GIMP_DRAWABLE (mask), options->mask_color);

  if (mask)
    {
      gimp_tool_control_set_tool_cursor        (tool->control,
                                                GIMP_TOOL_CURSOR_PAINTBRUSH);
      gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                                GIMP_TOOL_CURSOR_ERASER);

      gimp_tool_control_set_toggled (tool->control, options->background);
    }
  else
    {
      gimp_tool_control_set_tool_cursor        (tool->control,
                                                GIMP_TOOL_CURSOR_FREE_SELECT);
      gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                                GIMP_TOOL_CURSOR_FREE_SELECT);

      gimp_tool_control_set_toggled (tool->control, FALSE);
    }
}

static void
gimp_foreground_select_tool_apply (GimpForegroundSelectTool *fg_select,
                                   GimpDisplay              *display)
{
  GimpTool             *tool    = GIMP_TOOL (fg_select);
  GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  g_return_if_fail (fg_select->mask != NULL);

  gimp_channel_select_channel (gimp_image_get_mask (display->image),
                               Q_("command|Foreground Select"),
                               fg_select->mask, 0, 0,
                               options->operation,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius);

  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

  gimp_image_flush (display->image);
}

static void
gimp_foreground_select_tool_stroke (GimpChannel    *mask,
                                    FgSelectStroke *stroke)
{
  GimpScanConvert *scan_convert = gimp_scan_convert_new ();

  if (stroke->num_points == 1)
    {
      GimpVector2 points[2];

      points[0] = points[1] = stroke->points[0];

      points[1].x += 0.01;
      points[1].y += 0.01;

      gimp_scan_convert_add_polyline (scan_convert, 2, points, FALSE);
    }
  else
    {
      gimp_scan_convert_add_polyline (scan_convert,
                                      stroke->num_points, stroke->points,
                                      FALSE);
    }

  gimp_scan_convert_stroke (scan_convert,
                            stroke->width,
                            GIMP_JOIN_ROUND, GIMP_CAP_ROUND, 10.0,
                            0.0, NULL);
  gimp_scan_convert_compose (scan_convert,
                             stroke->background ?
                             GIMP_CHANNEL_OP_SUBTRACT : GIMP_CHANNEL_OP_ADD,
                             gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                             0, 0);
  gimp_scan_convert_free (scan_convert);
}

static void
gimp_foreground_select_tool_push_stroke (GimpForegroundSelectTool    *fg_select,
                                         GimpDisplay                 *display,
                                         GimpForegroundSelectOptions *options)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (display->shell);
  FgSelectStroke   *stroke;

  g_return_if_fail (fg_select->stroke != NULL);

  stroke = g_slice_new (FgSelectStroke);

  stroke->background = options->background;
  stroke->width      = ROUND ((gdouble) options->stroke_width / shell->scale_y);
  stroke->num_points = fg_select->stroke->len;
  stroke->points     = (GimpVector2 *) g_array_free (fg_select->stroke, FALSE);

  fg_select->stroke = NULL;

  fg_select->strokes = g_list_append (fg_select->strokes, stroke);

  fg_select->refinement |= (stroke->background ?
                            SIOX_REFINEMENT_ADD_BACKGROUND :
                            SIOX_REFINEMENT_ADD_FOREGROUND);
}

static gboolean
gimp_foreground_select_tool_idle_select (GimpForegroundSelectTool *fg_select)
{
  GimpTool *tool = GIMP_TOOL (fg_select);

  fg_select->idle_id = 0;

  if (tool->display)
    gimp_free_select_tool_select (GIMP_FREE_SELECT_TOOL (tool), tool->display);

  return FALSE;
}

static void
gimp_foreground_select_options_notify (GimpForegroundSelectOptions *options,
                                       GParamSpec                  *pspec,
                                       GimpForegroundSelectTool    *fg_select)
{
  SioxRefinementType refinement = 0;

  if (! fg_select->mask)
    return;

  if (strcmp (pspec->name, "smoothness") == 0)
    {
      refinement = SIOX_REFINEMENT_CHANGE_SMOOTHNESS;
    }
  else if (strcmp (pspec->name, "contiguous") == 0)
    {
      refinement = SIOX_REFINEMENT_CHANGE_MULTIBLOB;
    }
  else if (g_str_has_prefix (pspec->name, "sensitivity"))
    {
      refinement = SIOX_REFINEMENT_CHANGE_SENSITIVITY;
    }

  if (refinement)
    {
      fg_select->refinement |= refinement;

      if (fg_select->idle_id)
        g_source_remove (fg_select->idle_id);

      fg_select->idle_id =
        g_idle_add_full (G_PRIORITY_LOW,
                         (GSourceFunc) gimp_foreground_select_tool_idle_select,
                         fg_select, NULL);
    }

  if (g_str_has_prefix (pspec->name, "mask-color"))
    {
      GimpTool *tool = GIMP_TOOL (fg_select);

      if (tool->display)
        gimp_display_shell_set_mask (GIMP_DISPLAY_SHELL (tool->display->shell),
                                     GIMP_DRAWABLE (fg_select->mask),
                                     options->mask_color);
    }
}
