/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __GIMP_HRULER_H__
#define __GIMP_HRULER_H__

#include "gimpruler.h"


G_BEGIN_DECLS


#define GIMP_TYPE_HRULER	    (gimp_hruler_get_type ())
#define GIMP_HRULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HRULER, GimpHRuler))
#define GIMP_HRULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HRULER, GimpHRulerClass))
#define GIMP_IS_HRULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_HRULER))
#define GIMP_IS_HRULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HRULER))
#define GIMP_HRULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_HRULER, GimpHRulerClass))


typedef GimpRulerClass  GimpHRulerClass;
typedef GimpRuler       GimpHRuler;


GType      gimp_hruler_get_type (void) G_GNUC_CONST;
GtkWidget* gimp_hruler_new      (void);


G_END_DECLS


#endif /* __GIMP_HRULER_H__ */
