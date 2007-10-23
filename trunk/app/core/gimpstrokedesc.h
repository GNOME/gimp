/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpstrokedesc.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_STROKE_DESC_H__
#define __GIMP_STROKE_DESC_H__


#include "gimpcontext.h"


#define GIMP_TYPE_STROKE_DESC            (gimp_stroke_desc_get_type ())
#define GIMP_STROKE_DESC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_STROKE_DESC, GimpStrokeDesc))
#define GIMP_STROKE_DESC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_STROKE_DESC, GimpStrokeDescClass))
#define GIMP_IS_STROKE_DESC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_STROKE_DESC))
#define GIMP_IS_STROKE_DESC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_STROKE_DESC))
#define GIMP_STROKE_DESC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_STROKE_DESC, GimpStrokeDescClass))


typedef struct _GimpStrokeDescClass GimpStrokeDescClass;

struct _GimpStrokeDesc
{
  GimpObject         parent_instance;

  Gimp              *gimp;

  GimpStrokeMethod   method;

  GimpStrokeOptions *stroke_options;
  GimpPaintInfo     *paint_info;

  GimpPaintOptions  *paint_options;
};

struct _GimpStrokeDescClass
{
  GimpObjectClass parent_class;
};


GType            gimp_stroke_desc_get_type  (void) G_GNUC_CONST;

GimpStrokeDesc * gimp_stroke_desc_new       (Gimp           *gimp,
                                             GimpContext    *context);
void             gimp_stroke_desc_prepare   (GimpStrokeDesc *desc,
                                             GimpContext    *context,
                                             gboolean        use_default_values);
void             gimp_stroke_desc_finish    (GimpStrokeDesc *desc);


#endif  /*  __GIMP_STROKE_DESC_H__  */
