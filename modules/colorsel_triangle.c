/*
 * colorsel_triangle module (C) 1999 Simon Budig <Simon.Budig@unix-ag.org>
 *    http://www.home.unix-ag.org/simon/gimp/colorsel.html
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
 *
 * Ported to loadable colour selector interface by Austin Donnelly
 * <austin@gimp.org>
 */

#include "config.h"

#include <stdlib.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


#define COLORWHEELRADIUS    (GIMP_COLOR_SELECTOR_SIZE / 2)
#define COLORTRIANGLERADIUS (COLORWHEELRADIUS - GIMP_COLOR_SELECTOR_BAR_SIZE)
#define PREVIEWSIZE         (2 * COLORWHEELRADIUS + 1)
#define BGCOLOR             180
#define PREVIEW_MASK        (GDK_EXPOSURE_MASK       | \
                             GDK_BUTTON_PRESS_MASK   | \
                             GDK_BUTTON_RELEASE_MASK | \
                             GDK_BUTTON_MOTION_MASK )


#define COLORSEL_TYPE_TRIANGLE            (colorsel_triangle_type)
#define COLORSEL_TRIANGLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLORSEL_TYPE_TRIANGLE, ColorselTriangle))
#define COLORSEL_TRIANGLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), COLORSEL_TYPE_TRIANGLE, ColorselTriangleClass))
#define COLORSEL_IS_TRIANGLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLORSEL_TYPE_TRIANGLE))
#define COLORSEL_IS_TRIANGLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), COLORSEL_TYPE_TRIANGLE))


typedef struct _ColorselTriangle      ColorselTriangle;
typedef struct _ColorselTriangleClass ColorselTriangleClass;

struct _ColorselTriangle
{
  GimpColorSelector  parent_instance;

  gdouble            oldsat;
  gdouble            oldval;
  gint               mode;
  GtkWidget         *preview;
};

struct _ColorselTriangleClass
{
  GimpColorSelectorClass  parent_class;
};


static GType      colorsel_triangle_get_type   (GTypeModule           *module);
static void       colorsel_triangle_class_init (ColorselTriangleClass *klass);
static void       colorsel_triangle_init       (ColorselTriangle      *triangle);

static void       colorsel_triangle_finalize        (GObject           *object);

static void       colorsel_triangle_set_color       (GimpColorSelector *selector,
                                                     const GimpRGB     *rgb,
                                                     const GimpHSV     *hsv);

static void       colorsel_xy_to_triangle_buf       (gint              x,
                                                     gint              y,
                                                     gdouble           hue,
                                                     guchar           *buf,
                                                     gint              sx,
                                                     gint              sy,
                                                     gint              vx,
                                                     gint              vy,
                                                     gint              hx,
                                                     gint              hy);

static GtkWidget *colorsel_triangle_create_preview  (ColorselTriangle *triangle);
static void       colorsel_triangle_update_previews (ColorselTriangle *triangle,
                                                     gboolean          hue_changed);
static gboolean   colorsel_triangle_event           (GtkWidget        *widget,
                                                     GdkEvent         *event,
                                                     ColorselTriangle *triangle);


static const GimpModuleInfo colorsel_triangle_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Painter-style triangle color selector"),
  "Simon Budig <Simon.Budig@unix-ag.org>",
  "v0.03",
  "(c) 1999, released under the GPL",
  "17 Jan 1999"
};

static const GtkTargetEntry targets[] =
{
  { "application/x-color", 0 }
};


static GType                   colorsel_triangle_type = 0;
static GimpColorSelectorClass *parent_class           = NULL;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &colorsel_triangle_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  colorsel_triangle_get_type (module);

  return TRUE;
}

static GType
colorsel_triangle_get_type (GTypeModule *module)
{
  if (! colorsel_triangle_type)
    {
      static const GTypeInfo select_info =
      {
        sizeof (ColorselTriangleClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) colorsel_triangle_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (ColorselTriangle),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) colorsel_triangle_init,
      };

      colorsel_triangle_type =
        g_type_module_register_type (module,
                                     GIMP_TYPE_COLOR_SELECTOR,
                                     "ColorselTriangle",
                                     &select_info, 0);
    }

  return colorsel_triangle_type;
}

