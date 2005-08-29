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

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimppickable.h"
#include "core/gimptoolinfo.h"

#include "display/gimpcanvas.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpdrawtool.h"
#include "gimprectangleoptions.h"
#include "gimprectangletool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  speed of key movement  */
#define ARROW_VELOCITY 25


static void    gimp_rectangle_tool_iface_base_init    (GimpRectangleToolInterface *rectangle_tool_iface);

/*  Rectangle helper functions  */
static void     rectangle_recalc                          (GimpRectangleTool *rectangle);
static void     rectangle_tool_start                      (GimpRectangleTool *rectangle);

/*  Rectangle dialog functions  */
static void     rectangle_info_update                     (GimpRectangleTool *rectangle);
static void     rectangle_response                        (GtkWidget         *widget,
                                                           gint               response_id,
                                                           GimpRectangleTool *rectangle);

static void     rectangle_selection_callback              (GtkWidget         *widget,
                                                           GimpRectangleTool *rectangle);
static void     rectangle_automatic_callback              (GtkWidget         *widget,
                                                           GimpRectangleTool *rectangle);

static void     rectangle_dimensions_changed              (GtkWidget         *widget,
                                                           GimpRectangleTool *rectangle);

static void     gimp_rectangle_tool_update_options    (GimpRectangleTool *rectangle,
                                                       GimpDisplay       *gdisp);
static GtkWidget * gimp_rectangle_controls            (GimpRectangleTool *rectangle);
static void     gimp_rectangle_tool_zero_controls     (GimpRectangleTool *rectangle);


GType
gimp_rectangle_tool_interface_get_type (void)
{
  static GType rectangle_tool_iface_type = 0;

  if (!rectangle_tool_iface_type)
    {
      static const GTypeInfo rectangle_tool_iface_info =
      {
        sizeof (GimpRectangleToolInterface),
	      (GBaseInitFunc)     gimp_rectangle_tool_iface_base_init,
	      (GBaseFinalizeFunc) NULL,
      };

      rectangle_tool_iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                                          "GimpRectangleToolInterface",
                                                          &rectangle_tool_iface_info,
                                                          0);
    }

  return rectangle_tool_iface_type;
}

static void
gimp_rectangle_tool_iface_base_init (GimpRectangleToolInterface *rectangle_tool_iface)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      initialized = TRUE;
    }
}

void
gimp_rectangle_tool_set_controls (GimpRectangleTool *tool,
                                  GtkWidget         *controls)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_controls)
    tool_iface->set_entry (tool, controls);
}

GtkWidget *
gimp_rectangle_tool_get_controls (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_controls)
    return tool_iface->get_controls (tool);

  return 0;
}

void
gimp_rectangle_tool_set_entry (GimpRectangleTool *tool,
                               GtkWidget         *entry)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_entry)
    tool_iface->set_entry (tool, entry);
}

GtkWidget *
gimp_rectangle_tool_get_entry (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_entry)
    return tool_iface->get_entry (tool);

  return 0;
}

void
gimp_rectangle_tool_set_startx (GimpRectangleTool *tool,
                                gint               startx)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_startx)
    tool_iface->set_startx (tool, startx);
}

gint
gimp_rectangle_tool_get_startx (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_startx)
    return tool_iface->get_startx (tool);

  return 0;
}

void
gimp_rectangle_tool_set_starty (GimpRectangleTool *tool,
                                gint               starty)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_starty)
    tool_iface->set_starty (tool, starty);
}

gint
gimp_rectangle_tool_get_starty (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_starty)
    return tool_iface->get_starty (tool);

  return 0;
}

void
gimp_rectangle_tool_set_lastx (GimpRectangleTool *tool,
                               gint               lastx)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_lastx)
    tool_iface->set_lastx (tool, lastx);
}

gint
gimp_rectangle_tool_get_lastx (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_lastx)
    return tool_iface->get_lastx (tool);

  return 0;
}

void
gimp_rectangle_tool_set_lasty (GimpRectangleTool *tool,
                               gint               lasty)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_lasty)
    tool_iface->set_lasty (tool, lasty);
}

gint
gimp_rectangle_tool_get_lasty (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_lasty)
    return tool_iface->get_lasty (tool);

  return 0;
}

void
gimp_rectangle_tool_set_pressx (GimpRectangleTool *tool,
                                gint               pressx)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_pressx)
    tool_iface->set_pressx (tool, pressx);
}

gint
gimp_rectangle_tool_get_pressx (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_pressx)
    return tool_iface->get_pressx (tool);

  return 0;
}

void
gimp_rectangle_tool_set_pressy (GimpRectangleTool *tool,
                                gint               pressy)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_pressy)
    tool_iface->set_pressy (tool, pressy);
}

gint
gimp_rectangle_tool_get_pressy (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_pressy)
    return tool_iface->get_pressy (tool);

  return 0;
}

void
gimp_rectangle_tool_set_x1 (GimpRectangleTool *tool,
                            gint               x1)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_x1)
    tool_iface->set_x1 (tool, x1);
}

gint
gimp_rectangle_tool_get_x1 (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_x1)
    return tool_iface->get_x1 (tool);

  return 0;
}

void
gimp_rectangle_tool_set_y1 (GimpRectangleTool *tool,
                            gint               y1)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_y1)
    tool_iface->set_y1 (tool, y1);
}

gint
gimp_rectangle_tool_get_y1 (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_y1)
    return tool_iface->get_y1 (tool);

  return 0;
}

void
gimp_rectangle_tool_set_x2 (GimpRectangleTool *tool,
                            gint               x2)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_x2)
    tool_iface->set_x2 (tool, x2);
}

gint
gimp_rectangle_tool_get_x2 (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_x2)
    return tool_iface->get_x2 (tool);

  return 0;
}

void
gimp_rectangle_tool_set_y2 (GimpRectangleTool *tool,
                            gint               y2)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_y2)
    tool_iface->set_y2 (tool, y2);
}

gint
gimp_rectangle_tool_get_y2 (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_y2)
    return tool_iface->get_y2 (tool);

  return 0;
}

void
gimp_rectangle_tool_set_function (GimpRectangleTool *tool,
                                  guint              function)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_function)
    tool_iface->set_function (tool, function);
}

guint
gimp_rectangle_tool_get_function (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_function)
    return tool_iface->get_function (tool);

  return 0;
}

void
gimp_rectangle_tool_set_dx1 (GimpRectangleTool *tool,
                             gint               dx1)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_dx1)
    tool_iface->set_dx1 (tool, dx1);
}

gint
gimp_rectangle_tool_get_dx1 (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_dx1)
    return tool_iface->get_dx1 (tool);

  return 0;
}

void
gimp_rectangle_tool_set_dy1 (GimpRectangleTool *tool,
                             gint               dy1)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_dy1)
    tool_iface->set_dy1 (tool, dy1);
}

gint
gimp_rectangle_tool_get_dy1 (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_dy1)
    return tool_iface->get_dy1 (tool);

  return 0;
}

void
gimp_rectangle_tool_set_dx2 (GimpRectangleTool *tool,
                             gint               dx2)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_dx2)
    tool_iface->set_dx2 (tool, dx2);
}

