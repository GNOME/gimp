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
#include "gimpbrushlist.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "interface.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "patterns.h"
#include "clone.h"
#include "selection.h"
#include "tools.h"

#include "libgimp/gimpintl.h"

#define TARGET_HEIGHT  15
#define TARGET_WIDTH   15

/*  the clone structures  */

typedef enum
{
  AlignNo,
  AlignYes,
  AlignRegister
} AlignType;

typedef struct _CloneOptions CloneOptions;
struct _CloneOptions
{
  CloneType  type;
  CloneType  type_d;
  GtkWidget *type_w;

  AlignType  aligned;
  AlignType  aligned_d;
  GtkWidget *aligned_w;
};

/*  clone tool options  */
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
static void         clone_draw            (Tool *);
static void         clone_motion          (PaintCore *, GimpDrawable *, GimpDrawable *, CloneType, int, int);
static void	    clone_line_image      (GImage *, GImage *, GimpDrawable *, GimpDrawable *, unsigned char *,
					   unsigned char *, int, int, int, int);
static void         clone_line_pattern    (GImage *, GimpDrawable *, GPatternP, unsigned char *,
					   int, int, int, int);

static Argument *   clone_invoker         (Argument *);


/*  functions  */

static void
clone_type_callback (GtkWidget *w,
		     gpointer   client_data)
{
  clone_options->type =(CloneType) client_data;
}

static void
align_type_callback (GtkWidget *w,
		     gpointer   client_data)
{
  clone_options->aligned =(AlignType) client_data;
}

static void
reset_clone_options (void)
{
  CloneOptions *options = clone_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->type_w), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->aligned_w), TRUE);
}

static CloneOptions *
create_clone_options (void)
{
  CloneOptions *options;
  GtkWidget    *vbox;
  GtkWidget    *radio_frame;
  GtkWidget    *radio_box;
  GtkWidget    *radio_button;
  GSList       *group = NULL;
  int           i;
  char *button_names[2] =
  {
    N_("Image Source"),
    N_("Pattern Source")
  };
  char *align_names[3] =
  {
    N_("Non Aligned"),
    N_("Aligned"),
    N_("Registered")
  };

  /*  the new options structure  */
  options = (CloneOptions *) g_malloc (sizeof (CloneOptions));
  options->type    = options->type_d    = ImageClone;
  options->aligned = options->aligned_d = AlignNo;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new (_("Source"));
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  for (i = 0; i < 2; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group,
						      gettext(button_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) clone_type_callback,
			  (void *)((long) i));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_widget_show (radio_button);

      if (i == options->type_d)
	options->type_w = radio_button;
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  /*  the radio frame and box  */
  radio_frame = gtk_frame_new (_("Alignment"));
  gtk_box_pack_start (GTK_BOX (vbox), radio_frame, FALSE, FALSE, 0);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

  /*  the radio buttons  */
  group = NULL;  
  for (i = 0; i < 3; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, align_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) align_type_callback,
			  (void *)((long) i));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_widget_show (radio_button);

      if (i == options->aligned_d)
	options->aligned_w = radio_button;
    }
  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);
  
  /*  Register this selection options widget with the main tools options dialog
   */
  tools_register (CLONE, vbox, _("Clone Tool Options"), reset_clone_options);

  return options;
}

static void
clone_src_drawable_destroyed_cb(GimpDrawable *drawable,
				GimpDrawable **src_drawable)
{
  if (drawable == *src_drawable)
  {
    *src_drawable = NULL;
    the_src_gdisp = NULL;
  }
}

static void
clone_set_src_drawable(GimpDrawable *drawable)
{
  if (src_drawable_ == drawable)
    return;
  if (src_drawable_)
    gtk_signal_disconnect_by_data(GTK_OBJECT (src_drawable_), &src_drawable_);
  src_drawable_ = drawable;
  if (drawable)
  {
    gtk_signal_connect (GTK_OBJECT (drawable),
			"destroy",
			GTK_SIGNAL_FUNC (clone_src_drawable_destroyed_cb),
			&src_drawable_);
  }
}


