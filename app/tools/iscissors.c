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

/* This tool is based on a paper from SIGGRAPH '95:
 *  "Intelligent Scissors for Image Composition", Eric N. Mortensen and
 *   William A. Barrett, Brigham Young University.
 *
 * thanks to Professor D. Forsyth for prompting us to implement this tool. */

/* The history of this implementation is lonog and varied.  It was
 * orignally done by Spencer and Peter, and worked fine in the 0.54
 * (motif only) release of the gimp.  Later revisions (0.99.something
 * until about 1.1.4) completely changed the algorithm used, until it
 * bore little resemblance to the one described in the paper above.
 * The 0.54 version of the algorithm was then forwards ported to 1.1.4
 * by Austin Donnelly.  */

#include "config.h"

#include <stdlib.h>

#include <gdk/gdkkeysyms.h>

#include "appenv.h"
#include "draw_core.h"
#include "channel_pvt.h"
#include "cursorutil.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "iscissors.h"
#include "edit_selection.h"
#include "paint_funcs.h"
#include "selection_options.h"
#include "temp_buf.h"
#include "tools.h"
#include "bezier_selectP.h"
#include "scan_convert.h"

#include "libgimp/gimpmath.h"

#ifdef DEBUG
#define TRC(x) g_print x
#define D(x) x
#else
#define TRC(x)
#define D(x)
#endif


/*  local structures  */

typedef struct _ICurve ICurve;

struct _ICurve
{
  int x1, y1;
  int x2, y2;
  GPtrArray *points;
};

/*  The possible states...  */
typedef enum
{
  NO_ACTION,
  SEED_PLACEMENT,
  SEED_ADJUSTMENT,
  WAITING
} Iscissors_state;

/*  The possible drawing states...  */
typedef enum
{
  DRAW_NOTHING      = 0x0,
  DRAW_CURRENT_SEED = 0x1,
  DRAW_CURVE        = 0x2,
  DRAW_ACTIVE_CURVE = 0x4
} Iscissors_draw;
#define  DRAW_ALL          (DRAW_CURRENT_SEED | DRAW_CURVE)

typedef struct _iscissors Iscissors;

struct _iscissors
{
  DrawCore       *core;         /*  Core select object               */

  SelectOps       op;

  gint            x, y;         /*  upper left hand coordinate       */
  gint            ix, iy;       /*  initial coordinates              */
  gint            nx, ny;       /*  new coordinates                  */

  TempBuf        *dp_buf;       /*  dynamic programming buffer              */

  ICurve         *curve1;       /*  1st curve connected to current point    */
  ICurve         *curve2;       /*  2nd curve connected to current point    */

  GSList         *curves;       /*  the list of curves                      */

  gboolean        first_point;  /*  is this the first point?                */
  gboolean        connected;    /*  is the region closed?                   */

  Iscissors_state state;        /*  state of iscissors               */
  Iscissors_draw  draw;         /*  items to draw on a draw request         */

  /* XXX might be useful */
  Channel        *mask;         /*  selection mask                   */
  TileManager    *gradient_map; /*  lazily filled gradient map */
};

typedef struct _IScissorsOptions IScissorsOptions;
struct _IScissorsOptions
{
  SelectionOptions  selection_options;
};


/**********************************************/
/*  Intelligent scissors selection apparatus  */


/*  Other defines...  */
#define  MAX_GRADIENT      179.606  /* == sqrt(127^2 + 127^2) */
#define  GRADIENT_SEARCH   32  /* how far to look when snapping to an edge */
#define  TARGET_HEIGHT     25
#define  TARGET_WIDTH      25
#define  POINT_WIDTH       8   /* size (in pixels) of seed handles */
#define  POINT_HALFWIDTH   (POINT_WIDTH / 2)
#define  EXTEND_BY         0.2 /* proportion to expand cost map by */
#define  FIXED             5   /* additional fixed size to expand cost map */
#define  MIN_GRADIENT      63  /* gradients < this are directionless */

#define  MAX_POINTS        2048

#ifdef USE_LAPLACIAN
# define COST_WIDTH        3  /* number of bytes for each pixel in cost map  */
#else
# define COST_WIDTH        2  /* number of bytes for each pixel in cost map  */
#endif
#define  BLOCK_WIDTH       64
#define  BLOCK_HEIGHT      64
#define  CONV_WIDTH        (BLOCK_WIDTH + 2)
#define  CONV_HEIGHT       (BLOCK_HEIGHT + 2)

/* weight to give between laplacian (_Z), gradient (_G) and direction (_D) */
#ifdef USE_LAPLACIAN
#  define  OMEGA_Z           0.16
#  define  OMEGA_D           0.42
#  define  OMEGA_G           0.42
#else
#  define  OMEGA_D           0.2
#  define  OMEGA_G           0.8
#endif

/* sentinel to mark seed point in ?cost? map */
#define  SEED_POINT        9

/*  Functional defines  */
#define  PIXEL_COST(x)     (x >> 8)
#define  PIXEL_DIR(x)      (x & 0x000000ff)


/*  static variables  */

/*  where to move on a given link direction  */
static int move [8][2] =
{
  { 1, 0 },
  { 0, 1 },
  { -1, 1 },
  { 1, 1 },
  { -1, 0 },
  { 0, -1 },
  { 1, -1 },
  { -1, -1 },
};

/* IE:
 * '---+---+---`
 * | 7 | 5 | 6 |
 * +---+---+---+
 * | 4 |   | 0 |
 * +---+---+---+
 * | 2 | 1 | 3 |
 * `---+---+---'
 */

/*  points for drawing curves  */
static GdkPoint    curve_points [MAX_POINTS];


/*  temporary convolution buffers --  */
D(static guint sent0 = 0xd0d0d0d0);
static guchar  maxgrad_conv0 [TILE_WIDTH * TILE_HEIGHT * 4] = "";
D(static guint sent1 = 0xd1d1d1d1);
static guchar  maxgrad_conv1 [TILE_WIDTH * TILE_HEIGHT * 4] = "";
D(static guint sent2 = 0xd2d2d2d2);
static guchar  maxgrad_conv2 [TILE_WIDTH * TILE_HEIGHT * 4] = "";
D(static guint sent3 = 0xd3d3d3d3);


static gint horz_deriv [9] =
{
  1, 0, -1,
  2, 0, -2,
  1, 0, -1,
};

static gint vert_deriv [9] =
{
  1, 2, 1,
  0, 0, 0,
  -1, -2, -1,
};


#ifdef USE_LAPLACIAN
static gint  laplacian [9] = 
{
  -1, -1, -1,
  -1, 8, -1,
  -1, -1, -1,
};
#endif

static gint blur_32 [9] = 
{
  1, 1, 1,
  1, 24, 1,
  1, 1, 1,
};

static gfloat    distance_weights [GRADIENT_SEARCH * GRADIENT_SEARCH];

static gint      diagonal_weight [256];
static gint      direction_value [256][4];
static gboolean  initialized = FALSE;
static Tile     *cur_tile = NULL;


/***********************************************************************/
/* static variables */

static IScissorsOptions *iscissors_options = NULL;


/***********************************************************************/
/*  Local function prototypes  */

static void   iscissors_button_press    (Tool *, GdkEventButton *, gpointer);
static void   iscissors_button_release  (Tool *, GdkEventButton *, gpointer);
static void   iscissors_motion          (Tool *, GdkEventMotion *, gpointer);
static void   iscissors_oper_update     (Tool *, GdkEventMotion *, gpointer);
static void   iscissors_modifier_update (Tool *, GdkEventKey *,    gpointer);
static void   iscissors_cursor_update   (Tool *, GdkEventMotion *, gpointer);
static void   iscissors_control         (Tool *, ToolAction, gpointer);

