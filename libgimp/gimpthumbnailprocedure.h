/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpthumbnailprocedure.h
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

#ifndef __GIMP_THUMBNAIL_PROCEDURE_H__
#define __GIMP_THUMBNAIL_PROCEDURE_H__

#include <libgimp/gimpprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpRunThumbnailFunc:
 * @procedure:   the #GimpProcedure that runs.
 * @file:        the #GFile to load the thumbnail from.
 * @size:        the requested thumbnail size.
 * @config:      the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in gimp_thumbnail_procedure_new().
 *
 * The thumbnail function is run during the lifetime of the GIMP session,
 * each time a plug-in thumbnail procedure is called.
 *
 * [class@ThumbnailProcedure] are always run non-interactively.
 *
 * On success, the returned array must contain:
 * 1. a [class@Image]: this is the only mandatory return value. It should
 *    ideally be a simple image whose dimensions are closest to @size and meant
 *    to be displayed as a small static image.
 * 2. (optional) the full image's width (not the thumbnail's image's), or 0 if
 *    unknown.
 * 3. (optional) the full image's height, or 0 if unknown.
 * 4. (optional) the [enum@ImageType] of the full image.
 * 5. (optional) the number of layers in the full image.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef GimpValueArray * (* GimpRunThumbnailFunc) (GimpProcedure        *procedure,
                                                   GFile                *file,
                                                   gint                  size,
                                                   GimpProcedureConfig  *config,
                                                   gpointer              run_data);


#define GIMP_TYPE_THUMBNAIL_PROCEDURE (gimp_thumbnail_procedure_get_type ())
G_DECLARE_FINAL_TYPE (GimpThumbnailProcedure, gimp_thumbnail_procedure, GIMP, THUMBNAIL_PROCEDURE, GimpProcedure)


GimpProcedure * gimp_thumbnail_procedure_new      (GimpPlugIn           *plug_in,
                                                   const gchar          *name,
                                                   GimpPDBProcType       proc_type,
                                                   GimpRunThumbnailFunc  run_func,
                                                   gpointer              run_data,
                                                   GDestroyNotify        run_data_destroy);


G_END_DECLS

#endif  /*  __GIMP_THUMBNAIL_PROCEDURE_H__  */
