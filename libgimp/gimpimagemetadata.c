/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpimagemetadata.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <sys/time.h>

#include <gtk/gtk.h>
#include <gexiv2/gexiv2.h>

#include "gimp.h"
#include "gimpui.h"
#include "gimpimagemetadata.h"

#include "libgimp-intl.h"


typedef struct
{
  gchar *tag;
  gint  type;
} XmpStructs;


static void        gimp_image_metadata_rotate        (GimpImage         *image,
                                                      GExiv2Orientation  orientation);
static GdkPixbuf * gimp_image_metadata_rotate_pixbuf (GdkPixbuf         *pixbuf,
                                                      GExiv2Orientation  orientation);
static void        gimp_image_metadata_rotate_query  (GimpImage         *image,
                                                      const gchar       *mime_type,
                                                      GimpMetadata      *metadata,
                                                      gboolean           interactive);
static gboolean    gimp_image_metadata_rotate_dialog (GimpImage         *image,
                                                      GExiv2Orientation  orientation,
                                                      const gchar       *parasite_name);


/*  public functions  */

/**
 * gimp_image_metadata_load_prepare:
 * @image:     The image
 * @mime_type: The loaded file's mime-type
 * @file:      The file to load the metadata from
 * @error:     Return location for error
 *
 * Loads and returns metadata from @file to be passed into
 * gimp_image_metadata_load_finish().
 *
 * Returns: (transfer full): The file's metadata.
 *
 * Since: 2.10
 */
GimpMetadata *
gimp_image_metadata_load_prepare (GimpImage    *image,
                                  const gchar  *mime_type,
                                  GFile        *file,
                                  GError      **error)
{
  GimpMetadata *metadata;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  metadata = gimp_metadata_load_from_file (file, error);

  if (metadata)
    {
      gexiv2_metadata_erase_exif_thumbnail (GEXIV2_METADATA (metadata));
    }

  return metadata;
}

/**
 * gimp_image_metadata_load_finish:
 * @image:       The image
 * @mime_type:   The loaded file's mime-type
 * @metadata:    The metadata to set on the image
 * @flags:       Flags to specify what of the metadata to apply to the image
 * @interactive: Whether this function is allowed to query info with dialogs
 *
 * Applies the @metadata previously loaded with
 * gimp_image_metadata_load_prepare() to the image, taking into account
 * the passed @flags.
 *
 * Since: 2.10
 */
void
gimp_image_metadata_load_finish (GimpImage             *image,
                                 const gchar           *mime_type,
                                 GimpMetadata          *metadata,
                                 GimpMetadataLoadFlags  flags,
                                 gboolean               interactive)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (mime_type != NULL);
  g_return_if_fail (GEXIV2_IS_METADATA (metadata));

  if (flags & GIMP_METADATA_LOAD_COMMENT)
    {
      gchar *comment;

      comment = gexiv2_metadata_get_tag_interpreted_string (GEXIV2_METADATA (metadata),
                                                            "Exif.Photo.UserComment");
      if (! comment)
        comment = gexiv2_metadata_get_tag_interpreted_string (GEXIV2_METADATA (metadata),
                                                              "Exif.Image.ImageDescription");

      if (comment)
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("gimp-comment",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (comment) + 1,
                                        comment);
          g_free (comment);

          gimp_image_attach_parasite (image, parasite);
          gimp_parasite_free (parasite);
        }
    }

  if (flags & GIMP_METADATA_LOAD_RESOLUTION)
    {
      gdouble   xres;
      gdouble   yres;
      GimpUnit  unit;

      if (gimp_metadata_get_resolution (metadata, &xres, &yres, &unit))
        {
          gimp_image_set_resolution (image, xres, yres);
          gimp_image_set_unit (image, unit);
        }
    }

  if (flags & GIMP_METADATA_LOAD_ORIENTATION)
    {
      gimp_image_metadata_rotate_query (image, mime_type,
                                        metadata, interactive);
    }

  if (flags & GIMP_METADATA_LOAD_COLORSPACE)
    {
      GimpColorProfile *profile = gimp_image_get_color_profile (image);

      /* only look for colorspace information from metadata if the
       * image didn't contain an embedded color profile
       */
      if (! profile)
        {
          GimpMetadataColorspace colorspace;

          colorspace = gimp_metadata_get_colorspace (metadata);

          switch (colorspace)
            {
            case GIMP_METADATA_COLORSPACE_UNSPECIFIED:
            case GIMP_METADATA_COLORSPACE_UNCALIBRATED:
            case GIMP_METADATA_COLORSPACE_SRGB:
              /* use sRGB, a NULL profile will do the right thing  */
              break;

            case GIMP_METADATA_COLORSPACE_ADOBERGB:
              profile = gimp_color_profile_new_rgb_adobe ();
              break;
            }

          if (profile)
            gimp_image_set_color_profile (image, profile);
        }

      if (profile)
        g_object_unref (profile);
    }

  gimp_image_set_metadata (image, metadata);
}

