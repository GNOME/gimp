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
                                                GimpMetadata         *metadata,
                                                GimpProcedureConfig  *config,
                                                gpointer              run_data);


#define GIMP_TYPE_EXPORT_PROCEDURE            (gimp_export_procedure_get_type ())
#define GIMP_EXPORT_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_EXPORT_PROCEDURE, GimpExportProcedure))
#define GIMP_EXPORT_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EXPORT_PROCEDURE, GimpExportProcedureClass))
#define GIMP_IS_EXPORT_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_EXPORT_PROCEDURE))
#define GIMP_IS_EXPORT_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EXPORT_PROCEDURE))
#define GIMP_EXPORT_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_EXPORT_PROCEDURE, GimpExportProcedureClass))


typedef struct _GimpExportProcedure        GimpExportProcedure;
typedef struct _GimpExportProcedureClass   GimpExportProcedureClass;
typedef struct _GimpExportProcedurePrivate GimpExportProcedurePrivate;

struct _GimpExportProcedure
{
  GimpFileProcedure           parent_instance;

  GimpExportProcedurePrivate *priv;
};

struct _GimpExportProcedureClass
{
  GimpFileProcedureClass parent_class;
};


GType           gimp_export_procedure_get_type              (void) G_GNUC_CONST;

GimpProcedure * gimp_export_procedure_new                   (GimpPlugIn        *plug_in,
                                                             const gchar       *name,
                                                             GimpPDBProcType    proc_type,
                                                             gboolean           export_metadata,
                                                             GimpRunExportFunc  run_func,
                                                             gpointer           run_data,
                                                             GDestroyNotify     run_data_destroy);

void            gimp_export_procedure_set_support_exif      (GimpExportProcedure *procedure,
                                                             gboolean             supports);
void            gimp_export_procedure_set_support_iptc      (GimpExportProcedure *procedure,
                                                             gboolean             supports);
void            gimp_export_procedure_set_support_xmp       (GimpExportProcedure *procedure,
                                                             gboolean             supports);
void            gimp_export_procedure_set_support_profile   (GimpExportProcedure *procedure,
                                                             gboolean             supports);
void            gimp_export_procedure_set_support_thumbnail (GimpExportProcedure *procedure,
                                                             gboolean             supports);
void            gimp_export_procedure_set_support_comment   (GimpExportProcedure *procedure,
                                                             gboolean             supports);

gboolean        gimp_export_procedure_get_support_exif      (GimpExportProcedure *procedure);
gboolean        gimp_export_procedure_get_support_iptc      (GimpExportProcedure *procedure);
gboolean        gimp_export_procedure_get_support_xmp       (GimpExportProcedure *procedure);
gboolean        gimp_export_procedure_get_support_profile   (GimpExportProcedure *procedure);
gboolean        gimp_export_procedure_get_support_thumbnail (GimpExportProcedure *procedure);
gboolean        gimp_export_procedure_get_support_comment   (GimpExportProcedure *procedure);


G_END_DECLS

#endif  /*  __GIMP_EXPORT_PROCEDURE_H__  */