static void   iscissors_reset           (Iscissors *iscissors);
static void   iscissors_draw            (Tool      *tool);

static TileManager * gradient_map_new          (GImage      *gimage);

static void          find_optimal_path         (TileManager *gradient_map,
						TempBuf     *dp_buf,
						gint         x1,
						gint         y1,
						gint         x2,
						gint         y2,
						gint         xs,
						gint         ys);
static void          find_max_gradient         (Iscissors   *iscissors,
						GImage      *gimage,
						gint        *x,
						gint        *y);
static void          calculate_curve           (Tool        *tool,
						ICurve      *curve);
static void          iscissors_draw_curve      (GDisplay    *gdisp,
						Iscissors   *iscissors,
						ICurve      *curve);
static void          iscissors_free_icurves    (GSList      *list);
static void          iscissors_free_buffers    (Iscissors   *iscissors);

static gint          mouse_over_vertex         (Iscissors   *iscissors,
						gint         x,
						gint         y);
static gboolean      clicked_on_vertex         (Tool        *tool);
static GSList      * mouse_over_curve          (Iscissors   *iscissors,
						gint         x,
						gint         y);
static gboolean      clicked_on_curve          (Tool        *tool);

static void          precalculate_arrays       (void);
static GPtrArray   * plot_pixels               (Iscissors   *iscissors,
						TempBuf     *dp_buf,
						gint         x1,
						gint         y1,
						gint         xs,
						gint         ys,
						gint         xe,
						gint         ye);

static void
iscissors_options_reset (void)
{
  IScissorsOptions *options = iscissors_options;

  selection_options_reset ((SelectionOptions *) options);
}

static IScissorsOptions *
iscissors_options_new (void)
{
  IScissorsOptions *options;

  /*  the new intelligent scissors tool options structure  */
  options = g_new (IScissorsOptions, 1);
  selection_options_init ((SelectionOptions *) options,
			  ISCISSORS,
			  iscissors_options_reset);

  return options;
}

Tool *
tools_new_iscissors (void)
{
  Tool * tool;
  Iscissors * private;

  if (!iscissors_options)
    {
      iscissors_options = iscissors_options_new ();
      tools_register (ISCISSORS, (ToolOptions *) iscissors_options);
    }

  tool = tools_new_tool (ISCISSORS);
  private = g_new (Iscissors, 1);

  private->core = draw_core_new (iscissors_draw);
  private->op           = -1;
  private->curves       = NULL;
  private->dp_buf       = NULL;
  private->state        = NO_ACTION;
  private->mask         = NULL;
  private->gradient_map = NULL;

  tool->auto_snap_to = FALSE;   /*  Don't snap to guides   */

  tool->private = (void *) private;
  
  tool->button_press_func    = iscissors_button_press;
  tool->button_release_func  = iscissors_button_release;
  tool->motion_func          = iscissors_motion;
  tool->oper_update_func     = iscissors_oper_update;
  tool->modifier_key_func    = iscissors_modifier_update;
  tool->cursor_update_func   = iscissors_cursor_update;
  tool->control_func         = iscissors_control;

  iscissors_reset (private);

  return tool;
}

void
tools_free_iscissors (Tool *tool)
{
  Iscissors * iscissors;

  iscissors = (Iscissors *) tool->private;

  TRC (("tools_free_iscissors\n"));

  /*  XXX? Undraw curve  */
  /*iscissors->draw = DRAW_CURVE;*/

  if (tool->state == ACTIVE)
    draw_core_stop (iscissors->core, tool);
  draw_core_free (iscissors->core);

  iscissors_reset (iscissors);

  g_free (iscissors);
}


/*  Local functions  */

static void
iscissors_button_press (Tool           *tool,
			GdkEventButton *bevent,
			gpointer        gdisp_ptr)
{
  GDisplay     *gdisp;
  GimpDrawable *drawable;
  Iscissors    *iscissors;
  gboolean      grab_pointer = FALSE;

  gdisp = (GDisplay *) gdisp_ptr;
  iscissors = (Iscissors *) tool->private;
  drawable = gimage_active_drawable (gdisp->gimage);

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &iscissors->x, &iscissors->y, FALSE, FALSE);

  /*  If the tool was being used in another image...reset it  */

  if (tool->state == ACTIVE && gdisp_ptr != tool->gdisp_ptr)
    {
      /*iscissors->draw = DRAW_CURVE; XXX? */
      draw_core_stop (iscissors->core, tool);
      iscissors_reset (iscissors);
    }

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  switch (iscissors->state)
    {
    case NO_ACTION:
#if 0
      /* XXX what's this supposed to do? */
      if (!(bevent->state & GDK_SHIFT_MASK) &&
	  !(bevent->state & GDK_CONTROL_MASK))
	if (selection_point_inside (gdisp->select, gdisp_ptr,
				    bevent->x, bevent->y))
	  {
	    init_edit_selection (tool, gdisp->select, gdisp_ptr,
				 bevent->x, bevent->y);
	    return;
	  }
#endif

      iscissors->state = SEED_PLACEMENT;
      iscissors->draw = DRAW_CURRENT_SEED;
      grab_pointer = TRUE;

      if (! (bevent->state & GDK_SHIFT_MASK))
	find_max_gradient (iscissors, gdisp->gimage,
			   &iscissors->x, &iscissors->y);

      iscissors->x = CLAMP (iscissors->x, 0, gdisp->gimage->width - 1);
      iscissors->y = CLAMP (iscissors->y, 0, gdisp->gimage->height - 1);

      iscissors->ix = iscissors->x;
      iscissors->iy = iscissors->y;

      /*  Initialize the selection core only on starting the tool  */
      draw_core_start (iscissors->core, gdisp->canvas->window, tool);
      break;

    default:
      /*  Check if the mouse click occured on a vertex or the curve itself  */
      if (clicked_on_vertex (tool))
	{
	  iscissors->nx = iscissors->x;
	  iscissors->ny = iscissors->y;
	  iscissors->state = SEED_ADJUSTMENT;
	  iscissors->draw = DRAW_ACTIVE_CURVE;
	  draw_core_resume (iscissors->core, tool);
	  grab_pointer = TRUE;
	}
      /*  If the iscissors is connected, check if the click was inside  */
      else if (iscissors->connected && iscissors->mask &&
	       channel_value (iscissors->mask, iscissors->x, iscissors->y))
	{
	  /*  Undraw the curve  */
	  tool->state = INACTIVE;
	  iscissors->draw = DRAW_CURVE;
	  draw_core_stop (iscissors->core, tool);

	  if (iscissors->op == SELECTION_REPLACE)
	    gimage_mask_clear (gdisp->gimage);
	  else
	    gimage_mask_undo (gdisp->gimage);

	  if (((SelectionOptions *) iscissors_options)->feather)
	    channel_feather (iscissors->mask,
			     gimage_get_mask (gdisp->gimage),
			     ((SelectionOptions *) iscissors_options)->feather_radius,
			     ((SelectionOptions *) iscissors_options)->feather_radius,
			     iscissors->op, 0, 0);
	  else
	    channel_combine_mask (gimage_get_mask (gdisp->gimage),
				  iscissors->mask, iscissors->op, 0, 0);

	  iscissors_reset (iscissors);

	  gdisplays_flush ();
	}
      /*  if we're not connected, we're adding a new point  */
      else if (!iscissors->connected)
	{
	  iscissors->state = SEED_PLACEMENT;
	  iscissors->draw = DRAW_CURRENT_SEED;
	  grab_pointer = TRUE;
	  
	  draw_core_resume (iscissors->core, tool);
	}
      break;
    }

  if (grab_pointer)
    gdk_pointer_grab (gdisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK |
		      GDK_BUTTON1_MOTION_MASK |
		      GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, bevent->time);  
}


