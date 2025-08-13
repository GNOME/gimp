/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectorloadprocedure.h
 * Copyright (C) 2024 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_VECTOR_LOAD_PROCEDURE_H__
#define __GIMP_VECTOR_LOAD_PROCEDURE_H__

#include <libgimp/gimpfileprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


typedef struct _GimpVectorLoadData
{
  gdouble   width;
  GimpUnit *width_unit;
  gboolean  exact_width;

  gdouble   height;
  GimpUnit *height_unit;
  gboolean  exact_height;

  gboolean  correct_ratio;

  gdouble   pixel_density;
  GimpUnit *density_unit;
  gboolean  exact_density;
} GimpVectorLoadData;

/**
 * GimpRunVectorLoadFunc:
 * @procedure:      the [class@Gimp.Procedure] that runs.
 * @run_mode:       the [enum@RunMode].
 * @file:           the [iface@Gio.File] to load from.
 * @width:          the desired width in pixel for the created image.
 * @height:         the desired height in pixel for the created image.
 * @extracted_data: dimensions returned by [callback@ExtractVectorFunc].
 * @metadata:       the [class@Gimp.Metadata] which will be added to the new image.
 * @flags: (inout): flags to filter which metadata will be added..
 * @config:         the @procedure's remaining arguments.
 * @data_from_extract: (closure): @data_for_run returned by [callback@ExtractVectorFunc].
 * @run_data: (closure): @run_data given in gimp_vector_load_procedure_new().
 *
 * The load function is run during the lifetime of the GIMP session, each time a
 * plug-in load procedure is called.
 *
 * You are expected to read @file and create a [class@Gimp.Image] out of its
 * data. This image will be the first return value.
 * @metadata will be filled from metadata from @file if our infrastructure
 * supports this format. You may tweak this object, for instance adding metadata
 * specific to the format. You can also edit @flags if you need to filter out
 * some specific common fields. For instance, it is customary to remove a
 * colorspace field with [flags@MetadataLoadFlags] when a profile was added.
 *
 * Regarding returned image dimensions:
 *
 * 1. If @width or @height is 0 or negative, the actual value will be computed
 *    so that ratio is preserved. If @prefer_native_dimension is %FALSE, at
 *    least one of the 2 dimensions should be strictly positive.
 * 2. If @preserve_ratio is %TRUE, then @width and @height are considered as a
 *    max size in their respective dimension. I.e. that the resulting image will
 *    be at most @widthx@height while preserving original ratio. @preserve_ratio
 *    is implied when any of the dimension is 0 or negative.
 * 3. If @prefer_native_dimension is %TRUE, and if the image has some kind of
 *    native size (if the format has such metadata or it can be computed), it
 *    will be used rather than @widthx@height. Note that if both dimensions are
 *    0 or negative, even if @prefer_native_dimension is TRUE yet the procedure
 *    cannot determine native dimensions, then maybe a dialog could be popped
 *    up (if implemented), unless the @run_mode is
 *    [enum@Gimp.RunMode.NONINTERACTIVE].
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef GimpValueArray * (* GimpRunVectorLoadFunc)       (GimpProcedure         *procedure,
                                                          GimpRunMode            run_mode,
                                                          GFile                 *file,
                                                          gint                   width,
                                                          gint                   height,
                                                          GimpVectorLoadData     extracted_data,
                                                          GimpMetadata          *metadata,
                                                          GimpMetadataLoadFlags *flags,
                                                          GimpProcedureConfig   *config,
                                                          gpointer               data_from_extract,
                                                          gpointer               run_data);

/**
 * GimpExtractVectorFunc:
 * @procedure:      the [class@Gimp.Procedure].
 * @run_mode:       the [enum@RunMode].
 * @metadata:       the [class@Gimp.Metadata] which will be added to the new image.
 * @config: (nullable): the @procedure's remaining arguments.
 * @file:           the [iface@Gio.File] to load from.
 * @extracted_data: (out): dimensions and pixel density extracted from @file.
 * @data_for_run: (out) (nullable): will be passed as @data_from_extract in [callback@RunVectorLoadFunc].
 * @data_for_run_destroy: (out) (nullable) (destroy data_for_run): the free function for @data_for_run.
 * @extract_data: (closure): the @extract_data given in [ctor@VectorLoadProcedure.new].
 * @error: (out): error to be filled when @file cannot be loaded.
 *
 * Loading a vector image happens in 2 steps:
 *
 * 1. this function is first run to determine which size should be actually requested.
 * 2. [callback@RunVectorLoadFunc] is called with the suggested @width and @height.
 *
 * This function is run during the lifetime of the GIMP session, as the first
 * step above. It should extract the maximum of information from the source
 * document to help GIMP take appropriate decisions for default values and also
 * for displaying relevant information in the load dialog (if necessary).
 *
 * The best case scenario is to be able to extract proper dimensions (@width and
 * @height) with valid units supported by GIMP. If not possible, returning
 * already processed dimensions then setting @exact_width and @exact_height to
 * %FALSE in @extracted_data is also an option. If all you can get are no-unit
 * dimensions, set them with %GIMP_UNIT_PIXEL and %correct_ratio to %TRUE to at
 * least give a valid ratio as a default.
 *
 * If there is no way to extract any valid default dimensions, not even a ratio,
 * then return %FALSE but leave %error as %NULL. [callback@RunVectorLoadFunc]
 * will still be called but default values might be bogus.
 * If the return value is %FALSE and %error is set, it means that the file is
 * invalid and cannot even be loaded. Thus [callback@RunVectorLoadFunc] won't be
 * run and %error is passed as the main run error.
 *
 * Note: when @procedure is run, the original arguments will be passed as
 * @config. Nevertheless it may happen that this function is called with a %NULL
 * @config, in particular when [method@VectorLoadProcedure.extract_dimensions] is
 * called. In such a case, the callback is expected to return whatever can be
 * considered "best judgement" defaults.
 *
 * Returns: %TRUE if any information could be extracted from @file.
 *
 * Since: 3.0
 **/
typedef gboolean         (* GimpExtractVectorFunc) (GimpProcedure             *procedure,
                                                    GimpRunMode                run_mode,
                                                    GFile                     *file,
                                                    GimpMetadata              *metadata,
                                                    GimpProcedureConfig       *config,
                                                    GimpVectorLoadData        *extracted_data,
                                                    gpointer                  *data_for_run,
                                                    GDestroyNotify            *data_for_run_destroy,
                                                    gpointer                   extract_data,
                                                    GError                   **error);


#define GIMP_TYPE_VECTOR_LOAD_PROCEDURE (gimp_vector_load_procedure_get_type ())
G_DECLARE_FINAL_TYPE (GimpVectorLoadProcedure, gimp_vector_load_procedure, GIMP, VECTOR_LOAD_PROCEDURE, GimpLoadProcedure)


GimpProcedure * gimp_vector_load_procedure_new                (GimpPlugIn                 *plug_in,
                                                               const gchar                *name,
                                                               GimpPDBProcType             proc_type,
                                                               GimpExtractVectorFunc       extract_func,
                                                               gpointer                    extract_data,
                                                               GDestroyNotify              extract_data_destroy,
                                                               GimpRunVectorLoadFunc       run_func,
                                                               gpointer                    run_data,
                                                               GDestroyNotify              run_data_destroy);

gboolean        gimp_vector_load_procedure_extract_dimensions (GimpVectorLoadProcedure    *procedure,
                                                               GFile                      *file,
                                                               GimpVectorLoadData         *data,
                                                               GError                    **error);


G_END_DECLS

#endif  /*  __GIMP_VECTOR_LOAD_PROCEDURE_H__  */
