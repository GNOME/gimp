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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include "appenv.h"
#include "brushes.h"
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "convolve.h"
#include "gdisplay.h"
#include "paint_funcs.h"
#include "paint_funcs_area.h"
#include "paint_core_16.h"
#include "paint_core.h"
#include "palette.h"
#include "pixelarea.h"
#include "selection.h"
#include "tools.h"

typedef enum
{
  Blur,
  Sharpen,
  Custom
} ConvolveType;

/*  forward function declarations  */
static void         calculate_matrix     (ConvolveType, double);
static void         integer_matrix       (float *, int *, int, int);
static void         copy_matrix          (float *, float *, int);
static int          sum_matrix           (int *, int);
static void        *convolve_non_gui_paint_func (PaintCore16 *, GimpDrawable *, int);
static void         convolve_motion      (PaintCore16 *, GimpDrawable *);
static Argument *   convolve_invoker     (Argument *);

#define FIELD_COLS    4
#define MIN_BLUR      64         /*  (8/9 original pixel)   */
#define MAX_BLUR      0.25       /*  (1/33 original pixel)  */
#define MIN_SHARPEN   -512
#define MAX_SHARPEN   -64

typedef struct _ConvolveOptions ConvolveOptions;
struct _ConvolveOptions
{
  ConvolveType  type;
  double        pressure;
};

/*  local variables  */
static int          matrix [25];
static int          matrix_size;
static int          matrix_divisor;
static ConvolveOptions *convolve_options = NULL;

static float        custom_matrix [25] =
{
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 1, 0, 0,
  0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,
};

static float        blur_matrix [25] =
{
  0, 0, 0, 0, 0,
  0, 1, 1, 1, 0,
  0, 1, MIN_BLUR, 1, 0,
  0, 1, 1, 1, 0,
  0, 0 ,0, 0, 0,
};

static float        sharpen_matrix [25] =
{
  0, 0, 0, 0, 0,
  0, 1, 1, 1, 0,
  0, 1, MIN_SHARPEN, 1, 0,
  0, 1, 1, 1, 0,
  0, 0, 0, 0, 0,
};


static void
convolve_scale_update (GtkAdjustment *adjustment,
		       double        *scale_val)
{
  *scale_val = adjustment->value;

  /*  recalculate the matrix  */
  calculate_matrix (convolve_options->type, convolve_options->pressure);
}

static void
convolve_type_callback (GtkWidget *w,
			gpointer  client_data)
{
  convolve_options->type = (ConvolveType) client_data;
  /*  recalculate the matrix  */
  calculate_matrix (convolve_options->type, convolve_options->pressure);
}

static ConvolveOptions *
create_convolve_options (void)
{
  ConvolveOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *pressure_scale;
  GtkWidget *radio_box;
  GtkWidget *radio_button;
  GtkObject *pressure_scale_data;
  GSList *group = NULL;
  int i;
  char *button_names[3] =
  {
    "Blur",
    "Sharpen",
    "Custom"
  };

  /*  the new options structure  */
  options = (ConvolveOptions *) g_malloc (sizeof (ConvolveOptions));
  options->type = Blur;
  options->pressure = 50.0;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);

  /*  the main label  */
  label = gtk_label_new ("Convolver Options");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the pressure scale  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Pressure");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  pressure_scale_data = gtk_adjustment_new (50.0, 0.0, 100.0, 1.0, 1.0, 0.0);
  pressure_scale = gtk_hscale_new (GTK_ADJUSTMENT (pressure_scale_data));
  gtk_box_pack_start (GTK_BOX (hbox), pressure_scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (pressure_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (pressure_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (pressure_scale_data), "value_changed",
		      (GtkSignalFunc) convolve_scale_update,
		      &options->pressure);
  gtk_widget_show (pressure_scale);
  gtk_widget_show (hbox);

  radio_box = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), radio_box, FALSE, FALSE, 0);

  /*  the radio buttons  */
  for (i = 0; i < 2; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, button_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) convolve_type_callback,
			  (gpointer) ((long) i));
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (radio_box);

  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (CONVOLVE, vbox);

  return options;
}

