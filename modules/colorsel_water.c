/* Watercolor color_select_module, Raph Levien <raph@acm.org>, February 1998
 *
 * Ported to loadable color-selector, Sven Neumann <sven@gimp.org>, May 1999
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


#define COLORSEL_TYPE_WATER            (colorsel_water_type)
#define COLORSEL_WATER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLORSEL_TYPE_WATER, ColorselWater))
#define COLORSEL_WATER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), COLORSEL_TYPE_WATER, ColorselWaterClass))
#define COLORSEL_IS_WATER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLORSEL_TYPE_WATER))
#define COLORSEL_IS_WATER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), COLORSEL_TYPE_WATER))


typedef struct _ColorselWater      ColorselWater;
typedef struct _ColorselWaterClass ColorselWaterClass;

struct _ColorselWater
{
  GimpColorSelector  parent_instance;

  gdouble            last_x;
  gdouble            last_y;

  gfloat             pressure_adjust;
  guint32            motion_time;
};

struct _ColorselWaterClass
{
  GimpColorSelectorClass  parent_class;
};


static GType      colorsel_water_get_type   (GTypeModule        *module);
static void       colorsel_water_class_init (ColorselWaterClass *klass);
static void       colorsel_water_init       (ColorselWater      *water);

static void       select_area_expose        (GtkWidget          *widget,
                                             GdkEventExpose     *event);
static gboolean   button_press_event        (GtkWidget          *widget,
                                             GdkEventButton     *event,
                                             ColorselWater      *water);
static gboolean   motion_notify_event       (GtkWidget          *widget,
                                             GdkEventMotion     *event,
                                             ColorselWater      *water);
static gboolean   proximity_out_event       (GtkWidget          *widget,
                                             GdkEventProximity  *event,
                                             ColorselWater      *water);
static void       pressure_adjust_update    (GtkAdjustment      *adj,
                                             ColorselWater      *water);


static const GimpModuleInfo colorsel_water_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Watercolor style color selector"),
  "Raph Levien <raph@acm.org>, Sven Neumann <sven@gimp.org>",
  "v0.4",
  "released under the GPL",
  "1998-2006"
};

static const GtkTargetEntry targets[] =
{
  { "application/x-color", 0 }
};


static GType                   colorsel_water_type = 0;
static GimpColorSelectorClass *parent_class           = NULL;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &colorsel_water_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  colorsel_water_get_type (module);

  return TRUE;
}

static GType
colorsel_water_get_type (GTypeModule *module)
{
  if (! colorsel_water_type)
    {
      const GTypeInfo select_info =
      {
        sizeof (ColorselWaterClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) colorsel_water_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (ColorselWater),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) colorsel_water_init,
      };

      colorsel_water_type =
        g_type_module_register_type (module,
                                     GIMP_TYPE_COLOR_SELECTOR,
                                     "ColorselWater",
                                     &select_info, 0);
    }

  return colorsel_water_type;
}

static void
colorsel_water_class_init (ColorselWaterClass *klass)
{
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  selector_class->name      = _("Watercolor");
  selector_class->help_id   = "gimp-colorselector-watercolor";
  selector_class->stock_id  = GIMP_STOCK_TOOL_PAINTBRUSH;
}

static void
colorsel_water_init (ColorselWater *water)
{
  GtkWidget *hbox;
  GtkWidget *area;
  GtkWidget *frame;
  GtkObject *adj;
  GtkWidget *scale;

  water->pressure_adjust = 1.0;

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (water), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  area = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (frame), area);
  g_signal_connect (area, "expose-event",
                    G_CALLBACK (select_area_expose),
                    NULL);

  /* Event signals */
  g_signal_connect (area, "motion-notify-event",
                    G_CALLBACK (motion_notify_event),
                    water);
  g_signal_connect (area, "button-press-event",
                    G_CALLBACK (button_press_event),
                    water);
  g_signal_connect (area, "proximity-out-event",
                    G_CALLBACK (proximity_out_event),
                    water);

  gtk_widget_add_events (area,
                         GDK_LEAVE_NOTIFY_MASK        |
                         GDK_BUTTON_PRESS_MASK        |
                         GDK_KEY_PRESS_MASK           |
                         GDK_POINTER_MOTION_MASK      |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_PROXIMITY_OUT_MASK);

  /* The following call enables tracking and processing of extension
   * events for the drawing area
   */
  gtk_widget_set_extension_events (area, GDK_EXTENSION_EVENTS_ALL);
  gtk_widget_grab_focus (area);

  adj = gtk_adjustment_new (200.0 - water->pressure_adjust * 100.0,
                            0.0, 200.0, 1.0, 1.0, 0.0);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (pressure_adjust_update),
                    water);

  scale = gtk_vscale_new (GTK_ADJUSTMENT (adj));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gimp_help_set_help_data (scale, _("Pressure"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), scale, FALSE, FALSE, 0);

  gtk_widget_show_all (hbox);
}

static gdouble
calc (gdouble x,
      gdouble y,
      gdouble angle)
{
  gdouble s = 2.0 * sin (angle * G_PI / 180.0) * 256.0;
  gdouble c = 2.0 * cos (angle * G_PI / 180.0) * 256.0;

  return 128 + (x - 0.5) * c - (y - 0.5) * s;
}

