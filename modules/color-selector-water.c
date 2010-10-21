/* Watercolor color_select_module, Raph Levien <raph@acm.org>, February 1998
 *
 * Ported to loadable color-selector, Sven Neumann <sven@gimp.org>, May 1999
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


#define COLORSEL_TYPE_WATER            (colorsel_water_get_type ())
#define COLORSEL_WATER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLORSEL_TYPE_WATER, ColorselWater))
#define COLORSEL_WATER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), COLORSEL_TYPE_WATER, ColorselWaterClass))
#define COLORSEL_IS_WATER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLORSEL_TYPE_WATER))
#define COLORSEL_IS_WATER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), COLORSEL_TYPE_WATER))


typedef struct _ColorselWater      ColorselWater;
typedef struct _ColorselWaterClass ColorselWaterClass;

struct _ColorselWater
{
  GimpColorSelector   parent_instance;

  GtkWidget          *area;

  gdouble             last_x;
  gdouble             last_y;

  gfloat              pressure_adjust;
  guint32             motion_time;

  GimpColorConfig    *config;
  GimpColorTransform *transform;
};

struct _ColorselWaterClass
{
  GimpColorSelectorClass  parent_class;
};


static GType      colorsel_water_get_type          (void);

static void       colorsel_water_dispose           (GObject           *object);

static void       colorsel_water_set_config        (GimpColorSelector *selector,
                                                    GimpColorConfig   *config);

static void       colorsel_water_create_transform  (ColorselWater     *water);
static void       colorsel_water_destroy_transform (ColorselWater     *water);

static gboolean   select_area_draw                 (GtkWidget         *widget,
                                                    cairo_t           *cr,
                                                    ColorselWater     *water);
static gboolean   button_press_event               (GtkWidget         *widget,
                                                    GdkEventButton    *event,
                                                    ColorselWater     *water);
static gboolean   motion_notify_event              (GtkWidget         *widget,
                                                    GdkEventMotion    *event,
                                                    ColorselWater     *water);
static gboolean   proximity_out_event              (GtkWidget         *widget,
                                                    GdkEventProximity *event,
                                                    ColorselWater     *water);
static void       pressure_adjust_update           (GtkAdjustment     *adj,
                                                    ColorselWater     *water);


static const GimpModuleInfo colorsel_water_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Watercolor style color selector"),
  "Raph Levien <raph@acm.org>, Sven Neumann <sven@gimp.org>",
  "v0.4",
  "released under the GPL",
  "1998-2006"
};


G_DEFINE_DYNAMIC_TYPE (ColorselWater, colorsel_water,
                       GIMP_TYPE_COLOR_SELECTOR)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &colorsel_water_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  colorsel_water_register_type (module);

  return TRUE;
}

static void
colorsel_water_class_init (ColorselWaterClass *klass)
{
  GObjectClass           *object_class   = G_OBJECT_CLASS (klass);
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  object_class->dispose      = colorsel_water_dispose;

  selector_class->name       = _("Watercolor");
  selector_class->help_id    = "gimp-colorselector-watercolor";
  selector_class->icon_name  = GIMP_ICON_COLOR_SELECTOR_WATER;
  selector_class->set_config = colorsel_water_set_config;
}

static void
colorsel_water_class_finalize (ColorselWaterClass *klass)
{
}

static void
colorsel_water_init (ColorselWater *water)
{
  GtkWidget     *hbox;
  GtkWidget     *frame;
  GtkAdjustment *adj;
  GtkWidget     *scale;

  colorsel_water_get_type (); /* useless function call to silence compiler */

  water->pressure_adjust = 1.0;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (water), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  water->area = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (frame), water->area);
  g_signal_connect (water->area, "draw",
                    G_CALLBACK (select_area_draw),
                    water);

  /* Event signals */
  g_signal_connect (water->area, "motion-notify-event",
                    G_CALLBACK (motion_notify_event),
                    water);
  g_signal_connect (water->area, "button-press-event",
                    G_CALLBACK (button_press_event),
                    water);
  g_signal_connect (water->area, "proximity-out-event",
                    G_CALLBACK (proximity_out_event),
                    water);

  gtk_widget_add_events (water->area,
                         GDK_LEAVE_NOTIFY_MASK        |
                         GDK_BUTTON_PRESS_MASK        |
                         GDK_KEY_PRESS_MASK           |
                         GDK_POINTER_MOTION_MASK      |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_PROXIMITY_OUT_MASK);

  /* The following call enables tracking and processing of extension
   * events for the drawing area
   */
  gtk_widget_set_extension_events (water->area, GDK_EXTENSION_EVENTS_ALL);
  gtk_widget_grab_focus (water->area);

  adj = gtk_adjustment_new (200.0 - water->pressure_adjust * 100.0,
                            0.0, 200.0, 1.0, 1.0, 0.0);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (pressure_adjust_update),
                    water);

  scale = gtk_scale_new (GTK_ORIENTATION_VERTICAL, adj);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gimp_help_set_help_data (scale, _("Pressure"), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), scale, FALSE, FALSE, 0);

  gtk_widget_show_all (hbox);

  gimp_widget_track_monitor (GTK_WIDGET (water),
                             G_CALLBACK (colorsel_water_destroy_transform),
                             NULL);
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
colorsel_water_dispose (GObject *object)
{
  colorsel_water_set_config (GIMP_COLOR_SELECTOR (object), NULL);

  G_OBJECT_CLASS (colorsel_water_parent_class)->dispose (object);
}

