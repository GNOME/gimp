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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimppattern.h"

#include "appenv.h"
#include "app_procs.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimprc.h"
#include "pixel_region.h"
#include "selection.h"
#include "temp_buf.h"
#include "tile_manager.h"
#include "undo.h"

#include "gimpbucketfilltool.h"
#include "gimpfuzzyselecttool.h"
#include "gimptool.h"
#include "paint_options.h"
#include "tool_manager.h"

#include "pdb/procedural_db.h"

#include "libgimp/gimpintl.h"

#include "pixmaps2.h"


typedef struct _BucketOptions BucketOptions;

struct _BucketOptions
{
  PaintOptions    paint_options;

  gboolean        sample_merged;
  gboolean        sample_merged_d;
  GtkWidget      *sample_merged_w;

  gdouble         threshold;
  /* gdouble      threshold_d; (from gimprc) */
  GtkObject      *threshold_w;

  BucketFillMode  fill_mode;
  BucketFillMode  fill_mode_d;
  GtkWidget      *fill_mode_w[3];
};


/*  the bucket fill tool options  */
static BucketOptions *bucket_options = NULL;

static GimpToolClass *parent_class   = NULL;


/*  local function prototypes  */

static void   gimp_bucket_fill_tool_class_init (GimpBucketFillToolClass *klass);
static void   gimp_bucket_fill_tool_init       (GimpBucketFillTool      *bucket_fill_tool);

static void   gimp_bucket_fill_tool_destroy    (GtkObject      *object);

static BucketOptions * bucket_options_new           (void);
static void            bucket_options_reset         (ToolOptions    *tool_options);

static void   gimp_bucket_fill_tool_button_press    (GimpTool       *tool,
						     GdkEventButton *bevent,
						     GDisplay       *gdisp);
static void   gimp_bucket_fill_tool_button_release  (GimpTool       *tool,
						     GdkEventButton *bevent,
						     GDisplay       *gdisp);
static void   gimp_bucket_fill_tool_cursor_update   (GimpTool       *tool,
						     GdkEventMotion *mevent,
						     GDisplay       *gdisp);
static void   gimp_bucket_fill_tool_modifier_key    (GimpTool       *tool,
						     GdkEventKey    *kevent,
						     GDisplay       *gdisp);

static void   bucket_fill_line_color                (guchar         *,
						     guchar         *,
						     guchar         *,
						     gboolean        ,
						     gint            ,
						     gint            );
static void   bucket_fill_line_pattern              (guchar         *,
						     guchar         *,
						     TempBuf        *,
						     gboolean        ,
						     gint            ,
						     gint            ,
						     gint            ,
						     gint            );


/*  functions  */

void
gimp_bucket_fill_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_BUCKET_FILL_TOOL,
                              TRUE,
			      "gimp:bucket_fill_tool",
			      _("Bucket Fill"),
			      _("Fill with a color or pattern"),
			      N_("/Tools/Paint Tools/Bucket Fill"), "<shift>B",
			      NULL, "tools/bucket_fill.html",
			      (const gchar **) fill_bits);
}

GtkType
gimp_bucket_fill_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpBucketFillTool",
        sizeof (GimpBucketFillTool),
        sizeof (GimpBucketFillToolClass),
        (GtkClassInitFunc) gimp_bucket_fill_tool_class_init,
        (GtkObjectInitFunc) gimp_bucket_fill_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_bucket_fill_tool_class_init (GimpBucketFillToolClass *klass)
{
  GtkObjectClass    *object_class;
  GimpToolClass     *tool_class;

  object_class = (GtkObjectClass *) klass;
  tool_class   = (GimpToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_TOOL);

  object_class->destroy      = gimp_bucket_fill_tool_destroy;

  tool_class->button_press   = gimp_bucket_fill_tool_button_press;
  tool_class->button_release = gimp_bucket_fill_tool_button_release;
  tool_class->modifier_key   = gimp_bucket_fill_tool_modifier_key;
  tool_class->cursor_update  = gimp_bucket_fill_tool_cursor_update;
}

