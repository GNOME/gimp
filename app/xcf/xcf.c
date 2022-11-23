/* LIGMA - The GNU Image Manipulation Program
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
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core/core-types.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmadrawable.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmaprogress.h"

#include "plug-in/ligmapluginmanager.h"
#include "plug-in/ligmapluginprocedure.h"

#include "xcf.h"
#include "xcf-private.h"
#include "xcf-load.h"
#include "xcf-read.h"
#include "xcf-save.h"

#include "ligma-intl.h"


typedef LigmaImage * LigmaXcfLoaderFunc (Ligma     *ligma,
                                       XcfInfo  *info,
                                       GError  **error);


static LigmaValueArray * xcf_load_invoker (LigmaProcedure         *procedure,
                                          Ligma                  *ligma,
                                          LigmaContext           *context,
                                          LigmaProgress          *progress,
                                          const LigmaValueArray  *args,
                                          GError               **error);
static LigmaValueArray * xcf_save_invoker (LigmaProcedure         *procedure,
                                          Ligma                  *ligma,
                                          LigmaContext           *context,
                                          LigmaProgress          *progress,
                                          const LigmaValueArray  *args,
                                          GError               **error);


static LigmaXcfLoaderFunc * const xcf_loaders[] =
{
  xcf_load_image,   /* version  0 */
  xcf_load_image,   /* version  1 */
  xcf_load_image,   /* version  2 */
  xcf_load_image,   /* version  3 */
  xcf_load_image,   /* version  4 */
  xcf_load_image,   /* version  5 */
  xcf_load_image,   /* version  6 */
  xcf_load_image,   /* version  7 */
  xcf_load_image,   /* version  8 */
  xcf_load_image,   /* version  9 */
  xcf_load_image,   /* version 10 */
  xcf_load_image,   /* version 11 */
  xcf_load_image,   /* version 12 */
  xcf_load_image,   /* version 13 */
  xcf_load_image,   /* version 14 */
  xcf_load_image,   /* version 15 */
  xcf_load_image,   /* version 16 */
  xcf_load_image,   /* version 17 */
  xcf_load_image,   /* version 18 */
};


