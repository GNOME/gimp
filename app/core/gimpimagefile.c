/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagefile.c
 *
 * Copyright (C) 2001-2004  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpthumb/gimpthumb.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-metadata.h"
#include "gimpimagefile.h"
#include "gimppickable.h"
#include "gimpprogress.h"

#include "file/file-open.h"

#include "gimp-intl.h"


enum
{
  INFO_CHANGED,
  LAST_SIGNAL
};


typedef struct _GimpImagefilePrivate GimpImagefilePrivate;

struct _GimpImagefilePrivate
{
  Gimp          *gimp;

  GFile         *file;
  GimpThumbnail *thumbnail;
  GIcon         *icon;
  GCancellable  *icon_cancellable;

  gchar         *description;
  gboolean       static_desc;

  gint           popup_size;
  gint           popup_width;
  gint           popup_height;
};

#define GET_PRIVATE(imagefile) ((GimpImagefilePrivate *) gimp_imagefile_get_instance_private ((GimpImagefile *) (imagefile)))


static void        gimp_imagefile_dispose          (GObject        *object);
static void        gimp_imagefile_finalize         (GObject        *object);

static void        gimp_imagefile_name_changed     (GimpObject     *object);

static gboolean    gimp_imagefile_get_popup_size   (GimpViewable  *viewable,
                                                    gint           width,
                                                    gint           height,
                                                    gboolean       dot_for_dot,
                                                    gint          *popup_width,
                                                    gint          *popup_height);
static GdkPixbuf * gimp_imagefile_get_new_pixbuf   (GimpViewable   *viewable,
                                                    GimpContext    *context,
                                                    gint            width,
                                                    gint            height,
                                                    GeglColor      *fg_color);
static gchar     * gimp_imagefile_get_description  (GimpViewable   *viewable,
                                                    gchar         **tooltip);

static void        gimp_imagefile_info_changed     (GimpImagefile  *imagefile);
static void        gimp_imagefile_notify_thumbnail (GimpImagefile  *imagefile,
                                                    GParamSpec     *pspec);

static void        gimp_imagefile_icon_callback    (GObject        *source_object,
                                                    GAsyncResult   *result,
                                                    gpointer        data);

static GdkPixbuf * gimp_imagefile_load_thumb       (GimpImagefile  *imagefile,
                                                    gint            width,
                                                    gint            height);
static gboolean    gimp_imagefile_save_thumb       (GimpImagefile  *imagefile,
                                                    GimpImage      *image,
                                                    gint            size,
                                                    gboolean        replace,
                                                    GError        **error);

static void     gimp_thumbnail_set_info_from_image (GimpThumbnail  *thumbnail,
                                                    const gchar    *mime_type,
                                                    GimpImage      *image);
static void     gimp_thumbnail_set_info            (GimpThumbnail  *thumbnail,
                                                    const gchar    *mime_type,
                                                    gint            width,
                                                    gint            height,
                                                    const Babl     *format,
                                                    gint            num_layers);


