/* The GIMP -- an image manipulation program
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

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
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpforegroundselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_foreground_select_tool_class_init (GimpForegroundSelectToolClass *klass);
static void   gimp_foreground_select_tool_init       (GimpForegroundSelectTool      *fg_select);
static void   gimp_foreground_select_tool_finalize       (GObject         *object);

static void   gimp_foreground_select_tool_control        (GimpTool        *tool,
                                                          GimpToolAction   action,
                                                          GimpDisplay     *gdisp);
static void   gimp_foreground_select_tool_oper_update    (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static void   gimp_foreground_select_tool_cursor_update  (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static gboolean  gimp_foreground_select_tool_key_press   (GimpTool        *tool,
                                                          GdkEventKey     *kevent,
                                                          GimpDisplay     *gdisp);
static void   gimp_foreground_select_tool_button_press   (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static void   gimp_foreground_select_tool_button_release (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static void   gimp_foreground_select_tool_motion         (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);

static void   gimp_foreground_select_tool_draw           (GimpDrawTool    *draw_tool);

static void   gimp_foreground_select_tool_select   (GimpFreeSelectTool *free_sel,
                                                    GimpDisplay        *gdisp);

static void   gimp_foreground_select_tool_set_mask (GimpForegroundSelectTool *fg_select,
                                                    GimpDisplay              *gdisp,
                                                    GimpChannel              *mask);
static void   gimp_foreground_select_tool_apply    (GimpForegroundSelectTool *fg_select,
                                                    GimpDisplay              *gdisp);

static void   gimp_foreground_select_tool_stroke   (GimpChannel  *mask,
                                                    gint          num_points,
                                                    GimpVector2  *points,
                                                    gint          width,
                                                    gboolean      foreground);
static void   gimp_foreground_select_tool_strokes_free    (GList *strokes);


static GimpFreeSelectToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_foreground_select_tool_register (GimpToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (GIMP_TYPE_FOREGROUND_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-foreground-select-tool",
                _("Foreground Select"),
                _("Extract foreground using SIOX algorithm"),
                N_("F_oreground Select"), NULL,
                NULL, GIMP_HELP_TOOL_FOREGROUND_SELECT,
                GIMP_STOCK_TOOL_FOREGROUND_SELECT,
                data);
}

GType
gimp_foreground_select_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpForegroundSelectToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_foreground_select_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpForegroundSelectTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_foreground_select_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_FREE_SELECT_TOOL,
                                          "GimpForegroundSelectTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

/*  private functions  */

static void
gimp_foreground_select_tool_class_init (GimpForegroundSelectToolClass *klass)
{
  GObjectClass            *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass           *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass       *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  GimpFreeSelectToolClass *free_select_tool_class;

  free_select_tool_class = GIMP_FREE_SELECT_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize         = gimp_foreground_select_tool_finalize;

  tool_class->control            = gimp_foreground_select_tool_control;
  tool_class->oper_update        = gimp_foreground_select_tool_oper_update;
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

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_FREE_SELECT);
  gimp_tool_control_set_toggle_tool_cursor (tool->control,
                                            GIMP_TOOL_CURSOR_PAINTBRUSH);

  fg_select->stroke       = NULL;
  fg_select->stroke_width = 6;
  fg_select->fg_strokes   = NULL;
  fg_select->bg_strokes   = NULL;
  fg_select->mask         = NULL;
}

static void
gimp_foreground_select_tool_finalize (GObject *object)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (object);

  if (fg_select->stroke)
    g_warning ("%s: stroke should be NULL at this point", G_STRLOC);

  if (fg_select->fg_strokes)
    g_warning ("%s: fg_strokes should be NULL at this point", G_STRLOC);

  if (fg_select->bg_strokes)
    g_warning ("%s: bg_strokes should be NULL at this point", G_STRLOC);

  if (fg_select->mask)
    g_warning ("%s: mask should be NULL at this point", G_STRLOC);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_foreground_select_tool_control (GimpTool       *tool,
                                     GimpToolAction  action,
                                     GimpDisplay    *gdisp)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  switch (action)
    {
    case HALT:
      gimp_foreground_select_tool_set_mask (fg_select, gdisp, NULL);

      gimp_foreground_select_tool_strokes_free (fg_select->fg_strokes);
      fg_select->fg_strokes = NULL;

      gimp_foreground_select_tool_strokes_free (fg_select->bg_strokes);
      fg_select->bg_strokes = NULL;
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_foreground_select_tool_oper_update (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         GdkModifierType  state,
                                         GimpDisplay     *gdisp)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  const gchar              *status    = NULL;

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, gdisp);

  if (fg_select->mask)
    {
      switch (GIMP_SELECTION_TOOL (tool)->op)
        {
        case SELECTION_REPLACE:
        case SELECTION_MOVE_MASK:
        case SELECTION_MOVE:
        case SELECTION_MOVE_COPY:
        case SELECTION_ANCHOR:
          status = _("Press Enter to apply selection");
          break;
        default:
          break;
        }
    }
  else
    {
      switch (GIMP_SELECTION_TOOL (tool)->op)
        {
        case SELECTION_ADD:
        case SELECTION_SUBTRACT:
        case SELECTION_INTERSECT:
        case SELECTION_REPLACE:
          status = _("Draw a rough outline around the object to extract");
          break;
        default:
          break;
        }
    }

  if (status)
    gimp_tool_replace_status (tool, gdisp, status);
}

