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
#include "gimage.h"
#include "gimage_mask.h"
#include "global_edit.h"
#include "edit_cmds.h"

static int int_value;
static int success;
static Argument *return_args;

extern TileManager *global_buf;


/**************/
/*  EDIT_CUT  */

static Argument *
edit_cut_invoker (Argument *args)
{
  GImage *gimage;
  GimpDrawable *drawable;

  drawable = NULL;

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
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  create the new image  */
  if (success)
    success = (edit_cut (gimage, drawable) != NULL);

  return procedural_db_return_args (&edit_cut_proc, success);
}

/*  The procedure definition  */
ProcArg edit_cut_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The drawable to cut from"
  }
};

ProcRecord edit_cut_proc =
{
  "gimp_edit_cut",
  "Cut from the specified drawable",
  "If there is a selection in the image, then the area specified by the selection is cut from the specified drawable and placed in an internal GIMP edit buffer.  It can subsequently be retrieved using the 'gimp-edit-paste' command.  If there is no selection, then the specified drawable will be removed and its contents stored in the internal GIMP edit buffer.  The drawable MUST belong to the specified image, or an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  edit_cut_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { edit_cut_invoker } },
};


/***************/
/*  EDIT_COPY  */

static Argument *
edit_copy_invoker (Argument *args)
{
  GImage *gimage;
  GimpDrawable *drawable;

  drawable = NULL;

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
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }

  /*  create the new image  */
  if (success)
    success = (edit_copy (gimage, drawable) != NULL);

  return procedural_db_return_args (&edit_copy_proc, success);
}

/*  The procedure definition  */
ProcArg edit_copy_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The drawable to copy from"
  }
};

ProcRecord edit_copy_proc =
{
  "gimp_edit_copy",
  "Copy from the specified drawable",
  "If there is a selection in the image, then the area specified by the selection is copied from the specified drawable and placed in an internal GIMP edit buffer.  It can subsequently be retrieved using the 'gimp-edit-paste' command.  If there is no selection, then the specified drawable's contents will be stored in the internal GIMP edit buffer.  The drawable MUST belong to the specified image, or an error is returned.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  edit_copy_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { edit_copy_invoker } },
};


/****************/
/*  EDIT_PASTE  */

static Argument *
edit_paste_invoker (Argument *args)
{
  GImage *gimage;
  GimpDrawable *drawable;
  int paste_into;

  drawable = NULL;

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
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  if (success)
    {
      int_value = args[2].value.pdb_int;
      paste_into = (int_value) ? TRUE : FALSE;
    }

  /*  create the new image  */
  if (success)
    success = edit_paste (gimage, drawable, global_buf, paste_into);

  return_args = procedural_db_return_args (&edit_paste_proc, success);

  if (success)
    return_args[1].value.pdb_int = success;

  return return_args;
}

/*  The procedure definition  */
ProcArg edit_paste_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The drawable to paste from"
  },
  { PDB_INT32,
    "paste_into",
    "clear selection, or paste behind it?"
  }
};

ProcArg edit_paste_out_args[] =
{
  { PDB_LAYER,
    "floating_sel",
    "the new floating selection"
  }
};

ProcRecord edit_paste_proc =
{
  "gimp_edit_paste",
  "Paste buffer to the specified drawable",
  "This procedure pastes a copy of the internal GIMP edit buffer to the specified drawable.  The GIMP edit buffer will be empty unless a call was previously made to either 'gimp-edit-cut' or 'gimp-edit-copy'.  The \"paste_into\" option specifies whether to clear the current image selection, or to paste the buffer \"behind\" the selection.  This allows the selection to act as a mask for the pasted buffer.  Anywhere that the selection mask is non-zero, the pasted buffer will show through.  The pasted buffer will be a new layer in the image which is designated as the image floating selection.  If the image has a floating selection at the time of pasting, the old floating selection will be anchored to it's drawable before the new floating selection is added.  This procedure returns the new floating layer.  The resulting floating selection will already be attached to the specified drawable, and a subsequent call to floating_sel_attach is not needed.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  3,
  edit_paste_args,

  /*  Output arguments  */
  1,
  edit_paste_out_args,

  /*  Exec method  */
  { { edit_paste_invoker } },
};


/****************/
/*  EDIT_CLEAR  */

static Argument *
edit_clear_invoker (Argument *args)
{
  GImage *gimage;
  GimpDrawable *drawable;

  drawable = NULL;

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
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }

  /*  create the new image  */
  if (success)
    success = edit_clear (gimage, drawable);

  return procedural_db_return_args (&edit_clear_proc, success);
}

/*  The procedure definition  */
ProcArg edit_clear_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The drawable to clear from"
  }
};

ProcRecord edit_clear_proc =
{
  "gimp_edit_clear",
  "Clear selected area of drawable",
  "This procedure clears the specified drawable.  If the drawable has an alpha channel, the cleared pixels will become transparent.  If the drawable does not have an alpha channel, cleared pixels will be set to the background color.  This procedure only affects regions within a selection if there is a selection active.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  edit_clear_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { edit_clear_invoker } },
};


/***************/
/*  EDIT_FILL  */

static Argument *
edit_fill_invoker (Argument *args)
{
  GImage *gimage;
  GimpDrawable *drawable;

  drawable = NULL;
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
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }

  /*  create the new image  */
  if (success)
    success = edit_fill (gimage, drawable);

  return procedural_db_return_args (&edit_fill_proc, success);
}

/*  The procedure definition  */
ProcArg edit_fill_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The drawable to fill from"
  }
};

ProcRecord edit_fill_proc =
{
  "gimp_edit_fill",
  "Fill selected area of drawable",
  "This procedure fills the specified drawable with the background color.  This procedure only affects regions within a selection if there is a selection active.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  edit_fill_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { edit_fill_invoker } },
};


/*****************/
/*  EDIT_STROKE  */

static Argument *
edit_stroke_invoker (Argument *args)
{
  GImage *gimage;
  GimpDrawable *drawable;

  drawable = NULL;

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
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }

  /*  create the new image  */
  if (success)
    success = gimage_mask_stroke (gimage, drawable);

  return procedural_db_return_args (&edit_stroke_proc, success);
}

/*  The procedure definition  */
ProcArg edit_stroke_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "The drawable to stroke to"
  }
};

ProcRecord edit_stroke_proc =
{
  "gimp_edit_stroke",
  "Stroke the current selection",
  "This procedure strokes the current selection, painting along the selection boundary with the active brush and foreground color.  The paint is applied to the specified drawable regardless of the active selection.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  2,
  edit_stroke_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { edit_stroke_invoker } },
};
