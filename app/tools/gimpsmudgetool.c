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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpbrush.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"

#include "gimpsmudgetool.h"
#include "paint_options.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "gdisplay.h"
#include "gimpui.h"

#include "libgimp/gimpintl.h"

#define WANT_SMUDGE_BITS
#include "icons.h"


/* default defines */

#define SMUDGE_DEFAULT_RATE   50.0

/*  the smudge structures  */

typedef struct _SmudgeOptions SmudgeOptions;

struct _SmudgeOptions
{
  PaintOptions  paint_options;

  gdouble       rate;
  gdouble       rate_d;
  GtkObject    *rate_w;
};


/*  function prototypes */
static void   gimp_smudge_tool_class_init (GimpSmudgeToolClass *klass);
static void   gimp_smudge_tool_init       (GimpSmudgeTool      *tool);

static void   gimp_smudge_tool_paint      (GimpPaintTool *paint_tool,
					   GimpDrawable  *drawable,
					   PaintState     state);
static void   gimp_smudge_tool_motion        (GimpPaintTool *paint_tool,
					      PaintPressureOptions *pressure_options,
					      gdouble       smudge_rate,
						  GimpDrawable *drawable);
static gboolean   gimp_smudge_tool_start          (GimpPaintTool *paint_tool,
						   GimpDrawable  *drawable);
static void       gimp_smudge_tool_finish        (GimpPaintTool *paint_tool,
						  GimpDrawable *drawable);

static void       gimp_smudge_tool_nonclipped_painthit_coords (GimpPaintTool *paint_tool,
							       gint      *x,
							       gint      *y, 
							       gint      *w,
							       gint      *h);
static void       gimp_smudge_tool_allocate_accum_buffer (gint       w,
							  gint       h, 
							  gint       bytes,
							  guchar    *do_fill);

static SmudgeOptions * smudge_options_new   (void);
static void            smudge_options_reset (ToolOptions *tool_options);


/*  local variables */
static PixelRegion  accumPR;
static guchar      *accum_data;

static GimpPaintToolClass *parent_class = NULL;

/*  the smudge tool options  */
static SmudgeOptions * smudge_options = NULL;

static gdouble  non_gui_rate;


/* global functions  */

void
gimp_smudge_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_SMUDGE_TOOL,
			      TRUE,
  			      "gimp:smudge_tool",
  			      _("Smudge"),
  			      _("Smudge image"),
      			      N_("/Tools/Paint Tools/Smudge"), "S",
  			      NULL, "tools/smudge.html",
			      (const gchar **) smudge_bits);
}

GtkType
gimp_smudge_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpSmudgeTool",
        sizeof (GimpSmudgeTool),
        sizeof (GimpSmudgeToolClass),
        (GtkClassInitFunc) gimp_smudge_tool_class_init,
        (GtkObjectInitFunc) gimp_smudge_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        NULL
      };

      tool_type = gtk_type_unique (GIMP_TYPE_PAINT_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_smudge_tool_class_init (GimpSmudgeToolClass *klass)
{
  GimpPaintToolClass *paint_tool_class;

  paint_tool_class = (GimpPaintToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PAINT_TOOL);

  paint_tool_class->paint = gimp_smudge_tool_paint;
}

static void
gimp_smudge_tool_init (GimpSmudgeTool *smudge)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (smudge);
  paint_tool = GIMP_PAINT_TOOL (smudge);

  if (! smudge_options)
    {
      smudge_options = smudge_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_SMUDGE_TOOL,
                                          (ToolOptions *) smudge_options);
    }

  tool->tool_cursor = GIMP_SMUDGE_TOOL_CURSOR;

  paint_tool->pick_colors =  TRUE;
  paint_tool->flags       |= TOOL_CAN_HANDLE_CHANGING_BRUSH;
}

static void
gimp_smudge_tool_paint (GimpPaintTool *paint_tool,
			GimpDrawable  *drawable,
			PaintState     state)
{
  /* initialization fails if the user starts outside the drawable */
  static gboolean initialized = FALSE;

  switch (state)
    {
    case MOTION_PAINT:
      if (!initialized)
	initialized = gimp_smudge_tool_start (paint_tool, drawable);
      if (initialized)
	gimp_smudge_tool_motion (paint_tool,
				 smudge_options->paint_options.pressure_options,
				 smudge_options->rate, drawable);
      break;

    case FINISH_PAINT:
      gimp_smudge_tool_finish (paint_tool, drawable);
      initialized = FALSE;
      break;

    default:
      break;
    }

  return;
}

static void
gimp_smudge_tool_finish (GimpPaintTool *paint_tool,
			 GimpDrawable  *drawable)
{
  if (accum_data)
    {
      g_free (accum_data);
      accum_data = NULL;
    }
}

