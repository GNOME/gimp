/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpmetadata.c
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>
#include <gexiv2/gexiv2.h>

#include "gimp.h"
#include "gimpui.h"
#include "gimpmetadata.h"

#include "libgimp-intl.h"


static void     gimp_image_metadata_rotate        (gint32             image_ID,
                                                   GExiv2Orientation  orientation);
static void     gimp_image_metadata_rotate_query  (gint32             image_ID,
                                                   const gchar       *mime_type,
                                                   GimpMetadata      *metadata,
                                                   gboolean           interactive);
static gboolean gimp_image_metadata_rotate_dialog (gint32             image_ID,
                                                   const gchar       *parasite_name);


/*  public functions  */

GimpMetadata *
gimp_image_metadata_load_prepare (gint32        image_ID,
                                  const gchar  *mime_type,
                                  GFile        *file,
                                  GError      **error)
{
  GimpMetadata *metadata;

  g_return_val_if_fail (image_ID > 0, NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  metadata = gimp_metadata_load_from_file (file, error);

  if (metadata)
    {
#if 0
      {
        gchar *xml = gimp_metadata_serialize (metadata);
        GimpMetadata *new = gimp_metadata_deserialize (xml);
        gchar *xml2 = gimp_metadata_serialize (new);

        FILE *f = fopen ("/tmp/gimp-test-xml1", "w");
        fprintf (f, "%s", xml);
        fclose (f);

        f = fopen ("/tmp/gimp-test-xml2", "w");
        fprintf (f, "%s", xml2);
        fclose (f);

        system ("diff -u /tmp/gimp-test-xml1 /tmp/gimp-test-xml2");

        g_free (xml);
        g_free (xml2);
        g_object_unref (new);
      }
#endif

      gexiv2_metadata_erase_exif_thumbnail (metadata);
    }

  return metadata;
}

void
gimp_image_metadata_load_finish (gint32                 image_ID,
                                 const gchar           *mime_type,
                                 GimpMetadata          *metadata,
                                 GimpMetadataLoadFlags  flags,
                                 gboolean               interactive)
{
  g_return_if_fail (image_ID > 0);
  g_return_if_fail (mime_type != NULL);
  g_return_if_fail (GEXIV2_IS_METADATA (metadata));

  if (flags & GIMP_METADATA_LOAD_COMMENT)
    {
      gchar *comment;

      comment = gexiv2_metadata_get_tag_string (metadata,
                                                "Exif.Photo.UserComment");
      if (! comment)
        comment = gexiv2_metadata_get_tag_string (metadata,
                                                  "Exif.Image.ImageDescription");

      if (comment)
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("gimp-comment",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (comment) + 1,
                                        comment);
          g_free (comment);

          gimp_image_attach_parasite (image_ID, parasite);
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
          gimp_image_set_resolution (image_ID, xres, yres);
          gimp_image_set_unit (image_ID, unit);
        }
    }

  if (flags & GIMP_METADATA_LOAD_ORIENTATION)
    {
      gimp_image_metadata_rotate_query (image_ID, mime_type,
                                        metadata, interactive);
    }

  gimp_image_set_metadata (image_ID, metadata);
}

