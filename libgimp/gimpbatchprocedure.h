/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbatchprocedure.h
 * Copyright (C) 2022 Jehan
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

#ifndef __GIMP_BATCH_PROCEDURE_H__
#define __GIMP_BATCH_PROCEDURE_H__

#include <libgimp/gimpprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpBatchFunc:
 * @procedure:   the #GimpProcedure that runs.
 * @run_mode:    the #GimpRunMode.
 * @config:      the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in gimp_batch_procedure_new().
 *
 * The batch function is run during the lifetime of the GIMP session,
 * each time a plug-in batch procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef GimpValueArray * (* GimpBatchFunc) (GimpProcedure        *procedure,
                                            GimpRunMode           run_mode,
                                            const gchar          *command,
                                            GimpProcedureConfig  *config,
                                            gpointer              run_data);


#define GIMP_TYPE_BATCH_PROCEDURE (gimp_batch_procedure_get_type ())
G_DECLARE_FINAL_TYPE (GimpBatchProcedure, gimp_batch_procedure, GIMP, BATCH_PROCEDURE, GimpProcedure)


GimpProcedure * gimp_batch_procedure_new                  (GimpPlugIn      *plug_in,
                                                           const gchar     *name,
                                                           const gchar     *interpreter_name,
                                                           GimpPDBProcType  proc_type,
                                                           GimpBatchFunc    run_func,
                                                           gpointer         run_data,
                                                           GDestroyNotify   run_data_destroy);

void            gimp_batch_procedure_set_interpreter_name (GimpBatchProcedure *procedure,
                                                           const gchar        *interpreter_name);
const gchar *   gimp_batch_procedure_get_interpreter_name (GimpBatchProcedure *procedure);

G_END_DECLS

#endif  /*  __GIMP_BATCH_PROCEDURE_H__  */
