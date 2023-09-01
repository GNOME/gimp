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

#include "config.h"

#include <gtk/gtk.h>

#include "imap_commands.h"
#include "imap_default_dialog.h"
#include "imap_grid.h"
#include "imap_main.h"
#include "imap_object.h"
#include "imap_string.h"

typedef struct {
   ObjectListCallbackFunc_t func;
   gpointer data;
} ObjectListCB_t;

static ObjectList_t *_paste_buffer;

static gpointer
object_list_callback_add(ObjectListCallback_t *list,
                         ObjectListCallbackFunc_t func, gpointer data)
{
   ObjectListCB_t *cb = g_new(ObjectListCB_t, 1);
   cb->func = func;
   cb->data = data;
   list->list = g_list_append(list->list, cb);
   return cb;
}

static void
object_list_callback_remove(ObjectListCallback_t *list, gpointer id)
{
   list->list = g_list_remove(list->list, id);
}

static void
object_list_callback_call(ObjectListCallback_t *list, Object_t *obj)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      ObjectListCB_t *cb = (ObjectListCB_t*) p->data;
      cb->func(obj, cb->data);
   }
}

gpointer
object_list_add_changed_cb(ObjectList_t *list, ObjectListCallbackFunc_t func,
                           gpointer data)
{
   return object_list_callback_add(&list->changed_cb, func, data);
}

gpointer
object_list_add_update_cb(ObjectList_t *list, ObjectListCallbackFunc_t func,
                          gpointer data)
{
   return object_list_callback_add(&list->update_cb, func, data);
}

gpointer
object_list_add_add_cb(ObjectList_t *list, ObjectListCallbackFunc_t func,
                       gpointer data)
{
   return object_list_callback_add(&list->add_cb, func, data);
}

gpointer
object_list_add_remove_cb(ObjectList_t *list, ObjectListCallbackFunc_t func,
                          gpointer data)
{
   return object_list_callback_add(&list->remove_cb, func, data);
}

gpointer
object_list_add_select_cb(ObjectList_t *list, ObjectListCallbackFunc_t func,
                          gpointer data)
{
   return object_list_callback_add(&list->select_cb, func, data);
}

gpointer
object_list_add_move_cb(ObjectList_t *list, ObjectListCallbackFunc_t func,
                        gpointer data)
{
   return object_list_callback_add(&list->move_cb, func, data);
}

gpointer
object_list_add_geometry_cb(ObjectList_t *list, ObjectListCallbackFunc_t func,
                            gpointer data)
{
   return object_list_callback_add(&list->geometry_cb, func, data);
}

gpointer
paste_buffer_add_add_cb(ObjectListCallbackFunc_t func, gpointer data)
{
   if (!_paste_buffer)
      _paste_buffer = make_object_list();
   return object_list_callback_add(&_paste_buffer->add_cb, func, data);
}

gpointer
paste_buffer_add_remove_cb(ObjectListCallbackFunc_t func, gpointer data)
{
   if (!_paste_buffer)
      _paste_buffer = make_object_list();
   return object_list_callback_add(&_paste_buffer->remove_cb, func, data);
}

void
object_list_remove_add_cb(ObjectList_t *list, gpointer id)
{
   object_list_callback_remove(&list->add_cb, id);
}

void
object_list_remove_select_cb(ObjectList_t *list, gpointer id)
{
   object_list_callback_remove(&list->select_cb, id);
}

void
object_list_remove_remove_cb(ObjectList_t *list, gpointer id)
{
   object_list_callback_remove(&list->remove_cb, id);
}

void
object_list_remove_move_cb(ObjectList_t *list, gpointer id)
{
   object_list_callback_remove(&list->move_cb, id);
}

void
object_list_remove_geometry_cb(ObjectList_t *list, gpointer id)
{
   object_list_callback_remove(&list->geometry_cb, id);
}

Object_t*
object_init(Object_t *obj, ObjectClass_t *class)
{
  obj->class = class;
  obj->refcount = 1;
  obj->selected = FALSE;
  obj->locked = FALSE;
  obj->url = g_strdup ("");
  obj->target = g_strdup ("");
  obj->comment = g_strdup ("");
  obj->accesskey = g_strdup ("");
  obj->tabindex = g_strdup ("");
  obj->mouse_over = g_strdup ("");
  obj->mouse_out = g_strdup ("");
  obj->focus = g_strdup ("");
  obj->blur = g_strdup ("");
  obj->click = g_strdup ("");

  return obj;
}