GimpMetadata *
gimp_image_metadata_save_prepare (gint32       image_ID,
                                  const gchar *mime_type)
{
  GimpMetadata *metadata;

  g_return_val_if_fail (image_ID > 0, NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);

  metadata = gimp_image_get_metadata (image_ID);

  if (metadata)
    {
      GDateTime          *datetime;
      const GimpParasite *comment_parasite;
      const gchar        *comment = NULL;
      gint                image_width;
      gint                image_height;
      gdouble             xres;
      gdouble             yres;
      gchar               buffer[32];

      image_width  = gimp_image_width  (image_ID);
      image_height = gimp_image_height (image_ID);

      datetime = g_date_time_new_now_local ();

      comment_parasite = gimp_image_get_parasite (image_ID, "gimp-comment");
      if (comment_parasite)
        comment = gimp_parasite_data (comment_parasite);

      /* EXIF */

      if (comment)
        {
          gexiv2_metadata_set_tag_string (metadata,
                                          "Exif.Photo.UserComment",
                                          comment);
          gexiv2_metadata_set_tag_string (metadata,
                                          "Exif.Image.ImageDescription",
                                          comment);
        }

      g_snprintf (buffer, sizeof (buffer),
                  "%d:%02d:%02d %02d:%02d:%02d",
                  g_date_time_get_year (datetime),
                  g_date_time_get_month (datetime),
                  g_date_time_get_day_of_month (datetime),
                  g_date_time_get_hour (datetime),
                  g_date_time_get_minute (datetime),
                  g_date_time_get_second (datetime));
      gexiv2_metadata_set_tag_string (metadata,
                                      "Exif.Image.DateTime",
                                      buffer);

      gexiv2_metadata_set_tag_string (metadata,
                                      "Exif.Image.Software",
                                      PACKAGE_STRING);

      gimp_metadata_set_pixel_size (metadata,
                                    image_width, image_height);

      gimp_image_get_resolution (image_ID, &xres, &yres);
      gimp_metadata_set_resolution (metadata, xres, yres,
                                    gimp_image_get_unit (image_ID));

      /* XMP */

      gexiv2_metadata_set_tag_string (metadata,
                                      "Xmp.dc.Format",
                                      mime_type);

      if (! g_strcmp0 (mime_type, "image/tiff"))
        {
          /* TIFF specific XMP data */

          g_snprintf (buffer, sizeof (buffer), "%d", image_width);
          gexiv2_metadata_set_tag_string (metadata,
                                          "Xmp.tiff.ImageWidth",
                                          buffer);

          g_snprintf (buffer, sizeof (buffer), "%d", image_height);
          gexiv2_metadata_set_tag_string (metadata,
                                          "Xmp.tiff.ImageLength",
                                          buffer);

          g_snprintf (buffer, sizeof (buffer),
                      "%d:%02d:%02d %02d:%02d:%02d",
                      g_date_time_get_year (datetime),
                      g_date_time_get_month (datetime),
                      g_date_time_get_day_of_month (datetime),
                      g_date_time_get_hour (datetime),
                      g_date_time_get_minute (datetime),
                      g_date_time_get_second (datetime));
          gexiv2_metadata_set_tag_string (metadata,
                                          "Xmp.tiff.DateTime",
                                          buffer);
        }

      /* IPTC */

      g_snprintf (buffer, sizeof (buffer),
                  "%d-%d-%d",
                  g_date_time_get_year (datetime),
                  g_date_time_get_month (datetime),
                  g_date_time_get_day_of_month (datetime));
      gexiv2_metadata_set_tag_string (metadata,
                                      "Iptc.Application2.DateCreated",
                                      buffer);

      g_snprintf (buffer, sizeof (buffer),
                  "%02d:%02d:%02d-%02d:%02d",
                  g_date_time_get_hour (datetime),
                  g_date_time_get_minute (datetime),
                  g_date_time_get_second (datetime),
                  g_date_time_get_hour (datetime),
                  g_date_time_get_minute (datetime));
      gexiv2_metadata_set_tag_string (metadata,
                                      "Iptc.Application2.TimeCreated",
                                      buffer);

      g_date_time_unref (datetime);
    }

  return metadata;
}

