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
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#include "colormaps.h"
#include "datafiles.h"
#include "devices.h"
#include "patterns.h"
#include "pattern_header.h"
#include "pattern_select.h"
#include "colormaps.h"
#include "errors.h"
#include "gimprc.h"
#include "dialog_handler.h"

#include "libgimp/gimpintl.h"

/*  global variables  */
GPatternP       active_pattern = NULL;
GSList         *pattern_list = NULL;
gint            num_patterns = 0;

PatternSelectP  pattern_select_dialog = NULL;

/*  static variables  */
static gint     have_default_pattern = 0;

/*  static function prototypes  */
static GSList * insert_pattern_in_list   (GSList *, GPatternP);
static void     load_pattern             (gchar *);
static void     free_pattern             (GPatternP);
static void     pattern_free_one         (gpointer, gpointer);
static gint     pattern_compare_func     (gconstpointer, 
					  gconstpointer);

/*  function declarations  */
void
patterns_init (gboolean no_data)
{
  GSList *list;

  if (pattern_list)
    patterns_free ();

  pattern_list = NULL;
  num_patterns = 0;

  if (!pattern_path)
    return;
  if (!no_data)
    datafiles_read_directories (pattern_path, load_pattern, 0);

  /*  assign indexes to the loaded patterns  */

  list = pattern_list;

  while (list)
    {
      /*  Set the pattern index  */
      ((GPattern *) list->data)->index = num_patterns++;
      list = g_slist_next (list);
    }
}


static void
pattern_free_one (gpointer data,
		  gpointer dummy)
{
  free_pattern ((GPatternP) data);
}

static gint
pattern_compare_func (gconstpointer first,
		      gconstpointer second)
{
  return strcmp (((const GPatternP)first)->name, 
		 ((const GPatternP)second)->name);
}

void
patterns_free ()
{
  if (pattern_list)
    {
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
	gimp_fatal_error (_("get_active_pattern(): "
			    "Specified default pattern not found!"));

    }
  else if (! active_pattern && pattern_list)
    active_pattern = (GPatternP) pattern_list->data;

  return active_pattern;
}


static GSList *
insert_pattern_in_list (GSList    *list,
			GPatternP  pattern)
{
  return g_slist_insert_sorted (list, pattern, pattern_compare_func);
}


static void
load_pattern (gchar *filename)
{
  GPatternP pattern;
  FILE * fp;
  gint bn_size;
  guchar buf [sz_PatternHeader];
  PatternHeader header;
  guint * hp;
  gint i;

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
  if (header.version != GPATTERN_FILE_VERSION)
    {
      g_message (_("Unknown GIMP version #%d in \"%s\"\n"), header.version,
		 filename);
      fclose (fp);
      free_pattern (pattern);
      return;
    }

  /*  Get a new pattern mask  */
  pattern->mask =
    temp_buf_new (header.width, header.height, header.bytes, 0, 0, NULL);

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

  pattern_list = insert_pattern_in_list (pattern_list, pattern);

  /* Check if the current pattern is the default one */

  if (strcmp (default_pattern, g_basename (filename)) == 0)
    {
      active_pattern = pattern;
      have_default_pattern = 1;
    }
}

GPatternP           
pattern_list_get_pattern (GSList *list,
			  gchar  *name)
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
get_pattern_by_index (gint index)
{
  GSList *list;
  GPatternP pattern = NULL;

  list = g_slist_nth (pattern_list, index);
  if (list)
    pattern = (GPatternP) list->data;

  return pattern;
}

void
select_pattern (GPatternP pattern)
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
create_pattern_dialog (void)
{
  if (!pattern_select_dialog)
    {
      /*  Create the dialog...  */
      pattern_select_dialog = pattern_select_new (NULL,NULL);

      /* register this one only */
      dialog_register (pattern_select_dialog->shell);
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
free_pattern (GPatternP pattern)
{
  if (pattern->mask)
    temp_buf_free (pattern->mask);
  if (pattern->filename)
    g_free (pattern->filename);
  if (pattern->name)
    g_free (pattern->name);

  g_free (pattern);
}
