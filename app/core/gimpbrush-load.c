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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <gtk/gtk.h>


#include <stdio.h>

#include "apptypes.h"

#include "brush_header.h"
#include "gimpbrush.h"
#include "gimprc.h"
#include "temp_buf.h"

/*  this needs to go away  */
#include "tools/paint_core.h"
#include "brush_scale.h"
#include "gimpbrushpipe.h"

#include "libgimp/gimpintl.h"


enum
{
  DIRTY,
  LAST_SIGNAL
};


static void        gimp_brush_class_init       (GimpBrushClass *klass);
static void        gimp_brush_init             (GimpBrush      *brush);
static void        gimp_brush_destroy          (GtkObject      *object);
static TempBuf   * gimp_brush_get_new_preview  (GimpViewable   *viewable,
						gint            width,
						gint            height);

static GimpBrush * gimp_brush_select_brush     (PaintCore      *paint_core);
static gboolean    gimp_brush_want_null_motion (PaintCore      *paint_core);


static guint gimp_brush_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GtkType
gimp_brush_get_type (void)
{
  static GtkType type = 0;

  if (! type)
    {
      static const GtkTypeInfo info =
      {
        "GimpBrush",
        sizeof (GimpBrush),
        sizeof (GimpBrushClass),
        (GtkClassInitFunc) gimp_brush_class_init,
        (GtkObjectInitFunc) gimp_brush_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

    type = gtk_type_unique (GIMP_TYPE_VIEWABLE, &info);
  }
  return type;
}

static void
gimp_brush_class_init (GimpBrushClass *klass)
{
  GtkObjectClass    *object_class;
  GimpViewableClass *viewable_class;

  object_class   = (GtkObjectClass *) klass;
  viewable_class = (GimpViewableClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_VIEWABLE);
  
  gimp_brush_signals[DIRTY] =
    gtk_signal_new ("dirty",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpBrushClass,
				       dirty),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, gimp_brush_signals, LAST_SIGNAL);

  object_class->destroy = gimp_brush_destroy;

  viewable_class->get_new_preview = gimp_brush_get_new_preview;

  klass->dirty            = NULL;

  klass->select_brush     = gimp_brush_select_brush;
  klass->want_null_motion = gimp_brush_want_null_motion;
}

static void
gimp_brush_init (GimpBrush *brush)
{
  brush->filename  = NULL;

  brush->spacing   = 20;
  brush->x_axis.x  = 15.0;
  brush->x_axis.y  =  0.0;
  brush->y_axis.x  =  0.0;
  brush->y_axis.y  = 15.0;

  brush->mask      = NULL;
  brush->pixmap    = NULL;
}

