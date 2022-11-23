/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmabrushselect.c
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
  gchar                *brush_callback;
  guint                 idle_id;
  gchar                *brush_name;
  gdouble               opacity;
  gint                  spacing;
  gint                  paint_mode;
  gint                  width;
  gint                  height;
  gint                  brush_mask_size;
  guchar               *brush_mask_data;
  LigmaRunBrushCallback  callback;
  gboolean              closing;
  gpointer              data;
  GDestroyNotify        data_destroy;
} LigmaBrushData;


/*  local function prototypes  */

static void             ligma_brush_data_free (LigmaBrushData        *data);

static LigmaValueArray * ligma_temp_brush_run  (LigmaProcedure        *procedure,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static gboolean         ligma_temp_brush_idle (LigmaBrushData        *data);


/*  public functions  */

/**
 * ligma_brush_select_new:
 * @title:      Title of the brush selection dialog.
 * @brush_name: The name of the brush to set as the first selected.
 * @opacity:    The initial opacity of the brush.
 * @spacing:    The initial spacing of the brush (if < 0 then use brush default spacing).
 * @paint_mode: The initial paint mode.
 * @callback: (scope notified): The callback function to call each time a settings change.
 * @data: (closure callback): the run_data given to @callback.
 * @data_destroy: (destroy data): the destroy function for @data.
 *
 * Invokes a brush selection dialog then run @callback with the selected
 * brush, various settings and user's @data.
 *
 * Returns: (transfer none): the name of a temporary PDB procedure. The
 *          string belongs to the brush selection dialog and will be
 *          freed automatically when the dialog is closed.
 **/
const gchar *
ligma_brush_select_new (const gchar          *title,
                       const gchar          *brush_name,
                       gdouble               opacity,
                       gint                  spacing,
                       LigmaLayerMode         paint_mode,
                       LigmaRunBrushCallback  callback,
                       gpointer              data,
                       GDestroyNotify        data_destroy)
{
  LigmaPlugIn    *plug_in = ligma_get_plug_in ();
  LigmaProcedure *procedure;
  gchar         *brush_callback;
  LigmaBrushData *brush_data;

  brush_callback = ligma_pdb_temp_procedure_name (ligma_get_pdb ());

  brush_data = g_slice_new0 (LigmaBrushData);

  brush_data->brush_callback = brush_callback;
  brush_data->callback       = callback;
  brush_data->data           = data;
  brush_data->data_destroy   = data_destroy;

  procedure = ligma_procedure_new (plug_in,
                                  brush_callback,
                                  LIGMA_PDB_PROC_TYPE_TEMPORARY,
                                  ligma_temp_brush_run,
                                  brush_data,
                                  (GDestroyNotify)
                                  ligma_brush_data_free);

  LIGMA_PROC_ARG_STRING (procedure, "brush-name",
                        "Brush name",
                        "The brush name",
                        NULL,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_DOUBLE (procedure, "opacity",
                        "Opacity",
                        NULL,
                        0.0, 100.0, 100.0,
                        G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "spacing",
                     "Spacing",
                     NULL,
                     -1, 1000, 20,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_ENUM (procedure, "paint-mode",
                      "Paint mode",
                      NULL,
                      LIGMA_TYPE_LAYER_MODE,
                      LIGMA_LAYER_MODE_NORMAL,
                      G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "mask-width",
                     "Brush width",
                     NULL,
                     0, 10000, 0,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "mask-height",
                     "Brush height",
                     NULL,
                     0, 10000, 0,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "mask-len",
                     "Mask length",
                     "Length of brush mask data",
                     0, G_MAXINT, 0,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_UINT8_ARRAY (procedure, "mask-data",
                             "Mask data",
                             "The brush mask data",
                             G_PARAM_READWRITE);

  LIGMA_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was closing",
                         FALSE,
                         G_PARAM_READWRITE);

  ligma_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (ligma_brushes_popup (brush_callback, title, brush_name,
                          opacity, spacing, paint_mode))
    {
      /* Allow callbacks to be watched */
      ligma_plug_in_extension_enable (plug_in);

      return brush_callback;
    }

  ligma_plug_in_remove_temp_procedure (plug_in, brush_callback);

  return NULL;
}

void
ligma_brush_select_destroy (const gchar *brush_callback)
{
  LigmaPlugIn *plug_in = ligma_get_plug_in ();

  g_return_if_fail (brush_callback != NULL);

  ligma_plug_in_remove_temp_procedure (plug_in, brush_callback);
}


/*  private functions  */

static void
ligma_brush_data_free (LigmaBrushData *data)
{
  if (data->idle_id)
    g_source_remove (data->idle_id);

  if (data->brush_callback)
    {
      ligma_brushes_close_popup (data->brush_callback);
      g_free (data->brush_callback);
    }

  g_free (data->brush_name);
  g_free (data->brush_mask_data);

  if (data->data_destroy)
    data->data_destroy (data->data);

  g_slice_free (LigmaBrushData, data);
}

static LigmaValueArray *
ligma_temp_brush_run (LigmaProcedure        *procedure,
                     const LigmaValueArray *args,
                     gpointer              run_data)
{
  LigmaBrushData *data = run_data;

  g_free (data->brush_name);
  g_free (data->brush_mask_data);

  data->brush_name      = LIGMA_VALUES_DUP_STRING      (args, 0);
  data->opacity         = LIGMA_VALUES_GET_DOUBLE      (args, 1);
  data->spacing         = LIGMA_VALUES_GET_INT         (args, 2);
  data->paint_mode      = LIGMA_VALUES_GET_ENUM        (args, 3);
  data->width           = LIGMA_VALUES_GET_INT         (args, 4);
  data->height          = LIGMA_VALUES_GET_INT         (args, 5);
  data->brush_mask_size = LIGMA_VALUES_GET_INT         (args, 6);
  data->brush_mask_data = LIGMA_VALUES_DUP_UINT8_ARRAY (args, 7);
  data->closing         = LIGMA_VALUES_GET_BOOLEAN     (args, 8);

  if (! data->idle_id)
    data->idle_id = g_idle_add ((GSourceFunc) ligma_temp_brush_idle, data);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static gboolean
ligma_temp_brush_idle (LigmaBrushData *data)
{
  data->idle_id = 0;

  if (data->callback)
    data->callback (data->brush_name,
                    data->opacity,
                    data->spacing,
                    data->paint_mode,
                    data->width,
                    data->height,
                    data->brush_mask_size,
                    data->brush_mask_data,
                    data->closing,
                    data->data);

  if (data->closing)
    {
      gchar *brush_callback = data->brush_callback;

      data->brush_callback = NULL;
      ligma_brush_select_destroy (brush_callback);
      g_free (brush_callback);
    }

  return G_SOURCE_REMOVE;
}