static void
iscissors_convert (Iscissors *iscissors,
		   gpointer   gdisp_ptr)
{
  GDisplay *gdisp = (GDisplay *) gdisp_ptr;
  ScanConverter    *sc;
  ScanConvertPoint *pts;
  guint   npts;
  GSList *list;
  ICurve *icurve;
  guint   packed;
  gint    i;
  gint    index;

  sc = scan_converter_new (gdisp->gimage->width, gdisp->gimage->height, 1);

  /* go over the curves in reverse order, adding the points we have */
  list = iscissors->curves;
  index = g_slist_length (list);
  while (index)
    {
      index--;

      icurve = (ICurve *) g_slist_nth_data (list, index);

      npts = icurve->points->len;
      pts = g_new (ScanConvertPoint, npts);

      for (i = 0; i < npts; i ++)
	{
	  packed = GPOINTER_TO_INT (g_ptr_array_index (icurve->points, i));
	  pts[i].x = packed & 0x0000ffff;
	  pts[i].y = packed >> 16;
	}

      scan_converter_add_points (sc, npts, pts);
      g_free (pts);
    }

  if (iscissors->mask)
    channel_delete (iscissors->mask);
  iscissors->mask = scan_converter_to_channel (sc, gdisp->gimage);
  scan_converter_free (sc);

  channel_invalidate_bounds (iscissors->mask);    
}


static void
iscissors_button_release (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
  Iscissors *iscissors;
  GDisplay  *gdisp;
  ICurve    *curve;

  gdisp = (GDisplay *) gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  TRC (("iscissors_button_release\n"));

  /* Make sure X didn't skip the button release event -- as it's known
   * to do */
  if (iscissors->state == WAITING)
    return;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();  /* XXX why the flush? */

  /*  Undraw everything  */
  switch (iscissors->state)
    {
    case SEED_PLACEMENT:
      iscissors->draw = DRAW_CURVE | DRAW_CURRENT_SEED;
      break;
    case SEED_ADJUSTMENT:
      iscissors->draw = DRAW_CURVE | DRAW_ACTIVE_CURVE;
      break;
    default:
      break;
    }

  draw_core_stop (iscissors->core, tool);

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      /*  Progress to the next stage of intelligent selection  */
      switch (iscissors->state)
	{
	case SEED_PLACEMENT:
	  /*  Add a new icurve  */
	  if (!iscissors->first_point)
	    {
	      /*  Determine if we're connecting to the first point  */
	      if (iscissors->curves)
		{
		  curve = (ICurve *) iscissors->curves->data;
		  if (abs (iscissors->x - curve->x1) < POINT_HALFWIDTH &&
		      abs (iscissors->y - curve->y1) < POINT_HALFWIDTH)
		    {
		      iscissors->x = curve->x1;
		      iscissors->y = curve->y1;
		      iscissors->connected = TRUE;
		    }
		}

	      /*  Create the new curve segment  */
	      if (iscissors->ix != iscissors->x ||
		  iscissors->iy != iscissors->y)
		{
		  curve = g_new (ICurve, 1);

		  curve->x1 = iscissors->ix;
		  curve->y1 = iscissors->iy;
		  iscissors->ix = curve->x2 = iscissors->x;
		  iscissors->iy = curve->y2 = iscissors->y;
		  curve->points = NULL;
		  TRC (("create new curve segment\n"));
		  iscissors->curves = g_slist_append (iscissors->curves,
						      (void *) curve);
		  TRC (("calculate curve\n"));
		  calculate_curve (tool, curve);
		}
	    }
	  else /* this was our first point */
	    {
	      iscissors->first_point = FALSE;
	    }
	  break;

	case SEED_ADJUSTMENT:
	  /*  recalculate both curves  */
	  if (iscissors->curve1)
	    {
	      iscissors->curve1->x1 = iscissors->nx;
	      iscissors->curve1->y1 = iscissors->ny;
	      calculate_curve (tool, iscissors->curve1);
	    }
	  if (iscissors->curve2)
	    {
	      iscissors->curve2->x2 = iscissors->nx;
	      iscissors->curve2->y2 = iscissors->ny;
	      calculate_curve (tool, iscissors->curve2);
	    }
	  break;

	default:
	  break;
	}
    }

  TRC (("button_release: draw core resume\n"));

  /*  Draw only the boundary  */
  iscissors->state = WAITING;
  iscissors->draw = DRAW_CURVE;
  draw_core_resume (iscissors->core, tool);

  /*  convert the curves into a region  */
  if (iscissors->connected)
    iscissors_convert (iscissors, gdisp_ptr);
}

static void
iscissors_motion (Tool           *tool,
		  GdkEventMotion *mevent,
		  gpointer        gdisp_ptr)
{
  Iscissors *iscissors;
  GDisplay *gdisp;
  
  gdisp = (GDisplay *) gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  if (tool->state != ACTIVE || iscissors->state == NO_ACTION)
    return;

  if (iscissors->state == SEED_PLACEMENT)
    iscissors->draw = DRAW_CURRENT_SEED;
  else if (iscissors->state == SEED_ADJUSTMENT)
    iscissors->draw = DRAW_ACTIVE_CURVE;

  draw_core_pause (iscissors->core, tool);

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, 
			       &iscissors->x, &iscissors->y, FALSE, FALSE);
  
  switch (iscissors->state)
    {
    case SEED_PLACEMENT:
      /*  Hold the shift key down to disable the auto-edge snap feature  */
      if (! (mevent->state & GDK_SHIFT_MASK))
	find_max_gradient (iscissors, gdisp->gimage,
			   &iscissors->x, &iscissors->y);

      iscissors->x = CLAMP (iscissors->x, 0, gdisp->gimage->width - 1);
      iscissors->y = CLAMP (iscissors->y, 0, gdisp->gimage->height - 1);

      if (iscissors->first_point)
	{
	  iscissors->ix = iscissors->x;
	  iscissors->iy = iscissors->y;
	}
      break;

    case SEED_ADJUSTMENT:
      /*  Move the current seed to the location of the cursor  */
      if (! (mevent->state & GDK_SHIFT_MASK))
	find_max_gradient (iscissors, gdisp->gimage,
			   &iscissors->x, &iscissors->y);

      iscissors->x = CLAMP (iscissors->x, 0, gdisp->gimage->width - 1);
      iscissors->y = CLAMP (iscissors->y, 0, gdisp->gimage->height - 1);

      iscissors->nx = iscissors->x;
      iscissors->ny = iscissors->y;
      break;

    default:
      break;
    }

  draw_core_resume (iscissors->core, tool);
}