static void
object_destruct(Object_t *obj)
{
  if (obj->class->destruct)
    obj->class->destruct(obj);

  g_free (obj->url);
  g_free (obj->target);
  g_free (obj->comment);
  g_free (obj->accesskey);
  g_free (obj->tabindex);
  g_free (obj->mouse_over);
  g_free (obj->mouse_out);
  g_free (obj->focus);
  g_free (obj->blur);
  g_free (obj->click);
  g_free (obj);
}

Object_t*
object_ref(Object_t *obj)
{
   obj->refcount++;
   return obj;
}

void
object_unref(Object_t *obj)
{
   if (!--obj->refcount)
      object_destruct(obj);
}

Object_t*
object_clone (Object_t *obj)
{
  Object_t *clone = obj->class->clone (obj);

  clone->class = obj->class;
  clone->refcount = 1;
  clone->selected = obj->selected;
  clone->locked = FALSE;
  clone->url = g_strdup (obj->url);
  clone->target = g_strdup (obj->target);
  clone->comment = g_strdup (obj->comment);
  clone->accesskey = g_strdup (obj->accesskey);
  clone->tabindex = g_strdup (obj->tabindex);
  clone->mouse_over = g_strdup (obj->mouse_over);
  clone->mouse_out = g_strdup (obj->mouse_out);
  clone->focus = g_strdup (obj->focus);
  clone->blur = g_strdup (obj->blur);
  clone->click = g_strdup (obj->click);

  return clone;
}

static Object_t*
object_copy (Object_t *src, Object_t *des)
{
  des->class = src->class;
  des->selected = src->selected;
  des->locked = FALSE;

  g_strreplace (&des->url, src->url);
  g_strreplace (&des->target, src->target);
  g_strreplace (&des->comment, src->comment);
  g_strreplace (&des->accesskey, src->accesskey);
  g_strreplace (&des->tabindex, src->tabindex);
  g_strreplace (&des->mouse_over, src->mouse_over);
  g_strreplace (&des->mouse_out, src->mouse_out);
  g_strreplace (&des->focus, src->focus);
  g_strreplace (&des->blur, src->blur);
  g_strreplace (&des->click, src->click);

  return des;
}

Object_t*
object_assign(Object_t *obj, Object_t *des)
{
   obj->class->assign(obj, des);
   return object_copy(obj, des);
}

void
object_draw(Object_t *obj, cairo_t *cr)
{
   PreferencesData_t *preferences = get_preferences();
   ColorSelData_t *colors = &preferences->colors;
   GdkRGBA *fg, *bg;
   gdouble dash = 4.;

   if (obj->selected & 4) {
      fg = &colors->interactive_fg;
      bg = &colors->interactive_bg;
      obj->selected &= ~4;
   } else if (obj->selected) {
      fg = &colors->selected_fg;
      bg = &colors->selected_bg;
   } else {
      fg = &colors->normal_fg;
      bg = &colors->normal_bg;
   }

   cairo_save (cr);
   gdk_cairo_set_source_rgba (cr, bg);
   obj->class->draw(obj, cr);
   gdk_cairo_set_source_rgba (cr, fg);
   cairo_set_dash (cr, &dash, 1, 0.);
   obj->class->draw(obj, cr);

   if (obj->selected && preferences->show_area_handle)
      obj->class->draw_sashes(obj, cr);
   cairo_restore (cr);
}

void
object_edit(Object_t *obj, gboolean add)
{
   if (!obj->class->info_dialog)
      obj->class->info_dialog = create_edit_area_info_dialog(obj);
   edit_area_info_dialog_show(obj->class->info_dialog, obj, add);
}

void
object_select(Object_t *obj)
{
   obj->selected = TRUE;
   object_list_callback_call(&obj->list->select_cb, obj);
   object_emit_geometry_signal(obj);
}

void
object_unselect(Object_t *obj)
{
   obj->selected = FALSE;
   object_list_callback_call(&obj->list->select_cb, obj);
   object_emit_geometry_signal(obj);
}

