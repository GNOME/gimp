/* LIGMA - The GNU Image Manipulation Program
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmadrawable.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmaprogress.h"

#include "pdb/ligmaprocedure.h"

#include "display/ligmadisplay.h"

#include "procedure-commands.h"


static inline gboolean
LIGMA_IS_PARAM_SPEC_RUN_MODE (GParamSpec *pspec)
{
  return (G_IS_PARAM_SPEC_ENUM (pspec) &&
          pspec->value_type == LIGMA_TYPE_RUN_MODE);
}

LigmaValueArray *
procedure_commands_get_run_mode_arg (LigmaProcedure *procedure)
{
  LigmaValueArray *args;
  gint            n_args = 0;

  args = ligma_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (ligma_value_array_length (args) > n_args &&
      LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (ligma_value_array_index (args, n_args),
                        LIGMA_RUN_INTERACTIVE);
      n_args++;
    }

  if (n_args > 0)
    ligma_value_array_truncate (args, n_args);

  return args;
}

LigmaValueArray *
procedure_commands_get_data_args (LigmaProcedure *procedure,
                                  LigmaObject    *object)
{
  LigmaValueArray *args;
  gint            n_args = 0;

  args = ligma_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (ligma_value_array_length (args) > n_args &&
      LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (ligma_value_array_index (args, n_args),
                        LIGMA_RUN_INTERACTIVE);
      n_args++;
    }

  if (ligma_value_array_length (args) > n_args &&
      G_IS_PARAM_SPEC_STRING (procedure->args[n_args]))
    {
      if (object)
        {
          g_value_set_string (ligma_value_array_index (args, n_args),
                              ligma_object_get_name (object));
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active data object for the plug-in!");
          ligma_value_array_unref (args);
          return NULL;
        }
    }

  if (n_args > 0)
    ligma_value_array_truncate (args, n_args);

  return args;
}

LigmaValueArray *
procedure_commands_get_image_args (LigmaProcedure   *procedure,
                                   LigmaImage       *image)
{
  LigmaValueArray *args;
  gint            n_args = 0;

  args = ligma_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (ligma_value_array_length (args) > n_args &&
      LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (ligma_value_array_index (args, n_args),
                        LIGMA_RUN_INTERACTIVE);
      n_args++;
    }

  if (ligma_value_array_length (args) > n_args &&
      LIGMA_IS_PARAM_SPEC_IMAGE (procedure->args[n_args]))
    {
      if (image)
        {
          g_value_set_object (ligma_value_array_index (args, n_args), image);
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active image for the plug-in!");
          ligma_value_array_unref (args);
          return NULL;
        }
    }

  if (n_args)
    ligma_value_array_truncate (args, n_args);

  return args;
}

LigmaValueArray *
procedure_commands_get_items_args (LigmaProcedure *procedure,
                                   LigmaImage     *image,
                                   GList         *items_list)
{
  LigmaValueArray *args;
  gint            n_args = 0;

  args = ligma_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (ligma_value_array_length (args) > n_args &&
      LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (ligma_value_array_index (args, n_args),
                        LIGMA_RUN_INTERACTIVE);
      n_args++;
    }

  if (ligma_value_array_length (args) > n_args &&
      LIGMA_IS_PARAM_SPEC_IMAGE (procedure->args[n_args]))
    {
      if (image)
        {
          g_value_set_object (ligma_value_array_index (args, n_args), image);
          n_args++;

          if (ligma_value_array_length (args) > n_args &&
              LIGMA_IS_PARAM_SPEC_ITEM (procedure->args[n_args]))
            {
              if (items_list)
                {
                  g_printerr ("%s: plug-in procedures expecting a single item are deprecated!\n",
                              G_STRFUNC);
                  g_value_set_object (ligma_value_array_index (args, n_args),
                                      items_list->data);
                  n_args++;
                }
              else
                {
                  g_warning ("Uh-oh, no selected items for the plug-in!");
                  ligma_value_array_unref (args);
                  return NULL;
                }
            }
          else if (ligma_value_array_length (args) > n_args + 1        &&
                   G_IS_PARAM_SPEC_INT (procedure->args[n_args])      &&
                   LIGMA_IS_PARAM_SPEC_OBJECT_ARRAY (procedure->args[n_args + 1]))
            {
              LigmaItem **items   = NULL;
              gint       n_items;

              n_items = g_list_length (items_list);

              g_value_set_int (ligma_value_array_index (args, n_args++),
                               n_items);

              if (items_list)
                {
                  GList *iter;
                  gint   i;

                  items = g_new (LigmaItem *, n_items);
                  for (iter = items_list, i = 0; iter; iter = iter->next, i++)
                    items[i] = iter->data;
                }

              ligma_value_set_object_array (ligma_value_array_index (args, n_args++),
                                           LIGMA_TYPE_ITEM,
                                           (GObject **) items, n_items);

              g_free (items);
            }
        }
    }

  if (n_args)
    ligma_value_array_truncate (args, n_args);

  return args;
}

LigmaValueArray *
procedure_commands_get_display_args (LigmaProcedure *procedure,
                                     LigmaDisplay   *display,
                                     LigmaObject    *settings)
{
  LigmaValueArray *args;
  gint            n_args = 0;

  args = ligma_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (ligma_value_array_length (args) > n_args &&
      LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (ligma_value_array_index (args, n_args),
                        LIGMA_RUN_INTERACTIVE);
      n_args++;
    }

  if (ligma_value_array_length (args) > n_args &&
      LIGMA_IS_PARAM_SPEC_DISPLAY (procedure->args[n_args]))
    {
      if (display)
        {
          g_value_set_object (ligma_value_array_index (args, n_args), display);
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active display for the plug-in!");
          ligma_value_array_unref (args);
          return NULL;
        }
    }

  if (ligma_value_array_length (args) > n_args &&
      LIGMA_IS_PARAM_SPEC_IMAGE (procedure->args[n_args]))
    {
      LigmaImage *image = display ? ligma_display_get_image (display) : NULL;

      if (image)
        {
          GList *drawables_list = ligma_image_get_selected_drawables (image);

          g_value_set_object (ligma_value_array_index (args, n_args), image);
          n_args++;

          if (ligma_value_array_length (args) > n_args &&
              LIGMA_IS_PARAM_SPEC_DRAWABLE (procedure->args[n_args]))
            {
              if (drawables_list)
                {
                  g_printerr ("%s: plug-in procedures expecting a single drawable are deprecated!\n",
                              G_STRFUNC);
                  g_value_set_object (ligma_value_array_index (args, n_args),
                                      drawables_list->data);
                  n_args++;
                }
              else
                {
                  g_warning ("Uh-oh, no selected drawables for the plug-in!");

                  ligma_value_array_unref (args);
                  g_list_free (drawables_list);

                  return NULL;
                }
            }
          else if (ligma_value_array_length (args) > n_args + 1        &&
                   G_IS_PARAM_SPEC_INT (procedure->args[n_args])      &&
                   LIGMA_IS_PARAM_SPEC_OBJECT_ARRAY (procedure->args[n_args + 1]))
            {
              LigmaDrawable **drawables   = NULL;
              gint           n_drawables;

              n_drawables = g_list_length (drawables_list);

              g_value_set_int (ligma_value_array_index (args, n_args++),
                               n_drawables);

              if (drawables_list)
                {
                  GList *iter;
                  gint   i;

                  drawables = g_new (LigmaDrawable *, n_drawables);
                  for (iter = drawables_list, i = 0; iter; iter = iter->next, i++)
                    drawables[i] = iter->data;
                }

              ligma_value_set_object_array (ligma_value_array_index (args, n_args++),
                                           LIGMA_TYPE_DRAWABLE,
                                           (GObject **) drawables, n_drawables);

              g_free (drawables);
            }
          g_list_free (drawables_list);
        }
    }

  if (ligma_value_array_length (args) > n_args &&
      g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (procedure->args[n_args]),
                   LIGMA_TYPE_OBJECT))
    {
      g_value_set_object (ligma_value_array_index (args, n_args), settings);
      n_args++;
    }

  if (n_args)
    ligma_value_array_truncate (args, n_args);

  return args;
}

gboolean
procedure_commands_run_procedure (LigmaProcedure  *procedure,
                                  Ligma           *ligma,
                                  LigmaProgress   *progress,
                                  LigmaValueArray *args)
{
  LigmaValueArray *return_vals;
  GError         *error = NULL;

  g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (args != NULL, FALSE);

  if (ligma_value_array_length (args) > 0 &&
      LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]))
    g_value_set_enum (ligma_value_array_index (args, 0),
                      LIGMA_RUN_NONINTERACTIVE);

  return_vals = ligma_procedure_execute (procedure, ligma,
                                        ligma_get_user_context (ligma),
                                        progress, args,
                                        &error);
  ligma_value_array_unref (return_vals);

  if (error)
    {
      ligma_message_literal (ligma,
                            G_OBJECT (progress), LIGMA_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);

      return FALSE;
    }

  return TRUE;
}

gboolean
procedure_commands_run_procedure_async (LigmaProcedure  *procedure,
                                        Ligma           *ligma,
                                        LigmaProgress   *progress,
                                        LigmaRunMode     run_mode,
                                        LigmaValueArray *args,
                                        LigmaDisplay    *display)
{
  GError *error = NULL;

  g_return_val_if_fail (LIGMA_IS_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (display == NULL || LIGMA_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (args != NULL, FALSE);

  if (ligma_value_array_length (args) > 0 &&
      LIGMA_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]))
    g_value_set_enum (ligma_value_array_index (args, 0),
                      run_mode);

  ligma_procedure_execute_async (procedure, ligma,
                                ligma_get_user_context (ligma),
                                progress, args,
                                display, &error);

  if (error)
    {
      ligma_message_literal (ligma,
                            G_OBJECT (progress), LIGMA_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);

      return FALSE;
    }

  return TRUE;
}
