/* vpropagate.c -- This is a plug-in for the GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-01-09 15:50:46 yasuhiro>
 * Version: 0.89a
 *
 * Copyright (C) 1996-1997 Shuji Narazaki <narazaki@InetQ.or.jp>
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

/* memo
   the initial value of each pixel is the value of the pixel itself.
   To determine whether it is an isolated local peak point, use:
   (self == min && (! modified_flag))   ; modified_flag holds history of update
   In other word, pixel itself is not a neighbor of it.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define	PLUG_IN_NAME	"plug_in_vpropagate"
#define SHORT_NAME	"vpropagate"

typedef guchar CH;
#define	VP_RGB	        (1 << 0)
#define VP_GRAY	        (1 << 1)
#define VP_WITH_ALPHA	(1 << 2)
#define VP_WO_ALPHA	(1 << 3)
#define num_direction	4
#define Right2Left	0
#define Bottom2Top	1
#define	Left2Right	2
#define Top2Bottom	3

static void	    query	               (void);
static void	    run                        (gchar         *name,
					        gint           nparams,
					        GimpParam        *param,   
                                                gint          *nreturn_vals,
					        GimpParam      **return_vals);

static GimpPDBStatusType  value_propagate            (gint           drawable_id);
static void         value_propagate_body       (gint           drawable_id);
static int	    vpropagate_dialog          (GimpImageBaseType     image_type);
static void	    prepare_row                (GimpPixelRgn     *pixel_rgn,
						guchar        *data,
						int            x,
						int            y,
						int            w);

static void         vpropagate_ok_callback     (GtkWidget     *widget,
						gpointer       data);
static GtkWidget *  gtk_table_add_toggle       (GtkWidget     *table,
						gchar	      *name,
						gint	       x1,
						gint	       x2,
						gint	       y,
						GtkSignalFunc  update,
						gint	      *value);

static int	    value_difference_check  (CH *, CH *, int);
static void         set_value               (GimpImageBaseType, int, CH *, CH *, CH *, void *);
static void	    initialize_white        (GimpImageBaseType, int, CH *, CH *, void **);
static void	    propagate_white         (GimpImageBaseType, int, CH *, CH *, CH *, void *);
static void	    initialize_black        (GimpImageBaseType, int, CH *, CH *, void **);
static void         propagate_black         (GimpImageBaseType, int, CH *, CH *, CH *, void *);
static void	    initialize_middle       (GimpImageBaseType, int, CH *, CH *, void **);
static void	    propagate_middle        (GimpImageBaseType, int, CH *, CH *, CH *, void *);
static void	    set_middle_to_peak      (GimpImageBaseType, int, CH *, CH *, CH *, void *);
static void	    set_foreground_to_peak  (GimpImageBaseType, int, CH *, CH *, CH *, void *);
static void	    initialize_foreground   (GimpImageBaseType, int, CH *, CH *, void **);
static void	    initialize_background   (GimpImageBaseType, int, CH *, CH *, void **);
static void	    propagate_a_color       (GimpImageBaseType, int, CH *, CH *, CH *, void *);
static void	    propagate_opaque        (GimpImageBaseType, int, CH *, CH *, CH *, void *);
static void	    propagate_transparent   (GimpImageBaseType, int, CH *, CH *, CH *, void *);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

#define UPDATE_STEP	    20
#define SCALE_WIDTH	   100
#define PROPAGATING_VALUE  1<<0
#define PROPAGATING_ALPHA  1<<1

/* parameters */
typedef struct
{
  gint		propagate_mode;
  gint		propagating_channel;
  gdouble	propagating_rate;
  gint		direction_mask;
  gint		lower_limit;
  gint		upper_limit;
  /* gint	channel_mask[3]; */
  /* gint	peak_includes_equals; */
  /* gint	overwrite; */
} VPValueType;

