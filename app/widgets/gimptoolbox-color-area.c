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

#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimptoolbox.h"
#include "gimptoolbox-color-area.h"

#include "gui/color-notebook.h"

#include "gimp-intl.h"


typedef enum
{
  FORE_AREA,
  BACK_AREA,
  SWAP_AREA,
  DEFAULT_AREA,
  INVALID_AREA
} ColorAreaTarget;

#define FOREGROUND 0
#define BACKGROUND 1


/*  local function prototypes  */

static ColorAreaTarget   color_area_target (gint                x,
                                            gint                y);

static gboolean color_area_expose_event    (GtkWidget          *widget,
                                            GdkEventExpose     *eevent,
                                            GimpContext        *context);
static void     color_area_draw_rect       (GdkDrawable        *drawable,
                                            GdkGC              *gc,
                                            gint                x,
                                            gint                y,
                                            gint                width,
                                            gint                height,
                                            const GimpRGB      *color);

static void     color_area_select_callback (ColorNotebook      *color_notebook,
                                            const GimpRGB      *color,
                                            ColorNotebookState  state,
                                            gpointer            data);
static void     color_area_edit            (GimpContext        *context,
                                            GtkWidget          *widget);
static void     color_area_drop_color      (GtkWidget          *widget,
                                            const GimpRGB      *color,
                                            gpointer            data);
static void     color_area_drag_color      (GtkWidget          *widget,
                                            GimpRGB            *color,
                                            gpointer            data);
static gboolean color_area_events          (GtkWidget          *widget,
                                            GdkEvent           *event,
                                            gpointer            data);


/*  Static variables  */
static GtkWidget     *color_area            = NULL;
static ColorNotebook *color_notebook        = NULL;
static gboolean       color_notebook_active = FALSE;
static gint           active_color          = FOREGROUND;
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
  g_signal_connect (color_area, "expose_event",
		    G_CALLBACK (color_area_expose_event),
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

static gboolean
color_area_expose_event (GtkWidget      *widget,
                         GdkEventExpose *eevent,
                         GimpContext    *context)
{
  gint       rect_w, rect_h;
  gint       width, height;
  gint       w, h;
  GimpRGB    color;
  GdkPixbuf *pixbuf;

  if (!GTK_WIDGET_DRAWABLE (color_area))
    return FALSE;

  width  = color_area->allocation.width;
  height = color_area->allocation.height;

  rect_w = (width  * 2) / 3;
  rect_h = (height * 2) / 3;

  /*  draw the background area  */
  gimp_context_get_background (context, &color);
  color_area_draw_rect (color_area->window, color_area->style->fg_gc[0],
			(width - rect_w), (height - rect_h), rect_w, rect_h,
			&color);

  gtk_paint_shadow (color_area->style, color_area->window, GTK_STATE_NORMAL,
                    active_color == FOREGROUND ? GTK_SHADOW_OUT : GTK_SHADOW_IN,
                    NULL, color_area, NULL,
                    (width - rect_w), (height - rect_h),
                    rect_w, rect_h);

  /*  draw the foreground area  */
  gimp_context_get_foreground (context, &color);
  color_area_draw_rect (color_area->window, color_area->style->fg_gc[0],
                        0, 0, rect_w, rect_h,
			&color);

  gtk_paint_shadow (color_area->style, color_area->window, GTK_STATE_NORMAL,
                    active_color == BACKGROUND ? GTK_SHADOW_OUT : GTK_SHADOW_IN,
                    NULL, color_area, NULL,
                    0, 0,
                    rect_w, rect_h);

  /*  draw the default colors pixbuf  */
  pixbuf = gtk_widget_render_icon (color_area, GIMP_STOCK_DEFAULT_COLORS,
                                   GTK_ICON_SIZE_MENU, NULL);
  w = gdk_pixbuf_get_width  (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);
  gdk_draw_pixbuf (color_area->window, NULL, pixbuf,
                   0, 0, 0, height - h, w, h,
                   GDK_RGB_DITHER_MAX, 0, 0);
  g_object_unref (pixbuf);

  /*  draw the swap colors pixbuf  */
  pixbuf = gtk_widget_render_icon (color_area, GIMP_STOCK_SWAP_COLORS,
                                   GTK_ICON_SIZE_MENU, NULL);
  w = gdk_pixbuf_get_width  (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);
  gdk_draw_pixbuf (color_area->window, NULL, pixbuf,
                   0, 0, width - w, 0, w, h,
                   GDK_RGB_DITHER_MAX, 0, 0);
  g_object_unref (pixbuf);

  return TRUE;
}

static void
color_area_draw_rect (GdkDrawable   *drawable,
                      GdkGC         *gc,
		      gint           x,
		      gint           y,
		      gint           width,
		      gint           height,
                      const GimpRGB *color)
{
  static guchar *color_area_rgb_buf = NULL;
  static gint    color_area_rgb_buf_size;

  gint    rowstride;
  guchar  r, g, b;
  gint    xx, yy;
  guchar *bp;

  gimp_rgb_get_uchar (color, &r, &g, &b);

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
color_area_select_callback (ColorNotebook      *color_notebook,
			    const GimpRGB      *color,
			    ColorNotebookState  state,
			    gpointer            data)
{
  GimpContext *context = GIMP_CONTEXT (data);

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
          break;
	}
    }
}

static void
color_area_edit (GimpContext *context,
                 GtkWidget   *widget)
{
  GimpRGB      color;
  const gchar *title;

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

  if (active_color == FOREGROUND)
    title = _("Change Foreground Color");
  else
    title = _("Change Background Color");

  if (! color_notebook)
    {
      GimpDialogFactory *toplevel_factory;

      toplevel_factory = gimp_dialog_factory_from_name ("toplevel");

      color_notebook = color_notebook_new (NULL, title, NULL, NULL,
                                           widget,
                                           toplevel_factory,
                                           "gimp-toolbox-color-dialog",
					   (const GimpRGB *) &color,
					   color_area_select_callback,
					   context, TRUE, FALSE);
      color_notebook_active = TRUE;
    }
  else
    {
      color_notebook_set_title (color_notebook, title);
      color_notebook_set_color (color_notebook, &color);

      color_notebook_show (color_notebook);
      color_notebook_active = TRUE;
    }
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
		  gtk_widget_queue_draw (widget);
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
		  color_area_edit (context, widget);
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
  GimpContext *context = GIMP_CONTEXT (data);

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
  GimpContext *context = GIMP_CONTEXT (data);

  if (color_notebook_active && active_color == edit_color)
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
