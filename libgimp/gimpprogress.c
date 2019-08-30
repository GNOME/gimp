/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprogress.c
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

#include "libgimpbase/gimpbase.h"

#include "gimpprogress.h"

#include "gimp.h"


typedef struct
{
  gchar              *progress_callback;
  GimpProgressVtable  vtable;
  gpointer            data;
  GDestroyNotify      data_destroy;
} GimpProgressData;


/*  local function prototypes  */

static void             gimp_progress_data_free (GimpProgressData     *data);

static GimpValueArray * gimp_temp_progress_run  (GimpProcedure        *procedure,
                                                 const GimpValueArray *args,
                                                 gpointer              run_data);


/*  private variables  */

static gdouble         gimp_progress_current = 0.0;
static const gdouble   gimp_progress_step    = (1.0 / 256.0);


/*  public functions  */

/**
 * gimp_progress_install_vtable:
 * @vtable:            a pointer to a @GimpProgressVtable.
 * @user_data:         a pointer that is passed as user_data to all vtable
 *                     functions.
 * @user_data_destroy: (nullable): destroy function for @user_data, or %NULL.
 *
 * Returns: the name of the temporary procedure that's been installed
 *
 * Since: 2.4
 **/
const gchar *
gimp_progress_install_vtable (const GimpProgressVtable *vtable,
                              gpointer                  user_data,
                              GDestroyNotify            user_data_destroy)
{
  GimpPlugIn       *plug_in;
  GimpProcedure    *procedure;
  gchar            *progress_callback;
  GimpProgressData *progress_data;

  g_return_val_if_fail (vtable != NULL, NULL);
  g_return_val_if_fail (vtable->start != NULL, NULL);
  g_return_val_if_fail (vtable->end != NULL, NULL);
  g_return_val_if_fail (vtable->set_text != NULL, NULL);
  g_return_val_if_fail (vtable->set_value != NULL, NULL);

  plug_in = gimp_get_plug_in ();

  progress_callback = gimp_pdb_temp_procedure_name (gimp_get_pdb ());

  progress_data = g_slice_new0 (GimpProgressData);

  progress_data->progress_callback = progress_callback;
  progress_data->vtable.start      = vtable->start;
  progress_data->vtable.end        = vtable->end;
  progress_data->vtable.set_text   = vtable->set_text;
  progress_data->vtable.set_value  = vtable->set_value;
  progress_data->vtable.pulse      = vtable->pulse;
  progress_data->vtable.get_window = vtable->get_window;
  progress_data->data              = user_data;
  progress_data->data_destroy      = user_data_destroy;

  procedure = gimp_procedure_new (plug_in,
                                  progress_callback,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  gimp_temp_progress_run,
                                  progress_data,
                                  (GDestroyNotify)
                                  gimp_progress_data_free);

  GIMP_PROC_ARG_ENUM (procedure, "command",
                      "Command",
                      "The progress command",
                      GIMP_TYPE_PROGRESS_COMMAND,
                      GIMP_PROGRESS_COMMAND_START,
                      G_PARAM_READWRITE);

  GIMP_PROC_ARG_STRING (procedure, "text",
                        "Text",
                        "The progress text",
                        NULL,
                        G_PARAM_READWRITE);

  GIMP_PROC_ARG_DOUBLE (procedure, "value",
                        "Vakue",
                        "The progress value",
                        0.0, 1.0, 0.0,
                        G_PARAM_READWRITE);

  GIMP_PROC_VAL_DOUBLE (procedure, "value",
                        "Vakue",
                        "The progress value",
                        0.0, 1.0, 0.0,
                        G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (_gimp_progress_install (progress_callback))
    {
      /* Allow callbacks to be watched */
      gimp_plug_in_extension_enable (plug_in);

      return progress_callback;
    }

  gimp_plug_in_remove_temp_procedure (plug_in, progress_callback);

  return NULL;
}

/**
 * gimp_progress_uninstall:
 * @progress_callback: the name of the temporary procedure to uninstall
 *
 * Uninstalls a temporary progress procedure that was installed using
 * gimp_progress_install().
 *
 * Since: 2.2
 **/
void
gimp_progress_uninstall (const gchar *progress_callback)
{
  GimpPlugIn *plug_in = gimp_get_plug_in ();

  g_return_if_fail (progress_callback != NULL);

  gimp_plug_in_remove_temp_procedure (plug_in, progress_callback);
}


/**
 * gimp_progress_init:
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
gimp_progress_init (const gchar  *message)
{
  GimpDisplay *display = gimp_default_display ();
  gboolean     success;

  gimp_progress_current = 0.0;

  success = _gimp_progress_init (message, display);

  return success;
}

/**
 * gimp_progress_init_printf:
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
gimp_progress_init_printf (const gchar *format,
                           ...)
{
  gchar    *text;
  gboolean  retval;
  va_list   args;

  g_return_val_if_fail (format != NULL, FALSE);

  va_start (args, format);
  text = g_strdup_vprintf (format, args);
  va_end (args);

  retval = gimp_progress_init (text);

  g_free (text);

  return retval;
}

/**
 * gimp_progress_set_text_printf:
 * @format: a standard printf() format string
 * @...: arguments for @format
 *
 * Changes the text in the progress bar for the current plug-in.
 *
 * This function changes the text in the progress bar for the current
 * plug-in. Unlike gimp_progress_init() it does not change the
 * displayed value.
 *
 * Returns: %TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
gimp_progress_set_text_printf (const gchar *format,
                               ...)
{
  gchar    *text;
  gboolean  retval;
  va_list   args;

  g_return_val_if_fail (format != NULL, FALSE);

  va_start (args, format);
  text = g_strdup_vprintf (format, args);
  va_end (args);

  retval = gimp_progress_set_text (text);

  g_free (text);

  return retval;
}

/**
 * gimp_progress_update:
 * @percentage: Percentage of progress completed (in the range from 0.0 to 1.0).
 *
 * Updates the progress bar for the current plug-in.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_progress_update (gdouble percentage)
{
  gboolean changed;

  if (percentage <= 0.0)
    {
      changed = (gimp_progress_current != 0.0);
      percentage = 0.0;
    }
  else if (percentage >= 1.0)
    {
      changed = (gimp_progress_current != 1.0);
      percentage = 1.0;
    }
  else
    {
      changed =
        (fabs (gimp_progress_current - percentage) > gimp_progress_step);

#ifdef GIMP_UNSTABLE
      if (! changed)
        {
          static gboolean warned = FALSE;
          static gint     count  = 0;

          count++;

          if (count > 3 && ! warned)
            {
              g_printerr ("%s is updating the progress too often\n",
                          g_get_prgname ());
              warned = TRUE;
            }
        }
#endif
    }

  /*  Suppress the update if the change was only marginal.  */
  if (! changed)
    return TRUE;

  gimp_progress_current = percentage;

  return _gimp_progress_update (gimp_progress_current);
}


