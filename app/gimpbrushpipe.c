/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Adrian Likins and Tor Lillqvist
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
#include <string.h>
#include <ctype.h>

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

#include "apptypes.h"
#include "appenv.h"
#include "brush_header.h"
#include "pattern_header.h"
#include "patterns.h"
#include "gimpbrush.h"
#include "gimpbrushpipe.h"
#include "paint_core.h"
#include "gimprc.h"

#include "libgimp/gimpmath.h"
#include "libgimp/gimpparasiteio.h"

#include "libgimp/gimpintl.h"


static GimpBrushClass  *parent_class;


static GimpBrush * gimp_brush_pipe_select_brush     (PaintCore *paint_core);
static gboolean    gimp_brush_pipe_want_null_motion (PaintCore *paint_core);
static void        gimp_brush_pipe_destroy          (GtkObject *object);


static GimpBrush *
gimp_brush_pipe_select_brush (PaintCore *paint_core)
{
  GimpBrushPipe *pipe;
  gint i, brushix, ix;
  gdouble angle;

  g_return_val_if_fail (paint_core != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH_PIPE (paint_core->brush), NULL);

  pipe = GIMP_BRUSH_PIPE (paint_core->brush);

  if (pipe->nbrushes == 1)
    return GIMP_BRUSH (pipe->current);

  brushix = 0;
  for (i = 0; i < pipe->dimension; i++)
    {
      switch (pipe->select[i])
	{
	case PIPE_SELECT_INCREMENTAL:
	  ix = (pipe->index[i] + 1) % pipe->rank[i];
	  break;
	case PIPE_SELECT_ANGULAR:
	  angle = atan2 (paint_core->cury - paint_core->lasty,
			 paint_core->curx - paint_core->lastx);
	  /* Offset angle to be compatible with PSP tubes */
	  angle += G_PI_2;
	  /* Map it to the [0..2*G_PI) interval */
	  if (angle < 0)
	    angle += 2.0 * G_PI;
	  else if (angle > 2.0 * G_PI)
	    angle -= 2.0 * G_PI;
	  ix = RINT (angle / (2.0 * G_PI) * pipe->rank[i]);
	  break;
	case PIPE_SELECT_RANDOM:
	  /* This probably isn't the right way */
	  ix = rand () % pipe->rank[i];
	  break;
	case PIPE_SELECT_PRESSURE:
	  ix = RINT (paint_core->curpressure * (pipe->rank[i] - 1));
	  break;
	case PIPE_SELECT_TILT_X:
	  ix = RINT (paint_core->curxtilt / 2.0 * pipe->rank[i]) + pipe->rank[i]/2;
	  break;
	case PIPE_SELECT_TILT_Y:
	  ix = RINT (paint_core->curytilt / 2.0 * pipe->rank[i]) + pipe->rank[i]/2;
	  break;
	case PIPE_SELECT_CONSTANT:
	default:
	  ix = pipe->index[i];
	  break;
	}
      pipe->index[i] = CLAMP (ix, 0, pipe->rank[i]-1);
      brushix += pipe->stride[i] * pipe->index[i];
    }

  /* Make sure is inside bounds */
  brushix = CLAMP (brushix, 0, pipe->nbrushes-1);

  pipe->current = pipe->brushes[brushix];

  return GIMP_BRUSH (pipe->current);
}

static gboolean
gimp_brush_pipe_want_null_motion (PaintCore *paint_core)
{
  GimpBrushPipe *pipe;
  gint i;

  g_return_val_if_fail (paint_core != NULL, TRUE);
  g_return_val_if_fail (GIMP_IS_BRUSH_PIPE (paint_core->brush), TRUE);

  pipe = GIMP_BRUSH_PIPE (paint_core->brush);

  if (pipe->nbrushes == 1)
    return TRUE;

  for (i = 0; i < pipe->dimension; i++)
    if (pipe->select[i] == PIPE_SELECT_ANGULAR)
      return FALSE;

  return TRUE;
}

