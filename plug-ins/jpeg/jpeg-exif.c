/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/*
 * EXIF-handling code for the jpeg plugin.  May eventually be better
 * to move this stuff into libgimpbase and make it available for
 * other plugins.
 */

#include "config.h"

#ifdef HAVE_EXIF

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include <jpeglib.h>
#include <jerror.h>

#include <libexif/exif-content.h>
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>

#define EXIF_HEADER_SIZE 8

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "jpeg.h"


#define jpeg_exif_content_get_value(c,t,v,m) \
	(exif_content_get_entry (c,t) ?	     \
	 jpeg_exif_entry_get_value (exif_content_get_entry (c,t),v,m) : NULL)


static void          jpeg_remove_exif_entry    (ExifData    *exif_data,
                                                ExifIfd      ifd,
                                                ExifTag      tag);

static const gchar * jpeg_exif_entry_get_value (ExifEntry   *e,
                                                gchar       *val,
                                                guint        maxlen);

static gboolean      jpeg_query                (const gchar *msg);


void
jpeg_apply_exif_data_to_image (const gchar   *filename,
                               const gint32   image_ID)
{
  GimpParasite *parasite      = NULL;
  ExifData     *exif_data     = NULL;
  guchar       *exif_buf      = NULL;
  guint         exif_buf_len  = 0;
  ExifEntry    *entry;
  gchar         value[1000];
  gint          byte_order;

  exif_data = exif_data_new_from_file (filename);
  if (!exif_data)
    return;

  exif_data_save_data (exif_data, &exif_buf, &exif_buf_len);

  if (exif_buf_len > EXIF_HEADER_SIZE)
    {
      parasite = gimp_parasite_new ("exif-data",
                                    GIMP_PARASITE_PERSISTENT,
                                    exif_buf_len, exif_buf);
      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);
    }

  free (exif_buf);

  byte_order = exif_data_get_byte_order (exif_data);

  /* get copyright and put it in a parasite */
  if (jpeg_exif_content_get_value (exif_data->ifd[EXIF_IFD_0],
                                   EXIF_TAG_COPYRIGHT,
                                   value, 1000))
    {
      parasite = gimp_parasite_new ("gimp-copyright",
                                    /* GIMP_PARASITE_PERSISTENT */ 0,
                                    strlen (value), value);
      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);
    }

  /* get image artist and put it in a parasite */
  if (jpeg_exif_content_get_value (exif_data->ifd[EXIF_IFD_0],
                                   EXIF_TAG_ARTIST,
                                   value, 1000))
    {
      parasite = gimp_parasite_new ("gimp-artist",
                                    /* GIMP_PARASITE_PERSISTENT */ 0,
                                    strlen (value), value);
      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);
    }

  /* get user comment and put it in a parasite */
  /* note that the format is undefined according to the spec */
  if (jpeg_exif_content_get_value (exif_data->ifd[EXIF_IFD_EXIF],
                                   EXIF_TAG_USER_COMMENT,
                                   value, 1000))
    {
      parasite = gimp_parasite_new ("jpeg-user-comment",
                                    /* GIMP_PARASITE_PERSISTENT */ 0,
                                    strlen (value), value);
      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);
    }

  /* get image description and put it in a parasite */
  /* this must be ascii, so we put it into gimp-comment */
  if (jpeg_exif_content_get_value (exif_data->ifd[EXIF_IFD_0],
                                   EXIF_TAG_IMAGE_DESCRIPTION,
                                   value, 1000))
    {
      parasite = gimp_parasite_new ("gimp-comment",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (value), value);
      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);
    }

  /* get orientation and rotate image accordingly if necessary */
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                       EXIF_TAG_ORIENTATION)))
    {
      gint orient = exif_get_short (entry->data, byte_order);

      if (load_interactive && orient != 1)
        if (jpeg_query (_("According to the EXIF data, this image is rotated. "
                          "Would you like GIMP to rotate it into the standard "
                          "orientation?")))
          {
            switch (orient)
              {
              case 0: /* invalid, so ignore */
              case 1: /* standard orientation, do nothing */
                break;
              case 2: /* flipped right-left */
                gimp_image_flip (image_ID, GIMP_ORIENTATION_HORIZONTAL);
                break;
              case 3: /* rotated 180 */
                break;
              case 4: /* flipped top-bottom */
                gimp_image_flip (image_ID, GIMP_ORIENTATION_VERTICAL);
                break;
              case 5: /* flipped diagonally around '\' */
                gimp_image_rotate (image_ID, GIMP_ROTATE_90);
                gimp_image_flip (image_ID, GIMP_ORIENTATION_HORIZONTAL);
                break;
              case 6: /* 90 CW */
                gimp_image_rotate (image_ID, GIMP_ROTATE_90);
                break;
              case 7: /* flipped diagonally around '/' */
                gimp_image_rotate (image_ID, GIMP_ROTATE_90);
                gimp_image_flip (image_ID, GIMP_ORIENTATION_VERTICAL);
                break;
              case 8: /* 90 CCW */
                gimp_image_rotate (image_ID, GIMP_ROTATE_270);
                break;
              default: /* invalid, ignore */
                break;
              }
        }
    }


  /* get Colorspace and put it in a parasite */
  /* this can only be 'sRGB' or 'uncalibrated' */
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_EXIF],
                                       EXIF_TAG_COLOR_SPACE)))
    {
      gint s = exif_get_short (entry->data, byte_order);

      if (s == 1)
        sprintf (value, "sRGB");
      else
        sprintf (value, "uncalibrated");

      parasite = gimp_parasite_new ("gimp-colorspace",
                                    /* GIMP_PARASITE_PERSISTENT */ 0,
                                    strlen (value), value);
      gimp_image_parasite_attach (image_ID, parasite);
      gimp_parasite_free (parasite);
    }

  exif_data_unref (exif_data);
}


