/* GIMP Wheel ColorSelector
 * Copyright (C) 2008  Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gimpcolorwheel.h"

#include "libgimp/libgimp-intl.h"


#define COLORSEL_TYPE_WHEEL            (colorsel_wheel_get_type ())
#define COLORSEL_WHEEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLORSEL_TYPE_WHEEL, ColorselWheel))
#define COLORSEL_WHEEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), COLORSEL_TYPE_WHEEL, ColorselWheelClass))
#define COLORSEL_IS_WHEEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLORSEL_TYPE_WHEEL))
#define COLORSEL_IS_WHEEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), COLORSEL_TYPE_WHEEL))


typedef struct _ColorselWheel      ColorselWheel;
typedef struct _ColorselWheelClass ColorselWheelClass;

struct _ColorselWheel
{
  GimpColorSelector  parent_instance;

  GtkWidget         *hsv;
  const Babl        *format;
};

struct _ColorselWheelClass
{
  GimpColorSelectorClass  parent_class;
};


GType         colorsel_wheel_get_type      (void);

static void   colorsel_wheel_set_color     (GimpColorSelector *selector,
                                            GeglColor         *color);
static void   colorsel_wheel_set_format    (GimpColorSelector *selector,
                                            const Babl        *format);
static void   colorsel_wheel_set_config    (GimpColorSelector *selector,
                                            GimpColorConfig   *config);
static void   colorsel_wheel_changed       (GimpColorWheel    *hsv,
                                            GimpColorSelector *selector);

static const GimpModuleInfo colorsel_wheel_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("HSV color wheel"),
  "Michael Natterer <mitch@gimp.org>",
  "v1.0",
  "(c) 2008, released under the GPL",
  "08 Aug 2008"
};


G_DEFINE_DYNAMIC_TYPE (ColorselWheel, colorsel_wheel,
                       GIMP_TYPE_COLOR_SELECTOR)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &colorsel_wheel_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  color_wheel_register_type (module);
  colorsel_wheel_register_type (module);

  return TRUE;
}

static void
colorsel_wheel_class_init (ColorselWheelClass *klass)
{
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  selector_class->name       = _("Wheel");
  selector_class->help_id    = "gimp-colorselector-triangle";
  selector_class->icon_name  = GIMP_ICON_COLOR_SELECTOR_TRIANGLE;
  selector_class->set_color  = colorsel_wheel_set_color;
  selector_class->set_config = colorsel_wheel_set_config;
  selector_class->set_format = colorsel_wheel_set_format;

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "ColorselWheel");
}

static void
colorsel_wheel_class_finalize (ColorselWheelClass *klass)
{
}

static void
colorsel_wheel_init (ColorselWheel *wheel)
{
  wheel->hsv = gimp_color_wheel_new ();
  g_object_add_weak_pointer (G_OBJECT (wheel->hsv),
                             (gpointer) &wheel->hsv);
  gtk_box_pack_start (GTK_BOX (wheel), wheel->hsv, TRUE, TRUE, 0);
  gtk_widget_show (wheel->hsv);

  g_signal_connect (wheel->hsv, "changed",
                    G_CALLBACK (colorsel_wheel_changed),
                    wheel);
}

static void
colorsel_wheel_set_color (GimpColorSelector *selector,
                          GeglColor         *color)
{
  ColorselWheel *wheel = COLORSEL_WHEEL (selector);
  gdouble        hsv[3];

  gegl_color_get_pixel (color, babl_format_with_space ("HSV double", wheel->format), hsv);
  g_signal_handlers_block_by_func (wheel->hsv,
                                   G_CALLBACK (colorsel_wheel_changed),
                                   wheel);
  gimp_color_wheel_set_color (GIMP_COLOR_WHEEL (wheel->hsv), hsv[0], hsv[1], hsv[2]);
  g_signal_handlers_unblock_by_func (wheel->hsv,
                                     G_CALLBACK (colorsel_wheel_changed),
                                     wheel);
}

static void
colorsel_wheel_set_format (GimpColorSelector *selector,
                           const Babl        *format)
{
  ColorselWheel *wheel = COLORSEL_WHEEL (selector);

  if (wheel->format != format)
    {
      wheel->format = format;
      gimp_color_wheel_set_format (GIMP_COLOR_WHEEL (wheel->hsv), format);
    }
}

static void
colorsel_wheel_set_config (GimpColorSelector *selector,
                           GimpColorConfig   *config)
{
  ColorselWheel *wheel = COLORSEL_WHEEL (selector);

  if (wheel->hsv)
    gimp_color_wheel_set_color_config (GIMP_COLOR_WHEEL (wheel->hsv), config);
}

static void
colorsel_wheel_changed (GimpColorWheel    *wheel,
                        GimpColorSelector *selector)
{
  GeglColor *color = gegl_color_new (NULL);
  gdouble    hsv[3];

  gimp_color_wheel_get_color (wheel, &hsv[0], &hsv[1], &hsv[2]);
  gegl_color_set_pixel (color, babl_format_with_space ("HSV double",
                                                       COLORSEL_WHEEL (selector)->format),
                        hsv);
  gimp_color_selector_set_color (selector, color);
  g_object_unref (color);
}