/*  private functions  */

static void
gimp_progress_data_free (GimpProgressData *data)
{
  _gimp_progress_uninstall (data->progress_callback);

  g_free (data->progress_callback);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (GimpProgressData, data);
}

static GimpValueArray *
gimp_temp_progress_run (GimpProcedure        *procedure,
                        const GimpValueArray *args,
                        gpointer              run_data)
{
  GimpProgressData    *progress_data = run_data;
  GimpProgressCommand  command;
  const gchar         *text;
  gdouble              value;

  command = GIMP_VALUES_GET_ENUM   (args, 0);
  text    = GIMP_VALUES_GET_STRING (args, 1);
  value   = GIMP_VALUES_GET_DOUBLE (args, 2);

  switch (command)
    {
    case GIMP_PROGRESS_COMMAND_START:
      progress_data->vtable.start (text, value != 0.0,
                                   progress_data->data);
      break;

    case GIMP_PROGRESS_COMMAND_END:
      progress_data->vtable.end (progress_data->data);
      break;

    case GIMP_PROGRESS_COMMAND_SET_TEXT:
      progress_data->vtable.set_text (text, progress_data->data);
      break;

    case GIMP_PROGRESS_COMMAND_SET_VALUE:
      progress_data->vtable.set_value (value, progress_data->data);
      break;

    case GIMP_PROGRESS_COMMAND_PULSE:
      if (progress_data->vtable.pulse)
        progress_data->vtable.pulse (progress_data->data);
      else
        progress_data->vtable.set_value (-1, progress_data->data);
      break;

    case GIMP_PROGRESS_COMMAND_GET_WINDOW:
      {
        GimpValueArray *return_vals;
        guint32         window_id = 0;

        if (progress_data->vtable.get_window)
          window_id = progress_data->vtable.get_window (progress_data->data);

        return_vals = gimp_procedure_new_return_values (procedure,
                                                        GIMP_PDB_SUCCESS,
                                                        NULL);
        GIMP_VALUES_SET_DOUBLE (return_vals, 1, window_id);

        return return_vals;
      }
      break;

    default:
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               NULL);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
