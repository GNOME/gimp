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
#include <string.h>
#include "appenv.h"
#include "drawable.h"
#include "general.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_cmds.h"
#include "floating_sel.h"
#include "libgimp/parasite.h"
#include "undo.h"

#include "layer_pvt.h"			/* ick. */
#include "drawable_pvt.h"		/* ick ick. */

static int int_value;
static float float_value;
static int success;
static Argument *return_args;


/* The Guides procs prototypes */
static Argument* gimp_image_add_hguide_invoker   (Argument*);
static Argument* gimp_image_add_vguide_invoker   (Argument*);
static Argument* gimp_image_delete_guide_invoker (Argument*);
static Argument* gimp_image_findnext_guide_invoker        (Argument*);
static Argument* gimp_image_get_guide_orientation_invoker (Argument*);
static Argument* gimp_image_get_guide_position_invoker    (Argument*);

/* The Parasite procs prototypes */
static Argument *gimp_image_find_parasite_invoker (Argument *);
static Argument *gimp_image_attach_parasite_invoker (Argument *);
static Argument *gimp_image_detach_parasite_invoker (Argument *);

/* The Tattoo procs prototypes */
static Argument *gimp_image_get_layer_by_tattoo_invoker (Argument *);
static Argument *gimp_image_get_channel_by_tattoo_invoker (Argument *);

static GImage * duplicate  (GImage *gimage);

static Argument * channel_ops_duplicate_invoker  (Argument *args);
/************************/
/*  GIMAGE_LIST_IMAGES  */

/* Yuup, this is somewhat unsmooth, to say the least */

static void
gimlist_cb(gpointer im, gpointer data){
	GSList** l=(GSList**)data;
	*l=g_slist_prepend(*l, im);
}

static Argument *
gimage_list_images_invoker (Argument *args)
{
  GSList *list=NULL;
  int num_images;
  int *image_ids;
  Argument *return_args;

  return_args = procedural_db_return_args (&gimage_get_layers_proc, TRUE);

  gimage_foreach(gimlist_cb, &list);
  
  num_images = g_slist_length (list);
  image_ids = NULL;

  if (num_images)
    {
       int i;
       image_ids = (int *) g_malloc (sizeof (int) * num_images);
       for (i = 0; i < num_images; i++, list = g_slist_next (list))
	       image_ids[i] = pdb_image_to_id(GIMP_IMAGE(list->data));
       
    }
  return_args[1].value.pdb_int = num_images;
  return_args[2].value.pdb_pointer = image_ids;
  g_slist_free(list);
  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_list_images_out_args[] =
{
  { PDB_INT32,
    "num_images",
    "The number of images currently open"
  },
  { PDB_INT32ARRAY,
    "image_ids",
    "The list of images currently open"
  }
};

ProcRecord gimage_list_images_proc =
{
  "gimp_list_images",
  "Returns the list of images currently open",
  "This procedure returns the list of images currently open in the GIMP.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  2,
  gimage_list_images_out_args,

  /*  Exec method  */
  { { gimage_list_images_invoker } },
};


/**********************/
/*  GIMAGE_NEW_IMAGE  */

static Argument *
gimage_new_invoker (Argument *args)
{
  int type;
  int width, height;
  GImage *gimage;

  type   = RGB;
  width  = 1;
  height = 1;
  gimage = NULL;

  success = TRUE;

  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (int_value > 0)
	width = int_value;
      else
	success = FALSE;
    }
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (int_value > 0)
	height = int_value;
      else
	success = FALSE;
    }
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value >= RGB && int_value <= INDEXED)
	type = int_value;
      else
	success = FALSE;
    }

  /*  create the new image  */
  if (success)
    success = ((gimage = gimage_new (width, height, type)) != NULL);

  return_args = procedural_db_return_args (&gimage_new_proc, success);

  if (success)
    return_args[1].value.pdb_int = pdb_image_to_id(gimage);

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_new_args[] =
{
  { PDB_INT32,
    "width",
    "The width of the image"
  },
  { PDB_INT32,
    "height",
    "The height of the image"
  },
  { PDB_INT32,
    "type",
    "The type of image: { RGB (0), GRAY (1), INDEXED (2) }"
  }
};

ProcArg gimage_new_out_args[] =
{
  { PDB_IMAGE,
    "image",
    "The ID of the newly created image"
  }
};

ProcRecord gimage_new_proc =
{
  "gimp_image_new",
  "Creates a new image with the specified width, height, and type",
  "Creates a new image, undisplayed with the specified extents and type.  A layer should be created and added before this image is displayed, or subsequent calls to 'gimp_display_new' with this image as an argument will fail.  Layers can be created using the 'gimp_layer_new' commands.  They can be added to an image using the 'gimp_image_add_layer' command.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_new_args,

  /*  Output arguments  */
  1,
  gimage_new_out_args,

  /*  Exec method  */
  { { gimage_new_invoker } },
};


/*******************/
/*  GIMAGE_RESIZE  */

static Argument *
gimage_resize_invoker (Argument *args)
{
  GImage *gimage;
  int new_width, new_height;
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
    gimage_resize (gimage, new_width, new_height, offx, offy);

  return procedural_db_return_args (&gimage_resize_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_resize_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_INT32,
    "new_width",
    "new image width: (new_width > 0)"
  },
  { PDB_INT32,
    "new_height",
    "new image height: (new_height > 0)"
  },
  { PDB_INT32,
    "offx",
    "x offset between upper left corner of old and new images: (new - old)"
  },
  { PDB_INT32,
    "offy",
    "y offset between upper left corner of old and new images: (new - old)"
  }
};

ProcRecord gimage_resize_proc =
{
  "gimp_image_resize",
  "Resize the image to the specified extents.",
  "This procedure resizes the image so that it's new width and height are equal to the supplied parameters.  Offsets are also provided which describe the position of the previous image's content.  No bounds checking is currently provided, so don't supply parameters that are out of bounds.  All channels within the image are resized according to the specified parameters; this includes the image selection mask.  All layers within the image are repositioned according to the specified offsets.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  gimage_resize_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_resize_invoker } },
};


/*******************/
/*  GIMAGE_SCALE  */

static Argument *
gimage_scale_invoker (Argument *args)
{
  GImage *gimage;
  int new_width, new_height;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
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
    gimage_scale (gimage, new_width, new_height);

  return procedural_db_return_args (&gimage_scale_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_scale_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_INT32,
    "new_width",
    "new image width: (new_width > 0)"
  },
  { PDB_INT32,
    "new_height",
    "new image height: (new_height > 0)"
  }
};

ProcRecord gimage_scale_proc =
{
  "gimp_image_scale",
  "Scale the image to the specified extents.",
  "This procedure scales the image so that it's new width and height are equal to the supplied parameters.  Offsets are also provided which describe the position of the previous image's content.  No bounds checking is currently provided, so don't supply parameters that are out of bounds.  All channels within the image are scaled according to the specified parameters; this includes the image selection mask.  All layers within the image are repositioned according to the specified offsets.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_scale_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_scale_invoker } },
};


/*******************/
/*  GIMAGE_DELETE  */

static Argument *
gimage_delete_invoker (Argument *args)
{
  GImage *gimage;

  success = TRUE;

  int_value = args[0].value.pdb_int;
  if ((gimage = gimage_get_ID (int_value)))
    gimage_delete (gimage);
  else
    success = FALSE;

  return procedural_db_return_args (&gimage_delete_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_delete_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image ID"
  }
};

ProcRecord gimage_delete_proc =
{
  "gimp_image_delete",
  "Delete the specified image",
  "If there are no other references to this image it will be deleted.  Other references are possible when more than one view to an image exists.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_delete_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_delete_invoker } },
};


/************************/
/*  GIMAGE_FREE_SHADOW  */

static Argument *
gimage_free_shadow_invoker (Argument *args)
{
  GImage *gimage;

  success = TRUE;

  int_value = args[0].value.pdb_int;
  if ((gimage = gimage_get_ID (int_value)))
    gimage_free_shadow (gimage);
  else
    success = FALSE;

  return procedural_db_return_args (&gimage_free_shadow_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_free_shadow_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image ID"
  }
};

ProcRecord gimage_free_shadow_proc =
{
  "gimp_image_free_shadow",
  "Free the specified image's shadow data (if it exists)",
  "This procedure is intended as a memory saving device.  If any shadow memory has been allocated, it will be freed automatically on a call to 'gimp_image_delete'.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_free_shadow_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_free_shadow_invoker } },
};


/***********************/
/*  GIMAGE_GET_LAYERS  */

static Argument *
gimage_get_layers_invoker (Argument *args)
{
  GImage *gimage;
  GSList *layer_list;
  int num_layers;
  int *layer_ids;
  Argument *return_args;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      success = ((gimage = gimage_get_ID (int_value)) != NULL);
    }

  return_args = procedural_db_return_args (&gimage_get_layers_proc, success);

  if (success)
    {
      layer_list = gimage->layers;
      num_layers = g_slist_length (layer_list);
      layer_ids = NULL;

      if (num_layers)
	{
	  int i;

	  layer_ids = (int *) g_malloc (sizeof (int) * num_layers);

	  for (i = 0; i < num_layers; i++, layer_list = g_slist_next (layer_list))
	    layer_ids[i] = drawable_ID (GIMP_DRAWABLE(((Layer *) layer_list->data)));
	}

      return_args[1].value.pdb_int = num_layers;
      return_args[2].value.pdb_pointer = layer_ids;
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_get_layers_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  }
};

ProcArg gimage_get_layers_out_args[] =
{
  { PDB_INT32,
    "num_layers",
    "The number of layers contained in the image"
  },
  { PDB_INT32ARRAY,
    "layer_ids",
    "The list of layers contained in the image"
  }
};

ProcRecord gimage_get_layers_proc =
{
  "gimp_image_get_layers",
  "Returns the list of layers contained in the specified image",
  "This procedure returns the list of layers contained in the specified image.  The order of layers is from topmost to bottommost.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_get_layers_args,

  /*  Output arguments  */
  2,
  gimage_get_layers_out_args,

  /*  Exec method  */
  { { gimage_get_layers_invoker } },
};


