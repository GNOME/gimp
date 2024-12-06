/*   GIMP - The GNU Image Manipulation Program
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Windows Shortcut (.lnk) plug-in
 *
 *   Copyright (C) 2024 Alex S.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC       "file-lnk-load"
#define PLUG_IN_BINARY  "file-lnk"
#define PLUG_IN_ROLE    "gimp-file-lnk"

typedef struct _Lnk      Lnk;
typedef struct _LnkClass LnkClass;

struct _Lnk
{
  GimpPlugIn      parent_instance;
};

struct _LnkClass
{
  GimpPlugInClass parent_class;
};


#define LNK_TYPE  (lnk_get_type ())
#define LNK(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LNK_TYPE, Lnk))

GType                   lnk_get_type         (void) G_GNUC_CONST;


static GList          * lnk_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * lnk_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static GimpValueArray * lnk_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage      * load_image           (GFile                 *file,
                                              GObject               *config,
                                              GimpRunMode            run_mode,
                                              GError               **error);


G_DEFINE_TYPE (Lnk, lnk, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (LNK_TYPE)
DEFINE_STD_SET_I18N


static void
lnk_class_init (LnkClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = lnk_query_procedures;
  plug_in_class->create_procedure = lnk_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
lnk_init (Lnk *lnk)
{
}

static GList *
lnk_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
lnk_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           lnk_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     "Windows Shortcut");

      gimp_procedure_set_documentation (procedure,
                                        "Follows a link to an image in a "
                                        ".lnk file",
                                        "Opens a .lnk file and if it points "
                                        "to an image, it asks GIMP to open "
                                        "the linked image.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2024");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "lnk");
    }

  return procedure;
}

static GimpValueArray *
lnk_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image = NULL;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  /*  We handle PDB errors by forwarding them to the caller in
   *  our return values.
   */
  gimp_plug_in_set_pdb_error_handler (gimp_procedure_get_plug_in (procedure),
                                      GIMP_PDB_ERROR_HANDLER_PLUGIN);

  image = load_image (file, G_OBJECT (config), run_mode, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpImage *
load_image (GFile        *file,
            GObject      *config,
            GimpRunMode   run_mode,
            GError      **error)
{
  GimpImage  *image = NULL;
  FILE       *fp;
  guint32     flags;
  gsize       base;
  guint       length;
  guint       link_pos;
  guint       link_size;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  /* Skip header size and GUID, then load data flags*/
  if (fseek (fp, 20, SEEK_SET) < 0 ||
      fread (&flags, sizeof (guint32), 1, fp) != 1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Invalid file."));
      fclose (fp);
      return NULL;
    }
  flags = GUINT32_FROM_LE (flags);

  /* If HasTargetIDList bit is set, we want to skip over those too */
  if (flags & 0x01)
    {
      gushort offset;

      if (fseek (fp, 76, SEEK_SET) < 0 ||
          fread (&offset, sizeof (gushort), 1, fp) != 1)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Invalid file."));
          fclose (fp);
          return NULL;
        }

      offset = GUINT16_TO_LE (offset);
      if (fseek (fp, offset, SEEK_CUR) < 0)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Invalid file."));
          fclose (fp);
          return NULL;
        }
    }
  base = ftell (fp);

  /* Read in data length, then skip more unnecessary data */
  if (fread (&length, sizeof (guint32), 1, fp) != 1 ||
      fseek (fp, 12, SEEK_CUR) < 0                  ||
      fread (&link_pos, sizeof (guint32), 1, fp) != 1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Invalid file."));
      fclose (fp);
      return NULL;
    }

  /* Length of the filename */
  link_size = length - link_pos - 1;

  if (link_size > 0)
    {
      gchar file_name[link_size];

      /* Jump to link address and read in the real file name */
      if (fseek (fp, base + link_pos, SEEK_SET) < 0 ||
          fread (file_name, sizeof (gchar), link_size, fp) != link_size)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Invalid file."));
          fclose (fp);
          return NULL;
        }

      file_name[link_size - 1] = '\0';
      image = gimp_file_load (GIMP_RUN_NONINTERACTIVE,
                              g_file_new_for_path (file_name));

      if (! image)
        g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                     "%s", gimp_pdb_get_last_error (gimp_get_pdb ()));

    }
  fclose (fp);

  return image;
}
