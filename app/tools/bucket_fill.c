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
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "brush_select.h"
#include "bucket_fill.h"
#include "cursorutil.h"
#include "drawable.h"
#include "fuzzy_select.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "interface.h"
#include "paint_funcs.h"
#include "paint_options.h"
#include "palette.h"
#include "patterns.h"
#include "selection.h"
#include "tool_options_ui.h"
#include "tools.h"
#include "undo.h"

#include "libgimp/gimpintl.h"

/*  the bucket fill structures  */

typedef struct _BucketTool BucketTool;
struct _BucketTool
{
  int     target_x;   /*  starting x coord          */
  int     target_y;   /*  starting y coord          */
};

typedef struct _BucketOptions BucketOptions;
struct _BucketOptions
{
  PaintOptions    paint_options;

  double          threshold;
  double          threshold_d;
  GtkObject      *threshold_w;

  int             sample_merged;
  int             sample_merged_d;
  GtkWidget      *sample_merged_w;

  BucketFillMode  fill_mode;
  BucketFillMode  fill_mode_d;
  GtkWidget      *fill_mode_w[3];
};


/*  the bucket fill tool options  */
static BucketOptions *bucket_options = NULL;


/*  local function prototypes  */
static void  bucket_fill_button_press    (Tool *, GdkEventButton *, gpointer);
static void  bucket_fill_button_release  (Tool *, GdkEventButton *, gpointer);
static void  bucket_fill_motion          (Tool *, GdkEventMotion *, gpointer);
static void  bucket_fill_cursor_update   (Tool *, GdkEventMotion *, gpointer);
static void  bucket_fill_control         (Tool *, int, gpointer);

static void  bucket_fill_region          (BucketFillMode, PixelRegion *, PixelRegion *,
					  unsigned char *, TempBuf *,
					  int, int, int);
static void  bucket_fill_line_color      (unsigned char *, unsigned char *,
					  unsigned char *, int, int, int);
static void  bucket_fill_line_pattern    (unsigned char *, unsigned char *,
					  TempBuf *, int, int, int, int, int);


/*  functions  */

static void
bucket_options_reset (void)
{
  BucketOptions *options = bucket_options;

  paint_options_reset ((PaintOptions *) options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
			    options->threshold_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->fill_mode_w[options->fill_mode_d]), TRUE);
}

static BucketOptions *
bucket_options_new (void)
{
  BucketOptions *options;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *frame;
  gchar* fill_mode_label[3] = { _("FG Color Fill"), 
				_("BG Color Fill"),
				_("Pattern Fill") };
  gint   fill_mode_value[3] = { FG_BUCKET_FILL, 
				BG_BUCKET_FILL, 
				PATTERN_BUCKET_FILL };

  /*  the new bucket fill tool options structure  */
  options = (BucketOptions *) g_malloc (sizeof (BucketOptions));
  paint_options_init ((PaintOptions *) options,
		      BUCKET_FILL,
		      bucket_options_reset);
  options->sample_merged = options->sample_merged_d = FALSE;
  options->threshold     = options->threshold_d     = 15.0;
  options->fill_mode     = options->fill_mode_d     = FG_BUCKET_FILL;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the threshold scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Threshold:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->threshold_w =
    gtk_adjustment_new (options->threshold_d, 1.0, 255.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->threshold_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->threshold_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->threshold);
  gtk_widget_show (scale);

  gtk_widget_show (hbox);

  /*  the sample merged toggle  */
  options->sample_merged_w =
    gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_signal_connect (GTK_OBJECT (options->sample_merged_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->sample_merged);
  gtk_box_pack_start (GTK_BOX (vbox), options->sample_merged_w, FALSE, FALSE, 0);
  gtk_widget_show (options->sample_merged_w);

  /*  fill type  */
  frame = tool_options_radio_buttons_new (_("Fill Type"), 
					  &options->fill_mode,
					   options->fill_mode_w,
					   fill_mode_label,
					   fill_mode_value,
					   3);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->fill_mode_w[options->fill_mode_d]), TRUE); 
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
}