void
object_move(Object_t *obj, gint dx, gint dy)
{
   obj->class->move(obj, dx, dy);
   object_emit_geometry_signal(obj);
}

void
object_move_sash(Object_t *obj, gint dx, gint dy)
{
   gint x, y, width, height;
   MoveSashFunc_t sash_func;

   obj->class->get_dimensions(obj, &x, &y, &width, &height);
   if (dx == 0)
      x += (width / 2);
   else
      x += width;

   if (dy == 0)
      y += (height / 2);
   else
      y += height;

   sash_func = obj->class->near_sash(obj, x, y);

   if (sash_func) {
      sash_func(obj, dx, dy);
      object_emit_geometry_signal(obj);
   }
}

void
object_remove(Object_t *obj)
{
   object_list_remove(obj->list, obj);
   object_emit_geometry_signal(obj);
}

void
object_lock(Object_t *obj)
{
   obj->locked = TRUE;
}

void
object_unlock(Object_t *obj)
{
   obj->locked = FALSE;
}

void
object_set_url(Object_t *obj, const gchar *url)
{
   g_strreplace (&obj->url, url);
}

void
object_set_target(Object_t *obj, const gchar *target)
{
   g_strreplace (&obj->target, target);
}

void
object_set_comment(Object_t *obj, const gchar *comment)
{
   g_strreplace (&obj->comment, comment);
}

void
object_set_accesskey (Object_t    *obj,
                      const gchar *accesskey)
{
  g_strreplace (&obj->accesskey, accesskey);
}

void
object_set_tabindex (Object_t    *obj,
                     const gchar *tabindex)
{
  g_strreplace (&obj->tabindex, tabindex);
}

void
object_set_mouse_over(Object_t *obj, const gchar *mouse_over)
{
   g_strreplace (&obj->mouse_over, mouse_over);
}

void
object_set_mouse_out(Object_t *obj, const gchar *mouse_out)
{
   g_strreplace (&obj->mouse_out, mouse_out);
}

void
object_set_focus(Object_t *obj, const gchar *focus)
{
   g_strreplace (&obj->focus, focus);
}

void
object_set_blur (Object_t    *obj,
                 const gchar *blur)
{
  g_strreplace (&obj->blur, blur);
}

void
object_set_click (Object_t    *obj,
                  const gchar *click)
{
  g_strreplace (&obj->click, click);
}

gint
object_get_position_in_list(Object_t *obj)
{
   return g_list_index(obj->list->list, (gpointer) obj);
}

void
object_emit_changed_signal(Object_t *obj)
{
   object_list_callback_call(&obj->list->changed_cb, obj);
}

void
object_emit_geometry_signal(Object_t *obj)
{
   object_list_callback_call(&obj->list->geometry_cb, obj);
}

void
object_emit_update_signal(Object_t *obj)
{
   object_list_callback_call(&obj->list->update_cb, obj);
}

void
do_object_locked_dialog(void)
{
   static DefaultDialog_t *dialog;
   if (!dialog) {
      dialog = make_default_dialog("Object locked");
      default_dialog_hide_cancel_button(dialog);
      default_dialog_hide_apply_button(dialog);
      default_dialog_set_label(
         dialog,
         "\n  You cannot delete the selected object  \n"
         "since it is currently being edited.\n");
   }
   default_dialog_show(dialog);
}

static Object_t*
object_factory_create_object(ObjectFactory_t *factory, gint x, gint y)
{
   return factory->obj = factory->create_object(x, y);
}

static gboolean
button_motion(GtkWidget *widget, GdkEventMotion *event,
              ObjectFactory_t *factory)
{
   gint x = get_real_coord((gint) event->x);
   gint y = get_real_coord((gint) event->y);

   round_to_grid(&x, &y);

   factory->set_xy(factory->obj, event->state, x, y);

   preview_redraw ();

   return FALSE;
}

