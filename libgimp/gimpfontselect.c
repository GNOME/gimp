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

static void             gimp_font_data_free (GimpFontData         *data);

static GimpValueArray * gimp_temp_font_run  (GimpProcedure        *procedure,
                                             const GimpValueArray *args,
                                             gpointer              run_data);
static gboolean         gimp_temp_font_idle (GimpFontData         *data);


/*  public functions  */

const gchar *
gimp_font_select_new (const gchar         *title,
                      const gchar         *font_name,
                      GimpRunFontCallback  callback,
                      gpointer             data,
                      GDestroyNotify       data_destroy)
{
  GimpPlugIn    *plug_in = gimp_get_plug_in ();
  GimpProcedure *procedure;
  gchar         *font_callback;
  GimpFontData  *font_data;

  font_callback = gimp_pdb_temp_procedure_name (gimp_get_pdb ());

  font_data = g_slice_new0 (GimpFontData);

  font_data->font_callback = font_callback;
  font_data->callback      = callback;
  font_data->data          = data;
  font_data->data_destroy  = data_destroy;

  procedure = gimp_procedure_new (plug_in,
                                  font_callback,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  gimp_temp_font_run,
                                  font_data,
                                  (GDestroyNotify)
                                  gimp_font_data_free);

  GIMP_PROC_ARG_STRING (procedure, "font-name",
                        "Font name",
                        "The font name",
                        NULL,
                        G_PARAM_READWRITE);

  GIMP_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was closing",
                         FALSE,
                         G_PARAM_READWRITE);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (gimp_fonts_popup (font_callback, title, font_name))
    {
      /* Allow callbacks to be watched */
      gimp_plug_in_extension_enable (plug_in);

      return font_callback;
    }

  gimp_plug_in_remove_temp_procedure (plug_in, font_callback);

  return NULL;
}

void
gimp_font_select_destroy (const gchar *font_callback)
{
  GimpPlugIn *plug_in = gimp_get_plug_in ();

  g_return_if_fail (font_callback != NULL);

  gimp_plug_in_remove_temp_procedure (plug_in, font_callback);
}


/*  private functions  */

static void
gimp_font_data_free (GimpFontData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  g_free (data->font_name);

  if (data->font_callback)
    {
      gimp_fonts_close_popup (data->font_callback);
      g_free (data->font_callback);
    }

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (GimpFontData, data);
}

static GimpValueArray *
gimp_temp_font_run (GimpProcedure        *procedure,
                    const GimpValueArray *args,
                    gpointer              run_data)
{
  GimpFontData *data = run_data;

  g_free (data->font_name);

  data->font_name = GIMP_VALUES_DUP_STRING  (args, 0);
  data->closing   = GIMP_VALUES_GET_BOOLEAN (args, 1);

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_font_idle,
                                data);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_temp_font_idle (GimpFontData *data)
{
  data->idle_id = 0;

  if (data->callback)
    data->callback (data->font_name,
                    data->closing,
                    data->data);

  if (data->closing)
    {
      gchar *font_callback = data->font_callback;

      data->font_callback = NULL;
      gimp_font_select_destroy (font_callback);
      g_free (font_callback);
    }

  return G_SOURCE_REMOVE;
}