static void
iscissors_draw (Tool *tool)
{
  GDisplay *gdisp;
  Iscissors *iscissors;
  ICurve *curve;
  GSList *list;
  int tx1, ty1, tx2, ty2;
  int txn, tyn;

  TRC (("iscissors_draw\n"));

  gdisp = (GDisplay *) tool->gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  gdisplay_transform_coords (gdisp, iscissors->ix, iscissors->iy, &tx1, &ty1,
			     FALSE);

  /*  Draw the crosshairs target if we're placing a seed  */
  if (iscissors->draw & DRAW_CURRENT_SEED)
    {
      gdisplay_transform_coords (gdisp, iscissors->x, iscissors->y, &tx2, &ty2,
				 FALSE);

      gdk_draw_line (iscissors->core->win, iscissors->core->gc, 
		     tx2 - (TARGET_WIDTH >> 1), ty2,
		     tx2 + (TARGET_WIDTH >> 1), ty2);
      gdk_draw_line (iscissors->core->win, iscissors->core->gc, 
		     tx2, ty2 - (TARGET_HEIGHT >> 1),
		     tx2, ty2 + (TARGET_HEIGHT >> 1));

      if (!iscissors->first_point)
	gdk_draw_line (iscissors->core->win, iscissors->core->gc, 
		       tx1, ty1, tx2, ty2);
    }

  if ((iscissors->draw & DRAW_CURVE) && !iscissors->first_point)
    {
      /*  Draw a point at the init point coordinates  */
      if (!iscissors->connected)
	gdk_draw_arc (iscissors->core->win, iscissors->core->gc, 1,
		      tx1 - POINT_HALFWIDTH, ty1 - POINT_HALFWIDTH, 
		      POINT_WIDTH, POINT_WIDTH, 0, 23040);

      /*  Go through the list of icurves, and render each one...  */
      list = iscissors->curves;
      while (list)
	{
	  curve = (ICurve *) list->data;

	  /*  plot the curve  */
	  iscissors_draw_curve (gdisp, iscissors, curve);

	  gdisplay_transform_coords (gdisp, curve->x1, curve->y1, &tx1, &ty1,
				     FALSE);

	  gdk_draw_arc (iscissors->core->win, iscissors->core->gc, 1,
			tx1 - POINT_HALFWIDTH, ty1 - POINT_HALFWIDTH, 
			POINT_WIDTH, POINT_WIDTH, 0, 23040);

	  list = g_slist_next (list);
	}
    }

  if (iscissors->draw & DRAW_ACTIVE_CURVE)
    {
      gdisplay_transform_coords (gdisp, iscissors->nx, iscissors->ny,
				 &txn, &tyn, FALSE);

      /*  plot both curves, and the control point between them  */
      if (iscissors->curve1)
	{
	  gdisplay_transform_coords (gdisp, iscissors->curve1->x2, 
				     iscissors->curve1->y2, &tx1, &ty1, FALSE);

	  gdk_draw_line (iscissors->core->win, iscissors->core->gc, 
			 tx1, ty1, txn, tyn);
	}
      if (iscissors->curve2)
	{
	  gdisplay_transform_coords (gdisp, iscissors->curve2->x1, 
				     iscissors->curve2->y1, &tx2, &ty2, FALSE);

	  gdk_draw_line (iscissors->core->win, iscissors->core->gc, 
			 tx2, ty2, txn, tyn);
	}

      gdk_draw_arc (iscissors->core->win, iscissors->core->gc, 1,
		    txn - POINT_HALFWIDTH, tyn - POINT_HALFWIDTH, 
		    POINT_WIDTH, POINT_WIDTH, 0, 23040);
    }
}


static void
iscissors_draw_curve (GDisplay  *gdisp,
		      Iscissors *iscissors,
		      ICurve    *curve)
{
  gpointer *point;
  guint     len;
  gint      tx, ty;
  gint      npts;
  guint32   coords;

  /* Uh, this shouldn't happen, but it does.  So we ignore it.
   * Quality code, baby. */
  if (!curve->points)
      return;

  npts = 0;
  point = curve->points->pdata;
  len = curve->points->len;
  while (len--)
    {
      coords = GPOINTER_TO_INT (*point);
      point++;
      gdisplay_transform_coords (gdisp, (coords & 0x0000ffff), 
				 (coords >> 16), &tx, &ty, FALSE);
      if (npts < MAX_POINTS)
	{
	  curve_points [npts].x = tx;
	  curve_points [npts].y = ty;
	  npts ++;
	}
      else
	{
	  g_warning ("too many points in ICurve segment!");
	  return;
	}
    }

  /*  draw the curve */
  gdk_draw_lines (iscissors->core->win, iscissors->core->gc,
		  curve_points, npts);
}

static void
iscissors_oper_update (Tool           *tool,
		       GdkEventMotion *mevent,
		       gpointer        gdisp_ptr)
{
  Iscissors *iscissors;
  GDisplay  *gdisp;
  gint       x, y;

  iscissors = (Iscissors *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &x, &y, FALSE, FALSE);

  if (mouse_over_vertex (iscissors, x, y))
    {
      iscissors->op = SELECTION_MOVE_MASK; /* abused */
    }
  else if (mouse_over_curve (iscissors, x, y))
    {
      iscissors->op = SELECTION_MOVE; /* abused */
    }
  else if (iscissors->connected && iscissors->mask &&
	   channel_value (iscissors->mask, x, y))
    {
      if (mevent->state & GDK_SHIFT_MASK &&
	  mevent->state & GDK_CONTROL_MASK)
	{
	  iscissors->op = SELECTION_INTERSECT;
	}
      else if (mevent->state & GDK_SHIFT_MASK)
	{
	  iscissors->op = SELECTION_ADD;
	}
      else if (mevent->state & GDK_CONTROL_MASK)
	{
	  iscissors->op = SELECTION_SUB;
	}
      else
	{
	  iscissors->op = SELECTION_REPLACE;
	}
    }
  else if (iscissors->connected && iscissors->mask)
    {
      iscissors->op = -1;
    }
  else
    {
      iscissors->op = -2;
    }
}

static void
iscissors_modifier_update (Tool        *tool,
			   GdkEventKey *kevent,
			   gpointer     gdisp_ptr)
{
  Iscissors *iscissors;
  SelectOps  op;

  iscissors = (Iscissors *) tool->private;

  op = iscissors->op;

  if (op == -2)
    return;

  switch (kevent->keyval)
    {
    case GDK_Shift_L: case GDK_Shift_R:
      if (op == SELECTION_REPLACE)
	op = SELECTION_ADD;
      else if (op == SELECTION_ADD)
	op = SELECTION_REPLACE;
      else if (op == SELECTION_SUB)
	op = SELECTION_INTERSECT;
      else if (op == SELECTION_INTERSECT)
	op = SELECTION_SUB;
      break;

    case GDK_Control_L: case GDK_Control_R:
      if (op == SELECTION_REPLACE)
	op = SELECTION_SUB;
      else if (op == SELECTION_ADD)
	op = SELECTION_INTERSECT;
      else if (op == SELECTION_SUB)
	op = SELECTION_REPLACE;
      else if (op == SELECTION_INTERSECT)
	op = SELECTION_ADD;
      break;
    }

  iscissors->op = op;
}

