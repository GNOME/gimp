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

#include "libgimpbase/gimpbase.h"
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


#define GIMP_RECTANGLE_TOOL_GET_PRIVATE(obj) (gimp_rectangle_tool_get_private ((GimpRectangleTool *) (obj)))

typedef struct _GimpRectangleToolPrivate GimpRectangleToolPrivate;

struct _GimpRectangleToolPrivate
{
  GtkWidget *controls;
  GtkWidget *dimensions_entry;

  gint       startx;     /*  starting x coord            */
  gint       starty;     /*  starting y coord            */

  gint       lastx;      /*  previous x coord            */
  gint       lasty;      /*  previous y coord            */

  gint       pressx;     /*  x where button pressed      */
  gint       pressy;     /*  y where button pressed      */

  gint       x1, y1;     /*  upper left hand coordinate  */
  gint       x2, y2;     /*  lower right hand coords     */

  guint      function;   /*  moving or resizing          */

  gint       dx1, dy1;   /*  display coords              */
  gint       dx2, dy2;   /*                              */

  gint       dcw, dch;   /*  width and height of edges   */

  gdouble    origx, origy;
  gdouble    sizew, sizeh;
};


static void    gimp_rectangle_tool_iface_base_init    (GimpRectangleToolInterface *rectangle_tool_iface);

static GimpRectangleToolPrivate *
               gimp_rectangle_tool_get_private        (GimpRectangleTool *tool);


/*  Rectangle helper functions  */
static void     rectangle_tool_start                      (GimpRectangleTool *rectangle);

/*  Rectangle dialog functions  */
static void     rectangle_info_update                     (GimpRectangleTool *rectangle);

static void     rectangle_selection_callback              (GtkWidget         *widget,
                                                           GimpRectangleTool *rectangle);
static void     rectangle_automatic_callback              (GtkWidget         *widget,
                                                           GimpRectangleTool *rectangle);

static void     rectangle_dimensions_changed              (GtkWidget         *widget,
                                                           GimpRectangleTool *rectangle);

static void     gimp_rectangle_tool_update_options    (GimpRectangleTool     *rectangle,
                                                       GimpDisplay           *gdisp);
static GtkWidget * gimp_rectangle_controls            (GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_zero_controls     (GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_width      (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_height     (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_aspect     (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);


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
gimp_rectangle_tool_iface_base_init (GimpRectangleToolInterface *tool_iface)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      g_object_interface_install_property (tool_iface,
        g_param_spec_object ("controls",
                             NULL, NULL,
                             GTK_TYPE_WIDGET,
                             G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_object ("dimensions-entry",
                             NULL, NULL,
                             GTK_TYPE_WIDGET,
                             G_PARAM_READWRITE));

      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("startx",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("starty",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));

      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("lastx",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("lasty",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));

      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("pressx",
                          NULL, NULL,
                          0, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("pressy",
                          NULL, NULL,
                          0, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));

      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("x1",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("y1",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("x2",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("y2",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));

      g_object_interface_install_property (tool_iface,
        g_param_spec_uint ("function",
                           NULL, NULL,
                           RECT_CREATING, RECT_EXECUTING,
                           0,
                           G_PARAM_READWRITE));

      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("dx1",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("dy1",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("dx2",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("dy2",
                          NULL, NULL,
                          -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));

      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("dcw",
                          NULL, NULL,
                          0, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_int ("dch",
                          NULL, NULL,
                          0, GIMP_MAX_IMAGE_SIZE,
                          0,
                          G_PARAM_READWRITE));

      g_object_interface_install_property (tool_iface,
        g_param_spec_double ("origx",
                             NULL, NULL,
                             -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                             0.0,
                             G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_double ("origy",
                             NULL, NULL,
                             -GIMP_MAX_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                             0.0,
                             G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_double ("sizew",
                             NULL, NULL,
                             0.0, GIMP_MAX_IMAGE_SIZE,
                             0.0,
                             G_PARAM_READWRITE));
      g_object_interface_install_property (tool_iface,
        g_param_spec_double ("sizeh",
                             NULL, NULL,
                             0.0, GIMP_MAX_IMAGE_SIZE,
                             0.0,
                             G_PARAM_READWRITE));

      initialized = TRUE;
    }
}

static void
gimp_rectangle_tool_private_finalize (GimpRectangleToolPrivate *private)
{
  g_free (private);
}

static GimpRectangleToolPrivate *
gimp_rectangle_tool_get_private (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("gimp-rectangle-tool-private");

  private = g_object_get_qdata ((GObject *) tool, private_key);

  if (! private)
    {
      private = g_new0 (GimpRectangleToolPrivate, 1);

      g_object_set_qdata_full ((GObject *) tool, private_key, private,
                               (GDestroyNotify) gimp_rectangle_tool_private_finalize);
    }

  return private;
}

/**
 * gimp_rectangle_tool_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #GimpToolOptions. A #GimpRectangleToolProp property is installed
 * for each property, using the values from the #GimpRectangleToolProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don't collide with some other property values they
 * are using (that's what %GIMP_RECTANGLE_TOOL_PROP_LAST is good for).
 **/
void
gimp_rectangle_tool_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_CONTROLS,
                                    "controls");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_DIMENSIONS_ENTRY,
                                    "dimensions-entry");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_STARTX,
                                    "startx");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_STARTY,
                                    "starty");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_LASTX,
                                    "lastx");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_LASTY,
                                    "lasty");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_PRESSX,
                                    "pressx");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_PRESSY,
                                    "pressy");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_X1,
                                    "x1");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_Y1,
                                    "y1");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_X2,
                                    "x2");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_Y2,
                                    "y2");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_FUNCTION,
                                    "function");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_DX1,
                                    "dx1");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_DY1,
                                    "dy1");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_DX2,
                                    "dx2");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_DY2,
                                    "dy2");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_DCW,
                                    "dcw");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_DCH,
                                    "dch");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_ORIGX,
                                    "origx");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_ORIGY,
                                    "origy");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_SIZEW,
                                    "sizew");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_SIZEH,
                                    "sizeh");
}

void
gimp_rectangle_tool_set_controls (GimpRectangleTool *tool,
                                  GtkWidget         *controls)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->controls = controls;
  g_object_notify (G_OBJECT (tool), "controls");
}

