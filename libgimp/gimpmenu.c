/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmenu.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include "gimp.h"
#include "gimpui.h"

#include "libgimp-intl.h"


#define MENU_THUMBNAIL_WIDTH   24
#define MENU_THUMBNAIL_HEIGHT  24


/* Copy data from temp_PDB call */
struct _GimpBrushData 
{
  guint                 idle_id;
  gchar                *name;
  gdouble               opacity;
  gint                  spacing;
  gint                  paint_mode;
  gint                  width;
  gint                  height;
  guchar               *brush_mask_data;
  GimpRunBrushCallback  callback;
  gboolean              closing;
  gpointer              data;
};

typedef struct _GimpBrushData GimpBrushData;


/* Copy data from temp_PDB call */
struct _GimpFontData 
{
  guint                idle_id;
  gchar               *name;
  GimpRunFontCallback  callback;
  gboolean             closing;
  gpointer             data;
};

typedef struct _GimpFontData GimpFontData;


/* Copy data from temp_PDB call */
struct _GimpGradientData 
{
  guint                    idle_id;
  gchar                   *name;
  gint                     width;
  gdouble                 *gradient_data;
  GimpRunGradientCallback  callback;
  gboolean                 closing;
  gpointer                 data;
};

typedef struct _GimpGradientData GimpGradientData;


/* Copy data from temp_PDB call */
struct _GimpPatternData 
{
  guint                    idle_id;
  gchar                   *name;
  gint                     width;
  gint                     height;
  gint                     bytes;
  guchar                  *pattern_mask_data;
  GimpRunPatternCallback   callback;
  gboolean                 closing;
  gpointer                 data;
};

typedef struct _GimpPatternData GimpPatternData;


static void      gimp_menu_callback      (GtkWidget         *widget,
                                          gint32            *id);

static gboolean  idle_brush_callback     (GimpBrushData     *bdata);
static gboolean  idle_font_callback      (GimpFontData      *fdata);
static gboolean  idle_gradient_callback  (GimpGradientData  *gdata);
static gboolean  idle_pattern_callback   (GimpPatternData   *pdata);

static void      temp_brush_invoker      (gchar             *name,
                                          gint               nparams,
                                          GimpParam         *param,
                                          gint              *nreturn_vals,
                                          GimpParam        **return_vals);
static gchar   * gen_temp_plugin_name    (void);
static void      fill_preview_with_thumb (GtkWidget         *widget,
                                          gint32             drawable_ID,
                                          gint               width,
                                          gint               height);


static GHashTable  *gbrush_ht    = NULL;
static GHashTable  *gfont_ht     = NULL;
static GHashTable  *ggradient_ht = NULL;
static GHashTable  *gpattern_ht  = NULL;


GtkWidget *
gimp_image_menu_new (GimpConstraintFunc constraint,
		     GimpMenuCallback   callback,
		     gpointer           data,
		     gint32             active_image)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gchar     *name;
  gchar     *label;
  gint32    *images;
  gint       nimages;
  gint       i, k;

  menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (menu), "gimp_callback", (gpointer) callback);
  g_object_set_data (G_OBJECT (menu), "gimp_callback_data", data);

  images = gimp_image_list (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_name (images[i]);

	label = g_strdup_printf ("%s-%d", name, images[i]);

	g_free (name);

	menuitem = gtk_menu_item_new_with_label (label);
	g_signal_connect (menuitem, "activate",
                          G_CALLBACK (gimp_menu_callback),
                          &images[i]);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	gtk_widget_show (menuitem);

	g_free (label);

	if (images[i] == active_image)
	  gtk_menu_set_active (GTK_MENU (menu), k);

	k += 1;
      }

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label (_("None"));
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (images)
    {
      if (active_image == -1)
	active_image = images[0];
      (* callback) (active_image, data);
    }

  return menu;
}


