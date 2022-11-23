/* bmp.c                                          */
/* Version 0.52                                   */
/* This is a File input and output filter for the */
/* Ligma. It loads and saves images in windows(TM) */
/* bitmap format.                                 */
/* Some Parts that deal with the interaction with */
/* LIGMA are taken from the GIF plugin by          */
/* Peter Mattis & Spencer Kimball and from the    */
/* PCX plugin by Francisco Bustamante.            */
/*                                                */
/* Alexander.Schulz@stud.uni-karlsruhe.de         */

/* Changes:   28.11.1997 Noninteractive operation */
/*            16.03.1998 Endian-independent!!     */
/*            21.03.1998 Little Bug-fix           */
/*            06.04.1998 Bugfix in Padding        */
/*            11.04.1998 Arch. cleanup (-Wall)    */
/*                       Parses gtkrc             */
/*            14.04.1998 Another Bug in Padding   */
/*            28.04.1998 RLE-Encoding rewritten   */
/*            29.10.1998 Changes by Tor Lillqvist */
/*                       <tml@iki.fi> to support  */
/*                       16 and 32 bit images     */
/*            28.11.1998 Bug in RLE-read-padding  */
/*                       fixed.                   */
/*            19.12.1999 Resolution support added */
/*            06.05.2000 Overhaul for 16&24-bit   */
/*                       plus better OS/2 code    */
/*                       by njl195@zepler.org.uk  */
/*            29.06.2006 Full support for 16/32   */
/*                       bits bitmaps and support */
/*                       for alpha channel        */
/*                       by p.filiciak@zax.pl     */

/*
 * LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "bmp.h"
#include "bmp-load.h"
#include "bmp-save.h"

#include "libligma/stdplugins-intl.h"


typedef struct _Bmp      Bmp;
typedef struct _BmpClass BmpClass;

struct _Bmp
{
  LigmaPlugIn      parent_instance;
};

struct _BmpClass
{
  LigmaPlugInClass parent_class;
};


#define BMP_TYPE  (bmp_get_type ())
#define BMP (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BMP_TYPE, Bmp))

GType                   bmp_get_type         (void) G_GNUC_CONST;

static GList          * bmp_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * bmp_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * bmp_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * bmp_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);



G_DEFINE_TYPE (Bmp, bmp, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (BMP_TYPE)
DEFINE_STD_SET_I18N


static void
bmp_class_init (BmpClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = bmp_query_procedures;
  plug_in_class->create_procedure = bmp_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
bmp_init (Bmp *bmp)
{
}

static GList *
bmp_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static LigmaProcedure *
bmp_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           bmp_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("Windows BMP image"));

      ligma_procedure_set_documentation (procedure,
                                        "Loads files of Windows BMP file format",
                                        "Loads files of Windows BMP file format",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Alexander Schulz",
                                      "Alexander Schulz",
                                      "1997");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/bmp");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "bmp");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,string,BM");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           bmp_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "INDEXED, GRAY, RGB*");

      ligma_procedure_set_menu_label (procedure, _("Windows BMP image"));

      ligma_procedure_set_documentation (procedure,
                                        "Saves files in Windows BMP file format",
                                        "Saves files in Windows BMP file format",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Alexander Schulz",
                                      "Alexander Schulz",
                                      "1997");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/bmp");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "bmp");

      LIGMA_PROC_ARG_BOOLEAN (procedure, "use-rle",
                             "Use RLE",
                             "Use run-lengh-encoding compression "
                             "(only valid for 4 and 8-bit indexed images)",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "write-color-space",
                             "Write color space information",
                             "Whether or not to write BITMAPV5HEADER "
                             "color space data",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "rgb-format",
                         "RGB format",
                         "Export format for RGB images "
                         "(0=RGB_565, 1=RGBA_5551, 2=RGB_555, 3=RGB_888,"
                         " 4=RGBA_8888, 5=RGBX_8888)",
                         0, 5, 3,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
bmp_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
bmp_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaProcedureConfig *config;
  LigmaPDBStatusType    status = LIGMA_PDB_SUCCESS;
  LigmaExportReturn     export = LIGMA_EXPORT_CANCEL;
  GError              *error = NULL;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, image, run_mode, args);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "BMP",
                                  LIGMA_EXPORT_CAN_HANDLE_RGB   |
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY  |
                                  LIGMA_EXPORT_CAN_HANDLE_ALPHA |
                                  LIGMA_EXPORT_CAN_HANDLE_INDEXED);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("BMP format does not support multiple layers."));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }

  status = save_image (file, image, drawables[0], run_mode,
                       procedure, G_OBJECT (config),
                       &error);

  ligma_procedure_config_end_run (config, status);
  g_object_unref (config);

  if (export == LIGMA_EXPORT_EXPORT)
    {
      ligma_image_delete (image);
      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
}
