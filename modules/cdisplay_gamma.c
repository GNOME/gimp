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

#include "app/dialog_handler.h"

#include "gimpmodregister.h"

#include <libgimp/gimpcolordisplay.h>
#include <libgimp/gimpmodule.h>
#include <libgimp/gimpparasite.h>
#include <libgimp/gimpmath.h>
#include <libgimp/gimpui.h>

#include "libgimp/gimpintl.h"


#define COLOR_DISPLAY_NAME _("Gamma")

typedef struct _GammaContext GammaContext;

struct _GammaContext
{
  GFunc    ok_func;
  gpointer ok_data;
  GFunc    cancel_func;
  gpointer cancel_data;

  gdouble  gamma;
  guchar  *lookup;

  GtkWidget *shell;
  GtkWidget *spinner;
};

static gpointer       gamma_new                       (int           type);
static gpointer       gamma_clone                     (gpointer      cd_ID);
static void           gamma_create_lookup_table       (GammaContext *context);
static void           gamma_destroy                   (gpointer      cd_ID);
static void           gamma_convert                   (gpointer      cd_ID,
						       guchar       *buf,
						       int           w,
						       int           h,
						       int           bpp,
						       int           bpl);
static void           gamma_load                      (gpointer      cd_ID,
						       GimpParasite *state);
static GimpParasite * gamma_save                      (gpointer      cd_ID);
static void           gamma_configure_ok_callback     (GtkWidget    *widget,
						       gpointer      data);
static void           gamma_configure_cancel_callback (GtkWidget    *widget,
						       gpointer      data);
static void           gamma_configure                 (gpointer      cd_ID,
						       GFunc         ok_func,
						       gpointer      ok_data,
						       GFunc         cancel_func,
						       gpointer      cancel_data);
static void           gamma_configure_cancel          (gpointer      cd_ID);

static GimpColorDisplayMethods methods = 
{
  NULL,
  gamma_new,
  gamma_clone,
  gamma_convert,
  gamma_destroy,
  NULL,
  gamma_load,
  gamma_save,
  gamma_configure,
  gamma_configure_cancel
};

static GimpModuleInfo info = 
{
  NULL,
  N_("Gamma color display filter"),
  "Manish Singh <yosh@gimp.org>",
  "v0.2",
  "(c) 1999, released under the GPL",
  "October 14, 2000"
};

G_MODULE_EXPORT GimpModuleStatus
module_init (GimpModuleInfo **inforet)
{
#ifndef __EMX__ 
  if (gimp_color_display_register (COLOR_DISPLAY_NAME, &methods))
#else
  if (mod_color_display_register (COLOR_DISPLAY_NAME, &methods))
#endif
    {
      *inforet = &info;
      return GIMP_MODULE_OK;
    }
  else
    return GIMP_MODULE_UNLOAD;
}

G_MODULE_EXPORT void
module_unload (void  *shutdown_data,
	       void (*completed_cb) (void *),
	       void  *completed_data)
{
#ifndef __EMX__
  gimp_color_display_unregister (COLOR_DISPLAY_NAME);
#else
  mod_color_display_unregister (COLOR_DISPLAY_NAME);
#endif
}


static gpointer
gamma_new (int type)
{
  int i;
  GammaContext *context;

  context = g_new0 (GammaContext, 1);
  context->gamma = 1.0;
  context->lookup = g_new (guchar, 256);

  for (i = 0; i < 256; i++)
    context->lookup[i] = i;

  return context;
}

static gpointer
gamma_clone (gpointer cd_ID)
{
  GammaContext *src_context = cd_ID;
  GammaContext *context;

  context = gamma_new (0);
  context->gamma = src_context->gamma;
  
  memcpy (context->lookup, src_context->lookup, sizeof (guchar) * 256);

  return context;
}

static void
gamma_create_lookup_table (GammaContext *context)
{
  gdouble one_over_gamma;
  gdouble ind;
  gint    i;

  if (context->gamma == 0.0)
    context->gamma = 1.0;

  one_over_gamma = 1.0 / context->gamma;

  for (i = 0; i < 256; i++)
    {
      ind = (double) i / 255.0;
      context->lookup[i] = (guchar) (gint) (255 * pow (ind, one_over_gamma));
    }
}

