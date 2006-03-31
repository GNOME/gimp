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

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "pdb-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpchannel.h"
#include "core/gimplayer.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "vectors/gimpvectors.h"

#include "plug-in/plug-in-run.h"

#include "gimpprocedure.h"
#include "internal_procs.h"
#include "procedural_db.h"

#include "gimp-intl.h"


/*  public functions  */

void
procedural_db_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->procedural_ht        = g_hash_table_new (g_str_hash, g_str_equal);
  gimp->procedural_compat_ht = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
procedural_db_free_entry (gpointer key,
                          gpointer value,
                          gpointer user_data)
{
  if (value)
    {
      GList *list;

      for (list = value; list; list = g_list_next (list))
        {
          ProcRecord *procedure = list->data;
          gint        i;

          for (i = 0; i < procedure->num_args; i++)
            g_param_spec_unref (procedure->args[i].pspec);

          for (i = 0; i < procedure->num_values; i++)
            g_param_spec_unref (procedure->values[i].pspec);

          g_free (procedure->args);
          g_free (procedure->values);
        }

      g_list_free (value);
    }
}

void
procedural_db_free (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->procedural_ht)
    {
      g_hash_table_foreach (gimp->procedural_ht,
                            procedural_db_free_entry, NULL);
      g_hash_table_destroy (gimp->procedural_ht);
      gimp->procedural_ht = NULL;
    }

  if (gimp->procedural_compat_ht)
    {
      g_hash_table_destroy (gimp->procedural_compat_ht);
      gimp->procedural_compat_ht = NULL;
    }
}

void
procedural_db_init_procs (Gimp *gimp)
{
  static const struct
  {
    const gchar *old_name;
    const gchar *new_name;
  }
  compat_procs[] =
  {
    { "gimp-blend",                      "gimp-edit-blend"                 },
    { "gimp-brushes-list",               "gimp-brushes-get-list"           },
    { "gimp-bucket-fill",                "gimp-edit-bucket-fill"           },
    { "gimp-channel-delete",             "gimp-drawable-delete"            },
    { "gimp-channel-get-name",           "gimp-drawable-get-name"          },
    { "gimp-channel-get-tattoo",         "gimp-drawable-get-tattoo"        },
    { "gimp-channel-get-visible",        "gimp-drawable-get-visible"       },
    { "gimp-channel-set-name",           "gimp-drawable-set-name"          },
    { "gimp-channel-set-tattoo",         "gimp-drawable-set-tattoo"        },
    { "gimp-channel-set-visible",        "gimp-drawable-set-visible"       },
    { "gimp-color-picker",               "gimp-image-pick-color"           },
    { "gimp-convert-grayscale",          "gimp-image-convert-grayscale"    },
    { "gimp-convert-indexed",            "gimp-image-convert-indexed"      },
    { "gimp-convert-rgb",                "gimp-image-convert-rgb"          },
    { "gimp-crop",                       "gimp-image-crop"                 },
    { "gimp-drawable-bytes",             "gimp-drawable-bpp"               },
    { "gimp-drawable-image",             "gimp-drawable-get-image"         },
    { "gimp-image-active-drawable",      "gimp-image-get-active-drawable"  },
    { "gimp-image-floating-selection",   "gimp-image-get-floating-sel"     },
    { "gimp-layer-delete",               "gimp-drawable-delete"            },
    { "gimp-layer-get-linked",           "gimp-drawable-get-linked"        },
    { "gimp-layer-get-name",             "gimp-drawable-get-name"          },
    { "gimp-layer-get-tattoo",           "gimp-drawable-get-tattoo"        },
    { "gimp-layer-get-visible",          "gimp-drawable-get-visible"       },
    { "gimp-layer-mask",                 "gimp-layer-get-mask"             },
    { "gimp-layer-set-linked",           "gimp-drawable-set-linked"        },
    { "gimp-layer-set-name",             "gimp-drawable-set-name"          },
    { "gimp-layer-set-tattoo",           "gimp-drawable-set-tattoo"        },
    { "gimp-layer-set-visible",          "gimp-drawable-set-visible"       },
    { "gimp-palette-refresh",            "gimp-palettes-refresh"           },
    { "gimp-patterns-list",              "gimp-patterns-get-list"          },
    { "gimp-temp-PDB-name",              "gimp-procedural-db-temp-name"    },
    { "gimp-undo-push-group-end",        "gimp-image-undo-group-end"       },
    { "gimp-undo-push-group-start",      "gimp-image-undo-group-start"     },

    /*  deprecations since 2.0  */
    { "gimp-brushes-get-opacity",        "gimp-context-get-opacity"        },
    { "gimp-brushes-get-paint-mode",     "gimp-context-get-paint-mode"     },
    { "gimp-brushes-set-brush",          "gimp-context-set-brush"          },
    { "gimp-brushes-set-opacity",        "gimp-context-set-opacity"        },
    { "gimp-brushes-set-paint-mode",     "gimp-context-set-paint-mode"     },
    { "gimp-channel-ops-duplicate",      "gimp-image-duplicate"            },
    { "gimp-channel-ops-offset",         "gimp-drawable-offset"            },
    { "gimp-gradients-get-active",       "gimp-context-get-gradient"       },
    { "gimp-gradients-get-gradient",     "gimp-context-get-gradient"       },
    { "gimp-gradients-set-active",       "gimp-context-set-gradient"       },
    { "gimp-gradients-set-gradient",     "gimp-context-set-gradient"       },
    { "gimp-image-get-cmap",             "gimp-image-get-colormap"         },
    { "gimp-image-set-cmap",             "gimp-image-set-colormap"         },
    { "gimp-palette-get-background",     "gimp-context-get-background"     },
    { "gimp-palette-get-foreground",     "gimp-context-get-foreground"     },
    { "gimp-palette-set-background",     "gimp-context-set-background"     },
    { "gimp-palette-set-default-colors", "gimp-context-set-default-colors" },
    { "gimp-palette-set-foreground",     "gimp-context-set-foreground"     },
    { "gimp-palette-swap-colors",        "gimp-context-swap-colors"        },
    { "gimp-palettes-set-palette",       "gimp-context-set-palette"        },
    { "gimp-patterns-set-pattern",       "gimp-context-set-pattern"        },
    { "gimp-selection-clear",            "gimp-selection-none"             },

    /*  deprecations since 2.2  */
    { "gimp-layer-get-preserve-trans",   "gimp-drawable-get-lock-alpha"    },
    { "gimp-layer-set-preserve-trans",   "gimp-drawable-set-lock-alpha"    }
  };

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  internal_procs_init (gimp);

  if (gimp->pdb_compat_mode != GIMP_PDB_COMPAT_OFF)
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (compat_procs); i++)
        g_hash_table_insert (gimp->procedural_compat_ht,
                             (gpointer) compat_procs[i].old_name,
                             (gpointer) compat_procs[i].new_name);
    }
}

