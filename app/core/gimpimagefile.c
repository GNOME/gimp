/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfileimage.c
 *
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
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

#include "base/temp-buf.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpimage.h"
#include "gimpimagefile.h"
#include "gimpmarshal.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "gimp-intl.h"


enum
{
  INFO_CHANGED,
  LAST_SIGNAL
};

static void       gimp_imagefile_class_init       (GimpImagefileClass *klass);
static void       gimp_imagefile_init             (GimpImagefile  *imagefile);
static void       gimp_imagefile_finalize         (GObject        *object);
static void       gimp_imagefile_name_changed     (GimpObject     *object);
static void       gimp_imagefile_info_changed     (GimpImagefile  *imagefile);
static void       gimp_imagefile_notify_thumbnail (GimpImagefile  *imagefile,
                                                   GParamSpec     *pspec);

static TempBuf  * gimp_imagefile_get_new_preview  (GimpViewable   *viewable,
                                                   gint            width,
                                                   gint            height);
static TempBuf  * gimp_imagefile_load_thumb       (GimpImagefile  *imagefile,
                                                   gint            size);
static gboolean   gimp_imagefile_save_thumb       (GimpImagefile  *imagefile,
                                                   GimpImage      *gimage,
                                                   gint            size,
                                                   GError        **error);

static gchar    * gimp_imagefile_get_description  (GimpViewable   *viewable,
                                                   gchar         **tooltip);


