/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpunit.c
 * Copyright (C) 1999 Michael Natterer <mitschel@cs.tu-berlin.de>
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

#include <stdlib.h>
#include <stdio.h>

/* NOTE:
 *
 * one of our header files is in libgimp/ (see the note there)
 */
#include "libgimp/gimpunit.h"

#include "unitrc.h"
#include "gimpunit_cmds.h"

#include "app_procs.h"
#include "gimprc.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimpenv.h"

/* internal structures */

typedef struct {
  guint    delete_on_exit;
  float    factor;
  gint     digits;
  gchar   *identifier;
  gchar   *symbol;
  gchar   *abbreviation;
  gchar   *singular;
  gchar   *plural;
} GimpUnitDef;

/*  these are the built-in units
 */
static GimpUnitDef gimp_unit_defs[UNIT_END] =
{
  /* pseudo unit */
  { FALSE,  0.0, 0, "pixels",      "px", "px", N_("pixel"),      N_("pixels") },

  /* standard units */
  { FALSE,  1.0, 2, "inches",      "''", "in", N_("inch"),       N_("inches") },
  { FALSE, 25.4, 1, "millimeters", "mm", "mm", N_("millimeter"), N_("millimeters") },

  /* professional units */
  { FALSE, 72.0, 0, "points",      "pt", "pt", N_("point"),      N_("points") },
  { FALSE,  6.0, 1, "picas",       "pc", "pc", N_("pica"),       N_("picas") },
};

static GSList* user_units = NULL;
static gint    number_of_user_units = 0;

static int success;

/* private functions */

static GimpUnitDef *
gimp_unit_get_user_unit (GUnit unit)
{
  return g_slist_nth_data (user_units, unit - UNIT_END);
}


/* public functions */

gint
gimp_unit_get_number_of_units (void)
{
  return UNIT_END + number_of_user_units;
}

gint
gimp_unit_get_number_of_built_in_units (void)
{
  return UNIT_END;
}


GUnit
gimp_unit_new (gchar  *identifier,
	       gfloat  factor,
	       gint    digits,
	       gchar  *symbol,
	       gchar  *abbreviation,
	       gchar  *singular,
	       gchar  *plural)
{
  GimpUnitDef *user_unit;

  user_unit = g_malloc (sizeof (GimpUnitDef));
  user_unit->delete_on_exit = TRUE;
  user_unit->factor = factor;
  user_unit->digits = digits;
  user_unit->identifier = g_strdup (identifier);
  user_unit->symbol = g_strdup (symbol);
  user_unit->abbreviation = g_strdup (abbreviation);
  user_unit->singular = g_strdup (singular);
  user_unit->plural = g_strdup (plural);

  user_units = g_slist_append (user_units, user_unit);
  number_of_user_units++;

  return UNIT_END + number_of_user_units - 1;
}


guint
gimp_unit_get_deletion_flag (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && 
			 (unit < (UNIT_END + number_of_user_units)), FALSE);

  if (unit < UNIT_END)
    return FALSE;

  return gimp_unit_get_user_unit (unit)->delete_on_exit;
}

void
gimp_unit_set_deletion_flag (GUnit  unit,
			     guint  deletion_flag)
{
  g_return_if_fail ( (unit >= UNIT_END) && 
		     (unit < (UNIT_END + number_of_user_units)));

  gimp_unit_get_user_unit (unit)->delete_on_exit =
    deletion_flag ? TRUE : FALSE;
}


gfloat
gimp_unit_get_factor (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && 
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].factor );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].factor;

  return gimp_unit_get_user_unit (unit)->factor;
}


gint
gimp_unit_get_digits (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].digits );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].digits;

  return gimp_unit_get_user_unit (unit)->digits;
}


gchar * 
gimp_unit_get_identifier (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) && 
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].identifier );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].identifier;

  return gimp_unit_get_user_unit (unit)->identifier;
}


gchar *
gimp_unit_get_symbol (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].symbol );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].symbol;

  return gimp_unit_get_user_unit (unit)->symbol;
}


gchar *
gimp_unit_get_abbreviation (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)),
			 gimp_unit_defs[UNIT_INCH].abbreviation );

  if (unit < UNIT_END)
    return gimp_unit_defs[unit].abbreviation;

  return gimp_unit_get_user_unit (unit)->abbreviation;
}