/*************************/
/*  GIMAGE_GET_CHANNELS  */

static Argument *
gimage_get_channels_invoker (Argument *args)
{
  GImage *gimage;
  int num_channels;
  int *channel_ids;
  GSList *channel_list;
  Argument *return_args;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      success = ((gimage = gimage_get_ID (int_value)) != NULL);
    }

  return_args = procedural_db_return_args (&gimage_get_channels_proc, success);

  if (success)
    {
      channel_list = gimage->channels;
      num_channels = g_slist_length (channel_list);
      channel_ids = NULL;

      if (num_channels)
	{
	  int i;

	  channel_ids = (int *) g_malloc (sizeof (int) * num_channels);

	  for (i = 0; i < num_channels; i++, channel_list = g_slist_next (channel_list))
	    channel_ids[i] = drawable_ID (GIMP_DRAWABLE(((Channel *) channel_list->data)));
	}

      return_args[1].value.pdb_int = num_channels;
      return_args[2].value.pdb_pointer = channel_ids;
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_get_channels_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  }
};

ProcArg gimage_get_channels_out_args[] =
{
  { PDB_INT32,
    "num_channels",
    "The number of channels contained in the image"
  },
  { PDB_INT32ARRAY,
    "channel_ids",
    "The list of channels contained in the image"
  }
};

ProcRecord gimage_get_channels_proc =
{
  "gimp_image_get_channels",
  "Returns the list of channels contained in the specified image",
  "This procedure returns the list of channels contained in the specified image.  This does not include the selection mask, or layer masks.  The order is from topmost to bottommost.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_get_channels_args,

  /*  Output arguments  */
  2,
  gimage_get_channels_out_args,

  /*  Exec method  */
  { { gimage_get_channels_invoker } },
};


/*****************************/
/*  GIMAGE_GET_ACTIVE_LAYER  */

static Argument *
gimage_get_active_layer_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;
  Argument *return_args;

  layer = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	layer = gimage_get_active_layer (gimage);
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gimage_get_active_layer_proc, success);

  if (success)
    return_args[1].value.pdb_int = (layer) ? drawable_ID (GIMP_DRAWABLE(layer)) : -1;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_get_active_layer_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  }
};

ProcArg gimage_get_active_layer_out_args[] =
{
  { PDB_LAYER,
    "layer_ID",
    "The ID of the active layer"
  }
};

ProcRecord gimage_get_active_layer_proc =
{
  "gimp_image_get_active_layer",
  "Returns the active layer of the specified image",
  "If there is an active layer, its ID will be returned, otherwise, -1.  If a channel is currently active, then no layer will be.  If a layer mask is active, then this will return the associated layer.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_get_active_layer_args,

  /*  Output arguments  */
  1,
  gimage_get_active_layer_out_args,

  /*  Exec method  */
  { { gimage_get_active_layer_invoker } },
};


/*******************************/
/*  GIMAGE_GET_ACTIVE_CHANNEL  */

static Argument *
gimage_get_active_channel_invoker (Argument *args)
{
  GImage *gimage;
  Channel *channel;
  Argument *return_args;

  channel = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	channel = gimage_get_active_channel (gimage);
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gimage_get_active_channel_proc, success);

  if (success)
    return_args[1].value.pdb_int = (channel) ? drawable_ID (GIMP_DRAWABLE(channel)) : -1;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_get_active_channel_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  }
};

ProcArg gimage_get_active_channel_out_args[] =
{
  { PDB_CHANNEL,
    "channel ID",
    "The ID of the active channel"
  }
};

ProcRecord gimage_get_active_channel_proc =
{
  "gimp_image_get_active_channel",
  "Returns the active channel of the specified image",
  "If there is an active channel, this will return the channel ID, otherwise, -1.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_get_active_channel_args,

  /*  Output arguments  */
  1,
  gimage_get_active_channel_out_args,

  /*  Exec method  */
  { { gimage_get_active_channel_invoker } },
};


/**************************/
/*  GIMAGE_GET_SELECTION  */

static Argument *
gimage_get_selection_invoker (Argument *args)
{
  GImage *gimage;
  Channel *mask;
  Argument *return_args;

  mask = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	success = ((mask = gimage_get_mask (gimage)) != NULL);
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gimage_get_selection_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(mask));

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_get_selection_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  }
};

ProcArg gimage_get_selection_out_args[] =
{
  { PDB_SELECTION,
    "selection mask ID",
    "The ID of the selection channel"
  }
};

ProcRecord gimage_get_selection_proc =
{
  "gimp_image_get_selection",
  "Returns the selection of the specified image",
  "This will always return a valid ID for a selection--which is represented as a channel internally.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_get_selection_args,

  /*  Output arguments  */
  1,
  gimage_get_selection_out_args,

  /*  Exec method  */
  { { gimage_get_selection_invoker } },
};


/*********************************/
/*  GIMAGE_GET_COMPONENT_ACTIVE  */

static Argument *
gimage_get_component_active_invoker (Argument *args)
{
  GImage *gimage;
  ChannelType comp_type;
  Argument *return_args;

  comp_type = Red;

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
      switch (int_value)
	{
	case 0:
	  comp_type = Red;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 1:
	  comp_type = Green;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 2:
	  comp_type = Blue;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 3:
	  comp_type = Gray;
	  success = (gimage_base_type (gimage) == GRAY);
	  break;
	case 4:
	  comp_type = Indexed;
	  success = (gimage_base_type (gimage) == INDEXED);
	  break;
	default: success = FALSE;
	}
    }

  return_args = procedural_db_return_args (&gimage_get_component_active_proc, success);

  if (success)
    return_args[1].value.pdb_int = gimage_get_component_active (gimage, comp_type);

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_get_component_active_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_INT32,
    "component",
    "The image component: { RED-CHANNEL (0), GREEN-CHANNEL (1), BLUE-CHANNEL (2), GRAY-CHANNEL (3), INDEXED-CHANNEL (4) }"
  }
};

ProcArg gimage_get_component_active_out_args[] =
{
  { PDB_INT32,
    "active",
    "1 for active, 0 for inactive"
  }
};

ProcRecord gimage_get_component_active_proc =
{
  "gimp_image_get_component_active",
  "Returns whether the specified component is active",
  "This procedure returns information on whether the specified image component (ie. red, green, blue intensity channels in an RGB image) is active or inactive--whether or not it can be modified.  If the specified component is not valid for the image type, an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_get_component_active_args,

  /*  Output arguments  */
  1,
  gimage_get_component_active_out_args,

  /*  Exec method  */
  { { gimage_get_component_active_invoker } },
};


/*********************************/
/*  GIMAGE_GET_COMPONENT_VISIBLE  */

static Argument *
gimage_get_component_visible_invoker (Argument *args)
{
  GImage *gimage;
  ChannelType comp_type;
  Argument *return_args;

  comp_type = Red;

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
      switch (int_value)
	{
	case 0:
	  comp_type = Red;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 1:
	  comp_type = Green;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 2:
	  comp_type = Blue;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 3:
	  comp_type = Gray;
	  success = (gimage_base_type (gimage) == GRAY);
	  break;
	case 4:
	  comp_type = Indexed;
	  success = (gimage_base_type (gimage) == INDEXED);
	  break;
	default: success = FALSE;
	}
    }

  return_args = procedural_db_return_args (&gimage_get_component_visible_proc, success);

  if (success)
    return_args[1].value.pdb_int = gimage_get_component_visible (gimage, comp_type);

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_get_component_visible_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_INT32,
    "component",
    "The image component: {RED-CHANNEL (0), GREEN-CHANNEL (1), BLUE-CHANNEL (2), GRAY-CHANNEL (3), INDEXED-CHANNEL (4) }"
  }
};

ProcArg gimage_get_component_visible_out_args[] =
{
  { PDB_INT32,
    "visible",
    "1 for visible, 0 for invisible"
  }
};

ProcRecord gimage_get_component_visible_proc =
{
  "gimp_image_get_component_visible",
  "Returns whether the specified component is visible",
  "This procedure returns information on whether the specified image component (ie. Red, Green, Blue intensity channels in an RGB image) is visible or invisible--whether or not it can be modified.  If the specified component is not valid for the image type, an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_get_component_visible_args,

  /*  Output arguments  */
  1,
  gimage_get_component_visible_out_args,

  /*  Exec method  */
  { { gimage_get_component_visible_invoker } },
};


/*****************************/
/*  GIMAGE_SET_ACTIVE_LAYER  */

static Argument *
gimage_set_active_layer_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;

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
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    gimage_set_active_layer (gimage, layer);

  return procedural_db_return_args (&gimage_set_active_layer_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_set_active_layer_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_LAYER,
    "layer",
    "The layer to be set active"
  }
};

ProcRecord gimage_set_active_layer_proc =
{
  "gimp_image_set_active_layer",
  "Sets the specified layer as active in the specified image.",
  "If the layer exists, it is set as the active layer in the image.  Any previous active layer or channel is set to inactive.  An exception is a previously existing floating selection, in which case this procedure will return an execution error.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_set_active_layer_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_set_active_layer_invoker } },
};


/*******************************/
/*  GIMAGE_SET_ACTIVE_CHANNEL  */

static Argument *
gimage_set_active_channel_invoker (Argument *args)
{
  GImage *gimage;
  Channel *channel;

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
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    gimage_set_active_channel (gimage, channel);

  return procedural_db_return_args (&gimage_set_active_channel_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_set_active_channel_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_CHANNEL,
    "channel",
    "The channel to be set active"
  }
};

ProcRecord gimage_set_active_channel_proc =
{
  "gimp_image_set_active_channel",
  "Sets the specified channel as active in the specified image.",
  "If the channel exists, it is set as the active channel in the image.  Any previous active channel or channel is set to inactive.  An exception is a previously existing floating selection, in which case this procedure will return an execution error.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_set_active_channel_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_set_active_channel_invoker } },
};


/*********************************/
/*  GIMAGE_UNSET_ACTIVE_CHANNEL  */

