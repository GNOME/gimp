/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "base/boundary.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppaintinfo.h"
#include "core/gimptoolinfo.h"
#include "core/gimpunit.h"

#include "paint/gimpbrushcore.h"
#include "paint/gimppaintcore.h"
#include "paint/gimppaintoptions.h"

#include "widgets/gimpdevices.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpstatusbar.h"

#include "gimpcoloroptions.h"
#include "gimpcolorpickertool.h"
#include "gimppainttool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define TARGET_SIZE    15
#define STATUSBAR_SIZE 128


static void   gimp_paint_tool_class_init     (GimpPaintToolClass  *klass);
static void   gimp_paint_tool_init           (GimpPaintTool       *paint_tool);

static GObject * gimp_paint_tool_constructor (GType                type,
                                              guint                n_params,
                                              GObjectConstructParam *params);
static void   gimp_paint_tool_finalize       (GObject             *object);

static void   gimp_paint_tool_control        (GimpTool	          *tool,
					      GimpToolAction       action,
					      GimpDisplay         *gdisp);
static void   gimp_paint_tool_button_press   (GimpTool            *tool,
                                              GimpCoords          *coords,
                                              guint32              time,
					      GdkModifierType      state,
					      GimpDisplay         *gdisp);
static void   gimp_paint_tool_button_release (GimpTool            *tool,
                                              GimpCoords          *coords,
                                              guint32              time,
					      GdkModifierType      state,
					      GimpDisplay         *gdisp);
static void   gimp_paint_tool_motion         (GimpTool            *tool,
                                              GimpCoords          *coords,
                                              guint32              time,
					      GdkModifierType      state,
					      GimpDisplay         *gdisp);
static void   gimp_paint_tool_arrow_key      (GimpTool            *tool,
                                              GdkEventKey         *kevent,
                                              GimpDisplay         *gdisp);
static void   gimp_paint_tool_modifier_key   (GimpTool            *tool,
                                              GdkModifierType      key,
                                              gboolean             press,
                                              GdkModifierType      state,
                                              GimpDisplay         *gdisp);
static void   gimp_paint_tool_oper_update    (GimpTool            *tool,
                                              GimpCoords          *coords,
					      GdkModifierType      state,
					      GimpDisplay         *gdisp);

static void   gimp_paint_tool_draw           (GimpDrawTool        *draw_tool);

static void   gimp_paint_tool_color_picked   (GimpColorTool       *color_tool,
                                              GimpImageType        sample_type,
                                              GimpRGB             *color,
                                              gint                 color_index);

static void   gimp_paint_tool_notify_brush   (GimpDisplayConfig   *config,
                                              GParamSpec          *pspec,
                                              GimpPaintTool       *paint_tool);


static GimpColorToolClass *parent_class = NULL;


GType
gimp_paint_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpPaintToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_paint_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPaintTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_paint_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_COLOR_TOOL,
					  "GimpPaintTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_paint_tool_class_init (GimpPaintToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpToolClass      *tool_class       = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass  *draw_tool_class  = GIMP_DRAW_TOOL_CLASS (klass);
  GimpColorToolClass *color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);;

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_paint_tool_constructor;
  object_class->finalize     = gimp_paint_tool_finalize;

  tool_class->control        = gimp_paint_tool_control;
  tool_class->button_press   = gimp_paint_tool_button_press;
  tool_class->button_release = gimp_paint_tool_button_release;
  tool_class->motion         = gimp_paint_tool_motion;
  tool_class->arrow_key      = gimp_paint_tool_arrow_key;
  tool_class->modifier_key   = gimp_paint_tool_modifier_key;
  tool_class->oper_update    = gimp_paint_tool_oper_update;

  draw_tool_class->draw      = gimp_paint_tool_draw;

  color_tool_class->picked   = gimp_paint_tool_color_picked;
}

