/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * metadata-rotation-import-dialog.h
 * Copyright (C) 2020 Jehan
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gexiv2/gexiv2.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-metadata.h"
#include "core/ligmapickable.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaviewabledialog.h"
#include "widgets/ligmawidgets-constructors.h"

#include "metadata-rotation-import-dialog.h"

#include "ligma-intl.h"


static LigmaMetadataRotationPolicy   ligma_image_metadata_rotate_dialog (LigmaImage         *image,
                                                                       LigmaContext       *context,
                                                                       GExiv2Orientation  orientation,
                                                                       gboolean          *dont_ask);
static GdkPixbuf                  * ligma_image_metadata_rotate_pixbuf (GdkPixbuf         *pixbuf,
                                                                       GExiv2Orientation  orientation);


/*  public functions  */

LigmaMetadataRotationPolicy
metadata_rotation_import_dialog_run (LigmaImage   *image,
                                     LigmaContext *context,
                                     GtkWidget   *parent,
                                     gboolean    *dont_ask)
{
  LigmaMetadata      *metadata;
  GExiv2Orientation  orientation;

  metadata    = ligma_image_get_metadata (image);
  orientation = gexiv2_metadata_get_orientation (GEXIV2_METADATA (metadata));

  if (orientation <= GEXIV2_ORIENTATION_NORMAL ||
      orientation >  GEXIV2_ORIENTATION_MAX)
    return LIGMA_METADATA_ROTATION_POLICY_KEEP;

  return ligma_image_metadata_rotate_dialog (image, context, orientation, dont_ask);
}

static LigmaMetadataRotationPolicy
ligma_image_metadata_rotate_dialog (LigmaImage         *image,
                                   LigmaContext       *context,
                                   GExiv2Orientation  orientation,
                                   gboolean          *dont_ask)
{
  GtkWidget   *dialog;
  GtkWidget   *main_vbox;
  GtkWidget   *vbox;
  GtkWidget   *label;
  GtkWidget   *toggle;
  const gchar *name;
  gchar       *title;
  GdkPixbuf   *pixbuf;
  gint         width;
  gint         scale_factor;
  gint         height;
  gint         response;

  name = ligma_image_get_display_name (image);
  title = g_strdup_printf (_("Rotate %s?"), name);

  dialog = ligma_dialog_new (title, "ligma-metadata-rotate-dialog",
                            NULL, 0, NULL, NULL,

                            _("_Keep Original"), GTK_RESPONSE_CANCEL,
                            _("_Rotate"),        GTK_RESPONSE_OK,

                            NULL);

  g_free (title);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  scale_factor = gtk_widget_get_scale_factor (main_vbox);
  width        = ligma_image_get_width  (image);
  height       = ligma_image_get_height (image);

#define MAX_THUMBNAIL_SIZE (128 * scale_factor)
  if (width > MAX_THUMBNAIL_SIZE || height > MAX_THUMBNAIL_SIZE)
    {
      /* Adjust the width/height ratio to a maximum size (relative to
       * current display scale factor.
       */
      if (width > height)
        {
          height = MAX_THUMBNAIL_SIZE * height / width;
          width  = MAX_THUMBNAIL_SIZE;
        }
      else
        {
          width  = MAX_THUMBNAIL_SIZE * width / height;
          height = MAX_THUMBNAIL_SIZE;
        }
    }

  ligma_pickable_flush (LIGMA_PICKABLE (image));
  pixbuf = ligma_viewable_get_pixbuf (LIGMA_VIEWABLE (image), context,
                                     width, height);
  if (pixbuf)
    {
      GdkPixbuf *rotated;
      GtkWidget *hbox;
      GtkWidget *image;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
      gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
      gtk_widget_show (vbox);

      label = gtk_label_new (_("Original"));
      gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
      ligma_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE,  PANGO_STYLE_ITALIC,
                                 -1);
      gtk_box_pack_end (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      image = gtk_image_new_from_pixbuf (pixbuf);
      gtk_box_pack_end (GTK_BOX (vbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
      gtk_widget_show (vbox);

      label = gtk_label_new (_("Rotated"));
      ligma_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE,  PANGO_STYLE_ITALIC,
                                 -1);
      gtk_box_pack_end (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      rotated = ligma_image_metadata_rotate_pixbuf (pixbuf, orientation);

      image = gtk_image_new_from_pixbuf (rotated);
      g_object_unref (rotated);

      gtk_box_pack_end (GTK_BOX (vbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);
    }

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   _("This image contains Exif orientation "
                                     "metadata."),
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.5,
                        NULL);
  ligma_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   _("Would you like to rotate the image?"),
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.5,
                        NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  toggle = gtk_check_button_new_with_mnemonic (_("_Don't ask me again"));
  gtk_box_pack_end (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
  gtk_widget_show (toggle);

  response  = ligma_dialog_run (LIGMA_DIALOG (dialog));
  *dont_ask = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)));

  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_OK) ? LIGMA_METADATA_ROTATION_POLICY_ROTATE : LIGMA_COLOR_PROFILE_POLICY_KEEP;
}

static GdkPixbuf *
ligma_image_metadata_rotate_pixbuf (GdkPixbuf         *pixbuf,
                                   GExiv2Orientation  orientation)
{
  GdkPixbuf *rotated = NULL;
  GdkPixbuf *temp;

  switch (orientation)
    {
    case GEXIV2_ORIENTATION_UNSPECIFIED:
    case GEXIV2_ORIENTATION_NORMAL:  /* standard orientation, do nothing */
      rotated = g_object_ref (pixbuf);
      break;

    case GEXIV2_ORIENTATION_HFLIP:
      rotated = gdk_pixbuf_flip (pixbuf, TRUE);
      break;

    case GEXIV2_ORIENTATION_ROT_180:
      rotated = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
      break;

    case GEXIV2_ORIENTATION_VFLIP:
      rotated = gdk_pixbuf_flip (pixbuf, FALSE);
      break;

    case GEXIV2_ORIENTATION_ROT_90_HFLIP:  /* flipped diagonally around '\' */
      temp = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE);
      rotated = gdk_pixbuf_flip (temp, TRUE);
      g_object_unref (temp);
      break;

    case GEXIV2_ORIENTATION_ROT_90:  /* 90 CW */
      rotated = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE);
      break;

    case GEXIV2_ORIENTATION_ROT_90_VFLIP:  /* flipped diagonally around '/' */
      temp = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_CLOCKWISE);
      rotated = gdk_pixbuf_flip (temp, FALSE);
      g_object_unref (temp);
      break;

    case GEXIV2_ORIENTATION_ROT_270:  /* 90 CCW */
      rotated = gdk_pixbuf_rotate_simple (pixbuf, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
      break;

    default: /* shouldn't happen */
      break;
    }

  return rotated;
}
