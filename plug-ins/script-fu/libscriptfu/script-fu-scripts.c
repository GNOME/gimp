/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <glib.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <libgimp/gimp.h>

#include "tinyscheme/scheme-private.h"

#include "script-fu-lib.h"
#include "script-fu-types.h"
#include "script-fu-script.h"
#include "script-fu-scripts.h"
#include "script-fu-utils.h"
#include "script-fu-register.h"
#include "script-fu-command.h"

#include "script-fu-intl.h"


/*
 *  Local Functions
 */

static void             script_fu_load_directory (GFile                *directory);
static void             script_fu_load_script    (GFile                *file);
static gboolean         script_fu_install_script (gpointer              foo,
                                                  GList                *scripts,
                                                  gpointer              data);
static void             script_fu_install_menu   (SFMenu               *menu);
static gboolean         script_fu_remove_script  (gpointer              foo,
                                                  GList                *scripts,
                                                  gpointer              data);

static gchar          * script_fu_menu_map       (const gchar          *menu_path);
static gint             script_fu_menu_compare   (gconstpointer         a,
                                                  gconstpointer         b);

static void             script_fu_try_map_menu           (SFScript     *script);
static void             script_fu_append_script_to_tree  (SFScript     *script);

/*
 *  Local variables
 */

static GTree *script_tree      = NULL;
static GList *script_menu_list = NULL;


/*
 *  Function definitions
 */



/* A method on the internal tree of scripts.
 *
 * Uninstall any PDB procedures declared by the scripts,
 * and free the tree of scripts.
 *
 * For some phases of the plugin protocol
 * the PDB procedures declared by the scripts were not installed.
 * In other words, uninstall is a try that may have no effect.
 */
static void
script_fu_scripts_clear_tree ( GimpPlugIn *plug_in)
{
  if (script_tree != NULL)
    {
      g_tree_foreach (script_tree,
                      (GTraverseFunc) script_fu_remove_script,
                      plug_in);
      g_tree_destroy (script_tree);
    }
}


/* Traverse list of paths, finding .scm files.
 * Load and eval any found script texts.
 * Script texts will call ScriptFu registration functions
 * e.g. script-fu-register and script-fu-menu-register,
 * which insert a SFScript record into script_tree,
 * and insert a SFMenu record into script_menu_list.
 * These are side effects on the state of the outer (SF) interpreter.
 *
 * Return the tree of scripts, as well as keeping a local pointer to the tree.
 * The other result (script_menu_list) is not returned, see script_fu_get_menu_list.
 *
 * Caller should free script_tree and script_menu_list,
 * but we usually don't, the interpreter just exits.
 *
 * This can be called more than once but does not accumulate into the tree:
 * it clears the tree on every call before reloading it.
 *
 * When we load plugin script files (.scm)
 * the list of paths can be just one directory,
 * a plugin's subdir of /plug-ins (independent interpreter).
 * The plugin manager only queries one .scm file,
 * having the same name as its parent dir and and having execute permission.
 *
 * But the list of paths can be many:
 * the sys and user /scripts dirs(extension-script-fu).
 *
 * Any dir in the paths may contain many .scm files.
 * We read all the .scm files in the directory.
 * Each .scm file may register (and define run func for) many PDB procedures.
 */
GTree *
script_fu_scripts_load_into_tree ( GimpPlugIn *plug_in,
                                   GList      *paths)
{
  script_fu_scripts_clear_tree (plug_in);

  script_tree = g_tree_new ((GCompareFunc) g_utf8_collate);

  if (paths)
    {
      GList *list;

      for (list = paths; list; list = g_list_next (list))
        {
          script_fu_load_directory (list->data);
        }
    }

  /*
   * Assert result is not NULL, but may be an empty tree.
   * When paths is NULL, or no scripts found at paths.
   */

  g_debug ("script_fu_find_scripts_into_tree found %i scripts", g_tree_nnodes (script_tree));
  return script_tree;
}

/*
 * Return list of SFMenu for recently loaded scripts.
 * List is non-empty only after a call to script_fu_find_scripts_into_tree.
 */
GList *
script_fu_get_menu_list (void)
{
  return script_menu_list;
}

/* Find scripts, create and install TEMPORARY PDB procedures,
 * owned by self PDB procedure (e.g. extension-script-fu.)
 */
