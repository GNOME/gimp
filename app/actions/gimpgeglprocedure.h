/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpgeglprocedure.h
 * Copyright (C) 2016 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_GEGL_PROCEDURE_H__
#define __GIMP_GEGL_PROCEDURE_H__


#include "pdb/gimpprocedure.h"


#define GIMP_TYPE_GEGL_PROCEDURE            (gimp_gegl_procedure_get_type ())
#define GIMP_GEGL_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GEGL_PROCEDURE, GimpGeglProcedure))
#define GIMP_GEGL_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GEGL_PROCEDURE, GimpGeglProcedureClass))
#define GIMP_IS_GEGL_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GEGL_PROCEDURE))
#define GIMP_IS_GEGL_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GEGL_PROCEDURE))
#define GIMP_GEGL_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_GEGL_PROCEDURE, GimpGeglProcedureClass))


typedef struct _GimpGeglProcedure      GimpGeglProcedure;
typedef struct _GimpGeglProcedureClass GimpGeglProcedureClass;

struct _GimpGeglProcedure
{
  GimpProcedure  parent_instance;

  GimpDrawableFilter *filter;
  gchar              *operation;

  GimpRunMode         default_run_mode;
  GimpObject         *default_settings;

  gchar              *menu_label;
};

struct _GimpGeglProcedureClass
{
  GimpProcedureClass parent_class;
};


GType           gimp_gegl_procedure_get_type (void) G_GNUC_CONST;

GimpProcedure * gimp_gegl_procedure_new           (Gimp               *gimp,
                                                   GimpDrawableFilter *filter,
                                                   GimpRunMode         default_run_mode,
                                                   GimpObject         *default_settings,
                                                   const gchar        *operation,
                                                   const gchar        *name,
                                                   const gchar        *menu_label,
                                                   const gchar        *tooltip,
                                                   const gchar        *icon_name,
                                                   const gchar        *help_id);

#endif /* __GIMP_GEGL_PROCEDURE_H__ */
