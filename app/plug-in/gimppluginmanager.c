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

#include "string.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcoreconfig.h"
#include "core/gimpdatafiles.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gui/plug-in-menus.h"

#include "plug-in.h"
#include "plug-ins.h"
#include "plug-in-def.h"
#include "plug-in-params.h"
#include "plug-in-proc.h"
#include "plug-in-progress.h"
#include "plug-in-rc.h"

#include "libgimp/gimpintl.h"


typedef struct _PlugInHelpPathDef PlugInHelpPathDef;

struct _PlugInHelpPathDef
{
  gchar *prog_name;
  gchar *help_path;
};


static void   plug_ins_init_file       (const gchar       *filename,
                                        gpointer           loader_data);
static void   plug_ins_add_to_db       (Gimp              *gimp);
static void   plug_ins_proc_def_insert (PlugInProcDef     *proc_def,
                                        void (* superceed_fn) (void *));
static void   plug_ins_proc_def_dead   (void              *freed_proc_def);


GSList *proc_defs          = NULL;
gchar  *std_plugins_domain = "gimp14-std-plug-ins";

static GSList   *plug_in_defs       = NULL;
static GSList   *gimprc_proc_defs   = NULL;
static GSList   *help_path_defs     = NULL;

static gboolean  write_pluginrc     = FALSE;


void
plug_ins_init (Gimp               *gimp,
               GimpInitStatusFunc  status_callback)
{
  gchar         *filename;
  gchar         *basename;
  GSList        *tmp;
  GSList        *tmp2;
  PlugInDef     *plug_in_def;
  PlugInProcDef *proc_def;
  gfloat         nplugins;
  gfloat         nth;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (status_callback != NULL);

  plug_in_init (gimp);

  /* search for binaries in the plug-in directory path */
  gimp_datafiles_read_directories (gimp->config->plug_in_path, 
                                   MODE_EXECUTABLE,
				   plug_ins_init_file, NULL);

  /* read the pluginrc file for cached data */
  if (gimp->config->pluginrc_path)
    {
      if (g_path_is_absolute (gimp->config->pluginrc_path))
        filename = g_strdup (gimp->config->pluginrc_path);
      else
        filename = g_build_filename (gimp_directory (),
                                     gimp->config->pluginrc_path,
                                     NULL);
    }
  else
    {
      filename = gimp_personal_rc_file ("pluginrc");
    }

  (* status_callback) (_("Resource configuration"), filename, -1);
  plug_in_rc_parse (filename);

  /* query any plug-ins that have changed since we last wrote out
   *  the pluginrc file.
   */
  tmp = plug_in_defs;
  (* status_callback) (_("New Plug-ins"), "", 0);
  nplugins = g_slist_length (tmp);
  nth = 0;
  while (tmp)
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      if (plug_in_def->query)
	{
	  write_pluginrc = TRUE;

	  if (gimp->be_verbose)
	    g_print (_("querying plug-in: \"%s\"\n"), plug_in_def->prog);

	  plug_in_call_query (gimp, plug_in_def);
	}

      basename = g_path_get_basename (plug_in_def->prog);
      (* status_callback) (NULL, basename, nth / nplugins);
      nth++;
      g_free (basename);
    }

  /* insert the proc defs */
  for (tmp = gimprc_proc_defs; tmp; tmp = g_slist_next (tmp))
    {
      proc_def = g_new (PlugInProcDef, 1);
      *proc_def = *((PlugInProcDef*) tmp->data);
      plug_ins_proc_def_insert (proc_def, NULL);
    }

  tmp = plug_in_defs;
  while (tmp)
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      tmp2 = plug_in_def->proc_defs;
      while (tmp2)
	{
	  proc_def = tmp2->data;
	  tmp2 = tmp2->next;

 	  proc_def->mtime = plug_in_def->mtime; 
	  plug_ins_proc_def_insert (proc_def, plug_ins_proc_def_dead);
	}
    }

  /* write the pluginrc file if necessary */
  if (write_pluginrc)
    {
      if (gimp->be_verbose)
	g_print (_("writing \"%s\"\n"), filename);

      plug_in_rc_write (plug_in_defs, filename);
    }

  g_free (filename);

  /* add the plug-in procs to the procedure database */
  plug_ins_add_to_db (gimp);

  if (! gimp->no_interface)
    {
      /* make the menu */
      plug_in_make_menu (plug_in_defs, std_plugins_domain);
    }

  /* initial the plug-ins */
  (* status_callback) (_("Plug-ins"), "", 0);

  tmp = plug_in_defs;
  nth=0;
  
  while (tmp)
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      if (plug_in_def->has_init)
	{
	  if (gimp->be_verbose)
	    g_print (_("initializing plug-in: \"%s\"\n"), plug_in_def->prog);

	  plug_in_call_init (gimp, plug_in_def);
	}

      basename = g_path_get_basename (plug_in_def->prog);
      (* status_callback) (NULL, basename, nth / nplugins);
      nth++;
      g_free (basename);
    }
    
  /* run the available extensions */
  if (gimp->be_verbose)
    g_print (_("Starting extensions: "));

  (* status_callback) (_("Extensions"), "", 0);

  tmp = proc_defs;
  nplugins = g_slist_length (tmp); nth = 0;

  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (proc_def->prog &&
	  (proc_def->db_info.num_args == 0) &&
	  (proc_def->db_info.proc_type == GIMP_EXTENSION))
	{
	  if (gimp->be_verbose)
	    g_print ("%s ", proc_def->db_info.name);

	  (* status_callback) (NULL, proc_def->db_info.name, nth / nplugins);

	  plug_in_run (gimp, &proc_def->db_info, NULL, 0, FALSE, TRUE, -1);
	}
    }

  if (gimp->be_verbose)
    g_print ("\n");

  /* create help path list and free up stuff */
  for (tmp = plug_in_defs; tmp; tmp = g_slist_next (tmp))
    {
      plug_in_def = tmp->data;

      if (plug_in_def->help_path)
	{
	  PlugInHelpPathDef *help_path_def;

	  help_path_def = g_new (PlugInHelpPathDef, 1);

	  help_path_def->prog_name = g_strdup (plug_in_def->prog);
	  help_path_def->help_path = g_strdup (plug_in_def->help_path);

	  help_path_defs = g_slist_prepend (help_path_defs, help_path_def);
	}

      plug_in_def_free (plug_in_def, FALSE);
    }

  g_slist_free (plug_in_defs);
  plug_in_defs = NULL;
}


