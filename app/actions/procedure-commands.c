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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "pdb/gimpprocedure.h"

#include "display/gimpdisplay.h"

#include "procedure-commands.h"


static inline gboolean
GIMP_IS_PARAM_SPEC_RUN_MODE (GParamSpec *pspec)
{
  return (G_IS_PARAM_SPEC_ENUM (pspec) &&
          pspec->value_type == GIMP_TYPE_RUN_MODE);
}

GimpValueArray *
procedure_commands_get_run_mode_arg (GimpProcedure *procedure)
{
  GimpValueArray *args;
  gint            n_args = 0;

  args = gimp_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (gimp_value_array_index (args, n_args),
                        GIMP_RUN_INTERACTIVE);
      n_args++;
    }

  if (n_args > 0)
    gimp_value_array_truncate (args, n_args);

  return args;
}

GimpValueArray *
procedure_commands_get_data_args (GimpProcedure *procedure,
                                  GimpObject    *object)
{
  GimpValueArray *args;
  gint            n_args = 0;

  args = gimp_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (gimp_value_array_index (args, n_args),
                        GIMP_RUN_INTERACTIVE);
      n_args++;
    }

  if (gimp_value_array_length (args) > n_args &&
      G_IS_PARAM_SPEC_STRING (procedure->args[n_args]))
    {
      if (object)
        {
          g_value_set_string (gimp_value_array_index (args, n_args),
                              gimp_object_get_name (object));
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active data object for the plug-in!");
          gimp_value_array_unref (args);
          return NULL;
        }
    }

  if (n_args > 0)
    gimp_value_array_truncate (args, n_args);

  return args;
}

GimpValueArray *
procedure_commands_get_image_args (GimpProcedure   *procedure,
                                   GimpImage       *image)
{
  GimpValueArray *args;
  gint            n_args = 0;

  args = gimp_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (gimp_value_array_index (args, n_args),
                        GIMP_RUN_INTERACTIVE);
      n_args++;
    }

  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_IMAGE (procedure->args[n_args]))
    {
      if (image)
        {
          g_value_set_object (gimp_value_array_index (args, n_args), image);
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active image for the plug-in!");
          gimp_value_array_unref (args);
          return NULL;
        }
    }

  if (n_args)
    gimp_value_array_truncate (args, n_args);

  return args;
}

GimpValueArray *
procedure_commands_get_items_args (GimpProcedure *procedure,
                                   GimpImage     *image,
                                   GList         *items_list)
{
  GimpValueArray *args;
  gint            n_args = 0;

  args = gimp_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (gimp_value_array_index (args, n_args),
                        GIMP_RUN_INTERACTIVE);
      n_args++;
    }

  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_IMAGE (procedure->args[n_args]))
    {
      if (image)
        {
          g_value_set_object (gimp_value_array_index (args, n_args), image);
          n_args++;

          if (gimp_value_array_length (args) > n_args &&
              GIMP_IS_PARAM_SPEC_ITEM (procedure->args[n_args]))
            {
              if (items_list)
                {
                  g_printerr ("%s: plug-in procedures expecting a single item are deprecated!\n",
                              G_STRFUNC);
                  g_value_set_object (gimp_value_array_index (args, n_args),
                                      items_list->data);
                  n_args++;
                }
              else
                {
                  g_warning ("Uh-oh, no selected items for the plug-in!");
                  gimp_value_array_unref (args);
                  return NULL;
                }
            }
          else if (gimp_value_array_length (args) > n_args + 1        &&
                   G_IS_PARAM_SPEC_INT (procedure->args[n_args])      &&
                   GIMP_IS_PARAM_SPEC_OBJECT_ARRAY (procedure->args[n_args + 1]))
            {
              GimpItem **items   = NULL;
              gint       n_items;

              n_items = g_list_length (items_list);

              g_value_set_int (gimp_value_array_index (args, n_args++),
                               n_items);

              if (items_list)
                {
                  GList *iter;
                  gint   i;

                  items = g_new (GimpItem *, n_items);
                  for (iter = items_list, i = 0; iter; iter = iter->next, i++)
                    items[i] = iter->data;
                }

              gimp_value_set_object_array (gimp_value_array_index (args, n_args++),
                                           GIMP_TYPE_ITEM,
                                           (GObject **) items, n_items);

              g_free (items);
            }
        }
    }

  if (n_args)
    gimp_value_array_truncate (args, n_args);

  return args;
}

