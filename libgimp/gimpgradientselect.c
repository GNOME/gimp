/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradientselect.c
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

#include "gimp.h"


typedef struct
{
  gchar                   *gradient_callback;
  guint                    idle_id;
  gchar                   *gradient_name;
  gint                     width;
  gdouble                 *gradient_data;
  GimpRunGradientCallback  callback;
  gboolean                 closing;
  gpointer                 data;
  GDestroyNotify           data_destroy;
} GimpGradientData;


/*  local function prototypes  */

static void      gimp_gradient_data_free        (GimpGradientData     *data);

static GimpValueArray * gimp_temp_gradient_run  (GimpProcedure        *procedure,
                                                 const GimpValueArray *args,
                                                 gpointer              run_data);
static gboolean         gimp_temp_gradient_idle (GimpGradientData     *data);


/*  public functions  */

const gchar *
gimp_gradient_select_new (const gchar             *title,
                          const gchar             *gradient_name,
                          gint                     sample_size,
                          GimpRunGradientCallback  callback,
                          gpointer                 data,
                          GDestroyNotify           data_destroy)
{
  GimpPlugIn       *plug_in = gimp_get_plug_in ();
  GimpProcedure    *procedure;
  gchar            *gradient_callback;
  GimpGradientData *gradient_data;

  gradient_callback = gimp_pdb_temp_procedure_name (gimp_get_pdb ());

  gradient_data = g_slice_new0 (GimpGradientData);

  gradient_data->gradient_callback = gradient_callback;
  gradient_data->callback          = callback;
  gradient_data->data              = data;
  gradient_data->data_destroy      = data_destroy;

  procedure = gimp_procedure_new (plug_in,
                                  gradient_callback,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  gimp_temp_gradient_run,
                                  gradient_data,
                                  (GDestroyNotify)
                                  gimp_gradient_data_free);

  GIMP_PROC_ARG_STRING (procedure, "gradient-name",
                        "Gradient name",
                        "The gradient name",
                        NULL,
                        G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "gradient-width",
                     "Gradient width",
                     "The gradient width",
                     0, G_MAXINT, 0,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_FLOAT_ARRAY (procedure, "gradient-data",
                             "Gradient data",
                             "The gradient data",
                             G_PARAM_READWRITE);

  GIMP_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was closing",
                         FALSE,
                         G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (gimp_gradients_popup (gradient_callback, title, gradient_name,
                            sample_size))
    {
      /* Allow callbacks to be watched */
      gimp_plug_in_extension_enable (plug_in);

      return gradient_callback;
    }

  gimp_plug_in_remove_temp_procedure (plug_in, gradient_callback);

  return NULL;
}

void
gimp_gradient_select_destroy (const gchar *gradient_callback)
{
  GimpPlugIn *plug_in = gimp_get_plug_in ();

  g_return_if_fail (gradient_callback != NULL);

  gimp_plug_in_remove_temp_procedure (plug_in, gradient_callback);
}


/*  private functions  */

static void
gimp_gradient_data_free (GimpGradientData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  if (data->gradient_callback)
    {
      gimp_gradients_close_popup (data->gradient_callback);
      g_free (data->gradient_callback);
    }

  g_free (data->gradient_name);
  g_free (data->gradient_data);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (GimpGradientData, data);
}

static GimpValueArray *
gimp_temp_gradient_run (GimpProcedure        *procedure,
                        const GimpValueArray *args,
                        gpointer              run_data)
{
  GimpGradientData *data = run_data;

  g_free (data->gradient_name);
  g_free (data->gradient_data);

  data->gradient_name = GIMP_VALUES_DUP_STRING      (args, 0);
  data->width         = GIMP_VALUES_GET_INT         (args, 1);
  data->gradient_data = GIMP_VALUES_DUP_FLOAT_ARRAY (args, 2);
  data->closing       = GIMP_VALUES_GET_BOOLEAN     (args, 3);

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_gradient_idle, data);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_temp_gradient_idle (GimpGradientData *data)
{
  data->idle_id = 0;

  if (data->callback)
    data->callback (data->gradient_name,
                    data->width,
                    data->gradient_data,
                    data->closing,
                    data->data);

  if (data->closing)
    {
      gchar *gradient_callback = data->gradient_callback;

      data->gradient_callback = NULL;
      gimp_gradient_select_destroy (gradient_callback);
      g_free (gradient_callback);
    }

  return G_SOURCE_REMOVE;
}
