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
} GimpPaletteData;


/*  local function prototypes  */

static void      gimp_palette_data_free     (GimpPaletteData  *data);

static void      gimp_temp_palette_run      (const gchar      *name,
                                             gint              nparams,
                                             const GimpParam  *param,
                                             gint             *nreturn_vals,
                                             GimpParam       **return_vals);
static gboolean  gimp_temp_palette_run_idle (GimpPaletteData  *palette_data);


/*  private variables  */

static GHashTable *gimp_palette_select_ht = NULL;


/*  public functions  */

const gchar *
gimp_palette_select_new (const gchar            *title,
                         const gchar            *palette_name,
                         GimpRunPaletteCallback  callback,
                         gpointer                data)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_STRING, "str",           "String"                      },
    { GIMP_PDB_INT32,  "num colors",    "Number of colors"            },
    { GIMP_PDB_INT32,  "dialog status", "If the dialog was closing "
                                        "[0 = No, 1 = Yes]"           }
  };

  gchar *palette_callback = gimp_procedural_db_temp_name ();

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

  if (gimp_palettes_popup (palette_callback, title, palette_name))
    {
      GimpPaletteData *palette_data;

      gimp_extension_enable (); /* Allow callbacks to be watched */

      /* Now add to hash table so we can find it again */
      if (! gimp_palette_select_ht)
        {
          gimp_palette_select_ht =
            g_hash_table_new_full (g_str_hash, g_str_equal,
                                   g_free,
                                   (GDestroyNotify) gimp_palette_data_free);
        }

      palette_data = g_slice_new0 (GimpPaletteData);

      palette_data->palette_callback = palette_callback;
      palette_data->callback      = callback;
      palette_data->data          = data;

      g_hash_table_insert (gimp_palette_select_ht,
                           palette_callback, palette_data);

      return palette_callback;
    }

  gimp_uninstall_temp_proc (palette_callback);
  g_free (palette_callback);

  return NULL;
}

void
gimp_palette_select_destroy (const gchar *palette_callback)
{
  GimpPaletteData *palette_data;

  g_return_if_fail (palette_callback != NULL);
  g_return_if_fail (gimp_palette_select_ht != NULL);

  palette_data = g_hash_table_lookup (gimp_palette_select_ht, palette_callback);

  if (! palette_data)
    {
      g_warning ("Can't find internal palette data");
      return;
    }

  if (palette_data->idle_id)
    g_source_remove (palette_data->idle_id);

  g_free (palette_data->palette_name);

  if (palette_data->palette_callback)
    gimp_palettes_close_popup (palette_data->palette_callback);

  gimp_uninstall_temp_proc (palette_callback);

  g_hash_table_remove (gimp_palette_select_ht, palette_callback);
}


/*  private functions  */

static void
gimp_palette_data_free (GimpPaletteData *data)
{
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
    }

  return FALSE;
}
