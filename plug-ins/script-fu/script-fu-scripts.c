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
#include <libgimp/gimpui.h>

#include "tinyscheme/scheme-private.h"

#include "scheme-wrapper.h"

#include "script-fu-types.h"

#include "script-fu-interface.h"
#include "script-fu-script.h"
#include "script-fu-scripts.h"
#include "script-fu-utils.h"

#include "script-fu-intl.h"


typedef struct
{
  SFScript *script;
  gchar    *menu_path;
} SFMenu;


/*
 *  Local Functions
 */

static gboolean  script_fu_run_command    (const gchar      *command,
                                           GError          **error);
static void      script_fu_load_directory (GFile            *directory);
static void      script_fu_load_script    (GFile            *file);
static gboolean  script_fu_install_script (gpointer          foo,
                                           GList            *scripts,
                                           gpointer          bar);
static void      script_fu_install_menu   (SFMenu           *menu);
static gboolean  script_fu_remove_script  (gpointer          foo,
                                           GList            *scripts,
                                           gpointer          bar);
static void      script_fu_script_proc    (const gchar      *name,
                                           gint              nparams,
                                           const GimpParam  *params,
                                           gint             *nreturn_vals,
                                           GimpParam       **return_vals);

static SFScript *script_fu_find_script    (const gchar      *name);

static gchar *   script_fu_menu_map       (const gchar      *menu_path);
static gint      script_fu_menu_compare   (gconstpointer     a,
                                           gconstpointer     b);


/*
 *  Local variables
 */

static GTree *script_tree      = NULL;
static GList *script_menu_list = NULL;


/*
 *  Function definitions
 */

void
script_fu_find_scripts (GList *path)
{
  GList *list;

  /*  Make sure to clear any existing scripts  */
  if (script_tree != NULL)
    {
      g_tree_foreach (script_tree,
                      (GTraverseFunc) script_fu_remove_script,
                      NULL);
      g_tree_destroy (script_tree);
    }

  if (! path)
    return;

  script_tree = g_tree_new ((GCompareFunc) g_utf8_collate);

  for (list = path; list; list = g_list_next (list))
    {
      script_fu_load_directory (list->data);
    }

  /*  Now that all scripts are read in and sorted, tell gimp about them  */
  g_tree_foreach (script_tree,
                  (GTraverseFunc) script_fu_install_script,
                  NULL);

  script_menu_list = g_list_sort (script_menu_list,
                                  (GCompareFunc) script_fu_menu_compare);

  /*  Install and nuke the list of menu entries  */
  g_list_free_full (script_menu_list,
                    (GDestroyNotify) script_fu_install_menu);
  script_menu_list = NULL;
}

