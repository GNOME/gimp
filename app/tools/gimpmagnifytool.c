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

#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scale.h"

#include "gimpmagnifytool.h"
#include "tool_options.h"
#include "tool_manager.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


typedef struct _MagnifyOptions MagnifyOptions;

struct _MagnifyOptions
{
  GimpToolOptions   tool_options;

  /* gint       resize_windows_on_zoom; (from gimprc) */
  gint          allow_resize_d;
  GtkWidget    *allow_resize_w;

  GimpZoomType  type;
  GimpZoomType  type_d;
  GtkWidget    *type_w[2];
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

static MagnifyOptions * magnify_options_new     (void);
static void             magnify_options_reset   (GimpToolOptions *tool_options);


static MagnifyOptions *magnify_options = NULL;

static GimpDrawToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_magnify_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_MAGNIFY_TOOL,
                              FALSE,
			      "gimp:magnify_tool",
			      _("Magnify"),
			      _("Zoom in & out"),
			      N_("/Tools/Magnify"), NULL,
			      NULL, "tools/magnify.html",
			      GIMP_STOCK_TOOL_ZOOM);
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

  if (! magnify_options)
    {
      magnify_options = magnify_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_MAGNIFY_TOOL,
					  (GimpToolOptions *) magnify_options);
    }

  magnify_tool->x = 0;
  magnify_tool->y = 0;
  magnify_tool->w = 0;
  magnify_tool->h = 0;

  tool->tool_cursor  = GIMP_ZOOM_TOOL_CURSOR;

  tool->scroll_lock  = TRUE;   /*  Disallow scrolling    */
  tool->auto_snap_to = FALSE;  /*  Don't snap to guides  */
}

static void
gimp_magnify_tool_button_press (GimpTool        *tool,
                                GimpCoords      *coords,
                                guint32          time,
				GdkModifierType  state,
				GimpDisplay     *gdisp)
{
  GimpMagnifyTool  *magnify;
  GimpDisplayShell *shell;

  magnify = GIMP_MAGNIFY_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  magnify->x = coords->x;
  magnify->y = coords->y;
  magnify->w = 0;
  magnify->h = 0;

  tool->state = ACTIVE;
  tool->gdisp = gdisp;

  gdk_pointer_grab (shell->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, time);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), shell->canvas->window);
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

  gdk_pointer_ungrab (time);
  gdk_flush ();

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  tool->state = INACTIVE;

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

      if (!w || !h)
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

      gdisp->scale = (scaledest << 8) + scalesrc;

      shell->offset_x = ((scaledest * ((x1 + x2) / 2)) / scalesrc -
                         (win_width / 2));
      shell->offset_y = ((scaledest * ((y1 + y2) / 2)) / scalesrc -
                         (win_height / 2));

      /*  resize the image  */
      gimp_display_shell_scale_resize (shell, gimprc.resize_windows_on_zoom,
                                       TRUE);
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

  if (tool->state != ACTIVE)
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
          gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON (options->type_w[GIMP_ZOOM_OUT]), TRUE);
          break;
        case GIMP_ZOOM_OUT:
          gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON (options->type_w[GIMP_ZOOM_IN]), TRUE);
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
  MagnifyOptions   *options;
  GimpDisplayShell *shell;

  options = (MagnifyOptions *) tool->tool_info->tool_options;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  if (options->type == GIMP_ZOOM_IN)
    {
      gimp_display_shell_install_tool_cursor (shell,
                                              GIMP_ZOOM_CURSOR,
                                              GIMP_ZOOM_TOOL_CURSOR,
                                              GIMP_CURSOR_MODIFIER_PLUS);
    }
  else
    {
      gimp_display_shell_install_tool_cursor (shell,
                                              GIMP_ZOOM_CURSOR,
                                              GIMP_ZOOM_TOOL_CURSOR,
                                              GIMP_CURSOR_MODIFIER_MINUS);
   }
}

static void
gimp_magnify_tool_draw (GimpDrawTool *draw_tool)
{
  GimpMagnifyTool *magnify;

  magnify = GIMP_MAGNIFY_TOOL (draw_tool);

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

static MagnifyOptions *
magnify_options_new (void)
{
  MagnifyOptions *options;

  GtkWidget *vbox;
  GtkWidget *frame;

  /*  the new magnify tool options structure  */
  options = g_new0 (MagnifyOptions, 1);

  tool_options_init ((GimpToolOptions *) options,
		     magnify_options_reset);

  options->allow_resize_d = gimprc.resize_windows_on_zoom;
  options->type_d         = options->type = GIMP_ZOOM_IN;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the allow_resize toggle button  */
  options->allow_resize_w =
    gtk_check_button_new_with_label (_("Allow Window Resizing"));
  g_signal_connect (G_OBJECT (options->allow_resize_w), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &(gimprc.resize_windows_on_zoom));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_resize_w),
				gimprc.resize_windows_on_zoom);
  gtk_box_pack_start (GTK_BOX (vbox), 
                      options->allow_resize_w, FALSE, FALSE, 0);
  gtk_widget_show (options->allow_resize_w);

  /*  tool toggle  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Tool Toggle"),
                           G_CALLBACK (gimp_radio_button_update),
                           &options->type,
			   (gpointer) options->type,

                           _("Zoom in"),  (gpointer) GIMP_ZOOM_IN,
                           &options->type_w[0],
                           _("Zoom out"), (gpointer) GIMP_ZOOM_OUT,
                           &options->type_w[1],

                           NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
}

static void
magnify_options_reset (GimpToolOptions *tool_options)
{
  MagnifyOptions *options;

  options = (MagnifyOptions *) tool_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_resize_w),
				options->allow_resize_d);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE);
}
