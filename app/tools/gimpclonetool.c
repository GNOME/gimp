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

#include "paint-funcs/paint-funcs.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimppattern.h"
#include "core/gimpcontext.h"
#include "core/gimpbrush.h"

#include "gimpclonetool.h"
#include "paint_options.h"
#include "tool_manager.h"
#include "tool_options.h"

#include "drawable.h"
#include "gdisplay.h"
#include "gimpui.h"
#include "selection.h"

#include "libgimp/gimpintl.h"

#define WANT_CLONE_BITS
#include "icons.h"


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


/*  forward function declarations  */

static void       gimp_clone_tool_class_init   (GimpCloneToolClass *klass);
static void       gimp_clone_tool_init         (GimpCloneTool      *tool);

static void       gimp_clone_tool_paint        (GimpPaintTool  *paint_tool,
						GimpDrawable   *drawable,
						PaintState      state);
static void       gimp_clone_tool_draw         (GimpDrawTool   *draw_tool);
static void       gimp_clone_tool_cursor_update (GimpTool       *tool,
						 GdkEventMotion *mevent,
						 GDisplay       *gdisp);
static void       gimp_clone_tool_motion       (GimpPaintTool  *paint_tool,
						GimpDrawable   *drawable,
						GimpDrawable   *src_drawable,
						PaintPressureOptions *pressure_options,
						CloneType       type,
						gint            offset_x,
						gint            offset_y);
static void       gimp_clone_tool_line_image   (GimpImage      *dest,
						GimpImage      *src,
						GimpDrawable   *d_drawable,
						GimpDrawable   *s_drawable,
						guchar         *s,
						guchar         *d,
						gint            has_alpha,
						gint            src_bytes,
						gint            dest_bytes,
						gint            width);
static void       gimp_clone_tool_line_pattern (GimpImage      *dest,
						GimpDrawable   *drawable,
						GimpPattern    *pattern,
						guchar         *d,
						gint            x,
						gint            y,
						gint            bytes,
						gint            width);

static CloneOptions * clone_options_new        (void);
static void           clone_options_reset      (ToolOptions *options);


/* The parent class */
static GimpPaintToolClass *parent_class;
 
/*  the clone tool options  */
static CloneOptions *clone_options = NULL;

/*  local variables  */
static gint          src_x         = 0;        /*                         */
static gint          src_y         = 0;        /*  position of clone src  */
static gint          dest_x        = 0;        /*                         */
static gint          dest_y        = 0;        /*  position of clone src  */
static gint          offset_x      = 0;        /*                         */
static gint          offset_y      = 0;        /*  offset for cloning     */
static gint          first         = TRUE;
static gint          trans_tx      = 0;        /*                         */
static gint          trans_ty      = 0;        /*  transformed target     */
static GDisplay     *the_src_gdisp = NULL;     /*  ID of source gdisplay  */
static GimpDrawable *src_drawable_ = NULL;     /*  source drawable        */

static GimpDrawable *non_gui_src_drawable;
static gint          non_gui_offset_x;
static gint          non_gui_offset_y;
static CloneType     non_gui_type;


/* global functions  */

void
gimp_clone_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_CLONE_TOOL,
			      TRUE,
  			      "gimp:clone_tool",
  			      _("Clone"),
  			      _("Paint using Patterns or Image Regions"),
      			      N_("/Tools/Paint Tools/Clone"), "C",
  			      NULL, "tools/clone.html",
			      (const gchar **) clone_bits);
}

GtkType
gimp_clone_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpCloneTool",
        sizeof (GimpCloneTool),
        sizeof (GimpCloneToolClass),
        (GtkClassInitFunc) gimp_clone_tool_class_init,
        (GtkObjectInitFunc) gimp_clone_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        NULL
      };

      tool_type = gtk_type_unique (GIMP_TYPE_PAINT_TOOL, &tool_info);
    }

  return tool_type;
}

/* static functions  */

