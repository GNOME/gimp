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
#include <math.h>
#include "appenv.h"
#include "gimpbrushgenerated.h"
#include "brush_header.h"
#include "brush_select.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "datafiles.h"
#include "devices.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "gimpsignal.h"
#include "menus.h"
#include "paint_core.h"
#include "gimplist.h"
#include "gimpbrush.h"
#include "gimplistP.h"
#include "gimpbrushlistP.h"


/*  global variables  */
GimpBrush     *active_brush = NULL;
GimpBrushList *brush_list   = NULL;

double              opacity = 1.0;
int                 paint_mode = 0;


BrushSelectP        brush_select_dialog = NULL;


/*  static variables  */
static int          success;
static Argument    *return_args;
static int          have_default_brush = 0;

/*  static function prototypes  */
static void   create_default_brush   (void);
static gint   brush_compare_func     (gconstpointer, gconstpointer);

static void brush_load                    (char *filename);

/* class functions */
static GimpObjectClass* parent_class;

static void
gimp_brush_list_add_func(GimpList* list, gpointer val)
{
  list->list=g_slist_insert_sorted (list->list, val, brush_compare_func);
  GIMP_BRUSH_LIST(list)->num_brushes++;
}

static void
gimp_brush_list_remove_func(GimpList* list, gpointer val)
{
  list->list=g_slist_remove(list->list, val);
  GIMP_BRUSH_LIST(list)->num_brushes--;
}

static void
gimp_brush_list_class_init (GimpBrushListClass *klass)
{
  GimpListClass *gimp_list_class;
  
  gimp_list_class = GIMP_LIST_CLASS(klass);
  gimp_list_class->add = gimp_brush_list_add_func;
  gimp_list_class->remove = gimp_brush_list_remove_func;
  parent_class = gtk_type_class (gimp_list_get_type ());
}

void
gimp_brush_list_init(GimpBrushList *list)
{
  list->num_brushes = 0;
}

GtkType gimp_brush_list_get_type(void)
{
  static GtkType type;
  if(!type)
  {
    GtkTypeInfo info={
      "GimpBrushList",
      sizeof(GimpBrushList),
      sizeof(GimpBrushListClass),
      (GtkClassInitFunc)gimp_brush_list_class_init,
      (GtkObjectInitFunc)gimp_brush_list_init,
      NULL,
      NULL };
    type=gtk_type_unique(gimp_list_get_type(), &info);
  }
  return type;
}

GimpBrushList *
gimp_brush_list_new ()
{
  GimpBrushList *list=GIMP_BRUSH_LIST(gtk_type_new(gimp_brush_list_get_type()));
  GIMP_LIST(list)->type = GIMP_TYPE_BRUSH;
  GIMP_LIST(list)->weak = 0;
  
  return list;
}


/*  function declarations  */
void
brushes_init (int no_data)
{

  if (brush_list)
    brushes_free();
  else
    brush_list = gimp_brush_list_new();

  if (brush_path == NULL || (no_data))
    create_default_brush ();
  else
    datafiles_read_directories (brush_path,(datafile_loader_t)brush_load, 0);
}

static void
brush_load(char *filename)
{
  if (strcmp(&filename[strlen(filename) - 4], ".gbr") == 0)
  {
    gimp_brush_list_add(brush_list, gimp_brush_new(filename));
  }
  else if (strcmp(&filename[strlen(filename) - 4], ".vbr") == 0)
  {
    gimp_brush_list_add(brush_list,
			GIMP_BRUSH(gimp_brush_generated_load(filename)));
  }
}

static gint
brush_compare_func (gconstpointer first, gconstpointer second)
{
  return strcmp (((const GimpBrush *)first)->name, 
		 ((const GimpBrush *)second)->name);
}


