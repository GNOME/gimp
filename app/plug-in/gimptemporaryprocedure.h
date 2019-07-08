/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptemporaryprocedure.h
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

#ifndef __GIMP_TEMPORARY_PROCEDURE_H__
#define __GIMP_TEMPORARY_PROCEDURE_H__


#include "gimppluginprocedure.h"


#define GIMP_TYPE_TEMPORARY_PROCEDURE            (gimp_temporary_procedure_get_type ())
#define GIMP_TEMPORARY_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEMPORARY_PROCEDURE, GimpTemporaryProcedure))
#define GIMP_TEMPORARY_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEMPORARY_PROCEDURE, GimpTemporaryProcedureClass))
#define GIMP_IS_TEMPORARY_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEMPORARY_PROCEDURE))
#define GIMP_IS_TEMPORARY_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEMPORARY_PROCEDURE))
#define GIMP_TEMPORARY_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TEMPORARY_PROCEDURE, GimpTemporaryProcedureClass))


typedef struct _GimpTemporaryProcedureClass GimpTemporaryProcedureClass;

struct _GimpTemporaryProcedure
{
  GimpPlugInProcedure  parent_instance;

  GimpPlugIn          *plug_in;
};

struct _GimpTemporaryProcedureClass
{
  GimpPlugInProcedureClass parent_class;
};


GType           gimp_temporary_procedure_get_type (void) G_GNUC_CONST;

GimpProcedure * gimp_temporary_procedure_new      (GimpPlugIn *plug_in);


#endif /* __GIMP_TEMPORARY_PROCEDURE_H__ */
