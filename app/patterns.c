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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "appenv.h"
#include "colormaps.h"
#include "datafiles.h"
#include "devices.h"
#include "patterns.h"
#include "pattern_header.h"
#include "pattern_select.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "menus.h"
#include "dialog_handler.h"

#include "libgimp/gimpintl.h"

/*  global variables  */
GPatternP           active_pattern = NULL;
GSList *            pattern_list = NULL;
int                 num_patterns = 0;

PatternSelectP      pattern_select_dialog = NULL;

/*  static variables  */
static int          success;
static Argument    *return_args;
static int          have_default_pattern = 0;

/*  static function prototypes  */
static GSList *     insert_pattern_in_list   (GSList *, GPatternP);
static void         load_pattern             (char *filename);
static void         free_pattern             (GPatternP);
static void         pattern_free_one         (gpointer, gpointer);
static gint         pattern_compare_func     (gconstpointer, 
					      gconstpointer);


/*  function declarations  */
void
patterns_init (int no_data)
{
  GSList *list;

  if (pattern_list)
    patterns_free ();

  pattern_list = NULL;
  num_patterns = 0;

  if (!pattern_path)
    return;
  if(!no_data)
    datafiles_read_directories (pattern_path, load_pattern, 0);

  /*  assign indexes to the loaded patterns  */

  list = pattern_list;

  while (list) {
    /*  Set the pattern index  */
    ((GPattern *) list->data)->index = num_patterns++;
    list = g_slist_next (list);
  }
}


static void
pattern_free_one (gpointer data, gpointer dummy)
{
  free_pattern ((GPatternP) data);
}

static gint
pattern_compare_func(gconstpointer first, gconstpointer second)
{
  return strcmp (((const GPatternP)first)->name, 
		 ((const GPatternP)second)->name);
}

void
patterns_free ()
{
  if (pattern_list) {
    g_slist_foreach (pattern_list, pattern_free_one, NULL);
    g_slist_free (pattern_list);
  }

  have_default_pattern = 0;
  active_pattern = NULL;
  num_patterns = 0;
  pattern_list = NULL;
}


void
pattern_select_dialog_free ()
{
  if (pattern_select_dialog)
    {
      pattern_select_free (pattern_select_dialog);
      pattern_select_dialog = NULL;
    }
}


GPatternP
get_active_pattern ()
{
  if (have_default_pattern)
    {
      have_default_pattern = 0;
      if (!active_pattern)
	fatal_error (_("Specified default pattern not found!"));

    }
  else if (! active_pattern && pattern_list)
    active_pattern = (GPatternP) pattern_list->data;

  return active_pattern;
}


static GSList *
insert_pattern_in_list (GSList *list, GPatternP pattern)
{
  return g_slist_insert_sorted (list, pattern, pattern_compare_func);
}


static void
load_pattern (char *filename)
{
  GPatternP pattern;
  FILE * fp;
  int bn_size;
  unsigned char buf [sz_PatternHeader];
  PatternHeader header;
  unsigned int * hp;
  int i;

  pattern = (GPatternP) g_malloc (sizeof (GPattern));

  pattern->filename = g_strdup (filename);
  pattern->name = NULL;
  pattern->mask = NULL;

  /*  Open the requested file  */
  if (! (fp = fopen (filename, "rb")))
    {
      free_pattern (pattern);
      return;
    }

  /*  Read in the header size  */
  if ((fread (buf, 1, sz_PatternHeader, fp)) < sz_PatternHeader)
    {
      fclose (fp);
      free_pattern (pattern);
      return;
    }

  /*  rearrange the bytes in each unsigned int  */
  hp = (unsigned int *) &header;
  for (i = 0; i < (sz_PatternHeader / 4); i++)
    hp [i] = (buf [i * 4] << 24) + (buf [i * 4 + 1] << 16) +
             (buf [i * 4 + 2] << 8) + (buf [i * 4 + 3]);

  /*  Check for correct file format */
  if (header.magic_number != GPATTERN_MAGIC)
    {
      /*  One thing that can save this error is if the pattern is version 1  */
      if (header.version != 1)
	{
	  fclose (fp);
	  free_pattern (pattern);
	  return;
	}
    }
  /*  Check for correct version  */
  if (header.version != FILE_VERSION)
    {
      g_message (_("Unknown GIMP version #%d in \"%s\"\n"), header.version,
		 filename);
      fclose (fp);
      free_pattern (pattern);
      return;
    }

  /*  Get a new pattern mask  */
  pattern->mask = temp_buf_new (header.width, header.height, header.bytes, 0, 0, NULL);

  /*  Read in the pattern name  */
  if ((bn_size = (header.header_size - sz_PatternHeader)))
    {
      pattern->name = (char *) g_malloc (sizeof (char) * bn_size);
      if ((fread (pattern->name, 1, bn_size, fp)) < bn_size)
	{
	  g_message (_("Error in GIMP pattern file...aborting."));
	  fclose (fp);
	  free_pattern (pattern);
	  return;
	}
    }
  else
    pattern->name = g_strdup (_("Unnamed"));

  /*  Read the pattern mask data  */
  /*  Read the image data  */
  if ((fread (temp_buf_data (pattern->mask), 1,
	      header.width * header.height * header.bytes, fp)) <
      header.width * header.height * header.bytes)
    g_message (_("GIMP pattern file appears to be truncated."));

  /*  Clean up  */
  fclose (fp);

  /*temp_buf_swap (pattern->mask);*/

  pattern_list = insert_pattern_in_list(pattern_list, pattern);

  /* Check if the current pattern is the default one */

  if (strcmp(default_pattern, g_basename(filename)) == 0) {
	  active_pattern = pattern;
	  have_default_pattern = 1;
  } /* if */
}

