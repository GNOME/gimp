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
#include <stdio.h>
#include <stdlib.h>
#include "appenv.h"
#include "floating_sel.h"
#include "general.h"
#include "gimage.h"
#include "layer.h"
#include "layer_cmds.h"
#include "undo.h"

#include "layer_pvt.h"
#include "drawable_pvt.h"

static int int_value;
static double fp_value;
static int success;


/***************/
/*  LAYER_NEW  */

static Argument *
layer_new_invoker (Argument *args)
{
  Layer *layer;
  int gimage_id;
  int width, height;
  int type;
  char *name;
  int opacity;
  int mode;
  Argument *return_args;
  Tag tag;
  Storage storage;

  layer     = NULL;
  gimage_id = -1;
  type      = 0;
  opacity   = 255;
  mode      = NORMAL_MODE;
  storage   = STORAGE_TILED;
  
  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (gimage_get_ID (int_value) != NULL)
	gimage_id = int_value;
      else
	success = FALSE;
    }
  if (success)
    {
      width = args[1].value.pdb_int;
      height = args[2].value.pdb_int;
      success = (width > 0 && height > 0);
    }
  if (success)
    {
#define LAYER_CMDS_C_1_cw
      int_value = args[3].value.pdb_int;
      tag = tag_from_drawable_type (int_value & 0xff);
      if (int_value & 0xff00)
        storage = STORAGE_SHM;
      if (!tag_valid (tag) )
	success = FALSE;
    }
  if (success)
    name = (char *) args[4].value.pdb_pointer;
  if (success)
    {
      fp_value = args[5].value.pdb_float;
      if (fp_value >= 0 && fp_value <= 100)
	opacity = (int) ((fp_value * 255) / 100);
      else
	success = FALSE;
    }
  if (success)
    {
      int_value = args[6].value.pdb_int;
      if (int_value >= NORMAL_MODE && int_value <= VALUE_MODE)
	mode = int_value;
      else
	success = FALSE;
    }

  if (success)
    success = ((layer = layer_new_tag (gimage_id, width, height, tag, storage, name, opacity, mode)) != NULL);

  return_args = procedural_db_return_args (&layer_new_proc, success);

  if (success)
    return_args[1].value.pdb_int = GIMP_DRAWABLE(layer)->ID;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_new_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image to which to add the layer"
  },
  { PDB_INT32,
    "width",
    "the layer width: (width > 0)"
  },
  { PDB_INT32,
    "height",
    "the layer height: (height > 0)"
  },
  { PDB_INT32,
    "type",
    "the layer type: { RGB_IMAGE (0), RGBA_IMAGE (1), GRAY_IMAGE (2), GRAYA_IMAGE (3), INDEXED_IMAGE (4), INDEXEDA_IMAGE (5) }"
  },
  { PDB_STRING,
    "name",
    "the layer name"
  },
  { PDB_FLOAT,
    "opacity",
    "the layer opacity: (0 <= opacity <= 100)"
  },
  { PDB_INT32,
    "mode",
    "the layer combination mode: { NORMAL (0), DISSOLVE (1), MULTIPLY (3), SCREEN (4), OVERLAY (5) DIFFERENCE (6), ADDITION (7), SUBTRACT (8), DARKEN-ONLY (9), LIGHTEN-ONLY (10), HUE (11), SATURATION (12), COLOR (13), VALUE (14) }"
  }
};

ProcArg layer_new_out_args[] =
{
  { PDB_LAYER,
    "layer",
    "the newly created layer"
  }
};

ProcRecord layer_new_proc =
{
  "gimp_layer_new",
  "Create a new layer",
  "This procedure creates a new layer with the specified width, height, and type.  Name, opacity, and mode are also supplied parameters.  The new layer still needs to be added to the image, as this is not automatic.  Add the new layer with the 'gimp_image_add_layer' command.  Other attributes such as layer mask modes, and offsets should be set with explicit procedure calls.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  7,
  layer_new_args,

  /*  Output arguments  */
  1,
  layer_new_out_args,

  /*  Exec method  */
  { { layer_new_invoker } },
};


/****************/
/*  LAYER_COPY  */

