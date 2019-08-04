/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfontselect.c
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
  gchar               *font_callback;
  guint                idle_id;
  gchar               *font_name;
  GimpRunFontCallback  callback;
  gboolean             closing;
  gpointer             data;
  GDestroyNotify       data_destroy;
} GimpFontData;


/*  local function prototypes  */

static void      gimp_font_data_free     (GimpFontData         *data);

static void      gimp_temp_font_run      (const gchar          *name,
                                          gint                  nparams,
                                          const GimpParam      *param,
                                          gint                 *nreturn_vals,
                                          GimpParam           **return_vals);
static GimpValueArray *
                 gimp_temp_font_run_func (GimpProcedure        *procedure,
                                          const GimpValueArray *args,
                                          gpointer              run_data);
static gboolean  gimp_temp_font_run_idle (GimpFontData         *font_data);


/*  private variables  */

static GHashTable *gimp_font_select_ht = NULL;


/*  public functions  */

const gchar *
gimp_font_select_new (const gchar         *title,
                      const gchar         *font_name,
                      GimpRunFontCallback  callback,
                      gpointer             data,
                      GDestroyNotify       data_destroy)
{
  GimpPlugIn   *plug_in       = gimp_get_plug_in ();
  gchar        *font_callback = gimp_procedural_db_temp_name ();
  GimpFontData *font_data;

  font_data = g_slice_new0 (GimpFontData);

  font_data->font_callback = font_callback;
  font_data->callback      = callback;
  font_data->data          = data;
  font_data->data_destroy  = data_destroy;

  if (plug_in)
    {
      GimpProcedure *procedure = gimp_procedure_new (plug_in,
                                                     font_callback,
                                                     GIMP_TEMPORARY,
                                                     gimp_temp_font_run_func,
                                                     font_data,
                                                     (GDestroyNotify)
                                                     gimp_font_data_free);

      gimp_procedure_add_argument (procedure,
                                   g_param_spec_string ("font-name",
                                                        "Font name",
                                                        "The font name",
                                                        NULL,
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
         { GIMP_PDB_STRING, "str",           "String"                     },
         { GIMP_PDB_INT32,  "dialog status", "If the dialog was closing "
                                             "[0 = No, 1 = Yes]"          }
        };

      gimp_install_temp_proc (font_callback,
                              "Temporary font popup callback procedure",
                              "",
                              "",
                              "",
                              "",
                              NULL,
                              "",
                              GIMP_TEMPORARY,
                              G_N_ELEMENTS (args), 0,
                              args, NULL,
                              gimp_temp_font_run);
    }

  if (gimp_fonts_popup (font_callback, title, font_name))
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
          if (! gimp_font_select_ht)
            {
              gimp_font_select_ht =
                g_hash_table_new_full (g_str_hash, g_str_equal,
                                       g_free,
                                       (GDestroyNotify) gimp_font_data_free);
            }

          g_hash_table_insert (gimp_font_select_ht, font_callback, font_data);
        }

      return font_callback;
    }

  if (plug_in)
    {
      gimp_plug_in_remove_temp_procedure (plug_in, font_callback);
    }
  else
    {
      gimp_uninstall_temp_proc (font_callback);
      gimp_font_data_free (font_data);
    }

  return NULL;
}

void
gimp_font_select_destroy (const gchar *font_callback)
{
  GimpPlugIn *plug_in = gimp_get_plug_in ();

  g_return_if_fail (font_callback != NULL);

  if (plug_in)
    {
      gimp_plug_in_remove_temp_procedure (plug_in, font_callback);
    }
  else
    {
      GimpFontData *font_data;

      g_return_if_fail (gimp_font_select_ht != NULL);

      font_data = g_hash_table_lookup (gimp_font_select_ht, font_callback);

      if (! font_data)
        {
          g_warning ("Can't find internal font data");
          return;
        }

      gimp_uninstall_temp_proc (font_callback);

      g_hash_table_remove (gimp_font_select_ht, font_callback);
    }
}


/*  private functions  */

static void
gimp_font_data_free (GimpFontData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  g_free (data->font_name);

  if (data->font_callback)
    gimp_fonts_close_popup (data->font_callback);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (GimpFontData, data);
}

static void
gimp_temp_font_run (const gchar      *name,
                    gint              nparams,
                    const GimpParam  *param,
                    gint             *nreturn_vals,
                    GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpFontData     *font_data;

  font_data = g_hash_table_lookup (gimp_font_select_ht, name);

  if (! font_data)
    {
      g_warning ("Can't find internal font data");
    }
  else
    {
      g_free (font_data->font_name);

      font_data->font_name = g_strdup (param[0].data.d_string);
      font_data->closing   = param[1].data.d_int32;

      if (! font_data->idle_id)
        font_data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_font_run_idle,
                                         font_data);
    }

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;
}

static GimpValueArray *
gimp_temp_font_run_func (GimpProcedure        *procedure,
                         const GimpValueArray *args,
                         gpointer              run_data)
{
  GimpFontData *data = run_data;

  g_free (data->font_name);

  data->font_name = g_value_dup_string  (gimp_value_array_index (args, 0));
  data->closing   = g_value_get_boolean (gimp_value_array_index (args, 1));

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_font_run_idle,
                                data);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_temp_font_run_idle (GimpFontData *font_data)
{
  font_data->idle_id = 0;

  if (font_data->callback)
    font_data->callback (font_data->font_name,
                         font_data->closing,
                         font_data->data);

  if (font_data->closing)
    {
      gchar *font_callback = font_data->font_callback;

      font_data->font_callback = NULL;
      gimp_font_select_destroy (font_callback);
    }

  return FALSE;
}
