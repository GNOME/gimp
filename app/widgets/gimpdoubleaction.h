/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadoubleaction.h
 * Copyright (C) 2022 Jehan
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

#ifndef __LIGMA_DOUBLE_ACTION_H__
#define __LIGMA_DOUBLE_ACTION_H__


#include "ligmaactionimpl.h"


#define LIGMA_TYPE_DOUBLE_ACTION            (ligma_double_action_get_type ())
#define LIGMA_DOUBLE_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DOUBLE_ACTION, LigmaDoubleAction))
#define LIGMA_DOUBLE_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DOUBLE_ACTION, LigmaDoubleActionClass))
#define LIGMA_IS_DOUBLE_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DOUBLE_ACTION))
#define LIGMA_IS_DOUBLE_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), LIGMA_TYPE_DOUBLE_ACTION))
#define LIGMA_DOUBLE_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), LIGMA_TYPE_DOUBLE_ACTION, LigmaDoubleActionClass))


typedef struct _LigmaDoubleActionClass LigmaDoubleActionClass;

struct _LigmaDoubleAction
{
  LigmaActionImpl  parent_instance;

  gdouble         value;
};

struct _LigmaDoubleActionClass
{
  LigmaActionImplClass parent_class;
};


GType              ligma_double_action_get_type (void) G_GNUC_CONST;

LigmaDoubleAction * ligma_double_action_new      (const gchar *name,
                                                const gchar *label,
                                                const gchar *tooltip,
                                                const gchar *icon_name,
                                                const gchar *help_id,
                                                gdouble      value);


#endif  /* __LIGMA_DOUBLE_ACTION_H__ */
