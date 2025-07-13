/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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


typedef struct _GimpGui GimpGui;

struct _GimpGui
{
  void           (* ungrab)                 (Gimp                *gimp);

  void           (* set_busy)               (Gimp                *gimp);
  void           (* unset_busy)             (Gimp                *gimp);

  void           (* show_message)           (Gimp                *gimp,
                                             GObject             *handler,
                                             GimpMessageSeverity  severity,
                                             const gchar         *domain,
                                             const gchar         *message);
  void           (* help)                   (Gimp                *gimp,
                                             GimpProgress        *progress,
                                             const gchar         *help_domain,
                                             const gchar         *help_id);

  gboolean       (* wait)                   (Gimp                *gimp,
                                             GimpWaitable        *waitable,
                                             const gchar         *message);

  const gchar  * (* get_program_class)      (Gimp                *gimp);
  gchar        * (* get_display_name)       (Gimp                *gimp,
                                             gint                 display_id,
                                             GObject            **monitor,
                                             gint                *monitor_number);
  guint32        (* get_user_time)          (Gimp                *gimp);

  GFile        * (* get_theme_dir)          (Gimp                *gimp);
  GFile        * (* get_icon_theme_dir)     (Gimp                *gimp);

  GimpObject   * (* get_window_strategy)    (Gimp                *gimp);
  GimpDisplay  * (* get_empty_display)      (Gimp                *gimp);
  GBytes       * (* display_get_window_id)  (GimpDisplay         *display);
  GimpDisplay  * (* display_create)         (Gimp                *gimp,
                                             GimpImage           *image,
                                             GimpUnit            *unit,
                                             gdouble              scale,
                                             GObject             *monitor);
  void           (* display_delete)         (GimpDisplay         *display);
  void           (* displays_reconnect)     (Gimp                *gimp,
                                             GimpImage           *old_image,
                                             GimpImage           *new_image);

  GimpProgress * (* progress_new)           (Gimp                *gimp,
                                             GimpDisplay         *display);
  void           (* progress_free)          (Gimp                *gimp,
                                             GimpProgress        *progress);

  gboolean       (* pdb_dialog_new)         (Gimp                *gimp,
                                             GimpContext         *context,
                                             GimpProgress        *progress,
                                             GType                contents_type,
                                             GBytes              *parent_handle,
                                             const gchar         *title,
                                             const gchar         *callback_name,
                                             GimpObject          *object,
                                             va_list              args);
  gboolean       (* pdb_dialog_set)         (Gimp                *gimp,
                                             GType                contents_type,
                                             const gchar         *callback_name,
                                             GimpObject          *object,
                                             va_list              args);
  gboolean       (* pdb_dialog_close)       (Gimp                *gimp,
                                             GType                contents_type,
                                             const gchar         *callback_name);
  gboolean       (* recent_list_add_file)   (Gimp                *gimp,
                                             GFile               *file,
                                             const gchar         *mime_type);
  void           (* recent_list_load)       (Gimp                *gimp);

  GMountOperation
               * (* get_mount_operation)    (Gimp                *gimp,
                                             GimpProgress        *progress);

  GimpColorProfilePolicy
                 (* query_profile_policy)   (Gimp                *gimp,
                                             GimpImage           *image,
                                             GimpContext         *context,
                                             GimpColorProfile   **dest_profile,
                                             GimpColorRenderingIntent *intent,
                                             gboolean            *bpc,
                                             gboolean            *dont_ask);

  GimpMetadataRotationPolicy
                 (* query_rotation_policy)  (Gimp                *gimp,
                                             GimpImage           *image,
                                             GimpContext         *context,
                                             gboolean            *dont_ask);
};


void           gimp_gui_init               (Gimp                *gimp);

void           gimp_gui_ungrab             (Gimp                *gimp);