static VPValueType vpvals = 
{
  0,
  3,				/* PROPAGATING_VALUE + PROPAGATING_ALPHA */
  1.0,				/* [0.0:1.0] */
  15,				/* propagate to all 4 directions */
  0,
  255
};

/* dialog vairables */
static gint	propagate_alpha;
static gint	propagate_value;
static gint	direction_mask_vec[4];
static gint	channel_mask[] = { 1, 1, 1 };
static gint	peak_max = 1;
static gint	peak_min = 1;
static gint	peak_includes_equals = 1;
static guchar	fore[3];

typedef struct
{
  gint    applicable_image_type;
  gchar  *name;
  void  (*initializer) ();
  void  (*updater) ();
  void  (*finalizer) ();
} ModeParam;

#define num_mode 8
static ModeParam modes[num_mode] = 
{
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("More White (Larger Value)"),
    initialize_white,      propagate_white,       set_value },
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("More Black (Smaller Value)"),
    initialize_black,      propagate_black,       set_value },
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("Middle Value to Peaks"),
    initialize_middle,     propagate_middle,      set_middle_to_peak },
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("Foreground to Peaks"),
    initialize_middle,     propagate_middle,      set_foreground_to_peak },
  { VP_RGB | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("Only Foreground"),
    initialize_foreground, propagate_a_color,     set_value },
  { VP_RGB | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("Only Background"),
    initialize_background, propagate_a_color,     set_value },
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA,
    N_("More Opaque"),
    NULL,                  propagate_opaque,      set_value },
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA,
    N_("More Transparent"),
    NULL,                  propagate_transparent, set_value },
};

typedef struct 
{
  gint run;
} VPInterface;

static VPInterface vpropagate_interface = { FALSE };
static gint	   drawable_id;

MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (not used)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32, "propagate-mode", "propagate 0:white, 1:black, 2:middle value 3:foreground to peak, 4:foreground, 5:background, 6:opaque, 7:transparent" },
    { GIMP_PDB_INT32, "propagating-channel", "channels which values are propagated" },
    { GIMP_PDB_FLOAT, "propagating-rate", "0.0 <= propagatating_rate <= 1.0" },
    { GIMP_PDB_INT32, "direction-mask", "0 <= direction-mask <= 15" },
    { GIMP_PDB_INT32, "lower-limit", "0 <= lower-limit <= 255" },
    { GIMP_PDB_INT32, "upper-limit", "0 <= upper-limit <= 255" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure (PLUG_IN_NAME,
			  "Propagate values of the layer",
			  "Propagate values of the layer",
			  "Shuji Narazaki (narazaki@InetQ.or.jp)",
			  "Shuji Narazaki",
			  "1996-1997",
			  N_("<Image>/Filters/Distorts/Value Propagate..."),
			  "RGB*,GRAY*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam	 values[1];
  GimpPDBStatusType	status = GIMP_PDB_SUCCESS;
  GimpRunModeType	run_mode;
  
  run_mode = param[0].data.d_int32;
  drawable_id = param[2].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data (PLUG_IN_NAME, &vpvals);
      /* building the values of dialog variables from vpvals. */
      propagate_alpha =
	(vpvals.propagating_channel & PROPAGATING_ALPHA) ? TRUE : FALSE;
      propagate_value =
	(vpvals.propagating_channel & PROPAGATING_VALUE) ? TRUE : FALSE;
      {
	int	i;
	for (i = 0; i < 4; i++)
	  direction_mask_vec[i] = (vpvals.direction_mask & (1 << i))
	    ? TRUE : FALSE;
      }
      if (! vpropagate_dialog (gimp_drawable_type(drawable_id)))
	return;
      break;
    case GIMP_RUN_NONINTERACTIVE:
      INIT_I18N();
      vpvals.propagate_mode = param[3].data.d_int32;
      vpvals.propagating_channel = param[4].data.d_int32;
      vpvals.propagating_rate = param[5].data.d_float;
      vpvals.direction_mask = param[6].data.d_int32;
      vpvals.lower_limit = param[7].data.d_int32;
      vpvals.upper_limit = param[8].data.d_int32;
      break;
    case GIMP_RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data (PLUG_IN_NAME, &vpvals);
      break;
    }
  
  status = value_propagate (drawable_id);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if (run_mode == GIMP_RUN_INTERACTIVE && status == GIMP_PDB_SUCCESS)
    gimp_set_data (PLUG_IN_NAME, &vpvals, sizeof (VPValueType));

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

/* registered function entry */
static GimpPDBStatusType
value_propagate (gint drawable_id)
{
  /* check the validness of parameters */
  if (!(vpvals.propagating_channel & (PROPAGATING_VALUE | PROPAGATING_ALPHA)))
    {
      /* gimp_message ("No channel selected."); */
      return GIMP_PDB_EXECUTION_ERROR;
    }
  if (vpvals.direction_mask == 0)
    {
      /* gimp_message ("No direction selected."); */
      return GIMP_PDB_EXECUTION_ERROR;
    }
  if ((vpvals.lower_limit < 0) || (255 < vpvals.lower_limit) ||
       (vpvals.upper_limit < 0) || (255 < vpvals.lower_limit) ||
       (vpvals.upper_limit < vpvals.lower_limit))
    {
      /* gimp_message ("Limit values are not valid."); */
      return GIMP_PDB_EXECUTION_ERROR;
    }
  value_propagate_body (drawable_id);
  return GIMP_PDB_SUCCESS;
}

static void
value_propagate_body (gint drawable_id)
{
  GimpDrawable  *drawable;
  GimpImageBaseType  dtype;
  ModeParam   operation;
  GimpPixelRgn   srcRgn, destRgn;
  guchar     *here, *best, *dest;
  guchar     *dest_row, *prev_row, *cur_row, *next_row, *pr, *cr, *nr, *swap;
  gint        width, height, bytes, index;
  gint        begx, begy, endx, endy, x, y, dx;
  gint        left_index, right_index, up_index, down_index;
  void       *tmp;

  /* calculate neighbors' indexes */
  left_index	= (vpvals.direction_mask & (1 << Left2Right)) ? -1 : 0;
  right_index	= (vpvals.direction_mask & (1 << Right2Left)) ?  1 : 0;
  up_index	= (vpvals.direction_mask & (1 << Top2Bottom)) ? -1 : 0;
  down_index	= (vpvals.direction_mask & (1 << Bottom2Top)) ?  1 : 0;
  operation = modes[vpvals.propagate_mode];
  tmp = NULL;

  drawable = gimp_drawable_get (drawable_id);
  dtype = gimp_drawable_type (drawable_id);

  /* Here I use the algorithm of blur.c . */
  gimp_drawable_mask_bounds (drawable->id, &begx, &begy, &endx, &endy);

  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  prev_row = g_new (guchar, (endx - begx + 2) * bytes);
  cur_row  = g_new (guchar, (endx - begx + 2) * bytes);
  next_row = g_new (guchar, (endx - begx + 2) * bytes);
  dest_row = g_new (guchar, (endx - begx) * bytes);

  gimp_pixel_rgn_init (&srcRgn, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destRgn, drawable, 0, 0, width, height, TRUE, TRUE);

  pr = prev_row + bytes;
  cr = cur_row + bytes;
  nr = next_row + bytes;

  prepare_row (&srcRgn, pr, begx, (0 < begy) ? begy : begy - 1, endx-begx);
  prepare_row (&srcRgn, cr, begx, begy, endx-begx);

  best = g_new (guchar, bytes);

  gimp_progress_init (_("Value propagating..."));
  gimp_palette_get_foreground (fore+0, fore+1, fore+2);

  /* start real job */
  for (y = begy ; y < endy ; y++) 
    {
      prepare_row (&srcRgn, nr, begx, ((y+1) < endy) ? y+1 : endy, endx-begx);
      for (index = 0; index < (endx - begx) * bytes; index++)
	dest_row[index] = cr[index];
      for (x = 0 ; x < endx - begx; x++) 
	{
	  dest = dest_row + (x * bytes);
	  here = cr + (x * bytes);
	  /* *** copy source value to best value holder *** */
	  memcpy ((void *)best, (void *)here, bytes);
	  if (operation.initializer)
	    (* operation.initializer)(dtype, bytes, best, here, &tmp);
	  /* *** gather neighbors' values: loop-unfolded version *** */
	  if (up_index == -1)
	    for (dx = left_index ; dx <= right_index ; dx++)
	      (* operation.updater)(dtype, bytes, here, pr+((x+dx)*bytes), best, tmp);
	  for (dx = left_index ; dx <= right_index ; dx++)
	    if (dx != 0)
	      (* operation.updater)(dtype, bytes, here, cr+((x+dx)*bytes), best, tmp);
	  if (down_index == 1)
	    for (dx = left_index ; dx <= right_index ; dx++)
	      (* operation.updater)(dtype, bytes, here, nr+((x+dx)*bytes), best, tmp);
	  /* *** store it to dest_row*** */
	  (* operation.finalizer)(dtype, bytes, best, here, dest, tmp);
	}
      /* now store destline to destRgn */
      gimp_pixel_rgn_set_row (&destRgn, dest_row, begx, y, endx - begx);
      /* shift the row pointers  */
      swap = pr;
      pr = cr;
      cr = nr;
      nr = swap;
      /* update each UPDATE_STEP (optimizer must generate cheap code) */ 
      if ((y % 5) == 0) /*(y % (int) ((endy - begy) / UPDATE_STEP)) == 0 */
	gimp_progress_update ((double)y / (double)(endy - begy));
    }
  /*  update the region  */
  gimp_progress_update(1.0);
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, begx, begy, endx-begx, endy-begy);
}

