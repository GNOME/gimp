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
#include "undo.h"
#include "undo_cmds.h"

#include "libgimp/gimpintl.h"

static int int_value;
static int success;

/***************************/
/*  UNDO_PUSH_GROUP_START  */

static Argument *
undo_push_group_start_invoker (Argument *args)
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
    undo_push_group_start (gimage, MISC_UNDO);

  return procedural_db_return_args (&undo_push_group_start_proc, success);
}

/*  The procedure definition  */
ProcArg undo_push_group_start_args[] =
{
  { PDB_IMAGE,
    "image",
    N_("The ID of the image in which to pop an undo group")
  }
};

ProcRecord undo_push_group_start_proc =
{
  "gimp_undo_push_group_start",
  N_("Starts a group undo"),
  N_("This function is used to start a group undo--necessary for logically combining two or more undo operations into a single operation.  This call must be used in conjunction with a 'gimp_undo_push_group_end' call."),
  N_("Spencer Kimball & Peter Mattis"),
  N_("Spencer Kimball & Peter Mattis"),
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  undo_push_group_start_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { undo_push_group_start_invoker } },
};


/*************************/
/*  UNDO_PUSH_GROUP_END  */

static Argument *
undo_push_group_end_invoker  (Argument *args)
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
    undo_push_group_end (gimage);

  return procedural_db_return_args (&undo_push_group_end_proc, success);
}

/*  The procedure definition  */
ProcArg undo_push_group_end_args[] =
{
  { PDB_IMAGE,
    "image",
    N_("The ID of the image in which to pop an undo group")
  }
};

ProcRecord undo_push_group_end_proc =
{
  "gimp_undo_push_group_end",
  N_("Finish a group undo"),
  N_("This function must be called once for each undo_push_group call that is made."),
  N_("Spencer Kimball & Peter Mattis"),
  N_("Spencer Kimball & Peter Mattis"),
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  undo_push_group_end_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { undo_push_group_end_invoker } },
};
