/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Manish Singh <yosh@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


#define DEFAULT_GAMMA 1.0


#define CDISPLAY_TYPE_GAMMA            (cdisplay_gamma_type)
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
  guchar           *lookup;

  GtkWidget        *hbox;
  GtkObject        *adjustment;
};

struct _CdisplayGammaClass
{
  GimpColorDisplayClass  parent_instance;
};


static GType   cdisplay_gamma_get_type   (GTypeModule        *module);
static void    cdisplay_gamma_class_init (CdisplayGammaClass *klass);
static void    cdisplay_gamma_init       (CdisplayGamma      *gamma);

static void    cdisplay_gamma_finalize          (GObject           *object);

static GimpColorDisplay * cdisplay_gamma_clone  (GimpColorDisplay *display);
static void    cdisplay_gamma_convert           (GimpColorDisplay *display,
                                                 guchar           *buf,
                                                 gint              w,
                                                 gint              h,
                                                 gint              bpp,
                                                 gint              bpl);
static void    cdisplay_gamma_load_state        (GimpColorDisplay *display,
                                                 GimpParasite     *state);
static GimpParasite * cdisplay_gamma_save_state (GimpColorDisplay *display);
static GtkWidget    * cdisplay_gamma_configure  (GimpColorDisplay *display);
static void    cdisplay_gamma_configure_reset   (GimpColorDisplay *display);

static void    gamma_create_lookup_table        (CdisplayGamma    *gamma);
static void    gamma_configure_adj_callback     (GtkAdjustment    *adj,
                                                 CdisplayGamma    *gamma);


static const GimpModuleInfo cdisplay_gamma_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Gamma color display filter"),
  "Manish Singh <yosh@gimp.org>",
  "v0.2",
  "(c) 1999, released under the GPL",
  "October 14, 2000"
};

static GType                  cdisplay_gamma_type = 0;
static GimpColorDisplayClass *parent_class        = NULL;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_gamma_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_gamma_get_type (module);

  return TRUE;
}

static GType
cdisplay_gamma_get_type (GTypeModule *module)
{
  if (! cdisplay_gamma_type)
    {
      static const GTypeInfo display_info =
      {
        sizeof (CdisplayGammaClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) cdisplay_gamma_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (CdisplayGamma),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) cdisplay_gamma_init,
      };

      cdisplay_gamma_type =
        g_type_module_register_type (module,
                                     GIMP_TYPE_COLOR_DISPLAY,
                                     "CdisplayGamma",
                                     &display_info, 0);
    }

  return cdisplay_gamma_type;
}

static void
cdisplay_gamma_class_init (CdisplayGammaClass *klass)
{
  GObjectClass          *object_class;
  GimpColorDisplayClass *display_class;

  object_class  = G_OBJECT_CLASS (klass);
  display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize         = cdisplay_gamma_finalize;

  display_class->name            = _("Gamma");
  display_class->help_id         = "gimp-colordisplay-gamma";
  display_class->clone           = cdisplay_gamma_clone;
  display_class->convert         = cdisplay_gamma_convert;
  display_class->load_state      = cdisplay_gamma_load_state;
  display_class->save_state      = cdisplay_gamma_save_state;
  display_class->configure       = cdisplay_gamma_configure;
  display_class->configure_reset = cdisplay_gamma_configure_reset;
}

static void
cdisplay_gamma_init (CdisplayGamma *gamma)
{
  gint i;

  gamma->gamma  = DEFAULT_GAMMA;
  gamma->lookup = g_new (guchar, 256);

  for (i = 0; i < 256; i++)
    gamma->lookup[i] = i;
}