static void
gimp_brush_pipe_destroy (GtkObject *object)
{
  GimpBrushPipe *pipe;
  gint i;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_BRUSH_PIPE (object));

  pipe = GIMP_BRUSH_PIPE (object);

  g_free (pipe->rank);
  g_free (pipe->stride);

  for (i = 1; i < pipe->nbrushes; i++)
    if (pipe->brushes[i])
      gtk_object_unref (GTK_OBJECT (pipe->brushes[i]));

  g_free (pipe->brushes);
  g_free (pipe->select);
  g_free (pipe->index);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_brush_pipe_class_init (GimpBrushPipeClass *klass)
{
  GtkObjectClass *object_class;
  GimpBrushClass *brush_class;

  object_class = GTK_OBJECT_CLASS (klass);
  brush_class = GIMP_BRUSH_CLASS (klass);

  parent_class = gtk_type_class (GIMP_TYPE_BRUSH);

  brush_class->select_brush     = gimp_brush_pipe_select_brush;
  brush_class->want_null_motion = gimp_brush_pipe_want_null_motion;

  object_class->destroy = gimp_brush_pipe_destroy;
}

void
gimp_brush_pipe_init (GimpBrushPipe *pipe)
{
  pipe->current   = NULL;
  pipe->dimension = 0;
  pipe->rank      = NULL;
  pipe->stride    = NULL;
  pipe->nbrushes  = 0;
  pipe->brushes   = NULL;
  pipe->select    = NULL;
  pipe->index     = NULL;
}

GtkType
gimp_brush_pipe_get_type (void)
{
  static GtkType type = 0;

  if (!type)
    {
      GtkTypeInfo info =
      {
	"GimpBrushPipe",
	sizeof (GimpBrushPipe),
	sizeof (GimpBrushPipeClass),
	(GtkClassInitFunc) gimp_brush_pipe_class_init,
	(GtkObjectInitFunc) gimp_brush_pipe_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL
      };

      type = gtk_type_unique (GIMP_TYPE_BRUSH, &info);
    }

  return type;
}

#include <errno.h>