gchar *
gimp_unit_get_singular (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)),
			 gettext(gimp_unit_defs[UNIT_INCH].singular) );

  if (unit < UNIT_END)
    return gettext (gimp_unit_defs[unit].singular);

  return gimp_unit_get_user_unit (unit)->singular;
}


gchar *
gimp_unit_get_plural (GUnit unit)
{
  g_return_val_if_fail ( (unit >= UNIT_PIXEL) &&
			 (unit < (UNIT_END + number_of_user_units)),
			 gettext(gimp_unit_defs[UNIT_INCH].plural) );

  if (unit < UNIT_END)
    return gettext (gimp_unit_defs[unit].plural);

  return gimp_unit_get_user_unit (unit)->plural;
}


/*  unitrc functions  **********/

void parse_unitrc (void)
{
  char *filename;

  filename = gimp_personal_rc_file ("unitrc");
  app_init_update_status(NULL, filename, -1);
  parse_gimprc_file (filename);
  g_free (filename);
}


void save_unitrc (void)
{
  int i;
  char *filename;
  FILE *fp;

  filename = gimp_personal_rc_file ("unitrc");

  fp = fopen (filename, "w");
  g_free (filename);
  if (!fp)
    return;

  fprintf(fp, _("# GIMP unitrc\n"));
  fprintf(fp, _("# This file contains your user unit database. You can\n"));
  fprintf(fp, _("# modify this list with the unit editor. You are not\n"));
  fprintf(fp, _("# supposed to edit it manually, but of course you can do.\n"));
  fprintf(fp, _("# This file will be entirely rewritten every time you\n")); 
  fprintf(fp, _("# quit the gimp.\n\n"));
  
  /* save window geometries */
  for (i = gimp_unit_get_number_of_built_in_units();
       i < gimp_unit_get_number_of_units ();
       i++)
    if (gimp_unit_get_deletion_flag (i) == FALSE)
      {
	fprintf (fp,"(unit-info \"%s\"\n", gimp_unit_get_identifier (i));
	fprintf (fp,"   (factor %f)\n", gimp_unit_get_factor (i));
	fprintf (fp,"   (digits %d)\n", gimp_unit_get_digits (i));
	fprintf (fp,"   (symbol \"%s\")\n", gimp_unit_get_symbol (i));
	fprintf (fp,"   (abbreviation \"%s\")\n", gimp_unit_get_abbreviation (i));
	fprintf (fp,"   (singular \"%s\")\n", gimp_unit_get_singular (i));
	fprintf (fp,"   (plural \"%s\"))\n\n", gimp_unit_get_plural (i));
      }
  
  fclose (fp);
}


/*  PDB stuff  **********/

/********************************
 *  GIMP_UNIT_GET_NUMBER_OF_UNITS
 */

