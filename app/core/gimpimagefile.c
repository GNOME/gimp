/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfileimage.c
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef G_OS_WIN32
#  ifndef S_ISREG
#  define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#  endif
#endif

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpimage.h"
#include "gimpimagefile.h"
#include "gimpmarshal.h"

#include "file/file-open.h"

#include "app_procs.h"

#include "libgimp/gimpintl.h"


#define TAG_DESCRIPTION        "tEXt::Description"
#define TAG_SOFTWARE           "tEXt::Software"
#define TAG_THUMB_URI          "tEXt::Thumb::URI"
#define TAG_THUMB_MTIME        "tEXt::Thumb::MTime"
#define TAG_THUMB_SIZE         "tEXt::Thumb::Size"
#define TAG_THUMB_IMAGE_WIDTH  "tEXt::Thumb::Image::Width"
#define TAG_THUMB_IMAGE_HEIGHT "tEXt::Thumb::Image::Height"
#define TAG_THUMB_GIMP_TYPE    "tEXt::Thumb::X-GIMP::Type"
#define TAG_THUMB_GIMP_LAYERS  "tEXt::Thumb::X-GIMP::Layers"


#define GIMP_IMAGEFILE_THUMB_SIZE_FAIL -1

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
                                                     gint            size,
                                                     GimpImageType   type,
                                                     gint            n_layers);
static TempBuf     * gimp_imagefile_get_new_preview (GimpViewable   *viewable,
                                                     gint            width,
                                                     gint            height);

static TempBuf     * gimp_imagefile_read_png_thumb  (GimpImagefile  *imagefile,
                                                     gint            size);
static const gchar * gimp_imagefile_png_thumb_name  (const gchar    *uri);
static gchar       * gimp_imagefile_png_thumb_path  (const gchar    *uri,
                                                     gint            size);
static gchar       * gimp_imagefile_find_png_thumb  (const gchar    *uri,
                                                     time_t          image_mtime,
                                                     gint            size,
                                                     time_t         *thumb_mtime,
                                                     gint           *thumb_size);

static TempBuf     * gimp_imagefile_read_xv_thumb   (GimpImagefile  *imagefile);
static guchar      * readXVThumb                    (const gchar    *filename,
                                                     gint           *width,
                                                     gint           *height,
                                                     gchar         **imginfo);

static gboolean      gimp_imagefile_test            (const gchar    *filename,
                                                     time_t         *mtime,
                                                     off_t          *size);


static const ThumbnailSize thumb_sizes[] =
{
  { "fail",   GIMP_IMAGEFILE_THUMB_SIZE_FAIL   },
  { "normal", GIMP_IMAGEFILE_THUMB_SIZE_NORMAL },
  { "large",  GIMP_IMAGEFILE_THUMB_SIZE_LARGE  }
};

static gchar *thumb_dir                                 = NULL;
static gchar *thumb_subdirs[G_N_ELEMENTS (thumb_sizes)] = { 0 };

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

  object_class->finalize          = gimp_imagefile_finalize;
  gimp_object_class->name_changed = gimp_imagefile_name_changed;
  viewable_class->get_new_preview = gimp_imagefile_get_new_preview;

  g_type_class_ref (GIMP_TYPE_IMAGE_TYPE);

  thumb_dir = g_build_filename (g_get_home_dir(), ".thumbnails", NULL);

  for (i = 0; i < G_N_ELEMENTS (thumb_sizes); i++)
    thumb_subdirs[i] = g_build_filename (g_get_home_dir(), ".thumbnails",
                                         thumb_sizes[i].dirname, NULL);
}

