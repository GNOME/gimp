/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppaletteselect.c
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
  gchar                  *palette_callback;
  guint                   idle_id;
  gchar                  *palette_name;
  gint                    num_colors;
  GimpRunPaletteCallback  callback;
  gboolean                closing;
  gpointer                data;
  GDestroyNotify          data_destroy;
} GimpPaletteData;


/*  local function prototypes  */

static void      gimp_palette_data_free     (GimpPaletteData      *data);

static void      gimp_temp_palette_run      (const gchar          *name,
                                             gint                  nparams,
                                             const GimpParam      *param,
                                             gint                 *nreturn_vals,
                                             GimpParam           **return_vals);
static GimpValueArray *
                 gimp_temp_palette_run_func (GimpProcedure        *procedure,
                                             const GimpValueArray *args,
                                             gpointer              run_data);
static gboolean  gimp_temp_palette_run_idle (GimpPaletteData      *palette_data);


/*  private variables  */

static GHashTable *gimp_palette_select_ht = NULL;


/*  public functions  */

const gchar *
gimp_palette_select_new (const gchar            *title,
                         const gchar            *palette_name,
                         GimpRunPaletteCallback  callback,
                         gpointer                data,
                         GDestroyNotify          data_destroy)
{
  GimpPlugIn      *plug_in          = gimp_get_plug_in ();
  gchar           *palette_callback = gimp_pdb_temp_name ();
  GimpPaletteData *palette_data;

  palette_data = g_slice_new0 (GimpPaletteData);

  palette_data->palette_callback = palette_callback;
  palette_data->callback         = callback;
  palette_data->data             = data;
  palette_data->data_destroy     = data_destroy;

  if (plug_in)
    {
      GimpProcedure *procedure = gimp_procedure_new (plug_in,
                                                     palette_callback,
                                                     GIMP_TEMPORARY,
                                                     gimp_temp_palette_run_func,
                                                     palette_data,
                                                     (GDestroyNotify)
                                                     gimp_palette_data_free);

      gimp_procedure_add_argument (procedure,
                                   g_param_spec_string ("palette-name",
                                                        "Palette name",
                                                        "The palette name",
                                                        NULL,
                                                        G_PARAM_READWRITE));
      gimp_procedure_add_argument (procedure,
                                   g_param_spec_int ("num-colors",
                                                     "Num colors",
                                                     "Number of colors",
                                                     0, G_MAXINT, 0,
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
        { GIMP_PDB_STRING, "str",           "String"                      },
        { GIMP_PDB_INT32,  "num colors",    "Number of colors"            },
        { GIMP_PDB_INT32,  "dialog status", "If the dialog was closing "
                                            "[0 = No, 1 = Yes]"           }
      };

      gimp_install_temp_proc (palette_callback,
                              "Temporary palette popup callback procedure",
                              "",
                              "",
                              "",
                              "",
                              NULL,
                              "",
                              GIMP_TEMPORARY,
                              G_N_ELEMENTS (args), 0,
                              args, NULL,
                              gimp_temp_palette_run);
    }

  if (gimp_palettes_popup (palette_callback, title, palette_name))
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
          if (! gimp_palette_select_ht)
            {
              gimp_palette_select_ht =
                g_hash_table_new_full (g_str_hash, g_str_equal,
                                       g_free,
                                       (GDestroyNotify) gimp_palette_data_free);
            }

          g_hash_table_insert (gimp_palette_select_ht,
                               g_strdup (palette_callback),
                               palette_data);
        }

      return palette_callback;
    }

  if (plug_in)
    {
      gimp_plug_in_remove_temp_procedure (plug_in, palette_callback);
    }
  else
    {
      gimp_uninstall_temp_proc (palette_callback);
      gimp_palette_data_free (palette_data);
    }

  return NULL;
}

void
gimp_palette_select_destroy (const gchar *palette_callback)
{
  GimpPlugIn *plug_in = gimp_get_plug_in ();

  g_return_if_fail (palette_callback != NULL);

  if (plug_in)
    {
      gimp_plug_in_remove_temp_procedure (plug_in, palette_callback);
    }
  else
    {
      GimpPaletteData *palette_data;

      g_return_if_fail (gimp_palette_select_ht != NULL);

      palette_data = g_hash_table_lookup (gimp_palette_select_ht,
                                          palette_callback);

      if (! palette_data)
        {
          g_warning ("Can't find internal palette data");
          return;
        }

      gimp_uninstall_temp_proc (palette_callback);

      g_hash_table_remove (gimp_palette_select_ht, palette_callback);
    }
}


/*  private functions  */

static void
gimp_palette_data_free (GimpPaletteData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  if (data->palette_callback)
    {
      gimp_palettes_close_popup (data->palette_callback);
      g_free (data->palette_callback);
    }

  g_free (data->palette_name);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (GimpPaletteData, data);
}

static void
gimp_temp_palette_run (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *param,
                       gint             *nreturn_vals,
                       GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPaletteData  *palette_data;

  palette_data = g_hash_table_lookup (gimp_palette_select_ht, name);

  if (! palette_data)
    {
      g_warning ("Can't find internal palette data");
    }
  else
    {
      g_free (palette_data->palette_name);

      palette_data->palette_name = g_strdup (param[0].data.d_string);
      palette_data->num_colors   = param[1].data.d_int32;
      palette_data->closing      = param[2].data.d_int32;

      if (! palette_data->idle_id)
        palette_data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_palette_run_idle,
                                            palette_data);
    }

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;
}

static GimpValueArray *
gimp_temp_palette_run_func (GimpProcedure        *procedure,
                            const GimpValueArray *args,
                            gpointer              run_data)
{
  GimpPaletteData *data = run_data;

  g_free (data->palette_name);

  data->palette_name = g_value_dup_string  (gimp_value_array_index (args, 0));
  data->num_colors   = g_value_get_int     (gimp_value_array_index (args, 1));
  data->closing      = g_value_get_boolean (gimp_value_array_index (args, 2));

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_palette_run_idle,
                                data);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_temp_palette_run_idle (GimpPaletteData *palette_data)
{
  palette_data->idle_id = 0;

  if (palette_data->callback)
    palette_data->callback (palette_data->palette_name,
                            palette_data->closing,
                            palette_data->data);

  if (palette_data->closing)
    {
      gchar *palette_callback = palette_data->palette_callback;

      palette_data->palette_callback = NULL;
      gimp_palette_select_destroy (palette_callback);
      g_free (palette_callback);
    }

  return G_SOURCE_REMOVE;
}
