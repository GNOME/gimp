/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfileimage.c
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * http://triq.net/~pearl/thumbnail-spec/
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif

#include "libgimpbase/gimpbase.h"

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


#define TAG_DESCRIPTION        "tEXt::Description"
#define TAG_SOFTWARE           "tEXt::Software"
#define TAG_THUMB_URI          "tEXt::Thumb::URI"
#define TAG_THUMB_MTIME        "tEXt::Thumb::MTime"
#define TAG_THUMB_SIZE         "tEXt::Thumb::Size"
#define TAG_THUMB_IMAGE_WIDTH  "tEXt::Thumb::Image::Width"
#define TAG_THUMB_IMAGE_HEIGHT "tEXt::Thumb::Image::Height"
#define TAG_THUMB_GIMP_TYPE    "tEXt::Thumb::X-GIMP::Type"
#define TAG_THUMB_GIMP_LAYERS  "tEXt::Thumb::X-GIMP::Layers"


enum
{
  INFO_CHANGED,
  LAST_SIGNAL
};

typedef struct
{
  const gchar *dirname;
  gint         size;
} ThumbnailSize;


static void          gimp_imagefile_class_init      (GimpImagefileClass *klass);
static void          gimp_imagefile_init            (GimpImagefile  *imagefile);
static void          gimp_imagefile_finalize        (GObject        *object);
static void          gimp_imagefile_name_changed    (GimpObject     *object);
static void          gimp_imagefile_set_info        (GimpImagefile  *imagefile,
                                                     gboolean        emit_always,
                                                     gint            width,
                                                     gint            height,
                                                     GimpImageType   type,
                                                     gint            n_layers);
static TempBuf     * gimp_imagefile_get_new_preview (GimpViewable   *viewable,
                                                     gint            width,
                                                     gint            height);
static gchar       * gimp_imagefile_get_description (GimpViewable   *viewable,
                                                     gchar         **tooltip);

static TempBuf     * gimp_imagefile_read_png_thumb  (GimpImagefile  *imagefile,
                                                     gint            thumb_size);
static gboolean      gimp_imagefile_save_png_thumb  (GimpImagefile  *imagefile,
                                                     GimpImage      *gimage,
                                                     const gchar    *thumb_name,
                                                     gint            thumb_size,
                                                     time_t          image_mtime,
                                                     off_t           image_size);
static void          gimp_imagefile_save_fail_thumb (GimpImagefile  *imagefile,
                                                     time_t         image_mtime,
                                                     off_t          image_size);
static const gchar * gimp_imagefile_png_thumb_name  (const gchar    *uri);
static gchar       * gimp_imagefile_png_thumb_path  (const gchar    *uri,
                                                     gint           *thumb_size);
static gchar       * gimp_imagefile_find_png_thumb  (const gchar    *uri,
                                                     gint           *thumb_size);

static TempBuf     * gimp_imagefile_read_xv_thumb   (GimpImagefile  *imagefile);
static guchar      * readXVThumb                    (const gchar    *filename,
                                                     gint           *width,
                                                     gint           *height,
                                                     gchar         **imginfo);

static gboolean      gimp_imagefile_test            (const gchar    *filename,
                                                     time_t         *mtime,
                                                     off_t          *size);


#define THUMB_SIZE_FAIL -1

static const ThumbnailSize thumb_sizes[] =
{
  { "fail",   THUMB_SIZE_FAIL },
  { "normal", 128             },
  { "large",  256             }
};

static gchar *thumb_dir                                 = NULL;
static gchar *thumb_subdirs[G_N_ELEMENTS (thumb_sizes)] = { 0 };
static gchar *thumb_fail_subdir                         = NULL;

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
  gint               i;

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

  thumb_dir = g_build_filename (g_get_home_dir(), ".thumbnails", NULL);

  for (i = 0; i < G_N_ELEMENTS (thumb_sizes); i++)
    thumb_subdirs[i] = g_build_filename (g_get_home_dir(), ".thumbnails",
                                         thumb_sizes[i].dirname, NULL);

  thumb_fail_subdir = thumb_subdirs[0];
  thumb_subdirs[0] = g_strdup_printf ("%s%cgimp-%d.%d",
                                      thumb_fail_subdir, G_DIR_SEPARATOR,
                                      GIMP_MAJOR_VERSION, GIMP_MINOR_VERSION);
}