static void
gimp_imagefile_init (GimpImagefile *imagefile)
{
  imagefile->width       = -1;
  imagefile->height      = -1;
  imagefile->size        = -1;
  imagefile->type        = -1;
  imagefile->n_layers    = -1;
  imagefile->description = NULL;
  imagefile->image_state = GIMP_IMAGEFILE_STATE_UNKNOWN;
  imagefile->image_mtime = 0;
  imagefile->thumb_state = GIMP_IMAGEFILE_STATE_UNKNOWN;
  imagefile->thumb_mtime = 0;
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
gimp_imagefile_new (const gchar *uri)
{
  GimpImagefile *imagefile;

  imagefile = GIMP_IMAGEFILE (g_object_new (GIMP_TYPE_IMAGEFILE, NULL));

  if (uri)
    gimp_object_set_name (GIMP_OBJECT (imagefile), uri);

  return imagefile;
}

void
gimp_imagefile_update (GimpImagefile *imagefile)
{
  const gchar *uri;

  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

  if (uri)
    {
      gchar *filename   = NULL;
      gchar *thumbname  = NULL;
      gint   thumb_size = GIMP_IMAGEFILE_THUMB_SIZE_FAIL;

      filename = g_filename_from_uri (uri, NULL, NULL);

      imagefile->image_mtime = 0;

      if (! filename)
        {
          /*  no thumbnails of remote images :-(  */

          imagefile->image_state = GIMP_IMAGEFILE_STATE_REMOTE;
          goto cleanup;
        }

      if (! gimp_imagefile_test (filename, &imagefile->image_mtime, NULL))
        {
          imagefile->image_state = GIMP_IMAGEFILE_STATE_NOT_FOUND;
          goto cleanup;
        }

      imagefile->image_state = GIMP_IMAGEFILE_STATE_EXISTS;

      /*  found the image, now look for the thumbnail  */

      imagefile->thumb_mtime = 0;

      thumbname =
        gimp_imagefile_find_png_thumb (uri,
                                       imagefile->image_mtime,
                                       GIMP_IMAGEFILE_THUMB_SIZE_NORMAL,
                                       &imagefile->thumb_mtime,
                                       &thumb_size);

      if (! thumbname)
        {
          imagefile->thumb_state = GIMP_IMAGEFILE_STATE_NOT_FOUND;
          goto cleanup;
        }

      imagefile->thumb_state = GIMP_IMAGEFILE_STATE_EXISTS;

    cleanup:
      g_free (filename);
      g_free (thumbname);
    }

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (imagefile));

  if (uri)
    {
      GimpImagefile *documents_imagefile;

      documents_imagefile = (GimpImagefile *)
        gimp_container_get_child_by_name (the_gimp->documents, uri);

      if (GIMP_IS_IMAGEFILE (documents_imagefile) &&
          (documents_imagefile != imagefile))
        {
          gimp_imagefile_update (GIMP_IMAGEFILE (documents_imagefile));
        }
    }
}

