/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapatternselect.c
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
  gchar                  *pattern_callback;
  guint                   idle_id;
  gchar                  *pattern_name;
  gint                    width;
  gint                    height;
  gint                    bytes;
  guchar                 *pattern_mask_data;
  LigmaRunPatternCallback  callback;
  gboolean                closing;
  gpointer                data;
  GDestroyNotify          data_destroy;
} LigmaPatternData;


/*  local function prototypes  */

static void             ligma_pattern_data_free (LigmaPatternData      *data);

static LigmaValueArray * ligma_temp_pattern_run  (LigmaProcedure        *procedure,
                                                const LigmaValueArray *args,
                                                gpointer              run_data);
static gboolean         ligma_temp_pattern_idle (LigmaPatternData      *data);


/*  public functions  */

const gchar *
ligma_pattern_select_new (const gchar            *title,
                         const gchar            *pattern_name,
                         LigmaRunPatternCallback  callback,
                         gpointer                data,
                         GDestroyNotify          data_destroy)
{
  LigmaPlugIn      *plug_in = ligma_get_plug_in ();
  LigmaProcedure   *procedure;
  gchar           *pattern_callback;
  LigmaPatternData *pattern_data;

  pattern_callback = ligma_pdb_temp_procedure_name (ligma_get_pdb ());

  pattern_data = g_slice_new0 (LigmaPatternData);

  pattern_data->pattern_callback = pattern_callback;
  pattern_data->callback         = callback;
  pattern_data->data             = data;
  pattern_data->data_destroy     = data_destroy;

  procedure = ligma_procedure_new (plug_in,
                                  pattern_callback,
                                  LIGMA_PDB_PROC_TYPE_TEMPORARY,
                                  ligma_temp_pattern_run,
                                  pattern_data,
                                  (GDestroyNotify)
                                  ligma_pattern_data_free);

  LIGMA_PROC_ARG_STRING (procedure, "pattern-name",
                        "Pattern name",
                        "The pattern name",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "mask-width",
                     "Mask width",
                     "Pattern width",
                     0, 10000, 0,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "mask-height",
                     "Mask height",
                     "Pattern height",
                     0, 10000, 0,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "mask-bpp",
                     "Mask bpp",
                     "Pattern bytes per pixel",
                     0, 10000, 0,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "mask-len",
                     "Mask length",
                     "Length of pattern mask data",
                     0, G_MAXINT, 0,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_UINT8_ARRAY (procedure, "mask-data",
                             "Mask data",
                             "The pattern mask data",
                             G_PARAM_READWRITE);

  LIGMA_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was cloaing",
                         FALSE,
                         G_PARAM_READWRITE);

  ligma_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (ligma_patterns_popup (pattern_callback, title, pattern_name))
    {
      /* Allow callbacks to be watched */
      ligma_plug_in_extension_enable (plug_in);

      return pattern_callback;
    }

  ligma_plug_in_remove_temp_procedure (plug_in, pattern_callback);

  return NULL;
}

void
ligma_pattern_select_destroy (const gchar *pattern_callback)
{
  LigmaPlugIn *plug_in = ligma_get_plug_in ();

  g_return_if_fail (pattern_callback != NULL);

  ligma_plug_in_remove_temp_procedure (plug_in, pattern_callback);
}


/*  private functions  */

static void
ligma_pattern_data_free (LigmaPatternData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  if (data->pattern_callback)
    {
      ligma_patterns_close_popup (data->pattern_callback);
      g_free (data->pattern_callback);
    }

  g_free (data->pattern_name);
  g_free (data->pattern_mask_data);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (LigmaPatternData, data);
}

static LigmaValueArray *
ligma_temp_pattern_run (LigmaProcedure        *procedure,
                       const LigmaValueArray *args,
                       gpointer              run_data)
{
  LigmaPatternData *data = run_data;

  g_free (data->pattern_name);
  g_free (data->pattern_mask_data);

  data->pattern_name      = LIGMA_VALUES_DUP_STRING      (args, 0);
  data->width             = LIGMA_VALUES_GET_INT         (args, 1);
  data->height            = LIGMA_VALUES_GET_INT         (args, 2);
  data->bytes             = LIGMA_VALUES_GET_INT         (args, 3);
  data->pattern_mask_data = LIGMA_VALUES_DUP_UINT8_ARRAY (args, 5);
  data->closing           = LIGMA_VALUES_GET_BOOLEAN     (args, 6);

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) ligma_temp_pattern_idle, data);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static gboolean
ligma_temp_pattern_idle (LigmaPatternData *data)
{
  data->idle_id = 0;

  if (data->callback)
    data->callback (data->pattern_name,
                    data->width,
                    data->height,
                    data->bytes,
                    data->pattern_mask_data,
                    data->closing,
                    data->data);

  if (data->closing)
    {
      gchar *pattern_callback = data->pattern_callback;

      data->pattern_callback = NULL;
      ligma_pattern_select_destroy (pattern_callback);
      g_free (pattern_callback);
    }

  return G_SOURCE_REMOVE;
}
