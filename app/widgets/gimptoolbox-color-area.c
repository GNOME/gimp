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
#include "color_area.h"
#include "color_select.h"
#include "colormaps.h"
#include "palette.h"
#include "pixelrow.h"

#define FORE_AREA 0
#define BACK_AREA 1
#define SWAP_AREA 2
#define DEF_AREA  3

/*  Global variables  */
int active_color = 0;

/*  Static variables  */
static GdkGC *color_area_gc = NULL;
static GtkWidget *color_area = NULL;
static GdkPixmap *color_area_pixmap = NULL;
static GdkPixmap *default_pixmap = NULL;
static GdkPixmap *swap_pixmap = NULL;
static ColorSelectP color_select = NULL;
static int color_select_active = 0;
static int edit_color;

static PixelRow revert_fg;
static guchar revert_fg_data[TAG_MAX_BYTES];

static PixelRow revert_bg;
static guchar revert_bg_data[TAG_MAX_BYTES];


/*  Local functions  */
static int
color_area_target (int x,
		   int y)
{
  int rect_w, rect_h;
  int width, height;

  gdk_window_get_size (color_area_pixmap, &width, &height);

  rect_w = width * 0.65;
  rect_h = height * 0.65;

  /*  foreground active  */
  if (x > 0 && x < rect_w &&
      y > 0 && y < rect_h)
    return FORE_AREA;
  else if (x > (width - rect_w) && x < width &&
	   y > (height - rect_h) && y < height)
    return BACK_AREA;
  else if (x > 0 && x < (width - rect_w) &&
	   y > rect_h && y < height)
    return DEF_AREA;
  else if (x > rect_w && x < width &&
	   y > 0 && y < (height - rect_h))
    return SWAP_AREA;
  else
    return -1;
}

static void
color_area_draw (void)
{
  GdkColor *win_bg;
  GdkColor fg, bg, bd;
  int rect_w, rect_h;
  int width, height;
  int def_width, def_height;
  int swap_width, swap_height;

  if (!color_area_pixmap)     /* we haven't gotten initial expose yet,
                               * no point in drawing anything */
    return;

  gdk_window_get_size (color_area_pixmap, &width, &height);

  win_bg = &(color_area->style->bg[GTK_STATE_NORMAL]);
  fg.pixel = foreground_pixel;
  bg.pixel = background_pixel;
  bd.pixel = g_black_pixel;

  rect_w = width * 0.65;
  rect_h = height * 0.65;

  gdk_gc_set_foreground (color_area_gc, win_bg);
  gdk_draw_rectangle (color_area_pixmap, color_area_gc, 1,
		      0, 0, width, height);

  gdk_gc_set_foreground (color_area_gc, &bg);
  gdk_draw_rectangle (color_area_pixmap, color_area_gc, 1,
		      (width - rect_w), (height - rect_h), rect_w, rect_h);

  if (active_color == FOREGROUND)
    gtk_draw_shadow (color_area->style, color_area_pixmap, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		     (width - rect_w), (height - rect_h), rect_w, rect_h);
  else
    gtk_draw_shadow (color_area->style, color_area_pixmap, GTK_STATE_NORMAL, GTK_SHADOW_IN,
		     (width - rect_w), (height - rect_h), rect_w, rect_h);

  gdk_gc_set_foreground (color_area_gc, &fg);
  gdk_draw_rectangle (color_area_pixmap, color_area_gc, 1,
		      0, 0, rect_w, rect_h);

  if (active_color == FOREGROUND)
    gtk_draw_shadow (color_area->style, color_area_pixmap, GTK_STATE_NORMAL, GTK_SHADOW_IN,
		     0, 0, rect_w, rect_h);
  else
    gtk_draw_shadow (color_area->style, color_area_pixmap, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		     0, 0, rect_w, rect_h);


  gdk_window_get_size (default_pixmap, &def_width, &def_height);
  gdk_draw_pixmap (color_area_pixmap, color_area_gc, default_pixmap,
		   0, 0, 0, height - def_height, def_width, def_height);

  gdk_window_get_size (swap_pixmap, &swap_width, &swap_height);
  gdk_draw_pixmap (color_area_pixmap, color_area_gc, swap_pixmap,
		   0, 0, width - swap_width, 0, swap_width, swap_height);

  gdk_draw_pixmap (color_area->window, color_area_gc, color_area_pixmap,
		   0, 0, 0, 0, width, height);
}

static void
color_area_select_callback (PixelRow * color,
			    ColorSelectState state,
			    void *client_data)
{
  if (color_select)
    {
      switch (state) {
      case COLOR_SELECT_OK:
	color_select_hide (color_select);
	color_select_active = 0;
	/* Fallthrough */
      case COLOR_SELECT_UPDATE:
	if (edit_color == FOREGROUND)
	  palette_set_foreground (color);
	else
	  palette_set_background (color);
	break;
      case COLOR_SELECT_CANCEL:
	color_select_hide (color_select);
	color_select_active = 0;
	palette_set_foreground (&revert_fg);
	palette_set_background (&revert_bg);
      }
    }
}

static void
color_area_edit (void)
{
  PixelRow r;
  guchar d[TAG_MAX_BYTES];

  pixelrow_init (&r,
                 tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                 d,
                 1);

  if (!color_select_active)
    {
      pixelrow_init (&revert_fg,
                     tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                     revert_fg_data,
                     1);
      palette_get_foreground (&revert_fg);

      pixelrow_init (&revert_bg,
                     tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                     revert_bg_data,
                     1);
      palette_get_background (&revert_bg);
    }
  if (active_color == FOREGROUND)
    {
      palette_get_foreground (&r);
      edit_color = FOREGROUND;
    }
  else
    {
      palette_get_background (&r);
      edit_color = BACKGROUND;
    }

  if (! color_select)
    {
      color_select = color_select_new (&r, color_area_select_callback, NULL, TRUE);
      color_select_active = 1;
    }
  else
    {
      if (! color_select_active)
	color_select_show (color_select);
      color_select_set_color (color_select, &r, 1);
    }
}

static gint
color_area_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventButton *bevent;
  int target;

  switch (event->type)
    {
    case GDK_CONFIGURE:
      if (color_area_pixmap)
	gdk_pixmap_unref (color_area_pixmap);

      color_area_pixmap = gdk_pixmap_new (widget->window,
					  widget->allocation.width,
					  widget->allocation.height, -1);

      break;

    case GDK_EXPOSE:
      if (GTK_WIDGET_DRAWABLE (widget))
	{
	  if (!color_area_gc)
	    color_area_gc = gdk_gc_new (widget->window);

	  color_area_draw ();
	}
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  switch ((target = color_area_target (bevent->x, bevent->y)))
	    {
	    case FORE_AREA:
	    case BACK_AREA:
	      if (target == active_color)
		color_area_edit ();
	      else
		{
		  active_color = target;
		  color_area_draw ();
		}
	      break;
	    case SWAP_AREA:
	      palette_swap_colors();
	      color_area_draw ();
	      break;
	    case DEF_AREA:
	      palette_set_default_colors();
	      color_area_draw ();
	      break;
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}

GtkWidget *
color_area_create (int        width,
		   int        height,
		   GdkPixmap *default_pmap,
		   GdkPixmap *swap_pmap)
{
  color_area = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (color_area), width, height);
  gtk_widget_set_events (color_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect (GTK_OBJECT (color_area), "event",
		      (GtkSignalFunc) color_area_events,
		      NULL);
  default_pixmap = default_pmap;
  swap_pixmap    = swap_pmap;

  return color_area;
}

void
color_area_update ()
{
  color_area_draw ();
}