static void
colorsel_water_set_config (GimpColorSelector *selector,
                           GimpColorConfig   *config)
{
  ColorselWater *water = COLORSEL_WATER (selector);

  if (config != water->config)
    {
      if (water->config)
        {
          g_signal_handlers_disconnect_by_func (water->config,
                                                colorsel_water_destroy_transform,
                                                water);
          g_object_unref (water->config);

          colorsel_water_destroy_transform (water);
        }

      water->config = config;

      if (water->config)
        {
          g_object_ref (water->config);

          g_signal_connect_swapped (water->config, "notify",
                                    G_CALLBACK (colorsel_water_destroy_transform),
                                    water);
        }
    }
}

static void
colorsel_water_create_transform (ColorselWater *water)
{
  if (water->config)
    {
      static GimpColorProfile *profile = NULL;

      const Babl *format = babl_format ("cairo-RGB24");

      if (G_UNLIKELY (! profile))
        profile = gimp_color_profile_new_rgb_srgb ();

      water->transform = gimp_widget_get_color_transform (water->area,
                                                          water->config,
                                                          profile,
                                                          format,
                                                          format);
    }
}

static void
colorsel_water_destroy_transform (ColorselWater *water)
{
  if (water->transform)
    {
      g_object_unref (water->transform);
      water->transform = NULL;
    }

  gtk_widget_queue_draw (GTK_WIDGET (water->area));
}

static gboolean
select_area_draw (GtkWidget     *widget,
                  cairo_t       *cr,
                  ColorselWater *water)
{
  GdkRectangle     area;
  GtkAllocation    allocation;
  gdouble          x1, y1, x2, y2;
  gdouble          dx;
  gdouble          dy;
  cairo_surface_t *surface;
  guchar          *dest;
  gdouble          y;
  gint             j;

  cairo_clip_extents (cr, &x1, &y1, &x2, &y2);

  area.x      = floor (x1);
  area.y      = floor (y1);
  area.width  = ceil (x2) - area.x;
  area.height = ceil (y2) - area.y;

  gtk_widget_get_allocation (widget, &allocation);

  dx = 1.0 / allocation.width;
  dy = 1.0 / allocation.height;

  surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
                                        area.width,
                                        area.height);

  dest = cairo_image_surface_get_data (surface);

  if (! water->transform)
    colorsel_water_create_transform (water);

  for (j = 0, y = area.y / allocation.height;
       j < area.height;
       j++, y += dy)
    {
      guchar  *d  = dest;

      gdouble  r  = calc (0, y, 0);
      gdouble  g  = calc (0, y, 120);
      gdouble  b  = calc (0, y, 240);

      gdouble  dr = calc (dx, y, 0)   - r;
      gdouble  dg = calc (dx, y, 120) - g;
      gdouble  db = calc (dx, y, 240) - b;

      gint     i;

      r += area.x * dr;
      g += area.x * dg;
      b += area.x * db;

      for (i = 0; i < area.width; i++)
        {
          GIMP_CAIRO_RGB24_SET_PIXEL (d,
                                      CLAMP ((gint) r, 0, 255),
                                      CLAMP ((gint) g, 0, 255),
                                      CLAMP ((gint) b, 0, 255));

          r += dr;
          g += dg;
          b += db;

          d += 4;
        }

      if (water->transform)
        gimp_color_transform_process_pixels (water->transform,
                                             babl_format ("cairo-RGB24"),
                                             dest,
                                             babl_format ("cairo-RGB24"),
                                             dest,
                                             area.width);

      dest += cairo_image_surface_get_stride (surface);
    }

  cairo_surface_mark_dirty (surface);
  cairo_set_source_surface (cr, surface, area.x, area.y);
  cairo_surface_destroy (surface);

  cairo_paint (cr);

  return FALSE;
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
  GtkAllocation allocation;
  gboolean      erase;

  gtk_widget_get_allocation (widget, &allocation);

  water->last_x = event->x / allocation.width;
  water->last_y = event->y / allocation.height;

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
  GtkAllocation  allocation;
  GdkTimeCoord **coords;
  gint           nevents;
  gint           i;
  gboolean       erase;

  gtk_widget_get_allocation (widget, &allocation);

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
                          x / allocation.width,
                          y / allocation.height, pressure);
            }

          g_free (coords);
        }
      else
        {
          gdouble pressure = 0.5;

          gdk_event_get_axis ((GdkEvent *) event, GDK_AXIS_PRESSURE, &pressure);

          draw_brush (water, widget, erase,
                      event->x / allocation.width,
                      event->y / allocation.height, pressure);
        }
    }

  /* Ask for more motion events in case the event was a hint */
  gdk_event_request_motions (event);

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
  water->pressure_adjust = (gtk_adjustment_get_upper (adj) -
                            gtk_adjustment_get_value (adj)) / 100.0;
}