static Argument *
layer_copy_invoker (Argument *args)
{
  Layer *layer;
  Layer *copy;
  int add_alpha;
  Argument *return_args;

  copy = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      int_value = args[1].value.pdb_int;
      add_alpha = (int_value) ? TRUE : FALSE;
    }

  if (success)
    success = ((copy = layer_copy (layer, add_alpha)) != NULL);

  return_args = procedural_db_return_args (&layer_copy_proc, success);

  if (success)
    return_args[1].value.pdb_int = GIMP_DRAWABLE(copy)->ID;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_copy_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer to copy"
  },
  { PDB_INT32,
    "add_apha",
    "add an alpha channel to the copied layer"
  }
};

ProcArg layer_copy_out_args[] =
{
  { PDB_LAYER,
    "layer_copy",
    "the newly copied layer"
  }
};

ProcRecord layer_copy_proc =
{
  "gimp_layer_copy",
  "Copy a layer",
  "This procedure copies the specified layer and returns the copy.  The newly copied layer is for use within the original layer's image.  It should not be subsequently added to any other image.  The copied layer can optionally have an added alpha channel.  This is useful if the background layer in an image is being copied and added to the same image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  layer_copy_args,

  /*  Output arguments  */
  1,
  layer_copy_out_args,

  /*  Exec method  */
  { { layer_copy_invoker } },
};


/***********************/
/*  LAYER_CREATE_MASK  */

static Argument *
layer_create_mask_invoker (Argument *args)
{
  Layer *layer;
  LayerMask *mask;
  AddMaskType mask_type;
  Argument *return_args;

  mask      = NULL;
  mask_type = WhiteMask;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      int_value = args[1].value.pdb_int;
      switch (int_value)
	{
	case 0: mask_type = WhiteMask; break;
	case 1: mask_type = BlackMask; break;
	case 2: mask_type = AlphaMask; break;
	default: success = FALSE;
	}
    }

  if (success)
    success = ((mask = layer_create_mask (layer, mask_type)) != NULL);

  return_args = procedural_db_return_args (&layer_create_mask_proc, success);

  if (success)
    return_args[1].value.pdb_int = GIMP_DRAWABLE(mask)->ID;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_create_mask_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer to which to add the mask"
  },
  { PDB_INT32,
    "mask_type",
    "the type of mask: { WHITE-MASK (0), BLACK-MASK (1), ALPHA-MASK (2) }"
  }
};

ProcArg layer_create_mask_out_args[] =
{
  { PDB_CHANNEL,
    "mask",
    "the newly created mask"
  }
};

ProcRecord layer_create_mask_proc =
{
  "gimp_layer_create_mask",
  "Create a layer mask for the specified specified layer",
  "This procedure creates a layer mask for the specified layer.  Layer masks serve as an additional alpha channel for a layer.  Three different types of masks are allowed initially: completely white masks (which will leave the layer fully visible), completely black masks (which will give the layer complete transparency), and the layer's already existing alpha channel (which will leave the layer fully visible, but which may be more useful than a white mask).  The layer mask still needs to be added to the layer.  This can be done with a call to 'gimage_add_layer_mask'.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  layer_create_mask_args,

  /*  Output arguments  */
  1,
  layer_create_mask_out_args,

  /*  Exec method  */
  { { layer_create_mask_invoker } },
};


/*****************/
/*  LAYER_SCALE  */

static Argument *
layer_scale_invoker (Argument *args)
{
  Layer *layer;
  Layer *floating_layer;
  GImage *gimage;
  int new_width, new_height;
  int local_origin;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      new_width = args[1].value.pdb_int;
      new_height = args[2].value.pdb_int;

      if (new_width <= 0 || new_height <= 0)
	success = FALSE;
    }
  if (success)
    local_origin = args[3].value.pdb_int;
  if (success)
    success = ((gimage = gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID)) != NULL);

  if (success)
    {
      floating_layer = gimage_floating_sel (gimage);

      undo_push_group_start (gimage, LAYER_SCALE_UNDO);

      if (floating_layer)
	floating_sel_relax (floating_layer, TRUE);

      layer_scale (layer, new_width, new_height, local_origin);

      if (floating_layer)
	floating_sel_rigor (floating_layer, TRUE);

      undo_push_group_end (gimage);
    }

  return procedural_db_return_args (&layer_scale_proc, success);
}

