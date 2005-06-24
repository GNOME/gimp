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

#include "pdb-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpprogress.h"

#include "plug-in/plug-in-run.h"

#include "internal_procs.h"
#include "procedural_db.h"

#include "gimp-intl.h"


typedef struct _PDBData PDBData;

struct _PDBData
{
  gchar  *identifier;
  gint32  bytes;
  guint8 *data;
};


void
procedural_db_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->procedural_ht           = g_hash_table_new (g_str_hash, g_str_equal);
  gimp->procedural_compat_ht    = g_hash_table_new (g_str_hash, g_str_equal);
  gimp->procedural_db_data_list = NULL;
}

static void
procedural_db_free_entry (gpointer key,
                          gpointer value,
                          gpointer user_data)
{
  if (value)
    g_list_free (value);
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

  procedural_db_free_data (gimp);
}

void
procedural_db_init_procs (Gimp               *gimp,
                          GimpInitStatusFunc  status_callback)
{
  static const struct
  {
    const gchar *old_name;
    const gchar *new_name;
  }
  compat_procs[] =
  {
    { "gimp_blend",                      "gimp_edit_blend"                 },
    { "gimp_brushes_set_brush",          "gimp_context_set_brush"          },
    { "gimp_brushes_get_opacity",        "gimp_context_get_opacity"        },
    { "gimp_brushes_set_opacity",        "gimp_context_set_opacity"        },
    { "gimp_brushes_get_paint_mode",     "gimp_context_get_paint_mode"     },
    { "gimp_brushes_set_paint_mode",     "gimp_context_set_paint_mode"     },
    { "gimp_brushes_list",               "gimp_brushes_get_list"           },
    { "gimp_bucket_fill",                "gimp_edit_bucket_fill"           },
    { "gimp_channel_delete",             "gimp_drawable_delete"            },
    { "gimp_channel_get_name",           "gimp_drawable_get_name"          },
    { "gimp_channel_get_tattoo",         "gimp_drawable_get_tattoo"        },
    { "gimp_channel_get_visible",        "gimp_drawable_get_visible"       },
    { "gimp_channel_set_name",           "gimp_drawable_set_name"          },
    { "gimp_channel_set_tattoo",         "gimp_drawable_set_tattoo"        },
    { "gimp_channel_set_visible",        "gimp_drawable_set_visible"       },
    { "gimp_color_picker",               "gimp_image_pick_color"           },
    { "gimp_convert_grayscale",          "gimp_image_convert_grayscale"    },
    { "gimp_convert_indexed",            "gimp_image_convert_indexed"      },
    { "gimp_convert_rgb",                "gimp_image_convert_rgb"          },
    { "gimp_crop",                       "gimp_image_crop"                 },
    { "gimp_drawable_bytes",             "gimp_drawable_bpp"               },
    { "gimp_drawable_image",             "gimp_drawable_get_image"         },
    { "gimp_gradients_get_active",       "gimp_context_get_gradient"       },
    { "gimp_gradients_set_active",       "gimp_context_set_gradient"       },
    { "gimp_gradients_get_gradient",     "gimp_context_get_gradient"       },
    { "gimp_gradients_set_gradient",     "gimp_context_set_gradient"       },
    { "gimp_image_active_drawable",      "gimp_image_get_active_drawable"  },
    { "gimp_image_floating_selection",   "gimp_image_get_floating_sel"     },
    { "gimp_image_get_cmap",             "gimp_image_get_colormap"         },
    { "gimp_image_set_cmap",             "gimp_image_set_colormap"         },
    { "gimp_layer_delete",               "gimp_drawable_delete"            },
    { "gimp_layer_get_linked",           "gimp_drawable_get_linked"        },
    { "gimp_layer_get_name",             "gimp_drawable_get_name"          },
    { "gimp_layer_get_tattoo",           "gimp_drawable_get_tattoo"        },
    { "gimp_layer_get_visible",          "gimp_drawable_get_visible"       },
    { "gimp_layer_mask",                 "gimp_layer_get_mask"             },
    { "gimp_layer_set_linked",           "gimp_drawable_set_linked"        },
    { "gimp_layer_set_name",             "gimp_drawable_set_name"          },
    { "gimp_layer_set_tattoo",           "gimp_drawable_set_tattoo"        },
    { "gimp_layer_set_visible",          "gimp_drawable_set_visible"       },
    { "gimp_palette_get_foreground",     "gimp_context_get_foreground"     },
    { "gimp_palette_get_background",     "gimp_context_get_background"     },
    { "gimp_palette_set_foreground",     "gimp_context_set_foreground"     },
    { "gimp_palette_set_background",     "gimp_context_set_background"     },
    { "gimp_palette_set_default_colors", "gimp_context_set_default_colors" },
    { "gimp_palette_swap_colors",        "gimp_context_swap_colors"        },
    { "gimp_palette_refresh",            "gimp_palettes_refresh"           },
    { "gimp_palettes_set_palette",       "gimp_context_set_palette"        },
    { "gimp_patterns_list",              "gimp_patterns_get_list"          },
    { "gimp_patterns_set_pattern",       "gimp_context_set_pattern"        },
    { "gimp_selection_clear",            "gimp_selection_none"             },
    { "gimp_temp_PDB_name",              "gimp_procedural_db_temp_name"    },
    { "gimp_undo_push_group_start",      "gimp_image_undo_group_start"     },
    { "gimp_undo_push_group_end",        "gimp_image_undo_group_end"       },
    { "gimp_channel_ops_duplicate",      "gimp_image_duplicate"            },
    { "gimp_channel_ops_offset",         "gimp_drawable_offset"            }
  };

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (status_callback != NULL);

  internal_procs_init (gimp, status_callback);

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
  list = g_list_prepend (list, (gpointer) procedure);

  g_hash_table_insert (gimp->procedural_ht,
                       (gpointer) procedure->name,
                       (gpointer) list);
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
    return (ProcRecord *) list->data;
  else
    return NULL;
}