G_DEFINE_TYPE_WITH_PRIVATE (GimpImagefile, gimp_imagefile, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_imagefile_parent_class

static guint gimp_imagefile_signals[LAST_SIGNAL] = { 0 };


static void
gimp_imagefile_class_init (GimpImagefileClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  gchar             *creator;

  gimp_imagefile_signals[INFO_CHANGED] =
    g_signal_new ("info-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpImagefileClass, info_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose               = gimp_imagefile_dispose;
  object_class->finalize              = gimp_imagefile_finalize;

  gimp_object_class->name_changed     = gimp_imagefile_name_changed;

  viewable_class->name_changed_signal = "info-changed";
  viewable_class->get_popup_size      = gimp_imagefile_get_popup_size;
  viewable_class->get_new_pixbuf      = gimp_imagefile_get_new_pixbuf;
  viewable_class->get_description     = gimp_imagefile_get_description;

  g_type_class_ref (GIMP_TYPE_IMAGE_TYPE);

  creator = g_strdup_printf ("gimp-%d.%d",
                             GIMP_MAJOR_VERSION, GIMP_MINOR_VERSION);

  gimp_thumb_init (creator, NULL);

  g_free (creator);
}

static void
gimp_imagefile_init (GimpImagefile *imagefile)
{
  GimpImagefilePrivate *private = GET_PRIVATE (imagefile);

  private->thumbnail = gimp_thumbnail_new ();

  g_signal_connect_object (private->thumbnail, "notify",
                           G_CALLBACK (gimp_imagefile_notify_thumbnail),
                           imagefile, G_CONNECT_SWAPPED);
}

static void
gimp_imagefile_dispose (GObject *object)
{
  GimpImagefilePrivate *private = GET_PRIVATE (object);

  if (private->icon_cancellable)
    {
      g_cancellable_cancel (private->icon_cancellable);
      g_clear_object (&private->icon_cancellable);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_imagefile_finalize (GObject *object)
{
  GimpImagefilePrivate *private = GET_PRIVATE (object);

  if (private->description)
    {
      if (! private->static_desc)
        g_free (private->description);

      private->description = NULL;
    }

  g_clear_object (&private->thumbnail);
  g_clear_object (&private->icon);
  g_clear_object (&private->file);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_imagefile_name_changed (GimpObject *object)
{
  GimpImagefilePrivate *private = GET_PRIVATE (object);

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  gimp_thumbnail_set_uri (private->thumbnail, gimp_object_get_name (object));

  g_clear_object (&private->file);

  if (gimp_object_get_name (object))
    private->file = g_file_new_for_uri (gimp_object_get_name (object));
}

static gboolean
gimp_imagefile_get_popup_size (GimpViewable *viewable,
                               gint          width,
                               gint          height,
                               gboolean      dot_for_dot,
                               gint         *popup_width,
                               gint         *popup_height)
{
  GimpImagefilePrivate *private = GET_PRIVATE (viewable);

  if (width  < private->popup_width ||
      height < private->popup_height)
    {
      *popup_width  = private->popup_width;
      *popup_height = private->popup_height;

      return TRUE;
    }

  return FALSE;
}

static GdkPixbuf *
gimp_imagefile_get_new_pixbuf (GimpViewable *viewable,
                               GimpContext  *context,
                               gint          width,
                               gint          height,
                               GeglColor    *fg_color G_GNUC_UNUSED)
{
  GimpImagefile *imagefile = GIMP_IMAGEFILE (viewable);

  if (! gimp_object_get_name (imagefile))
    return NULL;

  return gimp_imagefile_load_thumb (imagefile, width, height);
}

static gchar *
gimp_imagefile_get_description (GimpViewable   *viewable,
                                gchar         **tooltip)
{
  GimpImagefile        *imagefile = GIMP_IMAGEFILE (viewable);
  GimpImagefilePrivate *private   = GET_PRIVATE (imagefile);
  GimpThumbnail        *thumbnail = private->thumbnail;
  gchar                *basename;
  gint                  image_width;
  gint                  image_height;

  if (! private->file)
    return NULL;

  g_object_get (thumbnail,
                "image-width",  &image_width,
                "image-height", &image_height,
                NULL);

  if (tooltip)
    {
      const gchar *name;
      const gchar *desc;

      name = gimp_file_get_utf8_name (private->file);
      desc = gimp_imagefile_get_desc_string (imagefile);

      if (desc)
        *tooltip = g_strdup_printf ("%s\n%s", name, desc);
      else
        *tooltip = g_strdup (name);
    }

  basename = g_path_get_basename (gimp_file_get_utf8_name (private->file));

  if (image_width > 0 && image_height > 0)
    {
      gchar *tmp = basename;

      basename = g_strdup_printf ("%s (%d × %d)",
                                  tmp,
                                  image_width,
                                  image_height);
      g_free (tmp);
    }

  return basename;
}


/*  public functions  */

GimpImagefile *
gimp_imagefile_new (Gimp  *gimp,
                    GFile *file)
{
  GimpImagefile *imagefile;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (file == NULL || G_IS_FILE (file), NULL);

  imagefile = g_object_new (GIMP_TYPE_IMAGEFILE, NULL);

  GET_PRIVATE (imagefile)->gimp = gimp;

  if (file)
    {
      gimp_object_take_name (GIMP_OBJECT (imagefile), g_file_get_uri (file));

      /* file member gets created by gimp_imagefile_name_changed() */
    }

  return imagefile;
}

GFile *
gimp_imagefile_get_file (GimpImagefile *imagefile)
{
  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), NULL);

  return GET_PRIVATE (imagefile)->file;
}

void
gimp_imagefile_set_file (GimpImagefile *imagefile,
                         GFile         *file)
{
  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (GET_PRIVATE (imagefile)->file != file)
    {
      gimp_object_take_name (GIMP_OBJECT (imagefile),
                             file ? g_file_get_uri (file) : NULL);
    }
}

GimpThumbnail *
gimp_imagefile_get_thumbnail (GimpImagefile *imagefile)
{
  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), NULL);

  return GET_PRIVATE (imagefile)->thumbnail;
}

GIcon *
gimp_imagefile_get_gicon (GimpImagefile *imagefile)
{
  GimpImagefilePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), NULL);

  private = GET_PRIVATE (imagefile);

  if (private->icon)
    return private->icon;

  if (private->file && ! private->icon_cancellable)
    {
      private->icon_cancellable = g_cancellable_new ();

      g_file_query_info_async (private->file, "standard::icon",
                               G_FILE_QUERY_INFO_NONE,
                               G_PRIORITY_DEFAULT,
                               private->icon_cancellable,
                               gimp_imagefile_icon_callback,
                               imagefile);
    }

  return NULL;
}

