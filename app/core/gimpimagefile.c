/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagefile.c
 *
 * Copyright (C) 2001-2004  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpthumb/gimpthumb.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimagefile.h"
#include "gimpmarshal.h"
#include "gimpprogress.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "gimp-intl.h"


enum
{
  INFO_CHANGED,
  LAST_SIGNAL
};


static void        gimp_imagefile_finalize         (GObject        *object);

static void        gimp_imagefile_name_changed     (GimpObject     *object);

static void        gimp_imagefile_info_changed     (GimpImagefile  *imagefile);
static void        gimp_imagefile_notify_thumbnail (GimpImagefile  *imagefile,
                                                    GParamSpec     *pspec);

static GdkPixbuf * gimp_imagefile_get_new_pixbuf   (GimpViewable   *viewable,
                                                    GimpContext    *context,
                                                    gint            width,
                                                    gint            height);
static GdkPixbuf * gimp_imagefile_load_thumb       (GimpImagefile  *imagefile,
                                                    gint            width,
                                                    gint            height);
static gboolean    gimp_imagefile_save_thumb       (GimpImagefile  *imagefile,
                                                    GimpImage      *image,
                                                    gint            size,
                                                    gboolean        replace,
                                                    GError        **error);

static gchar     * gimp_imagefile_get_description  (GimpViewable   *viewable,
                                                    gchar         **tooltip);

static void     gimp_thumbnail_set_info_from_image (GimpThumbnail  *thumbnail,
                                                    const gchar    *mime_type,
                                                    GimpImage      *image);
static void     gimp_thumbnail_set_info            (GimpThumbnail  *thumbnail,
                                                    const gchar    *mime_type,
                                                    gint            width,
                                                    gint            height);


G_DEFINE_TYPE (GimpImagefile, gimp_imagefile, GIMP_TYPE_VIEWABLE)

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
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize              = gimp_imagefile_finalize;

  gimp_object_class->name_changed     = gimp_imagefile_name_changed;

  viewable_class->name_changed_signal = "info-changed";
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
  imagefile->gimp        = NULL;
  imagefile->thumbnail   = gimp_thumbnail_new ();
  imagefile->description = NULL;

  g_signal_connect_object (imagefile->thumbnail, "notify",
                           G_CALLBACK (gimp_imagefile_notify_thumbnail),
                           imagefile, G_CONNECT_SWAPPED);
}