void *
clone_paint_func (PaintCore *paint_core,
		  GimpDrawable *drawable,
		  int        state)
{
  GDisplay * gdisp;
  GDisplay * src_gdisp;
  int x1, y1, x2, y2;
  static int orig_src_x, orig_src_y;

  gdisp = (GDisplay *) active_tool->gdisp_ptr;

  switch (state)
    {
    case MOTION_PAINT :
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

          if (clone_options->aligned == AlignRegister)
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

	  clone_motion (paint_core, drawable, src_drawable_, clone_options->type, offset_x, offset_y);
	}

      draw_core_pause (paint_core->core, active_tool);
      break;

    case INIT_PAINT :
      if (paint_core->state & GDK_CONTROL_MASK)
	{
	  the_src_gdisp = gdisp;
	  clone_set_src_drawable(drawable);
	  src_x = paint_core->curx;
	  src_y = paint_core->cury;
	  first = TRUE;
	}
      else if (clone_options->aligned == AlignNo)
      {
	first = TRUE;
	orig_src_x = src_x;
	orig_src_y = src_y;
      }
      if (clone_options->type == PatternClone)
	if (!get_active_pattern ())
	  g_message (_("No patterns available for this operation."));
      break;

    case FINISH_PAINT :
      draw_core_stop (paint_core->core, active_tool);
      if (clone_options->aligned == AlignNo && !first)
      {
	src_x = orig_src_x;
	src_y = orig_src_y;
      } 
      return NULL;
      break;

    default :
      break;
    }

  /*  Calculate the coordinates of the target  */
  src_gdisp = the_src_gdisp;
  if (!src_gdisp)
    {
      the_src_gdisp = gdisp;
      src_gdisp = the_src_gdisp;
    }

  /*  Find the target cursor's location onscreen  */
  gdisplay_transform_coords (src_gdisp, src_x, src_y, &trans_tx, &trans_ty, 1);

  if (state == INIT_PAINT)
    /*  Initialize the tool drawing core  */
    draw_core_start (paint_core->core,
		     src_gdisp->canvas->window,
		     active_tool);
  else if (state == MOTION_PAINT)
    draw_core_resume (paint_core->core, active_tool);

  return NULL;
}

Tool *
tools_new_clone ()
{
  Tool * tool;
  PaintCore * private;

  if (! clone_options)
    clone_options = create_clone_options ();

  tool = paint_core_new (CLONE);

  private = (PaintCore *) tool->private;
  private->paint_func = clone_paint_func;
  private->core->draw_func = clone_draw;

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

  if (clone_options->type == ImageClone)
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
clone_motion (PaintCore *paint_core,
	      GimpDrawable *drawable,
	      GimpDrawable *src_drawable,
	      CloneType  type,
	      int        offset_x,
	      int        offset_y)
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
  GPatternP pattern;

  pr      = NULL;
  pattern = NULL;

  /*  Make sure we still have a source if we are doing image cloning */
  if (type == ImageClone)
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

  /*  Get a region which can be used to paint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable)))
    return;

  switch (type)
    {
    case ImageClone:
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
	  x1 = BOUNDS (area->x + offset_x, 0, drawable_width (src_drawable));
	  y1 = BOUNDS (area->y + offset_y, 0, drawable_height (src_drawable));
	  x2 = BOUNDS (area->x + offset_x + area->width,
		       0, drawable_width (src_drawable));
	  y2 = BOUNDS (area->y + offset_y + area->height,
		       0, drawable_height (src_drawable));

	  if (!(x2 - x1) || !(y2 - y1))
	    return;

	  pixel_region_init (&srcPR, drawable_data (src_drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
	}
      else
	{
	  x1 = BOUNDS (area->x + offset_x, 0, drawable_width (drawable));
	  y1 = BOUNDS (area->y + offset_y, 0, drawable_height (drawable));
	  x2 = BOUNDS (area->x + offset_x + area->width,
		       0, drawable_width (drawable));
	  y2 = BOUNDS (area->y + offset_y + area->height,
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

    case PatternClone:
      pattern = get_active_pattern ();

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
	    case ImageClone:
	      clone_line_image (gimage, src_gimage, drawable, src_drawable, s, d,
				has_alpha, srcPR.bytes, destPR.bytes, destPR.w);
	      s += srcPR.rowstride;
	      break;
	    case PatternClone:
	      clone_line_pattern (gimage, drawable, pattern, d,
				  area->x + offset_x, area->y + y + offset_y,
				  destPR.bytes, destPR.w);
	      break;
	    }

	  d += destPR.rowstride;
	}
    }

  /*  paste the newly painted canvas to the gimage which is being worked on  */
  paint_core_paste_canvas (paint_core, drawable, OPAQUE_OPACITY,
			   (int) (gimp_brush_get_opacity () * 255),
			   gimp_brush_get_paint_mode (), SOFT, CONSTANT);
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
		    GPatternP      pattern,
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
  clone_motion (paint_core, drawable, non_gui_src_drawable,
		non_gui_type, non_gui_offset_x, non_gui_offset_y);

  return NULL;
}


