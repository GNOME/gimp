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

#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scale.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpmagnifyoptions.h"
#include "gimpmagnifytool.h"

#include "libgimp/gimpintl.h"


static void   gimp_magnify_tool_class_init      (GimpMagnifyToolClass *klass);
static void   gimp_magnify_tool_init            (GimpMagnifyTool      *tool);

static void   gimp_magnify_tool_button_press    (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 guint32          time,
						 GdkModifierType  state,
						 GimpDisplay     *gdisp);
static void   gimp_magnify_tool_button_release  (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 guint32          time,
						 GdkModifierType  state,
						 GimpDisplay     *gdisp);
static void   gimp_magnify_tool_motion          (GimpTool        *tool,
                                                 GimpCoords      *coords,
                                                 guint32          time,
						 GdkModifierType  state,
						 GimpDisplay     *gdisp);
static void   gimp_magnify_tool_modifier_key    (GimpTool        *tool,
                                                 GdkModifierType  key,
                                                 gboolean         press,
						 GdkModifierType  state,
						 GimpDisplay     *gdisp);
static void   gimp_magnify_tool_cursor_update   (GimpTool        *tool,
                                                 GimpCoords      *coords,
						 GdkModifierType  state,
						 GimpDisplay     *gdisp);

static void   gimp_magnify_tool_draw            (GimpDrawTool    *draw_tool);

static void   zoom_in                           (gint            *src,
                                                 gint            *dest,
                                                 gint             scale);
static void   zoom_out                          (gint            *src,
                                                 gint            *dest,
                                                 gint             scale);


static GimpDrawToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_magnify_tool_register (GimpToolRegisterCallback  callback,
                            gpointer                  data)
{
  (* callback) (GIMP_TYPE_MAGNIFY_TOOL,
                GIMP_TYPE_MAGNIFY_OPTIONS,
                gimp_magnify_options_gui,
                FALSE,
                "gimp-magnify-tool",
                _("Magnify"),
                _("Zoom in & out"),
                N_("/Tools/Magnify"), NULL,
                NULL, "tools/magnify.html",
                GIMP_STOCK_TOOL_ZOOM,
                data);
}

GType
gimp_magnify_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpMagnifyToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_magnify_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpMagnifyTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_magnify_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpMagnifyTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_magnify_tool_class_init (GimpMagnifyToolClass *klass)
{
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_tool_class;

  tool_class      = GIMP_TOOL_CLASS (klass);
  draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->button_press   = gimp_magnify_tool_button_press;
  tool_class->button_release = gimp_magnify_tool_button_release;
  tool_class->motion         = gimp_magnify_tool_motion;
  tool_class->modifier_key   = gimp_magnify_tool_modifier_key;
  tool_class->cursor_update  = gimp_magnify_tool_cursor_update;

  draw_tool_class->draw      = gimp_magnify_tool_draw;
}

static void
gimp_magnify_tool_init (GimpMagnifyTool *magnify_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (magnify_tool);

  magnify_tool->x = 0;
  magnify_tool->y = 0;
  magnify_tool->w = 0;
  magnify_tool->h = 0;

  gimp_tool_control_set_scroll_lock            (tool->control, TRUE);
  gimp_tool_control_set_snap_to                (tool->control, FALSE);

  gimp_tool_control_set_cursor                 (tool->control,
                                                GIMP_ZOOM_CURSOR);
  gimp_tool_control_set_tool_cursor            (tool->control,
                                                GIMP_ZOOM_TOOL_CURSOR);
  gimp_tool_control_set_cursor_modifier        (tool->control,
                                                GIMP_CURSOR_MODIFIER_PLUS);

  gimp_tool_control_set_toggle_cursor          (tool->control,
                                                GIMP_ZOOM_CURSOR);
  gimp_tool_control_set_toggle_tool_cursor     (tool->control,
                                                GIMP_ZOOM_TOOL_CURSOR);
  gimp_tool_control_set_toggle_cursor_modifier (tool->control,
                                                GIMP_CURSOR_MODIFIER_MINUS);
}

static void
gimp_magnify_tool_button_press (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
				GdkModifierType  state,
				GimpDisplay     *gdisp)
{
  GimpMagnifyTool *magnify;

  magnify = GIMP_MAGNIFY_TOOL (tool);

  magnify->x = coords->x;
  magnify->y = coords->y;
  magnify->w = 0;
  magnify->h = 0;

  gimp_tool_control_activate (tool->control);
  tool->gdisp = gdisp;

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);
}