void
procedural_db_register (Gimp       *gimp,
                        ProcRecord *procedure)
{
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (procedure != NULL);

  list = g_hash_table_lookup (gimp->procedural_ht, procedure->name);

  g_hash_table_insert (gimp->procedural_ht,
                       procedure->name,
                       g_list_prepend (list, procedure));
}

void
procedural_db_unregister (Gimp        *gimp,
                          const gchar *name)
{
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (name != NULL);

  list = g_hash_table_lookup (gimp->procedural_ht, name);

  if (list)
    {
      list = g_list_remove (list, list->data);

      if (list)
        g_hash_table_insert (gimp->procedural_ht, (gpointer) name, list);
      else
        g_hash_table_remove (gimp->procedural_ht, name);
    }
}

ProcRecord *
procedural_db_lookup (Gimp        *gimp,
                      const gchar *name)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  list = g_hash_table_lookup (gimp->procedural_ht, name);

  if (list)
    return list->data;
  else
    return NULL;
}

Argument *
procedural_db_execute (Gimp         *gimp,
                       GimpContext  *context,
                       GimpProgress *progress,
                       const gchar  *name,
                       Argument     *args,
                       gint          n_args,
                       gint         *n_return_vals)
{
  Argument *return_vals = NULL;
  GList    *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (n_return_vals != NULL, NULL);

  list = g_hash_table_lookup (gimp->procedural_ht, name);

  if (list == NULL)
    {
      g_message (_("PDB calling error:\nprocedure '%s' not found"), name);

      return_vals = gimp_procedure_get_return_values (NULL, FALSE);
      g_value_set_enum (&return_vals->value, GIMP_PDB_CALLING_ERROR);

      *n_return_vals = 1;

      return return_vals;
    }

  for (; list; list = g_list_next (list))
    {
      ProcRecord *procedure = list->data;

      g_return_val_if_fail (procedure != NULL, NULL);

      return_vals = gimp_procedure_execute (procedure,
                                            gimp, context, progress,
                                            args, n_args,
                                            n_return_vals);

      if (g_value_get_enum (&return_vals[0].value) == GIMP_PDB_PASS_THROUGH)
        {
          /*  If the return value is GIMP_PDB_PASS_THROUGH and there is
           *  a next procedure in the list, destroy the return values
           *  and run the next procedure.
           */
          if (g_list_next (list))
            procedural_db_destroy_args (return_vals, *n_return_vals, TRUE);
        }
      else
        {
          /*  No GIMP_PDB_PASS_THROUGH, break out of the list of
           *  procedures and return the current return values.
           */
          break;
        }
    }

  return return_vals;
}

