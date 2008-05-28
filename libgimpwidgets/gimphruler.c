/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimphruler.h"


#define RULER_HEIGHT          13
#define MINIMUM_INCR          5
#define MAXIMUM_SUBDIVIDE     5


typedef struct
{
  gint xsrc;
  gint ysrc;
} GimpHRulerPrivate;


static gint gimp_hruler_motion_notify (GtkWidget      *widget,
                                       GdkEventMotion *event);
static void gimp_hruler_draw_ticks    (GimpRuler      *ruler);
static void gimp_hruler_draw_pos      (GimpRuler      *ruler);

G_DEFINE_TYPE (GimpHRuler, gimp_hruler, GIMP_TYPE_RULER)

#define GIMP_HRULER_GET_PRIVATE(ruler) \
  G_TYPE_INSTANCE_GET_PRIVATE (ruler, GIMP_TYPE_HRULER, GimpHRulerPrivate)


static void
gimp_hruler_class_init (GimpHRulerClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GimpRulerClass *ruler_class  = GIMP_RULER_CLASS (klass);

  widget_class->motion_notify_event = gimp_hruler_motion_notify;

  ruler_class->draw_ticks = gimp_hruler_draw_ticks;
  ruler_class->draw_pos   = gimp_hruler_draw_pos;

  g_type_class_add_private (object_class, sizeof (GimpHRulerPrivate));
}

static void
gimp_hruler_init (GimpHRuler *hruler)
{
  GtkWidget *widget = GTK_WIDGET (hruler);

  widget->requisition.width  = widget->style->xthickness * 2 + 1;
  widget->requisition.height = widget->style->ythickness * 2 + RULER_HEIGHT;
}


GtkWidget*
gimp_hruler_new (void)
{
  return g_object_new (GIMP_TYPE_HRULER, NULL);
}

static gint
gimp_hruler_motion_notify (GtkWidget      *widget,
                           GdkEventMotion *event)
{
  GimpRuler *ruler = GIMP_RULER (widget);
  gdouble    lower;
  gdouble    upper;
  gdouble    position;
  gint       x;

  gdk_event_request_motions (event);
  x = event->x;

  gimp_ruler_get_range (ruler, &lower, &upper, NULL, NULL);

  position = lower + ((upper - lower) * x) / widget->allocation.width;

  g_object_set (ruler, "position", position, NULL);

  gimp_ruler_draw_pos (ruler);

  return FALSE;
}