void
gimp_imagefile_set_mime_type (GimpImagefile *imagefile,
                              const gchar   *mime_type)
{
  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));

  g_object_set (GET_PRIVATE (imagefile)->thumbnail,
                "image-mimetype", mime_type,
                NULL);
}

void
gimp_imagefile_update (GimpImagefile *imagefile)
{
  GimpImagefilePrivate *private;
  gchar                *uri;

  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));

  private = GET_PRIVATE (imagefile);

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (imagefile));

  g_object_get (private->thumbnail,
                "image-uri", &uri,
                NULL);

  if (uri)
    {
      GimpImagefile *documents_imagefile = (GimpImagefile *)
        gimp_container_get_child_by_name (private->gimp->documents, uri);

      if (documents_imagefile != imagefile &&
          GIMP_IS_IMAGEFILE (documents_imagefile))
        gimp_viewable_invalidate_preview (GIMP_VIEWABLE (documents_imagefile));

      g_free (uri);
    }
}

gboolean
gimp_imagefile_create_thumbnail (GimpImagefile  *imagefile,
                                 GimpContext    *context,
                                 GimpProgress   *progress,
                                 gint            size,
                                 gboolean        replace,
                                 GError        **error)
{
  GimpImagefilePrivate *private;
  GimpThumbnail        *thumbnail;
  GimpThumbState        image_state;

  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* thumbnailing is disabled, we successfully did nothing */
  if (size < 1)
    return TRUE;

  private = GET_PRIVATE (imagefile);

  thumbnail = private->thumbnail;

  gimp_thumbnail_set_uri (thumbnail,
                          gimp_object_get_name (imagefile));

  image_state = gimp_thumbnail_peek_image (thumbnail);

  if (image_state == GIMP_THUMB_STATE_REMOTE ||
      image_state >= GIMP_THUMB_STATE_EXISTS)
    {
      GimpImage     *image;
      gboolean       success;
      gint           width      = 0;
      gint           height     = 0;
      const gchar   *mime_type  = NULL;
      const Babl    *format     = NULL;
      gint           num_layers = -1;

      /*  we only want to attempt thumbnailing on readable, regular files  */
      if (g_file_is_native (private->file))
        {
          GFileInfo *file_info;
          gboolean   regular;
          gboolean   readable;

          file_info = g_file_query_info (private->file,
                                         G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                         G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
                                         G_FILE_QUERY_INFO_NONE,
                                         NULL, NULL);

          regular  = (g_file_info_get_attribute_uint32 (file_info, G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_REGULAR);
          readable = g_file_info_get_attribute_boolean (file_info,
                                                        G_FILE_ATTRIBUTE_ACCESS_CAN_READ);

          g_object_unref (file_info);

          if (! (regular && readable))
            return TRUE;
        }

      g_object_ref (imagefile);

      image = file_open_thumbnail (private->gimp, context, progress,
                                   private->file, size,
                                   &mime_type, &width, &height,
                                   &format, &num_layers, error);

      if (image)
        {
          gimp_thumbnail_set_info (private->thumbnail,
                                   mime_type, width, height,
                                   format, num_layers);
        }
      else
        {
          GimpPDBStatusType  status;

          if (error && *error)
            {
              g_printerr ("Info: Thumbnail load procedure failed: %s\n"
                          "      Falling back to metadata or file load.\n",
                          (*error)->message);
              g_clear_error (error);
            }

          image = gimp_image_metadata_load_thumbnail (private->gimp, private->file, &width, &height, &format, error);
          if (image)
            {
              gimp_thumbnail_set_info (private->thumbnail,
                                       mime_type, width, height,
                                       format, 0);
            }
          else
            {
              if (error && *error)
                {
                  g_printerr ("Info: metadata load failed: %s\n"
                              "      Falling back to file load procedure.\n",
                              (*error)->message);
                  g_clear_error (error);
                }

              image = file_open_image (private->gimp, context, progress,
                                       private->file, size, size, TRUE,
                                       FALSE, NULL,
                                       GIMP_RUN_NONINTERACTIVE,
                                       NULL, &status, &mime_type, error);

              if (image)
                gimp_thumbnail_set_info_from_image (private->thumbnail,
                                                    mime_type, image);
            }
        }

      if (image)
        {
          success = gimp_imagefile_save_thumb (imagefile,
                                               image, size, replace,
                                               error);

          g_object_unref (image);
        }
      else
        {
          /* If the error object is already set (i.e. we have an error
           * message for why the thumbnail creation failed), this is the
           * error we want to return. Ignore any error from failed
           * thumbnail saving.
           */
          gimp_thumbnail_save_failure (thumbnail,
                                       "GIMP " GIMP_VERSION,
                                       error && *error ? NULL : error);
          gimp_imagefile_update (imagefile);
          success = FALSE;
        }

      g_object_unref (imagefile);

      if (! success)
        {
          g_object_set (thumbnail,
                        "thumb-state", GIMP_THUMB_STATE_FAILED,
                        NULL);
        }

      return success;
    }

  return TRUE;
}

/*  The weak version doesn't ref the imagefile but deals gracefully
 *  with an imagefile that is destroyed while the thumbnail is
 *  created. This allows one to use this function w/o the need to
 *  block the user interface.
 */
void
gimp_imagefile_create_thumbnail_weak (GimpImagefile *imagefile,
                                      GimpContext   *context,
                                      GimpProgress  *progress,
                                      gint           size,
                                      gboolean       replace)
{
  GimpImagefilePrivate *private;
  GimpImagefile        *local;

  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));

  if (size < 1)
    return;

  private = GET_PRIVATE (imagefile);

  if (! private->file)
    return;

  local = gimp_imagefile_new (private->gimp, private->file);

  g_object_add_weak_pointer (G_OBJECT (imagefile), (gpointer) &imagefile);

  if (! gimp_imagefile_create_thumbnail (local, context, progress, size, replace,
                                         NULL))
    {
      /* The weak version works on a local copy so the thumbnail
       * status of the actual image is not properly updated in case of
       * creation failure, thus it would end up in a generic
       * GIMP_THUMB_STATE_NOT_FOUND, which is less informative.
       */
      g_object_set (private->thumbnail,
                    "thumb-state", GIMP_THUMB_STATE_FAILED,
                    NULL);
    }

  if (imagefile)
    {
      GFile *file = gimp_imagefile_get_file (imagefile);

      if (file && g_file_equal (file, gimp_imagefile_get_file (local)))
        {
          gimp_imagefile_update (imagefile);
        }

      g_object_remove_weak_pointer (G_OBJECT (imagefile),
                                    (gpointer) &imagefile);
    }

  g_object_unref (local);
}

