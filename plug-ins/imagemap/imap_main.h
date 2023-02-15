/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#ifndef _IMAP_MAIN_H
#define _IMAP_MAIN_H

#include "imap_mru.h"
#include "imap_grid.h"
#include "imap_object.h"
#include "imap_preferences.h"
#include "imap_preview.h"

struct _GimpImap
{
  GimpPlugIn      parent_instance;
  GtkApplication *app;

  GtkWidget      *dlg;
  GtkWidget      *grid_toggle;

  gpointer        grid_data;
  gchar          *tmp_filename;
  GimpDrawable   *drawable;
  gboolean        success;

  GtkBuilder     *builder;
};

#define PLUG_IN_PROC   "plug-in-imagemap"
#define PLUG_IN_BINARY "imagemap"
#define PLUG_IN_ROLE   "gimp-imagemap"

#define GIMP_TYPE_IMAP  (gimp_imap_get_type ())
G_DECLARE_FINAL_TYPE (GimpImap, gimp_imap, GIMP, IMAP, GimpPlugIn)

typedef enum {NCSA, CERN, CSIM} MapFormat_t;

typedef struct {
   MapFormat_t map_format;
   gchar   *image_name;
   gchar   *title;
   gchar   *author;
   gchar   *default_url;
   gchar   *description;
   gint     old_image_width;
   gint     old_image_height;
   gboolean color;              /* Color (TRUE) or Gray (FALSE) */
   gboolean show_gray;
} MapInfo_t;

void main_set_dimension (gint width, gint height);
void main_clear_dimension (void);
void load (const gchar *filename,
           gpointer     data);
void save_as (const gchar *filename);
void dump_output (gpointer param, OutputFunc_t output);
GtkWidget *get_dialog (void);
MRU_t *get_mru (void);
MapInfo_t *get_map_info (void);
PreferencesData_t *get_preferences (void);

gint get_image_width  (void);
gint get_image_height (void);

void set_busy_cursor    (void);
void remove_busy_cursor (void);

void main_toolbar_set_grid (gboolean active);

void set_zoom       (gint zoom_factor);
gint get_real_coord (gint coord);
void draw_line(cairo_t *cr, gint x1, gint y1, gint x2,
               gint y2);
void draw_rectangle(cairo_t *cr, gboolean filled, gint x, gint y,
                    gint width, gint height);
void draw_circle(cairo_t *cr, gint x, gint y,
                 gint r);
void draw_polygon(cairo_t *cr, GList *list);

const char *get_filename(void);

ObjectList_t *get_shapes(void);
void add_shape(Object_t *obj);
void update_shape(Object_t *obj);
void select_shape(GtkWidget *widget, GdkEventButton *event);
void edit_shape(gint x, gint y);

void do_popup_menu (GdkEventButton *event,
                    gpointer        data);
void draw_shapes(cairo_t *cr);

void show_url(void);
void hide_url(void);

void            set_preview_color          (GSimpleAction  *action,
                                            GVariant       *new_state,
                                            gpointer        user_data);
void            set_zoom_factor            (GSimpleAction  *action,
                                            GVariant       *new_state,
                                            gpointer        user_data);
void            set_func                   (GSimpleAction  *action,
                                            GVariant       *new_state,
                                            gpointer        user_data);
void            do_edit_selected_shape     (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_zoom_in                 (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_zoom_out                (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_close                   (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_quit                    (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_undo                    (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_redo                    (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_cut                     (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_copy                    (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_paste                   (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_select_all              (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_deselect_all            (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_clear                   (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_move_up                 (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_move_down               (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_move_to_front           (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_send_to_back            (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_use_gimp_guides_dialog  (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            do_create_guides_dialog    (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            save                       (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            imap_help                  (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data);
void            toggle_area_list           (GSimpleAction *action,
                                            GVariant      *new_state,
                                            gpointer       user_data);
const gchar *   get_image_name             (void);

GtkWidget *     add_tool_button            (GtkWidget     *toolbar,
                                            const char    *action,
                                            const char    *icon,
                                            const char    *label,
                                            const char    *tooltip);
GtkWidget *     add_toggle_button          (GtkWidget     *toolbar,
                                            const char    *action,
                                            const char    *icon,
                                            const char    *label,
                                            const char    *tooltip);
void            add_tool_separator         (GtkWidget     *toolbar,
                                            gboolean       expand);

#endif /* _IMAP_MAIN_H */
