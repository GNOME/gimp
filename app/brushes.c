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
#include "brushes.h"
#include "brush_header.h"
#include "brush_select.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "datafiles.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "menus.h"


/*  global variables  */
GBrushP             active_brush = NULL;
GSList *            brush_list = NULL;
int                 num_brushes = 0;

double              opacity = 1.0;
int                 paint_mode = 0;


BrushSelectP        brush_select_dialog = NULL;


/*  static variables  */
static int          success;
static Argument    *return_args;
static int          have_default_brush = 0;

/*  static function prototypes  */
static GSList *     insert_brush_in_list   (GSList *, GBrushP);
static void         create_default_brush   (void);
static void         load_brush             (char *filename);
static void         free_brush             (GBrushP);
static void         brushes_free_one       (gpointer, gpointer);
static gint         brush_compare_func     (gconstpointer, gconstpointer);

/*  function declarations  */
void
brushes_init (int no_data)
{
  GSList * list;
  GBrushP gb_start = NULL;
  gint gb_count = 0;

  if (brush_list)
    brushes_free();

  brush_list = NULL;
  num_brushes = 0;

  if (!brush_path)
    create_default_brush ();
  if(!no_data)
    datafiles_read_directories (brush_path, load_brush, 0);

  /*  assign indexes to the loaded brushes  */

  list = brush_list;

  while (list) {
    /*  Set the brush index  */
    /* ALT make names unique */
    GBrushP gb = (GBrushP)list->data;
    gb->index = num_brushes++;
    list = g_slist_next(list);
    if(list) {
      GBrushP gb2 = (GBrushP)list->data;

      if(gb_start == NULL) {
        gb_start = gb;
      }

      if(gb_start->name
	 && gb2->name
	 && (strcmp(gb_start->name,gb2->name) == 0)) {

        gint b_digits = 2;
        gint gb_tmp_cnt = gb_count++;

        /* Alter gb2... */
        g_free(gb2->name);
        while((gb_tmp_cnt /= 10) > 0)
          b_digits++;
        /* name str + " #" + digits + null */
        gb2->name = g_malloc(strlen(gb_start->name)+3+b_digits);
        sprintf(gb2->name,"%s #%d",gb_start->name,gb_count);
      }
      else
      {
        gb_start = gb2;
        gb_count = 0;
      }
    }
  }
}


static void
brushes_free_one (gpointer data, gpointer dummy)
{
  free_brush ((GBrushP) data);
}


static gint
brush_compare_func (gconstpointer first, gconstpointer second)
{
  return strcmp (((GBrushP)first)->name, ((GBrushP)second)->name);
}


void
brushes_free ()
{
  if (brush_list) {
    g_slist_foreach (brush_list, brushes_free_one, NULL);
    g_slist_free (brush_list);
  }

  have_default_brush = 0;
  active_brush = NULL;
  num_brushes = 0;
  brush_list = NULL;
}


void
brush_select_dialog_free ()
{
  if (brush_select_dialog)
    {
      brush_select_free (brush_select_dialog);
      brush_select_dialog = NULL;
    }
}


GBrushP
get_active_brush ()
{
  if (have_default_brush)
    {
      have_default_brush = 0;
      if (!active_brush)
	fatal_error ("Specified default brush not found!");
    }
  else if (! active_brush && brush_list)
    active_brush = (GBrushP) brush_list->data;

  return active_brush;
}


static GSList *
insert_brush_in_list (GSList *list, GBrushP brush)
{
  return g_slist_insert_sorted (list, brush, brush_compare_func);
}

static void
create_default_brush ()
{
  GBrushP brush;
  gchar filled[] = { 255 };

  brush = g_new (GBrush, 1);

  brush->filename = NULL;
  brush->name = g_strdup ("Default");
  brush->mask = temp_buf_new (1, 1, 1, 0, 0, (unsigned char *)filled);
  brush->spacing = 25;

  /*  Swap the brush to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    temp_buf_swap (brush->mask);

  brush_list = insert_brush_in_list(brush_list, brush);

  /*  Make this the default, active brush  */
  active_brush = brush;
  have_default_brush = 1;
}