gint
gimp_rectangle_tool_get_dx2 (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_dx2)
    return tool_iface->get_dx2 (tool);

  return 0;
}

void
gimp_rectangle_tool_set_dy2 (GimpRectangleTool *tool,
                             gint               dy2)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_dy2)
    tool_iface->set_dy2 (tool, dy2);
}

gint
gimp_rectangle_tool_get_dy2 (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_dy2)
    return tool_iface->get_dy2 (tool);

  return 0;
}

void
gimp_rectangle_tool_set_dcw (GimpRectangleTool *tool,
                             gint               dcw)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_dcw)
    tool_iface->set_dcw (tool, dcw);
}

gint
gimp_rectangle_tool_get_dcw (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_dcw)
    return tool_iface->get_dcw (tool);

  return 0;
}

void
gimp_rectangle_tool_set_dch (GimpRectangleTool *tool,
                             gint               dch)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_dch)
    tool_iface->set_dch (tool, dch);
}

gint
gimp_rectangle_tool_get_dch (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_dch)
    return tool_iface->get_dch (tool);

  return 0;
}

void
gimp_rectangle_tool_set_origx (GimpRectangleTool *tool,
                               gdouble            origx)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_origx)
    tool_iface->set_origx (tool, origx);
}

gdouble
gimp_rectangle_tool_get_origx (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_origx)
    return tool_iface->get_origx (tool);

  return 0;
}

void
gimp_rectangle_tool_set_origy (GimpRectangleTool *tool,
                               gdouble            origy)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_origy)
    tool_iface->set_origy (tool, origy);
}

gdouble
gimp_rectangle_tool_get_origy (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_origy)
    return tool_iface->get_origy (tool);

  return 0;
}

void
gimp_rectangle_tool_set_sizew (GimpRectangleTool *tool,
                               gdouble            sizew)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_sizew)
    tool_iface->set_sizew (tool, sizew);
}

gdouble
gimp_rectangle_tool_get_sizew (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_sizew)
    return tool_iface->get_sizew (tool);

  return 0;
}

void
gimp_rectangle_tool_set_sizeh (GimpRectangleTool *tool,
                               gdouble            sizeh)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->set_sizeh)
    tool_iface->set_sizeh (tool, sizeh);
}

gdouble
gimp_rectangle_tool_get_sizeh (GimpRectangleTool *tool)
{
  GimpRectangleToolInterface *tool_iface;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (tool_iface->get_sizeh)
    return tool_iface->get_sizeh (tool);

  return 0;
}

void
gimp_rectangle_tool_constructor (GObject *object)
{
  GimpTool                   *tool;
  GimpRectangleTool          *rectangle;
  GimpRectangleToolInterface *tool_iface;
  GtkContainer               *controls_container;
  GObject                    *options;

  tool       = GIMP_TOOL (object);
  rectangle  = GIMP_RECTANGLE_TOOL (object);
  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);
  
  g_return_if_fail (tool_iface->get_controls && tool_iface->set_controls);

  tool->gdisp = NULL;

  g_assert (GIMP_IS_TOOL_INFO (tool->tool_info));

  options = G_OBJECT (tool->tool_info->tool_options);

  controls_container = GTK_CONTAINER (g_object_get_data (options,
                                                         "controls-container"));

  tool_iface->set_controls (rectangle, gimp_rectangle_controls (rectangle));
  gtk_container_add (controls_container, tool_iface->get_controls (rectangle));
}

void
gimp_rectangle_tool_finalize (GObject *object)
{
  GimpRectangleTool          *rectangle = GIMP_RECTANGLE_TOOL (object);
  GimpRectangleToolInterface *tool_iface;

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);
  
  g_return_if_fail (tool_iface->get_controls && tool_iface->set_controls);
  
  if (tool_iface->get_controls (rectangle))
    {
      gtk_widget_destroy (tool_iface->get_controls (rectangle));
      tool_iface->set_controls (rectangle, NULL);
    }
}

gboolean
gimp_rectangle_tool_initialize (GimpTool    *tool,
                                GimpDisplay *gdisp)
{
  GimpRectangleTool          *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolInterface *tool_iface;
  GimpSizeEntry              *entry;

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);
  
  g_return_val_if_fail (tool_iface->get_entry, FALSE);
  
  entry = GIMP_SIZE_ENTRY (tool_iface->get_entry (rectangle));

  if (gdisp != tool->gdisp)
    {
      gint     width  = gimp_image_get_width (gdisp->gimage);
      gint     height = gimp_image_get_height (gdisp->gimage);
      GimpUnit unit;
      gdouble  xres;
      gdouble  yres;

      gimp_size_entry_set_refval_boundaries (entry, 0, 0, height);
      gimp_size_entry_set_refval_boundaries (entry, 1, 0, width);
      gimp_size_entry_set_refval_boundaries (entry, 2, 0, width);
      gimp_size_entry_set_refval_boundaries (entry, 3, 0, height);

      gimp_size_entry_set_size (entry, 0, 0, height);
      gimp_size_entry_set_size (entry, 1, 0, width);
      gimp_size_entry_set_size (entry, 2, 0, width);
      gimp_size_entry_set_size (entry, 3, 0, height);

      gimp_image_get_resolution (gdisp->gimage, &xres, &yres);

      gimp_size_entry_set_resolution (entry, 0, yres, TRUE);
      gimp_size_entry_set_resolution (entry, 1, xres, TRUE);
      gimp_size_entry_set_resolution (entry, 2, xres, TRUE);
      gimp_size_entry_set_resolution (entry, 3, yres, TRUE);

      unit = gimp_display_shell_get_unit (GIMP_DISPLAY_SHELL (gdisp->shell));
      gimp_size_entry_set_unit (entry, unit);
    }

  return TRUE;
}

void
gimp_rectangle_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *gdisp)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      rectangle_recalc (rectangle);
      break;

    case HALT:
      rectangle_response (NULL, GTK_RESPONSE_CANCEL, rectangle);
      break;

    default:
      break;
    }
}

void
gimp_rectangle_tool_button_press (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
  GimpRectangleTool          *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolInterface *tool_iface;
  GimpDrawTool               *draw_tool = GIMP_DRAW_TOOL (tool);

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  if (gdisp != tool->gdisp)
    {
      if (gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_stop (draw_tool);

      tool_iface->set_function (rectangle, RECT_CREATING);
      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);

      tool->gdisp = gdisp;

      tool_iface->set_x1 (rectangle, ROUND (coords->x));
      tool_iface->set_y1 (rectangle, ROUND (coords->y));
      tool_iface->set_x2 (rectangle, ROUND (coords->x));
      tool_iface->set_y2 (rectangle, ROUND (coords->y));

      rectangle_tool_start (rectangle);
    }

  tool_iface->set_pressx (rectangle, ROUND (coords->x));
  tool_iface->set_pressy (rectangle, ROUND (coords->y));
  tool_iface->set_lastx (rectangle, ROUND (coords->x));
  tool_iface->set_lasty (rectangle, ROUND (coords->y));
  tool_iface->set_startx (rectangle, ROUND (coords->x));
  tool_iface->set_starty (rectangle, ROUND (coords->y));

  gimp_tool_control_activate (tool->control);
}

