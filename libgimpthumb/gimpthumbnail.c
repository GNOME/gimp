/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2004  Sven Neumann <sven@ligma.org>
 *                          Michael Natterer <mitch@ligma.org>
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
#include <errno.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"

#ifdef G_OS_WIN32
#include "libligmabase/ligmawin32-io.h"
#include <process.h>
#define _getpid getpid
#endif

#include "ligmathumb-types.h"
#include "ligmathumb-error.h"
#include "ligmathumb-utils.h"
#include "ligmathumbnail.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmathumbnail
 * @title: LigmaThumbnail
 * @short_description: The LigmaThumbnail object
 *
 * The LigmaThumbnail object
 **/


/*  #define LIGMA_THUMB_DEBUG  */


#if defined (LIGMA_THUMB_DEBUG) && defined (__GNUC__)
#define LIGMA_THUMB_DEBUG_CALL(t) \
        g_printerr ("%s: %s\n", \
                     __FUNCTION__, t->image_uri ? t->image_uri : "(null)")
#else
#define LIGMA_THUMB_DEBUG_CALL(t) ((void)(0))
#endif


#define TAG_DESCRIPTION           "tEXt::Description"
#define TAG_SOFTWARE              "tEXt::Software"
#define TAG_THUMB_URI             "tEXt::Thumb::URI"
#define TAG_THUMB_MTIME           "tEXt::Thumb::MTime"
#define TAG_THUMB_FILESIZE        "tEXt::Thumb::Size"
#define TAG_THUMB_MIMETYPE        "tEXt::Thumb::Mimetype"
#define TAG_THUMB_IMAGE_WIDTH     "tEXt::Thumb::Image::Width"
#define TAG_THUMB_IMAGE_HEIGHT    "tEXt::Thumb::Image::Height"
#define TAG_THUMB_LIGMA_TYPE       "tEXt::Thumb::X-LIGMA::Type"
#define TAG_THUMB_LIGMA_LAYERS     "tEXt::Thumb::X-LIGMA::Layers"


enum
{
  PROP_0,
  PROP_IMAGE_STATE,
  PROP_IMAGE_URI,
  PROP_IMAGE_MTIME,
  PROP_IMAGE_FILESIZE,
  PROP_IMAGE_MIMETYPE,
  PROP_IMAGE_WIDTH,
  PROP_IMAGE_HEIGHT,
  PROP_IMAGE_TYPE,
  PROP_IMAGE_NUM_LAYERS,
  PROP_THUMB_STATE
};


static void      ligma_thumbnail_finalize     (GObject        *object);
static void      ligma_thumbnail_set_property (GObject        *object,
                                              guint           property_id,
                                              const GValue   *value,
                                              GParamSpec     *pspec);
static void      ligma_thumbnail_get_property (GObject        *object,
                                              guint           property_id,
                                              GValue         *value,
                                              GParamSpec     *pspec);
static void      ligma_thumbnail_reset_info   (LigmaThumbnail  *thumbnail);

static void      ligma_thumbnail_update_image (LigmaThumbnail  *thumbnail);
static void      ligma_thumbnail_update_thumb (LigmaThumbnail  *thumbnail,
                                              LigmaThumbSize   size);

static gboolean  ligma_thumbnail_save         (LigmaThumbnail  *thumbnail,
                                              LigmaThumbSize   size,
                                              const gchar    *filename,
                                              GdkPixbuf      *pixbuf,
                                              const gchar    *software,
                                              GError        **error);
#ifdef LIGMA_THUMB_DEBUG
static void      ligma_thumbnail_debug_notify (GObject        *object,
                                              GParamSpec     *pspec);
#endif


G_DEFINE_TYPE (LigmaThumbnail, ligma_thumbnail, G_TYPE_OBJECT)

#define parent_class ligma_thumbnail_parent_class


