/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/* #define ICO_DBG */

#include "ico.h"
#include "ico-load.h"
#include "ico-save.h"

#include "libgimp/stdplugins-intl.h"

#define LOAD_PROC        "file-ico-load"
#define LOAD_THUMB_PROC  "file-ico-load-thumb"
#define SAVE_PROC        "file-ico-save"


typedef struct _Ico      Ico;
typedef struct _IcoClass IcoClass;

struct _Ico
{
  GimpPlugIn      parent_instance;
};

struct _IcoClass
{
  GimpPlugInClass parent_class;
};


#define ICO_TYPE  (ico_get_type ())
#define ICO (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ICO_TYPE, Ico))

GType                   ico_get_type         (void) G_GNUC_CONST;

static GList          * ico_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * ico_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * ico_load             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * ico_load_thumb       (GimpProcedure        *procedure,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * ico_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              gint32                image_id,
                                              gint32                drawable_id,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);


G_DEFINE_TYPE (Ico, ico, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (ICO_TYPE)


static void
ico_class_init (IcoClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = ico_query_procedures;
  plug_in_class->create_procedure = ico_create_procedure;
}

static void
ico_init (Ico *ico)
{
}

static GList *
ico_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
ico_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name, GIMP_PLUGIN,
                                           ico_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("Microsoft Windows icon"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_BRUSH);

      gimp_procedure_set_documentation (procedure,
                                        "Loads files of Windows ICO file format",
                                        "Loads files of Windows ICO file format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>",
                                      "Christian Kreibich <christian@whoop.org>",
                                      "2002");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-ico");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "ico");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,\\000\\001\\000\\000,0,string,\\000\\002\\000\\000");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name, GIMP_PLUGIN,
                                      ico_load_thumb, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Loads a preview from an Windows ICO file",
                                        "",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann",
                                      "Sven Neumann <sven@gimp.org>",
                                      "2005");

      GIMP_PROC_ARG_STRING (procedure, "uri",
                            "URI",
                            "URI of the file to load",
                            NULL,
                            GIMP_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "thumb-size",
                         "Thumb Size",
                         "Preferred thumbnail size",
                         16, 2014, 256,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_VAL_IMAGE (procedure, "image",
                           "Image",
                           "Thumbnail image",
                           FALSE,
                           GIMP_PARAM_READWRITE);

      GIMP_PROC_VAL_INT (procedure, "image-width",
                         "Image width",
                         "Width of the full-sized image",
                         1, GIMP_MAX_IMAGE_SIZE, 1,
                         GIMP_PARAM_READWRITE);

      GIMP_PROC_VAL_INT (procedure, "image-height",
                         "Image height",
                         "Height of the full-sized image",
                         1, GIMP_MAX_IMAGE_SIZE, 1,
                         GIMP_PARAM_READWRITE);
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name, GIMP_PLUGIN,
                                           ico_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("Microsoft Windows icon"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_BRUSH);

      gimp_procedure_set_documentation (procedure,
                                        "Saves files in Windows ICO file format",
                                        "Saves files in Windows ICO file format",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>",
                                      "Christian Kreibich <christian@whoop.org>",
                                      "2002");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-ico");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "ico");
    }

  return procedure;
}

static GimpValueArray *
ico_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  gchar          *filename;
  gint32          image_id;
  GError         *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  filename = g_file_get_path (file);

  image_id = ico_load_image (filename, &error);

  g_free (filename);

  if (image_id < 1)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  gimp_value_set_image_id (gimp_value_array_index (return_vals, 1),
                           image_id);

  return return_vals;
}

static GimpValueArray *
ico_load_thumb (GimpProcedure        *procedure,
                const GimpValueArray *args,
                gpointer              run_data)
{
  GimpValueArray *return_vals;
  const gchar    *filename;
  gint            width;
  gint            height;
  gint32          image_id;
  GError         *error = NULL;
  INIT_I18N ();
  gegl_init (NULL, NULL);

  filename = g_value_get_string (gimp_value_array_index (args, 0));
  width    = g_value_get_int    (gimp_value_array_index (args, 1));
  height   = width;

  image_id = ico_load_thumbnail_image (filename,
                                       &width, &height, &error);

  if (image_id < 1)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  gimp_value_set_image_id (gimp_value_array_index (return_vals, 1), image_id);
  g_value_set_int         (gimp_value_array_index (return_vals, 2), width);
  g_value_set_int         (gimp_value_array_index (return_vals, 3), height);

  return return_vals;
}

static GimpValueArray *
ico_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          gint32                image_id,
          gint32                drawable_id,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpPDBStatusType  status;
  gchar             *filename;
  GError            *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  filename = g_file_get_path (file);

  status = ico_save_image (filename, image_id, run_mode, &error);

  g_free (filename);

  return gimp_procedure_new_return_values (procedure, status, error);
}

gint
ico_rowstride (gint width,
               gint bpp)
{
  switch (bpp)
    {
    case 1:
      if ((width % 32) == 0)
        return width / 8;
      else
        return 4 * (width/32 + 1);
      break;

    case 4:
      if ((width % 8) == 0)
        return width / 2;
      else
        return 4 * (width/8 + 1);
      break;

    case 8:
      if ((width % 4) == 0)
        return width;
      else
        return 4 * (width/4 + 1);
      break;

    case 24:
      if (((width*3) % 4) == 0)
        return width * 3;
      else
        return 4 * (width*3/4+1);

    case 32:
      return width * 4;

    default:
      g_warning ("invalid bitrate: %d\n", bpp);
      g_assert_not_reached ();
      return width * (bpp/8);
    }
}

guint8 *
ico_alloc_map (gint  width,
               gint  height,
               gint  bpp,
               gint *length)
{
  gint    len = 0;
  guint8 *map = NULL;

  len = ico_rowstride (width, bpp) * height;

  *length = len;
  map = g_new0 (guint8, len);

  return map;
}
