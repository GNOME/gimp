/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrushselect.c
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
  gchar                *brush_callback;
  guint                 idle_id;
  gchar                *brush_name;
  gdouble               opacity;
  gint                  spacing;
  gint                  paint_mode;
  gint                  width;
  gint                  height;
  guchar               *brush_mask_data;
  GimpRunBrushCallback  callback;
  gboolean              closing;
  gpointer              data;
} GimpBrushData;


/*  local function prototypes  */

static void      gimp_brush_data_free     (GimpBrushData    *data);

static void      gimp_temp_brush_run      (const gchar      *name,
                                           gint              nparams,
                                           const GimpParam  *param,
                                           gint             *nreturn_vals,
                                           GimpParam       **return_vals);
static gboolean  gimp_temp_brush_run_idle (GimpBrushData    *brush_data);


/*  private variables  */

static GHashTable *gimp_brush_select_ht = NULL;


/*  public functions  */

const gchar *
gimp_brush_select_new (const gchar          *title,
                       const gchar          *brush_name,
                       gdouble               opacity,
                       gint                  spacing,
                       GimpLayerMode         paint_mode,
                       GimpRunBrushCallback  callback,
                       gpointer              data)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_STRING,    "str",           "String"                     },
    { GIMP_PDB_FLOAT,     "opacity",       "Opacity"                    },
    { GIMP_PDB_INT32,     "spacing",       "Spacing"                    },
    { GIMP_PDB_INT32,     "paint mode",    "Paint mode"                 },
    { GIMP_PDB_INT32,     "mask width",    "Brush width"                },
    { GIMP_PDB_INT32,     "mask height"    "Brush height"               },
    { GIMP_PDB_INT32,     "mask len",      "Length of brush mask data"  },
    { GIMP_PDB_INT8ARRAY, "mask data",     "The brush mask data"        },
    { GIMP_PDB_INT32,     "dialog status", "If the dialog was closing "
                                           "[0 = No, 1 = Yes]"          }
  };

  gchar *brush_callback = gimp_procedural_db_temp_name ();

  gimp_install_temp_proc (brush_callback,
                          "Temporary brush popup callback procedure",
                          "",
                          "",
                          "",
                          "",
                          NULL,
                          "",
                          GIMP_TEMPORARY,
                          G_N_ELEMENTS (args), 0,
                          args, NULL,
                          gimp_temp_brush_run);

  if (gimp_brushes_popup (brush_callback, title, brush_name,
                          opacity, spacing, paint_mode))
    {
      GimpBrushData *brush_data;

      gimp_extension_enable (); /* Allow callbacks to be watched */

      /* Now add to hash table so we can find it again */
      if (! gimp_brush_select_ht)
        {
          gimp_brush_select_ht =
            g_hash_table_new_full (g_str_hash, g_str_equal,
                                   g_free,
                                   (GDestroyNotify) gimp_brush_data_free);
        }

      brush_data = g_slice_new0 (GimpBrushData);

      brush_data->brush_callback = brush_callback;
      brush_data->callback       = callback;
      brush_data->data           = data;

      g_hash_table_insert (gimp_brush_select_ht, brush_callback, brush_data);

      return brush_callback;
    }

  gimp_uninstall_temp_proc (brush_callback);
  g_free (brush_callback);

  return NULL;
}

void
gimp_brush_select_destroy (const gchar *brush_callback)
{
  GimpBrushData *brush_data;

  g_return_if_fail (brush_callback != NULL);
  g_return_if_fail (gimp_brush_select_ht != NULL);

  brush_data = g_hash_table_lookup (gimp_brush_select_ht, brush_callback);

  if (! brush_data)
    {
      g_warning ("Can't find internal brush data");
      return;
    }

  if (brush_data->idle_id)
    g_source_remove (brush_data->idle_id);

  g_free (brush_data->brush_name);
  g_free (brush_data->brush_mask_data);

  if (brush_data->brush_callback)
    gimp_brushes_close_popup (brush_data->brush_callback);

  gimp_uninstall_temp_proc (brush_callback);

  g_hash_table_remove (gimp_brush_select_ht, brush_callback);
}


/*  private functions  */

static void
gimp_brush_data_free (GimpBrushData *data)
{
  g_slice_free (GimpBrushData, data);
}

static void
gimp_temp_brush_run (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpBrushData    *brush_data;

  brush_data = g_hash_table_lookup (gimp_brush_select_ht, name);

  if (! brush_data)
    {
      g_warning ("Can't find internal brush data");
    }
  else
    {
      g_free (brush_data->brush_name);
      g_free (brush_data->brush_mask_data);

      brush_data->brush_name      = g_strdup (param[0].data.d_string);
      brush_data->opacity         = param[1].data.d_float;
      brush_data->spacing         = param[2].data.d_int32;
      brush_data->paint_mode      = param[3].data.d_int32;
      brush_data->width           = param[4].data.d_int32;
      brush_data->height          = param[5].data.d_int32;
      brush_data->brush_mask_data = g_memdup (param[7].data.d_int8array,
                                              param[6].data.d_int32);
      brush_data->closing         = param[8].data.d_int32;

      if (! brush_data->idle_id)
        brush_data->idle_id = g_idle_add ((GSourceFunc) gimp_temp_brush_run_idle,
                                          brush_data);
    }

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;
}

static gboolean
gimp_temp_brush_run_idle (GimpBrushData *brush_data)
{
  brush_data->idle_id = 0;

  if (brush_data->callback)
    brush_data->callback (brush_data->brush_name,
                          brush_data->opacity,
                          brush_data->spacing,
                          brush_data->paint_mode,
                          brush_data->width,
                          brush_data->height,
                          brush_data->brush_mask_data,
                          brush_data->closing,
                          brush_data->data);

  if (brush_data->closing)
    {
      gchar *brush_callback = brush_data->brush_callback;

      brush_data->brush_callback = NULL;
      gimp_brush_select_destroy (brush_callback);
    }

  return FALSE;
}