pointer
script_fu_add_script (scheme  *sc,
                      pointer  a)
{
  SFScript    *script;
  const gchar *name;
  const gchar *menu_label;
  const gchar *blurb;
  const gchar *author;
  const gchar *copyright;
  const gchar *date;
  const gchar *image_types;
  gint         n_args;
  gint         i;

  /*  Check the length of a  */
  if (sc->vptr->list_length (sc, a) < 7)
    {
      g_message (_("Too few arguments to 'script-fu-register' call"));
      return sc->NIL;
    }

  /*  Find the script name  */
  name = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);

  /*  Find the script menu_label  */
  menu_label = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);

  /*  Find the script blurb  */
  blurb = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);

  /*  Find the script author  */
  author = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);

  /*  Find the script copyright  */
  copyright = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);

  /*  Find the script date  */
  date = sc->vptr->string_value (sc->vptr->pair_car (a));
  a = sc->vptr->pair_cdr (a);

  /*  Find the script image types  */
  if (sc->vptr->is_pair (a))
    {
      image_types = sc->vptr->string_value (sc->vptr->pair_car (a));
      a = sc->vptr->pair_cdr (a);
    }
  else
    {
      image_types = sc->vptr->string_value (a);
      a = sc->NIL;
    }

  /*  Check the supplied number of arguments  */
  n_args = sc->vptr->list_length (sc, a) / 3;

  /*  Create a new script  */
  script = script_fu_script_new (name,
                                 menu_label,
                                 blurb,
                                 author,
                                 copyright,
                                 date,
                                 image_types,
                                 n_args);

  for (i = 0; i < script->n_args; i++)
    {
      SFArg *arg = &script->args[i];

      if (a != sc->NIL)
        {
          if (!sc->vptr->is_integer (sc->vptr->pair_car (a)))
            return foreign_error (sc, "script-fu-register: argument types must be integer values", 0);

          arg->type = sc->vptr->ivalue (sc->vptr->pair_car (a));
          a = sc->vptr->pair_cdr (a);
        }
      else
        return foreign_error (sc, "script-fu-register: missing type specifier", 0);

      if (a != sc->NIL)
        {
          if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
            return foreign_error (sc, "script-fu-register: argument labels must be strings", 0);

          arg->label = g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
          a = sc->vptr->pair_cdr (a);
        }
      else
        return foreign_error (sc, "script-fu-register: missing arguments label", 0);

      if (a != sc->NIL)
        {
          switch (arg->type)
            {
            case SF_IMAGE:
            case SF_DRAWABLE:
            case SF_LAYER:
            case SF_CHANNEL:
            case SF_VECTORS:
            case SF_DISPLAY:
              if (!sc->vptr->is_integer (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: default IDs must be integer values", 0);

              arg->default_value.sfa_image =
                sc->vptr->ivalue (sc->vptr->pair_car (a));
              break;

            case SF_COLOR:
              if (sc->vptr->is_string (sc->vptr->pair_car (a)))
                {
                  if (! gimp_rgb_parse_css (&arg->default_value.sfa_color,
                                            sc->vptr->string_value (sc->vptr->pair_car (a)),
                                            -1))
                    return foreign_error (sc, "script-fu-register: invalid default color name", 0);

                  gimp_rgb_set_alpha (&arg->default_value.sfa_color, 1.0);
                }
              else if (sc->vptr->is_list (sc, sc->vptr->pair_car (a)) &&
                       sc->vptr->list_length(sc, sc->vptr->pair_car (a)) == 3)
                {
                  pointer color_list;
                  guchar  r, g, b;

                  color_list = sc->vptr->pair_car (a);
                  r = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)), 0, 255);
                  color_list = sc->vptr->pair_cdr (color_list);
                  g = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)), 0, 255);
                  color_list = sc->vptr->pair_cdr (color_list);
                  b = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)), 0, 255);

                  gimp_rgb_set_uchar (&arg->default_value.sfa_color, r, g, b);
                }
              else
                {
                  return foreign_error (sc, "script-fu-register: color defaults must be a list of 3 integers or a color name", 0);
                }
              break;

            case SF_TOGGLE:
              if (!sc->vptr->is_integer (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: toggle default must be an integer value", 0);

              arg->default_value.sfa_toggle =
                (sc->vptr->ivalue (sc->vptr->pair_car (a))) ? TRUE : FALSE;
              break;

            case SF_VALUE:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: value defaults must be string values", 0);

              arg->default_value.sfa_value =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_STRING:
            case SF_TEXT:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: string defaults must be string values", 0);

              arg->default_value.sfa_value =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_ADJUSTMENT:
              {
                pointer adj_list;

                if (!sc->vptr->is_list (sc, a))
                  return foreign_error (sc, "script-fu-register: adjustment defaults must be a list", 0);

                adj_list = sc->vptr->pair_car (a);
                arg->default_value.sfa_adjustment.value =
                  sc->vptr->rvalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.lower =
                  sc->vptr->rvalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.upper =
                  sc->vptr->rvalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.step =
                  sc->vptr->rvalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.page =
                  sc->vptr->rvalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.digits =
                  sc->vptr->ivalue (sc->vptr->pair_car (adj_list));

                adj_list = sc->vptr->pair_cdr (adj_list);
                arg->default_value.sfa_adjustment.type =
                  sc->vptr->ivalue (sc->vptr->pair_car (adj_list));
              }
              break;

            case SF_FILENAME:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: filename defaults must be string values", 0);
              /* fallthrough */

            case SF_DIRNAME:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: dirname defaults must be string values", 0);

              arg->default_value.sfa_file.filename =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));

#ifdef G_OS_WIN32
              {
                /* Replace POSIX slashes with Win32 backslashes. This
                 * is just so script-fus can be written with only
                 * POSIX directory separators.
                 */
                gchar *filename = arg->default_value.sfa_file.filename;

                while (*filename)
                  {
                    if (*filename == '/')
                      *filename = G_DIR_SEPARATOR;

                    filename++;
                  }
              }
