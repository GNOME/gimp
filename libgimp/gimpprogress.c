/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaprogress.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gio/gio.h>

#include "libligmabase/ligmabase.h"

#include "ligmaprogress.h"

#include "ligma.h"


typedef struct
{
  gchar              *progress_callback;
  LigmaProgressVtable  vtable;
  gpointer            data;
  GDestroyNotify      data_destroy;
} LigmaProgressData;


/*  local function prototypes  */

static void             ligma_progress_data_free (LigmaProgressData     *data);

static LigmaValueArray * ligma_temp_progress_run  (LigmaProcedure        *procedure,
                                                 const LigmaValueArray *args,
                                                 gpointer              run_data);


/*  private variables  */

static gdouble         ligma_progress_current = 0.0;
static const gdouble   ligma_progress_step    = (1.0 / 256.0);
static const gint64    ligma_progress_delay   = 50000; /* 50 millisecond */


/*  public functions  */

/**
 * ligma_progress_install_vtable:
 * @vtable:            a pointer to a @LigmaProgressVtable.
 * @user_data:         a pointer that is passed as user_data to all vtable
 *                     functions.
 * @user_data_destroy: (nullable): destroy function for @user_data, or %NULL.
 *
 * Returns: the name of the temporary procedure that's been installed
 *
 * Since: 2.4
 **/
