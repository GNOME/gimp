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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "operations/ligma-operation-config.h"
#include "operations/ligmaoperationsettings.h"

#include "core/ligma.h"
#include "core/ligma-filter-history.h"
#include "core/ligmaimage.h"
#include "core/ligmaprogress.h"

#include "widgets/ligmaaction.h"

#include "actions.h"
#include "filters-commands.h"
#include "ligmageglprocedure.h"
#include "procedure-commands.h"


/*  local function prototypes  */

static gchar * filters_parse_operation (Ligma          *ligma,
                                        const gchar   *operation_str,
                                        const gchar   *icon_name,
                                        LigmaObject   **settings);

static void    filters_run_procedure   (Ligma          *ligma,
                                        LigmaDisplay   *display,
                                        LigmaProcedure *procedure,
                                        LigmaRunMode    run_mode);


/*  public functions  */

void
filters_apply_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaImage     *image;
  GList         *drawables;
  gchar         *operation;
  LigmaObject    *settings;
  LigmaProcedure *procedure;
  GVariant      *variant;

  return_if_no_drawables (image, drawables, data);

  if (g_list_length (drawables) != 1)
    {
      /* We only support running filters on single drawable for now. */
      g_list_free (drawables);
      return;
    }

  operation = filters_parse_operation (image->ligma,
                                       g_variant_get_string (value, NULL),
                                       ligma_action_get_icon_name (action),
                                       &settings);

  procedure = ligma_gegl_procedure_new (image->ligma,
                                       LIGMA_RUN_NONINTERACTIVE, settings,
                                       operation,
                                       ligma_action_get_name (action),
                                       ligma_action_get_label (action),
                                       ligma_action_get_tooltip (action),
                                       ligma_action_get_icon_name (action),
                                       ligma_action_get_help_id (action));

  g_free (operation);

  if (settings)
    g_object_unref (settings);

  ligma_filter_history_add (image->ligma, procedure);

  variant = g_variant_new_uint64 (GPOINTER_TO_SIZE (procedure));
  g_variant_take_ref (variant);
  filters_history_cmd_callback (NULL, variant, data);

  g_variant_unref (variant);
  g_object_unref (procedure);
  g_list_free (drawables);
}

void
filters_apply_interactive_cmd_callback (LigmaAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  LigmaImage     *image;
  GList         *drawables;
  LigmaProcedure *procedure;
  GVariant      *variant;

  return_if_no_drawables (image, drawables, data);

  if (g_list_length (drawables) != 1)
    {
      /* We only support running filters on single drawable for now. */
      g_list_free (drawables);
      return;
    }

  procedure = ligma_gegl_procedure_new (image->ligma,
                                       LIGMA_RUN_INTERACTIVE, NULL,
                                       g_variant_get_string (value, NULL),
                                       ligma_action_get_name (action),
                                       ligma_action_get_label (action),
                                       ligma_action_get_tooltip (action),
                                       ligma_action_get_icon_name (action),
                                       ligma_action_get_help_id (action));

  ligma_filter_history_add (image->ligma, procedure);

  variant = g_variant_new_uint64 (GPOINTER_TO_SIZE (procedure));
  g_variant_take_ref (variant);
  filters_history_cmd_callback (NULL, variant, data);

  g_variant_unref (variant);
  g_object_unref (procedure);
  g_list_free (drawables);
}

void
filters_repeat_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  Ligma          *ligma;
  LigmaDisplay   *display;
  LigmaProcedure *procedure;
  LigmaRunMode    run_mode;

  return_if_no_ligma (ligma, data);
  return_if_no_display (display, data);

  run_mode = (LigmaRunMode) g_variant_get_int32 (value);

  procedure = ligma_filter_history_nth (ligma, 0);

  if (procedure)
    filters_run_procedure (ligma, display, procedure, run_mode);
}

void
filters_history_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  Ligma          *ligma;
  LigmaDisplay   *display;
  LigmaProcedure *procedure;
  gsize          hack;
  return_if_no_ligma (ligma, data);
  return_if_no_display (display, data);

  hack = g_variant_get_uint64 (value);

  procedure = GSIZE_TO_POINTER (hack);

  filters_run_procedure (ligma, display, procedure, LIGMA_RUN_INTERACTIVE);
}


/*  private functions  */

static gchar *
filters_parse_operation (Ligma         *ligma,
                         const gchar  *operation_str,
                         const gchar  *icon_name,
                         LigmaObject  **settings)
{
  const gchar *newline = strchr (operation_str, '\n');

  *settings = NULL;

  if (newline)
    {
      gchar       *operation;
      const gchar *serialized;

      operation  = g_strndup (operation_str, newline - operation_str);
      serialized = newline + 1;

      if (*serialized)
        {
          GError *error = NULL;

          *settings =
            g_object_new (ligma_operation_config_get_type (ligma, operation,
                                                          icon_name,
                                                          LIGMA_TYPE_OPERATION_SETTINGS),
                          NULL);

          if (! ligma_config_deserialize_string (LIGMA_CONFIG (*settings),
                                                serialized, -1, NULL,
                                                &error))
            {
              g_warning ("filters_parse_operation: deserializing hardcoded "
                         "operation settings failed: %s",
                         error->message);
              g_clear_error (&error);

              g_object_unref (*settings);
              *settings = NULL;
            }
        }

      return operation;
    }

  return g_strdup (operation_str);
}

static void
filters_run_procedure (Ligma          *ligma,
                       LigmaDisplay   *display,
                       LigmaProcedure *procedure,
                       LigmaRunMode    run_mode)
{
  LigmaObject     *settings = NULL;
  LigmaValueArray *args;

  if (LIGMA_IS_GEGL_PROCEDURE (procedure))
    {
      LigmaGeglProcedure *gegl_procedure = LIGMA_GEGL_PROCEDURE (procedure);

      if (gegl_procedure->default_run_mode == LIGMA_RUN_NONINTERACTIVE)
        run_mode = LIGMA_RUN_NONINTERACTIVE;

      settings = gegl_procedure->default_settings;
    }

  args = procedure_commands_get_display_args (procedure, display, settings);

  if (args)
    {
      gboolean success = FALSE;

      if (run_mode == LIGMA_RUN_NONINTERACTIVE)
        {
          success =
            procedure_commands_run_procedure (procedure, ligma,
                                              LIGMA_PROGRESS (display),
                                              args);
        }
      else
        {
          success =
            procedure_commands_run_procedure_async (procedure, ligma,
                                                    LIGMA_PROGRESS (display),
                                                    run_mode, args,
                                                    display);
        }

      if (success)
        ligma_filter_history_add (ligma, procedure);

      ligma_value_array_unref (args);
    }
}