static Argument *
gimp_unit_get_number_of_units_invoker (Argument *args)
{
  Argument *return_args;

  return_args= procedural_db_return_args(&gimp_unit_get_number_of_units_proc,
					 TRUE);

  return_args[1].value.pdb_int = gimp_unit_get_number_of_units ();

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_get_number_of_units_out_args[] =
{
  { PDB_INT32,
    "#units",
    "the number of units"
  }
};

ProcRecord gimp_unit_get_number_of_units_proc =
{
  "gimp_unit_get_number_of_units",
  "Returns the number of units",
  "This procedure returns the number of defined units.",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  0, NULL,
  1, gimp_unit_get_number_of_units_out_args,

  /*  Exec method  */
  { { gimp_unit_get_number_of_units_invoker } },
};


/******************************
 *  GIMP_UNIT_NEW
 */

static Argument *
gimp_unit_new_invoker (Argument *args)
{
  Argument *return_args;

  return_args= procedural_db_return_args(&gimp_unit_new_proc, TRUE);

  return_args[1].value.pdb_int =
    gimp_unit_new (g_strdup (args[0].value.pdb_pointer),
		   args[1].value.pdb_float,
		   args[2].value.pdb_int,
		   g_strdup (args[3].value.pdb_pointer),
		   g_strdup (args[4].value.pdb_pointer),
		   g_strdup (args[5].value.pdb_pointer),
		   g_strdup (args[6].value.pdb_pointer));

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_new_args[] =
{
  { PDB_STRING,
    "identifier",
    "the new unit's identifier"
  },
  { PDB_FLOAT,
    "factor",
    "the new unit's factor"
  },
  { PDB_INT32,
    "digits",
    "the new unit's digits"
  },
  { PDB_STRING,
    "symbol",
    "the new unit's symbol"
  },
  { PDB_STRING,
    "abbreviation",
    "the new unit's abbreviation"
  },
  { PDB_STRING,
    "singular",
    "the new unit's singular form"
  },
  { PDB_STRING,
    "plural",
    "the new unit's plural form"
  }
};

ProcArg gimp_unit_new_out_args[] =
{
  { PDB_INT32,
    "unit ID",
    "the new unit's ID",
  }
};

ProcRecord gimp_unit_new_proc =
{
  "gimp_unit_new",
  "Creates a new unit and returns it's integer ID",
  "This procedure creates a new unit and returns it's integer ID. Note that the new unit will have it's deletion flag set to TRUE, so you will have to set it to FALSE with gimp_unit_set_deletion_flag to make it persistent.",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  7, gimp_unit_new_args,
  1, gimp_unit_new_out_args,

  /*  Exec method  */
  { { gimp_unit_new_invoker } },
};


/******************************
 *  GIMP_UNIT_GET_DELETION_FLAG
 */

static Argument *
gimp_unit_get_deletion_flag_invoker (Argument *args)
{
  Argument *return_args;
  GUnit unit;

  success = TRUE;

  unit = args[0].value.pdb_int;
  if ((unit < UNIT_PIXEL) || (unit >= gimp_unit_get_number_of_units ()))
    success = FALSE;

  return_args= procedural_db_return_args(&gimp_unit_get_deletion_flag_proc,
					 success);

  if (success)
    return_args[1].value.pdb_int = gimp_unit_get_deletion_flag (unit);

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_get_deletion_flag_args[] =
{
  { PDB_INT32,
    "unit ID",
    "the unit's integer ID"
  }
};

ProcArg gimp_unit_get_deletion_flag_out_args[] =
{
  { PDB_INT32,
    "boolean",
    "the unit's deletion flag",
  }
};

ProcRecord gimp_unit_get_deletion_flag_proc =
{
  "gimp_unit_get_deletion_flag",
  "Returns the deletion flag of the unit",
  "This procedure returns the deletion flag of the unit. If this value is TRUE the unit's definition will not be saved in the user's unitrc file on gimp exit.",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  1, gimp_unit_get_deletion_flag_args,
  1, gimp_unit_get_deletion_flag_out_args,

  /*  Exec method  */
  { { gimp_unit_get_deletion_flag_invoker } },
};


/******************************
 *  GIMP_UNIT_SET_DELETION_FLAG
 */

static Argument *
gimp_unit_set_deletion_flag_invoker (Argument *args)
{
  Argument *return_args;
  GUnit unit;

  success = TRUE;

  unit = args[0].value.pdb_int;
  if ((unit < UNIT_END) || (unit >= gimp_unit_get_number_of_units ()))
    success = FALSE;
  else
    gimp_unit_set_deletion_flag (unit, args[1].value.pdb_int);

  return_args= procedural_db_return_args(&gimp_unit_set_deletion_flag_proc,
					 success);

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_set_deletion_flag_args[] =
{
  { PDB_INT32,
    "unit ID",
    "the unit's integer ID"
  },
  { PDB_INT32,
    "boolean",
    "the new deletion flag of the unit",
  }
};

ProcRecord gimp_unit_set_deletion_flag_proc =
{
  "gimp_unit_set_deletion_flag",
  "Sets the deletion flag of a unit",
  "This procedure sets the unit's deletion flag. If the deletion flag of a unit is TRUE on gimp exit, this unit's definition will not be saved in the user's unitrc.",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  2, gimp_unit_set_deletion_flag_args,
  0, NULL,

  /*  Exec method  */
  { { gimp_unit_set_deletion_flag_invoker } },
};


/******************************
 *  GIMP_UNIT_GET_IDENTIFIER
 */

static Argument *
gimp_unit_get_identifier_invoker (Argument *args)
{
  Argument *return_args;
  GUnit unit;

  success = TRUE;

  unit = args[0].value.pdb_int;
  if ((unit < UNIT_PIXEL) || (unit >= gimp_unit_get_number_of_units ()))
    success = FALSE;

  return_args= procedural_db_return_args(&gimp_unit_get_identifier_proc,
					 success);

  return_args[1].value.pdb_pointer =
    success ? g_strdup (gimp_unit_get_identifier (unit)) : NULL;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_get_identifier_args[] =
{
  { PDB_INT32,
    "unit ID",
    "the unit's integer ID"
  }
};

ProcArg gimp_unit_get_identifier_out_args[] =
{
  { PDB_STRING,
    "string",
    "the unit's textual identifier",
  }
};

ProcRecord gimp_unit_get_identifier_proc =
{
  "gimp_unit_get_identifier",
  "Returns the identifier of the unit",
  "This procedure returns the textual identifier of the unit. For built-in units it will be the english singular form of the unit's name. For user-defined units this should equal to the singular form.",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  1, gimp_unit_get_identifier_args,
  1, gimp_unit_get_identifier_out_args,

  /*  Exec method  */
  { { gimp_unit_get_identifier_invoker } },
};


/******************************
 *  GIMP_UNIT_GET_FACTOR
 */

static Argument *
gimp_unit_get_factor_invoker (Argument *args)
{
  Argument *return_args;
  GUnit unit;

  success = TRUE;

  unit = args[0].value.pdb_int;
  if ((unit < UNIT_INCH) || (unit >= gimp_unit_get_number_of_units ()))
    success = FALSE;

  return_args= procedural_db_return_args(&gimp_unit_get_factor_proc, success);

  return_args[1].value.pdb_float =
    success ? gimp_unit_get_factor (unit) : 1.0;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_get_factor_args[] =
{
  { PDB_INT32,
    "unit ID",
    "the unit's integer ID"
  }
};

ProcArg gimp_unit_get_factor_out_args[] =
{
  { PDB_FLOAT,
    "float",
    "the unit's factor",
  }
};

ProcRecord gimp_unit_get_factor_proc =
{
  "gimp_unit_get_factor",
  "Returns the factor of the unit",
  "This procedure returns the unit's factor which indicates how many units make up an inch. Note that asking for the factor of \"pixels\" will produce an error.",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  1, gimp_unit_get_factor_args,
  1, gimp_unit_get_factor_out_args,

  /*  Exec method  */
  { { gimp_unit_get_factor_invoker } },
};


/******************************
 *  GIMP_UNIT_GET_DIGITS
 */

static Argument *
gimp_unit_get_digits_invoker (Argument *args)
{
  Argument *return_args;
  GUnit unit;

  success = TRUE;

  unit = args[0].value.pdb_int;
  if ((unit < UNIT_INCH) || (unit >= gimp_unit_get_number_of_units ()))
    success = FALSE;

  return_args= procedural_db_return_args(&gimp_unit_get_digits_proc, success);

  return_args[1].value.pdb_int =
    success ? gimp_unit_get_digits (unit) : 0;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_get_digits_args[] =
{
  { PDB_INT32,
    "unit ID",
    "the unit's integer ID"
  }
};

ProcArg gimp_unit_get_digits_out_args[] =
{
  { PDB_INT32,
    "digits",
    "the unit's number if digits",
  }
};

ProcRecord gimp_unit_get_digits_proc =
{
  "gimp_unit_get_digits",
  "Returns the digits of the unit",
  "This procedure returns the number of digits you should provide in input or output functions to get approximately the same accuracy as with two digits and inches. Note that asking for the digits of \"pixels\" will produce an error.",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  1, gimp_unit_get_digits_args,
  1, gimp_unit_get_digits_out_args,

  /*  Exec method  */
  { { gimp_unit_get_digits_invoker } },
};


/******************************
 *  GIMP_UNIT_GET_SYMBOL
 */

static Argument *
gimp_unit_get_symbol_invoker (Argument *args)
{
  Argument *return_args;
  GUnit unit;

  success = TRUE;

  unit = args[0].value.pdb_int;
  if ((unit < UNIT_PIXEL) || (unit >= gimp_unit_get_number_of_units ()))
    success = FALSE;

  return_args= procedural_db_return_args(&gimp_unit_get_symbol_proc, success);

  return_args[1].value.pdb_pointer =
    success ? g_strdup (gimp_unit_get_symbol (unit)) : NULL;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_get_symbol_args[] =
{
  { PDB_INT32,
    "unit ID",
    "the unit's integer ID"
  }
};

ProcArg gimp_unit_get_symbol_out_args[] =
{
  { PDB_STRING,
    "string",
    "the unit's symbol",
  }
};

ProcRecord gimp_unit_get_symbol_proc =
{
  "gimp_unit_get_symbol",
  "Returns the symbol of the unit",
  "This procedure returns the symbol of the unit (\"''\" for inches).",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  1, gimp_unit_get_symbol_args,
  1, gimp_unit_get_symbol_out_args,

  /*  Exec method  */
  { { gimp_unit_get_symbol_invoker } },
};


/******************************
 *  GIMP_UNIT_GET_ABBREVIATION
 */

static Argument *
gimp_unit_get_abbreviation_invoker (Argument *args)
{
  Argument *return_args;
  GUnit unit;

  success = TRUE;

  unit = args[0].value.pdb_int;
  if ((unit < UNIT_PIXEL) || (unit >= gimp_unit_get_number_of_units ()))
    success = FALSE;

  return_args= procedural_db_return_args(&gimp_unit_get_abbreviation_proc,
					 success);

  return_args[1].value.pdb_pointer =
    success ? g_strdup (gimp_unit_get_abbreviation (unit)) : NULL;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_get_abbreviation_args[] =
{
  { PDB_INT32,
    "unit ID",
    "the unit's integer ID"
  }
};

ProcArg gimp_unit_get_abbreviation_out_args[] =
{
  { PDB_STRING,
    "string",
    "the unit's abbreviation",
  }
};

ProcRecord gimp_unit_get_abbreviation_proc =
{
  "gimp_unit_get_abbreviation",
  "Returns the abbreviation of the unit",
  "This procedure returns the abbreviation of the unit (\"in\" for inches).",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  1, gimp_unit_get_abbreviation_args,
  1, gimp_unit_get_abbreviation_out_args,

  /*  Exec method  */
  { { gimp_unit_get_abbreviation_invoker } },
};


/******************************
 *  GIMP_UNIT_GET_SINGULAR
 */

static Argument *
gimp_unit_get_singular_invoker (Argument *args)
{
  Argument *return_args;
  GUnit unit;

  success = TRUE;

  unit = args[0].value.pdb_int;
  if ((unit < UNIT_PIXEL) || (unit >= gimp_unit_get_number_of_units ()))
    success = FALSE;

  return_args= procedural_db_return_args(&gimp_unit_get_singular_proc, success);

  return_args[1].value.pdb_pointer =
    success ? g_strdup (gimp_unit_get_singular (unit)) : NULL;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_get_singular_args[] =
{
  { PDB_INT32,
    "unit ID",
    "the unit's integer ID"
  }
};

ProcArg gimp_unit_get_singular_out_args[] =
{
  { PDB_STRING,
    "string",
    "the unit's singular form",
  }
};

ProcRecord gimp_unit_get_singular_proc =
{
  "gimp_unit_get_singular",
  "Returns the singular for of the unit",
  "This procedure returns the singular form of the unit.",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  1, gimp_unit_get_singular_args,
  1, gimp_unit_get_singular_out_args,

  /*  Exec method  */
  { { gimp_unit_get_singular_invoker } },
};


/******************************
 *  GIMP_UNIT_GET_PLURAL
 */

static Argument *
gimp_unit_get_plural_invoker (Argument *args)
{
  Argument *return_args;
  GUnit unit;

  success = TRUE;

  unit = args[0].value.pdb_int;
  if ((unit < UNIT_PIXEL) || (unit >= gimp_unit_get_number_of_units ()))
    success = FALSE;

  return_args= procedural_db_return_args(&gimp_unit_get_plural_proc, success);

  return_args[1].value.pdb_pointer =
    success ? g_strdup (gimp_unit_get_plural (unit)) : NULL;

  return return_args;
}

/*  The procedure definition  */
ProcArg gimp_unit_get_plural_args[] =
{
  { PDB_INT32,
    "unit ID",
    "the unit's integer ID"
  }
};

ProcArg gimp_unit_get_plural_out_args[] =
{
  { PDB_STRING,
    "string",
    "the unit's plural form",
  }
};

ProcRecord gimp_unit_get_plural_proc =
{
  "gimp_unit_get_plural",
  "Returns the plural form of the unit",
  "This procedure returns the plural form of the unit.",
  "Michael Natterer",
  "Michael Natterer",
  "1999",
  PDB_INTERNAL,

  /*  Input & output arguments  */
  1, gimp_unit_get_plural_args,
  1, gimp_unit_get_plural_out_args,

  /*  Exec method  */
  { { gimp_unit_get_plural_invoker } },
};