static void
prepare_row (GimpPixelRgn *pixel_rgn,
	     guchar    *data,
	     gint       x,
	     gint       y,
	     gint       w)
{
  gint b;

  if (y == 0)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (y + 1), w);
  else if (y == pixel_rgn->h)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (y - 1), w);
  else
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, y, w);

  /*  Fill in edge pixels  */
  for (b = 0; b < pixel_rgn->bpp; b++)
    {
      data[-(gint)pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

static void
set_value (GimpImageBaseType  dtype, 
	   gint        bytes, 
	   guchar     *best, 
	   guchar     *here, 
	   guchar     *dest, 
	   void       *tmp)
{
  gint	value_chs = 0;
  gint	alpha = 0;
  gint	ch;

  switch (dtype)
    {
    case GIMP_RGB_IMAGE:
      value_chs = 3;
      break;
    case GIMP_RGBA_IMAGE:
      value_chs = 3;
      alpha = 3;
      break;
    case GIMP_GRAY_IMAGE:
      value_chs = 1;
      break;
    case GIMP_GRAYA_IMAGE:
      value_chs = 1;
      alpha = 1;
      break;
    default:
      break;
    }
  for (ch = 0; ch < value_chs; ch++)
    {
      if (vpvals.propagating_channel & PROPAGATING_VALUE) /* value channel */
	*dest++ = (CH)(vpvals.propagating_rate * best[ch]
		       + (1.0 - vpvals.propagating_rate) * here[ch]);
      else
	*dest++ = here[ch];
    }
  if (alpha)
    {
      if (vpvals.propagating_channel & PROPAGATING_ALPHA) /* alpha channel */
	*dest++ = (CH)(vpvals.propagating_rate * best[alpha]
		       + (1.0 - vpvals.propagating_rate) * here[alpha]);
      else
	*dest++ = here[alpha];
    }
}

static int
value_difference_check (CH  *pos1, 
			CH  *pos2, 
			gint ch)
{
  gint	index;
  int	diff;

  for (index = 0 ; index < ch; index++)
    if (channel_mask[index] != 0)
      {
	diff = abs(pos1[index] - pos2[index]);
	if (! ((vpvals.lower_limit <= diff) && (diff <= vpvals.upper_limit)))
	  return 0;
      }
  return 1;
}

/* mothods for each mode */
static void
initialize_white (GimpImageBaseType   dtype, 
		  gint         bytes, 
		  CH          *best, 
		  CH          *here, 
		  void       **tmp)
{
  if (*tmp == NULL)
    *tmp = (void *) g_new (gfloat, 1);
  *(float *)*tmp = sqrt (channel_mask[0] * here[0] * here[0] 
			 + channel_mask[1] * here[1] * here[1] 
			 + channel_mask[2] * here[2] * here[2]);
}

static void
propagate_white (GimpImageBaseType  dtype, 
		 gint        bytes, 
		 CH         *orig, 
		 CH         *here, 
		 CH         *best, 
		 void       *tmp)
{
  float	v_here;

  switch (dtype)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      v_here = sqrt (channel_mask[0] * here[0] * here[0] 
		     + channel_mask[1] * here[1] * here[1] 
		     + channel_mask[2] * here[2] * here[2]);
     if (*(float *)tmp < v_here && value_difference_check(orig, here, 3))
	{
	  *(float *)tmp = v_here;
	  memcpy(best, here, 3 * sizeof(CH)); /* alpha channel holds old value */
	}
      break;
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      if (*best < *here && value_difference_check(orig, here, 1))
	*best = *here;
      break;
    default:
      break;
    }
}

static void
initialize_black (GimpImageBaseType   dtype, 
		  gint         channels, 
		  CH          *best, 
		  CH          *here, 
		  void       **tmp)
{
  if (*tmp == NULL)
    *tmp = (void *) g_new (gfloat, 1);
  *(float *)*tmp = sqrt (channel_mask[0] * here[0] * here[0] 
			 + channel_mask[1] * here[1] * here[1] 
			 + channel_mask[2] * here[2] * here[2]);
}

static void
propagate_black (GimpImageBaseType  image_type, 
		 gint        channels, 
		 CH         *orig, 
		 CH         *here, 
		 CH         *best, 
		 void       *tmp)
{
  float	v_here;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      v_here = sqrt (channel_mask[0] * here[0] * here[0] 
		     + channel_mask[1] * here[1] * here[1] 
		     + channel_mask[2] * here[2] * here[2]);
      if (v_here < *(float *)tmp && value_difference_check(orig, here, 3))
	{	
	  *(float *)tmp = v_here;
	  memcpy (best, here, 3 * sizeof(CH)); /* alpha channel holds old value */
	}
      break;
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      if (*here < *best && value_difference_check(orig, here, 1))
	*best = *here;
      break;
    default:
      break;
    }
}

