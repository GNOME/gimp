/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimptemplate.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_TEMPLATE_H__
#define __GIMP_TEMPLATE_H__


#include "gimpviewable.h"


#define GIMP_TYPE_TEMPLATE            (gimp_template_get_type ())
#define GIMP_TEMPLATE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEMPLATE, GimpTemplate))
#define GIMP_TEMPLATE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEMPLATE, GimpTemplateClass))
#define GIMP_IS_TEMPLATE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEMPLATE))
#define GIMP_IS_TEMPLATE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEMPLATE))
#define GIMP_TEMPLATE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TEMPLATE, GimpTemplateClass))


typedef struct _GimpTemplateClass GimpTemplateClass;

struct _GimpTemplate
{
  GimpViewable       parent_instance;

  gint               width;
  gint               height;
  GimpUnit           unit;

  gdouble            xresolution;
  gdouble            yresolution;
  GimpUnit           resolution_unit;

  GimpImageBaseType  image_type;
  GimpFillType       fill_type;

  gchar             *comment;
  gchar             *filename;

  guint64            initial_size;
};

struct _GimpTemplateClass
{
  GimpViewableClass  parent_instance;
};


GType          gimp_template_get_type        (void) G_GNUC_CONST;

GimpTemplate * gimp_template_new             (const gchar    *name);

void           gimp_template_set_from_image  (GimpTemplate   *template,
                                              GimpImage      *image);

GimpImage    * gimp_template_create_image    (Gimp           *gimp,
                                              GimpTemplate   *template,
                                              GimpContext    *context);


#endif /* __GIMP_TEMPLATE__ */