/*  The procedure definition  */
ProcArg layer_scale_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "new_width",
    "new layer width: (new_width > 0)"
  },
  { PDB_INT32,
    "new_height",
    "new layer height: (new_height > 0)"
  },
  { PDB_INT32,
    "local_origin",
    "use a local origin, or the image origin?"
  }
};

ProcRecord layer_scale_proc =
{
  "gimp_layer_scale",
  "Scale the layer to the specified extents.",
  "This procedure scales the layer so that it's new width and height are equal to the supplied parameters.  The \"local_origin\" parameter specifies whether to scale from the center of the layer, or from the image origin.  This operation only works if the layer has been added to an image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  layer_scale_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_scale_invoker } },
};


/******************/
/*  LAYER_RESIZE  */

static Argument *
layer_resize_invoker (Argument *args)
{
  Layer *layer;
  Layer *floating_layer;
  GImage *gimage;
  int new_width, new_height;
  int offx, offy;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      new_width = args[1].value.pdb_int;
      new_height = args[2].value.pdb_int;

      if (new_width <= 0 || new_height <= 0)
	success = FALSE;
    }
  if (success)
    {
      offx = args[3].value.pdb_int;
      offy = args[4].value.pdb_int;
    }
  if (success)
    success = ((gimage = gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID)) != NULL);

  if (success)
    {
      floating_layer = gimage_floating_sel (gimage);

      undo_push_group_start (gimage, LAYER_SCALE_UNDO);

      if (floating_layer)
	floating_sel_relax (floating_layer, TRUE);

      layer_resize (layer, new_width, new_height, offx, offy);

      if (floating_layer)
	floating_sel_rigor (floating_layer, TRUE);

      undo_push_group_end (gimage);
    }

  return procedural_db_return_args (&layer_resize_proc, success);
}

/*  The procedure definition  */
ProcArg layer_resize_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "new_width",
    "new layer width: (new_width > 0)"
  },
  { PDB_INT32,
    "new_height",
    "new layer height: (new_height > 0)"
  },
  { PDB_INT32,
    "offx",
    "x offset between upper left corner of old and new layers: (new - old)"
  },
  { PDB_INT32,
    "offy",
    "y offset between upper left corner of old and new layers: (new - old)"
  }
};

ProcRecord layer_resize_proc =
{
  "gimp_layer_resize",
  "Resize the layer to the specified extents.",
  "This procedure resizes the layer so that it's new width and height are equal to the supplied parameters.  Offsets are also provided which describe the position of the previous layer's content.  No bounds checking is currently provided, so don't supply parameters that are out of bounds.  This operation on works if the layer has been added to an image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  layer_resize_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_resize_invoker } },
};


/******************/
/*  LAYER_DELETE  */

static Argument *
layer_delete_invoker (Argument *args)
{
  Layer *layer;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    layer_delete (layer);

  return procedural_db_return_args (&layer_delete_proc, success);
}

/*  The procedure definition  */
ProcArg layer_delete_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer to delete"
  }
};

ProcRecord layer_delete_proc =
{
  "gimp_layer_delete",
  "Delete a layer",
  "This procedure deletes the specified layer.  This does not need to be done if a gimage containing this layer was already deleted.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_delete_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_delete_invoker } },
};


/*********************/
/*  LAYER_TRANSLATE  */

static Argument *
layer_translate_invoker (Argument *args)
{
  Layer *layer, *tmp_layer;
  Layer *floating_layer;
  GImage *gimage;
  GSList *layer_list;
  int offx, offy;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      offx = args[1].value.pdb_int;
      offy = args[2].value.pdb_int;
    }
  if (success)
    success = ((gimage = gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID)) != NULL);

  if (success)
    {
      floating_layer = gimage_floating_sel (gimage);

      undo_push_group_start (gimage, LINKED_LAYER_UNDO);

      if (floating_layer)
	floating_sel_relax (floating_layer, TRUE);

      layer_list = gimage->layers;
      while (layer_list)
	{
	  tmp_layer = (Layer *) layer_list->data;
	  if ((tmp_layer == layer) || tmp_layer->linked)
	    layer_translate (tmp_layer, offx, offy);
	  layer_list = g_slist_next (layer_list);
	}

      if (floating_layer)
	floating_sel_rigor (floating_layer, TRUE);

      undo_push_group_end (gimage);
    }

  return procedural_db_return_args (&layer_translate_proc, success);
}

