/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpradioaction.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
 * Copyright (C) 2023 Jehan
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

#pragma once

#include "gimptoggleaction.h"


#define GIMP_TYPE_RADIO_ACTION            (gimp_radio_action_get_type ())
#define GIMP_RADIO_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RADIO_ACTION, GimpRadioAction))
#define GIMP_RADIO_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_RADIO_ACTION, GimpRadioActionClass))
#define GIMP_IS_RADIO_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_RADIO_ACTION))
#define GIMP_IS_RADIO_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GIMP_TYPE_RADIO_ACTION))
#define GIMP_RADIO_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMP_TYPE_RADIO_ACTION, GimpRadioActionClass))


typedef struct _GimpRadioActionClass   GimpRadioActionClass;
typedef struct _GimpRadioActionPrivate GimpRadioActionPrivate;

struct _GimpRadioAction
{
  GimpToggleAction        parent_instance;

  GimpRadioActionPrivate *priv;
};

struct _GimpRadioActionClass
{
  GimpToggleActionClass  parent_class;
};


GType         gimp_radio_action_get_type          (void) G_GNUC_CONST;

GimpAction  * gimp_radio_action_new               (const gchar *name,
                                                   const gchar *label,
                                                   const gchar *short_label,
                                                   const gchar *tooltip,
                                                   const gchar *icon_name,
                                                   const gchar *help_id,
                                                   gint         value,
                                                   GimpContext *context);

GSList      * gimp_radio_action_get_group         (GimpRadioAction *action);
void          gimp_radio_action_set_group         (GimpRadioAction *action,
                                                   GSList          *group);
void          gimp_radio_action_set_group_label   (GimpRadioAction *action,
                                                   const gchar     *label);
const gchar * gimp_radio_action_get_group_label   (GimpRadioAction *action);

gint          gimp_radio_action_get_current_value (GimpRadioAction *action);
void          gimp_radio_action_set_current_value (GimpRadioAction *action,
                                                   gint             current_value);