static void
colorsel_triangle_class_init (ColorselTriangleClass *klass)
{
  GObjectClass           *object_class;
  GimpColorSelectorClass *selector_class;

  object_class   = G_OBJECT_CLASS (klass);
  selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize    = colorsel_triangle_finalize;

  selector_class->name      = _("Triangle");
  selector_class->help_id   = "gimp-colorselector-triangle";
  selector_class->stock_id  = GIMP_STOCK_COLOR_TRIANGLE;
  selector_class->set_color = colorsel_triangle_set_color;
}

static void
colorsel_triangle_init (ColorselTriangle *triangle)
{
  GtkWidget *frame;
  GtkWidget *hbox;

  triangle->oldsat = 0;
  triangle->oldval = 0;
  triangle->mode   = 0;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (triangle), hbox, TRUE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);
  gtk_widget_show (frame);

  triangle->preview = colorsel_triangle_create_preview (triangle);
  gtk_container_add (GTK_CONTAINER (frame), triangle->preview);
  gtk_widget_show (triangle->preview);

  colorsel_triangle_update_previews (triangle, TRUE);
}

static void
colorsel_triangle_finalize (GObject *object)
{
  ColorselTriangle *triangle;

  triangle = COLORSEL_TRIANGLE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
colorsel_triangle_set_color (GimpColorSelector *selector,
			     const GimpRGB     *rgb,
			     const GimpHSV     *hsv)
{
  ColorselTriangle *triangle;

  triangle = COLORSEL_TRIANGLE (selector);

  colorsel_triangle_update_previews (triangle, TRUE);
}

static GtkWidget *
colorsel_triangle_create_preview (ColorselTriangle *triangle)
{
  GtkWidget *preview;
  guchar     buf[3 * PREVIEWSIZE];
  gint       i;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (preview), GDK_RGB_DITHER_MAX);
  gtk_widget_set_events (GTK_WIDGET (preview), PREVIEW_MASK );
  gtk_preview_size (GTK_PREVIEW (preview), PREVIEWSIZE, PREVIEWSIZE);

  g_signal_connect (preview, "motion_notify_event",
                    G_CALLBACK (colorsel_triangle_event),
                    triangle);
  g_signal_connect (preview, "button_press_event",
                    G_CALLBACK (colorsel_triangle_event),
                    triangle);
  g_signal_connect (preview, "button_release_event",
                    G_CALLBACK (colorsel_triangle_event),
                    triangle);

  for (i=0; i < 3 * PREVIEWSIZE; i += 3)
    buf[i] = buf[i+1] = buf[i+2] = BGCOLOR;
  for (i=0; i < PREVIEWSIZE; i++)
    gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, i, PREVIEWSIZE);

  return preview;
}