void
gimp_rectangle_tool_button_release (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  GimpRectangleTool          *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolInterface *tool_iface;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  gimp_tool_control_halt (tool->control);
  gimp_tool_pop_status (tool, gdisp);

  if (! (state & GDK_BUTTON3_MASK))
    {
      if ( tool_iface->get_lastx && tool_iface->get_pressx &&
           tool_iface->get_lasty && tool_iface->get_pressy &&
           (tool_iface->get_lastx (rectangle) == tool_iface->get_pressx (rectangle)) &&
           (tool_iface->get_lasty (rectangle) == tool_iface->get_pressy (rectangle)))
        {
          rectangle_response (NULL, GIMP_RECTANGLE_MODE_EXECUTE, rectangle);
        }

      rectangle_info_update (rectangle);
    }
}

void
gimp_rectangle_tool_motion (GimpTool        *tool,
                            GimpCoords      *coords,
                            guint32          time,
                            GdkModifierType  state,
                            GimpDisplay     *gdisp)
{
  GimpRectangleTool             *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolInterface    *tool_iface;
  GimpRectangleOptions          *options;
  GimpRectangleOptionsInterface *options_iface;
  gint                           x1, y1, x2, y2;
  gint                           curx, cury;
  gint                           inc_x, inc_y;
  gint                           min_x, min_y, max_x, max_y;
  gboolean                       fixed_width;
  gboolean                       fixed_height;
  gboolean                       fixed_aspect;
  gboolean                       fixed_center;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  /*  This is the only case when the motion events should be ignored--
      we're just waiting for the button release event to execute  */
  if (tool_iface->get_function (rectangle) == RECT_EXECUTING)
    return;

  g_return_if_fail (tool_iface->get_startx && tool_iface->get_starty
      && tool_iface->get_lastx && tool_iface->get_lasty
      && tool_iface->get_function
      && tool_iface->get_x1 && tool_iface->get_y1
      && tool_iface->get_x2 && tool_iface->get_y2
      && tool_iface->set_startx && tool_iface->set_starty
      && tool_iface->set_lastx && tool_iface->set_lasty
      && tool_iface->set_x1 && tool_iface->set_y1
      && tool_iface->set_x2 && tool_iface->set_y2);
  
  curx = ROUND (coords->x);
  cury = ROUND (coords->y);

  x1 = tool_iface->get_startx (rectangle);
  y1 = tool_iface->get_starty (rectangle);
  x2 = curx;
  y2 = cury;

  inc_x = (x2 - x1);
  inc_y = (y2 - y1);

  /*  If there have been no changes... return  */
  if (tool_iface->get_lastx (rectangle) == x2 && tool_iface->get_lasty (rectangle) == y2)
    return;

  options = GIMP_RECTANGLE_OPTIONS (tool->tool_info->tool_options);
  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  g_return_if_fail (options_iface->get_fixed_width && options_iface->get_fixed_height
      && options_iface->get_fixed_aspect && options_iface->get_fixed_center
      && options_iface->get_width && options_iface->get_height
      && options_iface->get_center_x && options_iface->get_center_y
      && options_iface->get_aspect);

  fixed_width  = options_iface->get_fixed_width (options);
  fixed_height = options_iface->get_fixed_height (options);
  fixed_aspect = options_iface->get_fixed_aspect (options);
  fixed_center = options_iface->get_fixed_center (options);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  min_x = min_y = 0;
  max_x = gdisp->gimage->width;
  max_y = gdisp->gimage->height;

  switch (tool_iface->get_function (rectangle))
    {
    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LEFT:
      x1 = tool_iface->get_x1 (rectangle) + inc_x;
      if (fixed_width)
        {
          x2 = x1 + options_iface->get_width (options);
          if (x1 < 0)
            {
              x1 = 0;
              x2 = options_iface->get_width (options);
            }
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - options_iface->get_width (options);
            }
        }
      else if (fixed_center)
        {
          x2 = x1 + 2 * (options_iface->get_center_x (options) - x1);
          if (x1 < 0)
            {
              x1 = 0;
              x2 = 2 * options_iface->get_center_x (options);
            }
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - 2 * (max_x - options_iface->get_center_x (options));
            }
        }
      else
        {
          x2 = MAX (x1, tool_iface->get_x2 (rectangle));
        }
      tool_iface->set_startx (rectangle, curx);
      break;

    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_RIGHT:
      x2 = tool_iface->get_x2 (rectangle) + inc_x;
      if (fixed_width)
        {
          x1 = x2 - options_iface->get_width (options);
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - options_iface->get_width (options);
            }
          if (x1 < 0)
            {
              x1 = 0;
              x2 = options_iface->get_width (options);
            }
        }
      else if (fixed_center)
        {
          x1 = x2 - 2 * (x2 - options_iface->get_center_x (options));
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - 2 * (max_x - options_iface->get_center_x (options));
            }
          if (x1 < 0)
            {
              x1 = 0;
              x2 = 2 * options_iface->get_center_x (options);
            }
        }
      else
        {
          x1 = MIN (tool_iface->get_x1 (rectangle), x2);
        }
      tool_iface->set_startx (rectangle, curx);
      break;

    case RECT_RESIZING_BOTTOM:
    case RECT_RESIZING_TOP:
      x1 = tool_iface->get_x1 (rectangle);
      x2 = tool_iface->get_x2 (rectangle);
      tool_iface->set_startx (rectangle, curx);
      break;

    case RECT_MOVING:
      if (fixed_width &&
          (tool_iface->get_x1 (rectangle) + inc_x < 0 ||
           tool_iface->get_x2 (rectangle) + inc_x > max_x))
        {
          x1 = tool_iface->get_x1 (rectangle);
          x2 = tool_iface->get_x2 (rectangle);
        }
      else
        {
          x1 = tool_iface->get_x1 (rectangle) + inc_x;
          x2 = tool_iface->get_x2 (rectangle) + inc_x;
        }
      tool_iface->set_startx (rectangle, curx);
      break;
    }

  switch (tool_iface->get_function (rectangle))
    {
    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_TOP:
      y1 = tool_iface->get_y1 (rectangle) + inc_y;
      if (fixed_height)
        {
          y2 = y1 + options_iface->get_height (options);
          if (y1 < 0)
            {
              y1 = 0;
              y2 = options_iface->get_height (options);
            }
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - options_iface->get_height (options);
            }
        }
      else if (fixed_center)
        {
          y2 = y1 + 2 * (options_iface->get_center_y (options) - y1);
          if (y1 < 0)
            {
              y1 = 0;
              y2 = 2 * options_iface->get_center_y (options);
            }
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - 2 * (max_y - options_iface->get_center_y (options));
            }
        }
      else
        {
          y2 = MAX (y1, tool_iface->get_y2 (rectangle));
        }
      tool_iface->set_starty (rectangle, cury);
      break;

    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_BOTTOM:
      y2 = tool_iface->get_y2 (rectangle) + inc_y;
      if (fixed_height)
        {
          y1 = y2 - options_iface->get_height (options);
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - options_iface->get_height (options);
            }
          if (y1 < 0)
            {
              y1 = 0;
              y2 = options_iface->get_height (options);
            }
        }
      else if (fixed_center)
        {
          y1 = y2 - 2 * (y2 - options_iface->get_center_y (options));
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - 2 * (max_y - options_iface->get_center_y (options));
            }
          if (y1 < 0)
            {
              y1 = 0;
              y2 = 2 * options_iface->get_center_y (options);
            }
        }
      else
        {
          y1 = MIN (tool_iface->get_y1 (rectangle), y2);
        }
      tool_iface->set_starty (rectangle, cury);
      break;

    case RECT_RESIZING_RIGHT:
    case RECT_RESIZING_LEFT:
      y1 = tool_iface->get_y1 (rectangle);
      y2 = tool_iface->get_y2 (rectangle);
      tool_iface->set_starty (rectangle, cury);
      break;

    case RECT_MOVING:
      if (fixed_height &&
          (tool_iface->get_y1 (rectangle) + inc_y < 0 ||
          tool_iface->get_y2 (rectangle) + inc_y > max_y))
        {
          y1 = tool_iface->get_y1 (rectangle);
          y2 = tool_iface->get_y2 (rectangle);
        }
      else
        {
          y1 = tool_iface->get_y1 (rectangle) + inc_y;
          y2 = tool_iface->get_y2 (rectangle) + inc_y;
        }
      tool_iface->set_starty (rectangle, cury);
      break;
    }

  /*  make sure that the coords are in bounds  */
  tool_iface->set_x1 (rectangle, MIN (x1, x2));
  tool_iface->set_y1 (rectangle, MIN (y1, y2));
  tool_iface->set_x2 (rectangle, MAX (x1, x2));
  tool_iface->set_y2 (rectangle, MAX (y1, y2));

  tool_iface->set_lastx (rectangle, curx);
  tool_iface->set_lasty (rectangle, cury);

  if (fixed_aspect)
    {
      gdouble aspect = options_iface->get_aspect (options);

      if (aspect > max_y)
        aspect = max_y;
      if (aspect < 1.0 / max_x)
        aspect = 1.0 / max_x;

      switch (tool_iface->get_function (rectangle))
        {
        case RECT_RESIZING_UPPER_LEFT:
        case RECT_RESIZING_LEFT:
          tool_iface->set_y2 (rectangle, tool_iface->get_y1 (rectangle) + aspect * (tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle)));
          if (tool_iface->get_y2 (rectangle) > max_y)
            {
              tool_iface->set_y2 (rectangle, max_y);
              tool_iface->set_x2 (rectangle, tool_iface->get_x1 (rectangle) + (tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle)) / aspect);
            }
          break;

        case RECT_RESIZING_UPPER_RIGHT:
        case RECT_RESIZING_RIGHT:
          tool_iface->set_y2 (rectangle, tool_iface->get_y1 (rectangle) + aspect * (tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle)));
          if (tool_iface->get_y2 (rectangle) > max_y)
            {
              tool_iface->set_y2 (rectangle, max_y);
              tool_iface->set_x1 (rectangle, tool_iface->get_x2 (rectangle) - (tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle)) / aspect);
            }
          break;

        case RECT_RESIZING_LOWER_LEFT:
          tool_iface->set_y1 (rectangle, tool_iface->get_y2 (rectangle) - aspect * (tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle)));
          if (tool_iface->get_y1 (rectangle) < 0)
            {
              tool_iface->set_y1 (rectangle, 0);
              tool_iface->set_x2 (rectangle, tool_iface->get_x1 (rectangle) + (tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle)) / aspect);
            }
          break;

        case RECT_RESIZING_LOWER_RIGHT:
          tool_iface->set_y1 (rectangle, tool_iface->get_y2 (rectangle) - aspect * (tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle)));
          if (tool_iface->get_y1 (rectangle) < 0)
            {
              tool_iface->set_y1 (rectangle, 0);
              tool_iface->set_x1 (rectangle, tool_iface->get_x2 (rectangle) - (tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle)) / aspect);
            }
          break;

        case RECT_RESIZING_TOP:
          tool_iface->set_x2 (rectangle, tool_iface->get_x1 (rectangle) + (tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle)) / aspect);
          if (tool_iface->get_x2 (rectangle) > max_x)
            {
              tool_iface->set_x2 (rectangle, max_x);
              tool_iface->set_y2 (rectangle, tool_iface->get_y1 (rectangle) + aspect * (tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle)));
            }
          break;

        case RECT_RESIZING_BOTTOM:
          tool_iface->set_x2 (rectangle, tool_iface->get_x1 (rectangle) + (tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle)) / aspect);
          if (tool_iface->get_x2 (rectangle) > max_x)
            {
              tool_iface->set_x2 (rectangle, max_x);
              tool_iface->set_y1 (rectangle, tool_iface->get_y2 (rectangle) - aspect * (tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle)));
            }
          break;

        default:
          break;
        }
    }

  /*  recalculate the coordinates for rectangle_draw based on the new values  */
  rectangle_recalc (rectangle);

  switch (tool_iface->get_function (rectangle))
    {
    case RECT_RESIZING_UPPER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x1 (rectangle) - coords->x,
                                          tool_iface->get_y1 (rectangle) - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_UPPER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x2 (rectangle) - coords->x,
                                          tool_iface->get_y1 (rectangle) - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LOWER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x1 (rectangle) - coords->x,
                                          tool_iface->get_y2 (rectangle) - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LOWER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x2 (rectangle) - coords->x,
                                          tool_iface->get_y2 (rectangle) - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x1 (rectangle) - coords->x, 0, 0, 0);
      break;

    case RECT_RESIZING_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x2 (rectangle) - coords->x, 0, 0, 0);
      break;

    case RECT_RESIZING_TOP:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, tool_iface->get_y1 (rectangle) - coords->y, 0, 0);
      break;

    case RECT_RESIZING_BOTTOM:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, tool_iface->get_y2 (rectangle) - coords->y, 0, 0);
      break;

    case RECT_MOVING:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x1 (rectangle) - coords->x,
                                          tool_iface->get_y1 (rectangle) - coords->y,
                                          tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle),
                                          tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle));
      break;

    default:
      break;
    }

  gimp_rectangle_tool_update_options (rectangle, gdisp);

  if (tool_iface->get_function (rectangle) == RECT_CREATING      ||
      tool_iface->get_function (rectangle) == RECT_RESIZING_UPPER_LEFT ||
      tool_iface->get_function (rectangle) == RECT_RESIZING_LOWER_RIGHT)
    {
      gimp_tool_pop_status (tool, gdisp);
      gimp_tool_push_status_coords (tool, gdisp,
                                    _("Rectangle: "),
                                    tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle),
                                    " x ",
                                    tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle));
    }

  if (tool_iface->get_function == RECT_CREATING)
    {
      if (inc_x < 0 && inc_y < 0)
        tool_iface->set_function (rectangle, RECT_RESIZING_UPPER_LEFT);
      else if (inc_x < 0 && inc_y > 0)
        tool_iface->set_function (rectangle, RECT_RESIZING_LOWER_LEFT);
      else if (inc_x > 0 && inc_y < 0)
        tool_iface->set_function (rectangle, RECT_RESIZING_UPPER_RIGHT);
      else if (inc_x > 0 && inc_y > 0)
        tool_iface->set_function (rectangle, RECT_RESIZING_LOWER_RIGHT);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

gboolean
gimp_rectangle_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *gdisp)
{
  GimpRectangleTool          *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolInterface *tool_iface;
  gint                        inc_x, inc_y;
  gint                        min_x, min_y;
  gint                        max_x, max_y;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), FALSE);

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  g_return_val_if_fail (tool_iface->get_x1 && tool_iface->get_x2
      && tool_iface->get_y1 && tool_iface->get_y2
      && tool_iface->set_x1 && tool_iface->set_x2
      && tool_iface->set_y1 && tool_iface->set_y2, FALSE);
  
  if (gdisp != tool->gdisp)
    return FALSE;

  inc_x = inc_y = 0;

  switch (kevent->keyval)
    {
    case GDK_Up:
      inc_y = -1;
      break;
    case GDK_Left:
      inc_x = -1;
      break;
    case GDK_Right:
      inc_x = 1;
      break;
    case GDK_Down:
      inc_y = 1;
      break;

    case GDK_KP_Enter:
    case GDK_Return:
      rectangle_response (NULL, GIMP_RECTANGLE_MODE_EXECUTE, rectangle);
      return TRUE;

    case GDK_Escape:
      rectangle_response (NULL, GTK_RESPONSE_CANCEL, rectangle);
      return TRUE;

    default:
      return FALSE;
    }

  /*  If the shift key is down, move by an accelerated increment  */
  if (kevent->state & GDK_SHIFT_MASK)
    {
      inc_y *= ARROW_VELOCITY;
      inc_x *= ARROW_VELOCITY;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  min_x = min_y = 0;
  max_x = gdisp->gimage->width;
  max_y = gdisp->gimage->height;

  tool_iface->set_x1 (rectangle, tool_iface->get_x1 (rectangle) + inc_x);
  tool_iface->set_x2 (rectangle, tool_iface->get_x2 (rectangle) + inc_x);
  tool_iface->set_y1 (rectangle, tool_iface->get_y1 (rectangle) + inc_y);
  tool_iface->set_y2 (rectangle, tool_iface->get_y2 (rectangle) + inc_y);

  rectangle_recalc (rectangle);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  return TRUE;
}