gboolean
gimp_imagefile_check_thumbnail (GimpImagefile *imagefile)
{
  GimpImagefilePrivate *private;
  gint                  size;

  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), FALSE);

  private = GET_PRIVATE (imagefile);

  size = private->gimp->config->thumbnail_size;

  if (size > 0)
    {
      GimpThumbState  state;

      state = gimp_thumbnail_check_thumb (private->thumbnail, size);

      return (state == GIMP_THUMB_STATE_OK);
    }

  return TRUE;
}

gboolean
gimp_imagefile_save_thumbnail (GimpImagefile  *imagefile,
                               const gchar    *mime_type,
                               GimpImage      *image,
                               GError        **error)
{
  GimpImagefilePrivate *private;
  gint                  size;
  gboolean              success = TRUE;

  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  private = GET_PRIVATE (imagefile);

  size = private->gimp->config->thumbnail_size;

  if (size > 0)
    {
      gimp_thumbnail_set_info_from_image (private->thumbnail,
                                          mime_type, image);

      success = gimp_imagefile_save_thumb (imagefile,
                                           image, size, FALSE,
                                           error);
    }

  return success;
}


/*  private functions  */

static void
gimp_imagefile_info_changed (GimpImagefile *imagefile)
{
  GimpImagefilePrivate *private = GET_PRIVATE (imagefile);

  if (private->description)
    {
      if (! private->static_desc)
        g_free (private->description);

      private->description = NULL;
    }

  g_clear_object (&private->icon);

  g_signal_emit (imagefile, gimp_imagefile_signals[INFO_CHANGED], 0);
}

