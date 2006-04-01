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

#include "gimpargument.h"
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
      g_list_foreach (value, (GFunc) gimp_procedure_free, NULL);
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
procedural_db_register (Gimp          *gimp,
                        GimpProcedure *procedure)
{
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

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

GimpProcedure *
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

GimpArgument *
procedural_db_execute (Gimp         *gimp,
                       GimpContext  *context,
                       GimpProgress *progress,
                       const gchar  *name,
                       GimpArgument *args,
                       gint          n_args,
                       gint         *n_return_vals)
{
  GimpArgument *return_vals = NULL;
  GList        *list;

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
      GimpProcedure *procedure = list->data;

      g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

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
            gimp_arguments_destroy (return_vals, *n_return_vals);
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

GimpArgument *
procedural_db_run_proc (Gimp         *gimp,
                        GimpContext  *context,
                        GimpProgress *progress,
                        const gchar  *name,
                        gint         *n_return_vals,
                        ...)
{
  GimpProcedure *procedure;
  GimpArgument  *args;
  GimpArgument  *return_vals;
  va_list        va_args;
  gint           i;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (n_return_vals != NULL, NULL);

  procedure = procedural_db_lookup (gimp, name);

  if (procedure == NULL)
    {
      g_message (_("PDB calling error:\nprocedure '%s' not found"), name);

      return_vals = gimp_procedure_get_return_values (NULL, FALSE);
      g_value_set_enum (&return_vals->value, GIMP_PDB_CALLING_ERROR);

      *n_return_vals = 1;

      return return_vals;
    }

  *n_return_vals = procedure->num_values + 1;

  args = gimp_procedure_get_arguments (procedure);

  va_start (va_args, n_return_vals);

  for (i = 0; i < procedure->num_args; i++)
    {
      GimpPDBArgType  arg_type = va_arg (va_args, GimpPDBArgType);
      GValue         *value;
      GimpArray      *array;
      gint            count;

      if (arg_type == GIMP_PDB_END)
        break;

      if (arg_type != args[i].type)
        {
          gchar *expected = procedural_db_type_name (procedure->args[i].type);
          gchar *got      = procedural_db_type_name (arg_type);

          gimp_arguments_destroy (args, procedure->num_args);

          g_message (_("PDB calling error for procedure '%s':\n"
                       "Argument #%d type mismatch (expected %s, got %s)"),
                     procedure->name, i + 1, expected, got);

          g_free (expected);
          g_free (got);

          return_vals = gimp_procedure_get_return_values (procedure, FALSE);
          g_value_set_enum (&return_vals->value, GIMP_PDB_CALLING_ERROR);

          return return_vals;
        }

      value = &args[i].value;

      switch (arg_type)
        {
        case GIMP_PDB_INT32:
          if (G_VALUE_HOLDS_INT (value))
            g_value_set_int (value, va_arg (va_args, gint));
          else if (G_VALUE_HOLDS_ENUM (value))
            g_value_set_enum (value, va_arg (va_args, gint));
          else if (G_VALUE_HOLDS_BOOLEAN (value))
            g_value_set_boolean (value, va_arg (va_args, gint) ? TRUE : FALSE);
          else
            g_return_val_if_reached (NULL);
          break;

        case GIMP_PDB_INT16:
          g_value_set_int (value, va_arg (va_args, gint));
          break;

        case GIMP_PDB_INT8:
          g_value_set_uint (value, va_arg (va_args, guint));
          break;

        case GIMP_PDB_FLOAT:
          g_value_set_double (value, va_arg (va_args, gdouble));
          break;

        case GIMP_PDB_STRING:
          g_value_set_static_string (value, va_arg (va_args, gchar *));
          break;

        case GIMP_PDB_INT32ARRAY:
          count = g_value_get_int (&args[i - 1].value);
          gimp_value_set_static_int32array (value,
                                            va_arg (va_args, gpointer),
                                            count);
          break;

        case GIMP_PDB_INT16ARRAY:
          count = g_value_get_int (&args[i - 1].value);
          gimp_value_set_static_int16array (value,
                                            va_arg (va_args, gpointer),
                                            count);
          break;

        case GIMP_PDB_INT8ARRAY:
          count = g_value_get_int (&args[i - 1].value);
          gimp_value_set_static_int8array (value,
                                           va_arg (va_args, gpointer),
                                           count);
          break;

        case GIMP_PDB_FLOATARRAY:
          count = g_value_get_int (&args[i - 1].value);
          gimp_value_set_static_floatarray (value,
                                            va_arg (va_args, gpointer),
                                            count);
          break;

        case GIMP_PDB_STRINGARRAY:
          count = g_value_get_int (&args[i - 1].value);
          gimp_value_set_static_stringarray (value,
                                             va_arg (va_args, gpointer),
                                             count);
          break;

        case GIMP_PDB_COLOR:
          {
            GimpRGB color = va_arg (va_args, GimpRGB);
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
          g_value_set_int (value, va_arg (va_args, gint));
          break;

        case GIMP_PDB_PARASITE:
          g_value_set_static_boxed (value, va_arg (va_args, gpointer));
          break;

        case GIMP_PDB_STATUS:
          g_value_set_enum (value, va_arg (va_args, gint));

        case GIMP_PDB_END:
          break;
        }
    }

  va_end (va_args);

  return_vals = procedural_db_execute (gimp, context, progress, name,
                                       args, procedure->num_args,
                                       n_return_vals);

  gimp_arguments_destroy (args, procedure->num_args);

  return return_vals;
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