typedef struct
{
  gshort min_modified;
  gshort max_modified;
  glong  original_value;
  glong  minv;
  CH     min[3];
  glong  maxv;
  CH     max[3];
} MiddlePacket;

static void
initialize_middle (GimpImageBaseType   image_type, 
		   gint         channels, 
		   CH          *best, 
		   CH          *here, 
		   void       **tmp)
{
  int index;
  MiddlePacket *data;

  if (*tmp == NULL)
    *tmp = (void *) g_new (MiddlePacket, 1);
  data = (MiddlePacket *)*tmp;
  for (index = 0; index < channels; index++)
    data->min[index] = data->max[index] = here[index];
  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      data->original_value = sqrt (channel_mask[0] * here[0] * here[0]
				   + channel_mask[1] * here[1] * here[1] 
				   + channel_mask[2] * here[2] * here[2]);
      break;
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      data->original_value = *here;
      break;
    default:
      break;
    }
  data->minv = data->maxv = data->original_value;
  data->min_modified = data->max_modified = 0;
}
  
static void
propagate_middle (GimpImageBaseType  image_type, 
		  gint        channels, 
		  CH         *orig, 
		  CH         *here, 
		  CH         *best, 
		  void       *tmp)
{
  float	v_here;
  MiddlePacket *data;

  data = (MiddlePacket *)tmp;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      v_here = sqrt (channel_mask[0] * here[0] * here[0] 
		     + channel_mask[1] * here[1] * here[1] 
		     + channel_mask[2] * here[2] * here[2]);
      if ((v_here <= data->minv) && value_difference_check(orig, here, 3))
	{	
	  data->minv = v_here;
	  memcpy (data->min, here, 3*sizeof(CH));
	  data->min_modified = 1;
	}
      if (data->maxv <= v_here && value_difference_check(orig, here, 3))
	{	
	  data->maxv = v_here;
	  memcpy (data->max, here, 3*sizeof(CH));
	  data->max_modified = 1;
	}
      break;
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      if ((*here <= data->min[0]) && value_difference_check(orig, here, 1))
	{
	  data->min[0] = *here;
	  data->min_modified = 1;
	}
      if ((data->max[0] <= *here) && value_difference_check(orig, here, 1))
	{
	  data->max[0] = *here;
	  data->max_modified = 1;
	}
      break;
    default:
      break;
    }
}

