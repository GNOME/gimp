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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#include <gtk/gtk.h>

#include "apptypes.h"

#include "datafiles.h"
#include "gimpcontext.h"
#include "gimprc.h"
#include "patterns.h"
#include "pattern_header.h"
#include "temp_buf.h"

#include "libgimp/gimpintl.h"


/*  global variables  */
GPattern *active_pattern = NULL;
GSList   *pattern_list   = NULL;
gint      num_patterns   = 0;


/*  static variables  */
static GPattern *standard_pattern = NULL;


/*  local function prototypes  */
static GPattern *  pattern_load_real      (gint           fd,
					   gchar         *filename,
					   gboolean       quiet);
static void        load_pattern           (gchar         *filename);
static void        pattern_free_func      (gpointer       data, 
					   gpointer       dummy);
static gint        pattern_compare_func   (gconstpointer  second, 
					   gconstpointer  first);

/*  public functions  */

void
patterns_init (gboolean no_data)
{
  GSList *list;

  if (pattern_list)
    patterns_free ();

  if (pattern_path != NULL && !no_data)
    datafiles_read_directories (pattern_path, load_pattern, 0);

  /*  assign indexes to the loaded patterns  */
  for (list = pattern_list; list; list = g_slist_next (list))
    {
      /*  Set the pattern index  */
      ((GPattern *) list->data)->index = num_patterns++;
    }

  gimp_context_refresh_patterns ();
}

void
patterns_free (void)
{
  if (pattern_list)
    {
      g_slist_foreach (pattern_list, pattern_free_func, NULL);
      g_slist_free (pattern_list);
    }

  num_patterns = 0;
  pattern_list = NULL;
}

GPattern *
patterns_get_standard_pattern (void)
{
  if (! standard_pattern)
    {
      guchar *data;
      gint row, col;

      standard_pattern = g_new (GPattern, 1);

      standard_pattern->filename = NULL;
      standard_pattern->name     = g_strdup ("Standard");
      standard_pattern->index    = -1;  /*  not part of the pattern list  */
      standard_pattern->mask     = temp_buf_new (32, 32, 3, 0, 0, NULL);

      data = temp_buf_data (standard_pattern->mask);

      for (row = 0; row < standard_pattern->mask->height; row++)
	for (col = 0; col < standard_pattern->mask->width; col++)
	  {
	    memset (data, (col % 2) && (row % 2) ? 255 : 0, 3);
	    data += 3;
	  }
    }

  return standard_pattern;
}

GPattern *
pattern_list_get_pattern_by_index (GSList *list,
				   gint    index)
{
  GPattern *pattern = NULL;

  list = g_slist_nth (list, index);
  if (list)
    pattern = (GPattern *) list->data;

  return pattern;
}

GPattern *
pattern_list_get_pattern (GSList *list,
			  gchar  *name)
{
  GPattern *pattern;

  for (; list; list = g_slist_next (list))
    {
      pattern = (GPattern *) list->data;
      
      if (!strcmp (pattern->name, name))
	return pattern;
    }

  return NULL;
}

GPattern *
pattern_load (gint      fd,
	      gchar    *filename)
{
  return pattern_load_real (fd, filename, TRUE);
}

