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

#include "color_cmds.h"
#include "gimpimage.h"
#include "gimpdrawable.h"
#include "gimplut.h"
#include "lut_funcs.h"

static Argument * brightness_contrast_invoker  (Argument *);
static Argument * levels_invoker               (Argument *);
static Argument * posterize_invoker (Argument *);

/*  ------------------------------------------------------------------  */
/*  --------- The brightness_contrast procedure definition  ----------  */
/*  ------------------------------------------------------------------  */

ProcArg brightness_contrast_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "brightness",
    "brightness adjustment: (-127 <= brightness <= 127)"
  },
  { PDB_INT32,
    "contrast",
    "constrast adjustment: (-127 <= contrast <= 127)"
  }
};

ProcRecord brightness_contrast_proc =
{
  "gimp_brightness_contrast",
  "Modify brightness/contrast in the specified drawable",
  "This procedures allows the brightness and contrast of the specified drawable to be modified.  Both 'brightness' and 'contrast' parameters are defined between -127 and 127.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  brightness_contrast_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { brightness_contrast_invoker } },
};


static Argument *
brightness_contrast_invoker (Argument *args)
{
  PixelRegion srcPR, destPR;
  int success = TRUE;
  int int_value;
  GimpImage *gimage;
  int brightness;
  int contrast;
  int x1, y1, x2, y2;
  GimpDrawable *drawable;

  drawable    = NULL;
  brightness  = 0;
  contrast    = 0;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = gimp_drawable_get_ID (int_value);
      if (drawable == NULL)
	success = FALSE;
      else
        gimage = gimp_drawable_gimage (drawable);
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! gimp_drawable_indexed (drawable);

  /*  brightness  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (int_value < -127 || int_value > 127)
	success = FALSE;
      else
	brightness = int_value;
    }
  /*  contrast  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value < -127 || int_value > 127)
	success = FALSE;
      else
	contrast = int_value;
    }

  /*  arrange to modify the brightness/contrast  */
  if (success)
    {
      GimpLut *lut;
      lut = brightness_contrast_lut_new(brightness / 255.0, contrast / 127.0,
					gimp_drawable_bytes(drawable));

      /*  The application should occur only within selection bounds  */
      gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixel_region_init (&srcPR, gimp_drawable_data (drawable),
			 x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, gimp_drawable_shadow (drawable),
			 x1, y1, (x2 - x1), (y2 - y1), TRUE);

      pixel_regions_process_parallel((p_func)gimp_lut_process, lut, 
				     2, &srcPR, &destPR);

      gimp_lut_free(lut);
      gimp_drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&brightness_contrast_proc, success);
}

/*  ------------------------------------------------------------------  */
/*  ---------------- The levels procedure definition -----------------  */
/*  ------------------------------------------------------------------  */

ProcArg levels_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "channel",
    "the channel to modify: { VALUE (0), RED (1), GREEN (2), BLUE (3), GRAY (0) }"
  },
  { PDB_INT32,
    "low_input",
    "intensity of lowest input: (0 <= low_input <= 255)"
  },
  { PDB_INT32,
    "high_input",
    "intensity of highest input: (0 <= high_input <= 255)"
  },
  { PDB_FLOAT,
    "gamma",
    "gamma correction factor: (0.1 <= gamma <= 10)"
  },
  { PDB_INT32,
    "low_output",
    "intensity of lowest output: (0 <= low_input <= 255)"
  },
  { PDB_INT32,
    "high_output",
    "intensity of highest output: (0 <= high_input <= 255)"
  }
};

ProcRecord levels_proc =
{
  "gimp_levels",
  "Modifies intensity levels in the specified drawable",
  "This tool allows intensity levels in the specified drawable to be remapped according to a set of parameters.  The low/high input levels specify an initial mapping from the source intensities.  The gamma value determines how intensities between the low and high input intensities are interpolated.  A gamma value of 1.0 results in a linear interpolation.  Higher gamma values result in more high-level intensities.  Lower gamma values result in more low-level intensities.  The low/high output levels constrain the final intensity mapping--that is, no final intensity will be lower than the low output level and no final intensity will be higher than the high output level.  This tool is only valid on RGB color and grayscale images.  It will not operate on indexed drawables.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  7,
  levels_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { levels_invoker } },
};