static void
gimp_paint_tool_init (GimpPaintTool *paint_tool)
{
  GimpTool *tool = GIMP_TOOL (paint_tool);

  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_EXACT);

  paint_tool->pick_colors = FALSE;
  paint_tool->draw_line   = FALSE;

  paint_tool->draw_brush  = TRUE;
  paint_tool->brush_x     = 0.0;
  paint_tool->brush_y     = 0.0;

  paint_tool->core        = NULL;
}

static GObject *
gimp_paint_tool_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tool       = GIMP_TOOL (object);
  paint_tool = GIMP_PAINT_TOOL (object);

  g_assert (GIMP_IS_TOOL_INFO (tool->tool_info));

  paint_tool->draw_brush =
    GIMP_DISPLAY_CONFIG (tool->tool_info->gimp->config)->show_brush_outline;

  g_signal_connect_object (tool->tool_info->gimp->config,
                           "notify::show-brush-outline",
                           G_CALLBACK (gimp_paint_tool_notify_brush),
                           paint_tool, 0);

  paint_tool->core = g_object_new (tool->tool_info->paint_info->paint_type,
                                   NULL);

  return object;
}

static void
gimp_paint_tool_finalize (GObject *object)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (object);

  if (paint_tool->core)
    {
      g_object_unref (paint_tool->core);
      paint_tool->core = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_paint_tool_enable_color_picker:
 * @tool: a #GimpPaintTool
 * @mode: the #GimpColorPickMode to set
 *
 * This is a convenience function used from the init method of paint
 * tools that want the color picking functionality. The @mode that is
 * set here is used to decide what cursor modifier to draw and if the
 * picked color goes to the foreground or background color.
 **/

void
gimp_paint_tool_enable_color_picker (GimpPaintTool     *tool,
                                     GimpColorPickMode  mode)
{
  g_return_if_fail (GIMP_IS_PAINT_TOOL (tool));

  tool->pick_colors = TRUE;

  GIMP_COLOR_TOOL (tool)->pick_mode = mode;
}


static void
gimp_paint_tool_control (GimpTool       *tool,
			 GimpToolAction  action,
			 GimpDisplay    *gdisp)
{
  GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpDrawable  *drawable;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  switch (action)
    {
    case HALT:
      gimp_paint_core_paint (paint_tool->core,
                             drawable,
                             GIMP_PAINT_OPTIONS (tool->tool_info->tool_options),
                             FINISH_PAINT, 0);
      gimp_paint_core_cleanup (paint_tool->core);

#if 0
      /*  evil hack i'm thinking about...  --mitch  */
      {
        /*  HALT means the current display is going to go away (TM),
         *  so try to find another display of the same image to make
         *  straight line drawing continue to work...
         */

        GSList *list;

        for (list = display_list; list; list = g_slist_next (list))
          {
            GimpDisplay *tmp_disp;

            tmp_disp = (GimpDisplay *) list->data;

            if (tmp_disp != gdisp && tmp_disp->gimage == gdisp->gimage)
              {
                tool->gdisp = tmp_disp;
                break;
              }
          }
      }
#endif
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

/**
 * gimp_paint_tool_round_line:
 * @core:          the #GimpPaintCore
 * @center_pixels: push coordinates to pixel centers?
 * @state:         the modifier state
 *
 * Adjusts core->last_coords and core_cur_coords in preparation to
 * drawing a straight line. If @center_pixels is TRUE the endpoints
 * get pushed to the center of the pixels. This avoids artefacts
 * for e.g. the hard mode. The rounding of the slope to 15 degree
 * steps if ctrl is pressed happens, as does rounding the start and
 * end coordinates (which may be fractional in high zoom modes) to
 * the center of pixels.
 **/
static void
gimp_paint_tool_round_line (GimpPaintCore   *core,
                            gboolean         center_pixels,
                            GdkModifierType  state)
{
  if (center_pixels)
    {
      core->last_coords.x = floor (core->last_coords.x) + 0.5;
      core->last_coords.y = floor (core->last_coords.y) + 0.5;
      core->cur_coords.x  = floor (core->cur_coords.x ) + 0.5;
      core->cur_coords.y  = floor (core->cur_coords.y ) + 0.5;
    }

  /* Restrict to multiples of 15 degrees if ctrl is pressed */
  if (state & GDK_CONTROL_MASK)
    gimp_paint_core_constrain (core);
}

static void
gimp_paint_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  GimpDrawTool     *draw_tool  = GIMP_DRAW_TOOL (tool);
  GimpPaintTool    *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpPaintOptions *paint_options;
  GimpPaintCore    *core;
  GimpBrush        *current_brush = NULL;
  GimpDrawable     *drawable;
  GdkDisplay       *gdk_display;
  GimpCoords        curr_coords;
  gint              off_x, off_y;

  paint_options = GIMP_PAINT_OPTIONS (tool->tool_info->tool_options);

  core = paint_tool->core;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  curr_coords = *coords;

  gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

  curr_coords.x -= off_x;
  curr_coords.y -= off_y;

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  if (tool->gdisp          &&
      tool->gdisp != gdisp &&
      tool->gdisp->gimage == gdisp->gimage)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->gdisp = gdisp;
    }

  gdk_display = gtk_widget_get_display (gdisp->shell);

  core->use_pressure = (gimp_devices_get_current (gdisp->gimage->gimp) !=
                        gdk_display_get_core_pointer (gdk_display));

  if (! gimp_paint_core_start (core, drawable, paint_options, &curr_coords))
    return;

  if ((gdisp != tool->gdisp) || ! paint_tool->draw_line)
    {
      /*  if this is a new image, reinit the core vals  */

      core->start_coords = core->cur_coords;
      core->last_coords  = core->cur_coords;

      core->distance     = 0.0;
      core->pixel_dist   = 0.0;
    }
  else if (paint_tool->draw_line)
    {
      /*  If shift is down and this is not the first paint
       *  stroke, then draw a line from the last coords to the pointer
       */
      gboolean hard;

      core->start_coords = core->last_coords;

      hard = (gimp_paint_options_get_brush_mode (paint_options) ==
              GIMP_BRUSH_HARD);
      gimp_paint_tool_round_line (core, hard, state);
    }

  /*  let the parent class activate the tool  */
  GIMP_TOOL_CLASS (parent_class)->button_press (tool,
                                                coords, time, state, gdisp);

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    return;

  /*  pause the current selection  */
  gimp_image_selection_control (gdisp->gimage, GIMP_SELECTION_PAUSE);

  /*  Let the specific painting function initialize itself  */
  gimp_paint_core_paint (core, drawable, paint_options, INIT_PAINT, time);

  /*  store the current brush pointer  */
  if (GIMP_IS_BRUSH_CORE (core))
    current_brush = GIMP_BRUSH_CORE (core)->brush;

  if (GIMP_PAINT_CORE_GET_CLASS (core)->traces_on_window)
    gimp_paint_core_paint (core, drawable, paint_options, PRETRACE_PAINT, time);

  /*  Paint to the image  */
  if (paint_tool->draw_line)
    {
      gimp_paint_core_interpolate (core, drawable, paint_options, time);
    }
  else
    {
      gimp_paint_core_paint (core, drawable, paint_options, MOTION_PAINT, time);
    }

  gimp_display_flush_now (gdisp);

  if (GIMP_PAINT_CORE_GET_CLASS (core)->traces_on_window)
    gimp_paint_core_paint (core, drawable, paint_options, POSTTRACE_PAINT, time);

  /*  restore the current brush pointer  */
  if (GIMP_IS_BRUSH_CORE (core))
    GIMP_BRUSH_CORE (core)->brush = current_brush;
}

static void
gimp_paint_tool_button_release (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
				GdkModifierType  state,
				GimpDisplay     *gdisp)
{
  GimpPaintTool    *paint_tool;
  GimpPaintOptions *paint_options;
  GimpPaintCore    *core;
  GimpDrawable     *drawable;

  paint_tool    = GIMP_PAINT_TOOL (tool);
  paint_options = GIMP_PAINT_OPTIONS (tool->tool_info->tool_options);

  core = paint_tool->core;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  /*  Let the specific painting function finish up  */
  gimp_paint_core_paint (core, drawable, paint_options, FINISH_PAINT, time);

  /*  resume the current selection  */
  gimp_image_selection_control (gdisp->gimage, GIMP_SELECTION_RESUME);

  /*  chain up to halt the tool */
  GIMP_TOOL_CLASS (parent_class)->button_release (tool,
                                                  coords, time, state, gdisp);

  if (state & GDK_BUTTON3_MASK)
    gimp_paint_core_cancel (core, drawable);
  else
    gimp_paint_core_finish (core, drawable);

  gimp_image_flush (gdisp->gimage);
}

static void
gimp_paint_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
			GdkModifierType  state,
			GimpDisplay     *gdisp)
{
  GimpPaintTool    *paint_tool = GIMP_PAINT_TOOL (tool);
  GimpPaintOptions *paint_options;
  GimpPaintCore    *core;
  GimpDrawable     *drawable;
  gint              off_x, off_y;

  paint_options = GIMP_PAINT_OPTIONS (tool->tool_info->tool_options);

  core = paint_tool->core;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  core->cur_coords = *coords;

  gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

  core->cur_coords.x -= off_x;
  core->cur_coords.y -= off_y;

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, gdisp);

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
    return;

  if (GIMP_PAINT_CORE_GET_CLASS (core)->traces_on_window)
    gimp_paint_core_paint (core, drawable, paint_options, PRETRACE_PAINT, time);

  gimp_paint_core_interpolate (core, drawable, paint_options, time);

  gimp_display_flush_now (gdisp);

  if (GIMP_PAINT_CORE_GET_CLASS (core)->traces_on_window)
    gimp_paint_core_paint (core, drawable, paint_options, POSTTRACE_PAINT, time);
}