static void
bucket_fill_button_press (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  BucketTool * bucket_tool;
  int use_offsets;

  gdisp = (GDisplay *) gdisp_ptr;
  bucket_tool = (BucketTool *) tool->private;

  use_offsets = (bucket_options->sample_merged) ? FALSE : TRUE;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &bucket_tool->target_x,
			       &bucket_tool->target_y, FALSE, use_offsets);

  /*  Make the tool active and set the gdisplay which owns it  */
  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  /*  Make the tool active and set the gdisplay which owns it  */
  tool->gdisp_ptr = gdisp_ptr;
  tool->state = ACTIVE;
}


static void
bucket_fill_button_release (Tool           *tool,
			    GdkEventButton *bevent,
			    gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  BucketTool * bucket_tool;
  Argument *return_vals;
  int nreturn_vals;

  gdisp = (GDisplay *) gdisp_ptr;
  bucket_tool = (BucketTool *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      return_vals = procedural_db_run_proc ("gimp_bucket_fill",
					    &nreturn_vals,
					    PDB_DRAWABLE, drawable_ID (gimage_active_drawable (gdisp->gimage)),
					    PDB_INT32, (gint32) bucket_options->fill_mode,
					    PDB_INT32, (gint32) PAINT_OPTIONS_GET_PAINT_MODE (bucket_options),
					    PDB_FLOAT, (gdouble) PAINT_OPTIONS_GET_OPACITY (bucket_options) * 100,
					    PDB_FLOAT, (gdouble) bucket_options->threshold,
					    PDB_INT32, (gint32) bucket_options->sample_merged,
					    PDB_FLOAT, (gdouble) bucket_tool->target_x,
					    PDB_FLOAT, (gdouble) bucket_tool->target_y,
					    PDB_END);

      if (return_vals && return_vals[0].value.pdb_int == PDB_SUCCESS)
	gdisplays_flush ();
      else
	g_message (_("Bucket Fill operation failed."));

      procedural_db_destroy_args (return_vals, nreturn_vals);
    }

  tool->state = INACTIVE;
}


static void
bucket_fill_motion (Tool           *tool,
		    GdkEventMotion *mevent,
		    gpointer        gdisp_ptr)
{
}


static void
bucket_fill_cursor_update (Tool           *tool,
			   GdkEventMotion *mevent,
			   gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Layer *layer;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;
  int x, y;
  int off_x, off_y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);
  if ((layer = gimage_get_active_layer (gdisp->gimage))) 
    {
      drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);
      if (x >= off_x && y >= off_y &&
	  x < (off_x + drawable_width (GIMP_DRAWABLE(layer))) &&
	  y < (off_y + drawable_height (GIMP_DRAWABLE(layer))))
	{
	  /*  One more test--is there a selected region?
	   *  if so, is cursor inside?
	   */
	  if (gimage_mask_is_empty (gdisp->gimage))
	    ctype = GDK_TCROSS;
	  else if (gimage_mask_value (gdisp->gimage, x, y))
	    ctype = GDK_TCROSS;
	}
    }
  gdisplay_install_tool_cursor (gdisp, ctype);
}

static void
bucket_fill_modifier_key_func (Tool        *tool,
			     GdkEventKey *kevent,
			     gpointer     gdisp_ptr)
{
  switch (kevent->keyval)
    {
    case GDK_Alt_L: case GDK_Alt_R:
      break;
    case GDK_Shift_L: case GDK_Shift_R:
      switch (bucket_options->fill_mode)
	{
	case FG_BUCKET_FILL:
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bucket_options->fill_mode_w[BG_BUCKET_FILL]), TRUE);
	  break;
	case BG_BUCKET_FILL:
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bucket_options->fill_mode_w[FG_BUCKET_FILL]), TRUE);
	  break;
	default:
	  break;
	}
      break;
    case GDK_Control_L: case GDK_Control_R:
      break;
    }
}

static void
bucket_fill_control (Tool     *tool,
		     int       action,
		     gpointer  gdisp_ptr)
{
}


