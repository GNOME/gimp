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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "appenv.h"
#include "brush_header.h"
#include "pattern_header.h"
#include "patterns.h"
#include "gimpbrush.h"
#include "gimpbrushpipe.h"
#include "gimpbrushpipeP.h"
#include "paint_core.h"
#include "gimprc.h"
#include "libgimp/gimpintl.h"

/* Code duplicated from plug-ins/common/gpb.c...
 * The struct, and code to parse/build it probably should be in libgimp.
 */

/* Parameters related to one single gih file, collected in a struct
 * just for clarity.
 */
#define MAXDIM 4
static struct {
  gint step;
  gint ncells;
  gint dim;
  gint cols;
  gint rows;
  gchar *placement;
  gint rank[MAXDIM];
  gchar *selection[MAXDIM];
} gihparms;


static GimpBrushClass* gimp_brush_class;
static GtkObjectClass* gimp_object_class;

static GimpBrush *gimp_brush_pixmap_select_brush (PaintCore *paint_core);
static gboolean gimp_brush_pixmap_want_null_motion (PaintCore *paint_core);

static void paint_line_pixmap_mask(GImage	   *dest,
				   GimpDrawable    *drawable,
				   GimpBrushPixmap *brush,
				   guchar	   *d,
				   int		    x,
				   int              y,
				   int              bytes,
				   int              width);

static void
gimp_brush_pixmap_destroy (GtkObject *object)
{
  GimpBrushPixmap *pixmap;

  g_return_if_fail (GIMP_IS_BRUSH_PIXMAP (object));

  pixmap = GIMP_BRUSH_PIXMAP (object);
  
  temp_buf_free (pixmap->pixmap_mask);

  (* GTK_OBJECT_CLASS (gimp_object_class)->destroy) (object);
}

static void
gimp_brush_pixmap_class_init (GimpBrushPixmapClass *klass)
{
  GtkObjectClass *object_class;
  GimpBrushClass *brush_class;
  
  object_class = GTK_OBJECT_CLASS (klass);
  brush_class = GIMP_BRUSH_CLASS (klass);

  gimp_brush_class = gtk_type_class (gimp_brush_get_type ());

  object_class->destroy =  gimp_brush_pixmap_destroy;
  brush_class->select_brush = gimp_brush_pixmap_select_brush;
  brush_class->want_null_motion = gimp_brush_pixmap_want_null_motion;
}

void
gimp_brush_pixmap_init (GimpBrushPixmap *brush)
{
  brush->pixmap_mask = NULL;
  brush->pipe = NULL;
}

GtkType
gimp_brush_pixmap_get_type (void)
{
  static GtkType type=0;
  if(!type){
    GtkTypeInfo info={
      "GimpBrushPixmap",
      sizeof (GimpBrushPixmap),
      sizeof (GimpBrushPixmapClass),
      (GtkClassInitFunc) gimp_brush_pixmap_class_init,
      (GtkObjectInitFunc) gimp_brush_pixmap_init,
     /* reserved_1 */ NULL,
     /* reserved_2 */ NULL,
    (GtkClassInitFunc) NULL};
    type = gtk_type_unique (GIMP_TYPE_BRUSH, &info);
  }
  return type;
}

static GimpBrush *
gimp_brush_pixmap_select_brush (PaintCore *paint_core)
{
  GimpBrushPipe *pipe;
  int i, brushix, ix;
  double angle;

  g_return_val_if_fail (GIMP_IS_BRUSH_PIXMAP (paint_core->brush), NULL);

  pipe = GIMP_BRUSH_PIXMAP (paint_core->brush)->pipe;

  if (pipe->nbrushes == 1)
    return GIMP_BRUSH (pipe->current);

  brushix = 0;
  for (i = 0; i < pipe->dimension; i++)
    {
      switch (pipe->select[i])
	{
	case PIPE_SELECT_CONSTANT:
	  ix = pipe->index[i];
	  break;
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
	    angle += 2.*G_PI;
	  else if (angle > 2.*G_PI)
	    angle -= 2.*G_PI;
	  ix = RINT (angle / (2.*G_PI) * pipe->rank[i]);
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
	}
      pipe->index[i] = BOUNDS (ix, 0, pipe->rank[i]-1);
      brushix += pipe->stride[i] * pipe->index[i];
      /* g_print ("ix at %d: %d, brushix: %d\n", i, ix, brushix); */
    }

  /* Make sure is inside bounds */
  brushix = BOUNDS (brushix, 0, pipe->nbrushes-1);

  pipe->current = pipe->brushes[brushix];

  return GIMP_BRUSH (pipe->current);
}