/*  The procedure definition  */
ProcArg layer_translate_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "offx",
    "offset in x direction"
  },
  { PDB_INT32,
    "offy",
    "offset in y direction"
  }
};

ProcRecord layer_translate_proc =
{
  "gimp_layer_translate",
  "Translate the layer by the specified offsets",
  "This procedure translates the layer by the amounts specified in the x and y arguments.  These can be negative, and are considered offsets from the current position.  This command only works if the layer has been added to an image.  All additional layers contained in the image which have the linked flag set to TRUE will also be translated by the specified offsets.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  layer_translate_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_translate_invoker } },
};


/*********************/
/*  LAYER_ADD_ALPHA  */

static Argument *
layer_add_alpha_invoker (Argument *args)
{
  Layer *layer;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    layer_add_alpha (layer);

  return procedural_db_return_args (&layer_add_alpha_proc, success);
}

/*  The procedure definition  */
ProcArg layer_add_alpha_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcRecord layer_add_alpha_proc =
{
  "gimp_layer_add_alpha",
  "Add an alpha channel to the layer if it doesn't already have one.",
  "This procedure adds an additional component to the specified layer if it does not already possess an alpha channel.  An alpha channel makes it possible to move a layer from the bottom of the layer stack and to clear and erase to transparency, instead of the background color.  This transforms images of type RGB to RGBA, Gray to GrayA, and Indexed to IndexedA.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_add_alpha_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_add_alpha_invoker } },
};


/********************/
/*  LAYER_GET_NAME  */

static Argument *
layer_get_name_invoker (Argument *args)
{
  Layer *layer;
  char *name = NULL;
  Argument *return_args;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)))
	name = GIMP_DRAWABLE(layer)->name;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&layer_get_name_proc, success);

  if (success)
    return_args[1].value.pdb_pointer = (name) ? g_strdup (name) : NULL;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_get_name_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcArg layer_get_name_out_args[] =
{
  { PDB_STRING,
    "name",
    "the layer name"
  }
};

ProcRecord layer_get_name_proc =
{
  "gimp_layer_get_name",
  "Get the name of the specified layer.",
  "This procedure returns the specified layer's name.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_get_name_args,

  /*  Output arguments  */
  1,
  layer_get_name_out_args,

  /*  Exec method  */
  { { layer_get_name_invoker } },
};


/********************/
/*  LAYER_SET_NAME  */

static Argument *
layer_set_name_invoker (Argument *args)
{
  Layer *layer;
  char *name;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      name = (char *) args[1].value.pdb_pointer;
      if (GIMP_DRAWABLE(layer)->name)
	{
	  g_free (GIMP_DRAWABLE(layer)->name);
	  GIMP_DRAWABLE(layer)->name = (name) ? g_strdup (name) : NULL;
	}
    }

  return procedural_db_return_args (&layer_set_name_proc, success);
}

/*  The procedure definition  */
ProcArg layer_set_name_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_STRING,
    "name",
    "the new layer name"
  }
};

ProcRecord layer_set_name_proc =
{
  "gimp_layer_set_name",
  "Set the name of the specified layer.",
  "This procedure sets the specified layer's name to the supplied name.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  layer_set_name_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_set_name_invoker } },
};


/***********************/
/*  LAYER_GET_VISIBLE  */

static Argument *
layer_get_visible_invoker (Argument *args)
{
  Layer *layer;
  int visible;
  Argument *return_args;

  visible = FALSE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)))
	visible = GIMP_DRAWABLE(layer)->visible;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&layer_get_visible_proc, success);

  if (success)
    return_args[1].value.pdb_int = visible;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_get_visible_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcArg layer_get_visible_out_args[] =
{
  { PDB_INT32,
    "visible",
    "the layer visibility"
  }
};