static void
ligma_thumbnail_class_init (LigmaThumbnailClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_thumbnail_finalize;
  object_class->set_property = ligma_thumbnail_set_property;
  object_class->get_property = ligma_thumbnail_get_property;

  g_object_class_install_property (object_class,
                                   PROP_IMAGE_STATE,
                                   g_param_spec_enum ("image-state", NULL,
                                                      "State of the image associated to the thumbnail object",
                                                      LIGMA_TYPE_THUMB_STATE,
                                                      LIGMA_THUMB_STATE_UNKNOWN,
                                                      LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_IMAGE_URI,
                                   g_param_spec_string ("image-uri", NULL,
                                                       "URI of the image file",
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_IMAGE_MTIME,
                                   g_param_spec_int64 ("image-mtime", NULL,
                                                       "Modification time of the image file in seconds since the Epoch",
                                                       G_MININT64, G_MAXINT64, 0,
                                                       LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_IMAGE_FILESIZE,
                                   g_param_spec_int64 ("image-filesize", NULL,
                                                       "Size of the image file in bytes",
                                                       0, G_MAXINT64, 0,
                                                       LIGMA_PARAM_READWRITE));
  /**
   * LigmaThumbnail::image-mimetype:
   *
   * Image mimetype
   *
   * Since: 2.2
   **/
  g_object_class_install_property (object_class,
                                   PROP_IMAGE_MIMETYPE,
                                   g_param_spec_string ("image-mimetype", NULL,
                                                        "Image mimetype",
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_IMAGE_WIDTH,
                                   g_param_spec_int ("image-width", NULL,
                                                     "Width of the image in pixels",
                                                     0, G_MAXINT, 0,
                                                     LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_IMAGE_HEIGHT,
                                   g_param_spec_int ("image-height", NULL,
                                                     "Height of the image in pixels",
                                                     0, G_MAXINT, 0,
                                                     LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_IMAGE_TYPE,
                                   g_param_spec_string ("image-type", NULL,
                                                        "String describing the type of the image format",
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_IMAGE_NUM_LAYERS,
                                   g_param_spec_int ("image-num-layers", NULL,
                                                     "The number of layers in the image",
                                                     0, G_MAXINT, 0,
                                                     LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_THUMB_STATE,
                                   g_param_spec_enum ("thumb-state", NULL,
                                                      "State of the thumbnail file",
                                                      LIGMA_TYPE_THUMB_STATE,
                                                      LIGMA_THUMB_STATE_UNKNOWN,
                                                      LIGMA_PARAM_READWRITE));
}

static void
ligma_thumbnail_init (LigmaThumbnail *thumbnail)
{
  thumbnail->image_state      = LIGMA_THUMB_STATE_UNKNOWN;
  thumbnail->image_uri        = NULL;
  thumbnail->image_filename   = NULL;
  thumbnail->image_mtime      = 0;
  thumbnail->image_filesize   = 0;
  thumbnail->image_mimetype   = NULL;
  thumbnail->image_width      = 0;
  thumbnail->image_height     = 0;
  thumbnail->image_type       = NULL;
  thumbnail->image_num_layers = 0;

  thumbnail->thumb_state      = LIGMA_THUMB_STATE_UNKNOWN;
  thumbnail->thumb_size       = -1;
  thumbnail->thumb_filename   = NULL;
  thumbnail->thumb_mtime      = 0;
  thumbnail->thumb_filesize   = 0;

#ifdef LIGMA_THUMB_DEBUG
  g_signal_connect (thumbnail, "notify",
                    G_CALLBACK (ligma_thumbnail_debug_notify),
                    NULL);
#endif
}

static void
ligma_thumbnail_finalize (GObject *object)
{
  LigmaThumbnail *thumbnail = LIGMA_THUMBNAIL (object);

  g_clear_pointer (&thumbnail->image_uri,      g_free);
  g_clear_pointer (&thumbnail->image_filename, g_free);
  g_clear_pointer (&thumbnail->image_mimetype, g_free);
  g_clear_pointer (&thumbnail->image_type,     g_free);
  g_clear_pointer (&thumbnail->thumb_filename, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_thumbnail_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaThumbnail *thumbnail = LIGMA_THUMBNAIL (object);

  switch (property_id)
    {
    case PROP_IMAGE_STATE:
      thumbnail->image_state = g_value_get_enum (value);
      break;
    case PROP_IMAGE_URI:
      ligma_thumbnail_set_uri (LIGMA_THUMBNAIL (object),
                              g_value_get_string (value));
      break;
    case PROP_IMAGE_MTIME:
      thumbnail->image_mtime = g_value_get_int64 (value);
      break;
    case PROP_IMAGE_FILESIZE:
      thumbnail->image_filesize = g_value_get_int64 (value);
      break;
    case PROP_IMAGE_MIMETYPE:
      g_free (thumbnail->image_mimetype);
      thumbnail->image_mimetype = g_value_dup_string (value);
      break;
    case PROP_IMAGE_WIDTH:
      thumbnail->image_width = g_value_get_int (value);
      break;
    case PROP_IMAGE_HEIGHT:
      thumbnail->image_height = g_value_get_int (value);
      break;
    case PROP_IMAGE_TYPE:
      g_free (thumbnail->image_type);
      thumbnail->image_type = g_value_dup_string (value);
      break;
    case PROP_IMAGE_NUM_LAYERS:
      thumbnail->image_num_layers = g_value_get_int (value);
      break;
    case PROP_THUMB_STATE:
      thumbnail->thumb_state = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_thumbnail_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaThumbnail *thumbnail = LIGMA_THUMBNAIL (object);

  switch (property_id)
    {
    case PROP_IMAGE_STATE:
      g_value_set_enum (value, thumbnail->image_state);
      break;
    case PROP_IMAGE_URI:
      g_value_set_string (value, thumbnail->image_uri);
      break;
    case PROP_IMAGE_MTIME:
      g_value_set_int64 (value, thumbnail->image_mtime);
      break;
    case PROP_IMAGE_FILESIZE:
      g_value_set_int64 (value, thumbnail->image_filesize);
      break;
    case PROP_IMAGE_MIMETYPE:
      g_value_set_string (value, thumbnail->image_mimetype);
      break;
    case PROP_IMAGE_WIDTH:
      g_value_set_int (value, thumbnail->image_width);
      break;
    case PROP_IMAGE_HEIGHT:
      g_value_set_int (value, thumbnail->image_height);
      break;
    case PROP_IMAGE_TYPE:
      g_value_set_string (value, thumbnail->image_type);
      break;
    case PROP_IMAGE_NUM_LAYERS:
      g_value_set_int (value, thumbnail->image_num_layers);
      break;
    case PROP_THUMB_STATE:
      g_value_set_enum (value, thumbnail->thumb_state);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * ligma_thumbnail_new:
 *
 * Creates a new #LigmaThumbnail object.
 *
 * Returns: a newly allocated LigmaThumbnail object
 **/
LigmaThumbnail *
ligma_thumbnail_new (void)
{
  return g_object_new (LIGMA_TYPE_THUMBNAIL, NULL);
}

/**
 * ligma_thumbnail_set_uri:
 * @thumbnail: a #LigmaThumbnail object
 * @uri: an escaped URI (transfer none)
 *
 * Sets the location of the image file associated with the #thumbnail.
 *
 * Stores duplicate of passed @uri.
 *
 * All information stored in the #LigmaThumbnail is reset.
 **/
void
ligma_thumbnail_set_uri (LigmaThumbnail *thumbnail,
                        const gchar   *uri)
{
  g_return_if_fail (LIGMA_IS_THUMBNAIL (thumbnail));

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  if (thumbnail->image_uri)
    g_free (thumbnail->image_uri);

  thumbnail->image_uri = g_strdup (uri);

  g_clear_pointer (&thumbnail->image_filename, g_free);
  g_clear_pointer (&thumbnail->thumb_filename, g_free);

  thumbnail->thumb_size     = -1;
  thumbnail->thumb_filesize = 0;
  thumbnail->thumb_mtime    = 0;

  g_object_set (thumbnail,
                "image-state",      LIGMA_THUMB_STATE_UNKNOWN,
                "image-filesize",   (gint64) 0,
                "image-mtime",      (gint64) 0,
                "image-mimetype",   NULL,
                "image-width",      0,
                "image-height",     0,
                "image-type",       NULL,
                "image-num-layers", 0,
                "thumb-state",      LIGMA_THUMB_STATE_UNKNOWN,
                NULL);
}

/**
 * ligma_thumbnail_set_filename:
 * @thumbnail: a #LigmaThumbnail object
 * @filename: a local filename in the encoding of the filesystem
 * @error: return location for possible errors
 *
 * Sets the location of the image file associated with the #thumbnail.
 *
 * Returns: %TRUE if the filename was successfully set,
 *               %FALSE otherwise
 **/
gboolean
ligma_thumbnail_set_filename (LigmaThumbnail  *thumbnail,
                             const gchar    *filename,
                             GError        **error)
{
  gchar *uri = NULL;

  g_return_val_if_fail (LIGMA_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  if (filename)
    uri = g_filename_to_uri (filename, NULL, error);

  ligma_thumbnail_set_uri (thumbnail, uri);

  g_free (uri);

  return (!filename || uri);
}

/**
 * ligma_thumbnail_set_from_thumb:
 * @thumbnail: a #LigmaThumbnail object
 * @filename: filename of a local thumbnail file
 * @error: return location for possible errors
 *
 * This function tries to load the thumbnail file pointed to by
 * @filename and retrieves the URI of the original image file from
 * it. This allows you to find the image file associated with a
 * thumbnail file.
 *
 * This will only work with thumbnails from the global thumbnail
 * directory that contain a valid Thumb::URI tag.
 *
 * Returns: %TRUE if the pixbuf could be loaded, %FALSE otherwise
 **/
gboolean
ligma_thumbnail_set_from_thumb (LigmaThumbnail  *thumbnail,
                               const gchar    *filename,
                               GError        **error)
{
  GdkPixbuf   *pixbuf;
  const gchar *uri;

  g_return_val_if_fail (LIGMA_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  pixbuf = gdk_pixbuf_new_from_file (filename, error);
  if (! pixbuf)
    return FALSE;

  uri = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_URI);
  if (! uri)
    {
      g_set_error (error, LIGMA_THUMB_ERROR, 0,
                   _("Thumbnail contains no Thumb::URI tag"));
      g_object_unref (pixbuf);
      return FALSE;
    }

  ligma_thumbnail_set_uri (thumbnail, uri);
  g_object_unref (pixbuf);

  return TRUE;
}

/**
 * ligma_thumbnail_peek_image:
 * @thumbnail: a #LigmaThumbnail object
 *
 * Checks the image file associated with the @thumbnail and updates
 * information such as state, filesize and modification time.
 *
 * Returns: the image's #LigmaThumbState after the update
 **/
LigmaThumbState
ligma_thumbnail_peek_image (LigmaThumbnail *thumbnail)
{
  g_return_val_if_fail (LIGMA_IS_THUMBNAIL (thumbnail),
                        LIGMA_THUMB_STATE_UNKNOWN);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  g_object_freeze_notify (G_OBJECT (thumbnail));

  ligma_thumbnail_update_image (thumbnail);

  g_object_thaw_notify (G_OBJECT (thumbnail));

  return thumbnail->image_state;
}

/**
 * ligma_thumbnail_peek_thumb:
 * @thumbnail: a #LigmaThumbnail object
 * @size: the preferred size of the thumbnail image
 *
 * Checks if a thumbnail file for the @thumbnail exists. It doesn't
 * load the thumbnail image and thus cannot check if the thumbnail is
 * valid and uptodate for the image file asosciated with the
 * @thumbnail.
 *
 * If you want to check the thumbnail, either attempt to load it using
 * ligma_thumbnail_load_thumb(), or, if you don't need the resulting
 * thumbnail pixbuf, use ligma_thumbnail_check_thumb().
 *
 * Returns: the thumbnail's #LigmaThumbState after the update
 **/
LigmaThumbState
ligma_thumbnail_peek_thumb (LigmaThumbnail *thumbnail,
                           LigmaThumbSize  size)
{
  g_return_val_if_fail (LIGMA_IS_THUMBNAIL (thumbnail),
                        LIGMA_THUMB_STATE_UNKNOWN);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  g_object_freeze_notify (G_OBJECT (thumbnail));

  ligma_thumbnail_update_image (thumbnail);
  ligma_thumbnail_update_thumb (thumbnail, size);

  g_object_thaw_notify (G_OBJECT (thumbnail));

  return thumbnail->thumb_state;
}

/**
 * ligma_thumbnail_check_thumb:
 * @thumbnail: a #LigmaThumbnail object
 * @size: the preferred size of the thumbnail image
 *
 * Checks if a thumbnail file for the @thumbnail exists, loads it and
 * verifies it is valid and uptodate for the image file asosciated
 * with the @thumbnail.
 *
 * Returns: the thumbnail's #LigmaThumbState after the update
 *
 * Since: 2.2
 **/
LigmaThumbState
ligma_thumbnail_check_thumb (LigmaThumbnail *thumbnail,
                            LigmaThumbSize  size)
{
  GdkPixbuf *pixbuf;

  g_return_val_if_fail (LIGMA_IS_THUMBNAIL (thumbnail), FALSE);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  if (ligma_thumbnail_peek_thumb (thumbnail, size) == LIGMA_THUMB_STATE_OK)
    return LIGMA_THUMB_STATE_OK;

  pixbuf = ligma_thumbnail_load_thumb (thumbnail, size, NULL);

  if (pixbuf)
    g_object_unref (pixbuf);

  return thumbnail->thumb_state;
}

static void
ligma_thumbnail_update_image (LigmaThumbnail *thumbnail)
{
  LigmaThumbState  state;
  gint64          mtime    = 0;
  gint64          filesize = 0;

  if (! thumbnail->image_uri)
    return;

  state = thumbnail->image_state;

  switch (state)
    {
    case LIGMA_THUMB_STATE_UNKNOWN:
      g_return_if_fail (thumbnail->image_filename == NULL);

      thumbnail->image_filename =
        _ligma_thumb_filename_from_uri (thumbnail->image_uri);

      if (! thumbnail->image_filename)
        state = LIGMA_THUMB_STATE_REMOTE;

      break;

    case LIGMA_THUMB_STATE_REMOTE:
      break;

    default:
      g_return_if_fail (thumbnail->image_filename != NULL);
      break;
    }

  switch (state)
    {
    case LIGMA_THUMB_STATE_REMOTE:
      break;

    default:
      switch (ligma_thumb_file_test (thumbnail->image_filename,
                                    &mtime, &filesize,
                                    &thumbnail->image_not_found_errno))
        {
        case LIGMA_THUMB_FILE_TYPE_REGULAR:
          state = LIGMA_THUMB_STATE_EXISTS;
          break;

        case LIGMA_THUMB_FILE_TYPE_FOLDER:
          state = LIGMA_THUMB_STATE_FOLDER;
          break;

        case LIGMA_THUMB_FILE_TYPE_SPECIAL:
          state = LIGMA_THUMB_STATE_SPECIAL;
          break;

        default:
          state = LIGMA_THUMB_STATE_NOT_FOUND;
          break;
        }
      break;
    }

  if (state != thumbnail->image_state)
    {
      g_object_set (thumbnail,
                    "image-state", state,
                    NULL);
    }

  if (mtime != thumbnail->image_mtime || filesize != thumbnail->image_filesize)
    {
      g_object_set (thumbnail,
                    "image-mtime",    mtime,
                    "image-filesize", filesize,
                    NULL);

      if (thumbnail->thumb_state == LIGMA_THUMB_STATE_OK)
        g_object_set (thumbnail,
                      "thumb-state", LIGMA_THUMB_STATE_OLD,
                      NULL);
    }
}

static void
ligma_thumbnail_update_thumb (LigmaThumbnail *thumbnail,
                             LigmaThumbSize  size)
{
  gchar          *filename;
  LigmaThumbState  state;
  gint64          filesize = 0;
  gint64          mtime    = 0;

  if (! thumbnail->image_uri)
    return;

  state = thumbnail->thumb_state;

  filename = ligma_thumb_find_thumb (thumbnail->image_uri, &size);

  /* We don't want to clear the LIGMA_THUMB_STATE_FAILED state, because
   * it is normal to have no filename if thumbnail creation failed. */
  if (state != LIGMA_THUMB_STATE_FAILED && ! filename)
    state = LIGMA_THUMB_STATE_NOT_FOUND;

  switch (state)
    {
    case LIGMA_THUMB_STATE_EXISTS:
    case LIGMA_THUMB_STATE_OLD:
    case LIGMA_THUMB_STATE_OK:
      g_return_if_fail (thumbnail->thumb_filename != NULL);

      if (thumbnail->thumb_size     == size     &&
          thumbnail->thumb_filesize == filesize &&
          thumbnail->thumb_mtime    == mtime)
        {
          g_free (filename);
          return;
        }
      break;
    default:
      break;
    }

  if (thumbnail->thumb_filename)
    g_free (thumbnail->thumb_filename);

  thumbnail->thumb_filename = filename;

  if (filename)
    state = (size > LIGMA_THUMB_SIZE_FAIL ?
             LIGMA_THUMB_STATE_EXISTS : LIGMA_THUMB_STATE_FAILED);

  thumbnail->thumb_size     = size;
  thumbnail->thumb_filesize = filesize;
  thumbnail->thumb_mtime    = mtime;

  if (state != thumbnail->thumb_state)
    {
      g_object_freeze_notify (G_OBJECT (thumbnail));

      g_object_set (thumbnail, "thumb-state", state, NULL);
      ligma_thumbnail_reset_info (thumbnail);

      g_object_thaw_notify (G_OBJECT (thumbnail));
    }
}

static void
ligma_thumbnail_reset_info (LigmaThumbnail *thumbnail)
{
  g_object_set (thumbnail,
                "image-width",      0,
                "image-height",     0,
                "image-type",       NULL,
                "image-num-layers", 0,
                NULL);
}

static void
ligma_thumbnail_set_info_from_pixbuf (LigmaThumbnail *thumbnail,
                                     GdkPixbuf     *pixbuf)
{
  const gchar  *option;
  gint          num;

  g_object_freeze_notify (G_OBJECT (thumbnail));

  ligma_thumbnail_reset_info (thumbnail);

  g_free (thumbnail->image_mimetype);
  thumbnail->image_mimetype =
    g_strdup (gdk_pixbuf_get_option (pixbuf, TAG_THUMB_MIMETYPE));

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_IMAGE_WIDTH);
  if (option && sscanf (option, "%d", &num) == 1)
    thumbnail->image_width = num;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_IMAGE_HEIGHT);
  if (option && sscanf (option, "%d", &num) == 1)
    thumbnail->image_height = num;

  thumbnail->image_type =
    g_strdup (gdk_pixbuf_get_option (pixbuf, TAG_THUMB_LIGMA_TYPE));

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_LIGMA_LAYERS);
  if (option && sscanf (option, "%d", &num) == 1)
    thumbnail->image_num_layers = num;

  g_object_thaw_notify (G_OBJECT (thumbnail));
}

static gboolean
ligma_thumbnail_save (LigmaThumbnail  *thumbnail,
                     LigmaThumbSize   size,
                     const gchar    *filename,
                     GdkPixbuf      *pixbuf,
                     const gchar    *software,
                     GError        **error)
{
  const gchar  *keys[12];
  gchar        *values[12];
  gchar        *basename;
  gchar        *dirname;
  gchar        *tmpname;
  gboolean      success;
  gint          i = 0;

  keys[i]   = TAG_DESCRIPTION;
  values[i] = g_strdup_printf ("Thumbnail of %s",  thumbnail->image_uri);
  i++;

  keys[i]   = TAG_SOFTWARE;
  values[i] = g_strdup (software);
  i++;

  keys[i]   = TAG_THUMB_URI;
  values[i] = g_strdup (thumbnail->image_uri);
  i++;

  keys[i]   = TAG_THUMB_MTIME;
  values[i] = g_strdup_printf ("%" G_GINT64_FORMAT, thumbnail->image_mtime);
  i++;

  keys[i]   = TAG_THUMB_FILESIZE;
  values[i] = g_strdup_printf ("%" G_GINT64_FORMAT, thumbnail->image_filesize);
  i++;

  if (thumbnail->image_mimetype)
    {
      keys[i]   = TAG_THUMB_MIMETYPE;
      values[i] = g_strdup (thumbnail->image_mimetype);
      i++;
    }

  if (thumbnail->image_width > 0)
    {
      keys[i]   = TAG_THUMB_IMAGE_WIDTH;
      values[i] = g_strdup_printf ("%d", thumbnail->image_width);
      i++;
    }

  if (thumbnail->image_height > 0)
    {
      keys[i]   = TAG_THUMB_IMAGE_HEIGHT;
      values[i] = g_strdup_printf ("%d", thumbnail->image_height);
      i++;
    }

  if (thumbnail->image_type)
    {
      keys[i]   = TAG_THUMB_LIGMA_TYPE;
      values[i] = g_strdup (thumbnail->image_type);
      i++;
    }

  if (thumbnail->image_num_layers > 0)
    {
      keys[i]   = TAG_THUMB_LIGMA_LAYERS;
      values[i] = g_strdup_printf ("%d", thumbnail->image_num_layers);
      i++;
    }

  keys[i]   = NULL;
  values[i] = NULL;

  basename = g_path_get_basename (filename);
  dirname  = g_path_get_dirname (filename);

  tmpname = g_strdup_printf ("%s%cligma-thumb-%d-%.8s",
                             dirname, G_DIR_SEPARATOR, getpid (), basename);

  g_free (dirname);
  g_free (basename);

  success = gdk_pixbuf_savev (pixbuf, tmpname, "png",
                              (gchar **) keys, values,
                              error);

  for (i = 0; keys[i]; i++)
    g_free (values[i]);

  if (success)
    {
#ifdef LIGMA_THUMB_DEBUG
      g_printerr ("thumbnail saved to temporary file %s\n", tmpname);
#endif

      success = (g_rename (tmpname, filename) == 0);

      if (! success)
        g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                     _("Could not create thumbnail for %s: %s"),
                     thumbnail->image_uri, g_strerror (errno));
    }

  if (success)
    {
#ifdef LIGMA_THUMB_DEBUG
      g_printerr ("temporary thumbnail file renamed to %s\n", filename);
#endif

      success = (g_chmod (filename, 0600) == 0);

      if (! success)
        g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                     "Could not set permissions of thumbnail for %s: %s",
                     thumbnail->image_uri, g_strerror (errno));

      g_object_freeze_notify (G_OBJECT (thumbnail));

      ligma_thumbnail_update_thumb (thumbnail, size);

      if (success &&
          thumbnail->thumb_state == LIGMA_THUMB_STATE_EXISTS &&
          strcmp (filename, thumbnail->thumb_filename) == 0)
        {
          thumbnail->thumb_state = LIGMA_THUMB_STATE_OK;
        }

      g_object_thaw_notify (G_OBJECT (thumbnail));
    }

  g_unlink (tmpname);
  g_free (tmpname);

  return success;
}

#ifdef LIGMA_THUMB_DEBUG
static void
ligma_thumbnail_debug_notify (GObject    *object,
                             GParamSpec *pspec)
{
  GValue       value = G_VALUE_INIT;
  gchar       *str   = NULL;
  const gchar *name;

  g_value_init (&value, pspec->value_type);
  g_object_get_property (object, pspec->name, &value);

  if (G_VALUE_HOLDS_STRING (&value))
    {
      str = g_value_dup_string (&value);
    }
  else if (g_value_type_transformable (pspec->value_type, G_TYPE_STRING))
    {
      GValue  tmp = G_VALUE_INIT;

      g_value_init (&tmp, G_TYPE_STRING);
      g_value_transform (&value, &tmp);

      str = g_value_dup_string (&tmp);

      g_value_unset (&tmp);
    }

  g_value_unset (&value);

  name = LIGMA_THUMBNAIL (object)->image_uri;

  g_printerr (" LigmaThumb (%s) %s: %s\n",
              name ? name : "(null)", pspec->name, str);

  g_free (str);
}
#endif


/**
 * ligma_thumbnail_load_thumb:
 * @thumbnail: a #LigmaThumbnail object
 * @size: the preferred #LigmaThumbSize for the preview
 * @error: return location for possible errors
 *
 * Attempts to load a thumbnail preview for the image associated with
 * @thumbnail. Before you use this function you need need to set an
 * image location using ligma_thumbnail_set_uri() or
 * ligma_thumbnail_set_filename(). You can also peek at the thumb
 * before loading it using ligma_thumbnail_peek_thumb.
 *
 * This function will return the best matching pixbuf for the
 * specified @size. It returns the pixbuf as loaded from disk. It is
 * left to the caller to scale it to the desired size. The returned
 * pixbuf may also represent an outdated preview of the image file.
 * In order to verify if the preview is uptodate, you should check the
 * "thumb_state" property after calling this function.
 *
 * Returns: (nullable) (transfer full): a preview pixbuf or %NULL if no
 *               thumbnail was found
 **/
GdkPixbuf *
ligma_thumbnail_load_thumb (LigmaThumbnail  *thumbnail,
                           LigmaThumbSize   size,
                           GError        **error)
{
  LigmaThumbState  state;
  GdkPixbuf      *pixbuf;
  const gchar    *option;
  gint64          image_mtime;
  gint64          image_size;

  g_return_val_if_fail (LIGMA_IS_THUMBNAIL (thumbnail), NULL);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  if (! thumbnail->image_uri)
    return NULL;

  state = ligma_thumbnail_peek_thumb (thumbnail, size);

  if (state < LIGMA_THUMB_STATE_EXISTS || state == LIGMA_THUMB_STATE_FAILED)
    return NULL;

  pixbuf = gdk_pixbuf_new_from_file (thumbnail->thumb_filename, NULL);
  if (! pixbuf)
    return NULL;

#ifdef LIGMA_THUMB_DEBUG
  g_printerr ("thumbnail loaded from %s\n", thumbnail->thumb_filename);
#endif

  g_object_freeze_notify (G_OBJECT (thumbnail));

  /* URI and mtime from the thumbnail need to match our file */
  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_URI);
  if (!option)
    goto finish;

  if (strcmp (option, thumbnail->image_uri))
    {
      /*  might be a local thumbnail, try if the local part matches  */
      const gchar *baseuri = strrchr (thumbnail->image_uri, '/');

      if (!baseuri || strcmp (option, baseuri))
        goto finish;
    }

  state = LIGMA_THUMB_STATE_OLD;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_MTIME);
  if (!option || sscanf (option, "%" G_GINT64_FORMAT, &image_mtime) != 1)
    goto finish;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_FILESIZE);
  if (option && sscanf (option, "%" G_GINT64_FORMAT, &image_size) != 1)
    goto finish;

  /* TAG_THUMB_FILESIZE is optional but must match if present */
  if (image_mtime == thumbnail->image_mtime &&
      (option == NULL || image_size == thumbnail->image_filesize))
    {
      if (thumbnail->thumb_size == LIGMA_THUMB_SIZE_FAIL)
        state = LIGMA_THUMB_STATE_FAILED;
      else
        state = LIGMA_THUMB_STATE_OK;
    }

  if (state == LIGMA_THUMB_STATE_FAILED)
    ligma_thumbnail_reset_info (thumbnail);
  else
    ligma_thumbnail_set_info_from_pixbuf (thumbnail, pixbuf);

 finish:
  if (thumbnail->thumb_size == LIGMA_THUMB_SIZE_FAIL ||
      (state != LIGMA_THUMB_STATE_OLD && state != LIGMA_THUMB_STATE_OK))
    {
      g_object_unref (pixbuf);
      pixbuf = NULL;
    }

  g_object_set (thumbnail,
                "thumb-state", state,
                NULL);

  g_object_thaw_notify (G_OBJECT (thumbnail));

  return pixbuf;
}

/**
 * ligma_thumbnail_save_thumb:
 * @thumbnail: a #LigmaThumbnail object
 * @pixbuf: a #GdkPixbuf representing the preview thumbnail
 * @software: a string describing the software saving the thumbnail
 * @error: return location for possible errors
 *
 * Saves a preview thumbnail for the image associated with @thumbnail.
 * to the global thumbnail repository.
 *
 * The caller is responsible for setting the image file location, it's
 * filesize, modification time. One way to set this info is to is to
 * call ligma_thumbnail_set_uri() followed by ligma_thumbnail_peek_image().
 * Since this won't work for remote images, it is left to the user of
 * ligma_thumbnail_save_thumb() to do this or to set the information
 * using the @thumbnail object properties.
 *
 * The image format type and the number of layers can optionally be
 * set in order to be stored with the preview image.
 *
 * Returns: %TRUE if a thumbnail was successfully written,
 *               %FALSE otherwise
 **/
gboolean
ligma_thumbnail_save_thumb (LigmaThumbnail  *thumbnail,
                           GdkPixbuf      *pixbuf,
                           const gchar    *software,
                           GError        **error)
{
  LigmaThumbSize  size;
  gchar         *name;
  gboolean       success;

  g_return_val_if_fail (LIGMA_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (thumbnail->image_uri != NULL, FALSE);
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), FALSE);
  g_return_val_if_fail (software != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  size = MAX (gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf));
  if (size < 1)
    return TRUE;

  name = ligma_thumb_name_from_uri (thumbnail->image_uri, size);
  if (! name)
    return TRUE;

  if (! ligma_thumb_ensure_thumb_dir (size, error))
    {
      g_free (name);
      return FALSE;
    }

  success = ligma_thumbnail_save (thumbnail,
                                 size, name, pixbuf, software,
                                 error);
  g_free (name);

  return success;
}

/**
 * ligma_thumbnail_save_thumb_local:
 * @thumbnail: a #LigmaThumbnail object
 * @pixbuf: a #GdkPixbuf representing the preview thumbnail
 * @software: a string describing the software saving the thumbnail
 * @error: return location for possible errors
 *
 * Saves a preview thumbnail for the image associated with @thumbnail
 * to the local thumbnail repository. Local thumbnails have been added
 * with version 0.7 of the spec.
 *
 * Please see also ligma_thumbnail_save_thumb(). The notes made there
 * apply here as well.
 *
 * Returns: %TRUE if a thumbnail was successfully written,
 *               %FALSE otherwise
 *
 * Since: 2.2
 **/
gboolean
ligma_thumbnail_save_thumb_local (LigmaThumbnail  *thumbnail,
                                 GdkPixbuf      *pixbuf,
                                 const gchar    *software,
                                 GError        **error)
{
  LigmaThumbSize  size;
  gchar         *name;
  gchar         *filename;
  gchar         *dirname;
  gboolean       success;

  g_return_val_if_fail (LIGMA_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (thumbnail->image_uri != NULL, FALSE);
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), FALSE);
  g_return_val_if_fail (software != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  size = MAX (gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf));
  if (size < 1)
    return TRUE;

  filename = _ligma_thumb_filename_from_uri (thumbnail->image_uri);
  if (! filename)
    return TRUE;

  dirname = g_path_get_dirname (filename);
  g_free (filename);

  name = ligma_thumb_name_from_uri_local (thumbnail->image_uri, size);
  if (! name)
    {
      g_free (dirname);
      return TRUE;
    }

  if (! ligma_thumb_ensure_thumb_dir_local (dirname, size, error))
    {
      g_free (name);
      g_free (dirname);
      return FALSE;
    }

  g_free (dirname);

  success = ligma_thumbnail_save (thumbnail,
                                 size, name, pixbuf, software,
                                 error);
  g_free (name);

  return success;
}

/**
 * ligma_thumbnail_save_failure:
 * @thumbnail: a #LigmaThumbnail object
 * @software: a string describing the software saving the thumbnail
 * @error: return location for possible errors
 *
 * Saves a failure thumbnail for the image associated with
 * @thumbnail. This is an empty pixbuf that indicates that an attempt
 * to create a preview for the image file failed. It should be used to
 * prevent the software from further attempts to create this thumbnail.
 *
 * Returns: %TRUE if a failure thumbnail was successfully written,
 *               %FALSE otherwise
 **/
gboolean
ligma_thumbnail_save_failure (LigmaThumbnail  *thumbnail,
                             const gchar    *software,
                             GError        **error)
{
  GdkPixbuf *pixbuf;
  gchar     *name;
  gchar     *desc;
  gchar     *time_str;
  gchar     *size_str;
  gboolean   success;

  g_return_val_if_fail (LIGMA_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (thumbnail->image_uri != NULL, FALSE);
  g_return_val_if_fail (software != NULL, FALSE);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  name = ligma_thumb_name_from_uri (thumbnail->image_uri, LIGMA_THUMB_SIZE_FAIL);
  if (! name)
    return TRUE;

  if (! ligma_thumb_ensure_thumb_dir (LIGMA_THUMB_SIZE_FAIL, error))
    {
      g_free (name);
      return FALSE;
    }

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 1, 1);

  desc = g_strdup_printf ("Thumbnail failure for %s", thumbnail->image_uri);
  time_str = g_strdup_printf ("%" G_GINT64_FORMAT, thumbnail->image_mtime);
  size_str = g_strdup_printf ("%" G_GINT64_FORMAT, thumbnail->image_filesize);

  success = gdk_pixbuf_save (pixbuf, name, "png", error,
                             TAG_DESCRIPTION,    desc,
                             TAG_SOFTWARE,       software,
                             TAG_THUMB_URI,      thumbnail->image_uri,
                             TAG_THUMB_MTIME,    time_str,
                             TAG_THUMB_FILESIZE, size_str,
                             NULL);
  if (success)
    {
      success = (g_chmod (name, 0600) == 0);

      if (success)
        ligma_thumbnail_update_thumb (thumbnail, LIGMA_THUMB_SIZE_NORMAL);
      else
        g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                     "Could not set permissions of thumbnail '%s': %s",
                     name, g_strerror (errno));
    }

  g_object_unref (pixbuf);

  g_free (size_str);
  g_free (time_str);
  g_free (desc);
  g_free (name);

  return success;
}

/**
 * ligma_thumbnail_delete_failure:
 * @thumbnail: a #LigmaThumbnail object
 *
 * Removes a failure thumbnail if one exists. This function should be
 * used after a thumbnail has been successfully created.
 *
 * Since: 2.2
 **/
void
ligma_thumbnail_delete_failure (LigmaThumbnail *thumbnail)
{
  gchar *filename;

  g_return_if_fail (LIGMA_IS_THUMBNAIL (thumbnail));
  g_return_if_fail (thumbnail->image_uri != NULL);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  filename = ligma_thumb_name_from_uri (thumbnail->image_uri,
                                       LIGMA_THUMB_SIZE_FAIL);
  if (filename)
    {
      g_unlink (filename);
      g_free (filename);
    }
}

/**
 * ligma_thumbnail_delete_others:
 * @thumbnail: a #LigmaThumbnail object
 * @size: the thumbnail size which should not be deleted
 *
 * Removes all other thumbnails from the global thumbnail
 * repository. Only the thumbnail for @size is not deleted.  This
 * function should be used after a thumbnail has been successfully
 * updated. See the spec for a more detailed description on when to
 * delete thumbnails.
 *
 * Since: 2.2
 **/
void
ligma_thumbnail_delete_others (LigmaThumbnail *thumbnail,
                              LigmaThumbSize  size)
{
  g_return_if_fail (LIGMA_IS_THUMBNAIL (thumbnail));
  g_return_if_fail (thumbnail->image_uri != NULL);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  _ligma_thumbs_delete_others (thumbnail->image_uri, size);
}

/**
 * ligma_thumbnail_has_failed:
 * @thumbnail: a #LigmaThumbnail object
 *
 * Checks if a valid failure thumbnail for the given thumbnail exists
 * in the global thumbnail repository. This may be the case even if
 * ligma_thumbnail_peek_thumb() doesn't return %LIGMA_THUMB_STATE_FAILED
 * since there might be a real thumbnail and a failure thumbnail for
 * the same image file.
 *
 * The application should not attempt to create the thumbnail if a
 * valid failure thumbnail exists.
 *
 * Returns: %TRUE if a failure thumbnail exists or
 *
 * Since: 2.2
 **/
gboolean
ligma_thumbnail_has_failed (LigmaThumbnail *thumbnail)
{
  GdkPixbuf   *pixbuf;
  const gchar *option;
  gchar       *filename;
  gint64       image_mtime;
  gint64       image_size;
  gboolean     failed = FALSE;

  g_return_val_if_fail (LIGMA_IS_THUMBNAIL (thumbnail), FALSE);
  g_return_val_if_fail (thumbnail->image_uri != NULL, FALSE);

  LIGMA_THUMB_DEBUG_CALL (thumbnail);

  filename = ligma_thumb_name_from_uri (thumbnail->image_uri,
                                       LIGMA_THUMB_SIZE_FAIL);
  if (! filename)
    return FALSE;

  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
  g_free (filename);

  if (! pixbuf)
    return FALSE;

  if (ligma_thumbnail_peek_image (thumbnail) < LIGMA_THUMB_STATE_EXISTS)
    goto finish;

  /* URI and mtime from the thumbnail need to match our file */
  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_URI);
  if (! option || strcmp (option, thumbnail->image_uri))
    goto finish;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_MTIME);
  if (!option || sscanf (option, "%" G_GINT64_FORMAT, &image_mtime) != 1)
    goto finish;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_FILESIZE);
  if (option && sscanf (option, "%" G_GINT64_FORMAT, &image_size) != 1)
    goto finish;

  /* TAG_THUMB_FILESIZE is optional but must match if present */
  if (image_mtime == thumbnail->image_mtime &&
      (option == NULL || image_size == thumbnail->image_filesize))
    {
      failed = TRUE;
    }

 finish:
  g_object_unref (pixbuf);

  return failed;
}
