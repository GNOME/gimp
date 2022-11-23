/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatriviallycancelablewaitable.h
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

#ifndef __LIGMA_TRIVIALLY_CANCELABLE_WAITABLE_H__
#define __LIGMA_TRIVIALLY_CANCELABLE_WAITABLE_H__


#include "ligmauncancelablewaitable.h"


#define LIGMA_TYPE_TRIVIALLY_CANCELABLE_WAITABLE            (ligma_trivially_cancelable_waitable_get_type ())
#define LIGMA_TRIVIALLY_CANCELABLE_WAITABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TRIVIALLY_CANCELABLE_WAITABLE, LigmaTriviallyCancelableWaitable))
#define LIGMA_TRIVIALLY_CANCELABLE_WAITABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TRIVIALLY_CANCELABLE_WAITABLE, LigmaTriviallyCancelableWaitableClass))
#define LIGMA_IS_TRIVIALLY_CANCELABLE_WAITABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TRIVIALLY_CANCELABLE_WAITABLE))
#define LIGMA_IS_TRIVIALLY_CANCELABLE_WAITABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TRIVIALLY_CANCELABLE_WAITABLE))
#define LIGMA_TRIVIALLY_CANCELABLE_WAITABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TRIVIALLY_CANCELABLE_WAITABLE, LigmaTriviallyCancelableWaitableClass))


typedef struct _LigmaTriviallyCancelableWaitablePrivate LigmaTriviallyCancelableWaitablePrivate;
typedef struct _LigmaTriviallyCancelableWaitableClass   LigmaTriviallyCancelableWaitableClass;

struct _LigmaTriviallyCancelableWaitable
{
  LigmaUncancelableWaitable                parent_instance;

  LigmaTriviallyCancelableWaitablePrivate *priv;
};

struct _LigmaTriviallyCancelableWaitableClass
{
  LigmaUncancelableWaitableClass  parent_class;
};


GType          ligma_trivially_cancelable_waitable_get_type (void) G_GNUC_CONST;

LigmaWaitable * ligma_trivially_cancelable_waitable_new      (LigmaWaitable *waitable);


#endif /* __LIGMA_TRIVIALLY_CANCELABLE_WAITABLE_H__ */
