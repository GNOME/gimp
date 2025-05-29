/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-rawtherapee.c -- raw file format plug-in that uses RawTherapee
 * Copyright (C) 2012 Simon Budig <simon@gimp.org>
 * Copyright (C) 2016 Tobias Ellinghaus <me@houz.org>
 * Copyright (C) 2017 Alberto Griggio <alberto.griggio@gmail.com>
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

#include <glib/gstdio.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"

#include "file-raw-formats.h"
#include "file-raw-utils.h"


#define LOAD_THUMB_PROC   "file-another-rawtherapee-load-thumb"
#define REGISTRY_KEY_BASE "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ART"


typedef struct _AnotherRawtherapee      AnotherRawtherapee;
typedef struct _AnotherRawtherapeeClass AnotherRawtherapeeClass;

struct _AnotherRawtherapee
{
  GimpPlugIn      parent_instance;
};

struct _AnotherRawtherapeeClass
{
  GimpPlugInClass parent_class;
};


#define ANOTHERRAWTHERAPEE_TYPE  (anotherrawtherapee_get_type ())
#define ANOTHERRAWTHERAPEE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANOTHERRAWTHERAPEE_TYPE, AnotherRawtherapee))

GType                   anotherrawtherapee_get_type         (void) G_GNUC_CONST;

static GList          * anotherrawtherapee_init_procedures  (GimpPlugIn            *plug_in);
static GimpProcedure  * anotherrawtherapee_create_procedure (GimpPlugIn            *plug_in,
                                                             const gchar           *name);

static GimpValueArray * anotherrawtherapee_load             (GimpProcedure         *procedure,
                                                             GimpRunMode            run_mode,
                                                             GFile                 *file,
                                                             GimpMetadata          *metadata,
                                                             GimpMetadataLoadFlags *flags,
                                                             GimpProcedureConfig   *config,
                                                             gpointer               run_data);
static GimpValueArray * anotherrawtherapee_load_thumb       (GimpProcedure         *procedure,
                                                             GFile                 *file,
                                                             gint                   size,
                                                             GimpProcedureConfig   *config,
                                                             gpointer               run_data);

static GimpImage      * load_image                          (GFile                 *file,
                                                             GimpRunMode            run_mode,
                                                             GError               **error);
static GimpImage      * load_thumbnail_image                (GFile                 *file,
                                                             gint                   thumb_size,
                                                             GError               **error);


G_DEFINE_TYPE (AnotherRawtherapee, anotherrawtherapee, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (ANOTHERRAWTHERAPEE_TYPE)
DEFINE_STD_SET_I18N


static void
anotherrawtherapee_class_init (AnotherRawtherapeeClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->init_procedures  = anotherrawtherapee_init_procedures;
  plug_in_class->create_procedure = anotherrawtherapee_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
anotherrawtherapee_init (AnotherRawtherapee *art)
{
}

static GList *
anotherrawtherapee_init_procedures (GimpPlugIn *plug_in)
{
  /* check if ART is installed
   * TODO: allow setting the location of the executable in preferences
   */
  gboolean  search_path      = FALSE;
  gchar    *exec_path        = file_raw_get_executable_path ("ART", "-cli",
                                                             "ART_EXECUTABLE",
                                                             NULL,
                                                             REGISTRY_KEY_BASE,
                                                             &search_path);
#ifdef G_OS_WIN32
  /* Issue #2716 - Prevent ART from opening a console window */
  gchar    *argv[]     = { exec_path, "-v", "-w", NULL };
#else
  gchar    *argv[]     = { exec_path, "-v", NULL };
#endif
  gchar    *art_stdout = NULL;
  gboolean  have_art   = FALSE;
  gint      i;

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    (search_path ? G_SPAWN_SEARCH_PATH : 0) |
                    G_SPAWN_STDERR_TO_DEV_NULL,
                    NULL,
                    NULL,
                    &art_stdout,
                    NULL,
                    NULL,
                    NULL))
    {
      gint rtmajor = 0;
      gint rtminor = 0;

      if (sscanf (art_stdout,
                  "ART, version %d.%d",
                  &rtmajor, &rtminor) == 2 &&
          ((rtmajor == 1 && rtminor >= 20) || rtmajor >= 2))
        {
          have_art = TRUE;
        }

      g_free (art_stdout);
    }

  g_free (exec_path);

  if (have_art)
    {
      GList *list = NULL;

      list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));

      for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
        {
          const FileFormat *format = &file_formats[i];
          gchar            *load_proc;

          load_proc = g_strdup_printf (format->load_proc_format, "another-rawtherapee");

          list = g_list_append (list, load_proc);
        }

      return list;
    }

  return NULL;
}