static gboolean
gimp_brush_pixmap_want_null_motion (PaintCore *paint_core)
{
  GimpBrushPipe *pipe;
  int i;

  g_return_val_if_fail (GIMP_IS_BRUSH_PIXMAP (paint_core->brush), TRUE);

  pipe = GIMP_BRUSH_PIXMAP (paint_core->brush)->pipe;

  if (pipe->nbrushes == 1)
    return TRUE;

  for (i = 0; i < pipe->dimension; i++)
    if (pipe->select[i] == PIPE_SELECT_ANGULAR)
      return FALSE;

  return TRUE;
}

static void
gimp_brush_pipe_destroy(GtkObject *object)
{
  GimpBrushPipe *pipe;
  int i;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_BRUSH_PIPE (object));

  pipe = GIMP_BRUSH_PIPE (object);

  g_free (pipe->rank);
  g_free (pipe->stride);

  for (i = 1; i < pipe->nbrushes; i++)
    gimp_object_destroy (pipe->brushes[i]);

  g_free (pipe->brushes);
  g_free (pipe->select);
  g_free (pipe->index);

  if (GTK_OBJECT_CLASS (gimp_object_class)->destroy)
    (* GTK_OBJECT_CLASS (gimp_object_class)->destroy) (object);
}

static void
gimp_brush_pipe_class_init (GimpBrushPipeClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (klass);

  gimp_object_class = gtk_type_class (GIMP_TYPE_OBJECT);
  object_class->destroy =  gimp_brush_pipe_destroy;
}

void
gimp_brush_pipe_init (GimpBrushPipe *pipe)
{
  pipe->dimension = 0;
  pipe->rank = NULL;
  pipe->nbrushes = 0;
  pipe->select = NULL;
  pipe->index = NULL;
}

GtkType
gimp_brush_pipe_get_type (void)
{
  static GtkType type=0;
  if (!type){
    GtkTypeInfo info={
      "GimpBrushPipe",
      sizeof (GimpBrushPipe),
      sizeof (GimpBrushPipeClass),
      (GtkClassInitFunc) gimp_brush_pipe_class_init,
      (GtkObjectInitFunc) gimp_brush_pipe_init,
     /* reserved_1 */ NULL,
     /* reserved_2 */ NULL,
    (GtkClassInitFunc) NULL};
    type = gtk_type_unique (GIMP_TYPE_BRUSH_PIXMAP, &info);
  }
  return type;
}

static void
init_pipe_parameters ()
{
  int i;

  gihparms.step = 100;
  gihparms.ncells = 1;
  gihparms.dim = 1;
  gihparms.cols = 1;
  gihparms.rows = 1;
  gihparms.placement = "constant";
  for (i = 0; i < MAXDIM; i++)
    gihparms.selection[i] = "random";
  gihparms.rank[0] = 1;
  for (i = 1; i < MAXDIM; i++)
    gihparms.rank[i] = 0;
}

