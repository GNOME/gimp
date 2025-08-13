/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpexportprocedure.h
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

#ifndef __GIMP_EXPORT_PROCEDURE_H__
#define __GIMP_EXPORT_PROCEDURE_H__

#include <libgimp/gimpfileprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpRunExportFunc:
 * @procedure:   the #GimpProcedure that runs.
 * @run_mode:    the #GimpRunMode.
 * @image:       the image to export.
 * @file:        the #GFile to export to.
 * @options:     the #GimpExportOptions settings.
 * @metadata:    metadata object prepared for the mimetype passed in
 *               gimp_file_procedure_set_mime_types() if export_metadata
 *               argument was set in gimp_export_procedure_new().
 * @config:      the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in gimp_export_procedure_new().
 *
 * The export function is run during the lifetime of the GIMP session,
 * each time a plug-in export procedure is called.
 *
 * If a MimeType was passed in gimp_export_procedure_new(), then @metadata will be
 * non-%NULL and can be tweaked by the run() function if needed. Otherwise you
 * can let it as-is and it will be stored back into the exported @file according
 * to rules on metadata export shared across formats.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef GimpValueArray * (* GimpRunExportFunc) (GimpProcedure        *procedure,
                                                GimpRunMode           run_mode,
                                                GimpImage            *image,
                                                GFile                *file,
                                                GimpExportOptions    *options,
                                                GimpMetadata         *metadata,
                                                GimpProcedureConfig  *config,
                                                gpointer              run_data);

/**
 * GimpExportGetCapabilitiesFunc:
 * @procedure:   the #GimpProcedure that runs.
 * @config:      the #GimpProcedureConfig.
 * @options:     the @GimpExportOptions object to update.
 * @get_capabilities_data: (closure): the @get_capabilities_data argument
 *                         given to [method@Gimp.ExportProcedure.set_capabilities].
 *
 * This function returns the capabilities requested by your export
 * procedure, depending on @config or @options.
 *
 * Returns: a bitfield of the capabilities you want when preparing the
 *          image for exporting.
 *
 * Since: 3.0
 **/
typedef GimpExportCapabilities   (* GimpExportGetCapabilitiesFunc) (GimpProcedure        *procedure,
                                                                    GimpProcedureConfig  *config,
                                                                    GimpExportOptions    *options,
                                                                    gpointer              get_capabilities_data);


#define GIMP_TYPE_EXPORT_PROCEDURE (gimp_export_procedure_get_type ())
G_DECLARE_FINAL_TYPE (GimpExportProcedure, gimp_export_procedure, GIMP, EXPORT_PROCEDURE, GimpFileProcedure)


GimpProcedure * gimp_export_procedure_new                   (GimpPlugIn                    *plug_in,
                                                             const gchar                   *name,
                                                             GimpPDBProcType                proc_type,
                                                             gboolean                       export_metadata,
                                                             GimpRunExportFunc              run_func,
                                                             gpointer                       run_data,
                                                             GDestroyNotify                 run_data_destroy);

void            gimp_export_procedure_set_capabilities      (GimpExportProcedure           *procedure,
                                                             GimpExportCapabilities         capabilities,
                                                             GimpExportGetCapabilitiesFunc  get_capabilities_func,
                                                             gpointer                       get_capabilities_data,
                                                             GDestroyNotify                 get_capabilities_data_destroy);


void            gimp_export_procedure_set_support_exif      (GimpExportProcedure           *procedure,
                                                             gboolean                       supports);
void            gimp_export_procedure_set_support_iptc      (GimpExportProcedure           *procedure,
                                                             gboolean                       supports);
void            gimp_export_procedure_set_support_xmp       (GimpExportProcedure           *procedure,
                                                             gboolean                       supports);
void            gimp_export_procedure_set_support_profile   (GimpExportProcedure           *procedure,
                                                             gboolean                       supports);
void            gimp_export_procedure_set_support_thumbnail (GimpExportProcedure           *procedure,
                                                             gboolean                       supports);
void            gimp_export_procedure_set_support_comment   (GimpExportProcedure           *procedure,
                                                             gboolean                       supports);

gboolean        gimp_export_procedure_get_support_exif      (GimpExportProcedure           *procedure);
gboolean        gimp_export_procedure_get_support_iptc      (GimpExportProcedure           *procedure);
gboolean        gimp_export_procedure_get_support_xmp       (GimpExportProcedure           *procedure);
gboolean        gimp_export_procedure_get_support_profile   (GimpExportProcedure           *procedure);
gboolean        gimp_export_procedure_get_support_thumbnail (GimpExportProcedure           *procedure);
gboolean        gimp_export_procedure_get_support_comment   (GimpExportProcedure           *procedure);


G_END_DECLS

#endif  /*  __GIMP_EXPORT_PROCEDURE_H__  */