static void
colorsel_triangle_update_previews (ColorselTriangle *triangle,
                                   gboolean          hue_changed)
{
  GimpColorSelector *selector;
  guchar             buf[3 * PREVIEWSIZE];
  gint               x, y, k, r2, dx, col;
  gint               x0, y0;
  gdouble            hue, sat, val, atn;
  gint               hx,hy, sx,sy, vx,vy;

  selector = GIMP_COLOR_SELECTOR (triangle);

  hue = (gdouble) selector->hsv.h * 2 * G_PI;

  /* Colored point (value = 1, saturation = 1) */
  hx = RINT (sin (hue) * COLORTRIANGLERADIUS);
  hy = RINT (cos (hue) * COLORTRIANGLERADIUS);

  /* Black point (value = 0, saturation not important) */
  sx = RINT (sin (hue - 2 * G_PI / 3) * COLORTRIANGLERADIUS);
  sy = RINT (cos (hue - 2 * G_PI / 3) * COLORTRIANGLERADIUS);

  /* White point (value = 1, saturation = 0) */
  vx = RINT (sin (hue + 2 * G_PI / 3) * COLORTRIANGLERADIUS);
  vy = RINT (cos (hue + 2 * G_PI / 3) * COLORTRIANGLERADIUS);

  hue = selector->hsv.h * 360.0;

  if (hue_changed)
    {
      for (y = COLORWHEELRADIUS; y >= -COLORWHEELRADIUS; y--)
	{
	  dx = RINT (sqrt (fabs (COLORWHEELRADIUS * COLORWHEELRADIUS - y * y)));
	  for (x = -dx, k = 0; x <= dx; x++)
	    {
	      buf[k] = buf[k+1] = buf[k+2] = BGCOLOR;
	      r2 = (x * x) + (y * y);

	      if (r2 <= COLORWHEELRADIUS * COLORWHEELRADIUS)
		{
		  if (r2 > COLORTRIANGLERADIUS * COLORTRIANGLERADIUS)
		    {
		      atn = atan2 (x, y);
		      if (atn < 0)
			atn = atn + 2 * G_PI;
		      gimp_hsv_to_rgb4 (buf + k, atn / (2 * G_PI), 1, 1);
		    }
		  else
		    {
		      colorsel_xy_to_triangle_buf (x, y, hue, buf + k, hx, hy, sx, sy, vx, vy);
		    }
		}

	      k += 3;
	    }

	  gtk_preview_draw_row (GTK_PREVIEW (triangle->preview), buf,
				COLORWHEELRADIUS - dx,
				COLORWHEELRADIUS - y, 2 * dx + 1);
	}

      /* marker in outer ring */

      x0 = RINT (sin (hue * G_PI / 180) *
		 ((gdouble) (COLORWHEELRADIUS - COLORTRIANGLERADIUS + 1) / 2 +
		  COLORTRIANGLERADIUS));
      y0 = RINT (cos (hue * G_PI / 180) *
		 ((gdouble) (COLORWHEELRADIUS - COLORTRIANGLERADIUS + 1) / 2 +
		  COLORTRIANGLERADIUS));

      atn = atan2 (x0, y0);
      if (atn < 0)
	atn = atn + 2 * G_PI;
      gimp_hsv_to_rgb4 (buf, atn / (2 * G_PI), 1, 1);

      col = GIMP_RGB_INTENSITY (buf[0], buf[1], buf[2]) > 127 ? 0 : 255;

      for (y = y0 - 4 ; y <= y0 + 4 ; y++)
	{
	  for (x = x0 - 4, k=0 ; x <= x0 + 4 ; x++)
	    {
	      r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);

	      if (r2 <= 20 && r2 >= 6)
		{
		  buf[k] = buf[k+1] = buf[k+2] = col;
		}
	      else
		{
		  atn = atan2 (x, y);
		  if (atn < 0)
		    atn = atn + 2 * G_PI;
		  gimp_hsv_to_rgb4 (buf + k, atn / (2 * G_PI), 1, 1);
		}

	      k += 3;
	    }

	  gtk_preview_draw_row (GTK_PREVIEW (triangle->preview), buf,
				COLORWHEELRADIUS + x0 - 4,
				COLORWHEELRADIUS - y, 9);
	}
    }
  else
    {
      /* delete marker in triangle */

      sat = triangle->oldsat;
      val = triangle->oldval;
      x0 = RINT (sx + (vx - sx) * val + (hx - vx) * sat * val);
      y0 = RINT (sy + (vy - sy) * val + (hy - vy) * sat * val);

      for (y = y0 - 4 ; y <= y0 + 4 ; y++)
	{
	  for (x = x0 - 4, k=0 ; x <= x0 + 4 ; x++)
	    {
	      buf[k] = buf[k+1] = buf[k+2] = BGCOLOR;
	      r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);

	      if (x * x + y * y > COLORTRIANGLERADIUS * COLORTRIANGLERADIUS)
		{
		  atn = atan2 (x, y);
		  if (atn < 0)
		    atn = atn + 2 * G_PI;
		  gimp_hsv_to_rgb4 (buf + k, atn / (2 * G_PI), 1, 1);
		}
	      else
		{
		  colorsel_xy_to_triangle_buf (x, y, hue, buf + k, hx, hy, sx, sy, vx, vy);
		}

	      k += 3;
	    }

	  gtk_preview_draw_row (GTK_PREVIEW (triangle->preview), buf,
				COLORWHEELRADIUS + x0 - 4,
				COLORWHEELRADIUS - y, 9);
	}
    }

  /* marker in triangle */

  col = gimp_rgb_intensity (&selector->rgb) > 0.5 ? 0 : 255;

  sat = triangle->oldsat = selector->hsv.s;
  val = triangle->oldval = selector->hsv.v;

  x0 = RINT (sx + (vx - sx) * val + (hx - vx) * sat * val);
  y0 = RINT (sy + (vy - sy) * val + (hy - vy) * sat * val);

  for (y = y0 - 4 ; y <= y0 + 4 ; y++)
    {
      for (x = x0 - 4, k=0 ; x <= x0 + 4 ; x++)
	{
	  buf[k] = buf[k+1] = buf[k+2] = BGCOLOR;
	  r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);

	  if (r2 <= 20 && r2 >= 6)
	    {
	      buf[k] = buf[k+1] = buf[k+2] = col;
	    }
	  else
	    {
	      if (x * x + y * y > COLORTRIANGLERADIUS * COLORTRIANGLERADIUS)
		{
		  atn = atan2 (x, y);
		  if (atn < 0)
		    atn = atn + 2 * G_PI;
		  gimp_hsv_to_rgb4 (buf + k, atn / (2 * G_PI), 1, 1);
		}
	      else
		{
		  colorsel_xy_to_triangle_buf (x, y, hue, buf + k, hx, hy, sx, sy, vx, vy);
		}
	    }

	  k += 3;
	}

      gtk_preview_draw_row (GTK_PREVIEW (triangle->preview), buf,
			    COLORWHEELRADIUS + x0 - 4,
			    COLORWHEELRADIUS - y, 9);
    }

  gtk_widget_queue_draw (triangle->preview);
}

