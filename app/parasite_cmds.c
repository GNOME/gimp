/* parasite_cmds.c
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "parasite.h"
#include "procedural_db.h"
#include "appenv.h"
#include <stdio.h>

static Argument *parasite_new_invoker (Argument *args);

ProcArg parasite_new_args[] =
{
  { PDB_STRING,
    "creator",
    "The creator ID of the parasite to create"
  },
  { PDB_STRING,
    "type",
    "The type ID of the parasite to create"
  },
  { PDB_INT32,
    "flags",
    "The flags ( 1 == persistance )"
  },
  { PDB_INT32,
    "size",
    "The size of the data in bytes"
  },
  { PDB_STRING,
    "data",
    "The data"
  }
};

ProcArg parasite_new_out_args[] =
{
  { PDB_PARASITE,
    "parasite",
    "the new parasite"
  },
};

ProcRecord parasite_new_proc =
{
  "parasite_new",
  "creates a new parasite.",
  "creates a new parasite unatached to to any image or drawable",
  "Jay Cox",
  "Jay Cox",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  parasite_new_args,

  /*  Output arguments  */
  1,
  parasite_new_out_args,

  /*  Exec method  */
  { { parasite_new_invoker } },
};

static Argument *
parasite_new_invoker (Argument *args)
{
  int success = TRUE;
  Argument *return_args;
  char *creator, *type;
  guint32 flags, size;
  void *data;

  /*  creator  */
  if (success)
  { creator = (char *) args[0].value.pdb_pointer; }
  /*  type  */
  if (success)
  {  type = (char *) args[1].value.pdb_pointer;   }
  /*  flags  */
  if (success)
  {  flags =  args[2].value.pdb_int;   }
  /*  size  */
  if (success)
  {  size =   args[3].value.pdb_int;   }
  /*  data  */
  if (success)
  {  
    data =   args[4].value.pdb_pointer;
    if (size > 0 && data == 0)
      success = FALSE;
  }

  return_args = procedural_db_return_args (&parasite_new_proc,
					   success);
  /*  The real work  */
  if (success)
    {
      return_args[1].value.pdb_pointer = 
	parasite_new (creator, type, flags, size, data);
    }

  return return_args;
}