static void
gimp_foreground_select_tool_cursor_update (GimpTool        *tool,
                                           GimpCoords      *coords,
                                           GdkModifierType  state,
                                           GimpDisplay     *gdisp)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->mask)
    {
      switch (GIMP_SELECTION_TOOL (tool)->op)
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

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static gboolean
gimp_foreground_select_tool_key_press (GimpTool    *tool,
                                       GdkEventKey *kevent,
                                       GimpDisplay *gdisp)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (gdisp != tool->gdisp)
    return FALSE;

  switch (kevent->keyval)
    {
    case GDK_KP_Enter:
    case GDK_Return:
      gimp_foreground_select_tool_apply (fg_select, gdisp);
      return TRUE;

    case GDK_Escape:
      gimp_tool_control (tool, HALT, gdisp);
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
                                          GimpDisplay     *gdisp)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);
  GimpDrawTool             *draw_tool = GIMP_DRAW_TOOL (tool);

  if (fg_select->mask)
    {
      GimpVector2 point = gimp_vector2_new (coords->x, coords->y);

      gimp_draw_tool_pause (draw_tool);

      if (gimp_draw_tool_is_active (draw_tool) && draw_tool->gdisp != gdisp)
        {
          gimp_draw_tool_stop (draw_tool);
        }

      gimp_tool_control_activate (tool->control);

      g_return_if_fail (fg_select->stroke == NULL);
      fg_select->stroke = g_array_new (FALSE, FALSE, sizeof (GimpVector2));

      g_array_append_val (fg_select->stroke, point);

      if (! gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_start (draw_tool, gdisp);

      gimp_draw_tool_resume (draw_tool);
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->button_press (tool,
                                                    coords, time, state, gdisp);
    }
}

static void
gimp_foreground_select_tool_button_release (GimpTool        *tool,
                                            GimpCoords      *coords,
                                            guint32          time,
                                            GdkModifierType  state,
                                            GimpDisplay     *gdisp)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->mask)
    {
      GList *list = fg_select->fg_strokes;

      gimp_tool_control_halt (tool->control);

      list = g_list_append (list, GINT_TO_POINTER (fg_select->stroke->len));
      list = g_list_append (list, g_array_free (fg_select->stroke, FALSE));
      list = g_list_append (list, GINT_TO_POINTER (fg_select->stroke_width));

      fg_select->fg_strokes = list;

      fg_select->stroke = NULL;

      gimp_foreground_select_tool_select (GIMP_FREE_SELECT_TOOL (tool), gdisp);
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->button_release (tool,
                                                      coords, time, state,
                                                      gdisp);
    }
}

static void
gimp_foreground_select_tool_motion (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->mask)
    {
      GimpVector2 *last = &g_array_index (fg_select->stroke,
                                          GimpVector2,
                                          fg_select->stroke->len - 1);

      if (last->x != coords->x || last->y != coords->y)
        {
          GimpVector2 point = gimp_vector2_new (coords->x, coords->y);

          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

          g_array_append_val (fg_select->stroke, point);

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->motion (tool,
                                              coords, time, state, gdisp);
    }
}

static void
gimp_foreground_select_tool_draw (GimpDrawTool *draw_tool)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (draw_tool);

  if (fg_select->stroke)
    gimp_draw_tool_draw_lines (draw_tool,
                               &g_array_index (fg_select->stroke,
                                               GimpVector2, 0),
                               fg_select->stroke->len,
                               FALSE, FALSE);

  if (! fg_select->mask)
    GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