/*  The clone procedure definition  */
ProcArg clone_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_DRAWABLE,
    "src_drawable",
    "the source drawable"
  },
  { PDB_INT32,
    "clone_type",
    "the type of clone: { IMAGE-CLONE (0), PATTERN-CLONE (1) }"
  },
  { PDB_FLOAT,
    "src_x",
    "the x coordinate in the source image"
  },
  { PDB_FLOAT,
    "src_y",
    "the y coordinate in the source image"
  },
  { PDB_INT32,
    "num_strokes",
    "number of stroke control points (count each coordinate as 2 points)"
  },
  { PDB_FLOATARRAY,
    "strokes",
    "array of stroke coordinates: {s1.x, s1.y, s2.x, s2.y, ..., sn.x, sn.y}"
  }
};


ProcRecord clone_proc =
{
  "gimp_clone",
  "Clone from the source to the dest drawable using the current brush",
  "This tool clones (copies) from the source drawable starting at the specified source coordinates to the dest drawable.  If the \"clone_type\" argument is set to PATTERN-CLONE, then the current pattern is used as the source and the \"src_drawable\" argument is ignored.  Pattern cloning assumes a tileable pattern and mods the sum of the src coordinates and subsequent stroke offsets with the width and height of the pattern.  For image cloning, if the sum of the src coordinates and subsequent stroke offsets exceeds the extents of the src drawable, then no paint is transferred.  The clone tool is capable of transforming between any image types including RGB->Indexed--although converting from any type to indexed is significantly slower.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  7,
  clone_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { clone_invoker } },
};


static Argument *
clone_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  GimpDrawable *src_drawable;
  double src_x, src_y;
  int num_strokes;
  double *stroke_array;
  int int_value;
  int i;
  
  drawable = NULL;
  num_strokes = 0;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL)                                        
        success = FALSE;
      else
        gimage = drawable_gimage (drawable);
    }
  /*  the src drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      src_drawable = drawable_get_ID (int_value);
      if (src_drawable == NULL || gimage != drawable_gimage (src_drawable))
	success = FALSE;
      else
	non_gui_src_drawable = src_drawable;
    }
  /*  the clone type  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      switch (int_value)
	{
	case 0: non_gui_type = ImageClone; break;
	case 1: non_gui_type = PatternClone; break;
	default: success = FALSE;
	}
    }
  /*  x, y offsets  */
  if (success)
    {
      src_x = args[3].value.pdb_float;
      src_y = args[4].value.pdb_float;
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

  /*  point array  */
  if (success)
    stroke_array = (double *) args[6].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_init (&non_gui_paint_core, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = clone_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      non_gui_offset_x = (int) (src_x - non_gui_paint_core.startx);
      non_gui_offset_y = (int) (src_y - non_gui_paint_core.starty);

      if (num_strokes == 1)
	clone_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /*  finish the painting  */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /*  cleanup  */
      paint_core_cleanup ();
    }

  return procedural_db_return_args (&clone_proc, success);
}
