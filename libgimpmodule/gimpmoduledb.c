/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmodules.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpcoreconfig.h"
#include "gimpdatafiles.h"
#include "gimplist.h"
#include "gimpmoduleinfo.h"
#include "gimpmodules.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


static void       gimp_modules_module_initialize  (const gchar *filename,
                                                   gpointer     loader_data);

static GimpModuleInfoObj * gimp_modules_module_find_by_path (Gimp       *gimp,
                                                             const char *fullpath);

#ifdef DUMP_DB
static void       print_module_info                (gpointer    data,
                                                    gpointer    user_data);
#endif

static gboolean   gimp_modules_write_modulerc      (Gimp       *gimp);

static void       gimp_modules_module_free_func    (gpointer    data, 
                                                    gpointer    user_data);
static void       gimp_modules_module_on_disk_func (gpointer    data, 
                                                    gpointer    user_data);
static void       gimp_modules_module_remove_func  (gpointer    data, 
                                                    gpointer    user_data);


void
gimp_modules_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->modules = gimp_list_new (GIMP_TYPE_MODULE_INFO,
                                 GIMP_CONTAINER_POLICY_WEAK);
  gimp_object_set_name (GIMP_OBJECT (gimp->modules), "modules");

  gimp->write_modulerc = FALSE;
}

void
gimp_modules_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->modules)
    {
      g_object_unref (G_OBJECT (gimp->modules));
      gimp->modules = NULL;
    }
}

void
gimp_modules_load (Gimp *gimp)
{
  gchar *filename;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("modulerc");
  gimprc_parse_file (filename);
  g_free (filename);

  if (g_module_supported ())
    gimp_datafiles_read_directories (gimp->config->module_path,
                                     0 /* no flags */,
				     gimp_modules_module_initialize,
                                     gimp);

#ifdef DUMP_DB
  gimp_container_foreach (modules, print_module_info, NULL);
#endif
}

void
gimp_modules_unload (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->write_modulerc)
    {
      if (gimp_modules_write_modulerc (gimp))
	{
	  gimp->write_modulerc = FALSE;
	}
    }

  gimp_container_foreach (gimp->modules, gimp_modules_module_free_func, NULL);
}

void
gimp_modules_refresh (Gimp *gimp)
{
  GList *kill_list = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /* remove modules we don't have on disk anymore */
  gimp_container_foreach (gimp->modules,
                          gimp_modules_module_on_disk_func,
                          &kill_list);
  g_list_foreach (kill_list,
                  gimp_modules_module_remove_func,
                  gimp);
  g_list_free (kill_list);

  /* walk filesystem and add new things we find */
  gimp_datafiles_read_directories (gimp->config->module_path,
                                   0 /* no flags */,
				   gimp_modules_module_initialize,
                                   gimp);
}

static void
gimp_modules_module_free_func (gpointer data,
                               gpointer user_data)
{
  GimpModuleInfoObj *module_info = data;

  if (module_info->module &&
      module_info->unload &&
      module_info->state == GIMP_MODULE_STATE_LOADED_OK)
    {
      gimp_module_info_module_unload (module_info, FALSE);
    }
}

static void
add_to_inhibit_string (gpointer data, 
		       gpointer user_data)
{
  GimpModuleInfoObj *module_info = data;
  GString           *str         = user_data;

  if (module_info->load_inhibit)
    {
      str = g_string_append_c (str, G_SEARCHPATH_SEPARATOR);
      str = g_string_append (str, module_info->fullpath);
    }
}

static gboolean
gimp_modules_write_modulerc (Gimp *gimp)
{
  GString  *str;
  gchar    *p;
  gchar    *filename;
  FILE     *fp;
  gboolean  saved = FALSE;

  str = g_string_new (NULL);
  gimp_container_foreach (gimp->modules, add_to_inhibit_string, str);
  if (str->len > 0)
    p = str->str + 1;
  else
    p = "";

  filename = gimp_personal_rc_file ("modulerc");
  fp = fopen (filename, "wt");
  g_free (filename);
  if (fp)
    {
      fprintf (fp, "(module-load-inhibit \"%s\")\n", p);
      fclose (fp);
      saved = TRUE;
    }

  g_string_free (str, TRUE);

  return saved;
}