GtkWidget *
gimp_layer_menu_new (GimpConstraintFunc constraint,
		     GimpMenuCallback   callback,
		     gpointer           data,
		     gint32             active_layer)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gchar     *name;
  gchar     *image_label;
  gchar     *label;
  gint32    *images;
  gint32    *layers;
  gint32     layer;
  gint       nimages;
  gint       nlayers;
  gint       i, j, k;

  menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (menu), "gimp_callback", callback);
  g_object_set_data (G_OBJECT (menu), "gimp_callback_data", data);

  layer = -1;

  images = gimp_image_list (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_name (images[i]);

	image_label = g_strdup_printf ("%s-%d", name, images[i]);

	g_free (name);

	layers = gimp_image_get_layers (images[i], &nlayers);
	for (j = 0; j < nlayers; j++)
	  if (!constraint || (* constraint) (images[i], layers[j], data))
	    {
	      GtkWidget *hbox;
	      GtkWidget *vbox;
	      GtkWidget *wcolor_box;
	      GtkWidget *wlabel;

	      name = gimp_layer_get_name (layers[j]);
	      label = g_strdup_printf ("%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new();
	      g_signal_connect (menuitem, "activate",
                                G_CALLBACK (gimp_menu_callback),
                                &layers[j]);

	      hbox = gtk_hbox_new (FALSE, 0);
	      gtk_container_add (GTK_CONTAINER (menuitem), hbox);
	      gtk_widget_show (hbox);

	      vbox = gtk_vbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	      gtk_widget_show (vbox);
	      
	      wcolor_box = gtk_preview_new (GTK_PREVIEW_COLOR);
	      gtk_preview_set_dither (GTK_PREVIEW (wcolor_box),
                                      GDK_RGB_DITHER_MAX);

	      fill_preview_with_thumb (wcolor_box,
				       layers[j],
				       MENU_THUMBNAIL_WIDTH,
				       MENU_THUMBNAIL_HEIGHT);

	      gtk_widget_set_size_request (GTK_WIDGET (wcolor_box),
                                           MENU_THUMBNAIL_WIDTH,
                                           MENU_THUMBNAIL_HEIGHT);

	      gtk_container_add (GTK_CONTAINER (vbox), wcolor_box);
	      gtk_widget_show (wcolor_box);

	      wlabel = gtk_label_new (label);
	      gtk_misc_set_alignment (GTK_MISC (wlabel), 0.0, 0.5);
	      gtk_box_pack_start (GTK_BOX (hbox), wlabel, TRUE, TRUE, 4);
	      gtk_widget_show (wlabel);

	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (layers[j] == active_layer)
		{
		  layer = active_layer;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (layer == -1)
		layer = layers[j];

	      k += 1;
	    }

	g_free (image_label);
      }
  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label (_("None"));
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (layer != -1)
    (* callback) (layer, data);

  return menu;
}

GtkWidget *
gimp_channel_menu_new (GimpConstraintFunc constraint,
		       GimpMenuCallback   callback,
		       gpointer           data,
		       gint32             active_channel)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gchar     *name;
  gchar     *image_label;
  gchar     *label;
  gint32    *images;
  gint32    *channels;
  gint32     channel;
  gint       nimages;
  gint       nchannels;
  gint       i, j, k;

  menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (menu), "gimp_callback", callback);
  g_object_set_data (G_OBJECT (menu), "gimp_callback_data", data);

  channel = -1;

  images = gimp_image_list (&nimages);
  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_name (images[i]);

	image_label = g_strdup_printf ("%s-%d", name, images[i]);

	g_free (name);

	channels = gimp_image_get_channels (images[i], &nchannels);
	for (j = 0; j < nchannels; j++)
	  if (!constraint || (* constraint) (images[i], channels[j], data))
	    {
	      GtkWidget *hbox;
	      GtkWidget *vbox;
	      GtkWidget *wcolor_box;
	      GtkWidget *wlabel;

	      name = gimp_channel_get_name (channels[j]);
	      label = g_strdup_printf ("%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new ();
	      g_signal_connect (menuitem, "activate",
                                G_CALLBACK (gimp_menu_callback),
                                &channels[j]);
	      
	      hbox = gtk_hbox_new (FALSE, 0);
	      gtk_container_add (GTK_CONTAINER (menuitem), hbox);
	      gtk_widget_show (hbox);

	      vbox = gtk_vbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	      gtk_widget_show (vbox);

	      wcolor_box = gtk_preview_new (GTK_PREVIEW_COLOR);
	      gtk_preview_set_dither (GTK_PREVIEW (wcolor_box),
				      GDK_RGB_DITHER_MAX);

 	      fill_preview_with_thumb (wcolor_box, 
				       channels[j], 
				       MENU_THUMBNAIL_WIDTH, 
				       MENU_THUMBNAIL_HEIGHT); 

	      gtk_widget_set_size_request (GTK_WIDGET (wcolor_box),
                                           MENU_THUMBNAIL_WIDTH,
                                           MENU_THUMBNAIL_HEIGHT);

	      gtk_container_add (GTK_CONTAINER(vbox), wcolor_box);
	      gtk_widget_show (wcolor_box);

	      wlabel = gtk_label_new (label);
	      gtk_misc_set_alignment (GTK_MISC (wlabel), 0.0, 0.5);
	      gtk_box_pack_start (GTK_BOX (hbox), wlabel, TRUE, TRUE, 4);
	      gtk_widget_show (wlabel);

	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (channels[j] == active_channel)
		{
		  channel = active_channel;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (channel == -1)
		channel = channels[j];

	      k += 1;
	    }

	g_free (image_label);
      }
  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label (_("None"));
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (channel != -1)
    (* callback) (channel, data);

  return menu;
}

