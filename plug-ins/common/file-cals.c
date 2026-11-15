/*   GIMP - The GNU Image Manipulation Program
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Continuous Acquisition and Life-cycle Support (CALS) Image plug-in
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


#define LOAD_PROC      "file-cals-load"
#define PLUG_IN_BINARY "file-cals"
#define PLUG_IN_ROLE   "gimp-file-cals"

typedef struct _Cals      Cals;
typedef struct _CalsClass CalsClass;

struct _Cals
{
  GimpPlugIn      parent_instance;
};

struct _CalsClass
{
  GimpPlugInClass parent_class;
};


#define CALS_TYPE  (cals_get_type ())
#define CALS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALS_TYPE, Cals))

GType                   cals_get_type         (void);


static GList          * cals_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * cals_create_procedure (GimpPlugIn            *plug_in,
                                               const gchar           *name);

static GimpValueArray * cals_load             (GimpProcedure         *procedure,
                                               GimpRunMode            run_mode,
                                               GFile                 *file,
                                               GimpMetadata          *metadata,
                                               GimpMetadataLoadFlags *flags,
                                               GimpProcedureConfig   *config,
                                               gpointer               run_data);

static GimpImage      * load_image            (GFile                 *file,
                                               GObject               *config,
                                               GimpRunMode            run_mode,
                                               GError               **error);

static GimpImage      * convert_to_tiff       (GFile                 *file,
                                               gushort                width,
                                               gushort                height,
                                               guint32                image_size,
                                               guchar                *data,
                                               GError               **error);

G_DEFINE_TYPE (Cals, cals, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (CALS_TYPE)
DEFINE_STD_SET_I18N


static void
cals_class_init (CalsClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = cals_query_procedures;
  plug_in_class->create_procedure = cals_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
cals_init (Cals *cals)
{
}

static GList *
cals_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
cals_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           cals_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     _("CALS Image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load file in the CALS file format"),
                                        NULL, name);
      gimp_procedure_set_attribution (procedure,
                                      "Alx Sa.",
                                      "Alx Sa.",
                                      "2026");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-mil,image/x-cals,image/x-cal");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "cal,cals");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,srcdocid");
    }

  return procedure;
}

static GimpValueArray *
cals_load (GimpProcedure         *procedure,
           GimpRunMode            run_mode,
           GFile                 *file,
           GimpMetadata          *metadata,
           GimpMetadataLoadFlags *flags,
           GimpProcedureConfig   *config,
           gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, G_OBJECT (config), run_mode, &error);
  gimp_progress_update (1.0);

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
  GimpImage *image;
  gchar      header_row[128];
  GString   *comment_str;
  gchar     *header_type;
  guchar    *data;
  guint      type       = 0;
  gushort    width      = 0;
  gushort    height     = 0;
  gushort    rotation   = 0;
  gushort    mirroring  = 0;
  guint32    resolution = 0;
  guint32    image_size;
  gsize      file_size;
  FILE      *fp;

  fp = g_fopen (g_file_peek_path (file), "rb");
  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  fseek (fp, 0, SEEK_END);
  file_size = ftell (fp);
  fseek (fp, 0, SEEK_SET);

  comment_str = g_string_new (NULL);
  for (gint i = 0; i < 16; i++)
    {
      if (fread (header_row, 128, 1, fp) == 0)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Could not read header from '%s'"),
                       gimp_file_get_utf8_name (file));
          fclose (fp);
          return NULL;
        }
      header_row[127] = '\0';

      header_type = g_ascii_strdown (header_row, -1);
      if (g_str_has_prefix (header_type, "rtype: "))
        {
          type = atoi (g_strstrip (header_row + 7));
        }
      else if (g_str_has_prefix (header_type, "rdensty: "))
        {
          resolution = atoi (g_strstrip (header_row + 9));
        }
      else if (g_str_has_prefix (header_type, "rpelcnt: "))
        {
          /* Entries are fixed width */
          header_row[15] = '\0';

          width  = atoi (g_strstrip (header_row + 9));
          height = atoi (g_strstrip (header_row + 16));
        }
      else if (g_str_has_prefix (header_type, "rorient: "))
        {
          header_row[12] = '\0';

          rotation  = atoi (g_strstrip (header_row + 9));
          mirroring = atoi (g_strstrip (header_row + 13));
        }
      else if (i < 11) /* After 12 rows, it's just padding */
        {
          g_string_append_printf (comment_str, "%s\n",
                                  g_strstrip (header_row));
        }
      g_free (header_type);
    }
  file_size -= 2048;

  if (type != 1)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("CALS type %d not yet supported"), type);
      fclose (fp);
      g_string_free (comment_str, TRUE);
      return NULL;
    }
  image_size = (guint32) file_size;

  if (width  == 0 ||
      height == 0 ||
      ! (data = g_try_malloc0 (file_size)))
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("There was not enough memory to complete the "
                     "operation."));
      fclose (fp);
      g_string_free (comment_str, TRUE);
      return NULL;
    }

  if (fread (data, file_size, 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read header from '%s'"),
                   gimp_file_get_utf8_name (file));
      fclose (fp);
      g_free (data);
      g_string_free (comment_str, TRUE);
      return NULL;
    }
  fclose (fp);

  /* Use libtiff to load CCITT Group 4 compressed data by converting
   * the data into a TIFF. */
  image = convert_to_tiff (file, width, height, image_size, data, error);
  g_free (data);

  if (image)
    {
      GimpParasite *comment_parasite = NULL;
      gchar        *comment;

      gimp_image_set_resolution (image, resolution, resolution);
      if (rotation != 0)
        {
          if (rotation == 90)
            gimp_image_rotate (image, GIMP_ROTATE_DEGREES270);
          else if (rotation == 180)
            gimp_image_rotate (image, GIMP_ROTATE_DEGREES180);
          else if (rotation == 270)
            gimp_image_rotate (image, GIMP_ROTATE_DEGREES90);
        }
      if (mirroring == 90)
        gimp_image_flip (image, GIMP_ORIENTATION_HORIZONTAL);

      comment = g_string_free (comment_str, FALSE);

      comment_parasite = gimp_parasite_new ("gimp-comment",
                                      GIMP_PARASITE_PERSISTENT,
                                      strlen (comment) + 1, comment);
      gimp_image_attach_parasite (image, comment_parasite);

      gimp_parasite_free (comment_parasite);
      g_free (comment);
    }

  return image;
}

