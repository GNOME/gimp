/* GIMP - The GNU Image Manipulation Program
 * 
 * gimpcage.h
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_CAGE_H__
#define __GIMP_CAGE_H__

#include <glib-object.h>
#include "libgimpmath/gimpmathtypes.h"
#include <gegl.h>
#include <gegl-buffer.h>


#define GIMP_TYPE_CAGE            (gimp_cage_get_type ())
#define GIMP_CAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CAGE, GimpCage))
#define GIMP_CAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CAGE, GimpCageClass))
#define GIMP_IS_CAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CAGE))
#define GIMP_IS_CAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CAGE))
#define GIMP_CAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CAGE, GimpCageClass))


typedef struct _GimpCageClass GimpCageClass;

struct _GimpCage
{
  GObject         parent_instance;
  
  gint            cage_vertice_number; //number of vertices used by the cage
  gint            cage_vertices_max; //number of vertices allocated in memory
  GimpVector2    *cage_vertices;
  
  GeglBuffer     *cage_vertices_coef;
  GeglBuffer     *cage_edges_coef;
  
  //test data
  GeglRectangle   extent;
};


struct _GimpCageClass
{
  GObjectClass parent_class;
};

GType         gimp_cage_get_type          (void) G_GNUC_CONST;


#endif /* __GIMP_CAGE_H__ */