static Argument *
gimage_unset_active_channel_invoker (Argument *args)
{
  GImage *gimage;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	gimage_unset_active_channel (gimage);
    }

  return procedural_db_return_args (&gimage_unset_active_channel_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_unset_active_channel_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  }
};

ProcRecord gimage_unset_active_channel_proc =
{
  "gimp_image_unset_active_channel",
  "Unsets the active channel in the specified image.",
  "If an active channel exists, it is unset.  There then exists no active channel, and if desired, one can be set through a call to 'Set Active Channel'.  No error is returned in the case of no existing active channel.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_unset_active_channel_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_unset_active_channel_invoker } },
};


/*********************************/
/*  GIMAGE_SET_COMPONENT_ACTIVE  */

static Argument *
gimage_set_component_active_invoker (Argument *args)
{
  GImage *gimage;
  ChannelType comp_type;
  int active;

  comp_type = Red;

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
      switch (int_value)
	{
	case 0:
	  comp_type = Red;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 1:
	  comp_type = Green;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 2:
	  comp_type = Blue;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 3:
	  comp_type = Gray;
	  success = (gimage_base_type (gimage) == GRAY);
	  break;
	case 4:
	  comp_type = Indexed;
	  success = (gimage_base_type (gimage) == INDEXED);
	  break;
	default: success = FALSE;
	}
    }
  if (success)
    active = args[2].value.pdb_int;

  if (success)
    gimage_set_component_active (gimage, comp_type, active);

  return procedural_db_return_args (&gimage_set_component_active_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_set_component_active_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_INT32,
    "component",
    "The image component: { RED-CHANNEL (0), GREEN-CHANNEL (1), BLUE-CHANNEL (2), GRAY-CHANNEL (3), INDEXED-CHANNEL (4) }"
  },
  { PDB_INT32,
    "active",
    "Active? 1 for true, 0 for false"
  }
};

ProcRecord gimage_set_component_active_proc =
{
  "gimp_image_set_component_active",
  "Sets the specified component's sensitivity",
  "This procedure sets whether the specified component is active or inactive--that is, whether it can be affected during painting operations.  If the specified component is not valid for the image type, an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_set_component_active_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_set_component_active_invoker } },
};


/*********************************/
/*  GIMAGE_SET_COMPONENT_VISIBLE  */

static Argument *
gimage_set_component_visible_invoker (Argument *args)
{
  GImage *gimage;
  ChannelType comp_type;
  int visible;

  comp_type = Red;

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
      switch (int_value)
	{
	case 0:
	  comp_type = Red;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 1:
	  comp_type = Green;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 2:
	  comp_type = Blue;
	  success = (gimage_base_type (gimage) == RGB);
	  break;
	case 3:
	  comp_type = Gray;
	  success = (gimage_base_type (gimage) == GRAY);
	  break;
	case 4:
	  comp_type = Indexed;
	  success = (gimage_base_type (gimage) == INDEXED);
	  break;
	default: success = FALSE;
	}
    }
  if (success)
    visible = args[2].value.pdb_int;

  if (success)
    gimage_set_component_visible (gimage, comp_type, visible);

  return procedural_db_return_args (&gimage_set_component_visible_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_set_component_visible_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_INT32,
    "component",
    "The image component: { RED-CHANNEL (0), GREEN-CHANNEL (1), BLUE-CHANNEL (2), GRAY-CHANNEL (3), INDEXED-CHANNEL (4) }"
  },
  { PDB_INT32,
    "visible",
    "Visible? 1 for true, 0 for false"
  }
};

ProcRecord gimage_set_component_visible_proc =
{
  "gimp_image_set_component_visible",
  "Sets the specified component's visibility",
  "This procedure sets whether the specified component is visible or invisible.  If the specified component is not valid for the image type, an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_set_component_visible_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_set_component_visible_invoker } },
};


/*********************************/
/*  GIMAGE_PICK_CORRELATE_LAYER  */

static Argument *
gimage_pick_correlate_layer_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;
  int x, y;
  Argument *return_args;

  layer = NULL;

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
    layer = gimage_pick_correlate_layer (gimage, x, y);

  return_args = procedural_db_return_args (&gimage_pick_correlate_layer_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(layer));

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_pick_correlate_layer_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_INT32,
    "x",
    "The x coordinate for the pick"
  },
  { PDB_INT32,
    "y",
    "The y coordinate for the pick"
  },
};

ProcArg gimage_pick_correlate_layer_out_args[] =
{
  { PDB_LAYER,
    "layer",
    "The layer found at the specified coordinates"
  }
};

ProcRecord gimage_pick_correlate_layer_proc =
{
  "gimp_image_pick_correlate_layer",
  "Find the layer visible at the specified coordinates",
  "This procedure finds the layer which is visible at the specified coordinates.  Layers which do not qualify are those whose extents do not pass within the specified coordinates, or which are transparent at the specified coordinates.  This procedure will return -1 if no layer is found.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_pick_correlate_layer_args,

  /*  Output arguments  */
  1,
  gimage_pick_correlate_layer_out_args,

  /*  Exec method  */
  { { gimage_pick_correlate_layer_invoker } },
};


/************************/
/*  GIMAGE_RAISE_LAYER  */

static Argument *
gimage_raise_layer_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;

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
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = ((gimage_raise_layer (gimage, layer)) != NULL);

  return procedural_db_return_args (&gimage_raise_layer_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_raise_layer_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_LAYER,
    "layer",
    "The layer to raise"
  }
};

ProcRecord gimage_raise_layer_proc =
{
  "gimp_image_raise_layer",
  "Raise the specified layer in the image's layer stack",
  "This procedure raises the specified layer one step in the existing layer stack.  It will not move the layer if there is no layer above it, or the layer has no alpha channel.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_raise_layer_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_raise_layer_invoker } },
};


/************************/
/*  GIMAGE_LOWER_LAYER  */

static Argument *
gimage_lower_layer_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;

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
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = ((gimage_lower_layer (gimage, layer)) != NULL);

  return procedural_db_return_args (&gimage_lower_layer_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_lower_layer_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_LAYER,
    "layer",
    "The layer to lower"
  }
};

ProcRecord gimage_lower_layer_proc =
{
  "gimp_image_lower_layer",
  "Lower the specified layer in the image's layer stack",
  "This procedure lowers the specified layer one step in the existing layer stack.  It will not move the layer if there is no layer below it, or the layer has no alpha channel.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_lower_layer_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_lower_layer_invoker } },
};

/*******************************/
/*  GIMAGE_RAISE_LAYER_TO_TOP  */

static Argument *
gimage_raise_layer_to_top_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;

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
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = ((gimage_raise_layer_to_top (gimage, layer)) != NULL);

  return procedural_db_return_args (&gimage_raise_layer_to_top_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_raise_layer_to_top_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_LAYER,
    "layer",
    "The layer to raise to top"
  }
};

ProcRecord gimage_raise_layer_to_top_proc =
{
  "gimp_image_raise_layer_to_top",
  "Raise the specified layer in the image's layer stack to top of stack",
  "This procedure raises the specified layer to top of the existing layer stack.  It will not move the layer if there is no layer above it, or the layer has no alpha channel.",
  "Wolfgang Hofer, Sven Neumann",
  "Wolfgang Hofer",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_raise_layer_to_top_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_raise_layer_to_top_invoker } },
};


/**********************************/
/*  GIMAGE_LOWER_LAYER_TO_BOTTOM  */

static Argument *
gimage_lower_layer_to_bottom_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;

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
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = ((gimage_lower_layer_to_bottom (gimage, layer)) != NULL);

  return procedural_db_return_args (&gimage_lower_layer_to_bottom_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_lower_layer_to_bottom_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_LAYER,
    "layer",
    "The layer to lower to bottom"
  }
};

ProcRecord gimage_lower_layer_to_bottom_proc =
{
  "gimp_image_lower_layer_to_bottom",
  "Lower the specified layer to bottom of the image's layer stack",
  "This procedure lowers the specified layer to bottom of the existing layer stack.  It will not move the layer if there is no layer below it, or the layer has no alpha channel.",
  "Wolfgang Hofer, Sven Neumann",
  "Wolfgang Hofer",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_lower_layer_to_bottom_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_lower_layer_to_bottom_invoker } },
};


/*********************************/
/*  GIMAGE_MERGE_VISIBLE_LAYERS  */

static Argument *
gimage_merge_visible_layers_invoker (Argument *args)
{
  GImage *gimage;
  MergeType merge_type;
  Layer *layer;
  Argument *return_args;

  merge_type = ExpandAsNecessary;
  layer      = NULL;

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
      switch (int_value)
	{
	case 0: merge_type = ExpandAsNecessary; break;
	case 1: merge_type = ClipToImage; break;
	case 2: merge_type = ClipToBottomLayer; break;
	default: success = FALSE;
	}
    }

  if (success)
    success = ((layer = gimage_merge_visible_layers (gimage, merge_type)) != NULL);

  return_args = procedural_db_return_args (&gimage_merge_visible_layers_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(layer));

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_merge_visible_layers_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_INT32,
    "merge_type",
    "The type of merge: { EXPAND-AS-NECESSARY (0), CLIP-TO-IMAGE (1), CLIP-TO-BOTTOM-LAYER (2) }"
  }
};

ProcArg gimage_merge_visible_layers_out_args[] =
{
  { PDB_LAYER,
    "layer",
    "The resulting layer"
  }
};

ProcRecord gimage_merge_visible_layers_proc =
{
  "gimp_image_merge_visible_layers",
  "Merge the visible image layers into one",
  "This procedure combines the visible layers into a single layer using the specified merge type.  A merge type of EXPAND-AS-NECESSARY expands the final layer to encompass the areas of the visible layers.  A merge type of CLIP-TO-IMAGE clips the final layer to the extents of the image.  A merge type of CLIP-TO-BOTTOM-LAYER clips the final layer to the size of the bottommost layer.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_merge_visible_layers_args,

  /*  Output arguments  */
  1,
  gimage_merge_visible_layers_out_args,

  /*  Exec method  */
  { { gimage_merge_visible_layers_invoker } },
};