GPatternP           
pattern_list_get_pattern (GSList *list, char *name)
{
  GPatternP patternp;

  while (list)
    {
      patternp = (GPatternP) list->data;
      
      if (!strcmp (patternp->name, name))
	{
	  return patternp;
	}
      list = g_slist_next (list);
    }
  return NULL;
}

GPatternP
get_pattern_by_index (int index)
{
  GSList *list;
  GPatternP pattern = NULL;

  list = g_slist_nth (pattern_list, index);
  if (list)
    pattern = (GPatternP) list->data;

  return pattern;
}


void
select_pattern (pattern)
     GPatternP pattern;
{
  /*  Set the active pattern  */
  active_pattern = pattern;

  /*  Make sure the active pattern is unswapped... */
  /*temp_buf_unswap (pattern->mask);*/

  /*  Keep up appearances in the pattern dialog  */
  if (pattern_select_dialog)
    pattern_select_select (pattern_select_dialog, pattern->index);

  device_status_update (current_device);
}


void
create_pattern_dialog ()
{
  if (!pattern_select_dialog)
    {
      /*  Create the dialog...  */
      pattern_select_dialog = pattern_select_new (NULL,NULL);

      /* register this one only */
      dialog_register(pattern_select_dialog->shell);
    }
  else
    {
      /*  Popup the dialog  */
      if (!GTK_WIDGET_VISIBLE (pattern_select_dialog->shell))
	gtk_widget_show (pattern_select_dialog->shell);
      else
	gdk_window_raise(pattern_select_dialog->shell->window);
    }
}


static void
free_pattern (pattern)
     GPatternP pattern;
{
  if (pattern->mask)
    temp_buf_free (pattern->mask);
  if (pattern->filename)
    g_free (pattern->filename);
  if (pattern->name)
    g_free (pattern->name);

  g_free (pattern);
}


/**************************/
/*  PATTERNS_GET_PATTERN  */