static void
gamma_destroy (gpointer cd_ID)
{
  GammaContext *context = cd_ID;

  if (context->shell)
    {
      dialog_unregister (context->shell);
      gtk_widget_destroy (context->shell);
    }

  g_free (context->lookup);
  g_free (context);
}

static void
gamma_convert (gpointer  cd_ID,
    	       guchar   *buf,
	       gint      width,
	       gint      height,
	       gint      bpp,
	       gint      bpl)
{
  guchar *lookup = ((GammaContext *) cd_ID)->lookup;
  gint    i, j = height;

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
	  *buf = lookup[*buf];
	  buf++;
	}
      buf += bpl;
    }
}

static void
gamma_load (gpointer      cd_ID,
            GimpParasite *state)
{
  GammaContext *context = cd_ID;

#if G_BYTE_ORDER == G_BIG_ENDIAN
  memcpy (&context->gamma, gimp_parasite_data (state), sizeof (gdouble));
#else
  guint32 buf[2], *data = gimp_parasite_data (state);

  buf[0] = g_ntohl (data[1]);
  buf[1] = g_ntohl (data[0]);

  memcpy (&context->gamma, buf, sizeof (gdouble));
#endif

  gamma_create_lookup_table (context);
}

static GimpParasite *
gamma_save (gpointer cd_ID)
{
  GammaContext *context = cd_ID;
  guint32 buf[2];

  memcpy (buf, &context->gamma, sizeof (double));

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  {
    guint32 tmp = g_htonl (buf[0]);
    buf[0] = g_htonl (buf[1]);
    buf[1] = tmp;
  }
#endif

  return gimp_parasite_new ("Display/Gamma", GIMP_PARASITE_PERSISTENT,
			    sizeof (double), &buf);
}

static void
gamma_configure_ok_callback (GtkWidget *widget,
			     gpointer   data)
{
  GammaContext *context = data;

  context->gamma =
    gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (context->spinner));
  gamma_create_lookup_table (context);

  dialog_unregister (context->shell);
  gtk_widget_destroy (GTK_WIDGET (context->shell));
  context->shell = NULL;

  if (context->ok_func)
    context->ok_func (context, context->ok_data);
}

static void
gamma_configure_cancel_callback (GtkWidget *widget,
				 gpointer   data)
{
  GammaContext *context = data;

  dialog_unregister (context->shell);
  gtk_widget_destroy (GTK_WIDGET (context->shell));
  context->shell = NULL;

  if (context->cancel_func)
    context->cancel_func (context, context->cancel_data);
}

static void
gamma_configure (gpointer cd_ID,
		 GFunc    ok_func,
		 gpointer ok_data,
		 GFunc    cancel_func,
		 gpointer cancel_data)
{
  GammaContext *context = cd_ID;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkObject *adjustment;

  if (!context->shell)
    {
      context->ok_func = ok_func;
      context->ok_data = ok_data;
      context->cancel_func = cancel_func;
      context->cancel_data = cancel_data;

      context->shell =
	gimp_dialog_new (_("Gamma"), "gamma",
			 gimp_standard_help_func, "modules/gamma.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), gamma_configure_ok_callback,
			 cd_ID, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gamma_configure_cancel_callback,
			 cd_ID, NULL, NULL, FALSE, TRUE,

			 NULL);

      dialog_register (context->shell);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (context->shell)->vbox),
			  hbox, FALSE, FALSE, 0);

      label = gtk_label_new ( _("Gamma:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

      adjustment = gtk_adjustment_new (context->gamma, 0.01, 10.0, 0.01, 0.1, 0.0);
      context->spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment),
					      0.1, 3);
      gtk_widget_set_usize (context->spinner, 100, 0);
      gtk_box_pack_start (GTK_BOX (hbox), context->spinner, FALSE, FALSE, 0);
    }

  gtk_widget_show_all (context->shell);
}

static void
gamma_configure_cancel (gpointer cd_ID)
{
  GammaContext *context = cd_ID;
 
  if (context->shell)
    {
      dialog_unregister (context->shell);
      gtk_widget_destroy (context->shell);
      context->shell = NULL;
    }

  if (context->cancel_func)
    context->cancel_func (context, context->cancel_data);
}