static GimpProcedure *
anotherrawtherapee_create_procedure (GimpPlugIn  *plug_in,
                                     const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                anotherrawtherapee_load_thumb,
                                                NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Load thumbnail from a raw image "
                                        "via ART",
                                        "This plug-in loads a thumbnail "
                                        "from a raw image by calling "
                                        "art-cli.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alberto Griggio",
                                      "Alberto Griggio",
                                      "2017, 2025");
    }
  else
    {
      gint i;

      for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
        {
          const FileFormat *format = &file_formats[i];
          gchar            *load_proc;
          gchar            *load_blurb;
          gchar            *load_help;

          load_proc = g_strdup_printf (format->load_proc_format, "another-rawtherapee");

          if (strcmp (name, load_proc))
            {
              g_free (load_proc);
              continue;
            }

          load_blurb = g_strdup_printf (format->load_blurb_format, "another-rawtherapee");
          load_help  = g_strdup_printf (format->load_help_format,  "another-rawtherapee");

          procedure = gimp_load_procedure_new (plug_in, name,
                                               GIMP_PDB_PROC_TYPE_PLUGIN,
                                               anotherrawtherapee_load,
                                               (gpointer) format, NULL);

          gimp_procedure_set_documentation (procedure,
                                            load_blurb, load_help, name);
          gimp_procedure_set_attribution (procedure,
                                          "Alberto Griggio",
                                          "Alberto Griggio",
                                          "2017, 2025");

          gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                              format->extensions);
          gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                          format->magic);

          gimp_load_procedure_set_handles_raw (GIMP_LOAD_PROCEDURE (procedure),
                                               TRUE);
          gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                    LOAD_THUMB_PROC);

          g_free (load_proc);
          g_free (load_blurb);
          g_free (load_help);

          break;
        }
    }

  return procedure;
}

static GimpValueArray *
anotherrawtherapee_load (GimpProcedure         *procedure,
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

  image = load_image (file, run_mode, &error);

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

static GimpValueArray *
anotherrawtherapee_load_thumb (GimpProcedure       *procedure,
                               GFile               *file,
                               gint                 size,
                               GimpProcedureConfig *config,
                               gpointer             run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  image = load_thumbnail_image (file, size, &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);
  GIMP_VALUES_SET_INT   (return_vals, 2, 0);
  GIMP_VALUES_SET_INT   (return_vals, 3, 0);
  GIMP_VALUES_SET_ENUM  (return_vals, 4, GIMP_RGB_IMAGE);
  GIMP_VALUES_SET_INT   (return_vals, 5, 1);

  gimp_value_array_truncate (return_vals, 6);

  return return_vals;
}

static GimpImage *
load_image (GFile        *file,
            GimpRunMode   run_mode,
            GError      **error)
{
  GimpImage *image       = NULL;
  GFile     *file_out    = gimp_temp_file ("tif");
  gchar     *art_stdout  = NULL;

  gboolean   search_path = FALSE;
  gchar     *exec_path   = file_raw_get_executable_path ("ART", NULL,
                                                         "ART_EXECUTABLE",
                                                         NULL,
                                                         REGISTRY_KEY_BASE,
                                                         &search_path);

  /* linear sRGB for now */
  gchar *argv[] =
    {
      exec_path,
      "-gimp",
      (gchar *) g_file_peek_path (file),
      (gchar *) g_file_peek_path (file_out),
      NULL
    };

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    /*G_SPAWN_STDOUT_TO_DEV_NULL |*/
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    NULL,
                    NULL,
                    &art_stdout,
                    NULL,
                    NULL,
                    error))
    {
      image = gimp_file_load (run_mode, file_out);
    }

  g_free (art_stdout);
  g_free (exec_path);

  g_file_delete (file_out, NULL, NULL);

  gimp_progress_update (1.0);

  return image;
}