static void
select_area_expose (GtkWidget      *widget,
                    GdkEventExpose *event)
{
  gdouble  width  = widget->allocation.width;
  gdouble  height = widget->allocation.height;
  gdouble  dx     = 1.0 / width;
  gdouble  dy     = 1.0 / height;
  guchar  *buf    = g_alloca (3 * event->area.width * event->area.height);
  guchar  *dest   = buf;
  gdouble  y;
  gint     i, j;

  for (j = 0, y = event->area.y / height; j < event->area.height; j++, y += dy)
    {
      guchar  *d  = dest;

      gdouble  r  = calc (0, y, 0);
      gdouble  g  = calc (0, y, 120);
      gdouble  b  = calc (0, y, 240);

      gdouble  dr = calc (dx, y, 0)   - r;
      gdouble  dg = calc (dx, y, 120) - g;
      gdouble  db = calc (dx, y, 240) - b;

      r += event->area.x * dr;
      g += event->area.x * dg;
      b += event->area.x * db;

      for (i = 0; i < event->area.width; i++)
        {
          d[0] = CLAMP ((gint) r, 0, 255);
          d[1] = CLAMP ((gint) g, 0, 255);
          d[2] = CLAMP ((gint) b, 0, 255);

          r += dr;
          g += dg;
          b += db;

          d += 3;
        }

      dest += event->area.width * 3;
    }

  gdk_draw_rgb_image_dithalign (widget->window,
                                widget->style->fg_gc[widget->state],
                                event->area.x, event->area.y,
                                event->area.width, event->area.height,
                                GDK_RGB_DITHER_MAX,
                                buf, 3 * event->area.width,
                                -event->area.x, -event->area.y);
}

static void
add_pigment (ColorselWater *water,
             gboolean       erase,
             gdouble        x,
             gdouble        y,
             gdouble        much)
{
  GimpColorSelector *selector = GIMP_COLOR_SELECTOR (water);

  much *= (gdouble) water->pressure_adjust;

  if (erase)
    {
      selector->rgb.r = 1.0 - (1.0 - selector->rgb.r) * (1.0 - much);
      selector->rgb.g = 1.0 - (1.0 - selector->rgb.g) * (1.0 - much);
      selector->rgb.b = 1.0 - (1.0 - selector->rgb.b) * (1.0 - much);
    }
  else
    {
      gdouble r = calc (x, y, 0)   / 256.0;
      gdouble g = calc (x, y, 120) / 256.0;
      gdouble b = calc (x, y, 240) / 256.0;

      selector->rgb.r *= (1.0 - (1.0 - r) * much);
      selector->rgb.g *= (1.0 - (1.0 - g) * much);
      selector->rgb.b *= (1.0 - (1.0 - b) * much);
    }

  gimp_rgb_clamp (&selector->rgb);

  gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);

  gimp_color_selector_color_changed (selector);
}

static void
draw_brush (ColorselWater *water,
            GtkWidget     *widget,
            gboolean       erase,
            gdouble        x,
            gdouble        y,
            gdouble        pressure)
{
  gdouble much = sqrt (SQR (x - water->last_x) + SQR (y - water->last_y));

  add_pigment (water, erase, x, y, much * pressure);

  water->last_x = x;
  water->last_y = y;
}

static gboolean
button_press_event (GtkWidget      *widget,
                    GdkEventButton *event,
                    ColorselWater  *water)
{
  gboolean erase;

  water->last_x = event->x / widget->allocation.width;
  water->last_y = event->y / widget->allocation.height;

  erase = (event->button != 1);
  /* FIXME: (event->source == GDK_SOURCE_ERASER) */

  if (event->state & GDK_SHIFT_MASK)
    erase = !erase;

  add_pigment (water, erase, water->last_x, water->last_y, 0.05);

  water->motion_time = event->time;

  return FALSE;
}

static gboolean
motion_notify_event (GtkWidget      *widget,
                     GdkEventMotion *event,
                     ColorselWater  *water)
{
  GdkTimeCoord **coords;
  gint           nevents;
  gint           i;
  gboolean       erase;

  if (event->state & (GDK_BUTTON1_MASK |
                      GDK_BUTTON2_MASK |
                      GDK_BUTTON3_MASK |
                      GDK_BUTTON4_MASK))
    {
      guint32 last_motion_time = event->time;

      erase = ((event->state &
                (GDK_BUTTON2_MASK | GDK_BUTTON3_MASK | GDK_BUTTON4_MASK)) ||
               FALSE);
      /* FIXME: (event->source == GDK_SOURCE_ERASER) */

      if (event->state & GDK_SHIFT_MASK)
        erase = !erase;

      water->motion_time = event->time;

      if (gdk_device_get_history (event->device,
                                  event->window,
                                  last_motion_time,
                                  event->time,
                                  &coords,
                                  &nevents))
        {
          for (i = 0; i < nevents; i++)
            {
              gdouble x        = 0.0;
              gdouble y        = 0.0;
              gdouble pressure = 0.5;

              gdk_device_get_axis (event->device, coords[i]->axes,
                                   GDK_AXIS_X, &x);
              gdk_device_get_axis (event->device, coords[i]->axes,
                                   GDK_AXIS_Y, &y);
              gdk_device_get_axis (event->device, coords[i]->axes,
                                   GDK_AXIS_PRESSURE, &pressure);

              draw_brush (water, widget, erase,
                          x / widget->allocation.width,
                          y / widget->allocation.height, pressure);
            }

          g_free (coords);
        }
      else
        {
          gdouble pressure = 0.5;

          gdk_event_get_axis ((GdkEvent *) event, GDK_AXIS_PRESSURE, &pressure);

          draw_brush (water, widget, erase,
                      event->x / widget->allocation.width,
                      event->y / widget->allocation.height, pressure);
        }
    }

  if (event->is_hint)
    gdk_device_get_state (event->device, event->window, NULL, NULL);

  return TRUE;
}

static gboolean
proximity_out_event (GtkWidget         *widget,
                     GdkEventProximity *event,
                     ColorselWater     *water)
{
  return TRUE;
}

static void
pressure_adjust_update (GtkAdjustment *adj,
                        ColorselWater *water)
{
  water->pressure_adjust = (adj->upper - adj->value) / 100.0;
}
