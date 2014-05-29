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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


#define DEFAULT_GAMMA 1.0


#define CDISPLAY_TYPE_GAMMA            (cdisplay_gamma_get_type ())
#define CDISPLAY_GAMMA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_GAMMA, CdisplayGamma))
#define CDISPLAY_GAMMA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_GAMMA, CdisplayGammaClass))
#define CDISPLAY_IS_GAMMA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_GAMMA))
#define CDISPLAY_IS_GAMMA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_GAMMA))


typedef struct _CdisplayGamma      CdisplayGamma;
typedef struct _CdisplayGammaClass CdisplayGammaClass;

struct _CdisplayGamma
{
  GimpColorDisplay  parent_instance;

  gdouble           gamma;
};

struct _CdisplayGammaClass
{
  GimpColorDisplayClass  parent_instance;
};


enum
{
  PROP_0,
  PROP_GAMMA
};


static GType       cdisplay_gamma_get_type        (void);

static void        cdisplay_gamma_set_property    (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void        cdisplay_gamma_get_property    (GObject            *object,
                                                   guint               property_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);

static void        cdisplay_gamma_convert_buffer  (GimpColorDisplay   *display,
                                                   GeglBuffer         *buffer,
                                                   GeglRectangle      *area);
static GtkWidget * cdisplay_gamma_configure       (GimpColorDisplay   *display);
static void        cdisplay_gamma_set_gamma       (CdisplayGamma      *gamma,
                                                   gdouble             value);


static const GimpModuleInfo cdisplay_gamma_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Gamma color display filter"),
  "Manish Singh <yosh@gimp.org>",
  "v0.2",
  "(c) 1999, released under the GPL",
  "October 14, 2000"
};


G_DEFINE_DYNAMIC_TYPE (CdisplayGamma, cdisplay_gamma,
                       GIMP_TYPE_COLOR_DISPLAY)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_gamma_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_gamma_register_type (module);

  return TRUE;
}

static void
cdisplay_gamma_class_init (CdisplayGammaClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  object_class->get_property     = cdisplay_gamma_get_property;
  object_class->set_property     = cdisplay_gamma_set_property;

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_GAMMA,
                                   "gamma", NULL,
                                   0.01, 10.0, DEFAULT_GAMMA,
                                   0);

  display_class->name            = _("Gamma");
  display_class->help_id         = "gimp-colordisplay-gamma";
  display_class->icon_name       = GIMP_STOCK_DISPLAY_FILTER_GAMMA;

  display_class->convert_buffer  = cdisplay_gamma_convert_buffer;
  display_class->configure       = cdisplay_gamma_configure;
}

static void
cdisplay_gamma_class_finalize (CdisplayGammaClass *klass)
{
}

static void
cdisplay_gamma_init (CdisplayGamma *gamma)
{
}

static void
cdisplay_gamma_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CdisplayGamma *gamma = CDISPLAY_GAMMA (object);

  switch (property_id)
    {
    case PROP_GAMMA:
      g_value_set_double (value, gamma->gamma);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_gamma_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CdisplayGamma *gamma = CDISPLAY_GAMMA (object);

  switch (property_id)
    {
    case PROP_GAMMA:
      cdisplay_gamma_set_gamma (gamma, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_gamma_convert_buffer (GimpColorDisplay *display,
                               GeglBuffer       *buffer,
                               GeglRectangle    *area)
{
  CdisplayGamma      *gamma = CDISPLAY_GAMMA (display);
  GeglBufferIterator *iter;
  gdouble             one_over_gamma;

  one_over_gamma = 1.0 / gamma->gamma;

  iter = gegl_buffer_iterator_new (buffer, area, 0,
                                   babl_format ("R'G'B'A float"),
                                   GEGL_BUFFER_READWRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *data  = iter->data[0];
      gint    count = iter->length;

      while (count--)
        {
          *data = pow (*data, one_over_gamma); data++;
          *data = pow (*data, one_over_gamma); data++;
          *data = pow (*data, one_over_gamma); data++;

          data++;
        }
    }
}

static GtkWidget *
cdisplay_gamma_configure (GimpColorDisplay *display)
{
  CdisplayGamma *gamma = CDISPLAY_GAMMA (display);
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *spinbutton;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  label = gtk_label_new_with_mnemonic (_("_Gamma:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_prop_spin_button_new (G_OBJECT (gamma), "gamma",
                                          0.1, 1.0, 3);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  return hbox;
}

static void
cdisplay_gamma_set_gamma (CdisplayGamma *gamma,
                          gdouble        value)
{
  if (value <= 0.0)
    value = 1.0;

  if (value != gamma->gamma)
    {
      gamma->gamma = value;

      g_object_notify (G_OBJECT (gamma), "gamma");
      gimp_color_display_changed (GIMP_COLOR_DISPLAY (gamma));
    }
}
