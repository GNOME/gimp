/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_VRULER_H__
#define __GIMP_VRULER_H__

#include "gimpruler.h"


G_BEGIN_DECLS


#define GIMP_TYPE_VRULER            (gimp_vruler_get_type ())
#define GIMP_VRULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VRULER, GimpVRuler))
#define GIMP_VRULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VRULER, GimpVRulerClass))
#define GIMP_IS_VRULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VRULER))
#define GIMP_IS_VRULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VRULER))
#define GIMP_VRULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VRULER, GimpVRulerClass))


typedef GimpRulerClass  GimpVRulerClass;
typedef GimpRuler       GimpVRuler;


GType      gimp_vruler_get_type (void) G_GNUC_CONST;
GtkWidget* gimp_vruler_new      (void);


G_END_DECLS


#endif /* __GIMP_VRULER_H__ */