static void
gimp_brush_destroy (GtkObject *object)
{
  GimpBrush *brush;

  brush = GIMP_BRUSH (object);

  g_free (brush->filename);

  if (brush->mask)
    temp_buf_free (brush->mask);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static TempBuf *
gimp_brush_get_new_preview (GimpViewable *viewable,
			    gint          width,
			    gint          height)
{
  GimpBrush   *brush;
  gboolean     is_popup   = FALSE;

  gboolean     scale      = FALSE;
  gint         brush_width;
  gint         brush_height;
  gint         offset_x;
  gint         offset_y;
  TempBuf     *mask_buf   = NULL;
  TempBuf     *pixmap_buf = NULL;
  TempBuf     *return_buf = NULL;
  guchar       white[3] = { 255, 255, 255 };
  guchar      *mask;
  guchar      *buf;
  guchar      *b;
  guchar       bg;
  gint         x, y;

  brush = GIMP_BRUSH (viewable);

  mask_buf     = gimp_brush_get_mask (brush);
  pixmap_buf   = gimp_brush_get_pixmap (brush);

  brush_width  = mask_buf->width;
  brush_height = mask_buf->height;

  if (brush_width > width || brush_height > height)
    {
      gdouble ratio_x = (gdouble) brush_width  / width;
      gdouble ratio_y = (gdouble) brush_height / height;

      brush_width  = (gdouble) brush_width  / MAX (ratio_x, ratio_y) + 0.5; 
      brush_height = (gdouble) brush_height / MAX (ratio_x, ratio_y) + 0.5;

      mask_buf = brush_scale_mask (mask_buf, brush_width, brush_height);

      if (pixmap_buf)
        {
          /* TODO: the scale function should scale the pixmap and the
	   *  mask in one run
	   */
          pixmap_buf =
	    brush_scale_pixmap (pixmap_buf, brush_width, brush_height);
        }

      scale = TRUE;
    }

  offset_x = (width  - brush_width)  / 2;
  offset_y = (height - brush_height) / 2;

  return_buf = temp_buf_new (width, height, 3, 0, 0, white);

  mask = temp_buf_data (mask_buf);
  buf  = temp_buf_data (return_buf);

  b = buf + (offset_y * return_buf->width + offset_x) * return_buf->bytes;

  if (pixmap_buf)
    {
      guchar *pixmap = temp_buf_data (pixmap_buf);

      for (y = 0; y < brush_height; y++)
        {
          for (x = 0; x < brush_width ; x++)
            {
              bg = (255 - *mask);

              *b++ = bg + (*mask * *pixmap++) / 255;
              *b++ = bg + (*mask * *pixmap++) / 255; 
              *b++ = bg + (*mask * *pixmap++) / 255;

              mask++;
            }

	  b += (return_buf->width - brush_width) * return_buf->bytes;
        }
    }
  else
    {
      for (y = 0; y < brush_height; y++)
        {
          for (x = 0; x < brush_width ; x++)
            {
              bg = 255 - *mask++;

              *b++ = bg;
              *b++ = bg;
              *b++ = bg;
            }

	  b += (return_buf->width - brush_width) * return_buf->bytes;
        }
    }

#define indicator_width  7
#define indicator_height 7

#define WHT { 255, 255, 255 }
#define BLK {   0,   0,   0 }
#define RED { 255, 127, 127 }

  if (scale)
    {
      static const guchar scale_indicator_bits[7][7][3] = 
      {
	{ WHT, WHT, WHT, WHT, WHT, WHT, WHT },
	{ WHT, WHT, WHT, BLK, WHT, WHT, WHT },
	{ WHT, WHT, WHT, BLK, WHT, WHT, WHT },
	{ WHT, BLK, BLK, BLK, BLK, BLK, WHT },
	{ WHT, WHT, WHT, BLK, WHT, WHT, WHT },
	{ WHT, WHT, WHT, BLK, WHT, WHT, WHT },
	{ WHT, WHT, WHT, WHT, WHT, WHT, WHT }
      };

      static const guchar scale_pipe_indicator_bits[7][7][3] = 
      {
	{ WHT, WHT, WHT, WHT, WHT, WHT, WHT },
	{ WHT, WHT, WHT, BLK, WHT, WHT, RED },
	{ WHT, WHT, WHT, BLK, WHT, RED, RED },
	{ WHT, BLK, BLK, BLK, BLK, BLK, RED },
	{ WHT, WHT, WHT, BLK, RED, RED, RED },
	{ WHT, WHT, RED, BLK, RED, RED, RED },
	{ WHT, RED, RED, RED, RED, RED, RED }
      };

      offset_x = width  - indicator_width;
      offset_y = height - indicator_height;

      b = buf + (offset_y * return_buf->width + offset_x) * return_buf->bytes;

      for (y = 0; y < indicator_height; y++)
	{
	  for (x = 0; x < indicator_height; x++)
	    {
	      if (GIMP_IS_BRUSH_PIPE (brush))
		{
		  *b++ = scale_pipe_indicator_bits[y][x][0];
		  *b++ = scale_pipe_indicator_bits[y][x][1];
		  *b++ = scale_pipe_indicator_bits[y][x][2];
		}
	      else
		{
		  *b++ = scale_indicator_bits[y][x][0];
		  *b++ = scale_indicator_bits[y][x][1];
		  *b++ = scale_indicator_bits[y][x][2];
		}
	    }

	  b += (return_buf->width - indicator_width) * return_buf->bytes;
	}

      temp_buf_free (mask_buf);

      if (pixmap_buf)
        temp_buf_free (pixmap_buf);
    }
  else if (!is_popup && GIMP_IS_BRUSH_PIPE (brush))
    {
      static const guchar pipe_indicator_bits[7][7][3] = 
      {
	{ WHT, WHT, WHT, WHT, WHT, WHT, WHT },
	{ WHT, WHT, WHT, WHT, WHT, WHT, RED },
	{ WHT, WHT, WHT, WHT, WHT, RED, RED },
	{ WHT, WHT, WHT, WHT, RED, RED, RED },
	{ WHT, WHT, WHT, RED, RED, RED, RED },
	{ WHT, WHT, RED, RED, RED, RED, RED },
	{ WHT, RED, RED, RED, RED, RED, RED }
      };

      offset_x = width  - indicator_width;
      offset_y = height - indicator_height;

      b = buf + (offset_y * return_buf->width + offset_x) * return_buf->bytes;

      for (y = 0; y < indicator_height; y++)
	{
	  for (x = 0; x < indicator_height; x++)
	    {
	      *b++ = pipe_indicator_bits[y][x][0];
	      *b++ = pipe_indicator_bits[y][x][1];
	      *b++ = pipe_indicator_bits[y][x][2];
	    }

	  b += (return_buf->width - indicator_width) * return_buf->bytes;
	}
    }

#undef indicator_width
#undef indicator_height

#undef WHT
#undef BLK
#undef RED

  return return_buf;
}

GimpBrush *
gimp_brush_load (const gchar *filename)
{
  GimpBrush *brush;
  gint       fd;

  g_return_val_if_fail (filename != NULL, NULL);

  fd = open (filename, O_RDONLY | _O_BINARY);
  if (fd == -1) 
    return NULL;

  brush = gimp_brush_load_brush (fd, filename);

  close (fd);

  brush->filename = g_strdup (filename);

  /*  Swap the brush to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    {
      temp_buf_swap (brush->mask);
      if (brush->pixmap)
	temp_buf_swap (brush->pixmap);
    }

  return brush;
}

static GimpBrush *
gimp_brush_select_brush (PaintCore *paint_core)
{
  return paint_core->brush;
}

static gboolean
gimp_brush_want_null_motion (PaintCore *paint_core)
{
  return TRUE;
}

TempBuf *
gimp_brush_get_mask (const GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  return brush->mask;
}

TempBuf *
gimp_brush_get_pixmap (const GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  return brush->pixmap;
}

gint
gimp_brush_get_spacing (const GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, 0);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), 0);

  return brush->spacing;
}

void
gimp_brush_set_spacing (GimpBrush *brush,
			gint       spacing)
{
  g_return_if_fail (brush != NULL);
  g_return_if_fail (GIMP_IS_BRUSH (brush));

  brush->spacing = spacing;
}

GimpBrush *
gimp_brush_load_brush (gint         fd,
		       const gchar *filename)
{
  GimpBrush   *brush;
  gint         bn_size;
  BrushHeader  header;
  gchar       *name;
  gint         i;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (fd != -1, NULL);

  /*  Read in the header size  */
  if (read (fd, &header, sizeof (header)) != sizeof (header))
    return NULL;

  /*  rearrange the bytes in each unsigned int  */
  header.header_size  = g_ntohl (header.header_size);
  header.version      = g_ntohl (header.version);
  header.width        = g_ntohl (header.width);
  header.height       = g_ntohl (header.height);
  header.bytes        = g_ntohl (header.bytes);
  header.magic_number = g_ntohl (header.magic_number);
  header.spacing      = g_ntohl (header.spacing);

  /*  Check for correct file format */
  /*  It looks as if version 1 did not have the same magic number.  (neo)  */
  if (header.version != 1 &&
      (header.magic_number != GBRUSH_MAGIC || header.version != 2))
    {
      g_message (_("Unknown brush format version #%d in \"%s\"."),
		 header.version, filename);
      return NULL;
    }

  if (header.version == 1)
    {
      /*  If this is a version 1 brush, set the fp back 8 bytes  */
      lseek (fd, -8, SEEK_CUR);
      header.header_size += 8;
      /*  spacing is not defined in version 1  */
      header.spacing = 25;
    }
  
   /*  Read in the brush name  */
  if ((bn_size = (header.header_size - sizeof (header))))
    {
      name = g_new (gchar, bn_size);
      if ((read (fd, name, bn_size)) < bn_size)
	{
	  g_message (_("Error in GIMP brush file \"%s\"."), filename);
	  g_free (name);
	  return NULL;
	}
    }
  else
    {
      name = g_strdup (_("Unnamed"));
    }

  switch (header.bytes)
    {
    case 1:
      brush = GIMP_BRUSH (gtk_type_new (GIMP_TYPE_BRUSH));
      brush->mask = temp_buf_new (header.width, header.height, 1,
				  0, 0, NULL);
      if (read (fd,
		temp_buf_data (brush->mask), header.width * header.height) <
	  header.width * header.height)
	{
	  g_message (_("GIMP brush file appears to be truncated: \"%s\"."),
		     filename);
	  g_free (name);
	  gtk_object_unref (GTK_OBJECT (brush));
	  return NULL;
	}
      break;

    case 4:
      brush = GIMP_BRUSH (gtk_type_new (GIMP_TYPE_BRUSH));
      brush->mask =   temp_buf_new (header.width, header.height, 1, 0, 0, NULL);
      brush->pixmap = temp_buf_new (header.width, header.height, 3, 0, 0, NULL);

      for (i = 0; i < header.width * header.height; i++)
	{
	  if (read (fd, temp_buf_data (brush->pixmap)
		    + i * 3, 3) != 3 ||
	      read (fd, temp_buf_data (brush->mask) + i, 1) != 1)
	    {
	      g_message (_("GIMP brush file appears to be truncated: \"%s\"."),
			 filename);
	      g_free (name);
	      gtk_object_unref (GTK_OBJECT (brush));
	      return NULL;
	    }
	}
      break;
      
    default:
      g_message ("Unsupported brush depth: %d\n"
		 "in file \"%s\"\n"
		 "GIMP Brushes must be GRAY or RGBA",
		 header.bytes, filename);
      g_free (name);
      return NULL;
    }

  gimp_object_set_name (GIMP_OBJECT (brush), name);

  g_free (name);

  brush->spacing  = header.spacing;
  brush->x_axis.x = header.width  / 2.0;
  brush->x_axis.y = 0.0;
  brush->y_axis.x = 0.0;
  brush->y_axis.y = header.height / 2.0;

  return brush;
}
