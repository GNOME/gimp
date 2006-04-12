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
#include <gobject/gvaluecollector.h>

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

#include "gimp-pdb.h"
#include "gimpprocedure.h"
#include "gimptemporaryprocedure.h" /* eek */
#include "internal_procs.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_pdb_free_entry (gpointer key,
                                   gpointer value,
                                   gpointer user_data);


/*  public functions  */

void
gimp_pdb_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->procedural_ht        = g_hash_table_new (g_str_hash, g_str_equal);
  gimp->procedural_compat_ht = g_hash_table_new (g_str_hash, g_str_equal);

  {
    volatile GType eek;

    eek = GIMP_TYPE_TEMPORARY_PROCEDURE;
  }
}

void
gimp_pdb_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->procedural_ht)
    {
      g_hash_table_foreach (gimp->procedural_ht,
                            gimp_pdb_free_entry, NULL);
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
gimp_pdb_init_procs (Gimp *gimp)
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
gimp_pdb_register (Gimp          *gimp,
                   GimpProcedure *procedure)
{
  const gchar *name;
  GList       *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  name = gimp_object_get_name (GIMP_OBJECT (procedure));

  list = g_hash_table_lookup (gimp->procedural_ht, name);

  g_hash_table_insert (gimp->procedural_ht, (gpointer) name,
                       g_list_prepend (list, g_object_ref (procedure)));
}

void
gimp_pdb_unregister (Gimp        *gimp,
                     const gchar *procedure_name)
{
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (procedure_name != NULL);

  list = g_hash_table_lookup (gimp->procedural_ht, procedure_name);

  if (list)
    {
      GimpProcedure *procedure = list->data;

      list = g_list_remove (list, procedure);

      if (list)
        g_hash_table_insert (gimp->procedural_ht, (gpointer) procedure_name,
                             list);
      else
        g_hash_table_remove (gimp->procedural_ht, procedure_name);

      g_object_unref (procedure);
    }
}

GimpProcedure *
gimp_pdb_lookup (Gimp        *gimp,
                 const gchar *procedure_name)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (procedure_name != NULL, NULL);

  list = g_hash_table_lookup (gimp->procedural_ht, procedure_name);

  if (list)
    return list->data;

  return NULL;
}

GValueArray *
gimp_pdb_execute (Gimp         *gimp,
                  GimpContext  *context,
                  GimpProgress *progress,
                  const gchar  *procedure_name,
                  GValueArray  *args)
{
  GValueArray *return_vals = NULL;
  GList       *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (procedure_name != NULL, NULL);

  list = g_hash_table_lookup (gimp->procedural_ht, procedure_name);

  if (list == NULL)
    {
      g_message (_("PDB calling error:\n"
                   "Procedure '%s' not found"), procedure_name);

      return_vals = gimp_procedure_get_return_values (NULL, FALSE);
      g_value_set_enum (return_vals->values, GIMP_PDB_CALLING_ERROR);

      return return_vals;
    }

  g_return_val_if_fail (args != NULL, NULL);

  for (; list; list = g_list_next (list))
    {
      GimpProcedure *procedure = list->data;

      g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

      return_vals = gimp_procedure_execute (procedure,
                                            gimp, context, progress,
                                            args);

      if (g_value_get_enum (&return_vals->values[0]) == GIMP_PDB_PASS_THROUGH)
        {
          /*  If the return value is GIMP_PDB_PASS_THROUGH and there is
           *  a next procedure in the list, destroy the return values
           *  and run the next procedure.
           */
          if (g_list_next (list))
            g_value_array_free (return_vals);
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

GValueArray *
gimp_pdb_run_proc (Gimp         *gimp,
                   GimpContext  *context,
                   GimpProgress *progress,
                   const gchar  *procedure_name,
                   ...)
{
  GimpProcedure *procedure;
  GValueArray   *args;
  GValueArray   *return_vals;
  va_list        va_args;
  gint           i;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (procedure_name != NULL, NULL);

  procedure = gimp_pdb_lookup (gimp, procedure_name);

  if (procedure == NULL)
    {
      g_message (_("PDB calling error:\n"
                   "Procedure '%s' not found"), procedure_name);

      return_vals = gimp_procedure_get_return_values (NULL, FALSE);
      g_value_set_enum (return_vals->values, GIMP_PDB_CALLING_ERROR);

      return return_vals;
    }

  args = gimp_procedure_get_arguments (procedure);

  va_start (va_args, procedure_name);

  for (i = 0; i < procedure->num_args; i++)
    {
      GValue *value;
      GType   arg_type;
      gchar  *error = NULL;

      arg_type = va_arg (va_args, GType);

      if (arg_type == G_TYPE_NONE)
        break;

      value = &args->values[i];

      if (arg_type != G_VALUE_TYPE (value))
        {
          const gchar *expected = g_type_name (G_VALUE_TYPE (value));
          const gchar *got      = g_type_name (arg_type);

          g_value_array_free (args);

          g_message (_("PDB calling error for procedure '%s':\n"
                       "Argument #%d type mismatch (expected %s, got %s)"),
                     gimp_object_get_name (GIMP_OBJECT (procedure)),
                     i + 1, expected, got);

          return_vals = gimp_procedure_get_return_values (procedure, FALSE);
          g_value_set_enum (return_vals->values, GIMP_PDB_CALLING_ERROR);

          return return_vals;
        }

      G_VALUE_COLLECT (value, va_args, G_VALUE_NOCOPY_CONTENTS, &error);

      if (error)
        {
          g_warning ("%s: %s", G_STRFUNC, error);
          g_free (error);

          g_value_array_free (args);

          return_vals = gimp_procedure_get_return_values (procedure, FALSE);
          g_value_set_enum (return_vals->values, GIMP_PDB_CALLING_ERROR);

          return return_vals;
        }
    }

  va_end (va_args);

  return_vals = gimp_pdb_execute (gimp, context, progress,
                                  procedure_name, args);

  g_value_array_free (args);

  return return_vals;
}


/*  private functions  */

static void
gimp_pdb_free_entry (gpointer key,
                     gpointer value,
                     gpointer user_data)
{
  if (value)
    {
      g_list_foreach (value, (GFunc) g_object_unref, NULL);
      g_list_free (value);
    }
}