static void
load_brush(char *filename)
{
  GBrushP brush;
  FILE * fp;
  int bn_size;
  unsigned char buf [sz_BrushHeader];
  BrushHeader header;
  unsigned int * hp;
  int i;

  brush = (GBrushP) g_malloc (sizeof (GBrush));

  brush->filename = g_strdup (filename);
  brush->name = NULL;
  brush->mask = NULL;

  /*  Open the requested file  */
  if (! (fp = fopen (filename, "r")))
    {
      free_brush (brush);
      return;
    }

  /*  Read in the header size  */
  if ((fread (buf, 1, sz_BrushHeader, fp)) < sz_BrushHeader)
    {
      fclose (fp);
      free_brush (brush);
      return;
    }

  /*  rearrange the bytes in each unsigned int  */
  hp = (unsigned int *) &header;
  for (i = 0; i < (sz_BrushHeader / 4); i++)
    hp [i] = (buf [i * 4] << 24) + (buf [i * 4 + 1] << 16) +
             (buf [i * 4 + 2] << 8) + (buf [i * 4 + 3]);

  /*  Check for correct file format */
  if (header.magic_number != GBRUSH_MAGIC)
    {
      /*  One thing that can save this error is if the brush is version 1  */
      if (header.version != 1)
	{
	  fclose (fp);
	  free_brush (brush);
	  return;
	}
    }
  /*  Check for correct version  */
  if (header.version != FILE_VERSION)
    {
      /*  If this is a version 1 brush, set the fp back 8 bytes  */
      if (header.version == 1)
	{
	  fseek (fp, -8, SEEK_CUR);
	  header.header_size += 8;
	  /*  spacing is not defined in version 1  */
	  header.spacing = 25;
	}
      else
	{
	  g_message ("Unknown GIMP version #%d in \"%s\"\n", header.version,
		     filename);
	  fclose (fp);
	  free_brush (brush);
	  return;
	}
    }

  /*  Get a new brush mask  */
  brush->mask = temp_buf_new (header.width, header.height, header.bytes, 0, 0, NULL);
  brush->spacing = header.spacing;

  /*  Read in the brush name  */
  if ((bn_size = (header.header_size - sz_BrushHeader)))
    {
      brush->name = (char *) g_malloc (sizeof (char) * bn_size);
      if ((fread (brush->name, 1, bn_size, fp)) < bn_size)
	{
	  g_message ("Error in GIMP brush file...aborting.");
	  fclose (fp);
	  free_brush (brush);
	  return;
	}
    }
  else
    brush->name = g_strdup ("Unnamed");

  /*  Read the brush mask data  */
  /*  Read the image data  */
  if ((fread (temp_buf_data (brush->mask), 1, header.width * header.height, fp)) <
      header.width * header.height)
    g_message ("GIMP brush file appears to be truncated.");

  /*  Clean up  */
  fclose (fp);

  /*  Swap the brush to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    temp_buf_swap (brush->mask);

  brush_list = insert_brush_in_list(brush_list, brush);

  /* Check if the current brush is the default one */

  if (strcmp(default_brush, prune_filename(filename)) == 0) {
	  active_brush = brush;
	  have_default_brush = 1;
  } /* if */
}


GBrushP
get_brush_by_index (int index)
{
  GSList *list;
  GBrushP brush = NULL;

  list = g_slist_nth (brush_list, index);
  if (list)
    brush = (GBrushP) list->data;

  return brush;
}


void
select_brush (GBrushP brush)
{
  /*  Make sure the active brush is swapped before we get a new one... */
  if (stingy_memory_use)
    temp_buf_swap (active_brush->mask);

  /*  Set the active brush  */
  active_brush = brush;

  /*  Make sure the active brush is unswapped... */
  if (stingy_memory_use)
    temp_buf_unswap (brush->mask);

  /*  Keep up appearances in the brush dialog  */
  if (brush_select_dialog)
    brush_select_select (brush_select_dialog, brush->index);
}


void
create_brush_dialog ()
{
  if (!brush_select_dialog)
    {
      /*  Create the dialog...  */
      brush_select_dialog = brush_select_new ();
    }
  else
    {
      /*  Popup the dialog  */
      if (!GTK_WIDGET_VISIBLE (brush_select_dialog->shell))
	gtk_widget_show (brush_select_dialog->shell);
      else
	gdk_window_raise(brush_select_dialog->shell->window);
    }

}


