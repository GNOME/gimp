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
#include <string.h>
#include "appenv.h"
#include "draw_core.h"
#include "edit_selection.h"
#include "errors.h"
#include "free_select.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "rect_select.h"
#include "paint_funcs.h"

typedef struct _free_select FreeSelect;

struct _free_select
{
  DrawCore *      core;       /*  Core select object                      */
  int             num_pts;    /*  Number of points in the polygon         */
  int             op;         /*  selection operation (ADD, SUB, etc)     */
};

typedef struct _FreeSelectPoint FreeSelectPoint;

struct _FreeSelectPoint
{
  double x, y;
};

#define DEFAULT_MAX_INC  1024
#define ROUND(x) ((int) (x + 0.5))
#define SUPERSAMPLE      3
#define SUPERSAMPLE2     9

#define NO   0
#define YES  1

/*  The global array of XPoints for drawing the polygon...  */
static GdkPoint *         global_pts = NULL;
static int                max_segs = 0;
static SelectionOptions * free_options = NULL;

static Argument *free_select_invoker (Argument *);

static int
add_point (int num_pts, int x, int y)
{
  if (num_pts >= max_segs)
    {
      max_segs += DEFAULT_MAX_INC;

      global_pts = (GdkPoint *) g_realloc ((void *) global_pts, sizeof (GdkPoint) * max_segs);

      if (!global_pts)
	fatal_error ("Unable to reallocate points array in free_select.");
    }

  global_pts[num_pts].x = x;
  global_pts[num_pts].y = y;

  return 1;
}


/*  Routines to scan convert the polygon  */

static GSList *
insert_into_sorted_list (GSList *list, int x)
{
  GSList *orig = list;
  GSList *rest;

  if (!list)
    return g_slist_prepend (list, (gpointer) ((long) x));

  while (list)
    {
      rest = g_slist_next (list);
      if (x < (long) list->data)
	{
	  rest = g_slist_prepend (rest, list->data);
	  list->next = rest;
	  list->data = (gpointer) ((long) x);
	  return orig;
	}
      else if (!rest)
	{
	  g_slist_append (list, (gpointer) ((long) x));
	  return orig;
	}
      list = g_slist_next (list);
    }

  return orig;
}

static void
convert_segment (GSList **scanlines, int width, int height,
		 int x1, int y1, int x2, int y2)
{
  int ydiff, y, tmp;
  float xinc, xstart;

  if (y1 > y2)
    { tmp = y2; y2 = y1; y1 = tmp;
      tmp = x2; x2 = x1; x1 = tmp; }
  ydiff = (y2 - y1);

  if ( ydiff )
    {
      xinc = (float) (x2 - x1) / (float) ydiff;
      xstart = x1 + 0.5 * xinc;
      for (y = y1 ; y < y2; y++)
	{
	  if (y >= 0 && y < height)
	    scanlines[y] = insert_into_sorted_list (scanlines[y], ROUND (xstart));
	  xstart += xinc;
	}
    }
}

static Channel *
scan_convert (int gimage_ID, int num_pts, FreeSelectPoint *pts,
	      int width, int height, int antialias)
{
  PixelRegion maskPR;
  Channel * mask;
  GSList **scanlines;
  GSList *list;
  unsigned char *buf, *b;
  int * vals, val;
  int start, end;
  int x, x2, w;
  int i, j;

  buf  = NULL;
  vals = NULL;

  if (num_pts < 3)
    return NULL;

  mask = channel_new_mask (gimage_ID, width, height);

  if (antialias)
    {
      buf = (unsigned char *) g_malloc (width);
      width  *= SUPERSAMPLE;
      height *= SUPERSAMPLE;
      /* allocate value array  */
      vals = (int *) g_malloc (sizeof (int) * width);
    }

  scanlines = (GSList **) g_malloc (sizeof (GSList *) * height);
  for (i = 0; i < height; i++)
    scanlines[i] = NULL;

  for (i = 0; i < (num_pts - 1); i++)
    {
      if (antialias)
	convert_segment (scanlines, width, height,
			 (int) pts[i].x * SUPERSAMPLE, (int) pts[i].y * SUPERSAMPLE,
			 (int) pts[i + 1].x * SUPERSAMPLE, (int) pts[i + 1].y * SUPERSAMPLE);
      else
	convert_segment (scanlines, width, height,
			 (int) pts[i].x, (int) pts[i].y,
			 (int) pts[i + 1].x, (int) pts[i + 1].y);
    }

  /*  check for a connecting Point  */
  if (pts[num_pts - 1].x != pts[0].x ||
      pts[num_pts - 1].y != pts[0].y)
    {
      if (antialias)
	convert_segment (scanlines, width, height,
			 (int) pts[num_pts - 1].x * SUPERSAMPLE,
			 (int) pts[num_pts - 1].y * SUPERSAMPLE,
			 (int) pts[0].x * SUPERSAMPLE, (int) pts[0].y * SUPERSAMPLE);
      else
	convert_segment (scanlines, width, height,
			 (int) pts[num_pts - 1].x, (int) pts[num_pts - 1].y,
			 (int) pts[0].x, (int) pts[0].y);
    }

  pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)), 0, 0, 
		     drawable_width (GIMP_DRAWABLE(mask)), 
		     drawable_height (GIMP_DRAWABLE(mask)), TRUE);
  for (i = 0; i < height; i++)
    {
      list = scanlines[i];

      /*  zero the vals array  */
      if (antialias && !(i % SUPERSAMPLE))
	memset (vals, 0, width * sizeof (int));

      while (list)
	{
	  x = (long) list->data;
	  list = g_slist_next(list);
	  if (!list)
	      g_message ("Cannot properly scanline convert polygon!\n");
	  else
	    {
	      /*  bounds checking  */
	      x = BOUNDS (x, 0, width);
	      x2 = BOUNDS ((long) list->data, 0, width);

	      w = x2 - x;

	      if (w > 0)
		{
		  if (! antialias)
		    channel_add_segment (mask, x, i, w, 255);
		  else
		    for (j = 0; j < w; j++)
		      vals[j + x] += 255;
		}
	      list = g_slist_next (list);
	    }
	}

      if (antialias && !((i+1) % SUPERSAMPLE))
	{
	  b = buf;
	  start = 0;
	  end = width;
	  for (j = start; j < end; j += SUPERSAMPLE)
	    {
	      val = 0;
	      for (x = 0; x < SUPERSAMPLE; x++)
		val += vals[j + x];

	      *b++ = (unsigned char) (val / SUPERSAMPLE2);
	    }

	  pixel_region_set_row (&maskPR, 0, (i / SUPERSAMPLE), 
				drawable_width (GIMP_DRAWABLE(mask)), buf);
	}

      g_slist_free (scanlines[i]);
    }

  if (antialias)
    {
      g_free (vals);
      g_free (buf);
    }

  g_free (scanlines);

  return mask;
}


