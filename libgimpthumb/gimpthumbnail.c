/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * http://triq.net/~pearl/thumbnail-spec/
 *
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <stdio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gimpthumb-types.h"
#include "gimpthumb-utils.h"
#include "gimpthumbnail.h"


#define TAG_DESCRIPTION        "tEXt::Description"
#define TAG_SOFTWARE           "tEXt::Software"
#define TAG_THUMB_URI          "tEXt::Thumb::URI"
#define TAG_THUMB_MTIME        "tEXt::Thumb::MTime"
#define TAG_THUMB_FILESIZE     "tEXt::Thumb::Size"
#define TAG_THUMB_IMAGE_WIDTH  "tEXt::Thumb::Image::Width"
#define TAG_THUMB_IMAGE_HEIGHT "tEXt::Thumb::Image::Height"
#define TAG_THUMB_GIMP_TYPE    "tEXt::Thumb::X-GIMP::Type"
#define TAG_THUMB_GIMP_LAYERS  "tEXt::Thumb::X-GIMP::Layers"


enum
{
  PROP_0,
  PROP_URI,
  PROP_STATE,
  PROP_MTIME,
  PROP_FILESIZE,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_TYPE,
  PROP_NUM_LAYERS
};


static void          gimp_thumbnail_class_init      (GimpThumbnailClass *klass);
static void          gimp_thumbnail_init            (GimpThumbnail  *thumbnail);
static void          gimp_thumbnail_finalize        (GObject        *object);
static void          gimp_thumbnail_set_property    (GObject        *object,
                                                     guint           property_id,
                                                     const GValue   *value,
                                                     GParamSpec     *pspec);
static void          gimp_thumbnail_get_property    (GObject        *object,
                                                     guint           property_id,
                                                     GValue         *value,
                                                     GParamSpec     *pspec);
static void          gimp_thumbnail_reset_info      (GimpThumbnail  *thumbnail);

static GdkPixbuf   * gimp_thumbnail_read_png_thumb  (GimpThumbnail  *thumbnail,
                                                     GimpThumbSize   thumb_size,
                                                     GError        **error);


static GObjectClass *parent_class = NULL;


