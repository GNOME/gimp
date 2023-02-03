/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprocedureaction.h
 * Copyright (C) 2004-2016 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PROCEDURE_ACTION_H__
#define __GIMP_PROCEDURE_ACTION_H__


#include "gimpactionimpl.h"


#define GIMP_TYPE_PROCEDURE_ACTION            (gimp_procedure_action_get_type ())
#define GIMP_PROCEDURE_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROCEDURE_ACTION, GimpProcedureAction))
#define GIMP_PROCEDURE_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PROCEDURE_ACTION, GimpProcedureActionClass))
#define GIMP_IS_PROCEDURE_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROCEDURE_ACTION))
#define GIMP_IS_PROCEDURE_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GIMP_TYPE_PROCEDURE_ACTION))
#define GIMP_PROCEDURE_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMP_TYPE_PROCEDURE_ACTION, GimpProcedureActionClass))


typedef struct _GimpProcedureActionClass GimpProcedureActionClass;

struct _GimpProcedureAction
{
  GimpActionImpl  parent_instance;

  GimpProcedure  *procedure;
};

struct _GimpProcedureActionClass
{
  GimpActionImplClass parent_class;
};


GType                 gimp_procedure_action_get_type (void) G_GNUC_CONST;

GimpProcedureAction * gimp_procedure_action_new      (const gchar   *name,
                                                      const gchar   *label,
                                                      const gchar   *tooltip,
                                                      const gchar   *icon_name,
                                                      const gchar   *help_id,
                                                      GimpProcedure *procedure,
                                                      GimpContext   *context);


#endif  /* __GIMP_PROCEDURE_ACTION_H__ */
