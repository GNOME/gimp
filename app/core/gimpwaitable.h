/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmawaitable.h
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

#ifndef __LIGMA_WAITABLE_H__
#define __LIGMA_WAITABLE_H__


#define LIGMA_TYPE_WAITABLE (ligma_waitable_get_type ())
G_DECLARE_INTERFACE (LigmaWaitable, ligma_waitable, LIGMA, WAITABLE, GObject)


struct _LigmaWaitableInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  void       (* wait)       (LigmaWaitable *waitable);
  gboolean   (* try_wait)   (LigmaWaitable *waitable);
  gboolean   (* wait_until) (LigmaWaitable *waitable,
                             gint64        end_time);
};


void       ligma_waitable_wait       (LigmaWaitable *waitable);
gboolean   ligma_waitable_try_wait   (LigmaWaitable *waitable);
gboolean   ligma_waitable_wait_until (LigmaWaitable *waitable,
                                     gint64        end_time);
gboolean   ligma_waitable_wait_for   (LigmaWaitable *waitable,
                                     gint64        wait_duration);


#endif  /* __LIGMA_WAITABLE_H__ */