void
gimp_imagefile_create_thumbnail (GimpImagefile *imagefile)
{
  const gchar *uri;

  g_return_if_fail (GIMP_IS_IMAGEFILE (imagefile));

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

  if (uri)
    {
      GimpImage         *gimage = NULL;
      GimpPDBStatusType  dummy;
      gchar             *filename;
      gchar             *thumb_name;
      time_t             mtime;
      off_t              size;

      filename = g_filename_from_uri (uri, NULL, NULL);

      /*  no thumbnails of remote images :-(  */
      if (! filename)
        return;

      thumb_name =
        gimp_imagefile_png_thumb_path (uri, GIMP_IMAGEFILE_THUMB_SIZE_NORMAL);

      /*  the thumbnail directory doesn't exist and couldn't be created */
      if (! thumb_name)
        return;

      if (gimp_imagefile_test (filename, &mtime, &size))
        {
          gimage = file_open_image (the_gimp,
                                    uri,
                                    uri,
                                    NULL,
                                    NULL,
                                    GIMP_RUN_NONINTERACTIVE,
                                    &dummy);
        }

      g_free (filename);

      if (gimage)
        {
          gchar     *temp_name = NULL;
          GdkPixbuf *pixbuf;
          gint       width, height;

          if (gimage->width  <= GIMP_IMAGEFILE_THUMB_SIZE_NORMAL &&
              gimage->height <= GIMP_IMAGEFILE_THUMB_SIZE_NORMAL)
            {
              width  = gimage->width;
              height = gimage->height;
            }
          else
            {
              if (gimage->width < gimage->height)
                {
                  height = GIMP_IMAGEFILE_THUMB_SIZE_NORMAL;
                  width  = MAX (1, (GIMP_IMAGEFILE_THUMB_SIZE_NORMAL * gimage->width) / gimage->height);
                }
              else
                {
                  width  = GIMP_IMAGEFILE_THUMB_SIZE_NORMAL;
                  height = MAX (1, (GIMP_IMAGEFILE_THUMB_SIZE_NORMAL * gimage->height) / gimage->width);
                }
            }

          pixbuf = 
            gimp_viewable_get_new_preview_pixbuf (GIMP_VIEWABLE (gimage), 
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
            t_str = g_strdup_printf ("%ld", mtime);
            w_str = g_strdup_printf ("%d",  gimage->width);
            h_str = g_strdup_printf ("%d",  gimage->height);
            s_str = g_strdup_printf ("%ld", size);
            l_str = g_strdup_printf ("%d",  gimage->layers->num_children);

            if (! gdk_pixbuf_save (pixbuf, thumb_name, "png", &error,
                                   TAG_DESCRIPTION,        desc,
                                   TAG_SOFTWARE,           "The GIMP",
                                   TAG_THUMB_URI,          uri,
                                   TAG_THUMB_MTIME,        t_str,
                                   TAG_THUMB_SIZE,         s_str,
                                   TAG_THUMB_IMAGE_WIDTH,  w_str,
                                   TAG_THUMB_IMAGE_HEIGHT, h_str,
                                   TAG_THUMB_GIMP_TYPE,    type_str,
                                   TAG_THUMB_GIMP_LAYERS,  l_str,
                                   NULL))
              {
                g_message (_("Couldn't write thumbnail for '%s'\nas '%s'.\n%s"), 
                           uri, thumb_name, error->message);
                g_error_free (error);
              }

            g_free (desc);
            g_free (t_str);
            g_free (w_str);
            g_free (h_str);
            g_free (s_str);
            g_free (l_str);
          }

          g_object_unref (G_OBJECT (pixbuf));
          g_object_unref (G_OBJECT (gimage));

          g_free (temp_name);
          g_free (thumb_name);

          gimp_imagefile_update (imagefile);
        }
    }
}

static void
gimp_imagefile_name_changed (GimpObject *object)
{
  GimpImagefile *imagefile;

  imagefile = GIMP_IMAGEFILE (object);

  if (GIMP_OBJECT_CLASS (parent_class)->name_changed)
    GIMP_OBJECT_CLASS (parent_class)->name_changed (object);

  imagefile->image_state = GIMP_IMAGEFILE_STATE_UNKNOWN;
  imagefile->image_mtime = 0;

  imagefile->thumb_state = GIMP_IMAGEFILE_STATE_UNKNOWN;
  imagefile->thumb_mtime = 0;

  gimp_imagefile_set_info (imagefile, TRUE, -1, -1, -1, -1, -1);
}

static void
gimp_imagefile_set_info (GimpImagefile *imagefile,
                         gboolean       emit_always,
                         gint           width,
                         gint           height,
                         gint           size,
                         GimpImageType  type,
                         gint           n_layers)
{
  gboolean changed;

  changed = (imagefile->width    != width  ||
             imagefile->height   != height ||
             imagefile->size     != size   ||
             imagefile->type     != type   ||
             imagefile->n_layers != n_layers);

  imagefile->width    = width;
  imagefile->height   = height;
  imagefile->size     = size;
  imagefile->type     = type;
  imagefile->n_layers = n_layers;

  if (imagefile->description)
    {
      if (!imagefile->static_desc)
        g_free (imagefile->description);
      imagefile->description = NULL;
    }

  if (changed || emit_always)
    g_signal_emit (G_OBJECT (imagefile),
                   gimp_imagefile_signals[INFO_CHANGED], 0);
}

