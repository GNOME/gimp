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


#define DEFAULT_CONTRAST 4.0


#define CDISPLAY_TYPE_CONTRAST            (cdisplay_contrast_type)
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
  guchar           *lookup;

  GtkWidget        *hbox;
  GtkObject        *adjustment;
};

struct _CdisplayContrastClass
{
  GimpColorDisplayClass  parent_instance;
};


static GType   cdisplay_contrast_get_type   (GTypeModule        *module);
static void    cdisplay_contrast_class_init (CdisplayContrastClass *klass);
static void    cdisplay_contrast_init       (CdisplayContrast      *contrast);

static void    cdisplay_contrast_finalize          (GObject           *object);

static GimpColorDisplay * cdisplay_contrast_clone  (GimpColorDisplay *display);
static void    cdisplay_contrast_convert           (GimpColorDisplay *display,
                                                 guchar           *buf,
                                                 gint              w,
                                                 gint              h,
                                                 gint              bpp,
                                                 gint              bpl);
static void    cdisplay_contrast_load_state        (GimpColorDisplay *display,
                                                 GimpParasite     *state);
static GimpParasite * cdisplay_contrast_save_state (GimpColorDisplay *display);
static GtkWidget    * cdisplay_contrast_configure  (GimpColorDisplay *display);
static void    cdisplay_contrast_configure_reset   (GimpColorDisplay *display);

static void    contrast_create_lookup_table        (CdisplayContrast    *contrast);
static void    contrast_configure_adj_callback     (GtkAdjustment    *adj,
                                                    CdisplayContrast    *contrast);


static const GimpModuleInfo cdisplay_contrast_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("High Contrast color display filter"),
  "Jay Cox <jaycox@earthlink.net>",
  "v0.2",
  "(c) 2000, released under the GPL",
  "October 14, 2000"
};

static GType                  cdisplay_contrast_type = 0;
static GimpColorDisplayClass *parent_class        = NULL;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_contrast_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_contrast_get_type (module);

  return TRUE;
}

static GType
cdisplay_contrast_get_type (GTypeModule *module)
{
  if (! cdisplay_contrast_type)
    {
      static const GTypeInfo display_info =
      {
        sizeof (CdisplayContrastClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) cdisplay_contrast_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (CdisplayContrast),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) cdisplay_contrast_init,
      };

      cdisplay_contrast_type =
        g_type_module_register_type (module,
                                     GIMP_TYPE_COLOR_DISPLAY,
                                     "CdisplayContrast",
                                     &display_info, 0);
    }

  return cdisplay_contrast_type;
}

static void
cdisplay_contrast_class_init (CdisplayContrastClass *klass)
{
  GObjectClass          *object_class;
  GimpColorDisplayClass *display_class;

  object_class  = G_OBJECT_CLASS (klass);
  display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize         = cdisplay_contrast_finalize;

  display_class->name            = _("Contrast");
  display_class->help_id         = "gimp-colordisplay-contrast";
  display_class->clone           = cdisplay_contrast_clone;
  display_class->convert         = cdisplay_contrast_convert;
  display_class->load_state      = cdisplay_contrast_load_state;
  display_class->save_state      = cdisplay_contrast_save_state;
  display_class->configure       = cdisplay_contrast_configure;
  display_class->configure_reset = cdisplay_contrast_configure_reset;
}

static void
cdisplay_contrast_init (CdisplayContrast *contrast)
{
  contrast->contrast  = DEFAULT_CONTRAST;
  contrast->lookup    = g_new (guchar, 256);

  contrast_create_lookup_table (contrast);
}