/* Inspired by Shreevatsa, https://shreevatsa.github.io/site/ccitt.html */
static GimpImage *
convert_to_tiff (GFile    *file,
                 gushort   width,
                 gushort   height,
                 guint32   image_size,
                 guchar   *data,
                 GError  **error)
{
  GFile          *temp_file;
  FILE           *temp_fp;
  gushort         temp_short;
  guint32         temp_int;
  GimpProcedure  *procedure;
  GimpImage      *image            = NULL;
  GimpValueArray *return_vals      = NULL;
  const gushort   num_tags         = 8;
  const guint32   header_size      = 110;
  const guint32   tag_values[8][4] = {
                                       { 256, 4, 1, width },
                                       { 257, 4, 1, height },
                                       { 258, 3, 1, 1 },
                                       { 259, 3, 1, 4 },
                                       { 262, 3, 1, 0 },
                                       { 273, 4, 1, header_size },
                                       { 278, 4, 1, height },
                                       { 279, 4, 1, image_size }
                                     };

  temp_file = gimp_temp_file ("tiff");
  temp_fp   = g_fopen (g_file_peek_path (temp_file), "wb");

  if (! temp_fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      g_file_delete (temp_file, NULL, NULL);
      g_object_unref (temp_file);
      return NULL;
    }

  /* Write header */
  fwrite ("II", 1, 2, temp_fp);
  temp_short = 0x2A;
  fwrite (&temp_short, 1, 2, temp_fp);
  temp_int = 8;
  fwrite (&temp_int, 1, 4, temp_fp);

  /* Number of Tags */
  fwrite (&num_tags, 1, 2, temp_fp);

  /* Tags */
  for (gint i = 0; i < num_tags; i++)
    {
      gushort tag_id   = (gushort) tag_values[i][0];
      gushort tag_type = (gushort) tag_values[i][1];

      fwrite (&tag_id, 1, 2, temp_fp);
      fwrite (&tag_type, 1, 2, temp_fp);
      fwrite (&tag_values[i][2], 1, 4, temp_fp);
      fwrite (&tag_values[i][3], 1, 4, temp_fp);
    }

  /* End of Tags */
  temp_int = 0;
  fwrite (&temp_int, 1, 4, temp_fp);

  /* Image data */
  fwrite (data, image_size, 1, temp_fp);
  fclose (temp_fp);

  procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (), "file-tiff-load");
  return_vals = gimp_procedure_run (procedure,
                                    "run-mode", GIMP_RUN_NONINTERACTIVE,
                                    "file",     temp_file,
                                    NULL);

  if (return_vals)
    {
      image = g_value_get_object (gimp_value_array_index (return_vals, 1));
      g_clear_pointer (&return_vals, gimp_value_array_unref);
    }

  g_file_delete (temp_file, NULL, NULL);
  g_object_unref (temp_file);

  return image;
}
