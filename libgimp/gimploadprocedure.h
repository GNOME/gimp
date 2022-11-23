/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaloadprocedure.h
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_LOAD_PROCEDURE_H__
#define __LIGMA_LOAD_PROCEDURE_H__

#include <libligma/ligmafileprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaRunLoadFunc:
 * @procedure:   the #LigmaProcedure that runs.
 * @run_mode:    the #LigmaRunMode.
 * @file:        the #GFile to load from.
 * @args:        the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in ligma_load_procedure_new().
 *
 * The load function is run during the lifetime of the LIGMA session,
 * each time a plug-in load procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef LigmaValueArray * (* LigmaRunLoadFunc) (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);


#define LIGMA_TYPE_LOAD_PROCEDURE            (ligma_load_procedure_get_type ())
#define LIGMA_LOAD_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LOAD_PROCEDURE, LigmaLoadProcedure))
#define LIGMA_LOAD_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LOAD_PROCEDURE, LigmaLoadProcedureClass))
#define LIGMA_IS_LOAD_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LOAD_PROCEDURE))
#define LIGMA_IS_LOAD_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LOAD_PROCEDURE))
#define LIGMA_LOAD_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LOAD_PROCEDURE, LigmaLoadProcedureClass))


typedef struct _LigmaLoadProcedure        LigmaLoadProcedure;
typedef struct _LigmaLoadProcedureClass   LigmaLoadProcedureClass;
typedef struct _LigmaLoadProcedurePrivate LigmaLoadProcedurePrivate;

struct _LigmaLoadProcedure
{
  LigmaFileProcedure         parent_instance;

  LigmaLoadProcedurePrivate *priv;
};

struct _LigmaLoadProcedureClass
{
  LigmaFileProcedureClass parent_class;
};


GType           ligma_load_procedure_get_type             (void) G_GNUC_CONST;

LigmaProcedure * ligma_load_procedure_new                  (LigmaPlugIn        *plug_in,
                                                          const gchar       *name,
                                                          LigmaPDBProcType    proc_type,
                                                          LigmaRunLoadFunc    run_func,
                                                          gpointer           run_data,
                                                          GDestroyNotify     run_data_destroy);

void            ligma_load_procedure_set_handles_raw      (LigmaLoadProcedure *procedure,
                                                          gboolean           handles_raw);
gboolean        ligma_load_procedure_get_handles_raw      (LigmaLoadProcedure *procedure);

void            ligma_load_procedure_set_thumbnail_loader (LigmaLoadProcedure *procedure,
                                                          const gchar       *thumbnail_proc);
const gchar   * ligma_load_procedure_get_thumbnail_loader (LigmaLoadProcedure *procedure);


G_END_DECLS

#endif  /*  __LIGMA_LOAD_PROCEDURE_H__  */
