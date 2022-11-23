/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmafontselect.c
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
  gchar               *font_callback;
  guint                idle_id;
  gchar               *font_name;
  LigmaRunFontCallback  callback;
  gboolean             closing;
  gpointer             data;
  GDestroyNotify       data_destroy;
} LigmaFontData;


/*  local function prototypes  */

static void             ligma_font_data_free (LigmaFontData         *data);

static LigmaValueArray * ligma_temp_font_run  (LigmaProcedure        *procedure,
                                             const LigmaValueArray *args,
                                             gpointer              run_data);
static gboolean         ligma_temp_font_idle (LigmaFontData         *data);


/*  public functions  */

const gchar *
ligma_font_select_new (const gchar         *title,
                      const gchar         *font_name,
                      LigmaRunFontCallback  callback,
                      gpointer             data,
                      GDestroyNotify       data_destroy)
{
  LigmaPlugIn    *plug_in = ligma_get_plug_in ();
  LigmaProcedure *procedure;
  gchar         *font_callback;
  LigmaFontData  *font_data;

  font_callback = ligma_pdb_temp_procedure_name (ligma_get_pdb ());

  font_data = g_slice_new0 (LigmaFontData);

  font_data->font_callback = font_callback;
  font_data->callback      = callback;
  font_data->data          = data;
  font_data->data_destroy  = data_destroy;

  procedure = ligma_procedure_new (plug_in,
                                  font_callback,
                                  LIGMA_PDB_PROC_TYPE_TEMPORARY,
                                  ligma_temp_font_run,
                                  font_data,
                                  (GDestroyNotify)
                                  ligma_font_data_free);

  LIGMA_PROC_ARG_STRING (procedure, "font-name",
                        "Font name",
                        "The font name",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was closing",
                         FALSE,
                         G_PARAM_READWRITE);

  ligma_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (ligma_fonts_popup (font_callback, title, font_name))
    {
      /* Allow callbacks to be watched */
      ligma_plug_in_extension_enable (plug_in);

      return font_callback;
    }

  ligma_plug_in_remove_temp_procedure (plug_in, font_callback);

  return NULL;
}

void
ligma_font_select_destroy (const gchar *font_callback)
{
  LigmaPlugIn *plug_in = ligma_get_plug_in ();

  g_return_if_fail (font_callback != NULL);

  ligma_plug_in_remove_temp_procedure (plug_in, font_callback);
}


/*  private functions  */

static void
ligma_font_data_free (LigmaFontData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  g_free (data->font_name);

  if (data->font_callback)
    {
      ligma_fonts_close_popup (data->font_callback);
      g_free (data->font_callback);
    }

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (LigmaFontData, data);
}

static LigmaValueArray *
ligma_temp_font_run (LigmaProcedure        *procedure,
                    const LigmaValueArray *args,
                    gpointer              run_data)
{
  LigmaFontData *data = run_data;

  g_free (data->font_name);

  data->font_name = LIGMA_VALUES_DUP_STRING  (args, 0);
  data->closing   = LIGMA_VALUES_GET_BOOLEAN (args, 1);

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) ligma_temp_font_idle,
                                data);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static gboolean
ligma_temp_font_idle (LigmaFontData *data)
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
      ligma_font_select_destroy (font_callback);
      g_free (font_callback);
    }

  return G_SOURCE_REMOVE;
}
