/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Manish Singh
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

#include "gdisplay_color.h"
#include "gdisplay.h"
#include "gimpimageP.h"
#include "gimpui.h"

#include "libgimp/parasite.h"
#include "libgimp/gimpintl.h"

#include <gtk/gtk.h>

typedef struct _ColorDisplayInfo ColorDisplayInfo;

struct _ColorDisplayInfo {
  char                    *name;
  GimpColorDisplayMethods  methods;
  GSList                  *refs;
};

static GHashTable *color_display_table = NULL;

typedef struct _GammaContext GammaContext;

struct _GammaContext
{
  double gamma;
  guchar *lookup;

  GtkWidget *shell;
  GtkWidget *spinner;
};

static void       color_display_foreach (gpointer key,
					 gpointer value,
					 gpointer user_data);

static void       gdisplay_color_detach_real (GDisplay         *gdisp,
					      ColorDisplayNode *node,
					      gboolean          unref);
static gint       node_name_compare          (ColorDisplayNode *node,
					      const char       *name);

static gpointer   gamma_new                       (int           type);
static void       gamma_create_lookup_table       (GammaContext *context);
static void       gamma_destroy                   (gpointer      cd_ID);
static void       gamma_convert                   (gpointer      cd_ID,
						   guchar       *buf,
						   int           width,
						   int           height,
						   int           bpp);
static void       gamma_load                      (gpointer      cd_ID,
						   Parasite     *state);
static Parasite * gamma_save                      (gpointer      cd_ID);
static void       gamma_configure_ok_callback     (GtkWidget    *widget,
						   gpointer      data);
static void       gamma_configure_cancel_callback (GtkWidget    *widget,
						   gpointer      data);
static void       gamma_configure                 (gpointer      cd_ID);

void
gdisplay_color_init (void)
{
  GimpColorDisplayMethods methods = {
    NULL,
    gamma_new,
    gamma_convert,
    gamma_destroy,
    NULL,
    gamma_load,
    gamma_save,
    gamma_configure
  };

  gimp_color_display_register ("Gamma", &methods);
}

gboolean
gimp_color_display_register (const char              *name,
    			     GimpColorDisplayMethods *methods)
{
  ColorDisplayInfo *info;
  
  info = g_new (ColorDisplayInfo, 1);
  
  info->name = g_strdup (name);
  info->methods = *methods;
  info->refs = NULL;
  
  if (!color_display_table)
    color_display_table = g_hash_table_new (g_str_hash, g_str_equal);
  
  if (g_hash_table_lookup (color_display_table, name))
    return FALSE;

  if (!methods->convert)
    return FALSE;

  g_hash_table_insert (color_display_table, info->name, info);
  
  return TRUE;
}

gboolean
gimp_color_display_unregister (const char *name)
{
  ColorDisplayInfo *info;
  GDisplay *gdisp;
  GList *node;

  if ((info = g_hash_table_lookup (color_display_table, name)))
    {
      GSList *refs = info->refs;

      while (refs)
	{
	  gdisp = (GDisplay *) refs->data;

	  node = g_list_find_custom (gdisp->cd_list, name, node_name_compare);
	  gdisp->cd_list = g_slist_remove_link (gdisp->cd_list, node);

	  gdisplay_color_detach_real (gdisp, node->data, FALSE);

	  g_list_free_1 (node);

	  refs = refs->next;
	}

      g_slist_free (info->refs);

      g_hash_table_remove (color_display_table, name);

      g_free (info->name);
      g_free (info);
    }
  
  return TRUE;
}

typedef struct _DisplayForeachData DisplayForeachData;

struct _DisplayForeachData
{
  GimpCDFunc func;
  gpointer   user_data;
};

void
gimp_color_display_foreach (GimpCDFunc func,
			    gpointer   user_data)
{
  DisplayForeachData data;

  data.func = func;
  data.user_data = user_data;

  g_hash_table_foreach (color_display_table, color_display_foreach, &data);
}

static void
color_display_foreach (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
  DisplayForeachData *data = (DisplayForeachData *) user_data;
  data->func (key, data->user_data);
}

void
gdisplay_color_attach (GDisplay   *gdisp,
    		       const char *name)
{
  ColorDisplayInfo *info;
  ColorDisplayNode *node;

  if ((info = g_hash_table_lookup (color_display_table, name)))
    {
      node = g_new (ColorDisplayNode, 1);

      node->cd_name = g_strdup (name);
      node->cd_ID = NULL;

      if (!info->refs && info->methods.init)
	info->methods.init ();

      info->refs = g_slist_append (info->refs, gdisp);

      if (info->methods.new)
	node->cd_ID = info->methods.new (gdisp->gimage->base_type);

      node->cd_convert = info->methods.convert;

      gdisp->cd_list = g_list_append (gdisp->cd_list, name);
    }
  else
    g_warning ("Tried to attach a nonexistant color display");
}

void
gdisplay_color_detach (GDisplay   *gdisp,
		       const char *name)
{
  GList *node;

  node = g_list_find_custom (gdisp->cd_list, name, node_name_compare);
  gdisplay_color_detach_real (gdisp, node->data, TRUE);

  gdisp->cd_list = g_list_remove_link (gdisp->cd_list, node);
  g_list_free_1 (node);
}

