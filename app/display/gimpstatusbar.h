/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_STATUSBAR_H__
#define __GIMP_STATUSBAR_H__

G_BEGIN_DECLS


/*  maximal length of the format string for the cursor-coordinates  */
#define CURSOR_FORMAT_LENGTH 32


#define GIMP_TYPE_STATUSBAR            (gimp_statusbar_get_type ())
#define GIMP_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_STATUSBAR, GimpStatusbar))
#define GIMP_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_STATUSBAR, GimpStatusbarClass))
#define GIMP_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_STATUSBAR))
#define GIMP_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_STATUSBAR))
#define GIMP_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_STATUSBAR, GimpStatusbarClass))

typedef struct _GimpStatusbarClass GimpStatusbarClass;

struct _GimpStatusbar
{
  GtkFrame             parent_instance;

  GimpDisplayShell    *shell;

  GSList              *messages;
  GHashTable          *context_ids;
  guint                seq_context_id;

  GdkPixbuf           *icon;
  GHashTable          *icon_hash;

  guint                temp_context_id;
  guint                temp_timeout_id;
  GimpMessageSeverity  temp_severity;

  gchar                cursor_format_str[CURSOR_FORMAT_LENGTH];
  gchar                cursor_format_str_f[CURSOR_FORMAT_LENGTH];
  gchar                length_format_str[CURSOR_FORMAT_LENGTH];

  GtkWidget           *cursor_label;
  GtkWidget           *unit_combo;
  GtkWidget           *scale_combo;
  GtkWidget           *rotate_label;
  GtkWidget           *horizontal_flip_icon;
  GtkWidget           *vertical_flip_icon;
  GtkWidget           *label; /* same as GtkStatusbar->label */

  GtkWidget           *progressbar;
  GtkWidget           *cancel_button;
  gboolean             progress_active;
  gboolean             progress_shown;
  gdouble              progress_value;
  guint64              progress_last_update_time;
};

struct _GimpStatusbarClass
{
  GtkFrameClass parent_class;
};


GType       gimp_statusbar_get_type              (void) G_GNUC_CONST;
GtkWidget * gimp_statusbar_new                   (void);

void        gimp_statusbar_set_shell             (GimpStatusbar       *statusbar,
                                                  GimpDisplayShell    *shell);

gboolean    gimp_statusbar_get_visible           (GimpStatusbar       *statusbar);
void        gimp_statusbar_set_visible           (GimpStatusbar       *statusbar,
                                                  gboolean             visible);
void        gimp_statusbar_empty                 (GimpStatusbar       *statusbar);
void        gimp_statusbar_fill                  (GimpStatusbar       *statusbar);

void        gimp_statusbar_override_window_title (GimpStatusbar       *statusbar);
void        gimp_statusbar_restore_window_title  (GimpStatusbar       *statusbar);

void        gimp_statusbar_push                  (GimpStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  ...) G_GNUC_PRINTF (4, 5);
void        gimp_statusbar_push_valist           (GimpStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  va_list              args) G_GNUC_PRINTF (4, 0);
void        gimp_statusbar_push_coords           (GimpStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  GimpCursorPrecision  precision,
                                                  const gchar         *title,
                                                  gdouble              x,
                                                  const gchar         *separator,
                                                  gdouble              y,
                                                  const gchar         *help);
void        gimp_statusbar_push_length           (GimpStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *title,
                                                  GimpOrientationType  axis,
                                                  gdouble              value,
                                                  const gchar         *help);
void        gimp_statusbar_replace               (GimpStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  ...) G_GNUC_PRINTF (4, 5);
void        gimp_statusbar_replace_valist        (GimpStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  va_list              args) G_GNUC_PRINTF (4, 0);
const gchar * gimp_statusbar_peek                (GimpStatusbar       *statusbar,
                                                  const gchar         *context);
void        gimp_statusbar_pop                   (GimpStatusbar       *statusbar,
                                                  const gchar         *context);

void        gimp_statusbar_push_temp             (GimpStatusbar       *statusbar,
                                                  GimpMessageSeverity  severity,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  ...) G_GNUC_PRINTF (4, 5);
void        gimp_statusbar_push_temp_valist      (GimpStatusbar       *statusbar,
                                                  GimpMessageSeverity  severity,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  va_list              args) G_GNUC_PRINTF (4, 0);
void        gimp_statusbar_pop_temp              (GimpStatusbar       *statusbar);

void        gimp_statusbar_update_cursor         (GimpStatusbar       *statusbar,
                                                  GimpCursorPrecision  precision,
                                                  gdouble              x,
                                                  gdouble              y);
void        gimp_statusbar_clear_cursor          (GimpStatusbar       *statusbar);


G_END_DECLS

#endif /* __GIMP_STATUSBAR_H__ */