void
gimp_rectangle_tool_modifier_key (GimpTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
}

void
gimp_rectangle_tool_oper_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 GimpDisplay     *gdisp)
{
  GimpRectangleTool          *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolInterface *tool_iface;
  GimpDrawTool               *draw_tool = GIMP_DRAW_TOOL (tool);
  gboolean                    inside_x;
  gboolean                    inside_y;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  g_return_if_fail (tool_iface->get_x1 && tool_iface->get_x2
      && tool_iface->get_y1 && tool_iface->get_y2
      && tool_iface->get_dcw && tool_iface->get_dch
      && tool_iface->set_function);
  
  if (tool->gdisp != gdisp)
    {
      return;
    }

  inside_x = coords->x > tool_iface->get_x1 (rectangle) && coords->x < tool_iface->get_x2 (rectangle);
  inside_y = coords->y > tool_iface->get_y1 (rectangle) && coords->y < tool_iface->get_y2 (rectangle);

  if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                coords->x, coords->y,
                                GIMP_HANDLE_SQUARE,
                                tool_iface->get_x1 (rectangle), tool_iface->get_y1 (rectangle),
                                tool_iface->get_dcw (rectangle), tool_iface->get_dch (rectangle),
                                GTK_ANCHOR_NORTH_WEST,
                                FALSE))
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_UPPER_LEFT);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x1 (rectangle) - coords->x,
                                          tool_iface->get_y1 (rectangle) - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     tool_iface->get_x2 (rectangle), tool_iface->get_y2 (rectangle),
                                     tool_iface->get_dcw (rectangle), tool_iface->get_dch (rectangle),
                                     GTK_ANCHOR_SOUTH_EAST,
                                     FALSE))
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_LOWER_RIGHT);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x2 (rectangle) - coords->x,
                                          tool_iface->get_y2 (rectangle) - coords->y,
                                          0, 0);
    }
  else if  (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                      coords->x, coords->y,
                                      GIMP_HANDLE_SQUARE,
                                      tool_iface->get_x2 (rectangle), tool_iface->get_y1 (rectangle),
                                      tool_iface->get_dcw (rectangle), tool_iface->get_dch (rectangle),
                                      GTK_ANCHOR_NORTH_EAST,
                                      FALSE))
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_UPPER_RIGHT);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x2 (rectangle) - coords->x,
                                          tool_iface->get_y1 (rectangle) - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     tool_iface->get_x1 (rectangle), tool_iface->get_y2 (rectangle),
                                     tool_iface->get_dcw (rectangle), tool_iface->get_dch (rectangle),
                                     GTK_ANCHOR_SOUTH_WEST,
                                     FALSE))
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_LOWER_LEFT);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x1 (rectangle) - coords->x,
                                          tool_iface->get_y2 (rectangle) - coords->y,
                                          0, 0);
    }
  else if ( (fabs (coords->x - tool_iface->get_x1 (rectangle)) < tool_iface->get_dcw (rectangle)) && inside_y)
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_LEFT);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x1 (rectangle) - coords->x, 0, 0, 0);

    }
  else if ( (fabs (coords->x - tool_iface->get_x2 (rectangle)) < tool_iface->get_dcw (rectangle)) && inside_y)
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_RIGHT);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          tool_iface->get_x2 (rectangle) - coords->x, 0, 0, 0);

    }
  else if ( (fabs (coords->y - tool_iface->get_y1 (rectangle)) < tool_iface->get_dch (rectangle)) && inside_x)
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_TOP);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, tool_iface->get_y1 (rectangle) - coords->y, 0, 0);

    }
  else if ( (fabs (coords->y - tool_iface->get_y2 (rectangle)) < tool_iface->get_dch (rectangle)) && inside_x)
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_BOTTOM);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, tool_iface->get_y2 (rectangle) - coords->y, 0, 0);

    }
  else if (inside_x && inside_y)
    {
      tool_iface->set_function (rectangle, RECT_MOVING);
    }
  /*  otherwise, the new function will be creating, since we want
   *  to start a new rectangle
   */
  else
    {
      tool_iface->set_function (rectangle, RECT_CREATING);

      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);
    }
}