gboolean
object_on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   static ObjectFactory_t *factory;
   PreferencesData_t *preferences = get_preferences();
   gint x = get_real_coord((gint) event->x);
   gint y = get_real_coord((gint) event->y);
   static Object_t *obj;

   if (event->type == GDK_2BUTTON_PRESS)
      return FALSE;
   round_to_grid(&x, &y);

   if (obj) {
      if (event->button == 1) {
         if (!factory->finish || factory->finish(obj, x, y)) {
            g_signal_handlers_disconnect_by_func(widget,
                                                 button_motion,
                                                 factory);
            if (object_is_valid(obj)) {
               Command_t *command = create_command_new(get_shapes(), obj);
               command_execute(command);
               if (preferences->prompt_for_area_info)
                  object_edit(obj, FALSE);
            } else {
               object_unref(obj);
            }
            preview_unset_tmp_obj (obj);
            preview_redraw ();
            obj = NULL;
            main_clear_dimension();
         }
      } else if (event->button == 3) {
         if (!factory->cancel || factory->cancel(event, obj)) {
            g_signal_handlers_disconnect_by_func(widget,
                                                 button_motion,
                                                 factory);
            object_unref(obj);
            preview_unset_tmp_obj (obj);
            preview_redraw ();
            obj = NULL;
            main_clear_dimension();
         }
         return TRUE;
      }
   } else {
      if (event->button == 1) {
         factory = ((ObjectFactory_t*(*)(guint)) data)(event->state);
         obj = object_factory_create_object(factory, x, y);
         preview_set_tmp_obj (obj);

         g_signal_connect(widget, "motion-notify-event",
                          G_CALLBACK(button_motion), factory);
      }
   }
   return FALSE;
}

ObjectList_t*
make_object_list(void)
{
  return g_new0 (ObjectList_t, 1);
}

void
object_list_destruct(ObjectList_t *list)
{
   object_list_remove_all(list);
   g_free(list->list);
}

ObjectList_t*
object_list_append_list(ObjectList_t *des, ObjectList_t *src)
{
   GList *p;
   if (!src)
     return des;
   for (p = src->list; p; p = p->next)
      object_list_append(des, object_clone((Object_t*) p->data));
   object_list_set_changed(des, TRUE);
   return des;
}

ObjectList_t*
object_list_copy(ObjectList_t *des, ObjectList_t *src)
{
   if (des)
      object_list_remove_all(des);
   else
      des = make_object_list();

   return object_list_append_list(des, src);
}

void
object_list_append(ObjectList_t *list, Object_t *object)
{
   object->list = list;
   list->list = g_list_append(list->list, (gpointer) object);
   object_list_set_changed(list, TRUE);
   object_list_callback_call(&list->add_cb, object);
}

void
object_list_prepend(ObjectList_t *list, Object_t *object)
{
   object->list = list;
   list->list = g_list_prepend(list->list, (gpointer) object);
   object_list_set_changed(list, TRUE);
   object_list_callback_call(&list->add_cb, object);
}

void
object_list_insert(ObjectList_t *list, gint position, Object_t *object)
{
   object->list = list;
   list->list = g_list_insert(list->list, (gpointer) object, position);
   object_list_set_changed(list, TRUE);
   object_list_callback_call(&list->add_cb, object);
}

void
object_list_remove(ObjectList_t *list, Object_t *object)
{
   list->list = g_list_remove(list->list, (gpointer) object);
   object_list_set_changed(list, TRUE);
   object_list_callback_call(&list->remove_cb, object);
   object_unref(object);
}

void
object_list_remove_link(ObjectList_t *list, GList *link)
{
   list->list = g_list_remove_link(list->list, link);
   object_list_set_changed(list, TRUE);
   object_list_callback_call(&list->remove_cb, (Object_t*) link->data);
}

void
object_list_update(ObjectList_t *list, Object_t *object)
{
   object_list_callback_call(&list->update_cb, object);
}

void
object_list_draw(ObjectList_t *list, cairo_t *cr)
{
   GList *p;
   for (p = list->list; p; p = p->next)
      object_draw((Object_t*) p->data, cr);
}

void
object_list_draw_selected(ObjectList_t *list, cairo_t *cr)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected)
         object_draw(obj, cr);
   }
}

Object_t*
object_list_find(ObjectList_t *list, gint x, gint y)
{
   Object_t *found = NULL;
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->class->point_is_on(obj, x, y))
         found = obj;
   }
   return found;
}

Object_t*
object_list_near_sash(ObjectList_t *list, gint x, gint y,
                      MoveSashFunc_t *sash_func)
{
   Object_t *found = NULL;
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected) {
         MoveSashFunc_t func = obj->class->near_sash(obj, x, y);
         if (func) {
            found = obj;
            *sash_func = func;
         }
      }
   }
   return found;
}

