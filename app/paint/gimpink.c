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
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpinkoptions.h"
#include "gimpinktool.h"
#include "gimpinktool-blob.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define SUBSAMPLE 8


/*  local function prototypes  */

static void        gimp_ink_tool_class_init      (GimpInkToolClass *klass);
static void        gimp_ink_tool_init            (GimpInkTool      *tool);

static void        gimp_ink_tool_finalize        (GObject          *object);

static void        gimp_ink_tool_control         (GimpTool         *tool,
                                                  GimpToolAction    action,
                                                  GimpDisplay      *gdisp);
static void        gimp_ink_tool_button_press    (GimpTool         *tool,
                                                  GimpCoords       *coords,
                                                  guint32           time,
                                                  GdkModifierType   state,
                                                  GimpDisplay      *gdisp);
static void        gimp_ink_tool_button_release  (GimpTool         *tool,
                                                  GimpCoords       *coords,
                                                  guint32           time,
                                                  GdkModifierType   state,
                                                  GimpDisplay      *gdisp);
static void        gimp_ink_tool_motion          (GimpTool         *tool,
                                                  GimpCoords       *coords,
                                                  guint32           time,
                                                  GdkModifierType   state,
                                                  GimpDisplay      *gdisp);
static void        gimp_ink_tool_cursor_update   (GimpTool         *tool,
                                                  GimpCoords       *coords,
                                                  GdkModifierType   state,
                                                  GimpDisplay      *gdisp);

static Blob *      ink_pen_ellipse      (GimpInkOptions  *options,
                                         gdouble          x_center,
                                         gdouble          y_center,
                                         gdouble          pressure,
                                         gdouble          xtilt,
                                         gdouble          ytilt,
                                         gdouble          velocity);

static void        time_smoother_add    (GimpInkTool     *ink_tool,
					 guint32          value);
static gdouble     time_smoother_result (GimpInkTool     *ink_tool);
static void        time_smoother_init   (GimpInkTool     *ink_tool,
					 guint32          initval);
static void        dist_smoother_add    (GimpInkTool     *ink_tool,
					 gdouble          value);
static gdouble     dist_smoother_result (GimpInkTool     *ink_tool);
static void        dist_smoother_init   (GimpInkTool     *ink_tool,
					 gdouble          initval);

static void        ink_init             (GimpInkTool     *ink_tool,
					 GimpDrawable    *drawable,
					 gdouble          x,
					 gdouble          y);
static void        ink_finish           (GimpInkTool     *ink_tool,
					 GimpDrawable    *drawable);
static void        ink_cleanup          (void);

/*  Rendering functions  */
static void        ink_set_paint_area   (GimpInkTool     *ink_tool,
					 GimpDrawable    *drawable,
					 Blob            *blob);
static void        ink_paste            (GimpInkTool     *ink_tool,
					 GimpDrawable    *drawable,
					 Blob            *blob);

static void        ink_to_canvas_tiles  (GimpInkTool     *ink_tool,
					 Blob            *blob,
					 guchar          *color);

static void        ink_set_undo_tiles   (GimpDrawable    *drawable,
					 gint             x,
					 gint             y,
					 gint             w,
					 gint             h);
static void        ink_set_canvas_tiles (gint             x,
					 gint             y,
					 gint             w,
					 gint             h);


/* local variables */

/*  undo blocks variables  */
static TileManager *undo_tiles = NULL;

/* Tiles used to render the stroke at 1 byte/pp */
static TileManager *canvas_tiles = NULL;

/* Flat buffer that is used to used to render the dirty region
 * for composition onto the destination drawable
 */
static TempBuf *canvas_buf = NULL;

static GimpToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_ink_tool_register (GimpToolRegisterCallback  callback,
                        gpointer                  data)
{
  (* callback) (GIMP_TYPE_INK_TOOL,
                GIMP_TYPE_INK_OPTIONS,
                gimp_ink_options_gui,
                GIMP_CONTEXT_OPACITY_MASK |
                GIMP_CONTEXT_PAINT_MODE_MASK,
                "gimp-ink-tool",
                _("Ink"),
                _("Draw in ink"),
                N_("/Tools/Paint Tools/In_k"), "K",
                NULL, GIMP_HELP_TOOL_INK,
                GIMP_STOCK_TOOL_INK,
                data);
}

