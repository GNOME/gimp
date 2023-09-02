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

#ifndef _IMAP_OBJECT_H
#define _IMAP_OBJECT_H

typedef struct Object_t Object_t;
typedef struct ObjectClass_t ObjectClass_t;
typedef struct ObjectList_t ObjectList_t;

#include "imap_edit_area_info.h"
#include "imap_menu_funcs.h"

struct Object_t {
  ObjectClass_t        *class;
  ObjectList_t         *list;
  gint                  refcount;
  gboolean              selected;
  gboolean              locked;
  gchar                *url;
  gchar                *target;
  gchar                *accesskey;
  gchar                *tabindex;
  gchar                *comment;
  gchar                *mouse_over;
  gchar                *mouse_out;
  gchar                *focus;
  gchar                *blur;
  gchar                *click;
};

typedef void (*MoveSashFunc_t)(Object_t*, gint, gint);
typedef void (*OutputFunc_t)(gpointer, const char*, ...) G_GNUC_PRINTF(2,3);

struct AreaInfoDialog_t;

struct ObjectClass_t {
   const gchar          *name;
   AreaInfoDialog_t     *info_dialog;

   gboolean (*is_valid)(Object_t *obj);
   void (*destruct)(Object_t *obj);
   Object_t* (*clone)(Object_t *obj);
   void (*assign)(Object_t *obj, Object_t *des);
   void (*normalize)(Object_t *obj);
   void (*draw)(Object_t *obj, cairo_t *cr);
   void (*draw_sashes)(Object_t *obj, cairo_t *cr);
   MoveSashFunc_t (*near_sash)(Object_t *obj, gint x, gint y);
   gboolean (*point_is_on)(Object_t *obj, gint x, gint y);
   void (*get_dimensions)(Object_t *obj, gint *x, gint *y, gint *width,
                          gint *height);
   void (*resize)(Object_t *obj, gint percentage_x, gint percentage_y);
   void (*move)(Object_t *obj, gint dx, gint dy);
   gpointer (*create_info_widget)(GtkWidget *frame);
   void (*update_info_widget)(Object_t *obj, gpointer data);
   void (*fill_info_tab)(Object_t *obj, gpointer data);
   void (*set_initial_focus)(Object_t *obj, gpointer data);
   void (*update)(Object_t *obj, gpointer data);
   void (*write_csim)(Object_t *obj, gpointer param, OutputFunc_t output);
   void (*write_cern)(Object_t *obj, gpointer param, OutputFunc_t output);
   void (*write_ncsa)(Object_t *obj, gpointer param, OutputFunc_t output);
   void (*do_popup)(Object_t *obj,
                    GdkEventButton *event,
                    gpointer        data);

   const gchar* (*get_icon_name)(void);
};

Object_t *object_ref(Object_t *obj);
void object_unref(Object_t *obj);
Object_t* object_init(Object_t *obj, ObjectClass_t *class);
Object_t* object_clone(Object_t *obj);
Object_t* object_assign(Object_t *src, Object_t *des);

void object_draw                 (Object_t        *obj,
                                  cairo_t         *cr);
void object_edit                 (Object_t        *obj,
                                  gboolean         add);
void object_select               (Object_t        *obj);
void object_unselect             (Object_t        *obj);
void object_move                 (Object_t        *obj,
                                  gint             dx,
                                  gint             dy);
void object_move_sash            (Object_t        *obj,
                                  gint             dx,
                                  gint             dy);
void object_remove               (Object_t        *obj);
void object_lock                 (Object_t        *obj);
void object_unlock               (Object_t        *obj);
void object_set_url              (Object_t        *obj,
                                  const gchar     *url);
void object_set_target           (Object_t        *obj,
                                  const gchar     *target);
void object_set_comment          (Object_t        *obj,
                                  const gchar     *comment);
void object_set_accesskey        (Object_t        *obj,
                                  const gchar     *accesskey);
void object_set_tabindex         (Object_t        *obj,
                                  const gchar     *tabindex);
void object_set_mouse_over       (Object_t        *obj,
                                 const gchar      *mouse_over);
void object_set_mouse_out        (Object_t        *obj,
                                  const gchar     *mouse_out);
void object_set_focus            (Object_t        *obj,
                                  const gchar     *focus);
void object_set_blur             (Object_t        *obj,
                                  const gchar     *blur);
void object_set_click            (Object_t        *obj,
                                  const gchar     *click);
gint object_get_position_in_list (Object_t        *obj);

void object_emit_changed_signal(Object_t *obj);
void object_emit_geometry_signal(Object_t *obj);
void object_emit_update_signal(Object_t *obj);

#define object_is_valid(obj) \
        ((obj)->class->is_valid(obj))

#define object_get_dimensions(obj, x, y, width, height) \
        ((obj)->class->get_dimensions((obj), (x), (y), (width), (height)))

#define object_normalize(obj) \
        ((obj)->class->normalize(obj))

#define object_resize(obj, per_x, per_y) \
        ((obj)->class->resize((obj), (per_x), (per_y)))

#define object_update(obj, data) \
        ((obj)->class->update((obj), (data)))

#define object_update_info_widget(obj, data) \
        ((obj)->class->update_info_widget((obj), (data)))

#define object_fill_info_tab(obj, data) \
        ((obj)->class->fill_info_tab((obj), (data)))

#define object_get_icon_name(obj) \
        ((obj)->class->get_icon_name())