GimpValueArray *
procedure_commands_get_display_args (GimpProcedure *procedure,
                                     GimpDisplay   *display,
                                     GimpObject    *settings)
{
  GimpValueArray *args;
  gint            n_args = 0;

  args = gimp_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (gimp_value_array_index (args, n_args),
                        GIMP_RUN_INTERACTIVE);
      n_args++;
    }

  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_DISPLAY (procedure->args[n_args]))
    {
      if (display)
        {
          g_value_set_object (gimp_value_array_index (args, n_args), display);
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active display for the plug-in!");
          gimp_value_array_unref (args);
          return NULL;
        }
    }

  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_IMAGE (procedure->args[n_args]))
    {
      GimpImage *image = display ? gimp_display_get_image (display) : NULL;

      if (image)
        {
          GList *drawables_list = gimp_image_get_selected_drawables (image);

          g_value_set_object (gimp_value_array_index (args, n_args), image);
          n_args++;

          if (gimp_value_array_length (args) > n_args &&
              GIMP_IS_PARAM_SPEC_DRAWABLE (procedure->args[n_args]))
            {
              if (drawables_list)
                {
                  g_printerr ("%s: plug-in procedures expecting a single drawable are deprecated!\n",
                              G_STRFUNC);
                  g_value_set_object (gimp_value_array_index (args, n_args),
                                      drawables_list->data);
                  n_args++;
                }
              else
                {
                  g_warning ("Uh-oh, no selected drawables for the plug-in!");

                  gimp_value_array_unref (args);
                  g_list_free (drawables_list);

                  return NULL;
                }
            }
          else if (gimp_value_array_length (args) > n_args + 1        &&
                   G_IS_PARAM_SPEC_INT (procedure->args[n_args])      &&
                   GIMP_IS_PARAM_SPEC_OBJECT_ARRAY (procedure->args[n_args + 1]))
            {
              GimpDrawable **drawables   = NULL;
              gint           n_drawables;

              n_drawables = g_list_length (drawables_list);

              g_value_set_int (gimp_value_array_index (args, n_args++),
                               n_drawables);

              if (drawables_list)
                {
                  GList *iter;
                  gint   i;

                  drawables = g_new (GimpDrawable *, n_drawables);
                  for (iter = drawables_list, i = 0; iter; iter = iter->next, i++)
                    drawables[i] = iter->data;
                }

              gimp_value_set_object_array (gimp_value_array_index (args, n_args++),
                                           GIMP_TYPE_DRAWABLE,
                                           (GObject **) drawables, n_drawables);

              g_free (drawables);
            }
          g_list_free (drawables_list);
        }
    }

  if (gimp_value_array_length (args) > n_args &&
      g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (procedure->args[n_args]),
                   GIMP_TYPE_OBJECT))
    {
      g_value_set_object (gimp_value_array_index (args, n_args), settings);
      n_args++;
    }

  if (n_args)
    gimp_value_array_truncate (args, n_args);

  return args;
}

gboolean
procedure_commands_run_procedure (GimpProcedure  *procedure,
                                  Gimp           *gimp,
                                  GimpProgress   *progress,
                                  GimpValueArray *args)
{
  GimpValueArray *return_vals;
  GError         *error = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (args != NULL, FALSE);

  if (gimp_value_array_length (args) > 0 &&
      GIMP_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]))
    g_value_set_enum (gimp_value_array_index (args, 0),
                      GIMP_RUN_NONINTERACTIVE);

  return_vals = gimp_procedure_execute (procedure, gimp,
                                        gimp_get_user_context (gimp),
                                        progress, args,
                                        &error);
  gimp_value_array_unref (return_vals);

  if (error)
    {
      gimp_message_literal (gimp,
                            G_OBJECT (progress), GIMP_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);

      return FALSE;
    }

  return TRUE;
}

gboolean
procedure_commands_run_procedure_async (GimpProcedure  *procedure,
                                        Gimp           *gimp,
                                        GimpProgress   *progress,
                                        GimpRunMode     run_mode,
                                        GimpValueArray *args,
                                        GimpDisplay    *display)
{
  GError *error = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (display == NULL || GIMP_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (args != NULL, FALSE);

  if (gimp_value_array_length (args) > 0 &&
      GIMP_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]))
    g_value_set_enum (gimp_value_array_index (args, 0),
                      run_mode);

  gimp_procedure_execute_async (procedure, gimp,
                                gimp_get_user_context (gimp),
                                progress, args,
                                display, &error);

  if (error)
    {
      gimp_message_literal (gimp,
                            G_OBJECT (progress), GIMP_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);

      return FALSE;
    }

  return TRUE;
}