GType
gimp_thumbnail_get_type (void)
{
  static GType thumbnail_type = 0;

  if (!thumbnail_type)
    {
      static const GTypeInfo thumbnail_info =
      {
        sizeof (GimpThumbnailClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_thumbnail_class_init,
	NULL,           /* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpThumbnail),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_thumbnail_init,
      };

      thumbnail_type = g_type_register_static (G_TYPE_OBJECT,
					       "GimpThumbnail",
                                               &thumbnail_info, 0);
    }

  return thumbnail_type;
}

static void
gimp_thumbnail_class_init (GimpThumbnailClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_thumbnail_finalize;
  object_class->set_property = gimp_thumbnail_set_property;
  object_class->get_property = gimp_thumbnail_get_property;

  g_object_class_install_property (object_class,
				   PROP_URI,
				   g_param_spec_string ("uri", NULL,
                                                        NULL,
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_STATE,
				   g_param_spec_enum ("state", NULL,
                                                      NULL,
                                                      GIMP_TYPE_THUMB_STATE,
                                                      GIMP_THUMB_STATE_UNKNOWN,
                                                      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_MTIME,
				   g_param_spec_int64 ("image-mtime", NULL,
                                                       NULL,
                                                       0, G_MAXINT64, 0,
                                                       G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_FILESIZE,
				   g_param_spec_int64 ("image-filesize", NULL,
                                                       NULL,
                                                       0, G_MAXINT64, 0,
                                                       G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_WIDTH,
				   g_param_spec_int ("image-width", NULL,
                                                     NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_HEIGHT,
				   g_param_spec_int ("image-height", NULL,
                                                     NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_TYPE,
                                   g_param_spec_string ("image-type", NULL,
                                                        NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_TYPE,
                                   g_param_spec_int ("image-num-layers", NULL,
                                                     NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));
}

static void
gimp_thumbnail_init (GimpThumbnail *thumbnail)
{
  thumbnail->uri              = NULL;
  thumbnail->state            = GIMP_THUMB_STATE_UNKNOWN;

  thumbnail->thumb_filename   = NULL;
  thumbnail->thumb_mtime      = 0;
  thumbnail->thumb_filesize   = 0;

  thumbnail->image_filename   = NULL;
  thumbnail->image_mtime      = 0;
  thumbnail->image_filesize   = 0;

  thumbnail->image_width      = 0;
  thumbnail->image_height     = 0;
  thumbnail->image_type       = 0;
  thumbnail->image_num_layers = 0;
}

static void
gimp_thumbnail_finalize (GObject *object)
{
  GimpThumbnail *thumbnail = GIMP_THUMBNAIL (object);

  if (thumbnail->uri)
    {
      g_free (thumbnail->uri);
      thumbnail->uri = NULL;
    }
  if (thumbnail->thumb_filename)
    {
      g_free (thumbnail->thumb_filename);
      thumbnail->thumb_filename = NULL;
    }
  if (thumbnail->image_filename)
    {
      g_free (thumbnail->image_filename);
      thumbnail->image_filename = NULL;
    }
  if (thumbnail->image_type)
    {
      g_free (thumbnail->image_type);
      thumbnail->image_type = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_thumbnail_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpThumbnail *thumbnail = GIMP_THUMBNAIL (object);

  switch (property_id)
    {
    case PROP_URI:
      gimp_thumbnail_set_uri (GIMP_THUMBNAIL (object),
                              g_value_get_string (value));
      break;
    case PROP_MTIME:
      thumbnail->image_mtime = g_value_get_int64 (value);
      break;
    case PROP_FILESIZE:
      thumbnail->image_filesize = g_value_get_int64 (value);
      break;
    case PROP_WIDTH:
      thumbnail->image_width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      thumbnail->image_height = g_value_get_int (value);
      break;
    case PROP_TYPE:
      g_free (thumbnail->image_type);
      thumbnail->image_type = g_value_dup_string (value);
      break;
    case PROP_NUM_LAYERS:
      thumbnail->image_num_layers = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_thumbnail_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpThumbnail *thumbnail = GIMP_THUMBNAIL (object);

  switch (property_id)
    {
    case PROP_URI:
      g_value_set_string (value, thumbnail->uri);
      break;
    case PROP_STATE:
      g_value_set_enum (value, thumbnail->state);
      break;
    case PROP_MTIME:
      g_value_set_int64 (value, thumbnail->image_mtime);
      break;
    case PROP_FILESIZE:
      g_value_set_int64 (value, thumbnail->image_filesize);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, thumbnail->image_width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, thumbnail->image_height);
      break;
    case PROP_TYPE:
      g_value_set_string (value, thumbnail->image_type);
      break;
    case PROP_NUM_LAYERS:
      g_value_set_int (value, thumbnail->image_height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_thumbnail_reset_info (GimpThumbnail *thumbnail)
{
  g_object_set (thumbnail,
                "image-width",       0,
                "image-height",      0,
                "image-type",        NULL,
                "image-num-layers",  0,
                NULL);
}

GimpThumbnail *
gimp_thumbnail_new (void)
{
  return g_object_new (GIMP_TYPE_THUMBNAIL, NULL);
}

void
gimp_thumbnail_set_uri (GimpThumbnail *thumbnail,
                        const gchar   *uri)
{
  g_return_if_fail (GIMP_IS_THUMBNAIL (thumbnail));

  if (thumbnail->thumb_filename)
    {
      g_free (thumbnail->thumb_filename);
      thumbnail->thumb_filename = NULL;
    }

  thumbnail->thumb_filesize = 0;
  thumbnail->thumb_mtime    = 0;

  g_object_set (thumbnail,
                "state",             GIMP_THUMB_STATE_UNKNOWN,
                "image-filesize",    0,
                "image-mtime",       0,
                "image-width",       0,
                "image-height",      0,
                "image-type",        NULL,
                "image-num-layers",  0,
                NULL);
}

void
gimp_thumbnail_update (GimpThumbnail *thumbnail)
{
  GimpThumbState  state          = thumbnail->state;
  gchar          *thumbname      = NULL;
  gint64          image_filesize = 0;
  gint64          image_mtime    = 0;

  if (! thumbnail->uri)
    return;

  /*  do we know the image filename already ?  */
  switch (state)
    {
    case GIMP_THUMB_STATE_UNKNOWN:
      g_return_if_fail (thumbnail->image_filename == NULL);

      thumbnail->image_filename = g_filename_from_uri (thumbnail->uri,
                                                       NULL, NULL);

      if (! thumbnail->image_filename)
        state = GIMP_THUMB_STATE_REMOTE;

      break;

    case GIMP_THUMB_STATE_REMOTE:
      break;

    default:
      g_return_if_fail (thumbnail->image_filename != NULL);
      break;
    }

  /*  check if the imagefile changed  */
  switch (state)
    {
    case GIMP_THUMB_STATE_REMOTE:
      break;

    default:
      if (gimp_thumb_file_test (thumbnail->image_filename,
                                &image_mtime, &image_filesize))
        {
          if (image_mtime != thumbnail->image_mtime ||
              image_filesize != thumbnail->image_filesize)
            {
              state = GIMP_THUMB_STATE_EXISTS;
            }
        }
      else
        {
          state = GIMP_THUMB_STATE_NOT_FOUND;
        }
      break;
    }

  /*  do we know the thumbnail filename ?  */
  switch (state)
    {
      case GIMP_THUMB_STATE_EXISTS:
        thumbname = gimp_thumbnail_find_png_thumb (thumbnail->uri, NULL);
        break;

#if 0
  thumbname = gimp_thumbnail_find_png_thumb (thumbnail->uri, NULL);

  thumbnail->state    = GIMP_THUMB_STATE_THUMBNAIL_NOT_FOUND;
  thumbnail->mtime    = thumb_mtime;
  thumbnail->filesize = thumb_filesize;

  thumbname = gimp_thumbnail_find_png_thumb (uri, NULL);

  if (thumbname)
    thumbnail->state = GIMP_THUMB_STATE_THUMBNAIL_EXISTS;

 cleanup:
  g_free (filename);
  g_free (thumbname);
#endif


    case GIMP_THUMB_STATE_UNKNOWN:
    case GIMP_THUMB_STATE_REMOTE:
    case GIMP_THUMB_STATE_NOT_FOUND:
    case GIMP_THUMB_STATE_THUMBNAIL_NOT_FOUND:
    case GIMP_THUMB_STATE_THUMBNAIL_EXISTS:
    case GIMP_THUMB_STATE_THUMBNAIL_OLD:
    case GIMP_THUMB_STATE_THUMBNAIL_FAILED:
    case GIMP_THUMB_STATE_THUMBNAIL_OK:
      break;
    }
}

gboolean
gimp_thumbnail_set_filename (GimpThumbnail  *thumbnail,
                             const gchar    *filename,
                             GError        **error)
{
  gchar *uri = NULL;

  g_return_val_if_fail (GIMP_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (filename)
    uri = g_filename_to_uri (filename, NULL, error);

  gimp_thumbnail_set_uri (thumbnail, uri);

  return (!filename || uri);
}


GimpThumbState
gimp_thumbnail_get_state (GimpThumbnail *thumbnail)
{
  g_return_val_if_fail (GIMP_IS_THUMBNAIL (thumbnail),
                        GIMP_THUMB_STATE_UNKNOWN);

  return thumbnail->state;
}

static void
gimp_thumbnail_set_info_from_pixbuf (GimpThumbnail *thumbnail,
                                     GdkPixbuf     *pixbuf)
{
  const gchar  *option;
  gint          n;

  g_object_freeze_notify (G_OBJECT (thumbnail));

  gimp_thumbnail_reset_info (thumbnail);

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_IMAGE_WIDTH);
  if (option && sscanf (option, "%d", &n) == 1)
    thumbnail->image_width = n;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_IMAGE_HEIGHT);
  if (option && sscanf (option, "%d", &n) == 1)
    thumbnail->image_height = n;

  thumbnail->image_type =
    g_strdup (gdk_pixbuf_get_option (pixbuf, TAG_THUMB_GIMP_TYPE));

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_GIMP_LAYERS);
  if (option && sscanf (option, "%d", &n) == 1)
    thumbnail->image_num_layers = 0;
}

GdkPixbuf *
gimp_thumbnail_get_pixbuf (GimpThumbnail  *thumbnail,
                           GimpThumbSize   size,
                           GError        **error)
{
  GdkPixbuf *pixbuf;

  if (! thumbnail->uri)
    return NULL;

  pixbuf = gimp_thumbnail_read_png_thumb (thumbnail, size, error);

  if (pixbuf)
    {
      gint width  = gdk_pixbuf_get_width (pixbuf);
      gint height = gdk_pixbuf_get_height (pixbuf);

      if (width > size || height > size)
        {
          GdkPixbuf *scaled;

          /*  FIXME: aspect ratio  */
          scaled = gdk_pixbuf_scale_simple (pixbuf,
                                            size, size,
                                            GDK_INTERP_BILINEAR);

          g_object_unref (pixbuf);
          pixbuf = scaled;
        }
    }

  return pixbuf;
}

/* PNG thumbnail handling routines according to the
   Thumbnail Managing Standard  http://triq.net/~pearl/thumbnail-spec/  */

static GdkPixbuf *
gimp_thumbnail_read_png_thumb (GimpThumbnail  *thumbnail,
                               GimpThumbSize   thumb_size,
                               GError        **error)
{
  GimpThumbState  old_state;
  GdkPixbuf      *pixbuf     = NULL;
  gchar          *thumbname  = NULL;
  const gchar    *option;
  gint64          image_mtime;
  gint64          image_size;

  if (thumbnail->state < GIMP_THUMB_STATE_EXISTS)
    return NULL;

  old_state = thumbnail->state;

  /* try to locate a thumbnail for this image */
  thumbnail->state = GIMP_THUMB_STATE_THUMBNAIL_NOT_FOUND;

  thumbname = gimp_thumbnail_find_png_thumb (thumbnail->uri, &thumb_size);

  if (!thumbname)
    goto cleanup;

  pixbuf = gdk_pixbuf_new_from_file (thumbname, error);

  if (! pixbuf)
    goto cleanup;

  g_free (thumbname);
  thumbname = NULL;

  /* URI and mtime from the thumbnail need to match our file */

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_URI);
  if (!option || strcmp (option, thumbnail->uri))
    goto cleanup;

  thumbnail->state = GIMP_THUMB_STATE_THUMBNAIL_OLD;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_MTIME);
  if (!option || sscanf (option, "%" G_GINT64_FORMAT, &image_mtime) != 1)
    goto cleanup;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_FILESIZE);
  if (option && sscanf (option, "%" G_GINT64_FORMAT, &image_size) != 1)
    goto cleanup;

  /* TAG_THUMB_FILESIZE is optional but must match if present */
  if (image_mtime == thumbnail->image_mtime &&
      (option == NULL || image_size == thumbnail->image_filesize))
    {
      if (! thumb_size)
        thumbnail->state = GIMP_THUMB_STATE_THUMBNAIL_FAILED;
      else
        thumbnail->state = GIMP_THUMB_STATE_THUMBNAIL_OK;
    }

  if (! thumb_size)
    {
      gimp_thumbnail_reset_info (thumbnail);
      goto cleanup;
    }

  gimp_thumbnail_set_info_from_pixbuf (thumbnail, pixbuf);

 cleanup:
  if (thumbnail->state != old_state)
    g_object_notify (G_OBJECT (thumbnail), "state");

  g_free (thumbname);

  if (thumbnail->state != GIMP_THUMB_STATE_THUMBNAIL_OK)
    {
      g_object_unref (pixbuf);
      return NULL;
    }

  return pixbuf;
}
