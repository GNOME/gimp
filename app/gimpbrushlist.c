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
#include "gimpbrushgenerated.h"
#include "gimpbrushpipe.h"
#include "brush_header.h"
#include "brush_select.h"
#include "datafiles.h"
#include "gimprc.h"
#include "gimpsignal.h"
#include "gimplist.h"
#include "gimpbrush.h"
#include "gimplistP.h"
#include "gimpbrushlistP.h"
#include "general.h"

#include "libgimp/gimpenv.h"

#include "libgimp/gimpintl.h"

/*  global variables  */
GimpBrushList    *brush_list     = NULL;


/*  local function prototypes  */
static void   brushes_brush_load (gchar         *filename);

static gint   brush_compare_func (gconstpointer  first,
				  gconstpointer  second);

/*  class functions  */
static GimpObjectClass* parent_class;

static void
gimp_brush_list_add_func (GimpList *list,
			  gpointer  val)
{
  list->list=g_slist_insert_sorted (list->list, val, brush_compare_func);
  GIMP_BRUSH_LIST (list)->num_brushes++;
}

static void
gimp_brush_list_remove_func (GimpList *list,
			     gpointer  val)
{
  list->list = g_slist_remove (list->list, val);

  GIMP_BRUSH_LIST (list)->num_brushes--;
}

static void
gimp_brush_list_class_init (GimpBrushListClass *klass)
{
  GimpListClass *gimp_list_class;
  
  gimp_list_class = GIMP_LIST_CLASS(klass);
  gimp_list_class->add    = gimp_brush_list_add_func;
  gimp_list_class->remove = gimp_brush_list_remove_func;

  parent_class = gtk_type_class (gimp_list_get_type ());
}

void
gimp_brush_list_init (GimpBrushList *list)
{
  list->num_brushes = 0;
}

GtkType
gimp_brush_list_get_type (void)
{
  static GtkType type = 0;

  if (!type)
    {
      GtkTypeInfo info =
      {
	"GimpBrushList",
	sizeof (GimpBrushList),
	sizeof (GimpBrushListClass),
	(GtkClassInitFunc) gimp_brush_list_class_init,
	(GtkObjectInitFunc) gimp_brush_list_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL
      };

      type = gtk_type_unique (gimp_list_get_type (), &info);
    }

  return type;
}

GimpBrushList *
gimp_brush_list_new (void)
{
  GimpBrushList *list;

  list = GIMP_BRUSH_LIST (gtk_type_new (gimp_brush_list_get_type ()));

  GIMP_LIST (list)->type = GIMP_TYPE_BRUSH;
  GIMP_LIST (list)->weak = FALSE;
  
  return list;
}

/*  function declarations  */
void
brushes_init (gint no_data)
{
  if (brush_list)
    brushes_free ();
  else
    brush_list = gimp_brush_list_new ();

  if (brush_path != NULL && !no_data)
    {
      brush_select_freeze_all ();

      datafiles_read_directories (brush_path, brushes_brush_load, 0);
      datafiles_read_directories (brush_vbr_path, brushes_brush_load, 0);

      brush_select_thaw_all ();
    }

  gimp_context_refresh_brushes ();
}

GimpBrush *
brushes_get_standard_brush (void)
{
  static GimpBrush *standard_brush = NULL;

  if (! standard_brush)
    {
      standard_brush =
	GIMP_BRUSH (gimp_brush_generated_new (5.0, 0.5, 0.0, 1.0));

      gimp_brush_set_name (standard_brush, "Standard");

      /*  set ref_count to 2 --> never swap the standard brush  */
      gtk_object_ref (GTK_OBJECT (standard_brush));
      gtk_object_ref (GTK_OBJECT (standard_brush));
    }

  return standard_brush;
}

