/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimageprocedure.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_IMAGE_PROCEDURE_H__
#define __GIMP_IMAGE_PROCEDURE_H__

#include <libgimp/gimpprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpRunImageFunc:
 * @procedure:   the #GimpProcedure that runs.
 * @run_mode:    the #GimpRunMode.
 * @image:       the #GimpImage.
 * @drawables: (array zero-terminated=1): the input #GimpDrawable-s.
 * @config:      the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in gimp_image_procedure_new().
 *
 * The image function is run during the lifetime of the GIMP session,
 * each time a plug-in image procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef GimpValueArray * (* GimpRunImageFunc) (GimpProcedure         *procedure,
                                               GimpRunMode            run_mode,
                                               GimpImage             *image,
                                               GimpDrawable         **drawables,
                                               GimpProcedureConfig   *config,
                                               gpointer               run_data);


#define GIMP_TYPE_IMAGE_PROCEDURE (gimp_image_procedure_get_type ())
G_DECLARE_FINAL_TYPE (GimpImageProcedure, gimp_image_procedure, GIMP, IMAGE_PROCEDURE, GimpProcedure)


GimpProcedure * gimp_image_procedure_new      (GimpPlugIn       *plug_in,
                                               const gchar      *name,
                                               GimpPDBProcType   proc_type,
                                               GimpRunImageFunc  run_func,
                                               gpointer          run_data,
                                               GDestroyNotify    run_data_destroy);


G_END_DECLS

#endif  /*  __GIMP_IMAGE_PROCEDURE_H__  */
