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

#include "core/core-types.h"
#include "display/display-types.h"
#include "libgimptool/gimptooltypes.h"

#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scale.h"

#include "widgets/gimpenummenu.h"

#include "gimpmagnifytool.h"
#include "tool_options.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


typedef struct _MagnifyOptions MagnifyOptions;

struct _MagnifyOptions
{
  GimpToolOptions   tool_options;

  gint          allow_resize;  /* default from gimprc.resize_windows_on_zoom */
  GtkWidget    *allow_resize_w;

  GimpZoomType  type;
  GimpZoomType  type_d;
  GtkWidget    *type_w;

  gdouble       threshold;
  gdouble       threshold_d;
  GtkObject    *threshold_w;
};


/*  magnify action functions  */
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

static GimpToolOptions * magnify_options_new    (GimpToolInfo    *tool_info);
static void              magnify_options_reset  (GimpToolOptions *tool_options);


static GimpDrawToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_magnify_tool_register (GimpToolRegisterCallback  callback,
                            Gimp                     *gimp)
{
  (* callback) (GIMP_TYPE_MAGNIFY_TOOL,
                magnify_options_new,
                FALSE,
                "gimp-magnify-tool",
                _("Magnify"),
                _("Zoom in & out"),
                N_("/Tools/Magnify"), NULL,
                NULL, "tools/magnify.html",
                GIMP_STOCK_TOOL_ZOOM,
                gimp);
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
  gimp_tool_control_set_cursor                 (tool->control, GIMP_ZOOM_CURSOR);
  gimp_tool_control_set_tool_cursor            (tool->control, GIMP_ZOOM_TOOL_CURSOR);
  gimp_tool_control_set_cursor_modifier        (tool->control, GIMP_CURSOR_MODIFIER_PLUS);
  gimp_tool_control_set_tool_cursor            (tool->control, GIMP_ZOOM_CURSOR);
  gimp_tool_control_set_toggle_tool_cursor     (tool->control, GIMP_ZOOM_TOOL_CURSOR);
  gimp_tool_control_set_toggle_cursor_modifier (tool->control, GIMP_CURSOR_MODIFIER_MINUS);

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

  gimp_tool_control_activate(tool->control);
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
  GimpMagnifyTool  *magnify;
  MagnifyOptions   *options;
  GimpDisplayShell *shell;
  gint              win_width, win_height;
  gint              width, height;
  gint              scalesrc, scaledest;
  gint              scale;
  gint              x1, y1, x2, y2, w, h;

  magnify = GIMP_MAGNIFY_TOOL (tool);

  options = (MagnifyOptions *) tool->tool_info->tool_options;

  shell = GIMP_DISPLAY_SHELL (tool->gdisp->shell);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt(tool->control);    /* sets paused_count to 0 -- is this ok? */

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
      scalesrc  = SCALESRC (gdisp);
      scaledest = SCALEDEST (gdisp);

      win_width  = shell->disp_width;
      win_height = shell->disp_height;
      width  = (win_width  * scalesrc) / scaledest;
      height = (win_height * scalesrc) / scaledest;

      /* we need to compute the mouse movement in screen coordinates */
      if ( (SCALEX (gdisp, w) < options->threshold) || 
           (SCALEY (gdisp, h) < options->threshold) )
	scale = 1;
      else
	scale = MIN ((width / w), (height / h));

      magnify->op = options->type;

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

  if (!gimp_tool_control_is_active(tool->control))
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
  MagnifyOptions *options;

  options = (MagnifyOptions *) tool->tool_info->tool_options;

  if (key == GDK_CONTROL_MASK)
    {
      switch (options->type)
        {
        case GIMP_ZOOM_IN:
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                                       GINT_TO_POINTER (GIMP_ZOOM_OUT));
          break;
        case GIMP_ZOOM_OUT:
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                                       GINT_TO_POINTER (GIMP_ZOOM_IN));
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
  MagnifyOptions *options;

  options = (MagnifyOptions *) tool->tool_info->tool_options;

  gimp_tool_control_set_toggle(tool->control, (options->type == GIMP_ZOOM_OUT));

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_magnify_tool_draw (GimpDrawTool *draw_tool)
{
  GimpMagnifyTool *magnify;
  MagnifyOptions  *options;

  magnify = GIMP_MAGNIFY_TOOL (draw_tool);
  options = (MagnifyOptions *) GIMP_TOOL (draw_tool)->tool_info->tool_options;

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


/*  magnify tool options functions  */

static GimpToolOptions *
magnify_options_new (GimpToolInfo *tool_info)
{
  MagnifyOptions *options;
  GtkWidget      *vbox;
  GtkWidget      *frame;
  GtkWidget      *table;

  options = g_new0 (MagnifyOptions, 1);

  tool_options_init ((GimpToolOptions *) options, tool_info);

  ((GimpToolOptions *) options)->reset_func = magnify_options_reset;

  options->allow_resize = gimprc.resize_windows_on_zoom;
  options->type_d       = options->type         = GIMP_ZOOM_IN;
  options->threshold_d  = options->threshold    = 5;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the allow_resize toggle button  */
  options->allow_resize_w =
    gtk_check_button_new_with_label (_("Allow Window Resizing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_resize_w),
				options->allow_resize);
  gtk_box_pack_start (GTK_BOX (vbox),  options->allow_resize_w,
                      FALSE, FALSE, 0);
  gtk_widget_show (options->allow_resize_w);

  g_signal_connect (G_OBJECT (options->allow_resize_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &options->allow_resize);

  /*  tool toggle  */
  frame = gimp_enum_radio_frame_new (GIMP_TYPE_ZOOM_TYPE,
                                     gtk_label_new (_("Tool Toggle (<Ctrl>)")),
                                     2,
                                     G_CALLBACK (gimp_radio_button_update),
                                     &options->type,
                                     &options->type_w);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type));

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  window threshold */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  options->threshold_w = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
					       _("Threshold:"), -1, 50,
					       options->threshold,
					       1.0, 15.0, 1.0, 3.0, 1,
					       TRUE, 0.0, 0.0,
					       NULL, NULL);

  g_signal_connect (G_OBJECT (options->threshold_w), "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &options->threshold);

  return (GimpToolOptions *) options;
}

static void
magnify_options_reset (GimpToolOptions *tool_options)
{
  MagnifyOptions *options;

  options = (MagnifyOptions *) tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_resize_w),
				gimprc.resize_windows_on_zoom);

  gimp_radio_group_set_active (GTK_RADIO_BUTTON (options->type_w),
                               GINT_TO_POINTER (options->type_d));

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
			    options->threshold_d);
}