static void
brushes_brush_load (gchar *filename)
{
  if (strcmp (&filename[strlen (filename) - 4], ".gbr") == 0)
    {
      GimpBrush *brush;
      brush = gimp_brush_new (filename);
      if (brush != NULL)
	gimp_brush_list_add (brush_list, brush);
      else
	g_message (_("Warning: Failed to load brush\n\"%s\""), filename);
    }
  else if (strcmp (&filename[strlen(filename) - 4], ".vbr") == 0)
    {
      GimpBrushGenerated *brush;
      brush = gimp_brush_generated_load (filename);
      if (brush != NULL)
	gimp_brush_list_add (brush_list, GIMP_BRUSH (brush));
      else
	g_message (_("Warning: Failed to load brush\n\"%s\""), filename);
    }
  else if (strcmp (&filename[strlen (filename) - 4], ".gpb") == 0)
    {
      GimpBrushPipe *brush;
      brush = gimp_brush_pixmap_load (filename);
      if (brush != NULL)
	gimp_brush_list_add (brush_list, GIMP_BRUSH (brush));
      else
	g_message (_("Warning: Failed to load pixmap brush\n\"%s\""), filename);
    }
  else if (strcmp (&filename[strlen (filename) - 4], ".gih") == 0)
    {
      GimpBrushPipe *brush;
      brush = gimp_brush_pipe_load (filename);
      if (brush != NULL)
	gimp_brush_list_add (brush_list, GIMP_BRUSH (brush));
      else
	g_message (_("Warning: Failed to load pixmap pipe\n\"%s\""), filename);
    }
}

static gint
brush_compare_func (gconstpointer first,
		    gconstpointer second)
{
  return strcmp (((const GimpBrush *) first)->name, 
		 ((const GimpBrush *) second)->name);
}

void
brushes_free (void)
{
  GList *vbr_path;
  gchar *vbr_dir;

  if (!brush_list)
    return;

  vbr_path = gimp_path_parse (brush_vbr_path, 16, TRUE, NULL);
  vbr_dir  = gimp_path_get_user_writable_dir (vbr_path);
  gimp_path_free (vbr_path);

  brush_select_freeze_all ();

  while (GIMP_LIST (brush_list)->list)
    {
      GimpBrush *brush = GIMP_BRUSH (GIMP_LIST (brush_list)->list->data);

      if (GIMP_IS_BRUSH_GENERATED (brush) && vbr_dir)
	{
	  gchar *filename = g_strdup (brush->filename);

	  if (!filename)
	    {
	      FILE *tmp_fp;
	      gint  unum = 0;

	      filename = g_strconcat (vbr_dir,
				      brush->name, ".vbr",
				      NULL);

	      /* make sure we don't overwrite an existing brush */
	      while ((tmp_fp = fopen (filename, "r")))
		{
		  fclose (tmp_fp);
		  g_free (filename);
		  filename = g_strdup_printf ("%s%s_%d.vbr",
					      vbr_dir,
					      brush->name,
					      unum);
		  unum++;
		}
	    }
	  else
	    {
	      if (strcmp (&filename[strlen (filename) - 4], ".vbr"))
		{
		  g_free (filename);
		  filename = NULL;
		}
	    }

	  /*  okay we are ready to try to save the generated file  */
	  if (filename)
	    {
	      gimp_brush_generated_save (GIMP_BRUSH_GENERATED (brush),
					 filename);
	      g_free (filename);
	    }
	}

      gimp_brush_list_remove (brush_list, brush);
    }

  g_free (vbr_dir);

  brush_select_thaw_all ();
}

#if 0
static GSList *
insert_brush_in_list (GSList    *list,
		      GimpBrush *brush)
{
  return g_slist_insert_sorted (list, brush, brush_compare_func);
}
#endif

gint
gimp_brush_list_get_brush_index (GimpBrushList *brush_list,
				 GimpBrush     *brush)
{
  /* fix me: make a gimp_list function that does this? */
  return g_slist_index (GIMP_LIST(brush_list)->list, brush);
}

GimpBrush *
gimp_brush_list_get_brush_by_index (GimpBrushList *brush_list,
				    gint           index)
{
  GimpBrush *brush = NULL;
  GSList *list;

  /* fix me: make a gimp_list function that does this? */
  list = g_slist_nth (GIMP_LIST (brush_list)->list, index);
  if (list)
    brush = (GimpBrush *) list->data;

  return brush;
}

