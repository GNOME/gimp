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
#include <string.h>
#include "appenv.h"
#include "general.h"
#include "gdisplay.h"
#include "gdisplay_cmds.h"

static int int_value;
static int success;
static Argument *return_args;


/******************/
/*  GDISPLAY_NEW  */

static Argument *
gdisplay_new_invoker (Argument *args)
{
  GImage *gimage;
  GDisplay *gdisp;
  unsigned int scale = 0x101;

  gdisp = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;

      /*  make sure the image has layers before displaying it  */
      if (success && gimage->layers == NULL)
	success = FALSE;
    }

  if (success)
    success = ((gdisp = gdisplay_new (gimage, scale)) != NULL);

  /*  create the new image  */
  return_args = procedural_db_return_args (&gdisplay_new_proc, success);

  if (success)
    return_args[1].value.pdb_int = gdisp->ID;

  return return_args;
}

/*  The procedure definition  */
ProcArg gdisplay_new_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  }
};

ProcArg gdisplay_new_out_args[] =
{
  { PDB_DISPLAY,
    "display",
    "the new display"
  }
};

ProcRecord gdisplay_new_proc =
{
  "gimp_display_new",
  "Creates a new display for the specified image",
  "Creates a new display for the specified image.  If the image already has a display, another is added.  Multiple displays are handled transparently by the GIMP.  The newly created display is returned and can be subsequently destroyed with a call to 'gimp_display_delete'.  This procedure only makes sense for use with the GIMP UI.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gdisplay_new_args,

  /*  Output arguments  */
  1,
  gdisplay_new_out_args,

  /*  Exec method  */
  { { gdisplay_new_invoker } },
};


/*********************/
/*  GDISPLAY_DELETE  */

static Argument *
gdisplay_delete_invoker (Argument *args)
{
  GDisplay *gdisplay;

  success = TRUE;

  int_value = args[0].value.pdb_int;
  if ((gdisplay = gdisplay_get_ID (int_value)))
    gtk_widget_destroy (gdisplay->shell);
  else
    success = FALSE;

  return procedural_db_return_args (&gdisplay_delete_proc, success);
}

/*  The procedure definition  */
ProcArg gdisplay_delete_args[] =
{
  { PDB_DISPLAY,
    "display",
    "The display"
  }
};

ProcRecord gdisplay_delete_proc =
{
  "gimp_display_delete",
  "Delete the specified display",
  "This procedure removes the specified display.  If this is the last remaining display for the underlying image, then the image is deleted also.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gdisplay_delete_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gdisplay_delete_invoker } },
};


/*********************/
/*  GDISPLAYS_FLUSH  */

static Argument *
gdisplays_flush_invoker (Argument *args)
{
  success = TRUE;
  gdisplays_flush ();

  return procedural_db_return_args (&gdisplays_flush_proc, success);
}

ProcRecord gdisplays_flush_proc =
{
  "gimp_displays_flush",
  "Flush all internal changes to the user interface",
  "This procedure takes no arguments and returns nothing except a success status.  Its purpose is to flush all pending updates of image manipulations to the user interface.  It should be called whenever appropriate.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gdisplays_flush_invoker } },
};