/*********************************************/
/*   GIMAGE_MERGE_DOWN                       */

static Argument *
gimage_merge_down_invoker (Argument *args)
{
  GImage *gimage;
  MergeType merge_type;
  Layer *layer;
  Argument *return_args;

  merge_type = ExpandAsNecessary;
  layer      = NULL;

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
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      int_value = args[2].value.pdb_int;
      switch (int_value)
	{
	case 0: merge_type = ExpandAsNecessary; break;
	case 1: merge_type = ClipToImage; break;
	case 2: merge_type = ClipToBottomLayer; break;
	default: success = FALSE;
	}
    }

  if (success)
    success = ((layer = gimp_image_merge_down (gimage, layer, merge_type)) != NULL);
  return_args = procedural_db_return_args (&gimage_merge_down_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(layer));

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_merge_down_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_LAYER,
    "layer",
    "The layer to merge down from"
  },
  { PDB_INT32,
    "merge_type",
    "The type of merge: { EXPAND-AS-NECESSARY (0), CLIP-TO-IMAGE (1), CLIP-TO-BOTTOM-LAYER (2) }"
  }
};

ProcArg gimage_merge_down_out_args[] =
{
  { PDB_LAYER,
    "layer",
    "The resulting layer"
  }
};

ProcRecord gimage_merge_down_proc =
{
  "gimp_image_merge_down",
  "Merge the layer passed and the first visible layer below",
  "This procedure combines the passed layer and the first visible layer below it using the specified merge type.  A merge type of EXPAND-AS-NECESSARY expands the final layer to encompass the areas of the visible layers.  A merge type of CLIP-TO-IMAGE clips the final layer to the extents of the image.  A merge type of CLIP-TO-BOTTOM-LAYER clips the final layer to the size of the bottommost layer.",
  "Larry Ewing",
  "Larry Ewing",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_merge_down_args,

  /*  Output arguments  */
  1,
  gimage_merge_down_out_args,

  /*  Exec method  */
  { { gimage_merge_down_invoker } },
};



/********************/
/*  GIMAGE_FLATTEN  */

static Argument *
gimage_flatten_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;
  Argument *return_args;

  layer = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = ((layer = gimage_flatten (gimage)) != NULL);

  return_args = procedural_db_return_args (&gimage_flatten_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(layer));

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_flatten_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  }
};

ProcArg gimage_flatten_out_args[] =
{
  { PDB_LAYER,
    "layer",
    "The resulting layer"
  }
};

ProcRecord gimage_flatten_proc =
{
  "gimp_image_flatten",
  "Flatten all visible layers into a single layer.  Discard all invisible layers",
  "This procedure combines the visible layers in a manner analogous to merging with the ClipToImage merge type.  Non-visible layers are discarded, and the resulting image is stripped of its alpha channel.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_flatten_args,

  /*  Output arguments  */
  1,
  gimage_flatten_out_args,

  /*  Exec method  */
  { { gimage_flatten_invoker } },
};


/**********************/
/*  GIMAGE_ADD_LAYER  */

static Argument *
gimage_add_layer_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;
  int position;

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
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;

      /*  make sure that this layer can be added to the specified image  */
      if ((!success) ||
          (drawable_color (GIMP_DRAWABLE(layer)) && gimage_base_type (gimage) != RGB) ||
	  (drawable_gray (GIMP_DRAWABLE(layer)) && gimage_base_type (gimage) != GRAY) ||
	  (drawable_indexed (GIMP_DRAWABLE(layer)) && gimage_base_type (gimage) != INDEXED))
	success = FALSE;
    }
  if (success)
    {
      int_value = args[2].value.pdb_int;
      position = MAXIMUM (int_value, -1);  /*  make sure it's -1 or greater  */
    }

  if (success)
    success = (gimage_add_layer (gimage, layer, position) != NULL);

  return procedural_db_return_args (&gimage_add_layer_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_add_layer_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_LAYER,
    "layer",
    "the layer"
  },
  { PDB_INT32,
    "position",
    "the layer position"
  }
};

ProcRecord gimage_add_layer_proc =
{
  "gimp_image_add_layer",
  "Add the specified layer to the image",
  "This procedure adds the specified layer to the gimage at the given position.  If the position is specified as -1, then the layer is inserted at the top of the layer stack.  If the layer to be added has no alpha channel, it must be added at position 0.  The layer type must be compatible with the image base type.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_add_layer_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_add_layer_invoker } },
};


/*************************/
/*  GIMAGE_REMOVE_LAYER  */

static Argument *
gimage_remove_layer_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;

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
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    gimage_remove_layer (gimage, layer);

  return procedural_db_return_args (&gimage_remove_layer_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_remove_layer_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_LAYER,
    "layer",
    "the layer"
  }
};

ProcRecord gimage_remove_layer_proc =
{
  "gimp_image_remove_layer",
  "Remove the specified layer from the image",
  "This procedure removes the specified layer from the image.  If the layer doesn't exist, an error is returned.  If there are no layers left in the image, this call will fail.  If this layer is the last layer remaining, the image will become empty and have no active layer.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_remove_layer_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_remove_layer_invoker } },
};


/***************************/
/*  GIMAGE_ADD_LAYER_MASK  */