Argument *
procedural_db_execute (Gimp         *gimp,
                       GimpContext  *context,
                       GimpProgress *progress,
                       const gchar  *name,
                       Argument     *args)
{
  Argument *return_args = NULL;
  GList    *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  list = g_hash_table_lookup (gimp->procedural_ht, name);

  if (list == NULL)
    {
      g_message (_("PDB calling error:\nprocedure '%s' not found"), name);

      return_args = g_new (Argument, 1);
      return_args->arg_type      = GIMP_PDB_STATUS;
      return_args->value.pdb_int = GIMP_PDB_CALLING_ERROR;

      return return_args;
    }

  for (; list; list = g_list_next (list))
    {
      ProcRecord *procedure = list->data;
      gint        i;

      g_return_val_if_fail (procedure != NULL, NULL);

      /*  check the arguments  */
      for (i = 0; i < procedure->num_args; i++)
        {
          if (args[i].arg_type != procedure->args[i].arg_type)
            {
              g_message (_("PDB calling error for procedure '%s':\n"
                           "Argument #%d type mismatch (expected %s, got %s)"),
                         procedure->name, i + 1,
                         pdb_type_name (procedure->args[i].arg_type),
                         pdb_type_name (args[i].arg_type));

              return_args = g_new (Argument, 1);
              return_args->arg_type      = GIMP_PDB_STATUS;
              return_args->value.pdb_int = GIMP_PDB_CALLING_ERROR;

              return return_args;
            }
        }

      /*  call the procedure  */
      switch (procedure->proc_type)
        {
        case GIMP_INTERNAL:
          return_args =
            (* procedure->exec_method.internal.marshal_func) (gimp, context,
                                                              progress,
                                                              args);
          break;

        case GIMP_PLUGIN:
        case GIMP_EXTENSION:
        case GIMP_TEMPORARY:
          return_args = plug_in_run (gimp, context, progress, procedure,
                                     args, procedure->num_args,
                                     TRUE, FALSE, -1);

          /*  If there are no return arguments, assume
           *  an execution error and fall through.
           */
          if (return_args)
            break;

        default:
          return_args = g_new (Argument, 1);
          return_args->arg_type      = GIMP_PDB_STATUS;
          return_args->value.pdb_int = GIMP_PDB_EXECUTION_ERROR;

          return return_args;
        }

      if (return_args[0].value.pdb_int != GIMP_PDB_SUCCESS      &&
          return_args[0].value.pdb_int != GIMP_PDB_PASS_THROUGH &&
          procedure->num_values > 0)
        {
          memset (&return_args[1], 0, sizeof (Argument) * procedure->num_values);
        }

      if (return_args[0].value.pdb_int == GIMP_PDB_PASS_THROUGH)
        {
          /*  If the return value is GIMP_PDB_PASS_THROUGH and there is
           *  a next procedure in the list, destroy the return values
           *  and run the next procedure.
           */
          if (g_list_next (list))
            procedural_db_destroy_args (return_args, procedure->num_values);
        }
      else
        {
          /*  No GIMP_PDB_PASS_THROUGH, break out of the list of
           *  procedures and return the current return values.
           */
          break;
        }
    }

  return return_args;
}