static void
free_brush (brush)
     GBrushP brush;
{
  if (brush->mask)
    temp_buf_free (brush->mask);
  if (brush->filename)
    g_free (brush->filename);
  if (brush->name)
    g_free (brush->name);

  g_free (brush);
}

double
get_brush_opacity ()
{
  return opacity;
}

int
get_brush_spacing ()
{
  if (active_brush)
    return active_brush->spacing;
  else
    return 0;
}

int
get_brush_paint_mode ()
{
  return paint_mode;
}

void
set_brush_opacity (opac)
     double opac;
{
  opacity = opac;
}

void
set_brush_spacing (spac)
     int spac;
{
  if (active_brush)
    active_brush->spacing = spac;
}


void
set_brush_paint_mode (pm)
     int pm;
{
  paint_mode = pm;
}


/***********************/
/*  BRUSHES_GET_BRUSH  */

static Argument *
brushes_get_brush_invoker (Argument *args)
{
  GBrushP brushp;

  success = (brushp = get_active_brush ()) != NULL;

  return_args = procedural_db_return_args (&brushes_get_brush_proc, success);

  if (success)
    {
      return_args[1].value.pdb_pointer = g_strdup (brushp->name);
      return_args[2].value.pdb_int = brushp->mask->width;
      return_args[3].value.pdb_int = brushp->mask->height;
      return_args[4].value.pdb_int = brushp->spacing;
    }

  return return_args;
}

static Argument *
brushes_refresh_brush_invoker (Argument *args)
{

  /* FIXME: I've hardcoded success to be 1, because brushes_init() is a
   *        void function right now.  It'd be nice if it returned a value at
   *        some future date, so we could tell if things blew up when reparsing
   *        the list (for whatever reason).
   *                       - Seth "Yes, this is a kludge" Burgess
   *                         <sjburges@ou.edu>
   */

  success = TRUE ;
  brushes_init(FALSE);
  return procedural_db_return_args (&brushes_refresh_brush_proc, success);
}


/*  The procedure definition  */
ProcArg brushes_get_brush_out_args[] =
{
  { PDB_STRING,
    "name",
    "the brush name"
  },
  { PDB_INT32,
    "width",
    "the brush width"
  },
  { PDB_INT32,
    "height",
    "the brush height"
  },
  { PDB_INT32,
    "spacing",
    "the brush spacing: (% of MAX [width, height])"
  }
};

ProcRecord brushes_get_brush_proc =
{
  "gimp_brushes_get_brush",
  "Retrieve information about the currently active brush mask",
  "This procedure retrieves information about the currently active brush mask.  This includes the brush name, the width and height, and the brush spacing paramter.  All paint operations and stroke operations use this mask to control the application of paint to the image.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  4,
  brushes_get_brush_out_args,

  /*  Exec method  */
  { { brushes_get_brush_invoker } },
};

/* =========================================================== */
/* REFRESH BRUSHES                                             */
/* =========================================================== */

ProcRecord brushes_refresh_brush_proc =
{
  "gimp_brushes_refresh",
  "Refresh current brushes",
  "This procedure retrieves all brushes currently in the user's brush path "
  "and updates the brush dialog accordingly.",
  "Seth Burgess<sjburges@ou.edu>",
  "Seth Burgess",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { brushes_refresh_brush_invoker } },
};

/***********************/
/*  BRUSHES_SET_BRUSH  */

static Argument *
brushes_set_brush_invoker (Argument *args)
{
  GBrushP brushp;
  GSList *list;
  char *name;

  success = (name = (char *) args[0].value.pdb_pointer) != NULL;

  if (success)
    {
      list = brush_list;
      success = FALSE;

      while (list)
	{
	  brushp = (GBrushP) list->data;

	  if (!strcmp (brushp->name, name))
	    {
	      success = TRUE;
	      select_brush (brushp);
	      break;
	    }

	  list = g_slist_next (list);
	}
    }

  return procedural_db_return_args (&brushes_set_brush_proc, success);
}

/*  The procedure definition  */
ProcArg brushes_set_brush_args[] =
{
  { PDB_STRING,
    "name",
    "the brush name"
  }
};