ProcRecord layer_get_visible_proc =
{
  "gimp_layer_get_visible",
  "Get the visibility of the specified layer.",
  "This procedure returns the specified layer's visibility.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_get_visible_args,

  /*  Output arguments  */
  1,
  layer_get_visible_out_args,

  /*  Exec method  */
  { { layer_get_visible_invoker } },
};


/***********************/
/*  LAYER_SET_VISIBLE  */

static Argument *
layer_set_visible_invoker (Argument *args)
{
  Layer *layer;
  int visible;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      visible = args[1].value.pdb_int;
      GIMP_DRAWABLE(layer)->visible = (visible) ? TRUE : FALSE;
    }

  return procedural_db_return_args (&layer_set_visible_proc, success);
}

/*  The procedure definition  */
ProcArg layer_set_visible_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "visible",
    "the new layer visibility"
  }
};

ProcRecord layer_set_visible_proc =
{
  "gimp_layer_set_visible",
  "Set the visibility of the specified layer.",
  "This procedure sets the specified layer's visibility.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  layer_set_visible_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_set_visible_invoker } },
};


/******************************/
/*  LAYER_GET_PRESERVE_TRANS  */

static Argument *
layer_get_preserve_trans_invoker (Argument *args)
{
  Layer *layer;
  int preserve_trans;
  Argument *return_args;

  preserve_trans = FALSE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)))
	preserve_trans = layer->preserve_trans;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&layer_get_preserve_trans_proc, success);

  if (success)
    return_args[1].value.pdb_int = preserve_trans;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_get_preserve_trans_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcArg layer_get_preserve_trans_out_args[] =
{
  { PDB_INT32,
    "preserve_trans",
    "the layer's preserve transparency setting"
  }
};

ProcRecord layer_get_preserve_trans_proc =
{
  "gimp_layer_get_preserve_trans",
  "Get the perserve transparency setting of the layer",
  "This procedure returns the specified layer's preserve transparency setting.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_get_preserve_trans_args,

  /*  Output arguments  */
  1,
  layer_get_preserve_trans_out_args,

  /*  Exec method  */
  { { layer_get_preserve_trans_invoker } },
};


/******************************/
/*  LAYER_SET_PRESERVE_TRANS  */

static Argument *
layer_set_preserve_trans_invoker (Argument *args)
{
  Layer *layer;
  int preserve_trans;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      preserve_trans = args[1].value.pdb_int;
      layer->preserve_trans = (preserve_trans) ? TRUE : FALSE;
    }

  return procedural_db_return_args (&layer_set_preserve_trans_proc, success);
}

/*  The procedure definition  */
ProcArg layer_set_preserve_trans_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "preserve_trans",
    "the new layer preserve transparency setting"
  }
};

ProcRecord layer_set_preserve_trans_proc =
{
  "gimp_layer_set_preserve_trans",
  "Set the preserve transparency parameter for the specified layer.",
  "This procedure sets the specified layer's preserve transparency option.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  layer_set_preserve_trans_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_set_preserve_trans_invoker } },
};


/**************************/
/*  LAYER_GET_APPLY_MASK  */

static Argument *
layer_get_apply_mask_invoker (Argument *args)
{
  Layer *layer;
  int apply_mask;
  Argument *return_args;

  apply_mask = FALSE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)))
	apply_mask = layer->apply_mask;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&layer_get_apply_mask_proc, success);

  if (success)
    return_args[1].value.pdb_int = apply_mask;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_get_apply_mask_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcArg layer_get_apply_mask_out_args[] =
{
  { PDB_INT32,
    "apply_mask",
    "the layer's apply mask setting"
  }
};

ProcRecord layer_get_apply_mask_proc =
{
  "gimp_layer_get_apply_mask",
  "Get the apply mask setting for the layer",
  "This procedure returns the specified layer's apply mask setting.  If the return value is non-zero, then the layer mask for this layer is currently being composited with the layer's alpha channel.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_get_apply_mask_args,

  /*  Output arguments  */
  1,
  layer_get_apply_mask_out_args,

  /*  Exec method  */
  { { layer_get_apply_mask_invoker } },
};