/**
 * gimp_image_metadata_load_thumbnail:
 * @file:  A #GFile image
 * @error: Return location for error message
 *
 * Retrieves a thumbnail from metadata if present.
 *
 * Returns: (transfer none) (nullable): a #GimpImage of the @file thumbnail.
 *
 * Since: 2.10
 */
GimpImage *
gimp_image_metadata_load_thumbnail (GFile   *file,
                                    GError **error)
{
  GimpMetadata *metadata;
  GInputStream *input_stream;
  GdkPixbuf    *pixbuf;
  guint8       *thumbnail_buffer;
  gint          thumbnail_size;
  GimpImage    *image = NULL;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  metadata = gimp_metadata_load_from_file (file, error);
  if (! metadata)
    return NULL;

  if (! gexiv2_metadata_get_exif_thumbnail (GEXIV2_METADATA (metadata),
                                            &thumbnail_buffer,
                                            &thumbnail_size))
    {
      g_object_unref (metadata);
      return NULL;
    }

  input_stream = g_memory_input_stream_new_from_data (thumbnail_buffer,
                                                      thumbnail_size,
                                                      (GDestroyNotify) g_free);
  pixbuf = gdk_pixbuf_new_from_stream (input_stream, NULL, error);
  g_object_unref (input_stream);

  if (pixbuf)
    {
      GimpLayer *layer;

      image = gimp_image_new (gdk_pixbuf_get_width  (pixbuf),
                              gdk_pixbuf_get_height (pixbuf),
                              GIMP_RGB);
      gimp_image_undo_disable (image);

      layer = gimp_layer_new_from_pixbuf (image, _("Background"),
                                          pixbuf,
                                          100.0,
                                          gimp_image_get_default_new_layer_mode (image),
                                          0.0, 0.0);
      g_object_unref (pixbuf);

      gimp_image_insert_layer (image, layer, NULL, 0);

      gimp_image_metadata_rotate (image,
                                  gexiv2_metadata_get_orientation (GEXIV2_METADATA (metadata)));
    }

  g_object_unref (metadata);

  return image;
}


/*  private functions  */

static void
gimp_image_metadata_rotate (GimpImage         *image,
                            GExiv2Orientation  orientation)
{
  switch (orientation)
    {
    case GEXIV2_ORIENTATION_UNSPECIFIED:
    case GEXIV2_ORIENTATION_NORMAL:  /* standard orientation, do nothing */
      break;

    case GEXIV2_ORIENTATION_HFLIP:
      gimp_image_flip (image, GIMP_ORIENTATION_HORIZONTAL);
      break;

    case GEXIV2_ORIENTATION_ROT_180:
      gimp_image_rotate (image, GIMP_ROTATE_180);
      break;

    case GEXIV2_ORIENTATION_VFLIP:
      gimp_image_flip (image, GIMP_ORIENTATION_VERTICAL);
      break;

    case GEXIV2_ORIENTATION_ROT_90_HFLIP:  /* flipped diagonally around '\' */
      gimp_image_rotate (image, GIMP_ROTATE_90);
      gimp_image_flip (image, GIMP_ORIENTATION_HORIZONTAL);
      break;

    case GEXIV2_ORIENTATION_ROT_90:  /* 90 CW */
      gimp_image_rotate (image, GIMP_ROTATE_90);
      break;

    case GEXIV2_ORIENTATION_ROT_90_VFLIP:  /* flipped diagonally around '/' */
      gimp_image_rotate (image, GIMP_ROTATE_90);
      gimp_image_flip (image, GIMP_ORIENTATION_VERTICAL);
      break;

    case GEXIV2_ORIENTATION_ROT_270:  /* 90 CCW */
      gimp_image_rotate (image, GIMP_ROTATE_270);
      break;

    default: /* shouldn't happen */
      break;
    }
}