GimpBrush *
gimp_brush_pipe_load (gchar *filename)
{
  GimpBrushPipe     *pipe = NULL;
  GimpPixPipeParams  params;
  gint     i;
  gint     num_of_brushes = 0;
  gint     totalcells;
  gchar   *paramstring;
  GString *buffer;
  gchar    c;
  gint     fd;

  g_return_val_if_fail (filename != NULL, NULL);

  fd = open (filename, O_RDONLY | _O_BINARY);
  if (fd == -1)
    {
      g_message ("Couldn't open file '%s'", filename);
      return NULL;
    }

  /* The file format starts with a painfully simple text header */

  /*  get the name  */
  buffer = g_string_new (NULL);
  while (read (fd, &c, 1) == 1 && c != '\n' && buffer->len < 1024)
    g_string_append_c (buffer, c);
    
  if (buffer->len > 0 && buffer->len < 1024)
    {
      pipe = GIMP_BRUSH_PIPE (gtk_type_new (GIMP_TYPE_BRUSH_PIPE));      
      GIMP_BRUSH (pipe)->name = buffer->str;
    }
  g_string_free (buffer, FALSE);

  if (!pipe)
    {
      g_message ("Couldn't read name for brush pipe from file '%s'\n", 
		 filename);
      close (fd);
      return NULL;
    }

  /*  get the number of brushes  */
  buffer = g_string_new (NULL);
  while (read (fd, &c, 1) == 1 && c != '\n' && buffer->len < 1024)
    g_string_append_c (buffer, c);

  if (buffer->len > 0 && buffer->len < 1024)
    {
      num_of_brushes = strtol (buffer->str, &paramstring, 10);
    }

  if (num_of_brushes < 1)
    {
      g_message (_("Brush pipes should have at least one brush:\n\"%s\""), 
		 filename);
      close (fd);
      gtk_object_sink (GTK_OBJECT (pipe));
      g_string_free (buffer, TRUE);
      return NULL;
    }

  while (*paramstring && isspace (*paramstring))
    paramstring++;

  if (*paramstring)
    {
      gimp_pixpipe_params_init (&params);
      gimp_pixpipe_params_parse (paramstring, &params);

      pipe->dimension = params.dim;
      pipe->rank      = g_new0 (gint, pipe->dimension);
      pipe->select    = g_new0 (PipeSelectModes, pipe->dimension);
      pipe->index     = g_new0 (gint, pipe->dimension);

      /* placement is not used at all ?? */
      if (params.free_placement_string)
	g_free (params.placement);

      for (i = 0; i < pipe->dimension; i++)
	{
	  pipe->rank[i] = params.rank[i];
	  if (strcmp (params.selection[i], "incremental") == 0)
	    pipe->select[i] = PIPE_SELECT_INCREMENTAL;
	  else if (strcmp (params.selection[i], "angular") == 0)
	    pipe->select[i] = PIPE_SELECT_ANGULAR;
	  else if (strcmp (params.selection[i], "velocity") == 0)
	    pipe->select[i] = PIPE_SELECT_VELOCITY;
	  else if (strcmp (params.selection[i], "random") == 0)
	    pipe->select[i] = PIPE_SELECT_RANDOM;
	  else if (strcmp (params.selection[i], "pressure") == 0)
	    pipe->select[i] = PIPE_SELECT_PRESSURE;
	  else if (strcmp (params.selection[i], "xtilt") == 0)
	    pipe->select[i] = PIPE_SELECT_TILT_X;
	  else if (strcmp (params.selection[i], "ytilt") == 0)
	    pipe->select[i] = PIPE_SELECT_TILT_Y;
	  else
	    pipe->select[i] = PIPE_SELECT_CONSTANT;
	  if (params.free_selection_string)
	    g_free (params.selection[i]);
	  pipe->index[i] = 0;
	}
    }
  else
    {
      pipe->dimension = 1;
      pipe->rank      = g_new (gint, 1);
      pipe->rank[0]   = num_of_brushes;
      pipe->select    = g_new (PipeSelectModes, 1);
      pipe->select[0] = PIPE_SELECT_INCREMENTAL;
      pipe->index     = g_new (gint, 1);
      pipe->index[0]  = 0;
    }

  g_string_free (buffer, TRUE);

  totalcells = 1;		/* Not all necessarily present, maybe */
  for (i = 0; i < pipe->dimension; i++)
    totalcells *= pipe->rank[i];
  pipe->stride = g_new0 (gint, pipe->dimension);
  for (i = 0; i < pipe->dimension; i++)
    {
      if (i == 0)
	pipe->stride[i] = totalcells / pipe->rank[i];
      else
	pipe->stride[i] = pipe->stride[i-1] / pipe->rank[i];
    }
  g_assert (pipe->stride[pipe->dimension-1] == 1);

  pipe->brushes = g_new0 (GimpBrush *, num_of_brushes);

  while (pipe->nbrushes < num_of_brushes)
    {
      pipe->brushes[pipe->nbrushes] = gimp_brush_load_brush (fd, filename);

      if (pipe->brushes[pipe->nbrushes])
	{
	  gtk_object_ref (GTK_OBJECT (pipe->brushes[pipe->nbrushes]));
	  gtk_object_sink (GTK_OBJECT (pipe->brushes[pipe->nbrushes]));
	  
	  g_free (GIMP_BRUSH (pipe->brushes[pipe->nbrushes])->name);
	  GIMP_BRUSH (pipe->brushes[pipe->nbrushes])->name = NULL;
	}
      else
	{
	  g_message (_("Failed to load one of the brushes in the brush pipe\n\"%s\""), 
		       filename);
	  close (fd);
	  gtk_object_sink (GTK_OBJECT (pipe));
	  return NULL;
	}
  
      pipe->nbrushes++;
    }

  /* Current brush is the first one. */
  pipe->current = pipe->brushes[0];

  /*  just to satisfy the code that relies on this crap  */
  GIMP_BRUSH (pipe)->filename = g_strdup (filename);
  GIMP_BRUSH (pipe)->spacing  = pipe->current->spacing;
  GIMP_BRUSH (pipe)->x_axis   = pipe->current->x_axis;
  GIMP_BRUSH (pipe)->y_axis   = pipe->current->y_axis;
  GIMP_BRUSH (pipe)->mask     = pipe->current->mask;
  GIMP_BRUSH (pipe)->pixmap   = pipe->current->pixmap;

  close (fd);

  return GIMP_BRUSH (pipe);
}


