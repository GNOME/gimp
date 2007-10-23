/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprogress.h
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PROGRESS_H__
#define __GIMP_PROGRESS_H__


#define GIMP_TYPE_PROGRESS               (gimp_progress_interface_get_type ())
#define GIMP_IS_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROGRESS))
#define GIMP_PROGRESS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROGRESS, GimpProgress))
#define GIMP_PROGRESS_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_PROGRESS, GimpProgressInterface))


typedef struct _GimpProgressInterface GimpProgressInterface;

struct _GimpProgressInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  GimpProgress * (* start)      (GimpProgress        *progress,
                                 const gchar         *message,
                                 gboolean             cancelable);
  void           (* end)        (GimpProgress        *progress);
  gboolean       (* is_active)  (GimpProgress        *progress);

  void           (* set_text)   (GimpProgress        *progress,
                                 const gchar         *message);
  void           (* set_value)  (GimpProgress        *progress,
                                 gdouble              percentage);
  gdouble        (* get_value)  (GimpProgress        *progress);
  void           (* pulse)      (GimpProgress        *progress);

  guint32        (* get_window) (GimpProgress        *progress);

  gboolean       (* message)    (GimpProgress        *progress,
                                 Gimp                *gimp,
                                 GimpMessageSeverity  severity,
                                 const gchar         *domain,
                                 const gchar         *message);

  /*  signals  */
  void           (* cancel)     (GimpProgress        *progress);
};


GType          gimp_progress_interface_get_type (void) G_GNUC_CONST;

GimpProgress * gimp_progress_start              (GimpProgress        *progress,
                                                 const gchar         *message,
                                                 gboolean             cancelable);
void           gimp_progress_end                (GimpProgress        *progress);
gboolean       gimp_progress_is_active          (GimpProgress        *progress);

void           gimp_progress_set_text           (GimpProgress        *progress,
                                                 const gchar         *message);
void           gimp_progress_set_value          (GimpProgress        *progress,
                                                 gdouble              percentage);
gdouble        gimp_progress_get_value          (GimpProgress        *progress);
void           gimp_progress_pulse              (GimpProgress        *progress);

guint32        gimp_progress_get_window         (GimpProgress        *progress);

gboolean       gimp_progress_message            (GimpProgress        *progress,
                                                 Gimp                *gimp,
                                                 GimpMessageSeverity  severity,
                                                 const gchar         *domain,
                                                 const gchar         *message);

void           gimp_progress_cancel             (GimpProgress        *progress);

void           gimp_progress_update_and_flush   (gint                 min,
                                                 gint                 max,
                                                 gint                 current,
                                                 gpointer             data);


#endif /* __GIMP_PROGRESS_H__ */