static GimpImage *
load_thumbnail_image (GFile   *file,
                      gint     thumb_size,
                      GError **error)
{
  GimpImage  *image            = NULL;
  GFile      *file_out         = gimp_temp_file ("jpg");
  GFile      *thumb_arp_file   = gimp_temp_file ("arp");
  FILE       *thumb_arp_f      = g_fopen (g_file_peek_path (thumb_arp_file), "w");
  gchar      *art_stdout = NULL;
  const char *arp_content =
    "[Version]\n"
    "AppVersion=1.0\n"
    "Version=20\n"
    "\n"
    "[Resize]\n"
    "Enabled=true\n"
    "AppliesTo=Cropped area\n"
    "Method=Lanczos\n"
    "Width=%d\n"
    "Height=%d\n"
    "\n"
    "[Sharpening]\n"
    "Enabled=false\n"
    "\n"
    "[SharpenEdge]\n"
    "Enabled=false\n"
    "\n"
    "[SharpenMicro]\n"
    "Enabled=false\n"
    "\n"
    "[Defringing]\n"
    "Enabled=false\n"
    "\n"
    "[Directional Pyramid Equalizer]\n"
    "Enabled=false\n"
    "\n"
    "[PostResizeSharpening]\n"
    "Enabled=false\n"
    "\n"
    "[Directional Pyramid Denoising]\n"
    "Enabled=false\n"
    "\n"
    "[Impulse Denoising]\n"
    "Enabled=false\n"
    "\n"
    "[Wavelet]\n"
    "Enabled=false\n"
    "\n"
    "[RAW Bayer]\n"
    "Method=fast\n"
    "\n"
    "[RAW X-Trans]\n"
    "Method=fast\n";


  gboolean  search_path = FALSE;
  gchar    *exec_path   = file_raw_get_executable_path ("ART", "-cli",
                                                        "ART_EXECUTABLE",
                                                        NULL,
                                                        REGISTRY_KEY_BASE,
                                                        &search_path);
  gchar *argv[] =
    {
      exec_path,
#ifdef G_OS_WIN32
      "-w",
#endif
      "-o", (gchar *) g_file_peek_path (file_out),
      "-d",
      "-s",
      "-j",
      "-p", (gchar *) g_file_peek_path (thumb_arp_file),
      "-f",
      "-c", (gchar *) g_file_peek_path (file),
      NULL
    };

  if (thumb_arp_f)
    {
      if (fprintf (thumb_arp_f, arp_content, thumb_size, thumb_size) < 0)
        {
          fclose (thumb_arp_f);
          thumb_arp_f = NULL;
        }
    }

  gimp_progress_init_printf (_("Opening thumbnail for '%s'"),
                             gimp_file_get_utf8_name (file));

  if (thumb_arp_f &&
      g_spawn_sync (NULL,
                    argv,
                    NULL,
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    NULL,
                    NULL,
                    &art_stdout,
                    NULL,
                    NULL,
                    error))
    {
      gimp_progress_update (0.5);

      image = gimp_file_load (GIMP_RUN_NONINTERACTIVE, file_out);
    }

  gimp_progress_update (1.0);

  if (thumb_arp_f)
    fclose (thumb_arp_f);

  g_file_delete (thumb_arp_file, NULL, NULL);
  g_free (art_stdout);
  g_free (exec_path);

  return image;
}
