/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaprogress.h
 * Copyright (C) 2004  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PROGRESS_H__
#define __LIGMA_PROGRESS_H__


#define LIGMA_TYPE_PROGRESS (ligma_progress_get_type ())
G_DECLARE_INTERFACE (LigmaProgress, ligma_progress, LIGMA, PROGRESS, GObject)


struct _LigmaProgressInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  LigmaProgress * (* start)         (LigmaProgress        *progress,
                                    gboolean             cancellable,
                                    const gchar         *message);
  void           (* end)           (LigmaProgress        *progress);
  gboolean       (* is_active)     (LigmaProgress        *progress);

  void           (* set_text)      (LigmaProgress        *progress,
                                    const gchar         *message);
  void           (* set_value)     (LigmaProgress        *progress,
                                    gdouble              percentage);
  gdouble        (* get_value)     (LigmaProgress        *progress);
  void           (* pulse)         (LigmaProgress        *progress);

  guint32        (* get_window_id) (LigmaProgress        *progress);

  gboolean       (* message)       (LigmaProgress        *progress,
                                    Ligma                *ligma,
                                    LigmaMessageSeverity  severity,
                                    const gchar         *domain,
                                    const gchar         *message);

  /*  signals  */
  void           (* cancel)        (LigmaProgress        *progress);
};


LigmaProgress * ligma_progress_start            (LigmaProgress        *progress,
                                               gboolean             cancellable,
                                               const gchar         *format,
                                               ...) G_GNUC_PRINTF (3, 4);
void           ligma_progress_end              (LigmaProgress        *progress);
gboolean       ligma_progress_is_active        (LigmaProgress        *progress);

void           ligma_progress_set_text         (LigmaProgress        *progress,
                                               const gchar         *format,
                                               ...) G_GNUC_PRINTF (2, 3);
void           ligma_progress_set_text_literal (LigmaProgress        *progress,
                                               const gchar         *message);
void           ligma_progress_set_value        (LigmaProgress        *progress,
                                               gdouble              percentage);
gdouble        ligma_progress_get_value        (LigmaProgress        *progress);
void           ligma_progress_pulse            (LigmaProgress        *progress);

guint32        ligma_progress_get_window_id    (LigmaProgress        *progress);

gboolean       ligma_progress_message          (LigmaProgress        *progress,
                                               Ligma                *ligma,
                                               LigmaMessageSeverity  severity,
                                               const gchar         *domain,
                                               const gchar         *message);

void           ligma_progress_cancel           (LigmaProgress        *progress);

void           ligma_progress_update_and_flush (gint                 min,
                                               gint                 max,
                                               gint                 current,
                                               gpointer             data);


#endif /* __LIGMA_PROGRESS_H__ */
