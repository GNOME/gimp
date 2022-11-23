/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaasync.h
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

#ifndef __LIGMA_ASYNC_H__
#define __LIGMA_ASYNC_H__


#define LIGMA_TYPE_ASYNC            (ligma_async_get_type ())
#define LIGMA_ASYNC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ASYNC, LigmaAsync))
#define LIGMA_ASYNC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ASYNC, LigmaAsyncClass))
#define LIGMA_IS_ASYNC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ASYNC))
#define LIGMA_IS_ASYNC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ASYNC))
#define LIGMA_ASYNC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ASYNC, LigmaAsyncClass))


typedef void (* LigmaAsyncCallback) (LigmaAsync *async,
                                    gpointer   data);


typedef struct _LigmaAsyncPrivate LigmaAsyncPrivate;
typedef struct _LigmaAsyncClass   LigmaAsyncClass;

struct _LigmaAsync
{
  GObject           parent_instance;

  LigmaAsyncPrivate *priv;
};

struct _LigmaAsyncClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void   (* waiting) (LigmaAsync *async);
};


GType       ligma_async_get_type                (void) G_GNUC_CONST;

LigmaAsync * ligma_async_new                     (void);

gboolean    ligma_async_is_synced               (LigmaAsync         *async);

void        ligma_async_add_callback            (LigmaAsync         *async,
                                                LigmaAsyncCallback  callback,
                                                gpointer           data);
void        ligma_async_add_callback_for_object (LigmaAsync         *async,
                                                LigmaAsyncCallback  callback,
                                                gpointer           data,
                                                gpointer           gobject);
void        ligma_async_remove_callback         (LigmaAsync         *async,
                                                LigmaAsyncCallback  callback,
                                                gpointer           data);

gboolean    ligma_async_is_stopped              (LigmaAsync         *async);

void        ligma_async_finish                  (LigmaAsync         *async,
                                                gpointer           result);
void        ligma_async_finish_full             (LigmaAsync         *async,
                                                gpointer           result,
                                                GDestroyNotify     result_destroy_func);
gboolean    ligma_async_is_finished             (LigmaAsync         *async);
gpointer    ligma_async_get_result              (LigmaAsync         *async);

void        ligma_async_abort                   (LigmaAsync         *async);

gboolean    ligma_async_is_canceled             (LigmaAsync         *async);

void        ligma_async_cancel_and_wait         (LigmaAsync         *async);


/*  stats  */

gint        ligma_async_get_n_running           (void);


#endif /* __LIGMA_ASYNC_H__ */