void *
convolve_paint_func (PaintCore16 *paint_core,
		     GimpDrawable *drawable,
		     int        state)
{
  switch (state)
    {
    case INIT_PAINT:
      /*  calculate the matrix  */
      calculate_matrix (convolve_options->type, convolve_options->pressure);
      break;

    case MOTION_PAINT:
      convolve_motion (paint_core, drawable);
      break;

    case FINISH_PAINT:
      break;
    }

  return NULL;
}

Tool *
tools_new_convolve ()
{
  Tool * tool;
  PaintCore16 * private;

  if (! convolve_options)
    convolve_options = create_convolve_options ();

  tool = paint_core_16_new (CONVOLVE);

  private = (PaintCore16 *) tool->private;
  private->paint_func = convolve_paint_func;

  return tool;
}


void
tools_free_convolve (Tool *tool)
{
  paint_core_free (tool);
}


static void
convolve_motion (
		 PaintCore16 *paint_core,
	         GimpDrawable *drawable
		)
{
  GImage *gimage;
  Tag tag = drawable_tag (drawable);
  Canvas *painthit_canvas; 
  PixelArea src_area, painthit_area;

  if ( !(gimage = drawable_gimage (drawable)) )
    return;

  if ( tag_format (tag) == FORMAT_INDEXED )
    return;
  
  /* Retrieve the painthit_canvas */
  painthit_canvas = paint_core_16_area (paint_core, drawable);
  pixelarea_init (&painthit_area, painthit_canvas, NULL, 0, 0, 0, 0, TRUE);

  /* Set up src_area with the drawable */ 
  pixelarea_init (&src_area, drawable_data_canvas (drawable), NULL,
		   paint_core->x, paint_core->y, paint_core->w, paint_core->h, FALSE);

  if (paint_core->w >= matrix_size && paint_core->h >= matrix_size)
  {
    /* Copy the pixels from the drawable to a temp
       canvas, convolve that and place the result in
       the painthit_canvas. */
    
    Canvas *temp_canvas;
    PixelArea temp_area;
    Tag temp_tag;
     
    temp_tag = tag_set_alpha (tag, ALPHA_YES);
    temp_canvas = canvas_new ( temp_tag, paint_core->w, paint_core->h, STORAGE_FLAT);


    /* these refs should really be done inside the area function via a
       pixelarea_process() loop.  the idea with the area funcs is that
       they handle all the messy details of reffing so that the high
       level code doesn't need to worry about it */
    canvas_portion_ref ( temp_canvas, 0, 0 );
    canvas_portion_ref ( painthit_canvas, 0, 0 );

    
    pixelarea_init (&temp_area, temp_canvas, NULL, 0 , 0 , 0, 0, TRUE);
    copy_area (&src_area, &temp_area);

    /* setup the temp_area and src_area again */
    pixelarea_init (&src_area, drawable_data_canvas (drawable), NULL,
		   paint_core->x, paint_core->y, paint_core->w, paint_core->h, FALSE);
    pixelarea_init (&temp_area, temp_canvas, NULL, 0 , 0 , 0, 0, TRUE);

    /* convolve from temp_area to painthit_area */    
    convolve_area (&temp_area, &painthit_area, matrix, matrix_size,
		       matrix_divisor, NORMAL);



    canvas_portion_unref ( painthit_canvas, 0, 0 );
    canvas_portion_unref ( temp_canvas, 0, 0 );


    
    canvas_delete ( temp_canvas );
  }
  else
  {
    copy_area (&src_area, &painthit_area);
  }
  
  /* Apply the painthit to the drawable */
  paint_core_16_area_replace (paint_core, drawable, 
				1.0,
                                (gfloat) get_brush_opacity (),
                                SOFT,
                                INCREMENTAL); 
}

