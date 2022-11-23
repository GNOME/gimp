/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_GUI_H__
#define __LIGMA_GUI_H__


typedef struct _LigmaGui LigmaGui;

struct _LigmaGui
{
  void           (* ungrab)                 (Ligma                *ligma);

  void           (* set_busy)               (Ligma                *ligma);
  void           (* unset_busy)             (Ligma                *ligma);

  void           (* show_message)           (Ligma                *ligma,
                                             GObject             *handler,
                                             LigmaMessageSeverity  severity,
                                             const gchar         *domain,
                                             const gchar         *message);
  void           (* help)                   (Ligma                *ligma,
                                             LigmaProgress        *progress,
                                             const gchar         *help_domain,
                                             const gchar         *help_id);

  gboolean       (* wait)                   (Ligma                *ligma,
                                             LigmaWaitable        *waitable,
                                             const gchar         *message);

  const gchar  * (* get_program_class)      (Ligma                *ligma);
  gchar        * (* get_display_name)       (Ligma                *ligma,
                                             gint                 display_id,
                                             GObject            **monitor,
                                             gint                *monitor_number);
  guint32        (* get_user_time)          (Ligma                *ligma);

  GFile        * (* get_theme_dir)          (Ligma                *ligma);
  GFile        * (* get_icon_theme_dir)     (Ligma                *ligma);

  LigmaObject   * (* get_window_strategy)    (Ligma                *ligma);
  LigmaDisplay  * (* get_empty_display)      (Ligma                *ligma);
  guint32        (* display_get_window_id)  (LigmaDisplay         *display);
  LigmaDisplay  * (* display_create)         (Ligma                *ligma,
                                             LigmaImage           *image,
                                             LigmaUnit             unit,
                                             gdouble              scale,
                                             GObject             *monitor);
  void           (* display_delete)         (LigmaDisplay         *display);
  void           (* displays_reconnect)     (Ligma                *ligma,
                                             LigmaImage           *old_image,
                                             LigmaImage           *new_image);

  LigmaProgress * (* progress_new)           (Ligma                *ligma,
                                             LigmaDisplay         *display);
  void           (* progress_free)          (Ligma                *ligma,
                                             LigmaProgress        *progress);

  gboolean       (* pdb_dialog_new)         (Ligma                *ligma,
                                             LigmaContext         *context,
                                             LigmaProgress        *progress,
                                             LigmaContainer       *container,
                                             const gchar         *title,
                                             const gchar         *callback_name,
                                             const gchar         *object_name,
                                             va_list              args);
  gboolean       (* pdb_dialog_set)         (Ligma                *ligma,
                                             LigmaContainer       *container,
                                             const gchar         *callback_name,
                                             const gchar         *object_name,
                                             va_list              args);
  gboolean       (* pdb_dialog_close)       (Ligma                *ligma,
                                             LigmaContainer       *container,
                                             const gchar         *callback_name);
  gboolean       (* recent_list_add_file)   (Ligma                *ligma,
                                             GFile               *file,
                                             const gchar         *mime_type);
  void           (* recent_list_load)       (Ligma                *ligma);

  GMountOperation
               * (* get_mount_operation)    (Ligma                *ligma,
                                             LigmaProgress        *progress);

  LigmaColorProfilePolicy
                 (* query_profile_policy)   (Ligma                *ligma,
                                             LigmaImage           *image,
                                             LigmaContext         *context,
                                             LigmaColorProfile   **dest_profile,
                                             LigmaColorRenderingIntent *intent,
                                             gboolean            *bpc,
                                             gboolean            *dont_ask);

  LigmaMetadataRotationPolicy
                 (* query_rotation_policy)  (Ligma                *ligma,
                                             LigmaImage           *image,
                                             LigmaContext         *context,
                                             gboolean            *dont_ask);
};


void           ligma_gui_init               (Ligma                *ligma);

void           ligma_gui_ungrab             (Ligma                *ligma);

