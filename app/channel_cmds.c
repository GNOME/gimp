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
#include "general.h"
#include "gimage.h"
#include "channel.h"
#include "channel_cmds.h"

#include "libgimp/gimpintl.h"

#include "channel_pvt.h"

static int int_value;
static double fp_value;
static int success;


/*****************/
/*  CHANNEL_NEW  */

static Argument *
channel_new_invoker (Argument *args)
{
  Channel *channel;
  int gimage_id;
  int width, height;
  char *name;
  int opacity;
  unsigned char color[3];
  Argument *return_args;

  channel   = NULL;
  gimage_id = -1;
  opacity   = 255;

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
    name = (char *) args[3].value.pdb_pointer;
  if (success)
    {
      fp_value = args[4].value.pdb_float;
      if (fp_value >= 0 && fp_value <= 100)
	opacity = (int) ((fp_value * 255) / 100);
      else
	success = FALSE;
    }
  if (success)
    {
      int i;
      unsigned char *color_array;

      color_array = (unsigned char *) args[5].value.pdb_pointer;
      for (i = 0; i < 3; i++)
	color[i] = color_array[i];
    }

  if (success)
    success = ((channel = channel_new (gimage_get_ID(gimage_id), width, height, name, opacity, color)) != NULL);

  return_args = procedural_db_return_args (&channel_new_proc, success);

  if (success)
    return_args[1].value.pdb_int = GIMP_DRAWABLE(channel)->ID;

  return return_args;
}

/*  The procedure definition  */
ProcArg channel_new_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image to which to add the channel"
  },
  { PDB_INT32,
    "width",
    "the channel width: (width > 0)"
  },
  { PDB_INT32,
    "height",
    "the channel height: (height > 0)"
  },
  { PDB_STRING,
    "name",
    "the channel name"
  },
  { PDB_FLOAT,
    "opacity",
    "the channel opacity: (0 <= opacity <= 100)"
  },
  { PDB_COLOR,
    "color",
    "the channel compositing color"
  }
};

ProcArg channel_new_out_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the newly created channel"
  }
};

ProcRecord channel_new_proc =
{
  "gimp_channel_new",
  "Create a new channel",
  "This procedure creates a new channel with the specified width and height.  Name, opacity, and color are also supplied parameters.  The new channel still needs to be added to the image, as this is not automatic.  Add the new channel with the 'gimp_image_add_channel' command.  Other attributes such as channel show masked, should be set with explicit procedure calls.  The channel's contents are undefined initially.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  6,
  channel_new_args,

  /*  Output arguments  */
  1,
  channel_new_out_args,

  /*  Exec method  */
  { { channel_new_invoker } },
};


/******************/
/*  CHANNEL_COPY  */

static Argument *
channel_copy_invoker (Argument *args)
{
  Channel *channel;
  Channel *copy;
  Argument *return_args;

  copy = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    success = ((copy = channel_copy (channel)) != NULL);

  return_args = procedural_db_return_args (&channel_copy_proc, success);

  if (success)
    return_args[1].value.pdb_int = GIMP_DRAWABLE(copy)->ID;

  return return_args;
}

/*  The procedure definition  */
ProcArg channel_copy_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel to copy"
  }
};

ProcArg channel_copy_out_args[] =
{
  { PDB_CHANNEL,
    "channel_copy",
    "the newly copied channel"
  }
};

ProcRecord channel_copy_proc =
{
  "gimp_channel_copy",
  "Copy a channel",
  "This procedure copies the specified channel and returns the copy.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  channel_copy_args,

  /*  Output arguments  */
  1,
  channel_copy_out_args,

  /*  Exec method  */
  { { channel_copy_invoker } },
};


/********************/
/*  CHANNEL_DELETE  */

static Argument *
channel_delete_invoker (Argument *args)
{
  Channel *channel;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  if (success)
    channel_delete (channel);

  return procedural_db_return_args (&channel_delete_proc, success);
}

/*  The procedure definition  */
ProcArg channel_delete_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel to delete"
  }
};

ProcRecord channel_delete_proc =
{
  "gimp_channel_delete",
  "Delete a channel",
  "This procedure deletes the specified channel.  This does not need to be done if a gimage containing this channel was already deleted.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  channel_delete_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { channel_delete_invoker } },
};


/**********************/
/*  CHANNEL_GET_NAME  */

static Argument *
channel_get_name_invoker (Argument *args)
{
  Channel *channel;
  char *name = NULL;
  Argument *return_args;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)))
	name = channel_get_name(channel);
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&channel_get_name_proc, success);

  if (success)
    return_args[1].value.pdb_pointer = (name) ? g_strdup (name) : NULL;

  return return_args;
}

/*  The procedure definition  */
ProcArg channel_get_name_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  }
};

ProcArg channel_get_name_out_args[] =
{
  { PDB_STRING,
    "name",
    "the channel name"
  }
};

ProcRecord channel_get_name_proc =
{
  "gimp_channel_get_name",
  "Get the name of the specified channel.",
  "This procedure returns the specified channel's name.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  channel_get_name_args,

  /*  Output arguments  */
  1,
  channel_get_name_out_args,

  /*  Exec method  */
  { { channel_get_name_invoker } },
};