static GdkPixbuf *
gimp_image_metadata_rotate_pixbuf (GdkPixbuf         *pixbuf,
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

static void
gimp_image_metadata_rotate_query (GimpImage    *image,
                                  const gchar  *mime_type,
                                  GimpMetadata *metadata,
                                  gboolean      interactive)
{
  GimpParasite      *parasite;
  gchar             *parasite_name;
  GExiv2Orientation  orientation;
  gboolean           query = interactive;

  orientation = gexiv2_metadata_get_orientation (GEXIV2_METADATA (metadata));

  if (orientation <= GEXIV2_ORIENTATION_NORMAL ||
      orientation >  GEXIV2_ORIENTATION_MAX)
    return;

  parasite_name = g_strdup_printf ("gimp-metadata-exif-rotate(%s)", mime_type);

  parasite = gimp_get_parasite (parasite_name);

  if (parasite)
    {
      if (strncmp (gimp_parasite_data (parasite), "yes",
                   gimp_parasite_data_size (parasite)) == 0)
        {
          query = FALSE;
        }
      else if (strncmp (gimp_parasite_data (parasite), "no",
                        gimp_parasite_data_size (parasite)) == 0)
        {
          gimp_parasite_free (parasite);
          g_free (parasite_name);
          return;
        }

      gimp_parasite_free (parasite);
    }

  if (query && ! gimp_image_metadata_rotate_dialog (image,
                                                    orientation,
                                                    parasite_name))
    {
      g_free (parasite_name);
      return;
    }

  g_free (parasite_name);

  gimp_image_metadata_rotate (image, orientation);
  gexiv2_metadata_set_orientation (GEXIV2_METADATA (metadata),
                                   GEXIV2_ORIENTATION_NORMAL);
}

static gboolean
gimp_image_metadata_rotate_dialog (GimpImage         *image,
                                   GExiv2Orientation  orientation,
                                   const gchar       *parasite_name)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *toggle;
  GdkPixbuf *pixbuf;
  gchar     *name;
  gchar     *title;
  gint       response;

  name = gimp_image_get_name (image);
  title = g_strdup_printf (_("Rotate %s?"), name);
  g_free (name);

  dialog = gimp_dialog_new (title, "gimp-metadata-rotate-dialog",
                            NULL, 0, NULL, NULL,

                            _("_Keep Original"), GTK_RESPONSE_CANCEL,
                            _("_Rotate"),        GTK_RESPONSE_OK,

                            NULL);

  g_free (title);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

#define THUMBNAIL_SIZE 128

  pixbuf = gimp_image_get_thumbnail (image,
                                     THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                     GIMP_PIXBUF_SMALL_CHECKS);

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
      gimp_label_set_attributes (GTK_LABEL (label),
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
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE,  PANGO_STYLE_ITALIC,
                                 -1);
      gtk_box_pack_end (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      rotated = gimp_image_metadata_rotate_pixbuf (pixbuf, orientation);
      g_object_unref (pixbuf);

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
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  /* eek */
  gtk_widget_set_size_request (GTK_WIDGET (label),
                               2 * THUMBNAIL_SIZE + 12, -1);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   _("Would you like to rotate the image?"),
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.5,
                        NULL);
  /* eek */
  gtk_widget_set_size_request (GTK_WIDGET (label),
                               2 * THUMBNAIL_SIZE + 12, -1);
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  toggle = gtk_check_button_new_with_mnemonic (_("_Don't ask me again"));
  gtk_box_pack_end (GTK_BOX (main_vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
  gtk_widget_show (toggle);

  response = gimp_dialog_run (GIMP_DIALOG (dialog));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)))
    {
      GimpParasite *parasite;
      const gchar  *str = (response == GTK_RESPONSE_OK) ? "yes" : "no";

      parasite = gimp_parasite_new (parasite_name,
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (str), str);
      gimp_attach_parasite (parasite);
      gimp_parasite_free (parasite);
    }

  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_OK);
}
