/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatemporaryprocedure.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_TEMPORARY_PROCEDURE_H__
#define __LIGMA_TEMPORARY_PROCEDURE_H__


#include "ligmapluginprocedure.h"


#define LIGMA_TYPE_TEMPORARY_PROCEDURE            (ligma_temporary_procedure_get_type ())
#define LIGMA_TEMPORARY_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEMPORARY_PROCEDURE, LigmaTemporaryProcedure))
#define LIGMA_TEMPORARY_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TEMPORARY_PROCEDURE, LigmaTemporaryProcedureClass))
#define LIGMA_IS_TEMPORARY_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEMPORARY_PROCEDURE))
#define LIGMA_IS_TEMPORARY_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TEMPORARY_PROCEDURE))
#define LIGMA_TEMPORARY_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TEMPORARY_PROCEDURE, LigmaTemporaryProcedureClass))


typedef struct _LigmaTemporaryProcedureClass LigmaTemporaryProcedureClass;

struct _LigmaTemporaryProcedure
{
  LigmaPlugInProcedure  parent_instance;

  LigmaPlugIn          *plug_in;
};

struct _LigmaTemporaryProcedureClass
{
  LigmaPlugInProcedureClass parent_class;
};


GType           ligma_temporary_procedure_get_type (void) G_GNUC_CONST;

LigmaProcedure * ligma_temporary_procedure_new      (LigmaPlugIn *plug_in);


#endif /* __LIGMA_TEMPORARY_PROCEDURE_H__ */