void
gdisplay_color_detach_all (GDisplay *gdisp)
{
  GList *list = gdisp->cd_list;

  while (list)
    {
      gdisplay_color_detach_real (gdisp, list->data, TRUE);
      list = list->next;
    }

  g_list_free (gdisp->cd_list);
  gdisp->cd_list = NULL;
}

static void
gdisplay_color_detach_real (GDisplay         *gdisp,
			    ColorDisplayNode *node,
			    gboolean          unref)
{
  ColorDisplayInfo *info;

  if ((info = g_hash_table_lookup (color_display_table, node->cd_name)))
    {
      if (info->methods.destroy)
	info->methods.destroy (node->cd_ID);

      if (unref)
        info->refs = g_slist_remove (info->refs, gdisp);
      
      if (!info->refs && info->methods.finalize)
	info->methods.finalize ();
    }

  g_free (node->cd_name);
  g_free (node);
}  

static gint
node_name_compare (ColorDisplayNode *node,
		   const char       *name)
{
  return strcmp (node->cd_name, name);
}

/* The Gamma Color Display */

static gpointer
gamma_new (int type)
{
  int i;
  GammaContext *context = NULL;

  context = g_new (GammaContext, 1);
  context->gamma = 1.0;
  context->lookup = g_new (guchar, 256);
  context->shell = NULL;
  context->spinner = NULL;

  for (i = 0; i < 256; i++)
    context->lookup[i] = i;

  return context;
}

static void
gamma_create_lookup_table (GammaContext *context)
{
  double one_over_gamma;
  double ind;
  int i;

  if (context->gamma == 0.0)
    context->gamma = 1.0;

  one_over_gamma = 1.0 / context->gamma;

  for (i = 0; i < 256; i++)
    {
      ind = (double) i / 255.0;
      context->lookup[i] = (guchar) (int) (255 * pow (ind, one_over_gamma));
    }
}

static void
gamma_destroy (gpointer cd_ID)
{
  GammaContext *context = (GammaContext *) cd_ID;

  if (context->shell)
    gtk_widget_destroy (context->shell);

  g_free (context->lookup);
  g_free (context);
}

static void
gamma_convert (gpointer  cd_ID,
    	       guchar   *buf,
	       int       width,
	       int       height,
	       int       bpp)
{
  int i;
  guchar *lookup = ((GammaContext *) cd_ID)->lookup;

  for (i = 0; i < width * height * bpp; i++)
    *buf++ = lookup[*buf];
}

static void
gamma_load (gpointer  cd_ID,
            Parasite *state)
{
  GammaContext *context = (GammaContext *) cd_ID;

#if G_BYTE_ORDER == G_BIG_ENDIAN
  memcpy (&context->gamma, parasite_data (state), sizeof (double));
#else
  guint32 buf[2], *data = parasite_data (state);

  buf[0] = g_ntohl (data[1]);
  buf[1] = g_ntohl (data[0]);

  memcpy (&context->gamma, buf, sizeof (double));
#endif

  gamma_create_lookup_table (context);
}

static Parasite *
gamma_save (gpointer cd_ID)
{
  GammaContext *context = (GammaContext *) cd_ID;
  guint32 buf[2];

  memcpy (buf, &context->gamma, sizeof (double));

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  {
    guint32 tmp = g_htonl (buf[0]);
    buf[0] = g_htonl (buf[1]);
    buf[1] = tmp;
  }
#endif

  return parasite_new ("Display/Gamma", PARASITE_PERSISTENT,
		       sizeof (double), &buf);
}

static void
gamma_configure_ok_callback (GtkWidget *widget,
			     gpointer   data)
{
  GammaContext *context = (GammaContext *) data;

  context->gamma = gtk_spin_button_get_value_as_float (context->spinner);
  gamma_create_lookup_table (context);

  gtk_widget_hide (widget);
}

static void
gamma_configure_cancel_callback (GtkWidget *widget,
				 gpointer   data)
{
  gtk_widget_hide (widget);
}

static void
gamma_configure (gpointer cd_ID)
{
  GammaContext *context = (GammaContext *) cd_ID;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkObject *adjustment;

  if (!context->shell)
    {
      context->shell =
	gimp_dialog_new (_("Gamma"), "gamma",
			 gimp_standard_help_func,
			 "dialogs/gamma_dialog.html",
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, FALSE,

			 _("OK"), gamma_configure_ok_callback,
			 cd_ID, NULL, FALSE, FALSE,
			 _("Cancel"), gamma_configure_cancel_callback,
			 cd_ID, NULL, TRUE, TRUE,

			 NULL);

      hbox = gtk_hbox_new (TRUE, 2);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (context->shell)->vbox),
			  hbox, FALSE, FALSE, 0);

      label = gtk_label_new (_("Gamma:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);

      adjustment = gtk_adjustment_new (1.0, 0.01, 10.0, 0.01, 0.1, 0.0);
      context->spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment),
					      0.1, 3);
      gtk_widget_set_usize (context->spinner, 100, 0);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
 
      gtk_widget_show_all (hbox);
    }

  gtk_widget_show (context->shell);
}
