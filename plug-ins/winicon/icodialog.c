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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#define ICO_DBG

#include "icodialog.h"
#include "main.h"

#include "libgimp/stdplugins-intl.h"


static GtkWidget * ico_preview_new   (gint32     layer);
static void        combo_bpp_changed (GtkWidget *combo,
                                      GObject   *hbox);


static GtkWidget *
ico_preview_new (gint32 layer)
{
  GtkWidget *image;
  GdkPixbuf *pixbuf;
  gint       width  = gimp_drawable_width (layer);
  gint       height = gimp_drawable_height (layer);

  pixbuf = gimp_drawable_get_thumbnail (layer,
                                        MIN (width, 128), MIN (height, 128),
                                        GIMP_PIXBUF_SMALL_CHECKS);
  image = gtk_image_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);

  return image;
}


/* This function creates and returns an hbox for an icon,
   which then gets added to the dialog's main vbox. */
static GtkWidget *
ico_create_icon_hbox (GtkWidget *icon_preview,
                      gint32     layer,
                      gint       layer_num)
{
  static GtkSizeGroup *size = NULL;

  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *alignment;
  GtkWidget *combo;

  hbox = gtk_hbox_new (FALSE, 6);

  alignment = gtk_alignment_new (1.0, 0.5, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 0);
  gtk_widget_show (alignment);

  /* To make life easier for the callbacks, we store the
     layer's ID and stacking number with the hbox. */

  g_object_set_data (G_OBJECT (hbox),
                     "icon_layer", GINT_TO_POINTER (layer));
  g_object_set_data (G_OBJECT (hbox),
                     "icon_layer_num", GINT_TO_POINTER (layer_num));

  g_object_set_data (G_OBJECT (hbox), "icon_preview", icon_preview);
  gtk_container_add (GTK_CONTAINER (alignment), icon_preview);
  gtk_widget_show (icon_preview);

  if (! size)
    size = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  gtk_size_group_add_widget (size, alignment);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

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

  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  return hbox;
}


GtkWidget *
ico_specs_dialog_new (gint num_layers)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *scrolledwindow;
  gint      *icon_depths, i;

  dialog = gimp_dialog_new (_("Save as Windows Icon"), "winicon",
                            NULL, 0,
                            gimp_standard_help_func, "plug-in-winicon",

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

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

  frame = gimp_frame_new (_("Icon Details"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame,
                      TRUE, TRUE, 0);
  gtk_widget_show (frame);

  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (frame), scrolledwindow);
  gtk_widget_show (scrolledwindow);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  g_object_set_data (G_OBJECT (dialog), "icons_vbox", vbox);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindow),
                                         vbox);
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

  g_snprintf (key, sizeof (key), "layer_%i_hbox", layer);
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
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Let's make the hbox accessible through the layer ID */
  g_snprintf (key, sizeof (key), "layer_%i_hbox", layer);
  g_object_set_data (G_OBJECT (dialog), key, hbox);
}

void
ico_specs_dialog_update_icon_preview (GtkWidget *dialog,
                                      gint32     layer,
                                      gint       bpp)
{
  GtkWidget *preview = ico_specs_dialog_get_layer_preview (dialog, layer);
  GdkPixbuf *pixbuf;
  gint       w       = gimp_drawable_width (layer);
  gint       h       = gimp_drawable_height (layer);

  if (! preview)
    return;

  if (bpp <= 8)
    {
      GimpDrawable *drawable;
      GimpDrawable *tmp;
      GimpPixelRgn  src_pixel_rgn, dst_pixel_rgn;
      gint32        image;
      gint32        tmp_image;
      gint32        tmp_layer;
      guchar       *buffer;

      image = gimp_drawable_get_image (layer);

      tmp_image = gimp_image_new (w, h, gimp_image_base_type (image));
      gimp_image_undo_disable (tmp_image);

      if (gimp_drawable_is_indexed (layer))
        {
          guchar *cmap;
          gint    num_colors;

          cmap = gimp_image_get_colormap (image, &num_colors);
          gimp_image_set_colormap (tmp_image, cmap, num_colors);
          g_free (cmap);
        }

      tmp_layer = gimp_layer_new (tmp_image, "temporary", w, h,
                                  gimp_drawable_type (layer),
                                  100, GIMP_NORMAL_MODE);
      gimp_image_add_layer (tmp_image, tmp_layer, 0);

      drawable = gimp_drawable_get (layer);
      tmp      = gimp_drawable_get (tmp_layer);

      gimp_pixel_rgn_init (&src_pixel_rgn, drawable, 0, 0, w, h, FALSE, FALSE);
      gimp_pixel_rgn_init (&dst_pixel_rgn, tmp,      0, 0, w, h, TRUE, FALSE);

      buffer = g_malloc (w * h * 4);
      gimp_pixel_rgn_get_rect (&src_pixel_rgn, buffer, 0, 0, w, h);
      gimp_pixel_rgn_set_rect (&dst_pixel_rgn, buffer, 0, 0, w, h);
      g_free (buffer);

      gimp_drawable_detach (tmp);
      gimp_drawable_detach (drawable);

      if (gimp_drawable_is_indexed (layer))
        gimp_image_convert_rgb (tmp_image);

      gimp_image_convert_indexed (tmp_image,
                                  GIMP_FS_DITHER, GIMP_MAKE_PALETTE,
                                  1 << bpp, TRUE, FALSE, "dummy");

      pixbuf = gimp_drawable_get_thumbnail (tmp_layer,
                                            MIN (w, 128), MIN (h, 128),
                                            GIMP_PIXBUF_SMALL_CHECKS);

      gimp_image_delete (tmp_image);
    }
  else
    {
      pixbuf = gimp_drawable_get_thumbnail (layer,
                                            MIN (w, 128), MIN (h, 128),
                                            GIMP_PIXBUF_SMALL_CHECKS);
    }

  gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);
  g_object_unref (pixbuf);
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