static void
gimp_imagefile_init (GimpImagefile *imagefile)
{
  imagefile->gimp        = NULL;
  imagefile->state       = GIMP_IMAGEFILE_STATE_UNKNOWN;
  imagefile->image_mtime = 0;
  imagefile->image_size  = -1;

  imagefile->width       = -1;
  imagefile->height      = -1;
  imagefile->type        = -1;
  imagefile->n_layers    = -1;

  imagefile->description = NULL;
}

static void
gimp_imagefile_finalize (GObject *object)
{
  GimpImagefile *imagefile = (GimpImagefile *) object;

  if (imagefile->description && ! imagefile->static_desc)
    g_free (imagefile->description);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpImagefile *
gimp_imagefile_new (Gimp        *gimp,
                    const gchar *uri)
{
  GimpImagefile *imagefile;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  imagefile = g_object_new (GIMP_TYPE_IMAGEFILE, NULL);

  if (uri)
    gimp_object_set_name (GIMP_OBJECT (imagefile), uri);

  imagefile->gimp = gimp;

  return imagefile;
}

void
gimp_imagefile_update (GimpImagefile *imagefile,
                       gint           thumb_size)
{
  const gchar *uri;

  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));
  g_return_if_fail (thumb_size <= 256);

  if (thumb_size < 1)
    return;

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

  if (uri)
    {
      gchar  *filename   = NULL;
      gchar  *thumbname  = NULL;
      off_t   image_size;

      filename = g_filename_from_uri (uri, NULL, NULL);

      imagefile->image_mtime = 0;
      imagefile->image_size  = -1;

      if (! filename)
        {
          /*  no thumbnails of remote images :-(  */

          imagefile->state = GIMP_IMAGEFILE_STATE_REMOTE;
          goto cleanup;
        }

      if (! gimp_imagefile_test (filename,
                                 &imagefile->image_mtime,
                                 &image_size))
        {
          imagefile->state = GIMP_IMAGEFILE_STATE_NOT_FOUND;
          goto cleanup;
        }

      /*  found the image, now look for the thumbnail  */
      imagefile->image_size = image_size;
      imagefile->state      = GIMP_IMAGEFILE_STATE_THUMBNAIL_NOT_FOUND;

      thumbname = gimp_imagefile_find_png_thumb (uri, &thumb_size);

      if (thumbname)
        imagefile->state = GIMP_IMAGEFILE_STATE_THUMBNAIL_EXISTS;

    cleanup:
      g_free (filename);
      g_free (thumbname);

      gimp_imagefile_set_info (imagefile, TRUE, -1, -1, -1, -1);
    }

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (imagefile));

  if (uri)
    {
      GimpImagefile *documents_imagefile;

      documents_imagefile = (GimpImagefile *)
        gimp_container_get_child_by_name (imagefile->gimp->documents, uri);

      if (GIMP_IS_IMAGEFILE (documents_imagefile) &&
          (documents_imagefile != imagefile))
        {
          gimp_imagefile_update (GIMP_IMAGEFILE (documents_imagefile),
                                 thumb_size);
        }
    }
}

void
gimp_imagefile_create_thumbnail (GimpImagefile *imagefile,
                                 gint           thumb_size)
{
  const gchar *uri;

  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));
  g_return_if_fail (thumb_size <= 256);

  if (! imagefile->gimp->config->layer_previews)
    return;

  if (thumb_size < 1)
    return;

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

  if (strstr (uri, "/.thumbnails/"))
    return;

  if (uri)
    {
      gchar             *filename;
      gboolean           file_exists;
      time_t             image_mtime;
      off_t              image_size;
      GimpImage         *gimage;
      GimpPDBStatusType  dummy;

      filename = g_filename_from_uri (uri, NULL, NULL);

      /*  no thumbnails of remote images :-(  */
      if (! filename)
        return;

      file_exists = gimp_imagefile_test (filename, &image_mtime, &image_size);
      g_free (filename);

      if (! file_exists)
        return;

      gimage = file_open_image (imagefile->gimp,
                                uri,
                                uri,
                                NULL,
                                GIMP_RUN_NONINTERACTIVE,
                                &dummy,
                                NULL);

      if (gimage)
        {
          gchar *thumb_name;

          thumb_size = MIN (thumb_size, MAX (gimage->width, gimage->height));
          thumb_name = gimp_imagefile_png_thumb_path (uri, &thumb_size);

          if (thumb_name)
            {
              gimp_imagefile_save_png_thumb (imagefile,
                                             gimage,
                                             thumb_name,
                                             thumb_size,
                                             image_mtime,
                                             image_size);
              g_free (thumb_name);
            }

          g_object_unref (gimage);
        }
      else
        {
          imagefile->state = GIMP_IMAGEFILE_STATE_THUMBNAIL_FAILED;

          gimp_imagefile_set_info (imagefile, TRUE, -1, -1, -1, -1);

          gimp_imagefile_save_fail_thumb (imagefile, image_mtime, image_size);
        }
    }
}

