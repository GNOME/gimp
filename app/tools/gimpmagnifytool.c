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
#include "appenv.h"
#include "draw_core.h"
#include "gdisplay.h"
#include "gimprc.h"
#include "info_window.h"
#include "magnify.h"
#include "scale.h"

/*   types of magnify operations  */
#define ZOOMIN            0
#define ZOOMOUT           1

/*  local structures  */

typedef struct _MagnifyOptions MagnifyOptions;

struct _MagnifyOptions
{
  int dummy;
};

typedef struct _magnify Magnify;

struct _magnify
{
  DrawCore *      core;       /*  Core select object          */

  int             x, y;       /*  upper left hand coordinate  */
  int             w, h;       /*  width and height            */

  int             op;         /*  magnify operation           */
};


/*   magnify utility functions  */
static void   zoom_in                   (int *, int *, int);
static void   zoom_out                  (int *, int *, int);

/*  magnify action functions  */
static void   magnify_button_press      (Tool *, GdkEventButton *, gpointer);
static void   magnify_button_release    (Tool *, GdkEventButton *, gpointer);
static void   magnify_motion            (Tool *, GdkEventMotion *, gpointer);
static void   magnify_cursor_update     (Tool *, GdkEventMotion *, gpointer);
static void   magnify_control           (Tool *, int, gpointer);

static void  *magnify_options = NULL;

/*   magnify utility functions  */

static void
zoom_in (int *src,
	 int *dest,
	 int  scale)
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
zoom_out (int *src,
	  int *dest,
	  int  scale)
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
magnify_toggle_update (GtkWidget *w,
		       gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static MagnifyOptions *
create_magnify_options (void)
{
  MagnifyOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *allow_resize_toggle;

  /*  the new options structure  */
  options = (MagnifyOptions *) g_malloc (sizeof (MagnifyOptions));

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new ("Magnify Options");

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the allow_resize toggle button  */
  allow_resize_toggle = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), allow_resize_toggle, FALSE, FALSE, 0);
  label = gtk_label_new ("Allow Window Resizing");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (allow_resize_toggle), label);
  gtk_widget_show (label);
  gtk_signal_connect (GTK_OBJECT (allow_resize_toggle), "toggled",
		      (GtkSignalFunc) magnify_toggle_update,
		      &allow_resize_windows);
  gtk_widget_show (allow_resize_toggle);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (allow_resize_toggle), allow_resize_windows);

  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (MAGNIFY, vbox);

  return options;
}


static void
magnify_button_press (Tool           *tool,
		      GdkEventButton *bevent,
		      gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  Magnify * magnify;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  magnify = (Magnify *) tool->private;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, TRUE, 0);

  magnify->x = x;
  magnify->y = y;
  magnify->w = 0;
  magnify->h = 0;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  if (bevent->state & GDK_SHIFT_MASK)
    magnify->op = ZOOMOUT;
  else
    magnify->op = ZOOMIN;  /*  default  */

  draw_core_start (magnify->core,
		   gdisp->canvas->window,
		   tool);
}


static void
magnify_button_release (Tool           *tool,
			GdkEventButton *bevent,
			gpointer        gdisp_ptr)
{
  Magnify * magnify;
  GDisplay * gdisp;
  int win_width, win_height;
  int width, height;
  int scalesrc, scaledest;
  int scale;
  int x1, y1, x2, y2, w, h;

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
      scalesrc = SCALESRC(gdisp);
      scaledest = SCALEDEST(gdisp);

      win_width = gdisp->disp_width;
      win_height = gdisp->disp_height;
      width = (win_width * scalesrc) / scaledest;
      height = (win_height * scalesrc) / scaledest;

      if (!w || !h)
	scale = 1;
      else
	scale = MINIMUM ((width / w), (height / h));

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
  Magnify * magnify;
  GDisplay * gdisp;
  int x, y;

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
magnify_cursor_update (Tool           *tool,
		       GdkEventMotion *mevent,
		       gpointer        gdisp_ptr)
{
  GDisplay * gdisp;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
}


void
magnify_draw (Tool *tool)
{
  GDisplay * gdisp;
  Magnify * magnify;
  int x1, y1, x2, y2;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  magnify = (Magnify *) tool->private;

  x1 = MINIMUM (magnify->x, magnify->x + magnify->w);
  y1 = MINIMUM (magnify->y, magnify->y + magnify->h);
  x2 = MAXIMUM (magnify->x, magnify->x + magnify->w);
  y2 = MAXIMUM (magnify->y, magnify->y + magnify->h);

  gdisplay_transform_coords (gdisp, x1, y1, &x1, &y1, 0);
  gdisplay_transform_coords (gdisp, x2, y2, &x2, &y2, 0);

  gdk_draw_rectangle (magnify->core->win, magnify->core->gc, 0,
		      x1, y1, (x2 - x1), (y2 - y1));
}


static void
magnify_control (Tool     *tool,
		 int       action,
		 gpointer  gdisp_ptr)
{
  Magnify * magnify;

  magnify = (Magnify *) tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (magnify->core, tool);
      break;
    case RESUME :
      draw_core_resume (magnify->core, tool);
      break;
    case HALT :
      draw_core_stop (magnify->core, tool);
      break;
    }
}


Tool *
tools_new_magnify (void)
{
  Tool * tool;
  Magnify * private;

  if (! magnify_options)
    magnify_options = create_magnify_options ();

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (Magnify *) g_malloc (sizeof (Magnify));

  private->core = draw_core_new (magnify_draw);
  private->x = private->y = 0;
  private->w = private->h = 0;

  tool->type = MAGNIFY;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  disallow scrolling  */
  tool->auto_snap_to = FALSE;
  tool->private = (void *) private;
  tool->button_press_func = magnify_button_press;
  tool->button_release_func = magnify_button_release;
  tool->motion_func = magnify_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = magnify_cursor_update;
  tool->control_func = magnify_control;

  return tool;
}


void
tools_free_magnify (Tool *tool)
{
  Magnify * magnify;

  magnify = (Magnify *) tool->private;

  draw_core_free (magnify->core);
  g_free (magnify);
}
