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

#include "libgimp/parasite.h"
#include "parasitelist.h"
#include "gimpparasite.h"
#include "procedural_db.h"
#include "appenv.h"
#include <stdio.h>

/* The Parasite procs prototypes */
static Argument *parasite_new_invoker (Argument *);
static Argument *gimp_find_parasite_invoker (Argument *);
static Argument *gimp_attach_parasite_invoker (Argument *);
static Argument *gimp_detach_parasite_invoker (Argument *);

static Argument *parasite_new_invoker (Argument *args);


/***** parasite_new ****/
ProcArg parasite_new_args[] =
{
  { PDB_STRING,
    "name",
    "The name of the parasite to create"
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
  4,
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
  char *name;
  guint32 flags, size;
  void *data;

  /*  name  */
  if (success)
  { name = (char *) args[0].value.pdb_pointer; }
  /*  flags  */
  if (success)
  {  flags =  args[1].value.pdb_int;   }
  /*  size  */
  if (success)
  {  size =   args[2].value.pdb_int;   }
  /*  data  */
  if (success)
  {  
    data =   args[3].value.pdb_pointer;
    if (size > 0 && data == 0)
      success = FALSE;
  }

  return_args = procedural_db_return_args (&parasite_new_proc,
                                           success);
  /*  The real work  */
  if (success)
    {
      return_args[1].value.pdb_pointer = 
        parasite_new (name, flags, size, data);
    }

  return return_args;
}


/***** gimp_find_parasite *****/
ProcArg gimp_find_parasite_args[] =
{
  { PDB_STRING,
    "name",
    "The name of the parasite to find"
  },
};

ProcArg gimp_find_parasite_out_args[] =
{
  { PDB_PARASITE,
    "parasite",
    "the found parasite"
  },
};

ProcRecord gimp_find_parasite_proc =
{
  "gimp_find_parasite",
  "Finds the named parasite.",
  "Finds and returns the named parasite that was previously attached to the gimp.",
  "Jay Cox",
  "Jay Cox",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimp_find_parasite_args,

  /*  Output arguments  */
  1,
  gimp_find_parasite_out_args,

  /*  Exec method  */
  { { gimp_find_parasite_invoker } },
};


static Argument *
gimp_find_parasite_invoker (Argument *args)
{
  int success = TRUE;
  Argument *return_args;
  char *name = NULL;

  /*  name  */
  if (success)
    {
      name = (char *) args[0].value.pdb_pointer;
    }

  return_args = procedural_db_return_args (&gimp_find_parasite_proc,
					   success);
  /*  The real work  */
  if (success)
    {
      return_args[1].value.pdb_pointer = 
	parasite_copy(gimp_find_parasite (name));
    }

  return return_args;
}

/*** gimp_attach_parasite ***/

ProcArg gimp_attach_parasite_args[] =
{
  { PDB_PARASITE,
    "parasite",
    "The parasite to attach to the gimp"
  }
};

ProcRecord gimp_attach_parasite_proc =
{
  "gimp_attach_parasite",
  "Add a parasite to the gimp",
  "This procedure attaches a parasite to the gimp.  It has no return values.",
  "Jay Cox",
  "Jay Cox",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimp_attach_parasite_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimp_attach_parasite_invoker } },
};


static Argument *
gimp_attach_parasite_invoker (Argument *args)
{
  int success = TRUE;
  Parasite *parasite = NULL;
  Argument *return_args;



  if (success)
    {
      parasite = (Parasite *)args[0].value.pdb_pointer;
      if (parasite == NULL)
	success = FALSE;
    }

  if (success)
    {
      gimp_attach_parasite (parasite);
    }

  return_args = procedural_db_return_args (&gimp_attach_parasite_proc,
					   success);

  return return_args;
}


/*** gimp_detach_parasite ***/

ProcArg gimp_detach_parasite_args[] =
{
  { PDB_STRING,
    "name",
    "The name of the parasite to detach from the gimp"
  }
};

ProcRecord gimp_detach_parasite_proc =
{
  "gimp_detach_parasite",
  "Removes a parasite from the gimp.",
  "This procedure detaches a parasite from the gimp.  It has no return values.",
  "Jay Cox",
  "Jay Cox",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  gimp_detach_parasite_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { gimp_detach_parasite_invoker } },
};


static Argument *
gimp_detach_parasite_invoker (Argument *args)
{
  int success = TRUE;
  char *parasite = NULL;
  Argument *return_args;

  if (success)
    {
      parasite = (char *)args[0].value.pdb_pointer;
      if (parasite == NULL)
	success = FALSE;
    }

  if (success)
    {
      gimp_detach_parasite (parasite);
    }

  return_args = procedural_db_return_args (&gimp_detach_parasite_proc,
					   success);

  return return_args;
}
