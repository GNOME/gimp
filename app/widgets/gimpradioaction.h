/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaradioaction.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
 * Copyright (C) 2008 Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_RADIO_ACTION_H__
#define __LIGMA_RADIO_ACTION_H__


#define LIGMA_TYPE_RADIO_ACTION            (ligma_radio_action_get_type ())
#define LIGMA_RADIO_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_RADIO_ACTION, LigmaRadioAction))
#define LIGMA_RADIO_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_RADIO_ACTION, LigmaRadioActionClass))
#define LIGMA_IS_RADIO_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_RADIO_ACTION))
#define LIGMA_IS_RADIO_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), LIGMA_TYPE_ACTION))
#define LIGMA_RADIO_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), LIGMA_TYPE_RADIO_ACTION, LigmaRadioActionClass))


typedef struct _LigmaRadioAction      LigmaRadioAction;
typedef struct _LigmaRadioActionClass LigmaRadioActionClass;

struct _LigmaRadioAction
{
  GtkRadioAction  parent_instance;
};

struct _LigmaRadioActionClass
{
  GtkRadioActionClass  parent_class;
};


GType            ligma_radio_action_get_type (void) G_GNUC_CONST;

GtkRadioAction * ligma_radio_action_new      (const gchar *name,
                                             const gchar *label,
                                             const gchar *tooltip,
                                             const gchar *icon_name,
                                             const gchar *help_id,
                                             gint         value);


#endif  /* __LIGMA_RADIO_ACTION_H__ */