/*************************************/
/*  Polygonal selection apparatus  */

static void
free_select (GImage *gimage, int num_pts, FreeSelectPoint *pts, int op,
	     int antialias, int feather, double feather_radius)
{
  Channel *mask;

  /*  if applicable, replace the current selection  */
  /*  or insure that a floating selection is anchored down...  */
  if (op == REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  mask = scan_convert (gimage->ID, num_pts, pts, gimage->width, gimage->height, antialias);

  if (mask)
    {
      if (feather)
	channel_feather (mask, gimage_get_mask (gimage),
			 feather_radius, op, 0, 0);
      else
	channel_combine_mask (gimage_get_mask (gimage),
			      mask, op, 0, 0);
      channel_delete (mask);
    }
}

void
free_select_button_press (Tool *tool, GdkEventButton *bevent,
			  gpointer gdisp_ptr)
{
  GDisplay *gdisp;
  FreeSelect *free_sel;

  gdisp = (GDisplay *) gdisp_ptr;
  free_sel = (FreeSelect *) tool->private;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  if (bevent->state & GDK_MOD1_MASK)
    {
      init_edit_selection (tool, gdisp_ptr, bevent, MaskTranslate);
      return;
    }
  else if ((bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
    free_sel->op = ADD;
  else if ((bevent->state & GDK_CONTROL_MASK) && !(bevent->state & GDK_SHIFT_MASK))
    free_sel->op = SUB;
  else if ((bevent->state & GDK_CONTROL_MASK) && (bevent->state & GDK_SHIFT_MASK))
    free_sel->op = INTERSECT;
  else
    {
      if (! (layer_is_floating_sel (gimage_get_active_layer (gdisp->gimage))) &&
	  gdisplay_mask_value (gdisp, bevent->x, bevent->y) > HALF_WAY)
	{
	  init_edit_selection (tool, gdisp_ptr, bevent, MaskToLayerTranslate);
	  return;
	}
      free_sel->op = REPLACE;
    }

  add_point (0, bevent->x, bevent->y);
  free_sel->num_pts = 1;

  draw_core_start (free_sel->core,
		   gdisp->canvas->window,
		   tool);
}

void
free_select_button_release (Tool *tool, GdkEventButton *bevent,
			    gpointer gdisp_ptr)
{
  FreeSelect *free_sel;
  FreeSelectPoint *pts;
  GDisplay *gdisp;
  int i;

  gdisp = (GDisplay *) gdisp_ptr;
  free_sel = (FreeSelect *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  draw_core_stop (free_sel->core, tool);

  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      pts = (FreeSelectPoint *) g_malloc (sizeof (FreeSelectPoint) * free_sel->num_pts);

      for (i = 0; i < free_sel->num_pts; i++)
	{
	  gdisplay_untransform_coords_f (gdisp, global_pts[i].x, global_pts[i].y,
					 &pts[i].x, &pts[i].y, FALSE);
	}

      free_select (gdisp->gimage, free_sel->num_pts, pts, free_sel->op,
		   free_options->antialias, free_options->feather,
		   free_options->feather_radius);

      g_free (pts);

      gdisplays_flush ();
    }
}

void
free_select_motion (Tool *tool, GdkEventMotion *mevent, gpointer gdisp_ptr)
{
  FreeSelect *free_sel;
  GDisplay *gdisp;

  if (tool->state != ACTIVE)
    return;

  gdisp = (GDisplay *) gdisp_ptr;
  free_sel = (FreeSelect *) tool->private;

  if (add_point (free_sel->num_pts, mevent->x, mevent->y))
    {
      gdk_draw_line (free_sel->core->win, free_sel->core->gc,
		     global_pts[free_sel->num_pts - 1].x,
		     global_pts[free_sel->num_pts - 1].y,
		     global_pts[free_sel->num_pts].x,
		     global_pts[free_sel->num_pts].y);
      free_sel->num_pts ++;
    }
}

static void
free_select_control (Tool *tool, int action, gpointer gdisp_ptr)
{
  FreeSelect * free_sel;

  free_sel = (FreeSelect *) tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (free_sel->core, tool);
      break;
    case RESUME :
      draw_core_resume (free_sel->core, tool);
      break;
    case HALT :
      draw_core_stop (free_sel->core, tool);
      break;
    }
}

void
free_select_draw (Tool *tool)
{
  FreeSelect * free_sel;
  int i;

  free_sel = (FreeSelect *) tool->private;

  for (i = 1; i < free_sel->num_pts; i++)
    gdk_draw_line (free_sel->core->win, free_sel->core->gc,
		   global_pts[i - 1].x, global_pts[i - 1].y,
		   global_pts[i].x, global_pts[i].y);
}

Tool *
tools_new_free_select (void)
{
  Tool * tool;
  FreeSelect * private;

  /*  The tool options  */
  if (!free_options)
    free_options = create_selection_options (FREE_SELECT);

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (FreeSelect *) g_malloc (sizeof (FreeSelect));

  private->core = draw_core_new (free_select_draw);
  private->num_pts = 0;

  tool->type = FREE_SELECT;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;   /*  Do not allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = free_select_button_press;
  tool->button_release_func = free_select_button_release;
  tool->motion_func = free_select_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = rect_select_cursor_update;
  tool->control_func = free_select_control;
  tool->preserve = TRUE;

  return tool;
}

void
tools_free_free_select (Tool *tool)
{
  FreeSelect * free_sel;

  free_sel = (FreeSelect *) tool->private;

  draw_core_free (free_sel->core);
  g_free (free_sel);
}

/*  The free_select procedure definition  */
ProcArg free_select_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_INT32,
    "num_pts",
    "number of points (count 1 coordinate as two points)"
  },
  { PDB_FLOATARRAY,
    "segs",
    "array of points: { p1.x, p1.y, p2.x, p2.y, ..., pn.x, pn.y}"
  },
  { PDB_INT32,
    "operation",
    "the selection operation: { ADD (0), SUB (1), REPLACE (2), INTERSECT (3) }"
  },
  { PDB_INT32,
    "antialias",
    "antialiasing option for selections"
  },
  { PDB_INT32,
    "feather",
    "feather option for selections"
  },
  { PDB_FLOAT,
    "feather_radius",
    "radius for feather operation"
  }
};

ProcRecord free_select_proc =
{
  "gimp_free_select",
  "Create a polygonal selection over the specified image",
  "This tool creates a polygonal selection over the specified image.  The polygonal region can be either added to, subtracted from, or replace the contents of the previous selection mask.  The polygon is specified through an array of floating point numbers and its length.  The length of array must be 2n, where n is the number of points.  Each point is defined by 2 floating point values which correspond to the x and y coordinates.  If the final point does not connect to the starting point, a connecting segment is automatically added.  If the feather option is enabled, the resulting selection is blurred before combining.  The blur is a gaussian blur with the specified feather radius.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  7,
  free_select_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { free_select_invoker } },
};


static Argument *
free_select_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  int op;
  int antialias;
  int feather;
  int num_pts;
  FreeSelectPoint *pt_array;
  double feather_radius;
  int int_value;

  op      = REPLACE;
  num_pts = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  num pts  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (int_value >= 2)
	num_pts = int_value / 2;
      else
	success = FALSE;
    }
  /*  point array  */
  if (success)
    pt_array = (FreeSelectPoint *) args[2].value.pdb_pointer;
  /*  operation  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      switch (int_value)
	{
	case 0: op = ADD; break;
	case 1: op = SUB; break;
	case 2: op = REPLACE; break;
	case 3: op = INTERSECT; break;
	default: success = FALSE;
	}
    }
  /*  antialiasing  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      antialias = (int_value) ? TRUE : FALSE;
    }
  /*  feathering  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      feather = (int_value) ? TRUE : FALSE;
    }
  /*  feather radius  */
  if (success)
    {
      feather_radius = args[6].value.pdb_float;
    }

  /*  call the free_select procedure  */
  if (success)
    free_select (gimage, num_pts, pt_array,
		 op, antialias, feather, feather_radius);

  return procedural_db_return_args (&free_select_proc, success);
}
