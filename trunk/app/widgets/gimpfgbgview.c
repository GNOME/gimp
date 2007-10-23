/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfgbgview.c
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

#include "gimpdnd.h"
#include "gimpfgbgview.h"


enum
{
  PROP_0,
  PROP_CONTEXT
};


static void     gimp_fg_bg_view_set_property (GObject        *object,
                                              guint           property_id,
                                              const GValue   *value,
                                              GParamSpec     *pspec);
static void     gimp_fg_bg_view_get_property (GObject        *object,
                                              guint           property_id,
                                              GValue         *value,
                                              GParamSpec     *pspec);

static void     gimp_fg_bg_view_destroy      (GtkObject      *object);
static gboolean gimp_fg_bg_view_expose       (GtkWidget      *widget,
                                              GdkEventExpose *eevent);


G_DEFINE_TYPE (GimpFgBgView, gimp_fg_bg_view, GTK_TYPE_WIDGET)

#define parent_class gimp_fg_bg_view_parent_class


static void
gimp_fg_bg_view_class_init (GimpFgBgViewClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class     = GTK_WIDGET_CLASS (klass);

  object_class->set_property = gimp_fg_bg_view_set_property;
  object_class->get_property = gimp_fg_bg_view_get_property;

  gtk_object_class->destroy  = gimp_fg_bg_view_destroy;

  widget_class->expose_event = gimp_fg_bg_view_expose;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_fg_bg_view_init (GimpFgBgView *view)
{
  GTK_WIDGET_SET_FLAGS (view, GTK_NO_WINDOW);

  view->context = NULL;
}

static void
gimp_fg_bg_view_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpFgBgView *view = GIMP_FG_BG_VIEW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      gimp_fg_bg_view_set_context (view, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_fg_bg_view_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpFgBgView *view = GIMP_FG_BG_VIEW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, view->context);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_fg_bg_view_destroy (GtkObject *object)
{
  GimpFgBgView *view = GIMP_FG_BG_VIEW (object);

  if (view->context)
    gimp_fg_bg_view_set_context (view, NULL);

  if (view->render_buf)
    {
      g_free (view->render_buf);
      view->render_buf = NULL;
      view->render_buf_size = 0;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_fg_bg_view_draw_rect (GimpFgBgView  *view,
                           GdkDrawable   *drawable,
                           GdkGC         *gc,
                           gint           x,
                           gint           y,
                           gint           width,
                           gint           height,
                           const GimpRGB *color)
{
  gint    rowstride;
  guchar  r, g, b;
  gint    xx, yy;
  guchar *bp;

  if (! (width > 0 && height > 0))
    return;

  gimp_rgb_get_uchar (color, &r, &g, &b);

  rowstride = 3 * ((width + 3) & -4);

  if (! view->render_buf || view->render_buf_size < height * rowstride)
    {
      view->render_buf_size = rowstride * height;

      g_free (view->render_buf);
      view->render_buf = g_malloc (view->render_buf_size);
    }

  bp = view->render_buf;
  for (xx = 0; xx < width; xx++)
    {
      *bp++ = r;
      *bp++ = g;
      *bp++ = b;
    }

  bp = view->render_buf;

  for (yy = 1; yy < height; yy++)
    {
      bp += rowstride;
      memcpy (bp, view->render_buf, rowstride);
    }

  gdk_draw_rgb_image (drawable, gc, x, y, width, height,
                      GDK_RGB_DITHER_MAX,
                      view->render_buf,
                      rowstride);
}

static gboolean
gimp_fg_bg_view_expose (GtkWidget      *widget,
                        GdkEventExpose *eevent)
{
  GimpFgBgView *view = GIMP_FG_BG_VIEW (widget);
  gint          x, y;
  gint          width, height;
  gint          rect_w, rect_h;
  GimpRGB       color;

  if (! GTK_WIDGET_DRAWABLE (widget))
    return FALSE;

  x      = widget->allocation.x;
  y      = widget->allocation.y;
  width  = widget->allocation.width;
  height = widget->allocation.height;

  rect_w = width  * 3 / 4;
  rect_h = height * 3 / 4;

  /*  draw the background area  */

  if (view->context)
    {
      gimp_context_get_background (view->context, &color);
      gimp_fg_bg_view_draw_rect (view,
                                 widget->window,
                                 widget->style->fg_gc[0],
                                 x + width  - rect_w + 1,
                                 y + height - rect_h + 1,
                                 rect_w - 2, rect_h - 2,
                                 &color);
    }

  gtk_paint_shadow (widget->style, widget->window, GTK_STATE_NORMAL,
                    GTK_SHADOW_IN,
                    NULL, widget, NULL,
                    x + width - rect_w, y + height - rect_h, rect_w, rect_h);

  /*  draw the foreground area  */

  if (view->context)
    {
      gimp_context_get_foreground (view->context, &color);
      gimp_fg_bg_view_draw_rect (view,
                                 widget->window,
                                 widget->style->fg_gc[0],
                                 x + 1, y + 1,
                                 rect_w - 2, rect_h - 2,
                                 &color);
    }

  gtk_paint_shadow (widget->style, widget->window, GTK_STATE_NORMAL,
                    GTK_SHADOW_OUT,
                    NULL, widget, NULL,
                    x, y, rect_w, rect_h);

  return TRUE;
}

/*  public functions  */

GtkWidget *
gimp_fg_bg_view_new (GimpContext *context)
{
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);

  return g_object_new (GIMP_TYPE_FG_BG_VIEW,
                       "context", context,
                       NULL);
}

void
gimp_fg_bg_view_set_context (GimpFgBgView *view,
                             GimpContext  *context)
{
  g_return_if_fail (GIMP_IS_FG_BG_VIEW (view));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (context == view->context)
    return;

  if (view->context)
    {
      g_signal_handlers_disconnect_by_func (view->context,
                                            gtk_widget_queue_draw,
                                            view);
      g_object_unref (view->context);
      view->context = NULL;
    }

  view->context = context;

  if (context)
    {
      g_object_ref (context);

      g_signal_connect_swapped (context, "foreground-changed",
                                G_CALLBACK (gtk_widget_queue_draw),
                                view);
      g_signal_connect_swapped (context, "background-changed",
                                G_CALLBACK (gtk_widget_queue_draw),
                                view);
    }

  g_object_notify (G_OBJECT (view), "context");
}