GtkWidget *
gimp_drawable_menu_new (GimpConstraintFunc constraint,
			GimpMenuCallback   callback,
			gpointer           data,
			gint32             active_drawable)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gchar     *name;
  gchar     *image_label;
  gchar     *label;
  gint32    *images;
  gint32    *layers;
  gint32    *channels;
  gint32     drawable;
  gint       nimages;
  gint       nlayers;
  gint       nchannels;
  gint       i, j, k;

  menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (menu), "gimp_callback", callback);
  g_object_set_data (G_OBJECT (menu), "gimp_callback_data", data);

  drawable = -1;

  images = gimp_image_list (&nimages);

  for (i = 0, k = 0; i < nimages; i++)
    if (!constraint || (* constraint) (images[i], -1, data))
      {
	name = gimp_image_get_name (images[i]);

	image_label = g_strdup_printf ("%s-%d", name, images[i]);

	g_free (name);

	layers = gimp_image_get_layers (images[i], &nlayers);

	for (j = 0; j < nlayers; j++)
	  if (!constraint || (* constraint) (images[i], layers[j], data))
	    {
	      GtkWidget *hbox;
	      GtkWidget *vbox;
	      GtkWidget *wcolor_box;
	      GtkWidget *wlabel;

	      name = gimp_layer_get_name (layers[j]);
	      label = g_strdup_printf ("%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new ();
	      g_signal_connect (menuitem, "activate",
                                G_CALLBACK (gimp_menu_callback),
                                &layers[j]);

	      hbox = gtk_hbox_new (FALSE, 0);
	      gtk_container_add (GTK_CONTAINER(menuitem), hbox);
	      gtk_widget_show (hbox);

	      vbox = gtk_vbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	      gtk_widget_show (vbox);
	      
	      wcolor_box = gtk_preview_new (GTK_PREVIEW_COLOR);
	      gtk_preview_set_dither (GTK_PREVIEW (wcolor_box), GDK_RGB_DITHER_MAX);

	      fill_preview_with_thumb (wcolor_box,
				       layers[j],
				       MENU_THUMBNAIL_WIDTH,
				       MENU_THUMBNAIL_HEIGHT);

	      gtk_widget_set_size_request (GTK_WIDGET (wcolor_box), 
                                           MENU_THUMBNAIL_WIDTH , 
                                           MENU_THUMBNAIL_HEIGHT);

	      gtk_container_add (GTK_CONTAINER(vbox), wcolor_box);
	      gtk_widget_show (wcolor_box);

	      wlabel = gtk_label_new (label);
	      gtk_misc_set_alignment (GTK_MISC (wlabel), 0.0, 0.5);
	      gtk_box_pack_start (GTK_BOX (hbox), wlabel, TRUE, TRUE, 4);
	      gtk_widget_show (wlabel);

	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (layers[j] == active_drawable)
		{
		  drawable = active_drawable;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (drawable == -1)
		drawable = layers[j];

	      k += 1;
	    }

	channels = gimp_image_get_channels (images[i], &nchannels);

	for (j = 0; j < nchannels; j++)
	  if (!constraint || (* constraint) (images[i], channels[j], data))
	    {
	      GtkWidget *hbox;
	      GtkWidget *vbox;
	      GtkWidget *wcolor_box;
	      GtkWidget *wlabel;

	      name = gimp_channel_get_name (channels[j]);
	      label = g_strdup_printf ("%s/%s", image_label, name);
	      g_free (name);

	      menuitem = gtk_menu_item_new ();
	      g_signal_connect (menuitem, "activate",
                                G_CALLBACK (gimp_menu_callback),
                                &channels[j]);

	      hbox = gtk_hbox_new (FALSE, 0);
	      gtk_container_add (GTK_CONTAINER (menuitem), hbox);
	      gtk_widget_show (hbox);

	      vbox = gtk_vbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	      gtk_widget_show (vbox);
	      
	      wcolor_box = gtk_preview_new (GTK_PREVIEW_COLOR);
	      gtk_preview_set_dither (GTK_PREVIEW (wcolor_box),
				      GDK_RGB_DITHER_MAX);

 	      fill_preview_with_thumb (wcolor_box, 
				       channels[j], 
				       MENU_THUMBNAIL_WIDTH, 
				       MENU_THUMBNAIL_HEIGHT); 

	      gtk_widget_set_size_request (GTK_WIDGET (wcolor_box), 
                                           MENU_THUMBNAIL_WIDTH , 
                                           MENU_THUMBNAIL_HEIGHT);
	      
	      gtk_container_add (GTK_CONTAINER (vbox), wcolor_box);
	      gtk_widget_show (wcolor_box);

	      wlabel = gtk_label_new (label);
	      gtk_misc_set_alignment (GTK_MISC (wlabel), 0.0, 0.5);
	      gtk_box_pack_start (GTK_BOX (hbox), wlabel, TRUE, TRUE, 4);
	      gtk_widget_show (wlabel);

	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	      gtk_widget_show (menuitem);

	      g_free (label);

	      if (channels[j] == active_drawable)
		{
		  drawable = active_drawable;
		  gtk_menu_set_active (GTK_MENU (menu), k);
		}
	      else if (drawable == -1)
		drawable = channels[j];

	      k += 1;
	    }

	g_free (image_label);
      }
  g_free (images);

  if (k == 0)
    {
      menuitem = gtk_menu_item_new_with_label (_("None"));
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  if (drawable != -1)
    (* callback) (drawable, data);

  return menu;
}


static void
gimp_menu_callback (GtkWidget *widget,
		    gint32    *id)
{
  GimpMenuCallback callback;
  gpointer         callback_data;

  callback = (GimpMenuCallback) g_object_get_data (G_OBJECT (widget->parent),
                                                   "gimp_callback");
  callback_data = g_object_get_data (G_OBJECT (widget->parent),
                                     "gimp_callback_data");

  (* callback) (*id, callback_data);
}

static void
fill_preview_with_thumb (GtkWidget *widget,
			 gint32     drawable_ID,
			 gint       width,
			 gint       height)
{
  guchar  *drawable_data;
  gint     bpp;
  gint     x, y;
  guchar  *src;
  gdouble  r, g, b, a;
  gdouble  c0, c1;
  guchar  *p0, *p1, *even, *odd;

  bpp = 0; /* Only returned */
  
  drawable_data = 
    gimp_drawable_get_thumbnail_data (drawable_ID, &width, &height, &bpp);

  gtk_preview_size (GTK_PREVIEW (widget), width, height);

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);
  src  = drawable_data;

  for (y = 0; y < height; y++)
    {
      p0 = even;
      p1 = odd;
      
      for (x = 0; x < width; x++) 
	{
	  if (bpp == 4)
	    {
	      r = ((gdouble) src[x*4+0]) / 255.0;
	      g = ((gdouble) src[x*4+1]) / 255.0;
	      b = ((gdouble) src[x*4+2]) / 255.0;
	      a = ((gdouble) src[x*4+3]) / 255.0;
	    }
	  else if (bpp == 3)
	    {
	      r = ((gdouble) src[x*3+0]) / 255.0;
	      g = ((gdouble) src[x*3+1]) / 255.0;
	      b = ((gdouble) src[x*3+2]) / 255.0;
	      a = 1.0;
	    }
	  else
	    {
	      r = ((gdouble) src[x*bpp+0]) / 255.0;
	      g = b = r;
	      if (bpp == 2)
		a = ((gdouble) src[x*bpp+1]) / 255.0;
	      else
		a = 1.0;
	    }

	  if ((x / GIMP_CHECK_SIZE_SM) & 1) 
	    {
	      c0 = GIMP_CHECK_LIGHT;
	      c1 = GIMP_CHECK_DARK;
	    } 
	  else 
	    {
	      c0 = GIMP_CHECK_DARK;
	      c1 = GIMP_CHECK_LIGHT;
	    }
	
	  *p0++ = (c0 + (r - c0) * a) * 255.0;
	  *p0++ = (c0 + (g - c0) * a) * 255.0;
	  *p0++ = (c0 + (b - c0) * a) * 255.0;

	  *p1++ = (c1 + (r - c1) * a) * 255.0;
	  *p1++ = (c1 + (g - c1) * a) * 255.0;
	  *p1++ = (c1 + (b - c1) * a) * 255.0;  
	}
      
      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	gtk_preview_draw_row (GTK_PREVIEW (widget), odd,  0, y, width);
      else
	gtk_preview_draw_row (GTK_PREVIEW (widget), even, 0, y, width);

      src += width * bpp;
    }

  g_free (even);
  g_free (odd);
}

static gboolean
idle_brush_callback (GimpBrushData *bdata)
{
  if (bdata->callback)
    bdata->callback (bdata->name,
		     bdata->opacity,
		     bdata->spacing,
		     bdata->paint_mode,
		     bdata->width,
		     bdata->height,
		     bdata->brush_mask_data,
		     bdata->closing,
		     bdata->data);

  g_free (bdata->name);  
  g_free (bdata->brush_mask_data); 

  bdata->idle_id         = 0;
  bdata->name            = NULL;
  bdata->brush_mask_data = NULL;

  return FALSE;
}

static gboolean
idle_font_callback (GimpFontData *fdata)
{
  if (fdata->callback)
    fdata->callback (fdata->name,
		     fdata->closing,
		     fdata->data);
  
  g_free (fdata->name);  

  fdata->idle_id = 0;
  fdata->name    = NULL;

  return FALSE;
}

static gboolean
idle_gradient_callback (GimpGradientData *gdata)
{
  if (gdata->callback)
    gdata->callback (gdata->name,
		     gdata->width,
		     gdata->gradient_data,
		     gdata->closing,
		     gdata->data);

  g_free (gdata->name);  
  g_free (gdata->gradient_data); 

  gdata->idle_id       = 0;
  gdata->name          = NULL;
  gdata->gradient_data = NULL;

  return FALSE;
}

static gboolean
idle_pattern_callback (GimpPatternData *pdata)
{
  if (pdata->callback)
    pdata->callback (pdata->name,
		     pdata->width,
		     pdata->height,
		     pdata->bytes,
		     pdata->pattern_mask_data,
		     pdata->closing,
		     pdata->data);
  
  g_free (pdata->name);  
  g_free (pdata->pattern_mask_data); 

  pdata->idle_id           = 0;
  pdata->name              = NULL;
  pdata->pattern_mask_data = NULL;

  return FALSE;
}

static void
temp_brush_invoker (gchar      *name,
		    gint        nparams,
		    GimpParam  *param,
		    gint       *nreturn_vals,
		    GimpParam **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpBrushData     *bdata;

  bdata = (GimpBrushData *) g_hash_table_lookup (gbrush_ht, name);

  if (! bdata)
    {
      g_warning("Can't find internal brush data");
    }
  else
    {
      g_free (bdata->name);
      g_free (bdata->brush_mask_data);

      bdata->name            = g_strdup (param[0].data.d_string);
      bdata->opacity         = (gdouble) param[1].data.d_float;
      bdata->spacing         = param[2].data.d_int32;
      bdata->paint_mode      = param[3].data.d_int32;
      bdata->width           = param[4].data.d_int32;
      bdata->height          = param[5].data.d_int32;
      bdata->brush_mask_data = g_memdup (param[7].data.d_int8array,
                                         param[6].data.d_int32);
      bdata->closing         = param[8].data.d_int32;

      if (! bdata->idle_id)
        bdata->idle_id = g_idle_add ((GSourceFunc) idle_brush_callback, bdata);
    }

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
temp_font_invoker (gchar      *name,
                   gint        nparams,
                   GimpParam  *param,
                   gint       *nreturn_vals,
                   GimpParam **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpFontData      *fdata;

  fdata = (GimpFontData *) g_hash_table_lookup (gfont_ht, name);

  if (! fdata)
    {
      g_warning ("Can't find internal font data");
    }
  else
    {
      g_free (fdata->name);

      fdata->name     = g_strdup (param[0].data.d_string);
      fdata->closing  = param[1].data.d_int32;

      if (! fdata->idle_id)
        fdata->idle_id = g_idle_add ((GSourceFunc) idle_font_callback, fdata);
    }

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
temp_gradient_invoker (gchar      *name,
		       gint        nparams,
		       GimpParam  *param,
		       gint       *nreturn_vals,
		       GimpParam **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpGradientData  *gdata;

  gdata = (GimpGradientData *) g_hash_table_lookup (ggradient_ht, name);

  if (! gdata)
    {
      g_warning("Can't find internal gradient data");
    }
  else
    {
      g_free (gdata->name);
      g_free (gdata->gradient_data);

      gdata->name          = g_strdup (param[0].data.d_string);
      gdata->width         = param[1].data.d_int32;
      gdata->gradient_data = g_memdup (param[2].data.d_floatarray,
                                       param[1].data.d_int32 * sizeof (gdouble));
      gdata->closing      = param[3].data.d_int32;

      if (! gdata->idle_id)
        gdata->idle_id = g_idle_add ((GSourceFunc) idle_gradient_callback,
                                     gdata);
    }

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
temp_pattern_invoker (gchar      *name,
		      gint        nparams,
		      GimpParam  *param,
		      gint       *nreturn_vals,
		      GimpParam **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpPatternData   *pdata;

  pdata = (GimpPatternData *) g_hash_table_lookup (gpattern_ht, name);

  if (! pdata)
    {
      g_warning ("Can't find internal pattern data");
    }
  else
    {
      g_free (pdata->name);
      g_free (pdata->pattern_mask_data);

      pdata->name              = g_strdup(param[0].data.d_string);
      pdata->width             = param[1].data.d_int32;
      pdata->height            = param[2].data.d_int32;
      pdata->bytes             = param[3].data.d_int32;
      pdata->pattern_mask_data = g_memdup (param[5].data.d_int8array,
                                           param[4].data.d_int32);
      pdata->closing           = param[6].data.d_int32;

      if (! pdata->idle_id)
        pdata->idle_id = g_idle_add ((GSourceFunc) idle_pattern_callback,
                                     pdata);
    }

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static gchar *
gen_temp_plugin_name (void)
{
  GimpParam *return_vals;
  gint   nreturn_vals;
  gchar *result;

  return_vals = gimp_run_procedure ("gimp_temp_PDB_name",
				    &nreturn_vals,
				    GIMP_PDB_END);

  result = "temp_name_gen_failed";
  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    result = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gchar *
gimp_interactive_selection_brush (const gchar          *title, 
				  const gchar          *brush_name,
				  gdouble               opacity,
				  gint                  spacing,
				  GimpLayerModeEffects  paint_mode,
				  GimpRunBrushCallback  callback,
				  gpointer              data)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_STRING,    "str",           "String" },
    { GIMP_PDB_FLOAT,     "opacity",       "Opacity" },
    { GIMP_PDB_INT32,     "spacing",       "Spacing" },
    { GIMP_PDB_INT32,     "paint mode",    "Paint mode" },
    { GIMP_PDB_INT32,     "mask width",    "Brush width" },
    { GIMP_PDB_INT32,     "mask height"    "Brush heigth" },
    { GIMP_PDB_INT32,     "mask len",      "Length of brush mask data" },
    { GIMP_PDB_INT8ARRAY, "mask data",     "The brush mask data" },
    { GIMP_PDB_INT32,     "dialog status", "Registers if the dialog was closing "
                                        "[0 = No, 1 = Yes]" },
  };
  static GimpParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;
  gint bnreturn_vals;
  GimpParam *pdbreturn_vals;
  gchar *pdbname = gen_temp_plugin_name ();
  GimpBrushData *bdata;

  bdata = g_new0 (GimpBrushData, 1);

  gimp_install_temp_proc (pdbname,
			  "Temp PDB for interactive popups",
			  "More here later",
			  "Andy Thomas",
			  "Andy Thomas",
			  "1997",
			  NULL,
			  "RGB*, GRAY*",
			  GIMP_TEMPORARY,
			  nargs, nreturn_vals,
			  args, return_vals,
			  temp_brush_invoker);

  pdbreturn_vals =
    gimp_run_procedure ("gimp_brushes_popup",
			&bnreturn_vals,
			GIMP_PDB_STRING, pdbname,
			GIMP_PDB_STRING, title,
			GIMP_PDB_STRING, brush_name,
			GIMP_PDB_FLOAT,  opacity,
			GIMP_PDB_INT32,  spacing,
			GIMP_PDB_INT32,  paint_mode,
			GIMP_PDB_END);

  gimp_extension_enable (); /* Allow callbacks to be watched */

  gimp_destroy_params (pdbreturn_vals,bnreturn_vals);

  /* Now add to hash table so we can find it again */
  if (! gbrush_ht)
    gbrush_ht = g_hash_table_new (g_str_hash, g_str_equal);
  
  bdata->callback = callback;
  bdata->data = data;
  g_hash_table_insert (gbrush_ht, pdbname, bdata);

  return pdbname;
}

gchar *
gimp_interactive_selection_font (const gchar         *title, 
                                 const gchar         *font_name,
                                 GimpRunFontCallback  callback,
                                 gpointer             data)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_STRING, "str",           "String" },
    { GIMP_PDB_INT32,  "dialog status", "Registers if the dialog was closing "
                                        "[0 = No, 1 = Yes]" },
  };
  static GimpParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;
  gint fnreturn_vals;
  GimpParam *pdbreturn_vals;
  gchar *pdbname = gen_temp_plugin_name ();
  GimpFontData *fdata;

  fdata = g_new0 (GimpFontData, 1);

  gimp_install_temp_proc (pdbname,
			  "Temp PDB for interactive popups",
			  "More here later",
			  "Andy Thomas",
			  "Andy Thomas",
			  "1997",
			  NULL,
			  "RGB*, GRAY*",
			  GIMP_TEMPORARY,
			  nargs, nreturn_vals,
			  args, return_vals,
			  temp_font_invoker);

  pdbreturn_vals =
    gimp_run_procedure ("gimp_fonts_popup",
                        &fnreturn_vals,
                        GIMP_PDB_STRING, pdbname,
                        GIMP_PDB_STRING, title,
                        GIMP_PDB_STRING, font_name,
                        GIMP_PDB_END);

  gimp_extension_enable (); /* Allow callbacks to be watched */

  gimp_destroy_params (pdbreturn_vals, fnreturn_vals);

  /* Now add to hash table so we can find it again */
  if (! gfont_ht)
    gfont_ht = g_hash_table_new (g_str_hash, g_str_equal);

  fdata->callback = callback;
  fdata->data = data;
  g_hash_table_insert (gfont_ht, pdbname, fdata);

  return pdbname;
}

