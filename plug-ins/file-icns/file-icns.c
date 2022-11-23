/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * file-icns.c
 * Copyright (C) 2004 Brion Vibber <brion@pobox.com>
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
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

/* #define ICNS_DBG */

#include "file-icns.h"
#include "file-icns-load.h"
#include "file-icns-save.h"

#include "libligma/stdplugins-intl.h"

#define LOAD_PROC           "file-icns-load"
#define LOAD_THUMB_PROC     "file-icns-load-thumb"
#define SAVE_PROC           "file-icns-save"


typedef struct _Icns      Icns;
typedef struct _IcnsClass IcnsClass;

struct _Icns
{
  LigmaPlugIn      parent_instance;
};

struct _IcnsClass
{
  LigmaPlugInClass parent_class;
};


#define ICNS_TYPE  (icns_get_type ())
#define ICNS (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ICNS_TYPE, Icns))

GType                   icns_get_type         (void) G_GNUC_CONST;

static GList          * icns_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * icns_create_procedure (LigmaPlugIn           *plug_in,
                                               const gchar          *name);

static LigmaValueArray * icns_load             (LigmaProcedure        *procedure,
                                               LigmaRunMode           run_mode,
                                               GFile                *file,
                                               const LigmaValueArray *args,
                                               gpointer              run_data);
static LigmaValueArray * icns_load_thumb       (LigmaProcedure        *procedure,
                                               GFile                *file,
                                               gint                  size,
                                               const LigmaValueArray *args,
                                               gpointer              run_data);
static LigmaValueArray * icns_save             (LigmaProcedure        *procedure,
                                               LigmaRunMode           run_mode,
                                               LigmaImage            *image,
                                               gint                  n_drawables,
                                               LigmaDrawable        **drawables,
                                               GFile                *file,
                                               const LigmaValueArray *args,
                                               gpointer              run_data);


G_DEFINE_TYPE (Icns, icns, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (ICNS_TYPE)
DEFINE_STD_SET_I18N

static void
icns_class_init (IcnsClass *klass)
{
  LigmaPlugInClass *plug_in_class  = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = icns_query_procedures;
  plug_in_class->create_procedure = icns_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
icns_init (Icns *icns)
{
}

static GList *
icns_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static LigmaProcedure *
icns_create_procedure (LigmaPlugIn  *plug_in,
                       const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           icns_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, N_("Icns"));

      ligma_procedure_set_documentation (procedure,
                                        "Loads files in Apple Icon Image format",
                                        "Loads Apple Icon Image files.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Brion Vibber <brion@pobox.com>",
                                      "Brion Vibber <brion@pobox.com>",
                                      "2004");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-icns");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "icns");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,string,\x69\x63\x6E\x73");

      ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = ligma_thumbnail_procedure_new (plug_in, name,
                                                LIGMA_PDB_PROC_TYPE_PLUGIN,
                                                icns_load_thumb, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Loads a preview from an Apple Icon Image file",
                                        "",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Brion Vibber <brion@pobox.com>",
                                      "Brion Vibber <brion@pobox.com>",
                                      "2004");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           icns_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure, _("Apple Icon Image"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_BRUSH);

      ligma_procedure_set_documentation (procedure,
                                        "Saves files in Apple Icon Image file format",
                                        "Saves files in Apple Icon Image file format",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Brion Vibber <brion@pobox.com>",
                                      "Brion Vibber <brion@pobox.com>",
                                      "2004");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-icns");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "icns");
    }

  return procedure;
}

static LigmaValueArray *
icns_load (LigmaProcedure        *procedure,
           LigmaRunMode           run_mode,
           GFile                *file,
           const LigmaValueArray *args,
           gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error       = NULL;

  gegl_init (NULL, NULL);

  image = icns_load_image (file, NULL, &error);

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
icns_load_thumb (LigmaProcedure        *procedure,
                 GFile                *file,
                 gint                  size,
                 const LigmaValueArray *args,
                 gpointer              run_data)
{
  LigmaValueArray *return_vals;
  gint            width;
  gint            height;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  width  = size;
  height = size;

  image = icns_load_thumbnail_image (file,
                                     &width, &height, 0, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);
  LIGMA_VALUES_SET_INT   (return_vals, 2, width);
  LIGMA_VALUES_SET_INT   (return_vals, 3, height);

  ligma_value_array_truncate (return_vals, 4);

  return return_vals;
}

static LigmaValueArray *
icns_save (LigmaProcedure        *procedure,
           LigmaRunMode           run_mode,
           LigmaImage            *image,
           gint                  n_drawables,
           LigmaDrawable        **drawables,
           GFile                *file,
           const LigmaValueArray *args,
           gpointer              run_data)
{
  LigmaPDBStatusType  status;
  GError            *error = NULL;

  gegl_init (NULL, NULL);

  status = icns_save_image (file, image, run_mode, &error);

  return ligma_procedure_new_return_values (procedure, status, error);
}

/* Buffer should point to *at least 5 byte buffer*! */
void
fourcc_get_string (gchar *fourcc,
                   gchar *buf)
{
  buf = fourcc;
  buf[4] = 0;
}
