/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <libgimp/stdplugins-intl.h>

#include "dds.h"
#include "ddsread.h"
#include "ddswrite.h"
#include "misc.h"


#define LOAD_PROC                "file-dds-load"
#define SAVE_PROC                "file-dds-save"

#define DECODE_YCOCG_PROC        "color-decode-ycocg"
#define DECODE_YCOCG_SCALED_PROC "color-decode-ycocg-scaled"
#define DECODE_ALPHA_EXP_PROC    "color-decode-alpha-exp"


typedef struct _Dds      Dds;
typedef struct _DdsClass DdsClass;

struct _Dds
{
  GimpPlugIn      parent_instance;
};

struct _DdsClass
{
  GimpPlugInClass parent_class;
};


#define DDS_TYPE  (dds_get_type ())
#define DDS (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDS_TYPE, Dds))

GType                   dds_get_type         (void) G_GNUC_CONST;

static GList          * dds_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * dds_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * dds_load             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * dds_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              gint                  n_drawables,
                                              GimpDrawable        **drawables,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
#if 0
static GimpValueArray * dds_decode           (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
#endif


G_DEFINE_TYPE (Dds, dds, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (DDS_TYPE)


static void
dds_class_init (DdsClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = dds_query_procedures;
  plug_in_class->create_procedure = dds_create_procedure;
}

static void
dds_init (Dds *dds)
{
}

static GList *
dds_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));
#if 0
  list = g_list_append (list, g_strdup (DECODE_YCOCG_PROC));
  list = g_list_append (list, g_strdup (DECODE_YCOCG_SCALED_PROC));
  list = g_list_append (list, g_strdup (DECODE_ALPHA_EXP_PROC));
#endif

  return list;
}

static GimpProcedure *
dds_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           dds_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("DDS image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in DDS image format",
                                        "Loads files in DDS image format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Shawn Kirst",
                                      "Shawn Kirst",
                                      "2008");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/dds");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "dds");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,DDS");

      GIMP_PROC_ARG_BOOLEAN (procedure, "load-mipmaps",
                             "Load mipmaps",
                             "Load mipmaps if present",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "decode-images",
                             "Decode images",
                             "Decode YCoCg/AExp images when detected",
                             TRUE,
                             G_PARAM_READWRITE);
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           dds_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "INDEXED, GRAY, RGB");

      gimp_procedure_set_menu_label (procedure, N_("DDS image"));

      gimp_procedure_set_documentation (procedure,
                                        "Saves files in DDS image format",
                                        "Saves files in DDS image format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Shawn Kirst",
                                      "Shawn Kirst",
                                      "2008");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/dds");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "dds");

      GIMP_PROC_ARG_INT (procedure, "compression-format",
                         "Compression format",
                         "Compression format (0 = None, 1 = BC1/DXT1, "
                         "2 = BC2/DXT3, 3 = BC3/DXT5, 4 = BC3n/DXT5nm, "
                         "5 = BC4/ATI1N, 6 = BC5/ATI2N, 7 = RXGB (DXT5), "
                         "8 = Alpha Exponent (DXT5), 9 = YCoCg (DXT5), "
                         "10 = YCoCg scaled (DXT5))",
                         0, 10, DDS_COMPRESS_NONE,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "mipmaps",
                         "Mipmaps",
                         "How to handle mipmaps (0 = No mipmaps, "
                         "1 = Generate mipmaps, "
                         "2 = Use existing mipmaps (layers)",
                         0, 2, DDS_MIPMAP_NONE,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "save-type",
                         "Save type",
                         "How to save the image (0 = selected layer, "
                         "1 = cube map, 2 = volume map, 3 = texture array",
                         0, 3, DDS_SAVE_SELECTED_LAYER,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "format",
                         "Format",
                         "Pixel format (0 = default, 1 = DDS_FORMAT_RGB8, "
                         "2 = DDS_FORMAT_RGBA8, 3 = DDS_FORMAT_BGR8, "
                         "4 = DDS_FORMAT_ABGR8, 5 = DDS_FORMAT_R5G6B5, "
                         "6 = DDS_FORMAT_RGBA4, 7 = DDS_FORMAT_RGB5A1, "
                         "8 = DDS_FORMAT_RGB10A2, 9 = DDS_FORMAT_R3G3B2, "
                         "10 = DDS_FORMAT_A8, 11 = DDS_FORMAT_L8, "
                         "12 = DDS_FORMAT_L8A8, 13 = DDS_FORMAT_AEXP, "
                         "14 = DDS_FORMAT_YCOCG)",
                         0, 14, DDS_FORMAT_DEFAULT,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "transparent-color",
                             "Transparent color",
                             "Make an indexed color transparent",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "transparent-index",
                         "Transparent index",
                         "Index of transparent color or -1 to disable "
                         "(for indexed images only).",
                         0, 255, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "mipmap-filter",
                         "Mipmap filter",
                         "Filtering to use when generating mipmaps "
                         "(0 = default, 1 = nearest, 2 = box, 3 = triangle, "
                         "4 = quadratic, 5 = bspline, 6 = mitchell, "
                         "7 = lanczos, 8 = kaiser)",
                         0, 8, DDS_MIPMAP_FILTER_DEFAULT,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "mipmap-wrap",
                         "Mipmap wrap",
                         "Wrap mode to use when generating mipmaps "
                         "(0 = default, 1 = mirror, 2 = repeat, 3 = clamp)",
                         0, 3, DDS_MIPMAP_WRAP_DEFAULT,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "gamma-correct",
                             "Gamme correct",
                             "Use gamma correct mipmap filtering",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "srgb",
                             "sRGB",
                             "Use sRGB colorspace for gamma correction",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "gamma",
                            "Gamma",
                            "Gamma value to use for gamma correction (i.e. 2.2)",
                            0.0, 10.0, 0.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "perceptual-metric",
                             "Perceptual metric",
                             "Use a perceptual error metric during compression",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "preserve-alpha-coverage",
                             "Preserve alpha coverage",
                             "Preserve alpha test converage for alpha "
                             "channel maps",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "alpha-test-threshold",
                            "Alpha test threshold",
                            "Alpha test threshold value for which alpha test "
                            "converage should be preserved",
                            0.0, 1.0, 0.5,
                            G_PARAM_READWRITE);
    }
