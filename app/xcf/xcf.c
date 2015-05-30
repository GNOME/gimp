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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
  xcf_load_image,   /* version 0 */
  xcf_load_image,   /* version 1 */
  xcf_load_image,   /* version 2 */
  xcf_load_image,   /* version 3 */
  xcf_load_image,   /* version 4 */
  xcf_load_image,   /* version 5 */
  xcf_load_image,   /* version 6 */
  xcf_load_image,   /* version 7 */
  xcf_load_image,   /* version 8 */
  xcf_load_image    /* version 9 */
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
  procedure = gimp_plug_in_procedure_new (GIMP_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = GIMP_INTERNAL;
  procedure->marshal_func = xcf_save_invoker;

  proc = GIMP_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("GIMP XCF image"));
  gimp_plug_in_procedure_set_icon (proc, GIMP_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "gimp-wilber",
                                   strlen ("gimp-wilber") + 1);
  gimp_plug_in_procedure_set_image_types (proc, "RGB*, GRAY*, INDEXED*");
  gimp_plug_in_procedure_set_file_proc (proc, "xcf", "", NULL);
  gimp_plug_in_procedure_set_mime_type (proc, "image/xcf");
  gimp_plug_in_procedure_set_handles_uri (proc);

  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-xcf-save");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-xcf-save",
                                     "Saves file in the .xcf file format",
                                     "The XCF file format has been designed "
                                     "specifically for loading and saving "
                                     "tiled and layered images in GIMP. "
                                     "This procedure will save the specified "
                                     "image in the xcf file format.",
                                     "Spencer Kimball & Peter Mattis",
                                     "Spencer Kimball & Peter Mattis",
                                     "1995-1996",
                                     NULL);

  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_int32 ("dummy-param",
                                                      "Dummy Param",
                                                      "Dummy parameter",
                                                      G_MININT32, G_MAXINT32, 0,
                                                      GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_image_id ("image",
                                                         "Image",
                                                         "Input image",
                                                         gimp, FALSE,
                                                         GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_drawable_id ("drawable",
                                                            "Drawable",
                                                            "Active drawable of input image",
                                                            gimp, TRUE,
                                                            GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("filename",
                                                       "Filename",
                                                       "The name of the file "
                                                       "to save the image in, "
                                                       "in the on-disk "
                                                       "character set and "
                                                       "encoding",
                                                       TRUE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("raw-filename",
                                                       "Raw filename",
                                                       "The basename of the "
                                                       "file, in UTF-8",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_plug_in_manager_add_procedure (gimp->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  gimp-xcf-load  */
  file = g_file_new_for_path ("gimp-xcf-load");
  procedure = gimp_plug_in_procedure_new (GIMP_PLUGIN, file);
  g_object_unref (file);

  procedure->proc_type    = GIMP_INTERNAL;
  procedure->marshal_func = xcf_load_invoker;

  proc = GIMP_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (N_("GIMP XCF image"));
  gimp_plug_in_procedure_set_icon (proc, GIMP_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "gimp-wilber",
                                   strlen ("gimp-wilber") + 1);
  gimp_plug_in_procedure_set_image_types (proc, NULL);
  gimp_plug_in_procedure_set_file_proc (proc, "xcf", "",
                                        "0,string,gimp\\040xcf\\040");
  gimp_plug_in_procedure_set_mime_type (proc, "image/xcf");
  gimp_plug_in_procedure_set_handles_uri (proc);

  gimp_object_set_static_name (GIMP_OBJECT (procedure), "gimp-xcf-load");
  gimp_procedure_set_static_strings (procedure,
                                     "gimp-xcf-load",
                                     "Loads file saved in the .xcf file format",
                                     "The XCF file format has been designed "
                                     "specifically for loading and saving "
                                     "tiled and layered images in GIMP. "
                                     "This procedure will load the specified "
                                     "file.",
                                     "Spencer Kimball & Peter Mattis",
                                     "Spencer Kimball & Peter Mattis",
                                     "1995-1996",
                                     NULL);

  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_int32 ("dummy-param",
                                                      "Dummy Param",
                                                      "Dummy parameter",
                                                      G_MININT32, G_MAXINT32, 0,
                                                      GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("filename",
                                                       "Filename",
                                                       "The name of the file "
                                                       "to load, in the "
                                                       "on-disk character "
                                                       "set and encoding",
                                                       TRUE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("raw-filename",
                                                       "Raw filename",
                                                       "The basename of the "
                                                       "file, in UTF-8",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));

  gimp_procedure_add_return_value (procedure,
                                   gimp_param_spec_image_id ("image",
                                                             "Image",
                                                             "Output image",
                                                             gimp, FALSE,
                                                             GIMP_PARAM_READWRITE));
  gimp_plug_in_manager_add_procedure (gimp->plug_in_manager, proc);
  g_object_unref (procedure);
}

void
xcf_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
}

static GimpValueArray *
xcf_load_invoker (GimpProcedure         *procedure,
                  Gimp                  *gimp,
                  GimpContext           *context,
                  GimpProgress          *progress,
                  const GimpValueArray  *args,
                  GError               **error)
{
  XcfInfo         info = { 0, };
  GimpValueArray *return_vals;
  GimpImage      *image   = NULL;
  const gchar    *uri;
  gchar          *filename;
  GFile          *file;
  gboolean        success = FALSE;
  gchar           id[14];
  GError         *my_error = NULL;

  gimp_set_busy (gimp);

  uri      = g_value_get_string (gimp_value_array_index (args, 1));
  file     = g_file_new_for_uri (uri);
  filename = g_file_get_parse_name (file);

  info.input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (info.input)
    {
      info.gimp        = gimp;
      info.seekable    = G_SEEKABLE (info.input);
      info.progress    = progress;
      info.filename    = filename;
      info.compression = COMPRESS_NONE;

      if (progress)
        gimp_progress_start (progress, FALSE, _("Opening '%s'"), filename);

      success = TRUE;

      info.cp += xcf_read_int8 (info.input, (guint8 *) id, 14);

      if (! g_str_has_prefix (id, "gimp xcf "))
        {
          success = FALSE;
        }
      else if (strcmp (id + 9, "file") == 0)
        {
          info.file_version = 0;
        }
      else if (id[9] == 'v')
        {
          info.file_version = atoi (id + 10);
        }
      else
        {
          success = FALSE;
        }

      if (success)
        {
          if (info.file_version >= 0 &&
              info.file_version < G_N_ELEMENTS (xcf_loaders))
            {
              image = (*(xcf_loaders[info.file_version])) (gimp, &info, error);

              if (! image)
                success = FALSE;
            }
          else
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("XCF error: unsupported XCF file version %d "
                             "encountered"), info.file_version);
              success = FALSE;
            }
        }

      g_object_unref (info.input);

      if (progress)
        gimp_progress_end (progress);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Could not open '%s' for reading: "),
                                  filename);
    }

  g_free (filename);
  g_object_unref (file);

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    gimp_value_set_image (gimp_value_array_index (return_vals, 1), image);

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
  XcfInfo         info = { 0, };
  GimpValueArray *return_vals;
  GimpImage      *image;
  const gchar    *uri;
  gchar          *filename;
  GFile          *file;
  gboolean        success  = FALSE;
  GError         *my_error = NULL;

  gimp_set_busy (gimp);

  image    = gimp_value_get_image (gimp_value_array_index (args, 1), gimp);
  uri      = g_value_get_string (gimp_value_array_index (args, 3));
  file     = g_file_new_for_uri (uri);
  filename = g_file_get_parse_name (file);

  info.output = G_OUTPUT_STREAM (g_file_replace (file,
                                                 NULL, FALSE, G_FILE_CREATE_NONE,
                                                 NULL, &my_error));

  if (info.output)
    {
      gboolean compat_mode = gimp_image_get_xcf_compat_mode (image);

      info.gimp         = gimp;
      info.seekable     = G_SEEKABLE (info.output);
      info.progress     = progress;
      info.filename     = filename;

      if (compat_mode)
        info.compression = COMPRESS_RLE;
      else
        info.compression = COMPRESS_ZLIB;

      info.file_version = gimp_image_get_xcf_version (image,
                                                      info.compression ==
                                                      COMPRESS_ZLIB,
                                                      NULL, NULL);

      if (progress)
        gimp_progress_start (progress, FALSE, _("Saving '%s'"), filename);

      success = xcf_save_image (&info, image, &my_error);

      if (success)
        {
          if (progress)
            gimp_progress_set_text (progress, _("Closing '%s'"), filename);

          success = g_output_stream_close (info.output, NULL, &my_error);
        }

      if (! success)
        g_propagate_prefixed_error (error, my_error,
                                    _("Error writing '%s': "),
                                    filename);

      g_object_unref (info.output);

      if (progress)
        gimp_progress_end (progress);
    }

  g_free (filename);
  g_object_unref (file);

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  gimp_unset_busy (gimp);

  return return_vals;
}