void
bucket_fill (GimpImage      *gimage,
	     GimpDrawable   *drawable,
	     BucketFillMode  fill_mode,
	     int             paint_mode,
	     double          opacity,
	     double          threshold,
	     int             sample_merged,
	     double          x,
	     double          y)
{
  TileManager *buf_tiles;
  PixelRegion bufPR, maskPR;
  Channel * mask = NULL;
  int bytes, has_alpha;
  int x1, y1, x2, y2;
  unsigned char col [MAX_CHANNELS];
  unsigned char *d1, *d2;
  GPatternP pattern;
  TempBuf * pat_buf;
  int new_buf = 0;

  pat_buf = NULL;

  if (fill_mode == FG_BUCKET_FILL)
    gimage_get_foreground (gimage, drawable, col);
  else if (fill_mode == BG_BUCKET_FILL)
    gimage_get_background (gimage, drawable, col);
  else if (fill_mode == PATTERN_BUCKET_FILL)
    {
      pattern = get_active_pattern ();

      if (!pattern)
	{
	  g_message (_("No available patterns for this operation."));
	  return;
	}

      /*  If the pattern doesn't match the image in terms of color type,
       *  transform it.  (ie  pattern is RGB, image is indexed)
       */
      if (((pattern->mask->bytes == 3) && !drawable_color (drawable)) ||
	  ((pattern->mask->bytes == 1) && !drawable_gray (drawable)))
	{
	  int size;

	  if ((pattern->mask->bytes == 1) && drawable_color (drawable))
	    pat_buf = temp_buf_new (pattern->mask->width, pattern->mask->height, 3, 0, 0, NULL);
	  else
	    pat_buf = temp_buf_new (pattern->mask->width, pattern->mask->height, 1, 0, 0, NULL);

	  d1 = temp_buf_data (pattern->mask);
	  d2 = temp_buf_data (pat_buf);

	  size = pattern->mask->width * pattern->mask->height;
	  while (size--)
	    {
	      gimage_transform_color (gimage, drawable, d1, d2,
				      (pattern->mask->bytes == 3) ? RGB : GRAY);
	      d1 += pattern->mask->bytes;
	      d2 += pat_buf->bytes;
	    }

	  new_buf = 1;
	}
      else
	pat_buf = pattern->mask;
    }

  gimp_add_busy_cursors ();

  bytes = drawable_bytes (drawable);
  has_alpha = drawable_has_alpha (drawable);

  /*  If there is no selection mask, the do a seed bucket
   *  fill...To do this, calculate a new contiguous region
   */
  if (! drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      mask = find_contiguous_region (gimage, drawable, TRUE, (int) threshold,
				     (int) x, (int) y, sample_merged);

      channel_bounds (mask, &x1, &y1, &x2, &y2);

      /*  make sure we handle the mask correctly if it was sample-merged  */
      if (sample_merged)
	{
	  int off_x, off_y;

	  /*  Limit the channel bounds to the drawable's extents  */
	  drawable_offsets (drawable, &off_x, &off_y);
	  x1 = BOUNDS (x1, off_x, (off_x + drawable_width (drawable)));
	  y1 = BOUNDS (y1, off_y, (off_y + drawable_height (drawable)));
	  x2 = BOUNDS (x2, off_x, (off_x + drawable_width (drawable)));
	  y2 = BOUNDS (y2, off_y, (off_y + drawable_height (drawable)));

	  pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE (mask)), 
			     x1, y1, (x2 - x1), (y2 - y1), TRUE);

	  /*  translate mask bounds to drawable coords  */
	  x1 -= off_x;
	  y1 -= off_y;
	  x2 -= off_x;
	  y2 -= off_y;
	}
      else
	pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE (mask)), 
			   x1, y1, (x2 - x1), (y2 - y1), TRUE);

      /*  if the gimage doesn't have an alpha channel,
       *  make sure that the temp buf does.  We need the
       *  alpha channel to fill with the region calculated above
       */
      if (! has_alpha)
	{
	  bytes ++;
	  has_alpha = 1;
	}
    }

  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1), bytes);
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);

  if (mask)
    bucket_fill_region (fill_mode, &bufPR, &maskPR, col, pat_buf, x1, y1, has_alpha);
  else
    bucket_fill_region (fill_mode, &bufPR, NULL, col, pat_buf, x1, y1, has_alpha);

  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimage_apply_image (gimage, drawable, &bufPR, TRUE,
		      (opacity * 255) / 100, paint_mode, NULL, x1, y1);

  /*  update the image  */
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the temporary buffer  */
  tile_manager_destroy (buf_tiles);

  /*  free the mask  */
  if (mask)
    channel_delete (mask);

  if (new_buf)
    temp_buf_free (pat_buf);

  gimp_remove_busy_cursors (NULL);
}