static void
gimp_magnify_tool_button_release (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
				  GdkModifierType  state,
				  GimpDisplay     *gdisp)
{
  GimpMagnifyTool    *magnify;
  GimpMagnifyOptions *options;
  GimpDisplayShell   *shell;
  gint                win_width, win_height;
  gint                width, height;
  gint                scalesrc, scaledest;
  gint                scale;
  gint                x1, y1, x2, y2, w, h;

  magnify = GIMP_MAGNIFY_TOOL (tool);
  options = GIMP_MAGNIFY_OPTIONS (tool->tool_info->tool_options);

  shell = GIMP_DISPLAY_SHELL (tool->gdisp->shell);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  /*  First take care of the case where the user "cancels" the action  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      x1 = (magnify->w < 0) ? magnify->x + magnify->w : magnify->x;
      y1 = (magnify->h < 0) ? magnify->y + magnify->h : magnify->y;
      w  = (magnify->w < 0) ? -magnify->w : magnify->w;
      h  = (magnify->h < 0) ? -magnify->h : magnify->h;
      x2 = x1 + w;
      y2 = y1 + h;

      /* these change the user zoom level, so should not be changed to
       * the resolution-aware scale macros -- austin
       */
      scalesrc  = SCALESRC (shell);
      scaledest = SCALEDEST (shell);

      win_width  = shell->disp_width;
      win_height = shell->disp_height;
      width  = (win_width  * scalesrc) / scaledest;
      height = (win_height * scalesrc) / scaledest;

      /* we need to compute the mouse movement in screen coordinates */
      if ( (SCALEX (shell, w) < options->threshold) || 
           (SCALEY (shell, h) < options->threshold) )
	scale = 1;
      else
	scale = MIN ((width / w), (height / h));

      magnify->op = options->zoom_type;

      switch (magnify->op)
	{
	case GIMP_ZOOM_IN:
	  zoom_in (&scalesrc, &scaledest, scale);
	  break;
	case GIMP_ZOOM_OUT:
	  zoom_out (&scalesrc, &scaledest, scale);
	  break;
	}

      gimp_display_shell_scale_by_values (shell,
                                          (scaledest << 8) + scalesrc,
                                          ((scaledest * ((x1 + x2) / 2)) / scalesrc -
                                           (win_width / 2)),
                                          ((scaledest * ((y1 + y2) / 2)) / scalesrc -
                                           (win_height / 2)),
                                          options->allow_resize);
    }
}

static void
gimp_magnify_tool_motion (GimpTool        *tool,
                          GimpCoords      *coords,
                          guint32          time,
			  GdkModifierType  state,
			  GimpDisplay     *gdisp)
{
  GimpMagnifyTool *magnify;

  if (!gimp_tool_control_is_active (tool->control))
    return;

  magnify = GIMP_MAGNIFY_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  magnify->w = (coords->x - magnify->x);
  magnify->h = (coords->y - magnify->y);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_magnify_tool_modifier_key (GimpTool        *tool,
                                GdkModifierType  key,
                                gboolean         press,
				GdkModifierType  state,
				GimpDisplay     *gdisp)
{
  GimpMagnifyOptions *options;

  options = GIMP_MAGNIFY_OPTIONS (tool->tool_info->tool_options);

  if (key == GDK_CONTROL_MASK)
    {
      switch (options->zoom_type)
        {
        case GIMP_ZOOM_IN:
          g_object_set (options, "zoom-type", GIMP_ZOOM_OUT, NULL);
          break;

        case GIMP_ZOOM_OUT:
          g_object_set (options, "zoom-type", GIMP_ZOOM_IN, NULL);
          break;

        default:
          break;
        }
    }
}

static void
gimp_magnify_tool_cursor_update (GimpTool        *tool,
                                 GimpCoords      *coords,
				 GdkModifierType  state,
				 GimpDisplay     *gdisp)
{
  GimpMagnifyOptions *options;

  options = GIMP_MAGNIFY_OPTIONS (tool->tool_info->tool_options);

  gimp_tool_control_set_toggle (tool->control,
                                options->zoom_type == GIMP_ZOOM_OUT);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_magnify_tool_draw (GimpDrawTool *draw_tool)
{
  GimpMagnifyTool    *magnify;
  GimpMagnifyOptions *options;

  magnify = GIMP_MAGNIFY_TOOL (draw_tool);
  options = GIMP_MAGNIFY_OPTIONS (GIMP_TOOL (draw_tool)->tool_info->tool_options);

  gimp_draw_tool_draw_rectangle (draw_tool,
                                 FALSE,
                                 magnify->x,
                                 magnify->y,
                                 magnify->w,
                                 magnify->h,
                                 FALSE);
}


/*  magnify utility functions  */

static void
zoom_in (gint *src,
	 gint *dest,
	 gint  scale)
{
  while (scale--)
    {
      if (*src > 1)
	(*src)--;
      else
	if (*dest < 0x10)
	  (*dest)++;
    }
}


static void
zoom_out (gint *src,
	  gint *dest,
	  gint  scale)
{
  while (scale--)
    {
      if (*dest > 1)
	(*dest)--;
      else
	if (*src < 0x10)
	  (*src)++;
    }
}