void
object_list_remove_all(ObjectList_t *list)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      object_list_callback_call(&list->remove_cb, obj);
      object_unref(obj);
   }
   g_list_free(list->list);
   list->list = NULL;
   object_list_set_changed(list, TRUE);
}

void
clear_paste_buffer(void)
{
   if (_paste_buffer)
      object_list_remove_all(_paste_buffer);
   else
      _paste_buffer = make_object_list();
}

ObjectList_t*
get_paste_buffer(void)
{
   return _paste_buffer;
}

gint
object_list_cut(ObjectList_t *list)
{
   GList *p, *q;
   gint count = 0;

   clear_paste_buffer();
   for (p = list->list; p; p = q) {
      Object_t *obj = (Object_t*) p->data;
      q = p->next;
      if (obj->selected) {
         if (obj->locked) {
            do_object_locked_dialog();
         } else {
            object_list_append(_paste_buffer, obj);
            object_list_remove_link(list, p);
            count++;
         }
      }
   }
   object_list_set_changed(list, (count) ? TRUE : FALSE);
   return count;
}

void
object_list_copy_to_paste_buffer(ObjectList_t *list)
{
   GList *p;

   clear_paste_buffer();
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected)
         object_list_append(_paste_buffer, object_clone(obj));
   }
}

void
object_list_paste(ObjectList_t *list)
{
   object_list_append_list(list, _paste_buffer);
}

void
object_list_delete_selected(ObjectList_t *list)
{
   GList *p, *q;
   for (p = list->list; p; p = q) {
      Object_t *obj = (Object_t*) p->data;
      q = p->next;
      if (obj->selected) {
         if (obj->locked) {
            do_object_locked_dialog();
         } else {
            object_list_remove_link(list, p);
            object_unref(obj);
         }
      }
   }
}

void
object_list_edit_selected(ObjectList_t *list)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected) {
         object_edit(obj, TRUE);
         break;
      }
   }
}

gint
object_list_select_all(ObjectList_t *list)
{
   GList *p;
   gint count = 0;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (!obj->selected) {
         object_select(obj);
         count++;
      }
   }
   return count;
}

void
object_list_select_next(ObjectList_t *list)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected) {
         object_unselect(obj);
         p = (p->next) ? p->next : list->list;
         object_select((Object_t*) p->data);
         for (p = p->next; p; p = p->next) {
            obj = (Object_t*) p->data;
            if (obj->selected)
               object_unselect(obj);
         }
         break;
      }
   }
}

void object_list_select_prev(ObjectList_t *list)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected) {
         GList *q = (p->prev) ? p->prev : g_list_last(list->list);
         for (; p; p = p->next) {
            obj = (Object_t*) p->data;
            if (obj->selected)
               object_unselect(obj);
         }
         object_select((Object_t*) q->data);
         break;
      }
   }
}

gint
object_list_select_region(ObjectList_t *list, gint x, gint y, gint width,
                          gint height)
{
   GList *p;
   gint count = 0;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      gint obj_x, obj_y, obj_width, obj_height;

      object_get_dimensions(obj, &obj_x, &obj_y, &obj_width, &obj_height);
      if (obj_x >= x && obj_x + obj_width <= x + width &&
          obj_y >= y && obj_y + obj_height <= y + height) {
         object_select(obj);
         count++;
      }
   }
   return count;
}

gint
object_list_deselect_all(ObjectList_t *list, Object_t *exception)
{
   GList *p;
   gint count = 0;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected && obj != exception) {
         object_unselect(obj);
         count++;
      }
   }
   return count;
}

gint
object_list_nr_selected(ObjectList_t *list)
{
   GList *p;
   gint count = 0;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected)
         count++;
   }
   return count;
}

void
object_list_resize(ObjectList_t *list, gint percentage_x, gint percentage_y)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      object_resize(obj, percentage_x, percentage_y);
   }
}

static void
object_list_swap_prev(ObjectList_t *list, GList *p)
{
   gpointer swap = p->data;
   p->data = p->prev->data;
   p->prev->data = swap;
   object_list_callback_call(&list->move_cb, (Object_t*) p->data);
   object_list_callback_call(&list->move_cb, (Object_t*) p->prev->data);
}

