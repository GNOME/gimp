/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabatchprocedure.h
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

#ifndef __LIGMA_BATCH_PROCEDURE_H__
#define __LIGMA_BATCH_PROCEDURE_H__

#include <libligma/ligmaprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaBatchFunc:
 * @procedure:   the #LigmaProcedure that runs.
 * @run_mode:    the #LigmaRunMode.
 * @args:        the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in ligma_batch_procedure_new().
 *
 * The batch function is run during the lifetime of the LIGMA session,
 * each time a plug-in batch procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef LigmaValueArray * (* LigmaBatchFunc) (LigmaProcedure        *procedure,
                                            LigmaRunMode           run_mode,
                                            const gchar          *command,
                                            const LigmaValueArray *args,
                                            gpointer              run_data);


#define LIGMA_TYPE_BATCH_PROCEDURE            (ligma_batch_procedure_get_type ())
#define LIGMA_BATCH_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BATCH_PROCEDURE, LigmaBatchProcedure))
#define LIGMA_BATCH_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BATCH_PROCEDURE, LigmaBatchProcedureClass))
#define LIGMA_IS_BATCH_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BATCH_PROCEDURE))
#define LIGMA_IS_BATCH_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BATCH_PROCEDURE))
#define LIGMA_BATCH_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BATCH_PROCEDURE, LigmaBatchProcedureClass))


typedef struct _LigmaBatchProcedure        LigmaBatchProcedure;
typedef struct _LigmaBatchProcedureClass   LigmaBatchProcedureClass;
typedef struct _LigmaBatchProcedurePrivate LigmaBatchProcedurePrivate;

struct _LigmaBatchProcedure
{
  LigmaProcedure              parent_instance;

  LigmaBatchProcedurePrivate *priv;
};

struct _LigmaBatchProcedureClass
{
  LigmaProcedureClass parent_class;
};


GType           ligma_batch_procedure_get_type             (void) G_GNUC_CONST;

LigmaProcedure * ligma_batch_procedure_new                  (LigmaPlugIn      *plug_in,
                                                           const gchar     *name,
                                                           const gchar     *interpreter_name,
                                                           LigmaPDBProcType  proc_type,
                                                           LigmaBatchFunc    run_func,
                                                           gpointer         run_data,
                                                           GDestroyNotify   run_data_destroy);

void            ligma_batch_procedure_set_interpreter_name (LigmaBatchProcedure *procedure,
                                                           const gchar        *interpreter_name);
const gchar *   ligma_batch_procedure_get_interpreter_name (LigmaBatchProcedure *procedure);

G_END_DECLS

#endif  /*  __LIGMA_BATCH_PROCEDURE_H__  */
