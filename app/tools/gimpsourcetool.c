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

#include "appenv.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimpui.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "paint_options.h"
#include "clone.h"
#include "selection.h"
#include "tools.h"
#include "cursorutil.h"

#include "libgimp/gimpintl.h"

#define TARGET_HEIGHT  15
#define TARGET_WIDTH   15

/* default types */
#define CLONE_DEFAULT_TYPE     IMAGE_CLONE
#define CLONE_DEFAULT_ALIGNED  ALIGN_NO

/*  the clone structures  */

typedef enum
{
  ALIGN_NO,
  ALIGN_YES,
  ALIGN_REGISTERED
} AlignType;

typedef struct _CloneOptions CloneOptions;

struct _CloneOptions
{
  PaintOptions  paint_options;

  CloneType     type;
  CloneType     type_d;
  GtkWidget    *type_w[2];  /* 2 radio buttons */

  AlignType     aligned;
  AlignType     aligned_d;
  GtkWidget    *aligned_w[3];  /* 3 radio buttons */
};


/*  the clone tool options  */
static CloneOptions *clone_options = NULL;

/*  local variables  */
static int           src_x = 0;                /*                         */
static int           src_y = 0;                /*  position of clone src  */
static int           dest_x = 0;               /*                         */
static int           dest_y = 0;               /*  position of clone src  */
static int           offset_x = 0;             /*                         */
static int           offset_y = 0;             /*  offset for cloning     */
static int           first = TRUE;
static int           trans_tx, trans_ty;       /*  transformed target     */
static GDisplay     *the_src_gdisp = NULL;     /*  ID of source gdisplay  */
static GimpDrawable *src_drawable_ = NULL;     /*  source drawable        */

static GimpDrawable *non_gui_src_drawable;
static int           non_gui_offset_x;
static int           non_gui_offset_y;
static CloneType     non_gui_type;

/*  forward function declarations  */

static void   clone_draw         (Tool *);
static void   clone_motion       (PaintCore *, GimpDrawable *, GimpDrawable *,
				  PaintPressureOptions *, CloneType, int, int);
static void   clone_line_image   (GImage *, GImage *, GimpDrawable *,
				  GimpDrawable *, unsigned char *,
				  unsigned char *, int, int, int, int);
static void   clone_line_pattern (GImage *, GimpDrawable *, GPattern *,
				  unsigned char *, int, int, int, int);


/*  functions  */

static void
clone_options_reset (void)
{
  CloneOptions *options = clone_options;

  paint_options_reset ((PaintOptions *) options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->aligned_w[options->aligned_d]), TRUE);
}

