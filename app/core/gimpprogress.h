/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprogress.h
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PROGRESS_H__
#define __GIMP_PROGRESS_H__


#define GIMP_TYPE_PROGRESS (gimp_progress_get_type ())
G_DECLARE_INTERFACE (GimpProgress, gimp_progress, GIMP, PROGRESS, GObject)


struct _GimpProgressInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  GimpProgress * (* start)         (GimpProgress        *progress,
                                    gboolean             cancellable,
                                    const gchar         *message);
  void           (* end)           (GimpProgress        *progress);
  gboolean       (* is_active)     (GimpProgress        *progress);

  void           (* set_text)      (GimpProgress        *progress,
                                    const gchar         *message);
  void           (* set_value)     (GimpProgress        *progress,
                                    gdouble              percentage);
  gdouble        (* get_value)     (GimpProgress        *progress);
  void           (* pulse)         (GimpProgress        *progress);

  GBytes       * (* get_window_id) (GimpProgress        *progress);

  gboolean       (* message)       (GimpProgress        *progress,
                                    Gimp                *gimp,
                                    GimpMessageSeverity  severity,
                                    const gchar         *domain,
                                    const gchar         *message);

  /*  signals  */
  void           (* cancel)        (GimpProgress        *progress);
};


GimpProgress * gimp_progress_start            (GimpProgress        *progress,
                                               gboolean             cancellable,
                                               const gchar         *format,
                                               ...) G_GNUC_PRINTF (3, 4);
void           gimp_progress_end              (GimpProgress        *progress);
gboolean       gimp_progress_is_active        (GimpProgress        *progress);

void           gimp_progress_set_text         (GimpProgress        *progress,
                                               const gchar         *format,
                                               ...) G_GNUC_PRINTF (2, 3);
void           gimp_progress_set_text_literal (GimpProgress        *progress,
                                               const gchar         *message);
void           gimp_progress_set_value        (GimpProgress        *progress,
                                               gdouble              percentage);
gdouble        gimp_progress_get_value        (GimpProgress        *progress);
void           gimp_progress_pulse            (GimpProgress        *progress);

GBytes       * gimp_progress_get_window_id    (GimpProgress        *progress);

gboolean       gimp_progress_message          (GimpProgress        *progress,
                                               Gimp                *gimp,
                                               GimpMessageSeverity  severity,
                                               const gchar         *domain,
                                               const gchar         *message);

void           gimp_progress_cancel           (GimpProgress        *progress);

void           gimp_progress_update_and_flush (gint                 min,
                                               gint                 max,
                                               gint                 current,
                                               gpointer             data);


#endif /* __GIMP_PROGRESS_H__ */
