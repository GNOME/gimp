/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptag.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_TAG_H__
#define __GIMP_TAG_H__


#include <glib-object.h>


#define GIMP_TYPE_TAG            (gimp_tag_get_type ())
#define GIMP_TAG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TAG, GimpTag))
#define GIMP_TAG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TAG, GimpTagClass))
#define GIMP_IS_TAG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TAG))
#define GIMP_IS_TAG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TAG))
#define GIMP_TAG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TAG, GimpTagClass))


typedef struct _GimpTagClass    GimpTagClass;

struct _GimpTag
{
  GObject parent_instance;
};

struct _GimpTagClass
{
  GObjectClass parent_class;
};


GType    gimp_tag_get_type (void) G_GNUC_CONST;

gboolean gimp_tag_equals   (const GimpTag *tag,
                            const GimpTag *other);


#endif /* __GIMP_TAG_H__ */

