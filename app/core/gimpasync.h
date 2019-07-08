/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpasync.h
 * Copyright (C) 2018 Ell
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

#ifndef __GIMP_ASYNC_H__
#define __GIMP_ASYNC_H__


#define GIMP_TYPE_ASYNC            (gimp_async_get_type ())
#define GIMP_ASYNC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ASYNC, GimpAsync))
#define GIMP_ASYNC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ASYNC, GimpAsyncClass))
#define GIMP_IS_ASYNC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ASYNC))
#define GIMP_IS_ASYNC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ASYNC))
#define GIMP_ASYNC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ASYNC, GimpAsyncClass))


typedef void (* GimpAsyncCallback) (GimpAsync *async,
                                    gpointer   data);


typedef struct _GimpAsyncPrivate GimpAsyncPrivate;
typedef struct _GimpAsyncClass   GimpAsyncClass;

struct _GimpAsync
{
  GObject           parent_instance;

  GimpAsyncPrivate *priv;
};

struct _GimpAsyncClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void   (* waiting) (GimpAsync *async);
};


GType       gimp_async_get_type                (void) G_GNUC_CONST;

GimpAsync * gimp_async_new                     (void);

gboolean    gimp_async_is_synced               (GimpAsync         *async);

void        gimp_async_add_callback            (GimpAsync         *async,
                                                GimpAsyncCallback  callback,
                                                gpointer           data);
void        gimp_async_add_callback_for_object (GimpAsync         *async,
                                                GimpAsyncCallback  callback,
                                                gpointer           data,
                                                gpointer           gobject);
void        gimp_async_remove_callback         (GimpAsync         *async,
                                                GimpAsyncCallback  callback,
                                                gpointer           data);

gboolean    gimp_async_is_stopped              (GimpAsync         *async);

void        gimp_async_finish                  (GimpAsync         *async,
                                                gpointer           result);
void        gimp_async_finish_full             (GimpAsync         *async,
                                                gpointer           result,
                                                GDestroyNotify     result_destroy_func);
gboolean    gimp_async_is_finished             (GimpAsync         *async);
gpointer    gimp_async_get_result              (GimpAsync         *async);

void        gimp_async_abort                   (GimpAsync         *async);

gboolean    gimp_async_is_canceled             (GimpAsync         *async);

void        gimp_async_cancel_and_wait         (GimpAsync         *async);


/*  stats  */

gint        gimp_async_get_n_running           (void);


#endif /* __GIMP_ASYNC_H__ */