static void
bucket_fill_line_color (unsigned char *buf,
			unsigned char *mask,
			unsigned char *col,
			int            has_alpha,
			int            bytes,
			int            width)
{
  int alpha, b;

  alpha = (has_alpha) ? bytes - 1 : bytes;
  while (width--)
    {
      for (b = 0; b < alpha; b++)
	buf[b] = col[b];

      if (has_alpha)
	{
	  if (mask)
	    buf[alpha] = *mask++;
	  else
	    buf[alpha] = OPAQUE_OPACITY;
	}

      buf += bytes;
    }
}


static void
bucket_fill_line_pattern (unsigned char *buf,
			  unsigned char *mask,
			  TempBuf       *pattern,
			  int            has_alpha,
			  int            bytes,
			  int            x,
			  int            y,
			  int            width)
{
  unsigned char *pat, *p;
  int alpha, b;
  int i;

  /*  Get a pointer to the appropriate scanline of the pattern buffer  */
  pat = temp_buf_data (pattern) +
    (y % pattern->height) * pattern->width * pattern->bytes;

  alpha = (has_alpha) ? bytes - 1 : bytes;
  for (i = 0; i < width; i++)
    {
      p = pat + ((i + x) % pattern->width) * pattern->bytes;

      for (b = 0; b < alpha; b++)
	buf[b] = p[b];

      if (has_alpha)
	{
	  if (mask)
	    buf[alpha] = *mask++;
	  else
	    buf[alpha] = OPAQUE_OPACITY;
	}

      buf += bytes;
    }
}


static void
bucket_fill_region (BucketFillMode  fill_mode,
		    PixelRegion    *bufPR,
		    PixelRegion    *maskPR,
		    unsigned char  *col,
		    TempBuf        *pattern,
		    int             off_x,
		    int             off_y,
		    int             has_alpha)
{
  unsigned char *s, *m;
  int y;
  void *pr;

  for (pr = pixel_regions_register (2, bufPR, maskPR); pr != NULL; pr = pixel_regions_process (pr))
    {
      s = bufPR->data;
      if (maskPR)
	m = maskPR->data;
      else
	m = NULL;

      for (y = 0; y < bufPR->h; y++)
	{
	  switch (fill_mode)
	    {
	    case FG_BUCKET_FILL:
	    case BG_BUCKET_FILL:
	      bucket_fill_line_color (s, m, col, has_alpha, bufPR->bytes, bufPR->w);
	      break;
	    case PATTERN_BUCKET_FILL:
	      bucket_fill_line_pattern (s, m, pattern, has_alpha, bufPR->bytes,
					off_x + bufPR->x, off_y + y + bufPR->y, bufPR->w);
	      break;
	    }
	  s += bufPR->rowstride;

	  if (maskPR)
	    m += maskPR->rowstride;
	}
    }
}

/*********************************/
/*  Global bucket fill functions */
/*********************************/


Tool *
tools_new_bucket_fill (void)
{
  Tool * tool;
  BucketTool * private;

  /*  The tool options  */
  if (! bucket_options)
    {
      bucket_options = bucket_options_new ();
      tools_register (BUCKET_FILL, (ToolOptions *) bucket_options);

      /*  press all default buttons  */
      bucket_options_reset ();
    }

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (BucketTool *) g_malloc (sizeof (BucketTool));

  tool->type = BUCKET_FILL;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = private;

  tool->button_press_func = bucket_fill_button_press;
  tool->button_release_func = bucket_fill_button_release;
  tool->motion_func = bucket_fill_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;  
  tool->modifier_key_func = bucket_fill_modifier_key_func;
  tool->cursor_update_func = bucket_fill_cursor_update;
  tool->control_func = bucket_fill_control;
  tool->preserve = TRUE;

  return tool;
}


void
tools_free_bucket_fill (Tool *tool)
{
  BucketTool * bucket_tool;

  bucket_tool = (BucketTool *) tool->private;

  g_free (bucket_tool);
}
