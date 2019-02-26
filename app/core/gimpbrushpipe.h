/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Adrian Likins and Tor Lillqvist
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

#ifndef __GIMP_BRUSH_PIPE_H__
#define __GIMP_BRUSH_PIPE_H__


#include "gimpbrush.h"


#define GIMP_TYPE_BRUSH_PIPE            (gimp_brush_pipe_get_type ())
#define GIMP_BRUSH_PIPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH_PIPE, GimpBrushPipe))
#define GIMP_BRUSH_PIPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH_PIPE, GimpBrushPipeClass))
#define GIMP_IS_BRUSH_PIPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH_PIPE))
#define GIMP_IS_BRUSH_PIPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH_PIPE))
#define GIMP_BRUSH_PIPE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH_PIPE, GimpBrushPipeClass))


typedef enum
{
  PIPE_SELECT_CONSTANT,
  PIPE_SELECT_INCREMENTAL,
  PIPE_SELECT_ANGULAR,
  PIPE_SELECT_VELOCITY,
  PIPE_SELECT_RANDOM,
  PIPE_SELECT_PRESSURE,
  PIPE_SELECT_TILT_X,
  PIPE_SELECT_TILT_Y
} PipeSelectModes;


typedef struct _GimpBrushPipeClass GimpBrushPipeClass;

struct _GimpBrushPipe
{
  GimpBrush         parent_instance;

  gint              dimension;
  gint             *rank;       /* Size in each dimension */
  gint             *stride;     /* Aux for indexing */
  PipeSelectModes  *select;     /* One mode per dimension */

  gint             *index;      /* Current index for incremental dimensions */

  gint              n_brushes;  /* Might be less than the product of the
                                 * ranks in some odd special case */
  GimpBrush       **brushes;
  GimpBrush        *current;    /* Currently selected brush */

  gchar            *params;     /* For pipe <-> image conversion */
};

struct _GimpBrushPipeClass
{
  GimpBrushClass  parent_class;
};


GType      gimp_brush_pipe_get_type   (void) G_GNUC_CONST;

gboolean   gimp_brush_pipe_set_params (GimpBrushPipe *pipe,
                                       const gchar   *paramstring);


#endif  /* __GIMP_BRUSH_PIPE_H__ */