static void
iscissors_cursor_update (Tool           *tool,
			 GdkEventMotion *mevent,
			 gpointer        gdisp_ptr)
{
  Iscissors *iscissors;
  GDisplay *gdisp;

  iscissors = (Iscissors *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;

  switch (iscissors->op)
    {
    case SELECTION_REPLACE:
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
				    RECT_SELECT,
				    CURSOR_MODIFIER_NONE,
				    FALSE);
      break;
    case SELECTION_ADD:
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
				    RECT_SELECT,
				    CURSOR_MODIFIER_PLUS,
				    FALSE);
      break;
    case SELECTION_SUB:
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
				    RECT_SELECT,
				    CURSOR_MODIFIER_MINUS,
				    FALSE);
      break;
    case SELECTION_INTERSECT:
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
				    RECT_SELECT,
				    CURSOR_MODIFIER_INTERSECT,
				    FALSE);
      break;
    case SELECTION_MOVE_MASK: /* abused */
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
				    ISCISSORS,
				    CURSOR_MODIFIER_MOVE,
				    FALSE);
      break;
    case SELECTION_MOVE: /* abused */
      gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
				    ISCISSORS,
				    CURSOR_MODIFIER_PLUS,
				    FALSE);
      break;
    case -1:
      gdisplay_install_tool_cursor (gdisp, GIMP_BAD_CURSOR,
				    ISCISSORS,
				    CURSOR_MODIFIER_NONE,
				    FALSE);
      break;
    default:
      switch (iscissors->state)
	{
	case WAITING:
	  gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					ISCISSORS,
					CURSOR_MODIFIER_PLUS,
					FALSE);
	  break;
	case SEED_PLACEMENT:
	case SEED_ADJUSTMENT:
	default:
	  gdisplay_install_tool_cursor (gdisp, GIMP_MOUSE_CURSOR,
					ISCISSORS,
					CURSOR_MODIFIER_NONE,
					FALSE);
	  break;
	}
    }
}

static void
iscissors_control (Tool       *tool,
		   ToolAction  action,
		   gpointer    gdisp_ptr)
{
  Iscissors * iscissors;
  Iscissors_draw draw;

  iscissors = (Iscissors *) tool->private;

  switch (iscissors->state)
    {
    case SEED_PLACEMENT:
      draw = DRAW_CURVE | DRAW_CURRENT_SEED;
      break;

    case SEED_ADJUSTMENT:
      draw = DRAW_CURVE | DRAW_ACTIVE_CURVE;
      break;

    default:
      draw = DRAW_CURVE;
      break;
    }

  iscissors->draw = draw;
  switch (action)
    {
    case PAUSE: 
      draw_core_pause (iscissors->core, tool);
      break;

    case RESUME:
      draw_core_resume (iscissors->core, tool);
      break;

    case HALT:
      draw_core_stop (iscissors->core, tool);
      iscissors_reset (iscissors);
      break;

    default:
      break;
    }
}


static void
iscissors_reset (Iscissors *iscissors)
{
  TRC( ("iscissors_reset\n"));

  /*  Free and reset the curve list  */
  if (iscissors->curves)
    {
      iscissors_free_icurves (iscissors->curves);
      TRC (("g_slist_free (iscissors->curves);\n"));
      g_slist_free (iscissors->curves);
      iscissors->curves = NULL;
    }

  /*  free mask  */
  if (iscissors->mask)
    channel_delete (iscissors->mask);
  iscissors->mask = NULL;

  /* free the gradient map */
  if (iscissors->gradient_map)
    {
      /* release any tile we were using */
      if (cur_tile)
      {
	TRC (("tile_release\n"));
	tile_release (cur_tile, FALSE);
      }
      cur_tile = NULL;

      TRC (("tile_manager_destroy (iscissors->gradient_map);\n"));
      tile_manager_destroy (iscissors->gradient_map);
      iscissors->gradient_map = NULL;
    }

  iscissors->curve1 = NULL;
  iscissors->curve2 = NULL;
  iscissors->first_point = TRUE;
  iscissors->connected = FALSE;
  iscissors->state = NO_ACTION;

  /*  Reset the dp buffers  */
  iscissors_free_buffers (iscissors);

  /*  If they haven't already been initialized, precalculate the diagonal
   *  weight and direction value arrays
   */
  if (!initialized)
    {
      precalculate_arrays ();
      initialized = TRUE;
    }
}


static void
iscissors_free_icurves (GSList *list)
{
  ICurve * curve;

  TRC (("iscissors_free_icurves\n"));

  while (list)
    {
      curve = (ICurve *) list->data;
      if (curve->points)
	{
	  TRC (("g_ptr_array_free (curve->points);\n"));
	  g_ptr_array_free (curve->points, TRUE);
	}
      TRC (("g_free (curve);\n"));
      g_free (curve);
      list = g_slist_next (list);
    }
}


static void
iscissors_free_buffers (Iscissors *iscissors)
{
  if (iscissors->dp_buf)
    temp_buf_free (iscissors->dp_buf);

  iscissors->dp_buf = NULL;
}


/* XXX need some scan-conversion routines from somewhere.  maybe. ? */

static gint
mouse_over_vertex (Iscissors *iscissors,
		   gint       x,
		   gint       y)
{
  GSList *list;
  ICurve *curve;
  gint curves_found = 0;

  /*  traverse through the list, returning non-zero if the current cursor
   *  position is on an existing curve vertex.  Set the curve1 and curve2
   *  variables to the two curves containing the vertex in question
   */

  iscissors->curve1 = iscissors->curve2 = NULL;

  list = iscissors->curves;

  while (list && curves_found < 2)
    {
      curve = (ICurve *) list->data;

      if (abs (curve->x1 - x) < POINT_HALFWIDTH &&
	  abs (curve->y1 - y) < POINT_HALFWIDTH)
	{
	  iscissors->curve1 = curve;
	  if (curves_found++)
	    return curves_found;
	}
      else if (abs (curve->x2 - x) < POINT_HALFWIDTH &&
	       abs (curve->y2 - y) < POINT_HALFWIDTH)
	{
	  iscissors->curve2 = curve;
	  if (curves_found++)
	    return curves_found;
	}

      list = g_slist_next (list);
    }

  return curves_found;
}

static gboolean
clicked_on_vertex (Tool *tool)
{
  Iscissors *iscissors;
  gint curves_found = 0;

  iscissors = (Iscissors *) tool->private;

  curves_found = mouse_over_vertex (iscissors, iscissors->x, iscissors->y);

  if (curves_found > 1)
    return TRUE;

  /*  if only one curve was found, the curves are unconnected, and
   *  the user only wants to move either the first or last point
   *  disallow this for now.
   */
  if (curves_found == 1)
    return FALSE;

  /*  no vertices were found at the cursor click point.  Now check whether
   *  the click occured on a curve.  If so, create a new vertex there and
   *  two curve segments to replace what used to be just one...
   */
  return clicked_on_curve (tool);
}


static GSList *
mouse_over_curve (Iscissors *iscissors,
		  gint       x,
		  gint       y)
{
  GSList   *list;
  gpointer *pt;
  gint      len;
  ICurve   *curve;
  guint32   coords;
  gint      tx, ty;

  /*  traverse through the list, returning the curve segment's list element
   *  if the current cursor position is on a curve... 
   */

  for (list = iscissors->curves; list; list = g_slist_next (list))
    {
      curve = (ICurve *) list->data;

      pt = curve->points->pdata;
      len = curve->points->len;
      while (len--)
	{
	  coords = GPOINTER_TO_INT (*pt);
	  pt++;
	  tx = coords & 0x0000ffff;
	  ty = coords >> 16;

	  /*  Is the specified point close enough to the curve?  */
	  if (abs (tx - x) < POINT_HALFWIDTH &&
	      abs (ty - y) < POINT_HALFWIDTH)
	    {
	      return list;
	    }
	}
    }

  return NULL;
}

