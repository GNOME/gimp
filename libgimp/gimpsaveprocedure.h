/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsaveprocedure.h
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

#ifndef __GIMP_SAVE_PROCEDURE_H__
#define __GIMP_SAVE_PROCEDURE_H__

#include <libgimp/gimpfileprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpRunSaveFunc:
 * @procedure:   the #GimpProcedure that runs.
 * @run_mode:    the #GimpRunMode.
 * @image:       the image to save.
 * @n_drawables: the number of drawables to save.
 * @drawables:   the drawables to save.
 * @file:        the #GFile to save to.
 * @args:        the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in gimp_save_procedure_new().
 *
 * The save function is run during the lifetime of the GIMP session,
 * each time a plug-in save procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef GimpValueArray * (* GimpRunSaveFunc) (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              gint                  n_drawables,
                                              GimpDrawable        **drawables,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);


#define GIMP_TYPE_SAVE_PROCEDURE            (gimp_save_procedure_get_type ())
#define GIMP_SAVE_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SAVE_PROCEDURE, GimpSaveProcedure))
#define GIMP_SAVE_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SAVE_PROCEDURE, GimpSaveProcedureClass))
#define GIMP_IS_SAVE_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SAVE_PROCEDURE))
#define GIMP_IS_SAVE_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SAVE_PROCEDURE))
#define GIMP_SAVE_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SAVE_PROCEDURE, GimpSaveProcedureClass))


typedef struct _GimpSaveProcedure        GimpSaveProcedure;
typedef struct _GimpSaveProcedureClass   GimpSaveProcedureClass;
typedef struct _GimpSaveProcedurePrivate GimpSaveProcedurePrivate;

struct _GimpSaveProcedure
{
  GimpFileProcedure         parent_instance;

  GimpSaveProcedurePrivate *priv;
};

struct _GimpSaveProcedureClass
{
  GimpFileProcedureClass parent_class;
};


GType           gimp_save_procedure_get_type (void) G_GNUC_CONST;

GimpProcedure * gimp_save_procedure_new      (GimpPlugIn      *plug_in,
                                              const gchar     *name,
                                              GimpPDBProcType  proc_type,
                                              GimpRunSaveFunc  run_func,
                                              gpointer         run_data,
                                              GDestroyNotify   run_data_destroy);


G_END_DECLS

#endif  /*  __GIMP_SAVE_PROCEDURE_H__  */