static void
object_list_swap_next(ObjectList_t *list, GList *p)
{
   gpointer swap = p->data;
   p->data = p->next->data;
   p->next->data = swap;
   object_list_callback_call(&list->move_cb, (Object_t*) p->data);
   object_list_callback_call(&list->move_cb, (Object_t*) p->next->data);
}

void
object_list_move_selected(ObjectList_t *list, gint dx, gint dy)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected)
         object_move(obj, dx, dy);
   }
}

void
object_list_move_up(ObjectList_t *list, Object_t *obj)
{
   GList *p = g_list_find(list->list, (gpointer) obj);
   object_list_swap_prev(list, p);
}

void
object_list_move_down(ObjectList_t *list, Object_t *obj)
{
   GList *p = g_list_find(list->list, (gpointer) obj);
   object_list_swap_next(list, p);
}

void
object_list_move_selected_up(ObjectList_t *list)
{
   GList *p;

   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected && p->prev)
         object_list_swap_prev(list, p);
   }
}

void
object_list_move_selected_down(ObjectList_t *list)
{
   GList *p;

   for (p = g_list_last(list->list); p; p = p->prev) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected && p->next)
         object_list_swap_next(list, p);
   }
}

void
object_list_move_to_front(ObjectList_t *list)
{
   GList *p, *q;
   guint length = g_list_length(list->list);

   for (p = list->list; length; p = q, length--) {
      Object_t *obj = (Object_t*) p->data;
      q = p->next;
      if (obj->selected) {
         object_list_remove_link(list, p);
         object_list_append(list, obj);
      }
   }
}

void
object_list_send_to_back(ObjectList_t *list)
{
   GList *p, *q;
   guint length = g_list_length(list->list);

   for (p = list->list; length; p = q, length--) {
      Object_t *obj = (Object_t*) p->data;
      q = p->next;
      if (obj->selected) {
         object_list_remove_link(list, p);
         object_list_prepend(list, obj);
      }
   }
}

void
object_list_move_sash_selected(ObjectList_t *list, gint dx, gint dy)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      if (obj->selected)
         object_move_sash(obj, dx, dy);
   }
}

static void
write_xml_attrib(const gchar *attrib, const gchar *value,
                 const gchar *default_text, gpointer param,
                 OutputFunc_t output)
{
   if (*value) {
      gchar *escaped_value = g_markup_escape_text(value, -1);
      output(param, " %s=\"%s\"", attrib, escaped_value);
      g_free(escaped_value);
   } else if (*default_text) {
      output(param, " %s", default_text);
   }
}

void
object_list_write_csim (ObjectList_t *list,
                        gpointer      param,
                        OutputFunc_t  output)
{
  GList *p;
  for (p = list->list; p; p = p->next)
    {
      Object_t *obj = (Object_t*) p->data;

      output (param, "<area shape=");
      obj->class->write_csim (obj, param, output);

      write_xml_attrib ("alt", obj->comment, "", param, output);
      write_xml_attrib ("target", obj->target, "", param, output);
      write_xml_attrib ("accesskey", obj->accesskey, "", param, output);
      write_xml_attrib ("tabindex", obj->tabindex, "", param, output);

      write_xml_attrib ("onmouseover", obj->mouse_over, "", param, output);
      write_xml_attrib ("onmouseout", obj->mouse_out, "", param, output);
      write_xml_attrib ("onfocus", obj->focus, "", param, output);
      write_xml_attrib ("onblur", obj->blur, "", param, output);
      write_xml_attrib ("onclick", obj->click, "", param, output);
      write_xml_attrib ("href", obj->url, " nohref=\"nohref\"", param, output);
      output (param," />\n");
    }
}

void
object_list_write_cern(ObjectList_t *list, gpointer param, OutputFunc_t output)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;
      obj->class->write_cern(obj, param, output);
      output(param, " %s\n", obj->url);
   }
}

void
object_list_write_ncsa(ObjectList_t *list, gpointer param, OutputFunc_t output)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Object_t *obj = (Object_t*) p->data;

      if (*obj->comment)
         output(param, "# %s\n", obj->comment);
      obj->class->write_ncsa(obj, param, output);
      output(param, "\n");
   }
}
