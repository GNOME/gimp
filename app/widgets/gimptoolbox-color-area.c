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

#include <string.h> /* memcpy */

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#ifdef __GNUC__
#warning FIXME #include "gui/gui-types.h"
#endif
#include "gui/gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "gimpdnd.h"
#include "gimptoolbox.h"
#include "gimptoolbox-color-area.h"

#include "gui/color-notebook.h"

#ifdef DISPLAY_FILTERS
#include "gdisplay_color.h"
#endif /* DISPLAY_FILTERS */

#include "libgimp/gimpintl.h"


typedef enum
{
  FORE_AREA,
  BACK_AREA,
  SWAP_AREA,
  DEFAULT_AREA,
  INVALID_AREA
} ColorAreaTarget;


/*  local function prototypes  */

static ColorAreaTarget   color_area_target (gint                x,
                                            gint                y);
static void     color_area_draw_rect       (GdkDrawable        *drawable,
                                            GdkGC              *gc,
                                            gint                x,
                                            gint                y,
                                            gint                width,
                                            gint                height,
                                            guchar              r,
                                            guchar              g,
                                            guchar              b);
static void     color_area_draw            (GimpContext        *context);
static void     color_area_select_callback (ColorNotebook      *color_notebook,
                                            const GimpRGB      *color,
                                            ColorNotebookState  state,
                                            gpointer            data);
static void     color_area_edit            (GimpContext        *context);
static void     color_area_drop_color      (GtkWidget          *widget,
                                            const GimpRGB      *color,
                                            gpointer            data);
static void     color_area_drag_color      (GtkWidget          *widget,
                                            GimpRGB            *color,
                                            gpointer            data);
static gboolean color_area_events          (GtkWidget          *widget,
                                            GdkEvent           *event,
                                            gpointer            data);


/*  Global variables  */
gint         active_color     = FOREGROUND;
GimpDisplay *color_area_gdisp = NULL;

/*  Static variables  */
static GtkWidget     *color_area            = NULL;
static ColorNotebook *color_notebook        = NULL;
static gboolean       color_notebook_active = FALSE;
static gint           edit_color;
static GimpRGB        revert_fg;
static GimpRGB        revert_bg;

/*  dnd stuff  */
static GtkTargetEntry color_area_target_table[] =
{
  GIMP_TARGET_COLOR
};


/*  public functions  */

GtkWidget *
gimp_toolbox_color_area_create (GimpToolbox *toolbox,
                                gint         width,
                                gint         height)
{
  GimpContext *context;

  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);

  context = GIMP_DOCK (toolbox)->context;

  color_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (color_area, width, height);
  gtk_widget_set_events (color_area,
			 GDK_EXPOSURE_MASK |
			 GDK_BUTTON_PRESS_MASK |
			 GDK_BUTTON_RELEASE_MASK |
                         GDK_ENTER_NOTIFY_MASK |
			 GDK_LEAVE_NOTIFY_MASK);

  g_signal_connect (color_area, "event",
		    G_CALLBACK (color_area_events),
		    context);

  /*  dnd stuff  */
  gtk_drag_source_set (color_area,
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       color_area_target_table,
                       G_N_ELEMENTS (color_area_target_table),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (color_area, color_area_drag_color, context);

  gimp_dnd_color_dest_add (color_area, color_area_drop_color, context);

  g_signal_connect_swapped (context, "foreground_changed",
                            G_CALLBACK (gtk_widget_queue_draw),
                            color_area);
  g_signal_connect_swapped (context, "background_changed",
                            G_CALLBACK (gtk_widget_queue_draw),
                            color_area);

#ifdef DISPLAY_FILTERS
  /* display filter dummy gdisplay */
  color_area_gdisp = g_new (GimpDisplay, 1);
  color_area_gdisp->cd_list = NULL;
  color_area_gdisp->cd_ui   = NULL;
  color_area_gdisp->gimage = g_new (GimpImage, 1);
  color_area_gdisp->gimage->base_type = RGB;
#endif /* DISPLAY_FILTERS */

  return color_area;
}


/*  private functions  */

static ColorAreaTarget
color_area_target (gint x,
		   gint y)
{
  gint rect_w, rect_h;
  gint width, height;

  width  = color_area->allocation.width;
  height = color_area->allocation.height;

  rect_w = (width  * 2) / 3;
  rect_h = (height * 2) / 3;

  /*  foreground active  */
  if (x > 0 && x < rect_w && y > 0 && y < rect_h)
    return FORE_AREA;
  else if (x > (width - rect_w)  && x < width  &&
	   y > (height - rect_h) && y < height)
    return BACK_AREA;
  else if (x > 0      && x < (width - rect_w) &&
	   y > rect_h && y < height)
    return DEFAULT_AREA;
  else if (x > rect_w && x < width &&
	   y > 0      && y < (height - rect_h))
    return SWAP_AREA;
  else
    return -1;
}