Argument *
procedural_db_run_proc (Gimp         *gimp,
                        GimpContext  *context,
                        GimpProgress *progress,
                        const gchar  *name,
                        gint         *nreturn_vals,
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
  g_return_val_if_fail (nreturn_vals != NULL, NULL);

  proc = procedural_db_lookup (gimp, name);

  if (proc == NULL)
    {
      *nreturn_vals = 1;

      return_vals = g_new (Argument, 1);
      return_vals->arg_type      = GIMP_PDB_STATUS;
      return_vals->value.pdb_int = GIMP_PDB_CALLING_ERROR;

      return return_vals;
    }

  /*  allocate the parameter array  */
  params = g_new (Argument, proc->num_args);

  va_start (args, nreturn_vals);

  for (i = 0; i < proc->num_args; i++)
    {
      params[i].arg_type = va_arg (args, GimpPDBArgType);

      if (proc->args[i].arg_type != params[i].arg_type)
        {
          g_message (_("PDB calling error for procedure '%s':\n"
                       "Argument #%d type mismatch (expected %s, got %s)"),
                     proc->name, i + 1,
                     pdb_type_name (proc->args[i].arg_type),
                     pdb_type_name (params[i].arg_type));
          g_free (params);

          *nreturn_vals = 0;
          return NULL;
        }

      switch (proc->args[i].arg_type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_INT16:
        case GIMP_PDB_INT8:
        case GIMP_PDB_DISPLAY:
          params[i].value.pdb_int = (gint32) va_arg (args, gint);
          break;

        case GIMP_PDB_FLOAT:
          params[i].value.pdb_float = (gdouble) va_arg (args, gdouble);
          break;

        case GIMP_PDB_STRING:
        case GIMP_PDB_INT32ARRAY:
        case GIMP_PDB_INT16ARRAY:
        case GIMP_PDB_INT8ARRAY:
        case GIMP_PDB_FLOATARRAY:
        case GIMP_PDB_STRINGARRAY:
          params[i].value.pdb_pointer = va_arg (args, gpointer);
          break;

        case GIMP_PDB_COLOR:
          params[i].value.pdb_color = va_arg (args, GimpRGB);
          break;

        case GIMP_PDB_REGION:
          break;

        case GIMP_PDB_IMAGE:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_BOUNDARY:
        case GIMP_PDB_PATH:
          params[i].value.pdb_int = (gint32) va_arg (args, gint);
          break;

        case GIMP_PDB_PARASITE:
          params[i].value.pdb_pointer = va_arg (args, gpointer);
          break;

        case GIMP_PDB_STATUS:
          params[i].value.pdb_int = (gint32) va_arg (args, gint);
          break;

        case GIMP_PDB_END:
          break;
        }
    }

  va_end (args);

  *nreturn_vals = proc->num_values;

  return_vals = procedural_db_execute (gimp, context, progress, name, params);

  g_free (params);

  return return_vals;
}

