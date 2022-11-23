/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_BRUSH_PIPE_H__
#define __LIGMA_BRUSH_PIPE_H__


#include "ligmabrush.h"


#define LIGMA_TYPE_BRUSH_PIPE            (ligma_brush_pipe_get_type ())
#define LIGMA_BRUSH_PIPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BRUSH_PIPE, LigmaBrushPipe))
#define LIGMA_BRUSH_PIPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BRUSH_PIPE, LigmaBrushPipeClass))
#define LIGMA_IS_BRUSH_PIPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BRUSH_PIPE))
#define LIGMA_IS_BRUSH_PIPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BRUSH_PIPE))
#define LIGMA_BRUSH_PIPE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BRUSH_PIPE, LigmaBrushPipeClass))


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


typedef struct _LigmaBrushPipeClass LigmaBrushPipeClass;

struct _LigmaBrushPipe
{
  LigmaBrush         parent_instance;

  gint              dimension;
  gint             *rank;       /* Size in each dimension */
  gint             *stride;     /* Aux for indexing */
  PipeSelectModes  *select;     /* One mode per dimension */

  gint             *index;      /* Current index for incremental dimensions */

  gint              n_brushes;  /* Might be less than the product of the
                                 * ranks in some odd special case */
  LigmaBrush       **brushes;
  LigmaBrush        *current;    /* Currently selected brush */

  gchar            *params;     /* For pipe <-> image conversion */
};

struct _LigmaBrushPipeClass
{
  LigmaBrushClass  parent_class;
};


GType      ligma_brush_pipe_get_type   (void) G_GNUC_CONST;

gboolean   ligma_brush_pipe_set_params (LigmaBrushPipe *pipe,
                                       const gchar   *paramstring);


#endif  /* __LIGMA_BRUSH_PIPE_H__ */