static Argument *
gimage_add_layer_mask_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;
  LayerMask *mask;

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
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if ((mask = layer_mask_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = ((mask = gimage_add_layer_mask (gimage, layer, mask)) != NULL);

  return procedural_db_return_args (&gimage_add_layer_mask_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_add_layer_mask_args[] =
{
  { PDB_IMAGE,
    "image",
    "the layer's image"
  },
  { PDB_LAYER,
    "layer",
    "the layer to receive the mask"
  },
  { PDB_CHANNEL,
    "mask",
    "the mask to add to the layer"
  }
};

ProcRecord gimage_add_layer_mask_proc =
{
  "gimp_image_add_layer_mask",
  "Add a layer mask to the specified layer",
  "This procedure adds a layer mask to the specified layer.  Layer masks serve as an additional alpha channel for a layer.  This procedure will fail if a number of prerequisites aren't met.  The layer cannot already have a layer mask.  The specified mask must exist and have the same dimensions as the layer.  Both the mask and the layer must have been created for use with the specified image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_add_layer_mask_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_add_layer_mask_invoker } },
};


/******************************/
/*  GIMAGE_REMOVE_LAYER_MASK  */

static Argument *
gimage_remove_layer_mask_invoker (Argument *args)
{
  GImage *gimage;
  Layer *layer;
  int mode;

  mode = APPLY;

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
      if ((layer = layer_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      int_value = args[2].value.pdb_int;
      switch (int_value)
	{
	case 0: mode = APPLY; break;
	case 1: mode = DISCARD; break;
	default: success = FALSE;
	}
    }

  if (success)
    gimage_remove_layer_mask (gimage, layer, mode);

  return procedural_db_return_args (&gimage_remove_layer_mask_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_remove_layer_mask_args[] =
{
  { PDB_IMAGE,
    "image",
    "the layer's image"
  },
  { PDB_LAYER,
    "layer",
    "the layer from which to remove mask"
  },
  { PDB_INT32,
    "mode",
    "removal mode: { APPLY (0), DISCARD (1) }"
  }
};

ProcRecord gimage_remove_layer_mask_proc =
{
  "gimp_image_remove_layer_mask",
  "Remove the specified layer mask from the layer",
  "This procedure removes the specified layer mask from the layer.  If the mask doesn't exist, an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_remove_layer_mask_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_remove_layer_mask_invoker } },
};


/**************************/
/*  GIMAGE_RAISE_CHANNEL  */

static Argument *
gimage_raise_channel_invoker (Argument *args)
{
  GImage *gimage;
  Channel *channel;

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
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = ((gimage_raise_channel (gimage, channel)) != NULL);

  return procedural_db_return_args (&gimage_raise_channel_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_raise_channel_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_CHANNEL,
    "channel",
    "The channel to raise"
  }
};

ProcRecord gimage_raise_channel_proc =
{
  "gimp_image_raise_channel",
  "Raise the specified channel in the image's channel stack",
  "This procedure raises the specified channel one step in the existing channel stack.  It will not move the channel if there is no channel above it.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_raise_channel_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_raise_channel_invoker } },
};


/************************/
/*  GIMAGE_LOWER_CHANNEL  */

static Argument *
gimage_lower_channel_invoker (Argument *args)
{
  GImage *gimage;
  Channel *channel;

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
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = ((gimage_lower_channel (gimage, channel)) != NULL);

  return procedural_db_return_args (&gimage_lower_channel_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_lower_channel_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_CHANNEL,
    "channel",
    "The channel to lower"
  }
};

ProcRecord gimage_lower_channel_proc =
{
  "gimp_image_lower_channel",
  "Lower the specified channel in the image's channel stack",
  "This procedure lowers the specified channel one step in the existing channel stack.  It will not move the channel if there is no channel below it.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_lower_channel_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_lower_channel_invoker } },
};


/************************/
/*  GIMAGE_ADD_CHANNEL  */

static Argument *
gimage_add_channel_invoker (Argument *args)
{
  GImage *gimage;
  Channel *channel;
  int position;

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
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      int_value = args[2].value.pdb_int;
      position = MAXIMUM (int_value, -1);  /*  make sure it's -1 or greater  */
    }

  if (success)
    success = (gimage_add_channel (gimage, channel, position) != NULL);

  return procedural_db_return_args (&gimage_add_channel_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_add_channel_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_CHANNEL,
    "channel",
    "the channel"
  },
  { PDB_INT32,
    "position",
    "the channel position"
  }
};

ProcRecord gimage_add_channel_proc =
{
  "gimp_image_add_channel",
  "Add the specified channel to the image",
  "This procedure adds the specified channel to the gimage.  The position channel is not currently used, so the channel is always inserted at the top of the channel stack.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_add_channel_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_add_channel_invoker } },
};


/***************************/
/*  GIMAGE_REMOVE_CHANNEL  */

static Argument *
gimage_remove_channel_invoker (Argument *args)
{
  GImage *gimage;
  Channel *channel;

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
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = (gimage_remove_channel (gimage, channel) != NULL);

  return procedural_db_return_args (&gimage_remove_channel_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_remove_channel_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_CHANNEL,
    "channel",
    "the channel"
  }
};

ProcRecord gimage_remove_channel_proc =
{
  "gimp_image_remove_channel",
  "Remove the specified channel from the image",
  "This procedure removes the specified channel from the image.  If the channel doesn't exist, an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_remove_channel_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_remove_channel_invoker } },
};


/****************************/
/*  GIMAGE_ACTIVE_DRAWABLE  */

static Argument *
gimage_active_drawable_invoker (Argument *args)
{
  GImage *gimage;
  GimpDrawable *drawable;
  Argument *return_args;

  drawable = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	success = ((drawable = gimage_active_drawable (gimage)) != NULL);
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gimage_active_drawable_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (drawable);

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_active_drawable_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_active_drawable_out_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the active drawable"
  }
};

ProcRecord gimage_active_drawable_proc =
{
  "gimp_image_active_drawable",
  "Get the image's active drawable",
  "This procedure returns the ID of the image's active drawable.  This can be either a layer, a channel, or a layer mask.  The active drawable is specified by the active image channel.  If that is -1, then by the active image layer.  If the active image layer has a layer mask and the layer mask is in edit mode, then the layer mask is the active drawable.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_active_drawable_args,

  /*  Output arguments  */
  1,
  gimage_active_drawable_out_args,

  /*  Exec method  */
  { { gimage_active_drawable_invoker } },
};


/**********************/
/*  GIMAGE_BASE_TYPE  */

static Argument *
gimage_base_type_invoker (Argument *args)
{
  GImage *gimage;
  int base_type;
  Argument *return_args;

  base_type = RGB_GIMAGE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	base_type = gimage_base_type (gimage);
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gimage_base_type_proc, success);

  if (success)
    return_args[1].value.pdb_int = base_type;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_base_type_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_base_type_out_args[] =
{
  { PDB_INT32,
    "base_type",
    "the image's base type: { RGB (0), GRAY (1), INDEXED (2) }"
  }
};

ProcRecord gimage_base_type_proc =
{
  "gimp_image_base_type",
  "Get the base type of the image",
  "This procedure returns the image's base type, which is one of: { RGB-CHANNEL, GRAY-CHANNEL, INDEXED-CHANNEL }.  Layers in the image must be of this subtype, but can have an optional alpha channel.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_base_type_args,

  /*  Output arguments  */
  1,
  gimage_base_type_out_args,

  /*  Exec method  */
  { { gimage_base_type_invoker } },
};


/*************************/
/*  GIMAGE_GET_FILENAME  */

static Argument *
gimage_get_filename_invoker (Argument *args)
{
  GImage *gimage;
  char *filename;
  Argument *return_args;

  filename = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	filename = gimage_filename (gimage);
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gimage_get_filename_proc, success);

  if (success)
    return_args[1].value.pdb_pointer = (filename) ? g_strdup (filename) : NULL;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_get_filename_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_get_filename_out_args[] =
{
  { PDB_STRING,
    "filename",
    "the image's filename"
  }
};

ProcRecord gimage_get_filename_proc =
{
  "gimp_image_get_filename",
  "Return the filename of the image",
  "This procedure returns the image's filename--if it was loaded or has since been saved.  Otherwise, returns NULL.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_get_filename_args,

  /*  Output arguments  */
  1,
  gimage_get_filename_out_args,

  /*  Exec method  */
  { { gimage_get_filename_invoker } },
};


/*************************/
/*  GIMAGE_SET_FILENAME  */

static Argument *
gimage_set_filename_invoker (Argument *args)
{
  GImage *gimage;
  char *filename;
  char *new_filename;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	filename = gimage_filename (gimage);
      else
	success = FALSE;
    }

  if (success)
    {
      new_filename = (char *) args[1].value.pdb_pointer;
      gimage_set_filename (gimage, new_filename);
    }

  return procedural_db_return_args (&gimage_set_filename_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_set_filename_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_STRING,
    "filename",
    "the image's filename"
  }
};

ProcRecord gimage_set_filename_proc =
{
  "gimp_image_set_filename",
  "Set the image's filename",
  "This procedure sets the image's filename.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimage_set_filename_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_set_filename_invoker } },
};



/***************************/
/*  GIMAGE_GET_RESOLUTION  */

static Argument *
gimage_get_resolution_invoker (Argument *args)
{
  GImage *gimage;
  float xresolution;
  float yresolution;
  Argument *return_args;

  xresolution = 0.0;
  yresolution = 0.0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value))) {
	xresolution = gimage->xresolution;
	yresolution = gimage->yresolution;
      } else {
	success = FALSE;
      }
    }

  return_args= procedural_db_return_args(&gimage_get_resolution_proc, success);

  if (success)
  {
    return_args[1].value.pdb_float = xresolution;
    return_args[2].value.pdb_float = yresolution;
  }

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_get_resolution_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_get_resolution_out_args[] =
{
  { PDB_FLOAT,
    "xresolution",
    "the image's resolution in the x-axis, in dots per inch"
  },
  { PDB_FLOAT,
    "yresolution",
    "the image's resolution in the y-axis, in dots per inch"
  }
};

ProcRecord gimage_get_resolution_proc =
{
  "gimp_image_get_resolution",
  "Return the resolution of the image",
  "This procedure returns the image's resolution, in dots per inch.  This value is independent of any of the layers in this image.  A return value of 0.0 means the image was invalid.",
  "Austin Donnelly",
  "Austin Donnelly",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_get_resolution_args,

  /*  Output arguments  */
  2,
  gimage_get_resolution_out_args,

  /*  Exec method  */
  { { gimage_get_resolution_invoker } },
};


/***************************/
/*  GIMAGE_SET_RESOLUTION  */

static Argument *
gimage_set_resolution_invoker (Argument *args)
{
  GImage *gimage;
  Argument *return_args;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (!(gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }

  if (success)
    {
      float_value = args[1].value.pdb_float;
      gimage->xresolution = float_value;
      float_value = args[2].value.pdb_float;
      gimage->yresolution = float_value;
    }

  return_args= procedural_db_return_args(&gimage_set_resolution_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_set_resolution_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_FLOAT,
    "xresolution",
    "resolution in x-axis, in dots per inch"
  },
  { PDB_FLOAT,
    "yresolution",
    "resolution in y-axis, in dots per inch"
  }
};

ProcRecord gimage_set_resolution_proc =
{
  "gimp_image_set_resolution",
  "Sets the resolution of the image",
  "This procedure sets the image's resolution, in dots per inch.  This value is independent of any of the layers in this image.  No scaling or resizing is performed.",
  "Austin Donnelly",
  "Austin Donnelly",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_set_resolution_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_set_resolution_invoker } },
};



/******************/
/*  GIMAGE_WIDTH  */

static Argument *
gimage_width_invoker (Argument *args)
{
  GImage *gimage;
  int width;
  Argument *return_args;

  width = 0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	width = gimage->width;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gimage_width_proc, success);

  if (success)
    return_args[1].value.pdb_int = width;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_width_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_width_out_args[] =
{
  { PDB_INT32,
    "width",
    "the image's width"
  }
};

ProcRecord gimage_width_proc =
{
  "gimp_image_width",
  "Return the width of the image",
  "This procedure returns the image's width.  This value is independent of any of the layers in this image.  This is the \"canvas\" width.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_width_args,

  /*  Output arguments  */
  1,
  gimage_width_out_args,

  /*  Exec method  */
  { { gimage_width_invoker } },
};


/*******************/
/*  GIMAGE_HEIGHT  */

static Argument *
gimage_height_invoker (Argument *args)
{
  GImage *gimage;
  int height;
  Argument *return_args;

  height = 0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	height = gimage->height;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gimage_height_proc, success);

  if (success)
    return_args[1].value.pdb_int = height;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_height_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_height_out_args[] =
{
  { PDB_INT32,
    "height",
    "the image's height"
  }
};

ProcRecord gimage_height_proc =
{
  "gimp_image_height",
  "Return the height of the image",
  "This procedure returns the image's height.  This value is independent of any of the layers in this image.  This is the \"canvas\" height.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_height_args,

  /*  Output arguments  */
  1,
  gimage_height_out_args,

  /*  Exec method  */
  { { gimage_height_invoker } },
};


/*********************/
/*  GIMAGE_GET_CMAP  */

static Argument *
gimage_get_cmap_invoker (Argument *args)
{
  GImage *gimage;
  unsigned char *cmap;
  Argument *return_args;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gimage_get_cmap_proc, success);

  if (success)
    {
      cmap = g_malloc (gimage->num_cols * 3);
      memcpy (cmap, gimage_cmap (gimage), gimage->num_cols * 3);
      return_args[1].value.pdb_int = gimage->num_cols * 3;
      return_args[2].value.pdb_pointer = cmap;
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_get_cmap_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_get_cmap_out_args[] =
{
  { PDB_INT32,
    "num_bytes",
    "number of bytes in the colormap array: 0 <= num_bytes <= 768"
  },
  { PDB_INT8ARRAY,
    "cmap",
    "the image's colormap"
  }
};

ProcRecord gimage_get_cmap_proc =
{
  "gimp_image_get_cmap",
  "Returns the image's colormap",
  "This procedure returns an actual pointer to the image's colormap, as well as the number of bytes contained in the colormap.  The actual number of colors in the transmitted colormap will be \"num_bytes\" / 3.  If the image is not of base type INDEXED, this pointer will be NULL.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_get_cmap_args,

  /*  Output arguments  */
  2,
  gimage_get_cmap_out_args,

  /*  Exec method  */
  { { gimage_get_cmap_invoker } },
};


/*********************/
/*  GIMAGE_SET_CMAP  */

static Argument *
gimage_set_cmap_invoker (Argument *args)
{
  GImage *gimage;
  int num_cols;
  unsigned char *cmap;

  num_cols = 0;

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
      if (int_value < 0 || int_value > COLORMAP_SIZE)
	success = FALSE;
      else
	num_cols = int_value / 3;
    }
  if (success)
    cmap = (unsigned char *) args[2].value.pdb_pointer;

  if (success)
    {
      if (gimage->num_cols && gimage->cmap)
	{
	  g_free (gimage->cmap);
	  gimage->cmap = NULL;
	}
      if (num_cols)
	{
	  gimage->cmap = (unsigned char *) g_malloc (COLORMAP_SIZE);
	  memcpy (gimage->cmap, cmap, num_cols * 3);
	}
      gimage->num_cols = num_cols;
    }

  return procedural_db_return_args (&gimage_set_cmap_proc, success);
}

