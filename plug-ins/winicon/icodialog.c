/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#include <config.h>

#include <stdio.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#define ICO_DBG

#include "icodialog.h"
#include "main.h"

#include "libgimp/stdplugins-intl.h"


static GtkWidget *ico_preview_new             (gint32     layer);
static void       ico_fill_preview_with_thumb (GtkWidget *widget,
                                               gint32     drawable_ID);
static void       combo_bpp_changed           (GtkWidget *combo,
                                               GObject   *hbox);


static GtkWidget *
ico_preview_new(gint32 layer)
{
  GtkWidget *icon_preview;

  icon_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  ico_fill_preview_with_thumb (icon_preview, layer);

  return icon_preview;
}


static void
ico_fill_preview_with_thumb (GtkWidget *widget,
			     gint32     drawable_ID)
{
  guchar  *drawable_data;
  gint     bpp;
  gint     x,y;
  gint     width;
  gint     height;
  guchar  *src;
  gdouble  r, g, b, a;
  gdouble  c0, c1;
  guchar  *p0, *p1;
  guchar  *even, *odd;

  width  = gimp_drawable_width (drawable_ID);
  height = gimp_drawable_height (drawable_ID);
  bpp    = 0; /* Only returned */

  if (width > 128)
    width = 128;
  if (height > 128)
    height = 128;

  drawable_data =
    gimp_drawable_get_thumbnail_data (drawable_ID, &width, &height, &bpp);

  if (width < 1 || height < 1)
    return;

  gtk_preview_size (GTK_PREVIEW (widget), width, height);

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);
  src = drawable_data;

  for (y = 0; y < height; y++)
    {
      p0 = even;
      p1 = odd;

      for (x = 0; x < width; x++)
	{
	  if (bpp == 4)
	    {
	      r = ((gdouble) src[x*4 + 0]) / 255.0;
	      g = ((gdouble) src[x*4 + 1]) / 255.0;
	      b = ((gdouble) src[x*4 + 2]) / 255.0;
	      a = ((gdouble) src[x*4 + 3]) / 255.0;
	    }
	  else if (bpp == 3)
	    {
	      r = ((gdouble) src[x*3 + 0]) / 255.0;
	      g = ((gdouble) src[x*3 + 1]) / 255.0;
	      b = ((gdouble) src[x*3 + 2]) / 255.0;
	      a = 1.0;
	    }
	  else
	    {
	      r = ((gdouble) src[x*bpp + 0]) / 255.0;
	      g = b = r;
	      if (bpp == 2)
		a = ((gdouble) src[x*2 + 1]) / 255.0;
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
  g_free (drawable_data);
}


/* This function creates and returns an hbox for an icon,
   which then gets added to the dialog's main vbox. */
static GtkWidget*
ico_create_icon_hbox (GtkWidget *icon_preview,
		      gint32     layer,
                      gint       layer_num)
{
  GtkWidget *hbox;
  GtkWidget *combo;

  hbox = gtk_hbox_new (FALSE, 0);

  /* To make life easier for the callbacks, we store the
     layer's ID and stacking number with the hbox. */

  g_object_set_data (G_OBJECT (hbox),
                     "icon_layer", GINT_TO_POINTER (layer));
  g_object_set_data (G_OBJECT (hbox),
                     "icon_layer_num", GINT_TO_POINTER (layer_num));

  g_object_set_data (G_OBJECT (hbox), "icon_preview", icon_preview);
  gtk_widget_show (icon_preview);
  gtk_box_pack_start (GTK_BOX (hbox), icon_preview, TRUE, TRUE, 0);

  combo = gimp_int_combo_box_new (_("1 bpp, 1-bit alpha, 2-slot palette"),   1,
                                  _("4 bpp, 1-bit alpha, 16-slot palette"),  4,
                                  _("8 bpp, 1-bit alpha, 256-slot palette"), 8,
                                  _("32 bpp, 8-bit alpha, no palette"),      32,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), 32);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (combo_bpp_changed),
                    hbox);

  g_object_set_data (G_OBJECT (hbox), "icon_menu", combo);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  return hbox;
}


GtkWidget*
ico_specs_dialog_new (gint num_layers)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *scrolledwindow;
  GtkWidget *viewport;
  gint      *icon_depths, i;

  dialog = gimp_dialog_new (_("GIMP Windows Icon Plugin"), "winicon",
			     NULL, 0,
			     gimp_standard_help_func, "plug-in-winicon",
			     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			     GTK_STOCK_OK,     GTK_RESPONSE_OK,
			     NULL);

  /* We store an array that holds each icon's requested bit depth
     with the dialog. It's queried when the dialog is closed so the
     save routine knows what colormaps etc to generate in the saved
     file. We store twice the number necessary because in the second
     set, the color depths that are automatically suggested are stored
     for later comparison.
  */

  icon_depths = g_new (gint, 2 * num_layers);
  for (i = 0; i < 2 * num_layers; i++)
    icon_depths[i] = 32;

  g_object_set_data (G_OBJECT (dialog), "icon_depths", icon_depths);

  vbox = GTK_DIALOG (dialog)->vbox;

  frame = gtk_frame_new (_("Icon details"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
  gtk_widget_show (frame);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow), 5);
  gtk_container_add (GTK_CONTAINER (frame), scrolledwindow);
  gtk_widget_show (scrolledwindow);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), viewport);
  gtk_widget_show (viewport);

  vbox = gtk_vbox_new (TRUE, 0);
  g_object_set_data (G_OBJECT (dialog), "icons_vbox", vbox);
  gtk_container_add (GTK_CONTAINER (viewport), vbox);
  gtk_widget_show (vbox);

  return dialog;
}