gboolean
gimp_image_metadata_save_finish (gint32                  image_ID,
                                 const gchar            *mime_type,
                                 GimpMetadata           *metadata,
                                 GimpMetadataSaveFlags   flags,
                                 GFile                  *file,
                                 GError                **error)
{
  GExiv2Metadata *new_metadata;
  gboolean        support_exif;
  gboolean        support_xmp;
  gboolean        support_iptc;
  gchar          *value;
  gboolean        success = FALSE;
  gint            i;

  g_return_val_if_fail (image_ID > 0, FALSE);
  g_return_val_if_fail (mime_type != NULL, FALSE);
  g_return_val_if_fail (GEXIV2_IS_METADATA (metadata), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* read metadata from saved file */
  new_metadata = gimp_metadata_load_from_file (file, error);

  if (! new_metadata)
    return FALSE;

  support_exif = gexiv2_metadata_get_supports_exif (new_metadata);
  support_xmp  = gexiv2_metadata_get_supports_xmp  (new_metadata);
  support_iptc = gexiv2_metadata_get_supports_iptc (new_metadata);

  if ((flags & GIMP_METADATA_SAVE_EXIF) && support_exif)
    {
      gchar **exif_data = gexiv2_metadata_get_exif_tags (metadata);

      for (i = 0; exif_data[i] != NULL; i++)
        {
          if (! gexiv2_metadata_has_tag (new_metadata, exif_data[i]) &&
              gimp_metadata_is_tag_supported (exif_data[i], mime_type))
            {
              value = gexiv2_metadata_get_tag_string (metadata, exif_data[i]);
              gexiv2_metadata_set_tag_string (new_metadata, exif_data[i],
                                              value);
              g_free (value);
            }
        }

      g_strfreev (exif_data);
    }

  if ((flags & GIMP_METADATA_SAVE_XMP) && support_xmp)
    {
      gchar **xmp_data = gexiv2_metadata_get_xmp_tags (metadata);

      for (i = 0; xmp_data[i] != NULL; i++)
        {
          if (! gexiv2_metadata_has_tag (new_metadata, xmp_data[i]) &&
              gimp_metadata_is_tag_supported (xmp_data[i], mime_type))
            {
              value = gexiv2_metadata_get_tag_string (metadata, xmp_data[i]);
              gexiv2_metadata_set_tag_string (new_metadata, xmp_data[i],
                                              value);
              g_free (value);
            }
        }

      g_strfreev (xmp_data);
    }

  if ((flags & GIMP_METADATA_SAVE_IPTC) && support_iptc)
    {
      gchar **iptc_data = gexiv2_metadata_get_iptc_tags (metadata);

      for (i = 0; iptc_data[i] != NULL; i++)
        {
          if (! gexiv2_metadata_has_tag (new_metadata, iptc_data[i]) &&
              gimp_metadata_is_tag_supported (iptc_data[i], mime_type))
            {
              value = gexiv2_metadata_get_tag_string (metadata, iptc_data[i]);
              gexiv2_metadata_set_tag_string (new_metadata, iptc_data[i],
                                              value);
              g_free (value);
            }
        }

      g_strfreev (iptc_data);
    }

  if (flags & GIMP_METADATA_SAVE_THUMBNAIL)
    {
      GdkPixbuf *thumb_pixbuf;
      gchar     *thumb_buffer;
      gint       image_width;
      gint       image_height;
      gsize      count;
      gint       thumbw;
      gint       thumbh;

#define EXIF_THUMBNAIL_SIZE 256

      image_width  = gimp_image_width  (image_ID);
      image_height = gimp_image_height (image_ID);

      if (image_width > image_height)
        {
          thumbw = EXIF_THUMBNAIL_SIZE;
          thumbh = EXIF_THUMBNAIL_SIZE * image_height / image_width;
        }
      else
        {
          thumbh = EXIF_THUMBNAIL_SIZE;
          thumbw = EXIF_THUMBNAIL_SIZE * image_height / image_width;
        }

      thumb_pixbuf = gimp_image_get_thumbnail (image_ID, thumbw, thumbh,
                                               GIMP_PIXBUF_KEEP_ALPHA);

      if (gdk_pixbuf_save_to_buffer (thumb_pixbuf, &thumb_buffer, &count,
                                     "jpeg", NULL,
                                     "quality", "75",
                                     NULL))
        {
          gchar buffer[32];

          gexiv2_metadata_set_exif_thumbnail_from_buffer (new_metadata,
                                                          (guchar *) thumb_buffer,
                                                          count);

          g_snprintf (buffer, sizeof (buffer), "%d", thumbw);
          gexiv2_metadata_set_tag_string (new_metadata,
                                          "Exif.Thumbnail.ImageWidth",
                                          buffer);

          g_snprintf (buffer, sizeof (buffer), "%d", thumbh);
          gexiv2_metadata_set_tag_string (new_metadata,
                                          "Exif.Thumbnail.ImageLength",
                                          buffer);

          gexiv2_metadata_set_tag_string (new_metadata,
                                          "Exif.Thumbnail.BitsPerSample",
                                          "8 8 8");
          gexiv2_metadata_set_tag_string (new_metadata,
                                          "Exif.Thumbnail.SamplesPerPixel",
                                          "3");
          gexiv2_metadata_set_tag_string (new_metadata,
                                          "Exif.Thumbnail.PhotometricInterpretation",
                                          "6");

          g_free (thumb_buffer);
        }

      g_object_unref (thumb_pixbuf);
    }

  success = gimp_metadata_save_to_file (new_metadata, file, error);

  g_object_unref (new_metadata);

  return success;
}


/*  private functions  */

static void
gimp_image_metadata_rotate (gint32             image_ID,
                            GExiv2Orientation  orientation)
{
  switch (orientation)
    {
    case GEXIV2_ORIENTATION_UNSPECIFIED:
    case GEXIV2_ORIENTATION_NORMAL:  /* standard orientation, do nothing */
      break;

    case GEXIV2_ORIENTATION_HFLIP:
      gimp_image_flip (image_ID, GIMP_ORIENTATION_HORIZONTAL);
      break;

    case GEXIV2_ORIENTATION_ROT_180:
      gimp_image_rotate (image_ID, GIMP_ROTATE_180);
      break;

    case GEXIV2_ORIENTATION_VFLIP:
      gimp_image_flip (image_ID, GIMP_ORIENTATION_VERTICAL);
      break;

    case GEXIV2_ORIENTATION_ROT_90_HFLIP:  /* flipped diagonally around '\' */
      gimp_image_rotate (image_ID, GIMP_ROTATE_90);
      gimp_image_flip (image_ID, GIMP_ORIENTATION_HORIZONTAL);
      break;

    case GEXIV2_ORIENTATION_ROT_90:  /* 90 CW */
      gimp_image_rotate (image_ID, GIMP_ROTATE_90);
      break;

    case GEXIV2_ORIENTATION_ROT_90_VFLIP:  /* flipped diagonally around '/' */
      gimp_image_rotate (image_ID, GIMP_ROTATE_90);
      gimp_image_flip (image_ID, GIMP_ORIENTATION_VERTICAL);
      break;

    case GEXIV2_ORIENTATION_ROT_270:  /* 90 CCW */
      gimp_image_rotate (image_ID, GIMP_ROTATE_270);
      break;

    default: /* shouldn't happen */
      break;
    }
}

static void
gimp_image_metadata_rotate_query (gint32        image_ID,
                                  const gchar  *mime_type,
                                  GimpMetadata *metadata,
                                  gboolean      interactive)
{
  GimpParasite      *parasite;
  gchar             *parasite_name;
  GExiv2Orientation  orientation;
  gboolean           query = interactive;

  orientation = gexiv2_metadata_get_orientation (metadata);

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

  if (query && ! gimp_image_metadata_rotate_dialog (image_ID,
                                                    parasite_name))
    {
      g_free (parasite_name);
      return;
    }

  g_free (parasite_name);

  gimp_image_metadata_rotate (image_ID, orientation);

  gexiv2_metadata_set_tag_string (metadata,
                                  "Exif.Image.Orientation", "1");
  gexiv2_metadata_set_tag_string (metadata,
                                  "Xmp.tiff.Orientation", "1");
}

static gboolean
gimp_image_metadata_rotate_dialog (gint32       image_ID,
                                   const gchar *parasite_name)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *toggle;
  GdkPixbuf *pixbuf;
  gint       response;

  dialog = gimp_dialog_new (_("Rotate Image?"), "gimp-metadata-rotate-dialog",
                            NULL, 0, NULL, NULL,

                            _("_Keep Orientation"), GTK_RESPONSE_CANCEL,
                            GIMP_STOCK_TOOL_ROTATE, GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gimp_window_set_transient (GTK_WINDOW (dialog));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

#define THUMBNAIL_SIZE 128

  pixbuf = gimp_image_get_thumbnail (image_ID,
                                     THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                     GIMP_PIXBUF_SMALL_CHECKS);

  if (pixbuf)
    {
      GtkWidget *image;
      gchar     *name;

      image = gtk_image_new_from_pixbuf (pixbuf);
      g_object_unref (pixbuf);

      gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);

      name = gimp_image_get_name (image_ID);

      label = gtk_label_new (name);
      gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_MIDDLE);
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE,  PANGO_STYLE_ITALIC,
                                 -1);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      g_free (name);
    }

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   _("According to the EXIF data, "
                                     "this image is rotated."),
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.5,
                        NULL);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   _("Would you like GIMP to rotate it "
                                     "into the standard orientation?"),
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.5,
                        NULL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  toggle = gtk_check_button_new_with_mnemonic (_("_Don't ask me again"));
  gtk_box_pack_end (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
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
