/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmathumbnailprocedure.h
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

#ifndef __LIGMA_THUMBNAIL_PROCEDURE_H__
#define __LIGMA_THUMBNAIL_PROCEDURE_H__

#include <libligma/ligmaprocedure.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaRunThumbnailFunc:
 * @procedure:   the #LigmaProcedure that runs.
 * @file:        the #GFile to load the thumbnail from.
 * @size:        the requested thumbnail size.
 * @args:        the @procedure's remaining arguments.
 * @run_data: (closure): the run_data given in ligma_thumbnail_procedure_new().
 *
 * The thumbnail function is run during the lifetime of the LIGMA session,
 * each time a plug-in thumbnail procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef LigmaValueArray * (* LigmaRunThumbnailFunc) (LigmaProcedure        *procedure,
                                                   GFile                *file,
                                                   gint                  size,
                                                   const LigmaValueArray *args,
                                                   gpointer              run_data);


#define LIGMA_TYPE_THUMBNAIL_PROCEDURE            (ligma_thumbnail_procedure_get_type ())
#define LIGMA_THUMBNAIL_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_THUMBNAIL_PROCEDURE, LigmaThumbnailProcedure))
#define LIGMA_THUMBNAIL_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_THUMBNAIL_PROCEDURE, LigmaThumbnailProcedureClass))
#define LIGMA_IS_THUMBNAIL_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_THUMBNAIL_PROCEDURE))
#define LIGMA_IS_THUMBNAIL_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_THUMBNAIL_PROCEDURE))
#define LIGMA_THUMBNAIL_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_THUMBNAIL_PROCEDURE, LigmaThumbnailProcedureClass))


typedef struct _LigmaThumbnailProcedure        LigmaThumbnailProcedure;
typedef struct _LigmaThumbnailProcedureClass   LigmaThumbnailProcedureClass;
typedef struct _LigmaThumbnailProcedurePrivate LigmaThumbnailProcedurePrivate;

struct _LigmaThumbnailProcedure
{
  LigmaProcedure                  parent_instance;

  LigmaThumbnailProcedurePrivate *priv;
};

struct _LigmaThumbnailProcedureClass
{
  LigmaProcedureClass parent_class;
};


GType           ligma_thumbnail_procedure_get_type (void) G_GNUC_CONST;

LigmaProcedure * ligma_thumbnail_procedure_new      (LigmaPlugIn           *plug_in,
                                                   const gchar          *name,
                                                   LigmaPDBProcType       proc_type,
                                                   LigmaRunThumbnailFunc  run_func,
                                                   gpointer              run_data,
                                                   GDestroyNotify        run_data_destroy);


G_END_DECLS

#endif  /*  __LIGMA_THUMBNAIL_PROCEDURE_H__  */