void
gimp_rectangle_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *gdisp)
{
  GimpRectangleTool          *rectangle   = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolInterface *tool_iface;
  GimpRectangleOptions       *options;
  GimpCursorType              cursor      = GDK_CROSSHAIR;
  GimpCursorModifier          modifier    = GIMP_CURSOR_MODIFIER_NONE;
  GimpToolCursorType          tool_cursor;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  g_return_if_fail (tool_iface->set_function);
  
  tool_cursor = gimp_tool_control_get_tool_cursor (tool->control);

  options = GIMP_RECTANGLE_OPTIONS (tool->tool_info->tool_options);

  if (tool->gdisp == gdisp && ! (state & GDK_BUTTON1_MASK))
    {
      switch (tool_iface->get_function (rectangle))
        {
        case RECT_CREATING:
          cursor = GDK_CROSSHAIR;
          break;
        case RECT_MOVING:
          cursor = GDK_FLEUR;
          break;
        case RECT_RESIZING_UPPER_LEFT:
          cursor = GDK_TOP_LEFT_CORNER;
          break;
        case RECT_RESIZING_UPPER_RIGHT:
          cursor = GDK_TOP_RIGHT_CORNER;
          break;
        case RECT_RESIZING_LOWER_LEFT:
          cursor = GDK_BOTTOM_LEFT_CORNER;
          break;
        case RECT_RESIZING_LOWER_RIGHT:
          cursor = GDK_BOTTOM_RIGHT_CORNER;
          break;
        case RECT_RESIZING_LEFT:
          cursor = GDK_LEFT_SIDE;
          break;
        case RECT_RESIZING_RIGHT:
          cursor = GDK_RIGHT_SIDE;
          break;
        case RECT_RESIZING_TOP:
          cursor = GDK_TOP_SIDE;
          break;
        case RECT_RESIZING_BOTTOM:
          cursor = GDK_BOTTOM_SIDE;
          break;

        default:
          break;
        }
    }

  gimp_tool_control_set_cursor (tool->control, cursor);
  gimp_tool_control_set_tool_cursor (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  gimp_tool_set_cursor (tool, gdisp,
                        gimp_tool_control_get_cursor (tool->control),
                        gimp_tool_control_get_tool_cursor (tool->control),
                        gimp_tool_control_get_cursor_modifier (tool->control));
}

void
gimp_rectangle_tool_draw (GimpDrawTool *draw)
{
  GimpRectangleTool          *rectangle = GIMP_RECTANGLE_TOOL (draw);
  GimpRectangleToolInterface *tool_iface;
  GimpTool                   *tool      = GIMP_TOOL (draw);
  GimpDisplayShell           *shell     = GIMP_DISPLAY_SHELL (tool->gdisp->shell);
  GimpCanvas                 *canvas    = GIMP_CANVAS (shell->canvas);

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  g_return_if_fail (tool_iface->get_dx1 && tool_iface->get_dy1
      && tool_iface->get_dx2 && tool_iface->get_dy2);
  
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         tool_iface->get_dx1 (rectangle), tool_iface->get_dy1 (rectangle),
                         tool_iface->get_dx2 (rectangle), tool_iface->get_dy1 (rectangle));
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         tool_iface->get_dx1 (rectangle), tool_iface->get_dy1 (rectangle),
                         tool_iface->get_dx1 (rectangle), tool_iface->get_dy2 (rectangle));
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         tool_iface->get_dx2 (rectangle), tool_iface->get_dy2 (rectangle),
                         tool_iface->get_dx1 (rectangle), tool_iface->get_dy2 (rectangle));
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         tool_iface->get_dx2 (rectangle), tool_iface->get_dy2 (rectangle),
                         tool_iface->get_dx2 (rectangle), tool_iface->get_dy1 (rectangle));
}

