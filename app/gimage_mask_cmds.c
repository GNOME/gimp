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
#include <stdio.h>
#include <stdlib.h>
#include "appenv.h"
#include "drawable.h"
#include "general.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gimage_mask_cmds.h"

static int int_value;
static double fp_value;
static int success;
static Argument *return_args;


/************************/
/*  GIMAGE_MASK_BOUNDS  */

static Argument *
gimage_mask_bounds_invoker (Argument *args)
{
  GImage *gimage;
  int x1, y1, x2, y2;
  int non_empty;

  non_empty = FALSE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    non_empty = gimage_mask_bounds (gimage, &x1, &y1, &x2, &y2);

  return_args = procedural_db_return_args (&gimage_mask_bounds_proc, success);

  if (success)
    {
      return_args[1].value.pdb_int = non_empty;
      return_args[2].value.pdb_int = x1;
      return_args[3].value.pdb_int = y1;
      return_args[4].value.pdb_int = x2;
      return_args[5].value.pdb_int = y2;
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_mask_bounds_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_mask_bounds_out_args[] =
{
  { PDB_INT32,
    "non-empty",
    "true if there is a selection"
  },
  { PDB_INT32,
    "x1",
    "x coordinate of upper left corner of selection bounds"
  },
  { PDB_INT32,
    "y1",
    "y coordinate of upper left corner of selection bounds"
  },
  { PDB_INT32,
    "x2",
    "x coordinate of lower right corner of selection bounds"
  },
  { PDB_INT32,
    "y2",
    "y coordinate of lower right corner of selection bounds"
  }
};

ProcRecord gimage_mask_bounds_proc =
{
  "gimp_selection_bounds",
  "Find the bounding box of the current selection",
  "This procedure returns whether there is a selection for the specified image.  If there is one, the upper left and lower right corners of the bounding box are returned.  These coordinates are relative to the image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_mask_bounds_args,

  /*  Output arguments  */
  5,
  gimage_mask_bounds_out_args,

  /*  Exec method  */
  { { gimage_mask_bounds_invoker } },
};


/***********************/
/*  GIMAGE_MASK_VALUE  */

static Argument *
gimage_mask_value_invoker (Argument *args)
{
  GImage *gimage;
  int x, y;
  int value;

  value = 0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      x = args[1].value.pdb_int;
      y = args[2].value.pdb_int;
    }
  if (success)
    value = gimage_mask_value (gimage, x, y);

  return_args = procedural_db_return_args (&gimage_mask_value_proc, success);

  if (success)
    return_args[1].value.pdb_int = value;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_mask_value_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_INT32,
    "x",
    "x coordinate of value"
  },
  { PDB_INT32,
    "y",
    "y coordinate of value"
  }
};

ProcArg gimage_mask_value_out_args[] =
{
  { PDB_INT32,
    "value",
    "value of the selection: (0 <= value <= 255)"
  }
};

ProcRecord gimage_mask_value_proc =
{
  "gimp_selection_value",
  "Find the value of the selection at the specified coordinates",
  "This procedure returns the value of the selection at the specified coordinates.  If the coordinates lie out of bounds, 0 is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_mask_value_args,

  /*  Output arguments  */
  1,
  gimage_mask_value_out_args,

  /*  Exec method  */
  { { gimage_mask_value_invoker } },
};


/**************************/
/*  GIMAGE_MASK_IS_EMPTY  */

