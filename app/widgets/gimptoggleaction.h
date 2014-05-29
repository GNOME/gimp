/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoggleaction.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_TOGGLE_ACTION_H__
#define __GIMP_TOGGLE_ACTION_H__


#define GIMP_TYPE_TOGGLE_ACTION            (gimp_toggle_action_get_type ())
#define GIMP_TOGGLE_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOGGLE_ACTION, GimpToggleAction))
#define GIMP_TOGGLE_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOGGLE_ACTION, GimpToggleActionClass))
#define GIMP_IS_TOGGLE_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOGGLE_ACTION))
#define GIMP_IS_TOGGLE_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GIMP_TYPE_ACTION))
#define GIMP_TOGGLE_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMP_TYPE_TOGGLE_ACTION, GimpToggleActionClass))


typedef struct _GimpToggleAction      GimpToggleAction;
typedef struct _GimpToggleActionClass GimpToggleActionClass;

struct _GimpToggleAction
{
  GtkToggleAction  parent_instance;
};

struct _GimpToggleActionClass
{
  GtkToggleActionClass  parent_class;
};


GType             gimp_toggle_action_get_type (void) G_GNUC_CONST;

GtkToggleAction * gimp_toggle_action_new      (const gchar *name,
                                               const gchar *label,
                                               const gchar *tooltip,
                                               const gchar *icon_name);


#endif  /* __GIMP_TOGGLE_ACTION_H__ */
