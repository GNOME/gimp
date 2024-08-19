/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginprocedure.h"

#include "xcf.h"
#include "xcf-private.h"
#include "xcf-load.h"
#include "xcf-read.h"
#include "xcf-save.h"

#include "gimp-intl.h"


typedef GimpImage * GimpXcfLoaderFunc (Gimp     *gimp,
                                       XcfInfo  *info,
                                       GError  **error);


static GimpValueArray * xcf_load_invoker (GimpProcedure         *procedure,
                                          Gimp                  *gimp,
                                          GimpContext           *context,
                                          GimpProgress          *progress,
                                          const GimpValueArray  *args,
                                          GError               **error);
static GimpValueArray * xcf_save_invoker (GimpProcedure         *procedure,
                                          Gimp                  *gimp,
                                          GimpContext           *context,
                                          GimpProgress          *progress,
                                          const GimpValueArray  *args,
                                          GError               **error);


static GimpXcfLoaderFunc * const xcf_loaders[] =
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
  xcf_load_image,   /* version 19 */
  xcf_load_image,   /* version 20 */
  xcf_load_image,   /* version 21 */
  xcf_load_image,   /* version 22 */
};


void
xcf_init (Gimp *gimp)
{
  GimpPlugInProcedure *proc;
  GFile               *file;
  GimpProcedure       *procedure;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /* So this is sort of a hack, but its better than it was before.  To
   * do this right there would be a file load-save handler type and
   * the whole interface would change but there isn't, and currently
   * the plug-in structure contains all the load-save info, so it
   * makes sense to use that for the XCF load/save handlers, even
   * though they are internal.  The only thing it requires is using a
   * PlugInProcDef struct.  -josh
   */

  /*  gimp-xcf-save  */
  file = g_file_new_for_path ("gimp-xcf-save");
  procedure = gimp_plug_in_procedure_new (GIMP_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = GIMP_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = xcf_save_invoker;

  proc = GIMP_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("GIMP XCF image"));
  gimp_plug_in_procedure_set_icon (proc, GIMP_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "gimp-wilber",
                                   strlen ("gimp-wilber") + 1,
                                   NULL);
  gimp_plug_in_procedure_set_image_types (proc, "RGB*, GRAY*, INDEXED*");
  gimp_plug_in_procedure_set_file_proc (proc, "xcf", "", NULL);
  gimp_plug_in_procedure_set_mime_types (proc, "image/x-xcf");
  gimp_plug_in_procedure_set_handles_remote (proc);

  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-xcf-save");
  gimp_procedure_set_static_help (procedure,
                                  "Saves file in the .xcf file format",
                                  "The XCF file format has been designed "
                                  "specifically for loading and saving "
                                  "tiled and layered images in GIMP. "
                                  "This procedure will save the specified "
                                  "image in the xcf file format.",
                                  NULL);
  gimp_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1995-1996");

  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     GIMP_TYPE_RUN_MODE,
                                                     GIMP_RUN_INTERACTIVE,
                                                     GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file "
                                                    "to save the image in",
                                                    G_TYPE_FILE,
                                                    GIMP_PARAM_READWRITE));
  gimp_plug_in_manager_add_procedure (gimp->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  gimp-xcf-load  */
  file = g_file_new_for_path ("gimp-xcf-load");
  procedure = gimp_plug_in_procedure_new (GIMP_PDB_PROC_TYPE_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = GIMP_PDB_PROC_TYPE_INTERNAL;
  procedure->marshal_func = xcf_load_invoker;

  proc = GIMP_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("GIMP XCF image"));
  gimp_plug_in_procedure_set_icon (proc, GIMP_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "gimp-wilber",
                                   strlen ("gimp-wilber") + 1,
                                   NULL);
  gimp_plug_in_procedure_set_image_types (proc, NULL);
  gimp_plug_in_procedure_set_file_proc (proc, "xcf", "",
                                        "0,string,gimp\\040xcf\\040");
  gimp_plug_in_procedure_set_mime_types (proc, "image/x-xcf");
  gimp_plug_in_procedure_set_handles_remote (proc);

  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-xcf-load");
  gimp_procedure_set_static_help (procedure,
                                  "Loads file saved in the .xcf file format",
                                  "The XCF file format has been designed "
                                  "specifically for loading and saving "
                                  "tiled and layered images in GIMP. "
                                  "This procedure will load the specified "
                                  "file.",
                                  NULL);
  gimp_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1995-1996");

  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     GIMP_TYPE_RUN_MODE,
                                                     GIMP_RUN_INTERACTIVE,
                                                     GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                  GIMP_PARAM_READWRITE));

  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_image ("image",
                                                          "Image",
                                                          "Output image",
                                                          FALSE,
                                                          GIMP_PARAM_READWRITE));
  gimp_plug_in_manager_add_procedure (gimp->plug_in_manager, proc);
  g_object_unref (procedure);
}