typedef struct {
   Object_t *obj;
   gboolean (*finish)(Object_t *obj, gint x, gint y);
   gboolean (*cancel)(GdkEventButton *event, Object_t *obj);
   Object_t* (*create_object)(gint x, gint y);
   void (*set_xy)(Object_t *obj, guint state, gint x, gint y);
} ObjectFactory_t;

gboolean object_on_button_press(GtkWidget *widget, GdkEventButton *event,
                                gpointer data);

typedef struct {
   GList *list;
} ObjectListCallback_t;

struct ObjectList_t {
   GList *list;
   gboolean changed;
   ObjectListCallback_t changed_cb;
   ObjectListCallback_t update_cb;
   ObjectListCallback_t add_cb;
   ObjectListCallback_t remove_cb;
   ObjectListCallback_t select_cb;
   ObjectListCallback_t move_cb;
   ObjectListCallback_t geometry_cb;
};

ObjectList_t *make_object_list (void);
void object_list_destruct(ObjectList_t *list);
ObjectList_t *object_list_copy(ObjectList_t *des, ObjectList_t *src);
ObjectList_t *object_list_append_list(ObjectList_t *des, ObjectList_t *src);

void object_list_append(ObjectList_t *list, Object_t *object);
void object_list_prepend(ObjectList_t *list, Object_t *object);
void object_list_insert(ObjectList_t *list, gint position, Object_t *object);
void object_list_remove(ObjectList_t *list, Object_t *object);
void object_list_remove_link(ObjectList_t *list, GList *link);
void object_list_update(ObjectList_t *list, Object_t *object);
void object_list_draw(ObjectList_t *list, cairo_t *cr);
void object_list_draw_selected(ObjectList_t *list, cairo_t *cr);
Object_t *object_list_find(ObjectList_t *list, gint x, gint y);
Object_t *object_list_near_sash(ObjectList_t *list, gint x, gint y,
                                MoveSashFunc_t *sash_func);

gint object_list_cut(ObjectList_t *list);
void object_list_copy_to_paste_buffer(ObjectList_t *list);
void object_list_paste(ObjectList_t *list);

void object_list_remove_all(ObjectList_t *list);
void object_list_delete_selected(ObjectList_t *list);
void object_list_edit_selected(ObjectList_t *list);
gint object_list_select_all(ObjectList_t *list);
void object_list_select_next(ObjectList_t *list);
void object_list_select_prev(ObjectList_t *list);
gint object_list_select_region(ObjectList_t *list, gint x, gint y, gint width,
                               gint height);
gint object_list_deselect_all(ObjectList_t *list, Object_t *exception);
gint object_list_nr_selected(ObjectList_t *list);
void object_list_resize(ObjectList_t *list, gint percentage_x,
                        gint percentage_y);
void object_list_move_selected(ObjectList_t *list, gint dx, gint dy);
void object_list_move_up(ObjectList_t *list, Object_t *obj);
void object_list_move_down(ObjectList_t *list, Object_t *obj);
void object_list_move_selected_up(ObjectList_t *list);
void object_list_move_selected_down(ObjectList_t *list);
void object_list_move_to_front(ObjectList_t *list);
void object_list_send_to_back(ObjectList_t *list);
void object_list_move_sash_selected(ObjectList_t *list, gint dx, gint dy);

void object_list_write_csim(ObjectList_t *list, gpointer param,
                            OutputFunc_t output);
void object_list_write_cern(ObjectList_t *list, gpointer param,
                            OutputFunc_t output);
void object_list_write_ncsa(ObjectList_t *list, gpointer param,
                            OutputFunc_t output);

typedef void (*ObjectListCallbackFunc_t)(Object_t*, gpointer);

gpointer object_list_add_changed_cb(ObjectList_t *list,
                                    ObjectListCallbackFunc_t func,
                                    gpointer data);
gpointer object_list_add_update_cb(ObjectList_t *list,
                                   ObjectListCallbackFunc_t func,
                                   gpointer data);
gpointer object_list_add_add_cb(ObjectList_t *list,
                                ObjectListCallbackFunc_t func, gpointer data);
gpointer object_list_add_remove_cb(ObjectList_t *list,
                                   ObjectListCallbackFunc_t func,
                                   gpointer data);
gpointer object_list_add_select_cb(ObjectList_t *list,
                                   ObjectListCallbackFunc_t func,
                                   gpointer data);
gpointer object_list_add_move_cb(ObjectList_t *list,
                                 ObjectListCallbackFunc_t func, gpointer data);
gpointer object_list_add_geometry_cb(ObjectList_t *list,
                                     ObjectListCallbackFunc_t func,
                                     gpointer data);

void object_list_remove_add_cb(ObjectList_t *list, gpointer id);
void object_list_remove_select_cb(ObjectList_t *list, gpointer id);
void object_list_remove_remove_cb(ObjectList_t *list, gpointer id);
void object_list_remove_move_cb(ObjectList_t *list, gpointer id);
void object_list_remove_geometry_cb(ObjectList_t *list, gpointer id);

#define object_list_clear_changed(list) ((list)->changed = FALSE)
#define object_list_set_changed(list, ischanged) \
        ((list)->changed = (ischanged))
#define object_list_get_changed(list) ((list)->changed)

void clear_paste_buffer(void);
gpointer paste_buffer_add_add_cb(ObjectListCallbackFunc_t func, gpointer data);
gpointer paste_buffer_add_remove_cb(ObjectListCallbackFunc_t func,
                                    gpointer data);
ObjectList_t *get_paste_buffer(void);

void do_object_locked_dialog(void);

#endif /* _IMAP_OBJECT_H */


