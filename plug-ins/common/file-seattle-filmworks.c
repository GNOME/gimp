/*   GIMP - The GNU Image Manipulation Program
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Seattle FilmWorks image plug-in
 *
 *   Copyright (C) 2025 Alex S.
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


#define LOAD_PROC      "file-seattle-filmworks-load"
#define PLUG_IN_BINARY "file-seattle-filmworks"
#define PLUG_IN_ROLE   "gimp-file-seattle-filmworks"

typedef struct _Seattle_FilmWorks      Seattle_FilmWorks;
typedef struct _Seattle_FilmWorksClass Seattle_FilmWorksClass;

struct _Seattle_FilmWorks
{
  GimpPlugIn      parent_instance;
};

struct _Seattle_FilmWorksClass
{
  GimpPlugInClass parent_class;
};

guchar huffman_table [420] = {
  0xFF,0xC4,0x01,0xA2,0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,
  0x0B,0x01,0x00,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,
  0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x10,0x00,
  0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,0x7D,0x01,
  0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,
  0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,0x24,
  0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,0x29,
  0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,
  0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,
  0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,
  0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,
  0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,
  0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,0xE3,
  0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,
  0xFA,0x11,0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,
  0x02,0x77,0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,
  0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,
  0x52,0xF0,0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,
  0x1A,0x26,0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,
  0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,
  0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,
  0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,
  0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,
  0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,
  0xD9,0xDA,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,
  0xF7,0xF8,0xF9,0xFA
};

#define SEATTLE_FILMWORKS_TYPE  (seattle_filmworks_get_type ())
#define SEATTLE_FILMWORKS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SEATTLE_FILMWORKS_TYPE, Seattle_FilmWorks))

GType                   seattle_filmworks_get_type         (void) G_GNUC_CONST;


static GList          * seattle_filmworks_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * seattle_filmworks_create_procedure (GimpPlugIn            *plug_in,
                                                            const gchar           *name);

static GimpValueArray * seattle_filmworks_load             (GimpProcedure         *procedure,
                                                            GimpRunMode            run_mode,
                                                            GFile                 *file,
                                                            GimpMetadata          *metadata,
                                                            GimpMetadataLoadFlags *flags,
                                                            GimpProcedureConfig   *config,
                                                            gpointer               run_data);

static GimpImage      * load_image                         (GFile                 *file,
                                                            GObject               *config,
                                                            GimpRunMode            run_mode,
                                                            GError               **error);

static GimpImage      * load_sfw93a                        (FILE                  *fp,
                                                            gsize                  file_size,
                                                            GError               **error);

static guint32          skip_to_next_marker                (guchar                *data,
                                                            gint                   index);
static guint            fix_marker                         (guchar                *data,
                                                            gint                   index);

G_DEFINE_TYPE (Seattle_FilmWorks, seattle_filmworks, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SEATTLE_FILMWORKS_TYPE)
DEFINE_STD_SET_I18N


static void
seattle_filmworks_class_init (Seattle_FilmWorksClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = seattle_filmworks_query_procedures;
  plug_in_class->create_procedure = seattle_filmworks_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
seattle_filmworks_init (Seattle_FilmWorks *seattle_filmworks)
{
}

static GList *
seattle_filmworks_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
seattle_filmworks_create_procedure (GimpPlugIn  *plug_in,
                                    const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           seattle_filmworks_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     _("Seattle FilmWorks Image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load file in the Seattle FilmWorks "
                                          "file format"),
                                        _("Load file in the Seattle FilmWorks "
                                          "file format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2025");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-seattle-filmcamera");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "sfw");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,SFW93A,0,string,SFW94A");
    }

  return procedure;
}

static GimpValueArray *
seattle_filmworks_load (GimpProcedure         *procedure,
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

/* Implementation references code by Joe Nord, available at
 * https://www.joenord.com/apps/sfwjpg/ */
