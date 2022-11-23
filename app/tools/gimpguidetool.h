/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_GUIDE_TOOL_H__
#define __LIGMA_GUIDE_TOOL_H__


#include "ligmadrawtool.h"


#define LIGMA_TYPE_GUIDE_TOOL            (ligma_guide_tool_get_type ())
#define LIGMA_GUIDE_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GUIDE_TOOL, LigmaGuideTool))
#define LIGMA_GUIDE_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GUIDE_TOOL, LigmaGuideToolClass))
#define LIGMA_IS_GUIDE_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GUIDE_TOOL))
#define LIGMA_IS_GUIDE_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GUIDE_TOOL))
#define LIGMA_GUIDE_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_GUIDE_TOOL, LigmaGuideToolClass))


typedef struct _LigmaGuideToolGuide LigmaGuideToolGuide;
typedef struct _LigmaGuideTool      LigmaGuideTool;
typedef struct _LigmaGuideToolClass LigmaGuideToolClass;

struct _LigmaGuideToolGuide
{
  LigmaGuide           *guide;

  gint                 old_position;
  gint                 position;
  LigmaOrientationType  orientation;
  gboolean             custom;
};

struct _LigmaGuideTool
{
  LigmaDrawTool        parent_instance;

  LigmaGuideToolGuide *guides;
  gint                n_guides;
};

struct _LigmaGuideToolClass
{
  LigmaDrawToolClass  parent_class;
};


GType   ligma_guide_tool_get_type        (void) G_GNUC_CONST;

void    ligma_guide_tool_start_new       (LigmaTool            *parent_tool,
                                         LigmaDisplay         *display,
                                         LigmaOrientationType  orientation);
void    ligma_guide_tool_start_edit      (LigmaTool            *parent_tool,
                                         LigmaDisplay         *display,
                                         LigmaGuide           *guide);
void    ligma_guide_tool_start_edit_many (LigmaTool            *parent_tool,
                                         LigmaDisplay         *display,
                                         GList               *guides);


#endif  /*  __LIGMA_GUIDE_TOOL_H__  */