/*  The procedure definition  */
ProcArg gimage_set_cmap_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_INT32,
    "num_bytes",
    "number of bytes in the new colormap: 0 <= num_colors <= 768"
  },
  { PDB_INT8ARRAY,
    "cmap",
    "the new colormap values"
  }
};

ProcRecord gimage_set_cmap_proc =
{
  "gimp_image_set_cmap",
  "Sets the entries in the image's colormap",
  "This procedure sets the entries in the specified image's colormap.  The number of entries is specified by the \"num_bytes\" parameter and corresponds the the number of INT8 triples that must be contained in the \"cmap\" array.  The actual number of colors in the transmitted colormap is \"num_bytes\" / 3.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  gimage_set_cmap_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_set_cmap_invoker } },
};


/************************/
/*  GIMAGE_ENABLE_UNDO  */

static Argument *
gimage_enable_undo_invoker (Argument *args)
{
  GImage *gimage;
  Argument *return_args;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = gimage_enable_undo (gimage);
  
  return_args = procedural_db_return_args (&gimage_enable_undo_proc, success);

  if (success)
    return_args[1].value.pdb_int = (success) ? TRUE : FALSE;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_enable_undo_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_enable_undo_out_args[] =
{
  { PDB_INT32,
    "enabled",
    "true if the image undo has been enabled",
  }
};

ProcRecord gimage_enable_undo_proc =
{
  "gimp_image_enable_undo",
  "Enable the image's undo stack",
  "This procedure enables the image's undo stack, allowing subsequent operations to store their undo steps.  This is generally called in conjunction with 'gimp_image_disable_undo' to temporarily disable an image undo stack.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_enable_undo_args,

  /*  Output arguments  */
  1,
  gimage_enable_undo_out_args,

  /*  Exec method  */
  { { gimage_enable_undo_invoker } }
};


/*************************/
/*  GIMAGE_DISABLE_UNDO  */

static Argument *
gimage_disable_undo_invoker (Argument *args)
{
  GImage *gimage;
  Argument *return_args;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = gimage_disable_undo (gimage);
  
  return_args = procedural_db_return_args (&gimage_disable_undo_proc, success);

  if (success)
    return_args[1].value.pdb_int = (success) ? TRUE : FALSE;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_disable_undo_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_disable_undo_out_args[] =
{
  { PDB_INT32,
    "disabled",
    "true if the image undo has been disabled",
  }
};

ProcRecord gimage_disable_undo_proc =
{
  "gimp_image_disable_undo",
  "Disable the image's undo stack",
  "This procedure disables the image's undo stack, allowing subsequent operations to ignore their undo steps.  This is generally called in conjunction with 'gimp_image_enable_undo' to temporarily disable an image undo stack.  This is advantageous because saving undo steps can be time and memory intensive.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_disable_undo_args,

  /*  Output arguments  */
  1,
  gimage_disable_undo_out_args,

  /*  Exec method  */
  { { gimage_disable_undo_invoker } }
};


/**********************/
/*  GIMAGE_CLEAN_ALL  */

static Argument *
gimage_clean_all_invoker (Argument *args)
{
  GImage *gimage;
  Argument *return_args;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    gimage_clean_all (gimage);
  
  return_args = procedural_db_return_args (&gimage_clean_all_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_clean_all_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcRecord gimage_clean_all_proc =
{
  "gimp_image_clean_all",
  "Set the image dirty count to 0",
  "This procedure sets the specified image's dirty count to 0, allowing operations to occur without having a 'dirtied' image.  This is especially useful for creating and loading images which should not initially be considered dirty, even though layers must be created, filled, and installed in the image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_clean_all_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimage_clean_all_invoker } }
};


/*************************/
/*  GIMAGE_FLOATING_SEL  */

static Argument *
gimage_floating_sel_invoker (Argument *args)
{
  GImage *gimage;
  Layer *floating_sel;
  Argument *return_args;

  floating_sel = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)))
	floating_sel = gimage_floating_sel (gimage);
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&gimage_floating_sel_proc, success);

  if (success)
    return_args[1].value.pdb_int = (floating_sel) ? drawable_ID (GIMP_DRAWABLE(floating_sel)) : -1;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimage_floating_sel_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gimage_floating_sel_out_args[] =
{
  { PDB_LAYER,
    "floating_sel",
    "the image's floating selection"
  }
};

ProcRecord gimage_floating_sel_proc =
{
  "gimp_image_floating_selection",
  "Return the floating selection of the image",
  "This procedure returns the image's floating_sel, if it exists.  If it doesn't exist, -1 is returned as the layer ID.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimage_floating_sel_args,

  /*  Output arguments  */
  1,
  gimage_floating_sel_out_args,

  /*  Exec method  */
  { { gimage_floating_sel_invoker } },
};

static GImage *
duplicate (GImage *gimage)
{
  PixelRegion srcPR, destPR;
  GImage *new_gimage;
  Layer *layer, *new_layer;
  Layer *floating_layer;
  Channel *channel, *new_channel;
  GSList *list;
  Layer *active_layer = NULL;
  Channel *active_channel = NULL;
  GimpDrawable *new_floating_sel_drawable = NULL;
  GimpDrawable *floating_sel_drawable = NULL;
  int count;

  /*  Create a new image  */
  new_gimage = gimage_new (gimage->width, gimage->height, gimage->base_type);
  gimage_disable_undo (new_gimage);

  floating_layer = gimage_floating_sel (gimage);
  if (floating_layer)
    {
      floating_sel_relax (floating_layer, FALSE);

      floating_sel_drawable = floating_layer->fs.drawable;
      floating_layer = NULL;
    }

  /*  Copy the layers  */
  list = gimage->layers;
  count = 0;
  layer = NULL;
  while (list)
    {
      layer = (Layer *) list->data;
      list = g_slist_next (list);

      new_layer = layer_copy (layer, FALSE);

      gimp_drawable_set_gimage(GIMP_DRAWABLE(new_layer), new_gimage);

      /*  Make sure the copied layer doesn't say: "<old layer> copy"  */
      layer_set_name(GIMP_LAYER(new_layer),
		     layer_get_name(GIMP_LAYER(layer)));

      /*  Make sure if the layer has a layer mask, it's name isn't screwed up  */
      if (new_layer->mask)
	{
	  gimp_drawable_set_name(GIMP_DRAWABLE(new_layer->mask),
			  gimp_drawable_get_name(GIMP_DRAWABLE(layer->mask)));
	}

      if (gimage->active_layer == layer)
	active_layer = new_layer;

      if (gimage->floating_sel == layer)
	floating_layer = new_layer;

      if (floating_sel_drawable == GIMP_DRAWABLE(layer))
	new_floating_sel_drawable = GIMP_DRAWABLE(new_layer);

      /*  Add the layer  */
      if (floating_layer != new_layer)
	gimage_add_layer (new_gimage, new_layer, count++);
    }

  /*  Copy the channels  */
  list = gimage->channels;
  count = 0;
  while (list)
    {
      channel = (Channel *) list->data;
      list = g_slist_next (list);

      new_channel = channel_copy (channel);

      gimp_drawable_set_gimage(GIMP_DRAWABLE(new_channel), new_gimage);

      /*  Make sure the copied channel doesn't say: "<old channel> copy"  */
      gimp_drawable_set_name(GIMP_DRAWABLE(new_channel),
			     gimp_drawable_get_name(GIMP_DRAWABLE(channel)));

      if (gimage->active_channel == channel)
	active_channel = (new_channel);

      if (floating_sel_drawable == GIMP_DRAWABLE(channel))
	new_floating_sel_drawable = GIMP_DRAWABLE(new_channel);

      /*  Add the channel  */
      gimage_add_channel (new_gimage, new_channel, count++);
    }

  /*  Copy the selection mask  */
  pixel_region_init (&srcPR, drawable_data (GIMP_DRAWABLE(gimage->selection_mask)), 0, 0, gimage->width, gimage->height, FALSE);
  pixel_region_init (&destPR, drawable_data (GIMP_DRAWABLE(new_gimage->selection_mask)), 0, 0, gimage->width, gimage->height, TRUE);
  copy_region (&srcPR, &destPR);
  new_gimage->selection_mask->bounds_known = FALSE;
  new_gimage->selection_mask->boundary_known = FALSE;

  /*  Set active layer, active channel  */
  new_gimage->active_layer = active_layer;
  new_gimage->active_channel = active_channel;
  if (floating_layer)
    floating_sel_attach (floating_layer, new_floating_sel_drawable);

  /*  Copy the colormap if necessary  */
  if (new_gimage->base_type == INDEXED)
    memcpy (new_gimage->cmap, gimage->cmap, gimage->num_cols * 3);
  new_gimage->num_cols = gimage->num_cols;

  /*  copy state of all color channels  */
  for (count = 0; count < MAX_CHANNELS; count++)
    {
      new_gimage->visible[count] = gimage->visible[count];
      new_gimage->active[count] = gimage->active[count];
    }

  gimage_enable_undo (new_gimage);

  return new_gimage;
}

/*  The duplicate procedure definition  */
ProcArg channel_ops_duplicate_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg channel_ops_duplicate_out_args[] =
{
  { PDB_IMAGE,
    "new_image",
    "the new, duplicated image"
  }
};

ProcRecord channel_ops_duplicate_proc =
{
  "gimp_channel_ops_duplicate",
  "Duplicate the specified image",
  "This procedure duplicates the specified image, copying all layers, channels, and image information.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  channel_ops_duplicate_args,

  /*  Output arguments  */
  1,
  channel_ops_duplicate_out_args,

  /*  Exec method  */
  { { channel_ops_duplicate_invoker } },
};


static Argument *
channel_ops_duplicate_invoker (Argument *args)
{
  Argument *return_args;
  int success = TRUE;
  int int_value;
  GImage *gimage, *new_gimage;

  new_gimage = NULL;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }

  if (success)
    success = ((new_gimage = duplicate ((void *) gimage)) != NULL);

  return_args = procedural_db_return_args (&channel_ops_duplicate_proc, success);

  if (success)
    return_args[1].value.pdb_int = pdb_image_to_id(new_gimage);

  return return_args;
}