static GimpImage *
load_image (GFile        *file,
            GObject      *config,
            GimpRunMode   run_mode,
            GError      **error)
{
  GimpImage  *image = NULL;
  gchar       magic[7];
  gsize       file_size;
  FILE       *fp;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  fseek (fp, 0, SEEK_END);
  file_size = ftell (fp);
  fseek (fp, 0, SEEK_SET);

  if (fread (magic, 1, 6, fp) != 6)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Invalid file."));
      fclose (fp);
      return NULL;
    }
  magic[6] = '\0';

  /* Read magic number to determine type */
  if (strcmp (magic, "SFW93A") == 0)
    {
      image = load_sfw93a (fp, file_size, error);

      if (image == NULL)
        {
          fclose (fp);
          return NULL;
        }
    }
  else if (strcmp (magic, "SFW94A") == 0)
    {
      GimpProcedure  *procedure;
      GimpValueArray *return_vals     = NULL;
      GFile          *temp_file       = NULL;
      FILE           *temp_fp;
      guchar          data[file_size - 0xE0];
      gint            jpeg_start;
      gint            jpeg_data;
      guint           index           = 0;
      guint           metadata_index  = 0;
      guint           metadata_len[2] = { 0, 0 };
      gchar          *roll_num;
      gchar          *photo_date;
      gint            fixed_marker;
      gboolean        has_huffman_table = FALSE;

      /* Load Camera Roll Number and Date metadata first */
      if (fseek (fp, 0xE0, SEEK_SET) != 0 ||
          fread (&data, file_size - 0xE0, 1, fp) != 1)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Invalid file."));
          fclose (fp);
          return NULL;
        }

      while (index < file_size && data[index])
        {
          if (data[index] == 0x20)
            metadata_len[metadata_index++] = index;

          index++;
        }

      roll_num   = g_malloc (metadata_len[0] + 1);
      photo_date = g_malloc (metadata_len[1] + 1);

      fseek (fp, 0xE0, SEEK_SET);
      fread (roll_num, metadata_len[0], 1, fp);
      roll_num[metadata_len[0]] = '\0';

      fseek (fp, 0xE0 + metadata_len[1] + 1, SEEK_SET);
      fread (photo_date, index - metadata_len[1], 1, fp);
      photo_date[metadata_len[1]] = '\0';

      fclose (fp);

      /* Convert SFW to JPEG format */
      while (index < file_size - 4)
        {
          if (data[index]     == 0xFF &&
              data[index + 1] == 0xC8 &&
              data[index + 2] == 0xFF &&
              data[index + 3] == 0xD0)
            break;

          index++;
        }

      /* Uncompressed SFW94A are convolved BMPs,
       * which aren't yet supported. */
      if (index >= file_size - 4)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Uncompressed SFW94A format is not yet supported."));
          return NULL;
        }

      jpeg_start = index;

      /* Correct SOI and APP0 markers */
      data[index + 1] = 0xD8;
      data[index + 3] = 0xE0;

      index += 6;
      if (index > file_size - 13)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Invalid file."));
          return NULL;
        }

      data[index++] = 'J';
      data[index++] = 'F';
      data[index++] = 'I';
      data[index++] = 'F';
      data[index++] = 0;
      data[index++] = 1;
      data[index++] = 0;

      /* Skip to next marker */
      index = jpeg_start + 4;
      index += skip_to_next_marker (data, jpeg_start + 4);

      if (index > file_size)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Invalid file."));
          return NULL;
        }

      /* Fix other markers */
      while (TRUE)
        {
          if (index + 1 > file_size)
            {
              g_set_error (error,
                           G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Invalid file."));
              return NULL;
            }

          fixed_marker = fix_marker (data, index);

          if (fixed_marker < 0)
            {
              g_set_error (error,
                           G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Invalid file."));
              return NULL;
            }

          if (fixed_marker == 0xC4)
            has_huffman_table = TRUE;
          else if (fixed_marker == 0xDA)
            break;

          index += skip_to_next_marker (data, index + 2) + 2;
        }
      jpeg_data = index;

      /* Search for ending marker */
      while (index < file_size - 2)
        {
          if (data[index]     == 0xFF &&
              data[index + 1] == 0xC9)
            break;

          index++;
        }

      data[index + 1] = 0xD9;

      /* Load as JPEG */
      temp_file = gimp_temp_file ("jpeg");
      temp_fp   = g_fopen (g_file_peek_path (temp_file), "wb");

      if (! temp_fp)
        {
          g_file_delete (temp_file, NULL, NULL);
          g_object_unref (temp_file);

          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Unable to convert SFW image."));
          return NULL;
        }

      /* Write header */
      fwrite (data + jpeg_start, 1, jpeg_data - jpeg_start, temp_fp);

      /* Write huffman table if necessary */
      if (! has_huffman_table)
        fwrite (huffman_table, 1, 420, temp_fp);

      /* Finish writing JPEG */
      fwrite (data + jpeg_data, sizeof (guchar), file_size - 0xE0 - jpeg_data, temp_fp);
      fclose (temp_fp);

      procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (), "file-jpeg-load");
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

      if (image)
        gimp_image_flip (image, GIMP_ORIENTATION_VERTICAL);

      if (roll_num && photo_date)
        {
          GimpParasite *parasite;
          gchar        *comment;
          gchar        *verified_comment;

          comment = g_strdup_printf ("%s\n%s", roll_num, photo_date);

          g_free (roll_num);
          g_free (photo_date);

          verified_comment = g_convert (comment, -1, "utf-8", "iso8859-1",
                                        NULL, NULL, NULL);

          if (verified_comment)
            g_free (comment);
          else
            verified_comment = comment;

          if (! g_utf8_validate (verified_comment, -1, NULL))
            {
              g_printerr ("Invalid comment ignored.\n");
            }
          else
            {
              parasite = gimp_parasite_new ("gimp-comment",
                                            GIMP_PARASITE_PERSISTENT,
                                            strlen (verified_comment) + 1,
                                            verified_comment);
              gimp_image_attach_parasite (image, parasite);
              gimp_parasite_free (parasite);

              g_free (verified_comment);
            }
        }
    }
  else
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Unsupported SFW format '%s'"), magic);
      fclose (fp);
      return NULL;
    }

  return image;
}