void
plug_ins_exit (Gimp *gimp)
{
  plug_in_kill (gimp);
}

gchar *
plug_in_image_types (gchar *name)
{
  PlugInDef     *plug_in_def;
  PlugInProcDef *proc_def;
  GSList        *tmp;

  g_return_val_if_fail (name != NULL, NULL);

  if (current_plug_in)
    {
      plug_in_def = current_plug_in->user_data;
      tmp = plug_in_def->proc_defs;
    }
  else
    {
      tmp = proc_defs;
    }

  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (strcmp (proc_def->db_info.name, name) == 0)
	return proc_def->image_types;
    }

  return NULL;
}

GSList *
plug_ins_extensions_parse (gchar *extensions)
{
  GSList *list;
  gchar  *extension;
  gchar  *next_token;

  list = NULL;

  /* EXTENSIONS can be NULL.  Avoid calling strtok if it is.  */
  if (extensions)
    {
      extensions = g_strdup (extensions);
      next_token = extensions;
      extension = strtok (next_token, " \t,");
      while (extension)
	{
	  list = g_slist_prepend (list, g_strdup (extension));
	  extension = strtok (NULL, " \t,");
	}
      g_free (extensions);
    }

  return g_slist_reverse (list);
}

void
plug_ins_add_internal (PlugInProcDef *proc_def)
{
  proc_defs = g_slist_prepend (proc_defs, proc_def);
}