Argument *
procedural_db_run_proc (Gimp         *gimp,
                        GimpContext  *context,
                        GimpProgress *progress,
                        const gchar  *name,
                        gint         *n_return_vals,
                        ...)
{
  ProcRecord *proc;
  Argument   *params;
  Argument   *return_vals;
  va_list     args;
  gint        i;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (n_return_vals != NULL, NULL);

  proc = procedural_db_lookup (gimp, name);

  if (proc == NULL)
    {
      g_message (_("PDB calling error:\nprocedure '%s' not found"), name);

      return_vals = gimp_procedure_get_return_values (NULL, FALSE);
      g_value_set_enum (&return_vals->value, GIMP_PDB_CALLING_ERROR);

      *n_return_vals = 1;

      return return_vals;
    }

  *n_return_vals = proc->num_values + 1;

  params = gimp_procedure_get_arguments (proc);

  va_start (args, n_return_vals);

  for (i = 0; i < proc->num_args; i++)
    {
      GimpPDBArgType  arg_type = va_arg (args, GimpPDBArgType);
      GValue         *value;

      if (arg_type == GIMP_PDB_END)
        break;

      if (arg_type != params[i].type)
        {
          gchar *expected = procedural_db_type_name (proc->args[i].type);
          gchar *got      = procedural_db_type_name (arg_type);

          procedural_db_destroy_args (params, proc->num_args, FALSE);

          g_message (_("PDB calling error for procedure '%s':\n"
                       "Argument #%d type mismatch (expected %s, got %s)"),
                     proc->name, i + 1, expected, got);

          g_free (expected);
          g_free (got);

          return_vals = gimp_procedure_get_return_values (proc, FALSE);
          g_value_set_enum (&return_vals->value, GIMP_PDB_CALLING_ERROR);

          return return_vals;
        }

      value = &params[i].value;

      switch (arg_type)
        {
        case GIMP_PDB_INT32:
          if (G_VALUE_HOLDS_INT (value))
            g_value_set_int (value, va_arg (args, gint));
          else if (G_VALUE_HOLDS_ENUM (value))
            g_value_set_enum (value, va_arg (args, gint));
          else if (G_VALUE_HOLDS_BOOLEAN (value))
            g_value_set_boolean (value, va_arg (args, gint) ? TRUE : FALSE);
          else
            g_return_val_if_reached (NULL);
          break;

        case GIMP_PDB_INT16:
          g_value_set_int (value, va_arg (args, gint));
          break;

        case GIMP_PDB_INT8:
          g_value_set_uint (value, va_arg (args, guint));
          break;

        case GIMP_PDB_FLOAT:
          g_value_set_double (value, va_arg (args, gdouble));
          break;

        case GIMP_PDB_STRING:
          g_value_set_static_string (value, va_arg (args, gchar *));
          break;

        case GIMP_PDB_INT32ARRAY:
        case GIMP_PDB_INT16ARRAY:
        case GIMP_PDB_INT8ARRAY:
        case GIMP_PDB_FLOATARRAY:
        case GIMP_PDB_STRINGARRAY:
          g_value_set_pointer (value, va_arg (args, gpointer));
          break;

        case GIMP_PDB_COLOR:
          {
            GimpRGB color = va_arg (args, GimpRGB);
            g_value_set_boxed (value, &color);
          }
          break;

        case GIMP_PDB_REGION:
        case GIMP_PDB_BOUNDARY:
          break;

        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_VECTORS:
          g_value_set_int (value, va_arg (args, gint));
          break;

        case GIMP_PDB_PARASITE:
          g_value_set_static_boxed (value, va_arg (args, gpointer));
          break;

        case GIMP_PDB_STATUS:
          g_value_set_enum (value, va_arg (args, gint));

        case GIMP_PDB_END:
          break;
        }
    }

  va_end (args);

  return_vals = procedural_db_execute (gimp, context, progress, name,
                                       params, proc->num_args,
                                       n_return_vals);

  procedural_db_destroy_args (params, proc->num_args, FALSE);

  return return_vals;
}