void
channel_ops_duplicate (void *gimage_ptr)
{
  GImage *gimage;
  GImage *new_gimage;

  gimage = (GImage *) gimage_ptr;
  new_gimage = duplicate (gimage);

  gdisplay_new (new_gimage, 0x0101);
}


/* GUIDES STUFF */


/*  The gimp_image_add_hguide procedure definition  */
ProcArg gimp_image_add_hguide_args[] =
{
  { PDB_IMAGE,
    "image",
    "the input image"
  },
  { PDB_INT32,
    "yposition",
    "The guide's y-offset from top of image"
  }
};

ProcArg gimp_image_add_hguide_out_args[] =
{
  { PDB_INT32,
    "guide_id",
    "The new guide's guide_id"
  }
};


ProcRecord gimp_image_add_hguide_proc =
{
  "gimp_image_add_hguide",
  "Add a horizontal Guide to an image",
  "This procedure adds a horizontal Guide to an image.  It takes the input image and the y-position of the new Guide as parameters.  It returns the guide_id of the new Guide.",
  "Adam D. Moss",
  "Adam D. Moss",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_add_hguide_args,

  /*  Output arguments  */
  1,
  gimp_image_add_hguide_out_args,

  /*  Exec method  */
  { { gimp_image_add_hguide_invoker } },
};


static Argument *
gimp_image_add_hguide_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  int offset;
  GImage *gimage;
  Guide *guide = NULL;
  Argument *return_args;


  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  if (success)
    {
      offset = args[1].value.pdb_int;
      if ((offset >= gimage->height) || (offset <= 0))
	success = FALSE;
    }

  if (success)
    {
      guide = gimp_image_add_hguide ((void *) gimage);
      guide->position = offset;
    }

  return_args = procedural_db_return_args (&gimp_image_add_hguide_proc,
					   success);

  if (success)
    return_args[1].value.pdb_int = guide->guide_ID;
  
  return return_args;
}




/*  The gimp_image_add_vguide procedure definition  */
ProcArg gimp_image_add_vguide_args[] =
{
  { PDB_IMAGE,
    "image",
    "the input image"
  },
  { PDB_INT32,
    "xposition",
    "The guide's x-offset from left of image"
  }
};

ProcArg gimp_image_add_vguide_out_args[] =
{
  { PDB_INT32,
    "guide_id",
    "The new guide's guide_id"
  }
};


ProcRecord gimp_image_add_vguide_proc =
{
  "gimp_image_add_vguide",
  "Add a vertical Guide to an image",
  "This procedure adds a horizontal Guide to an image.  It takes the input image and the x-position of the new Guide as parameters.  It returns the guide_id of the new Guide.",
  "Adam D. Moss",
  "Adam D. Moss",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_add_vguide_args,

  /*  Output arguments  */
  1,
  gimp_image_add_vguide_out_args,

  /*  Exec method  */
  { { gimp_image_add_vguide_invoker } },
};


static Argument *
gimp_image_add_vguide_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  int offset;
  GImage *gimage;
  Guide *guide = NULL;
  Argument *return_args;


  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  if (success)
    {
      offset = args[1].value.pdb_int;
      if ((offset >= gimage->width) || (offset <= 0))
	success = FALSE;
    }

  if (success)
    {
      guide = gimp_image_add_vguide ((void *) gimage);
      guide->position = offset;
    }

  return_args = procedural_db_return_args (&gimp_image_add_vguide_proc,
					   success);

  if (success)
    return_args[1].value.pdb_int = guide->guide_ID;
  
  return return_args;
}




/*  The gimp_image_delete_guide procedure definition  */
ProcArg gimp_image_delete_guide_args[] =
{
  { PDB_IMAGE,
    "image",
    "The input image"
  },
  { PDB_INT32,
    "guide_ID",
    "The guide_ID of the Guide to be removed"
  }
};


ProcRecord gimp_image_delete_guide_proc =
{
  "gimp_image_delete_guide",
  "Deletes a Guide from an image",
  "This procedure takes an image and a guide_id as input and removes the specified Guide from the specified image.",
  "Adam D. Moss",
  "Adam D. Moss",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_delete_guide_args,

  /*  Output arguments  */
  0,
  NULL,
  
  /*  Exec method  */
  { { gimp_image_delete_guide_invoker } },
};


static Argument *
gimp_image_delete_guide_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  int guide_id;
  GImage *gimage;
  GList *guides;
  Argument *return_args;


  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  if (success)
    {
      guide_id = args[1].value.pdb_int;
      success = FALSE;
      guides = gimage->guides;
      while (guides)
	{
	  if ((((Guide*)guides->data)->guide_ID == guide_id) &&
	      (((Guide*)guides->data)->position >= 0) )
	    {
	      GList *tmp_next;

	      /* printf("Gotcha at %p\n", ((Guide*)guides->data)); */
	      success = TRUE;

	      tmp_next = g_list_next(guides);

	      ((Guide*)guides->data)->position = -1;
	      undo_push_guide (gimage, ((Guide*)guides->data));
	      /*gimp_image_remove_guide (gimage, ((Guide*)guides->data));*/

	      guides = tmp_next;
	    }
	  else
	    {
	      guides = g_list_next(guides);
	    }
	}
    }

  return_args = procedural_db_return_args (&gimp_image_delete_guide_proc,
					   success);

  return return_args;
}






/*  The gimp_image_findnext_guide procedure definition  */
ProcArg gimp_image_findnext_guide_args[] =
{
  { PDB_IMAGE,
    "image",
    "The input image"
  },
  { PDB_INT32,
    "guide_ID",
    "The guide_ID of current Guide (0 if first invocation)"
  }
};

ProcArg gimp_image_findnext_guide_out_args[] =
{
  { PDB_INT32,
    "guide_id",
    "The next guide's guide_id"
  }
};


ProcRecord gimp_image_findnext_guide_proc =
{
  "gimp_image_findnext_guide",
  "Find next Guide on an image",
  "This procedure takes an image and a guide_id as input and finds the guide_id of the successor of the given guide_id in the image's Guide list.  If the supplied guide_id is 0, the procedure will return the first Guide.  The procedure will return 0 if given the final guide_id as an argument or the image has no guides.",
  "Adam D. Moss",
  "Adam D. Moss",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_findnext_guide_args,

  /*  Output arguments  */
  1,
  gimp_image_findnext_guide_out_args,
  
  /*  Exec method  */
  { { gimp_image_findnext_guide_invoker } },
};


static Argument *
gimp_image_findnext_guide_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  int guide_id;
  int rtn_guide_id = 0;
  GImage *gimage;
  GList *guides;
  Argument *return_args;


  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  if (success)
    {
      guide_id = args[1].value.pdb_int;
      guides = gimage->guides;

      rtn_guide_id = 0;

      if (guides == NULL) /* No guides */
	{
	  rtn_guide_id = 0;
	}
      else
	{
	  if (guide_id == 0) /* init - Return first guide_ID in list */
	    {
	      while (guides != NULL &&
		     ((Guide*)guides->data)->position < 0)
		guides = g_list_next(guides);

	      if (guides) /* didn't just come to end of list */
		rtn_guide_id = ((Guide*)guides->data)->guide_ID;
	    }
	  else
	    {
	      success = FALSE;
	      
	      while (guides)
		{
		  if ((((Guide*)guides->data)->guide_ID == guide_id) &&
		      (((Guide*)guides->data)->position>=0) )
		    {
		      GList* tmplist;

		      /*printf("Gotcha at %p: %d, %d, %d\n",
			     ((Guide*)guides->data),
			     ((Guide*)guides->data)->position,
			     ((Guide*)guides->data)->orientation,
			     ((Guide*)guides->data)->guide_ID
			     );*/

		      success = TRUE;

		      tmplist = g_list_next(guides);
		      
		      rtn_guide_id = 0;

		      while ((tmplist != NULL) &&
			     (((Guide*)tmplist->data)->position < 0) )
			{
			  tmplist = g_list_next(tmplist);
			}

		      if (tmplist != NULL)
			rtn_guide_id = ((Guide*)tmplist->data)->guide_ID;

		      goto got_it;
		    }
		  
		  guides = g_list_next(guides);
		}
	    }
	}
    }

 got_it:
  return_args = procedural_db_return_args (&gimp_image_findnext_guide_proc,
					   success);

  if (success)
    return_args[1].value.pdb_int = rtn_guide_id;

  return return_args;
}









/*  The gimp_image_get_guide_orientation procedure definition  */
ProcArg gimp_image_get_guide_orientation_args[] =
{
  { PDB_IMAGE,
    "image",
    "The input image"
  },
  { PDB_INT32,
    "guide_ID",
    "The guide_ID"
  }
};

ProcArg gimp_image_get_guide_orientation_out_args[] =
{
  { PDB_INT32,
    "orientation",
    "The guide's orientation - vert=0, horiz=1"
  }
};


ProcRecord gimp_image_get_guide_orientation_proc =
{
  "gimp_image_get_guide_orientation",
  "Get orientation of a Guide on an image",
  "This procedure takes an image and a guide_id as input and returns the orientations of the guide, where 0=vertical and 1=horizontal.",
  "Adam D. Moss",
  "Adam D. Moss",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_get_guide_orientation_args,

  /*  Output arguments  */
  1,
  gimp_image_get_guide_orientation_out_args,
  
  /*  Exec method  */
  { { gimp_image_get_guide_orientation_invoker } },
};


