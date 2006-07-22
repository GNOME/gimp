/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpfilloptions.h
 * Copyright (C) 2003 Simon Budig
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

#ifndef __GIMP_FILL_OPTIONS_H__
#define __GIMP_FILL_OPTIONS_H__


#include "gimpcontext.h"


#define GIMP_TYPE_FILL_OPTIONS            (gimp_fill_options_get_type ())
#define GIMP_FILL_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILL_OPTIONS, GimpFillOptions))
#define GIMP_FILL_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILL_OPTIONS, GimpFillOptionsClass))
#define GIMP_IS_FILL_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILL_OPTIONS))
#define GIMP_IS_FILL_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILL_OPTIONS))
#define GIMP_FILL_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILL_OPTIONS, GimpFillOptionsClass))


typedef struct _GimpFillOptionsClass GimpFillOptionsClass;

struct _GimpFillOptions
{
  GimpContext      parent_instance;

  GimpStrokeStyle  style;

  gboolean         antialias;
};

struct _GimpFillOptionsClass
{
  GimpContextClass parent_class;
};


GType  gimp_fill_options_get_type         (void) G_GNUC_CONST;

#endif  /*  __GIMP_FILL_OPTIONS_H__  */
