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
#include "patterns.h"
#include "pattern_header.h"
#include "gimpbrush.h"
#include "gimpbrushlist.h"

#include "gimpsignal.h"
#include "gimprc.h"

#include "paint_core.h"
#include "temp_buf.h"

#include "libgimp/gimpintl.h"

enum
{
  DIRTY,
  RENAME,
  LAST_SIGNAL
};

static GimpBrush * gimp_brush_select_brush     (PaintCore *paint_core);
static gboolean    gimp_brush_want_null_motion (PaintCore *paint_core);

static guint gimp_brush_signals[LAST_SIGNAL];

static GimpObjectClass *parent_class;

static void
gimp_brush_destroy (GtkObject *object)
{
  GimpBrush *brush = GIMP_BRUSH (object);

  g_free (brush->filename);
  g_free (brush->name);

  if (brush->mask)
    temp_buf_free (brush->mask);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_brush_class_init (GimpBrushClass *klass)
{
  GtkObjectClass *object_class;
  GtkType         type;
  
  object_class = GTK_OBJECT_CLASS (klass);

  parent_class = gtk_type_class (gimp_object_get_type ());
  
  type = object_class->type;

  object_class->destroy = gimp_brush_destroy;

  klass->select_brush = gimp_brush_select_brush;
  klass->want_null_motion = gimp_brush_want_null_motion;

  gimp_brush_signals[DIRTY] =
    gimp_signal_new ("dirty",  GTK_RUN_FIRST, type, 0, gimp_sigtype_void);

  gimp_brush_signals[RENAME] =
    gimp_signal_new ("rename", GTK_RUN_FIRST, type, 0, gimp_sigtype_void);

  gtk_object_class_add_signals (object_class, gimp_brush_signals, LAST_SIGNAL);
}

void
gimp_brush_init (GimpBrush *brush)
{
  brush->filename  = NULL;
  brush->name      = NULL;

  brush->spacing   = 20;
  brush->x_axis.x  = 15.0;
  brush->x_axis.y  =  0.0;
  brush->y_axis.x  =  0.0;
  brush->y_axis.y  = 15.0;

  brush->mask      = NULL;
  brush->pixmap    = NULL;
}


GtkType
gimp_brush_get_type (void)
{
  static GtkType type = 0;

  if (!type)
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

    type = gtk_type_unique (gimp_object_get_type (), &info);
  }
  return type;
}

GimpBrush *
gimp_brush_load (gchar *filename)
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
gimp_brush_get_mask (GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  return brush->mask;
}

TempBuf *
gimp_brush_get_pixmap (GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  return brush->pixmap;
}

gchar *
gimp_brush_get_name (GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  return brush->name;
}

void
gimp_brush_set_name (GimpBrush *brush, 
		     gchar     *name)
{
  g_return_if_fail (brush != NULL);
  g_return_if_fail (GIMP_IS_BRUSH (brush));

  if (strcmp (brush->name, name) == 0)
    return;

  if (brush->name)
    g_free (brush->name);
  brush->name = g_strdup (name);

  gtk_signal_emit (GTK_OBJECT (brush), gimp_brush_signals[RENAME]);
}

gint
gimp_brush_get_spacing (GimpBrush *brush)
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
gimp_brush_load_brush (gint   fd,
		       gchar *filename)
{
  GimpBrush   *brush;
  GPattern    *pattern;
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
      brush = GIMP_BRUSH (gtk_type_new (gimp_brush_get_type ()));
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
      
      /*  For backwards-compatibility, check if a pattern follows.
	  The obsolete .gpb format did it this way.  */
      pattern = pattern_load (fd, filename);
      
      if (pattern)
	{
	  if (pattern->mask && pattern->mask->bytes == 3)
	    {
	      brush->pixmap = pattern->mask;
	      pattern->mask = NULL;
	    }
	  pattern_free (pattern);
	}
      else
	{
	  /*  rewind to make brush pipe loader happy  */
	  if (lseek (fd, - ((off_t) sizeof (PatternHeader)), SEEK_CUR) < 0)
	    {
	      g_message (_("GIMP brush file appears to be corrupted: \"%s\"."),
			 filename);
	      g_free (name);
	      gtk_object_unref (GTK_OBJECT (brush));
	      return NULL;
	    }
	}
      break;

    case 4:
      brush = GIMP_BRUSH (gtk_type_new (gimp_brush_get_type ()));
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
      g_message ("Unsupported brush depth: %d\n in file \"%s\"\nGIMP Brushes must be GRAY or RGBA\n",
		 header.bytes, filename);
      g_free (name);
      return NULL;
    }

  brush->name     = name;
  brush->spacing  = header.spacing;
  brush->x_axis.x = header.width  / 2.0;
  brush->x_axis.y = 0.0;
  brush->y_axis.x = 0.0;
  brush->y_axis.y = header.height / 2.0;

  return brush;
}