static void
gimp_paint_tool_arrow_key (GimpTool     *tool,
                           GdkEventKey  *kevent,
                           GimpDisplay  *gdisp)
{
  if (tool->gdisp)
    {
      GimpContext *context;
      gdouble opacity;

      context = gimp_get_user_context (tool->gdisp->gimage->gimp);
      opacity = gimp_context_get_opacity (context);
      switch (kevent->keyval)
        {
          case GDK_Left:
            opacity = CLAMP (opacity - 0.01, 0, 1);
            break;
          case GDK_Right:
            opacity = CLAMP (opacity + 0.01, 0, 1);
            break;
          case GDK_Up:
            opacity = CLAMP (opacity + 0.1, 0, 1);
            break;
          case GDK_Down:
            opacity = CLAMP (opacity - 0.1, 0, 1);
            break;
          default:
            break;
        }
      gimp_context_set_opacity (context, opacity);
    }
}

static void
gimp_paint_tool_modifier_key (GimpTool        *tool,
                              GdkModifierType  key,
                              gboolean         press,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpPaintTool *paint_tool;
  GimpDrawTool  *draw_tool;

  paint_tool = GIMP_PAINT_TOOL (tool);
  draw_tool  = GIMP_DRAW_TOOL (tool);

  if (key != GDK_CONTROL_MASK)
    return;

  if (paint_tool->pick_colors && ! paint_tool->draw_line)
    {
      if (press)
        {
          GimpContainer *tool_info_list;
          GimpToolInfo  *info;

          tool_info_list = gdisp->gimage->gimp->tool_info_list;

          info = (GimpToolInfo *)
            gimp_container_get_child_by_name (tool_info_list,
                                              "gimp-color-picker-tool");

          if (GIMP_IS_TOOL_INFO (info))
            {
              if (gimp_draw_tool_is_active (draw_tool))
                gimp_draw_tool_stop (draw_tool);

              gimp_color_tool_enable (GIMP_COLOR_TOOL (tool),
                                      GIMP_COLOR_OPTIONS (info->tool_options));
            }
        }
      else
        {
          if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (tool)))
            gimp_color_tool_disable (GIMP_COLOR_TOOL (tool));
        }
    }
}

