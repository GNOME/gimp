/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcontrollerinfo.h
 * Copyright (C) 2004-2005 Michael Natterer <mitch@gimp.org>
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

#include "core/gimpviewable.h"


typedef gboolean (* GimpControllerEventSnooper) (GimpControllerInfo        *info,
                                                 GimpController            *controller,
                                                 const GimpControllerEvent *event,
                                                 gpointer                   user_data);


#define GIMP_TYPE_CONTROLLER_INFO            (gimp_controller_info_get_type ())
#define GIMP_CONTROLLER_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTROLLER_INFO, GimpControllerInfo))
#define GIMP_CONTROLLER_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTROLLER_INFO, GimpControllerInfoClass))
#define GIMP_IS_CONTROLLER_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTROLLER_INFO))
#define GIMP_IS_CONTROLLER_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTROLLER_INFO))
#define GIMP_CONTROLLER_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTROLLER_INFO, GimpControllerInfoClass))


typedef struct _GimpControllerInfoClass GimpControllerInfoClass;

struct _GimpControllerInfo
{
  GimpViewable                parent_instance;

  gboolean                    enabled;
  gboolean                    debug_events;

  GimpController             *controller;
  GHashTable                 *mapping;

  GimpControllerEventSnooper  snooper;
  gpointer                    snooper_data;
};

struct _GimpControllerInfoClass
{
  GimpViewableClass  parent_class;

  gboolean (* event_mapped) (GimpControllerInfo        *info,
                             GimpController            *controller,
                             const GimpControllerEvent *event,
                             const gchar               *action_name);
};


GType    gimp_controller_info_get_type          (void) G_GNUC_CONST;

GimpControllerInfo * gimp_controller_info_new   (GType                       type);

void     gimp_controller_info_set_enabled       (GimpControllerInfo         *info,
                                                 gboolean                    enabled);
gboolean gimp_controller_info_get_enabled       (GimpControllerInfo         *info);

void     gimp_controller_info_set_event_snooper (GimpControllerInfo         *info,
                                                 GimpControllerEventSnooper  snooper,
                                                 gpointer                    snooper_data);