ProcRecord brushes_set_brush_proc =
{
  "gimp_brushes_set_brush",
  "Set the specified brush as the active brush",
  "This procedure allows the active brush mask to be set by specifying its name.  The name is simply a string which corresponds to one of the names of the installed brushes.  If there is no matching brush found, this procedure will return an error.  Otherwise, the specified brush becomes active and will be used in all subsequent paint operations.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  brushes_set_brush_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { brushes_set_brush_invoker } },
};


/*************************/
/*  BRUSHES_GET_OPACITY  */

static Argument *
brushes_get_opacity_invoker (Argument *args)
{
  return_args = procedural_db_return_args (&brushes_get_opacity_proc, TRUE);
  return_args[1].value.pdb_float = get_brush_opacity () * 100.0;

  return return_args;
}

/*  The procedure definition  */
ProcArg brushes_get_opacity_out_args[] =
{
  { PDB_FLOAT,
    "opacity",
    "the brush opacity: 0 <= opacity <= 100"
  }
};

ProcRecord brushes_get_opacity_proc =
{
  "gimp_brushes_get_opacity",
  "Get the brush opacity",
  "This procedure returns the opacity setting for brushes.  This value is set globally and will remain the same even if the brush mask is changed.  The return value is a floating point number between 0 and 100.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  1,
  brushes_get_opacity_out_args,

  /*  Exec method  */
  { { brushes_get_opacity_invoker } },
};


/*************************/
/*  BRUSHES_SET_OPACITY  */

static Argument *
brushes_set_opacity_invoker (Argument *args)
{
  double opacity;

  opacity = args[0].value.pdb_float;
  success = (opacity >= 0.0 && opacity <= 100.0);

  if (success)
    set_brush_opacity (opacity / 100.0);

  return procedural_db_return_args (&brushes_set_opacity_proc, success);
}

/*  The procedure definition  */
ProcArg brushes_set_opacity_args[] =
{
  { PDB_FLOAT,
    "opacity",
    "the brush opacity: 0 <= opacity <= 100"
  }
};

ProcRecord brushes_set_opacity_proc =
{
  "gimp_brushes_set_opacity",
  "Set the brush opacity",
  "This procedure modifies the opacity setting for brushes.  This value is set globally and will remain the same even if the brush mask is changed.  The value should be a floating point number between 0 and 100.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  brushes_set_opacity_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { brushes_set_opacity_invoker } },
};


/*************************/
/*  BRUSHES_GET_SPACING  */

static Argument *
brushes_get_spacing_invoker (Argument *args)
{
  return_args = procedural_db_return_args (&brushes_get_spacing_proc, TRUE);
  return_args[1].value.pdb_int = get_brush_spacing ();

  return return_args;
}

/*  The procedure definition  */
ProcArg brushes_get_spacing_out_args[] =
{
  { PDB_INT32,
    "spacing",
    "the brush spacing: 0 <= spacing <= 1000"
  }
};

ProcRecord brushes_get_spacing_proc =
{
  "gimp_brushes_get_spacing",
  "Get the brush spacing",
  "This procedure returns the spacing setting for brushes.  This value is set per brush and will change if a different brush is selected.  The return value is an integer between 0 and 1000 which represents percentage of the maximum of the width and height of the mask.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  1,
  brushes_get_spacing_out_args,

  /*  Exec method  */
  { { brushes_get_spacing_invoker } },
};


/*************************/
/*  BRUSHES_SET_SPACING  */

static Argument *
brushes_set_spacing_invoker (Argument *args)
{
  int spacing;

  spacing = args[0].value.pdb_int;
  success = (spacing >= 0 && spacing <= 1000);

  if (success)
    set_brush_spacing (spacing);

  return procedural_db_return_args (&brushes_set_spacing_proc, success);
}

/*  The procedure definition  */
ProcArg brushes_set_spacing_args[] =
{
  { PDB_INT32,
    "spacing",
    "the brush spacing: 0 <= spacing <= 1000"
  }
};