static void
gimp_imagefile_finalize (GObject *object)
{
  GimpImagefile *imagefile = GIMP_IMAGEFILE (object);

  if (imagefile->description)
    {
      if (! imagefile->static_desc)
        g_free (imagefile->description);

      imagefile->description = NULL;
    }

  if (imagefile->thumbnail)
    {
      g_object_unref (imagefile->thumbnail);
      imagefile->thumbnail = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpImagefile *
gimp_imagefile_new (Gimp        *gimp,
                    const gchar *uri)
{
  GimpImagefile *imagefile;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  imagefile = g_object_new (GIMP_TYPE_IMAGEFILE, NULL);

  imagefile->gimp = gimp;

  if (uri)
    gimp_object_set_name (GIMP_OBJECT (imagefile), uri);

  return imagefile;
}

void
gimp_imagefile_update (GimpImagefile *imagefile)
{
  gchar *uri;

  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (imagefile));

  g_object_get (imagefile->thumbnail,
                "image-uri", &uri,
                NULL);

  if (uri)
    {
      GimpImagefile *documents_imagefile = (GimpImagefile *)
        gimp_container_get_child_by_name (imagefile->gimp->documents, uri);

      if (documents_imagefile != imagefile &&
          GIMP_IS_IMAGEFILE (documents_imagefile))
        gimp_viewable_invalidate_preview (GIMP_VIEWABLE (documents_imagefile));

      g_free (uri);
    }
}

void
gimp_imagefile_create_thumbnail (GimpImagefile *imagefile,
                                 GimpContext   *context,
                                 GimpProgress  *progress,
                                 gint           size,
                                 gboolean       replace)
{
  GimpThumbnail  *thumbnail;
  GimpThumbState  image_state;

  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (! imagefile->gimp->config->layer_previews)
    return;

  if (size < 1)
    return;

  thumbnail = imagefile->thumbnail;

  gimp_thumbnail_set_uri (thumbnail,
                          gimp_object_get_name (GIMP_OBJECT (imagefile)));

  image_state = gimp_thumbnail_peek_image (thumbnail);

  if (image_state == GIMP_THUMB_STATE_REMOTE ||
      image_state >= GIMP_THUMB_STATE_EXISTS)
    {
      GimpImage    *image;
      gboolean      success;
      gint          width     = 0;
      gint          height    = 0;
      const gchar  *mime_type = NULL;
      GError       *error     = NULL;

      g_object_ref (imagefile);

      image = file_open_thumbnail (imagefile->gimp, context, progress,
                                   thumbnail->image_uri, size,
                                   &mime_type, &width, &height);

      if (image)
        {
          gimp_thumbnail_set_info (imagefile->thumbnail,
                                   mime_type, width, height);
        }
      else
        {
          GimpPDBStatusType  status;

          image = file_open_image (imagefile->gimp, context, progress,
                                   thumbnail->image_uri,
                                   thumbnail->image_uri,
                                   FALSE, NULL, GIMP_RUN_NONINTERACTIVE,
                                   &status, &mime_type, NULL);

          if (image)
            gimp_thumbnail_set_info_from_image (imagefile->thumbnail,
                                                mime_type, image);
        }

      if (image)
        {
          success = gimp_imagefile_save_thumb (imagefile,
                                               image, size, replace,
                                               &error);

          g_object_unref (image);
        }
      else
        {
          success = gimp_thumbnail_save_failure (thumbnail,
                                                 "GIMP " GIMP_VERSION,
                                                 &error);
          gimp_imagefile_update (imagefile);
        }

      g_object_unref (imagefile);

      if (! success)
        {
          gimp_message (imagefile->gimp, G_OBJECT (progress),
                        GIMP_MESSAGE_ERROR,
                        "%s", error->message);
          g_clear_error (&error);
        }
    }
}

/*  The weak version doesn't ref the imagefile but deals gracefully
 *  with an imagefile that is destroyed while the thumbnail is
 *  created. Thia allows to use this function w/o the need to block
 *  the user interface.
 */
void
gimp_imagefile_create_thumbnail_weak (GimpImagefile *imagefile,
                                      GimpContext   *context,
                                      GimpProgress  *progress,
                                      gint           size,
                                      gboolean       replace)
{
  GimpImagefile *local;
  const gchar   *uri;

  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));

  if (! imagefile->gimp->config->layer_previews)
    return;

  if (size < 1)
    return;

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));
  if (!uri)
    return;

  local = gimp_imagefile_new (imagefile->gimp, uri);

  g_object_add_weak_pointer (G_OBJECT (imagefile), (gpointer) &imagefile);

  gimp_imagefile_create_thumbnail (local, context, progress, size, replace);

  if (imagefile)
    {
      uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

      if (uri &&
          strcmp (uri, gimp_object_get_name (GIMP_OBJECT (local))) == 0)
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
  gint  size;

  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), FALSE);

  size = imagefile->gimp->config->thumbnail_size;

  if (size > 0)
    {
      GimpThumbState  state;

      state = gimp_thumbnail_check_thumb (imagefile->thumbnail, size);

      return (state == GIMP_THUMB_STATE_OK);
    }

  return TRUE;
}

gboolean
gimp_imagefile_save_thumbnail (GimpImagefile *imagefile,
                               const gchar   *mime_type,
                               GimpImage     *image)
{
  gint      size;
  gboolean  success = TRUE;
  GError   *error   = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  size = imagefile->gimp->config->thumbnail_size;

  if (size > 0)
    {
      gimp_thumbnail_set_info_from_image (imagefile->thumbnail,
                                          mime_type, image);

      success = gimp_imagefile_save_thumb (imagefile,
                                           image, size, FALSE,
                                           &error);
      if (! success)
        {
          gimp_message (imagefile->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "%s", error->message);
          g_clear_error (&error);
        }
    }

  return success;
}

static void
gimp_imagefile_name_changed (GimpObject *object)
{
  GimpImagefile *imagefile = GIMP_IMAGEFILE (object);

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  gimp_viewable_set_stock_id (GIMP_VIEWABLE (imagefile), NULL);

  gimp_thumbnail_set_uri (imagefile->thumbnail, gimp_object_get_name (object));
}

