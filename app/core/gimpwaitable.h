/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpwaitable.h
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

#ifndef __GIMP_WAITABLE_H__
#define __GIMP_WAITABLE_H__


#define GIMP_TYPE_WAITABLE               (gimp_waitable_get_type ())
#define GIMP_IS_WAITABLE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_WAITABLE))
#define GIMP_WAITABLE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_WAITABLE, GimpWaitable))
#define GIMP_WAITABLE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_WAITABLE, GimpWaitableInterface))


typedef struct _GimpWaitableInterface GimpWaitableInterface;

struct _GimpWaitableInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  void       (* wait)       (GimpWaitable *waitable);
  gboolean   (* try_wait)   (GimpWaitable *waitable);
  gboolean   (* wait_until) (GimpWaitable *waitable,
                             gint64        end_time);
};


GType      gimp_waitable_get_type   (void) G_GNUC_CONST;

void       gimp_waitable_wait       (GimpWaitable *waitable);
gboolean   gimp_waitable_try_wait   (GimpWaitable *waitable);
gboolean   gimp_waitable_wait_until (GimpWaitable *waitable,
                                     gint64        end_time);
gboolean   gimp_waitable_wait_for   (GimpWaitable *waitable,
                                     gint64        wait_duration);


#endif  /* __GIMP_WAITABLE_H__ */
