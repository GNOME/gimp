/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_CONVOLVE_H__
#define __GIMP_CONVOLVE_H__


#include "gimppaintcore.h"
#include "gimppaintoptions.h"


#define GIMP_TYPE_CONVOLVE            (gimp_convolve_get_type ())
#define GIMP_CONVOLVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONVOLVE, GimpConvolve))
#define GIMP_CONVOLVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONVOLVE, GimpConvolveClass))
#define GIMP_IS_CONVOLVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONVOLVE))
#define GIMP_IS_CONVOLVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONVOLVE))
#define GIMP_CONVOLVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONVOLVE, GimpConvolveClass))


typedef struct _GimpConvolve      GimpConvolve;
typedef struct _GimpConvolveClass GimpConvolveClass;

struct _GimpConvolve
{
  GimpPaintCore parent_instance;
};

struct _GimpConvolveClass
{
  GimpPaintCoreClass parent_class;
};


typedef struct _GimpConvolveOptions GimpConvolveOptions;

struct _GimpConvolveOptions
{
  GimpPaintOptions  paint_options;

  ConvolveType      type;
  ConvolveType      type_d;
  GtkWidget        *type_w[2];

  gdouble           rate;
  gdouble           rate_d;
  GtkObject        *rate_w;
};


GType                 gimp_convolve_get_type    (void) G_GNUC_CONST;

GimpConvolveOptions * gimp_convolve_options_new (void);


#endif  /*  __GIMP_CONVOLVE_H__  */