gchar *
gimp_interactive_selection_gradient (const gchar             *title, 
				     const gchar             *gradient_name,
				     gint                     sample_size,
				     GimpRunGradientCallback  callback,
				     gpointer                 data)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_STRING,    "str",           "String" },
    { GIMP_PDB_INT32,     "grad width",    "Gradient width" },
    { GIMP_PDB_FLOATARRAY,"grad data",     "The gradient mask data" },
    { GIMP_PDB_INT32,     "dialog status", "Registers if the dialog was closing "
                                        "[0 = No, 1 = Yes]" },
  };
  static GimpParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;
  gint    bnreturn_vals;
  GimpParam *pdbreturn_vals;
  gchar  *pdbname = gen_temp_plugin_name();
  GimpGradientData *gdata;

  gdata = g_new0 (GimpGradientData, 1);

  gimp_install_temp_proc (pdbname,
			  "Temp PDB for interactive popups",
			  "More here later",
			  "Andy Thomas",
			  "Andy Thomas",
			  "1997",
			  NULL,
			  "RGB*, GRAY*",
			  GIMP_TEMPORARY,
			  nargs, nreturn_vals,
			  args, return_vals,
			  temp_gradient_invoker);

  pdbreturn_vals =
    gimp_run_procedure ("gimp_gradients_popup",
			&bnreturn_vals,
			GIMP_PDB_STRING, pdbname,
			GIMP_PDB_STRING, title,
			GIMP_PDB_STRING, gradient_name,
			GIMP_PDB_INT32,  sample_size,
			GIMP_PDB_END);

  gimp_extension_enable (); /* Allow callbacks to be watched */

  gimp_destroy_params (pdbreturn_vals,bnreturn_vals);

  /* Now add to hash table so we can find it again */
  if (! ggradient_ht)
    ggradient_ht = g_hash_table_new (g_str_hash, g_str_equal);

  gdata->callback = callback;
  gdata->data = data;
  g_hash_table_insert (ggradient_ht, pdbname,gdata);

  return pdbname;
}

