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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "plug-in-types.h"

#include "config/gimpcoreconfig.h"
#include "config/gimpconfig-path.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "plug-in.h"
#include "plug-ins.h"
#include "plug-in-def.h"
#include "plug-in-params.h"
#include "plug-in-proc.h"
#include "plug-in-progress.h"
#include "plug-in-rc.h"
#include "plug-in-run.h"

#include "gimp-intl.h"


#define STD_PLUGINS_DOMAIN GETTEXT_PACKAGE "-std-plug-ins"


typedef struct _PlugInLocaleDomainDef PlugInLocaleDomainDef;
typedef struct _PlugInHelpDomainDef   PlugInHelpDomainDef;

struct _PlugInLocaleDomainDef
{
  gchar *prog_name;
  gchar *domain_name;
  gchar *domain_path;
};

struct _PlugInHelpDomainDef
{
  gchar *prog_name;
  gchar *domain_name;
  gchar *domain_uri;
};


static void            plug_ins_init_file (const GimpDatafileData *file_data,
                                           gpointer                user_data);
static void            plug_ins_add_to_db       (Gimp             *gimp);
static PlugInProcDef * plug_ins_proc_def_insert (Gimp             *gimp,
                                                 PlugInProcDef    *proc_def);


/*  public functions  */

void
plug_ins_init (Gimp               *gimp,
               GimpInitStatusFunc  status_callback)
{
  gchar         *filename;
  gchar         *basename;
  gchar         *path;
  GSList        *tmp;
  GSList        *tmp2;
  GList         *extensions = NULL;
  GList         *list;
  PlugInDef     *plug_in_def;
  PlugInProcDef *proc_def;
  gdouble        n_plugins;
  gdouble        n_extensions;
  gdouble        nth;
  GError        *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (status_callback != NULL);

  plug_in_init (gimp);

  /* search for binaries in the plug-in directory path */
  path = gimp_config_path_expand (gimp->config->plug_in_path, TRUE, NULL);

  gimp_datafiles_read_directories (path,
                                   G_FILE_TEST_IS_EXECUTABLE,
				   plug_ins_init_file,
                                   &gimp->plug_in_defs);

  g_free (path);


  /* read the pluginrc file for cached data */
  if (gimp->config->plug_in_rc_path)
    {
      filename = gimp_config_path_expand (gimp->config->plug_in_rc_path,
                                          TRUE, NULL);

      if (!g_path_is_absolute (filename))
        {
          gchar *tmp = g_build_filename (gimp_directory (), filename, NULL);

          g_free (filename);
          filename = tmp;
        }
    }
  else
    {
      filename = gimp_personal_rc_file ("pluginrc");
    }

  (* status_callback) (_("Resource configuration"),
		       gimp_filename_to_utf8 (filename), -1);

  if (! plug_in_rc_parse (gimp, filename, &error))
    {
      g_message (error->message);
      g_error_free (error);
    }

  /*  Query any plug-ins that have changed since we last wrote out
   *  the pluginrc file.
   */
  (* status_callback) (_("Querying new Plug-ins"), "", 0);
  n_plugins = g_slist_length (gimp->plug_in_defs);

  for (tmp = gimp->plug_in_defs, nth = 0;
       tmp;
       tmp = g_slist_next (tmp), nth++)
    {
      plug_in_def = tmp->data;

      basename = g_path_get_basename (plug_in_def->prog);
      (* status_callback) (NULL, gimp_filename_to_utf8 (basename),
			   nth / n_plugins);
      g_free (basename);

      if (plug_in_def->needs_query)
	{
	  gimp->write_pluginrc = TRUE;

	  if (gimp->be_verbose)
	    g_print (_("Querying plug-in: '%s'\n"),
		     gimp_filename_to_utf8 (plug_in_def->prog));

	  plug_in_call_query (gimp, plug_in_def);
	}
    }

  (* status_callback) (NULL, NULL, 1.0);

  /* insert the proc defs */
  for (tmp = gimp->plug_in_defs; tmp; tmp = g_slist_next (tmp))
    {
      plug_in_def = tmp->data;

      for (tmp2 = plug_in_def->proc_defs; tmp2; tmp2 = g_slist_next (tmp2))
	{
          PlugInProcDef *overridden_proc_def;

	  proc_def = tmp2->data;

 	  proc_def->mtime = plug_in_def->mtime;

	  overridden_proc_def = plug_ins_proc_def_insert (gimp, proc_def);

          if (overridden_proc_def)
            {
              GSList *tmp2;

              g_warning ("removing duplicate PDB procedure \"%s\"",
                         overridden_proc_def->db_info.name);

              /* search the plugin list to see if any plugins had references to
               * the overridden_proc_def.
               */
              for (tmp2 = gimp->plug_in_defs; tmp2; tmp2 = g_slist_next (tmp2))
                {
                  PlugInDef *plug_in_def2;

                  plug_in_def2 = tmp2->data;

                  plug_in_def2->proc_defs =
                    g_slist_remove (plug_in_def2->proc_defs,
                                    overridden_proc_def);
                }

              plug_in_proc_def_free (overridden_proc_def);
            }
	}
    }

  /* write the pluginrc file if necessary */
  if (gimp->write_pluginrc)
    {
      GError *error = NULL;

      if (gimp->be_verbose)
	g_print (_("Writing '%s'\n"),
		 gimp_filename_to_utf8 (filename));

      if (! plug_in_rc_write (gimp->plug_in_defs, filename, &error))
        {
          g_message ("%s", error->message);
          g_error_free (error);
        }

      gimp->write_pluginrc = FALSE;
    }

  g_free (filename);

  /* add the plug-in procs to the procedure database */
  plug_ins_add_to_db (gimp);

  /* restore file procs order */
  gimp->load_procs = g_slist_reverse (gimp->load_procs);
  gimp->save_procs = g_slist_reverse (gimp->save_procs);

  /* create help_path and locale_domain lists */
  for (tmp = gimp->plug_in_defs; tmp; tmp = g_slist_next (tmp))
    {
      plug_in_def = tmp->data;

      if (plug_in_def->locale_domain_name)
	{
	  PlugInLocaleDomainDef *def;

	  def = g_new (PlugInLocaleDomainDef, 1);

	  def->prog_name   = g_strdup (plug_in_def->prog);
	  def->domain_name = g_strdup (plug_in_def->locale_domain_name);
          def->domain_path = g_strdup (plug_in_def->locale_domain_path);

	  gimp->plug_in_locale_domains =
            g_slist_prepend (gimp->plug_in_locale_domains, def);

#ifdef VERBOSE
          g_print ("added locale domain \"%s\" for path \"%s\"\n",
                   def->domain_name ? def->domain_name : "(null)",
                   def->domain_path ?
                   gimp_filename_to_utf8 (def->domain_path) : "(null)");
#endif
	}

      if (plug_in_def->help_domain_name)
	{
	  PlugInHelpDomainDef *def;

	  def = g_new (PlugInHelpDomainDef, 1);

	  def->prog_name   = g_strdup (plug_in_def->prog);
	  def->domain_name = g_strdup (plug_in_def->help_domain_name);
	  def->domain_uri  = g_strdup (plug_in_def->help_domain_uri);

	  gimp->plug_in_help_domains =
            g_slist_prepend (gimp->plug_in_help_domains, def);

#ifdef VERBOSE
          g_print ("added help domain \"%s\" for base uri \"%s\"\n",
                   def->domain_name ? def->domain_name : "(null)",
                   def->domain_uri  ? def->domain_uri  : "(null)");
#endif
	}
    }

  if (! gimp->no_interface)
    gimp_menus_init (gimp, gimp->plug_in_defs, STD_PLUGINS_DOMAIN);

  /* initial the plug-ins */
  (* status_callback) (_("Initializing Plug-ins"), "", 0);

  for (tmp = gimp->plug_in_defs, nth = 0;
       tmp;
       tmp = g_slist_next (tmp), nth++)
    {
      plug_in_def = tmp->data;

      basename = g_path_get_basename (plug_in_def->prog);
      (* status_callback) (NULL, gimp_filename_to_utf8 (basename),
			   nth / n_plugins);
      g_free (basename);

      if (plug_in_def->has_init)
	{
	  if (gimp->be_verbose)
	    g_print (_("Initializing plug-in: '%s'\n"),
                     gimp_filename_to_utf8 (plug_in_def->prog));

	  plug_in_call_init (gimp, plug_in_def);
	}
    }

  (* status_callback) (NULL, NULL, 1.0);

  /* build list of automatically started extensions */
  for (tmp = gimp->plug_in_proc_defs, nth = 0;
       tmp;
       tmp = g_slist_next (tmp), nth++)
    {
      proc_def = tmp->data;

      if (proc_def->prog                                &&
	  proc_def->db_info.proc_type == GIMP_EXTENSION &&
	  proc_def->db_info.num_args  == 0)
	{
          extensions = g_list_prepend (extensions, proc_def);
        }
    }

  extensions   = g_list_reverse (extensions);
  n_extensions = g_list_length (extensions);

  /* run the available extensions */
  if (extensions)
    {
      (* status_callback) (_("Starting Extensions"), "", 0);

      for (list = extensions, nth = 0;
           list;
           list = g_list_next (list), nth++)
        {
          proc_def = list->data;

	  if (gimp->be_verbose)
	    g_print (_("Starting extension: '%s'\n"), proc_def->db_info.name);

	  (* status_callback) (NULL, proc_def->db_info.name, nth / n_plugins);

	  plug_in_run (gimp, &proc_def->db_info, NULL, 0, FALSE, TRUE, -1);
	}

      (* status_callback) (NULL, NULL, 1.0);

      g_list_free (extensions);
    }

  /* free up stuff */
  for (tmp = gimp->plug_in_defs; tmp; tmp = g_slist_next (tmp))
    {
      plug_in_def = tmp->data;

      plug_in_def_free (plug_in_def, FALSE);
    }

  g_slist_free (gimp->plug_in_defs);
  gimp->plug_in_defs = NULL;
}