static void
gimp_brush_list_uniquefy_brush_name (GimpBrushList *brush_list,
				     GimpBrush     *brush)
{
  GSList *list, *listb;
  GimpBrush *brushb;
  int number = 1;
  char *newname;
  char *oldname;
  char *ext;
  g_return_if_fail (GIMP_IS_BRUSH_LIST (brush_list));
  g_return_if_fail (GIMP_IS_BRUSH (brush));
  list = GIMP_LIST (brush_list)->list;
  while (list)
    {
      brushb = GIMP_BRUSH (list->data);
      if (brush != brushb &&
	  strcmp (gimp_brush_get_name (brush),
		  gimp_brush_get_name (brushb)) == 0)
	{ /* names conflict */
	  oldname = gimp_brush_get_name (brush);
	  newname = g_malloc (strlen (oldname) + 10); /* if this aint enough 
							 yer screwed */
	  strcpy (newname, oldname);
	  if ((ext = strrchr(newname, '#')))
	    {
	      number = atoi (ext+1);
	      if (&ext[(int)(log10 (number) + 1)] !=
		  &newname[strlen (newname) - 1])
		{
		  number = 1;
		  ext = &newname[strlen (newname)];
		}
	    }
	  else
	    {
	      number = 1;
	      ext = &newname[strlen (newname)];
	    }
	  sprintf (ext, "#%d", number+1);
	  listb = GIMP_LIST (brush_list)->list;
	  while (listb) /* make sure the new name is unique */
	    {
	      brushb = GIMP_BRUSH (listb->data);
	      if (brush != brushb &&
		  strcmp (newname, gimp_brush_get_name (brushb)) == 0)
		{
		  number++;
		  sprintf (ext, "#%d", number+1);
		  listb = GIMP_LIST (brush_list)->list;
		}
	      listb = listb->next;
	    }
	  gimp_brush_set_name (brush, newname);
	  g_free (newname);
	  if (gimp_list_have (GIMP_LIST (brush_list), brush))
	    { /* ought to have a better way than this to resort the brush */
	      gtk_object_ref (GTK_OBJECT (brush));
	      gimp_brush_list_remove (brush_list, brush);
	      gimp_brush_list_add (brush_list, brush);
	      gtk_object_unref (GTK_OBJECT (brush));
	    }
	  return;
	}
      list = list->next;
    }
}

static void
brush_renamed (GimpBrush     *brush,
	       GimpBrushList *brush_list)
{
  gimp_brush_list_uniquefy_brush_name (brush_list, brush);
}

void
gimp_brush_list_add (GimpBrushList *brush_list,
		     GimpBrush     *brush)
{
  gimp_brush_list_uniquefy_brush_name (brush_list, brush);
  gimp_list_add (GIMP_LIST (brush_list), brush);
  gtk_object_unref (GTK_OBJECT (brush));
  gtk_signal_connect (GTK_OBJECT (brush), "rename",
		      GTK_SIGNAL_FUNC (brush_renamed),
		      brush_list);
}

void
gimp_brush_list_remove (GimpBrushList *brush_list,
			GimpBrush     *brush)
{
  gtk_signal_disconnect_by_data (GTK_OBJECT (brush), brush_list);

  gimp_list_remove (GIMP_LIST (brush_list), brush);
}

gint
gimp_brush_list_length (GimpBrushList *brush_list)
{
  g_return_val_if_fail (GIMP_IS_BRUSH_LIST (brush_list), 0);

  return brush_list->num_brushes;
}

GimpBrush *
gimp_brush_list_get_brush (GimpBrushList *blist,
			   gchar         *name)
{
  GimpBrush *brushp;
  GSList *list;

  if (blist == NULL)
    return NULL;

  for (list = GIMP_LIST (brush_list)->list; list; list = g_slist_next (list))
    {
      brushp = (GimpBrush *) list->data;

      if (!strcmp (brushp->name, name))
	return brushp;
    }

  return NULL;
}