static void
gimp_imagefile_notify_thumbnail (GimpImagefile *imagefile,
                                 GParamSpec    *pspec)
{
  if (strcmp (pspec->name, "image-state") == 0 ||
      strcmp (pspec->name, "thumb-state") == 0)
    {
      gimp_imagefile_info_changed (imagefile);
    }
}

static void
gimp_imagefile_icon_callback (GObject      *source_object,
                              GAsyncResult *result,
                              gpointer      data)
{
  GimpImagefile        *imagefile;
  GimpImagefilePrivate *private;
  GFile                *file  = G_FILE (source_object);
  GError               *error = NULL;
  GFileInfo            *file_info;

  file_info = g_file_query_info_finish (file, result, &error);

  if (error)
    {
      /* we were cancelled from dispose() and the imagefile is
       * long gone, bail out
       */
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          g_clear_error (&error);
          return;
        }

#ifdef GIMP_UNSTABLE
      g_printerr ("%s: %s\n", G_STRFUNC, error->message);
#endif

      g_clear_error (&error);
    }

  imagefile = GIMP_IMAGEFILE (data);
  private   = GET_PRIVATE (imagefile);

  if (file_info)
    {
      private->icon = g_object_ref (G_ICON (g_file_info_get_attribute_object (file_info, G_FILE_ATTRIBUTE_STANDARD_ICON)));
      g_object_unref (file_info);
    }

  g_clear_object (&private->icon_cancellable);

  if (private->icon)
    gimp_viewable_invalidate_preview (GIMP_VIEWABLE (imagefile));
}