PlugInProcDef *
plug_ins_file_handler (gchar *name,
                       gchar *extensions,
                       gchar *prefixes,
                       gchar *magics)
{
  PlugInDef     *plug_in_def;
  PlugInProcDef *proc_def;
  GSList        *tmp;

  g_return_val_if_fail (name != NULL, NULL);

  if (current_plug_in)
    {
      plug_in_def = current_plug_in->user_data;
      tmp = plug_in_def->proc_defs;
    }
  else
    {
      tmp = proc_defs;
    }

  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (strcmp (proc_def->db_info.name, name) == 0)
	{
	  /* EXTENSIONS can be proc_def->extensions  */
	  if (proc_def->extensions != extensions)
	    {
	      if (proc_def->extensions)
		g_free (proc_def->extensions);
	      proc_def->extensions = g_strdup (extensions);
	    }

	  proc_def->extensions_list =
            plug_ins_extensions_parse (proc_def->extensions);

	  /* PREFIXES can be proc_def->prefixes  */
	  if (proc_def->prefixes != prefixes)
	    {
	      if (proc_def->prefixes)
		g_free (proc_def->prefixes);
	      proc_def->prefixes = g_strdup (prefixes);
	    }

	  proc_def->prefixes_list =
            plug_ins_extensions_parse (proc_def->prefixes);

	  /* MAGICS can be proc_def->magics  */
	  if (proc_def->magics != magics)
	    {
	      if (proc_def->magics)
		g_free (proc_def->magics);
	      proc_def->magics = g_strdup (magics);
	    }

	  proc_def->magics_list =
            plug_ins_extensions_parse (proc_def->magics);

	  return proc_def;
	}
    }

  return NULL;
}

void
plug_ins_def_add (PlugInDef *plug_in_def)
{
  PlugInDef     *tplug_in_def;
  PlugInProcDef *proc_def;
  GSList        *tmp;
  gchar         *basename1;
  gchar         *basename2;

  basename1 = g_path_get_basename (plug_in_def->prog);

  /*  If this is a file load or save plugin, make sure we have
   *  something for one of the extensions, prefixes, or magic number.
   *  Other bits of code rely on detecting file plugins by the presence
   *  of one of these things, but Nick Lamb's alien/unknown format
   *  loader needs to be able to register no extensions, prefixes or
   *  magics. -- austin 13/Feb/99
   */
  for (tmp = plug_in_def->proc_defs; tmp; tmp = g_slist_next (tmp))
    {
      proc_def = tmp->data;

      if (!proc_def->extensions && !proc_def->prefixes && !proc_def->magics &&
	  proc_def->menu_path &&
	  (!strncmp (proc_def->menu_path, "<Load>", 6) ||
	   !strncmp (proc_def->menu_path, "<Save>", 6)))
	{
	  proc_def->extensions = g_strdup ("");
	}
    }

  for (tmp = plug_in_defs; tmp; tmp = g_slist_next (tmp))
    {
      tplug_in_def = tmp->data;

      basename2 = g_path_get_basename (tplug_in_def->prog);

      if (strcmp (basename1, basename2) == 0)
	{
	  if ((g_ascii_strcasecmp (plug_in_def->prog, tplug_in_def->prog) == 0) &&
	      (plug_in_def->mtime == tplug_in_def->mtime))
	    {
	      /* Use cached plug-in entry */
	      tmp->data = plug_in_def;
	      plug_in_def_free (tplug_in_def, TRUE);
	    }
	  else
	    {
	      plug_in_def_free (plug_in_def, TRUE);    
	    }

	  g_free (basename2);

	  return;
	}

      g_free (basename2);
    }

  g_free (basename1);

  write_pluginrc = TRUE;
  g_print ("\"%s\" executable not found\n", plug_in_def->prog);
  plug_in_def_free (plug_in_def, FALSE);
}

gchar *
plug_ins_menu_path (gchar *name)
{
  PlugInDef *plug_in_def;
  PlugInProcDef *proc_def;
  GSList *tmp, *tmp2;

  g_return_val_if_fail (name != NULL, NULL);

  for (tmp = plug_in_defs; tmp; tmp = g_slist_next (tmp))
    {
      plug_in_def = tmp->data;

      for (tmp2 = plug_in_def->proc_defs; tmp2; tmp2 = g_slist_next (tmp2))
	{
	  proc_def = tmp2->data;

	  if (strcmp (proc_def->db_info.name, name) == 0)
	    return proc_def->menu_path;
	}
    }

  for (tmp = proc_defs; tmp; tmp = g_slist_next (tmp))
    {
      proc_def = tmp->data;

      if (strcmp (proc_def->db_info.name, name) == 0)
	return proc_def->menu_path;
    }

  return NULL;
}

gchar *
plug_ins_help_path (gchar *prog_name)
{
  PlugInHelpPathDef *help_path_def;
  GSList *list;

  if (!prog_name || !strlen (prog_name))
    return NULL;

  for (list = help_path_defs; list; list = g_slist_next (list))
    {
      help_path_def = (PlugInHelpPathDef *) list->data;

      if (help_path_def &&
	  help_path_def->prog_name &&
	  strcmp (help_path_def->prog_name, prog_name) == 0)
	return help_path_def->help_path;
    }

  return NULL;
}