static gboolean
clicked_on_curve (Tool *tool)
{
  Iscissors *iscissors;
  GSList    *list, *new_link;
  ICurve    *curve, *new_curve;

  iscissors = (Iscissors *) tool->private;

  /*  traverse through the list, getting back the curve segment's list
   *  element if the current cursor position is on a curve...
   *  If this occurs, replace the curve with two new curves,
   *  separated by a new vertex.
   */
  list = mouse_over_curve (iscissors, iscissors->x, iscissors->y);

  if (list)
    {
      curve = (ICurve *) list->data;

      /*  Since we're modifying the curve, undraw the existing one  */
      iscissors->draw = DRAW_CURVE;
      draw_core_pause (iscissors->core, tool);

      /*  Create the new curve  */
      new_curve = g_new (ICurve, 1);

      new_curve->x2 = curve->x2;
      new_curve->y2 = curve->y2;
      new_curve->x1 = curve->x2 = iscissors->x;
      new_curve->y1 = curve->y2 = iscissors->y;
      new_curve->points = NULL;

      /*  Create the new link and supply the new curve as data  */
      new_link = g_slist_alloc ();
      new_link->data = (void *) new_curve;

      /*  Insert the new link in the list  */
      new_link->next = list->next;
      list->next = new_link;

      iscissors->curve1 = new_curve;
      iscissors->curve2 = curve;

      /*  Redraw the curve  */
      draw_core_resume (iscissors->core, tool);

      return TRUE;
    }

  return FALSE;
}


static void
precalculate_arrays (void)
{
  gint i;

  for (i = 0; i < 256; i++)
    {
      /*  The diagonal weight array  */
      diagonal_weight [i] = (int) (i * G_SQRT2);

      /*  The direction value array  */
      direction_value [i][0] = (127 - abs (127 - i)) * 2;
      direction_value [i][1] = abs (127 - i) * 2;
      direction_value [i][2] = abs (191 - i) * 2;
      direction_value [i][3] = abs (63 - i) * 2;

      TRC (("i: %d, v0: %d, v1: %d, v2: %d, v3: %d\n", i,
	    direction_value [i][0],
	    direction_value [i][1],
	    direction_value [i][2],
	    direction_value [i][3]));
    }

  /*  set the 256th index of the direction_values to the hightest cost  */
  direction_value [255][0] = 255;
  direction_value [255][1] = 255;
  direction_value [255][2] = 255;
  direction_value [255][3] = 255;
}


static void
calculate_curve (Tool   *tool,
		 ICurve *curve)
{
  GDisplay  *gdisp;
  Iscissors *iscissors;
  gint x, y, dir;
  gint xs, ys, xe, ye;
  gint x1, y1, x2, y2;
  gint width, height;
  gint ewidth, eheight;

  TRC (("calculate_curve(%p, %p)\n", tool, curve));

  /*  Calculate the lowest cost path from one vertex to the next as specified
   *  by the parameter "curve".
   *    Here are the steps:
   *      1)  Calculate the appropriate working area for this operation
   *      2)  Allocate a temp buf for the dynamic programming array
   *      3)  Run the dynamic programming algorithm to find the optimal path
   *      4)  Translate the optimal path into pixels in the icurve data
   *            structure.
   */

  gdisp = (GDisplay *) tool->gdisp_ptr;
  iscissors = (Iscissors *) tool->private;

  /*  Get the bounding box  */
  xs = CLAMP (curve->x1, 0, gdisp->gimage->width - 1);
  ys = CLAMP (curve->y1, 0, gdisp->gimage->height - 1);
  xe = CLAMP (curve->x2, 0, gdisp->gimage->width - 1);
  ye = CLAMP (curve->y2, 0, gdisp->gimage->height - 1);
  x1 = MIN (xs, xe);
  y1 = MIN (ys, ye);
  x2 = MAX (xs, xe) + 1;  /*  +1 because if xe = 199 & xs = 0, x2 - x1, width = 200  */
  y2 = MAX (ys, ye) + 1;

  /*  expand the boundaries past the ending points by 
   *  some percentage of width and height.  This serves the following purpose:
   *  It gives the algorithm more area to search so better solutions
   *  are found.  This is particularly helpful in finding "bumps" which
   *  fall outside the bounding box represented by the start and end
   *  coordinates of the "curve".
   */
  ewidth = (x2 - x1) * EXTEND_BY + FIXED;
  eheight = (y2 - y1) * EXTEND_BY + FIXED;

  if (xe >= xs)
    x2 += CLAMP (ewidth, 0, gdisp->gimage->width - x2);
  else
    x1 -= CLAMP (ewidth, 0, x1);
  if (ye >= ys)
    y2 += CLAMP (eheight, 0, gdisp->gimage->height - y2);
  else
    y1 -= CLAMP (eheight, 0, y1);

  /* blow away any previous points list we might have */
  if (curve->points)
    {
      TRC (("1229: g_ptr_array_free (curve->points);\n"));
      g_ptr_array_free (curve->points, TRUE);
      curve->points = NULL;
    }

  /*  If the bounding box has width and height...  */
  if ((x2 - x1) && (y2 - y1))
    {
      width = (x2 - x1);
      height = (y2 - y1);

      /* Initialise the gradient map tile manager for this image if we
       * don't already have one. */
      if (!iscissors->gradient_map)
	  iscissors->gradient_map = gradient_map_new (gdisp->gimage);

      TRC (("dp buf resize\n"));
      /*  allocate the dynamic programming array  */
      iscissors->dp_buf = 
	temp_buf_resize (iscissors->dp_buf, 4, x1, y1, width, height);

      TRC (("find_optimal_path\n"));
      /*  find the optimal path of pixels from (x1, y1) to (x2, y2)  */
      find_optimal_path (iscissors->gradient_map, iscissors->dp_buf,
			 x1, y1, x2, y2, xs, ys);
      
      /*  get a list of the pixels in the optimal path  */
      TRC (("plot_pixels\n"));
      curve->points = plot_pixels (iscissors, iscissors->dp_buf,
				   x1, y1, xs, ys, xe, ye);
    }
  /*  If the bounding box has no width  */
  else if ((x2 - x1) == 0)
    {
      /*  plot a vertical line  */
      y = ys;  
      dir = (ys > ye) ? -1 : 1;
      curve->points = g_ptr_array_new ();
      while (y != ye)
	{
	  g_ptr_array_add (curve->points, GINT_TO_POINTER ((y << 16) + xs));
	  y += dir;
	}
    }
  /*  If the bounding box has no height  */
  else if ((y2 - y1) == 0)
    {
      /*  plot a horizontal line  */
      x = xs;
      dir = (xs > xe) ? -1 : 1;
      curve->points = g_ptr_array_new ();
      while (x != xe)
	{
	  g_ptr_array_add (curve->points, GINT_TO_POINTER ((ys << 16) + x));
	  x += dir;
	}
    }
}


/* badly need to get a replacement - this is _way_ too expensive */
static gboolean
gradient_map_value (TileManager *map,
		    gint         x,
		    gint         y,
		    guint8      *grad,
		    guint8      *dir)
{
  static int cur_tilex;
  static int cur_tiley;
  guint8 *p;

  if (!cur_tile ||
      x / TILE_WIDTH != cur_tilex ||
      y / TILE_HEIGHT != cur_tiley)
    {
      if (cur_tile)
	tile_release (cur_tile, FALSE);
      cur_tile = tile_manager_get_tile (map, x, y, TRUE, FALSE);
      if (!cur_tile)
	return FALSE;
      cur_tilex = x / TILE_WIDTH;
      cur_tiley = y / TILE_HEIGHT;	
    }

  p = tile_data_pointer (cur_tile, x % TILE_WIDTH, y % TILE_HEIGHT);
  *grad = p[0];
  *dir  = p[1];

  return TRUE;
}

