/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacontrollerinfo.h
 * Copyright (C) 2004-2005 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTROLLER_INFO_H__
#define __LIGMA_CONTROLLER_INFO_H__


#include "core/ligmaviewable.h"


typedef gboolean (* LigmaControllerEventSnooper) (LigmaControllerInfo        *info,
                                                 LigmaController            *controller,
                                                 const LigmaControllerEvent *event,
                                                 gpointer                   user_data);


#define LIGMA_TYPE_CONTROLLER_INFO            (ligma_controller_info_get_type ())
#define LIGMA_CONTROLLER_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTROLLER_INFO, LigmaControllerInfo))
#define LIGMA_CONTROLLER_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTROLLER_INFO, LigmaControllerInfoClass))
#define LIGMA_IS_CONTROLLER_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTROLLER_INFO))
#define LIGMA_IS_CONTROLLER_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTROLLER_INFO))
#define LIGMA_CONTROLLER_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTROLLER_INFO, LigmaControllerInfoClass))


typedef struct _LigmaControllerInfoClass LigmaControllerInfoClass;

struct _LigmaControllerInfo
{
  LigmaViewable                parent_instance;

  gboolean                    enabled;
  gboolean                    debug_events;

  LigmaController             *controller;
  GHashTable                 *mapping;

  LigmaControllerEventSnooper  snooper;
  gpointer                    snooper_data;
};

struct _LigmaControllerInfoClass
{
  LigmaViewableClass  parent_class;

  gboolean (* event_mapped) (LigmaControllerInfo        *info,
                             LigmaController            *controller,
                             const LigmaControllerEvent *event,
                             const gchar               *action_name);
};


GType    ligma_controller_info_get_type          (void) G_GNUC_CONST;

LigmaControllerInfo * ligma_controller_info_new   (GType                       type);

void     ligma_controller_info_set_enabled       (LigmaControllerInfo         *info,
                                                 gboolean                    enabled);
gboolean ligma_controller_info_get_enabled       (LigmaControllerInfo         *info);

void     ligma_controller_info_set_event_snooper (LigmaControllerInfo         *info,
                                                 LigmaControllerEventSnooper  snooper,
                                                 gpointer                    snooper_data);


#endif /* __LIGMA_CONTROLLER_INFO_H__ */