static guint gimp_imagefile_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GType
gimp_imagefile_get_type (void)
{
  static GType imagefile_type = 0;

  if (!imagefile_type)
    {
      static const GTypeInfo imagefile_info =
      {
        sizeof (GimpImagefileClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_imagefile_class_init,
	NULL,           /* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpImagefile),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_imagefile_init,
      };

      imagefile_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
					       "GimpImagefile",
                                               &imagefile_info, 0);
    }

  return imagefile_type;
}

static void
gimp_imagefile_class_init (GimpImagefileClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;
  gchar             *creator;

  parent_class = g_type_class_peek_parent (klass);

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  gimp_imagefile_signals[INFO_CHANGED] =
    g_signal_new ("info_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpImagefileClass, info_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize              = gimp_imagefile_finalize;

  gimp_object_class->name_changed     = gimp_imagefile_name_changed;

  viewable_class->name_changed_signal = "info_changed";
  viewable_class->get_new_preview     = gimp_imagefile_get_new_preview;
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
gimp_imagefile_update (GimpImagefile *imagefile,
                       gint           size)
{
  gchar *uri;

  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));

  if (size < 1)
    return;

  gimp_thumbnail_set_uri (imagefile->thumbnail,
                          gimp_object_get_name (GIMP_OBJECT (imagefile)));

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (imagefile));

  g_object_get (imagefile->thumbnail,
                "image-uri", &uri,
                NULL);

  if (uri)
    {
      GimpImagefile *documents_imagefile;

      documents_imagefile = (GimpImagefile *)
        gimp_container_get_child_by_name (imagefile->gimp->documents, uri);

      if (GIMP_IS_IMAGEFILE (documents_imagefile) &&
          (documents_imagefile != imagefile))
        {
          gimp_imagefile_update (documents_imagefile, size);
        }

      g_free (uri);
    }
}

void
gimp_imagefile_create_thumbnail (GimpImagefile *imagefile,
                                 gint           size)
{
  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));

  if (! imagefile->gimp->config->layer_previews)
    return;

  if (size < 1)
    return;

  gimp_thumbnail_set_uri (imagefile->thumbnail,
                          gimp_object_get_name (GIMP_OBJECT (imagefile)));

  if (gimp_thumbnail_peek_image (imagefile->thumbnail) >= GIMP_THUMB_STATE_EXISTS)
    {
      GimpImage         *gimage;
      GimpPDBStatusType  dummy;
      gboolean           success;
      GError            *error = NULL;

      gimage = file_open_image (imagefile->gimp,
                                imagefile->thumbnail->image_uri,
                                imagefile->thumbnail->image_uri,
                                NULL,
                                GIMP_RUN_NONINTERACTIVE,
                                &dummy,
                                NULL);

      if (gimage)
        {
          success = gimp_imagefile_save_thumb (imagefile,
                                               gimage, size, &error);

          g_object_unref (gimage);
        }
      else
        {
          success = gimp_thumbnail_save_failure (imagefile->thumbnail,
                                                 "The GIMP " GIMP_VERSION,
                                                 &error);
        }

      if (!success)
        {
          g_message (error->message);
          g_error_free (error);
        }
    }
}

gboolean
gimp_imagefile_save_thumbnail (GimpImagefile *imagefile,
                               GimpImage     *gimage)
{
  gboolean  success;
  GError   *error = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  success = gimp_imagefile_save_thumb (imagefile,
                                       gimage,
                                       gimage->gimp->config->thumbnail_size,
                                       &error);

  if (! success)
    {
      g_message (error->message);
      g_error_free (error);
    }

  return success;
}

static void
gimp_imagefile_name_changed (GimpObject *object)
{
  GimpImagefile *imagefile = GIMP_IMAGEFILE (object);

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

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

static TempBuf *
gimp_imagefile_get_new_preview (GimpViewable *viewable,
                                gint          width,
                                gint          height)
{
  GimpImagefile *imagefile;
  TempBuf       *temp_buf;

  imagefile = GIMP_IMAGEFILE (viewable);

  if (! GIMP_OBJECT (imagefile)->name)
    return NULL;

  temp_buf = gimp_imagefile_load_thumb (imagefile, MAX (width, height));

  if (temp_buf)
    {
      gint preview_width;
      gint preview_height;

      gimp_viewable_calc_preview_size (temp_buf->width,
                                       temp_buf->height,
                                       width,
                                       height,
                                       TRUE, 1.0, 1.0,
                                       &preview_width,
                                       &preview_height,
                                       NULL);

      if (preview_width  < temp_buf->width &&
          preview_height < temp_buf->height)
        {
          TempBuf *scaled_buf;

          scaled_buf = temp_buf_scale (temp_buf,
                                       preview_width, preview_height);

          temp_buf_free (temp_buf);

          return scaled_buf;
        }
    }

  return temp_buf;
}

static gchar *
gimp_imagefile_get_description (GimpViewable   *viewable,
                                gchar         **tooltip)
{
  GimpImagefile *imagefile;
  GimpThumbnail *thumbnail;
  gchar         *basename;

  imagefile = GIMP_IMAGEFILE (viewable);
  thumbnail = imagefile->thumbnail;

  if (! thumbnail->image_uri)
    return NULL;

  if (tooltip)
    {
      gchar       *filename;
      const gchar *desc;

      filename = file_utils_uri_to_utf8_filename (thumbnail->image_uri);
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

  basename = file_utils_uri_to_utf8_basename (thumbnail->image_uri);

  if (thumbnail->image_width > 0 && thumbnail->image_height > 0)
    {
      gchar *tmp = basename;

      basename = g_strdup_printf ("%s (%d x %d)",
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

    case GIMP_THUMB_STATE_REMOTE:
      imagefile->description = _("Remote image");
      imagefile->static_desc = TRUE;
      break;

    case GIMP_THUMB_STATE_NOT_FOUND:
      imagefile->description = _("Could not open");
      imagefile->static_desc = TRUE;
      break;

    default:
      {
        GString *str = g_string_new (NULL);

        if (thumbnail->image_filesize > 0)
          {
            gchar *size = gimp_memsize_to_string (thumbnail->image_filesize);

            g_string_append (str, size);
            g_free (size);

            g_string_append_c (str, '\n');
          }

        switch (thumbnail->thumb_state)
          {
          case GIMP_THUMB_STATE_NOT_FOUND:
            g_string_append (str, _("No preview available"));
            break;

          case GIMP_THUMB_STATE_EXISTS:
            g_string_append (str, _("Loading preview ..."));
            break;

          case GIMP_THUMB_STATE_OLD:
            g_string_append (str, _("Preview is out of date"));
            break;

          case GIMP_THUMB_STATE_FAILED:
            g_string_append (str, _("Cannot create preview"));
            break;

          case GIMP_THUMB_STATE_OK:
            {
              if (thumbnail->image_width > 0 && thumbnail->image_height > 0)
                {
                  g_string_append_printf (str, _("%d x %d pixels"),
                                          thumbnail->image_width,
                                          thumbnail->image_height);
                  g_string_append_c (str, '\n');
                }

              if (thumbnail->image_type)
                g_string_append (str, gettext (thumbnail->image_type));

              if (thumbnail->image_num_layers > 0)
                {
                  if (thumbnail->image_type)
                    g_string_append_len (str, ", ", 2);

                  if (thumbnail->image_num_layers == 1)
                    g_string_append (str, _("1 Layer"));
                  else
                    g_string_append_printf (str, _("%d Layers"),
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

static TempBuf *
gimp_imagefile_load_thumb (GimpImagefile *imagefile,
                           gint           size)
{
  GimpThumbnail *thumbnail;
  TempBuf       *temp_buf = NULL;
  GdkPixbuf     *pixbuf   = NULL;
  GError        *error    = NULL;

  thumbnail = imagefile->thumbnail;

  if (gimp_thumbnail_peek_thumb (thumbnail, size) < GIMP_THUMB_STATE_EXISTS)
    return NULL;

  pixbuf = gimp_thumbnail_load_thumb (thumbnail, size, &error);

  if (!pixbuf)
    {
      if (error)
        g_message (_("Could not open thumbnail '%s': %s"),
                   thumbnail->thumb_filename, error->message);
      goto cleanup;
    }

  {
    gint    width;
    gint    height;
    gint    bytes;
    guchar *src;
    guchar *dest;
    gint    y;

    width  = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);
    bytes  = gdk_pixbuf_get_n_channels (pixbuf);

    temp_buf = temp_buf_new (width, height, bytes, 0, 0, NULL);

    dest = temp_buf_data (temp_buf);
    src  = gdk_pixbuf_get_pixels (pixbuf);

    for (y = 0; y < height; y++)
      {
        memcpy (dest, src, width * bytes);
        dest += width * bytes;
        src += gdk_pixbuf_get_rowstride (pixbuf);
      }
  }

 cleanup:
  if (pixbuf)
    g_object_unref (pixbuf);
  if (error)
    g_error_free (error);

  return temp_buf;
}

static gboolean
gimp_imagefile_save_thumb (GimpImagefile  *imagefile,
                           GimpImage      *gimage,
                           gint            size,
                           GError        **error)
{
  GdkPixbuf     *pixbuf;
  GEnumClass    *enum_class;
  GimpImageType  type;
  const gchar   *type_str;
  gint           num_layers;
  gint           width, height;
  gboolean       success = FALSE;

  if (size < 1)
    return TRUE;

  if (gimage->width <= size && gimage->height <= size)
    {
      width  = gimage->width;
      height = gimage->height;

      size = MIN (size, MAX (width, height));
    }
  else
    {
      if (gimage->width < gimage->height)
        {
          height = size;
          width  = MAX (1, (size * gimage->width) / gimage->height);
        }
      else
        {
          width  = size;
          height = MAX (1, (size * gimage->height) / gimage->width);
        }
    }

  pixbuf = gimp_viewable_get_new_preview_pixbuf (GIMP_VIEWABLE (gimage),
                                                 width, height);

  /*  when layer previews are disabled, we won't get a pixbuf  */
  if (! pixbuf)
    return TRUE;

  type = GIMP_IMAGE_TYPE_FROM_BASE_TYPE (gimp_image_base_type (gimage));

  if (gimp_image_has_alpha (gimage))
    type = GIMP_IMAGE_TYPE_WITH_ALPHA (type);

  enum_class = g_type_class_peek (GIMP_TYPE_IMAGE_TYPE);
  type_str = g_enum_get_value (enum_class, type)->value_name;

  num_layers = gimp_container_num_children (gimage->layers);

  g_object_set (imagefile->thumbnail,
                "image-width",      gimage->width,
                "image-height",     gimage->height,
                "image-type",       type_str,
                "image-num-layers", num_layers,
                NULL);

  success = gimp_thumbnail_save_thumb (imagefile->thumbnail,
                                       pixbuf,
                                       "The GIMP " GIMP_VERSION,
                                       error);

  g_object_unref (pixbuf);

  if (success)
    gimp_imagefile_update (imagefile, size);

  return success;
}