static void
cdisplay_gamma_finalize (GObject *object)
{
  CdisplayGamma *gamma = CDISPLAY_GAMMA (object);

  if (gamma->hbox)
    gtk_widget_destroy (gamma->hbox);

  if (gamma->lookup)
    {
      g_free (gamma->lookup);
      gamma->lookup = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GimpColorDisplay *
cdisplay_gamma_clone (GimpColorDisplay *display)
{
  CdisplayGamma *gamma = CDISPLAY_GAMMA (display);
  CdisplayGamma *copy;

  copy = CDISPLAY_GAMMA (gimp_color_display_new (G_TYPE_FROM_INSTANCE (gamma)));

  copy->gamma = gamma->gamma;

  memcpy (copy->lookup, gamma->lookup, sizeof (guchar) * 256);

  return GIMP_COLOR_DISPLAY (copy);
}

static void
cdisplay_gamma_convert (GimpColorDisplay *display,
                        guchar           *buf,
                        gint              width,
                        gint              height,
                        gint              bpp,
                        gint              bpl)
{
  CdisplayGamma *gamma = CDISPLAY_GAMMA (display);
  gint           i, j  = height;

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

  width *= bpp;
  bpl -= width;

  while (j--)
    {
      i = width;
      while (i--)
	{
	  *buf = gamma->lookup[*buf];
	  buf++;
	}
      buf += bpl;
    }
}

static void
cdisplay_gamma_load_state (GimpColorDisplay *display,
                           GimpParasite     *state)
{
  CdisplayGamma *gamma = CDISPLAY_GAMMA (display);

#if G_BYTE_ORDER == G_BIG_ENDIAN
  memcpy (&gamma->gamma, gimp_parasite_data (state), sizeof (gdouble));
#else
  {
    guint32        buf[2];
    const guint32 *data;

    data = gimp_parasite_data (state);

    buf[0] = g_ntohl (data[1]);
    buf[1] = g_ntohl (data[0]);

    memcpy (&gamma->gamma, buf, sizeof (gdouble));
  }
#endif

  gamma_create_lookup_table (gamma);
}

static GimpParasite *
cdisplay_gamma_save_state (GimpColorDisplay *display)
{
  CdisplayGamma *gamma = CDISPLAY_GAMMA (display);
  guint32        buf[2];

  memcpy (buf, &gamma->gamma, sizeof (double));

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  {
    guint32 tmp;

    tmp    = g_htonl (buf[0]);
    buf[0] = g_htonl (buf[1]);
    buf[1] = tmp;
  }
#endif

  return gimp_parasite_new ("Display/Gamma", GIMP_PARASITE_PERSISTENT,
			    sizeof (double), &buf);
}

static GtkWidget *
cdisplay_gamma_configure (GimpColorDisplay *display)
{
  CdisplayGamma *gamma = CDISPLAY_GAMMA (display);
  GtkWidget     *label;
  GtkWidget     *spinbutton;

  if (gamma->hbox)
    gtk_widget_destroy (gamma->hbox);

  gamma->hbox = gtk_hbox_new (FALSE, 6);

  g_signal_connect (gamma->hbox, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &gamma->hbox);

  label = gtk_label_new_with_mnemonic (_("_Gamma:"));
  gtk_box_pack_start (GTK_BOX (gamma->hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&gamma->adjustment,
                                     gamma->gamma, 0.01, 10.0, 0.01, 0.1, 0.0,
                                     0.1, 3);
  gtk_box_pack_start (GTK_BOX (gamma->hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  g_signal_connect (gamma->adjustment, "value_changed",
                    G_CALLBACK (gamma_configure_adj_callback),
                    gamma);

  return gamma->hbox;
}

static void
cdisplay_gamma_configure_reset (GimpColorDisplay *display)
{
  CdisplayGamma *gamma = CDISPLAY_GAMMA (display);

  if (gamma->adjustment)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (gamma->adjustment), DEFAULT_GAMMA);
}

static void
gamma_create_lookup_table (CdisplayGamma *gamma)
{
  gdouble one_over_gamma;
  gdouble ind;
  gint    i;

  if (gamma->gamma == 0.0)
    gamma->gamma = 1.0;

  one_over_gamma = 1.0 / gamma->gamma;

  for (i = 0; i < 256; i++)
    {
      ind = (gdouble) i / 255.0;
      gamma->lookup[i] = (guchar) (gint) (255 * pow (ind, one_over_gamma));
    }
}

static void
gamma_configure_adj_callback (GtkAdjustment *adj,
                              CdisplayGamma *gamma)
{
  gamma->gamma = adj->value;

  gamma_create_lookup_table (gamma);

  gimp_color_display_changed (GIMP_COLOR_DISPLAY (gamma));
}