static void
gimp_bucket_fill_tool_init (GimpBucketFillTool *bucket_fill_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (bucket_fill_tool);

  if (! bucket_options)
    {
      bucket_options = bucket_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_BUCKET_FILL_TOOL,
					  (ToolOptions *) bucket_options);

      bucket_options_reset ((ToolOptions *) bucket_options);
    }

  tool->tool_cursor = GIMP_BUCKET_FILL_TOOL_CURSOR;

  tool->scroll_lock = TRUE;  /*  Disallow scrolling  */
}

static void
gimp_bucket_fill_tool_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bucket_options_reset (ToolOptions *tool_options)
{
  BucketOptions *options;

  options = (BucketOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->threshold_w),
			    default_threshold);
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

  options = g_new0 (BucketOptions, 1);

  paint_options_init ((PaintOptions *) options,
		      GIMP_TYPE_BUCKET_FILL_TOOL,
		      bucket_options_reset);

  options->sample_merged = options->sample_merged_d = FALSE;
  options->threshold                                = default_threshold;
  options->fill_mode     = options->fill_mode_d     = FG_BUCKET_FILL;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the sample merged toggle  */
  options->sample_merged_w =
    gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_signal_connect (GTK_OBJECT (options->sample_merged_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->sample_merged);
  gtk_box_pack_start (GTK_BOX (vbox), options->sample_merged_w, FALSE, FALSE, 0);
  gtk_widget_show (options->sample_merged_w);

  /*  the threshold scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Threshold:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->threshold_w =
    gtk_adjustment_new (default_threshold, 0.0, 255.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->threshold_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->threshold_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->threshold);
  gtk_widget_show (scale);

  gtk_widget_show (hbox);

  /*  fill type  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Fill Type"),
			   gimp_radio_button_update,
			   &options->fill_mode, (gpointer) options->fill_mode,

			   _("FG Color Fill"),
			   (gpointer) FG_BUCKET_FILL,
			   &options->fill_mode_w[0],
			   _("BG Color Fill"),
			   (gpointer) BG_BUCKET_FILL,
			   &options->fill_mode_w[1],
			   _("Pattern Fill"),
			   (gpointer) PATTERN_BUCKET_FILL,
			   &options->fill_mode_w[2],

			   NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return options;
}

/*  bucket fill action functions  */

static void
gimp_bucket_fill_tool_button_press (GimpTool       *tool,
				    GdkEventButton *bevent,
				    GDisplay       *gdisp)
{
  GimpBucketFillTool *bucket_tool;
  gboolean            use_offsets;

  bucket_tool = GIMP_BUCKET_FILL_TOOL (tool);

  use_offsets = (bucket_options->sample_merged) ? FALSE : TRUE;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &bucket_tool->target_x,
			       &bucket_tool->target_y, FALSE, use_offsets);

  /*  Make the tool active and set the gdisplay which owns it  */
  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON1_MOTION_MASK |
		    GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  /*  Make the tool active and set the gdisplay which owns it  */
  tool->gdisp = gdisp;
  tool->state = ACTIVE;
}