void
xcf_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
}

GimpImage *
xcf_load_stream (Gimp          *gimp,
                 GInputStream  *input,
                 GFile         *input_file,
                 GimpProgress  *progress,
                 GError       **error)
{
  XcfInfo      info  = { 0, };
  const gchar *filename;
  GimpImage   *image = NULL;
  gchar        id[14];
  gboolean     success;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (input_file == NULL || G_IS_FILE (input_file), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (input_file)
    filename = gimp_file_get_utf8_name (input_file);
  else
    filename = _("Memory Stream");

  info.gimp             = gimp;
  info.input            = input;
  info.seekable         = G_SEEKABLE (input);
  info.bytes_per_offset = 4;
  info.progress         = progress;
  info.file             = input_file;
  info.compression      = COMPRESS_NONE;

  if (progress)
    gimp_progress_start (progress, FALSE, _("Opening '%s'"), filename);

  success = TRUE;

  xcf_read_int8 (&info, (guint8 *) id, 14);

  if (! g_str_has_prefix (id, "gimp xcf "))
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
          image = (*(xcf_loaders[info.file_version])) (gimp, &info, error);

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
    gimp_progress_end (progress);

  return image;
}

gboolean
xcf_save_stream (Gimp           *gimp,
                 GimpImage      *image,
                 GOutputStream  *output,
                 GFile          *output_file,
                 GimpProgress   *progress,
                 GError        **error)
{
  XcfInfo       info     = { 0, };
  const gchar  *filename;
  gboolean      success  = FALSE;
  GError       *my_error = NULL;
  GCancellable *cancellable;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (output_file == NULL || G_IS_FILE (output_file), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (output_file)
    filename = gimp_file_get_utf8_name (output_file);
  else
    filename = _("Memory Stream");

  info.gimp             = gimp;
  info.output           = output;
  info.seekable         = G_SEEKABLE (output);
  info.bytes_per_offset = 4;
  info.progress         = progress;
  info.file             = output_file;

  if (gimp_image_get_xcf_compression (image))
    info.compression = COMPRESS_ZLIB;
  else
    info.compression = COMPRESS_RLE;

  info.file_version = gimp_image_get_xcf_version (image,
                                                  info.compression ==
                                                  COMPRESS_ZLIB,
                                                  NULL, NULL, NULL);

  if (info.file_version >= 11)
    info.bytes_per_offset = 8;

  if (progress)
    gimp_progress_start (progress, FALSE, _("Saving '%s'"), filename);

  success = xcf_save_image (&info, image, &my_error);

  cancellable = g_cancellable_new ();
  if (success)
    {
      if (progress)
        gimp_progress_set_text (progress, _("Closing '%s'"), filename);
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
    gimp_progress_end (progress);

  return success;
}


/*  private functions  */

static GimpValueArray *
xcf_load_invoker (GimpProcedure         *procedure,
                  Gimp                  *gimp,
                  GimpContext           *context,
                  GimpProgress          *progress,
                  const GimpValueArray  *args,
                  GError               **error)
{
  GimpValueArray *return_vals;
  GimpImage      *image = NULL;
  GFile          *file;
  GInputStream   *input;
  GError         *my_error = NULL;

  gimp_set_busy (gimp);

  file = g_value_get_object (gimp_value_array_index (args, 1));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (input)
    {
      image = xcf_load_stream (gimp, input, file, progress, error);

      g_object_unref (input);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Could not open '%s' for reading: "),
                                  gimp_file_get_utf8_name (file));
    }

  return_vals = gimp_procedure_get_return_values (procedure, image != NULL,
                                                  error ? *error : NULL);

  if (image)
    g_value_set_object (gimp_value_array_index (return_vals, 1), image);

  gimp_unset_busy (gimp);

  return return_vals;
}

static GimpValueArray *
xcf_save_invoker (GimpProcedure         *procedure,
                  Gimp                  *gimp,
                  GimpContext           *context,
                  GimpProgress          *progress,
                  const GimpValueArray  *args,
                  GError               **error)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GFile          *file;
  GOutputStream  *output;
  gboolean        success  = FALSE;
  GError         *my_error = NULL;

  gimp_set_busy (gimp);

  image = g_value_get_object (gimp_value_array_index (args, 1));
  file  = g_value_get_object (gimp_value_array_index (args, 2));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &my_error));

  if (output)
    {
      success = xcf_save_stream (gimp, image, output, file, progress, error);

      g_object_unref (output);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Error creating '%s': "),
                                  gimp_file_get_utf8_name (file));
    }

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  gimp_unset_busy (gimp);

  return return_vals;
}
