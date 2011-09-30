/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1999 Manish Singh <yosh@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


#define DEFAULT_CONTRAST 1.0


#define CDISPLAY_TYPE_CONTRAST            (cdisplay_contrast_get_type ())
#define CDISPLAY_CONTRAST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_CONTRAST, CdisplayContrast))
#define CDISPLAY_CONTRAST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_CONTRAST, CdisplayContrastClass))
#define CDISPLAY_IS_CONTRAST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_CONTRAST))
#define CDISPLAY_IS_CONTRAST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_CONTRAST))


typedef struct _CdisplayContrast      CdisplayContrast;
typedef struct _CdisplayContrastClass CdisplayContrastClass;

struct _CdisplayContrast
{
  GimpColorDisplay  parent_instance;

  gdouble           contrast;
  guchar            lookup[256];
};

struct _CdisplayContrastClass
{
  GimpColorDisplayClass  parent_instance;
};


enum
{
  PROP_0,
  PROP_CONTRAST
};


GType              cdisplay_contrast_get_type        (void);

static void        cdisplay_contrast_set_property    (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void        cdisplay_contrast_get_property    (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static void        cdisplay_contrast_convert_surface (GimpColorDisplay *display,
                                                      cairo_surface_t  *surface);
static GtkWidget * cdisplay_contrast_configure       (GimpColorDisplay *display);
static void        cdisplay_contrast_set_contrast    (CdisplayContrast *contrast,
                                                      gdouble           value);


static const GimpModuleInfo cdisplay_contrast_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("High Contrast color display filter"),
  "Jay Cox <jaycox@gimp.org>",
  "v0.2",
  "(c) 2000, released under the GPL",
  "October 14, 2000"
};

G_DEFINE_DYNAMIC_TYPE (CdisplayContrast, cdisplay_contrast,
                       GIMP_TYPE_COLOR_DISPLAY)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_contrast_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_contrast_register_type (module);

  return TRUE;
}

static void
cdisplay_contrast_class_init (CdisplayContrastClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  object_class->get_property     = cdisplay_contrast_get_property;
  object_class->set_property     = cdisplay_contrast_set_property;

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_CONTRAST,
                                   "contrast", NULL,
                                   0.01, 10.0, DEFAULT_CONTRAST,
                                   0);

  display_class->name            = _("Contrast");
  display_class->help_id         = "gimp-colordisplay-contrast";
  display_class->stock_id        = GIMP_STOCK_DISPLAY_FILTER_CONTRAST;

  display_class->convert_surface = cdisplay_contrast_convert_surface;
  display_class->configure       = cdisplay_contrast_configure;
}

static void
cdisplay_contrast_class_finalize (CdisplayContrastClass *klass)
{
}

static void
cdisplay_contrast_init (CdisplayContrast *contrast)
{
}

static void
cdisplay_contrast_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  CdisplayContrast *contrast = CDISPLAY_CONTRAST (object);

  switch (property_id)
    {
    case PROP_CONTRAST:
      g_value_set_double (value, contrast->contrast);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_contrast_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  CdisplayContrast *contrast = CDISPLAY_CONTRAST (object);

  switch (property_id)
    {
    case PROP_CONTRAST:
      cdisplay_contrast_set_contrast (contrast, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_contrast_convert_surface (GimpColorDisplay *display,
                                   cairo_surface_t  *surface)
{
  CdisplayContrast *contrast = CDISPLAY_CONTRAST (display);
  gint              width    = cairo_image_surface_get_width (surface);
  gint              height   = cairo_image_surface_get_height (surface);
  gint              stride   = cairo_image_surface_get_stride (surface);
  guchar           *buf      = cairo_image_surface_get_data (surface);
  cairo_format_t    fmt      = cairo_image_surface_get_format (surface);
  gint              i, j, skip;
  gint              r, g, b, a;

  if (fmt != CAIRO_FORMAT_ARGB32)
    return;

  /* You will not be using the entire buffer most of the time.
   * Hence, the simplistic code for this is as follows:
   *
   * for (j = 0; j < height; j++)
   *   {
   *     for (i = 0; i < width * bpp; i++)
   *       buf[i] = lookup[buf[i]];
   *     buf += bpl;
   *   }
   */

  j = height;
  skip = stride - 4 * width;

  while (j--)
    {
      i = width;
      while (i--)
        {
          GIMP_CAIRO_ARGB32_GET_PIXEL (buf, r, g, b, a);
          r = contrast->lookup[r];
          g = contrast->lookup[g];
          b = contrast->lookup[b];
          GIMP_CAIRO_ARGB32_SET_PIXEL (buf, r, g, b, a);
          buf += 4;
        }
      buf += skip;
    }
}

static GtkWidget *
cdisplay_contrast_configure (GimpColorDisplay *display)
{
  CdisplayContrast *contrast = CDISPLAY_CONTRAST (display);
  GtkWidget        *hbox;
  GtkWidget        *label;
  GtkWidget        *spinbutton;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  label = gtk_label_new_with_mnemonic (_("Contrast c_ycles:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_prop_spin_button_new (G_OBJECT (contrast), "contrast",
                                          0.1, 1.0, 3);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  return hbox;
}

static void
cdisplay_contrast_set_contrast (CdisplayContrast *contrast,
                                gdouble           value)
{
  if (value <= 0.0)
    value = 1.0;

  if (value != contrast->contrast)
    {
      gint i;

      contrast->contrast = value;

      for (i = 0; i < 256; i++)
        {
          contrast->lookup[i] = (guchar) (gint)
            (255 * .5 * (1 + sin (value * 2 * G_PI * i / 255.0)));
        }

      g_object_notify (G_OBJECT (contrast), "contrast");
      gimp_color_display_changed (GIMP_COLOR_DISPLAY (contrast));
    }
}