static CloneOptions *
clone_options_new (void)
{
  CloneOptions *options;
  GtkWidget *vbox;
  GtkWidget *frame;

  /*  the new clone tool options structure  */
  options = g_new (CloneOptions, 1);
  paint_options_init ((PaintOptions *) options,
		      CLONE,
		      clone_options_reset);
  options->type    = options->type_d    = CLONE_DEFAULT_TYPE;
  options->aligned = options->aligned_d = CLONE_DEFAULT_ALIGNED;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  frame = gimp_radio_group_new2 (TRUE, _("Source"),
				 gimp_radio_button_update,
				 &options->type, (gpointer) options->type,

				 _("Image Source"), (gpointer) IMAGE_CLONE,
				 &options->type_w[0],
				 _("Pattern Source"), (gpointer) PATTERN_CLONE,
				 &options->type_w[1],

				 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gimp_radio_group_new2 (TRUE, _("Alignment"),
				 gimp_radio_button_update,
				 &options->aligned, (gpointer) options->aligned,

				 _("Non Aligned"), (gpointer) ALIGN_NO,
				 &options->aligned_w[0],
				 _("Aligned"), (gpointer) ALIGN_YES,
				 &options->aligned_w[1],
				 _("Registered"), (gpointer) ALIGN_REGISTERED,
				 &options->aligned_w[2],

				 NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  
  return options;
}

static void
clone_src_drawable_destroyed_cb (GimpDrawable  *drawable,
				 GimpDrawable **src_drawable)
{
  if (drawable == *src_drawable)
    {
      *src_drawable = NULL;
      the_src_gdisp = NULL;
    }
}

static void
clone_set_src_drawable (GimpDrawable *drawable)
{
  if (src_drawable_ == drawable)
    return;
  if (src_drawable_)
    gtk_signal_disconnect_by_data (GTK_OBJECT (src_drawable_), &src_drawable_);
  src_drawable_ = drawable;
  if (drawable)
    {
      gtk_signal_connect (GTK_OBJECT (drawable), "destroy",
			  GTK_SIGNAL_FUNC (clone_src_drawable_destroyed_cb),
			  &src_drawable_);
    }
}

void *
clone_paint_func (PaintCore    *paint_core,
		  GimpDrawable *drawable,
		  gint          state)
{
  GDisplay * gdisp;
  GDisplay * src_gdisp;
  int x1, y1, x2, y2;
  static int orig_src_x, orig_src_y;

  gdisp = (GDisplay *) active_tool->gdisp_ptr;

  switch (state)
    {
    case PRETRACE_PAINT:
      draw_core_pause (paint_core->core, active_tool);
      break;
    case MOTION_PAINT:
      x1 = paint_core->curx;
      y1 = paint_core->cury;
      x2 = paint_core->lastx;
      y2 = paint_core->lasty;

      /*  If the control key is down, move the src target and return */
      if (paint_core->state & GDK_CONTROL_MASK)
	{
	  src_x = x1;
	  src_y = y1;
	  first = TRUE;
	}
      /*  otherwise, update the target  */
      else
	{
	  dest_x = x1;
	  dest_y = y1;

          if (clone_options->aligned == ALIGN_REGISTERED)
            {
	      offset_x = 0;
	      offset_y = 0;
            }
          else if (first)
	    {
	      offset_x = src_x - dest_x;
	      offset_y = src_y - dest_y;
	      first = FALSE;
	    }

	  src_x = dest_x + offset_x;
	  src_y = dest_y + offset_y;

	  clone_motion (paint_core, drawable, src_drawable_, 
			clone_options->paint_options.pressure_options, 
			clone_options->type, offset_x, offset_y);
	}

      break;

    case INIT_PAINT:
      if (paint_core->state & GDK_CONTROL_MASK)
	{
	  the_src_gdisp = gdisp;
	  clone_set_src_drawable(drawable);
	  src_x = paint_core->curx;
	  src_y = paint_core->cury;
	  first = TRUE;
	}
      else if (clone_options->aligned == ALIGN_NO)
	{
	  first = TRUE;
	  orig_src_x = src_x;
	  orig_src_y = src_y;
	}
      if (clone_options->type == PATTERN_CLONE)
	if (! gimp_context_get_pattern (NULL))
	  g_message (_("No patterns available for this operation."));
      break;

    case FINISH_PAINT:
      draw_core_stop (paint_core->core, active_tool);
      if (clone_options->aligned == ALIGN_NO && !first)
	{
	  src_x = orig_src_x;
	  src_y = orig_src_y;
	} 
      return NULL;
      break;

    default:
      break;
    }

  /*  Calculate the coordinates of the target  */
  src_gdisp = the_src_gdisp;
  if (!src_gdisp)
    {
      the_src_gdisp = gdisp;
      src_gdisp = the_src_gdisp;
    }

  if (state == INIT_PAINT)
    /*  Initialize the tool drawing core  */
    draw_core_start (paint_core->core,
		     src_gdisp->canvas->window,
		     active_tool);
  else if (state == POSTTRACE_PAINT)
    {
      /*  Find the target cursor's location onscreen  */
      gdisplay_transform_coords (src_gdisp, src_x, src_y,
				 &trans_tx, &trans_ty, 1);
      draw_core_resume (paint_core->core, active_tool);      
    }
  return NULL;
}

void
clone_cursor_update (Tool           *tool,
		     GdkEventMotion *mevent,
		     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  Layer *layer;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, (double) mevent->x, (double) mevent->y,
			       &x, &y, TRUE, FALSE);
 
  if ((layer = gimage_get_active_layer (gdisp->gimage))) 
    {
      int off_x, off_y;
      drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

      if (x >= off_x && y >= off_y &&
	  x < (off_x + drawable_width (GIMP_DRAWABLE (layer))) &&
	  y < (off_y + drawable_height (GIMP_DRAWABLE (layer))))
	{
	  /*  One more test--is there a selected region?
	   *  if so, is cursor inside?
	   */
	  if (gimage_mask_is_empty (gdisp->gimage))
	    ctype = GIMP_MOUSE_CURSOR;
	  else if (gimage_mask_value (gdisp->gimage, x, y))
	    ctype = GIMP_MOUSE_CURSOR;
	}
    }
  
  if (clone_options->type == IMAGE_CLONE)
    {
      if (mevent->state & GDK_CONTROL_MASK)
	ctype = GIMP_CROSSHAIR_SMALL_CURSOR;
      else if (!src_drawable_)
	ctype = GIMP_BAD_CURSOR;
    }

  gdisplay_install_tool_cursor (gdisp, ctype,
				ctype == GIMP_CROSSHAIR_SMALL_CURSOR ?
				TOOL_TYPE_NONE : CLONE,
				CURSOR_MODIFIER_NONE,
				FALSE);
}

Tool *
tools_new_clone (void)
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (! clone_options)
    {
      clone_options = clone_options_new ();
      tools_register (CLONE, (ToolOptions *) clone_options);

      /*  press all default buttons  */
      clone_options_reset ();
    }

  tool = paint_core_new (CLONE);
  /* the clone tool provides its own cursor_update_function 
     until I figure out somethinh nicer -- Sven  */
  tool->cursor_update_func = clone_cursor_update;

  private = (PaintCore *) tool->private;
  private->paint_func = clone_paint_func;
  private->core->draw_func = clone_draw;
  private->flags |= TOOL_TRACES_ON_WINDOW;

  return tool;
}