void
procedural_db_destroy_args (Argument *args,
                            gint      n_args,
                            gboolean  full_destroy)
{
  gint i;

  if (! args && n_args)
    return;

  for (i = n_args - 1; i >= 0; i--)
    {
      switch (args[i].type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_INT16:
        case GIMP_PDB_INT8:
        case GIMP_PDB_FLOAT:
        case GIMP_PDB_STRING:
          break;

        case GIMP_PDB_INT32ARRAY:
        case GIMP_PDB_INT16ARRAY:
        case GIMP_PDB_INT8ARRAY:
        case GIMP_PDB_FLOATARRAY:
          if (full_destroy)
            g_free (g_value_get_pointer (&args[i].value));
          break;

        case GIMP_PDB_STRINGARRAY:
          if (full_destroy)
            {
              gchar **array;
              gint    count;
              gint    j;

              count = g_value_get_int (&args[i - 1].value);
              array = g_value_get_pointer (&args[i].value);

              for (j = 0; j < count; j++)
                g_free (array[j]);

              g_free (array);
            }
          break;

        case GIMP_PDB_COLOR:
        case GIMP_PDB_REGION:
        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_BOUNDARY:
        case GIMP_PDB_VECTORS:
        case GIMP_PDB_PARASITE:
        case GIMP_PDB_STATUS:
        case GIMP_PDB_END:
          break;
        }

      g_value_unset (&args[i].value);
    }

  g_free (args);
}

void
procedural_db_argument_init (Argument *arg,
                             ProcArg  *proc_arg)
{
  g_return_if_fail (arg != NULL);
  g_return_if_fail (proc_arg != NULL);

  arg->type = proc_arg->type;
  g_value_init (&arg->value, proc_arg->pspec->value_type);
}

void
procedural_db_compat_arg_init (Argument       *arg,
                               GimpPDBArgType  arg_type)
{
  g_return_if_fail (arg != NULL);

  arg->type = arg_type;

  switch (arg_type)
    {
    case GIMP_PDB_INT32:
    case GIMP_PDB_INT16:
      g_value_init (&arg->value, G_TYPE_INT);
      break;

    case GIMP_PDB_INT8:
      g_value_init (&arg->value, G_TYPE_UINT);
      break;

    case GIMP_PDB_FLOAT:
      g_value_init (&arg->value, G_TYPE_DOUBLE);
      break;

    case GIMP_PDB_STRING:
      g_value_init (&arg->value, G_TYPE_STRING);
      break;

    case GIMP_PDB_INT32ARRAY:
    case GIMP_PDB_INT16ARRAY:
    case GIMP_PDB_INT8ARRAY:
    case GIMP_PDB_FLOATARRAY:
    case GIMP_PDB_STRINGARRAY:
      g_value_init (&arg->value, G_TYPE_POINTER);
      break;

    case GIMP_PDB_COLOR:
      g_value_init (&arg->value, GIMP_TYPE_RGB);
      break;

    case GIMP_PDB_REGION:
    case GIMP_PDB_BOUNDARY:
      break;

    case GIMP_PDB_DISPLAY:
    case GIMP_PDB_IMAGE:
    case GIMP_PDB_LAYER:
    case GIMP_PDB_CHANNEL:
    case GIMP_PDB_DRAWABLE:
    case GIMP_PDB_SELECTION:
    case GIMP_PDB_VECTORS:
      g_value_init (&arg->value, G_TYPE_INT);
      break;

    case GIMP_PDB_PARASITE:
      g_value_init (&arg->value, GIMP_TYPE_PARASITE);
      break;

    case GIMP_PDB_STATUS:
      g_value_init (&arg->value, GIMP_TYPE_PDB_STATUS_TYPE);
      break;

    case GIMP_PDB_END:
      break;
    }
}

gchar *
procedural_db_type_name (GimpPDBArgType type)
{
  const gchar *name;

  if (! gimp_enum_get_value (GIMP_TYPE_PDB_ARG_TYPE, type,
                             &name, NULL, NULL, NULL))
    {
      return  g_strdup_printf ("(PDB type %d unknown)", type);
    }

  return g_strdup (name);
}