void
brushes_free ()
{
  
  if (brush_list)
  {
    while (GIMP_LIST(brush_list)->list)
      gimp_brush_list_remove(brush_list, GIMP_LIST(brush_list)->list->data);
  }

  have_default_brush = 0;
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


GimpBrush *
get_active_brush ()
{
  if (have_default_brush)
    {
      have_default_brush = 0;
      if (!active_brush)
	fatal_error ("Specified default brush not found!");
    }
  else if (! active_brush && brush_list)
    /* need a gimp_list_get_first() type function */
    select_brush((GimpBrush *) GIMP_LIST(brush_list)->list->data);

  return active_brush;
}

#if 0
static GSList *
insert_brush_in_list (GSList *list, GimpBrush * brush)
{
  return g_slist_insert_sorted (list, brush, brush_compare_func);
}
#endif

static void
create_default_brush ()
{
  GimpBrushGenerated *brush;
  
  brush = gimp_brush_generated_new(5.0, .5, 0.0, 1.0);

  /*  Swap the brush to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    temp_buf_swap (GIMP_BRUSH(brush)->mask);

  /*  Make this the default, active brush  */
  select_brush(GIMP_BRUSH(brush));
  have_default_brush = 1;
}



int
gimp_brush_list_get_brush_index (GimpBrushList *brush_list,
				 GimpBrush *brush)
{
  /* fix me: make a gimp_list function that does this? */
  return g_slist_index (GIMP_LIST(brush_list)->list, brush);
}

GimpBrush *
gimp_brush_list_get_brush_by_index (GimpBrushList *brush_list, int index)
{
  GSList *list;
  GimpBrush * brush = NULL;
  /* fix me: make a gimp_list function that does this? */
  list = g_slist_nth (GIMP_LIST(brush_list)->list, index);
  if (list)
    brush = (GimpBrush *) list->data;

  return brush;
}

static void
gimp_brush_list_uniquefy_brush_name(GimpBrushList *brush_list,
				    GimpBrush *brush)
{
  GSList *list, *listb;
  GimpBrush *brushb;
  int number = 1;
  char *newname;
  char *oldname;
  char *ext;
  g_return_if_fail(GIMP_IS_BRUSH_LIST(brush_list));
  g_return_if_fail(GIMP_IS_BRUSH(brush));
  list = GIMP_LIST(brush_list)->list;
  while (list)
  {
    brushb = GIMP_BRUSH(list->data);
    if (brush != brushb &&
	strcmp(gimp_brush_get_name(brush), gimp_brush_get_name(brushb)) == 0)
    { /* names conflict */
      oldname = gimp_brush_get_name(brush);
      newname = g_malloc(strlen(oldname)+10); /* if this aint enough 
						 yer screwed */
      strcpy (newname, oldname);
      if ((ext = strrchr(newname, '#')))
      {
	number = atoi(ext+1);
	if (&ext[(int)(log10(number) + 1)] != &newname[strlen(newname) - 1])
	{
	  number = 1;
	  ext = &newname[strlen(newname)];
	}
      }
      else
      {
	number = 1;
	ext = &newname[strlen(newname)];
      }
      sprintf(ext, "#%d", number+1);
      listb = GIMP_LIST(brush_list)->list;
      while (listb) /* make sure the new name is unique */
      {
	brushb = GIMP_BRUSH(listb->data);
	if (brush != brushb && strcmp(newname,
				      gimp_brush_get_name(brushb)) == 0)
	{
	  number++;
	  sprintf(ext, "#%d", number+1);
	  listb = GIMP_LIST(brush_list)->list;
	}
	listb = listb->next;
      }
      gimp_brush_set_name(brush, newname);
      g_free(newname);
      if (gimp_list_have(GIMP_LIST(brush_list), brush))
      { /* ought to have a better way than this to resort the brush */
	gtk_object_ref(GTK_OBJECT(brush));
	gimp_brush_list_remove(brush_list, brush);
	gimp_brush_list_add(brush_list, brush);
	gtk_object_unref(GTK_OBJECT(brush));
      }
      return;
    }
    list = list->next;
  }
}

static void brush_renamed(GimpBrush *brush, GimpBrushList *brush_list)
{
  gimp_brush_list_uniquefy_brush_name(brush_list, brush);
}

void
gimp_brush_list_add (GimpBrushList *brush_list, GimpBrush * brush)
{
  gimp_brush_list_uniquefy_brush_name(brush_list, brush);
  gimp_list_add(GIMP_LIST(brush_list), brush);
  gtk_object_sink(GTK_OBJECT(brush));
  gtk_signal_connect(GTK_OBJECT(brush), "rename",
		     (GtkSignalFunc)brush_renamed, brush_list);
}

void
gimp_brush_list_remove (GimpBrushList *brush_list, GimpBrush * brush)
{
  gtk_signal_disconnect_by_data(GTK_OBJECT(brush), brush_list);
  gimp_list_remove(GIMP_LIST(brush_list), brush);
}

int
gimp_brush_list_length (GimpBrushList *brush_list)
{
  g_return_val_if_fail(GIMP_IS_BRUSH_LIST(brush_list), 0);
  return (brush_list->num_brushes);
}

GimpBrush *
gimp_brush_list_get_brush(GimpBrushList *blist, char *name)
{
  GimpBrush *brushp;
  GSList *list;
  if (blist == NULL)
    return NULL;

  list = GIMP_LIST(brush_list)->list;

  while (list)
  {
    brushp = (GimpBrush *) list->data;

    if (!strcmp (brushp->name, name))
    {
      return brushp;
    }

    list = g_slist_next (list);
  }
  return NULL;
}

void
select_brush (GimpBrush * brush)
{
  /*  Make sure the active brush is swapped before we get a new one... */
  if (stingy_memory_use && active_brush && active_brush->mask)
    temp_buf_swap (active_brush->mask);

  if (active_brush)
    gtk_object_unref(GTK_OBJECT(active_brush));

  /*  Set the active brush  */
  active_brush = brush;
  
  gtk_object_ref(GTK_OBJECT(active_brush));

  /*  Make sure the active brush is unswapped... */
  if (stingy_memory_use)
    temp_buf_unswap (brush->mask);

  /*  Keep up appearances in the brush dialog  */
  if (brush_select_dialog)
    brush_select_select (brush_select_dialog,
			 gimp_brush_list_get_brush_index(brush_list, brush));

  device_status_update (current_device);
}


void
create_brush_dialog ()
{
  if (!brush_select_dialog)
    {
      /*  Create the dialog...  */
      brush_select_dialog = brush_select_new (NULL,NULL,0.0,0,0);
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



/***********************/
/*  BRUSHES_GET_BRUSH  */

static Argument *
brushes_get_brush_invoker (Argument *args)
{
  GimpBrush * brushp;

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

/* access functions */
double
gimp_brush_get_opacity ()
{
  return opacity;
}

int
gimp_brush_get_spacing ()
{
  if (active_brush)
    return active_brush->spacing;
  else
    return 0;
}

int
gimp_brush_get_paint_mode ()
{
  return paint_mode;
}

void
gimp_brush_set_opacity (opac)
     double opac;
{
  opacity = opac;
}

void
gimp_brush_set_spacing (spac)
     int spac;
{
  if (active_brush)
    active_brush->spacing = spac;
}

void
gimp_brush_set_paint_mode (pm)
     int pm;
{
  paint_mode = pm;
}

/****************************/
/* PDB Interface To Brushes */
/****************************/

/*************************/
/*  BRUSHES_GET_OPACITY  */

static Argument *
brushes_get_opacity_invoker (Argument *args)
{
  return_args = procedural_db_return_args (&brushes_get_opacity_proc, TRUE);
  return_args[1].value.pdb_float = gimp_brush_get_opacity () * 100.0;

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
    gimp_brush_set_opacity (opacity / 100.0);

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
  return_args[1].value.pdb_int = gimp_brush_get_spacing ();

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
    gimp_brush_set_spacing (spacing);

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
  return_args[1].value.pdb_int = gimp_brush_get_paint_mode ();

  return return_args;
}

/*  The procedure definition  */
ProcArg brushes_get_paint_mode_out_args[] =
{
  { PDB_INT32,
    "paint_mode",
    "the paint mode: { NORMAL (0), DISSOLVE (1), BEHIND (2), MULTIPLY/BURN (3), SCREEN (4), OVERLAY (5) DIFFERENCE (6), ADDITION (7), SUBTRACT (8), DARKEN-ONLY (9), LIGHTEN-ONLY (10), HUE (11), SATURATION (12), COLOR (13), VALUE (14), DIVIDE/DODGE (15) }"
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
    gimp_brush_set_paint_mode (paint_mode);

  return procedural_db_return_args (&brushes_set_paint_mode_proc, success);
}

/*  The procedure definition  */
ProcArg brushes_set_paint_mode_args[] =
{
  { PDB_INT32,
    "paint_mode",
    "the paint mode: { NORMAL (0), DISSOLVE (1), BEHIND (2), MULTIPLY/BURN (3), SCREEN (4), OVERLAY (5) DIFFERENCE (6), ADDITION (7), SUBTRACT (8), DARKEN-ONLY (9), LIGHTEN-ONLY (10), HUE (11), SATURATION (12), COLOR (13), VALUE (14), DIVIDE/DODGE (15) }"
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



/***********************/
/*  BRUSHES_SET_BRUSH  */


static Argument *
brushes_set_brush_invoker (Argument *args)
{
  GimpBrush * brushp;
  char *name;

  success = (name = (char *) args[0].value.pdb_pointer) != NULL;

  if (success)
  {
    brushp = gimp_brush_list_get_brush(brush_list, name);
    if (brushp)
      select_brush(brushp);
    else
      success = 0;
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




/******************/
/*  BRUSHES_LIST  */

static Argument *
brushes_list_invoker (Argument *args)
{
  GimpBrush * brushp;
  GSList *list;
  char **brushes;
  int i;

  brushes = (char **) g_malloc (sizeof (char *) * brush_list->num_brushes);

  success = (list = GIMP_LIST(brush_list)->list) != NULL;
  i = 0;

  while (list)
    {
      brushp = (GimpBrush *) list->data;

      brushes[i++] = g_strdup (brushp->name);
      list = g_slist_next (list);
    }

  return_args = procedural_db_return_args (&brushes_list_proc, success);

  if (success)
    {
      return_args[1].value.pdb_int = brush_list->num_brushes;
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