static Argument *
gimage_mask_is_empty_invoker (Argument *args)
{
  GImage *gimage;
  int is_empty;

  is_empty = TRUE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    is_empty = gimage_mask_is_empty (gimage);

  return_args = procedural_db_return_args (&gimage_mask_is_empty_proc, success);

  if (success)
    return_args[1].value.pdb_int = is_empty;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_mask_is_empty_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_mask_is_empty_out_args[] =
{
  { PDB_INT32,
    "is_empty",
    "is the selection empty?"
  }
};

ProcRecord gimage_mask_is_empty_proc =
{
  "gimp_selection_is_empty",
  "Determine whether the selection in empty",
  "This procedure returns non-zero if the selection for the specified image is not empty.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_mask_is_empty_args,

  /*  Output arguments  */
  1,
  gimage_mask_is_empty_out_args,

  /*  Exec method  */
  { { gimage_mask_is_empty_invoker } },
};


/***************************/
/*  GIMAGE_MASK_TRANSLATE  */

static Argument *
gimage_mask_translate_invoker (Argument *args)
{
  GImage *gimage;
  int offx, offy;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      offx = args[1].value.pdb_int;
      offy = args[2].value.pdb_int;
    }
  if (success)
    gimage_mask_translate (gimage, offx, offy);

  return procedural_db_return_args (&gimage_mask_translate_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_translate_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_INT32,
    "offset_x",
    "x offset for translation"
  },
  { PDB_INT32,
    "offset_y",
    "y offset for translation"
  }
};

ProcRecord gimage_mask_translate_proc =
{
  "gimp_selection_translate",
  "Translate the selection by the specified offsets",
  "This procedure actually translates the selection for the specified image by the specified offsets.  Regions that are translated from beyond the bounds of the image are set to empty.  Valid regions of the selection which are translated beyond the bounds of the image because of this call are lost.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_mask_translate_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_translate_invoker } },
};


/***************************/
/*  GIMAGE_MASK_FLOAT  */

static Argument *
gimage_mask_float_invoker (Argument *args)
{
  GImage *gimage;
  GimpDrawable *drawable;
  int offx, offy;
  Layer *layer;

  layer = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL)
        {
	  success = FALSE;
	  gimage  = NULL;
	}
      else
        {
	  gimage = drawable_gimage (drawable);
	}
    }
  if (success)
    {
      offx = args[1].value.pdb_int;
      offy = args[2].value.pdb_int;
    }
  if (success)
    success = ((layer = gimage_mask_float (gimage, drawable, offx, offy)) != NULL);

  return_args = procedural_db_return_args (&gimage_mask_float_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(layer));

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_mask_float_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable from which to float selection"
  },
  { PDB_INT32,
    "offset_x",
    "x offset for translation"
  },
  { PDB_INT32,
    "offset_y",
    "y offset for translation"
  }
};

ProcArg gimage_mask_float_out_args[] =
{
  { PDB_LAYER,
    "layer",
    "the floated layer"
  }
};

ProcRecord gimage_mask_float_proc =
{
  "gimp_selection_float",
  "Float the selection from the specified drawable with initial offsets as specified",
  "This procedure determines the region of the specified drawable that lies beneath the current selection.  The region is then cut from the drawable and the resulting data is made into a new layer which is instantiated as a floating selection.  The offsets allow initial positioning of the new floating selection.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_mask_float_args,

  /*  Output arguments  */
  1,
  gimage_mask_float_out_args,

  /*  Exec method  */
  { { gimage_mask_float_invoker } },
};


/***********************/
/*  GIMAGE_MASK_CLEAR  */