static GPattern *
pattern_load_real (gint      fd,
	           gchar    *filename,
	           gboolean  quiet)
{
  GPattern      *pattern;
  PatternHeader  header;
  gint    bn_size;
  gchar  *name;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (fd != -1, NULL);

  /*  Read in the header size  */
  if (read (fd, &header, sizeof (header)) != sizeof (header)) 
    return NULL;

  /*  rearrange the bytes in each unsigned int  */
  header.header_size  = g_ntohl (header.header_size);
  header.version      = g_ntohl (header.version);
  header.width        = g_ntohl (header.width);
  header.height       = g_ntohl (header.height);
  header.bytes        = g_ntohl (header.bytes);
  header.magic_number = g_ntohl (header.magic_number);

 /*  Check for correct file format */
  if (header.magic_number != GPATTERN_MAGIC || header.version != 1 || 
      header.header_size <= sizeof (header)) 
    {
      if (!quiet)
	g_message (_("Unknown pattern format version #%d in \"%s\"."),
		   header.version, filename);
      return NULL;
    }
  
  /*  Check for supported bit depths  */
  if (header.bytes != 1 && header.bytes != 3)
    {
      g_message ("Unsupported pattern depth: %d\n in file \"%s\"\nGIMP Patterns must be GRAY or RGB\n",
		 header.bytes, filename);
      return NULL;
    }

   /*  Read in the brush name  */
  if ((bn_size = (header.header_size - sizeof (header))))
    {
      name = g_new (gchar, bn_size);
      if ((read (fd, name, bn_size)) < bn_size)
	{
	  g_message (_("Error in GIMP pattern file \"%s\"."), filename);
	  g_free (name);
	  return NULL;
	}
    }
  else
    {
      name = g_strdup (_("Unnamed"));
    }

  pattern = g_new0 (GPattern, 1);
  pattern->mask = temp_buf_new (header.width, header.height, header.bytes,
				0, 0, NULL);
  if (read (fd, temp_buf_data (pattern->mask), 
	    header.width * header.height * header.bytes) < header.width * header.height * header.bytes)
      {
	g_message (_("GIMP pattern file appears to be truncated: \"%s\"."),
		   filename);
	pattern_free (pattern);
	return NULL;
      }

  pattern->name = name;

  return pattern;
}

void
pattern_free (GPattern *pattern)
{
  if (pattern->mask)
    temp_buf_free (pattern->mask);
  if (pattern->filename)
    g_free (pattern->filename);
  if (pattern->name)
    g_free (pattern->name);

  g_free (pattern);
}

/*  private functions  */

static void
pattern_free_func (gpointer data,
		   gpointer dummy)
{
  pattern_free ((GPattern *) data);
}

static gint
pattern_compare_func (gconstpointer first,
		      gconstpointer second)
{
  return strcmp (((const GPattern *) first)->name, 
		 ((const GPattern *) second)->name);
}

static void
load_pattern (gchar *filename)
{
  GPattern *pattern;
  gint      fd;
  GSList   *list;
  GSList   *list2;
  gint      unique_ext = 0;
  gchar    *ext;
  gchar    *new_name = NULL;

  g_return_if_fail (filename != NULL);

  fd = open (filename, O_RDONLY | _O_BINARY);
  if (fd == -1) 
    return;

  pattern = pattern_load_real (fd, filename, FALSE);

  close (fd);

  if (!pattern)
    return;

  pattern->filename = g_strdup (filename);

  /*  Swap the pattern to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    temp_buf_swap (pattern->mask);

  /* uniquefy pattern name */
  for (list = pattern_list; list; list = g_slist_next (list))
    {
      if (! strcmp (((GPattern *) list->data)->name, pattern->name))
	{
	  ext = strrchr (pattern->name, '#');

	  if (ext)
	    {
	      gchar *ext_str;

	      unique_ext = atoi (ext + 1);

	      ext_str = g_strdup_printf ("%d", unique_ext);

	      /*  check if the extension really is of the form "#<n>"  */
	      if (! strcmp (ext_str, ext + 1))
		{
		  *ext = '\0';
		}
	      else
		{
		  unique_ext = 0;
		}

	      g_free (ext_str);
	    }
	  else
	    {
	      unique_ext = 0;
	    }

	  do
	    {
	      unique_ext++;

	      g_free (new_name);

	      new_name = g_strdup_printf ("%s#%d", pattern->name, unique_ext);

	      for (list2 = pattern_list; list2; list2 = g_slist_next (list2))
		{
		  if (! strcmp (((GPattern *) list2->data)->name, new_name))
		    {
		      break;
		    }
		}
	    }
	  while (list2);

	  g_free (pattern->name);

	  pattern->name = new_name;

	  break;
	}
    }

  pattern_list = g_slist_insert_sorted (pattern_list, pattern,
					pattern_compare_func);
}