gchar *
gimp_interactive_selection_pattern (const gchar            *title,
				    const gchar            *pattern_name,
				    GimpRunPatternCallback  callback,
				    gpointer                data)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_STRING,   "str",           "String" },
    { GIMP_PDB_INT32,    "mask width",    "Pattern width" },
    { GIMP_PDB_INT32,    "mask height",   "Pattern heigth" },
    { GIMP_PDB_INT32,    "mask bpp",      "Pattern bytes per pixel" },
    { GIMP_PDB_INT32,    "mask len",      "Length of pattern mask data" },
    { GIMP_PDB_INT8ARRAY,"mask data",     "The pattern mask data" },
    { GIMP_PDB_INT32,    "dialog status", "Registers if the dialog was closing "
                                       "[0 = No, 1 = Yes]" },
  };
  static GimpParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;
  gint bnreturn_vals;
  GimpParam *pdbreturn_vals;
  gchar *pdbname = gen_temp_plugin_name ();
  GimpPatternData *pdata;

  pdata = g_new0 (GimpPatternData, 1);

  gimp_install_temp_proc (pdbname,
			  "Temp PDB for interactive popups",
			  "More here later",
			  "Andy Thomas",
			  "Andy Thomas",
			  "1997",
			  NULL,
			  "RGB*, GRAY*",
			  GIMP_TEMPORARY,
			  nargs, nreturn_vals,
			  args, return_vals,
			  temp_pattern_invoker);

  pdbreturn_vals =
    gimp_run_procedure("gimp_patterns_popup",
		       &bnreturn_vals,
		       GIMP_PDB_STRING, pdbname,
		       GIMP_PDB_STRING, title,
		       GIMP_PDB_STRING, pattern_name,
		       GIMP_PDB_END);

  gimp_extension_enable (); /* Allow callbacks to be watched */

  gimp_destroy_params (pdbreturn_vals, bnreturn_vals);

  /* Now add to hash table so we can find it again */
  if (! gpattern_ht)
    gpattern_ht = g_hash_table_new (g_str_hash, g_str_equal);

  pdata->callback = callback;
  pdata->data = data;
  g_hash_table_insert (gpattern_ht, pdbname,pdata);

  return pdbname;
}