const gchar *
ligma_progress_install_vtable (const LigmaProgressVtable *vtable,
                              gpointer                  user_data,
                              GDestroyNotify            user_data_destroy)
{
  LigmaPlugIn       *plug_in;
  LigmaProcedure    *procedure;
  gchar            *progress_callback;
  LigmaProgressData *progress_data;

  g_return_val_if_fail (vtable != NULL, NULL);
  g_return_val_if_fail (vtable->start != NULL, NULL);
  g_return_val_if_fail (vtable->end != NULL, NULL);
  g_return_val_if_fail (vtable->set_text != NULL, NULL);
  g_return_val_if_fail (vtable->set_value != NULL, NULL);

  plug_in = ligma_get_plug_in ();

  progress_callback = ligma_pdb_temp_procedure_name (ligma_get_pdb ());

  progress_data = g_slice_new0 (LigmaProgressData);

  progress_data->progress_callback = progress_callback;
  progress_data->vtable.start      = vtable->start;
  progress_data->vtable.end        = vtable->end;
  progress_data->vtable.set_text   = vtable->set_text;
  progress_data->vtable.set_value  = vtable->set_value;
  progress_data->vtable.pulse      = vtable->pulse;
  progress_data->vtable.get_window = vtable->get_window;
  progress_data->data              = user_data;
  progress_data->data_destroy      = user_data_destroy;

  procedure = ligma_procedure_new (plug_in,
                                  progress_callback,
                                  LIGMA_PDB_PROC_TYPE_TEMPORARY,
                                  ligma_temp_progress_run,
                                  progress_data,
                                  (GDestroyNotify)
                                  ligma_progress_data_free);

  LIGMA_PROC_ARG_ENUM (procedure, "command",
                      "Command",
                      "The progress command",
                      LIGMA_TYPE_PROGRESS_COMMAND,
                      LIGMA_PROGRESS_COMMAND_START,
                      G_PARAM_READWRITE);

  LIGMA_PROC_ARG_STRING (procedure, "text",
                        "Text",
                        "The progress text",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_DOUBLE (procedure, "value",
                        "Value",
                        "The progress value",
                        0.0, 1.0, 0.0,
                        G_PARAM_READWRITE);

  LIGMA_PROC_VAL_DOUBLE (procedure, "value",
                        "Value",
                        "The progress value",
                        0.0, 1.0, 0.0,
                        G_PARAM_READWRITE);

  ligma_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (_ligma_progress_install (progress_callback))
    {
      /* Allow callbacks to be watched */
      ligma_plug_in_extension_enable (plug_in);

      return progress_callback;
    }

  ligma_plug_in_remove_temp_procedure (plug_in, progress_callback);

  return NULL;
}

/**
 * ligma_progress_uninstall:
 * @progress_callback: the name of the temporary procedure to uninstall
 *
 * Uninstalls a temporary progress procedure that was installed using
 * ligma_progress_install().
 *
 * Since: 2.2
 **/
void
ligma_progress_uninstall (const gchar *progress_callback)
{
  LigmaPlugIn *plug_in = ligma_get_plug_in ();

  g_return_if_fail (progress_callback != NULL);

  ligma_plug_in_remove_temp_procedure (plug_in, progress_callback);
}


/**
 * ligma_progress_init:
 * @message: Message to use in the progress dialog.
 *
 * Initializes the progress bar for the current plug-in.
 *
 * Initializes the progress bar for the current plug-in. It is only
 * valid to call this procedure from a plug-in.
 *
 * Returns: TRUE on success.
 */
gboolean
ligma_progress_init (const gchar  *message)
{
  LigmaDisplay *display = ligma_default_display ();
  gboolean     success;

  ligma_progress_current = 0.0;

  success = _ligma_progress_init (message, display);

  return success;
}

/**
 * ligma_progress_init_printf:
 * @format: a standard printf() format string
 * @...: arguments for @format
 *
 * Initializes the progress bar for the current plug-in.
 *
 * Initializes the progress bar for the current plug-in. It is only
 * valid to call this procedure from a plug-in.
 *
 * Returns: %TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
ligma_progress_init_printf (const gchar *format,
                           ...)
{
  gchar    *text;
  gboolean  retval;
  va_list   args;

  g_return_val_if_fail (format != NULL, FALSE);

  va_start (args, format);
  text = g_strdup_vprintf (format, args);
  va_end (args);

  retval = ligma_progress_init (text);

  g_free (text);

  return retval;
}

/**
 * ligma_progress_set_text_printf:
 * @format: a standard printf() format string
 * @...: arguments for @format
 *
 * Changes the text in the progress bar for the current plug-in.
 *
 * This function changes the text in the progress bar for the current
 * plug-in. Unlike ligma_progress_init() it does not change the
 * displayed value.
 *
 * Returns: %TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
ligma_progress_set_text_printf (const gchar *format,
                               ...)
{
  gchar    *text;
  gboolean  retval;
  va_list   args;

  g_return_val_if_fail (format != NULL, FALSE);

  va_start (args, format);
  text = g_strdup_vprintf (format, args);
  va_end (args);

  retval = ligma_progress_set_text (text);

  g_free (text);

  return retval;
}

/**
 * ligma_progress_update:
 * @percentage: Percentage of progress completed (in the range from 0.0
 *              to 1.0).
 *
 * Updates the progress bar for the current plug-in.
 *
 * The library will handle over-updating by possibly dropping silently
 * some updates when they happen too close next to each other (either
 * time-wise or step-wise).
 * The caller does not have to take care of this aspect of progression
 * and can focus on computing relevant progression steps.
 *
 * Returns: TRUE on success.
 */
gboolean
ligma_progress_update (gdouble percentage)
{
  static gint64 last_update = 0;
  gboolean      changed;

  if (percentage <= 0.0)
    {
      changed = (ligma_progress_current != 0.0);
      percentage = 0.0;
    }
  else if (percentage >= 1.0)
    {
      changed = (ligma_progress_current != 1.0);
      percentage = 1.0;
    }
  else
    {
      if (last_update == 0 ||
          g_get_monotonic_time () - last_update >= ligma_progress_delay)
        {
          /* If the progression step is too small, better not show it. */
          changed =
            (fabs (ligma_progress_current - percentage) > ligma_progress_step);
        }
      else
        {
          /* Too many changes in a short time interval. */
          changed = FALSE;
        }
    }

  /*  Suppress the update if the change was only marginal or progression
   *  update happens too often. This is not an error, it is just
   *  unneeded to overload the GUI with constant updates.
   */
  if (! changed)
    return TRUE;

  ligma_progress_current = percentage;
  last_update = g_get_monotonic_time ();

  return _ligma_progress_update (ligma_progress_current);
}


/*  private functions  */

static void
ligma_progress_data_free (LigmaProgressData *data)
{
  _ligma_progress_uninstall (data->progress_callback);

  g_free (data->progress_callback);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (LigmaProgressData, data);
}

static LigmaValueArray *
ligma_temp_progress_run (LigmaProcedure        *procedure,
                        const LigmaValueArray *args,
                        gpointer              run_data)
{
  LigmaProgressData    *progress_data = run_data;
  LigmaProgressCommand  command;
  const gchar         *text;
  gdouble              value;

  command = LIGMA_VALUES_GET_ENUM   (args, 0);
  text    = LIGMA_VALUES_GET_STRING (args, 1);
  value   = LIGMA_VALUES_GET_DOUBLE (args, 2);

  switch (command)
    {
    case LIGMA_PROGRESS_COMMAND_START:
      progress_data->vtable.start (text, value != 0.0,
                                   progress_data->data);
      break;

    case LIGMA_PROGRESS_COMMAND_END:
      progress_data->vtable.end (progress_data->data);
      break;

    case LIGMA_PROGRESS_COMMAND_SET_TEXT:
      progress_data->vtable.set_text (text, progress_data->data);
      break;

    case LIGMA_PROGRESS_COMMAND_SET_VALUE:
      progress_data->vtable.set_value (value, progress_data->data);
      break;

    case LIGMA_PROGRESS_COMMAND_PULSE:
      if (progress_data->vtable.pulse)
        progress_data->vtable.pulse (progress_data->data);
      else
        progress_data->vtable.set_value (-1, progress_data->data);
      break;

    case LIGMA_PROGRESS_COMMAND_GET_WINDOW:
      {
        LigmaValueArray *return_vals;
        guint64         window_id = 0;

        if (progress_data->vtable.get_window)
          window_id = progress_data->vtable.get_window (progress_data->data);

        return_vals = ligma_procedure_new_return_values (procedure,
                                                        LIGMA_PDB_SUCCESS,
                                                        NULL);
        LIGMA_VALUES_SET_DOUBLE (return_vals, 1, window_id);

        return return_vals;
      }
      break;

    default:
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               NULL);
    }

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}
