/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcriticaldialog.h
 * Copyright (C) 2018  Jehan <jehan@gimp.org>
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

#ifndef __GIMP_CRITICAL_DIALOG_H__
#define __GIMP_CRITICAL_DIALOG_H__

G_BEGIN_DECLS


#define GIMP_TYPE_CRITICAL_DIALOG            (gimp_critical_dialog_get_type ())
#define GIMP_CRITICAL_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CRITICAL_DIALOG, GimpCriticalDialog))
#define GIMP_CRITICAL_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CRITICAL_DIALOG, GimpCriticalDialogClass))
#define GIMP_IS_CRITICAL_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CRITICAL_DIALOG))
#define GIMP_IS_CRITICAL_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CRITICAL_DIALOG))
#define GIMP_CRITICAL_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CRITICAL_DIALOG, GimpCriticalDialogClass))


typedef struct _GimpCriticalDialog       GimpCriticalDialog;
typedef struct _GimpCriticalDialogClass  GimpCriticalDialogClass;

struct _GimpCriticalDialog
{
  GtkDialog        parent_instance;

  GtkWidget       *main_vbox;
  GtkWidget       *top_label;
  GtkWidget       *center_label;
  GtkWidget       *bottom_label;
  GtkWidget       *details;

  gchar           *program;
  gint             pid;

  gchar           *last_version;
  gchar           *release_date;
};

struct _GimpCriticalDialogClass
{
  GtkDialogClass   parent_class;
};


GType       gimp_critical_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_critical_dialog_new      (const gchar        *title,
                                           const gchar        *last_version,
                                           gint64              release_timestamp);
void        gimp_critical_dialog_add      (GtkWidget          *dialog,
                                           const gchar        *message,
                                           const gchar        *trace,
                                           gboolean            is_fatal,
                                           const gchar        *program,
                                           gint                pid);


G_END_DECLS

#endif /* __GIMP_CRITICAL_DIALOG_H__ */
