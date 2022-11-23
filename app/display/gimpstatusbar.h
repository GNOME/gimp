/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_STATUSBAR_H__
#define __LIGMA_STATUSBAR_H__

G_BEGIN_DECLS


/*  maximal length of the format string for the cursor-coordinates  */
#define CURSOR_FORMAT_LENGTH 32


#define LIGMA_TYPE_STATUSBAR            (ligma_statusbar_get_type ())
#define LIGMA_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_STATUSBAR, LigmaStatusbar))
#define LIGMA_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_STATUSBAR, LigmaStatusbarClass))
#define LIGMA_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_STATUSBAR))
#define LIGMA_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_STATUSBAR))
#define LIGMA_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_STATUSBAR, LigmaStatusbarClass))

typedef struct _LigmaStatusbarClass LigmaStatusbarClass;

struct _LigmaStatusbar
{
  GtkFrame             parent_instance;

  Ligma                *ligma;
  LigmaDisplayShell    *shell;
  LigmaImage           *image;

  GSList              *messages;
  GHashTable          *context_ids;
  guint                seq_context_id;

  guint                statusbar_pos_redraw_idle_id;
  gchar               *cursor_string_todraw;
  gchar               *cursor_string_last;

  GdkPixbuf           *icon;
  GHashTable          *icon_hash;
  gint                 icon_space_width;

  guint                temp_context_id;
  guint                temp_timeout_id;
  LigmaMessageSeverity  temp_severity;

  gchar                cursor_format_str[CURSOR_FORMAT_LENGTH];
  gchar                cursor_format_str_f[CURSOR_FORMAT_LENGTH];
  gchar                length_format_str[CURSOR_FORMAT_LENGTH];
  LigmaCursorPrecision  cursor_precision;
  gint                 cursor_w_digits;
  gint                 cursor_h_digits;


  GtkWidget           *cursor_label;
  GtkWidget           *unit_combo;
  GtkWidget           *scale_combo;
  GtkWidget           *rotate_widget;
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

  GtkWidget           *soft_proof_button;
  GtkWidget           *soft_proof_container;
  GtkWidget           *soft_proof_popover;
  GtkWidget           *proof_colors_toggle;
  GtkWidget           *profile_label;
  GtkWidget           *profile_combo;
  GtkWidget           *rendering_intent_combo;
  GtkWidget           *bpc_toggle;
  GtkWidget           *optimize_combo;
  GtkWidget           *out_of_gamut_toggle;

  GSList              *size_widgets;
};

struct _LigmaStatusbarClass
{
  GtkFrameClass parent_class;
};


GType       ligma_statusbar_get_type              (void) G_GNUC_CONST;
GtkWidget * ligma_statusbar_new                   (void);

void        ligma_statusbar_set_shell             (LigmaStatusbar       *statusbar,
                                                  LigmaDisplayShell    *shell);

gboolean    ligma_statusbar_get_visible           (LigmaStatusbar       *statusbar);
void        ligma_statusbar_set_visible           (LigmaStatusbar       *statusbar,
                                                  gboolean             visible);
void        ligma_statusbar_empty                 (LigmaStatusbar       *statusbar);
void        ligma_statusbar_fill                  (LigmaStatusbar       *statusbar);

void        ligma_statusbar_override_window_title (LigmaStatusbar       *statusbar);
void        ligma_statusbar_restore_window_title  (LigmaStatusbar       *statusbar);

void        ligma_statusbar_push                  (LigmaStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  ...) G_GNUC_PRINTF (4, 5);
void        ligma_statusbar_push_valist           (LigmaStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  va_list              args) G_GNUC_PRINTF (4, 0);
void        ligma_statusbar_push_coords           (LigmaStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  LigmaCursorPrecision  precision,
                                                  const gchar         *title,
                                                  gdouble              x,
                                                  const gchar         *separator,
                                                  gdouble              y,
                                                  const gchar         *help);
void        ligma_statusbar_push_length           (LigmaStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *title,
                                                  LigmaOrientationType  axis,
                                                  gdouble              value,
                                                  const gchar         *help);
void        ligma_statusbar_replace               (LigmaStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  ...) G_GNUC_PRINTF (4, 5);
void        ligma_statusbar_replace_valist        (LigmaStatusbar       *statusbar,
                                                  const gchar         *context,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  va_list              args) G_GNUC_PRINTF (4, 0);
const gchar * ligma_statusbar_peek                (LigmaStatusbar       *statusbar,
                                                  const gchar         *context);
void        ligma_statusbar_pop                   (LigmaStatusbar       *statusbar,
                                                  const gchar         *context);

void        ligma_statusbar_push_temp             (LigmaStatusbar       *statusbar,
                                                  LigmaMessageSeverity  severity,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  ...) G_GNUC_PRINTF (4, 5);
void        ligma_statusbar_push_temp_valist      (LigmaStatusbar       *statusbar,
                                                  LigmaMessageSeverity  severity,
                                                  const gchar         *icon_name,
                                                  const gchar         *format,
                                                  va_list              args) G_GNUC_PRINTF (4, 0);
void        ligma_statusbar_pop_temp              (LigmaStatusbar       *statusbar);

void        ligma_statusbar_update_cursor         (LigmaStatusbar       *statusbar,
                                                  LigmaCursorPrecision  precision,
                                                  gdouble              x,
                                                  gdouble              y);
void        ligma_statusbar_clear_cursor          (LigmaStatusbar       *statusbar);


G_END_DECLS

#endif /* __LIGMA_STATUSBAR_H__ */