const gchar *
gimp_imagefile_get_desc_string (GimpImagefile *imagefile)
{
  GimpImagefilePrivate *private;
  GimpThumbnail        *thumbnail;
  GimpThumbState        image_state;
  GimpThumbState        thumb_state;
  gint                  image_width;
  gint                  image_height;
  gchar                *image_uri;
  gchar                *image_type;
  gint64                image_filesize;
  gint                  image_num_layers;
  gint                  image_not_found_errno;

  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), NULL);

  private = GET_PRIVATE (imagefile);

  if (private->description)
    return (const gchar *) private->description;

  thumbnail = private->thumbnail;

  g_object_get (thumbnail,
                "image-state",           &image_state,
                "thumb-state",           &thumb_state,
                "image-width",           &image_width,
                "image-height",          &image_height,
                "image-uri",             &image_uri,
                "image-type",            &image_type,
                "image-filesize",        &image_filesize,
                "image-num-layers",      &image_num_layers,
                "image-not-found-errno", &image_not_found_errno,
                NULL);

  switch (image_state)
    {
    case GIMP_THUMB_STATE_UNKNOWN:
      private->description = NULL;
      private->static_desc = TRUE;
      break;

    case GIMP_THUMB_STATE_FOLDER:
      private->description = (gchar *) _("Folder");
      private->static_desc = TRUE;
      break;

    case GIMP_THUMB_STATE_SPECIAL:
      private->description = (gchar *) _("Special File");
      private->static_desc = TRUE;
      break;

    case GIMP_THUMB_STATE_NOT_FOUND:
      private->description =
        (gchar *) g_strerror (image_not_found_errno);
      private->static_desc = TRUE;
      break;

    default:
      {
        GString *str = g_string_new (NULL);

        if (image_state == GIMP_THUMB_STATE_REMOTE)
          {
            g_string_append (str, _("Remote File"));
          }

        if (image_filesize > 0)
          {
            gchar *size = g_format_size (image_filesize);

            if (str->len > 0)
              g_string_append_c (str, '\n');

            g_string_append (str, size);
            g_free (size);
          }

        switch (thumb_state)
          {
          case GIMP_THUMB_STATE_NOT_FOUND:
            if (str->len > 0)
              g_string_append_c (str, '\n');
            g_string_append (str, _("Click to create preview"));
            break;

          case GIMP_THUMB_STATE_EXISTS:
            if (str->len > 0)
              g_string_append_c (str, '\n');
            g_string_append (str, _("Loading preview..."));
            break;

          case GIMP_THUMB_STATE_OLD:
            if (str->len > 0)
              g_string_append_c (str, '\n');
            g_string_append (str, _("Preview is out of date"));
            break;

          case GIMP_THUMB_STATE_FAILED:
            if (str->len > 0)
              g_string_append_c (str, '\n');
            g_string_append (str, _("Cannot create preview"));
            break;

          case GIMP_THUMB_STATE_OK:
            {
              if (image_state == GIMP_THUMB_STATE_REMOTE)
                {
                  if (str->len > 0)
                    g_string_append_c (str, '\n');

                  g_string_append (str, _("(Preview may be out of date)"));
                }

              if (image_width > 0 && image_height > 0)
                {
                  if (str->len > 0)
                    g_string_append_c (str, '\n');

                  g_string_append_printf (str,
                                          ngettext ("%d × %d pixel",
                                                    "%d × %d pixels",
                                                    image_height),
                                          image_width,
                                          image_height);
                }

              if (image_type)
                {
                  if (str->len > 0)
                    g_string_append_c (str, '\n');

                  g_string_append (str, gettext (image_type));
                }

              if (image_num_layers > 0)
                {
                  if (image_type)
                    g_string_append_len (str, ", ", 2);
                  else if (str->len > 0)
                    g_string_append_c (str, '\n');

                  g_string_append_printf (str,
                                          ngettext ("%d layer",
                                                    "%d layers",
                                                    image_num_layers),
                                          image_num_layers);
                }
            }
            break;

          default:
            break;
          }

        private->description = g_string_free (str, FALSE);
        private->static_desc = FALSE;
      }
    }

  return (const gchar *) private->description;
}