static void
rectangle_recalc (GimpRectangleTool *rectangle)
{
  GimpTool                      *tool     = GIMP_TOOL (rectangle);
  GimpDisplayShell              *shell    = GIMP_DISPLAY_SHELL (tool->gdisp->shell);
  GimpRectangleToolInterface    *tool_iface;
  GimpRectangleOptions          *options;
  GimpRectangleOptionsInterface *options_iface;
  gint                           dx1, dy1;
  gint                           dx2, dy2;

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (tool);

  g_return_if_fail (tool_iface->get_x1 && tool_iface->get_y1
      && tool_iface->get_x2 && tool_iface->get_y2
      && tool_iface->set_dx1 && tool_iface->set_dy1
      && tool_iface->set_dx2 && tool_iface->set_dy2
      && tool_iface->set_dcw && tool_iface->set_dch);

  options = GIMP_RECTANGLE_OPTIONS (tool->tool_info->tool_options);
  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  if (! tool->gdisp)
    return;

  if (options_iface->get_highlight && options_iface->get_highlight (options))
    {
      GdkRectangle rect;

      rect.x      = tool_iface->get_x1 (rectangle);
      rect.y      = tool_iface->get_y1 (rectangle);
      rect.width  = tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle);
      rect.height = tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle);

      gimp_display_shell_set_highlight (shell, &rect);
    }

  gimp_display_shell_transform_xy (shell,
                                   tool_iface->get_x1 (rectangle), tool_iface->get_y1 (rectangle),
                                   &dx1, &dy1,
                                   FALSE);
  gimp_display_shell_transform_xy (shell,
                                   tool_iface->get_x2 (rectangle), tool_iface->get_y2 (rectangle),
                                   &dx2, &dy2,
                                   FALSE);
  tool_iface->set_dx1 (rectangle, dx1);
  tool_iface->set_dy1 (rectangle, dy1);
  tool_iface->set_dx2 (rectangle, dx2);
  tool_iface->set_dy2 (rectangle, dy2);

#define SRW 10
#define SRH 10

  tool_iface->set_dcw (rectangle, ((tool_iface->get_dx2 (rectangle) - tool_iface->get_dx1 (rectangle)) < SRW) ?
    (tool_iface->get_dx2 (rectangle) - tool_iface->get_dx1 (rectangle)) : SRW);

  tool_iface->set_dch (rectangle, ((tool_iface->get_dy2 (rectangle) - tool_iface->get_dy1 (rectangle)) < SRH) ?
    (tool_iface->get_dy2 (rectangle) - tool_iface->get_dy1 (rectangle)) : SRH);

#undef SRW
#undef SRH

  rectangle_info_update (rectangle);
}

static void
rectangle_tool_start (GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);

  rectangle_recalc (rectangle);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, tool->gdisp,
                                _("Rectangle: "), 0, " x ", 0);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->gdisp);
}

static void
rectangle_info_update (GimpRectangleTool *rectangle)
{
  GimpRectangleToolInterface *tool_iface;
  
  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  g_return_if_fail (tool_iface->get_x1 && tool_iface->get_y1
      && tool_iface->get_x2 && tool_iface->get_y2
      && tool_iface->set_origx && tool_iface->set_origy
      && tool_iface->set_sizew && tool_iface->set_sizeh);

  tool_iface->set_origx (rectangle, tool_iface->get_x1 (rectangle));
  tool_iface->set_origy (rectangle, tool_iface->get_y1 (rectangle));
  tool_iface->set_sizew (rectangle, tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle));
  tool_iface->set_sizeh (rectangle, tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle));
}