void
plug_ins_exit (Gimp *gimp)
{
  GSList *list;

  plug_in_exit (gimp);

  for (list = gimp->plug_in_locale_domains; list; list = g_slist_next (list))
    {
      PlugInLocaleDomainDef *def = list->data;

      g_free (def->prog_name);
      g_free (def->domain_name);
      g_free (def->domain_path);
      g_free (def);
    }

  g_slist_free (gimp->plug_in_locale_domains);
  gimp->plug_in_locale_domains = NULL;

  for (list = gimp->plug_in_help_domains; list; list = g_slist_next (list))
    {
      PlugInHelpDomainDef *def = list->data;

      g_free (def->prog_name);
      g_free (def->domain_name);
      g_free (def->domain_uri);
      g_free (def);
    }

  g_slist_free (gimp->plug_in_help_domains);
  gimp->plug_in_help_domains = NULL;
}

void
plug_ins_add_internal (Gimp          *gimp,
                       PlugInProcDef *proc_def)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (proc_def != NULL);

  gimp->plug_in_proc_defs = g_slist_prepend (gimp->plug_in_proc_defs,
                                             proc_def);
}

PlugInProcDef *
plug_ins_file_handler (Gimp  *gimp,
                       gchar *name,
                       gchar *extensions,
                       gchar *prefixes,
                       gchar *magics)
{
  GSList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (gimp->current_plug_in)
    list = gimp->current_plug_in->plug_in_def->proc_defs;
  else
    list = gimp->plug_in_proc_defs;

  for (; list; list = g_slist_next (list))
    {
      PlugInProcDef *proc_def;

      proc_def = list->data;

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
plug_ins_def_add_from_rc (Gimp      *gimp,
                          PlugInDef *plug_in_def)
{
  GSList *list;
  gchar  *basename1;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (plug_in_def != NULL);
  g_return_if_fail (plug_in_def->prog != NULL);

  if (! g_path_is_absolute (plug_in_def->prog))
    {
      g_warning ("plug_ins_def_add_from_rc: filename not absolute (skipping)");
      plug_in_def_free (plug_in_def, TRUE);
      return;
    }

  basename1 = g_path_get_basename (plug_in_def->prog);

  /*  If this is a file load or save plugin, make sure we have
   *  something for one of the extensions, prefixes, or magic number.
   *  Other bits of code rely on detecting file plugins by the presence
   *  of one of these things, but Nick Lamb's alien/unknown format
   *  loader needs to be able to register no extensions, prefixes or
   *  magics. -- austin 13/Feb/99
   */
  for (list = plug_in_def->proc_defs; list; list = g_slist_next (list))
    {
      PlugInProcDef *proc_def;

      proc_def = (PlugInProcDef *) list->data;

      if (! proc_def->extensions &&
          ! proc_def->prefixes   &&
          ! proc_def->magics     &&
	  proc_def->menu_path    &&
	  (! strncmp (proc_def->menu_path, "<Load>", 6) ||
	   ! strncmp (proc_def->menu_path, "<Save>", 6)))
	{
	  proc_def->extensions = g_strdup ("");
	}
    }

  /*  Check if the entry mentioned in pluginrc matches an executable
   *  found in the plug_in_path.
   */
  for (list = gimp->plug_in_defs; list; list = g_slist_next (list))
    {
      PlugInDef *ondisk_plug_in_def;
      gchar     *basename2;

      ondisk_plug_in_def = (PlugInDef *) list->data;

      basename2 = g_path_get_basename (ondisk_plug_in_def->prog);

      if (! strcmp (basename1, basename2))
	{
	  if (! g_ascii_strcasecmp (plug_in_def->prog,
                                    ondisk_plug_in_def->prog) &&
	      (plug_in_def->mtime == ondisk_plug_in_def->mtime))
	    {
	      /* Use pluginrc entry, deleting ondisk entry */
	      list->data = plug_in_def;
	      plug_in_def_free (ondisk_plug_in_def, TRUE);
	    }
	  else
	    {
              /* Use ondisk entry, deleting pluginrc entry */
	      plug_in_def_free (plug_in_def, TRUE);
	    }

	  g_free (basename2);
	  g_free (basename1);

	  return;
	}

      g_free (basename2);
    }

  g_free (basename1);

  gimp->write_pluginrc = TRUE;
  g_printerr ("executable not found: '%s'\n",
              gimp_filename_to_utf8 (plug_in_def->prog));
  plug_in_def_free (plug_in_def, FALSE);
}

void
plug_ins_temp_proc_def_add (Gimp          *gimp,
                            PlugInProcDef *proc_def)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (proc_def != NULL);

  if (! gimp->no_interface)
    {
      if (proc_def->menu_path)
        {
          const gchar *progname;
          const gchar *locale_domain;
          const gchar *help_domain;

          progname = plug_in_proc_def_get_progname (proc_def);

          locale_domain = plug_ins_locale_domain (gimp, progname, NULL);
          help_domain   = plug_ins_help_domain (gimp, progname, NULL);

          gimp_menus_create_entry (gimp, proc_def,
                                   locale_domain, help_domain);
        }
    }

  /*  Register the procedural database entry  */
  procedural_db_register (gimp, &proc_def->db_info);

  /*  Add the definition to the global list  */
  gimp->plug_in_proc_defs = g_slist_append (gimp->plug_in_proc_defs, proc_def);
}

void
plug_ins_temp_proc_def_remove (Gimp          *gimp,
                               PlugInProcDef *proc_def)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (proc_def != NULL);

  if (! gimp->no_interface)
    {
      if (proc_def->menu_path)
        gimp_menus_delete_entry (gimp, proc_def->menu_path);
    }

  /*  Unregister the procedural database entry  */
  procedural_db_unregister (gimp, proc_def->db_info.name);

  /*  Remove the definition from the global list  */
  gimp->plug_in_proc_defs = g_slist_remove (gimp->plug_in_proc_defs, proc_def);

  /*  Destroy the definition  */
  plug_in_proc_def_free (proc_def);
}