ProcRecord brushes_set_spacing_proc =
{
  "gimp_brushes_set_spacing",
  "Set the brush spacing",
  "This procedure modifies the spacing setting for the current brush.  This value is set on a per-brush basis and will change if a different brush mask is selected.  The value should be a integer between 0 and 1000.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  brushes_set_spacing_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { brushes_set_spacing_invoker } },
};


/****************************/
/*  BRUSHES_GET_PAINT_MODE  */

static Argument *
brushes_get_paint_mode_invoker (Argument *args)
{
  return_args = procedural_db_return_args (&brushes_get_paint_mode_proc, TRUE);
  return_args[1].value.pdb_int = get_brush_paint_mode ();

  return return_args;
}

/*  The procedure definition  */
ProcArg brushes_get_paint_mode_out_args[] =
{
  { PDB_INT32,
    "paint_mode",
    "the paint mode: { NORMAL (0), DISSOLVE (1), BEHIND (2), MULTIPLY (3), SCREEN (4), OVERLAY (5) DIFFERENCE (6), ADDITION (7), SUBTRACT (8), DARKEN-ONLY (9), LIGHTEN-ONLY (10), HUE (11), SATURATION (12), COLOR (13), VALUE (14) }"
  }
};

ProcRecord brushes_get_paint_mode_proc =
{
  "gimp_brushes_get_paint_mode",
  "Get the brush paint_mode",
  "This procedure returns the paint-mode setting for brushes.  This value is set globally and will not change if a different brush is selected.  The return value is an integer between 0 and 13 which corresponds to the values listed in the argument description.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  1,
  brushes_get_paint_mode_out_args,

  /*  Exec method  */
  { { brushes_get_paint_mode_invoker } },
};


/****************************/
/*  BRUSHES_SET_PAINT_MODE  */

static Argument *
brushes_set_paint_mode_invoker (Argument *args)
{
  int paint_mode;

  paint_mode = args[0].value.pdb_int;
  success = (paint_mode >= NORMAL_MODE && paint_mode <= VALUE_MODE);

  if (success)
    set_brush_paint_mode (paint_mode);

  return procedural_db_return_args (&brushes_set_paint_mode_proc, success);
}

/*  The procedure definition  */
ProcArg brushes_set_paint_mode_args[] =
{
  { PDB_INT32,
    "paint_mode",
    "the paint mode: { NORMAL (0), DISSOLVE (1), BEHIND (2), MULTIPLY (3), SCREEN (4), OVERLAY (5) DIFFERENCE (6), ADDITION (7), SUBTRACT (8), DARKEN-ONLY (9), LIGHTEN-ONLY (10), HUE (11), SATURATION (12), COLOR (13), VALUE (14) }"
  }
};

ProcRecord brushes_set_paint_mode_proc =
{
  "gimp_brushes_set_paint_mode",
  "Set the brush paint_mode",
  "This procedure modifies the paint_mode setting for the current brush.  This value is set globally and will not change if a different brush mask is selected.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  brushes_set_paint_mode_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { brushes_set_paint_mode_invoker } },
};


/******************/
/*  BRUSHES_LIST  */

static Argument *
brushes_list_invoker (Argument *args)
{
  GBrushP brushp;
  GSList *list;
  char **brushes;
  int i;

  brushes = (char **) g_malloc (sizeof (char *) * num_brushes);

  success = (list = brush_list) != NULL;
  i = 0;

  while (list)
    {
      brushp = (GBrushP) list->data;

      brushes[i++] = g_strdup (brushp->name);
      list = g_slist_next (list);
    }

  return_args = procedural_db_return_args (&brushes_list_proc, success);

  if (success)
    {
      return_args[1].value.pdb_int = num_brushes;
      return_args[2].value.pdb_pointer = brushes;
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg brushes_list_out_args[] =
{
  { PDB_INT32,
    "num_brushes",
    "the number of brushes in the brush list"
  },
  { PDB_STRINGARRAY,
    "brush_list",
    "the list of brush names"
  }
};

ProcRecord brushes_list_proc =
{
  "gimp_brushes_list",
  "Retrieve a complete listing of the available brushes",
  "This procedure returns a complete listing of available GIMP brushes.  Each name returned can be used as input to the 'gimp_brushes_set_brush'",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  2,
  brushes_list_out_args,

  /*  Exec method  */
  { { brushes_list_invoker } },
};