void
xcf_init (Ligma *ligma)
{
  LigmaPlugInProcedure *proc;
  GFile               *file;
  LigmaProcedure       *procedure;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  /* So this is sort of a hack, but its better than it was before.  To
   * do this right there would be a file load-save handler type and
   * the whole interface would change but there isn't, and currently
   * the plug-in structure contains all the load-save info, so it
   * makes sense to use that for the XCF load/save handlers, even
   * though they are internal.  The only thing it requires is using a
   * PlugInProcDef struct.  -josh
   */

  /*  ligma-xcf-save  */
  file = g_file_new_for_path ("ligma-xcf-save");
  procedure = ligma_plug_in_procedure_new (LIGMA_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = LIGMA_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = xcf_save_invoker;

  proc = LIGMA_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("LIGMA XCF image"));
  ligma_plug_in_procedure_set_icon (proc, LIGMA_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "ligma-wilber",
                                   strlen ("ligma-wilber") + 1,
                                   NULL);
  ligma_plug_in_procedure_set_image_types (proc, "RGB*, GRAY*, INDEXED*");
  ligma_plug_in_procedure_set_file_proc (proc, "xcf", "", NULL);
  ligma_plug_in_procedure_set_mime_types (proc, "image/x-xcf");
  ligma_plug_in_procedure_set_handles_remote (proc);

  ligma_object_set_static_name (LIGMA_OBJECT (procedure), "ligma-xcf-save");
  ligma_procedure_set_static_help (procedure,
                                  "Saves file in the .xcf file format",
                                  "The XCF file format has been designed "
                                  "specifically for loading and saving "
                                  "tiled and layered images in LIGMA. "
                                  "This procedure will save the specified "
                                  "image in the xcf file format.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1995-1996");

  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_enum ("dummy-param",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     LIGMA_TYPE_RUN_MODE,
                                                     LIGMA_RUN_INTERACTIVE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_int ("n-drawables",
                                                 "Num drawables",
                                                 "Number of drawables",
                                                 0, G_MAXINT, 0,
                                                 LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_object_array ("drawables",
                                                             "Drawables",
                                                             "Selected drawables",
                                                             LIGMA_TYPE_DRAWABLE,
                                                             LIGMA_PARAM_READWRITE | LIGMA_PARAM_NO_VALIDATE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file "
                                                    "to save the image in",
                                                    G_TYPE_FILE,
                                                    LIGMA_PARAM_READWRITE));
  ligma_plug_in_manager_add_procedure (ligma->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  ligma-xcf-load  */
  file = g_file_new_for_path ("ligma-xcf-load");
  procedure = ligma_plug_in_procedure_new (LIGMA_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = LIGMA_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = xcf_load_invoker;

  proc = LIGMA_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("LIGMA XCF image"));
  ligma_plug_in_procedure_set_icon (proc, LIGMA_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "ligma-wilber",
                                   strlen ("ligma-wilber") + 1,
                                   NULL);
  ligma_plug_in_procedure_set_image_types (proc, NULL);
  ligma_plug_in_procedure_set_file_proc (proc, "xcf", "",
                                        "0,string,ligma\\040xcf\\040");
  ligma_plug_in_procedure_set_mime_types (proc, "image/x-xcf");
  ligma_plug_in_procedure_set_handles_remote (proc);

  ligma_object_set_static_name (LIGMA_OBJECT (procedure), "ligma-xcf-load");
  ligma_procedure_set_static_help (procedure,
                                  "Loads file saved in the .xcf file format",
                                  "The XCF file format has been designed "
                                  "specifically for loading and saving "
                                  "tiled and layered images in LIGMA. "
                                  "This procedure will load the specified "
                                  "file.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1995-1996");

  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_enum ("dummy-param",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     LIGMA_TYPE_RUN_MODE,
                                                     LIGMA_RUN_INTERACTIVE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                  LIGMA_PARAM_READWRITE));

  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_image ("image",
                                                          "Image",
                                                          "Output image",
                                                          FALSE,
                                                          LIGMA_PARAM_READWRITE));
  ligma_plug_in_manager_add_procedure (ligma->plug_in_manager, proc);
  g_object_unref (procedure);
}

void
xcf_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
}

LigmaImage *
xcf_load_stream (Ligma          *ligma,
                 GInputStream  *input,
                 GFile         *input_file,
                 LigmaProgress  *progress,
                 GError       **error)
{
  XcfInfo      info  = { 0, };
  const gchar *filename;
  LigmaImage   *image = NULL;
  gchar        id[14];
  gboolean     success;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (input_file == NULL || G_IS_FILE (input_file), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (input_file)
    filename = ligma_file_get_utf8_name (input_file);
  else
    filename = _("Memory Stream");

  info.ligma             = ligma;
  info.input            = input;
  info.seekable         = G_SEEKABLE (input);
  info.bytes_per_offset = 4;
  info.progress         = progress;
  info.file             = input_file;
  info.compression      = COMPRESS_NONE;

  if (progress)
    ligma_progress_start (progress, FALSE, _("Opening '%s'"), filename);

  success = TRUE;

  xcf_read_int8 (&info, (guint8 *) id, 14);

  if (! g_str_has_prefix (id, "ligma xcf "))
    {
      success = FALSE;
    }
  else if (strcmp (id + 9, "file") == 0)
    {
      info.file_version = 0;
    }
  else if (id[9]  == 'v' &&
           id[13] == '\0')
    {
      info.file_version = atoi (id + 10);
    }
  else
    {
      success = FALSE;
    }

  if (info.file_version >= 11)
    info.bytes_per_offset = 8;

  if (success)
    {
      if (info.file_version >= 0 &&
          info.file_version < G_N_ELEMENTS (xcf_loaders))
        {
          image = (*(xcf_loaders[info.file_version])) (ligma, &info, error);

          if (! image)
            success = FALSE;

          g_input_stream_close (info.input, NULL, NULL);
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("XCF error: unsupported XCF file version %d "
                         "encountered"), info.file_version);
          success = FALSE;
        }
    }

  if (progress)
    ligma_progress_end (progress);

  return image;
}