static void
rectangle_response (GtkWidget         *widget,
                    gint               response_id,
                    GimpRectangleTool *rectangle)
{
  GimpTool                   *tool = GIMP_TOOL (rectangle);
  GimpRectangleToolInterface *tool_iface;
  gboolean                    finish = TRUE;

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  if (response_id == GIMP_RECTANGLE_MODE_EXECUTE)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      g_return_if_fail (tool_iface->get_x1 && tool_iface->get_y1
        && tool_iface->get_x2 && tool_iface->get_y2);

      finish = gimp_rectangle_tool_execute (GIMP_RECTANGLE_TOOL (tool),
                                            tool_iface->get_x1 (rectangle), tool_iface->get_y1 (rectangle),
                                            tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle),
                                            tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle));

      rectangle_recalc (rectangle);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
    }

  if (finish)
    {
      gimp_display_shell_set_highlight (GIMP_DISPLAY_SHELL (tool->gdisp->shell),
                                        NULL);

      if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (rectangle)))
        gimp_draw_tool_stop (GIMP_DRAW_TOOL (rectangle));

      if (gimp_tool_control_is_active (GIMP_TOOL (rectangle)->control))
        gimp_tool_control_halt (GIMP_TOOL (rectangle)->control);

      gimp_image_flush (tool->gdisp->gimage);

      tool->gdisp    = NULL;
      tool->drawable = NULL;

    }
}

static void
rectangle_selection_callback (GtkWidget         *widget,
                              GimpRectangleTool *rectangle)
{
  GimpRectangleToolInterface *tool_iface;
  GimpRectangleOptions       *options;
  GimpDisplay                *gdisp;
  gint                        x1, y1;
  gint                        x2, y2;

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  g_return_if_fail (tool_iface->set_x1 && tool_iface->set_y1
      && tool_iface->set_x2 && tool_iface->set_y2);
  
  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL (rectangle)->tool_info->tool_options);
  gdisp   = GIMP_TOOL (rectangle)->gdisp;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

  if (gimp_channel_bounds (gimp_image_get_mask (gdisp->gimage),
                           &x1, &y1,
                           &x2, &y2))
    {
      tool_iface->set_x1 (rectangle, x1);
      tool_iface->set_y1 (rectangle, y1);
      tool_iface->set_x2 (rectangle, x2);
      tool_iface->set_y2 (rectangle, y2);
    }
  else
    {
      tool_iface->set_x1 (rectangle, 0);
      tool_iface->set_y1 (rectangle, 0);
      tool_iface->set_x2 (rectangle, gdisp->gimage->width);
      tool_iface->set_y2 (rectangle, gdisp->gimage->height);
    }

  rectangle_recalc (rectangle);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
}

static void
rectangle_automatic_callback (GtkWidget         *widget,
                              GimpRectangleTool *rectangle)
{
  GimpRectangleToolInterface *tool_iface;
  GimpRectangleOptions       *options;
  GimpDisplay                *gdisp;
  gint                        offset_x, offset_y;
  gint                        width, height;
  gint                        x1, y1, x2, y2;
  gint                        shrunk_x1;
  gint                        shrunk_y1;
  gint                        shrunk_x2;
  gint                        shrunk_y2;

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  g_return_if_fail (tool_iface->get_x1 && tool_iface->get_y1
      && tool_iface->get_x2 && tool_iface->get_y2
      && tool_iface->set_x1 && tool_iface->set_y1
      && tool_iface->set_x2 && tool_iface->set_y2);

  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL (rectangle)->tool_info->tool_options);
  gdisp   = GIMP_TOOL (rectangle)->gdisp;

  width    = gdisp->gimage->width;
  height   = gdisp->gimage->height;
  offset_x = 0;
  offset_y = 0;

  x1 = tool_iface->get_x1 (rectangle) - offset_x  > 0      ? tool_iface->get_x1 (rectangle) - offset_x : 0;
  x2 = tool_iface->get_x2 (rectangle) - offset_x  < width  ? tool_iface->get_x2 (rectangle) - offset_x : width;
  y1 = tool_iface->get_y1 (rectangle) - offset_y  > 0      ? tool_iface->get_y1 (rectangle) - offset_y : 0;
  y2 = tool_iface->get_y2 (rectangle) - offset_y  < height ? tool_iface->get_y2 (rectangle) - offset_y : height;

  if (gimp_image_crop_auto_shrink (gdisp->gimage,
                                   x1, y1, x2, y2,
                                   FALSE,
                                   &shrunk_x1,
                                   &shrunk_y1,
                                   &shrunk_x2,
                                   &shrunk_y2))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      tool_iface->set_x1 (rectangle, offset_x + shrunk_x1);
      tool_iface->set_x2 (rectangle, offset_x + shrunk_x2);
      tool_iface->set_y1 (rectangle, offset_y + shrunk_y1);
      tool_iface->set_y2 (rectangle, offset_y + shrunk_y2);

      rectangle_recalc (rectangle);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
    }
}

static void
rectangle_dimensions_changed (GtkWidget         *widget,
                              GimpRectangleTool *rectangle)
{
  GimpRectangleToolInterface *tool_iface;
  GimpSizeEntry              *entry = GIMP_SIZE_ENTRY (widget);
  gdouble                     x1, y1, x2, y2;
  GimpCoords                  coords;

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  g_return_if_fail (tool_iface->get_x1 && tool_iface->get_y1
      && tool_iface->get_x2 && tool_iface->get_y2
      && tool_iface->set_startx && tool_iface->set_starty
      && tool_iface->set_function);
  
  if (! GIMP_TOOL (rectangle)->gdisp)
    return;

  x1 = gimp_size_entry_get_refval (entry, 2);
  y1 = gimp_size_entry_get_refval (entry, 3);
  x2 = gimp_size_entry_get_refval (entry, 1);
  y2 = gimp_size_entry_get_refval (entry, 0);

  if (x1 != tool_iface->get_x1 (rectangle))
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_LEFT);
      coords.x = x1;
      coords.y = y1;
      tool_iface->set_startx (rectangle, tool_iface->get_x1 (rectangle));
      tool_iface->set_starty (rectangle, tool_iface->get_y1 (rectangle));
    }
  else if (y1 != tool_iface->get_y1 (rectangle))
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_TOP);
      coords.x = x1;
      coords.y = y1;
      tool_iface->set_startx (rectangle, tool_iface->get_x1 (rectangle));
      tool_iface->set_starty (rectangle, tool_iface->get_y1 (rectangle));
    }
  else if (x2 != tool_iface->get_x2 (rectangle))
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_RIGHT);
      coords.x = x2;
      coords.y = y2;
      tool_iface->set_startx (rectangle, tool_iface->get_x2 (rectangle));
      tool_iface->set_starty (rectangle, tool_iface->get_y2 (rectangle));
    }
  else if (y2 != tool_iface->get_y2 (rectangle))
    {
      tool_iface->set_function (rectangle, RECT_RESIZING_BOTTOM);
      coords.x = x2;
      coords.y = y2;
      tool_iface->set_startx (rectangle, tool_iface->get_x2 (rectangle));
      tool_iface->set_starty (rectangle, tool_iface->get_y2 (rectangle));
    }
  else
    return;

  /* use the motion handler to handle this, to avoid duplicating
     a bunch of code */
  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->gdisp);
}