#endif
              break;

            case SF_FONT:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: font defaults must be string values", 0);

              arg->default_value.sfa_font =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_PALETTE:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: palette defaults must be string values", 0);

              arg->default_value.sfa_palette =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_PATTERN:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: pattern defaults must be string values", 0);

              arg->default_value.sfa_pattern =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_BRUSH:
              {
                pointer brush_list;

                if (!sc->vptr->is_list (sc, a))
                  return foreign_error (sc, "script-fu-register: brush defaults must be a list", 0);

                brush_list = sc->vptr->pair_car (a);
                arg->default_value.sfa_brush.name =
                  g_strdup (sc->vptr->string_value (sc->vptr->pair_car (brush_list)));

                brush_list = sc->vptr->pair_cdr (brush_list);
                arg->default_value.sfa_brush.opacity =
                  sc->vptr->rvalue (sc->vptr->pair_car (brush_list));

                brush_list = sc->vptr->pair_cdr (brush_list);
                arg->default_value.sfa_brush.spacing =
                  sc->vptr->ivalue (sc->vptr->pair_car (brush_list));

                brush_list = sc->vptr->pair_cdr (brush_list);
                arg->default_value.sfa_brush.paint_mode =
                  sc->vptr->ivalue (sc->vptr->pair_car (brush_list));
              }
              break;

            case SF_GRADIENT:
              if (!sc->vptr->is_string (sc->vptr->pair_car (a)))
                return foreign_error (sc, "script-fu-register: gradient defaults must be string values", 0);

              arg->default_value.sfa_gradient =
                g_strdup (sc->vptr->string_value (sc->vptr->pair_car (a)));
              break;

            case SF_OPTION:
              {
                pointer option_list;

                if (!sc->vptr->is_list (sc, a))
                  return foreign_error (sc, "script-fu-register: option defaults must be a list", 0);

                for (option_list = sc->vptr->pair_car (a);
                     option_list != sc->NIL;
                     option_list = sc->vptr->pair_cdr (option_list))
                  {
                    arg->default_value.sfa_option.list =
                      g_slist_append (arg->default_value.sfa_option.list,
                                      g_strdup (sc->vptr->string_value
                                                (sc->vptr->pair_car (option_list))));
                  }
              }
              break;

            case SF_ENUM:
              {
                pointer      option_list;
                const gchar *val;
                gchar       *type_name;
                GEnumValue  *enum_value;
                GType        enum_type;

                if (!sc->vptr->is_list (sc, a))
                  return foreign_error (sc, "script-fu-register: enum defaults must be a list", 0);

                option_list = sc->vptr->pair_car (a);
                if (!sc->vptr->is_string (sc->vptr->pair_car (option_list)))
                  return foreign_error (sc, "script-fu-register: first element in enum defaults must be a type-name", 0);

                val = sc->vptr->string_value (sc->vptr->pair_car (option_list));

                if (g_str_has_prefix (val, "Gimp"))
                  type_name = g_strdup (val);
                else
                  type_name = g_strconcat ("Gimp", val, NULL);

                enum_type = g_type_from_name (type_name);
                if (! G_TYPE_IS_ENUM (enum_type))
                  {
                    g_free (type_name);
                    return foreign_error (sc, "script-fu-register: first element in enum defaults must be the name of a registered type", 0);
                  }

                arg->default_value.sfa_enum.type_name = type_name;

                option_list = sc->vptr->pair_cdr (option_list);
                if (!sc->vptr->is_string (sc->vptr->pair_car (option_list)))
                  return foreign_error (sc, "script-fu-register: second element in enum defaults must be a string", 0);

                enum_value =
                  g_enum_get_value_by_nick (g_type_class_peek (enum_type),
                                            sc->vptr->string_value (sc->vptr->pair_car (option_list)));
                if (enum_value)
                  arg->default_value.sfa_enum.history = enum_value->value;
              }
              break;
            }

          a = sc->vptr->pair_cdr (a);
        }
      else
        {
          return foreign_error (sc, "script-fu-register: missing default argument", 0);
        }
    }

  /*  fill all values from defaults  */
  script_fu_script_reset (script, TRUE);

  if (script->menu_label[0] == '<')
    {
      gchar *mapped = script_fu_menu_map (script->menu_label);

      if (mapped)
        {
          g_free (script->menu_label);
          script->menu_label = mapped;
        }
    }

  {
    GList *list = g_tree_lookup (script_tree, script->menu_label);

    g_tree_insert (script_tree, (gpointer) script->menu_label,
                    g_list_append (list, script));
  }

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


/*  private functions  */

static gboolean
script_fu_run_command (const gchar  *command,
                       GError      **error)
{
  GString  *output;
  gboolean  success = FALSE;

  output = g_string_new (NULL);
  ts_register_output_func (ts_gstring_output_func, output);

  if (ts_interpret_string (command))
    {
      g_set_error (error, 0, 0, "%s", output->str);
    }
  else
    {
      success = TRUE;
    }

  g_string_free (output, TRUE);

  return success;
}

