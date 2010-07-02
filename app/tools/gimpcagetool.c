/* GIMP - The GNU Image Manipulation Program
 *
 * gimpcagetool.c
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


#include "config.h"

#include <string.h>

#include <gtk/gtk.h>


#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"

#include "widgets/gimphelp-ids.h"

#include "gimpcagetool.h"
#include "gimpcageoptions.h"


#include "gimp-intl.h"

static void   gimp_cage_tool_finalize       (GObject *object);
static void   gimp_cage_tool_button_press   (GimpTool              *tool,
                                             const GimpCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             GimpButtonPressType    press_type,
                                             GimpDisplay           *display);
static void   gimp_cage_tool_button_release (GimpTool              *tool,
                                             const GimpCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             GimpButtonReleaseType  release_type,
                                             GimpDisplay           *display);
static void   gimp_cage_tool_draw           (GimpDrawTool *draw_tool);


G_DEFINE_TYPE (GimpCageTool, gimp_cage_tool, GIMP_TYPE_TRANSFORM_TOOL)

#define parent_class gimp_cage_tool_parent_class


void
gimp_cage_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_CAGE_TOOL, //Tool type
                GIMP_TYPE_CAGE_OPTIONS, //Tool options type
                gimp_cage_options_gui, //Tool opions gui
                0, //context_props
                "gimp-cage-tool",
                _("Cage Transform"),
                _("Cage Transform: Deform a selection with a cage"),
                N_("_Cage Transform"), "<shift>R",
                NULL, GIMP_HELP_TOOL_CAGE,
                GIMP_STOCK_TOOL_CAGE,
                data);
}

static void
gimp_cage_tool_class_init (GimpCageToolClass *klass)
{
  GObjectClass           *object_class         = G_OBJECT_CLASS (klass);
  GimpToolClass          *tool_class           = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass      *draw_tool_class      = GIMP_DRAW_TOOL_CLASS (klass);
  GimpTransformToolClass *transformtool_class  = GIMP_TRANSFORM_TOOL_CLASS (klass);
  
  object_class->finalize          = gimp_cage_tool_finalize;
  
  tool_class->button_press        = gimp_cage_tool_button_press;
  tool_class->button_release      = gimp_cage_tool_button_release;
  
  draw_tool_class->draw           = gimp_cage_tool_draw;
}

static void
gimp_cage_tool_init (GimpCageTool *self)
{

}


static void
gimp_cage_tool_finalize (GObject *object)
{
  GimpCageTool        *ct  = GIMP_CAGE_TOOL (object);
  //GimpCageToolPrivate *priv = GET_PRIVATE (ct);

  /*g_free (priv->points);
  g_free (priv->segment_indices);
  g_free (priv->saved_points_lower_segment);
  g_free (priv->saved_points_higher_segment);*/

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void gimp_cage_tool_button_press (GimpTool              *tool,
                                         const GimpCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonPressType    press_type,
                                         GimpDisplay           *display)
{

  
}

static void gimp_cage_tool_button_release (GimpTool              *tool,
                                           const GimpCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           GimpButtonReleaseType  release_type,
                                           GimpDisplay           *display)
{                                         

}

static void gimp_cage_tool_draw (GimpDrawTool *draw_tool)
{
  GimpCageTool       *fst                   = GIMP_CAGE_TOOL (draw_tool);
  //GimpCageToolPrivate *priv                  = GET_PRIVATE (fst);
  GimpTool           *tool                  = GIMP_TOOL (draw_tool);
  
  gimp_draw_tool_draw_line(draw_tool, 10, 10, 10, 40, FALSE);
  gimp_draw_tool_draw_line(draw_tool, 10, 40, 40, 40, FALSE);
  gimp_draw_tool_draw_line(draw_tool, 40, 40, 40, 10, FALSE);
  gimp_draw_tool_draw_line(draw_tool, 40, 10, 10, 10, FALSE);
  
}