static void
set_middle_to_peak (GimpImageBaseType  image_type, 
		    gint        channels, 
		    CH         *here, 
		    CH         *best, 
		    CH         *dest, 
		    void       *tmp)
{
  gint	value_chs = 0;
  gint	alpha = 0;
  gint	ch;
  MiddlePacket	*data;

  data = (MiddlePacket *)tmp;
  if (! ((peak_min & (data->minv == data->original_value))
	 || (peak_max & (data->maxv == data->original_value))))
    return;
  if ((! peak_includes_equals)
      && ((peak_min & (! data->min_modified))
	  || (peak_max & (! data->max_modified))))
      return;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
      value_chs = 3;
      break;
    case GIMP_RGBA_IMAGE:
      value_chs = 3;
      alpha = 3;
      break;
    case GIMP_GRAY_IMAGE:
      value_chs = 1;
      break;
    case GIMP_GRAYA_IMAGE:
      value_chs = 1;
      alpha = 1;
      break;
    default:
      break;
    }
  for (ch = 0; ch < value_chs; ch++)
    {
      if (vpvals.propagating_channel & PROPAGATING_VALUE) /* value channel */
	*dest++ = (CH)(vpvals.propagating_rate * 0.5 * (data->min[ch] + data->max[ch])
		       + (1.0 - vpvals.propagating_rate) * here[ch]);
      else
	*dest++ = here[ch];
    }
  if (alpha)
    {
      if (vpvals.propagating_channel & PROPAGATING_ALPHA) /* alpha channel */
	*dest++ = (CH)(vpvals.propagating_rate * best[alpha]
		       + (1.0 - vpvals.propagating_rate) * here[alpha]);
      else
	*dest++ = here[alpha];
    }
}

static void
set_foreground_to_peak (GimpImageBaseType  image_type, 
			gint        channels, 
			CH         *here, 
			CH         *best, 
			CH         *dest, 
			void       *tmp)
{
  gint	value_chs = 0;
  gint	alpha = 0;
  gint	ch;
  MiddlePacket	*data;

  data = (MiddlePacket *)tmp;
  if (! ((peak_min & (data->minv == data->original_value))
	 || (peak_max & (data->maxv == data->original_value))))
    return;
  if (peak_includes_equals
      && ((peak_min & (! data->min_modified))
	  || (peak_max & (! data->max_modified))))
      return;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
      value_chs = 3;
      break;
    case GIMP_RGBA_IMAGE:
      value_chs = 3;
      alpha = 3;
      break;
    case GIMP_GRAY_IMAGE:
      value_chs = 1;
      break;
    case GIMP_GRAYA_IMAGE:
      value_chs = 1;
      alpha = 1;
      break;
    default:
      break;
    }
  for (ch = 0; ch < value_chs; ch++)
    {
      if (vpvals.propagating_channel & PROPAGATING_VALUE) /* value channel */
	*dest++ = (CH)(vpvals.propagating_rate*fore[ch]
		       + (1.0 - vpvals.propagating_rate)*here[ch]);
      else
	*dest++ = here[ch];
    }
}

