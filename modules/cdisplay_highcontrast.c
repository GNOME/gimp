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
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/gimpmodule.h"

#include "libgimp/gimpintl.h"


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

  GFunc             ok_func;
  gpointer          ok_data;
  GFunc             cancel_func;
  gpointer          cancel_data;

  gdouble           contrast;
  guchar           *lookup;

  GtkWidget        *shell;
  GtkWidget        *spinner;
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
static void    cdisplay_contrast_configure         (GimpColorDisplay *display,
                                                 GFunc             ok_func,
                                                 gpointer          ok_data,
                                                 GFunc             cancel_func,
                                                 gpointer          cancel_data);
static void    cdisplay_contrast_configure_cancel  (GimpColorDisplay *display);

static void    contrast_create_lookup_table        (CdisplayContrast    *contrast);
static void    contrast_configure_ok_callback      (GtkWidget        *widget,
                                                 CdisplayContrast    *contrast);
static void    contrast_configure_cancel_callback  (GtkWidget        *widget,
                                                 CdisplayContrast    *contrast);


static GimpModuleInfo cdisplay_contrast_info = 
{
  N_("High Contrast color display filter"),
  "Jay Cox <jaycox@earthlink.net>",
  "v0.2",
  "(c) 2000, released under the GPL",
  "October 14, 2000"
};

static GType                  cdisplay_contrast_type = 0;
static GimpColorDisplayClass *parent_class        = NULL;


G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule     *module,
                      GimpModuleInfo **inforet)
{
  cdisplay_contrast_get_type (module);

  if (inforet)
    *inforet = &cdisplay_contrast_info;

  return TRUE;
}

static GType
cdisplay_contrast_get_type (GTypeModule *module)
{
  if (! cdisplay_contrast_type)
    {
      static const GTypeInfo select_info =
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
                                     &select_info, 0);
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

  object_class->finalize          = cdisplay_contrast_finalize;

  display_class->name             = _("Contrast");
  display_class->help_page        = "modules/contrast.html";
  display_class->clone            = cdisplay_contrast_clone;
  display_class->convert          = cdisplay_contrast_convert;
  display_class->load_state       = cdisplay_contrast_load_state;
  display_class->save_state       = cdisplay_contrast_save_state;
  display_class->configure        = cdisplay_contrast_configure;
  display_class->configure_cancel = cdisplay_contrast_configure_cancel;
}

static void
cdisplay_contrast_init (CdisplayContrast *contrast)
{
  contrast->contrast  = 4.0;
  contrast->lookup    = g_new (guchar, 256);

  contrast_create_lookup_table (contrast);
}

static void
cdisplay_contrast_finalize (GObject *object)
{
  CdisplayContrast *contrast;

  contrast = CDISPLAY_CONTRAST (object);

  if (contrast->shell)
    {
      gtk_widget_destroy (contrast->shell);
      contrast->shell = NULL;
    }

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
    guint32 buf[2], *data = gimp_parasite_data (state);

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

static void
cdisplay_contrast_configure (GimpColorDisplay *display,
                             GFunc             ok_func,
                             gpointer          ok_data,
                             GFunc             cancel_func,
                             gpointer          cancel_data)
{
  CdisplayContrast *contrast;
  GtkWidget        *hbox;
  GtkWidget        *label;
  GtkObject        *adjustment;

  contrast = CDISPLAY_CONTRAST (display);

  if (!contrast->shell)
    {
      contrast->ok_func     = ok_func;
      contrast->ok_data     = ok_data;
      contrast->cancel_func = cancel_func;
      contrast->cancel_data = cancel_data;

      contrast->shell =
	gimp_dialog_new (_("High Contrast"), "high_contrast",
			 gimp_standard_help_func, "modules/highcontrast.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 GTK_STOCK_CANCEL, contrast_configure_cancel_callback,
			 contrast, NULL, NULL, FALSE, TRUE,

			 GTK_STOCK_OK, contrast_configure_ok_callback,
			 contrast, NULL, NULL, TRUE, FALSE,

			 NULL);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (contrast->shell)->vbox),
			  hbox, FALSE, FALSE, 0);

      label = gtk_label_new ( _("Contrast Cycles:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

      adjustment = gtk_adjustment_new (contrast->contrast, 0.01, 10.0, 0.01, 0.1, 0.0);
      contrast->spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment),
					      0.1, 3);
      gtk_box_pack_start (GTK_BOX (hbox), contrast->spinner, FALSE, FALSE, 0);
    }

  gtk_widget_show_all (contrast->shell);
}

static void
cdisplay_contrast_configure_cancel (GimpColorDisplay *display)
{
  CdisplayContrast *contrast;
 
  contrast = CDISPLAY_CONTRAST (display);

  if (contrast->shell)
    {
      gtk_widget_destroy (contrast->shell);
      contrast->shell = NULL;
    }

  if (contrast->cancel_func)
    contrast->cancel_func (contrast, contrast->cancel_data);
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
contrast_configure_ok_callback (GtkWidget        *widget,
                                CdisplayContrast *contrast)
{
  contrast->contrast =
    gtk_spin_button_get_value (GTK_SPIN_BUTTON (contrast->spinner));

  contrast_create_lookup_table (contrast);

  gtk_widget_destroy (GTK_WIDGET (contrast->shell));
  contrast->shell = NULL;

  if (contrast->ok_func)
    contrast->ok_func (contrast, contrast->ok_data);
}

static void
contrast_configure_cancel_callback (GtkWidget        *widget,
                                    CdisplayContrast *contrast)
{
  gimp_color_display_configure_cancel (GIMP_COLOR_DISPLAY (contrast));
}