static void
script_fu_load_directory (GFile *directory)
{
  GFileEnumerator *enumerator;

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
          GFileType file_type = g_file_info_get_file_type (info);

          if ((file_type == G_FILE_TYPE_REGULAR ||
               file_type == G_FILE_TYPE_DIRECTORY) &&
              ! g_file_info_get_is_hidden (info))
            {
              GFile *child = g_file_enumerator_get_child (enumerator, info);

              if (file_type == G_FILE_TYPE_DIRECTORY)
                script_fu_load_directory (child);
              else
                script_fu_load_script (child);

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

/*
 *  The following function is a GTraverseFunction.
 */
static gboolean
script_fu_install_script (gpointer  foo G_GNUC_UNUSED,
                          GList    *scripts,
                          gpointer  bar G_GNUC_UNUSED)
{
  GList *list;

  for (list = scripts; list; list = g_list_next (list))
    {
      SFScript *script = list->data;

      script_fu_script_install_proc (script, script_fu_script_proc);
    }

  return FALSE;
}

static void
script_fu_install_menu (SFMenu *menu)
{
  gimp_plugin_menu_register (menu->script->name, menu->menu_path);

  g_free (menu->menu_path);
  g_slice_free (SFMenu, menu);
}

/*
 *  The following function is a GTraverseFunction.
 */
static gboolean
script_fu_remove_script (gpointer  foo G_GNUC_UNUSED,
                         GList    *scripts,
                         gpointer  bar G_GNUC_UNUSED)
{
  GList *list;

  for (list = scripts; list; list = g_list_next (list))
    {
      SFScript *script = list->data;

      script_fu_script_uninstall_proc (script);
      script_fu_script_free (script);
    }

  g_list_free (scripts);

  return FALSE;
}

static void
script_fu_script_proc (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *params,
                       gint             *nreturn_vals,
                       GimpParam       **return_vals)
{
  static GimpParam   values[2] = { { 0, }, { 0, } };
  GimpPDBStatusType  status    = GIMP_PDB_SUCCESS;
  SFScript          *script;
  GError            *error     = NULL;

  if (values[1].type == GIMP_PDB_STRING && values[1].data.d_string)
    {
      g_free (values[1].data.d_string);
      values[1].data.d_string = NULL;
    }

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type = GIMP_PDB_STATUS;

  script = script_fu_find_script (name);

  if (! script)
    status = GIMP_PDB_CALLING_ERROR;

  if (status == GIMP_PDB_SUCCESS)
    {
      GimpRunMode run_mode = params[0].data.d_int32;

      ts_set_run_mode (run_mode);

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          {
            gint min_args = 0;

            /*  First, try to collect the standard script arguments...  */
            min_args = script_fu_script_collect_standard_args (script,
                                                               nparams, params);

            /*  ...then acquire the rest of arguments (if any) with a dialog  */
            if (script->n_args > min_args)
              {
                status = script_fu_interface (script, min_args);
                break;
              }
            /*  otherwise (if the script takes no more arguments), skip
             *  this part and run the script directly (fallthrough)
             */
          }

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there  */
          if (nparams != (script->n_args + 1))
            status = GIMP_PDB_CALLING_ERROR;

          if (status == GIMP_PDB_SUCCESS)
            {
              gchar *command;

              command = script_fu_script_get_command_from_params (script,
                                                                  params);

              /*  run the command through the interpreter  */
              if (! script_fu_run_command (command, &error))
                {
                  status                  = GIMP_PDB_EXECUTION_ERROR;
                  *nreturn_vals           = 2;
                  values[1].type          = GIMP_PDB_STRING;
                  values[1].data.d_string = error->message;

                  error->message = NULL;
                  g_error_free (error);
                }

              g_free (command);
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          {
            gchar *command;

            /*  First, try to collect the standard script arguments  */
            script_fu_script_collect_standard_args (script, nparams, params);

            command = script_fu_script_get_command (script);

            /*  run the command through the interpreter  */
            if (! script_fu_run_command (command, &error))
              {
                status                  = GIMP_PDB_EXECUTION_ERROR;
                *nreturn_vals           = 2;
                values[1].type          = GIMP_PDB_STRING;
                values[1].data.d_string = error->message;

                error->message = NULL;
                g_error_free (error);
              }

            g_free (command);
          }
          break;

        default:
          break;
        }
    }

  values[0].data.d_status = status;
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

static SFScript *
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
    { "<Image>/Script-Fu/Animators",     "<Image>/Filters/Animation/Animators" },
    { "<Image>/Script-Fu/Decor",         "<Image>/Filters/Decor"               },
    { "<Image>/Script-Fu/Render",        "<Image>/Filters/Render"              },
    { "<Image>/Script-Fu/Selection",     "<Image>/Select/Modify"               },
    { "<Image>/Script-Fu/Shadow",        "<Image>/Filters/Light and Shadow/Shadow" },
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