/**********************/
/*  CHANNEL_SET_NAME  */

static Argument *
channel_set_name_invoker (Argument *args)
{
  Channel *channel;
  char *name;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      name = (char *) args[1].value.pdb_pointer;
      channel_set_name(channel, name);
    }

  return procedural_db_return_args (&channel_set_name_proc, success);
}

/*  The procedure definition  */
ProcArg channel_set_name_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  },
  { PDB_STRING,
    "name",
    "the new channel name"
  }
};

ProcRecord channel_set_name_proc =
{
  "gimp_channel_set_name",
  "Set the name of the specified channel.",
  "This procedure sets the specified channel's name to the supplied name.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  channel_set_name_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { channel_set_name_invoker } },
};


/*************************/
/*  CHANNEL_GET_VISIBLE  */

static Argument *
channel_get_visible_invoker (Argument *args)
{
  Channel *channel;
  int visible;
  Argument *return_args;

  visible = FALSE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)))
	visible = GIMP_DRAWABLE(channel)->visible;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&channel_get_visible_proc, success);

  if (success)
    return_args[1].value.pdb_int = visible;

  return return_args;
}

/*  The procedure definition  */
ProcArg channel_get_visible_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  }
};

ProcArg channel_get_visible_out_args[] =
{
  { PDB_INT32,
    "visible",
    "the channel visibility"
  }
};

ProcRecord channel_get_visible_proc =
{
  "gimp_channel_get_visible",
  "Get the visibility of the specified channel.",
  "This procedure returns the specified channel's visibility.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  channel_get_visible_args,

  /*  Output arguments  */
  1,
  channel_get_visible_out_args,

  /*  Exec method  */
  { { channel_get_visible_invoker } },
};


/*************************/
/*  CHANNEL_SET_VISIBLE  */

static Argument *
channel_set_visible_invoker (Argument *args)
{
  Channel *channel;
  int visible;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      visible = args[1].value.pdb_int;
      GIMP_DRAWABLE(channel)->visible = (visible) ? TRUE : FALSE;
    }

  return procedural_db_return_args (&channel_set_visible_proc, success);
}

/*  The procedure definition  */
ProcArg channel_set_visible_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  },
  { PDB_INT32,
    "visible",
    "the new channel visibility"
  }
};

ProcRecord channel_set_visible_proc =
{
  "gimp_channel_set_visible",
  "Set the visibility of the specified channel.",
  "This procedure sets the specified channel's visibility.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  channel_set_visible_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { channel_set_visible_invoker } },
};


/*****************************/
/*  CHANNEL_GET_SHOW_MASKED  */

static Argument *
channel_get_show_masked_invoker (Argument *args)
{
  Channel *channel;
  int show_masked;
  Argument *return_args;

  show_masked = FALSE;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)))
	show_masked = channel->show_masked;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&channel_get_show_masked_proc, success);

  if (success)
    return_args[1].value.pdb_int = show_masked;

  return return_args;
}

/*  The procedure definition  */
ProcArg channel_get_show_masked_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  }
};

ProcArg channel_get_show_masked_out_args[] =
{
  { PDB_INT32,
    "show_masked",
    "composite method for channel"
  }
};

ProcRecord channel_get_show_masked_proc =
{
  "gimp_channel_get_show_masked",
  "Get the composite type for the channel",
  "This procedure returns the specified channel's composite type.  If it is non-zero, then the channel is composited with the image so that masked regions are shown.  Otherwise, selected regions are shown.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  channel_get_show_masked_args,

  /*  Output arguments  */
  1,
  channel_get_show_masked_out_args,

  /*  Exec method  */
  { { channel_get_show_masked_invoker } },
};


/*****************************/
/*  CHANNEL_SET_SHOW_MASKED  */

static Argument *
channel_set_show_masked_invoker (Argument *args)
{
  Channel *channel;
  int show_masked;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      show_masked = args[1].value.pdb_int;
      channel->show_masked = (show_masked) ? TRUE : FALSE;
    }

  return procedural_db_return_args (&channel_set_show_masked_proc, success);
}

/*  The procedure definition  */
ProcArg channel_set_show_masked_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  },
  { PDB_INT32,
    "show_masked",
    "the new channel show_masked value"
  }
};

ProcRecord channel_set_show_masked_proc =
{
  "gimp_channel_set_show_masked",
  "Set the composite type for the specified channel.",
  "This procedure sets the specified channel's composite type.  If it is non-zero, then the channel is composited with the image so that masked regions are shown.  Otherwise, selected regions are shown.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  channel_set_show_masked_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { channel_set_show_masked_invoker } },
};


/*************************/
/*  CHANNEL_GET_OPACITY  */

static Argument *
channel_get_opacity_invoker (Argument *args)
{
  Channel *channel;
  double opacity;
  Argument *return_args;

  opacity = 0.0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)))
	opacity = ((channel->opacity) * 100.0) / 255.0;
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&channel_get_opacity_proc, success);

  if (success)
    return_args[1].value.pdb_float = opacity;

  return return_args;
}