static void
cdisplay_contrast_finalize (GObject *object)
{
  CdisplayContrast *contrast;

  contrast = CDISPLAY_CONTRAST (object);

  if (contrast->hbox)
    gtk_widget_destroy (contrast->hbox);

  if (contrast->lookup)
    {
      g_free (contrast->lookup);
      contrast->lookup = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GimpColorDisplay *
cdisplay_contrast_clone (GimpColorDisplay *display)
{
  CdisplayContrast *contrast;
  CdisplayContrast *copy;

  contrast = CDISPLAY_CONTRAST (display);

  copy = CDISPLAY_CONTRAST (gimp_color_display_new (G_TYPE_FROM_INSTANCE (contrast)));

  copy->contrast = contrast->contrast;

  memcpy (copy->lookup, contrast->lookup, sizeof (guchar) * 256);

  return GIMP_COLOR_DISPLAY (copy);
}

static void
cdisplay_contrast_convert (GimpColorDisplay *display,
                           guchar           *buf,
                           gint              width,
                           gint              height,
                           gint              bpp,
                           gint              bpl)
{
  CdisplayContrast *contrast;
  gint              i, j   = height;

  contrast = CDISPLAY_CONTRAST (display);

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
	  *buf = contrast->lookup[*buf];
	  buf++;
	}
      buf += bpl;
    }
}

static void
cdisplay_contrast_load_state (GimpColorDisplay *display,
                              GimpParasite     *state)
{
  CdisplayContrast *contrast;

  contrast = CDISPLAY_CONTRAST (display);

#if G_BYTE_ORDER == G_BIG_ENDIAN
  memcpy (&contrast->contrast, gimp_parasite_data (state), sizeof (gdouble));
#else
  {
    guint32        buf[2];
    const guint32 *data;

    data = gimp_parasite_data (state);

    buf[0] = g_ntohl (data[1]);
    buf[1] = g_ntohl (data[0]);

    memcpy (&contrast->contrast, buf, sizeof (gdouble));
  }
#endif

  contrast_create_lookup_table (contrast);
}

static GimpParasite *
cdisplay_contrast_save_state (GimpColorDisplay *display)
{
  CdisplayContrast *contrast;
  guint32           buf[2];

  contrast = CDISPLAY_CONTRAST (display);

  memcpy (buf, &contrast->contrast, sizeof (double));

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  {
    guint32 tmp;

    tmp    = g_htonl (buf[0]);
    buf[0] = g_htonl (buf[1]);
    buf[1] = tmp;
  }
#endif

  return gimp_parasite_new ("Display/Contrast", GIMP_PARASITE_PERSISTENT,
			    sizeof (double), &buf);
}

static GtkWidget *
cdisplay_contrast_configure (GimpColorDisplay *display)
{
  CdisplayContrast *contrast;
  GtkWidget        *label;
  GtkWidget        *spinbutton;

  contrast = CDISPLAY_CONTRAST (display);

  if (contrast->hbox)
    gtk_widget_destroy (contrast->hbox);

  contrast->hbox = gtk_hbox_new (FALSE, 2);
  g_object_add_weak_pointer (G_OBJECT (contrast->hbox),
                             (gpointer) &contrast->hbox);

  label = gtk_label_new ( _("Contrast Cycles:"));
  gtk_box_pack_start (GTK_BOX (contrast->hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&contrast->adjustment,
                                     contrast->contrast,
                                     0.01, 10.0, 0.01, 0.1, 0.0,
                                     0.1, 3);
  gtk_box_pack_start (GTK_BOX (contrast->hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (contrast->adjustment, "value_changed",
                    G_CALLBACK (contrast_configure_adj_callback),
                    contrast);

  return contrast->hbox;
}

static void
cdisplay_contrast_configure_reset (GimpColorDisplay *display)
{
  CdisplayContrast *contrast;

  contrast = CDISPLAY_CONTRAST (display);

  if (contrast->adjustment)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (contrast->adjustment),
                              DEFAULT_CONTRAST);
}

static void
contrast_create_lookup_table (CdisplayContrast *contrast)
{
  gint    i;

  if (contrast->contrast == 0.0)
    contrast->contrast = 1.0;

  for (i = 0; i < 256; i++)
    {
      contrast->lookup[i] = (guchar) (gint)
        (255 * .5 * (1 + sin (contrast->contrast * 2 * G_PI * i / 255.0)));
    }
}

static void
contrast_configure_adj_callback (GtkAdjustment    *adj,
                                 CdisplayContrast *contrast)
{
  contrast->contrast = adj->value;

  contrast_create_lookup_table (contrast);

  gimp_color_display_changed (GIMP_COLOR_DISPLAY (contrast));
}
