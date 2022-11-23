/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmauncancelablewaitable.h
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

#ifndef __LIGMA_UNCANCELABLE_WAITABLE_H__
#define __LIGMA_UNCANCELABLE_WAITABLE_H__


#define LIGMA_TYPE_UNCANCELABLE_WAITABLE            (ligma_uncancelable_waitable_get_type ())
#define LIGMA_UNCANCELABLE_WAITABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_UNCANCELABLE_WAITABLE, LigmaUncancelableWaitable))
#define LIGMA_UNCANCELABLE_WAITABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_UNCANCELABLE_WAITABLE, LigmaUncancelableWaitableClass))
#define LIGMA_IS_UNCANCELABLE_WAITABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_UNCANCELABLE_WAITABLE))
#define LIGMA_IS_UNCANCELABLE_WAITABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_UNCANCELABLE_WAITABLE))
#define LIGMA_UNCANCELABLE_WAITABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_UNCANCELABLE_WAITABLE, LigmaUncancelableWaitableClass))


typedef struct _LigmaUncancelableWaitablePrivate LigmaUncancelableWaitablePrivate;
typedef struct _LigmaUncancelableWaitableClass   LigmaUncancelableWaitableClass;

struct _LigmaUncancelableWaitable
{
  GObject       parent_instance;

  LigmaWaitable *waitable;
};

struct _LigmaUncancelableWaitableClass
{
  GObjectClass  parent_class;
};


GType          ligma_uncancelable_waitable_get_type (void) G_GNUC_CONST;

LigmaWaitable * ligma_uncancelable_waitable_new      (LigmaWaitable *waitable);


#endif /* __LIGMA_UNCANCELABLE_WAITABLE_H__ */