static void
colorsel_xy_to_triangle_buf (gint     x,
                             gint     y,
			     gdouble  hue,
                             guchar  *buf,
			     gint     hx,
                             gint     hy, /* colored point */
			     gint     sx,
                             gint     sy, /* black point */
			     gint     vx,
                             gint     vy) /* white point */
{
  gdouble sat, val;

  /*
   * The value is 1 - (the distance from the H->V line).
   * I forgot the linear algebra behind it...
   */
  val = (gdouble) ( (x - sx) * (hy - vy) -  (y - sy) * (hx - vx)) /
	(gdouble) ((vx - sx) * (hy - vy) - (vy - sy) * (hx - vx));

  if (val >= 0 && val<= 1)
    {
      if (abs (hy - vy) < abs (hx - vx))
	{
	  sat = (val == 0 ? 0: ((gdouble) (x - sx - val * (vx - sx)) /
					  (val * (hx - vx))));
	}
      else
	{
	  sat = (val == 0 ? 0: ((gdouble) (y - sy - val * (vy - sy)) /
					  (val * (hy - vy))));
	}

      /* Yes, this ugly 1.00*01 fixes some subtle rounding errors... */
      if (sat >= 0 && sat <= 1.000000000000001)
	gimp_hsv_to_rgb4 (buf, hue / 360, sat, val);
    }
}