void
jpeg_setup_exif_for_save (ExifData      *exif_data,
                          const gint32   image_ID)
{
  ExifRational  r;
  gdouble       xres, yres;
  ExifEntry    *entry;
  GimpParasite *parasite;
  gint          byte_order = exif_data_get_byte_order (exif_data);

  /* set orientation to top - left */
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                       EXIF_TAG_ORIENTATION)))
    {
      exif_set_short (entry->data, byte_order, (ExifShort) 1);
    }

  /* set x and y resolution */
  gimp_image_get_resolution (image_ID, &xres, &yres);
  r.numerator =   xres;
  r.denominator = 1;
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                       EXIF_TAG_X_RESOLUTION)))
    {
      exif_set_rational (entry->data, byte_order, r);
    }
  r.numerator = yres;
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                       EXIF_TAG_Y_RESOLUTION)))
    {
      exif_set_rational (entry->data, byte_order, r);
    }

  /* set resolution unit, always inches */
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                       EXIF_TAG_RESOLUTION_UNIT)))
    {
      exif_set_short (entry->data, byte_order, (ExifShort) 2);
    }

  /* set software to "The GIMP" */
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                       EXIF_TAG_SOFTWARE)))
    {
      entry->data = g_strdup ("The GIMP");
      entry->size = strlen ("The GIMP") + 1;
      entry->components = entry->size;
    }

  /* set the width and height */
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                       EXIF_TAG_PIXEL_X_DIMENSION)))
    {
      exif_set_long (entry->data, byte_order,
                     (ExifLong) gimp_image_width (image_ID));
    }
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                       EXIF_TAG_PIXEL_Y_DIMENSION)))
    {
      exif_set_long (entry->data, byte_order,
                     (ExifLong) gimp_image_height (image_ID));
    }

  /*
   * set the date & time image was saved
   * note, date & time of original photo is stored elsewwhere, we
   * aren't losing it.
   */
  if ((entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_0],
                                       EXIF_TAG_DATE_TIME)))
    {
      /* small memory leak here */
      entry->data = NULL;
      exif_entry_initialize (entry, EXIF_TAG_DATE_TIME);
    }

   /* save "gimp-comment" as the image description */
  parasite = gimp_image_parasite_find (image_ID, "gimp-comment");
  if (parasite)
    {
      entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_EXIF],
                                      EXIF_TAG_IMAGE_DESCRIPTION);
      if (!entry)
        {
          entry = exif_entry_new ();
          exif_content_add_entry (exif_data->ifd[EXIF_IFD_EXIF],
                                  entry);
          exif_entry_initialize (entry, EXIF_TAG_IMAGE_DESCRIPTION);
          entry->data = g_strndup (parasite->data, parasite->size);
          entry->size = parasite->size;
          entry->components = parasite->size;
        }
      gimp_parasite_free (parasite);
    }

  parasite = gimp_image_parasite_find (image_ID, "gimp-copyright");
  if (parasite)
    {
      entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_EXIF],
                                      EXIF_TAG_COPYRIGHT);
      if (!entry)
        {
          entry = exif_entry_new ();
          exif_content_add_entry (exif_data->ifd[EXIF_IFD_EXIF],
                                  entry);
          exif_entry_initialize (entry, EXIF_TAG_COPYRIGHT);
          entry->data = g_strndup (parasite->data, parasite->size);
          entry->size = parasite->size;
          entry->components = parasite->size;
        }
      gimp_parasite_free (parasite);
    }

  parasite = gimp_image_parasite_find (image_ID, "gimp-artist");
  if (parasite)
    {
      entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_EXIF],
                                      EXIF_TAG_ARTIST);
      if (!entry)
        {
          entry = exif_entry_new ();
          exif_content_add_entry (exif_data->ifd[EXIF_IFD_EXIF],
                                  entry);
          exif_entry_initialize (entry, EXIF_TAG_ARTIST);
          entry->data = g_strndup (parasite->data, parasite->size);
          entry->size = parasite->size;
          entry->components = parasite->size;
        }
      gimp_parasite_free (parasite);
    }

  parasite = gimp_image_parasite_find (image_ID, "jpeg-user-comment");
  if (parasite)
    {
      entry = exif_content_get_entry (exif_data->ifd[EXIF_IFD_EXIF],
                                      EXIF_TAG_USER_COMMENT);
      if (!entry)
        {
          entry = exif_entry_new ();
          exif_content_add_entry (exif_data->ifd[EXIF_IFD_EXIF],
                                  entry);
          exif_entry_initialize (entry, EXIF_TAG_USER_COMMENT);
          entry->data = g_strndup (parasite->data, parasite->size);
          entry->size = parasite->size;
          entry->components = parasite->size;
        }
      gimp_parasite_free (parasite);
    }

  /* should set components configuration, don't know how */

  /*
   *remove entries that don't apply to jpeg
   *(may have come from tiff or raw)
  */
  jpeg_remove_exif_entry(exif_data, EXIF_IFD_0, EXIF_TAG_COMPRESSION);
  jpeg_remove_exif_entry(exif_data, EXIF_IFD_0, EXIF_TAG_IMAGE_WIDTH);
  jpeg_remove_exif_entry(exif_data, EXIF_IFD_0, EXIF_TAG_IMAGE_LENGTH);
  jpeg_remove_exif_entry(exif_data, EXIF_IFD_0, EXIF_TAG_BITS_PER_SAMPLE);
  jpeg_remove_exif_entry(exif_data, EXIF_IFD_0, EXIF_TAG_SAMPLES_PER_PIXEL);
  jpeg_remove_exif_entry(exif_data, EXIF_IFD_0, EXIF_TAG_PHOTOMETRIC_INTERPRETATION);
  jpeg_remove_exif_entry(exif_data, EXIF_IFD_0, EXIF_TAG_STRIP_OFFSETS);
  jpeg_remove_exif_entry(exif_data, EXIF_IFD_0, EXIF_TAG_PLANAR_CONFIGURATION);
  jpeg_remove_exif_entry(exif_data, EXIF_IFD_0, EXIF_TAG_YCBCR_SUB_SAMPLING);

  /* should set thumbnail attributes */
}


static void
jpeg_remove_exif_entry (ExifData  *exif_data,
                        ExifIfd    ifd,
                        ExifTag    tag)
{
  ExifEntry  *entry = exif_content_get_entry (exif_data->ifd[ifd],
                                              tag);

  if (entry)
    exif_content_remove_entry (exif_data->ifd[ifd], entry);
}


static const gchar *
jpeg_exif_entry_get_value (ExifEntry *e,
                           gchar     *val,
                           guint      maxlen)
{

#ifdef HAVE_EXIF_0_6
  return exif_entry_get_value (e, val, maxlen);
#else
  strncpy (val, exif_entry_get_value (e), maxlen);
  return val;
#endif /* HAVE_EXIF_0_6 */

}


static gboolean
jpeg_query (const gchar *msg)
{
  GtkWidget *dialog;
  gboolean   ret;

  dialog = gtk_message_dialog_new (NULL, 0,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_OK_CANCEL,
                                   "%s", msg);

  ret = gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return (ret == GTK_RESPONSE_OK);
}


#endif /* HAVE_EXIF */