static GtkWidget *
ico_specs_dialog_get_layer_preview (GtkWidget *dialog,
                                    gint32     layer)
{
  GtkWidget *preview;
  GtkWidget *icon_hbox;
  gchar      key[MAXLEN];

  g_snprintf (key, MAXLEN, "layer_%i_hbox", layer);
  icon_hbox = g_object_get_data (G_OBJECT (dialog), key);

  if (!icon_hbox)
    {
      D(("Something's wrong -- couldn't look up hbox by layer ID\n"));
      return NULL;
    }

  preview = g_object_get_data (G_OBJECT (icon_hbox), "icon_preview");

  if (!icon_hbox)
    {
      D(("Something's wrong -- couldn't look up preview from hbox\n"));
      return NULL;
    }

  return preview;
}


void
ico_specs_dialog_add_icon (GtkWidget *dialog,
                           gint32     layer,
                           gint       layer_num)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *preview;
  gchar      key[MAXLEN];

  vbox = g_object_get_data (G_OBJECT (dialog), "icons_vbox");
  preview = ico_preview_new (layer);
  hbox = ico_create_icon_hbox (preview, layer, layer_num);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 2);
  gtk_widget_show (hbox);

  /* Let's make the hbox accessible through the layer ID */
  g_snprintf (key, MAXLEN, "layer_%i_hbox", layer);
  g_object_set_data (G_OBJECT (dialog), key, hbox);
}


void
ico_specs_dialog_update_icon_preview (GtkWidget *dialog,
                                      gint32     layer,
                                      gint       bpp)
{
  GtkWidget    *preview;
  GimpPixelRgn  src_pixel_rgn, dst_pixel_rgn;
  gint32        tmp_image;
  gint32        tmp_layer;
  gint          w, h;
  guchar       *buffer;
  gboolean      result;

  tmp_image = gimp_image_new (gimp_drawable_width (layer),
                              gimp_drawable_height (layer),
                              GIMP_RGB);

  w = gimp_drawable_width (layer);
  h = gimp_drawable_height (layer);

  tmp_layer = gimp_layer_new (tmp_image, "temporary", w, h,
                              GIMP_RGBA_IMAGE, 100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (tmp_image, tmp_layer, 0);

  gimp_pixel_rgn_init (&src_pixel_rgn, gimp_drawable_get (layer),
		       0, 0, w, h, TRUE, FALSE);
  gimp_pixel_rgn_init (&dst_pixel_rgn, gimp_drawable_get (tmp_layer),
		       0, 0, w, h, TRUE, FALSE);

  buffer = g_malloc (w * h * 4);
  gimp_pixel_rgn_get_rect (&src_pixel_rgn, buffer, 0, 0, w, h);
  gimp_pixel_rgn_set_rect (&dst_pixel_rgn, buffer, 0, 0, w, h);

  if (bpp < 32)
    {
      result = gimp_image_convert_indexed (tmp_image,
                                           GIMP_FS_DITHER,
                                           GIMP_MAKE_PALETTE,
                                           1 << bpp,
                                           TRUE,
                                           FALSE,
                                           "dummy");
    }

  preview = ico_specs_dialog_get_layer_preview (dialog, layer);
  ico_fill_preview_with_thumb (preview, tmp_layer);
  gtk_widget_queue_draw (preview);

  gimp_image_delete (tmp_image);
  g_free (buffer);
}

static void
combo_bpp_changed (GtkWidget *combo,
                   GObject   *hbox)
{
  GtkWidget *dialog = gtk_widget_get_toplevel (combo);
  gint32     layer;
  gint       layer_num;
  gint       bpp;
  gint      *icon_depths;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &bpp);

  icon_depths = g_object_get_data (G_OBJECT (dialog), "icon_depths");
  if (! icon_depths)
    {
      D(("Something's wrong -- can't get icon_depths array from dialog\n"));
      return;
    }

  layer     = GPOINTER_TO_INT (g_object_get_data (hbox, "icon_layer"));
  layer_num = GPOINTER_TO_INT (g_object_get_data (hbox, "icon_layer_num"));

  /* Update vector entry for later when we're actually saving,
     and update the preview right away ... */
  icon_depths[layer_num] = bpp;
  ico_specs_dialog_update_icon_preview (dialog, layer, bpp);
}