static Argument *
gimp_image_get_guide_orientation_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  int guide_id;
  int rtn_orientation = 0;
  GImage *gimage;
  GList *guides;
  Argument *return_args;


  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  if (success)
    {
      guide_id = args[1].value.pdb_int;
      guides = gimage->guides;

      success = FALSE;

      while (guides)
	{
	  if ((((Guide*)guides->data)->guide_ID == guide_id) &&
	      (((Guide*)guides->data)->position>=0) )
	    {
	      success = TRUE;
	      switch (((Guide*)guides->data)->orientation)
		{
		case VERTICAL_GUIDE: rtn_orientation = 0; break;
		case HORIZONTAL_GUIDE: rtn_orientation = 1; break;
		default:
		  {
		    rtn_orientation = 0;
		    g_warning("Guide has bad orientation.");
		    break;
		  }
		}
	      goto got_it;
	    }
	  
	  guides = g_list_next(guides);
	}
    }
  
 got_it:
  return_args = procedural_db_return_args
    (&gimp_image_get_guide_orientation_proc, success);

  if (success)
    return_args[1].value.pdb_int = rtn_orientation;

  return return_args;
}












/*  The gimp_image_get_guide_position procedure definition  */
ProcArg gimp_image_get_guide_position_args[] =
{
  { PDB_IMAGE,
    "image",
    "The input image"
  },
  { PDB_INT32,
    "guide_ID",
    "The guide_ID"
  }
};

ProcArg gimp_image_get_guide_position_out_args[] =
{
  { PDB_INT32,
    "position",
    "The guide's position relative to top or left of image"
  }
};


ProcRecord gimp_image_get_guide_position_proc =
{
  "gimp_image_get_guide_position",
  "Get position of a Guide on an image",
  "This procedure takes an image and a guide_id as input and returns the position of the guide relative to the top or left of the image.",
  "Adam D. Moss",
  "Adam D. Moss",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_get_guide_position_args,

  /*  Output arguments  */
  1,
  gimp_image_get_guide_position_out_args,
  
  /*  Exec method  */
  { { gimp_image_get_guide_position_invoker } },
};


static Argument *
gimp_image_get_guide_position_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  int guide_id;
  int rtn_position = -1;
  GImage *gimage;
  GList *guides;
  Argument *return_args;


  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  if (success)
    {
      guide_id = args[1].value.pdb_int;
      guides = gimage->guides;

      success = FALSE;

      while (guides)
	{
	  if ((((Guide*)guides->data)->guide_ID == guide_id) &&
	      (((Guide*)guides->data)->position>=0) )
	    {
	      success = TRUE;
	      rtn_position = ((Guide*)guides->data)->position;
	      goto got_it;
	    }
	  
	  guides = g_list_next(guides);
	}
    }
  
 got_it:
  return_args = procedural_db_return_args
    (&gimp_image_get_guide_position_proc, success);

  if (success)
    return_args[1].value.pdb_int = rtn_position;

  return return_args;
}

/***************** Parasitic Stuff *****************/

/*** gimp_image_find_parasite ***/
ProcArg gimp_image_find_parasite_args[] =
{
  { PDB_IMAGE,
    "image",
    "the input image"
  },
  { PDB_STRING,
    "name",
    "The name of the parasite to find"
  },
};

ProcArg gimp_image_find_parasite_out_args[] =
{
  { PDB_PARASITE,
    "parasite",
    "the found parasite"
  },
};

ProcRecord gimp_image_find_parasite_proc =
{
  "gimp_image_find_parasite",
  "Finds a parasite of a specified type and creator in an image.",
  "Finds a parasite of a specified type and creator in an image.",
  "Jay Cox",
  "Jay Cox",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_find_parasite_args,

  /*  Output arguments  */
  1,
  gimp_image_find_parasite_out_args,

  /*  Exec method  */
  { { gimp_image_find_parasite_invoker } },
};


static Argument *
gimp_image_find_parasite_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  GImage *gimage;
  Argument *return_args;
  char *name = NULL;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  /*  name  */
  if (success)
    {
      name = (char *) args[1].value.pdb_pointer;
    }

  return_args = procedural_db_return_args (&gimp_image_find_parasite_proc,
					   success);
  /*  The real work  */
  if (success)
    {
      return_args[1].value.pdb_pointer = 
	parasite_copy(gimp_image_find_parasite (gimage, name));
    }

  return return_args;
}

/*** gimp_image_attach_parasite ***/

ProcArg gimp_image_attach_parasite_args[] =
{
  { PDB_IMAGE,
    "image",
    "the input image"
  },
  { PDB_PARASITE,
    "parasite",
    "The parasite to attach to the image"
  }
};

ProcRecord gimp_image_attach_parasite_proc =
{
  "gimp_image_attach_parasite",
  "Add a parasite to an image",
  "This procedure attaches a parasite to an image.  It has no return values.",
  "Jay Cox",
  "Jay Cox",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_attach_parasite_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimp_image_attach_parasite_invoker } },
};


static Argument *
gimp_image_attach_parasite_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  GImage *gimage;
  Parasite *parasite = NULL;
  Argument *return_args;


  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  if (success)
    {
      parasite = (Parasite *)args[1].value.pdb_pointer;
      if (parasite == NULL)
	success = FALSE;
    }

  if (success)
    {
      gimp_image_attach_parasite (gimage, parasite);
    }

  return_args = procedural_db_return_args (&gimp_image_attach_parasite_proc,
					   success);

  return return_args;
}


/*** gimp_image_detach_parasite ***/

ProcArg gimp_image_detach_parasite_args[] =
{
  { PDB_IMAGE,
    "image",
    "the input image"
  },
  { PDB_STRING,
    "name",
    "The name of the parasite to detach from the image"
  }
};

ProcRecord gimp_image_detach_parasite_proc =
{
  "gimp_image_detach_parasite",
  "Removes a parasite from an image",
  "This procedure detaches a parasite to an image.  It has no return values.",
  "Jay Cox",
  "Jay Cox",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_detach_parasite_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimp_image_detach_parasite_invoker } },
};


static Argument *
gimp_image_detach_parasite_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  GImage *gimage;
  char *parasite = NULL;
  Argument *return_args;


  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  if (success)
    {
      parasite = (char *)args[1].value.pdb_pointer;
      if (parasite == NULL)
	success = FALSE;
    }

  if (success)
    {
      gimp_image_detach_parasite (gimage, parasite);
    }

  return_args = procedural_db_return_args (&gimp_image_detach_parasite_proc,
					   success);

  return return_args;
}


/******************************/
/*****  Tattoo functions  *****/

/*** gimp_image_get_layer_by_tattoo ***/

ProcArg gimp_image_get_layer_by_tattoo_args[] =
{
  { PDB_IMAGE,
    "image",
    "the input image"
  },
  { PDB_INT32,
    "tattoo",
    "The tattoo of the layer to find"
  }
};

ProcArg gimp_image_get_layer_by_tattoo_out_args[] =
{
  { PDB_LAYER,
    "layer",
    "the layer with the specified tattoo"
  },
};

ProcRecord gimp_image_get_layer_by_tattoo_proc =
{
  "gimp_image_get_layer_by_tattoo",
  "Find a layer with a given tattoo in an image",
  "This procedure returns the layer with the given tattoo in the specified image.",
  "Jay Cox",
  "Jay Cox",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_get_layer_by_tattoo_args,

  /*  Output arguments  */
  1,
  gimp_image_get_layer_by_tattoo_out_args,

  /*  Exec method  */
  { { gimp_image_get_layer_by_tattoo_invoker } },
};


static Argument *
gimp_image_get_layer_by_tattoo_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  GImage *gimage;
  Tattoo tattoo;
  Argument *return_args;
  Layer *layer;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  if (success)
    {
      tattoo = args[1].value.pdb_int;
      if (tattoo == 0)
	success = FALSE;
    }

  if (success)
    {
      layer = gimp_image_get_layer_by_tattoo (gimage, tattoo);
      if (layer == NULL)
	success = FALSE;
    }

  return_args = 
    procedural_db_return_args (&gimp_image_get_layer_by_tattoo_proc,
			       success);
  if (success)
    {
      return_args[1].value.pdb_int = drawable_ID(GIMP_DRAWABLE(layer));
    }

  return return_args;
}


/*** gimp_image_get_channel_by_tattoo ***/

ProcArg gimp_image_get_channel_by_tattoo_args[] =
{
  { PDB_IMAGE,
    "image",
    "the input image"
  },
  { PDB_INT32,
    "tattoo",
    "The tattoo of the channel to find"
  }
};

ProcArg gimp_image_get_channel_by_tattoo_out_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel with the specified tattoo"
  },
};

ProcRecord gimp_image_get_channel_by_tattoo_proc =
{
  "gimp_image_get_channel_by_tattoo",
  "Find a channel with a given tattoo in an image",
  "This procedure returns the channel with the given tattoo in the specified image.",
  "Jay Cox",
  "Jay Cox",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  gimp_image_get_channel_by_tattoo_args,

  /*  Output arguments  */
  1,
  gimp_image_get_channel_by_tattoo_out_args,

  /*  Exec method  */
  { { gimp_image_get_channel_by_tattoo_invoker } },
};


static Argument *
gimp_image_get_channel_by_tattoo_invoker (Argument *args)
{
  int success = TRUE;
  int int_value;
  GImage *gimage;
  Tattoo tattoo;
  Argument *return_args;
  Channel *channel;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
        success = FALSE;
    }

  if (success)
    {
      tattoo = args[1].value.pdb_int;
      if (tattoo == 0)
	success = FALSE;
    }

  if (success)
    {
      channel = gimp_image_get_channel_by_tattoo (gimage, tattoo);
      if (channel == NULL)
	success = FALSE;
    }

  return_args = 
    procedural_db_return_args (&gimp_image_get_channel_by_tattoo_proc,
			       success);
  if (success)
    {
      return_args[1].value.pdb_int = drawable_ID(GIMP_DRAWABLE(channel));
    }

  return return_args;
}