void
script_fu_find_scripts (GimpPlugIn *plug_in,
                        GList      *path)
{
  script_fu_scripts_load_into_tree (plug_in, path);

  /*  Now that all scripts are read in and sorted, tell gimp about them  */
  g_tree_foreach (script_tree,
                  (GTraverseFunc) script_fu_install_script,
                  plug_in);

  script_menu_list = g_list_sort (script_menu_list,
                                  (GCompareFunc) script_fu_menu_compare);

  /*  Install and nuke the list of menu entries  */
  g_list_free_full (script_menu_list,
                    (GDestroyNotify) script_fu_install_menu);
  script_menu_list = NULL;
}



/* For a script's call to script-fu-register.
 * Traverse Scheme argument list creating a new SFScript
 * whose drawable_arity is SF_PROC_ORDINARY.
 *
 * Return NIL or a foreign_error
 */
pointer
script_fu_add_script (scheme  *sc,
                      pointer  a)
{
  SFScript    *script;
  pointer      args_error;

  /*  Check metadata args args are present */
  if (sc->vptr->list_length (sc, a) < 7)
    return foreign_error (sc, "script-fu-register: Not enough arguments", 0);

  /* pass handle to pointer into script (on the stack) */
  script = script_fu_script_new_from_metadata_args (sc, &a);

  /* Require drawable_arity defaults to SF_PROC_ORDINARY.
   * script-fu-register specifies an ordinary GimpProcedure.
   * We may go on to infer a different arity.
   */
  g_assert (script->drawable_arity == SF_NO_DRAWABLE);

  args_error = script_fu_script_create_formal_args (sc, &a, script);
  if (args_error != sc->NIL)
    return args_error;

  /*  fill all values from defaults  */
  script_fu_script_reset (script, TRUE);

  /* Infer whether the script really requires one drawable,
   * so that later we can set the sensitivity.
   * For backward compatibility:
   * v2 script-fu-register does not require author to declare drawable arity.
   */
  script_fu_script_infer_drawable_arity (script);

  script->proc_class = GIMP_TYPE_PROCEDURE;

  script_fu_script_set_is_old_style (script);

  script_fu_try_map_menu (script);
  script_fu_append_script_to_tree (script);
  return sc->NIL;
}

/* For a script's call to script-fu-register-filter.
 * Traverse Scheme argument list creating a new SFScript
 * whose drawable_arity is SF_PROC_IMAGE_MULTIPLE_DRAWABLE or
 * SF_PROC_IMAGE_SINGLE_DRAWABLE
 *
 * Same as script-fu-register, except one more arg for drawable_arity.
 *
 * Return NIL or a foreign_error
 */
pointer
script_fu_add_script_filter (scheme  *sc,
                             pointer  a)
{
  SFScript    *script;
  pointer      args_error;  /* a foreign_error or NIL. */

  /* Check metadata args args are present.
   * Has one more arg than script-fu-register.
   */
  if (sc->vptr->list_length (sc, a) < 8)
    return foreign_error (sc, "script-fu-register-filter: Not enough arguments", 0);

  /* pass handle i.e. "&a" ("a" of type "pointer" is on the stack) */
  script = script_fu_script_new_from_metadata_args (sc, &a);

  /* Check semantic error: a script declaring it takes an image must specify
   * image types.  Otherwise the script's menu item will be enabled
   * even when no images exist.
   */
  if (g_strcmp0(script->image_types, "")==0)
    return foreign_error (sc, "script-fu-register-filter: A filter must declare image types.", 0);

  args_error = script_fu_script_parse_drawable_arity_arg (sc, &a, script);
  if (args_error != sc->NIL)
      return args_error;

  args_error = script_fu_script_create_formal_args (sc, &a, script);
  if (args_error != sc->NIL)
      return args_error;

  script->proc_class = GIMP_TYPE_IMAGE_PROCEDURE;

  script_fu_try_map_menu (script);
  script_fu_append_script_to_tree (script);
  return sc->NIL;
}

/* For a script's call to script-fu-register-procedure.
 * Traverse Scheme argument list creating a new SFScript
 * whose drawable_arity is SF_NO_DRAWABLE
 *
 * Return NIL or a foreign_error
 */
pointer
script_fu_add_script_regular (scheme  *sc,
                             pointer  a)
{
  SFScript    *script;
  pointer      args_error;  /* a foreign_error or NIL. */

  /* Check metadata args args are present.
   * Has two less arg than script-fu-register.
   * Last metadata arg is "copyright date"
   */
  if (sc->vptr->list_length (sc, a) < 5)
    return foreign_error (sc, "script-fu-register-filter: Not enough arguments", 0);

  /* pass handle i.e. "&a" ("a" of type "pointer" is on the stack) */
  script = script_fu_script_new_from_metadata_regular (sc, &a);

  script_fu_script_set_drawable_arity_none (script);

  /* Parse the args to this function that are formal declarations of
   * the args to the PDB procedure being defined.
   */
  args_error = script_fu_script_create_formal_args (sc, &a, script);
  if (args_error != sc->NIL)
      return args_error;

  script->proc_class = GIMP_TYPE_PROCEDURE;

  script_fu_try_map_menu (script);
  script_fu_append_script_to_tree (script);
  return sc->NIL;
}