static void
gimp_clone_tool_class_init (GimpCloneToolClass *klass)
{
  GimpPaintToolClass *paint_tool_class;
  GimpDrawToolClass  *draw_tool_class;
  GimpToolClass      *tool_class;

  paint_tool_class = (GimpPaintToolClass *) klass;
  draw_tool_class  = (GimpDrawToolClass *) klass;
  tool_class       = (GimpToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PAINT_TOOL);

  tool_class->cursor_update = gimp_clone_tool_cursor_update;

  draw_tool_class->draw     = gimp_clone_tool_draw;
  paint_tool_class->paint   = gimp_clone_tool_paint;
}

static void
gimp_clone_tool_init (GimpCloneTool *clone)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (clone);
  paint_tool = GIMP_PAINT_TOOL (clone);

  if (! clone_options)
    {
      clone_options = clone_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_CLONE_TOOL,
                                          (ToolOptions *) clone_options);
    }

  tool->tool_cursor = GIMP_CLONE_TOOL_CURSOR;

  paint_tool->pick_colors =  TRUE;
  paint_tool->flags       |= TOOL_CAN_HANDLE_CHANGING_BRUSH;
  paint_tool->flags       |= TOOL_TRACES_ON_WINDOW;
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

static void
gimp_clone_tool_paint (GimpPaintTool    *paint_tool,
		       GimpDrawable *drawable,
		       PaintState    state)
{
  GDisplay    *gdisp;
  GDisplay    *src_gdisp;
  gint         x1, y1, x2, y2;
  static gint  orig_src_x, orig_src_y;
  GimpDrawTool *draw_tool;

  gdisp = (GDisplay *) active_tool->gdisp;
  draw_tool = GIMP_DRAW_TOOL(paint_tool);

  switch (state)
    {
    case PRETRACE_PAINT:
      gimp_draw_tool_pause (draw_tool);
      break;

    case MOTION_PAINT:
      x1 = paint_tool->curx;
      y1 = paint_tool->cury;
      x2 = paint_tool->lastx;
      y2 = paint_tool->lasty;

      /*  If the control key is down, move the src target and return */
      if (paint_tool->state & GDK_CONTROL_MASK)
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

	  gimp_clone_tool_motion (paint_tool, drawable, src_drawable_, 
				  clone_options->paint_options.pressure_options, 
				  clone_options->type, offset_x, offset_y);
	}

      break;

    case INIT_PAINT:
      if (paint_tool->state & GDK_CONTROL_MASK)
	{
	  the_src_gdisp = gdisp;
	  clone_set_src_drawable(drawable);
	  src_x = paint_tool->curx;
	  src_y = paint_tool->cury;
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
      gimp_draw_tool_stop (draw_tool);
      if (clone_options->aligned == ALIGN_NO && !first)
	{
	  src_x = orig_src_x;
	  src_y = orig_src_y;
	} 
      return;
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
    gimp_draw_tool_start (draw_tool,
			  src_gdisp->canvas->window);
  else if (state == POSTTRACE_PAINT)
    {
      /*  Find the target cursor's location onscreen  */
      gdisplay_transform_coords (src_gdisp, src_x, src_y,
				 &trans_tx, &trans_ty, 1);
      gimp_draw_tool_resume (draw_tool);      
    }
}

void
gimp_clone_tool_cursor_update (GimpTool       *tool,
			       GdkEventMotion *mevent,
			       GDisplay       *gdisp)
{
  GimpLayer     *layer;
  GdkCursorType  ctype = GDK_TOP_LEFT_ARROW;
  gint           x, y;

  gdisplay_untransform_coords (gdisp, (double) mevent->x, (double) mevent->y,
			       &x, &y, TRUE, FALSE);
 
  if ((layer = gimp_image_get_active_layer (gdisp->gimage))) 
    {
      int off_x, off_y;

      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

      if (x >= off_x && y >= off_y &&
	  x < (off_x + gimp_drawable_width (GIMP_DRAWABLE (layer))) &&
	  y < (off_y + gimp_drawable_height (GIMP_DRAWABLE (layer))))
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

  gdisplay_install_tool_cursor (gdisp,
				ctype,
				ctype == GIMP_CROSSHAIR_SMALL_CURSOR ?
				GIMP_TOOL_CURSOR_NONE :
				GIMP_CLONE_TOOL_CURSOR,
				GIMP_CURSOR_MODIFIER_NONE);
}

static void
gimp_clone_tool_draw (GimpDrawTool *draw_tool)
{
  if (draw_tool->gc != NULL && clone_options->type == IMAGE_CLONE)
    {
      gdk_draw_line (draw_tool->win, draw_tool->gc,
		     trans_tx - (TARGET_WIDTH >> 1), trans_ty,
		     trans_tx + (TARGET_WIDTH >> 1), trans_ty);
      gdk_draw_line (draw_tool->win, draw_tool->gc,
		     trans_tx, trans_ty - (TARGET_HEIGHT >> 1),
		     trans_tx, trans_ty + (TARGET_HEIGHT >> 1));
    }
}

static void
gimp_clone_tool_motion (GimpPaintTool        *paint_tool,
			GimpDrawable         *drawable,
			GimpDrawable         *src_drawable,
			PaintPressureOptions *pressure_options,
			CloneType             type,
			int                   offset_x,
			int                   offset_y)
{
  GimpImage   *gimage;
  GimpImage   *src_gimage = NULL;
  guchar      *s;
  guchar      *d;
  TempBuf     *orig;
  TempBuf     *area;
  gpointer     pr;
  gint         y;
  gint         x1, y1, x2, y2;
  gint         has_alpha = -1;
  PixelRegion  srcPR, destPR;
  GimpPattern *pattern;
  gint         opacity;
  gdouble      scale;

  pr      = NULL;
  pattern = NULL;

  /*  Make sure we still have a source if we are doing image cloning */
  if (type == IMAGE_CLONE)
    {
      if (!src_drawable)
	return;
      if (! (src_gimage = gimp_drawable_gimage (src_drawable)))
	return;
      /*  Determine whether the source image has an alpha channel  */
      has_alpha = gimp_drawable_has_alpha (src_drawable);
    }

  /*  We always need a destination image */
  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  if (pressure_options->size)
    scale = paint_tool->curpressure;
  else
    scale = 1.0;

  /*  Get a region which can be used to paint to  */
  if (! (area = gimp_paint_tool_get_paint_area (paint_tool, drawable, scale)))
    return;

  switch (type)
    {
    case IMAGE_CLONE:
      /*  Set the paint area to transparent  */
      temp_buf_data_clear (area);

      /*  If the source gimage is different from the destination,
       *  then we should copy straight from the destination image
       *  to the canvas.
       *  Otherwise, we need a call to get_orig_image to make sure
       *  we get a copy of the unblemished (offset) image
       */
      if (src_drawable != drawable)
	{
	  x1 = CLAMP (area->x + offset_x, 0, gimp_drawable_width (src_drawable));
	  y1 = CLAMP (area->y + offset_y, 0, gimp_drawable_height (src_drawable));
	  x2 = CLAMP (area->x + offset_x + area->width,
		      0, gimp_drawable_width (src_drawable));
	  y2 = CLAMP (area->y + offset_y + area->height,
		      0, gimp_drawable_height (src_drawable));

	  if (!(x2 - x1) || !(y2 - y1))
	    return;

	  pixel_region_init (&srcPR, gimp_drawable_data (src_drawable),
			     x1, y1, (x2 - x1), (y2 - y1), FALSE);
	}
      else
	{
	  x1 = CLAMP (area->x + offset_x, 0, gimp_drawable_width (drawable));
	  y1 = CLAMP (area->y + offset_y, 0, gimp_drawable_height (drawable));
	  x2 = CLAMP (area->x + offset_x + area->width,
		      0, gimp_drawable_width (drawable));
	  y2 = CLAMP (area->y + offset_y + area->height,
		      0, gimp_drawable_height (drawable));

	  if (!(x2 - x1) || !(y2 - y1))
	    return;

	  /*  get the original image  */
	  orig = gimp_paint_tool_get_orig_image (paint_tool, drawable, x1, y1, x2, y2);

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
	      gimp_clone_tool_line_image (gimage, src_gimage, drawable, 
					  src_drawable, s, d, has_alpha, 
					  srcPR.bytes, destPR.bytes, destPR.w);
	      s += srcPR.rowstride;
	      break;
	    case PATTERN_CLONE:
	      gimp_clone_tool_line_pattern (gimage, drawable, pattern, d,
					    area->x + offset_x, 
					    area->y + y + offset_y,
					    destPR.bytes, destPR.w);
	      break;
	    }

	  d += destPR.rowstride;
	}
    }

  opacity = 255.0 * gimp_context_get_opacity (NULL);
  if (pressure_options->opacity)
    opacity = opacity * 2.0 * paint_tool->curpressure;

  /*  paste the newly painted canvas to the gimage which is being worked on  */
  gimp_paint_tool_paste_canvas (paint_tool, drawable, 
				MIN (opacity, 255),
				(gint) (gimp_context_get_opacity (NULL) * 255),
				gimp_context_get_paint_mode (NULL),
				pressure_options->pressure ? PRESSURE : SOFT, 
				scale, CONSTANT);
}


static void
gimp_clone_tool_line_image (GimpImage    *dest,
			    GimpImage    *src,
			    GimpDrawable *d_drawable,
			    GimpDrawable *s_drawable,
			    guchar       *s,
			    guchar       *d,
			    gint          has_alpha,
			    gint          src_bytes,
			    gint          dest_bytes,
			    gint          width)
{
  guchar rgb[3];
  gint   src_alpha, dest_alpha;

  src_alpha  = src_bytes - 1;
  dest_alpha = dest_bytes - 1;

  while (width--)
    {
      gimp_image_get_color (src, gimp_drawable_type (s_drawable), rgb, s);
      gimp_image_transform_color (dest, d_drawable, rgb, d, RGB);

      if (has_alpha)
	d[dest_alpha] = s[src_alpha];
      else
	d[dest_alpha] = OPAQUE_OPACITY;

      s += src_bytes;
      d += dest_bytes;
    }
}

static void
gimp_clone_tool_line_pattern (GimpImage    *dest,
			      GimpDrawable *drawable,
			      GimpPattern  *pattern,
			      guchar       *d,
			      gint          x,
			      gint          y,
			      gint          bytes,
			      gint          width)
{
  guchar *pat, *p;
  gint    color, alpha;
  gint    i;

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

      gimp_image_transform_color (dest, drawable, p, d, color);

      d[alpha] = OPAQUE_OPACITY;

      d += bytes;
    }
}