GimpObject   * gimp_get_window_strategy    (Gimp                *gimp);
GimpDisplay  * gimp_get_empty_display      (Gimp                *gimp);
GimpDisplay  * gimp_get_display_by_id      (Gimp                *gimp,
                                            gint                 ID);
gint           gimp_get_display_id         (Gimp                *gimp,
                                            GimpDisplay         *display);
GBytes       * gimp_get_display_window_id  (Gimp                *gimp,
                                            GimpDisplay         *display);
GimpDisplay  * gimp_create_display         (Gimp                *gimp,
                                            GimpImage           *image,
                                            GimpUnit            *unit,
                                            gdouble              scale,
                                            GObject             *monitor);
void           gimp_delete_display         (Gimp                *gimp,
                                            GimpDisplay         *display);
void           gimp_reconnect_displays     (Gimp                *gimp,
                                            GimpImage           *old_image,
                                            GimpImage           *new_image);

void           gimp_set_busy               (Gimp                *gimp);
void           gimp_set_busy_until_idle    (Gimp                *gimp);
void           gimp_unset_busy             (Gimp                *gimp);

void           gimp_show_message           (Gimp                *gimp,
                                            GObject             *handler,
                                            GimpMessageSeverity  severity,
                                            const gchar         *domain,
                                            const gchar         *message);
void           gimp_help                   (Gimp                *gimp,
                                            GimpProgress        *progress,
                                            const gchar         *help_domain,
                                            const gchar         *help_id);

void           gimp_wait                   (Gimp                *gimp,
                                            GimpWaitable        *waitable,
                                            const gchar         *format,
                                            ...) G_GNUC_PRINTF (3, 4);

GimpProgress * gimp_new_progress           (Gimp                *gimp,
                                            GimpDisplay         *display);
void           gimp_free_progress          (Gimp                *gimp,
                                            GimpProgress        *progress);

const gchar  * gimp_get_program_class      (Gimp                *gimp);
gchar        * gimp_get_display_name       (Gimp                *gimp,
                                            gint                 display_id,
                                            GObject            **monitor,
                                            gint                *monitor_number);
guint32        gimp_get_user_time          (Gimp                *gimp);
GFile        * gimp_get_theme_dir          (Gimp                *gimp);
GFile        * gimp_get_icon_theme_dir     (Gimp                *gimp);

gboolean       gimp_pdb_dialog_new         (Gimp                *gimp,
                                            GimpContext         *context,
                                            GimpProgress        *progress,
                                            GType                contents_type,
                                            GBytes              *parent_handle,
                                            const gchar         *title,
                                            const gchar         *callback_name,
                                            GimpObject          *object,
                                            ...) G_GNUC_NULL_TERMINATED;
gboolean       gimp_pdb_dialog_set         (Gimp                *gimp,
                                            GType                contents_type,
                                            const gchar         *callback_name,
                                            GimpObject          *object,
                                            ...) G_GNUC_NULL_TERMINATED;
gboolean       gimp_pdb_dialog_close       (Gimp                *gimp,
                                            GType                contents_type,
                                            const gchar         *callback_name);

gboolean       gimp_recent_list_add_file   (Gimp                *gimp,
                                            GFile               *file,
                                            const gchar         *mime_type);
void           gimp_recent_list_load       (Gimp                *gimp);

GMountOperation
             * gimp_get_mount_operation    (Gimp                *gimp,
                                            GimpProgress        *progress);

GimpColorProfilePolicy
               gimp_query_profile_policy   (Gimp                *gimp,
                                            GimpImage           *image,
                                            GimpContext         *context,
                                            GimpColorProfile   **dest_profile,
                                            GimpColorRenderingIntent *intent,
                                            gboolean            *bpc,
                                            gboolean            *dont_ask);

GimpMetadataRotationPolicy
               gimp_query_rotation_policy  (Gimp                *gimp,
                                            GimpImage           *image,
                                            GimpContext         *context,
                                            gboolean            *dont_ask);