static GdkPixbuf *
gimp_imagefile_load_thumb (GimpImagefile *imagefile,
                           gint           width,
                           gint           height)
{
  GimpImagefilePrivate *private   = GET_PRIVATE (imagefile);
  GimpThumbnail        *thumbnail = private->thumbnail;
  GimpThumbState        image_state;
  gchar                *image_uri;
  gint                  image_width;
  gint                  image_height;
  GdkPixbuf            *pixbuf = NULL;
  GError               *error  = NULL;
  gint                  size;
  gint                  preview_width;
  gint                  preview_height;

  g_object_get (thumbnail,
                "image-state",  &image_state,
                "image-uri",    &image_uri,
                "image-width",  &image_width,
                "image-height", &image_height,
                NULL);

  /*  use the size remembered below if a pixbuf of the remembered
   *  dimensions is requested
   */
  if (width  == private->popup_width &&
      height == private->popup_height)
    {
      size = private->popup_size;
    }
  else
    {
      size = MAX (width, height);
    }

  if (gimp_thumbnail_peek_thumb (thumbnail, size) < GIMP_THUMB_STATE_EXISTS)
    return NULL;

  if (image_state == GIMP_THUMB_STATE_NOT_FOUND)
    return NULL;

  pixbuf = gimp_thumbnail_load_thumb (thumbnail, size, &error);

  if (! pixbuf)
    {
      if (error)
        {
          GimpThumbSize  thumb_size = size;
          gchar         *thumb_filename;

          thumb_filename = gimp_thumb_find_thumb (image_uri, &thumb_size);

          gimp_message (private->gimp, NULL, GIMP_MESSAGE_ERROR,
                        _("Could not open thumbnail '%s': %s"),
                        thumb_filename, error->message);
          g_clear_error (&error);

          if (thumb_filename)
            g_free (thumb_filename);
        }

      return NULL;
    }

  /*  remember the actual dimensions of the returned pixbuf, and the
   *  size used to request it, so we can later use that size to get a
   *  popup.
   */
  private->popup_size   = size;
  private->popup_width  = gdk_pixbuf_get_width  (pixbuf);
  private->popup_height = gdk_pixbuf_get_height (pixbuf);

  gimp_viewable_calc_preview_size (private->popup_width,
                                   private->popup_height,
                                   width,
                                   height,
                                   TRUE, 1.0, 1.0,
                                   &preview_width,
                                   &preview_height,
                                   NULL);

  if (preview_width  != private->popup_width ||
      preview_height != private->popup_height)
    {
      GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
                                                   preview_width,
                                                   preview_height,
                                                   GDK_INTERP_BILINEAR);
      g_object_unref (pixbuf);
      pixbuf = scaled;
    }

  if (gdk_pixbuf_get_n_channels (pixbuf) != 3)
    {
      GdkPixbuf *tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                                       preview_width, preview_height);

      gdk_pixbuf_composite_color (pixbuf, tmp,
                                  0, 0, preview_width, preview_height,
                                  0.0, 0.0, 1.0, 1.0,
                                  GDK_INTERP_NEAREST, 255,
                                  0, 0, GIMP_CHECK_SIZE_SM,
                                  0x66666666, 0x99999999);

      g_object_unref (pixbuf);
      pixbuf = tmp;
    }

  return pixbuf;
}

