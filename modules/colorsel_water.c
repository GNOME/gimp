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


/* definitions and variables */

#define IMAGE_SIZE   GIMP_COLOR_SELECTOR_SIZE


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
  gdouble            last_pressure;

  gfloat             pressure_adjust;
  guint32            motion_time;
  gint               button_state;
};

struct _ColorselWaterClass
{
  GimpColorSelectorClass  parent_class;
};


static GType      colorsel_water_get_type   (GTypeModule           *module);
static void       colorsel_water_class_init (ColorselWaterClass *klass);
static void       colorsel_water_init       (ColorselWater      *water);

static void       colorsel_water_finalize   (GObject           *object);

static void       colorsel_water_set_color  (GimpColorSelector *selector,
                                             const GimpRGB     *rgb,
                                             const GimpHSV     *hsv);

static void       select_area_draw          (GtkWidget         *preview);
static gboolean   button_press_event        (GtkWidget         *widget,
                                             GdkEventButton    *event,
                                             ColorselWater     *water);
static gboolean   button_release_event      (GtkWidget         *widget,
                                             GdkEventButton    *event,
                                             ColorselWater     *water);
static gboolean   motion_notify_event       (GtkWidget         *widget,
                                             GdkEventMotion    *event,
                                             ColorselWater     *water);
static gboolean   proximity_out_event       (GtkWidget         *widget,
                                             GdkEventProximity *event,
                                             ColorselWater     *water);
static void       pressure_adjust_update    (GtkAdjustment     *adj,
                                             ColorselWater     *water);


static const GimpModuleInfo colorsel_water_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Watercolor style color selector"),
  "Raph Levien <raph@acm.org>, Sven Neumann <sven@gimp.org>",
  "v0.3",
  "(c) 1998-1999, released under the GPL",
  "May, 10 1999"
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
      static const GTypeInfo select_info =
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
  GObjectClass           *object_class;
  GimpColorSelectorClass *selector_class;

  object_class   = G_OBJECT_CLASS (klass);
  selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize    = colorsel_water_finalize;

  selector_class->name      = _("Watercolor");
  selector_class->help_id   = "gimp-colorselector-watercolor";
  selector_class->stock_id  = GIMP_STOCK_TOOL_PAINTBRUSH;
  selector_class->set_color = colorsel_water_set_color;
}

static void
colorsel_water_init (ColorselWater *water)
{
  GtkWidget *preview;
  GtkWidget *event_box;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkObject *adj;
  GtkWidget *scale;

  water->pressure_adjust = 1.0;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (water), hbox, TRUE, FALSE, 0);

  hbox2 = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, TRUE, FALSE, 0);

  /* the event box */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox2), frame, FALSE, FALSE, 0); 

  event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (frame), event_box);

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview, IMAGE_SIZE, IMAGE_SIZE);
  gtk_container_add (GTK_CONTAINER (event_box), preview);
  g_signal_connect (preview, "size-allocate",
                    G_CALLBACK (select_area_draw), NULL);

  /* Event signals */
  g_signal_connect (event_box, "motion_notify_event",
                    G_CALLBACK (motion_notify_event),
                    water);
  g_signal_connect (event_box, "button_press_event",
                    G_CALLBACK (button_press_event),
                    water);
  g_signal_connect (event_box, "button_release_event",
                    G_CALLBACK (button_release_event),
                    water);
  g_signal_connect (event_box, "proximity_out_event",
                    G_CALLBACK (proximity_out_event),
                    water);

  gtk_widget_set_events (event_box,
                         GDK_EXPOSURE_MASK            |
                         GDK_LEAVE_NOTIFY_MASK        |
                         GDK_BUTTON_PRESS_MASK        |
                         GDK_KEY_PRESS_MASK           |
                         GDK_POINTER_MOTION_MASK      |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_PROXIMITY_OUT_MASK);

  /* The following call enables tracking and processing of extension
   * events for the drawing area
   */
  gtk_widget_set_extension_events (event_box, GDK_EXTENSION_EVENTS_ALL);
  gtk_widget_grab_focus (event_box);

  adj = gtk_adjustment_new (200.0 - water->pressure_adjust * 100.0,
                            0.0, 200.0, 1.0, 1.0, 0.0);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (pressure_adjust_update),
                    water);
  scale = gtk_vscale_new (GTK_ADJUSTMENT (adj));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gimp_help_set_help_data (scale, _("Pressure"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox2), scale, FALSE, FALSE, 0);

  gtk_widget_show_all (hbox);
}

