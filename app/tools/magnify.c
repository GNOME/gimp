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
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>

#include "appenv.h"
#include "cursorutil.h"
#include "draw_core.h"
#include "gdisplay.h"
#include "gimprc.h"
#include "gimpui.h"
#include "info_window.h"
#include "magnify.h"
#include "scale.h"

#include "config.h"
#include "libgimp/gimpintl.h"

/*  the magnify structures  */

typedef struct _Magnify Magnify;

struct _Magnify
{
  DrawCore *core;       /*  Core select object          */

  gint      x, y;       /*  upper left hand coordinate  */
  gint      w, h;       /*  width and height            */

  gint      op;         /*  magnify operation           */
};

typedef struct _MagnifyOptions MagnifyOptions;

struct _MagnifyOptions
{
  ToolOptions  tool_options;

  /* gint      allow_resize_windows; (from gimprc) */
  gint         allow_resize_d;
  GtkWidget   *allow_resize_w;

  ZoomType     type;
  ZoomType     type_d;
  GtkWidget   *type_w[2];
};


/*  the magnify tool options  */
static MagnifyOptions *magnify_options = NULL;


/*  magnify action functions  */
static void   magnify_button_press      (Tool *, GdkEventButton *, gpointer);
static void   magnify_button_release    (Tool *, GdkEventButton *, gpointer);
static void   magnify_motion            (Tool *, GdkEventMotion *, gpointer);
static void   magnify_modifier_update   (Tool *, GdkEventKey *,    gpointer);
static void   magnify_cursor_update     (Tool *, GdkEventMotion *, gpointer);
static void   magnify_control           (Tool *, ToolAction,       gpointer);

/*  magnify utility functions  */
static void   zoom_in                   (gint *src,
					 gint *dest,
					 gint  scale);
static void   zoom_out                  (gint *src,
					 gint *dest,
					 gint  scale);


/*  magnify tool options functions  */

static void
magnify_options_reset (void)
{
  MagnifyOptions *options = magnify_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_resize_w),
				options->allow_resize_d);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE);
}

static MagnifyOptions *
magnify_options_new (void)
{
  MagnifyOptions *options;

  GtkWidget *vbox;
  GtkWidget *frame;

  /*  the new magnify tool options structure  */
  options = g_new (MagnifyOptions, 1);
  tool_options_init ((ToolOptions *) options,
		     _("Magnify Tool"),
		     magnify_options_reset);
  options->allow_resize_d = allow_resize_windows;
  options->type_d         = options->type = ZOOMIN;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the allow_resize toggle button  */
  options->allow_resize_w =
    gtk_check_button_new_with_label (_("Allow Window Resizing"));
  gtk_signal_connect (GTK_OBJECT (options->allow_resize_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &allow_resize_windows);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->allow_resize_w),
				allow_resize_windows);
  gtk_box_pack_start (GTK_BOX (vbox), options->allow_resize_w, FALSE, FALSE, 0);
  gtk_widget_show (options->allow_resize_w);

  /*  tool toggle  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Tool Toggle"),
                           gimp_radio_button_update,
                           &options->type, (gpointer) options->type,

                           _("Zoom in"), (gpointer) ZOOMIN,
                           &options->type_w[0],
                           _("Zoom out"), (gpointer) ZOOMOUT,
                           &options->type_w[1],

                           NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
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


/*  magnify action functions  */

static void
magnify_button_press (Tool           *tool,
		      GdkEventButton *bevent,
		      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Magnify *magnify;
  gint x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  magnify = (Magnify *) tool->private;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, TRUE, 0);

  magnify->x = x;
  magnify->y = y;
  magnify->w = 0;
  magnify->h = 0;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  draw_core_start (magnify->core,
		   gdisp->canvas->window,
		   tool);
}


static void
magnify_button_release (Tool           *tool,
			GdkEventButton *bevent,
			gpointer        gdisp_ptr)
{
  Magnify *magnify;
  GDisplay *gdisp;
  gint win_width, win_height;
  gint width, height;
  gint scalesrc, scaledest;
  gint scale;
  gint x1, y1, x2, y2, w, h;

  gdisp = (GDisplay *) gdisp_ptr;
  magnify = (Magnify *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  draw_core_stop (magnify->core, tool);
  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      x1 = (magnify->w < 0) ? magnify->x + magnify->w : magnify->x;
      y1 = (magnify->h < 0) ? magnify->y + magnify->h : magnify->y;
      w = (magnify->w < 0) ? -magnify->w : magnify->w;
      h = (magnify->h < 0) ? -magnify->h : magnify->h;
      x2 = x1 + w;
      y2 = y1 + h;

      /* these change the user zoom level, so should not be changed to
       * the resolution-aware scale macros -- austin */
      scalesrc = SCALESRC (gdisp);
      scaledest = SCALEDEST (gdisp);

      win_width = gdisp->disp_width;
      win_height = gdisp->disp_height;
      width = (win_width * scalesrc) / scaledest;
      height = (win_height * scalesrc) / scaledest;

      if (!w || !h)
	scale = 1;
      else
	scale = MIN ((width / w), (height / h));

      magnify->op = magnify_options->type;

      switch (magnify->op)
	{
	case ZOOMIN:
	  zoom_in (&scalesrc, &scaledest, scale);
	  break;
	case ZOOMOUT:
	  zoom_out (&scalesrc, &scaledest, scale);
	  break;
	}

      gdisp->scale = (scaledest << 8) + scalesrc;
      gdisp->offset_x = (scaledest * ((x1 + x2) / 2)) / scalesrc -
	(win_width / 2);
      gdisp->offset_y = (scaledest * ((y1 + y2) / 2)) / scalesrc -
	(win_height / 2);

      /*  resize the image  */
      resize_display (gdisp, allow_resize_windows, TRUE);
    }
}