static void
parse_brush_pipe_parameters (gchar *parameters)
{
  guchar *p, *q, *r, *s;	/* Don't you love single-char identifiers?  */
  gint i;

  q = parameters;
  while ((p = strtok (q, " \r\n")) != NULL)
    {
      q = NULL;
      r = strchr (p, ':');
      if (r)
	*r = 0;

      if (strcmp (p, "ncells") == 0)
	{
	  if (r)
	    gihparms.ncells = atoi (r + 1);
	}
      else if (strcmp (p, "step") == 0)
	{
	  if (r)
	    gihparms.step = atoi (r + 1);
	}
      else if (strcmp (p, "dim") == 0)
	{
	  if (r)
	    gihparms.dim = atoi (r + 1);
	}
      else if (strcmp (p, "cols") == 0)
	{
	  if (r)
	    gihparms.cols = atoi (r + 1);
	}
      else if (strcmp (p, "rows") == 0)
	{
	  if (r)
	    gihparms.rows = atoi (r + 1);
	}
      else if (strcmp (p, "placement") == 0)
	{
	  if (r)
	    gihparms.placement = g_strdup (r + 1);
	}
      else if (strncmp (p, "rank", strlen ("rank")) == 0)
	{
	  if (r)
	    {
	      i = atoi (p + strlen ("rank"));
	      if (i >= 0 && i < gihparms.dim)
		gihparms.rank[i] = atoi (r + 1);
	    }
	}
      else if (strncmp (p, "sel", strlen ("sel")) == 0)
	{
	  if (r)
	    {
	      i = atoi (p + strlen ("sel"));
	      if (i >= 0 && i < gihparms.dim)
		gihparms.selection[i] = g_strdup (r + 1);
	    }
	}
      if (r)
	*r = ':';
    }
}

GimpBrushPipe *
gimp_brush_pipe_load (char *filename)
{
  GimpBrushPipe *pipe;
  GPatternP pattern;
  FILE *fp;
  guchar buf[1024];
  guchar *name;
  int i;
  int num_of_brushes;
  int totalcells;
  gchar *params;

  if ((fp = fopen (filename, "rb")) == NULL)
    return NULL;

  /* The file format starts with a painfully simple text header
   * and we use a painfully simple way to read it
   */
  if (fgets (buf, 1024, fp) == NULL)
    {
      fclose (fp);
      return NULL;
    }
  buf[strlen (buf) - 1] = 0;

  pipe = GIMP_BRUSH_PIPE (gimp_type_new (GIMP_TYPE_BRUSH_PIPE));
  name = g_strdup (buf);

  /* get the number of brushes */
  if (fgets (buf, 1024, fp) == NULL)
    {
      fclose (fp);
      gimp_object_destroy (pipe);
      return NULL;
    }
  num_of_brushes = strtol(buf, &params, 10);
  if (num_of_brushes < 1)
    {
      g_message (_("pixmap brush pipe should have at least one brush"));
      fclose (fp);
      gimp_object_destroy (pipe);
      return NULL;
    }

  while (*params && isspace(*params))
    params++;

  if (*params)
    {
      init_pipe_parameters ();
      parse_brush_pipe_parameters (params);
      pipe->dimension = gihparms.dim;
      pipe->rank = g_new (int, pipe->dimension);
      pipe->select = g_new (PipeSelectModes, pipe->dimension);
      pipe->index = g_new (int, pipe->dimension);
      for (i = 0; i < pipe->dimension; i++)
	{
	  pipe->rank[i] = gihparms.rank[i];
	  if (strcmp (gihparms.selection[i], "incremental") == 0)
	    pipe->select[i] = PIPE_SELECT_INCREMENTAL;
	  else if (strcmp (gihparms.selection[i], "angular") == 0)
	    pipe->select[i] = PIPE_SELECT_ANGULAR;
	  else if (strcmp (gihparms.selection[i], "velocity") == 0)
	    pipe->select[i] = PIPE_SELECT_VELOCITY;
	  else if (strcmp (gihparms.selection[i], "random") == 0)
	    pipe->select[i] = PIPE_SELECT_RANDOM;
	  else if (strcmp (gihparms.selection[i], "pressure") == 0)
	    pipe->select[i] = PIPE_SELECT_PRESSURE;
	  else if (strcmp (gihparms.selection[i], "xtilt") == 0)
	    pipe->select[i] = PIPE_SELECT_TILT_X;
	  else if (strcmp (gihparms.selection[i], "ytilt") == 0)
	    pipe->select[i] = PIPE_SELECT_TILT_Y;
	  else
	    pipe->select[i] = PIPE_SELECT_CONSTANT;
	  pipe->index[i] = 0;
	}
    }
  else
    {
      pipe->dimension = 1;
      pipe->rank = g_new (int, 1);
      pipe->rank[0] = num_of_brushes;
      pipe->select = g_new (PipeSelectModes, 1);
      pipe->select[0] = PIPE_SELECT_INCREMENTAL;
      pipe->index = g_new (int, 1);
      pipe->index[0] = 0;
    }

  totalcells = 1;		/* Not all necessarily present, maybe */
  for (i = 0; i < pipe->dimension; i++)
    totalcells *= pipe->rank[i];
  pipe->stride = g_new (int, pipe->dimension);
  for (i = 0; i < pipe->dimension; i++)
    {
      if (i == 0)
	pipe->stride[i] = totalcells / pipe->rank[i];
      else
	pipe->stride[i] = pipe->stride[i-1] / pipe->rank[i];
    }
  g_assert (pipe->stride[pipe->dimension-1] == 1);

  pattern = (GPatternP) g_malloc (sizeof (GPattern));
  pattern->filename = NULL;
  pattern->name = NULL;
  pattern->mask = NULL;

  pipe->brushes =
    (GimpBrushPixmap **) g_new0 (GimpBrushPixmap *, num_of_brushes);

  /* First pixmap brush in the list is the pipe itself */
  pipe->brushes[0] = GIMP_BRUSH_PIXMAP (pipe);

  /* Current pixmap brush is the first one. */
  pipe->current = pipe->brushes[0];

  while (pipe->nbrushes < num_of_brushes)
    {
      if (pipe->nbrushes > 0)
	{
	  pipe->brushes[pipe->nbrushes] =
	    GIMP_BRUSH_PIXMAP (gimp_type_new (GIMP_TYPE_BRUSH_PIXMAP));
	  GIMP_BRUSH (pipe->brushes[pipe->nbrushes])->name = NULL;
	}
      
      pipe->brushes[pipe->nbrushes]->pipe = pipe;

      /* load the brush */
      if (!gimp_brush_load_brush (GIMP_BRUSH (pipe->brushes[pipe->nbrushes]),
				  fp, filename)
	  || !load_pattern_pattern (pattern, fp, filename))
       {
	  g_message (_("failed to load one of the pixmap brushes in the pipe"));
	  fclose (fp);
	  g_free (pattern);
	  gimp_object_destroy (pipe);
	  return NULL;
       }

      if (pipe->nbrushes == 0)
	{
	  /* Replace name with the whole pipe's name */
	  GIMP_BRUSH (pipe)->name = name;
	}
      pipe->brushes[pipe->nbrushes]->pixmap_mask = pattern->mask;

      pipe->nbrushes++;
    }

  /*  Clean up  */
  fclose (fp);

  g_free(pattern);
  return pipe;
}