gboolean
xcf_save_stream (Ligma           *ligma,
                 LigmaImage      *image,
                 GOutputStream  *output,
                 GFile          *output_file,
                 LigmaProgress   *progress,
                 GError        **error)
{
  XcfInfo       info     = { 0, };
  const gchar  *filename;
  gboolean      success  = FALSE;
  GError       *my_error = NULL;
  GCancellable *cancellable;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (output_file == NULL || G_IS_FILE (output_file), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (output_file)
    filename = ligma_file_get_utf8_name (output_file);
  else
    filename = _("Memory Stream");

  info.ligma             = ligma;
  info.output           = output;
  info.seekable         = G_SEEKABLE (output);
  info.bytes_per_offset = 4;
  info.progress         = progress;
  info.file             = output_file;

  if (ligma_image_get_xcf_compression (image))
    info.compression = COMPRESS_ZLIB;
  else
    info.compression = COMPRESS_RLE;

  info.file_version = ligma_image_get_xcf_version (image,
                                                  info.compression ==
                                                  COMPRESS_ZLIB,
                                                  NULL, NULL, NULL);

  if (info.file_version >= 11)
    info.bytes_per_offset = 8;

  if (progress)
    ligma_progress_start (progress, FALSE, _("Saving '%s'"), filename);

  success = xcf_save_image (&info, image, &my_error);

  cancellable = g_cancellable_new ();
  if (success)
    {
      if (progress)
        ligma_progress_set_text (progress, _("Closing '%s'"), filename);
    }
  else
    {
      /* When closing the stream, the image will be actually saved,
       * unless we properly cancel it with a GCancellable.
       * Not closing the stream is not an option either, as this will
       * happen anyway when finalizing the output.
       * So let's make sure now that we don't overwrite the XCF file
       * when an error occurred.
       */
      g_cancellable_cancel (cancellable);
    }
  success = g_output_stream_close (info.output, cancellable, &my_error);
  g_object_unref (cancellable);

  if (! success && my_error)
    g_propagate_prefixed_error (error, my_error,
                                _("Error writing '%s': "), filename);

  if (progress)
    ligma_progress_end (progress);

  return success;
}


/*  private functions  */

static LigmaValueArray *
xcf_load_invoker (LigmaProcedure         *procedure,
                  Ligma                  *ligma,
                  LigmaContext           *context,
                  LigmaProgress          *progress,
                  const LigmaValueArray  *args,
                  GError               **error)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image = NULL;
  GFile          *file;
  GInputStream   *input;
  GError         *my_error = NULL;

  ligma_set_busy (ligma);

  file = g_value_get_object (ligma_value_array_index (args, 1));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (input)
    {
      image = xcf_load_stream (ligma, input, file, progress, error);

      g_object_unref (input);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Could not open '%s' for reading: "),
                                  ligma_file_get_utf8_name (file));
    }

  return_vals = ligma_procedure_get_return_values (procedure, image != NULL,
                                                  error ? *error : NULL);

  if (image)
    g_value_set_object (ligma_value_array_index (return_vals, 1), image);

  ligma_unset_busy (ligma);

  return return_vals;
}

static LigmaValueArray *
xcf_save_invoker (LigmaProcedure         *procedure,
                  Ligma                  *ligma,
                  LigmaContext           *context,
                  LigmaProgress          *progress,
                  const LigmaValueArray  *args,
                  GError               **error)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GFile          *file;
  GOutputStream  *output;
  gboolean        success  = FALSE;
  GError         *my_error = NULL;

  ligma_set_busy (ligma);

  image = g_value_get_object (ligma_value_array_index (args, 1));
  file  = g_value_get_object (ligma_value_array_index (args, 4));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &my_error));

  if (output)
    {
      success = xcf_save_stream (ligma, image, output, file, progress, error);

      g_object_unref (output);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Error creating '%s': "),
                                  ligma_file_get_utf8_name (file));
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  ligma_unset_busy (ligma);

  return return_vals;
}
