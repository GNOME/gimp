/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-rawtherapee.c -- raw file format plug-in that uses RawTherapee
 * Copyright (C) 2012 Simon Budig <simon@ligma.org>
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

#include <libligma/ligma.h>

#include "libligma/stdplugins-intl.h"

#include "file-raw-formats.h"
#include "file-raw-utils.h"


#define LOAD_THUMB_PROC   "file-rawtherapee-load-thumb"
#define REGISTRY_KEY_BASE "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\rawtherapee"


typedef struct _Rawtherapee      Rawtherapee;
typedef struct _RawtherapeeClass RawtherapeeClass;

struct _Rawtherapee
{
  LigmaPlugIn      parent_instance;
};

struct _RawtherapeeClass
{
  LigmaPlugInClass parent_class;
};


#define RAWTHERAPEE_TYPE  (rawtherapee_get_type ())
#define RAWTHERAPEE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RAWTHERAPEE_TYPE, Rawtherapee))

GType                   rawtherapee_get_type         (void) G_GNUC_CONST;

static GList          * rawtherapee_init_procedures  (LigmaPlugIn           *plug_in);
static LigmaProcedure  * rawtherapee_create_procedure (LigmaPlugIn           *plug_in,
                                                      const gchar          *name);

static LigmaValueArray * rawtherapee_load             (LigmaProcedure        *procedure,
                                                      LigmaRunMode           run_mode,
                                                      GFile                *file,
                                                      const LigmaValueArray *args,
                                                      gpointer              run_data);
static LigmaValueArray * rawtherapee_load_thumb       (LigmaProcedure        *procedure,
                                                      GFile                *file,
                                                      gint                  size,
                                                      const LigmaValueArray *args,
                                                      gpointer              run_data);

static LigmaImage      * load_image                   (GFile                *file,
                                                      LigmaRunMode           run_mode,
                                                      GError              **error);
static LigmaImage      * load_thumbnail_image         (GFile                *file,
                                                      gint                  thumb_size,
                                                      GError              **error);


G_DEFINE_TYPE (Rawtherapee, rawtherapee, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (RAWTHERAPEE_TYPE)
DEFINE_STD_SET_I18N


static void
rawtherapee_class_init (RawtherapeeClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->init_procedures  = rawtherapee_init_procedures;
  plug_in_class->create_procedure = rawtherapee_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
rawtherapee_init (Rawtherapee *rawtherapee)
{
}

static GList *
rawtherapee_init_procedures (LigmaPlugIn *plug_in)
{
  /* check if rawtherapee is installed
   * TODO: allow setting the location of the executable in preferences
   */
  gboolean  search_path      = FALSE;
  gchar    *exec_path        = file_raw_get_executable_path ("rawtherapee", NULL,
                                                             "RAWTHERAPEE_EXECUTABLE",
                                                             "com.rawtherapee.rawtherapee",
                                                             REGISTRY_KEY_BASE,
                                                             &search_path);
#ifdef G_OS_WIN32
  /* Issue #2716 - Prevent RT from opening a console window */
  gchar    *argv[]             = { exec_path, "-v", "-w", NULL };
#else
  gchar    *argv[]             = { exec_path, "-v", NULL };
#endif
  gchar    *rawtherapee_stdout = NULL;
  gboolean  have_rawtherapee   = FALSE;
  gint      i;

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    (search_path ? G_SPAWN_SEARCH_PATH : 0) |
                    G_SPAWN_STDERR_TO_DEV_NULL,
                    NULL,
                    NULL,
                    &rawtherapee_stdout,
                    NULL,
                    NULL,
                    NULL))
    {
      gint rtmajor = 0;
      gint rtminor = 0;

      if (sscanf (rawtherapee_stdout,
                  "RawTherapee, version %d.%d",
                  &rtmajor, &rtminor) == 2 &&
          ((rtmajor == 5 && rtminor >= 2) || rtmajor >= 6))
        {
          have_rawtherapee = TRUE;
        }

      g_free (rawtherapee_stdout);
    }

  g_free (exec_path);

  if (have_rawtherapee)
    {
      GList *list = NULL;

      list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));

      for (i = 0; i < G_N_ELEMENTS (file_formats); i++)
        {
          const FileFormat *format = &file_formats[i];
          gchar            *load_proc;

          load_proc = g_strdup_printf (format->load_proc_format, "rawtherapee");

          list = g_list_append (list, load_proc);
        }

      return list;
    }

  return NULL;
}

static LigmaProcedure *
rawtherapee_create_procedure (LigmaPlugIn  *plug_in,
                            const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = ligma_thumbnail_procedure_new (plug_in, name,
                                                LIGMA_PDB_PROC_TYPE_PLUGIN,
                                                rawtherapee_load_thumb, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Load thumbnail from a raw image "
                                        "via rawtherapee",
                                        "This plug-in loads a thumbnail "
                                        "from a raw image by calling "
                                        "rawtherapee-cli.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Alberto Griggio",
                                      "Alberto Griggio",
                                      "2017");
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

          load_proc = g_strdup_printf (format->load_proc_format, "rawtherapee");

          if (strcmp (name, load_proc))
            {
              g_free (load_proc);
              continue;
            }

          load_blurb = g_strdup_printf (format->load_blurb_format, "rawtherapee");
          load_help  = g_strdup_printf (format->load_help_format,  "rawtherapee");

          procedure = ligma_load_procedure_new (plug_in, name,
                                               LIGMA_PDB_PROC_TYPE_PLUGIN,
                                               rawtherapee_load,
                                               (gpointer) format, NULL);

          ligma_procedure_set_documentation (procedure,
                                            load_blurb, load_help, name);
          ligma_procedure_set_attribution (procedure,
                                          "Alberto Griggio",
                                          "Alberto Griggio",
                                          "2017");

          ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                              format->mime_type);
          ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                              format->extensions);
          ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                          format->magic);

          ligma_load_procedure_set_handles_raw (LIGMA_LOAD_PROCEDURE (procedure),
                                               TRUE);
          ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                    LOAD_THUMB_PROC);

          g_free (load_proc);
          g_free (load_blurb);
          g_free (load_help);

          break;
        }
    }

  return procedure;
}