#if 0 /* leave these to the stub functions. */

static gpointer
gimp_clone_tool_non_gui_paint_func (GimpPaintTool    *paint_tool,
				    GimpDrawable     *drawable,
				    PaintState        state)
{
  gimp_clone_tool_motion (paint_tool, drawable, non_gui_src_drawable,
			  &non_gui_pressure_options,
			  non_gui_type, non_gui_offset_x, non_gui_offset_y);

  return NULL;
}

gboolean
gimp_clone_tool_non_gui_default (GimpDrawable *drawable,
				 gint          num_strokes,
				 gdouble      *stroke_array)
{
  GimpDrawable *src_drawable = NULL;
  CloneType     clone_type = CLONE_DEFAULT_TYPE;
  gdouble       local_src_x = 0.0;
  gdouble       local_src_y = 0.0;
  CloneOptions *options = clone_options;

  if (options)
    {
      clone_type = options->type;
      src_drawable = src_drawable_;
      local_src_x = src_x;
      local_src_y = src_y;
    }
  
  return gimp_clone_tool_non_gui (drawable,
				  src_drawable,
				  clone_type,
				  local_src_x,local_src_y,
				  num_strokes, stroke_array);
}

gboolean
gimp_clone_tool_non_gui (GimpDrawable *drawable,
			 GimpDrawable *src_drawable,
			 CloneType     clone_type,
			 gdouble       src_x,
			 gdouble       src_y,
			 gint          num_strokes,
			 gdouble      *stroke_array)
{
  gint i;

  if (gimp_paint_tool_start (&non_gui_paint_tool, drawable,
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

#endif /* 0 - non-gui functions */

static CloneOptions *
clone_options_new (void)
{
  CloneOptions *options;
  GtkWidget    *vbox;
  GtkWidget    *frame;

  /*  the new clone tool options structure  */
  options = g_new (CloneOptions, 1);
  paint_options_init ((PaintOptions *) options,
		      GIMP_TYPE_CLONE_TOOL,
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
clone_options_reset (ToolOptions *tool_options)
{
  CloneOptions *options;

  options = (CloneOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w[options->type_d]), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->aligned_w[options->aligned_d]), TRUE);
}