#if 0
  else if (! strcmp (name, DECODE_YCOCG_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            dds_decode, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGBA");

      gimp_procedure_set_menu_label (procedure, N_("Decode YCoCg"));
      /* gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Colors"); */

      gimp_procedure_set_documentation (procedure,
                                        "Converts YCoCg encoded pixels to RGB",
                                        "Converts YCoCg encoded pixels to RGB",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Shawn Kirst",
                                      "Shawn Kirst",
                                      "2008");
    }
  else if (! strcmp (name, DECODE_YCOCG_SCALED_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            dds_decode, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGBA");

      gimp_procedure_set_menu_label (procedure, N_("Decode YCoCg (scaled)"));
      /* gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Colors"); */

      gimp_procedure_set_documentation (procedure,
                                        "Converts YCoCg (scaled) encoded "
                                        "pixels to RGB",
                                        "Converts YCoCg (scaled) encoded "
                                        "pixels to RGB",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Shawn Kirst",
                                      "Shawn Kirst",
                                      "2008");
    }
  else if (! strcmp (name, DECODE_ALPHA_EXP_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            dds_decode, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGBA");

      gimp_procedure_set_menu_label (procedure, N_("Decode Alpha exponent"));
      /* gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Colors"); */

      gimp_procedure_set_documentation (procedure,
                                        "Converts alpha exponent encoded "
                                        "pixels to RGB",
                                        "Converts alpha exponent encoded "
                                        "pixels to RGB",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Shawn Kirst",
                                      "Shawn Kirst",
                                      "2008");
    }
#endif

  return procedure;
}

static GimpValueArray *
dds_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpProcedureConfig *config;
  GimpValueArray      *return_vals;
  GimpPDBStatusType    status;
  GimpImage           *image;
  GError              *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  config = gimp_procedure_create_config (procedure);
  gimp_procedure_config_begin_run (config, NULL, run_mode, args);

  status = read_dds (file, &image, run_mode == GIMP_RUN_INTERACTIVE,
                     procedure, G_OBJECT (config));

  gimp_procedure_config_end_run (config, status);
  g_object_unref (config);

  if (status != GIMP_PDB_SUCCESS)
    return gimp_procedure_new_return_values (procedure, status, error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
dds_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpProcedureConfig *config;
  GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
  GimpExportReturn     export = GIMP_EXPORT_CANCEL;
  GError              *error = NULL;
  gdouble              gamma;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  config = gimp_procedure_create_config (procedure);
  gimp_procedure_config_begin_run (config, NULL, run_mode, args);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init ("dds");

      export = gimp_export_image (&image, &n_drawables, &drawables, "DDS",
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                  GIMP_EXPORT_CAN_HANDLE_LAYERS);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  g_object_get (config,
                "gamma", &gamma,
                NULL);

  /* gimp_gamma () got removed and was always returning 2.2 anyway.
   * XXX Review this piece of code if we expect gamma value could be
   * parameterized.
   */
  if (gamma < 1e-04f)
    g_object_set (config,
                  "gamma", 2.2,
                  NULL);

  /* TODO: support multiple-layers selection, especially as DDS has
   * DDS_SAVE_SELECTED_LAYER option support.
   */
  status = write_dds (file, image, drawables[0],
                      run_mode == GIMP_RUN_INTERACTIVE,
                      procedure, G_OBJECT (config));

  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

  gimp_procedure_config_end_run (config, status);
  g_object_unref (config);

  return gimp_procedure_new_return_values (procedure, status, error);
}

#if 0
static GimpValueArray *
dds_decode (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GimpDrawable         *drawable,
            const GimpValueArray *args,
            gpointer              run_data)
{
  const gchar *name = gimp_procedure_get_name (procedure);

  if (! strcmp (name, DECODE_YCOCG_PROC))
    {
      decode_ycocg_image (drawable, TRUE);
    }
  else if (! strcmp (name, DECODE_YCOCG_SCALED_PROC))
    {
      decode_ycocg_scaled_image (drawable, TRUE);
    }
  else if (! strcmp (name, DECODE_ALPHA_EXP_PROC))
    {
      decode_alpha_exp_image (drawable, TRUE);
    }

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
#endif