static GimpImage *
load_sfw93a (FILE    *fp,
             gsize    file_size,
             GError **error)
{
  /* SFW93A files are basically JPEGs with an extra 30 bytes
     in the header */
  GimpProcedure  *procedure;
  GimpValueArray *return_vals = NULL;
  GimpImage      *image       = NULL;
  GFile          *temp_file   = NULL;
  FILE           *temp_fp;
  guchar          data[file_size - 30];

  if (fseek (fp, 30, SEEK_SET) < 0 ||
      fread (&data, file_size - 30, 1, fp) != 1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Invalid file."));
      return NULL;
    }

  fclose (fp);

  temp_file = gimp_temp_file ("jpeg");
  temp_fp   = g_fopen (g_file_peek_path (temp_file), "wb");

  if (! temp_fp)
    {
      g_file_delete (temp_file, NULL, NULL);
      g_object_unref (temp_file);

      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Unable to convert SFW image."));
      return NULL;
    }

  fwrite (data, sizeof (guchar), file_size - 30, temp_fp);
  fclose (temp_fp);

  procedure   = gimp_pdb_lookup_procedure (gimp_get_pdb (), "file-jpeg-load");
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

static guint32
skip_to_next_marker (guchar *data,
                     gint    index)
{
  guint32 skip_index;

  skip_index = (256 * data[index]) + data[index + 1];
  return skip_index;
}

static guint
fix_marker (guchar *data,
            gint    index)
{
  if (data[index] != 0xFF)
    return -1;

  /* Only certain markers were changed */
  if (data[index + 1] == 0xA0 ||
      data[index + 1] == 0xA4)
    {
      data[index + 1] += 0x20;
    }
  else if (data[index + 1] == 0xC8 ||
           data[index + 1] == 0xC9 ||
           data[index + 1] == 0xCA ||
           data[index + 1] == 0xCB ||
           data[index + 1] == 0xD0)
    {
      data[index + 1] += 0x10;
    }

  return data[index + 1];
}