static gboolean
colorsel_triangle_event (GtkWidget        *widget,
                         GdkEvent         *event,
                         ColorselTriangle *triangle)
{
  GimpColorSelector *selector;
  gint               x,y, angle, mousex, mousey;
  gdouble            r;
  gdouble            hue, sat, val;
  gint               hx,hy, sx,sy, vx,vy;

  selector = GIMP_COLOR_SELECTOR (triangle);

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      gtk_grab_add (widget);
      x = event->button.x - COLORWHEELRADIUS - 1;
      y = - event->button.y + COLORWHEELRADIUS + 1;
      r = sqrt ((gdouble) (x * x + y * y));
      angle = ((gint) RINT (atan2 (x, y) / G_PI * 180) + 360 ) % 360;
      if ( /* r <= COLORWHEELRADIUS  && */ r > COLORTRIANGLERADIUS)
        triangle->mode = 1;  /* Dragging in the Ring */
      else
        triangle->mode = 2;  /* Dragging in the Triangle */
      break;

    case GDK_MOTION_NOTIFY:
      x = event->motion.x - COLORWHEELRADIUS - 1;
      y = - event->motion.y + COLORWHEELRADIUS + 1;
      r = sqrt ((gdouble) (x * x + y * y));
      angle = ((gint) RINT (atan2 (x, y) / G_PI * 180) + 360 ) % 360;
      break;

    case GDK_BUTTON_RELEASE:
      triangle->mode = 0;
      gtk_grab_remove (widget);

      /* callback the user */
      gimp_color_selector_color_changed (GIMP_COLOR_SELECTOR (triangle));

      return FALSE;
      break;

    default:
      gtk_widget_get_pointer (widget, &x, &y);
      x = x - COLORWHEELRADIUS - 1;
      y = - y + COLORWHEELRADIUS + 1;
      r = sqrt ((gdouble) (x * x + y * y));
      angle = ((gint) RINT (atan2 (x, y) / G_PI * 180) + 360 ) % 360;
      break;
    }

  gtk_widget_get_pointer (widget, &mousex, &mousey);
  if ((event->type == GDK_MOTION_NOTIFY &&
      (mousex != event->motion.x || mousey != event->motion.y)))
     return FALSE;

  if (triangle->mode == 1 ||
      (r > COLORWHEELRADIUS &&
       (abs (angle - selector->hsv.h * 360.0) < 30 ||
	abs (abs (angle - selector->hsv.h * 360.0) - 360) < 30)))
    {
      selector->hsv.h = angle / 360.0;
      gimp_hsv_to_rgb (&selector->hsv, &selector->rgb);
      colorsel_triangle_update_previews (triangle, TRUE);
    }
  else
    {
      hue = selector->hsv.h * 2 * G_PI;
      hx = sin (hue) * COLORTRIANGLERADIUS;
      hy = cos (hue) * COLORTRIANGLERADIUS;
      sx = sin (hue - 2 * G_PI / 3) * COLORTRIANGLERADIUS;
      sy = cos (hue - 2 * G_PI / 3) * COLORTRIANGLERADIUS;
      vx = sin (hue + 2 * G_PI / 3) * COLORTRIANGLERADIUS;
      vy = cos (hue + 2 * G_PI / 3) * COLORTRIANGLERADIUS;
      hue = selector->hsv.h * 360.0;

      if ((x - sx) * vx + (y - sy) * vy < 0)
	{
	  sat = 1;
	  val = ((gdouble) ( (x - sx) * (hx - sx) +  (y - sy) * (hy - sy)))
	                 / ((hx - sx) * (hx - sx) + (hy - sy) * (hy - sy));
	  if (val < 0)
	    val = 0;
	  else if (val > 1)
	    val = 1;
	}
      else if ((x - sx) * hx + (y - sy) * hy < 0)
	{
	  sat = 0;
	  val = ((gdouble) ( (x - sx) * (vx - sx) +  (y - sy) * (vy - sy)))
	                 / ((vx - sx) * (vx - sx) + (vy - sy) * (vy - sy));
	  if (val < 0)
	    val = 0;
	  else if (val > 1)
	    val = 1;
	}
      else if ((x - hx) * sx + (y - hy) * sy < 0)
	{
	  val = 1;
	  sat = ((gdouble) ( (x - vx) * (hx - vx) +  (y - vy) * (hy - vy)))
	                 / ((hx - vx) * (hx - vx) + (hy - vy) * (hy - vy));
	  if (sat < 0)
	    sat = 0;
	  else if (sat > 1)
	    sat = 1;
	}
      else
	{
	  val =   (gdouble) ( (x - sx) * (hy - vy) -  (y - sy) * (hx - vx))
	        / (gdouble) ((vx - sx) * (hy - vy) - (vy - sy) * (hx - vx));
	  if (val <= 0)
	    {
	      val = 0;
	      sat = 0;
	    }
	  else
	    {
	      if (val > 1)
		val = 1;
	      if (hy == vy)
		sat = (gdouble) (x - sx - val * (vx - sx)) / (val * (gdouble) (hx - vx));
	      else
		sat = (gdouble) (y - sy - val * (vy - sy)) / (val * (gdouble) (hy - vy));

	      if (sat < 0)
		sat = 0;
	      else if (sat > 1)
		sat = 1;
	    }
	}

      selector->hsv.s = sat;
      selector->hsv.v = val;
      gimp_hsv_to_rgb (&selector->hsv, &selector->rgb);
      colorsel_triangle_update_previews (triangle, FALSE);
    }

  /* callback the user */
  gimp_color_selector_color_changed (GIMP_COLOR_SELECTOR (triangle));

  return FALSE;
}