static void
gimp_imagefile_save_fail_thumb (GimpImagefile *imagefile,
                                time_t         image_mtime,
                                off_t          image_size)
{
  GdkPixbuf   *pixbuf;
  const gchar *uri;
  gint         thumb_size = THUMB_SIZE_FAIL;
  gchar       *thumb_name;
  gchar       *desc;
  gchar       *t_str;
  gchar       *s_str;
  GError      *error = NULL;

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

  thumb_name = gimp_imagefile_png_thumb_path (uri, &thumb_size);

  if (!thumb_name)
    return;

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 1, 1);

  desc  = g_strdup_printf ("Thumbnail failure for %s", uri);
  t_str = g_strdup_printf ("%ld",  image_mtime);
  s_str = g_strdup_printf ("%" G_GINT64_FORMAT, (gint64) image_size);

  if (! gdk_pixbuf_save (pixbuf, thumb_name, "png", &error,
                         TAG_DESCRIPTION,        desc,
                         TAG_SOFTWARE,           ("The GIMP "
                                                  GIMP_VERSION),
                         TAG_THUMB_URI,          uri,
                         TAG_THUMB_MTIME,        t_str,
                         TAG_THUMB_SIZE,         s_str,
                         NULL))
    {
      g_message (_("Could not write thumbnail for '%s' as '%s': %s"),
                 uri, thumb_name, error->message);
      g_error_free (error);
    }
  else if (chmod (thumb_name, 0600))
    {
      g_message (_("Could not set permissions of thumbnail '%s': %s"),
                 thumb_name, g_strerror (errno));
    }

  g_object_unref (pixbuf);

  g_free (desc);
  g_free (t_str);
  g_free (s_str);
  g_free (thumb_name);
}

gboolean
gimp_imagefile_save_thumbnail (GimpImagefile *imagefile,
                               GimpImage     *gimage)
{
  const gchar *uri;
  const gchar *image_uri;
  gchar       *filename;
  gint         thumb_size;
  gchar       *thumb_name;
  time_t       image_mtime;
  off_t        image_size;
  gboolean     success = FALSE;

  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  thumb_size = imagefile->gimp->config->thumbnail_size;

  g_return_val_if_fail (thumb_size <= 256, FALSE);

  if (! imagefile->gimp->config->layer_previews)
    return TRUE;

  if (thumb_size < 1)
    return TRUE;

  uri       = gimp_object_get_name (GIMP_OBJECT (imagefile));
  image_uri = gimp_object_get_name (GIMP_OBJECT (gimage));

  g_return_val_if_fail (uri && image_uri && ! strcmp (uri, image_uri), FALSE);

  if (strstr (uri, "/.thumbnails/"))
    return FALSE;

  filename = g_filename_from_uri (uri, NULL, NULL);

  /*  no thumbnails of remote images :-(  */
  if (! filename)
    return FALSE;

  thumb_size = MIN (thumb_size, MAX (gimage->width, gimage->height));
  thumb_name = gimp_imagefile_png_thumb_path (uri, &thumb_size);

  /*  the thumbnail directory exists or could be created */
  if (thumb_name)
    {
      if (gimp_imagefile_test (filename, &image_mtime, &image_size))
        {
          success = gimp_imagefile_save_png_thumb (imagefile,
                                                   gimage,
                                                   thumb_name,
                                                   thumb_size,
                                                   image_mtime,
                                                   image_size);
        }

      g_free (thumb_name);
    }

  g_free (filename);

  return success;
}