gboolean
gimp_rectangle_tool_execute (GimpRectangleTool *rectangle,
                             gint               x,
                             gint               y,
                             gint               w,
                             gint               h)
{
  GimpRectangleToolInterface *tool_iface;
  GimpTool                   *tool             = GIMP_TOOL (rectangle);
  gboolean                    rectangle_exists;
  GimpChannel                *selection_mask;
  gint                        val;
  gboolean                    selected;
  GimpImage                  *gimage;
  gint                        x1, y1;
  gint                        x2, y2;

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  g_return_val_if_fail (tool_iface->set_x1 && tool_iface->set_y1
      && tool_iface->set_x2 && tool_iface->set_y2
      && tool_iface->get_x1 && tool_iface->get_y1
      && tool_iface->get_x2 && tool_iface->get_y2
      && tool_iface->get_pressx && tool_iface->get_pressy
      && tool_iface->get_function && tool_iface->execute, FALSE);

  rectangle_exists = (tool_iface->get_function (rectangle) != RECT_CREATING);
  
  gimage = tool->gdisp->gimage;
  selection_mask = gimp_image_get_mask (gimage);
  val = gimp_pickable_get_opacity_at (GIMP_PICKABLE (selection_mask),
                                      tool_iface->get_pressx (rectangle), tool_iface->get_pressy (rectangle));

  selected = (val > 127);

  if (rectangle_exists && selected)
    {
      if (gimp_channel_bounds (selection_mask,
                               &x1, &y1,
                               &x2, &y2))
        {
          tool_iface->set_x1 (rectangle, x1);
          tool_iface->set_y1 (rectangle, y1);
          tool_iface->set_x2 (rectangle, x2);
          tool_iface->set_y2 (rectangle, y2);
        }
      else
        {
          tool_iface->set_x1 (rectangle, 0);
          tool_iface->set_y1 (rectangle, 0);
          tool_iface->set_x2 (rectangle, gimage->width);
          tool_iface->set_y2 (rectangle, gimage->height);
        }

      return FALSE;
    }

  tool_iface->execute (rectangle,
      tool_iface->get_x1 (rectangle),
      tool_iface->get_y1 (rectangle),
      tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle),
      tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle));
  return TRUE;
}

static void
gimp_rectangle_tool_update_options (GimpRectangleTool *rectangle,
                                    GimpDisplay       *gdisp)
{
  GimpRectangleToolInterface    *tool_iface;
  GimpDisplayShell              *shell;
  gdouble                        width;
  gdouble                        height;
  gdouble                        aspect;
  gdouble                        center_x, center_y;
  GimpSizeEntry                 *entry;
  GimpRectangleOptions          *options;
  GimpRectangleOptionsInterface *options_iface;

  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  g_return_if_fail (tool_iface->get_x1 && tool_iface->get_y1
      && tool_iface->get_x2 && tool_iface->get_y2);
  
  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  entry   = GIMP_SIZE_ENTRY (tool_iface->get_entry (rectangle));
  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL (rectangle)->tool_info->tool_options);
  options_iface = GIMP_RECTANGLE_OPTIONS_GET_INTERFACE (options);

  width  = tool_iface->get_x2 (rectangle) - tool_iface->get_x1 (rectangle);
  height = tool_iface->get_y2 (rectangle) - tool_iface->get_y1 (rectangle);

  if (width > 0.01)
    aspect = height / width;
  else
    aspect = 0;

  center_x = (tool_iface->get_x1 (rectangle) + tool_iface->get_x2 (rectangle)) / 2;
  center_y = (tool_iface->get_y1 (rectangle) + tool_iface->get_y2 (rectangle)) / 2;

  g_signal_handlers_block_by_func (tool_iface->get_entry (rectangle),
                                   rectangle_dimensions_changed,
                                   rectangle);

  gimp_size_entry_set_refval (entry, 0, tool_iface->get_y2 (rectangle));
  gimp_size_entry_set_refval (entry, 1, tool_iface->get_x2 (rectangle));
  gimp_size_entry_set_refval (entry, 2, tool_iface->get_x1 (rectangle));
  gimp_size_entry_set_refval (entry, 3, tool_iface->get_y1 (rectangle));

  if (!options_iface->get_fixed_width (options))
    g_object_set (options, "width",  width, NULL);

  if (!options_iface->get_fixed_height (options))
    g_object_set (options, "height", height, NULL);

  if (!options_iface->get_fixed_aspect (options))
    g_object_set (options, "aspect", aspect, NULL);

  if (!options_iface->get_fixed_center (options))
    g_object_set (options,
                  "center-x", center_x,
                  "center-y", center_y,
                  NULL);

  g_signal_handlers_unblock_by_func (tool_iface->get_entry (rectangle),
                                     rectangle_dimensions_changed,
                                     rectangle);
}

static GtkWidget *
gimp_rectangle_controls (GimpRectangleTool *rectangle)
{
  GimpRectangleToolInterface *tool_iface;
  GtkWidget                  *vbox;
  GtkObject                  *adjustment;
  GtkWidget                  *spinbutton;
  GimpRectangleOptions       *options;
  GtkWidget                  *entry;
  
  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  g_return_val_if_fail (tool_iface->set_entry, NULL);
  
  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL (rectangle)->tool_info->tool_options);

  vbox = gtk_vbox_new (FALSE, 0);

  entry = gimp_size_entry_new (2, GIMP_UNIT_PIXEL, "%a",
                               TRUE, TRUE, FALSE, 4,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);
  tool_iface->set_entry (rectangle, entry);

  gtk_table_set_row_spacings (GTK_TABLE (entry), 3);
  gtk_table_set_col_spacings (GTK_TABLE (entry), 3);

  spinbutton = gimp_spin_button_new (&adjustment, 1, 0, 1, 1, 10, 1, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 4);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), spinbutton, 1, 2, 2, 3);
  gtk_widget_show (spinbutton);

  spinbutton = gimp_spin_button_new (&adjustment, 1, 0, 1, 1, 10, 1, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 4);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), spinbutton, 2, 3, 2, 3);
  gtk_widget_show (spinbutton);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("1 "), 1, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("2 "), 2, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("X"), 0, 1, 0.5);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("Y"), 0, 2, 0.5);

  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (rectangle_dimensions_changed),
                    rectangle);

  gtk_widget_show (entry);

  gimp_rectangle_tool_zero_controls (rectangle);

  gtk_widget_show (vbox);

  return vbox;
}

static void
gimp_rectangle_tool_zero_controls (GimpRectangleTool *rectangle)
{
  GimpRectangleToolInterface    *tool_iface;
  GimpSizeEntry                 *entry;
  
  tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);
  g_return_if_fail (tool_iface->get_entry);

  entry = GIMP_SIZE_ENTRY (tool_iface->get_entry (rectangle));

  gimp_size_entry_set_refval (entry, 0, 0);
  gimp_size_entry_set_refval (entry, 1, 0);
  gimp_size_entry_set_refval (entry, 2, 0);
  gimp_size_entry_set_refval (entry, 3, 0);
}
