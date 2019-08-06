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

static void      gimp_gradient_data_free     (GimpGradientData     *data);

static void      gimp_temp_gradient_run      (const gchar          *name,
                                              gint                  nparams,
                                              const GimpParam      *param,
                                              gint                 *nreturn_vals,
                                              GimpParam           **return_vals);
static GimpValueArray *
                 gimp_temp_gradient_run_func (GimpProcedure        *procedure,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static gboolean  gimp_temp_gradient_run_idle (GimpGradientData     *gradient_data);


/*  private variables  */

static GHashTable *gimp_gradient_select_ht = NULL;


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
  gchar            *gradient_callback;
  GimpGradientData *gradient_data;

  if (plug_in)
    gradient_callback = gimp_pdb_temp_procedure_name (gimp_get_pdb ());
  else
    gradient_callback = gimp_pdb_temp_name ();

  gradient_data = g_slice_new0 (GimpGradientData);

  gradient_data->gradient_callback = gradient_callback;
  gradient_data->callback          = callback;
  gradient_data->data              = data;
  gradient_data->data_destroy      = data_destroy;

  if (plug_in)
    {
      GimpProcedure *procedure = gimp_procedure_new (plug_in,
                                                     gradient_callback,
                                                     GIMP_TEMPORARY,
                                                     gimp_temp_gradient_run_func,
                                                     gradient_data,
                                                     (GDestroyNotify)
                                                     gimp_gradient_data_free);

      gimp_procedure_add_argument (procedure,
                                   g_param_spec_string ("gradient-name",
                                                        "Gradient name",
                                                        "The gradient name",
                                                        NULL,
                                                        G_PARAM_READWRITE));
      gimp_procedure_add_argument (procedure,
                                   gimp_param_spec_int32 ("gradient-width",
                                                          "Gradient width",
                                                          "The gradient width",
                                                          0, G_MAXINT, 0,
                                                          G_PARAM_READWRITE));
      gimp_procedure_add_argument (procedure,
                                   gimp_param_spec_float_array ("gradient-data",
                                                                "Gradient data",
                                                                "The gradient "
                                                                "data",
                                                                G_PARAM_READWRITE));
      gimp_procedure_add_argument (procedure,
                                   g_param_spec_boolean ("closing",
                                                         "Closing",
                                                         "If the dialog was "
                                                         "closing",
                                                         FALSE,
                                                         G_PARAM_READWRITE));

      gimp_plug_in_add_temp_procedure (plug_in, procedure);
      g_object_unref (procedure);
    }
  else
    {
      static const GimpParamDef args[] =
      {
        { GIMP_PDB_STRING,    "str",            "String"                     },
        { GIMP_PDB_INT32,     "gradient width", "Gradient width"             },
        { GIMP_PDB_FLOATARRAY,"gradient data",  "The gradient mask data"     },
        { GIMP_PDB_INT32,     "dialog status",  "If the dialog was closing "
                                                "[0 = No, 1 = Yes]"          }
      };

      gimp_install_temp_proc (gradient_callback,
                              "Temporary gradient popup callback procedure",
                              "",
                              "",
                              "",
                              "",
                              NULL,
                              "",
                              GIMP_TEMPORARY,
                              G_N_ELEMENTS (args), 0,
                              args, NULL,
                              gimp_temp_gradient_run);
    }

  if (gimp_gradients_popup (gradient_callback, title, gradient_name,
                            sample_size))
    {
      /* Allow callbacks to be watched */
      if (plug_in)
        {
          gimp_plug_in_extension_enable (plug_in);
        }
      else
        {
          gimp_extension_enable ();

          /* Now add to hash table so we can find it again */
          if (! gimp_gradient_select_ht)
            {
              gimp_gradient_select_ht =
                g_hash_table_new_full (g_str_hash, g_str_equal,
                                       g_free,
                                       (GDestroyNotify) gimp_gradient_data_free);
            }


          g_hash_table_insert (gimp_gradient_select_ht,
                               g_strdup (gradient_callback),
                               gradient_data);
        }

      return gradient_callback;
    }

  if (plug_in)
    {
      gimp_plug_in_remove_temp_procedure (plug_in, gradient_callback);
    }
  else
    {
      gimp_uninstall_temp_proc (gradient_callback);
      gimp_gradient_data_free (gradient_data);
    }

  return NULL;
}

void
gimp_gradient_select_destroy (const gchar *gradient_callback)
{
  GimpPlugIn *plug_in = gimp_get_plug_in ();

  g_return_if_fail (gradient_callback != NULL);

  if (plug_in)
    {
      gimp_plug_in_remove_temp_procedure (plug_in, gradient_callback);
    }
  else
    {
      GimpGradientData *gradient_data;

      g_return_if_fail (gimp_gradient_select_ht != NULL);

      gradient_data = g_hash_table_lookup (gimp_gradient_select_ht,
                                           gradient_callback);

      if (! gradient_data)
        {
          g_warning ("Can't find internal gradient data");
          return;
        }

      g_hash_table_remove (gimp_gradient_select_ht, gradient_callback);

      gimp_uninstall_temp_proc (gradient_callback);
    }
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

static void
gimp_temp_gradient_run (const gchar      *name,
                        gint              nparams,
                        const GimpParam  *param,
                        gint             *nreturn_vals,
                        GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpGradientData *gradient_data;

  gradient_data = g_hash_table_lookup (gimp_gradient_select_ht, name);

  if (! gradient_data)
    {
      g_warning ("Can't find internal gradient data");
    }
  else
    {
      g_free (gradient_data->gradient_name);
      g_free (gradient_data->gradient_data);

      gradient_data->gradient_name = g_strdup (param[0].data.d_string);
      gradient_data->width         = param[1].data.d_int32;
      gradient_data->gradient_data = g_memdup (param[2].data.d_floatarray,
                                               param[1].data.d_int32 *
                                               sizeof (gdouble));
      gradient_data->closing       = param[3].data.d_int32;

      if (! gradient_data->idle_id)
        gradient_data->idle_id =
          g_idle_add ((GSourceFunc) gimp_temp_gradient_run_idle,
                      gradient_data);
    }

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;
}

static GimpValueArray *
gimp_temp_gradient_run_func (GimpProcedure        *procedure,
                             const GimpValueArray *args,
                             gpointer              run_data)
{
  GimpGradientData *data = run_data;

  g_free (data->gradient_name);
  g_free (data->gradient_data);

  data->gradient_name = g_value_dup_string  (gimp_value_array_index (args, 0));
  data->width         = g_value_get_int     (gimp_value_array_index (args, 1));
  data->gradient_data = gimp_value_dup_float_array (gimp_value_array_index (args, 2));
  data->closing       = g_value_get_boolean (gimp_value_array_index (args, 3));

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_gradient_run_idle,
                                data);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_temp_gradient_run_idle (GimpGradientData *gradient_data)
{
  gradient_data->idle_id = 0;

  if (gradient_data->callback)
    gradient_data->callback (gradient_data->gradient_name,
                             gradient_data->width,
                             gradient_data->gradient_data,
                             gradient_data->closing,
                             gradient_data->data);

  if (gradient_data->closing)
    {
      gchar *gradient_callback = gradient_data->gradient_callback;

      gradient_data->gradient_callback = NULL;
      gimp_gradient_select_destroy (gradient_callback);
      g_free (gradient_callback);
    }

  return G_SOURCE_REMOVE;
}
