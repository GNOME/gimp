/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_GUI_H__
#define __GIMP_GUI_H__


typedef struct _GimpGui GimpGui;

struct _GimpGui
{
  void           (* ungrab)              (Gimp                *gimp);

  void           (* threads_enter)       (Gimp                *gimp);
  void           (* threads_leave)       (Gimp                *gimp);

  void           (* set_busy)            (Gimp                *gimp);
  void           (* unset_busy)          (Gimp                *gimp);

  void           (* show_message)        (Gimp                *gimp,
                                          GObject             *handler,
                                          GimpMessageSeverity  severity,
                                          const gchar         *domain,
                                          const gchar         *message);
  void           (* help)                (Gimp                *gimp,
                                          const gchar         *help_domain,
                                          const gchar         *help_id);

  const gchar  * (* get_program_class)   (Gimp                *gimp);
  gchar        * (* get_display_name)    (Gimp                *gimp,
                                          gint                 display_ID,
                                          gint                *monitor_number);
  const gchar  * (* get_theme_dir)       (Gimp                *gimp);

  GimpObject   * (* display_get_by_id)   (Gimp                *gimp,
                                          gint                 ID);
  gint           (* display_get_id)      (GimpObject          *display);
  guint32        (* display_get_window)  (GimpObject          *display);
  GimpObject   * (* display_create)      (GimpImage           *image,
                                          GimpUnit             unit,
                                          gdouble              scale);
  void           (* display_delete)      (GimpObject          *display);
  void           (* displays_reconnect)  (Gimp                *gimp,
                                          GimpImage           *old_image,
                                          GimpImage           *new_image);

  GimpProgress * (* progress_new)        (Gimp                *gimp,
                                          GimpObject          *display);
  void           (* progress_free)       (Gimp                *gimp,
                                          GimpProgress        *progress);

  gboolean       (* pdb_dialog_new)      (Gimp                *gimp,
                                          GimpContext         *context,
                                          GimpProgress        *progress,
                                          GimpContainer       *container,
                                          const gchar         *title,
                                          const gchar         *callback_name,
                                          const gchar         *object_name,
                                          va_list              args);
  gboolean       (* pdb_dialog_set)      (Gimp                *gimp,
                                          GimpContainer       *container,
                                          const gchar         *callback_name,
                                          const gchar         *object_name,
                                          va_list              args);
  gboolean       (* pdb_dialog_close)    (Gimp                *gimp,
                                          GimpContainer       *container,
                                          const gchar         *callback_name);
  gboolean       (* recent_list_add_uri) (Gimp                *gimp,
                                          const gchar         *uri,
                                          const gchar         *mime_type);

};


void           gimp_gui_init             (Gimp                *gimp);

void           gimp_gui_ungrab           (Gimp                *gimp);

void           gimp_threads_enter        (Gimp                *gimp);
void           gimp_threads_leave        (Gimp                *gimp);

GimpObject   * gimp_get_display_by_ID    (Gimp                *gimp,
                                          gint                 ID);
gint           gimp_get_display_ID       (Gimp                *gimp,
                                          GimpObject          *display);
guint32        gimp_get_display_window   (Gimp                *gimp,
                                          GimpObject          *display);
GimpObject   * gimp_create_display       (Gimp                *gimp,
                                          GimpImage           *image,
                                          GimpUnit             unit,
                                          gdouble              scale);
void           gimp_delete_display       (Gimp                *gimp,
                                          GimpObject          *display);
void           gimp_reconnect_displays   (Gimp                *gimp,
                                          GimpImage           *old_image,
                                          GimpImage           *new_image);

void           gimp_set_busy             (Gimp                *gimp);
void           gimp_set_busy_until_idle  (Gimp                *gimp);
void           gimp_unset_busy           (Gimp                *gimp);

void           gimp_show_message         (Gimp                *gimp,
                                          GObject             *handler,
                                          GimpMessageSeverity  severity,
                                          const gchar         *domain,
                                          const gchar         *message);
void           gimp_help                 (Gimp                *gimp,
                                          const gchar         *help_domain,
                                          const gchar         *help_id);

GimpProgress * gimp_new_progress         (Gimp                *gimp,
                                          GimpObject          *display);
void           gimp_free_progress        (Gimp                *gimp,
                                          GimpProgress        *progress);

const gchar  * gimp_get_program_class    (Gimp                *gimp);
gchar        * gimp_get_display_name     (Gimp                *gimp,
                                          gint                 display_ID,
                                          gint                *monitor_number);
const gchar  * gimp_get_theme_dir        (Gimp                *gimp);

gboolean       gimp_pdb_dialog_new       (Gimp                *gimp,
                                          GimpContext         *context,
                                          GimpProgress        *progress,
                                          GimpContainer       *container,
                                          const gchar         *title,
                                          const gchar         *callback_name,
                                          const gchar         *object_name,
                                          ...) G_GNUC_NULL_TERMINATED;
gboolean       gimp_pdb_dialog_set       (Gimp                *gimp,
                                          GimpContainer       *container,
                                          const gchar         *callback_name,
                                          const gchar         *object_name,
                                          ...) G_GNUC_NULL_TERMINATED;
gboolean       gimp_pdb_dialog_close     (Gimp                *gimp,
                                          GimpContainer       *container,
                                          const gchar         *callback_name);
gboolean       gimp_recent_list_add_uri  (Gimp                *gimp,
                                          const gchar         *uri,
                                          const gchar         *mime_type);

#endif  /* __GIMP_GUI_H__ */