static Argument *
patterns_get_pattern_invoker (Argument *args)
{
  GPatternP patternp;

  success = (patternp = get_active_pattern ()) != NULL;

  return_args = procedural_db_return_args (&patterns_get_pattern_proc, success);

  if (success)
    {
      return_args[1].value.pdb_pointer = g_strdup (patternp->name);
      return_args[2].value.pdb_int = patternp->mask->width;
      return_args[3].value.pdb_int = patternp->mask->height;
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg patterns_get_pattern_out_args[] =
{
  { PDB_STRING,
    "name",
    "the pattern name"
  },
  { PDB_INT32,
    "width",
    "the pattern width"
  },
  { PDB_INT32,
    "height",
    "the pattern height"
  }
};

ProcRecord patterns_get_pattern_proc =
{
  "gimp_patterns_get_pattern",
  "Retrieve information about the currently active pattern",
  "This procedure retrieves information about the currently active pattern.  This includes the pattern name, and the pattern extents (width and height).  All clone and bucket-fill operations with patterns will use this pattern to control the application of paint to the image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  3,
  patterns_get_pattern_out_args,

  /*  Exec method  */
  { { patterns_get_pattern_invoker } },
};


/*******************************/
/*  PATTERNS_GET_PATTERN_DATA  */

static Argument *
patterns_get_pattern_data_invoker (Argument *args)
{
  GPatternP patternp = NULL;
  GSList *list;
  char *name;

  success = (name = (char *) args[0].value.pdb_pointer) != NULL;

  if (!success)
    {
      /* No name use active pattern */
      success = (patternp = get_active_pattern ()) != NULL;
    }
  else
    {
      list = pattern_list;
      success = FALSE;

      while (list)
	{
	  patternp = (GPatternP) list->data;

	  if (!strcmp (patternp->name, name))
	    {
	      success = TRUE;
	      break;
	    }

	  list = g_slist_next (list);
	}
    }

  return_args = procedural_db_return_args (&patterns_get_pattern_data_proc, success);

  if (success)
    {
      return_args[1].value.pdb_pointer = g_strdup (patternp->name);
      return_args[2].value.pdb_int = patternp->mask->width;
      return_args[3].value.pdb_int = patternp->mask->height;
      return_args[4].value.pdb_int = patternp->mask->bytes;
      return_args[5].value.pdb_int = patternp->mask->height*patternp->mask->width*patternp->mask->bytes;
      return_args[6].value.pdb_pointer = g_malloc(return_args[5].value.pdb_int);
      g_memmove(return_args[6].value.pdb_pointer,
		temp_buf_data (patternp->mask),
		return_args[5].value.pdb_int); 
    }

  return return_args;
}

/*  The procedure definition  */

ProcArg patterns_get_pattern_data_in_args[] =
{
  { PDB_STRING,
    "name",
    "the pattern name (\"\" means current active pattern) "
  }
};

ProcArg patterns_get_pattern_data_out_args[] =
{
  { PDB_STRING,
    "name",
    "the pattern name"
  },
  { PDB_INT32,
    "width",
    "the pattern width"
  },
  { PDB_INT32,
    "height",
    "the pattern height"
  },
  { PDB_INT32, 
    "mask bpp",
    "pattern bytes per pixel"},
  { PDB_INT32, 
    "mask len",
    "length of pattern mask data"},
  { PDB_INT8ARRAY,
    "mask data",
    "the pattern mask data"},
};

ProcRecord patterns_get_pattern_data_proc =
{
  "gimp_patterns_get_pattern_data",
  "Retrieve information about the currently active pattern (including data)",
  "This procedure retrieves information about the currently active pattern.  This includes the pattern name, and the pattern extents (width and height). It also returns the pattern data",
  "Andy Thomas",
  "Andy Thomas",
  "1998",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(patterns_get_pattern_data_in_args) / sizeof(patterns_get_pattern_data_in_args[0]),
  patterns_get_pattern_data_in_args,

  /*  Output arguments  */
  sizeof(patterns_get_pattern_data_out_args) / sizeof(patterns_get_pattern_data_out_args[0]),
  patterns_get_pattern_data_out_args,

  /*  Exec method  */
  { { patterns_get_pattern_data_invoker } },
};


/**************************/
/*  PATTERNS_SET_PATTERN  */

static Argument *
patterns_set_pattern_invoker (Argument *args)
{
  GPatternP patternp;
  GSList *list;
  char *name;

  success = (name = (char *) args[0].value.pdb_pointer) != NULL;

  if (success)
    {
      list = pattern_list;
      success = FALSE;

      while (list)
	{
	  patternp = (GPatternP) list->data;

	  if (!strcmp (patternp->name, name))
	    {
	      success = TRUE;
	      select_pattern (patternp);
	      break;
	    }

	  list = g_slist_next (list);
	}
    }

  return procedural_db_return_args (&patterns_set_pattern_proc, success);
}

/*  The procedure definition  */
ProcArg patterns_set_pattern_args[] =
{
  { PDB_STRING,
    "name",
    "the pattern name"
  }
};

ProcRecord patterns_set_pattern_proc =
{
  "gimp_patterns_set_pattern",
  "Set the specified pattern as the active pattern",
  "This procedure allows the active pattern mask to be set by specifying its name.  The name is simply a string which corresponds to one of the names of the installed patterns.  If there is no matching pattern found, this procedure will return an error.  Otherwise, the specified pattern becomes active and will be used in all subsequent paint operations.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  patterns_set_pattern_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { patterns_set_pattern_invoker } },
};


/*******************/
/*  PATTERNS_LIST  */

static Argument *
patterns_list_invoker (Argument *args)
{
  GPatternP patternp;
  GSList *list;
  char **patterns;
  int i;

  patterns = (char **) g_malloc (sizeof (char *) * num_patterns);

  success = (list = pattern_list) != NULL;
  i = 0;

  while (list)
    {
      patternp = (GPatternP) list->data;

      patterns[i++] = g_strdup (patternp->name);
      list = g_slist_next (list);
    }

  return_args = procedural_db_return_args (&patterns_list_proc, success);

  if (success)
    {
      return_args[1].value.pdb_int = num_patterns;
      return_args[2].value.pdb_pointer = patterns;
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg patterns_list_out_args[] =
{
  { PDB_INT32,
    "num_patterns",
    "the number of patterns in the pattern list"
  },
  { PDB_STRINGARRAY,
    "pattern_list",
    "the list of pattern names"
  }
};

ProcRecord patterns_list_proc =
{
  "gimp_patterns_list",
  "Retrieve a complete listing of the available patterns",
  "This procedure returns a complete listing of available GIMP patterns.  Each name returned can be used as input to the 'gimp_patterns_set_pattern'",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  2,
  patterns_list_out_args,

  /*  Exec method  */
  { { patterns_list_invoker } },
};
