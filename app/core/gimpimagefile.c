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

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpimagefile.h"

#include "libgimp/gimpintl.h"


typedef struct
{
  const gchar *dirname;
  gint         size;
} ThumbnailSize;

static const ThumbnailSize thumb_sizes[] = 
{
  { "48x48",   48  },
  { "64x64",   64  },
  { "96x96",   96  },
  { "128x128", 128 },
  { "144x144", 144 },
  { "160x160", 160 },
  { "192x192", 192 },
};


static void      gimp_imagefile_class_init (GimpImagefileClass *klass);
static void      gimp_imagefile_init       (GimpImagefile      *imagefile);
static TempBuf * gimp_imagefile_get_new_preview (GimpViewable  *viewable,
                                                 gint            width,
                                                 gint            height);

static TempBuf * gimp_imagefile_read_png_thumb  (GimpImagefile  *imagefile,
                                                 gint            size);
static gchar   * gimp_imagefile_png_thumb_name  (const gchar    *dirname,
                                                 const gchar    *basename,
                                                 gint            size);

static TempBuf * gimp_imagefile_read_xv_thumb   (GimpImagefile  *imagefile);
static guchar  * readXVThumb                    (const gchar    *filename,
                                                 gint           *width,
                                                 gint           *height,
                                                 gchar         **imginfo);


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
	NULL,		/* class_finalize */
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
  GimpViewableClass *viewable_class;

  parent_class = g_type_class_peek_parent (klass);

  viewable_class = GIMP_VIEWABLE_CLASS (klass);

  viewable_class->get_new_preview = gimp_imagefile_get_new_preview;
}

static void
gimp_imagefile_init (GimpImagefile *imagefile)
{
  imagefile->width  = 0;
  imagefile->height = 0;
  imagefile->size   = -1;
}

GimpImagefile *
gimp_imagefile_new (const gchar *filename)
{
  GimpImagefile *imagefile;

  g_return_val_if_fail (filename != NULL, NULL);

  imagefile = GIMP_IMAGEFILE (g_object_new (GIMP_TYPE_IMAGEFILE, NULL));

  gimp_object_set_name (GIMP_OBJECT (imagefile), filename);

  return imagefile;
}

static void
gimp_imagefile_set_info (GimpImagefile *imagefile,
                         gint           width,
                         gint           height,
                         gint           size)
{
  gboolean changed;

  changed = (imagefile->width != width || imagefile->height != height ||
             imagefile->size != size);

  imagefile->width  = width;
  imagefile->height = height;
  imagefile->size   = size;

  /* emit a name changed signal so the container view updates */
  if (changed)
    gimp_object_name_changed (GIMP_OBJECT (imagefile));
}

static void
gimp_imagefile_set_info_from_pixbuf (GimpImagefile *imagefile,
                                     GdkPixbuf     *pixbuf)
{
  const gchar *option;
  gint         img_width;
  gint         img_height;
  gint         img_size;

  option = gdk_pixbuf_get_option (pixbuf, "tEXt::OriginalWidth");
  if (!option || sscanf (option, "%d", &img_width) != 1)
    img_width = 0;
  
  option = gdk_pixbuf_get_option (pixbuf, "tEXt::OriginalHeight");
  if (!option || sscanf (option, "%d", &img_height) != 1)
    img_height = 0;
  
  option = gdk_pixbuf_get_option (pixbuf, "tEXt::OriginalSize");
  if (!option || sscanf (option, "%d", &img_size) != 1)
    img_size = -1;

  gimp_imagefile_set_info (imagefile, img_width, img_height, img_size);
}

static TempBuf *
gimp_imagefile_get_new_preview (GimpViewable *viewable,
                                gint          width,
                                gint          height)
{
  GimpImagefile *imagefile;
  TempBuf       *temp_buf;

  imagefile = GIMP_IMAGEFILE (viewable);

  g_return_val_if_fail (GIMP_OBJECT (imagefile)->name != NULL, NULL);

  temp_buf = gimp_imagefile_read_png_thumb (imagefile, MAX (width, height));

  if (!temp_buf)
    temp_buf = gimp_imagefile_read_xv_thumb (imagefile);

  return temp_buf;
}


/* PNG thumbnail reading routines according to the 
   Thumbnail Managing Standard  http://triq.net/~pearl/thumbnail-spec/  */