static void
colorsel_water_finalize (GObject *object)
{
  ColorselWater *water;

  water = COLORSEL_WATER (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
colorsel_water_set_color (GimpColorSelector *selector,
                          const GimpRGB     *rgb,
                          const GimpHSV     *hsv)
{
  ColorselWater *water;

  water = COLORSEL_WATER (selector);
}

static gdouble
calc (gdouble x,
      gdouble y,
      gdouble angle)
{
  gdouble s, c;

  s = 1.6 * sin (angle * G_PI / 180) * 256.0 / IMAGE_SIZE;
  c = 1.6 * cos (angle * G_PI / 180) * 256.0 / IMAGE_SIZE;

  return 128 + (x - (IMAGE_SIZE >> 1)) * c - (y - (IMAGE_SIZE >> 1)) * s;
}

/* Initialize the preview */
static void
select_area_draw (GtkWidget *preview)
{
  guchar  buf[3 * IMAGE_SIZE * IMAGE_SIZE];
  gint    x, y;
  gdouble r, g, b;
  gdouble dr, dg, db;

  for (y = 0; y < IMAGE_SIZE; y++)
    {
      r = calc (0, y, 0);
      g = calc (0, y, 120);
      b = calc (0, y, 240);

      dr = calc (1, y, 0) - r;
      dg = calc (1, y, 120) - g;
      db = calc (1, y, 240) - b;

      for (x = 0; x < IMAGE_SIZE; x++)
        {
          buf[(x + IMAGE_SIZE * y) * 3]     = CLAMP ((gint) r, 0, 255);
          buf[(x + IMAGE_SIZE * y) * 3 + 1] = CLAMP ((gint) g, 0, 255);
          buf[(x + IMAGE_SIZE * y) * 3 + 2] = CLAMP ((gint) b, 0, 255);
          r += dr;
          g += dg;
          b += db;
        }
    }
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, IMAGE_SIZE, IMAGE_SIZE,
                          GIMP_RGB_IMAGE,
                          buf,
                          3 * IMAGE_SIZE);
}

static void
add_pigment (ColorselWater *water,
             gboolean       erase,
             gdouble        x,
             gdouble        y,
             gdouble        much)
{
  GimpColorSelector *selector;
  gdouble            r, g, b;

  selector = GIMP_COLOR_SELECTOR (water);

  much *= (gdouble) water->pressure_adjust; 

  if (erase)
    {
      selector->rgb.r = 1 - (1 - selector->rgb.r) * (1 - much);
      selector->rgb.g = 1 - (1 - selector->rgb.g) * (1 - much);
      selector->rgb.b = 1 - (1 - selector->rgb.b) * (1 - much);
    }
  else
    {
      r = calc (x, y, 0) / 255.0;
      if (r < 0) r = 0;
      if (r > 1) r = 1;

      g = calc (x, y, 120) / 255.0;
      if (g < 0) g = 0;
      if (g > 1) g = 1;

      b = calc (x, y, 240) / 255.0;
      if (b < 0) b = 0;
      if (b > 1) b = 1;

      selector->rgb.r *= (1 - (1 - r) * much);
      selector->rgb.g *= (1 - (1 - g) * much);
      selector->rgb.b *= (1 - (1 - b) * much);
    }

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
  gdouble much; /* how much pigment to mix in */

  if (pressure < water->last_pressure)
    water->last_pressure = pressure;

  much = sqrt ((x - water->last_x) * (x - water->last_x) +
               (y - water->last_y) * (y - water->last_y) +
               1000 *
               (pressure - water->last_pressure) *
               (pressure - water->last_pressure));

  much *= pressure * 0.05;

  add_pigment (water, erase, x, y, much);

  water->last_x        = x;
  water->last_y        = y;
  water->last_pressure = pressure;
}

static gboolean
button_press_event (GtkWidget      *widget,
                    GdkEventButton *event,
                    ColorselWater  *water)
{
  gboolean erase = FALSE;

  water->last_x        = event->x;
  water->last_y        = event->y;
  water->last_pressure = 0.5;

  gdk_event_get_axis ((GdkEvent *) event, GDK_AXIS_PRESSURE,
                      &water->last_pressure);

  water->button_state |= 1 << event->button;

  erase = (event->button != 1) || FALSE;
  /* FIXME: (event->source == GDK_SOURCE_ERASER) */

  add_pigment (water, erase, event->x, event->y, 0.05);
  water->motion_time = event->time;

  return FALSE;
}

static gboolean
button_release_event (GtkWidget      *widget,
                      GdkEventButton *event,
                      ColorselWater  *water)
{
  water->button_state &= ~(1 << event->button);

  return TRUE;
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
      guint32 last_motion_time;

      last_motion_time = event->time;

      erase = ((event->state &
                (GDK_BUTTON2_MASK | GDK_BUTTON3_MASK | GDK_BUTTON4_MASK)) ||
               FALSE);
      /* FIXME: (event->source == GDK_SOURCE_ERASER) */

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

              draw_brush (water, widget, erase, x, y, pressure);
            }

          g_free (coords);
        }
      else
        {
          gdouble pressure = 0.5;

          gdk_event_get_axis ((GdkEvent *) event, GDK_AXIS_PRESSURE, &pressure);

          draw_brush (water, widget, erase, event->x, event->y, pressure);
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
