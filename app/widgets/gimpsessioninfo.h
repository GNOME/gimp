/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfo.h
 * Copyright (C) 2001-2008 Michael Natterer <mitch@gimp.org>
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

#include "core/gimpobject.h"


#define GIMP_TYPE_SESSION_INFO            (gimp_session_info_get_type ())
#define GIMP_SESSION_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SESSION_INFO, GimpSessionInfo))
#define GIMP_SESSION_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SESSION_INFO, GimpSessionInfoClass))
#define GIMP_IS_SESSION_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SESSION_INFO))
#define GIMP_IS_SESSION_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SESSION_INFO))
#define GIMP_SESSION_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SESSION_INFO, GimpSessionInfoClass))


typedef struct _GimpSessionInfoPrivate  GimpSessionInfoPrivate;
typedef struct _GimpSessionInfoClass    GimpSessionInfoClass;

/**
 * GimpSessionInfo:
 *
 * Contains session info for one toplevel window in the interface such
 * as a dock, the empty-image-window, or the open/save dialog.
 */
struct _GimpSessionInfo
{
  GimpObject  parent_instance;

  GimpSessionInfoPrivate *p;
};

struct _GimpSessionInfoClass
{
  GimpObjectClass  parent_class;
};


GType             gimp_session_info_get_type                (void) G_GNUC_CONST;

GimpSessionInfo * gimp_session_info_new                     (void);

void              gimp_session_info_restore                 (GimpSessionInfo        *info,
                                                             GimpDialogFactory      *factory,
                                                             GdkMonitor             *monitor);
void              gimp_session_info_apply_geometry          (GimpSessionInfo        *info,
                                                             GdkMonitor             *current_monitor,
                                                             gboolean                apply_stored_monitor);
void              gimp_session_info_read_geometry           (GimpSessionInfo        *info,
                                                             GdkEventConfigure      *cevent);
void              gimp_session_info_get_info                (GimpSessionInfo        *info);
void              gimp_session_info_get_info_with_widget    (GimpSessionInfo        *info,
                                                             GtkWidget              *widget);
void              gimp_session_info_clear_info              (GimpSessionInfo        *info);
gboolean          gimp_session_info_is_singleton            (GimpSessionInfo        *info);
gboolean          gimp_session_info_is_session_managed      (GimpSessionInfo        *info);
gboolean          gimp_session_info_get_remember_size       (GimpSessionInfo        *info);
gboolean          gimp_session_info_get_remember_if_open    (GimpSessionInfo        *info);
GtkWidget       * gimp_session_info_get_widget              (GimpSessionInfo        *info);
void              gimp_session_info_set_widget              (GimpSessionInfo        *info,
                                                             GtkWidget              *widget);
GimpDialogFactoryEntry *
                  gimp_session_info_get_factory_entry       (GimpSessionInfo        *info);
void              gimp_session_info_set_factory_entry       (GimpSessionInfo        *info,
                                                             GimpDialogFactoryEntry *entry);
gboolean          gimp_session_info_get_open                (GimpSessionInfo        *info);
void              gimp_session_info_append_book             (GimpSessionInfo        *info,
                                                             GimpSessionInfoBook    *book);
gint              gimp_session_info_get_x                   (GimpSessionInfo        *info);
gint              gimp_session_info_get_y                   (GimpSessionInfo        *info);
gint              gimp_session_info_get_width               (GimpSessionInfo        *info);
gint              gimp_session_info_get_height              (GimpSessionInfo        *info);

void              gimp_session_info_set_position_accuracy   (gint                    accuracy);
gint              gimp_session_info_apply_position_accuracy (gint                    position);