static void 
gimp_smudge_tool_nonclipped_painthit_coords (GimpPaintTool *paint_tool,
					     gint          *x, 
					     gint          *y, 
					     gint          *w, 
					     gint          *h)
{
  /* Note: these are the brush mask size plus a border of 1 pixel */
  *x = (gint) paint_tool->curx - paint_tool->brush->mask->width/2 - 1;
  *y = (gint) paint_tool->cury - paint_tool->brush->mask->height/2 - 1;
  *w = paint_tool->brush->mask->width + 2;
  *h = paint_tool->brush->mask->height + 2;
}

static gboolean
gimp_smudge_tool_start (GimpPaintTool *paint_tool,
			GimpDrawable  *drawable)
{
  GimpImage   *gimage;
  TempBuf     *area;
  PixelRegion  srcPR;
  gint         x, y, w, h;
  gint         was_clipped;
  guchar      *do_fill = NULL;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return FALSE;

  /*  If the image type is indexed, don't smudge  */
  if (gimp_drawable_is_indexed (drawable))
    return FALSE;

  area = gimp_paint_tool_get_paint_area (paint_tool, drawable, 1.0);

  if (!area) 
    return FALSE;
  
  /*  adjust the x and y coordinates to the upper left corner of the brush  */
  gimp_smudge_tool_nonclipped_painthit_coords (paint_tool, &x, &y, &w, &h);
  
  if (x != area->x || y != area->y || w != area->width || h != area->height)
    was_clipped = TRUE;
  else 
    was_clipped = FALSE;

  /* When clipped, accum_data may contain pixels that map to
     off-canvas pixels of the under-the-brush image, particulary
     when the brush image contains an edge or corner of the
     image. These off-canvas pixels are not a part of the current
     composite, but may be composited in later generations. do_fill
     contains a copy of the color of the pixel at the center of the
     brush; assumed this is a reasonable choice for off- canvas pixels
     that may enter into the blend */

  if (was_clipped)
    do_fill = gimp_drawable_get_color_at (drawable,
                 CLAMP ((gint) paint_tool->curx, 0, gimp_drawable_width (drawable) - 1),
                 CLAMP ((gint) paint_tool->cury, 0, gimp_drawable_height (drawable) - 1));

  gimp_smudge_tool_allocate_accum_buffer (w, h, 
					  gimp_drawable_bytes (drawable), 
					  do_fill);

  accumPR.x = area->x - x; 
  accumPR.y = area->y - y;
  accumPR.w = area->width;
  accumPR.h = area->height;
  accumPR.rowstride = accumPR.bytes * w; 
  accumPR.data = accum_data 
	+ accumPR.rowstride * accumPR.y 
	+ accumPR.x * accumPR.bytes;

  pixel_region_init (&srcPR, gimp_drawable_data (drawable), 
		     area->x, area->y, area->width, area->height, FALSE);

  /* copy the region under the original painthit. */
  copy_region (&srcPR, &accumPR);

  accumPR.x = area->x - x; 
  accumPR.y = area->y - y;
  accumPR.w = area->width;
  accumPR.h = area->height;
  accumPR.rowstride = accumPR.bytes * w;
  accumPR.data = accum_data
	+ accumPR.rowstride * accumPR.y 
	+ accumPR.x * accumPR.bytes;

  if (do_fill) 
    g_free(do_fill);

  return TRUE;
}

static void
gimp_smudge_tool_allocate_accum_buffer (gint    w,
					gint    h,
					gint    bytes,
					guchar *do_fill)
{
  /*  Allocate the accumulation buffer */
  accumPR.bytes = bytes;
  accum_data = g_malloc (w * h * bytes);
 
  if (do_fill != NULL)
    {
      /* guchar color[3] = {0,0,0}; */
      accumPR.x = 0; 
      accumPR.y = 0;
      accumPR.w = w;
      accumPR.h = h;
      accumPR.rowstride = accumPR.bytes * w;
      accumPR.data = accum_data;
      color_region (&accumPR, (const guchar*)do_fill);
    }
}