/**************************/
/*  LAYER_SET_APPLY_MASK  */

static Argument *
layer_set_apply_mask_invoker (Argument *args)
{
  Layer *layer;
  int apply_mask;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      apply_mask = args[1].value.pdb_int;
      if (layer->mask)
	layer->apply_mask = (apply_mask) ? TRUE : FALSE;
      else
	success = FALSE;
    }

  return procedural_db_return_args (&layer_set_apply_mask_proc, success);
}

/*  The procedure definition  */
ProcArg layer_set_apply_mask_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "apply_mask",
    "the new layer apply mask setting"
  }
};

ProcRecord layer_set_apply_mask_proc =
{
  "gimp_layer_set_apply_mask",
  "Set the apply mask parameter for the specified layer.",
  "This procedure sets the specified layer's apply mask parameter.  This controls whether the layer's mask is currently affecting the alpha channel.  If there is no layer mask, this function will return an error",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  layer_set_apply_mask_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_set_apply_mask_invoker } },
};


/*************************/
/*  LAYER_GET_SHOW_MASK  */

static Argument *
layer_get_show_mask_invoker (Argument *args)
{
  Layer *layer;
  int show_mask;
  Argument *return_args;

  show_mask = FALSE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)))
	show_mask = layer->show_mask;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&layer_get_show_mask_proc, success);

  if (success)
    return_args[1].value.pdb_int = show_mask;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_get_show_mask_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcArg layer_get_show_mask_out_args[] =
{
  { PDB_INT32,
    "show_mask",
    "the layer's show mask setting"
  }
};

ProcRecord layer_get_show_mask_proc =
{
  "gimp_layer_get_show_mask",
  "Get the show mask setting for the layer",
  "This procedure returns the specified layer's show mask setting.  If the value is non-zero, then the layer's mask is currently being shown instead of the layer.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_get_show_mask_args,

  /*  Output arguments  */
  1,
  layer_get_show_mask_out_args,

  /*  Exec method  */
  { { layer_get_show_mask_invoker } },
};


/*************************/
/*  LAYER_SET_SHOW_MASK  */

static Argument *
layer_set_show_mask_invoker (Argument *args)
{
  Layer *layer;
  int show_mask;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      show_mask = args[1].value.pdb_int;
      if (layer->mask)
	layer->show_mask = (show_mask) ? TRUE : FALSE;
      else
	success = FALSE;
    }

  return procedural_db_return_args (&layer_set_show_mask_proc, success);
}

/*  The procedure definition  */
ProcArg layer_set_show_mask_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "show_mask",
    "the new layer show mask setting"
  }
};

ProcRecord layer_set_show_mask_proc =
{
  "gimp_layer_set_show_mask",
  "Set the show mask parameter for the specified layer.",
  "This procedure sets the specified layer's show mask parameter.  This setting controls whether the layer or it's mask is visible.  Non-zero values indicate that the mask should be visible.  If the layer has no mask, then this function returns an error.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  layer_set_show_mask_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_set_show_mask_invoker } },
};


/*************************/
/*  LAYER_GET_EDIT_MASK  */

static Argument *
layer_get_edit_mask_invoker (Argument *args)
{
  Layer *layer;
  int edit_mask;
  Argument *return_args;

  edit_mask = FALSE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)))
	edit_mask = layer->edit_mask;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&layer_get_edit_mask_proc, success);

  if (success)
    return_args[1].value.pdb_int = edit_mask;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_get_edit_mask_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcArg layer_get_edit_mask_out_args[] =
{
  { PDB_INT32,
    "edit_mask",
    "the layer's edit mask setting"
  }
};

ProcRecord layer_get_edit_mask_proc =
{
  "gimp_layer_get_edit_mask",
  "Get the edit mask setting for the layer",
  "This procedure returns the specified layer's edit mask setting.  If the value is non-zero, then the layer's mask is currently active, and not the layer.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_get_edit_mask_args,

  /*  Output arguments  */
  1,
  layer_get_edit_mask_out_args,

  /*  Exec method  */
  { { layer_get_edit_mask_invoker } },
};


/*************************/
/*  LAYER_SET_EDIT_MASK  */