static gint
calculate_link (TileManager *gradient_map,
		gint         x,
		gint         y,
		guint32      pixel,
		gint         link)
{
  gint value = 0;
  guint8 grad1, dir1, grad2, dir2;

  if (!gradient_map_value (gradient_map, x, y, &grad1, &dir1))
    {
      grad1 = 0;
      dir1 = 255;
    }

  /* Convert the gradient into a cost: large gradients are good, and
   * so have low cost. */
  grad1 = 255 - grad1;

  /*  calculate the contribution of the gradient magnitude  */
  if (link > 1)
    value += diagonal_weight [grad1] * OMEGA_G;
  else
    value += grad1 * OMEGA_G;

  /*  calculate the contribution of the gradient direction  */
  x += (gint8)(pixel & 0xff);
  y += (gint8)((pixel & 0xff00) >> 8);
  if (!gradient_map_value (gradient_map, x, y, &grad2, &dir2))
    {
      grad2 = 0;
      dir2 = 255;
    }  
  value += (direction_value [dir1][link] + direction_value [dir2][link]) *
    OMEGA_D;

  return value;
}


static GPtrArray *
plot_pixels (Iscissors *iscissors,
	     TempBuf   *dp_buf,
	     gint       x1,
	     gint       y1,
	     gint       xs,
	     gint       ys,
	     gint       xe,
	     gint       ye)
{
  gint       x, y;
  guint32    coords;
  gint       link;
  gint       width;
  guint     *data;
  GPtrArray *list;

  width = dp_buf->width;

  /*  Start the data pointer at the correct location  */
  data = (guint *) temp_buf_data (dp_buf) + (ye - y1) * width + (xe - x1);

  x = xe;
  y = ye;

  list = g_ptr_array_new ();

  while (1)
    {
      coords = (y << 16) + x;
      g_ptr_array_add (list, GINT_TO_POINTER (coords));

      link = PIXEL_DIR (*data);
      if (link == SEED_POINT)
	return list;

      x += move [link][0];
      y += move [link][1];
      data += move [link][1] * width + move [link][0];
    }

  /*  won't get here  */
  return NULL;
}


#define PACK(x, y) ((((y) & 0xff) << 8) | ((x) & 0xff))
#define OFFSET(pixel) ((gint8)((pixel) & 0xff) + \
  ((gint8)(((pixel) & 0xff00) >> 8)) * dp_buf->width)


static void
find_optimal_path (TileManager *gradient_map,
		   TempBuf     *dp_buf,
		   gint         x1,
		   gint         y1,
		   gint         x2,
		   gint         y2,
		   gint         xs,
		   gint         ys)
{
  gint i, j, k;
  gint x, y;
  gint link;
  gint linkdir;
  gint dirx, diry;
  gint min_cost;
  gint new_cost;
  gint offset;
  gint cum_cost [8];
  gint link_cost [8];
  gint pixel_cost [8];
  guint32 pixel [8];
  guint32 * data, *d;

  TRC (("find_optimal_path (%p, %p, [%d,%d-%d,%d] %d, %d)\n",
	gradient_map, dp_buf, x1, y1, x2, y2, xs, ys));

  /*  initialize the dynamic programming buffer  */
  data = (guint32 *) temp_buf_data (dp_buf);
  for (i = 0; i < dp_buf->height; i++)
    for (j = 0; j < dp_buf->width; j++)
      *data++ = 0;  /*  0 cumulative cost, 0 direction  */

  /*  what directions are we filling the array in according to?  */
  dirx = (xs - x1 == 0) ? 1 : -1;
  diry = (ys - y1 == 0) ? 1 : -1;
  linkdir = (dirx * diry);

  y = ys;

  /*  Start the data pointer at the correct location  */
  data = (guint32 *) temp_buf_data (dp_buf);

  TRC (("find_optimal_path: mainloop\n"));

  for (i = 0; i < dp_buf->height; i++)
    {
      x = xs;

      d = data + (y-y1) * dp_buf->width + (x-x1);

      for (j = 0; j < dp_buf->width; j++)
	{
	  min_cost = G_MAXINT;

	  /* pixel[] array encodes how to get to a neigbour, if possible.
	   * 0 means no connection (eg edge).
	   * Rest packed as bottom two bytes: y offset then x offset.
	   * Initially, we assume we can't get anywhere. */
	  for (k = 0; k < 8; k++)
	    pixel [k] = 0;

	  /*  Find the valid neighboring pixels  */
	  /*  the previous pixel  */
	  if (j)
	    pixel [((dirx == 1) ? 4 : 0)] = PACK (-dirx, 0);

	  /*  the previous row of pixels  */
	  if (i)
	    {
	      pixel [((diry == 1) ? 5 : 1)] = PACK (0, -diry);

	      link = (linkdir == 1) ? 3 : 2;
	      if (j)
		pixel [((diry == 1) ? (link + 4) : link)] = PACK(-dirx, -diry);

	      link = (linkdir == 1) ? 2 : 3;
	      if (j != dp_buf->width - 1)
		pixel [((diry == 1) ? (link + 4) : link)] = PACK (dirx, -diry);
	    }

	  /*  find the minimum cost of going through each neighbor to reach the
	   *  seed point...
	   */
	  link = -1;
	  for (k = 0; k < 8; k ++)
	    if (pixel [k])
	      {
		link_cost [k] = calculate_link (gradient_map,
						xs + j*dirx, ys + i*diry,
						pixel [k],
						((k > 3) ? k - 4 : k));
		offset = OFFSET (pixel [k]);
		pixel_cost [k] = PIXEL_COST (d [offset]);
		cum_cost [k] = pixel_cost [k] + link_cost [k];
		if (cum_cost [k] < min_cost)
		  {
		    min_cost = cum_cost [k];
		    link = k;
		  }
	      }

	  /*  If anything can be done...  */
	  if (link >= 0)
	    {
	      /*  set the cumulative cost of this pixel and the new direction  */
	      *d = (cum_cost [link] << 8) + link;

	      /*  possibly change the links from the other pixels to this pixel...
	       *  these changes occur if a neighboring pixel will receive a lower
	       *  cumulative cost by going through this pixel.  
	       */
	      for (k = 0; k < 8; k ++)
		if (pixel [k] && k != link)
		  {
		    /*  if the cumulative cost at the neighbor is greater than
		     *  the cost through the link to the current pixel, change the
		     *  neighbor's link to point to the current pixel.
		     */
		    new_cost = link_cost [k] + cum_cost [link];
		    if (pixel_cost [k] > new_cost)
		    {
		      /*  reverse the link direction   /-----------------------\ */
		      offset = OFFSET (pixel [k]);
		      d [offset] = (new_cost << 8) + ((k > 3) ? k - 4 : k + 4);
		    }
		  }
	    } 
	  /*  Set the seed point  */
	  else if (!i && !j)
	    *d = SEED_POINT;

	  /*  increment the data pointer and the x counter  */
	  d += dirx;
	  x += dirx;
	}

      /*  increment the y counter  */
      y += diry;
    }

  TRC (("done: find_optimal_path\n"));
}