pointer
script_fu_add_menu (scheme  *sc,
                    pointer  a)
{
  SFScript    *script;
  SFMenu      *menu;
  const gchar *name;
  const gchar *path;

  /*  Check the length of a  */
  if (sc->vptr->list_length (sc, a) != 2)
    return foreign_error (sc, "Incorrect number of arguments for script-fu-menu-register", 0);

  /*  Find the script PDB entry name  */
  name = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);

  script = script_fu_find_script (name);

  if (! script)
    {
      g_message ("Procedure %s in script-fu-menu-register does not exist",
                 name);
      return sc->NIL;
    }

  /*  Create a new list of menus  */
  menu = g_slice_new0 (SFMenu);

  menu->script = script;

  /*  Find the script menu path  */
  path = sc->vptr->string_value (sc->vptr->pair_car (a));

  menu->menu_path = script_fu_menu_map (path);

  if (! menu->menu_path)
    menu->menu_path = g_strdup (path);

  script_menu_list = g_list_prepend (script_menu_list, menu);

  return sc->NIL;
}

/* For a call to script-fu-register-i18n,
 * marshall scheme values into local SFScript struct.
 *
 * Returns sc->NIL on success, else a foreign_error.
 * Many kinds of failure will not prevent the plugin from registering and working,
 * only prevent the plugin from being translated properly.
 *
 * Although the set_18n callback is called many times in the same interpreter session,
 * before create proc and run proc, the script is only interpreted for it's registrations once.
 * However, an ill-formed script can call script-fu-register-i18n
 * many times for the same procedure, see below.
 */
pointer
script_fu_add_i18n (scheme  *sc,
                    pointer  a)
{
  SFScript    *script;
  const gchar *proc_name;

  gchar *i18n_domain                = NULL;
  gchar *i18n_catalog_relative_path = NULL;

  g_debug ("%s", G_STRFUNC);

  /*  Check arg count  */
  if (sc->vptr->list_length (sc, a) < 2)
    return foreign_error (sc, "script-fu-register-i18n takes two or three args", 0);

  /* PDB procedure name. */
  if (sc->vptr->is_string (sc->vptr->pair_car (a)))
    {
      proc_name = sc->vptr->string_value (sc->vptr->pair_car (a));
      a = sc->vptr->pair_cdr (a);
    }
  else
    {
      return foreign_error (sc, "script-fu-register-i18n requires first arg is string script name", 0);
    }

  script = script_fu_find_script (proc_name);

  if (! script)
    return foreign_error (sc, "script-fu-register-i18n called with invalid procedure name", 0);

  /* Not an error to interpret script-fu-register-i18n twice for the same procedure.
   * When there are two calls to script-fu-register-i18n for the same procedure
   * in one script, the latter will have effect.
   */
  if (script->i18n_domain_name != NULL || script->i18n_catalog_relative_path != NULL)
    g_warning ("%s called twice for same procedure %s", G_STRFUNC, proc_name);

  /*  i18n domain name  */
  if (sc->vptr->is_string (sc->vptr->pair_car (a)))
    {
      i18n_domain = sc->vptr->string_value (sc->vptr->pair_car (a));
      a = sc->vptr->pair_cdr (a);
    }
  else
    {
      return foreign_error (sc, "script-fu-register-i18n requires second arg is string domain name", 0);
    }

  /* optional catalog path */
  if (a != sc->NIL)
    {
      if (sc->vptr->is_string (sc->vptr->pair_car (a)))
        {
          i18n_catalog_relative_path = sc->vptr->string_value (sc->vptr->pair_car (a));
          a = sc->vptr->pair_cdr (a);
        }
      else
        {
          return foreign_error (sc, "script-fu-register-i18n requires optional third arg is catalog path", 0);
        }
    }

  /* Call setter from local vars, strings owned by inner interpreter TS. */
  script_fu_script_set_i18n (script, i18n_domain, i18n_catalog_relative_path);

  return sc->NIL; /* success */
}

/* Have one or more SFScript (global data structs) been created?
 * i.e. one or more script files loaded, i.e. interpreted for their registration functions.
 * Returns a state of the interpreter.
 */