static void
gimp_imagefile_info_changed (GimpImagefile *imagefile)
{
  if (imagefile->description)
    {
      if (! imagefile->static_desc)
        g_free (imagefile->description);

      imagefile->description = NULL;
    }

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

static GdkPixbuf *
gimp_imagefile_get_new_pixbuf (GimpViewable *viewable,
                               GimpContext  *context,
                               gint          width,
                               gint          height)
{
  GimpImagefile *imagefile = GIMP_IMAGEFILE (viewable);
  GdkPixbuf     *pixbuf;
  const gchar   *stock_id  = NULL;

  if (! GIMP_OBJECT (imagefile)->name)
    return NULL;

  pixbuf = gimp_imagefile_load_thumb (imagefile, width, height);

  switch (imagefile->thumbnail->image_state)
    {
    case GIMP_THUMB_STATE_REMOTE:
      stock_id = "gtk-network";
      break;

    case GIMP_THUMB_STATE_FOLDER:
      stock_id = "gtk-directory";
      break;

    case GIMP_THUMB_STATE_SPECIAL:
      stock_id = "gtk-harddisk";
      break;

    case GIMP_THUMB_STATE_NOT_FOUND:
      stock_id = "gtk-dialog-question";
      break;

    default:
      break;
    }

  gimp_viewable_set_stock_id (GIMP_VIEWABLE (imagefile), stock_id);

  return pixbuf;
}

static gchar *
gimp_imagefile_get_description (GimpViewable   *viewable,
                                gchar         **tooltip)
{
  GimpImagefile *imagefile = GIMP_IMAGEFILE (viewable);
  GimpThumbnail *thumbnail = imagefile->thumbnail;
  gchar         *basename;

  if (! thumbnail->image_uri)
    return NULL;

  if (tooltip)
    {
      gchar       *filename;
      const gchar *desc;

      filename = file_utils_uri_display_name (thumbnail->image_uri);
      desc     = gimp_imagefile_get_desc_string (imagefile);

      if (desc)
        {
          *tooltip = g_strdup_printf ("%s\n%s", filename, desc);
          g_free (filename);
        }
      else
        {
          *tooltip = filename;
        }
    }

  basename = file_utils_uri_display_basename (thumbnail->image_uri);

  if (thumbnail->image_width > 0 && thumbnail->image_height > 0)
    {
      gchar *tmp = basename;

      basename = g_strdup_printf ("%s (%d × %d)",
                                  tmp,
                                  thumbnail->image_width,
                                  thumbnail->image_height);
      g_free (tmp);
    }

  return basename;
}

const gchar *
gimp_imagefile_get_desc_string (GimpImagefile *imagefile)
{
  GimpThumbnail *thumbnail;

  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), NULL);

  if (imagefile->description)
    return (const gchar *) imagefile->description;

  thumbnail = imagefile->thumbnail;

  switch (thumbnail->image_state)
    {
    case GIMP_THUMB_STATE_UNKNOWN:
      imagefile->description = NULL;
      imagefile->static_desc = TRUE;
      break;

    case GIMP_THUMB_STATE_FOLDER:
      imagefile->description = _("Folder");
      imagefile->static_desc = TRUE;
      break;

    case GIMP_THUMB_STATE_SPECIAL:
      imagefile->description = _("Special File");
      imagefile->static_desc = TRUE;
      break;

    case GIMP_THUMB_STATE_NOT_FOUND:
      imagefile->description =
        (gchar *) g_strerror (thumbnail->image_not_found_errno);
      imagefile->static_desc = TRUE;
      break;

    default:
      {
        GString *str = g_string_new (NULL);

        if (thumbnail->image_state == GIMP_THUMB_STATE_REMOTE)
          {
            g_string_append (str, _("Remote File"));
          }

        if (thumbnail->image_filesize > 0)
          {
            gchar *size = gimp_memsize_to_string (thumbnail->image_filesize);

            if (str->len > 0)
              g_string_append_c (str, '\n');

            g_string_append (str, size);
            g_free (size);
          }

        switch (thumbnail->thumb_state)
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
              if (thumbnail->image_state == GIMP_THUMB_STATE_REMOTE)
                {
                  if (str->len > 0)
                    g_string_append_c (str, '\n');

                  g_string_append (str, _("(Preview may be out of date)"));
                }

              if (thumbnail->image_width > 0 && thumbnail->image_height > 0)
                {
                  if (str->len > 0)
                    g_string_append_c (str, '\n');

                  g_string_append_printf (str,
                                          ngettext ("%d × %d pixel",
                                                    "%d × %d pixels",
                                                    thumbnail->image_height),
                                          thumbnail->image_width,
                                          thumbnail->image_height);
                }

              if (thumbnail->image_type)
                {
                  if (str->len > 0)
                    g_string_append_c (str, '\n');

                  g_string_append (str, gettext (thumbnail->image_type));
                }

              if (thumbnail->image_num_layers > 0)
                {
                  if (thumbnail->image_type)
                    g_string_append_len (str, ", ", 2);
                  else if (str->len > 0)
                    g_string_append_c (str, '\n');

                  g_string_append_printf (str,
                                          ngettext ("%d layer",
                                                    "%d layers",
                                                    thumbnail->image_num_layers),
                                          thumbnail->image_num_layers);
                }
            }
            break;

          default:
            break;
          }

        imagefile->description = g_string_free (str, FALSE);
        imagefile->static_desc = FALSE;
      }
    }

  return (const gchar *) imagefile->description;
}