void
tools_free_clone (Tool *tool)
{
  paint_core_free (tool);
}

static void
clone_draw (Tool *tool)
{
  PaintCore * paint_core;

  paint_core = (PaintCore *) tool->private;

  if (paint_core->core->gc != NULL && clone_options->type == IMAGE_CLONE)
    {
      gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		     trans_tx - (TARGET_WIDTH >> 1), trans_ty,
		     trans_tx + (TARGET_WIDTH >> 1), trans_ty);
      gdk_draw_line (paint_core->core->win, paint_core->core->gc,
		     trans_tx, trans_ty - (TARGET_HEIGHT >> 1),
		     trans_tx, trans_ty + (TARGET_HEIGHT >> 1));
    }
}

static void
clone_motion (PaintCore            *paint_core,
	      GimpDrawable         *drawable,
	      GimpDrawable         *src_drawable,
	      PaintPressureOptions *pressure_options,
	      CloneType             type,
	      int                   offset_x,
	      int                   offset_y)
{
  GImage *gimage;
  GImage *src_gimage = NULL;
  unsigned char * s;
  unsigned char * d;
  TempBuf * orig;
  TempBuf * area;
  void * pr;
  int y;
  int x1, y1, x2, y2;
  int has_alpha = -1;
  PixelRegion srcPR, destPR;
  GPattern *pattern;
  gint opacity;
  gdouble scale;

  pr      = NULL;
  pattern = NULL;

  /*  Make sure we still have a source if we are doing image cloning */
  if (type == IMAGE_CLONE)
    {
      if (!src_drawable)
	return;
      if (! (src_gimage = drawable_gimage (src_drawable)))
	return;
      /*  Determine whether the source image has an alpha channel  */
      has_alpha = drawable_has_alpha (src_drawable);
    }

  /*  We always need a destination image */
  if (! (gimage = drawable_gimage (drawable)))
    return;

  if (pressure_options->size)
    scale = paint_core->curpressure;
  else
    scale = 1.0;

  /*  Get a region which can be used to paint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable, scale)))
    return;

  switch (type)
    {
    case IMAGE_CLONE:
      /*  Set the paint area to transparent  */
      memset (temp_buf_data (area), 0, area->width * area->height * area->bytes);

      /*  If the source gimage is different from the destination,
       *  then we should copy straight from the destination image
       *  to the canvas.
       *  Otherwise, we need a call to get_orig_image to make sure
       *  we get a copy of the unblemished (offset) image
       */
      if (src_drawable != drawable)
	{
	  x1 = CLAMP (area->x + offset_x, 0, drawable_width (src_drawable));
	  y1 = CLAMP (area->y + offset_y, 0, drawable_height (src_drawable));
	  x2 = CLAMP (area->x + offset_x + area->width,
		      0, drawable_width (src_drawable));
	  y2 = CLAMP (area->y + offset_y + area->height,
		      0, drawable_height (src_drawable));

	  if (!(x2 - x1) || !(y2 - y1))
	    return;

	  pixel_region_init (&srcPR, drawable_data (src_drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
	}
      else
	{
	  x1 = CLAMP (area->x + offset_x, 0, drawable_width (drawable));
	  y1 = CLAMP (area->y + offset_y, 0, drawable_height (drawable));
	  x2 = CLAMP (area->x + offset_x + area->width,
		      0, drawable_width (drawable));
	  y2 = CLAMP (area->y + offset_y + area->height,
		      0, drawable_height (drawable));

	  if (!(x2 - x1) || !(y2 - y1))
	    return;

	  /*  get the original image  */
	  orig = paint_core_get_orig_image (paint_core, drawable, x1, y1, x2, y2);

	  srcPR.bytes = orig->bytes;
	  srcPR.x = 0; srcPR.y = 0;
	  srcPR.w = x2 - x1;
	  srcPR.h = y2 - y1;
	  srcPR.rowstride = srcPR.bytes * orig->width;
	  srcPR.data = temp_buf_data (orig);
	}

      offset_x = x1 - (area->x + offset_x);
      offset_y = y1 - (area->y + offset_y);

      /*  configure the destination  */
      destPR.bytes = area->bytes;
      destPR.x = 0; destPR.y = 0;
      destPR.w = srcPR.w;
      destPR.h = srcPR.h;
      destPR.rowstride = destPR.bytes * area->width;
      destPR.data = temp_buf_data (area) + offset_y * destPR.rowstride +
	offset_x * destPR.bytes;

      pr = pixel_regions_register (2, &srcPR, &destPR);
      break;

    case PATTERN_CLONE:
      pattern = gimp_context_get_pattern (NULL);

      if (!pattern)
	return;

      destPR.bytes = area->bytes;
      destPR.x = 0; destPR.y = 0;
      destPR.w = area->width;
      destPR.h = area->height;
      destPR.rowstride = destPR.bytes * area->width;
      destPR.data = temp_buf_data (area);

      pr = pixel_regions_register (1, &destPR);
      break;
    }

  for (; pr != NULL; pr = pixel_regions_process (pr))
    {
      s = srcPR.data;
      d = destPR.data;
      for (y = 0; y < destPR.h; y++)
	{
	  switch (type)
	    {
	    case IMAGE_CLONE:
	      clone_line_image (gimage, src_gimage, drawable, src_drawable, s, d,
				has_alpha, srcPR.bytes, destPR.bytes, destPR.w);
	      s += srcPR.rowstride;
	      break;
	    case PATTERN_CLONE:
	      clone_line_pattern (gimage, drawable, pattern, d,
				  area->x + offset_x, area->y + y + offset_y,
				  destPR.bytes, destPR.w);
	      break;
	    }

	  d += destPR.rowstride;
	}
    }

  opacity = 255.0 * gimp_context_get_opacity (NULL);
  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_core->curpressure;

  /*  paste the newly painted canvas to the gimage which is being worked on  */
  paint_core_paste_canvas (paint_core, drawable, 
			   MIN (opacity, 255),
			   (int) (gimp_context_get_opacity (NULL) * 255),
			   gimp_context_get_paint_mode (NULL),
			   pressure_options->pressure ? PRESSURE : SOFT, 
			   scale, CONSTANT);
}


static void
clone_line_image (GImage        *dest,
		  GImage        *src,
		  GimpDrawable  *d_drawable,
		  GimpDrawable  *s_drawable,
		  unsigned char *s,
		  unsigned char *d,
		  int            has_alpha,
		  int            src_bytes,
		  int            dest_bytes,
		  int            width)
{
  unsigned char rgb[3];
  int src_alpha, dest_alpha;

  src_alpha = src_bytes - 1;
  dest_alpha = dest_bytes - 1;

  while (width--)
    {
      gimage_get_color (src, drawable_type (s_drawable), rgb, s);
      gimage_transform_color (dest, d_drawable, rgb, d, RGB);

      if (has_alpha)
	d[dest_alpha] = s[src_alpha];
      else
	d[dest_alpha] = OPAQUE_OPACITY;

      s += src_bytes;
      d += dest_bytes;
    }
}

static void
clone_line_pattern (GImage        *dest,
		    GimpDrawable  *drawable,
		    GPattern      *pattern,
		    unsigned char *d,
		    int            x,
		    int            y,
		    int            bytes,
		    int            width)
{
  unsigned char *pat, *p;
  int color, alpha;
  int i;

  /*  Make sure x, y are positive  */
  while (x < 0)
    x += pattern->mask->width;
  while (y < 0)
    y += pattern->mask->height;

  /*  Get a pointer to the appropriate scanline of the pattern buffer  */
  pat = temp_buf_data (pattern->mask) +
    (y % pattern->mask->height) * pattern->mask->width * pattern->mask->bytes;
  color = (pattern->mask->bytes == 3) ? RGB : GRAY;

  alpha = bytes - 1;

  for (i = 0; i < width; i++)
    {
      p = pat + ((i + x) % pattern->mask->width) * pattern->mask->bytes;

      gimage_transform_color (dest, drawable, p, d, color);

      d[alpha] = OPAQUE_OPACITY;

      d += bytes;
    }
}

static void *
clone_non_gui_paint_func (PaintCore *paint_core,
			  GimpDrawable *drawable,
			  int        state)
{
  clone_motion (paint_core, drawable, non_gui_src_drawable, &non_gui_pressure_options,
		non_gui_type, non_gui_offset_x, non_gui_offset_y);

  return NULL;
}

gboolean
clone_non_gui_default (GimpDrawable *drawable,
		       int           num_strokes,
		       double       *stroke_array)
{
  GimpDrawable *src_drawable = NULL;
  CloneType     clone_type = CLONE_DEFAULT_TYPE;
  double        local_src_x = 0.0;
  double        local_src_y = 0.0;
  CloneOptions *options = clone_options;
  
  if(options)
    {
      clone_type = options->type;
      src_drawable = src_drawable_;
      local_src_x = src_x;
      local_src_y = src_y;
    }
  
  return clone_non_gui(drawable,
		       src_drawable,
		       clone_type,
		       local_src_x,local_src_y,
		       num_strokes,stroke_array);
}

gboolean
clone_non_gui (GimpDrawable *drawable,
    	       GimpDrawable *src_drawable,
	       CloneType     clone_type,
	       double        src_x,
	       double        src_y,
	       int           num_strokes,
	       double       *stroke_array)
{
  int i;

  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = clone_non_gui_paint_func;
      
      non_gui_type = clone_type;

      non_gui_src_drawable = src_drawable;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      non_gui_offset_x = (int) (src_x - non_gui_paint_core.startx);
      non_gui_offset_y = (int) (src_y - non_gui_paint_core.starty);

      clone_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /* Finish the painting */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /* Cleanup */
      paint_core_cleanup ();
      return TRUE;
    }
  else
    return FALSE;
}