GtkWidget *
gimp_rectangle_tool_get_controls (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), NULL);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->controls;
}

void
gimp_rectangle_tool_set_entry (GimpRectangleTool *tool,
                               GtkWidget         *entry)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->dimensions_entry = entry;
  g_object_notify (G_OBJECT (tool), "dimensions-entry");
}

GtkWidget *
gimp_rectangle_tool_get_entry (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), NULL);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->dimensions_entry;
}

void
gimp_rectangle_tool_set_startx (GimpRectangleTool *tool,
                                gint               startx)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->startx = startx;
  g_object_notify (G_OBJECT (tool), "startx");
}

gint
gimp_rectangle_tool_get_startx (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->startx;
}

void
gimp_rectangle_tool_set_starty (GimpRectangleTool *tool,
                                gint               starty)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->starty = starty;
  g_object_notify (G_OBJECT (tool), "starty");
}

gint
gimp_rectangle_tool_get_starty (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->starty;
}

void
gimp_rectangle_tool_set_lastx (GimpRectangleTool *tool,
                               gint               lastx)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->lastx = lastx;
  g_object_notify (G_OBJECT (tool), "lastx");
}

gint
gimp_rectangle_tool_get_lastx (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->lastx;
}

void
gimp_rectangle_tool_set_lasty (GimpRectangleTool *tool,
                               gint               lasty)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->lasty = lasty;
  g_object_notify (G_OBJECT (tool), "lasty");
}

gint
gimp_rectangle_tool_get_lasty (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->lasty;
}

void
gimp_rectangle_tool_set_pressx (GimpRectangleTool *tool,
                                gint               pressx)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->pressx = pressx;
  g_object_notify (G_OBJECT (tool), "pressx");
}

gint
gimp_rectangle_tool_get_pressx (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->pressx;
}

void
gimp_rectangle_tool_set_pressy (GimpRectangleTool *tool,
                                gint               pressy)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->pressy = pressy;
  g_object_notify (G_OBJECT (tool), "pressy");
}

gint
gimp_rectangle_tool_get_pressy (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->pressy;
}

void
gimp_rectangle_tool_set_x1 (GimpRectangleTool *tool,
                            gint               x1)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->x1 = x1;
  g_object_notify (G_OBJECT (tool), "x1");
}

gint
gimp_rectangle_tool_get_x1 (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->x1;
}

void
gimp_rectangle_tool_set_y1 (GimpRectangleTool *tool,
                            gint               y1)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->y1 = y1;
  g_object_notify (G_OBJECT (tool), "y1");
}

gint
gimp_rectangle_tool_get_y1 (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->y1;
}

void
gimp_rectangle_tool_set_x2 (GimpRectangleTool *tool,
                            gint               x2)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->x2 = x2;
  g_object_notify (G_OBJECT (tool), "x2");
}

gint
gimp_rectangle_tool_get_x2 (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->x2;
}

void
gimp_rectangle_tool_set_y2 (GimpRectangleTool *tool,
                            gint               y2)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->y2 = y2;
  g_object_notify (G_OBJECT (tool), "y2");
}

gint
gimp_rectangle_tool_get_y2 (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->y2;
}

void
gimp_rectangle_tool_set_function (GimpRectangleTool *tool,
                                  guint              function)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->function = function;
  g_object_notify (G_OBJECT (tool), "function");
}

guint
gimp_rectangle_tool_get_function (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->function;
}

void
gimp_rectangle_tool_set_dx1 (GimpRectangleTool *tool,
                             gint               dx1)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->dx1 = dx1;
  g_object_notify (G_OBJECT (tool), "dx1");
}

gint
gimp_rectangle_tool_get_dx1 (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->dx1;
}

void
gimp_rectangle_tool_set_dy1 (GimpRectangleTool *tool,
                             gint               dy1)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->dy1 = dy1;
  g_object_notify (G_OBJECT (tool), "dy1");
}

gint
gimp_rectangle_tool_get_dy1 (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->dy1;
}

void
gimp_rectangle_tool_set_dx2 (GimpRectangleTool *tool,
                             gint               dx2)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->dx2 = dx2;
  g_object_notify (G_OBJECT (tool), "dx2");
}

gint
gimp_rectangle_tool_get_dx2 (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->dx2;
}

void
gimp_rectangle_tool_set_dy2 (GimpRectangleTool *tool,
                             gint               dy2)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->dy2 = dy2;
  g_object_notify (G_OBJECT (tool), "dy2");
}

gint
gimp_rectangle_tool_get_dy2 (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->dy2;
}

void
gimp_rectangle_tool_set_dcw (GimpRectangleTool *tool,
                             gint               dcw)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->dcw = dcw;
  g_object_notify (G_OBJECT (tool), "dcw");
}

gint
gimp_rectangle_tool_get_dcw (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->dcw;
}

void
gimp_rectangle_tool_set_dch (GimpRectangleTool *tool,
                             gint               dch)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->dch = dch;
  g_object_notify (G_OBJECT (tool), "dch");
}

gint
gimp_rectangle_tool_get_dch (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->dch;
}

void
gimp_rectangle_tool_set_origx (GimpRectangleTool *tool,
                               gdouble            origx)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->origx = origx;
  g_object_notify (G_OBJECT (tool), "origx");
}

gdouble
gimp_rectangle_tool_get_origx (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0.0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->origx;
}

void
gimp_rectangle_tool_set_origy (GimpRectangleTool *tool,
                               gdouble            origy)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->origy = origy;
  g_object_notify (G_OBJECT (tool), "origy");
}

gdouble
gimp_rectangle_tool_get_origy (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0.0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->origy;
}

void
gimp_rectangle_tool_set_sizew (GimpRectangleTool *tool,
                               gdouble            sizew)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->sizew = sizew;
  g_object_notify (G_OBJECT (tool), "sizew");
}

gdouble
gimp_rectangle_tool_get_sizew (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0.0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->sizew;
}

void
gimp_rectangle_tool_set_sizeh (GimpRectangleTool *tool,
                               gdouble            sizeh)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->sizeh = sizeh;
  g_object_notify (G_OBJECT (tool), "sizeh");
}