gboolean
script_fu_scripts_are_loaded (void)
{
  return (script_tree != NULL);
}

/*  private functions  */

/* Load scripts from a directory tree.
 * Recursively, descending into subdirectories.
 * Only loads terminal files with suffix .scm.
 * Not all such files need be plugin scripts.
 * Does not load or traverse hidden files and directories
 * and directories named like an init directory.
 * ScriptFu separately loads certain files in init directories.
 */
static void
script_fu_load_directory (GFile *directory)
{
  GFileEnumerator *enumerator;

  g_debug ("Load dir: %s", g_file_get_parse_name (directory));

  enumerator = g_file_enumerate_children (directory,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);

  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFileType file_type = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE);

          if ((file_type == G_FILE_TYPE_REGULAR ||
               file_type == G_FILE_TYPE_DIRECTORY) &&
              ! g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN))
            {
              GFile *child = g_file_enumerator_get_child (enumerator, info);

              if (file_type == G_FILE_TYPE_DIRECTORY)
                {
                  /* Skip any init subdirectory.
                   * It has scripts already loaded, and scripts we don't want loaded.
                   */
                  if (! script_fu_is_init_directory (child))
                    /* Recursive! */
                    script_fu_load_directory (child);
                }
              else
                {
                  /* This will only load a .scm file. */
                  script_fu_load_script (child);
                }

              g_object_unref (child);
            }

          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }
}

static void
script_fu_load_script (GFile *file)
{
  if (gimp_file_has_extension (file, ".scm"))
    {
      gchar  *path    = g_file_get_path (file);
      gchar  *escaped = script_fu_strescape (path);
      gchar  *command;
      GError *error   = NULL;

      command = g_strdup_printf ("(load \"%s\")", escaped);
      g_free (escaped);

      if (! script_fu_run_command (command, &error))
        {
          gchar *message = g_strdup_printf (_("Error while loading %s:"),
                                            gimp_file_get_utf8_name (file));

          g_message ("%s\n\n%s", message, error->message);

          g_clear_error (&error);
          g_free (message);
        }

#ifdef G_OS_WIN32
      /* No, I don't know why, but this is
       * necessary on NT 4.0.
       */
      Sleep (0);
#endif

      g_free (command);
      g_free (path);
    }
}

/* This is-a GTraverseFunction.
 *
 * Traverse.  For each, install TEMPORARY PDB proc.
 * Returning FALSE means entire list was traversed.
 */
static gboolean
script_fu_install_script (gpointer  foo G_GNUC_UNUSED,
                          GList    *scripts,
                          gpointer  data)
{
  GimpPlugIn *plug_in = data;
  GList      *list;

  for (list = scripts; list; list = g_list_next (list))
    {
      SFScript *script = list->data;

      const gchar* name = script->name;
      if (script_fu_is_defined (name))
        script_fu_script_install_proc (plug_in, script);
      else
        g_warning ("Run function not defined, or does not match PDB procedure name: %s", name);
    }

  return FALSE;
}

static void
script_fu_install_menu (SFMenu *menu)
{
  GimpPlugIn    *plug_in   = gimp_get_plug_in ();
  GimpProcedure *procedure = NULL;

  procedure = gimp_plug_in_get_temp_procedure (plug_in,
                                               menu->script->name);

  if (procedure)
    gimp_procedure_add_menu_path (procedure, menu->menu_path);

  g_free (menu->menu_path);
  g_slice_free (SFMenu, menu);
}

/* Traverse list of scripts, uninstalling from PDB and
 * freeing the script data.
 * Then free the list, now empty of content.
 *
 * This function has type GTraverseFunction.
 */
static gboolean
script_fu_remove_script (gpointer  foo G_GNUC_UNUSED,
                         GList    *scripts,
                         gpointer  data)
{
  GimpPlugIn *plug_in = data;
  GList      *list;

  g_debug ("%s", G_STRFUNC);

  for (list = scripts; list; list = g_list_next (list))
    {
      SFScript *script = list->data;

      script_fu_script_uninstall_proc (plug_in, script);
      script_fu_script_free (script);
    }

  g_list_free (scripts);

  return FALSE;
}



/* this is a GTraverseFunction */
static gboolean
script_fu_lookup_script (gpointer      *foo G_GNUC_UNUSED,
                         GList         *scripts,
                         gconstpointer *name)
{
  GList *list;

  for (list = scripts; list; list = g_list_next (list))
    {
      SFScript *script = list->data;

      if (strcmp (script->name, *name) == 0)
        {
          /* store the script in the name pointer and stop the traversal */
          *name = script;
          return TRUE;
        }
    }

  return FALSE;
}