static void
plug_ins_init_file (const gchar *filename,
                    gpointer     loader_data)
{
  GSList      *tmp;
  PlugInDef   *plug_in_def;
  gchar       *plug_in_name;
  gchar       *basename;

  basename = g_path_get_basename (filename);

  plug_in_def = NULL;
  tmp = plug_in_defs;

  while (tmp)
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      plug_in_name = g_path_get_basename (plug_in_def->prog);

      if (g_ascii_strcasecmp (basename, plug_in_name) == 0)
	{
	  g_print ("duplicate plug-in: \"%s\" (skipping)\n", filename);
	  return;
	}

      g_free (plug_in_name);

      plug_in_def = NULL;
    }

  g_free (basename);

  plug_in_def = plug_in_def_new (filename);
  plug_in_def->mtime = gimp_datafile_mtime ();
  plug_in_def->query = TRUE;

  plug_in_defs = g_slist_append (plug_in_defs, plug_in_def);
}

static void
plug_ins_add_to_db (Gimp *gimp)
{
  PlugInProcDef *proc_def;
  Argument       args[4];
  Argument      *return_vals;
  GSList        *tmp;

  tmp = proc_defs;

  while (tmp)
    {
      proc_def = tmp->data;
      tmp = tmp->next;

      if (proc_def->prog && (proc_def->db_info.proc_type != GIMP_INTERNAL))
	{
	  proc_def->db_info.exec_method.plug_in.filename = proc_def->prog;
	  procedural_db_register (gimp, &proc_def->db_info);
	}
    }

  for (tmp = proc_defs; tmp; tmp = tmp->next)
    {
      proc_def = tmp->data;

      if (proc_def->extensions || proc_def->prefixes || proc_def->magics)
        {
          args[0].arg_type          = GIMP_PDB_STRING;
          args[0].value.pdb_pointer = proc_def->db_info.name;

          args[1].arg_type          = GIMP_PDB_STRING;
          args[1].value.pdb_pointer = proc_def->extensions;

	  args[2].arg_type          = GIMP_PDB_STRING;
	  args[2].value.pdb_pointer = proc_def->prefixes;

	  args[3].arg_type          = GIMP_PDB_STRING;
	  args[3].value.pdb_pointer = proc_def->magics;

          if (proc_def->image_types)
            {
              return_vals = 
		procedural_db_execute (gimp,
				       "gimp_register_save_handler", 
				       args);
              g_free (return_vals);
            }
          else
            {
              return_vals = 
		procedural_db_execute (gimp,
				       "gimp_register_magic_load_handler", 
				       args);
              g_free (return_vals);
            }
	}
    }
}

static void
plug_ins_proc_def_insert (PlugInProcDef *proc_def,
                          void (*superceed_fn)(void*))
{
  PlugInProcDef *tmp_proc_def;
  GSList *tmp;
  GSList *prev;
  GSList *list;

  prev = NULL;
  tmp  = proc_defs;

  while (tmp)
    {
      tmp_proc_def = tmp->data;

      if (strcmp (proc_def->db_info.name, tmp_proc_def->db_info.name) == 0)
	{
	  tmp->data = proc_def;

	  if (proc_def->menu_path)
	    g_free (proc_def->menu_path);
	  if (proc_def->accelerator)
	    g_free (proc_def->accelerator);

	  proc_def->menu_path   = tmp_proc_def->menu_path;
	  proc_def->accelerator = tmp_proc_def->accelerator;

	  tmp_proc_def->menu_path   = NULL;
	  tmp_proc_def->accelerator = NULL;

	  if (superceed_fn)
	    (* superceed_fn) (tmp_proc_def);

	  plug_in_proc_def_destroy (tmp_proc_def, FALSE);
	  return;
	}
      else if (!proc_def->menu_path ||
	       (tmp_proc_def->menu_path &&
		(strcmp (proc_def->menu_path, tmp_proc_def->menu_path) < 0)))
	{
	  list = g_slist_alloc ();
	  list->data = proc_def;

	  list->next = tmp;
	  if (prev)
	    prev->next = list;
	  else
	    proc_defs = list;
	  return;
	}

      prev = tmp;
      tmp = tmp->next;
    }

  proc_defs = g_slist_append (proc_defs, proc_def);
}

/* called when plug_in_proc_def_insert causes a proc_def to be
 * overridden and thus g_free()d.
 */
