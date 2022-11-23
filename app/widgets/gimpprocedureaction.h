/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaprocedureaction.h
 * Copyright (C) 2004-2016 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PROCEDURE_ACTION_H__
#define __LIGMA_PROCEDURE_ACTION_H__


#include "ligmaactionimpl.h"


#define LIGMA_TYPE_PROCEDURE_ACTION            (ligma_procedure_action_get_type ())
#define LIGMA_PROCEDURE_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PROCEDURE_ACTION, LigmaProcedureAction))
#define LIGMA_PROCEDURE_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PROCEDURE_ACTION, LigmaProcedureActionClass))
#define LIGMA_IS_PROCEDURE_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PROCEDURE_ACTION))
#define LIGMA_IS_PROCEDURE_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), LIGMA_TYPE_PROCEDURE_ACTION))
#define LIGMA_PROCEDURE_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), LIGMA_TYPE_PROCEDURE_ACTION, LigmaProcedureActionClass))


typedef struct _LigmaProcedureActionClass LigmaProcedureActionClass;

struct _LigmaProcedureAction
{
  LigmaActionImpl  parent_instance;

  LigmaProcedure  *procedure;
};

struct _LigmaProcedureActionClass
{
  LigmaActionImplClass parent_class;
};


GType                 ligma_procedure_action_get_type (void) G_GNUC_CONST;

LigmaProcedureAction * ligma_procedure_action_new      (const gchar   *name,
                                                      const gchar   *label,
                                                      const gchar   *tooltip,
                                                      const gchar   *icon_name,
                                                      const gchar   *help_id,
                                                      LigmaProcedure *procedure);


#endif  /* __LIGMA_PROCEDURE_ACTION_H__ */
