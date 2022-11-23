/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasaveprocedure.h
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

#ifndef __LIGMA_SAVE_PROCEDURE_H__
#define __LIGMA_SAVE_PROCEDURE_H__

#include <libligma/ligmafileprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaRunSaveFunc:
 * @procedure:   the #LigmaProcedure that runs.
 * @run_mode:    the #LigmaRunMode.
 * @image:       the image to save.
 * @n_drawables: the number of drawables to save.
 * @drawables: (array length=n_drawables):   the drawables to save.
 * @file:        the #GFile to save to.
 * @args:        the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in ligma_save_procedure_new().
 *
 * The save function is run during the lifetime of the LIGMA session,
 * each time a plug-in save procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef LigmaValueArray * (* LigmaRunSaveFunc) (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);


#define LIGMA_TYPE_SAVE_PROCEDURE            (ligma_save_procedure_get_type ())
#define LIGMA_SAVE_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SAVE_PROCEDURE, LigmaSaveProcedure))
#define LIGMA_SAVE_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SAVE_PROCEDURE, LigmaSaveProcedureClass))
#define LIGMA_IS_SAVE_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SAVE_PROCEDURE))
#define LIGMA_IS_SAVE_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SAVE_PROCEDURE))
#define LIGMA_SAVE_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SAVE_PROCEDURE, LigmaSaveProcedureClass))


typedef struct _LigmaSaveProcedure        LigmaSaveProcedure;
typedef struct _LigmaSaveProcedureClass   LigmaSaveProcedureClass;
typedef struct _LigmaSaveProcedurePrivate LigmaSaveProcedurePrivate;

struct _LigmaSaveProcedure
{
  LigmaFileProcedure         parent_instance;

  LigmaSaveProcedurePrivate *priv;
};

struct _LigmaSaveProcedureClass
{
  LigmaFileProcedureClass parent_class;
};


GType           ligma_save_procedure_get_type (void) G_GNUC_CONST;

LigmaProcedure * ligma_save_procedure_new      (LigmaPlugIn      *plug_in,
                                              const gchar     *name,
                                              LigmaPDBProcType  proc_type,
                                              LigmaRunSaveFunc  run_func,
                                              gpointer         run_data,
                                              GDestroyNotify   run_data_destroy);

void            ligma_save_procedure_set_support_exif      (LigmaSaveProcedure *procedure,
                                                           gboolean           supports);
void            ligma_save_procedure_set_support_iptc      (LigmaSaveProcedure *procedure,
                                                           gboolean           supports);
void            ligma_save_procedure_set_support_xmp       (LigmaSaveProcedure *procedure,
                                                           gboolean           supports);
void            ligma_save_procedure_set_support_profile   (LigmaSaveProcedure *procedure,
                                                           gboolean           supports);
void            ligma_save_procedure_set_support_thumbnail (LigmaSaveProcedure *procedure,
                                                           gboolean           supports);
void            ligma_save_procedure_set_support_comment   (LigmaSaveProcedure *procedure,
                                                            gboolean           supports);

gboolean        ligma_save_procedure_get_support_exif      (LigmaSaveProcedure *procedure);
gboolean        ligma_save_procedure_get_support_iptc      (LigmaSaveProcedure *procedure);
gboolean        ligma_save_procedure_get_support_xmp       (LigmaSaveProcedure *procedure);
gboolean        ligma_save_procedure_get_support_profile   (LigmaSaveProcedure *procedure);
gboolean        ligma_save_procedure_get_support_thumbnail (LigmaSaveProcedure *procedure);
gboolean        ligma_save_procedure_get_support_comment   (LigmaSaveProcedure *procedure);


G_END_DECLS

#endif  /*  __LIGMA_SAVE_PROCEDURE_H__  */