static void
calculate_matrix (ConvolveType type,
		  double       pressure)
{
  float percent;

  /*  find percent of tool pressure  */
  percent = pressure / 100.0;

  /*  get the appropriate convolution matrix and size and divisor  */
  switch (type)
    {
    case Blur:
      matrix_size = 5;
      blur_matrix [12] = MIN_BLUR + percent * (MAX_BLUR - MIN_BLUR);
      integer_matrix (blur_matrix, matrix, matrix_size, 100);
      break;

    case Sharpen:
      matrix_size = 5;
      sharpen_matrix [12] = MIN_SHARPEN + percent * (MAX_SHARPEN - MIN_SHARPEN);
      integer_matrix (sharpen_matrix, matrix, matrix_size, 1);
      break;

    case Custom:
      matrix_size = 5;
      integer_matrix (custom_matrix, matrix, matrix_size, 1);
      break;
    }

  matrix_divisor = sum_matrix (matrix, matrix_size);

  if (!matrix_divisor)
    matrix_divisor = 1;
}

static void
integer_matrix (float *source,
		int   *dest,
		int    size,
		int    factor)
{
  int i;

  for (i = 0; i < size*size; i++)
    *dest++ = (int) (*source ++ * factor);
}

static void
copy_matrix (float *src,
	     float *dest,
	     int    size)
{
  int i;

  for (i = 0; i < size*size; i++)
    *dest++ = *src++;
}

static int
sum_matrix (int *matrix,
	    int  size)
{
  int sum = 0;

  size *= size;

  while (size --)
    sum += *matrix++;

  return sum;
}


static void *
convolve_non_gui_paint_func (PaintCore16 *paint_core,
			     GimpDrawable *drawable,
			     int        state)
{
  convolve_motion (paint_core, drawable);

  return NULL;
}


/*  The convolve procedure definition  */
ProcArg convolve_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_FLOAT,
    "pressure",
    "the pressure: 0 <= pressure <= 100"
  },
  { PDB_INT32,
    "convolve_type",
    "convolve type: { BLUR (0), SHARPEN (1) }"
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


ProcRecord convolve_proc =
{
  "gimp_convolve",
  "Convolve (Blur, Sharpen) using the current brush",
  "This tool convolves the specified drawable with either a sharpening or blurring kernel.  The pressure parameter controls the magnitude of the operation.  Like the paintbrush, this tool linearly interpolates between the specified stroke coordinates.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  6,
  convolve_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { convolve_invoker } },
};


static Argument *
convolve_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  double pressure;
  ConvolveType type;
  int num_strokes;
  double *stroke_array;
  ConvolveType int_value;
  double fp_value;
  int i;

  drawable = NULL;
  pressure    = 100.0;
  type        = Blur;
  num_strokes = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  the pressure  */
  if (success)
    {
      fp_value = args[2].value.pdb_int;
      if (fp_value >= 0.0 && fp_value <= 100.0)
	pressure = fp_value;
      else
	success = FALSE;
    }
  /*  the convolve type  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      switch (int_value)
	{
	case 0: type = Blur; break;
	case 1: type = Sharpen; break;
	case 2: success = FALSE; break; /*type = Custom; break;*/
	default: success = FALSE;
	}
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

  /*  point array  */
  if (success)
    stroke_array = (double *) args[5].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_16_init (&non_gui_paint_core_16, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  calculate the convolution kernel  */
      calculate_matrix (type, pressure);

      /*  set the paint core's paint func  */
      non_gui_paint_core_16.paint_func = (PaintFunc16)convolve_non_gui_paint_func;

      non_gui_paint_core_16.startx = non_gui_paint_core_16.lastx = stroke_array[0];
      non_gui_paint_core_16.starty = non_gui_paint_core_16.lasty = stroke_array[1];

      if (num_strokes == 1)
	convolve_non_gui_paint_func (&non_gui_paint_core_16, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core_16.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core_16.cury = stroke_array[i * 2 + 1];

	  paint_core_16_interpolate (&non_gui_paint_core_16, drawable);

	  non_gui_paint_core_16.lastx = non_gui_paint_core_16.curx;
	  non_gui_paint_core_16.lasty = non_gui_paint_core_16.cury;
	}

      /*  finish the painting  */
      paint_core_16_finish (&non_gui_paint_core_16, drawable, -1);

      /*  cleanup  */
      paint_core_16_cleanup ();
    }

  return procedural_db_return_args (&convolve_proc, success);
}