static Argument *
layer_set_edit_mask_invoker (Argument *args)
{
  Layer *layer;
  int edit_mask;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      edit_mask = args[1].value.pdb_int;
      if (layer->mask)
	layer->edit_mask = (edit_mask) ? TRUE : FALSE;
      else
	success = FALSE;
    }

  return procedural_db_return_args (&layer_set_edit_mask_proc, success);
}

/*  The procedure definition  */
ProcArg layer_set_edit_mask_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "edit_mask",
    "the new layer edit mask setting"
  }
};

ProcRecord layer_set_edit_mask_proc =
{
  "gimp_layer_set_edit_mask",
  "Set the edit mask parameter for the specified layer.",
  "This procedure sets the specified layer's edit mask parameter.  This setting controls whether the layer or it's mask is currently active for editing.  If the specified layer has no layer mask, then this procedure will return an error.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  layer_set_edit_mask_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_set_edit_mask_invoker } },
};


/***********************/
/*  LAYER_GET_OPACITY  */

static Argument *
layer_get_opacity_invoker (Argument *args)
{
  Layer *layer;
  double opacity;
  Argument *return_args;

  opacity = 0.0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)))
	opacity = ((layer->opacity) * 100.0) / 255.0;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&layer_get_opacity_proc, success);

  if (success)
    return_args[1].value.pdb_float = opacity;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_get_opacity_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcArg layer_get_opacity_out_args[] =
{
  { PDB_FLOAT,
    "opacity",
    "the layer opacity",
  }
};

ProcRecord layer_get_opacity_proc =
{
  "gimp_layer_get_opacity",
  "Get the opacity of the specified layer.",
  "This procedure returns the specified layer's opacity.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_get_opacity_args,

  /*  Output arguments  */
  1,
  layer_get_opacity_out_args,

  /*  Exec method  */
  { { layer_get_opacity_invoker } },
};


/***********************/
/*  LAYER_SET_OPACITY  */

static Argument *
layer_set_opacity_invoker (Argument *args)
{
  Layer *layer;
  double opacity;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      opacity = args[1].value.pdb_float;
      layer->opacity = (int) ((opacity * 255) / 100);
    }

  return procedural_db_return_args (&layer_set_opacity_proc, success);
}

/*  The procedure definition  */
ProcArg layer_set_opacity_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_FLOAT,
    "opacity",
    "the new layer opacity: (0 <= opacity <= 100)"
  }
};

ProcRecord layer_set_opacity_proc =
{
  "gimp_layer_set_opacity",
  "Set the opacity of the specified layer.",
  "This procedure sets the specified layer's opacity.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  layer_set_opacity_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_set_opacity_invoker } },
};


/********************/
/*  LAYER_GET_MODE  */

static Argument *
layer_get_mode_invoker (Argument *args)
{
  Layer *layer;
  int mode;
  Argument *return_args;

  mode = 0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)))
	mode = layer->mode;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&layer_get_mode_proc, success);

  if (success)
    return_args[1].value.pdb_int = mode;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_get_mode_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcArg layer_get_mode_out_args[] =
{
  { PDB_INT32,
    "mode",
    "the layer combination mode: { NORMAL (0), DISSOLVE (1), MULTIPLY (3), SCREEN (4), OVERLAY (5) DIFFERENCE (6), ADDITION (7), SUBTRACT (8), DARKEN-ONLY (9), LIGHTEN-ONLY (10), HUE (11), SATURATION (12), COLOR (13), VALUE (14) }"
  }
};

ProcRecord layer_get_mode_proc =
{
  "gimp_layer_get_mode",
  "Get the combination mode of the specified layer.",
  "This procedure returns the specified layer's combination mode.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_get_mode_args,

  /*  Output arguments  */
  1,
  layer_get_mode_out_args,

  /*  Exec method  */
  { { layer_get_mode_invoker } },
};


/********************/
/*  LAYER_SET_MODE  */

static Argument *
layer_set_mode_invoker (Argument *args)
{
  Layer *layer;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (int_value >= NORMAL_MODE && int_value <= VALUE_MODE)
	layer->mode = int_value;
      else
	success = FALSE;
    }

  return procedural_db_return_args (&layer_set_mode_proc, success);
}

