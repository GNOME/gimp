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


/**
 * GimpRunVectorLoadFunc:
 * @procedure:      the [class@Gimp.Procedure] that runs.
 * @run_mode:       the [enum@RunMode].
 * @file:           the [iface@Gio.File] to load from.
 * @width:          the desired width in pixel for the created image.
 * @height:         the desired height in pixel for the created image.
 * @keep_ratio:     whether dimension ratio should be preserved.
 * @prefer_native_dimension: whether native dimension should override widthxheight.
 * @metadata:       the [class@Gimp.Metadata] which will be added to the new image.
 * @flags: (inout): flags to filter which metadata will be added..
 * @config:         the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in gimp_vector_load_procedure_new().
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
 *    up (if implemented), unless the @run_mode is [enum@RunMode.NONINTERACTIVE].
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
                                                          gboolean               keep_ratio,
                                                          gboolean               prefer_native_dimension,
                                                          GimpMetadata          *metadata,
                                                          GimpMetadataLoadFlags *flags,
                                                          GimpProcedureConfig   *config,
                                                          gpointer               run_data);


#define GIMP_TYPE_VECTOR_LOAD_PROCEDURE (gimp_vector_load_procedure_get_type ())
G_DECLARE_FINAL_TYPE (GimpVectorLoadProcedure, gimp_vector_load_procedure, GIMP, VECTOR_LOAD_PROCEDURE, GimpLoadProcedure)

typedef struct _GimpVectorLoadProcedure        GimpVectorLoadProcedure;


GimpProcedure * gimp_vector_load_procedure_new                  (GimpPlugIn              *plug_in,
                                                                 const gchar             *name,
                                                                 GimpPDBProcType          proc_type,
                                                                 GimpRunVectorLoadFunc    run_func,
                                                                 gpointer                 run_data,
                                                                 GDestroyNotify           run_data_destroy);


G_END_DECLS

#endif  /*  __GIMP_VECTOR_LOAD_PROCEDURE_H__  */