static void
color_area_draw_rect (GdkDrawable *drawable,
                      GdkGC       *gc,
		      gint         x,
		      gint         y,
		      gint         width,
		      gint         height,
		      guchar       r,
		      guchar       g,
		      guchar       b)
{
  static guchar *color_area_rgb_buf = NULL;
  static gint    color_area_rgb_buf_size;
  static gint    rowstride;
  gint    xx, yy;
  guchar *bp;
#ifdef DISPLAY_FILTERS
  GList  *list;
#endif /* DISPLAY_FILTERS */

  rowstride = 3 * ((width + 3) & -4);

  if (!color_area_rgb_buf || color_area_rgb_buf_size < height * rowstride)
    {
      color_area_rgb_buf_size = rowstride * height;

      g_free (color_area_rgb_buf);
      color_area_rgb_buf = g_malloc (color_area_rgb_buf_size);
    }

  bp = color_area_rgb_buf;
  for (xx = 0; xx < width; xx++)
    {
      *bp++ = r;
      *bp++ = g;
      *bp++ = b;
    }

  bp = color_area_rgb_buf;

#ifdef DISPLAY_FILTERS
  for (list = color_area_gdisp->cd_list; list; list = g_list_next (list))
    {
      ColorDisplayNode *node = (ColorDisplayNode *) list->data;

      node->cd_convert (node->cd_ID, bp, width, 1, 3, rowstride);
    }
#endif /* DISPLAY_FILTERS */

  for (yy = 1; yy < height; yy++)
    {
      bp += rowstride;
      memcpy (bp, color_area_rgb_buf, rowstride);
    }

  gdk_draw_rgb_image (drawable, gc, x, y, width, height,
		      GDK_RGB_DITHER_MAX,
		      color_area_rgb_buf,
		      rowstride);
}

static void
color_area_draw (GimpContext *context)
{
  gint       rect_w, rect_h;
  gint       width, height;
  gint       w, h;
  GimpRGB    color;
  guchar     r, g, b;
  GdkPixbuf *pixbuf;

  if (!GTK_WIDGET_DRAWABLE (color_area))
    return;

  width  = color_area->allocation.width;
  height = color_area->allocation.height;

  rect_w = (width  * 2) / 3;
  rect_h = (height * 2) / 3;

  /*  draw the background area  */
  gimp_context_get_background (context, &color);
  gimp_rgb_get_uchar (&color, &r, &g, &b);
  color_area_draw_rect (color_area->window, color_area->style->fg_gc[0],
			(width - rect_w), (height - rect_h), rect_w, rect_h,
			r, g, b);

  if (active_color == FOREGROUND)
    gtk_paint_shadow (color_area->style, color_area->window, GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      NULL, color_area, NULL,
                      (width - rect_w), (height - rect_h),
                      rect_w, rect_h);
  else
    gtk_paint_shadow (color_area->style, color_area->window, GTK_STATE_NORMAL,
                      GTK_SHADOW_IN,
                      NULL, color_area, NULL,
                      (width - rect_w), (height - rect_h),
                      rect_w, rect_h);

  /*  draw the foreground area  */
  gimp_context_get_foreground (context, &color);
  gimp_rgb_get_uchar (&color, &r, &g, &b);
  color_area_draw_rect (color_area->window, color_area->style->fg_gc[0],
                        0, 0, rect_w, rect_h,
			r, g, b);

  if (active_color == FOREGROUND)
    gtk_paint_shadow (color_area->style, color_area->window, GTK_STATE_NORMAL,
                      GTK_SHADOW_IN,
                      NULL, color_area, NULL,
                      0, 0,
                      rect_w, rect_h);
  else
    gtk_paint_shadow (color_area->style, color_area->window, GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      NULL, color_area, NULL,
                      0, 0,
                      rect_w, rect_h);

  /*  draw the default colors pixbuf  */
  pixbuf = gtk_widget_render_icon (color_area, GIMP_STOCK_DEFAULT_COLORS,
                                   GTK_ICON_SIZE_MENU, NULL);
  w = gdk_pixbuf_get_width  (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);
  /* FIXME: remove when we no longer support GTK 2.0.x */
#if GTK_CHECK_VERSION(2,2,0)
  gdk_draw_pixbuf (color_area->window, NULL, pixbuf,
                   0, 0, 0, height - h, w, h,
                   GDK_RGB_DITHER_MAX, 0, 0);
#else
  gdk_pixbuf_render_to_drawable_alpha (pixbuf, color_area->window,
                                       0, 0, 0, height - h, w, h,
                                       GDK_PIXBUF_ALPHA_FULL, 127,
                                       GDK_RGB_DITHER_MAX, 0, 0);
#endif
  g_object_unref (pixbuf);

  /*  draw the swap colors pixbuf  */
  pixbuf = gtk_widget_render_icon (color_area, GIMP_STOCK_SWAP_COLORS,
                                   GTK_ICON_SIZE_MENU, NULL);
  w = gdk_pixbuf_get_width  (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);
  /* FIXME: remove when we no longer support GTK 2.0.x */
#if GTK_CHECK_VERSION(2,2,0)
  gdk_draw_pixbuf (color_area->window, NULL, pixbuf,
                   0, 0, width - w, 0, w, h,
                   GDK_RGB_DITHER_MAX, 0, 0);
#else
  gdk_pixbuf_render_to_drawable_alpha (pixbuf, color_area->window,
                                       0, 0, width - w, 0, w, h,
                                       GDK_PIXBUF_ALPHA_FULL, 127,
                                       GDK_RGB_DITHER_MAX, 0, 0);
#endif
  g_object_unref (pixbuf);
}