gimp_foreground_select_tool_select (GimpFreeSelectTool *free_sel,
                                    GimpDisplay        *gdisp)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (free_sel);

  GimpImage       *gimage   = gdisp->gimage;
  GimpDrawable    *drawable = gimp_image_active_drawable (gimage);
  GimpScanConvert *scan_convert;
  GimpChannel     *mask;
  GList           *list;

  if (! drawable)
    return;

  gimp_set_busy (gimage->gimp);

  scan_convert = gimp_scan_convert_new ();
  gimp_scan_convert_add_polyline (scan_convert,
                                  free_sel->num_points, free_sel->points, TRUE);

  mask = gimp_channel_new (gimage,
                           gimp_image_get_width (gimage),
                           gimp_image_get_height (gimage),
                           "foreground-extraction", NULL);

  gimp_scan_convert_render_value (scan_convert,
                                  gimp_drawable_data (GIMP_DRAWABLE (mask)),
                                  0, 0, 127);
  gimp_scan_convert_free (scan_convert);

  for (list = fg_select->fg_strokes; list; list = list->next->next->next)
    {
      gint         num_points = GPOINTER_TO_INT (list->data);
      GimpVector2 *points     = list->next->data;
      gint         width      = GPOINTER_TO_INT (list->next->next->data);

      gimp_foreground_select_tool_stroke (mask,
                                          num_points, points, width, TRUE);
    }

  for (list = fg_select->bg_strokes; list; list = list->next->next->next)
    {
      gint         num_points = GPOINTER_TO_INT (list->data);
      GimpVector2 *points     = list->next->data;
      gint         width      = GPOINTER_TO_INT (list->next->next->data);

      gimp_foreground_select_tool_stroke (mask,
                                          num_points, points, width, FALSE);
    }

  gimp_drawable_foreground_extract (drawable,
                                    GIMP_FOREGROUND_EXTRACT_SIOX,
                                    GIMP_DRAWABLE (mask),
                                    GIMP_PROGRESS (gdisp));

  gimp_foreground_select_tool_set_mask (GIMP_FOREGROUND_SELECT_TOOL (free_sel),
                                        gdisp, mask);
  g_object_unref (mask);

  gimp_unset_busy (gimage->gimp);
}

static void
gimp_foreground_select_tool_set_mask (GimpForegroundSelectTool *fg_select,
                                      GimpDisplay              *gdisp,
                                      GimpChannel              *mask)
{
  if (fg_select->mask == mask)
    return;

  if (fg_select->mask)
    {
      g_object_unref (fg_select->mask);
      fg_select->mask = NULL;
    }

  if (mask)
    fg_select->mask = g_object_ref (mask);

  gimp_display_shell_set_mask (GIMP_DISPLAY_SHELL (gdisp->shell),
                               GIMP_DRAWABLE (mask));

  gimp_tool_control_set_toggle (GIMP_TOOL (fg_select)->control, mask != NULL);
}

static void
gimp_foreground_select_tool_apply (GimpForegroundSelectTool *fg_select,
                                   GimpDisplay              *gdisp)
{
  GimpTool             *tool = GIMP_TOOL (fg_select);
  GimpChannelOps        op   = GIMP_SELECTION_TOOL (tool)->op;
  GimpSelectionOptions *options;

  if (! fg_select->mask)
    return;

  if (op > GIMP_CHANNEL_OP_INTERSECT)
    op = GIMP_CHANNEL_OP_REPLACE;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_channel_select_channel (gimp_image_get_mask (gdisp->gimage),
                               tool->tool_info->blurb,
                               fg_select->mask, 0, 0,
                               op,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius);

  gimp_foreground_select_tool_set_mask (fg_select, gdisp, NULL);

  gimp_image_flush (gdisp->gimage);
}

static void
gimp_foreground_select_tool_stroke (GimpChannel  *mask,
                                    gint          num_points,
                                    GimpVector2  *points,
                                    gint          width,
                                    gboolean      foreground)
{
  GimpScanConvert *scan_convert = gimp_scan_convert_new ();
  GimpItem        *item         = GIMP_ITEM (mask);
  GimpChannel     *channel      = gimp_channel_new (gimp_item_get_image (item),
                                                    gimp_item_width (item),
                                                    gimp_item_height (item),
                                                    "tmp", NULL);

  /* FIXME: We should be able to get away w/o a temporary drawable
   *        by doing some changes to GimpScanConvert.
   */

  gimp_scan_convert_add_polyline (scan_convert, num_points, points, FALSE);
  gimp_scan_convert_stroke (scan_convert,
                            width, GIMP_JOIN_MITER, GIMP_CAP_ROUND, 10.0,
                            0.0, NULL);
  gimp_scan_convert_render (scan_convert,
                            gimp_drawable_data (GIMP_DRAWABLE (channel)),
                            0, 0, FALSE);
  gimp_scan_convert_free (scan_convert);

  gimp_channel_combine_mask (mask, channel,
                             foreground ?
                             GIMP_CHANNEL_OP_ADD : GIMP_CHANNEL_OP_SUBTRACT,
                             0, 0);

  g_object_unref (channel);
}

static void
gimp_foreground_select_tool_strokes_free (GList *strokes)
{
  GList *list;

  for (list = strokes; list; list = list->next->next->next)
    g_free (list->next->data);

  g_list_free (strokes);
}