gdouble
gimp_rectangle_tool_get_sizeh (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0.0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->sizeh;
}

void
gimp_rectangle_tool_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpRectangleTool *tool = GIMP_RECTANGLE_TOOL (object);

  switch (property_id)
    {
    case GIMP_RECTANGLE_TOOL_PROP_CONTROLS:
      gimp_rectangle_tool_set_controls (tool, g_value_get_object (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DIMENSIONS_ENTRY:
      gimp_rectangle_tool_set_entry (tool, g_value_get_object (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_STARTX:
      gimp_rectangle_tool_set_startx (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_STARTY:
      gimp_rectangle_tool_set_starty (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_LASTX:
      gimp_rectangle_tool_set_lastx (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_LASTY:
      gimp_rectangle_tool_set_lasty (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_PRESSX:
      gimp_rectangle_tool_set_pressx (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_PRESSY:
      gimp_rectangle_tool_set_pressy (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_X1:
      gimp_rectangle_tool_set_x1 (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_Y1:
      gimp_rectangle_tool_set_y1 (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_X2:
      gimp_rectangle_tool_set_x2 (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_Y2:
      gimp_rectangle_tool_set_y2 (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_FUNCTION:
      gimp_rectangle_tool_set_function (tool, g_value_get_uint (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DX1:
      gimp_rectangle_tool_set_dx1 (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DY1:
      gimp_rectangle_tool_set_dy1 (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DX2:
      gimp_rectangle_tool_set_dx2 (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DY2:
      gimp_rectangle_tool_set_dy2 (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DCW:
      gimp_rectangle_tool_set_dcw (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DCH:
      gimp_rectangle_tool_set_dch (tool, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_ORIGX:
      gimp_rectangle_tool_set_origx (tool, g_value_get_double (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_ORIGY:
      gimp_rectangle_tool_set_origy (tool, g_value_get_double (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_SIZEW:
      gimp_rectangle_tool_set_sizew (tool, g_value_get_double (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_SIZEH:
      gimp_rectangle_tool_set_sizeh (tool, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_rectangle_tool_get_property (GObject      *object,
                                  guint         property_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  GimpRectangleTool *tool = GIMP_RECTANGLE_TOOL (object);

  switch (property_id)
    {
    case GIMP_RECTANGLE_TOOL_PROP_CONTROLS:
      g_value_set_object (value, gimp_rectangle_tool_get_controls (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DIMENSIONS_ENTRY:
      g_value_set_object (value, gimp_rectangle_tool_get_entry (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_STARTX:
      g_value_set_int (value, gimp_rectangle_tool_get_startx (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_STARTY:
      g_value_set_int (value, gimp_rectangle_tool_get_starty (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_LASTX:
      g_value_set_int (value, gimp_rectangle_tool_get_lastx (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_LASTY:
      g_value_set_int (value, gimp_rectangle_tool_get_lasty (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_PRESSX:
      g_value_set_int (value, gimp_rectangle_tool_get_pressx (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_PRESSY:
      g_value_set_int (value, gimp_rectangle_tool_get_pressy (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_X1:
      g_value_set_int (value, gimp_rectangle_tool_get_x1 (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_Y1:
      g_value_set_int (value, gimp_rectangle_tool_get_y1 (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_X2:
      g_value_set_int (value, gimp_rectangle_tool_get_x2 (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_Y2:
      g_value_set_int (value, gimp_rectangle_tool_get_y2 (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_FUNCTION:
      g_value_set_uint (value, gimp_rectangle_tool_get_function (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DX1:
      g_value_set_int (value, gimp_rectangle_tool_get_dx1 (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DY1:
      g_value_set_int (value, gimp_rectangle_tool_get_dy1 (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DX2:
      g_value_set_int (value, gimp_rectangle_tool_get_dx2 (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DY2:
      g_value_set_int (value, gimp_rectangle_tool_get_dy2 (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DCW:
      g_value_set_int (value, gimp_rectangle_tool_get_dcw (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_DCH:
      g_value_set_int (value, gimp_rectangle_tool_get_dch (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_ORIGX:
      g_value_set_double (value, gimp_rectangle_tool_get_origx (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_ORIGY:
      g_value_set_double (value, gimp_rectangle_tool_get_origy (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_SIZEW:
      g_value_set_double (value, gimp_rectangle_tool_get_sizew (tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_SIZEH:
      g_value_set_double (value, gimp_rectangle_tool_get_sizeh (tool));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_rectangle_tool_constructor (GObject *object)
{
  GimpTool          *tool;
  GimpRectangleTool *rectangle;
  GtkContainer      *controls_container;
  GtkWidget         *controls;
  GObject           *options;

  tool       = GIMP_TOOL (object);
  rectangle  = GIMP_RECTANGLE_TOOL (object);

  tool->gdisp = NULL;

  g_assert (GIMP_IS_TOOL_INFO (tool->tool_info));

  options = G_OBJECT (tool->tool_info->tool_options);

  g_signal_connect_object (options, "notify::width",
                           G_CALLBACK (gimp_rectangle_tool_notify_width),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::height",
                           G_CALLBACK (gimp_rectangle_tool_notify_height),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::aspect",
                           G_CALLBACK (gimp_rectangle_tool_notify_aspect),
                           rectangle, 0);

  controls_container = GTK_CONTAINER (g_object_get_data (options,
                                                         "controls-container"));

  controls = gimp_rectangle_controls (rectangle);
  g_object_set (tool, "controls", controls, NULL);
  gtk_container_add (controls_container, controls);
}

void
gimp_rectangle_tool_dispose (GObject *object)
{
  GimpTool          *tool      = GIMP_TOOL (object);
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (object);
  GtkWidget         *controls;
  GObject           *options;

  g_object_get (rectangle, "controls", &controls, NULL);
  options = G_OBJECT (tool->tool_info->tool_options);

  if (controls)
    {
      gtk_widget_destroy (controls);
      g_free (controls);
      g_object_set (rectangle, "controls", NULL, NULL);
    }

  g_signal_handlers_disconnect_by_func (options,
                                        G_CALLBACK (gimp_rectangle_tool_notify_width),
                                        rectangle);
  g_signal_handlers_disconnect_by_func (options,
                                        G_CALLBACK (gimp_rectangle_tool_notify_height),
                                        rectangle);
  g_signal_handlers_disconnect_by_func (options,
                                        G_CALLBACK (gimp_rectangle_tool_notify_aspect),
                                        rectangle);
}

gboolean
gimp_rectangle_tool_initialize (GimpTool    *tool,
                                GimpDisplay *gdisp)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpSizeEntry     *entry;

  g_object_get (rectangle, "dimensions-entry", &entry, NULL);

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
gimp_rectangle_tool_button_press (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
  GimpRectangleTool    *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpDrawTool         *draw_tool = GIMP_DRAW_TOOL (tool);

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  if (gdisp != tool->gdisp)
    {
      if (gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_stop (draw_tool);

      g_object_set (rectangle, "function", RECT_CREATING, NULL);
      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);

      tool->gdisp = gdisp;

      g_object_set (rectangle,
          "x1", ROUND (coords->x),
          "y1", ROUND (coords->y),
          "x2", ROUND (coords->x),
          "y2", ROUND (coords->y),
          NULL);

      rectangle_tool_start (rectangle);
    }

  g_object_set (rectangle,
      "pressx", ROUND (coords->x),
      "pressy", ROUND (coords->y),
      "lastx", ROUND (coords->x),
      "lasty", ROUND (coords->y),
      "startx", ROUND (coords->x),
      "starty", ROUND (coords->y),
      NULL);

  gimp_tool_control_activate (tool->control);
}

void
gimp_rectangle_tool_button_release (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  guint              function;
  gint               lastx, lasty;
  gint               pressx, pressy;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  g_object_get (rectangle, "function", &function, NULL);
  if (function == RECT_EXECUTING)
    gimp_tool_pop_status (tool, gdisp);

  if (! (state & GDK_BUTTON3_MASK))
    {
      g_object_get (rectangle,
          "lastx", &lastx,
          "lasty", &lasty,
          "pressx", &pressx,
          "pressy", &pressy,
          NULL);
      if (lastx == pressx && lasty == pressy)
        {
          gimp_rectangle_tool_response (NULL, GIMP_RECTANGLE_MODE_EXECUTE,
                                        rectangle);
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
  GimpRectangleTool    *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleOptions *options;
  guint                 function;
  gint                  x1, y1, x2, y2;
  gint                  curx, cury;
  gint                  inc_x, inc_y;
  gint                  lastx, lasty;
  gint                  min_x, min_y, max_x, max_y;
  gint                  rx1, ry1, rx2, ry2;
  gboolean              fixed_width;
  gboolean              fixed_height;
  gboolean              fixed_aspect;
  gboolean              fixed_center;
  gdouble               width, height;
  gdouble               center_x, center_y;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  /*  This is the only case when the motion events should be ignored--
      we're just waiting for the button release event to execute  */
  g_object_get (rectangle, "function", &function, NULL);
  if (function == RECT_EXECUTING)
    return;

  curx = ROUND (coords->x);
  cury = ROUND (coords->y);

  g_object_get (rectangle,
      "startx", &x1,
      "starty", &y1,
      NULL);
  x2 = curx;
  y2 = cury;

  inc_x = (x2 - x1);
  inc_y = (y2 - y1);

  /*  If there have been no changes... return  */
  g_object_get (rectangle,
      "lastx", &lastx,
      "lasty", &lasty,
      NULL);
  if (lastx == x2 && lasty == y2)
    return;

  options = GIMP_RECTANGLE_OPTIONS (tool->tool_info->tool_options);

  g_object_get (options,
      "new-fixed-width", &fixed_width,
      "new-fixed-height", &fixed_height,
      "fixed-aspect", &fixed_aspect,
      "fixed-center", &fixed_center,
      NULL);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  min_x = min_y = 0;
  max_x = gdisp->gimage->width;
  max_y = gdisp->gimage->height;

  g_object_get (options,
      "width", &width,
      "height", &height,
      "center-x", &center_x,
      "center-y", &center_y,
      NULL);

  g_object_get (rectangle,
      "x1", &rx1,
      "y1", &ry1,
      "x2", &rx2,
      "y2", &ry2,
      NULL);

  switch (function)
    {
    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LEFT:
      x1 = rx1 + inc_x;
      if (fixed_width)
        {
          x2 = x1 + width;
          if (x1 < 0)
            {
              x1 = 0;
              x2 = width;
            }
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - width;
            }
        }
      else if (fixed_center)
        {
          x2 = x1 + 2 * (center_x - x1);
          if (x1 < 0)
            {
              x1 = 0;
              x2 = 2 * center_x;
            }
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - 2 * (max_x - center_x);
            }
        }
      else
        {
          x2 = MAX (x1, rx2);
        }
      g_object_set (rectangle, "startx", curx, NULL);
      break;

    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_RIGHT:
      x2 = rx2 + inc_x;
      if (fixed_width)
        {
          x1 = x2 - width;
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - width;
            }
          if (x1 < 0)
            {
              x1 = 0;
              x2 = width;
            }
        }
      else if (fixed_center)
        {
          x1 = x2 - 2 * (x2 - center_x);
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - 2 * (max_x - center_x);
            }
          if (x1 < 0)
            {
              x1 = 0;
              x2 = 2 * center_x;
            }
        }
      else
        {
          x1 = MIN (rx1, x2);
        }
      g_object_set (rectangle, "startx", curx, NULL);
      break;

    case RECT_RESIZING_BOTTOM:
    case RECT_RESIZING_TOP:
      x1 = rx1;
      x2 = rx2;
      g_object_set (rectangle, "startx", curx, NULL);
      break;

    case RECT_MOVING:
      x1 = rx1 + inc_x;
      x2 = rx2 + inc_x;
      g_object_set (rectangle, "startx", curx, NULL);
      break;
    }

  switch (function)
    {
    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_TOP:
      y1 = ry1 + inc_y;
      if (fixed_height)
        {
          y2 = y1 + height;
          if (y1 < 0)
            {
              y1 = 0;
              y2 = height;
            }
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - height;
            }
        }
      else if (fixed_center)
        {
          y2 = y1 + 2 * (center_y - y1);
          if (y1 < 0)
            {
              y1 = 0;
              y2 = 2 * center_y;
            }
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - 2 * (max_y - center_y);
            }
        }
      else
        {
          y2 = MAX (y1, ry2);
        }
      g_object_set (rectangle, "starty", cury, NULL);
      break;

    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_BOTTOM:
      y2 = ry2 + inc_y;
      if (fixed_height)
        {
          y1 = y2 - height;
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - height;
            }
          if (y1 < 0)
            {
              y1 = 0;
              y2 = height;
            }
        }
      else if (fixed_center)
        {
          y1 = y2 - 2 * (y2 - center_y);
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - 2 * (max_y - center_y);
            }
          if (y1 < 0)
            {
              y1 = 0;
              y2 = 2 * center_y;
            }
        }
      else
        {
          y1 = MIN (ry1, y2);
        }
      g_object_set (rectangle, "starty", cury, NULL);
      break;

    case RECT_RESIZING_RIGHT:
    case RECT_RESIZING_LEFT:
      y1 = ry1;
      y2 = ry2;
      g_object_set (rectangle, "starty", cury, NULL);
      break;

    case RECT_MOVING:
      y1 = ry1 + inc_y;
      y2 = ry2 + inc_y;
      g_object_set (rectangle, "starty", cury, NULL);
      break;
    }

  if (fixed_aspect)
    {
      gdouble aspect;
      g_object_get (options,
        "aspect", &aspect,
        NULL);

      if (aspect > max_y)
        aspect = max_y;
      if (aspect < 1.0 / max_x)
        aspect = 1.0 / max_x;

      switch (function)
        {
        case RECT_RESIZING_UPPER_LEFT:
          if (inc_x == 0 || inc_y / inc_x > aspect)
            x1 = rx2 - (ry2 - y1) / aspect + .5;
          else
            y1 = ry2 - aspect * (rx2 - x1) + .5;
          break;

        case RECT_RESIZING_UPPER_RIGHT:
        case RECT_RESIZING_TOP:
          if (inc_x == 0 || inc_y / inc_x > aspect)
            x2 = rx1 + (ry2 - y1) / aspect + .5;
          else
            y1 = ry2 - aspect * (x2 - rx1) + .5;
          break;

        case RECT_RESIZING_LOWER_LEFT:
        case RECT_RESIZING_LEFT:
          if (inc_x == 0 || inc_y / inc_x > aspect)
            x1 = rx2 - (y2 - ry1) / aspect + .5;
          else
            y2 = ry1 + aspect * (rx2 - x1) + .5;
          break;

        case RECT_RESIZING_LOWER_RIGHT:
        case RECT_RESIZING_RIGHT:
        case RECT_RESIZING_BOTTOM:
          if (inc_x == 0 || inc_y / inc_x > aspect)
            x2 = rx1 + (y2 - ry1) / aspect + .5;
          else
            y2 = ry1 + aspect * (x2 - rx1) + .5;
          break;

        default:
          break;
        }
    }

  /*  make sure that the coords are in bounds  */
  g_object_set (rectangle,
      "x1", MIN (x1, x2),
      "y1", MIN (y1, y2),
      "x2", MAX (x1, x2),
      "y2", MAX (y1, y2),
      "lastx", curx,
      "lasty", cury,
      NULL);

  /*  recalculate the coordinates for rectangle_draw based on the new values  */
  gimp_rectangle_tool_configure (rectangle);

  g_object_get (rectangle,
      "x1", &rx1,
      "y1", &ry1,
      "x2", &rx2,
      "y2", &ry2,
      NULL);

  switch (function)
    {
    case RECT_RESIZING_UPPER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx1 - coords->x,
                                          ry1 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_UPPER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx2 - coords->x,
                                          ry1 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LOWER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx1 - coords->x,
                                          ry2 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LOWER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx2 - coords->x,
                                          ry2 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx1 - coords->x, 0, 0, 0);
      break;

    case RECT_RESIZING_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx2 - coords->x, 0, 0, 0);
      break;

    case RECT_RESIZING_TOP:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, ry1 - coords->y, 0, 0);
      break;

    case RECT_RESIZING_BOTTOM:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, ry2 - coords->y, 0, 0);
      break;

    case RECT_MOVING:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx1 - coords->x,
                                          ry1 - coords->y,
                                          rx2 - rx1,
                                          ry2 - ry1);
      break;

    default:
      break;
    }

  gimp_rectangle_tool_update_options (rectangle, gdisp);

  if (function == RECT_CREATING      ||
      function == RECT_RESIZING_UPPER_LEFT ||
      function == RECT_RESIZING_LOWER_RIGHT)
    {
      gimp_tool_pop_status (tool, gdisp);
      gimp_tool_push_status_coords (tool, gdisp,
                                    _("Rectangle: "),
                                    rx2 - rx1,
                                    " x ",
                                    ry2 - ry1);
    }

  if (function == RECT_CREATING)
    {
      if (inc_x < 0 && inc_y < 0)
        function = RECT_RESIZING_UPPER_LEFT;
      else if (inc_x < 0 && inc_y > 0)
        function = RECT_RESIZING_LOWER_LEFT;
      else if (inc_x > 0 && inc_y < 0)
        function = RECT_RESIZING_UPPER_RIGHT;
      else if (inc_x > 0 && inc_y > 0)
        function = RECT_RESIZING_LOWER_RIGHT;

      g_object_set (rectangle, "function", function, NULL);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

gboolean
gimp_rectangle_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *gdisp)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  gint               inc_x, inc_y;
  gint               min_x, min_y;
  gint               max_x, max_y;
  gint               x1, y1, x2, y2;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), FALSE);

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
      gimp_rectangle_tool_response (NULL, GIMP_RECTANGLE_MODE_EXECUTE, rectangle);
      return TRUE;

    case GDK_Escape:
      gimp_rectangle_tool_response (NULL, GTK_RESPONSE_CANCEL, rectangle);
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

  g_object_get (rectangle,
      "x1", &x1,
      "y1", &y1,
      "x2", &x2,
      "y2", &y2,
      NULL);

  g_object_set (rectangle,
      "x1", x1 + inc_x,
      "y1", y1 + inc_y,
      "x2", x2 + inc_x,
      "y2", y2 + inc_y,
      NULL);

  gimp_rectangle_tool_configure (rectangle);

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
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpDrawTool      *draw_tool = GIMP_DRAW_TOOL (tool);
  gint               x1, y1, x2, y2;
  gint               dcw, dch;
  gboolean           inside_x;
  gboolean           inside_y;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  if (tool->gdisp != gdisp)
    {
      return;
    }

  g_object_get (rectangle,
      "x1", &x1,
      "y1", &y1,
      "x2", &x2,
      "y2", &y2,
      "dcw", &dcw,
      "dch", &dch,
      NULL);

  inside_x = coords->x > x1 && coords->x < x2;
  inside_y = coords->y > y1 && coords->y < y2;

  if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                coords->x, coords->y,
                                GIMP_HANDLE_SQUARE,
                                x1, y1,
                                dcw, dch,
                                GTK_ANCHOR_NORTH_WEST,
                                FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_UPPER_LEFT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x1 - coords->x,
                                          y1 - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     x2, y2,
                                     dcw, dch,
                                     GTK_ANCHOR_SOUTH_EAST,
                                     FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_LOWER_RIGHT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x2 - coords->x,
                                          y2 - coords->y,
                                          0, 0);
    }
  else if  (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                      coords->x, coords->y,
                                      GIMP_HANDLE_SQUARE,
                                      x2, y1,
                                      dcw, dch,
                                      GTK_ANCHOR_NORTH_EAST,
                                      FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_UPPER_RIGHT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x2 - coords->x,
                                          y1 - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     x1, y2,
                                     dcw, dch,
                                     GTK_ANCHOR_SOUTH_WEST,
                                     FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_LOWER_LEFT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x1 - coords->x,
                                          y2 - coords->y,
                                          0, 0);
    }
  else if ( (fabs (coords->x - x1) < dcw) && inside_y)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_LEFT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x1 - coords->x, 0, 0, 0);

    }
  else if ( (fabs (coords->x - x2) < dcw) && inside_y)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_RIGHT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x2 - coords->x, 0, 0, 0);

    }
  else if ( (fabs (coords->y - y1) < dch) && inside_x)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_TOP, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, y1 - coords->y, 0, 0);

    }
  else if ( (fabs (coords->y - y2) < dch) && inside_x)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_BOTTOM, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, y2 - coords->y, 0, 0);

    }
  else if (inside_x && inside_y)
    {
      g_object_set (rectangle, "function", RECT_MOVING, NULL);
    }
  /*  otherwise, the new function will be creating, since we want
   *  to start a new rectangle
   */
  else
    {
      g_object_set (rectangle, "function", RECT_CREATING, NULL);

      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);
    }
}

void
gimp_rectangle_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *gdisp)
{
  GimpRectangleTool    *rectangle   = GIMP_RECTANGLE_TOOL (tool);
  GimpCursorType        cursor      = GDK_CROSSHAIR;
  GimpCursorModifier    modifier    = GIMP_CURSOR_MODIFIER_NONE;
  GimpToolCursorType    tool_cursor;
  guint                 function;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  tool_cursor = gimp_tool_control_get_tool_cursor (tool->control);

  if (tool->gdisp == gdisp && ! (state & GDK_BUTTON1_MASK))
    {
      g_object_get (rectangle, "function", &function, NULL);

      switch (function)
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
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (draw);
  GimpTool          *tool      = GIMP_TOOL (draw);
  GimpDisplayShell  *shell     = GIMP_DISPLAY_SHELL (tool->gdisp->shell);
  GimpCanvas        *canvas    = GIMP_CANVAS (shell->canvas);
  gint               dx1, dy1, dx2, dy2;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  g_object_get (rectangle,
      "dx1", &dx1,
      "dy1", &dy1,
      "dx2", &dx2,
      "dy2", &dy2,
      NULL);

  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         dx1, dy1,
                         dx2, dy1);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         dx1, dy2 - 1,
                         dx2, dy2 - 1);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         dx1, dy1 + 1,
                         dx1, dy2 - 1);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         dx2 - 1, dy1 + 1,
                         dx2 - 1, dy2 - 1);
}

void
gimp_rectangle_tool_configure (GimpRectangleTool *rectangle)
{
  GimpTool             *tool     = GIMP_TOOL (rectangle);
  GimpDisplayShell     *shell    = GIMP_DISPLAY_SHELL (tool->gdisp->shell);
  GimpRectangleOptions *options;
  gint                  dx1, dy1;
  gint                  dx2, dy2;
  gboolean              highlight;
  gint                  x1, y1;
  gint                  x2, y2;

  options = GIMP_RECTANGLE_OPTIONS (tool->tool_info->tool_options);

  if (! tool->gdisp)
    return;

  g_object_get (options, "highlight", &highlight, NULL);

  g_object_get (rectangle,
      "x1", &x1,
      "y1", &y1,
      "x2", &x2,
      "y2", &y2,
      NULL);

  if (highlight)
    {
      GdkRectangle rect;

      rect.x      = x1;
      rect.y      = y1;
      rect.width  = x2 - x1;
      rect.height = y2 - y1;

      gimp_display_shell_set_highlight (shell, &rect);
    }

  gimp_display_shell_transform_xy (shell,
                                   x1, y1,
                                   &dx1, &dy1,
                                   FALSE);
  gimp_display_shell_transform_xy (shell,
                                   x2, y2,
                                   &dx2, &dy2,
                                   FALSE);
  g_object_set (rectangle,
      "dx1", dx1,
      "dy1", dy1,
      "dx2", dx2,
      "dy2", dy2,
      NULL);

#define SRW 10
#define SRH 10

  g_object_set (rectangle,
      "dcw", ((dx2 - dx1) < SRW) ? (dx2 - dx1) : SRW,
      "dch", ((dy2 - dy1) < SRH) ? (dy2 - dy1) : SRH,
      NULL);

#undef SRW
#undef SRH

  rectangle_info_update (rectangle);
}

static void
rectangle_tool_start (GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);

  gimp_rectangle_tool_configure (rectangle);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, tool->gdisp,
                                _("Rectangle: "), 0, " x ", 0);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->gdisp);
}

static void
rectangle_info_update (GimpRectangleTool *rectangle)
{
  gint x1, y1, x2, y2;

  g_object_get (rectangle,
      "x1", &x1,
      "y1", &y1,
      "x2", &x2,
      "y2", &y2,
      NULL);

  g_object_set (rectangle,
      "origx", (gdouble) x1,
      NULL);
  g_object_set (rectangle,
      "origy", (gdouble) y1,
      NULL);
  g_object_set (rectangle,
      "sizew", (gdouble) (x2 - x1),
      NULL);
  g_object_set (rectangle,
      "sizeh", (gdouble) (y2 - y1),
      NULL);
}

void
gimp_rectangle_tool_response (GtkWidget         *widget,
                              gint               response_id,
                              GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);
  gboolean  finish = TRUE;
  gint      x1, y1, x2, y2;

  if (response_id == GIMP_RECTANGLE_MODE_EXECUTE)
    {
      g_object_get (rectangle,
        "x1", &x1,
        "y1", &y1,
        "x2", &x2,
        "y2", &y2,
        NULL);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      finish = gimp_rectangle_tool_execute (GIMP_RECTANGLE_TOOL (tool),
                                            x1, y1,
                                            x2 - x1,
                                            y2 - y1);

      gimp_rectangle_tool_configure (rectangle);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
    }

  if (finish)
    {
      gimp_display_shell_set_highlight (GIMP_DISPLAY_SHELL (tool->gdisp->shell),
                                        NULL);

      if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (rectangle)))
        gimp_draw_tool_stop (GIMP_DRAW_TOOL (rectangle));

      if (gimp_tool_control_is_active (tool->control))
        gimp_tool_control_halt (tool->control);

      gimp_image_flush (tool->gdisp->gimage);

      tool->gdisp    = NULL;
      tool->drawable = NULL;

    }
}

static void
rectangle_selection_callback (GtkWidget         *widget,
                              GimpRectangleTool *rectangle)
{
  GimpDisplay *gdisp;
  gint         x1, y1;
  gint         x2, y2;

  gdisp   = GIMP_TOOL (rectangle)->gdisp;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

  if (gimp_channel_bounds (gimp_image_get_mask (gdisp->gimage),
                           &x1, &y1,
                           &x2, &y2))
    {
      g_object_set (rectangle,
          "x1", x1,
          "y1", y1,
          "x2", x2,
          "y2", y2,
          NULL);
    }
  else
    {
      g_object_set (rectangle,
          "x1", 0,
          "y1", 0,
          "x2", gdisp->gimage->width,
          "y2", gdisp->gimage->height,
          NULL);
    }

  gimp_rectangle_tool_configure (rectangle);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
}

static void
rectangle_automatic_callback (GtkWidget         *widget,
                              GimpRectangleTool *rectangle)
{
  GimpDisplay *gdisp;
  gint         offset_x, offset_y;
  gint         width, height;
  gint         rx1, ry1, rx2, ry2;
  gint         x1, y1, x2, y2;
  gint         shrunk_x1;
  gint         shrunk_y1;
  gint         shrunk_x2;
  gint         shrunk_y2;

  gdisp   = GIMP_TOOL (rectangle)->gdisp;

  width    = gdisp->gimage->width;
  height   = gdisp->gimage->height;
  offset_x = 0;
  offset_y = 0;

  g_object_get (rectangle,
      "x1", &rx1,
      "y1", &ry1,
      "x2", &rx2,
      "y2", &ry2,
      NULL);

  x1 = rx1 - offset_x  > 0      ? rx1 - offset_x : 0;
  x2 = rx2 - offset_x  < width  ? rx2 - offset_x : width;
  y1 = ry1 - offset_y  > 0      ? ry1 - offset_y : 0;
  y2 = ry2 - offset_y  < height ? ry2 - offset_y : height;

  if (gimp_image_crop_auto_shrink (gdisp->gimage,
                                   x1, y1, x2, y2,
                                   FALSE,
                                   &shrunk_x1,
                                   &shrunk_y1,
                                   &shrunk_x2,
                                   &shrunk_y2))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      g_object_set (rectangle,
          "x1", offset_x + shrunk_x1,
          "y1", offset_y + shrunk_y1,
          "x2", offset_x + shrunk_x2,
          "y2", offset_y + shrunk_y2,
          NULL);

      gimp_rectangle_tool_configure (rectangle);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
    }
}

static void
rectangle_dimensions_changed (GtkWidget         *widget,
                              GimpRectangleTool *rectangle)
{
  GimpSizeEntry *entry = GIMP_SIZE_ENTRY (widget);
  gint           rx1, ry1, rx2, ry2;
  gdouble        x1, y1, x2, y2;
  GimpCoords     coords;

  if (! GIMP_TOOL (rectangle)->gdisp)
    return;

  g_object_get (rectangle,
      "x1", &rx1,
      "y1", &ry1,
      "x2", &rx2,
      "y2", &ry2,
      NULL);

  x1 = gimp_size_entry_get_refval (entry, 2);
  y1 = gimp_size_entry_get_refval (entry, 3);
  x2 = gimp_size_entry_get_refval (entry, 1);
  y2 = gimp_size_entry_get_refval (entry, 0);

  if (x1 != rx1)
    {
      coords.x = x1;
      coords.y = y1;
      g_object_set (rectangle,
          "function", RECT_RESIZING_LEFT,
          "startx", rx1,
          "starty", ry1,
          NULL);
    }
  else if (y1 != ry1)
    {
      coords.x = x1;
      coords.y = y1;
      g_object_set (rectangle,
          "function", RECT_RESIZING_TOP,
          "startx", rx1,
          "starty", ry1,
          NULL);
    }
  else if (x2 != rx2)
    {
      coords.x = x2;
      coords.y = y2;
      g_object_set (rectangle,
          "function", RECT_RESIZING_RIGHT,
          "startx", rx2,
          "starty", ry2,
          NULL);
    }
  else if (y2 != ry2)
    {
      coords.x = x2;
      coords.y = y2;
      g_object_set (rectangle,
          "function", RECT_RESIZING_BOTTOM,
          "startx", rx2,
          "starty", ry2,
          NULL);
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
  GimpRectangleToolInterface *tool_iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);
  gint                        rx1, ry1, rx2, ry2;

  g_return_val_if_fail (tool_iface->execute, FALSE);

  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  return tool_iface->execute (rectangle, rx1, ry1, rx2 - rx1, ry2 - ry1);
}

static void
gimp_rectangle_tool_update_options (GimpRectangleTool *rectangle,
                                    GimpDisplay       *gdisp)
{
  GimpDisplayShell     *shell;
  gdouble               width;
  gdouble               height;
  gdouble               aspect;
  gdouble               center_x, center_y;
  GimpSizeEntry        *entry;
  gint                  x1, y1, x2, y2;
  GimpRectangleOptions *options;
  gboolean              fixed_width;
  gboolean              fixed_height;
  gboolean              fixed_aspect;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  g_object_get (rectangle,
                "dimensions-entry", &entry,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL (rectangle)->tool_info->tool_options);
  g_object_get (options,
                "new-fixed-width",  &fixed_width,
                "new-fixed-height", &fixed_height,
                "fixed-aspect",     &fixed_aspect,
                NULL);

  width  = x2 - x1;
  height = y2 - y1;

  if (width > 0.01)
    aspect = height / width;
  else
    aspect = 0;

  center_x = (x1 + x2) / 2;
  center_y = (y1 + y2) / 2;

  g_signal_handlers_block_by_func (entry,
                                   rectangle_dimensions_changed,
                                   rectangle);

  gimp_size_entry_set_refval (entry, 0, y2);
  gimp_size_entry_set_refval (entry, 1, x2);
  gimp_size_entry_set_refval (entry, 2, x1);
  gimp_size_entry_set_refval (entry, 3, y1);

  g_signal_handlers_block_by_func (options,
                                   gimp_rectangle_tool_notify_width,
                                   rectangle);
  g_signal_handlers_block_by_func (options,
                                   gimp_rectangle_tool_notify_height,
                                   rectangle);
  g_signal_handlers_block_by_func (options,
                                   gimp_rectangle_tool_notify_aspect,
                                   rectangle);

  if (! fixed_width)
    g_object_set (options,
                  "width",  width,
                  NULL);

  if (! fixed_height)
    g_object_set (options,
                  "height", height,
                  NULL);

  if (! fixed_aspect)
    g_object_set (options,
                  "aspect", aspect,
                  NULL);

  g_signal_handlers_unblock_by_func (options,
                                     gimp_rectangle_tool_notify_width,
                                     rectangle);
  g_signal_handlers_unblock_by_func (options,
                                     gimp_rectangle_tool_notify_height,
                                     rectangle);
  g_signal_handlers_unblock_by_func (options,
                                     gimp_rectangle_tool_notify_aspect,
                                     rectangle);

  g_object_set (options,
                "center-x", center_x,
                "center-y", center_y,
                NULL);

  g_signal_handlers_unblock_by_func (entry,
                                     rectangle_dimensions_changed,
                                     rectangle);
}

static GtkWidget *
gimp_rectangle_controls (GimpRectangleTool *rectangle)
{
  GtkWidget *vbox;
  GtkObject *adjustment;
  GtkWidget *spinbutton;
  GtkWidget *entry;

  vbox = gtk_vbox_new (FALSE, 0);

  entry = gimp_size_entry_new (2, GIMP_UNIT_PIXEL, "%a",
                               TRUE, TRUE, FALSE, 4,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);
  g_object_set (rectangle, "dimensions-entry", entry, NULL);

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
  GimpSizeEntry *entry;

  g_object_get (rectangle, "dimensions-entry", &entry, NULL);

  gimp_size_entry_set_refval (entry, 0, 0);
  gimp_size_entry_set_refval (entry, 1, 0);
  gimp_size_entry_set_refval (entry, 2, 0);
  gimp_size_entry_set_refval (entry, 3, 0);
}

/*
 * we handle changes in width by treating them as movement of the right edge
 */
static void
gimp_rectangle_tool_notify_width (GimpRectangleOptions *options,
                                  GParamSpec           *pspec,
                                  GimpRectangleTool    *rectangle)
{
  gint       rx1, rx2, ry1, ry2;
  GimpCoords coords;
  gdouble    width  = gimp_rectangle_options_get_width (options);

  /* make sure a rectangle exists */
  if (! GIMP_TOOL (rectangle)->gdisp)
    return;

  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  coords.x = rx1 + width;
  coords.y = ry2;

  g_object_set (rectangle,
                "function", RECT_RESIZING_RIGHT,
                "startx", rx2,
                "starty", ry2,
                NULL);

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->gdisp);
}

/*
 * we handle changes in height by treating them as movement of the bottom edge
 */
static void
gimp_rectangle_tool_notify_height (GimpRectangleOptions *options,
                                   GParamSpec           *pspec,
                                   GimpRectangleTool    *rectangle)
{
  gint       rx1, rx2, ry1, ry2;
  GimpCoords coords;
  gdouble    height  = gimp_rectangle_options_get_height (options);

  if (! GIMP_TOOL (rectangle)->gdisp)
    return;

  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  coords.x = rx2;
  coords.y = ry1 + height;

  g_object_set (rectangle,
                "function", RECT_RESIZING_BOTTOM,
                "startx", rx2,
                "starty", ry2,
                NULL);

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->gdisp);
}

/*
 * we handle changes in aspect by treating them as movement of the bottom edge
 */
static void
gimp_rectangle_tool_notify_aspect (GimpRectangleOptions *options,
                                   GParamSpec           *pspec,
                                   GimpRectangleTool    *rectangle)
{
  gint       rx1, rx2, ry1, ry2;
  GimpCoords coords;
  gdouble    aspect  = gimp_rectangle_options_get_aspect (options);

  if (! GIMP_TOOL (rectangle)->gdisp)
    return;

  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  coords.x = rx2;
  coords.y = ry1 + aspect * (rx2 - rx1);

  g_object_set (rectangle,
                "function", RECT_RESIZING_BOTTOM,
                "startx", rx2,
                "starty", ry2,
                NULL);

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->gdisp);
}

