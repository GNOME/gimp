/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapaletteselect.c
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
  gchar                  *palette_callback;
  guint                   idle_id;
  gchar                  *palette_name;
  gint                    num_colors;
  LigmaRunPaletteCallback  callback;
  gboolean                closing;
  gpointer                data;
  GDestroyNotify          data_destroy;
} LigmaPaletteData;


/*  local function prototypes  */

static void             ligma_palette_data_free (LigmaPaletteData      *data);

static LigmaValueArray * ligma_temp_palette_run   (LigmaProcedure        *procedure,
                                                 const LigmaValueArray *args,
                                                 gpointer              run_data);
static gboolean         ligma_temp_palette_idle  (LigmaPaletteData      *data);


/*  public functions  */

const gchar *
ligma_palette_select_new (const gchar            *title,
                         const gchar            *palette_name,
                         LigmaRunPaletteCallback  callback,
                         gpointer                data,
                         GDestroyNotify          data_destroy)
{
  LigmaPlugIn      *plug_in = ligma_get_plug_in ();
  LigmaProcedure   *procedure;
  gchar           *palette_callback;
  LigmaPaletteData *palette_data;

  palette_callback = ligma_pdb_temp_procedure_name (ligma_get_pdb ());

  palette_data = g_slice_new0 (LigmaPaletteData);

  palette_data->palette_callback = palette_callback;
  palette_data->callback         = callback;
  palette_data->data             = data;
  palette_data->data_destroy     = data_destroy;

  procedure = ligma_procedure_new (plug_in,
                                  palette_callback,
                                  LIGMA_PDB_PROC_TYPE_TEMPORARY,
                                  ligma_temp_palette_run,
                                  palette_data,
                                  (GDestroyNotify)
                                  ligma_palette_data_free);

  LIGMA_PROC_ARG_STRING (procedure, "palette-name",
                        "Palette name",
                        "The palette name",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "num-colors",
                     "Num colors",
                     "Number of colors",
                     0, G_MAXINT, 0,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was closing",
                         FALSE,
                         G_PARAM_READWRITE);

  ligma_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (ligma_palettes_popup (palette_callback, title, palette_name))
    {
      /* Allow callbacks to be watched */
      ligma_plug_in_extension_enable (plug_in);

      return palette_callback;
    }

  ligma_plug_in_remove_temp_procedure (plug_in, palette_callback);

  return NULL;
}

void
ligma_palette_select_destroy (const gchar *palette_callback)
{
  LigmaPlugIn *plug_in = ligma_get_plug_in ();

  g_return_if_fail (palette_callback != NULL);

  ligma_plug_in_remove_temp_procedure (plug_in, palette_callback);
}


/*  private functions  */

static void
ligma_palette_data_free (LigmaPaletteData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  if (data->palette_callback)
    {
      ligma_palettes_close_popup (data->palette_callback);
      g_free (data->palette_callback);
    }

  g_free (data->palette_name);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (LigmaPaletteData, data);
}

static LigmaValueArray *
ligma_temp_palette_run (LigmaProcedure        *procedure,
                       const LigmaValueArray *args,
                       gpointer              run_data)
{
  LigmaPaletteData *data = run_data;

  g_free (data->palette_name);

  data->palette_name = LIGMA_VALUES_DUP_STRING  (args, 0);
  data->num_colors   = LIGMA_VALUES_GET_INT     (args, 1);
  data->closing      = LIGMA_VALUES_GET_BOOLEAN (args, 2);

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) ligma_temp_palette_idle,
                                data);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static gboolean
ligma_temp_palette_idle (LigmaPaletteData *data)
{
  data->idle_id = 0;

  if (data->callback)
    data->callback (data->palette_name,
                    data->closing,
                    data->data);

  if (data->closing)
    {
      gchar *palette_callback = data->palette_callback;

      data->palette_callback = NULL;
      ligma_palette_select_destroy (palette_callback);
      g_free (palette_callback);
    }

  return G_SOURCE_REMOVE;
}