/* name must be of the form lib*.so (Unix) or *.dll (Win32) */
static gboolean
valid_module_name (const gchar *filename)
{
  gchar *basename;
  gint   len;

  basename = g_path_get_basename (filename);

  len = strlen (basename);

#if !defined(G_OS_WIN32) && !defined(G_WITH_CYGWIN) && !defined(__EMX__)
  if (len < 3 + 1 + 3)
    goto no_module;

  if (strncmp (basename, "lib", 3))
    goto no_module;

  if (strcmp (basename + len - 3, ".so"))
    goto no_module;
#else
  if (len < 1 + 4)
    goto no_module;

  if (g_strcasecmp (basename + len - 4, ".dll"))
    goto no_module;
#endif

  g_free (basename);

  return TRUE;

 no_module:
  g_free (basename);

  return FALSE;
}

static void
gimp_modules_module_initialize (const gchar *filename,
                                gpointer     loader_data)
{
  GimpModuleInfoObj *module_info;
  Gimp              *gimp;

  gimp = GIMP (loader_data);

  if (! valid_module_name (filename))
    return;

  /* don't load if we already know about it */
  if (gimp_modules_module_find_by_path (gimp, filename))
    return;

  module_info = gimp_module_info_new (filename);

  gimp_module_info_set_load_inhibit (module_info,
                                     gimp->config->module_db_load_inhibit);

  if (! module_info->load_inhibit)
    {
      if (gimp->be_verbose)
	g_print (_("loading module: '%s'\n"), filename);

      gimp_module_info_module_load (module_info, TRUE);
    }
  else
    {
      if (gimp->be_verbose)
	g_print (_("skipping module: '%s'\n"), filename);

      module_info->state = GIMP_MODULE_STATE_UNLOADED_OK;
    }

  gimp_container_add (gimp->modules, GIMP_OBJECT (module_info));
}

typedef struct
{
  const gchar       *search_key;
  GimpModuleInfoObj *found;
} find_by_path_closure;

static void
gimp_modules_module_path_cmp_func (gpointer data, 
                                   gpointer user_data)
{
  GimpModuleInfoObj    *module_info;
  find_by_path_closure *closure;

  module_info = (GimpModuleInfoObj *) data;
  closure     = (find_by_path_closure *) user_data;

  if (! strcmp (module_info->fullpath, closure->search_key))
    closure->found = module_info;
}

static GimpModuleInfoObj *
gimp_modules_module_find_by_path (Gimp       *gimp,
                                  const char *fullpath)
{
  find_by_path_closure cl;

  cl.found      = NULL;
  cl.search_key = fullpath;

  gimp_container_foreach (gimp->modules,
                          gimp_modules_module_path_cmp_func, &cl);

  return cl.found;
}

#ifdef DUMP_DB
static void
print_module_info (gpointer data, 
		   gpointer user_data)
{
  GimpModuleInfoObj *i = data;

  g_print ("\n%s: %s\n",
	   i->fullpath, statename[i->state]);
  g_print ("  module:%p  lasterr:%s  init:%p  unload:%p\n",
	   i->module, i->last_module_error? i->last_module_error : "NONE",
	   i->init, i->unload);
  if (i->info)
    {
      g_print ("  shutdown_data: %p\n"
	       "  purpose:   %s\n"
	       "  author:    %s\n"
	       "  version:   %s\n"
	       "  copyright: %s\n"
	       "  date:      %s\n",
	       i->info->shutdown_data,
	       i->info->purpose, i->info->author, i->info->version,
	       i->info->copyright, i->info->date);
    }
}
#endif

static void
gimp_modules_module_on_disk_func (gpointer data, 
                                  gpointer user_data)
{
  GimpModuleInfoObj  *module_info;
  GList             **kill_list;
  gint                old_on_disk;
  struct stat         statbuf;
  gint                ret;

  module_info = (GimpModuleInfoObj *) data;
  kill_list   = (GList **) user_data;

  old_on_disk = module_info->on_disk;

  ret = stat (module_info->fullpath, &statbuf);
  if (ret != 0)
    module_info->on_disk = FALSE;
  else
    module_info->on_disk = TRUE;

  /* if it's not on the disk, and it isn't in memory, mark it to be
   * removed later.
   */
  if (! module_info->on_disk && ! module_info->module)
    {
      *kill_list = g_list_append (*kill_list, module_info);
      module_info = NULL;
    }

  if (module_info && module_info->on_disk != old_on_disk)
    gimp_module_info_modified (module_info);
}

static void
gimp_modules_module_remove_func (gpointer data, 
                                 gpointer user_data)
{
  GimpModuleInfoObj *module_info;
  Gimp              *gimp;

  module_info = (GimpModuleInfoObj *) data;
  gimp        = (Gimp *) user_data;

  gimp_container_remove (gimp->modules, GIMP_OBJECT (module_info));

  g_object_unref (G_OBJECT (module_info));
}