/*  The procedure definition  */
ProcArg channel_get_opacity_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  }
};

ProcArg channel_get_opacity_out_args[] =
{
  { PDB_FLOAT,
    "opacity",
    "the channel opacity",
  }
};

ProcRecord channel_get_opacity_proc =
{
  "gimp_channel_get_opacity",
  "Get the opacity of the specified channel.",
  "This procedure returns the specified channel's opacity.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  channel_get_opacity_args,

  /*  Output arguments  */
  1,
  channel_get_opacity_out_args,

  /*  Exec method  */
  { { channel_get_opacity_invoker } },
};


/*************************/
/*  CHANNEL_SET_OPACITY  */

static Argument *
channel_set_opacity_invoker (Argument *args)
{
  Channel *channel;
  double opacity;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      opacity = args[1].value.pdb_float;
      channel->opacity = (int) ((opacity * 255) / 100);
    }

  return procedural_db_return_args (&channel_set_opacity_proc, success);
}

/*  The procedure definition  */
ProcArg channel_set_opacity_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  },
  { PDB_FLOAT,
    "opacity",
    "the new channel opacity: (0 <= opacity <= 100)"
  }
};

ProcRecord channel_set_opacity_proc =
{
  "gimp_channel_set_opacity",
  "Set the opacity of the specified channel.",
  "This procedure sets the specified channel's opacity.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  channel_set_opacity_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { channel_set_opacity_invoker } },
};


/***********************/
/*  CHANNEL_GET_COLOR  */

static Argument *
channel_get_color_invoker (Argument *args)
{
  Channel *channel;
  unsigned char *color;
  Argument *return_args;

  color = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)))
	{
	  color = (unsigned char *) g_malloc (3);
	  color[RED_PIX] = channel->col[RED_PIX];
	  color[GREEN_PIX] = channel->col[GREEN_PIX];
	  color[BLUE_PIX] = channel->col[BLUE_PIX];
	}
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&channel_get_color_proc, success);

  if (success)
    return_args[1].value.pdb_pointer = color;

  return return_args;
}

/*  The procedure definition  */
ProcArg channel_get_color_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  }
};

ProcArg channel_get_color_out_args[] =
{
  { PDB_COLOR,
    "color",
    "the channel's composite color",
  }
};

ProcRecord channel_get_color_proc =
{
  "gimp_channel_get_color",
  "Get the compositing color of the specified channel.",
  "This procedure returns the specified channel's compositing color.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  channel_get_color_args,

  /*  Output arguments  */
  1,
  channel_get_color_out_args,

  /*  Exec method  */
  { { channel_get_color_invoker } },
};


/***********************/
/*  CHANNEL_SET_COLOR  */

static Argument *
channel_set_color_invoker (Argument *args)
{
  Channel *channel;
  unsigned char * color;
  int i;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)) == NULL)
	success = FALSE;
    }
  if (success)
    {
      color = (unsigned char *) args[1].value.pdb_pointer;
      for (i = 0; i < 3; i++)
	channel->col[i] = color[i];
    }

  return procedural_db_return_args (&channel_set_color_proc, success);
}

/*  The procedure definition  */
ProcArg channel_set_color_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  },
  { PDB_COLOR,
    "color",
    "the composite color"
  }
};

ProcRecord channel_set_color_proc =
{
  "gimp_channel_set_color",
  "Set the compositing color of the specified channel.",
  "This procedure sets the specified channel's compositing color.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  channel_set_color_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { channel_set_color_invoker } },
};

/***************************/
/*  CHANNEL_GET_TATTOO_PROC  */

static Argument *
channel_get_tattoo_invoker (Argument *args)
{
  Channel *channel;
  int tattoo;
  Argument *return_args;

  tattoo = 0;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((channel = channel_get_ID (int_value)))
	tattoo = channel_get_tattoo (channel);
      else
	success = FALSE;
    }

  return_args = procedural_db_return_args (&channel_get_tattoo_proc, success);

  if (success)
    return_args[1].value.pdb_int = tattoo;

  return return_args;
}

/*  The procedure definition  */
ProcArg channel_get_tattoo_args[] =
{
  { PDB_CHANNEL,
    "channel",
    "the channel"
  }
};

ProcArg channel_get_tattoo_out_args[] =
{
  { PDB_INT32,
    "tattoo",
    "the tattoo associated with the given channel"
  }
};

ProcRecord channel_get_tattoo_proc =
{
  "gimp_channel_get_tattoo",
  "Returns the tattoo associated with the specified channel.",
  "This procedure returns the tattoo associated with the specified channel.  A tattoo is a unique and permenant identifier attached to a channel that can be used to uniquely identify a channel within an image even between sessions",
  "Jay Cox",
  "Jay Cox",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  channel_get_tattoo_args,

  /*  Output arguments  */
  1,
  channel_get_tattoo_out_args,

  /*  Exec method  */
  { { channel_get_tattoo_invoker } },
};