LigmaObject   * ligma_get_window_strategy    (Ligma                *ligma);
LigmaDisplay  * ligma_get_empty_display      (Ligma                *ligma);
LigmaDisplay  * ligma_get_display_by_id      (Ligma                *ligma,
                                            gint                 ID);
gint           ligma_get_display_id         (Ligma                *ligma,
                                            LigmaDisplay         *display);
guint32        ligma_get_display_window_id  (Ligma                *ligma,
                                            LigmaDisplay         *display);
LigmaDisplay  * ligma_create_display         (Ligma                *ligma,
                                            LigmaImage           *image,
                                            LigmaUnit             unit,
                                            gdouble              scale,
                                            GObject             *monitor);
void           ligma_delete_display         (Ligma                *ligma,
                                            LigmaDisplay         *display);
void           ligma_reconnect_displays     (Ligma                *ligma,
                                            LigmaImage           *old_image,
                                            LigmaImage           *new_image);

void           ligma_set_busy               (Ligma                *ligma);
void           ligma_set_busy_until_idle    (Ligma                *ligma);
void           ligma_unset_busy             (Ligma                *ligma);

void           ligma_show_message           (Ligma                *ligma,
                                            GObject             *handler,
                                            LigmaMessageSeverity  severity,
                                            const gchar         *domain,
                                            const gchar         *message);
void           ligma_help                   (Ligma                *ligma,
                                            LigmaProgress        *progress,
                                            const gchar         *help_domain,
                                            const gchar         *help_id);

void           ligma_wait                   (Ligma                *ligma,
                                            LigmaWaitable        *waitable,
                                            const gchar         *format,
                                            ...) G_GNUC_PRINTF (3, 4);

LigmaProgress * ligma_new_progress           (Ligma                *ligma,
                                            LigmaDisplay         *display);
void           ligma_free_progress          (Ligma                *ligma,
                                            LigmaProgress        *progress);

const gchar  * ligma_get_program_class      (Ligma                *ligma);
gchar        * ligma_get_display_name       (Ligma                *ligma,
                                            gint                 display_id,
                                            GObject            **monitor,
                                            gint                *monitor_number);
guint32        ligma_get_user_time          (Ligma                *ligma);
GFile        * ligma_get_theme_dir          (Ligma                *ligma);
GFile        * ligma_get_icon_theme_dir     (Ligma                *ligma);

gboolean       ligma_pdb_dialog_new         (Ligma                *ligma,
                                            LigmaContext         *context,
                                            LigmaProgress        *progress,
                                            LigmaContainer       *container,
                                            const gchar         *title,
                                            const gchar         *callback_name,
                                            const gchar         *object_name,
                                            ...) G_GNUC_NULL_TERMINATED;
gboolean       ligma_pdb_dialog_set         (Ligma                *ligma,
                                            LigmaContainer       *container,
                                            const gchar         *callback_name,
                                            const gchar         *object_name,
                                            ...) G_GNUC_NULL_TERMINATED;
gboolean       ligma_pdb_dialog_close       (Ligma                *ligma,
                                            LigmaContainer       *container,
                                            const gchar         *callback_name);
gboolean       ligma_recent_list_add_file   (Ligma                *ligma,
                                            GFile               *file,
                                            const gchar         *mime_type);
void           ligma_recent_list_load       (Ligma                *ligma);

GMountOperation
             * ligma_get_mount_operation    (Ligma                *ligma,
                                            LigmaProgress        *progress);

LigmaColorProfilePolicy
               ligma_query_profile_policy   (Ligma                *ligma,
                                            LigmaImage           *image,
                                            LigmaContext         *context,
                                            LigmaColorProfile   **dest_profile,
                                            LigmaColorRenderingIntent *intent,
                                            gboolean            *bpc,
                                            gboolean            *dont_ask);

LigmaMetadataRotationPolicy
               ligma_query_rotation_policy  (Ligma                *ligma,
                                            LigmaImage           *image,
                                            LigmaContext         *context,
                                            gboolean            *dont_ask);


#endif  /* __LIGMA_GUI_H__ */
