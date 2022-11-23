/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasessioninfo.h
 * Copyright (C) 2001-2008 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_SESSION_INFO_H__
#define __LIGMA_SESSION_INFO_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_SESSION_INFO            (ligma_session_info_get_type ())
#define LIGMA_SESSION_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SESSION_INFO, LigmaSessionInfo))
#define LIGMA_SESSION_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SESSION_INFO, LigmaSessionInfoClass))
#define LIGMA_IS_SESSION_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SESSION_INFO))
#define LIGMA_IS_SESSION_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SESSION_INFO))
#define LIGMA_SESSION_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SESSION_INFO, LigmaSessionInfoClass))


typedef struct _LigmaSessionInfoPrivate  LigmaSessionInfoPrivate;
typedef struct _LigmaSessionInfoClass    LigmaSessionInfoClass;

/**
 * LigmaSessionInfo:
 *
 * Contains session info for one toplevel window in the interface such
 * as a dock, the empty-image-window, or the open/save dialog.
 */
struct _LigmaSessionInfo
{
  LigmaObject  parent_instance;

  LigmaSessionInfoPrivate *p;
};

struct _LigmaSessionInfoClass
{
  LigmaObjectClass  parent_class;
};


GType             ligma_session_info_get_type                (void) G_GNUC_CONST;

LigmaSessionInfo * ligma_session_info_new                     (void);

void              ligma_session_info_restore                 (LigmaSessionInfo        *info,
                                                             LigmaDialogFactory      *factory,
                                                             GdkMonitor             *monitor);
void              ligma_session_info_apply_geometry          (LigmaSessionInfo        *info,
                                                             GdkMonitor             *current_monitor,
                                                             gboolean                apply_stored_monitor);
void              ligma_session_info_read_geometry           (LigmaSessionInfo        *info,
                                                             GdkEventConfigure      *cevent);
void              ligma_session_info_get_info                (LigmaSessionInfo        *info);
void              ligma_session_info_get_info_with_widget    (LigmaSessionInfo        *info,
                                                             GtkWidget              *widget);
void              ligma_session_info_clear_info              (LigmaSessionInfo        *info);
gboolean          ligma_session_info_is_singleton            (LigmaSessionInfo        *info);
gboolean          ligma_session_info_is_session_managed      (LigmaSessionInfo        *info);
gboolean          ligma_session_info_get_remember_size       (LigmaSessionInfo        *info);
gboolean          ligma_session_info_get_remember_if_open    (LigmaSessionInfo        *info);
GtkWidget       * ligma_session_info_get_widget              (LigmaSessionInfo        *info);
void              ligma_session_info_set_widget              (LigmaSessionInfo        *info,
                                                             GtkWidget              *widget);
LigmaDialogFactoryEntry *
                  ligma_session_info_get_factory_entry       (LigmaSessionInfo        *info);
void              ligma_session_info_set_factory_entry       (LigmaSessionInfo        *info,
                                                             LigmaDialogFactoryEntry *entry);
gboolean          ligma_session_info_get_open                (LigmaSessionInfo        *info);
void              ligma_session_info_append_book             (LigmaSessionInfo        *info,
                                                             LigmaSessionInfoBook    *book);
gint              ligma_session_info_get_x                   (LigmaSessionInfo        *info);
gint              ligma_session_info_get_y                   (LigmaSessionInfo        *info);
gint              ligma_session_info_get_width               (LigmaSessionInfo        *info);
gint              ligma_session_info_get_height              (LigmaSessionInfo        *info);

void              ligma_session_info_set_position_accuracy   (gint                    accuracy);
gint              ligma_session_info_apply_position_accuracy (gint                    position);


#endif  /* __LIGMA_SESSION_INFO_H__ */