SFScript *
script_fu_find_script (const gchar *name)
{
  gconstpointer script = name;

  g_tree_foreach (script_tree,
                  (GTraverseFunc) script_fu_lookup_script,
                  &script);

  if (script == name)
    return NULL;

  return (SFScript *) script;
}

static gchar *
script_fu_menu_map (const gchar *menu_path)
{
  /*  for backward compatibility, we fiddle with some menu paths  */
  const struct
  {
    const gchar *old;
    const gchar *new;
  } mapping[] = {
    { "<Image>/Script-Fu/Alchemy",       "<Image>/Filters/Artistic"            },
    { "<Image>/Script-Fu/Alpha to Logo", "<Image>/Filters/Alpha to Logo"       },
    { "<Image>/Script-Fu/Animators",     "<Image>/Filters/Animation" },
    { "<Image>/Script-Fu/Decor",         "<Image>/Filters/Decor"               },
    { "<Image>/Script-Fu/Render",        "<Image>/Filters/Render"              },
    { "<Image>/Script-Fu/Selection",     "<Image>/Select/Modify"               },
    { "<Image>/Script-Fu/Shadow",        "<Image>/Filters/Light and Shadow/[Shadow]" },
    { "<Image>/Script-Fu/Stencil Ops",   "<Image>/Filters/Decor"               }
  };

  gint i;

  for (i = 0; i < G_N_ELEMENTS (mapping); i++)
    {
      if (g_str_has_prefix (menu_path, mapping[i].old))
        {
          const gchar *suffix = menu_path + strlen (mapping[i].old);

          if (*suffix != '/')
            continue;

          return g_strconcat (mapping[i].new, suffix, NULL);
        }
    }

  return NULL;
}

static gint
script_fu_menu_compare (gconstpointer a,
                        gconstpointer b)
{
  const SFMenu *menu_a = a;
  const SFMenu *menu_b = b;
  gint          retval = 0;

  if (menu_a->menu_path && menu_b->menu_path)
    {
      retval = g_utf8_collate (menu_a->menu_path,
                               menu_b->menu_path);

      if (retval == 0 &&
          menu_a->script->menu_label && menu_b->script->menu_label)
        {
          retval = g_utf8_collate (menu_a->script->menu_label,
                                   menu_b->script->menu_label);
        }
    }

  return retval;
}

/* Is name a defined symbol in the interpreter state?
 * (Defined in any script already loaded.)
 * Where "symbol" has the usual lisp meaning: a unique name associated with
 * a variable or function.
 *
 * The most common use is
 * test the name of a PDB proc, which in ScriptFu must match
 * a defined function that is the inner run function.
 * I.E. check for typos by author of script.
 * Used during query, to preflight so that we don't install a PDB proc
 * that won't run later (during the run phase)
 * giving "undefined symbol" for extension-script-fu.
 * Note that if instead we create a PDB proc having no defined run func,
 * script-fu-interpreter would load and define a same-named scheme function
 * that calls the PDB, and can enter an infinite loop.
 */
gboolean
script_fu_is_defined (const gchar * name)
{
  gchar   *scheme_text;
  GError  *error = NULL;
  gboolean result;

  /* text to be interpreted is a call to an internal scheme function. */
  scheme_text = g_strdup_printf (" (symbol? %s ) ",  name);

  /* Use script_fu_run_command, it correctly handles the string yielded.
   * But we don't need the string yielded.
   * If defined, string yielded is "#t", else is "Undefined symbol" or "#f"
   */
  result = script_fu_run_command (scheme_text, &error);
  if (!result)
    {
      g_debug ("script_fu_is_defined returns false");
      /* error contains string yielded by interpretation. */
      g_error_free (error);
    }
  return result;
}


/* Side effects on script. */
static void
script_fu_try_map_menu (SFScript *script)
{
  if (script->menu_label[0] == '<')
    {
      gchar *mapped = script_fu_menu_map (script->menu_label);

      if (mapped)
        {
          g_free (script->menu_label);
          script->menu_label = mapped;
        }
    }
}

/* Append to ordered tree.
 * Side effects on script_tree.
 */
static void
script_fu_append_script_to_tree (SFScript *script)
{
  GList *list = g_tree_lookup (script_tree, script->menu_label);

  g_tree_insert (script_tree, (gpointer) script->menu_label,
                  g_list_append (list, script));
}
