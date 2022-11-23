/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Vector tool
 * Copyright (C) 2003 Simon Budig  <simon@ligma.org>
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

#ifndef __LIGMA_VECTOR_TOOL_H__
#define __LIGMA_VECTOR_TOOL_H__


#include "ligmadrawtool.h"


#define LIGMA_TYPE_VECTOR_TOOL            (ligma_vector_tool_get_type ())
#define LIGMA_VECTOR_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VECTOR_TOOL, LigmaVectorTool))
#define LIGMA_VECTOR_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_VECTOR_TOOL, LigmaVectorToolClass))
#define LIGMA_IS_VECTOR_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_VECTOR_TOOL))
#define LIGMA_IS_VECTOR_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_VECTOR_TOOL))
#define LIGMA_VECTOR_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_VECTOR_TOOL, LigmaVectorToolClass))

#define LIGMA_VECTOR_TOOL_GET_OPTIONS(t)  (LIGMA_VECTOR_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaVectorTool      LigmaVectorTool;
typedef struct _LigmaVectorToolClass LigmaVectorToolClass;

struct _LigmaVectorTool
{
  LigmaDrawTool    parent_instance;

  LigmaVectors    *vectors;        /* the current Vector data           */
  LigmaVectorMode  saved_mode;     /* used by modifier_key()            */

  LigmaToolWidget *widget;
  LigmaToolWidget *grab_widget;
};

struct _LigmaVectorToolClass
{
  LigmaDrawToolClass  parent_class;
};


void    ligma_vector_tool_register    (LigmaToolRegisterCallback  callback,
                                      gpointer                  data);

GType   ligma_vector_tool_get_type    (void) G_GNUC_CONST;

void    ligma_vector_tool_set_vectors (LigmaVectorTool           *vector_tool,
                                      LigmaVectors              *vectors);

#endif  /*  __LIGMA_VECTOR_TOOL_H__  */