static void
initialize_foreground (GimpImageBaseType   image_type, 
		       gint         channels, 
		       CH          *here, 
		       CH          *best, 
		       void       **tmp)
{
  CH *ch;

  if (*tmp == NULL)
    {
      *tmp = (void *) g_new (CH, 3);
      ch = (CH *)*tmp;
      gimp_palette_get_foreground (&ch[0], &ch[1], &ch[2]);
    }
}

static void
initialize_background (GimpImageBaseType   image_type, 
		       gint         channels, 
		       CH          *here, 
		       CH          *best, 
		       void       **tmp)
{
  CH *ch;

  if (*tmp == NULL)
    {
      *tmp = (void *) g_new (CH, 3);
      ch = (CH *)*tmp;
      gimp_palette_get_background (&ch[0], &ch[1], &ch[2]);
    }
}

static void
propagate_a_color (GimpImageBaseType  image_type, 
		   gint        channels, 
		   CH         *orig, 
		   CH         *here, 
		   CH         *best, 
		   void       *tmp)
{
  CH *fg = (CH *)tmp;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      if (here[0] == fg[0] && here[1] == fg[1] && here[2] == fg[2] &&
	  value_difference_check(orig, here, 3))
	{
	  memcpy (best, here, 3 * sizeof(CH)); /* alpha channel holds old value */
	}
      break;
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      break;
    default:
      break;
    }
}

static void
propagate_opaque (GimpImageBaseType  image_type, 
		  gint        channels, 
		  CH         *orig, 
		  CH         *here, 
		  CH         *best, 
		  void       *tmp)
{
  switch (image_type)
    {
    case GIMP_RGBA_IMAGE:
      if (best[3] < here[3] && value_difference_check(orig, here, 3))
	memcpy(best, here, channels * sizeof(CH));
      break;
    case GIMP_GRAYA_IMAGE:
      if (best[1] < here[1] && value_difference_check(orig, here, 1))
	memcpy(best, here, channels * sizeof(CH));
      break;
    default:
      break;
    }
}

static void
propagate_transparent (GimpImageBaseType  image_type, 
		       gint        channels, 
		       CH         *orig, 
		       CH         *here, 
		       CH         *best, 
		       void       *tmp)
{
  switch (image_type)
    {
    case GIMP_RGBA_IMAGE:
      if (here[3] < best[3] && value_difference_check(orig, here, 3))
	memcpy(best, here, channels * sizeof(CH));
      break;
    case GIMP_GRAYA_IMAGE:
      if (here[1] < best[1] && value_difference_check(orig, here, 1))
	memcpy(best, here, channels * sizeof(CH));
      break;
    default:
      break;
    }
}

/* dialog stuff */