static void
gimp_paint_tool_oper_update (GimpTool        *tool,
                             GimpCoords      *coords,
                             GdkModifierType  state,
                             GimpDisplay     *gdisp)
{
  GimpPaintTool    *paint_tool;
  GimpPaintOptions *paint_options;
  GimpDrawTool     *draw_tool;
  GimpPaintCore    *core;
  GimpDisplayShell *shell;
  GimpDrawable     *drawable;

  paint_tool = GIMP_PAINT_TOOL (tool);
  draw_tool  = GIMP_DRAW_TOOL (tool);
  paint_options = GIMP_PAINT_OPTIONS (tool->tool_info->tool_options);

  if (gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (draw_tool)))
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, gdisp);
      return;
    }

  core = paint_tool->core;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (gimp_draw_tool_is_active (draw_tool))
    gimp_draw_tool_stop (draw_tool);

  gimp_statusbar_pop (GIMP_STATUSBAR (shell->statusbar),
                      g_type_name (G_TYPE_FROM_INSTANCE (tool)));

  if (tool->gdisp          &&
      tool->gdisp != gdisp &&
      tool->gdisp->gimage == gdisp->gimage)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->gdisp = gdisp;
    }

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (drawable && shell->proximity)
    {
      if (gdisp == tool->gdisp && (state & GDK_SHIFT_MASK))
        {
          /*  If shift is down and this is not the first paint stroke,
           *  draw a line
           */

          gdouble  dx, dy, dist;
          gchar    status_str[STATUSBAR_SIZE];
          gint     off_x, off_y;
          gboolean hard;

          core->cur_coords = *coords;

          gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

          core->cur_coords.x -= off_x;
          core->cur_coords.y -= off_y;

          hard = (gimp_paint_options_get_brush_mode (paint_options) ==
                  GIMP_BRUSH_HARD);
          gimp_paint_tool_round_line (core, hard, state);

          dx = core->cur_coords.x - core->last_coords.x;
          dy = core->cur_coords.y - core->last_coords.y;

          /*  show distance in statusbar  */
          if (shell->unit == GIMP_UNIT_PIXEL)
            {
              dist = sqrt (SQR (dx) + SQR (dy));

              g_snprintf (status_str, sizeof (status_str), "%.1f %s",
                          dist, _("pixels"));
            }
          else
            {
              GimpImage *image = gdisp->gimage;
              gchar      format_str[64];

              g_snprintf (format_str, sizeof (format_str), "%%.%df %s",
                          _gimp_unit_get_digits (image->gimp, shell->unit),
                          _gimp_unit_get_symbol (image->gimp, shell->unit));

              dist = (_gimp_unit_get_factor (image->gimp, shell->unit) *
                      sqrt (SQR (dx / image->xresolution) +
                            SQR (dy / image->yresolution)));

              g_snprintf (status_str, sizeof (status_str), format_str, dist);
            }

          gimp_statusbar_push (GIMP_STATUSBAR (shell->statusbar),
                               g_type_name (G_TYPE_FROM_INSTANCE (tool)),
                               status_str);

          paint_tool->brush_x = core->cur_coords.x + off_x;
          paint_tool->brush_y = core->cur_coords.y + off_y;

          paint_tool->draw_line = TRUE;
        }
      else
        {
          paint_tool->brush_x = coords->x;
          paint_tool->brush_y = coords->y;

          paint_tool->draw_line = FALSE;
        }

      gimp_draw_tool_start (draw_tool, gdisp);
    }

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, gdisp);
}