static void
color_area_select_callback (ColorNotebook      *color_notebook,
			    const GimpRGB      *color,
			    ColorNotebookState  state,
			    gpointer            data)
{
  GimpContext *context;

  context = GIMP_CONTEXT (data);

  if (color_notebook)
    {
      switch (state)
	{
	case COLOR_NOTEBOOK_OK:
	  color_notebook_hide (color_notebook);
	  color_notebook_active = FALSE;
	  /* Fallthrough */
	case COLOR_NOTEBOOK_UPDATE:
	  if (edit_color == FOREGROUND)
	    gimp_context_set_foreground (context, color);
	  else
	    gimp_context_set_background (context, color);
	  break;
	case COLOR_NOTEBOOK_CANCEL:
	  color_notebook_hide (color_notebook);
	  color_notebook_active = FALSE;
	  gimp_context_set_foreground (context, &revert_fg);
	  gimp_context_set_background (context, &revert_bg);
	}
    }
}

static void
color_area_edit (GimpContext *context)
{
  GimpRGB color;

  if (! color_notebook_active)
    {
      gimp_context_get_foreground (context, &revert_fg);
      gimp_context_get_background (context, &revert_bg);
    }

  if (active_color == FOREGROUND)
    gimp_context_get_foreground (context, &color);
  else
    gimp_context_get_background (context, &color);

  edit_color = active_color;

#define FG_TITLE _("Change Foreground Color")
#define BG_TITLE _("Change Background Color")

  if (! color_notebook)
    {
      color_notebook = color_notebook_new (active_color == FOREGROUND ?
                                           FG_TITLE : BG_TITLE,
					   (const GimpRGB *) &color,
					   color_area_select_callback,
					   context, TRUE, FALSE);
      color_notebook_active = TRUE;
    }
  else
    {
      color_notebook_set_title (color_notebook,
                                active_color == FOREGROUND ?
                                FG_TITLE : BG_TITLE);
      color_notebook_set_color (color_notebook, &color);

      if (! color_notebook_active)
	{
	  color_notebook_show (color_notebook);
	  color_notebook_active = TRUE;
	}
      else
	{
	  color_notebook_show (color_notebook);
	}
    }

#undef FG_TITLE
#undef BG_TITLE
}

static gint
color_area_events (GtkWidget *widget,
		   GdkEvent  *event,
                   gpointer   data)
{
  GimpContext     *context;
  GdkEventButton  *bevent;
  ColorAreaTarget  target;

  static ColorAreaTarget press_target = INVALID_AREA;

  context = GIMP_CONTEXT (data);

  switch (event->type)
    {
    case GDK_EXPOSE:
      color_area_draw (context);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  target = color_area_target (bevent->x, bevent->y);
	  press_target = INVALID_AREA;

	  switch (target)
	    {
	    case FORE_AREA:
	    case BACK_AREA:
	      if (target != active_color)
		{
		  active_color = target;
		  color_area_draw (context);
		}
	      else
		{
		  press_target = target;
		}
	      break;
	    case SWAP_AREA:
	      gimp_context_swap_colors (context);
	      break;
	    case DEFAULT_AREA:
	      gimp_context_set_default_colors (context);
	      break;
	    default:
	      break;
	    }
	}
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  target = color_area_target (bevent->x, bevent->y);

	  if (target == press_target)
	    {
	      switch (target)
		{
		case FORE_AREA:
		case BACK_AREA:
		  color_area_edit (context);
		  break;
		default:
		  break;
		}
	    }
	  press_target = INVALID_AREA;
	}
      break;

    case GDK_LEAVE_NOTIFY:
      press_target = INVALID_AREA;
      break;

    default:
      break;
    }

  return FALSE;
}

static void
color_area_drag_color (GtkWidget *widget,
		       GimpRGB   *color,
		       gpointer   data)
{
  GimpContext *context;

  context = GIMP_CONTEXT (data);

  if (active_color == FOREGROUND)
    gimp_context_get_foreground (context, color);
  else
    gimp_context_get_background (context, color);
}

static void
color_area_drop_color (GtkWidget     *widget,
		       const GimpRGB *color,
		       gpointer       data)
{
  GimpContext *context;

  context = GIMP_CONTEXT (data);

  if (color_notebook_active &&
      active_color == edit_color)
    {
      color_notebook_set_color (color_notebook, color);
    }
  else
    {
      if (active_color == FOREGROUND)
	gimp_context_set_foreground (context, color);
      else
	gimp_context_set_background (context, color);
    }
}