static void
gimp_smudge_tool_motion (GimpPaintTool        *paint_tool,
			 PaintPressureOptions *pressure_options,
			 gdouble               smudge_rate,
			 GimpDrawable         *drawable)
{
  GimpImage   *gimage;
  TempBuf     *area;
  PixelRegion  srcPR, destPR, tempPR;
  gdouble      rate;
  gint         opacity;
  gint         x, y, w, h;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  /*  If the image type is indexed, don't smudge  */
  if (gimp_drawable_is_indexed (drawable))
    return;

  gimp_smudge_tool_nonclipped_painthit_coords (paint_tool, &x, &y, &w, &h);

  /*  Get the paint area */
  /*  Smudge won't scale!  */
  if (! (area = gimp_paint_tool_get_paint_area (paint_tool, drawable, 1.0)))
    return;

  /* srcPR will be the pixels under the current painthit from 
     the drawable*/

  pixel_region_init (&srcPR, gimp_drawable_data (drawable), 
		     area->x, area->y, area->width, area->height, FALSE);

  /* Enable pressure sensitive rate */
  if (pressure_options->rate)
    rate = MIN (smudge_rate / 100.0 * paint_tool->curpressure * 2.0, 1.0);
  else
    rate = smudge_rate / 100.0;

  /* The tempPR will be the built up buffer (for smudge) */ 
  tempPR.bytes = accumPR.bytes;
  tempPR.rowstride = accumPR.rowstride;
  tempPR.x = area->x - x; 
  tempPR.y = area->y - y;
  tempPR.w = area->width;
  tempPR.h = area->height;
  tempPR.data = accum_data +
    tempPR.rowstride * tempPR.y + tempPR.x * tempPR.bytes;

  /* The dest will be the paint area we got above (= canvas_buf) */    

  destPR.bytes = area->bytes;                                     
  destPR.x = 0; destPR.y = 0;                                     
  destPR.w = area->width;                                         
  destPR.h = area->height;                                        
  destPR.rowstride = area->width * area->bytes;                  
  destPR.data = temp_buf_data (area); 

  /*  
     Smudge uses the buffer Accum.
     For each successive painthit Accum is built like this
	Accum =  rate*Accum  + (1-rate)*I.
     where I is the pixels under the current painthit. 
     Then the paint area (canvas_buf) is built as 
	(Accum,1) (if no alpha),
  */

  blend_region (&srcPR, &tempPR, &tempPR, ROUND (rate * 255.0));

  /* re-init the tempPR */

  tempPR.bytes = accumPR.bytes;
  tempPR.rowstride = accumPR.rowstride;
  tempPR.x = area->x - x; 
  tempPR.y = area->y - y;
  tempPR.w = area->width;
  tempPR.h = area->height;
  tempPR.data = accum_data 
	+ tempPR.rowstride * tempPR.y 
	+ tempPR.x * tempPR.bytes;

  if (! gimp_drawable_has_alpha (drawable))                             
    add_alpha_region (&tempPR, &destPR);                          
  else                                                            
    copy_region (&tempPR, &destPR);

  opacity = 255 * gimp_context_get_opacity (NULL);
  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_tool->curpressure;

  /*Replace the newly made paint area to the gimage*/ 
  gimp_paint_tool_replace_canvas (paint_tool, drawable, 
				  MIN (opacity, 255),
				  OPAQUE_OPACITY, 
				  pressure_options->pressure ? PRESSURE : SOFT,
				  1.0, INCREMENTAL);
}


static GimpSmudgeTool *non_gui_smudge = NULL;

gboolean
gimp_smudge_tool_non_gui_default (GimpDrawable *drawable,
				  gint          num_strokes,
				  gdouble      *stroke_array)
{
  gdouble           rate = SMUDGE_DEFAULT_RATE;
  SmudgeOptions *options = smudge_options;

  if (options)
    rate = options->rate;

  return gimp_smudge_tool_non_gui (drawable, rate, num_strokes, stroke_array);
}

gboolean
gimp_smudge_tool_non_gui (GimpDrawable *drawable,
			  gdouble       rate,
			  gint          num_strokes,
			  gdouble      *stroke_array)
{
  GimpPaintTool *paint_tool;
  gint           i;

  if (! non_gui_smudge)
    {
      non_gui_smudge = gtk_type_new (GIMP_TYPE_SMUDGE_TOOL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_smudge);

  if (gimp_paint_tool_start (paint_tool, drawable,
			    stroke_array[0], stroke_array[1]))
    {
      gimp_smudge_tool_start (paint_tool, drawable);

      non_gui_rate = rate;

      paint_tool->curx = paint_tool->startx = 
	paint_tool->lastx = stroke_array[0];
      paint_tool->cury = paint_tool->starty = 
	paint_tool->lasty = stroke_array[1];

      gimp_smudge_tool_paint (paint_tool, drawable, 0); 

      for (i = 1; i < num_strokes; i++)
	{
	  paint_tool->curx = stroke_array[i * 2 + 0];
	  paint_tool->cury = stroke_array[i * 2 + 1];

	  gimp_paint_tool_interpolate (paint_tool, drawable);

	  paint_tool->lastx = paint_tool->curx;
	  paint_tool->lasty = paint_tool->cury;
	}

      gimp_paint_tool_finish (paint_tool, drawable);

      gimp_paint_tool_cleanup ();

      gimp_smudge_tool_finish (paint_tool, drawable);

      return TRUE;
    }

  return FALSE;
}

static SmudgeOptions *
smudge_options_new (void)
{
  SmudgeOptions *options;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *scale;

  options = g_new0 (SmudgeOptions, 1);

  paint_options_init ((PaintOptions *) options,
		      GIMP_TYPE_SMUDGE_TOOL,
		      smudge_options_reset);

  options->rate = options->rate_d = SMUDGE_DEFAULT_RATE;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the rate scale  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Rate:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  options->rate_w =
    gtk_adjustment_new (options->rate_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->rate_w));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->rate_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->rate);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);

  return options;
}

static void
smudge_options_reset (ToolOptions *tool_options)
{
  SmudgeOptions *options;

  options = (SmudgeOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->rate_w), options->rate_d);
}