static void
gimp_imagefile_set_info_from_pixbuf (GimpImagefile *imagefile,
                                     GdkPixbuf     *pixbuf)
{
  const gchar  *option;
  gint          img_width  = -1;
  gint          img_height = -1;
  gint          img_size   = -1;
  GimpImageType img_type   = -1;
  gint          img_layers = -1;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_IMAGE_WIDTH);
  if (!option || sscanf (option, "%d", &img_width) != 1)
    img_width = -1;
  
  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_IMAGE_HEIGHT);
  if (!option || sscanf (option, "%d", &img_height) != 1)
    img_height = -1;
  
  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_SIZE);
  if (!option || sscanf (option, "%d", &img_size) != 1)
    img_size = -1;

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
    img_size = -1;

  gimp_imagefile_set_info (imagefile, FALSE,
                           img_width, img_height, img_size,
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

  if (!temp_buf)
    temp_buf = gimp_imagefile_read_xv_thumb (imagefile);

  return temp_buf;
}

const gchar *
gimp_imagefile_get_description (GimpImagefile *imagefile)
{
  g_return_val_if_fail (GIMP_IS_IMAGEFILE (imagefile), NULL);

  if (imagefile->description)
    return (const gchar *) imagefile->description;

  switch (imagefile->thumb_state)
    {
    case GIMP_IMAGEFILE_STATE_UNKNOWN:
    case GIMP_IMAGEFILE_STATE_REMOTE:
    case GIMP_IMAGEFILE_STATE_NOT_FOUND:
      imagefile->description = _("No preview available");
      imagefile->static_desc = TRUE;
      break;

    case GIMP_IMAGEFILE_STATE_EXISTS:
      if (imagefile->image_mtime > imagefile->thumb_mtime)
        {
          imagefile->description = _("Thumbnail is out of date");
          imagefile->static_desc = TRUE;
        }
      else
        {
          GString    *str;
          GEnumClass *enum_class;
          GEnumValue *enum_value;

          str = g_string_new (NULL);

          if (imagefile->width > -1 && imagefile->height > -1)
            {
              /* image size */
              g_string_append_printf (str, _("%d x %d"), 
                                      imagefile->width, imagefile->height);
            }

          enum_class = g_type_class_peek (GIMP_TYPE_IMAGE_TYPE);
          enum_value = g_enum_get_value (enum_class, imagefile->type);

          if (enum_value)
            {
              if (str->len)
                g_string_append_len (str, ", ", 2);

              g_string_append (str, gettext (enum_value->value_name));
            }

          if (imagefile->size > -1)
            {
              gchar *size;

              size = gimp_image_new_get_memsize_string (imagefile->size);

              if (str->len)
                g_string_append_len (str, "\n", 1);

              g_string_append (str, size);
              g_free (size);
            }

          if (imagefile->n_layers > -1)
            {
              gchar *n_layers;

              if (imagefile->n_layers == 1)
                n_layers = g_strdup_printf (_("%d Layer"), imagefile->n_layers);
              else
                n_layers = g_strdup_printf (_("%d Layers"), imagefile->n_layers);

              if (str->len)
                {
                  if (imagefile->size > -1)
                    g_string_append_len (str, ", ", 2);
                  else
                    g_string_append_len (str, "\n", 1);
               }

              g_string_append (str, n_layers);
              g_free (n_layers);
            }

          imagefile->static_desc = FALSE;
          imagefile->description = g_string_free (str, FALSE);
        }
      break;

    default:
      break;
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


/* PNG thumbnail reading routines according to the 
   Thumbnail Managing Standard  http://triq.net/~pearl/thumbnail-spec/  */

static TempBuf *
gimp_imagefile_read_png_thumb (GimpImagefile *imagefile,
                               gint           size)
{
  TempBuf     *temp_buf   = NULL;
  GdkPixbuf   *pixbuf     = NULL;
  gchar       *thumbname  = NULL;
  gint         thumb_size = GIMP_IMAGEFILE_THUMB_SIZE_FAIL;
  const gchar *option;
  gint         width;
  gint         height;
  gint         bytes;
  time_t       y;
  guchar      *src;
  guchar      *dest;
  GError      *error = NULL;

  if (imagefile->image_state != GIMP_IMAGEFILE_STATE_EXISTS)
    return NULL;

  /* try to locate a thumbnail for this image */
  imagefile->thumb_state = GIMP_IMAGEFILE_STATE_NOT_FOUND;
  imagefile->thumb_mtime = 0;

  thumbname = gimp_imagefile_find_png_thumb (GIMP_OBJECT (imagefile)->name,
                                             imagefile->image_mtime,
                                             size,
                                             &imagefile->thumb_mtime,
                                             &thumb_size);
  if (!thumbname)
    goto cleanup;

  imagefile->thumb_state = GIMP_IMAGEFILE_STATE_EXISTS;

  pixbuf = gdk_pixbuf_new_from_file (thumbname, &error);  

  if (!pixbuf)
    {
      g_message (_("Could not open thumbnail\nfile '%s':\n%s"),
                 thumbname, error->message);
      goto cleanup;
    }
  
  g_free (thumbname);
  thumbname = NULL;

  /* URI and mtime from the thumbnail need to match our file */
 
  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_URI);
  if (!option || strcmp (option, GIMP_OBJECT (imagefile)->name))
    goto cleanup;

  option = gdk_pixbuf_get_option (pixbuf, TAG_THUMB_MTIME);
  if (!option || sscanf (option, "%ld", &y) != 1 || y != imagefile->image_mtime)
    goto cleanup;

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
  gimp_imagefile_set_info_from_pixbuf (imagefile, pixbuf);

 cleanup:
  g_free (thumbname);
  if (pixbuf)
    g_object_unref (pixbuf);  
  if (error)
    g_error_free (error);

  return temp_buf;
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
                               gint         size)
{
  const gchar *name;
  gchar       *thumb_name = NULL;
  gint         i, n;

  name = gimp_imagefile_png_thumb_name (uri);

  n = G_N_ELEMENTS (thumb_sizes);
  for (i = 1; i < n && thumb_sizes[i].size < size; i++)
    /* nothing */;

  if (i == n)
    i--;

  if (! g_file_test (thumb_subdirs[i], G_FILE_TEST_IS_DIR))
    {
      if (g_file_test (thumb_dir, G_FILE_TEST_IS_DIR) || 
          (mkdir (thumb_dir, 0700) == 0))
        {
          mkdir (thumb_subdirs[i], 0700);
        }

      if (! g_file_test (thumb_subdirs[i], G_FILE_TEST_IS_DIR))
        {
          g_message (_("Couldn't create thumbnail directory '%s'"), 
                     thumb_subdirs[i]);
          return NULL;
        }
    }

  thumb_name = g_build_filename (thumb_subdirs[i], name, NULL);

  return thumb_name;
}

static gchar *
gimp_imagefile_find_png_thumb (const gchar *uri,
                               time_t       image_mtime,
                               gint         size,
                               time_t      *thumb_mtime,
                               gint        *thumb_size)
{
  const gchar *name;
  gchar       *thumb_name;
  gint         i, n;

  name = gimp_imagefile_png_thumb_name (uri);

  n = G_N_ELEMENTS (thumb_sizes);
  for (i = 1; i < n && thumb_sizes[i].size < size; i++)
    /* nothing */;

  n = i;
  for (; i < G_N_ELEMENTS (thumb_sizes); i++)
    {
      thumb_name = g_build_filename (thumb_subdirs[i], name, NULL);

      if (gimp_imagefile_test (thumb_name, thumb_mtime, NULL) &&
          image_mtime <= *thumb_mtime) 
        {
          *thumb_size = thumb_sizes[i].size;
          return thumb_name;
        }

      g_free (thumb_name);
    }

  for (i = n - 1; i >= 0; i--)
    {
      thumb_name = g_build_filename (thumb_subdirs[i], name, NULL);

      if (gimp_imagefile_test (thumb_name, thumb_mtime, NULL) &&
          image_mtime <= *thumb_mtime)
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
                               img_width, img_height, -1, -1, -1);

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

