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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gimpbrush.h"
#include "gimpbrushlist.h"
#include "gimpsignal.h"
#include "gimprc.h"
#include "brush_header.h"

enum{
  DIRTY,
  RENAME,
  LAST_SIGNAL
};

static guint gimp_brush_signals[LAST_SIGNAL];
static GimpObjectClass* parent_class;

static void
gimp_brush_destroy(GtkObject *object)
{
  GimpBrush* brush=GIMP_BRUSH(object);
  if (brush->filename)
    g_free(brush->filename);
  if (brush->name)
    g_free(brush->name);
  if (brush->mask)
     temp_buf_free(brush->mask);
  GTK_OBJECT_CLASS(parent_class)->destroy (object);

}

static void
gimp_brush_class_init (GimpBrushClass *klass)
{
  GtkObjectClass *object_class;
  GtkType type;
  
  object_class = GTK_OBJECT_CLASS(klass);

  parent_class = gtk_type_class (gimp_object_get_type ());
  
  type=object_class->type;

  object_class->destroy =  gimp_brush_destroy;

  gimp_brush_signals[DIRTY] =
    gimp_signal_new ("dirty", 0, type, 0, gimp_sigtype_void);

  gimp_brush_signals[RENAME] =
    gimp_signal_new ("rename", 0, type, 0, gimp_sigtype_void);

  gtk_object_class_add_signals (object_class, gimp_brush_signals, LAST_SIGNAL);
}

void
gimp_brush_init(GimpBrush *brush)
{
  brush->filename  = NULL;
  brush->name      = NULL;
  brush->spacing   = 20;
  brush->mask      = NULL;
  brush->x_axis.x    = 15.0;
  brush->x_axis.y    =  0.0;
  brush->y_axis.x    =  0.0;
  brush->y_axis.y    = 15.0;
}

GtkType gimp_brush_get_type(void)
{
  static GtkType type;
  if(!type){
    GtkTypeInfo info={
      "GimpBrush",
      sizeof(GimpBrush),
      sizeof(GimpBrushClass),
      (GtkClassInitFunc)gimp_brush_class_init,
      (GtkObjectInitFunc)gimp_brush_init,
      NULL,
      NULL };
    type=gtk_type_unique(gimp_object_get_type(), &info);
  }
  return type;
}

GimpBrush *
gimp_brush_new (char *filename)
{
  GimpBrush *brush=GIMP_BRUSH(gtk_type_new(gimp_brush_get_type ()));
  gimp_brush_load(brush, filename);
  return brush;
}

TempBuf *
gimp_brush_get_mask (GimpBrush *brush)
{
  g_return_val_if_fail(GIMP_IS_BRUSH(brush), NULL);
  return brush->mask;
}

char *
gimp_brush_get_name (GimpBrush *brush)
{
  g_return_val_if_fail(GIMP_IS_BRUSH(brush), NULL);
  return brush->name;
}

void
gimp_brush_set_name (GimpBrush *brush, char *name)
{
  g_return_if_fail(GIMP_IS_BRUSH(brush));
  if (strcmp(brush->name, name) == 0)
    return;
  if (brush->name)
    g_free(brush->name);
  brush->name = g_strdup(name);
  gtk_signal_emit(GTK_OBJECT(brush), gimp_brush_signals[RENAME]);
}

void
gimp_brush_load(GimpBrush *brush, char *filename)
{
  FILE * fp;
  int bn_size;
  unsigned char buf [sz_BrushHeader];
  BrushHeader header;
  unsigned int * hp;
  int i;

  brush->filename = g_strdup (filename);

  /*  Open the requested file  */
  if (! (fp = fopen (filename, "r")))
    {
      gimp_object_destroy (brush);
      return;
    }

  /*  Read in the header size  */
  if ((fread (buf, 1, sz_BrushHeader, fp)) < sz_BrushHeader)
    {
      fclose (fp);
      gimp_object_destroy (brush);
      return;
    }

  /*  rearrange the bytes in each unsigned int  */
  hp = (unsigned int *) &header;
  for (i = 0; i < (sz_BrushHeader / 4); i++)
    hp [i] = (buf [i * 4] << 24) + (buf [i * 4 + 1] << 16) +
             (buf [i * 4 + 2] << 8) + (buf [i * 4 + 3]);

  /*  Check for correct file format */
  if (header.magic_number != GBRUSH_MAGIC)
    {
      /*  One thing that can save this error is if the brush is version 1  */
      if (header.version != 1)
	{
	  fclose (fp);
	  gimp_object_destroy (brush);
	  return;
	}
    }

  if (header.version == 1)
  {
     /*  If this is a version 1 brush, set the fp back 8 bytes  */
     fseek (fp, -8, SEEK_CUR);
     header.header_size += 8;
     /*  spacing is not defined in version 1  */
     header.spacing = 25;
  }

  /*  Read in the brush name  */
  if ((bn_size = (header.header_size - sz_BrushHeader)))
  {
    brush->name = (char *) g_malloc (sizeof (char) * bn_size);
    if ((fread (brush->name, 1, bn_size, fp)) < bn_size)
    {
      g_message ("Error in GIMP brush file...aborting.");
      fclose (fp);
      gimp_object_destroy (brush);
      return;
    }
  }
  else
    brush->name = g_strdup ("Unnamed");

  switch(header.version)
  {
   case 1:
   case 2:
     /*  Get a new brush mask  */
     brush->mask = temp_buf_new (header.width, header.height, header.bytes,
				 0, 0, NULL);
     brush->spacing = header.spacing;
     /* set up spacing axis */
     brush->x_axis.x = header.width  / 2.0;
     brush->x_axis.y = 0.0;
     brush->y_axis.x = 0.0;
     brush->y_axis.y = header.height / 2.0;
  /*  Read the brush mask data  */
     if ((fread (temp_buf_data (brush->mask), 1, header.width * header.height,
		 fp)) <	 header.width * header.height)
       g_message ("GIMP brush file appears to be truncated.");
     break;
   default:
     g_message ("Unknown brush format version #%d in \"%s\"\n",
		header.version, filename);
     fclose (fp);
     gimp_object_destroy (brush);
     return;
  }

  /*  Clean up  */
  fclose (fp);

  /*  Swap the brush to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    temp_buf_swap (brush->mask);

  /* Check if the current brush is the default one */
/* lets see if it works with out this for now */
/*  if (strcmp(default_brush, prune_filename(filename)) == 0) {
	  active_brush = brush;
	  have_default_brush = 1;
  }*/ /* if */
}
