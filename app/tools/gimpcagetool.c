/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "gimptransformoptions.h"

#include "gimp-intl.h"


G_DEFINE_TYPE (GimpCageTool, gimp_cage_tool, GIMP_TYPE_TRANSFORM_TOOL)

#define parent_class gimp_cage_tool_parent_class


void
gimp_cage_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_CAGE_TOOL,
                GIMP_TYPE_TRANSFORM_OPTIONS,
                gimp_transform_options_gui,
                GIMP_CONTEXT_BACKGROUND_MASK,
                "gimp-cage-tool",
                _("Rotate"),
                _("Rotate Tool: Rotate the layer, selection or path"),
                N_("_Rotate"), "<shift>R",
                NULL, GIMP_HELP_TOOL_ROTATE,
                GIMP_STOCK_TOOL_ROTATE,
                data);
}

static void
gimp_cage_tool_class_init (GimpCageToolClass *klass)
{

}

static void
gimp_cage_tool_init (GimpCageTool *self)
{

}


