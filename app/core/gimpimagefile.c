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


static void      gimp_imagefile_class_init      (GimpImagefileClass *klass);
static void      gimp_imagefile_init            (GimpImagefile      *imagefile);
static void      gimp_imagefile_finalize        (GObject            *object);
static TempBuf * gimp_imagefile_get_new_preview (GimpViewable       *viewable,
                                                 gint                width,
                                                 gint                height);

static guchar  * readXVThumb  (const gchar   *filename,
                               gint          *width,
                               gint          *height,
                               gchar        **imginfo);


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
  GObjectClass      *object_class;
  GimpViewableClass *viewable_class;

  object_class = G_OBJECT_CLASS (klass);
  viewable_class = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  viewable_class->get_new_preview = gimp_imagefile_get_new_preview;

  object_class->finalize = gimp_imagefile_finalize;
}

static void
gimp_imagefile_init (GimpImagefile *imagefile)
{
  imagefile->width  = -1;
  imagefile->height = -1;
  imagefile->size   = -1;
}

static void
gimp_imagefile_finalize (GObject *object)
{
  GimpImagefile *imagefile;

  imagefile = GIMP_IMAGEFILE (object);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
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

static TempBuf *
gimp_imagefile_get_new_preview (GimpViewable *viewable,
                                gint          width,
                                gint          height)
{
  GimpImagefile *imagefile;
  const gchar   *basename;
  gchar         *dirname;
  gchar         *thumbname;
  struct stat    file_stat;
  struct stat    thumb_stat;
  gint           thumb_width;
  gint           thumb_height;
  gint           x, y;
  gboolean       thumb_may_be_outdated = FALSE;
  TempBuf       *temp_buf;
  guchar        *raw_thumb;
  guchar        *src;
  guchar        *dest;
  guchar         white[3] = { 0xff, 0xff, 0xff };
  gchar         *image_info = NULL;

  imagefile = GIMP_IMAGEFILE (viewable);

  g_return_val_if_fail (GIMP_OBJECT (imagefile)->name != NULL, NULL);
 
  dirname  = g_dirname (GIMP_OBJECT (imagefile)->name);
  basename = g_basename (GIMP_OBJECT (imagefile)->name);

  thumbname = g_strconcat (dirname, G_DIR_SEPARATOR_S,
                           ".xvpics", G_DIR_SEPARATOR_S,
                           basename, NULL);

  g_free (dirname);

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

  raw_thumb = readXVThumb (thumbname, 
                           &thumb_width, &thumb_height, &image_info);
 
  g_free (thumbname);

  /* FIXME: use image info */
  g_free (image_info);
  
  if (!raw_thumb)
    return NULL;

  /* FIXME: scale if necessary */
  temp_buf = temp_buf_new (width, height, 3, 0, 0, white);
  src  = raw_thumb;
  dest = temp_buf_data (temp_buf);

  for (y = 0; y < MIN (height, thumb_height); y++)
    {
      for (x = 0; x < MIN (width, thumb_width); x++)
        {
          dest[x*3]   =  ((src[x]>>5)*255)/7;
          dest[x*3+1] = (((src[x]>>2)&7)*255)/7;
          dest[x*3+2] =  ((src[x]&3)*255)/3;
        }
      src  += thumb_width;
      dest += width * 3;
    }
  g_free (raw_thumb);

  return temp_buf;
}

/* The readXVThumb function source may be re-used under
   the XFree86-style license. <adam@gimp.org> */
guchar *
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
      g_warning ("Thumbnail doesn't have the 'P7 332' header.");
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
      g_warning ("Thumbnail is of funky depth.");
      fclose (fp);
      return NULL;
    }

  if ((*w)<1 || (*h)<1 || (*w)>80 || (*h)>60)
    {
      g_warning ("Thumbnail size bad.  Corrupted?");
      fclose (fp);
      return NULL;
    }

  buf = g_malloc ((*w) * (*h));

  fread (buf, (*w) * (*h), 1, fp);
  fclose (fp);
  
  return buf;
}