GimpBrushPipe *
gimp_brush_pixmap_load (char *filename)
{
  GimpBrushPipe *pipe;
  GPatternP pattern;
  FILE *fp;

  if ((fp = fopen (filename, "rb")) == NULL)
    return NULL;

  pipe = GIMP_BRUSH_PIPE (gimp_type_new (GIMP_TYPE_BRUSH_PIPE));

  /* A (single) pixmap brush is a pixmap pipe brush with just one pixmap */
  pipe->dimension = 1;
  pipe->rank = g_new (int, 1);
  pipe->rank[0] = 1;
  pipe->select = g_new (PipeSelectModes, 1);
  pipe->select[0] = PIPE_SELECT_INCREMENTAL;
  pipe->index = g_new (int, 1);
  pipe->index[0] = 0;

  pattern = (GPatternP) g_malloc (sizeof (GPattern));
  pattern->filename = NULL;
  pattern->name = NULL;
  pattern->mask = NULL;

  pipe->brushes = (GimpBrushPixmap **) g_new (GimpBrushPixmap *, 1);

  pipe->brushes[0] = GIMP_BRUSH_PIXMAP (pipe);
  pipe->current = pipe->brushes[0];

  pipe->brushes[0]->pipe = pipe;

  /* load the brush */
  if (!gimp_brush_load_brush (GIMP_BRUSH (pipe->brushes[0]),
			      fp, filename)
      || !load_pattern_pattern (pattern, fp, filename))
    {
      g_message (_("failed to load pixmap brush"));
      fclose (fp);
      g_free (pattern);
      gimp_object_destroy (pipe);
      return NULL;
    }
  
  pipe->brushes[0]->pixmap_mask = pattern->mask;

  pipe->nbrushes = 1;
  /*  Clean up  */
  fclose (fp);

  g_free(pattern);
  return pipe;
}