GType
gimp_ink_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpInkToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_ink_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpInkTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_ink_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TOOL,
					  "GimpInkTool",
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_ink_tool_class_init (GimpInkToolClass *klass)
{
  GObjectClass   *object_class;
  GimpToolClass  *tool_class;

  object_class = G_OBJECT_CLASS (klass);
  tool_class   = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_ink_tool_finalize;

  tool_class->control        = gimp_ink_tool_control;
  tool_class->button_press   = gimp_ink_tool_button_press;
  tool_class->button_release = gimp_ink_tool_button_release;
  tool_class->motion         = gimp_ink_tool_motion;
  tool_class->cursor_update  = gimp_ink_tool_cursor_update;
}

static void
gimp_ink_tool_init (GimpInkTool *ink_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (ink_tool);

  gimp_tool_control_set_motion_mode (tool->control, GIMP_MOTION_MODE_EXACT);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_INK_TOOL_CURSOR);
}

static void
gimp_ink_tool_finalize (GObject *object)
{
  GimpInkTool *ink_tool;

  ink_tool = GIMP_INK_TOOL (object);

  if (ink_tool->last_blob)
    {
      g_free (ink_tool->last_blob);
      ink_tool->last_blob = NULL;
    }

  ink_cleanup ();

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_ink_tool_control (GimpTool       *tool,
                       GimpToolAction  action,
                       GimpDisplay    *gdisp)
{
  GimpInkTool *ink_tool;

  ink_tool = GIMP_INK_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      ink_cleanup ();
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_ink_tool_button_press (GimpTool        *tool,
                            GimpCoords      *coords,
                            guint32          time,
                            GdkModifierType  state,
                            GimpDisplay     *gdisp)
{
  GimpInkTool    *ink_tool;
  GimpInkOptions *options;
  GimpDrawable   *drawable;
  GimpCoords      curr_coords;
  gint            off_x, off_y;
  Blob           *b;

  ink_tool = GIMP_INK_TOOL (tool);
  options  = GIMP_INK_OPTIONS (tool->tool_info->tool_options);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  curr_coords = *coords;

  gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

  curr_coords.x -= off_x;
  curr_coords.y -= off_y;

  ink_init (ink_tool, drawable, curr_coords.x, curr_coords.y);

  gimp_tool_control_activate (tool->control);
  tool->gdisp = gdisp;

  /*  pause the current selection  */
  gimp_image_selection_control (gdisp->gimage, GIMP_SELECTION_PAUSE);

  b = ink_pen_ellipse (options,
                       curr_coords.x,
                       curr_coords.y,
		       curr_coords.pressure,
                       curr_coords.xtilt,
                       curr_coords.ytilt,
		       10.0);

  ink_paste (ink_tool, drawable, b);
  ink_tool->last_blob = b;

  time_smoother_init (ink_tool, time);
  ink_tool->last_time = time;

  dist_smoother_init (ink_tool, 0.0);
  ink_tool->init_velocity = TRUE;

  ink_tool->lastx = curr_coords.x;
  ink_tool->lasty = curr_coords.y;

  gimp_display_flush_now (gdisp);
}

static void
gimp_ink_tool_button_release (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  GimpInkTool *ink_tool;
  GimpImage   *gimage;

  ink_tool = GIMP_INK_TOOL (tool);

  gimage = gdisp->gimage;

  /*  resume the current selection  */
  gimp_image_selection_control (gdisp->gimage, GIMP_SELECTION_RESUME);

  /*  Set tool state to inactive -- no longer painting */
  gimp_tool_control_halt (tool->control);

  /*  free the last blob  */
  g_free (ink_tool->last_blob);
  ink_tool->last_blob = NULL;

  ink_finish (ink_tool, gimp_image_active_drawable (gdisp->gimage));
  gimp_image_flush (gdisp->gimage);
}

static void
gimp_ink_tool_motion (GimpTool        *tool,
                      GimpCoords      *coords,
                      guint32          time,
                      GdkModifierType  state,
                      GimpDisplay     *gdisp)
{
  GimpInkTool    *ink_tool;
  GimpInkOptions *options;
  GimpDrawable   *drawable;
  GimpCoords      curr_coords;
  gint            off_x, off_y;
  Blob           *b;
  Blob           *blob_union;
  gdouble         velocity;
  gdouble         dist;
  gdouble         lasttime, thistime;

  ink_tool = GIMP_INK_TOOL (tool);
  options  = GIMP_INK_OPTIONS (tool->tool_info->tool_options);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  curr_coords = *coords;

  gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

  curr_coords.x -= off_x;
  curr_coords.y -= off_y;

  lasttime = ink_tool->last_time;

  time_smoother_add (ink_tool, time);
  thistime = ink_tool->last_time = time_smoother_result (ink_tool);

  /* The time resolution on X-based GDK motion events is
     bloody awful, hence the use of the smoothing function.
     Sadly this also means that there is always the chance of
     having an indeterminite velocity since this event and
     the previous several may still appear to issue at the same
     instant. -ADM */

  if (thistime == lasttime)
    thistime = lasttime + 1;

  if (ink_tool->init_velocity)
    {
      dist_smoother_init (ink_tool,
			  dist = sqrt ((ink_tool->lastx - curr_coords.x) *
                                       (ink_tool->lastx - curr_coords.x) +
				       (ink_tool->lasty - curr_coords.y) *
                                       (ink_tool->lasty - curr_coords.y)));
      ink_tool->init_velocity = FALSE;
    }
  else
    {
      dist_smoother_add (ink_tool,
			 sqrt ((ink_tool->lastx - curr_coords.x) *
                               (ink_tool->lastx - curr_coords.x) +
			       (ink_tool->lasty - curr_coords.y) *
                               (ink_tool->lasty - curr_coords.y)));

      dist = dist_smoother_result (ink_tool);
    }

  ink_tool->lastx = curr_coords.x;
  ink_tool->lasty = curr_coords.y;

  velocity = 10.0 * sqrt ((dist) / (gdouble) (thistime - lasttime));

  b = ink_pen_ellipse (options,
                       curr_coords.x,
                       curr_coords.y,
                       curr_coords.pressure,
                       curr_coords.xtilt,
		       curr_coords.ytilt,
                       velocity);

  blob_union = blob_convex_union (ink_tool->last_blob, b);
  g_free (ink_tool->last_blob);
  ink_tool->last_blob = b;

  ink_paste (ink_tool, drawable, blob_union);
  g_free (blob_union);

  gimp_display_flush_now (gdisp);
}

static void
gimp_ink_tool_cursor_update (GimpTool        *tool,
                             GimpCoords      *coords,
                             GdkModifierType  state,
                             GimpDisplay     *gdisp)
{
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;

  if (gimp_display_coords_in_active_drawable (gdisp, coords))
    {
      GimpChannel *selection = gimp_image_get_mask (gdisp->gimage);

      /*  One more test--is there a selected region?
       *  if so, is cursor inside?
       */
      if (gimp_channel_is_empty (selection))
        ctype = GIMP_MOUSE_CURSOR;
      else if (gimp_channel_value (selection, coords->x, coords->y))
        ctype = GIMP_MOUSE_CURSOR;
    }

  gimp_tool_control_set_cursor (tool->control, ctype);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}


static Blob *
ink_pen_ellipse (GimpInkOptions *options,
                 gdouble         x_center,
		 gdouble         y_center,
		 gdouble         pressure,
		 gdouble         xtilt,
		 gdouble         ytilt,
		 gdouble         velocity)
{
  BlobFunc function = blob_ellipse;
  gdouble  size;
  gdouble  tsin, tcos;
  gdouble  aspect, radmin;
  gdouble  x,y;
  gdouble  tscale;
  gdouble  tscale_c;
  gdouble  tscale_s;

  /* Adjust the size depending on pressure. */

  size = options->size * (1.0 + options->size_sensitivity *
                          (2.0 * pressure - 1.0) );

  /* Adjust the size further depending on pointer velocity
     and velocity-sensitivity.  These 'magic constants' are
     'feels natural' tigert-approved. --ADM */

  if (velocity < 3.0)
    velocity = 3.0;

#ifdef VERBOSE
  g_printerr ("%g (%g) -> ", size, velocity);
#endif

  size = (options->vel_sensitivity *
          ((4.5 * size) / (1.0 + options->vel_sensitivity * (2.0 * velocity)))
          + (1.0 - options->vel_sensitivity) * size);

#ifdef VERBOSE
  g_printerr ("%g\n", (gfloat) size);
#endif

  /* Clamp resulting size to sane limits */

  if (size > options->size * (1.0 + options->size_sensitivity))
    size = options->size * (1.0 + options->size_sensitivity);

  if (size * SUBSAMPLE < 1.0)
    size = 1.0 / SUBSAMPLE;

  /* Add brush angle/aspect to tilt vectorially */

  /* I'm not happy with the way the brush widget info is combined with
     tilt info from the brush. My personal feeling is that representing
     both as affine transforms would make the most sense. -RLL */

  tscale = options->tilt_sensitivity * 10.0;
  tscale_c = tscale * cos (gimp_deg_to_rad (options->tilt_angle));
  tscale_s = tscale * sin (gimp_deg_to_rad (options->tilt_angle));

  x = (options->blob_aspect * cos (options->blob_angle) +
       xtilt * tscale_c - ytilt * tscale_s);
  y = (options->blob_aspect * sin (options->blob_angle) +
       ytilt * tscale_c + xtilt * tscale_s);

#ifdef VERBOSE
  g_printerr ("angle %g aspect %g; %g %g; %g %g\n",
              options->blob_angle, options->blob_aspect,
              tscale_c, tscale_s, x, y);
#endif

  aspect = sqrt (x*x + y*y);

  if (aspect != 0)
    {
      tcos = x / aspect;
      tsin = y / aspect;
    }
  else
    {
      tsin = sin (options->blob_angle);
      tcos = cos (options->blob_angle);
    }

  aspect = CLAMP (aspect, 1.0, 10.0);

  radmin = MAX (1.0, SUBSAMPLE * size / aspect);

  switch (options->blob_type)
    {
    case GIMP_INK_BLOB_TYPE_ELLIPSE:
      function = blob_ellipse;
      break;

    case GIMP_INK_BLOB_TYPE_SQUARE:
      function = blob_square;
      break;

    case GIMP_INK_BLOB_TYPE_DIAMOND:
      function = blob_diamond;
      break;
    }

  return (* function) (x_center * SUBSAMPLE,
                       y_center * SUBSAMPLE,
                       radmin * aspect * tcos,
                       radmin * aspect * tsin,
                       -radmin * tsin,
                       radmin * tcos);
}

static void
dist_smoother_init (GimpInkTool *ink_tool,
		    gdouble      initval)
{
  gint i;

  ink_tool->dt_index = 0;

  for (i = 0; i < DIST_SMOOTHER_BUFFER; i++)
    {
      ink_tool->dt_buffer[i] = initval;
    }
}

static gdouble
dist_smoother_result (GimpInkTool *ink_tool)
{
  gint    i;
  gdouble result = 0.0;

  for (i = 0; i < DIST_SMOOTHER_BUFFER; i++)
    {
      result += ink_tool->dt_buffer[i];
    }

  return (result / (gdouble) DIST_SMOOTHER_BUFFER);
}

static void
dist_smoother_add (GimpInkTool *ink_tool,
		   gdouble      value)
{
  ink_tool->dt_buffer[ink_tool->dt_index] = value;

  if ((++ink_tool->dt_index) == DIST_SMOOTHER_BUFFER)
    ink_tool->dt_index = 0;
}


static void
time_smoother_init (GimpInkTool *ink_tool,
		    guint32      initval)
{
  gint i;

  ink_tool->ts_index = 0;

  for (i = 0; i < TIME_SMOOTHER_BUFFER; i++)
    {
      ink_tool->ts_buffer[i] = initval;
    }
}

static gdouble
time_smoother_result (GimpInkTool *ink_tool)
{
  gint    i;
  guint64 result = 0;

  for (i = 0; i < TIME_SMOOTHER_BUFFER; i++)
    {
      result += ink_tool->ts_buffer[i];
    }

#ifdef _MSC_VER
  return (gdouble) (gint64) (result / TIME_SMOOTHER_BUFFER);
#else
  return (result / TIME_SMOOTHER_BUFFER);
#endif
}

static void
time_smoother_add (GimpInkTool *ink_tool,
		   guint32      value)
{
  ink_tool->ts_buffer[ink_tool->ts_index] = value;

  if ((++ink_tool->ts_index) == TIME_SMOOTHER_BUFFER)
    ink_tool->ts_index = 0;
}

static void
ink_init (GimpInkTool  *ink_tool,
	  GimpDrawable *drawable,
	  gdouble       x,
	  gdouble       y)
{
  GimpItem *item = GIMP_ITEM (drawable);

  /*  Allocate the undo structure  */
  if (undo_tiles)
    tile_manager_unref (undo_tiles);

  undo_tiles = tile_manager_new (gimp_item_width  (item),
				 gimp_item_height (item),
				 gimp_drawable_bytes (drawable));

  /*  Allocate the canvas blocks structure  */
  if (canvas_tiles)
    tile_manager_unref (canvas_tiles);

  canvas_tiles = tile_manager_new (gimp_item_width  (item),
				   gimp_item_height (item), 1);

  /*  Get the initial undo extents  */
  ink_tool->x1 = ink_tool->x2 = x;
  ink_tool->y1 = ink_tool->y2 = y;
}

static void
ink_finish (GimpInkTool  *ink_tool,
	    GimpDrawable *drawable)
{
  GimpImage *gimage;

  gimp_drawable_push_undo (drawable, _("Ink"),
                           ink_tool->x1, ink_tool->y1,
                           ink_tool->x2, ink_tool->y2,
                           undo_tiles, TRUE);

  tile_manager_unref (undo_tiles);
  undo_tiles = NULL;

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  invalidate the previews -- have to do it here, because
   *  it is not done during the actual painting.
   */
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
}

static void
ink_cleanup (void)
{
  /*  If the undo tiles exist, nuke them  */
  if (undo_tiles)
    {
      tile_manager_unref (undo_tiles);
      undo_tiles = NULL;
    }

  /*  If the canvas blocks exist, nuke them  */
  if (canvas_tiles)
    {
      tile_manager_unref (canvas_tiles);
      canvas_tiles = NULL;
    }

  /*  Free the temporary buffer if it exist  */
  if (canvas_buf)
    {
      temp_buf_free (canvas_buf);
      canvas_buf = NULL;
    }
}

/*********************************
 *  Rendering functions          *
 *********************************/

/* Some of this stuff should probably be combined with the
 * code it was copied from in paint_core.c; but I wanted
 * to learn this stuff, so I've kept it simple.
 *
 * The following only supports CONSTANT mode. Incremental
 * would, I think, interact strangely with the way we
 * do things. But it wouldn't be hard to implement at all.
 */

static void
ink_set_paint_area (GimpInkTool  *ink_tool,
		    GimpDrawable *drawable,
		    Blob         *blob)
{
  GimpItem *item = GIMP_ITEM (drawable);
  gint      x, y, width, height;
  gint      x1, y1, x2, y2;
  gint      bytes;

  blob_bounds (blob, &x, &y, &width, &height);

  bytes = gimp_drawable_bytes_with_alpha (drawable);

  x1 = CLAMP (x / SUBSAMPLE - 1,            0, gimp_item_width  (item));
  y1 = CLAMP (y / SUBSAMPLE - 1,            0, gimp_item_height (item));
  x2 = CLAMP ((x + width)  / SUBSAMPLE + 2, 0, gimp_item_width  (item));
  y2 = CLAMP ((y + height) / SUBSAMPLE + 2, 0, gimp_item_height (item));

  /*  configure the canvas buffer  */
  if ((x2 - x1) && (y2 - y1))
    canvas_buf = temp_buf_resize (canvas_buf, bytes, x1, y1,
				  (x2 - x1), (y2 - y1));
}

enum { ROW_START, ROW_STOP };

/* The insertion sort here, for SUBSAMPLE = 8, tends to beat out
 * qsort() by 4x with CFLAGS=-O2, 2x with CFLAGS=-g
 */
static void
insert_sort (gint *data,
	     gint  n)
{
  gint i, j, k;
  gint tmp1, tmp2;

  for (i=2; i<2*n; i+=2)
    {
      tmp1 = data[i];
      tmp2 = data[i+1];
      j = 0;
      while (data[j] < tmp1)
	j += 2;

      for (k=i; k>j; k-=2)
	{
	  data[k] = data[k-2];
	  data[k+1] = data[k-1];
	}

      data[j] = tmp1;
      data[j+1] = tmp2;
    }
}

static void
fill_run (guchar *dest,
	  guchar  alpha,
	  gint    w)
{
  if (alpha == 255)
    {
      memset (dest, 255, w);
    }
  else
    {
      while (w--)
	{
	  *dest = MAX(*dest, alpha);
	  dest++;
	}
    }
}

static void
render_blob_line (Blob   *blob,
		  guchar *dest,
		  gint    x,
		  gint    y,
		  gint    width)
{
  gint  buf[4 * SUBSAMPLE];
  gint *data    = buf;
  gint  n       = 0;
  gint  i, j;
  gint  current = 0;  /* number of filled rows at this point
		       * in the scan line
		       */
  gint last_x;

  /* Sort start and ends for all lines */

  j = y * SUBSAMPLE - blob->y;
  for (i = 0; i < SUBSAMPLE; i++)
    {
      if (j >= blob->height)
	break;

      if ((j > 0) && (blob->data[j].left <= blob->data[j].right))
	{
	  data[2 * n]                     = blob->data[j].left;
	  data[2 * n + 1]                 = ROW_START;
	  data[2 * SUBSAMPLE + 2 * n]     = blob->data[j].right;
	  data[2 * SUBSAMPLE + 2 * n + 1] = ROW_STOP;
	  n++;
	}
      j++;
    }

  /*   If we have less than SUBSAMPLE rows, compress */
  if (n < SUBSAMPLE)
    {
      for (i = 0; i < 2 * n; i++)
	data[2 * n + i] = data[2 * SUBSAMPLE + i];
    }

  /*   Now count start and end separately */
  n *= 2;

  insert_sort (data, n);

  /* Discard portions outside of tile */

  while ((n > 0) && (data[0] < SUBSAMPLE*x))
    {
      if (data[1] == ROW_START)
	current++;
      else
	current--;
      data += 2;
      n--;
    }

  while ((n > 0) && (data[2*(n-1)] >= SUBSAMPLE*(x+width)))
    n--;

  /* Render the row */

  last_x = 0;
  for (i = 0; i < n;)
    {
      gint cur_x = data[2 * i] / SUBSAMPLE - x;
      gint pixel;

      /* Fill in portion leading up to this pixel */
      if (current && cur_x != last_x)
	fill_run (dest + last_x, (255 * current) / SUBSAMPLE, cur_x - last_x);

      /* Compute the value for this pixel */
      pixel = current * SUBSAMPLE;

      while (i<n)
	{
	  gint tmp_x = data[2 * i] / SUBSAMPLE;

	  if (tmp_x - x != cur_x)
	    break;

	  if (data[2 * i + 1] == ROW_START)
	    {
	      current++;
	      pixel += ((tmp_x + 1) * SUBSAMPLE) - data[2 * i];
	    }
	  else
	    {
	      current--;
	      pixel -= ((tmp_x + 1) * SUBSAMPLE) - data[2 * i];
	    }

	  i++;
	}

      dest[cur_x] = MAX (dest[cur_x], (pixel * 255) / (SUBSAMPLE * SUBSAMPLE));

      last_x = cur_x + 1;
    }

  if (current != 0)
    fill_run (dest + last_x, (255 * current)/ SUBSAMPLE, width - last_x);
}

static void
render_blob (PixelRegion *dest,
	     Blob        *blob)
{
  gint      i;
  gint      h;
  guchar   *s;
  gpointer  pr;

  for (pr = pixel_regions_register (1, dest);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      h = dest->h;
      s = dest->data;

      for (i=0; i<h; i++)
	{
	  render_blob_line (blob, s,
			    dest->x, dest->y + i, dest->w);
	  s += dest->rowstride;
	}
    }
}

static void
ink_paste (GimpInkTool  *ink_tool,
	   GimpDrawable *drawable,
	   Blob         *blob)
{
  GimpImage   *gimage;
  GimpContext *context;
  PixelRegion  srcPR;
  gint         offx, offy;
  guchar       col[MAX_CHANNELS];

  if (! (gimage = gimp_item_get_image (GIMP_ITEM (drawable))))
    return;

  context = GIMP_CONTEXT (GIMP_TOOL (ink_tool)->tool_info->tool_options);

  /* Get the the buffer */
  ink_set_paint_area (ink_tool, drawable, blob);

  /* check to make sure there is actually a canvas to draw on */
  if (!canvas_buf)
    return;

  gimp_image_get_foreground (gimage, drawable, context, col);

  /*  set the alpha channel  */
  col[canvas_buf->bytes - 1] = OPAQUE_OPACITY;

  /*  color the pixels  */
  color_pixels (temp_buf_data (canvas_buf), col,
		canvas_buf->width * canvas_buf->height, canvas_buf->bytes);

  /*  set undo blocks  */
  ink_set_undo_tiles (drawable,
		      canvas_buf->x, canvas_buf->y,
		      canvas_buf->width, canvas_buf->height);

  /*  initialize any invalid canvas tiles  */
  ink_set_canvas_tiles (canvas_buf->x, canvas_buf->y,
			canvas_buf->width, canvas_buf->height);

  ink_to_canvas_tiles (ink_tool, blob, col);

  /*  initialize canvas buf source pixel regions  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  /*  apply the paint area to the gimage  */
  gimp_drawable_apply_region (drawable, &srcPR,
                              FALSE, NULL,
                              gimp_context_get_opacity (context),
                              gimp_context_get_paint_mode (context),
                              undo_tiles,  /*  specify an alternative src1  */
                              canvas_buf->x, canvas_buf->y);

  /*  Update the undo extents  */
  ink_tool->x1 = MIN (ink_tool->x1, canvas_buf->x);
  ink_tool->y1 = MIN (ink_tool->y1, canvas_buf->y);
  ink_tool->x2 = MAX (ink_tool->x2, (canvas_buf->x + canvas_buf->width));
  ink_tool->y2 = MAX (ink_tool->y2, (canvas_buf->y + canvas_buf->height));

  /*  Update the gimage -- it is important to call gimp_image_update()
   *  instead of gimp_drawable_update() because we don't want the
   *  drawable and image previews to be constantly invalidated
   */
  gimp_item_offsets (GIMP_ITEM (drawable), &offx, &offy);
  gimp_image_update (gimage,
                     canvas_buf->x + offx,
                     canvas_buf->y + offy,
                     canvas_buf->width,
                     canvas_buf->height);
}

/* This routine a) updates the representation of the stroke
 * in the canvas tiles, then renders the dirty bit of it
 * into canvas_buf.
 */
static void
ink_to_canvas_tiles (GimpInkTool *ink_tool,
		     Blob        *blob,
		     guchar      *color)
{
  PixelRegion srcPR, maskPR;

  /*  draw the blob on the canvas tiles  */
  pixel_region_init (&srcPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, TRUE);

  render_blob (&srcPR, blob);

  /*  combine the canvas tiles and the canvas buf  */
  srcPR.bytes = canvas_buf->bytes;
  srcPR.x = 0; srcPR.y = 0;
  srcPR.w = canvas_buf->width;
  srcPR.h = canvas_buf->height;
  srcPR.rowstride = canvas_buf->width * canvas_buf->bytes;
  srcPR.data = temp_buf_data (canvas_buf);

  pixel_region_init (&maskPR, canvas_tiles,
		     canvas_buf->x, canvas_buf->y,
		     canvas_buf->width, canvas_buf->height, FALSE);

  /*  apply the canvas tiles to the canvas buf  */
  apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
}

static void
ink_set_undo_tiles (GimpDrawable *drawable,
		    gint          x,
		    gint          y,
		    gint          w,
		    gint          h)
{
  gint  i, j;
  Tile *src_tile;
  Tile *dest_tile;

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
	{
	  dest_tile = tile_manager_get_tile (undo_tiles, j, i, FALSE, FALSE);

	  if (! tile_is_valid (dest_tile))
	    {
	      src_tile = tile_manager_get_tile (gimp_drawable_data (drawable),
						j, i, TRUE, FALSE);
	      tile_manager_map_tile (undo_tiles, j, i, src_tile);
	      tile_release (src_tile, FALSE);
	    }
	}
    }
}


static void
ink_set_canvas_tiles (gint x,
		      gint y,
		      gint w,
		      gint h)
{
  Tile *tile;
  gint  i, j;

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    {
      for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
	{
	  tile = tile_manager_get_tile (canvas_tiles, j, i, FALSE, FALSE);

	  if (! tile_is_valid (tile))
	    {
	      tile = tile_manager_get_tile (canvas_tiles, j, i, TRUE, TRUE);
	      memset (tile_data_pointer (tile, 0, 0),
		      0,
		      tile_size (tile));
	      tile_release (tile, TRUE);
	    }
	}
    }
}