static Argument *
levels_invoker (Argument *args)
{
  PixelRegion srcPR, destPR;
  int success = TRUE;
  GimpDrawable *drawable;
  int channel;
  int low_inputv;
  int high_inputv;
  double gammav;
  int low_outputv;
  int high_outputv;
  int int_value;
  double fp_value;
  int x1, y1, x2, y2;
  int i;
  int            low_input[5];
  double         gamma[5];
  int            high_input[5];
  int            low_output[5];
  int            high_output[5];

  drawable = NULL;
  low_inputv   = 0;
  high_inputv  = 0;
  gammav       = 1.0;
  low_outputv  = 0;
  high_outputv = 0;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = gimp_drawable_get_ID (int_value);
      if (drawable == NULL)                                        
        success = FALSE;
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! gimp_drawable_indexed (drawable);
   
  /*  channel  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (success)
	{
	  if (gimp_drawable_gray (drawable))
	    {
	      if (int_value != 0)
		success = FALSE;
	    }
	  else if (gimp_drawable_color (drawable))
	    {
	      if (int_value < 0 || int_value > 3)
		success = FALSE;
	    }
	  else
	    success = FALSE;
	}
      channel = int_value;
    }
  /*  low input  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	low_inputv = int_value;
      else
	success = FALSE;
    }
  /*  high input  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	high_inputv = int_value;
      else
	success = FALSE;
    }
  /*  gamma  */
  if (success)
    {
      fp_value = args[4].value.pdb_float;
      if (fp_value >= 0.1 && fp_value <= 10.0)
	gammav = fp_value;
      else
	success = FALSE;
    }
  /*  low output  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	low_outputv = int_value;
      else
	success = FALSE;
    }
  /*  high output  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	high_outputv = int_value;
      else
	success = FALSE;
    }

  /*  arrange to modify the levels  */
  if (success)
    {
      GimpLut *lut;
      for (i = 0; i < 5; i++)
	{
	  low_input[i] = 0;
	  gamma[i] = 1.0;
	  high_input[i] = 255;
	  low_output[i] = 0;
	  high_output[i] = 255;
	}


      low_input[channel] = low_inputv;
      high_input[channel] = high_inputv;
      gamma[channel] = gammav;
      low_output[channel] = low_outputv;
      high_output[channel] = high_outputv;

      /*  setup the lut  */
      lut = levels_lut_new(gamma, low_input, high_input,
			   low_output, high_output,
			   gimp_drawable_bytes(drawable));

      /*  The application should occur only within selection bounds  */
      gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixel_region_init (&srcPR, gimp_drawable_data (drawable),
			 x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, gimp_drawable_shadow (drawable),
			 x1, y1, (x2 - x1), (y2 - y1), TRUE);

      pixel_regions_process_parallel((p_func)gimp_lut_process, lut, 
				     2, &srcPR, &destPR);

      gimp_lut_free(lut);
      gimp_drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&levels_proc, success);
}

/*  ------------------------------------------------------------------  */
/*  ---------------- The posterize procedure definition --------------  */
/*  ------------------------------------------------------------------  */

ProcArg posterize_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "levels",
    "levels of posterization: (2 <= levels <= 255)"
  }
};

ProcRecord posterize_proc =
{
  "gimp_posterize",
  "Posterize the specified drawable",
  "This procedures reduces the number of shades allows in each intensity channel to the specified 'levels' parameter.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  posterize_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { posterize_invoker } },
};


static Argument *
posterize_invoker (Argument *args)
{
  PixelRegion srcPR, destPR;
  int success = TRUE;
  GimpImage *gimage;
  GimpDrawable *drawable;
  int levels;
  int int_value;
  int x1, y1, x2, y2;

  drawable = NULL;
  levels = 0;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = gimp_drawable_get_ID (int_value);
      if (drawable == NULL)                                        
        success = FALSE;
      else
        gimage = gimp_drawable_gimage (drawable);
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! gimp_drawable_indexed (drawable);
    
  /*  levels  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (int_value >= 2 && int_value < 256)
	levels = int_value;
      else
	success = FALSE;
    }

  /*  arrange to modify the levels  */
  if (success)
    {
      GimpLut *lut;
      lut = posterize_lut_new(levels, gimp_drawable_bytes(drawable));
      /*  The application should occur only within selection bounds  */
      gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixel_region_init (&srcPR, gimp_drawable_data (drawable),
			 x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, gimp_drawable_shadow (drawable),
			 x1, y1, (x2 - x1), (y2 - y1), TRUE);

      pixel_regions_process_parallel((p_func)gimp_lut_process, lut, 
				     2, &srcPR, &destPR);

      gimp_lut_free(lut);
      gimp_drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&posterize_proc, success);
}
