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
 * @procedure: the #GimpProcedure that runs.
 * @run_mode:  the #GimpRunMode.
 * @image:     the #GimpImage.
 * @drawable:  the #GimpDrawable.
 * @args:      the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in gimp_image_procedure_new().
 *
 * The image function is run during the lifetime of the GIMP session,
 * each time a plug-in image procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef GimpValueArray * (* GimpRunImageFunc) (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               const GimpValueArray *args,
                                               gpointer              run_data);


#define GIMP_TYPE_IMAGE_PROCEDURE            (gimp_image_procedure_get_type ())
#define GIMP_IMAGE_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_PROCEDURE, GimpImageProcedure))
#define GIMP_IMAGE_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_PROCEDURE, GimpImageProcedureClass))
#define GIMP_IS_IMAGE_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_PROCEDURE))
#define GIMP_IS_IMAGE_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_PROCEDURE))
#define GIMP_IMAGE_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_PROCEDURE, GimpImageProcedureClass))


typedef struct _GimpImageProcedure        GimpImageProcedure;
typedef struct _GimpImageProcedureClass   GimpImageProcedureClass;
typedef struct _GimpImageProcedurePrivate GimpImageProcedurePrivate;

struct _GimpImageProcedure
{
  GimpProcedure              parent_instance;

  GimpImageProcedurePrivate *priv;
};

struct _GimpImageProcedureClass
{
  GimpProcedureClass parent_class;
};


GType           gimp_image_procedure_get_type (void) G_GNUC_CONST;

GimpProcedure * gimp_image_procedure_new      (GimpPlugIn       *plug_in,
                                               const gchar      *name,
                                               GimpPDBProcType   proc_type,
                                               GimpRunImageFunc  run_func,
                                               gpointer          run_data,
                                               GDestroyNotify    run_data_destroy);


G_END_DECLS

#endif  /*  __GIMP_IMAGE_PROCEDURE_H__  */