/*  The procedure definition  */
ProcArg layer_set_mode_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "mode",
    "the new layer combination mode"
  }
};

ProcRecord layer_set_mode_proc =
{
  "gimp_layer_set_mode",
  "Set the combination mode of the specified layer.",
  "This procedure sets the specified layer's combination mode.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  layer_set_mode_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_set_mode_invoker } },
};


/***********************/
/*  LAYER_SET_OFFSETS  */

static Argument *
layer_set_offsets_invoker (Argument *args)
{
  Layer *layer;
  Layer *floating_layer;
  GImage *gimage;
  int offx, offy;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      offx = args[1].value.pdb_int;
      offy = args[2].value.pdb_int;
    }
  if (success)
    success = ((gimage = gimage_get_ID (GIMP_DRAWABLE(layer)->gimage_ID)) != NULL);

  if (success)
    {
      floating_layer = gimage_floating_sel (gimage);

      undo_push_group_start (gimage, LINKED_LAYER_UNDO);

      if (floating_layer)
	floating_sel_relax (floating_layer, TRUE);

      layer_translate (layer, (offx - GIMP_DRAWABLE(layer)->offset_x), (offy - GIMP_DRAWABLE(layer)->offset_y));

      if (floating_layer)
	floating_sel_rigor (floating_layer, TRUE);

      undo_push_group_end (gimage);
    }

  return procedural_db_return_args (&layer_set_offsets_proc, success);
}

/*  The procedure definition  */
ProcArg layer_set_offsets_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "offx",
    "offset in x direction"
  },
  { PDB_INT32,
    "offy",
    "offset in y direction"
  }
};

ProcRecord layer_set_offsets_proc =
{
  "gimp_layer_set_offsets",
  "Set the layer offsets",
  "This procedure sets the offsets for the specified layer.  The offsets are relative to the image origin and can be any values.  This operation is valid only on layers which have been added to an image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  layer_set_offsets_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { layer_set_offsets_invoker } },
};


/****************/
/*  LAYER_MASK  */

static Argument *
layer_mask_invoker (Argument *args)
{
  Layer *layer;
  int mask;
  Argument *return_args;

  mask = -1;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)))
	mask = (layer->mask) ? GIMP_DRAWABLE(layer->mask)->ID : -1;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&layer_mask_proc, success);

  if (success)
    return_args[1].value.pdb_int = mask;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_mask_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcArg layer_mask_out_args[] =
{
  { PDB_CHANNEL,
    "mask",
    "the layer mask"
  }
};

ProcRecord layer_mask_proc =
{
  "gimp_layer_mask",
  "Get the specified layer's mask if it exists.",
  "This procedure returns the specified layer's mask, or -1 if none exists.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_mask_args,

  /*  Output arguments  */
  1,
  layer_mask_out_args,

  /*  Exec method  */
  { { layer_mask_invoker } },
};


/***************************/
/*  LAYER_IS_FLOATING_SEL  */

static Argument *
layer_is_floating_sel_invoker (Argument *args)
{
  Layer *layer;
  int is_floating_sel;
  Argument *return_args;

  is_floating_sel = FALSE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((layer = layer_get_ID (int_value)))
	is_floating_sel = layer_is_floating_sel (layer);
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&layer_is_floating_sel_proc, success);

  if (success)
    return_args[1].value.pdb_int = is_floating_sel;

  return return_args;
}

/*  The procedure definition  */
ProcArg layer_is_floating_sel_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcArg layer_is_floating_sel_out_args[] =
{
  { PDB_CHANNEL,
    "is_floating_sel",
    "non-zero if the layer is a floating selection"
  }
};

ProcRecord layer_is_floating_sel_proc =
{
  "gimp_layer_is_floating_sel",
  "Is the specified layer a floating selection?",
  "This procedure returns whether the layer is a floating selection.  Floating selections are special cases of layers which are attached to a specific drawable.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  layer_is_floating_sel_args,

  /*  Output arguments  */
  1,
  layer_is_floating_sel_out_args,

  /*  Exec method  */
  { { layer_is_floating_sel_invoker } },
};