static void
plug_ins_proc_def_dead (void *freed_proc_def)
{
  GSList        *tmp;
  PlugInDef     *plug_in_def;
  PlugInProcDef *proc_def = freed_proc_def;

  g_warning ("removing duplicate PDB procedure \"%s\"",
	     proc_def->db_info.name);

  /* search the plugin list to see if any plugins had references to 
   * the recently freed proc_def.
   */
  for (tmp = plug_in_defs; tmp; tmp = g_slist_next (tmp))
    {
      plug_in_def = tmp->data;

      plug_in_def->proc_defs = g_slist_remove (plug_in_def->proc_defs,
					       freed_proc_def);
    }
}

void
plug_ins_proc_def_remove (PlugInProcDef *proc_def,
                          Gimp          *gimp)
{
  if (! gimp->no_interface)
    {
      if (proc_def->menu_path)
        plug_in_delete_menu_entry (proc_def->menu_path);
    }

  /*  Unregister the procedural database entry  */
  procedural_db_unregister (gimp, proc_def->db_info.name);

  /*  Remove the defintion from the global list  */
  proc_defs = g_slist_remove (proc_defs, proc_def);

  /*  Destroy the definition  */
  plug_in_proc_def_destroy (proc_def, TRUE);
}

PlugInImageType
plug_ins_image_types_parse (gchar *image_types)
{
  gchar           *type_spec = image_types;
  PlugInImageType  types     = 0;

  /* 
   *  If the plug_in registers with image_type == NULL or "", return 0
   *  By doing so it won't be touched by plug_in_set_menu_sensitivity() 
   */
  if (!image_types)
    return types;

  while (*image_types)
    {
      while (*image_types &&
	     ((*image_types == ' ') ||
	      (*image_types == '\t') ||
	      (*image_types == ',')))
	image_types++;

      if (*image_types)
	{
	  if (strncmp (image_types, "RGBA", 4) == 0)
	    {
	      types |= PLUG_IN_RGBA_IMAGE;
	      image_types += 4;
	    }
	  else if (strncmp (image_types, "RGB*", 4) == 0)
	    {
	      types |= PLUG_IN_RGB_IMAGE | PLUG_IN_RGBA_IMAGE;
	      image_types += 4;
	    }
	  else if (strncmp (image_types, "RGB", 3) == 0)
	    {
	      types |= PLUG_IN_RGB_IMAGE;
	      image_types += 3;
	    }
	  else if (strncmp (image_types, "GRAYA", 5) == 0)
	    {
	      types |= PLUG_IN_GRAYA_IMAGE;
	      image_types += 5;
	    }
	  else if (strncmp (image_types, "GRAY*", 5) == 0)
	    {
	      types |= PLUG_IN_GRAY_IMAGE | PLUG_IN_GRAYA_IMAGE;
	      image_types += 5;
	    }
	  else if (strncmp (image_types, "GRAY", 4) == 0)
	    {
	      types |= PLUG_IN_GRAY_IMAGE;
	      image_types += 4;
	    }
	  else if (strncmp (image_types, "INDEXEDA", 8) == 0)
	    {
	      types |= PLUG_IN_INDEXEDA_IMAGE;
	      image_types += 8;
	    }
	  else if (strncmp (image_types, "INDEXED*", 8) == 0)
	    {
	      types |= PLUG_IN_INDEXED_IMAGE | PLUG_IN_INDEXEDA_IMAGE;
	      image_types += 8;
	    }
	  else if (strncmp (image_types, "INDEXED", 7) == 0)
	    {
	      types |= PLUG_IN_INDEXED_IMAGE;
	      image_types += 7;
	    }
	  else if (strncmp (image_types, "*", 1) == 0)
	    {
	      types |= PLUG_IN_RGB_IMAGE | PLUG_IN_RGBA_IMAGE
	             | PLUG_IN_GRAY_IMAGE | PLUG_IN_GRAYA_IMAGE
	             | PLUG_IN_INDEXED_IMAGE | PLUG_IN_INDEXEDA_IMAGE;
	      image_types += 1;
	    }
	  else
	    {
              g_warning ("image_type contains unrecognizable parts: '%s'", type_spec);
	      while (*image_types &&
                     ((*image_types != ' ') ||
                      (*image_types != '\t') ||
                      (*image_types != ',')))
		image_types++;
	    }
	}
    }

  return types;
}