static LigmaValueArray *
rawtherapee_load (LigmaProcedure        *procedure,
                  LigmaRunMode           run_mode,
                  GFile                *file,
                  const LigmaValueArray *args,
                  gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  image = load_image (file, run_mode, &error);

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
rawtherapee_load_thumb (LigmaProcedure        *procedure,
                        GFile                *file,
                        gint                  size,
                        const LigmaValueArray *args,
                        gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  image = load_thumbnail_image (file, size, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);
  LIGMA_VALUES_SET_INT   (return_vals, 2, 0);
  LIGMA_VALUES_SET_INT   (return_vals, 3, 0);
  LIGMA_VALUES_SET_ENUM  (return_vals, 4, LIGMA_RGB_IMAGE);
  LIGMA_VALUES_SET_INT   (return_vals, 5, 1);

  ligma_value_array_truncate (return_vals, 6);

  return return_vals;
}

static LigmaImage *
load_image (GFile        *file,
            LigmaRunMode   run_mode,
            GError      **error)
{
  LigmaImage *image              = NULL;
  GFile     *file_out           = ligma_temp_file ("tif");
  gchar     *rawtherapee_stdout = NULL;

  gboolean   search_path        = FALSE;
  gchar     *exec_path          = file_raw_get_executable_path ("rawtherapee", NULL,
                                                               "RAWTHERAPEE_EXECUTABLE",
                                                               "com.rawtherapee.rawtherapee",
                                                               REGISTRY_KEY_BASE,
                                                               &search_path);

  /* linear sRGB for now as LIGMA uses that internally in many places anyway */
  gchar *argv[] =
    {
      exec_path,
      "-ligma",
      (gchar *) g_file_peek_path (file),
      (gchar *) g_file_peek_path (file_out),
      NULL
    };

  ligma_progress_init_printf (_("Opening '%s'"),
                             ligma_file_get_utf8_name (file));

  if (g_spawn_sync (NULL,
                    argv,
                    NULL,
                    /*G_SPAWN_STDOUT_TO_DEV_NULL |*/
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    NULL,
                    NULL,
                    &rawtherapee_stdout,
                    NULL,
                    NULL,
                    error))
    {
      image = ligma_file_load (run_mode, file_out);
      if (image)
        ligma_image_set_file (image, file);
    }

  /*if (rawtherapee_stdout) printf ("%s\n", rawtherapee_stdout);*/
  g_free (rawtherapee_stdout);
  g_free (exec_path);

  g_file_delete (file_out, NULL, NULL);

  ligma_progress_update (1.0);

  return image;
}

static LigmaImage *
load_thumbnail_image (GFile   *file,
                      gint     thumb_size,
                      GError **error)
{
  LigmaImage *image            = NULL;
  GFile     *file_out         = ligma_temp_file ("jpg");
  GFile     *thumb_pp3_file   = ligma_temp_file ("pp3");
  FILE      *thumb_pp3_f      = fopen (g_file_peek_path (thumb_pp3_file), "w");
  gchar     *rawtherapee_stdout = NULL;
  const char *pp3_content =
    "[Version]\n"
    "AppVersion=5.0\n"
    "Version=326\n"
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
  gchar    *exec_path   = file_raw_get_executable_path ("rawtherapee", "-cli",
                                                        "RAWTHERAPEE_EXECUTABLE",
                                                        "com.rawtherapee.rawtherapee",
                                                        REGISTRY_KEY_BASE,
                                                        &search_path);
  gchar *argv[] =
    {
      exec_path,
      "-o", (gchar *) g_file_peek_path (file_out),
      "-d",
      "-s",
      "-j",
      "-p", (gchar *) g_file_peek_path (thumb_pp3_file),
      "-f",
      "-c", (gchar *) g_file_peek_path (file),
      NULL
    };

  if (thumb_pp3_f)
    {
      if (fprintf (thumb_pp3_f, pp3_content, thumb_size, thumb_size) < 0)
        {
          fclose (thumb_pp3_f);
          thumb_pp3_f = NULL;
        }
    }

  ligma_progress_init_printf (_("Opening thumbnail for '%s'"),
                             ligma_file_get_utf8_name (file));

  if (thumb_pp3_f &&
      g_spawn_sync (NULL,
                    argv,
                    NULL,
                    G_SPAWN_STDERR_TO_DEV_NULL |
                    (search_path ? G_SPAWN_SEARCH_PATH : 0),
                    NULL,
                    NULL,
                    &rawtherapee_stdout,
                    NULL,
                    NULL,
                    error))
    {
      ligma_progress_update (0.5);

      image = ligma_file_load (LIGMA_RUN_NONINTERACTIVE, file_out);
      if (image)
        {
          /* is this needed for thumbnails? */
          ligma_image_set_file (image, file);
        }
    }

  ligma_progress_update (1.0);

  if (thumb_pp3_f)
    fclose (thumb_pp3_f);

  g_file_delete (thumb_pp3_file, NULL, NULL);
  g_free (rawtherapee_stdout);
  g_free (exec_path);

  return image;
}