static void
gimp_paint_tool_draw (GimpDrawTool *draw_tool)
{
  if (! gimp_color_tool_is_enabled (GIMP_COLOR_TOOL (draw_tool)))
    {
      GimpPaintTool *paint_tool = GIMP_PAINT_TOOL (draw_tool);
      GimpPaintCore *core       = paint_tool->core;

      if (paint_tool->draw_line)
        {
          /*  Draw start target  */
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CROSS,
                                      core->last_coords.x,
                                      core->last_coords.y,
                                      TARGET_SIZE,
                                      TARGET_SIZE,
                                      GTK_ANCHOR_CENTER,
                                      TRUE);

          /*  Draw end target  */
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_CROSS,
                                      core->cur_coords.x,
                                      core->cur_coords.y,
                                      TARGET_SIZE,
                                      TARGET_SIZE,
                                      GTK_ANCHOR_CENTER,
                                      TRUE);

          /*  Draw the line between the start and end coords  */
          gimp_draw_tool_draw_line (draw_tool,
                                    core->last_coords.x,
                                    core->last_coords.y,
                                    core->cur_coords.x,
                                    core->cur_coords.y,
                                    TRUE);
        }

      if (paint_tool->draw_brush && GIMP_IS_BRUSH_CORE (core))
        {
          GimpBrushCore *brush_core = GIMP_BRUSH_CORE (core);
          GimpContext   *context;
          GimpBrush     *brush;
          TempBuf       *mask;

          context =
            GIMP_CONTEXT (GIMP_TOOL (draw_tool)->tool_info->tool_options);

          brush = gimp_context_get_brush (context);
          mask  = gimp_brush_get_mask (brush);

          if (brush != brush_core->grr_brush && brush_core->brush_bound_segs)
            {
              g_free (brush_core->brush_bound_segs);
              brush_core->brush_bound_segs   = NULL;
              brush_core->n_brush_bound_segs = 0;
            }

          if (! brush_core->brush_bound_segs)
            {
              PixelRegion PR = { 0, };

              PR.data      = temp_buf_data (mask);
              PR.x         = 0;
              PR.y         = 0;
              PR.w         = mask->width;
              PR.h         = mask->height;
              PR.bytes     = mask->bytes;
              PR.rowstride = PR.w * PR.bytes;

              brush_core->brush_bound_segs =
                find_mask_boundary (&PR, &brush_core->n_brush_bound_segs,
                                    WithinBounds,
                                    0, 0,
                                    PR.w, PR.h,
                                    0);
            }

          if (brush_core->brush_bound_segs)
            {
              GimpPaintOptions *paint_options;
              gdouble           brush_x, brush_y;

              paint_options = GIMP_PAINT_OPTIONS (context);

              brush_x = paint_tool->brush_x - ((gdouble) mask->width  / 2.0);
              brush_y = paint_tool->brush_y - ((gdouble) mask->height / 2.0);

              if (gimp_paint_options_get_brush_mode (paint_options) == GIMP_BRUSH_HARD)
                {
#define EPSILON 0.000001

                  /*  Add EPSILON before rounding since e.g.
                   *  (5.0 - 0.5) may end up at (4.499999999....)
                   *  due to floating point fnords
                   */
                  brush_x = RINT (brush_x + EPSILON);
                  brush_y = RINT (brush_y + EPSILON);

#undef EPSILON
                }

              gimp_draw_tool_draw_boundary (draw_tool,
                                            brush_core->brush_bound_segs,
                                            brush_core->n_brush_bound_segs,
                                            brush_x,
                                            brush_y,
                                            FALSE);
            }
        }
    }

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
gimp_paint_tool_color_picked (GimpColorTool *color_tool,
                              GimpImageType  sample_type,
                              GimpRGB       *color,
                              gint           color_index)
{
  GimpTool *tool = GIMP_TOOL (color_tool);

  if (tool->gdisp)
    {
      GimpContext *context;

      context = gimp_get_user_context (tool->gdisp->gimage->gimp);

      switch (color_tool->pick_mode)
        {
        case GIMP_COLOR_PICK_MODE_NONE:
          break;

        case GIMP_COLOR_PICK_MODE_FOREGROUND:
          gimp_context_set_foreground (context, color);
          break;

        case GIMP_COLOR_PICK_MODE_BACKGROUND:
          gimp_context_set_background (context, color);
          break;
        }
    }
}

static void
gimp_paint_tool_notify_brush (GimpDisplayConfig *config,
                              GParamSpec        *pspec,
                              GimpPaintTool     *paint_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (paint_tool));

  paint_tool->draw_brush = config->show_brush_outline;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (paint_tool));
}