static GdkPixbuf *
gimp_imagefile_load_thumb (GimpImagefile *imagefile,
                           gint           width,
                           gint           height)
{
  GimpThumbnail *thumbnail = imagefile->thumbnail;
  GdkPixbuf     *pixbuf    = NULL;
  GError        *error     = NULL;
  gint           size      = MAX (width, height);
  gint           pixbuf_width;
  gint           pixbuf_height;
  gint           preview_width;
  gint           preview_height;

  if (gimp_thumbnail_peek_thumb (thumbnail, size) < GIMP_THUMB_STATE_EXISTS)
    return NULL;

  if (thumbnail->image_state == GIMP_THUMB_STATE_NOT_FOUND)
    return NULL;

  pixbuf = gimp_thumbnail_load_thumb (thumbnail, size, &error);

  if (! pixbuf)
    {
      if (error)
        {
          gimp_message (imagefile->gimp, NULL, GIMP_MESSAGE_ERROR,
                        _("Could not open thumbnail '%s': %s"),
                        thumbnail->thumb_filename, error->message);
          g_clear_error (&error);
        }

      return NULL;
    }

  pixbuf_width  = gdk_pixbuf_get_width (pixbuf);
  pixbuf_height = gdk_pixbuf_get_height (pixbuf);

  gimp_viewable_calc_preview_size (pixbuf_width,
                                   pixbuf_height,
                                   width,
                                   height,
                                   TRUE, 1.0, 1.0,
                                   &preview_width,
                                   &preview_height,
                                   NULL);

  if (preview_width < pixbuf_width || preview_height < pixbuf_height)
    {
      GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
                                                   preview_width,
                                                   preview_height,
                                                   GDK_INTERP_BILINEAR);
      g_object_unref (pixbuf);
      pixbuf = scaled;

      pixbuf_width  = preview_width;
      pixbuf_height = preview_height;
    }

  if (gdk_pixbuf_get_n_channels (pixbuf) != 3)
    {
      GdkPixbuf *tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                                       pixbuf_width, pixbuf_height);

      gdk_pixbuf_composite_color (pixbuf, tmp,
                                  0, 0, pixbuf_width, pixbuf_height,
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
  GimpThumbnail *thumbnail = imagefile->thumbnail;
  GdkPixbuf     *pixbuf;
  gint           width, height;
  gboolean       success = FALSE;

  if (size < 1)
    return TRUE;

  if (image->width <= size && image->height <= size)
    {
      width  = image->width;
      height = image->height;

      size = MAX (width, height);
    }
  else
    {
      if (image->width < image->height)
        {
          height = size;
          width  = MAX (1, (size * image->width) / image->height);
        }
      else
        {
          width  = size;
          height = MAX (1, (size * image->height) / image->width);
        }
    }

  pixbuf = gimp_viewable_get_new_pixbuf (GIMP_VIEWABLE (image),
                                         /* random context, unused */
                                         gimp_get_user_context (image->gimp),
                                         width, height);

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
  GimpEnumDesc  *desc;
  GimpImageType  type;

  /*  peek the thumbnail to make sure that mtime and filesize are set  */
  gimp_thumbnail_peek_image (thumbnail);

  type = GIMP_IMAGE_TYPE_FROM_BASE_TYPE (gimp_image_base_type (image));

  if (gimp_image_has_alpha (image))
    type = GIMP_IMAGE_TYPE_WITH_ALPHA (type);

  desc = gimp_enum_get_desc (g_type_class_peek (GIMP_TYPE_IMAGE_TYPE), type);

  g_object_set (thumbnail,
                "image-mimetype",   mime_type,
                "image-width",      gimp_image_get_width (image),
                "image-height",     gimp_image_get_height (image),
                "image-type",       desc->value_desc,
                "image-num-layers", gimp_container_num_children (image->layers),
                NULL);
}

static void
gimp_thumbnail_set_info (GimpThumbnail *thumbnail,
                         const gchar   *mime_type,
                         gint           width,
                         gint           height)
{
  /*  peek the thumbnail to make sure that mtime and filesize are set  */
  gimp_thumbnail_peek_image (thumbnail);

  g_object_set (thumbnail,
                "image-mimetype",   mime_type,
                "image-width",      width,
                "image-height",     height,
                "image-type",       NULL,
                "image-num-layers", NULL,
                NULL);
}