/* Called to fill in a newly referenced tile in the gradient map */
static void
gradmap_tile_validate (TileManager *tm,
		       Tile        *tile)
{
  static gboolean first_gradient = TRUE;
  gint    x, y;
  gint    dw, dh;
  gint    sw, sh;
  gint    i, j;
  gint    b;
  gfloat  gradient;
  guint8 *gradmap;
  guint8 *tiledata;
  guint8 *datah, *datav;
  gint8   hmax, vmax;
  Tile   *srctile;
  PixelRegion srcPR, destPR;
  GImage *gimage;

  gimage = (GImage *) tile_manager_get_user_data (tm);

  if (first_gradient)
    {
      gint radius = GRADIENT_SEARCH >> 1;
      /*  compute the distance weights  */
      for (i = 0; i < GRADIENT_SEARCH; i++)
	for (j = 0; j < GRADIENT_SEARCH; j++)
	  distance_weights [i * GRADIENT_SEARCH + j] =
	    1.0 / (1 + sqrt (SQR(i - radius) + SQR(j - radius)));
      first_gradient = FALSE;
    }

  tile_manager_get_tile_coordinates (tm, tile, &x, &y);
  dw = tile_ewidth (tile);
  dh = tile_eheight (tile);

  TRC (("fill req for tile %p @ (%d, %d)\n", tile, x, y));

  /* get corresponding tile in the gimage */
  srctile = tile_manager_get_tile (gimp_image_composite (gimage),
				   x, y, TRUE, FALSE);
  if (!srctile)
    {
      g_warning ("bad tile coords?");
      return;
    }
  sw = tile_ewidth (srctile);
  sh = tile_eheight (srctile);

  if (dw != sw || dh != sh)
    g_warning ("dw:%d sw:%d  dh:%d sh:%d\n", dw, sw, dh, sh);

  srcPR.w = MIN (dw, sw);
  srcPR.h = MIN (dh, sh);
  srcPR.bytes = gimp_image_composite_bytes (gimage);
  srcPR.data = tile_data_pointer (srctile, 0, 0);
  srcPR.rowstride = srcPR.w * srcPR.bytes;

  /* XXX tile edges? */

  /*  Blur the source to get rid of noise  */
  destPR.rowstride = TILE_WIDTH * 4;
  destPR.data = maxgrad_conv0;
  convolve_region (&srcPR, &destPR, blur_32, 3, 32, NORMAL_CONVOL);

  /*  Set the "src" temp buf up as the new source Pixel Region  */
  srcPR.rowstride = destPR.rowstride;
  srcPR.data = destPR.data;

  /*  Get the horizontal derivative  */
  destPR.data = maxgrad_conv1;
  convolve_region (&srcPR, &destPR, horz_deriv, 3, 1, NEGATIVE_CONVOL);

  /*  Get the vertical derivative  */
  destPR.data = maxgrad_conv2;
  convolve_region (&srcPR, &destPR, vert_deriv, 3, 1, NEGATIVE_CONVOL);

  /* calculate overall gradient */
  tiledata = tile_data_pointer (tile, 0, 0);
  for (i = 0; i < srcPR.h; i++)
    {
      datah = maxgrad_conv1 + srcPR.rowstride*i;
      datav = maxgrad_conv2 + srcPR.rowstride*i;
      gradmap = tiledata + tile_ewidth (tile) * COST_WIDTH * i;

      for (j = 0; j < srcPR.w; j++)
	{
	  hmax = datah[0] - 128;
	  vmax = datav[0] - 128;
	  for (b = 1; b < srcPR.bytes; b++)
	    {
	      if (abs (datah[b] - 128) > abs (hmax)) hmax = datah[b] - 128;
	      if (abs (datav[b] - 128) > abs (vmax)) vmax = datav[b] - 128;
	    }

	  if (i == 0 || j == 0 || i == srcPR.h-1 || j == srcPR.w-1)
	  {
	      gradmap[j*COST_WIDTH] = 0;
	      gradmap[j*COST_WIDTH + 1] = 255;
	      goto contin;
	  }

	  /* 1 byte absolute magitude first */
	  gradient = sqrt(SQR(hmax) + SQR(vmax));
	  gradmap[j*COST_WIDTH] = gradient * 255 / MAX_GRADIENT;

	  /* then 1 byte direction */
	  if (gradient > MIN_GRADIENT)
	    {
	      gfloat direction;
	      if (!hmax)
		direction = (vmax > 0) ? G_PI_2 : -G_PI_2;
	      else
		direction = atan ((double) vmax / (double) hmax);
	      /* Scale the direction from between 0 and 254,
	       *  corresponding to -PI/2, PI/2 255 is reserved for
	       *  directionless pixels */
	      gradmap[j*COST_WIDTH + 1] =
		(guint8) (254 * (direction + G_PI_2) / G_PI);
	    }
	  else
	    gradmap[j*COST_WIDTH + 1] = 255; /* reserved for weak gradient */

	contin:
	  {
#ifdef DEBUG
	    int g = gradmap[j*COST_WIDTH];
	    int d = gradmap[j*COST_WIDTH + 1];
	    TRC (("%c%c", 'a' + (g * 25 / 255), '0' + (d / 25)));
#endif /* DEBUG */
	  }

	  datah += srcPR.bytes;
	  datav += srcPR.bytes;
	}
      TRC (("\n"));
    }
  TRC (("\n"));

  tile_release (srctile, FALSE);
}

static TileManager *
gradient_map_new (GImage *gimage)
{
  TileManager *tm;

  tm = tile_manager_new (gimage->width, gimage->height,
			 sizeof (guint8) * COST_WIDTH);
  tile_manager_set_user_data (tm, gimage);
  tile_manager_set_validate_proc (tm, gradmap_tile_validate);

  return tm;
}

static void
find_max_gradient (Iscissors *iscissors,
		   GImage    *gimage,
		   gint      *x,
		   gint      *y)
{
  PixelRegion srcPR;
  gint    radius;
  gint    i, j;
  gint    endx, endy;
  gint    sx, sy, cx, cy;
  gint    x1, y1, x2, y2;
  void   *pr;
  guint8 *gradient;
  gfloat  g, max_gradient;

  TRC (("find_max_gradient(%d, %d)\n", *x, *y));

  /* Initialise the gradient map tile manager for this image if we
   * don't already have one. */
  if (!iscissors->gradient_map)
    iscissors->gradient_map = gradient_map_new (gimage);

  radius = GRADIENT_SEARCH >> 1;

  /*  calculate the extent of the search  */
  cx = CLAMP (*x, 0, gimage->width);
  cy = CLAMP (*y, 0, gimage->height);
  sx = cx - radius;
  sy = cy - radius;
  x1 = CLAMP (cx - radius, 0, gimage->width);
  y1 = CLAMP (cy - radius, 0, gimage->height);
  x2 = CLAMP (cx + radius, 0, gimage->width);
  y2 = CLAMP (cy + radius, 0, gimage->height);
  /*  calculate the factor to multiply the distance from the cursor by  */

  max_gradient = 0;
  *x = cx;
  *y = cy;

  /*  Find the point of max gradient  */
  pixel_region_init (&srcPR, iscissors->gradient_map,
		     x1, y1, x2 - x1, y2 - y1, FALSE);

  /* this iterates over 1, 2 or 4 tiles only */
  for (pr = pixel_regions_register (1, &srcPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      endx = srcPR.x + srcPR.w;
      endy = srcPR.y + srcPR.h;
      for (i = srcPR.y; i < endy; i++)
	{
	  gradient = srcPR.data + srcPR.rowstride * (i - srcPR.y);
	  for (j = srcPR.x; j < endx; j++)
	    {
	      g = *gradient;
	      gradient += COST_WIDTH;
	      g *= distance_weights [(i-y1) * GRADIENT_SEARCH + (j-x1)];
	      if (g > max_gradient)
		{
		  max_gradient = g;
		  *x = j;
		  *y = i;
		}
	    }
	}
    }

  TRC (("done: find_max_gradient(%d, %d)\n", *x, *y));
}

/* End of iscissors.c */
