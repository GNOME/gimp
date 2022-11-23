/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmagradientselect.c
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

#include "ligma.h"


typedef struct
{
  gchar                   *gradient_callback;
  guint                    idle_id;
  gchar                   *gradient_name;
  gint                     width;
  gdouble                 *gradient_data;
  LigmaRunGradientCallback  callback;
  gboolean                 closing;
  gpointer                 data;
  GDestroyNotify           data_destroy;
} LigmaGradientData;


/*  local function prototypes  */

static void      ligma_gradient_data_free        (LigmaGradientData     *data);

static LigmaValueArray * ligma_temp_gradient_run  (LigmaProcedure        *procedure,
                                                 const LigmaValueArray *args,
                                                 gpointer              run_data);
static gboolean         ligma_temp_gradient_idle (LigmaGradientData     *data);


/*  public functions  */

const gchar *
ligma_gradient_select_new (const gchar             *title,
                          const gchar             *gradient_name,
                          gint                     sample_size,
                          LigmaRunGradientCallback  callback,
                          gpointer                 data,
                          GDestroyNotify           data_destroy)
{
  LigmaPlugIn       *plug_in = ligma_get_plug_in ();
  LigmaProcedure    *procedure;
  gchar            *gradient_callback;
  LigmaGradientData *gradient_data;

  gradient_callback = ligma_pdb_temp_procedure_name (ligma_get_pdb ());

  gradient_data = g_slice_new0 (LigmaGradientData);

  gradient_data->gradient_callback = gradient_callback;
  gradient_data->callback          = callback;
  gradient_data->data              = data;
  gradient_data->data_destroy      = data_destroy;

  procedure = ligma_procedure_new (plug_in,
                                  gradient_callback,
                                  LIGMA_PDB_PROC_TYPE_TEMPORARY,
                                  ligma_temp_gradient_run,
                                  gradient_data,
                                  (GDestroyNotify)
                                  ligma_gradient_data_free);

  LIGMA_PROC_ARG_STRING (procedure, "gradient-name",
                        "Gradient name",
                        "The gradient name",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "gradient-width",
                     "Gradient width",
                     "The gradient width",
                     0, G_MAXINT, 0,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_FLOAT_ARRAY (procedure, "gradient-data",
                             "Gradient data",
                             "The gradient data",
                             G_PARAM_READWRITE);

  LIGMA_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was closing",
                         FALSE,
                         G_PARAM_READWRITE);

  ligma_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (ligma_gradients_popup (gradient_callback, title, gradient_name,
                            sample_size))
    {
      /* Allow callbacks to be watched */
      ligma_plug_in_extension_enable (plug_in);

      return gradient_callback;
    }

  ligma_plug_in_remove_temp_procedure (plug_in, gradient_callback);

  return NULL;
}

void
ligma_gradient_select_destroy (const gchar *gradient_callback)
{
  LigmaPlugIn *plug_in = ligma_get_plug_in ();

  g_return_if_fail (gradient_callback != NULL);

  ligma_plug_in_remove_temp_procedure (plug_in, gradient_callback);
}


/*  private functions  */

static void
ligma_gradient_data_free (LigmaGradientData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  if (data->gradient_callback)
    {
      ligma_gradients_close_popup (data->gradient_callback);
      g_free (data->gradient_callback);
    }

  g_free (data->gradient_name);
  g_free (data->gradient_data);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (LigmaGradientData, data);
}

static LigmaValueArray *
ligma_temp_gradient_run (LigmaProcedure        *procedure,
                        const LigmaValueArray *args,
                        gpointer              run_data)
{
  LigmaGradientData *data = run_data;

  g_free (data->gradient_name);
  g_free (data->gradient_data);

  data->gradient_name = LIGMA_VALUES_DUP_STRING      (args, 0);
  data->width         = LIGMA_VALUES_GET_INT         (args, 1);
  data->gradient_data = LIGMA_VALUES_DUP_FLOAT_ARRAY (args, 2);
  data->closing       = LIGMA_VALUES_GET_BOOLEAN     (args, 3);

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) ligma_temp_gradient_idle, data);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static gboolean
ligma_temp_gradient_idle (LigmaGradientData *data)
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
      ligma_gradient_select_destroy (gradient_callback);
      g_free (gradient_callback);
    }

  return G_SOURCE_REMOVE;
}