static int
vpropagate_dialog (GimpImageBaseType image_type)
{
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *toggle_vbox;
  GtkWidget *button;
  GtkWidget *sep;
  GtkObject *adj;
  GSList *group = NULL;
  gint	  index = 0;

  gimp_ui_init ("vpropagate", FALSE);

  dlg = gimp_dialog_new (_("Value Propagate"), "vpropagate",
			 gimp_standard_help_func, "filters/vpropagate.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,
			 _("OK"), vpropagate_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,
			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Propagate Mode */
  frame = gtk_frame_new (_("Propagate Mode"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  toggle_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (toggle_vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);
    
  for (index = 0; index < num_mode; index++)
    {
      button = gtk_radio_button_new_with_label (group,
						gettext (modes[index].name));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (toggle_vbox), button, FALSE, FALSE, 0);
      gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) index);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (gimp_radio_button_update),
			  &vpvals.propagate_mode);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    index == vpvals.propagate_mode);
      gtk_widget_show (button);
    }

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  /* Parameter settings */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (10, 3, FALSE); /* 4 raw, 2 columns(name and value) */
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Lower Threshold:"), SCALE_WIDTH, 0,
			      vpvals.lower_limit, 0, 255, 1, 8, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &vpvals.lower_limit);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Upper Threshold:"), SCALE_WIDTH, 0,
			      vpvals.upper_limit, 0, 255, 1, 8, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &vpvals.upper_limit);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Propagating Rate:"), SCALE_WIDTH, 0,
			      vpvals.propagating_rate, 0, 1, 0.01, 0.1, 2,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &vpvals.propagating_rate);

  sep = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (table), sep, 0, 3, 3, 4, GTK_FILL, 0, 0, 0);
  gtk_widget_show (sep);

  gtk_table_add_toggle (table, _("To Left"), 0, 1, 5,
			GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			&direction_mask_vec[Right2Left]);
  gtk_table_add_toggle (table, _("To Right"), 2, 3, 5,
			GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			&direction_mask_vec[Left2Right]);
  gtk_table_add_toggle (table, _("To Top"), 1, 2, 4,
			GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			&direction_mask_vec[Bottom2Top]);
  gtk_table_add_toggle (table, _("To Bottom"), 1, 2, 6,
			GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			&direction_mask_vec[Top2Bottom]);
  if ((image_type == GIMP_RGBA_IMAGE) | (image_type == GIMP_GRAYA_IMAGE))
    {
      sep = gtk_hseparator_new ();
      gtk_table_attach (GTK_TABLE (table), sep, 0, 3, 7, 8, GTK_FILL, 0, 0, 0);
      gtk_widget_show (sep);

      {
	GtkWidget *toggle;
	
	toggle =
	  gtk_table_add_toggle (table, _("Propagating Alpha Channel"),
				0, 3, 8,
				(GtkSignalFunc) gimp_toggle_button_update,
				&propagate_alpha);
	if (gimp_layer_get_preserve_transparency (drawable_id))
	  {
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), 0);
	    gtk_widget_set_sensitive (toggle, FALSE);
	  }
      }
      gtk_table_add_toggle (table, _("Propagating Value Channel"), 0, 3, 9,
			    (GtkSignalFunc) gimp_toggle_button_update,
			    &propagate_value);
    }
  gtk_widget_show (table);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);
  gtk_widget_show (dlg);
  
  gtk_main ();
  gdk_flush ();

  return vpropagate_interface.run;
}

static void
vpropagate_ok_callback (GtkWidget *widget,
			gpointer   data)
{
  gint i, result;

  for (i = result = 0; i < 4; i++)
    result |= (direction_mask_vec[i] ? 1 : 0) << i;
  vpvals.direction_mask = result;

  vpvals.propagating_channel = ((propagate_alpha ? PROPAGATING_ALPHA : 0) |
				(propagate_value ? PROPAGATING_VALUE : 0));

  vpropagate_interface.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static GtkWidget *
gtk_table_add_toggle (GtkWidget     *table,
		      gchar	    *name,
		      gint	     x1,
		      gint	     x2,
		      gint	     y,
		      GtkSignalFunc  update,
		      gint	    *value)
{
  GtkWidget *toggle;
  
  toggle = gtk_check_button_new_with_label(name);
  gtk_table_attach (GTK_TABLE (table), toggle, x1, x2, y, y+1,
		    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) update,
		      value);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *value);
  gtk_widget_show (toggle);

  return toggle;
}