static Argument *
gimage_mask_clear_invoker (Argument *args)
{
  GImage *gimage;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    gimage_mask_clear (gimage);

  return procedural_db_return_args (&gimage_mask_clear_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_clear_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcRecord gimage_mask_clear_proc =
{
  "gimp_selection_clear",
  "Set the selection to none, clearing all previous content",
  "This procedure sets the selection mask to empty, assigning the value 0 to every pixel in the selection channel.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_mask_clear_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_clear_invoker } },
};


/***********************/
/*  GIMAGE_MASK_INVERT  */

static Argument *
gimage_mask_invert_invoker (Argument *args)
{
  GImage *gimage;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    gimage_mask_invert (gimage);

  return procedural_db_return_args (&gimage_mask_invert_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_invert_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcRecord gimage_mask_invert_proc =
{
  "gimp_selection_invert",
  "Invert the selection mask",
  "This procedure inverts the selection mask.  For every pixel in the selection channel, its new value is calculated as (255 - old_value).",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_mask_invert_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_invert_invoker } },
};


/*************************/
/*  GIMAGE_MASK_SHARPEN  */

static Argument *
gimage_mask_sharpen_invoker (Argument *args)
{
  GImage *gimage;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    gimage_mask_sharpen (gimage);

  return procedural_db_return_args (&gimage_mask_sharpen_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_sharpen_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcRecord gimage_mask_sharpen_proc =
{
  "gimp_selection_sharpen",
  "Sharpen the selection mask",
  "This procedure sharpens the selection mask.  For every pixel in the selection channel, if the value is > 0, the new pixel is assigned a value of 255.  This removes any \"anti-aliasing\" that might exist in the selection mask's boundary.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_mask_sharpen_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_sharpen_invoker } },
};


/*********************/
/*  GIMAGE_MASK_ALL  */

static Argument *
gimage_mask_all_invoker (Argument *args)
{
  GImage *gimage;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    gimage_mask_all (gimage);

  return procedural_db_return_args (&gimage_mask_all_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_all_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcRecord gimage_mask_all_proc =
{
  "gimp_selection_all",
  "Select all of the image",
  "This procedure sets the selection mask to completely encompass the image.  Every pixel in the selection channel is set to 255.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_mask_all_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_all_invoker } },
};


/*********************/
/*  GIMAGE_MASK_NONE */

static Argument *
gimage_mask_none_invoker (Argument *args)
{
  GImage *gimage;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    gimage_mask_none (gimage);

  return procedural_db_return_args (&gimage_mask_none_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_none_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcRecord gimage_mask_none_proc =
{
  "gimp_selection_none",
  "Deselect the entire image",
  "This procedure deselects the entire image.  Every pixel in the selection channel is set to 0.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_mask_none_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_none_invoker } },
};


/************************/
/*  GIMAGE_MASK_FEATHER */

static Argument *
gimage_mask_feather_invoker (Argument *args)
{
  GImage *gimage;
  double radius;

  radius = 0.0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      fp_value = args[1].value.pdb_float;
      if (fp_value > 0)
	radius = fp_value;
    }
  if (success)
    gimage_mask_feather (gimage, radius);

  return procedural_db_return_args (&gimage_mask_feather_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_feather_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_FLOAT,
    "radius",
    "radius of feather (in pixels)"
  }
};

ProcRecord gimage_mask_feather_proc =
{
  "gimp_selection_feather",
  "Feather the image's selection",
  "This procedure feathers the selection.  Feathering is implemented using a gaussian blur.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_mask_feather_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_feather_invoker } },
};


/***********************/
/*  GIMAGE_MASK_BORDER */

static Argument *
gimage_mask_border_invoker (Argument *args)
{
  GImage *gimage;
  int radius;

  radius = 0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (int_value > 0)
	radius = int_value;
    }
  if (success)
    gimage_mask_border (gimage, radius);

  return procedural_db_return_args (&gimage_mask_border_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_border_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_INT32,
    "radius",
    "radius of border (pixels)"
  }
};

ProcRecord gimage_mask_border_proc =
{
  "gimp_selection_border",
  "Border the image's selection",
  "This procedure borders the selection.  Bordering creates a new selection which is defined along the boundary of the previous selection at every point within the specified radius.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_mask_border_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_border_invoker } },
};


/***********************/
/*  GIMAGE_MASK_GROW */

static Argument *
gimage_mask_grow_invoker (Argument *args)
{
  GImage *gimage;
  int steps = 0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (int_value > 0)
	steps = int_value;
    }
  if (success)
    gimage_mask_grow (gimage, steps);

  return procedural_db_return_args (&gimage_mask_grow_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_grow_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_INT32,
    "steps",
    "steps of grow (pixels)"
  }
};

ProcRecord gimage_mask_grow_proc =
{
  "gimp_selection_grow",
  "Grow the image's selection",
  "This procedure grows the selection.  Growing involves expanding the boundary in all directions by the specified pixel amount.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_mask_grow_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_grow_invoker } },
};


/***********************/
/*  GIMAGE_MASK_SHRINK */

static Argument *
gimage_mask_shrink_invoker (Argument *args)
{
  GImage *gimage;
  int steps = 0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (int_value < 0)
	success = FALSE;
      else
	steps = int_value;
    }
  if (success)
    gimage_mask_shrink (gimage, steps);

  return procedural_db_return_args (&gimage_mask_shrink_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_shrink_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_INT32,
    "radius",
    "radius of shrink (pixels)"
  }
};

ProcRecord gimage_mask_shrink_proc =
{
  "gimp_selection_shrink",
  "Shrink the image's selection",
  "This procedure shrinks the selection.  Shrinking invovles trimming the existing selection boundary on all sides by the specified number of pixels.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_mask_shrink_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_shrink_invoker } },
};


/****************************/
/*  GIMAGE_MASK_LAYER_ALPHA */

static Argument *
gimage_mask_layer_alpha_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
        {
	  success = FALSE;
	  gimage  = NULL;
	}
      else
        {
	  gimage = drawable_gimage (GIMP_DRAWABLE(layer));
	}
    }
  if (success)
    gimage_mask_layer_alpha (gimage, layer);

  return procedural_db_return_args (&gimage_mask_layer_alpha_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_layer_alpha_args[] =
{
  { PDB_LAYER,
    "layer",
    "layer with alpha"
  }
};

ProcRecord gimage_mask_layer_alpha_proc =
{
  "gimp_selection_layer_alpha",
  "Transfer the specified layer's alpha channel to the selection mask",
  "This procedure requires a layer with an alpha channel.  The alpha channel information is used to create a selection mask such that for any pixel in the image defined in the specified layer, that layer pixel's alpha value is transferred to the selection mask.  If the layer is undefined at a particular image pixel, the associated selection mask value is set to 0.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_mask_layer_alpha_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_layer_alpha_invoker } },
};


/**********************/
/*  GIMAGE_MASK_LOAD  */

static Argument *
gimage_mask_load_invoker (Argument *args)
{
  GImage *gimage;
  Channel *channel;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)) == NULL)
        {
	  success = FALSE;
	  gimage  = NULL;
	}
      else
        {
          gimage = drawable_gimage (GIMP_DRAWABLE(channel));
	  success = (drawable_width (GIMP_DRAWABLE(channel)) == gimage->width && 
		     drawable_height (GIMP_DRAWABLE(channel)) == gimage->height);
	}
    }

  if (success)
    gimage_mask_load (gimage, channel);

  return procedural_db_return_args (&gimage_mask_load_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_mask_load_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  }
};

ProcRecord gimage_mask_load_proc =
{
  "gimp_selection_load",
  "Transfer the specified channel to the selection mask",
  "This procedure loads the specified channel into the selection mask.  This essentially involves a copy of the channel's content in to the selection mask.  Therefore, the channel must have the same width and height of the image, or an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_mask_load_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_mask_load_invoker } },
};


/**********************/
/*  GIMAGE_MASK_SAVE  */

static Argument *
gimage_mask_save_invoker (Argument *args)
{
  GImage *gimage;
  Channel *channel;

  channel = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = ((channel = gimage_mask_save (gimage)) != NULL);

  return_args = procedural_db_return_args (&gimage_mask_save_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(channel));

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_mask_save_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_mask_save_out_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the new channel"
  }
};

ProcRecord gimage_mask_save_proc =
{
  "gimp_selection_save",
  "Copy the selection mask to a new channel",
  "This procedure copies the selection mask and stores the content in a new channel.  The new channel is automatically inserted into the image's list of channels.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_mask_save_args,

  /*  Output arguments  */
  1,
  gimage_mask_save_out_args,

  /*  Exec method  */
  { { gimage_mask_save_invoker } },
};