const gchar *
plug_ins_locale_domain (Gimp         *gimp,
                        const gchar  *prog_name,
                        const gchar **domain_path)
{
  GSList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (prog_name != NULL, NULL);

  if (domain_path)
    *domain_path = gimp_locale_directory ();

  for (list = gimp->plug_in_locale_domains; list; list = g_slist_next (list))
    {
      PlugInLocaleDomainDef *def = list->data;

      if (def && def->prog_name && ! strcmp (def->prog_name, prog_name))
        {
          if (domain_path && def->domain_path)
            *domain_path = def->domain_path;

          return def->domain_name;
        }
    }

  return STD_PLUGINS_DOMAIN;
}

const gchar *
plug_ins_help_domain (Gimp         *gimp,
                      const gchar  *prog_name,
                      const gchar **domain_uri)
{
  GSList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (prog_name != NULL, NULL);

  if (domain_uri)
    *domain_uri = NULL;

  for (list = gimp->plug_in_help_domains; list; list = g_slist_next (list))
    {
      PlugInHelpDomainDef *def = list->data;

      if (def && def->prog_name && ! strcmp (def->prog_name, prog_name))
        {
          if (domain_uri && def->domain_uri)
            *domain_uri = def->domain_uri;

          return def->domain_name;
        }
    }

  return NULL;
}