Argument *
procedural_db_return_args (ProcRecord *procedure,
                           gboolean    success)
{
  Argument *return_args;
  gint      i;

  g_return_val_if_fail (procedure != NULL, NULL);

  return_args = g_new0 (Argument, procedure->num_values + 1);

  return_args[0].arg_type = GIMP_PDB_STATUS;

  if (success)
    return_args[0].value.pdb_int = GIMP_PDB_SUCCESS;
  else
    return_args[0].value.pdb_int = GIMP_PDB_EXECUTION_ERROR;

  /*  Set the arg types for the return values  */
  for (i = 0; i < procedure->num_values; i++)
    return_args[i + 1].arg_type = procedure->values[i].arg_type;

  return return_args;
}

void
procedural_db_destroy_args (Argument *args,
                            gint      nargs)
{
  gint i;

  if (! args)
    return;

  for (i = 0; i < nargs; i++)
    {
      switch (args[i].arg_type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_INT16:
        case GIMP_PDB_INT8:
        case GIMP_PDB_FLOAT:
          break;

        case GIMP_PDB_STRING:
        case GIMP_PDB_INT32ARRAY:
        case GIMP_PDB_INT16ARRAY:
        case GIMP_PDB_INT8ARRAY:
        case GIMP_PDB_FLOATARRAY:
          g_free (args[i].value.pdb_pointer);
          break;

        case GIMP_PDB_STRINGARRAY:
          {
            gchar **stringarray;
            gint    count;
            gint    j;

            count       = args[i - 1].value.pdb_int;
            stringarray = args[i].value.pdb_pointer;

            for (j = 0; j < count; j++)
              g_free (stringarray[j]);

            g_free (args[i].value.pdb_pointer);
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
        case GIMP_PDB_PATH:
          break;

        case GIMP_PDB_PARASITE:
          gimp_parasite_free ((GimpParasite *) (args[i].value.pdb_pointer));
          break;

        case GIMP_PDB_STATUS:
        case GIMP_PDB_END:
          break;
        }
    }

  g_free (args);
}

void
procedural_db_free_data (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->procedural_db_data_list)
    {
      GList *list;

      for (list = gimp->procedural_db_data_list;
           list;
           list = g_list_next (list))
        {
          PDBData *data = list->data;

          g_free (data->identifier);
          g_free (data->data);
          g_free (data);
        }

      g_list_free (gimp->procedural_db_data_list);
      gimp->procedural_db_data_list = NULL;
    }
}

void
procedural_db_set_data (Gimp         *gimp,
                        const gchar  *identifier,
                        gint32        bytes,
                        const guint8 *data)
{
  GList   *list;
  PDBData *pdb_data;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (bytes > 0);
  g_return_if_fail (data != NULL);

  for (list = gimp->procedural_db_data_list; list; list = g_list_next (list))
    {
      pdb_data = (PDBData *) list->data;

      if (! strcmp (pdb_data->identifier, identifier))
        break;
    }

  /* If there isn't already data with the specified identifier, create one */
  if (list == NULL)
    {
      pdb_data = g_new0 (PDBData, 1);
      pdb_data->identifier = g_strdup (identifier);

      gimp->procedural_db_data_list =
        g_list_prepend (gimp->procedural_db_data_list, pdb_data);
    }
  else
    {
      g_free (pdb_data->data);
    }

  pdb_data->bytes = bytes;
  pdb_data->data  = g_memdup (data, bytes);
}

const guint8 *
procedural_db_get_data (Gimp        *gimp,
                        const gchar *identifier,
                        gint32      *bytes)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (bytes != NULL, NULL);

  *bytes = 0;

  for (list = gimp->procedural_db_data_list; list; list = g_list_next (list))
    {
      PDBData *pdb_data = list->data;

      if (! strcmp (pdb_data->identifier, identifier))
        {
          *bytes = pdb_data->bytes;
          return pdb_data->data;
        }
    }

  return NULL;
}
