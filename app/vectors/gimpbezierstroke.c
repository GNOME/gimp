/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpstroke.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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


#include "config.h"

#include "glib-object.h"

#include "vectors-types.h"

#include "gimpanchor.h"
#include "gimpbezierstroke.h"



/*  private variables  */


static GimpStrokeClass *parent_class = NULL;


static void gimp_bezier_stroke_class_init (GimpBezierStrokeClass *klass);
static void gimp_bezier_stroke_init       (GimpBezierStroke      *bezier_stroke);

static void gimp_bezier_stroke_finalize   (GObject               *object);


GType
gimp_bezier_stroke_get_type (void)
{
  static GType bezier_stroke_type = 0;

  if (! bezier_stroke_type)
    {
      static const GTypeInfo bezier_stroke_info =
      {
        sizeof (GimpBezierStrokeClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_bezier_stroke_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpBezierStroke),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_bezier_stroke_init,
      };

      bezier_stroke_type = g_type_register_static (GIMP_TYPE_STROKE,
                                                   "GimpBezierStroke", 
                                                   &bezier_stroke_info, 0);
    }

  return bezier_stroke_type;
}

static void
gimp_bezier_stroke_class_init (GimpBezierStrokeClass *klass)
{
  GObjectClass      *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize             = gimp_bezier_stroke_finalize;
}

static void
gimp_bezier_stroke_init (GimpBezierStroke *bezier_stroke)
{
  /* pass */
};

static void
gimp_bezier_stroke_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* Bezier specific functions */

GimpStroke *
gimp_bezier_stroke_new (const GimpCoords *start)
{
  GimpBezierStroke *bezier_stroke;
  GimpStroke       *stroke;
  GimpAnchor       *anchor;

  bezier_stroke = g_object_new (GIMP_TYPE_BEZIER_STROKE, NULL);

  stroke = GIMP_STROKE (bezier_stroke);

  anchor = g_new0 (GimpAnchor, 1);
  anchor->position.x = start->x;
  anchor->position.y = start->y;
  anchor->position.pressure = 1;

  g_printerr ("Adding at %f, %f\n", start->x, start->y);
  
  anchor->type = 0;    /* FIXME */
  anchor->active = FALSE;

  stroke->anchors = g_list_append (stroke->anchors, anchor);
  return stroke;
}