TempBuf *
gimp_brush_pixmap_pixmap (GimpBrushPixmap *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH_PIXMAP (brush), NULL);

  return brush->pixmap_mask;
}

void
color_area_with_pixmap (PaintCore *paint_core,
			GImage *dest,
			GimpDrawable *drawable,
			TempBuf *area)
{

  PixelRegion destPR;
  void *pr;
  guchar *d;
  int ulx, uly, offsetx, offsety, y;
  GimpBrushPixmap *pixmap;
  
  g_return_if_fail (GIMP_IS_BRUSH_PIXMAP (paint_core->brush));

  pixmap = GIMP_BRUSH_PIXMAP (paint_core->brush);

  destPR.bytes = area->bytes;
  destPR.x = 0; destPR.y = 0;
  destPR.w = area->width;
  destPR.h = area->height;
  destPR.rowstride = destPR.bytes * area->width;
  destPR.data = temp_buf_data (area);
		
  pr = pixel_regions_register (1, &destPR);

  /* Calculate upper left corner of brush as in
   * paint_core_get_paint_area.  Ugly to have to do this here, too.
   */

  ulx = (int) paint_core->curx - (paint_core->brush->mask->width >> 1);
  uly = (int) paint_core->cury - (paint_core->brush->mask->height >> 1);

  offsetx = area->x - ulx;
  offsety = area->y - uly;

  for (; pr != NULL; pr = pixel_regions_process (pr))
    {
      d = destPR.data;
      for (y = 0; y < destPR.h; y++)
	{
	  paint_line_pixmap_mask (dest, drawable, pixmap,
				  d, offsetx, y + offsety,
				  destPR.bytes, destPR.w);
	  d += destPR.rowstride;
	}
    }
}

static void
paint_line_pixmap_mask (GImage          *dest,
			GimpDrawable    *drawable,
			GimpBrushPixmap *brush,
			guchar	        *d,
			int		 x,
			int              y,
			int              bytes,
			int              width)
{
  guchar *b, *p;
  int x_index;
  gdouble alpha;
  gdouble factor = 0.00392156986;  /* 1.0/255.0 */
  gint i;
  gint j;
  guchar *mask;

  /*  Make sure x, y are positive  */
  while (x < 0)
    x += brush->pixmap_mask->width;
  while (y < 0)
    y += brush->pixmap_mask->height;

  /* Point to the approriate scanline */
  b = temp_buf_data (brush->pixmap_mask) +
    (y % brush->pixmap_mask->height) * brush->pixmap_mask->width * brush->pixmap_mask->bytes;
    
  /* ditto, except for the brush mask, so we can pre-multiply the alpha value */
  mask = temp_buf_data((brush->gbrush).mask) +
    (y % brush->pixmap_mask->height) * brush->pixmap_mask->width;
 
  for (i = 0; i < width; i++)
    {
      /* attempt to avoid doing this calc twice in the loop */
      x_index = ((i + x) % brush->pixmap_mask->width);
      p = b + x_index * brush->pixmap_mask->bytes;
      d[bytes-1] = mask[x_index];

      /* multiply alpha into the pixmap data */
      /* maybe we could do this at tool creation or brush switch time? */
      /* and compute it for the whole brush at once and cache it?  */
      alpha = d[bytes-1] * factor;
      d[0] *= alpha;
      d[1] *= alpha;
      d[2] *= alpha;
      /* printf("i: %i d->r: %i d->g: %i d->b: %i d->a: %i\n",i,(int)d[0], (int)d[1], (int)d[2], (int)d[3]); */
      gimage_transform_color (dest, drawable, p, d, RGB);
      d += bytes;
    }
}

