/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstringaction.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "gimpactionimpl.h"


#define GIMP_TYPE_STRING_ACTION            (gimp_string_action_get_type ())
#define GIMP_STRING_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_STRING_ACTION, GimpStringAction))
#define GIMP_STRING_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_STRING_ACTION, GimpStringActionClass))
#define GIMP_IS_STRING_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_STRING_ACTION))
#define GIMP_IS_STRING_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GIMP_TYPE_STRING_ACTION))
#define GIMP_STRING_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMP_TYPE_STRING_ACTION, GimpStringActionClass))


typedef struct _GimpStringActionClass GimpStringActionClass;

struct _GimpStringAction
{
  GimpActionImpl  parent_instance;

  gchar          *value;
};

struct _GimpStringActionClass
{
  GimpActionImplClass parent_class;
};


GType              gimp_string_action_get_type (void) G_GNUC_CONST;

GimpStringAction * gimp_string_action_new      (const gchar *name,
                                                const gchar *label,
                                                const gchar *short_label,
                                                const gchar *tooltip,
                                                const gchar *icon_name,
                                                const gchar *help_id,
                                                const gchar *value,
                                                GimpContext *context);