static void
magnify_motion (Tool           *tool,
		GdkEventMotion *mevent,
		gpointer        gdisp_ptr)
{
  Magnify *magnify;
  GDisplay *gdisp;
  gint x, y;

  if (tool->state != ACTIVE)
    return;

  gdisp = (GDisplay *) gdisp_ptr;
  magnify = (Magnify *) tool->private;

  draw_core_pause (magnify->core, tool);

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, 0);
  magnify->w = (x - magnify->x);
  magnify->h = (y - magnify->y);

  draw_core_resume (magnify->core, tool);
}


static void
magnify_modifier_update (Tool        *tool,
			 GdkEventKey *kevent,
			 gpointer     gdisp_ptr)
{
  switch (kevent->keyval)
    {
    case GDK_Alt_L: case GDK_Alt_R:
      break;
    case GDK_Shift_L: case GDK_Shift_R:
      break;
    case GDK_Control_L: case GDK_Control_R:
      if (magnify_options->type == ZOOMIN)
        gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (magnify_options->type_w[ZOOMOUT]), TRUE);
      else
        gtk_toggle_button_set_active
	  (GTK_TOGGLE_BUTTON (magnify_options->type_w[ZOOMIN]), TRUE);
      break;
    }
}
  
static void
magnify_cursor_update (Tool           *tool,
		       GdkEventMotion *mevent,
		       gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;

  if (magnify_options->type == ZOOMIN)
    {
      gdisplay_install_tool_cursor (gdisp, GIMP_ZOOM_CURSOR,
				    MAGNIFY,
				    CURSOR_MODIFIER_PLUS,
				    FALSE);
    }
  else
    {
      gdisplay_install_tool_cursor (gdisp, GIMP_ZOOM_CURSOR,
				    MAGNIFY,
				    CURSOR_MODIFIER_MINUS,
				    FALSE);
   }
}


void
magnify_draw (Tool *tool)
{
  GDisplay *gdisp;
  Magnify *magnify;
  gint x1, y1, x2, y2;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  magnify = (Magnify *) tool->private;

  x1 = MIN (magnify->x, magnify->x + magnify->w);
  y1 = MIN (magnify->y, magnify->y + magnify->h);
  x2 = MAX (magnify->x, magnify->x + magnify->w);
  y2 = MAX (magnify->y, magnify->y + magnify->h);

  gdisplay_transform_coords (gdisp, x1, y1, &x1, &y1, 0);
  gdisplay_transform_coords (gdisp, x2, y2, &x2, &y2, 0);

  gdk_draw_rectangle (magnify->core->win, magnify->core->gc, 0,
		      x1, y1, (x2 - x1), (y2 - y1));
}


static void
magnify_control (Tool       *tool,
		 ToolAction  action,
		 gpointer    gdisp_ptr)
{
  Magnify *magnify;

  magnify = (Magnify *) tool->private;

  switch (action)
    {
    case PAUSE:
      draw_core_pause (magnify->core, tool);
      break;

    case RESUME:
      draw_core_resume (magnify->core, tool);
      break;

    case HALT:
      draw_core_stop (magnify->core, tool);
      break;

    default:
      break;
    }
}


Tool *
tools_new_magnify (void)
{
  Tool *tool;
  Magnify *private;

  /*  The tool options  */
  if (! magnify_options)
    {
      magnify_options = magnify_options_new ();
      tools_register (MAGNIFY, (ToolOptions *) magnify_options);
    }

  tool = tools_new_tool (MAGNIFY);
  private = g_new0 (Magnify, 1);

  private->core = draw_core_new (magnify_draw);
  private->x = private->y = 0;
  private->w = private->h = 0;

  tool->scroll_lock  = TRUE;   /*  Disallow scrolling  */
  tool->auto_snap_to = FALSE;  /*  Don't snap to guides  */

  tool->private = (void *) private;

  tool->button_press_func   = magnify_button_press;
  tool->button_release_func = magnify_button_release;
  tool->motion_func         = magnify_motion;
  tool->modifier_key_func   = magnify_modifier_update;
  tool->cursor_update_func  = magnify_cursor_update;
  tool->control_func        = magnify_control;

  return tool;
}


void
tools_free_magnify (Tool *tool)
{
  Magnify *magnify;

  magnify = (Magnify *) tool->private;

  draw_core_free (magnify->core);
  g_free (magnify);
}