static gboolean
gimp_imagefile_save_thumb (GimpImagefile  *imagefile,
                           GimpImage      *image,
                           gint            size,
                           gboolean        replace,
                           GError        **error)
{
  GimpImagefilePrivate *private   = GET_PRIVATE (imagefile);
  GimpThumbnail        *thumbnail = private->thumbnail;
  GdkPixbuf            *pixbuf;
  gint                  width, height;
  gboolean              success = FALSE;

  if (size < 1)
    return TRUE;

  if (gimp_image_get_width  (image) <= size &&
      gimp_image_get_height (image) <= size)
    {
      width  = gimp_image_get_width  (image);
      height = gimp_image_get_height (image);

      size = MAX (width, height);
    }
  else
    {
      if (gimp_image_get_width (image) < gimp_image_get_height (image))
        {
          height = size;
          width  = MAX (1, (size * gimp_image_get_width (image) /
                            gimp_image_get_height (image)));
        }
      else
        {
          width  = size;
          height = MAX (1, (size * gimp_image_get_height (image) /
                            gimp_image_get_width (image)));
        }
    }

  /*  we need the projection constructed NOW, not some time later  */
  gimp_pickable_flush (GIMP_PICKABLE (image));

  pixbuf = gimp_viewable_get_new_pixbuf (GIMP_VIEWABLE (image),
                                         /* random context, unused */
                                         gimp_get_user_context (image->gimp),
                                         width, height, NULL);

  /*  when layer previews are disabled, we won't get a pixbuf  */
  if (! pixbuf)
    return TRUE;

  success = gimp_thumbnail_save_thumb (thumbnail,
                                       pixbuf,
                                       "GIMP " GIMP_VERSION,
                                       error);

  g_object_unref (pixbuf);

  if (success)
    {
      if (replace)
        gimp_thumbnail_delete_others (thumbnail, size);
      else
        gimp_thumbnail_delete_failure (thumbnail);

      gimp_imagefile_update (imagefile);
    }

  return success;
}

static void
gimp_thumbnail_set_info_from_image (GimpThumbnail *thumbnail,
                                    const gchar   *mime_type,
                                    GimpImage     *image)
{
  const Babl *format;

  /*  peek the thumbnail to make sure that mtime and filesize are set  */
  gimp_thumbnail_peek_image (thumbnail);

  format = gimp_image_get_layer_format (image,
                                        gimp_image_has_alpha (image));

  g_object_set (thumbnail,
                "image-mimetype",   mime_type,
                "image-width",      gimp_image_get_width  (image),
                "image-height",     gimp_image_get_height (image),
                "image-type",       gimp_babl_format_get_description (format),
                "image-num-layers", gimp_image_get_n_layers (image),
                NULL);
}

/**
 * gimp_thumbnail_set_info:
 * @thumbnail: #GimpThumbnail object
 * @mime_type:  MIME type of the image associated with this thumbnail
 * @width:      width of the image associated with this thumbnail
 * @height:     height of the image associated with this thumbnail
 * @format:     format of the image (or NULL if the type is not known)
 * @num_layers: number of layers in the image
 *              (or -1 if the number of layers is not known)
 *
 * Set information about the image associated with the @thumbnail object.
 */
static void
gimp_thumbnail_set_info (GimpThumbnail *thumbnail,
                         const gchar   *mime_type,
                         gint           width,
                         gint           height,
                         const Babl    *format,
                         gint           num_layers)
{
  /*  peek the thumbnail to make sure that mtime and filesize are set  */
  gimp_thumbnail_peek_image (thumbnail);

  g_object_set (thumbnail,
                "image-mimetype", mime_type,
                "image-width",    width,
                "image-height",   height,
                NULL);

  if (format)
    g_object_set (thumbnail,
                  "image-type", gimp_babl_format_get_description (format),
                  NULL);

  if (num_layers != -1)
    g_object_set (thumbnail,
                  "image-num-layers", num_layers,
                  NULL);
}