static TempBuf *
gimp_imagefile_read_png_thumb (GimpImagefile *imagefile,
                               gint           size)
{
  TempBuf   *temp_buf;
  GdkPixbuf *pixbuf;
  gchar     *basename;
  gchar     *dirname;
  gchar     *fullname;
  gchar     *thumbname;
  gint       width;
  gint       height;
  gint       bytes;
  gint       y;
  guchar    *src;
  guchar    *dest;
  GError    *error;
  
  fullname = g_strconcat (GIMP_OBJECT (imagefile)->name, ".png", NULL);

  dirname  = g_path_get_dirname (GIMP_OBJECT (imagefile)->name);
  basename = g_path_get_basename (fullname);

  thumbname = gimp_imagefile_png_thumb_name (dirname, 
                                             basename, 
                                             size);
  g_free (dirname);
  g_free (basename);

  if (!thumbname)
    thumbname = gimp_imagefile_png_thumb_name (g_get_home_dir(), 
                                               fullname, 
                                               size);
  
  g_free (fullname);

  if (!thumbname)
    return NULL;

  pixbuf = gdk_pixbuf_new_from_file (thumbname, &error);  

  if (!pixbuf)
    {
      g_message (_("Could not open thumbnail\nfile '%s':\n%s"),
                 thumbname, error->message);
      g_free (thumbname);
      g_error_free (error);
      return NULL;
    }
  
  g_free (thumbname);

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

  gimp_imagefile_set_info_from_pixbuf (imagefile, pixbuf);

  g_object_unref (pixbuf);

  return temp_buf;
}

static gchar *
gimp_imagefile_png_thumb_name (const gchar *dirname,
                               const gchar *basename,
                               gint         size)
{
  gchar *thumbname;
  gint   i, n;

  n = G_N_ELEMENTS (thumb_sizes);

  for (i = 0; i < n && thumb_sizes[i].size < size; i++)
    /* nothing */;
  
  n = i;

  for (i = n; i < G_N_ELEMENTS (thumb_sizes); i++)
    {
      thumbname = g_build_filename (dirname, 
                                    ".thumbnails", 
                                    thumb_sizes[i].dirname, 
                                    basename, NULL);
      
      if (g_file_test (thumbname, G_FILE_TEST_EXISTS))
        return thumbname;
      
      g_free (thumbname);
    }
  
  for (i = n - 1; i >= 0; i--)
    {
      thumbname = g_build_filename (dirname, 
                                    ".thumbnails", 
                                    thumb_sizes[i].dirname, 
                                    basename, NULL);
      
      if (g_file_test (thumbname, G_FILE_TEST_EXISTS))
        return thumbname;
      
      g_free (thumbname);
    }

  return NULL;
}

/* xvpics thumbnail reading routines for backward compatibility */

static TempBuf *
gimp_imagefile_read_xv_thumb (GimpImagefile *imagefile)
{
  gchar         *basename;
  gchar         *dirname;
  gchar         *thumbname;
  struct stat    file_stat;
  struct stat    thumb_stat;
  gint           width;
  gint           height;
  gint           x, y;
  gboolean       thumb_may_be_outdated = FALSE;
  TempBuf       *temp_buf;
  guchar        *raw_thumb;
  guchar        *src;
  guchar        *dest;
  gchar         *image_info = NULL;

  dirname  = g_path_get_dirname (GIMP_OBJECT (imagefile)->name);
  basename = g_path_get_basename (GIMP_OBJECT (imagefile)->name);

  thumbname = g_build_filename (dirname, ".xvpics", basename, NULL);

  g_free (dirname);
  g_free (basename);

  /*  If the file is newer than its thumbnail, the thumbnail may
   *  be out of date.
   */
  if ((stat (thumbname, &thumb_stat) == 0) &&
      (stat (GIMP_OBJECT (imagefile)->name, &file_stat ) == 0))
    {
      if ((thumb_stat.st_mtime) < (file_stat.st_mtime))
	{
	  thumb_may_be_outdated = TRUE;
	}
    }

  raw_thumb = readXVThumb (thumbname, &width, &height, &image_info);
 
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
      gimp_imagefile_set_info (imagefile, img_width, img_height, -1);

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
