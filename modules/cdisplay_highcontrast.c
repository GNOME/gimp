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

#include <math.h>
#include <string.h>

#include <libgimp/color_display.h>
#include <libgimp/gimpintl.h>
#include <libgimp/gimpmodule.h>
#include <libgimp/parasite.h>
#include <libgimp/gimpui.h>

#include <gtk/gtk.h>
#include "modregister.h"

#define COLOR_DISPLAY_NAME _("High Contrast")

typedef struct _ContrastContext ContrastContext;

struct _ContrastContext
{
  GFunc    ok_func;
  gpointer ok_data;
  GFunc    cancel_func;
  gpointer cancel_data;

  double  contrast;
  guchar *lookup;

  GtkWidget *shell;
  GtkWidget *spinner;
};

static gpointer   contrast_new                       (int           type);
static gpointer   contrast_clone                     (gpointer      cd_ID);
static void       contrast_create_lookup_table       (ContrastContext *context);
static void       contrast_destroy                   (gpointer      cd_ID);
static void       contrast_convert                   (gpointer      cd_ID,
						      guchar       *buf,
						      int           w,
						      int           h,
						      int           bpp,
						      int           bpl);
static void       contrast_load                      (gpointer      cd_ID,
						      Parasite     *state);
static Parasite * contrast_save                      (gpointer      cd_ID);
static void       contrast_configure_ok_callback     (GtkWidget    *widget,
						      gpointer      data);
static void       contrast_configure_cancel_callback (GtkWidget    *widget,
						      gpointer      data);
static void       contrast_configure                 (gpointer      cd_ID,
						      GFunc         ok_func,
						      gpointer      ok_data,
						      GFunc         cancel_func,
						      gpointer      cancel_data);
static void       contrast_configure_cancel          (gpointer      cd_ID);

static GimpColorDisplayMethods methods = 
{
  NULL,
  contrast_new,
  contrast_clone,
  contrast_convert,
  contrast_destroy,
  NULL,
  contrast_load,
  contrast_save,
  contrast_configure,
  contrast_configure_cancel
};

static GimpModuleInfo info = 
{
  NULL,
  N_("High Contrast color display filter"),
  "Jay Cox <jaycox@earthlink.net>",
  "v0.1",
  "(c) 2000, released under the GPL",
  "April 28, 2000"
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
module_unload (void *shutdown_data,
	       void (*completed_cb)(void *),
	       void *completed_data)
{
#ifndef __EMX__ 
  gimp_color_display_unregister (COLOR_DISPLAY_NAME);
#else
  mod_color_display_unregister (COLOR_DISPLAY_NAME);
#endif
}


static gpointer
contrast_new (int type)
{
  ContrastContext *context;

  context = g_new0 (ContrastContext, 1);
  context->contrast = 4.0;
  context->lookup = g_new (guchar, 256);

  contrast_create_lookup_table (context);

  return context;
}

static gpointer
contrast_clone (gpointer cd_ID)
{
  ContrastContext *src_context = cd_ID;
  ContrastContext *context;

  context = contrast_new (0);
  context->contrast = src_context->contrast;
  
  memcpy (context->lookup, src_context->lookup, sizeof(guchar) * 256);

  return context;
}

static void
contrast_create_lookup_table (ContrastContext *context)
{
  int i;

  if (context->contrast == 0.0)
    context->contrast = 1.0;

  for (i = 0; i < 256; i++)
    {
      context->lookup[i] 
	= (guchar) (int)(255 * .5 * (1 + sin(context->contrast*2*M_PI*i/255.0)));
    }
}

static void
contrast_destroy (gpointer cd_ID)
{
  ContrastContext *context = cd_ID;

  if (context->shell)
    gtk_widget_destroy (context->shell);

  g_free (context->lookup);
  g_free (context);
}

static void
contrast_convert (gpointer  cd_ID,
		  guchar   *buf,
		  int       width,
		  int       height,
		  int       bpp,
		  int       bpl)
{
  guchar *lookup = ((ContrastContext *) cd_ID)->lookup;
  int i, j = height;

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
contrast_load (gpointer  cd_ID,
	       Parasite *state)
{
  ContrastContext *context = cd_ID;

#if G_BYTE_ORDER == G_BIG_ENDIAN
  memcpy (&context->contrast, parasite_data (state), sizeof (double));
#else
  guint32 buf[2], *data = parasite_data (state);

  buf[0] = g_ntohl (data[1]);
  buf[1] = g_ntohl (data[0]);

  memcpy (&context->contrast, buf, sizeof (double));
#endif

  contrast_create_lookup_table (context);
}

static Parasite *
contrast_save (gpointer cd_ID)
{
  ContrastContext *context = cd_ID;
  guint32 buf[2];

  memcpy (buf, &context->contrast, sizeof (double));

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  {
    guint32 tmp = g_htonl (buf[0]);
    buf[0] = g_htonl (buf[1]);
    buf[1] = tmp;
  }
#endif

  return parasite_new ("Display/Contrast", PARASITE_PERSISTENT,
		       sizeof (double), &buf);
}

static void
contrast_configure_ok_callback (GtkWidget *widget,
				gpointer   data)
{
  ContrastContext *context = data;

  context->contrast = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (context->spinner));
  contrast_create_lookup_table (context);

  gtk_widget_destroy (GTK_WIDGET (context->shell));
  context->shell = NULL;

  if (context->ok_func)
    context->ok_func (context, context->ok_data);
}

static void
contrast_configure_cancel_callback (GtkWidget *widget,
				    gpointer   data)
{
  ContrastContext *context = data;

  gtk_widget_destroy (GTK_WIDGET (context->shell));
  context->shell = NULL;

  if (context->cancel_func)
    context->cancel_func (context, context->cancel_data);
}

static void
contrast_configure (gpointer cd_ID,
		    GFunc    ok_func,
		    gpointer ok_data,
		    GFunc    cancel_func,
		    gpointer cancel_data)
{
  ContrastContext *context = cd_ID;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkObject *adjustment;

  if (!context->shell)
    {
      context->ok_func = ok_func;
      context->ok_data = ok_data;
      context->cancel_func = cancel_func;
      context->cancel_data = cancel_data;

      context->shell = gimp_dialog_new (_("High Contrast"), "high contrast",
					gimp_standard_help_func, "modules/highcontrast.html",
					GTK_WIN_POS_MOUSE,
					FALSE, TRUE, FALSE,

					_("OK"), contrast_configure_ok_callback,
					cd_ID, NULL, NULL, TRUE, FALSE,
					_("Cancel"), contrast_configure_cancel_callback,
					cd_ID, NULL, NULL, FALSE, TRUE,
					
					NULL);

      hbox = gtk_hbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (context->shell)->vbox),
			  hbox, FALSE, FALSE, 0);

      label = gtk_label_new ( _("Contrast Cycles:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

      adjustment = gtk_adjustment_new (4.0, 1.0, 20.0, 0.5, 1.0, 0.0);
      context->spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment),
					      0.1, 3);
      gtk_widget_set_usize (context->spinner, 100, 0);
      gtk_box_pack_start (GTK_BOX (hbox), context->spinner, FALSE, FALSE, 0); 
    }

  gtk_widget_show_all (context->shell);
}

static void
contrast_configure_cancel (gpointer cd_ID)
{
  ContrastContext *context = cd_ID;
 
  if (context->shell)
    gtk_widget_destroy (context->shell);

  context->shell = NULL;

  if (context->cancel_func)
    context->cancel_func (context, context->cancel_data);
}
