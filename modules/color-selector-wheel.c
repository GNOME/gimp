/* LIGMA Wheel ColorSelector
 * Copyright (C) 2008  Michael Natterer <mitch@ligma.org>
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

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmamodule/ligmamodule.h"
#include "libligmawidgets/ligmawidgets.h"

#include "ligmacolorwheel.h"

#include "libligma/libligma-intl.h"


#define COLORSEL_TYPE_WHEEL            (colorsel_wheel_get_type ())
#define COLORSEL_WHEEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLORSEL_TYPE_WHEEL, ColorselWheel))
#define COLORSEL_WHEEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), COLORSEL_TYPE_WHEEL, ColorselWheelClass))
#define COLORSEL_IS_WHEEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COLORSEL_TYPE_WHEEL))
#define COLORSEL_IS_WHEEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), COLORSEL_TYPE_WHEEL))


typedef struct _ColorselWheel      ColorselWheel;
typedef struct _ColorselWheelClass ColorselWheelClass;

struct _ColorselWheel
{
  LigmaColorSelector  parent_instance;

  GtkWidget         *hsv;
};

struct _ColorselWheelClass
{
  LigmaColorSelectorClass  parent_class;
};


static GType  colorsel_wheel_get_type      (void);

static void   colorsel_wheel_set_color     (LigmaColorSelector *selector,
                                            const LigmaRGB     *rgb,
                                            const LigmaHSV     *hsv);
static void   colorsel_wheel_set_config    (LigmaColorSelector *selector,
                                            LigmaColorConfig   *config);
static void   colorsel_wheel_changed       (LigmaColorWheel    *hsv,
                                            LigmaColorSelector *selector);

static const LigmaModuleInfo colorsel_wheel_info =
{
  LIGMA_MODULE_ABI_VERSION,
  N_("HSV color wheel"),
  "Michael Natterer <mitch@ligma.org>",
  "v1.0",
  "(c) 2008, released under the GPL",
  "08 Aug 2008"
};


G_DEFINE_DYNAMIC_TYPE (ColorselWheel, colorsel_wheel,
                       LIGMA_TYPE_COLOR_SELECTOR)


G_MODULE_EXPORT const LigmaModuleInfo *
ligma_module_query (GTypeModule *module)
{
  return &colorsel_wheel_info;
}

G_MODULE_EXPORT gboolean
ligma_module_register (GTypeModule *module)
{
  color_wheel_register_type (module);
  colorsel_wheel_register_type (module);

  return TRUE;
}

static void
colorsel_wheel_class_init (ColorselWheelClass *klass)
{
  LigmaColorSelectorClass *selector_class = LIGMA_COLOR_SELECTOR_CLASS (klass);

  selector_class->name       = _("Wheel");
  selector_class->help_id    = "ligma-colorselector-triangle";
  selector_class->icon_name  = LIGMA_ICON_COLOR_SELECTOR_TRIANGLE;
  selector_class->set_color  = colorsel_wheel_set_color;
  selector_class->set_config = colorsel_wheel_set_config;

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "ColorselWheel");
}

static void
colorsel_wheel_class_finalize (ColorselWheelClass *klass)
{
}

static void
colorsel_wheel_init (ColorselWheel *wheel)
{
  wheel->hsv = ligma_color_wheel_new ();
  g_object_add_weak_pointer (G_OBJECT (wheel->hsv),
                             (gpointer) &wheel->hsv);
  gtk_box_pack_start (GTK_BOX (wheel), wheel->hsv, TRUE, TRUE, 0);
  gtk_widget_show (wheel->hsv);

  g_signal_connect (wheel->hsv, "changed",
                    G_CALLBACK (colorsel_wheel_changed),
                    wheel);
}

static void
colorsel_wheel_set_color (LigmaColorSelector *selector,
                          const LigmaRGB     *rgb,
                          const LigmaHSV     *hsv)
{
  ColorselWheel *wheel = COLORSEL_WHEEL (selector);

  ligma_color_wheel_set_color (LIGMA_COLOR_WHEEL (wheel->hsv),
                              hsv->h, hsv->s, hsv->v);
}

static void
colorsel_wheel_set_config (LigmaColorSelector *selector,
                           LigmaColorConfig   *config)
{
  ColorselWheel *wheel = COLORSEL_WHEEL (selector);

  if (wheel->hsv)
    ligma_color_wheel_set_color_config (LIGMA_COLOR_WHEEL (wheel->hsv), config);
}

static void
colorsel_wheel_changed (LigmaColorWheel    *hsv,
                        LigmaColorSelector *selector)
{
  ligma_color_wheel_get_color (hsv,
                              &selector->hsv.h,
                              &selector->hsv.s,
                              &selector->hsv.v);
  ligma_hsv_to_rgb (&selector->hsv, &selector->rgb);

  ligma_color_selector_emit_color_changed (selector);
}