static void
gimp_imagefile_name_changed (GimpObject *object)
{
  GimpImagefile *imagefile;

  imagefile = GIMP_IMAGEFILE (object);

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  imagefile->state       = GIMP_IMAGEFILE_STATE_UNKNOWN;
  imagefile->image_mtime = 0;
  imagefile->image_size  = -1;

  gimp_imagefile_set_info (imagefile, TRUE, -1, -1, -1, -1);
}

static void
gimp_imagefile_set_info (GimpImagefile *imagefile,
                         gboolean       emit_always,
                         gint           width,
                         gint           height,
                         GimpImageType  type,
                         gint           n_layers)
{
  gboolean changed;

  changed = (imagefile->width    != width  ||
             imagefile->height   != height ||
             imagefile->type     != type   ||
             imagefile->n_layers != n_layers);

  imagefile->width    = width;
  imagefile->height   = height;
  imagefile->type     = type;
  imagefile->n_layers = n_layers;

  if (imagefile->description)
    {
      if (!imagefile->static_desc)
        g_free (imagefile->description);
      imagefile->description = NULL;
    }

  if (changed || emit_always)
    g_signal_emit (imagefile, gimp_imagefile_signals[INFO_CHANGED], 0);
}

static void
gimp_imagefile_set_info_from_pixbuf (GimpImagefile *imagefile,
                                     gboolean       emit_always,
                                     GdkPixbuf     *pixbuf)
{
  const gchar  *option;
  gint          img_width  = -1;
  gint          img_height = -1;
  GimpImageType img_type   = -1;
  gint          img_layers = -1;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_IMAGE_WIDTH);
  if (!option || sscanf (option, "%d", &img_width) != 1)
    img_width = -1;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_IMAGE_HEIGHT);
  if (!option || sscanf (option, "%d", &img_height) != 1)
    img_height = -1;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_GIMP_TYPE);
  if (option)
    {
      GEnumClass *enum_class;
      GEnumValue *enum_value;

      enum_class = g_type_class_peek (GIMP_TYPE_IMAGE_TYPE);
      enum_value = g_enum_get_value_by_nick (enum_class, option);

      if (enum_value)
        img_type = enum_value->value;
    }

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_GIMP_LAYERS);
  if (!option || sscanf (option, "%d", &img_layers) != 1)
    img_layers = -1;

  gimp_imagefile_set_info (imagefile, emit_always,
                           img_width, img_height,
                           img_type, img_layers);
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

  temp_buf = gimp_imagefile_read_png_thumb (imagefile, MAX (width, height));

  if (! temp_buf)
    temp_buf = gimp_imagefile_read_xv_thumb (imagefile);

  if (temp_buf)
    {
      gint preview_width;
      gint preview_height;

      gimp_viewable_calc_preview_size (viewable,
                                       temp_buf->width,
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
  const gchar   *uri;
  gchar         *basename;

  imagefile = GIMP_IMAGEFILE (viewable);

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

  basename = file_utils_uri_to_utf8_basename (uri);

  if (tooltip)
    {
      gchar       *filename;
      const gchar *desc;

      filename = file_utils_uri_to_utf8_filename (uri);
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

  if (imagefile->width > 0 && imagefile->height > 0)
    {
      gchar *tmp = basename;

      basename = g_strdup_printf ("%s (%d x %d)",
                                  tmp,
                                  imagefile->width,
                                  imagefile->height);
      g_free (tmp);
    }

  return basename;
}

const gchar *
gimp_imagefile_get_desc_string (GimpImagefile *imagefile)
{
  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), NULL);

  if (imagefile->description)
    return (const gchar *) imagefile->description;

  switch (imagefile->state)
    {
    case GIMP_IMAGEFILE_STATE_UNKNOWN:
      imagefile->description = NULL;
      imagefile->static_desc = TRUE;
      break;

    case GIMP_IMAGEFILE_STATE_REMOTE:
      imagefile->description = _("Remote image");
      imagefile->static_desc = TRUE;
      break;

    case GIMP_IMAGEFILE_STATE_NOT_FOUND:
      imagefile->description = _("Could not open");
      imagefile->static_desc = TRUE;
      break;

    default:
      {
        GString *str;

        str = g_string_new (NULL);

        if (imagefile->image_size > -1)
          {
            gchar *size;

            size = gimp_memsize_to_string (imagefile->image_size);

            g_string_append (str, size);
            g_free (size);

            g_string_append_c (str, '\n');
          }

        switch (imagefile->state)
          {
          case GIMP_IMAGEFILE_STATE_THUMBNAIL_NOT_FOUND:
            g_string_append (str, _("No preview available"));
            break;

          case GIMP_IMAGEFILE_STATE_THUMBNAIL_EXISTS:
            g_string_append (str, _("Loading preview ..."));
            break;

          case GIMP_IMAGEFILE_STATE_THUMBNAIL_OLD:
            g_string_append (str, _("Preview is out of date"));
            break;

          case GIMP_IMAGEFILE_STATE_THUMBNAIL_FAILED:
            g_string_append (str, _("Cannot create preview"));
            break;

          case GIMP_IMAGEFILE_STATE_THUMBNAIL_OK:
            {
              GEnumClass *enum_class;
              GEnumValue *enum_value;

              if (imagefile->width > -1 && imagefile->height > -1)
                {
                  g_string_append_printf (str, _("%d x %d pixels"),
                                          imagefile->width,
                                          imagefile->height);
                  g_string_append_c (str, '\n');
                }

              enum_class = g_type_class_peek (GIMP_TYPE_IMAGE_TYPE);
              enum_value = g_enum_get_value (enum_class, imagefile->type);

              if (enum_value)
                g_string_append (str, gettext (enum_value->value_name));

              if (imagefile->n_layers > -1)
                {
                  if (enum_value)
                    g_string_append_len (str, ", ", 2);

                  if (imagefile->n_layers == 1)
                    g_string_append (str, _("1 Layer"));
                  else
                    g_string_append_printf (str, _("%d Layers"),
                                            imagefile->n_layers);
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

static gboolean
gimp_imagefile_test (const gchar *filename,
                     time_t      *mtime,
                     off_t       *size)
{
  struct stat s;

  if (stat (filename, &s) == 0 && (S_ISREG (s.st_mode)))
    {
      if (mtime)
        *mtime = s.st_mtime;
      if (size)
        *size = s.st_size;

      return TRUE;
    }

  return FALSE;
}


/* PNG thumbnail handling routines according to the
   Thumbnail Managing Standard  http://triq.net/~pearl/thumbnail-spec/  */

static TempBuf *
gimp_imagefile_read_png_thumb (GimpImagefile *imagefile,
                               gint           thumb_size)
{
  GimpImagefileState  old_state;
  TempBuf            *temp_buf   = NULL;
  GdkPixbuf          *pixbuf     = NULL;
  gchar              *thumbname  = NULL;
  const gchar        *option;
  gint                width;
  gint                height;
  gint                bytes;
  time_t              thumb_image_mtime;
  gint64              thumb_image_size;
  guchar             *src;
  guchar             *dest;
  gint                y;
  GError             *error = NULL;

  if (imagefile->state < GIMP_IMAGEFILE_STATE_EXISTS)
    return NULL;

  old_state = imagefile->state;

  /* try to locate a thumbnail for this image */
  imagefile->state = GIMP_IMAGEFILE_STATE_THUMBNAIL_NOT_FOUND;

  thumbname = gimp_imagefile_find_png_thumb (GIMP_OBJECT (imagefile)->name,
                                             &thumb_size);

  if (!thumbname)
    goto cleanup;

  pixbuf = gdk_pixbuf_new_from_file (thumbname, &error);

  if (!pixbuf)
    {
      g_message (_("Could not open thumbnail '%s': %s"),
                 thumbname, error->message);
      goto cleanup;
    }

  g_free (thumbname);
  thumbname = NULL;

  /* URI and mtime from the thumbnail need to match our file */

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_URI);
  if (!option || strcmp (option, GIMP_OBJECT (imagefile)->name))
    goto cleanup;

  imagefile->state = GIMP_IMAGEFILE_STATE_THUMBNAIL_OLD;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_MTIME);
  if (!option || sscanf (option, "%ld", &thumb_image_mtime) != 1)
    goto cleanup;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_SIZE);
  if (option && sscanf (option, "%" G_GINT64_FORMAT, &thumb_image_size) != 1)
    goto cleanup;

  /* TAG_THUMB_SIZE is optional but must match if present */
  if (thumb_image_mtime == imagefile->image_mtime &&
      (option == NULL || thumb_image_size == (gint64) imagefile->image_size))
    {
      if (thumb_size == THUMB_SIZE_FAIL)
        imagefile->state = GIMP_IMAGEFILE_STATE_THUMBNAIL_FAILED;
      else
        imagefile->state = GIMP_IMAGEFILE_STATE_THUMBNAIL_OK;
    }

  if (thumb_size == THUMB_SIZE_FAIL)
    {
      gimp_imagefile_set_info (imagefile, TRUE, -1, -1, -1, -1);
      goto cleanup;
    }

  /* now convert the pixbuf to a tempbuf */

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

  /* extract into about the original file from the pixbuf */
  gimp_imagefile_set_info_from_pixbuf (imagefile,
                                       imagefile->state != old_state,
                                       pixbuf);

 cleanup:
  g_free (thumbname);
  if (pixbuf)
    g_object_unref (pixbuf);
  if (error)
    g_error_free (error);

  return temp_buf;
}

static gboolean
gimp_imagefile_save_png_thumb (GimpImagefile *imagefile,
                               GimpImage     *gimage,
                               const gchar   *thumb_name,
                               gint           thumb_size,
                               time_t         image_mtime,
                               off_t          image_size)
{
  const gchar *uri;
  gchar       *temp_name = NULL;
  GdkPixbuf   *pixbuf;
  gint         width, height;
  gboolean     success = FALSE;

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

  if (gimage->width <= thumb_size && gimage->height <= thumb_size)
    {
      width  = gimage->width;
      height = gimage->height;

      thumb_size = MIN (thumb_size, MAX (width, height));
    }
  else
    {
      if (gimage->width < gimage->height)
        {
          height = thumb_size;
          width  = MAX (1, (thumb_size * gimage->width) / gimage->height);
        }
      else
        {
          width  = thumb_size;
          height = MAX (1, (thumb_size * gimage->height) / gimage->width);
        }
    }

  pixbuf = gimp_viewable_get_new_preview_pixbuf (GIMP_VIEWABLE (gimage),
                                                 width, height);

  {
    GEnumClass    *enum_class;
    GimpImageType  type;
    gchar         *type_str;
    gchar         *desc;
    gchar         *t_str;
    gchar         *w_str;
    gchar         *h_str;
    gchar         *s_str;
    gchar         *l_str;
    GError        *error = NULL;

    type = GIMP_IMAGE_TYPE_FROM_BASE_TYPE (gimp_image_base_type (gimage));

    if (gimp_image_has_alpha (gimage))
      type = GIMP_IMAGE_TYPE_WITH_ALPHA (type);

    enum_class = g_type_class_peek (GIMP_TYPE_IMAGE_TYPE);
    type_str = g_enum_get_value (enum_class, type)->value_nick;

    desc  = g_strdup_printf ("Thumbnail of %s", uri);
    t_str = g_strdup_printf ("%ld",  image_mtime);
    w_str = g_strdup_printf ("%d",   gimage->width);
    h_str = g_strdup_printf ("%d",   gimage->height);
    s_str = g_strdup_printf ("%" G_GINT64_FORMAT, (gint64) image_size);
    l_str = g_strdup_printf ("%d",   gimage->layers->num_children);

    success =  gdk_pixbuf_save (pixbuf, thumb_name, "png", &error,
                                TAG_DESCRIPTION,        desc,
                                TAG_SOFTWARE,           ("The GIMP "
                                                         GIMP_VERSION),
                                TAG_THUMB_URI,          uri,
                                TAG_THUMB_MTIME,        t_str,
                                TAG_THUMB_SIZE,         s_str,
                                TAG_THUMB_IMAGE_WIDTH,  w_str,
                                TAG_THUMB_IMAGE_HEIGHT, h_str,
                                TAG_THUMB_GIMP_TYPE,    type_str,
                                TAG_THUMB_GIMP_LAYERS,  l_str,
                                NULL);

    if (! success)
      {
        g_message (_("Could not write thumbnail for '%s' as '%s': %s"),
                   uri, thumb_name, error->message);
        g_error_free (error);
      }
    else if (chmod (thumb_name, 0600))
      {
        g_message (_("Could not set permissions of thumbnail '%s': %s"),
                   thumb_name, g_strerror (errno));
      }

    g_free (desc);
    g_free (t_str);
    g_free (w_str);
    g_free (h_str);
    g_free (s_str);
    g_free (l_str);
  }

  g_object_unref (pixbuf);

  g_free (temp_name);

  gimp_imagefile_update (imagefile, thumb_size);

  return success;
}

static const gchar *
gimp_imagefile_png_thumb_name (const gchar *uri)
{
  static gchar name[40];
  guchar       digest[16];
  guchar       n;
  gint         i;

  gimp_md5_get_digest (uri, -1, digest);

  for (i = 0; i < 16; i++)
    {
      n = (digest[i] >> 4) & 0xF;
      name[i * 2]     = (n > 9) ? 'a' + n - 10 : '0' + n;

      n = digest[i] & 0xF;
      name[i * 2 + 1] = (n > 9) ? 'a' + n - 10 : '0' + n;
    }

  strncpy (name + 32, ".png", 5);

  return (const gchar *) name;
}

static gchar *
gimp_imagefile_png_thumb_path (const gchar *uri,
                               gint        *thumb_size)
{
  const gchar *name;
  gchar       *thumb_name = NULL;
  gint         i;

  if (strstr (uri, "/.thumbnails/"))
    return NULL;

  name = gimp_imagefile_png_thumb_name (uri);

  if (*thumb_size == THUMB_SIZE_FAIL)
    {
      i = 0;
    }
  else
    {
      for (i = 1;
           i < G_N_ELEMENTS (thumb_sizes) && thumb_sizes[i].size < *thumb_size;
           i++)
        /* nothing */;

      if (i == G_N_ELEMENTS (thumb_sizes))
        i--;
    }

  *thumb_size = thumb_sizes[i].size;

  if (! g_file_test (thumb_subdirs[i], G_FILE_TEST_IS_DIR))
    {
      if (g_file_test (thumb_dir, G_FILE_TEST_IS_DIR) ||
          (mkdir (thumb_dir, S_IRUSR | S_IWUSR | S_IXUSR) == 0))
        {
          if (i == 0)
            mkdir (thumb_fail_subdir, S_IRUSR | S_IWUSR | S_IXUSR);

          mkdir (thumb_subdirs[i], S_IRUSR | S_IWUSR | S_IXUSR);
        }

      if (! g_file_test (thumb_subdirs[i], G_FILE_TEST_IS_DIR))
        {
          g_message (_("Could not create thumbnail folder '%s'."),
                     thumb_subdirs[i]);
          return NULL;
        }
    }

  thumb_name = g_build_filename (thumb_subdirs[i], name, NULL);

  return thumb_name;
}

static gchar *
gimp_imagefile_find_png_thumb (const gchar *uri,
                               gint        *thumb_size)
{
  const gchar *name;
  gchar       *thumb_name;
  gint         i, n;

  name = gimp_imagefile_png_thumb_name (uri);

  n = G_N_ELEMENTS (thumb_sizes);
  for (i = 1; i < n && thumb_sizes[i].size < *thumb_size; i++)
    /* nothing */;

  n = i;
  for (; i < G_N_ELEMENTS (thumb_sizes); i++)
    {
      thumb_name = g_build_filename (thumb_subdirs[i], name, NULL);

      if (gimp_imagefile_test (thumb_name, NULL, NULL))
        {
          *thumb_size = thumb_sizes[i].size;
          return thumb_name;
        }

      g_free (thumb_name);
    }

  for (i = n - 1; i >= 0; i--)
    {
      thumb_name = g_build_filename (thumb_subdirs[i], name, NULL);

      if (gimp_imagefile_test (thumb_name, NULL, NULL))
        {
          *thumb_size = thumb_sizes[i].size;
          return thumb_name;
        }

      g_free (thumb_name);
    }

  return NULL;
}

/* xvpics thumbnail reading routines for backward compatibility */

static TempBuf *
gimp_imagefile_read_xv_thumb (GimpImagefile *imagefile)
{
  gchar    *filename;
  gchar    *basename;
  gchar    *dirname;
  gchar    *thumbname;
  time_t    file_mtime;
  time_t    thumb_mtime;
  gint      width;
  gint      height;
  gint      x, y;
  TempBuf  *temp_buf;
  guchar   *src;
  guchar   *dest;
  guchar   *raw_thumb  = NULL;
  gchar    *image_info = NULL;

  filename = g_filename_from_uri (GIMP_OBJECT (imagefile)->name, NULL, NULL);

  /* can't load thumbnails of non "file:" URIs */
  if (! filename)
    return NULL;

  /* check if the image file exists at all */
  if (!gimp_imagefile_test (filename, &file_mtime, NULL))
    {
      g_free (filename);
      return NULL;
    }

  dirname  = g_path_get_dirname (filename);
  basename = g_path_get_basename (filename);

  g_free (filename);

  thumbname = g_build_filename (dirname, ".xvpics", basename, NULL);

  g_free (dirname);
  g_free (basename);

  if (gimp_imagefile_test (thumbname, &thumb_mtime, NULL) &&
      thumb_mtime >= file_mtime)
    {
      raw_thumb = readXVThumb (thumbname, &width, &height, &image_info);
    }

  g_free (thumbname);

  if (!raw_thumb)
    return NULL;

  imagefile->state = GIMP_IMAGEFILE_STATE_THUMBNAIL_OLD;

  if (image_info)
    {
      gint img_width;
      gint img_height;

      if (sscanf (image_info, "%dx%d", &img_width, &img_height) != 2)
        {
          img_width  = 0;
          img_height = 0;
        }
      gimp_imagefile_set_info (imagefile, FALSE,
                               img_width, img_height, -1, -1);

      g_free (image_info);
    }

  temp_buf = temp_buf_new (width, height, 3, 0, 0, NULL);
  src  = raw_thumb;
  dest = temp_buf_data (temp_buf);

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          dest[x*3]   =  ((src[x]>>5)*255)/7;
          dest[x*3+1] = (((src[x]>>2)&7)*255)/7;
          dest[x*3+2] =  ((src[x]&3)*255)/3;
        }
      src  += width;
      dest += width * 3;
    }

  g_free (raw_thumb);

  return temp_buf;
}


/* The readXVThumb function source may be re-used under
   the XFree86-style license. <adam@gimp.org> */
static guchar *
readXVThumb (const gchar  *fnam,
             gint         *w,
             gint         *h,
	     gchar       **imginfo)
{
  FILE *fp;
  const gchar *P7_332 = "P7 332";
  gchar P7_buf[7];
  gchar linebuf[200];
  guchar *buf;
  gint twofivefive;
  void *ptr;

  *imginfo = NULL;

  fp = fopen (fnam, "rb");
  if (!fp)
    return NULL;

  fread (P7_buf, 6, 1, fp);

  if (strncmp (P7_buf, P7_332, 6)!=0)
    {
      g_warning ("Thumbnail does not have the 'P7 332' header.");
      fclose (fp);
      return NULL;
    }

  /*newline*/
  fread (P7_buf, 1, 1, fp);

  do
    {
      ptr = fgets (linebuf, 199, fp);
      if ((strncmp (linebuf, "#IMGINFO:", 9) == 0) &&
	  (linebuf[9] != '\0') &&
	  (linebuf[9] != '\n'))
	{
	  if (linebuf[strlen(linebuf)-1] == '\n')
	    linebuf[strlen(linebuf)-1] = '\0';

	  if (linebuf[9] != '\0')
	    {
	      if (*imginfo)
		g_free (*imginfo);
	      *imginfo = g_strdup (&linebuf[9]);
	    }
	}
    }
  while (ptr && linebuf[0]=='#'); /* keep throwing away comment lines */

  if (!ptr)
    {
      /* g_warning("Thumbnail ended - not an image?"); */
      fclose (fp);
      return NULL;
    }

  sscanf (linebuf, "%d %d %d\n", w, h, &twofivefive);

  if (twofivefive!=255)
    {
      g_warning ("Thumbnail depth is incorrect.");
      fclose (fp);
      return NULL;
    }

  if ((*w)<1 || (*h)<1 || (*w)>80 || (*h)>60)
    {
      g_warning ("Thumbnail size is bad.  Corrupted?");
      fclose (fp);
      return NULL;
    }

  buf = g_malloc ((*w) * (*h));

  fread (buf, (*w) * (*h), 1, fp);
  fclose (fp);

  return buf;
}