gint
plug_ins_help_domains (Gimp    *gimp,
                       gchar ***help_domains,
                       gchar ***help_uris)
{
  GSList *list;
  gint    n_domains;
  gint    i;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), 0);
  g_return_val_if_fail (help_domains != NULL, 0);
  g_return_val_if_fail (help_uris != NULL, 0);

  n_domains = g_slist_length (gimp->plug_in_help_domains);

  *help_domains = g_new0 (gchar *, n_domains);
  *help_uris    = g_new0 (gchar *, n_domains);

  for (list = gimp->plug_in_help_domains, i = 0;
       list;
       list = g_slist_next (list), i++)
    {
      PlugInHelpDomainDef *def = list->data;

      (*help_domains)[i] = g_strdup (def->domain_name);
      (*help_uris)[i]    = g_strdup (def->domain_uri);
    }

  return n_domains;
}

PlugInProcDef *
plug_ins_proc_def_find (Gimp       *gimp,
                        ProcRecord *proc_rec)
{
  GSList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (proc_rec != NULL, NULL);

  for (list = gimp->plug_in_proc_defs; list; list = g_slist_next (list))
    {
      PlugInProcDef *proc_def;

      proc_def = (PlugInProcDef *) list->data;

      if (proc_rec == &proc_def->db_info)
        return proc_def;
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
              g_warning ("image_type contains unrecognizable parts: '%s'",
                         type_spec);

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


/*  private functions  */

static void
plug_ins_init_file (const GimpDatafileData *file_data,
                    gpointer                user_data)
{
  PlugInDef  *plug_in_def;
  GSList    **plug_in_defs;
  GSList     *list;

  plug_in_defs = (GSList **) user_data;

  for (list = *plug_in_defs; list; list = g_slist_next (list))
    {
      gchar *plug_in_name;

      plug_in_def = (PlugInDef *) list->data;

      plug_in_name = g_path_get_basename (plug_in_def->prog);

      if (g_ascii_strcasecmp (file_data->basename, plug_in_name) == 0)
	{
	  g_printerr ("skipping duplicate plug-in: '%s'\n",
                      gimp_filename_to_utf8 (file_data->filename));

          g_free (plug_in_name);

	  return;
	}

      g_free (plug_in_name);
    }

  plug_in_def = plug_in_def_new (file_data->filename);

  plug_in_def_set_mtime (plug_in_def, file_data->mtime);
  plug_in_def_set_needs_query (plug_in_def, TRUE);

  *plug_in_defs = g_slist_append (*plug_in_defs, plug_in_def);
}

static void
plug_ins_add_to_db (Gimp *gimp)
{
  PlugInProcDef *proc_def;
  GSList        *list;

  for (list = gimp->plug_in_proc_defs; list; list = g_slist_next (list))
    {
      proc_def = (PlugInProcDef *) list->data;

      if (proc_def->prog && (proc_def->db_info.proc_type != GIMP_INTERNAL))
	{
	  proc_def->db_info.exec_method.plug_in.filename = proc_def->prog;
	  procedural_db_register (gimp, &proc_def->db_info);
	}
    }

  for (list = gimp->plug_in_proc_defs; list; list = list->next)
    {
      proc_def = (PlugInProcDef *) list->data;

      if (proc_def->extensions || proc_def->prefixes || proc_def->magics)
        {
          Argument  args[4];
          Argument *return_vals;

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

static PlugInProcDef *
plug_ins_proc_def_insert (Gimp          *gimp,
                          PlugInProcDef *proc_def)
{
  GSList *list;
  GSList *prev = NULL;

  for (list = gimp->plug_in_proc_defs; list; list = g_slist_next (list))
    {
      PlugInProcDef *tmp_proc_def;

      tmp_proc_def = (PlugInProcDef *) list->data;

      if (strcmp (proc_def->db_info.name, tmp_proc_def->db_info.name) == 0)
	{
	  list->data = proc_def;

	  if (proc_def->menu_path)
	    g_free (proc_def->menu_path);
	  if (proc_def->accelerator)
	    g_free (proc_def->accelerator);

	  proc_def->menu_path   = tmp_proc_def->menu_path;
	  proc_def->accelerator = tmp_proc_def->accelerator;

	  tmp_proc_def->menu_path   = NULL;
	  tmp_proc_def->accelerator = NULL;

	  return tmp_proc_def;
	}
      else if (! proc_def->menu_path ||
	       (tmp_proc_def->menu_path &&
		(strcmp (proc_def->menu_path, tmp_proc_def->menu_path) < 0)))
	{
          GSList *new;

	  new = g_slist_alloc ();
	  new->data = proc_def;

	  new->next = list;
	  if (prev)
	    prev->next = new;
	  else
	    gimp->plug_in_proc_defs = new;

	  return NULL;
	}

      prev = list;
    }

  gimp->plug_in_proc_defs = g_slist_append (gimp->plug_in_proc_defs, proc_def);

  return NULL;
}
