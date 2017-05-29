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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimp-filter-history.h"
#include "core/gimpprogress.h"

#include "actions.h"
#include "filters-commands.h"
#include "gimpgeglprocedure.h"
#include "procedure-commands.h"


/*  public functions  */

void
filters_filter_cmd_callback (GtkAction   *action,
                             const gchar *operation,
                             gpointer     data)
{
  GimpDisplay   *display;
  GimpProcedure *procedure;
  return_if_no_display (display, data);

  procedure = gimp_gegl_procedure_new (action_data_get_gimp (data),
                                       operation,
                                       gtk_action_get_name (action),
                                       gtk_action_get_label (action),
                                       gtk_action_get_tooltip (action),
                                       gtk_action_get_icon_name (action),
                                       g_object_get_qdata (G_OBJECT (action),
                                                           GIMP_HELP_ID));

  gimp_filter_history_add (action_data_get_gimp (data), procedure);
  filters_history_cmd_callback (NULL, procedure, data);

  g_object_unref (procedure);
}

void
filters_repeat_cmd_callback (GtkAction *action,
                             gint       value,
                             gpointer   data)
{
  Gimp          *gimp;
  GimpDisplay   *display;
  GimpRunMode    run_mode;
  GimpProcedure *procedure;
  return_if_no_gimp (gimp, data);
  return_if_no_display (display, data);

  run_mode = (GimpRunMode) value;

  procedure = gimp_filter_history_nth (gimp, 0);

  if (procedure)
    {
      GimpValueArray *args;

      args = procedure_commands_get_display_args (procedure, display, NULL);

      if (args)
        {
          if (procedure_commands_run_procedure_async (procedure, gimp,
                                                      GIMP_PROGRESS (display),
                                                      run_mode, args,
                                                      display))
            {
              gimp_filter_history_add (gimp, procedure);
            }

          gimp_value_array_unref (args);
        }
    }
}

void
filters_history_cmd_callback (GtkAction     *action,
                              GimpProcedure *procedure,
                              gpointer       data)
{
  Gimp           *gimp;
  GimpDisplay    *display;
  GimpValueArray *args;
  return_if_no_gimp (gimp, data);
  return_if_no_display (display, data);

  args = procedure_commands_get_display_args (procedure, display, NULL);

  if (args)
    {
      if (procedure_commands_run_procedure_async (procedure, gimp,
                                                  GIMP_PROGRESS (display),
                                                  GIMP_RUN_INTERACTIVE, args,
                                                  display))
        {
          gimp_filter_history_add (gimp, procedure);
        }

      gimp_value_array_unref (args);
    }
}
