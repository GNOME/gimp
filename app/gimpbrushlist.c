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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <math.h>

#include "appenv.h"
#include "gimpbrushpixmap.h"
#include "gimpbrushgenerated.h"
#include "brush_header.h"
#include "brush_select.h"
#include "colormaps.h"
#include "datafiles.h"
#include "devices.h"
#include "errors.h"
#include "gimprc.h"
#include "gimpsignal.h"
#include "menus.h"
#include "paint_core.h"
#include "paint_options.h"
#include "gimplist.h"
#include "gimpbrush.h"
#include "gimplistP.h"
#include "gimpbrushlistP.h"
#include "dialog_handler.h"

#include "libgimp/gimpintl.h"

/*  global variables  */
GimpBrush     *active_brush = NULL;
GimpBrushList *brush_list   = NULL;

BrushSelectP   brush_select_dialog = NULL;


/*  static variables  */
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
      GimpBrush *brush;
      brush = gimp_brush_new(filename);
      if (brush != NULL)
	gimp_brush_list_add(brush_list, brush);
      else
	g_message("Warning: failed to load brush \"%s\"", filename);
    }
  else if (strcmp(&filename[strlen(filename) - 4], ".vbr") == 0)
    {
      GimpBrushGenerated *brush;
      brush = gimp_brush_generated_load(filename);
      if (brush != NULL)
	gimp_brush_list_add(brush_list, GIMP_BRUSH(brush));
      else
	g_message("Warning: failed to load brush \"%s\"", filename);
    }
  else if (strcmp(&filename[strlen(filename) - 4], ".gpb") == 0)
    {
      GimpBrushPixmap *brush;
      brush = gimp_brush_pixmap_load(filename);
      if (brush != NULL)
	gimp_brush_list_add(brush_list, GIMP_BRUSH(brush));
      else
	g_message("Warning: failed to load brush \"%s\"", filename);
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
	gimp_fatal_error (_("get_active_brush(): Specified default brush not found!"));
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
create_brush_dialog (void)
{
  if (!brush_select_dialog)
    {
      /*  Create the dialog...  */
      brush_select_dialog = brush_select_new (NULL,NULL,0.0,0,0);
      
      /* register this one only */
      dialog_register(brush_select_dialog->shell);
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