static void
gimp_bucket_fill_tool_button_release (GimpTool       *tool,
				      GdkEventButton *bevent,
				      GDisplay       *gdisp)
{
  GimpBucketFillTool *bucket_tool;
  Argument           *return_vals;
  gint                nreturn_vals;

  bucket_tool = GIMP_BUCKET_FILL_TOOL (tool);

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  if the 3rd button isn't pressed, fill the selected region  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      return_vals =
	procedural_db_run_proc ("gimp_bucket_fill",
				&nreturn_vals,
				PDB_DRAWABLE, gimp_drawable_get_ID (gimp_image_active_drawable (gdisp->gimage)),
				PDB_INT32, (gint32) bucket_options->fill_mode,
				PDB_INT32, (gint32) gimp_context_get_paint_mode (NULL),
				PDB_FLOAT, (gdouble) gimp_context_get_opacity (NULL) * 100,
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
gimp_bucket_fill_tool_cursor_update (GimpTool       *tool,
				     GdkEventMotion *mevent,
				     GDisplay       *gdisp)
{
  GimpLayer          *layer;
  GdkCursorType       ctype     = GDK_TOP_LEFT_ARROW;
  GimpCursorModifier  cmodifier = GIMP_CURSOR_MODIFIER_NONE;
  gint                x, y;
  gint                off_x, off_y;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &x, &y, FALSE, FALSE);

  if ((layer = gimp_image_get_active_layer (gdisp->gimage))) 
    {
      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

      if (x >= off_x && y >= off_y &&
	  x < (off_x + gimp_drawable_width (GIMP_DRAWABLE (layer))) &&
	  y < (off_y + gimp_drawable_height (GIMP_DRAWABLE (layer))))
	{
	  /*  One more test--is there a selected region?
	   *  if so, is cursor inside?
	   */
	  if (gimage_mask_is_empty (gdisp->gimage) ||
	      gimage_mask_value (gdisp->gimage, x, y))
	    {
	      ctype = GIMP_MOUSE_CURSOR;

	      switch (bucket_options->fill_mode)
		{
		case FG_BUCKET_FILL:
		  cmodifier = GIMP_CURSOR_MODIFIER_FOREGROUND;
		  break;
		case BG_BUCKET_FILL:
		  cmodifier = GIMP_CURSOR_MODIFIER_BACKGROUND;
		  break;
		case PATTERN_BUCKET_FILL:
		  cmodifier = GIMP_CURSOR_MODIFIER_PATTERN;
		  break;
		}
	    }
	}
    }

  gdisplay_install_tool_cursor (gdisp,
				ctype,
				GIMP_BUCKET_FILL_TOOL_CURSOR,
				cmodifier);
}

static void
gimp_bucket_fill_tool_modifier_key (GimpTool    *tool,
				    GdkEventKey *kevent,
				    GDisplay    *gdisp)
{
  switch (kevent->keyval)
    {
    case GDK_Alt_L: case GDK_Alt_R:
      break;
    case GDK_Shift_L: case GDK_Shift_R:
      break;
    case GDK_Control_L: case GDK_Control_R:
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
    }
}

void
bucket_fill (GimpImage      *gimage,
	     GimpDrawable   *drawable,
	     BucketFillMode  fill_mode,
	     gint            paint_mode,
	     gdouble         opacity,
	     gdouble         threshold,
	     gboolean        sample_merged,
	     gdouble         x,
	     gdouble         y)
{
  TileManager *buf_tiles;
  PixelRegion  bufPR, maskPR;
  GimpChannel *mask = NULL;
  gint         bytes;
  gboolean     has_alpha;
  gint         x1, y1, x2, y2;
  guchar       col [MAX_CHANNELS];
  guchar      *d1, *d2;
  GimpPattern *pattern;
  TempBuf     *pat_buf;
  gboolean     new_buf = FALSE;

  pat_buf = NULL;

  if (fill_mode == FG_BUCKET_FILL)
    gimp_image_get_foreground (gimage, drawable, col);
  else if (fill_mode == BG_BUCKET_FILL)
    gimp_image_get_background (gimage, drawable, col);
  else if (fill_mode == PATTERN_BUCKET_FILL)
    {
      pattern = gimp_context_get_pattern (NULL);

      if (!pattern)
	{
	  g_message (_("No available patterns for this operation."));
	  return;
	}

      /*  If the pattern doesn't match the image in terms of color type,
       *  transform it.  (ie  pattern is RGB, image is indexed)
       */
      if (((pattern->mask->bytes == 3) && !gimp_drawable_is_rgb  (drawable)) ||
	  ((pattern->mask->bytes == 1) && !gimp_drawable_is_gray (drawable)))
	{
	  int size;

	  if ((pattern->mask->bytes == 1) && gimp_drawable_is_rgb (drawable))
	    pat_buf = temp_buf_new (pattern->mask->width, pattern->mask->height,
				    3, 0, 0, NULL);
	  else
	    pat_buf = temp_buf_new (pattern->mask->width, pattern->mask->height,
				    1, 0, 0, NULL);

	  d1 = temp_buf_data (pattern->mask);
	  d2 = temp_buf_data (pat_buf);

	  size = pattern->mask->width * pattern->mask->height;
	  while (size--)
	    {
	      gimp_image_transform_color (gimage, drawable, d1, d2,
					  (pattern->mask->bytes == 3) ? RGB : GRAY);
	      d1 += pattern->mask->bytes;
	      d2 += pat_buf->bytes;
	    }

	  new_buf = TRUE;
	}
      else
	pat_buf = pattern->mask;
    }

  gimp_set_busy ();

  bytes = gimp_drawable_bytes (drawable);
  has_alpha = gimp_drawable_has_alpha (drawable);

  /*  If there is no selection mask, the do a seed bucket
   *  fill...To do this, calculate a new contiguous region
   */
  if (! gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      mask = find_contiguous_region (gimage, drawable, TRUE, (int) threshold,
				     (int) x, (int) y, sample_merged);

      gimp_channel_bounds (mask, &x1, &y1, &x2, &y2);

      /*  make sure we handle the mask correctly if it was sample-merged  */
      if (sample_merged)
	{
	  gint off_x, off_y;

	  /*  Limit the channel bounds to the drawable's extents  */
	  gimp_drawable_offsets (drawable, &off_x, &off_y);
	  x1 = CLAMP (x1, off_x, (off_x + gimp_drawable_width (drawable)));
	  y1 = CLAMP (y1, off_y, (off_y + gimp_drawable_height (drawable)));
	  x2 = CLAMP (x2, off_x, (off_x + gimp_drawable_width (drawable)));
	  y2 = CLAMP (y2, off_y, (off_y + gimp_drawable_height (drawable)));

	  pixel_region_init (&maskPR, gimp_drawable_data (GIMP_DRAWABLE (mask)), 
			     x1, y1, (x2 - x1), (y2 - y1), TRUE);

	  /*  translate mask bounds to drawable coords  */
	  x1 -= off_x;
	  y1 -= off_y;
	  x2 -= off_x;
	  y2 -= off_y;
	}
      else
	pixel_region_init (&maskPR, gimp_drawable_data (GIMP_DRAWABLE (mask)), 
			   x1, y1, (x2 - x1), (y2 - y1), TRUE);

      /*  if the gimage doesn't have an alpha channel,
       *  make sure that the temp buf does.  We need the
       *  alpha channel to fill with the region calculated above
       */
      if (! has_alpha)
	{
	  bytes ++;
	  has_alpha = TRUE;
	}
    }

  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1), bytes);
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);

  if (mask)
    bucket_fill_region (fill_mode, &bufPR, &maskPR, col, pat_buf, x1, y1, has_alpha);
  else
    bucket_fill_region (fill_mode, &bufPR, NULL, col, pat_buf, x1, y1, has_alpha);

  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimp_image_apply_image (gimage, drawable, &bufPR, TRUE,
			  (opacity * 255) / 100, paint_mode, NULL, x1, y1);

  /*  update the image  */
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the temporary buffer  */
  tile_manager_destroy (buf_tiles);

  /*  free the mask  */
  if (mask)
    gtk_object_unref (GTK_OBJECT (mask));

  if (new_buf)
    temp_buf_free (pat_buf);

  gimp_unset_busy ();
}

static void
bucket_fill_line_color (guchar   *buf,
			guchar   *mask,
			guchar   *col,
			gboolean  has_alpha,
			gint      bytes,
			gint      width)
{
  gint alpha, b;

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
bucket_fill_line_pattern (guchar   *buf,
			  guchar   *mask,
			  TempBuf  *pattern,
			  gboolean  has_alpha,
			  gint      bytes,
			  gint      x,
			  gint      y,
			  gint      width)
{
  guchar *pat, *p;
  gint    alpha, b;
  gint    i;

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

void
bucket_fill_region (BucketFillMode  fill_mode,
		    PixelRegion    *bufPR,
		    PixelRegion    *maskPR,
		    guchar         *col,
		    TempBuf        *pattern,
		    gint            off_x,
		    gint            off_y,
		    gboolean        has_alpha)
{
  guchar *s, *m;
  gint    y;
  void   *pr;

  for (pr = pixel_regions_register (2, bufPR, maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
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