static void
gimp_hruler_draw_ticks (GimpRuler *ruler)
{
  GtkWidget             *widget = GTK_WIDGET (ruler);
  const GimpRulerMetric *metric;
  GdkDrawable    *backing_store;
  cairo_t        *cr;
  gint            i;
  gint            width, height;
  gint            xthickness;
  gint            ythickness;
  gint            length, ideal_length;
  gdouble         lower, upper; /* Upper and lower limits, in ruler units */
  gdouble         increment;    /* Number of pixels per unit */
  gint            scale;         /* Number of units per major unit */
  gdouble         start, end, cur;
  gchar           unit_str[32];
  gint            digit_height;
  gint            digit_offset;
  gint            text_width;
  gint            pos;
  gdouble         max_size;
  PangoLayout    *layout;
  PangoRectangle  logical_rect, ink_rect;

  if (! GTK_WIDGET_DRAWABLE (widget))
    return;

  backing_store = _gimp_ruler_get_backing_store (ruler);

  xthickness = widget->style->xthickness;
  ythickness = widget->style->ythickness;

  layout = _gimp_ruler_create_pango_layout (widget, "012456789");
  pango_layout_get_extents (layout, &ink_rect, &logical_rect);

  digit_height = PANGO_PIXELS (ink_rect.height) + 2;
  digit_offset = ink_rect.y;

  width = widget->allocation.width;
  height = widget->allocation.height - ythickness * 2;

  gtk_paint_box (widget->style, backing_store,
                 GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                 NULL, widget, "hruler",
                 0, 0,
                 widget->allocation.width, widget->allocation.height);

  cr = gdk_cairo_create (backing_store);
  gdk_cairo_set_source_color (cr, &widget->style->fg[widget->state]);

  cairo_rectangle (cr,
                   xthickness,
                   height + ythickness,
                   widget->allocation.width - 2 * xthickness,
                   1);

  gimp_ruler_get_range (ruler, &lower, &upper, NULL, &max_size);

  metric = _gimp_ruler_get_metric (ruler);

  upper = upper / metric->pixels_per_unit;
  lower = lower / metric->pixels_per_unit;

  if ((upper - lower) == 0)
    goto out;

  increment = (gdouble) width / (upper - lower);

  /* determine the scale
   *  We calculate the text size as for the vruler instead of using
   *  text_width = gdk_string_width(font, unit_str), so that the result
   *  for the scale looks consistent with an accompanying vruler
   */
  scale = ceil (max_size / metric->pixels_per_unit);
  g_snprintf (unit_str, sizeof (unit_str), "%d", scale);
  text_width = strlen (unit_str) * digit_height + 1;

  for (scale = 0; scale < G_N_ELEMENTS (metric->ruler_scale); scale++)
    if (metric->ruler_scale[scale] * fabs (increment) > 2 * text_width)
      break;

  if (scale == G_N_ELEMENTS (metric->ruler_scale))
    scale = G_N_ELEMENTS (metric->ruler_scale) - 1;

  /* drawing starts here */
  length = 0;
  for (i = MAXIMUM_SUBDIVIDE - 1; i >= 0; i--)
    {
      gdouble subd_incr = ((gdouble) metric->ruler_scale[scale] /
                           (gdouble) metric->subdivide[i]);

      if (subd_incr * fabs (increment) <= MINIMUM_INCR)
        continue;

      /* Calculate the length of the tickmarks. Make sure that
       * this length increases for each set of ticks
       */
      ideal_length = height / (i + 1) - 1;
      if (ideal_length > ++length)
        length = ideal_length;

      if (lower < upper)
        {
          start = floor (lower / subd_incr) * subd_incr;
          end   = ceil  (upper / subd_incr) * subd_incr;
        }
      else
        {
          start = floor (upper / subd_incr) * subd_incr;
          end   = ceil  (lower / subd_incr) * subd_incr;
        }


      for (cur = start; cur <= end; cur += subd_incr)
        {
          pos = ROUND ((cur - lower) * increment);

          cairo_rectangle (cr,
                           pos, height + ythickness - length,
                           1,   length);

          /* draw label */
          if (i == 0)
            {
              g_snprintf (unit_str, sizeof (unit_str), "%d", (int) cur);

              pango_layout_set_text (layout, unit_str, -1);
              pango_layout_get_extents (layout, &logical_rect, NULL);

              gtk_paint_layout (widget->style,
                                backing_store,
                                GTK_WIDGET_STATE (widget),
                                FALSE,
                                NULL,
                                widget,
                                "hruler",
                                pos + 2,
                                ythickness + PANGO_PIXELS (logical_rect.y - digit_offset),
                                layout);
            }
        }
    }

  cairo_fill (cr);
out:
  cairo_destroy (cr);

  g_object_unref (layout);
}

static void
gimp_hruler_draw_pos (GimpRuler *ruler)
{
  GtkWidget *widget = GTK_WIDGET (ruler);

  if (GTK_WIDGET_DRAWABLE (ruler))
    {
      GimpHRulerPrivate *priv = GIMP_HRULER_GET_PRIVATE (ruler);
      gint               x, y;
      gint               width, height;
      gint               bs_width, bs_height;
      gint               xthickness;
      gint               ythickness;

      xthickness = widget->style->xthickness;
      ythickness = widget->style->ythickness;
      width  = widget->allocation.width;
      height = widget->allocation.height - ythickness * 2;

      bs_width = height / 2 + 2;
      bs_width |= 1;  /* make sure it's odd */
      bs_height = bs_width / 2 + 1;

      if ((bs_width > 0) && (bs_height > 0))
        {
          GdkDrawable *backing_store = _gimp_ruler_get_backing_store (ruler);
          cairo_t     *cr            = gdk_cairo_create (widget->window);
          gdouble      lower;
          gdouble      upper;
          gdouble      position;
          gdouble      increment;

          /*  If a backing store exists, restore the ruler  */
          if (backing_store)
            gdk_draw_drawable (widget->window,
                               widget->style->black_gc,
                               backing_store,
                               priv->xsrc, priv->ysrc,
                               priv->xsrc, priv->ysrc,
                               bs_width, bs_height);

          gimp_ruler_get_range (ruler, &lower, &upper, &position, NULL);

          increment = (gdouble) width / (upper - lower);

          x = ROUND ((position - lower) * increment) + (xthickness - bs_width) / 2 - 1;
          y = (height + bs_height) / 2 + ythickness;

          gdk_cairo_set_source_color (cr, &widget->style->fg[widget->state]);

          cairo_move_to (cr, x,                  y);
          cairo_line_to (cr, x + bs_width / 2., y + bs_height);
          cairo_line_to (cr, x + bs_width,      y);
          cairo_fill (cr);

          cairo_destroy (cr);

          priv->xsrc = x;
          priv->ysrc = y;
        }
    }
}
