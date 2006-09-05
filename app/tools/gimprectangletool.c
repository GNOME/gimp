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
#include "core/gimppickable.h"
#include "core/gimpmarshal.h"

#include "display/gimpcanvas.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpdrawtool.h"
#include "gimprectangleoptions.h"
#include "gimprectangletool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"

enum
{
  RECTANGLE_CHANGED,
  LAST_SIGNAL
};


/*  speed of key movement  */
#define ARROW_VELOCITY 25


#define GIMP_RECTANGLE_TOOL_GET_PRIVATE(obj) \
  (gimp_rectangle_tool_get_private ((GimpRectangleTool *) (obj)))


typedef struct _GimpRectangleToolPrivate GimpRectangleToolPrivate;

struct _GimpRectangleToolPrivate
{
  gint       pressx;     /*  x where button pressed         */
  gint       pressy;     /*  y where button pressed         */

  gint       x1, y1;     /*  upper left hand coordinate     */
  gint       x2, y2;     /*  lower right hand coords        */

  guint      function;   /*  moving or resizing             */

  gboolean   constrain;  /* constrain to image bounds       */

  /* Internal state */
  gint       startx;     /*  starting x coord               */
  gint       starty;     /*  starting y coord               */

  gint       lastx;      /*  previous x coord               */
  gint       lasty;      /*  previous y coord               */

  gint       dx1, dy1;   /*  display coords                 */
  gint       dx2, dy2;   /*                                 */

  gint       dcw, dch;   /*  width and height of edges      */

  gint       saved_x1;   /*  for saving in case action is canceled */
  gint       saved_y1;
  gint       saved_x2;
  gint       saved_y2;
  gdouble    saved_center_x;
  gdouble   saved_center_y;

  GimpRectangleGuide guide; /* synced with options->guide, only exists for drawing */
};


static void gimp_rectangle_tool_iface_base_init     (GimpRectangleToolInterface *iface);

static GimpRectangleToolPrivate *
            gimp_rectangle_tool_get_private         (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_pressx          (GimpRectangleTool *tool,
                                                     gint               pressx);
gint        gimp_rectangle_tool_get_pressx          (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_pressy          (GimpRectangleTool *tool,
                                                     gint               pressy);
gint        gimp_rectangle_tool_get_pressy          (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_x1              (GimpRectangleTool *tool,
                                                     gint               x1);
gint        gimp_rectangle_tool_get_x1              (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_y1              (GimpRectangleTool *tool,
                                                     gint               y1);
gint        gimp_rectangle_tool_get_y1              (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_x2              (GimpRectangleTool *tool,
                                                     gint               x2);
gint        gimp_rectangle_tool_get_x2              (GimpRectangleTool *tool);
void        gimp_rectangle_tool_set_y2              (GimpRectangleTool *tool,
                                                     gint               y2);
gint        gimp_rectangle_tool_get_y2              (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_function        (GimpRectangleTool *tool,
                                                     guint              function);
guint       gimp_rectangle_tool_get_function        (GimpRectangleTool *tool);
gboolean    gimp_rectangle_tool_get_constrain       (GimpRectangleTool *tool);

/*  Rectangle helper functions  */
static void     rectangle_tool_start                (GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_draw_guides     (GimpDrawTool          *draw_tool);

/*  Rectangle dialog functions  */
static void     gimp_rectangle_tool_update_options  (GimpRectangleTool     *rectangle,
                                                     GimpDisplay           *display);

static void     gimp_rectangle_tool_notify_width      (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_height     (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_aspect     (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_highlight  (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_guide      (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_dimensions (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_check_function    (GimpRectangleTool     *rectangle,
                                                       gint                   curx,
                                                       gint                   cury);

static guint gimp_rectangle_tool_signals[LAST_SIGNAL] = { 0 };


GType
gimp_rectangle_tool_interface_get_type (void)
{
  static GType iface_type = 0;

  if (!iface_type)
    {
      static const GTypeInfo iface_info =
      {
        sizeof (GimpRectangleToolInterface),
        (GBaseInitFunc)     gimp_rectangle_tool_iface_base_init,
        (GBaseFinalizeFunc) NULL,
      };

      iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                           "GimpRectangleToolInterface",
                                           &iface_info, 0);

      g_type_interface_add_prerequisite (iface_type, GIMP_TYPE_DRAW_TOOL);
    }

  return iface_type;
}

static void
gimp_rectangle_tool_iface_base_init (GimpRectangleToolInterface *iface)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      gimp_rectangle_tool_signals[RECTANGLE_CHANGED] =
        g_signal_new ("rectangle-changed",
                      G_TYPE_FROM_INTERFACE (iface),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimpRectangleToolInterface,
                                       rectangle_changed),
                      NULL, NULL,
                      gimp_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

      g_object_interface_install_property (iface,
                                           g_param_spec_int ("pressx",
                                                             NULL, NULL,
                                                             G_MININT, G_MAXINT,
                                                             0,
                                                             GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_int ("pressy",
                                                             NULL, NULL,
                                                             G_MININT, G_MAXINT,
                                                             0,
                                                             GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_int ("x1",
                                                             NULL, NULL,
                                                             -GIMP_MAX_IMAGE_SIZE,
                                                             GIMP_MAX_IMAGE_SIZE,
                                                             0,
                                                             GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_int ("y1",
                                                             NULL, NULL,
                                                             -GIMP_MAX_IMAGE_SIZE,
                                                             GIMP_MAX_IMAGE_SIZE,
                                                             0,
                                                             GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_int ("x2",
                                                             NULL, NULL,
                                                             -GIMP_MAX_IMAGE_SIZE,
                                                             GIMP_MAX_IMAGE_SIZE,
                                                             0,
                                                             GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_int ("y2",
                                                             NULL, NULL,
                                                             -GIMP_MAX_IMAGE_SIZE,
                                                             GIMP_MAX_IMAGE_SIZE,
                                                             0,
                                                             GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_uint ("function",
                                                              NULL, NULL,
                                                              RECT_INACTIVE,
                                                              RECT_EXECUTING,
                                                              RECT_INACTIVE,
                                                              GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("constrain",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_PARAM_READWRITE));

      iface->rectangle_changed = NULL;

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
  static GQuark private_key = 0;

  GimpRectangleToolPrivate *private;

  if (G_UNLIKELY (private_key == 0))
    private_key = g_quark_from_static_string ("gimp-rectangle-tool-private");

  private = g_object_get_qdata (G_OBJECT (tool), private_key);

  if (! private)
    {
      private = g_new0 (GimpRectangleToolPrivate, 1);

      g_object_set_qdata_full (G_OBJECT (tool), private_key, private,
                               (GDestroyNotify)
                               gimp_rectangle_tool_private_finalize);
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
                                    GIMP_RECTANGLE_TOOL_PROP_CONSTRAIN,
                                    "constrain");
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
gimp_rectangle_tool_set_constrain (GimpRectangleTool *tool,
                                   gboolean           constrain)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->constrain = constrain ? TRUE : FALSE;

  g_object_notify (G_OBJECT (tool), "constrain");
}

gboolean
gimp_rectangle_tool_get_constrain (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->constrain;
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
    case GIMP_RECTANGLE_TOOL_PROP_CONSTRAIN:
      gimp_rectangle_tool_set_constrain (tool, g_value_get_boolean (value));
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
    case GIMP_RECTANGLE_TOOL_PROP_CONSTRAIN:
      g_value_set_boolean (value, gimp_rectangle_tool_get_constrain (tool));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_rectangle_tool_constructor (GObject *object)
{
  GimpTool          *tool      = GIMP_TOOL (object);
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (object);
  GObject           *options;

  tool->display = NULL;

  options = G_OBJECT (gimp_tool_get_options (tool));

  g_signal_connect_object (options, "notify::width",
                           G_CALLBACK (gimp_rectangle_tool_notify_width),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::height",
                           G_CALLBACK (gimp_rectangle_tool_notify_height),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::aspect",
                           G_CALLBACK (gimp_rectangle_tool_notify_aspect),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::highlight",
                           G_CALLBACK (gimp_rectangle_tool_notify_highlight),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::guide",
                           G_CALLBACK (gimp_rectangle_tool_notify_guide),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::dimensions-entry",
                           G_CALLBACK (gimp_rectangle_tool_notify_dimensions),
                           rectangle, 0);
}

void
gimp_rectangle_tool_dispose (GObject *object)
{
  GimpTool          *tool      = GIMP_TOOL (object);
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (object);
  GObject           *options   = G_OBJECT (gimp_tool_get_options (tool));

  g_signal_handlers_disconnect_by_func (options,
                                        G_CALLBACK (gimp_rectangle_tool_notify_width),
                                        rectangle);
  g_signal_handlers_disconnect_by_func (options,
                                        G_CALLBACK (gimp_rectangle_tool_notify_height),
                                        rectangle);
  g_signal_handlers_disconnect_by_func (options,
                                        G_CALLBACK (gimp_rectangle_tool_notify_aspect),
                                        rectangle);
  g_signal_handlers_disconnect_by_func (options,
                                        G_CALLBACK (gimp_rectangle_tool_notify_highlight),
                                        rectangle);
  g_signal_handlers_disconnect_by_func (options,
                                        G_CALLBACK (gimp_rectangle_tool_notify_dimensions),
                                        rectangle);
}

gboolean
gimp_rectangle_tool_initialize (GimpTool    *tool,
                                GimpDisplay *display)
{
  GimpRectangleToolPrivate *private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  GObject                  *options = G_OBJECT (gimp_tool_get_options (tool));
  GimpSizeEntry            *entry;

  g_object_get (options,
                "dimensions-entry", &entry,
                "guide",            &private->guide,
                NULL);

  if (display != tool->display)
    {
      gint     width  = gimp_image_get_width (display->image);
      gint     height = gimp_image_get_height (display->image);
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

      gimp_image_get_resolution (display->image, &xres, &yres);

      gimp_size_entry_set_resolution (entry, 0, yres, TRUE);
      gimp_size_entry_set_resolution (entry, 1, xres, TRUE);
      gimp_size_entry_set_resolution (entry, 2, xres, TRUE);
      gimp_size_entry_set_resolution (entry, 3, yres, TRUE);

      unit = gimp_display_shell_get_unit (GIMP_DISPLAY_SHELL (display->shell));
      gimp_size_entry_set_unit (entry, unit);
    }

  return TRUE;
}

void
gimp_rectangle_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      gimp_rectangle_tool_configure (rectangle);
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_rectangle_tool_halt (rectangle);
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
                                  GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpDrawTool             *draw_tool = GIMP_DRAW_TOOL (tool);
  guint                     function;
  GimpRectangleToolPrivate *private;
  gint                      x         = ROUND (coords->x);
  gint                      y         = ROUND (coords->y);
  GimpRectangleOptions     *options;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  if (display != tool->display)
    {
      if (gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_stop (draw_tool);

      function = RECT_CREATING;
      g_object_set (rectangle, "function", function,
                    "x1", x,
                    "y1", y,
                    "x2", x,
                    "y2", y,
                    NULL);
      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);

      tool->display = display;
      rectangle_tool_start (rectangle);
    }

  /* save existing shape in case of cancellation */
  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  g_object_get (rectangle,
                "x1", &private->saved_x1,
                "y1", &private->saved_y1,
                "x2", &private->saved_x2,
                "y2", &private->saved_y2,
                NULL);
  g_object_get (options,
                "center-x", &private->saved_center_x,
                "center-y", &private->saved_center_y,
                NULL);

  g_object_get (rectangle,
                "function", &function,
                NULL);

  if (function == RECT_CREATING)
    {
      g_object_set (options,
                    "center-x", (gdouble) x,
                    "center-y", (gdouble) y,
                    NULL);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      g_object_set (rectangle,
                    "x1", x,
                    "y1", y,
                    "x2", x,
                    "y2", y,
                    NULL);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  g_object_set (rectangle,
                "pressx", x,
                "pressy", y,
                NULL);

  private->startx = x;
  private->starty = y;
  private->lastx = x;
  private->lasty = y;

  gimp_tool_control_activate (tool->control);
}

void
gimp_rectangle_tool_button_release (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolPrivate *private;
  guint                     function;
  GimpRectangleOptions     *options;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  gimp_tool_control_halt (tool->control);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  g_object_get (rectangle, "function", &function, NULL);

  if (function == RECT_EXECUTING)
    gimp_tool_pop_status (tool, display);

  if (! (state & GDK_BUTTON3_MASK))
    {
      if (gimp_rectangle_tool_no_movement (rectangle))
        {
          if (gimp_rectangle_tool_execute (rectangle))
            gimp_rectangle_tool_halt (rectangle);
        }
      else
        {
          g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
        }
    }
  else
    {
      g_object_set (options,
                    "center-x", private->saved_center_x,
                    "center-y", private->saved_center_y,
                    NULL);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      g_object_set (rectangle,
                    "x1", private->saved_x1,
                    "y1", private->saved_y1,
                    "x2", private->saved_x2,
                    "y2", private->saved_y2,
                    NULL);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

void
gimp_rectangle_tool_motion (GimpTool        *tool,
                            GimpCoords      *coords,
                            guint32          time,
                            GdkModifierType  state,
                            GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;
  guint                     function;
  gint                      x1, y1, x2, y2;
  gint                      curx, cury;
  gint                      inc_x, inc_y;
  gint                      min_x, min_y, max_x, max_y;
  gint                      rx1, ry1, rx2, ry2;
  gboolean                  fixed_width;
  gboolean                  fixed_height;
  gboolean                  fixed_aspect;
  gboolean                  fixed_center;
  gdouble                   width, height;
  gdouble                   center_x, center_y;
  gboolean                  aspect_square;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  /*  This is the only case when the motion events should be ignored --
   *  we're just waiting for the button release event to execute.
   */
  g_object_get (rectangle, "function", &function, NULL);

  if (function == RECT_EXECUTING)
    return;

  curx = ROUND (coords->x);
  cury = ROUND (coords->y);

  /* fix function, startx, starty if user has "flipped" the rectangle */
  gimp_rectangle_tool_check_function (rectangle, curx, cury);

  inc_x = curx - private->startx;
  inc_y = cury - private->starty;

  /*  If there have been no changes... return  */
  if (private->lastx == curx && private->lasty == cury)
    return;

  g_object_get (options,
                "new-fixed-width",  &fixed_width,
                "new-fixed-height", &fixed_height,
                "fixed-aspect",     &fixed_aspect,
                "fixed-center",     &fixed_center,
                "aspect-square",    &aspect_square,
                NULL);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  min_x = min_y = 0;
  max_x = display->image->width;
  max_y = display->image->height;

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
  x1 = rx1;
  y1 = ry1;
  x2 = rx2;
  y2 = ry2;

  switch (function)
    {
    case RECT_INACTIVE:
      g_warning ("function is RECT_INACTIVE while mouse is moving");
      break;

    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LEFT:
      x1 = rx1 + inc_x;
      if (fixed_width)
        x2 = x1 + width;
      else if (fixed_center)
        x2 = x1 + 2 * (center_x - x1);
      else
        x2 = MAX (x1, rx2);
      break;

    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_RIGHT:
      x2 = rx2 + inc_x;
      if (fixed_width)
        x1 = x2 - width;
      else if (fixed_center)
        x1 = x2 - 2 * (x2 - center_x);
      else
        x1 = MIN (rx1, x2);
      break;

    case RECT_RESIZING_BOTTOM:
    case RECT_RESIZING_TOP:
      x1 = rx1;
      x2 = rx2;
      break;

    case RECT_MOVING:
      x1 = rx1 + inc_x;
      x2 = rx2 + inc_x;
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
        y2 = y1 + height;
      else if (fixed_center)
        y2 = y1 + 2 * (center_y - y1);
      else
        y2 = MAX (y1, ry2);
      break;

    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_BOTTOM:
      y2 = ry2 + inc_y;
      if (fixed_height)
        y1 = y2 - height;
      else if (fixed_center)
        y1 = y2 - 2 * (y2 - center_y);
      else
        y1 = MIN (ry1, y2);
      break;

    case RECT_RESIZING_RIGHT:
    case RECT_RESIZING_LEFT:
      y1 = ry1;
      y2 = ry2;
      break;

    case RECT_MOVING:
      y1 = ry1 + inc_y;
      y2 = ry2 + inc_y;
      break;
    }

  if (fixed_aspect || aspect_square)
    {
      gdouble aspect;

      if (aspect_square)
        aspect = 1;
      else
        {
          g_object_get (options, "aspect", &aspect, NULL);
          aspect = CLAMP (aspect, 1.0 / max_y, max_x);
        }

      switch (function)
        {
        case RECT_RESIZING_UPPER_LEFT:
          /*
           * The same basically happens for each corner, just with a
           * different fixed corner. To keep within aspect ratio and
           * at the same time keep the cursor on one edge if not the
           * corner itself: - calculate the two positions of the
           * corner in question on the base of the current mouse
           * cursor position and the fixed corner opposite the one
           * selected.  - decide on which egde we are inside the
           * rectangle dimension - if we are on the inside of the
           * vertical edge then we use the x position of the cursor,
           * otherwise we are on the inside (or close enough) of the
           * horizontal edge and then we use the y position of the
           * cursor for the base of our new corner.
           */
          x1 = rx2 - (ry2 - cury) * aspect + .5;
          y1 = ry2 - (rx2 - curx) / aspect + .5;
          if ((y1 < cury) && (cury < y2))
            x1 = curx;
          else
            y1 = cury;
          if (fixed_center)
            {
              x2 = x1 + 2 * (center_x - x1);
              y2 = y1 + 2 * (center_y - y1);
            }
          break;

        case RECT_RESIZING_UPPER_RIGHT:
          x2 = rx1 + (ry2 - cury) * aspect + .5;
          y1 = ry2 - (curx - rx1) / aspect + .5;
          if ((y1 < cury) && (cury < y2))
            x2 = curx;
          else
            y1 = cury;
          if (fixed_center)
            {
              x1 = x2 - 2 * (x2 - center_x);
              y2 = y1 + 2 * (center_y - y1);
            }
          break;

        case RECT_RESIZING_LOWER_LEFT:
          x1 = rx2 - (cury - ry1) * aspect + .5;
          y2 = ry1 + (rx2 - curx) / aspect + .5;
          if ((y1 < cury) && (cury < y2))
            x1 = curx;
          else
            y2 = cury;
          if (fixed_center)
            {
              x2 = x1 + 2 * (center_x - x1);
              y1 = y2 - 2 * (y2 - center_y);
            }
          break;

        case RECT_RESIZING_LOWER_RIGHT:
          x2 = rx1 + (cury - ry1) * aspect + .5;
          y2 = ry1 + (curx - rx1) / aspect + .5;
          if ((y1 < cury) && (cury < y2))
            x2 = curx;
          else
            y2 = cury;
          if (fixed_center)
            {
              x1 = x2 - 2 * (x2 - center_x);
              y1 = y2 - 2 * (y2 - center_y);
            }
          break;

        case RECT_RESIZING_TOP:
          x2 = rx1 + (ry2 - y1) * aspect + .5;
          if (fixed_center)
            x1 = x2 - 2 * (x2 - center_x);
          break;

        case RECT_RESIZING_LEFT:
          /* When resizing the left hand delimiter then the aspect
           * dictates the height of the result, any inc_y is redundant
           * and not relevant to the result
           */
          y2 = ry1 + (rx2 - x1) / aspect + .5;
          if (fixed_center)
            y1 = y2 - 2 * (y2 - center_y);
          break;

        case RECT_RESIZING_BOTTOM:
          x2 = rx1 + (y2 - ry1) * aspect + .5;
          if (fixed_center)
            x1 = x2 - 2 * (x2 - center_x);
          break;

        case RECT_RESIZING_RIGHT:
          /* When resizing the right hand delimiter then the aspect
           * dictates the height of the result, any inc_y is redundant
           * and not relevant to the result
           */
          y2 = ry1 + (x2 - rx1) / aspect + 0.5;
          if (fixed_center)
            y1 = y2 - 2 * (y2 - center_y);
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
                NULL);

  if (function != RECT_CREATING)
    {
      private->startx = curx;
      private->starty = cury;
    }

  private->lastx = curx;
  private->lasty = cury;

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

  gimp_rectangle_tool_update_options (rectangle, display);

  if (function != RECT_MOVING && function != RECT_EXECUTING)
    {
      gint w, h;

      gimp_tool_pop_status (tool, display);

      if (gimp_rectangle_tool_get_constrain (rectangle))
        {
          w = MIN (rx2, max_x) - MAX (rx1, min_x);
          h = MIN (ry2, max_y) - MAX (ry1, min_y);
        }
      else
        {
          w = rx2 - rx1;
          h = ry2 - ry1;
        }

      if (w > 0 && h > 0)
        gimp_tool_push_status_coords (tool, display,
                                      _("Rectangle: "), w, " × ", h);
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

void
gimp_rectangle_tool_modifier_key (GimpTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpRectangleTool    *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleOptions *options   = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);


  if (press)
    {
      if (key == GDK_SHIFT_MASK)
        {
          gboolean aspect_square;

          g_object_get (options,
                        "aspect-square", &aspect_square,
                        NULL);

          g_object_set (options,
                        "aspect-square", ! aspect_square,
                        NULL);
        }

      if (key == GDK_CONTROL_MASK)
        {
          gboolean fixed_center;

          g_object_get (options,
                        "fixed-center", &fixed_center,
                        NULL);

          g_object_set (options,
                        "fixed-center", ! fixed_center,
                        NULL);

         if (! fixed_center)
           {
             gdouble center_x = gimp_rectangle_tool_get_pressx (rectangle);
             gdouble center_y = gimp_rectangle_tool_get_pressy (rectangle);

             g_object_set (options,
                           "center-x", center_x,
                           "center-y", center_y,
                           NULL);
           }
        }
    }
}

/*
 * gimp_rectangle_tool_check_function() is needed to deal with
 * situations where the user drags a corner or edge across one of the
 * existing edges, thereby changing its function.  Ugh.
 */
static void
gimp_rectangle_tool_check_function (GimpRectangleTool *rectangle,
                                    gint               curx,
                                    gint               cury)
{
  GimpRectangleToolPrivate *private;
  guint                     function;
  guint                     new_function;
  gint                      x1, y1, x2, y2;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  g_object_get (rectangle,
                "function", &function,
                "x1",       &x1,
                "y1",       &y1,
                "x2",       &x2,
                "y2",       &y2,
                NULL);

  new_function = function;

  switch (function)
    {
    case RECT_RESIZING_LEFT:
      if (curx > x2)
        {
          new_function = RECT_RESIZING_RIGHT;
          private->startx = x2;
        }
      break;

    case RECT_RESIZING_RIGHT:
      if (curx < x1)
        {
          new_function = RECT_RESIZING_LEFT;
          private->startx = x1;
        }
      break;

    case RECT_RESIZING_TOP:
      if (cury > y2)
        {
          new_function = RECT_RESIZING_BOTTOM;
          private->starty = y2;
        }
      break;

    case RECT_RESIZING_BOTTOM:
      if (cury < y1)
        {
          new_function = RECT_RESIZING_TOP;
          private->starty = y1;
        }
      break;

    case RECT_RESIZING_UPPER_LEFT:
      if (curx > x2 && cury > y2)
        {
          new_function = RECT_RESIZING_LOWER_RIGHT;
          private->startx = x2;
          private->starty = y2;
        }
      else if (curx > x2)
        {
          new_function = RECT_RESIZING_UPPER_RIGHT;
          private->startx = x2;
        }
      else if (cury > y2)
        {
          new_function = RECT_RESIZING_LOWER_LEFT;
          private->starty = y2;
        }
      break;

    case RECT_RESIZING_UPPER_RIGHT:
      if (curx < x1 && cury > y2)
        {
          new_function = RECT_RESIZING_LOWER_LEFT;
          private->startx = x1;
          private->starty = y2;
        }
      else if (curx < x1)
        {
          new_function = RECT_RESIZING_UPPER_LEFT;
          private->startx = x1;
        }
      else if (cury > y2)
        {
          new_function = RECT_RESIZING_LOWER_RIGHT;
          private->starty = y2;
        }
      break;

    case RECT_RESIZING_LOWER_LEFT:
      if (curx > x2 && cury < y1)
        {
          new_function = RECT_RESIZING_UPPER_RIGHT;
          private->startx = x2;
          private->starty = y1;
        }
      else if (curx > x2)
        {
          new_function = RECT_RESIZING_LOWER_RIGHT;
          private->startx = x2;
        }
      else if (cury < y1)
        {
          new_function = RECT_RESIZING_UPPER_LEFT;
          private->starty = y1;
        }
      break;

    case RECT_RESIZING_LOWER_RIGHT:
      if (curx < x1 && cury < y1)
        {
          new_function = RECT_RESIZING_UPPER_LEFT;
          private->startx = x1;
          private->starty = y1;
        }
      else if (curx < x1)
        {
          new_function = RECT_RESIZING_LOWER_LEFT;
          private->startx = x1;
        }
      else if (cury < y1)
        {
          new_function = RECT_RESIZING_UPPER_RIGHT;
          private->starty = y1;
        }
      break;

    default:
      break;
    }

  if (new_function != function)
    g_object_set (rectangle, "function", new_function, NULL);

}

gboolean
gimp_rectangle_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  gint               inc_x, inc_y;
  gint               min_x, min_y;
  gint               max_x, max_y;
  gint               x1, y1, x2, y2;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), FALSE);

  if (display != tool->display)
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
      if (gimp_rectangle_tool_execute (rectangle))
        gimp_rectangle_tool_halt (rectangle);
      return TRUE;

    case GDK_Escape:
      gimp_rectangle_tool_cancel (rectangle);
      gimp_rectangle_tool_halt (rectangle);
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
  max_x = display->image->width;
  max_y = display->image->height;

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

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);

  return TRUE;
}

void
gimp_rectangle_tool_oper_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 gboolean         proximity,
                                 GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolPrivate *private;
  GimpDrawTool             *draw_tool = GIMP_DRAW_TOOL (tool);
  gint                      x1, y1, x2, y2;
  gboolean                  inside_x;
  gboolean                  inside_y;
  gdouble                   handle_w, handle_h;
  GimpDisplayShell         *shell;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  if (tool->display != display)
    return;

  shell = GIMP_DISPLAY_SHELL (tool->display->shell);

  g_object_get (rectangle,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  handle_w = private->dcw / SCALEFACTOR_X (shell);
  handle_h = private->dch / SCALEFACTOR_Y (shell);

  inside_x = coords->x > x1 && coords->x < x2;
  inside_y = coords->y > y1 && coords->y < y2;

  if (gimp_draw_tool_on_handle (draw_tool, display,
                                coords->x, coords->y,
                                GIMP_HANDLE_SQUARE,
                                x1, y1,
                                private->dcw, private->dch,
                                GTK_ANCHOR_NORTH_WEST,
                                FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_UPPER_LEFT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x1 - coords->x,
                                          y1 - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, display,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     x2, y2,
                                     private->dcw, private->dch,
                                     GTK_ANCHOR_SOUTH_EAST,
                                     FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_LOWER_RIGHT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x2 - coords->x,
                                          y2 - coords->y,
                                          0, 0);
    }
  else if  (gimp_draw_tool_on_handle (draw_tool, display,
                                      coords->x, coords->y,
                                      GIMP_HANDLE_SQUARE,
                                      x2, y1,
                                      private->dcw, private->dch,
                                      GTK_ANCHOR_NORTH_EAST,
                                      FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_UPPER_RIGHT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x2 - coords->x,
                                          y1 - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, display,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     x1, y2,
                                     private->dcw, private->dch,
                                     GTK_ANCHOR_SOUTH_WEST,
                                     FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_LOWER_LEFT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x1 - coords->x,
                                          y2 - coords->y,
                                          0, 0);
    }
  else if ( (fabs (coords->x - x1) < handle_w) && inside_y)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_LEFT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x1 - coords->x, 0, 0, 0);

    }
  else if ( (fabs (coords->x - x2) < handle_w) && inside_y)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_RIGHT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x2 - coords->x, 0, 0, 0);

    }
  else if ( (fabs (coords->y - y1) < handle_h) && inside_x)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_TOP, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, y1 - coords->y, 0, 0);

    }
  else if ( (fabs (coords->y - y2) < handle_h) && inside_x)
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
                                   GimpDisplay     *display)
{
  GimpRectangleTool *rectangle;
  GimpCursorType     cursor   = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier modifier = GIMP_CURSOR_MODIFIER_NONE;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle = GIMP_RECTANGLE_TOOL (tool);

  if (tool->display == display && ! (state & GDK_BUTTON1_MASK))
    {
      guint function;

      g_object_get (rectangle, "function", &function, NULL);

      switch (function)
        {
        case RECT_CREATING:
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
          break;
        case RECT_MOVING:
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
          modifier = GIMP_CURSOR_MODIFIER_MOVE;
          break;
        case RECT_RESIZING_UPPER_LEFT:
          cursor = GIMP_CURSOR_CORNER_TOP_LEFT;
          break;
        case RECT_RESIZING_UPPER_RIGHT:
          cursor = GIMP_CURSOR_CORNER_TOP_RIGHT;
          break;
        case RECT_RESIZING_LOWER_LEFT:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_LEFT;
          break;
        case RECT_RESIZING_LOWER_RIGHT:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
          break;
        case RECT_RESIZING_LEFT:
          cursor = GIMP_CURSOR_SIDE_LEFT;
          break;
        case RECT_RESIZING_RIGHT:
          cursor = GIMP_CURSOR_SIDE_RIGHT;
          break;
        case RECT_RESIZING_TOP:
          cursor = GIMP_CURSOR_SIDE_TOP;
          break;
        case RECT_RESIZING_BOTTOM:
          cursor = GIMP_CURSOR_SIDE_BOTTOM;
          break;

        default:
          break;
        }
    }

  gimp_tool_control_set_cursor (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);
}

#define ANCHOR_SIZE 6

void
gimp_rectangle_tool_draw (GimpDrawTool *draw_tool)
{
  GimpRectangleToolPrivate *private;
  gint                      x1, x2, y1, y2;
  guint                     function;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (draw_tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (draw_tool);

  g_object_get (GIMP_RECTANGLE_TOOL (draw_tool), "function", &function, NULL);

  if (function == RECT_INACTIVE)
    return;

  x1 = private->x1;
  x2 = private->x2;
  y1 = private->y1;
  y2 = private->y2;

  gimp_draw_tool_draw_rectangle (draw_tool, FALSE,
                                 x1, y1, x2 - x1, y2 - y1, FALSE);

  gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                              x1, y1, ANCHOR_SIZE, ANCHOR_SIZE,
                              GTK_ANCHOR_NORTH_WEST, FALSE);
  gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                              x2, y1, ANCHOR_SIZE, ANCHOR_SIZE,
                              GTK_ANCHOR_NORTH_EAST, FALSE);
  gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                              x1, y2, ANCHOR_SIZE, ANCHOR_SIZE,
                              GTK_ANCHOR_SOUTH_WEST, FALSE);
  gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                              x2, y2, ANCHOR_SIZE, ANCHOR_SIZE,
                              GTK_ANCHOR_SOUTH_EAST, FALSE);

  gimp_rectangle_tool_draw_guides (draw_tool);
}

static void
gimp_rectangle_tool_draw_guides (GimpDrawTool *draw_tool)
{
  GimpTool                 *tool;
  GimpRectangleToolPrivate *private;
  gint                      x1, x2, y1, y2;

  tool    = GIMP_TOOL (draw_tool);
  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (draw_tool);

  x1 = private->x1;
  x2 = private->x2;
  y1 = private->y1;
  y2 = private->y2;

  switch (private->guide)
    {
    case GIMP_RECTANGLE_GUIDE_NONE:
      break;

    case GIMP_RECTANGLE_GUIDE_CENTER_LINES:
      gimp_draw_tool_draw_line (draw_tool,
                                x1, (y1 + y2) / 2,
                                x2, (y1 + y2) / 2, FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                (x1 + x2) / 2, y1,
                                (x1 + x2) / 2, y2, FALSE);
      break;

    case GIMP_RECTANGLE_GUIDE_THIRDS:
      gimp_draw_tool_draw_line (draw_tool,
                                x1, (2 * y1 + y2) / 3,
                                x2, (2 * y1 + y2) / 3, FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                x1, (y1 + 2 * y2) / 3,
                                x2, (y1 + 2 * y2) / 3, FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                (2 * x1 + x2) / 3, y1,
                                (2 * x1 + x2) / 3, y2, FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                (x1 + 2 * x2) / 3, y1,
                                (x1 + 2 * x2) / 3, y2, FALSE);
      break;

    case GIMP_RECTANGLE_GUIDE_GOLDEN:
      gimp_draw_tool_draw_line (draw_tool,
                                x1,
                                (2 * y1 + (1 + sqrt(5)) * y2) / (3 + sqrt(5)),
                                x2,
                                (2 * y1 + (1 + sqrt(5)) * y2) / (3 + sqrt(5)),
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                x1,
                                ((1 + sqrt(5)) * y1 + 2 * y2) / (3 + sqrt(5)),
                                x2,
                                ((1 + sqrt(5)) * y1 + 2 * y2) / (3 + sqrt(5)),
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                (2 * x1 + (1 + sqrt(5)) * x2) / (3 + sqrt(5)),
                                y1,
                                (2 * x1 + (1 + sqrt(5)) * x2) / (3 + sqrt(5)),
                                y2,
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                ((1 + sqrt(5)) * x1 + 2 * x2) / (3 + sqrt(5)),
                                y1,
                                ((1 + sqrt(5)) * x1 + 2 * x2) / (3 + sqrt(5)),
                                y2, FALSE);
      break;
    }
}

void
gimp_rectangle_tool_configure (GimpRectangleTool *rectangle)
{
  GimpTool                 *tool = GIMP_TOOL (rectangle);
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;
  GimpDisplayShell         *shell;
  gboolean                  highlight;
  gint                      x1, y1;
  gint                      x2, y2;
  gint                      dx1, dx2, dy1, dy2, dcw, dch;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  if (! tool->display)
    return;

  shell = GIMP_DISPLAY_SHELL (tool->display->shell);

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

#define SRW 10
#define SRH 10

  dcw = ((dx2 - dx1) < SRW) ? (dx2 - dx1) : SRW;
  dch = ((dy2 - dy1) < SRH) ? (dy2 - dy1) : SRH;

#undef SRW
#undef SRH

  private->dx1 = dx1;
  private->dx2 = dx2;
  private->dy1 = dy1;
  private->dy2 = dy2;
  private->dcw = dcw;
  private->dch = dch;
}

static void
rectangle_tool_start (GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);

  gimp_rectangle_tool_configure (rectangle);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, tool->display,
                                _("Rectangle: "), 0, " x ", 0);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->display);
}

void
gimp_rectangle_tool_halt (GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);

  gimp_display_shell_set_highlight (GIMP_DISPLAY_SHELL (tool->display->shell),
                                    NULL);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (rectangle)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (rectangle));

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);

  tool->display  = NULL;
  tool->drawable = NULL;

  g_object_set (rectangle, "function", RECT_INACTIVE, NULL);
}

gboolean
gimp_rectangle_tool_execute (GimpRectangleTool *rectangle)
{
  GimpRectangleToolInterface *iface;
  gboolean                    retval = FALSE;

  iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  if (iface->execute)
    {
      gint x1, y1, x2, y2;

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      retval = iface->execute (rectangle, x1, y1, x2 - x1, y2 - y1);

      gimp_rectangle_tool_configure (rectangle);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
    }

  return retval;
}

void
gimp_rectangle_tool_cancel (GimpRectangleTool *rectangle)
{
  GimpRectangleToolInterface *iface;

  iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  if (iface->cancel)
    iface->cancel (rectangle);
}

static void
gimp_rectangle_tool_update_options (GimpRectangleTool *rectangle,
                                    GimpDisplay       *display)
{
  GimpRectangleOptions *options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle);
  gdouble               width;
  gdouble               height;
  gdouble               aspect;
  gdouble               center_x, center_y;
  GimpSizeEntry        *entry;
  gint                  x1, y1, x2, y2;
  gboolean              fixed_width;
  gboolean              fixed_height;
  gboolean              fixed_aspect;
  gboolean              aspect_square;

  g_object_get (rectangle,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  g_object_get (options,
                "new-fixed-width",  &fixed_width,
                "new-fixed-height", &fixed_height,
                "fixed-aspect",     &fixed_aspect,
                "aspect-square",    &aspect_square,
                "dimensions-entry", &entry,
                NULL);

  width  = x2 - x1;
  height = y2 - y1;

  if (aspect_square)
    aspect = 1;
  else if (height > 0.01)
    aspect = width / height;
  else
    aspect = 0.0;

  center_x = (x1 + x2) / 2.0;
  center_y = (y1 + y2) / 2.0;

  g_signal_handlers_block_by_func (options,
                                   gimp_rectangle_tool_notify_dimensions,
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

  if (aspect_square || ! fixed_aspect)
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

  g_signal_handlers_unblock_by_func (options,
                                     gimp_rectangle_tool_notify_dimensions,
                                     rectangle);
}

/*
 * we handle changes in width by treating them as movement of the right edge
 */
static void
gimp_rectangle_tool_notify_width (GimpRectangleOptions *options,
                                  GParamSpec           *pspec,
                                  GimpRectangleTool    *rectangle)
{
  GimpRectangleToolPrivate *private;
  gint                      rx1, rx2, ry1, ry2;
  GimpCoords                coords;
  gdouble                   width;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  /* make sure a rectangle exists */
  if (! GIMP_TOOL (rectangle)->display)
    return;

  g_object_get (options,
                "width", &width,
                NULL);
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
                NULL);
  private->startx = rx2;
  private->starty = ry2;

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->display);

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
}

/*
 * we handle changes in height by treating them as movement of the bottom edge
 */
static void
gimp_rectangle_tool_notify_height (GimpRectangleOptions *options,
                                   GParamSpec           *pspec,
                                   GimpRectangleTool    *rectangle)
{
  GimpRectangleToolPrivate *private;
  gint                      rx1, rx2, ry1, ry2;
  GimpCoords                coords;
  gdouble                   height;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  if (! GIMP_TOOL (rectangle)->display)
    return;

  g_object_get (options,
                "height", &height,
                NULL);
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
                NULL);
  private->startx = rx2;
  private->starty = ry2;

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->display);

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
}

/*
 * we handle changes in aspect by treating them as movement of the bottom edge
 */
static void
gimp_rectangle_tool_notify_aspect (GimpRectangleOptions *options,
                                   GParamSpec           *pspec,
                                   GimpRectangleTool    *rectangle)
{
  GimpRectangleToolPrivate *private;
  gint                      rx1, rx2, ry1, ry2;
  GimpCoords                coords;
  gdouble                   aspect;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  if (! GIMP_TOOL (rectangle)->display)
    return;

  g_object_get (options,
                "aspect", &aspect,
                NULL);
  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  coords.x = rx2;
  coords.y = ry1 + (rx2 - rx1) / aspect;

  g_object_set (rectangle,
                "function", RECT_RESIZING_BOTTOM,
                NULL);
  private->startx = rx2;
  private->starty = ry2;

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->display);

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
}

static void
gimp_rectangle_tool_notify_highlight (GimpRectangleOptions *options,
                                      GParamSpec           *pspec,
                                      GimpRectangleTool    *rectangle)
{
  GimpTool         *tool = GIMP_TOOL (rectangle);
  GimpDisplayShell *shell;
  gboolean          highlight;

  if (! tool->display)
    return;

  shell = GIMP_DISPLAY_SHELL (tool->display->shell);

  g_object_get (options, "highlight", &highlight, NULL);

  if (highlight)
    {
      GdkRectangle rect;
      gint         x1, y1;
      gint         x2, y2;

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      rect.x      = x1;
      rect.y      = y1;
      rect.width  = x2 - x1;
      rect.height = y2 - y1;

      gimp_display_shell_set_highlight (shell, &rect);
    }
  else
    {
      gimp_display_shell_set_highlight (shell, NULL);
    }
}

static void
gimp_rectangle_tool_notify_guide (GimpRectangleOptions *options,
                                  GParamSpec           *pspec,
                                  GimpRectangleTool    *rectangle)
{
  GimpTool                 *tool = GIMP_TOOL (rectangle);
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  if (tool->display)
    gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

  g_object_get (options,
                "guide", &private->guide,
                NULL);

  if (tool->display)
    gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
}

static void
gimp_rectangle_tool_notify_dimensions (GimpRectangleOptions *options,
                                       GParamSpec           *pspec,
                                       GimpRectangleTool    *rectangle)
{
  GimpRectangleToolPrivate *private;
  gint                      rx1, rx2, ry1, ry2;
  GimpCoords                coords;
  GimpSizeEntry            *entry;
  gdouble                   x1, y1, x2, y2;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  g_object_get (options, "dimensions-entry", &entry, NULL);

  if (! GIMP_TOOL (rectangle)->display)
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
                    NULL);
      private->startx = rx1;
      private->starty = ry1;
    }
  else if (y1 != ry1)
    {
      coords.x = x1;
      coords.y = y1;
      g_object_set (rectangle,
                    "function", RECT_RESIZING_TOP,
                    NULL);
      private->startx = rx1;
      private->starty = ry1;
    }
  else if (x2 != rx2)
    {
      coords.x = x2;
      coords.y = y2;
      g_object_set (rectangle,
                    "function", RECT_RESIZING_RIGHT,
                    NULL);
      private->startx = rx2;
      private->starty = ry2;
    }
  else if (y2 != ry2)
    {
      coords.x = x2;
      coords.y = y2;
      g_object_set (rectangle,
                    "function", RECT_RESIZING_BOTTOM,
                    NULL);
      private->startx = rx2;
      private->starty = ry2;
    }
  else
    return;

  /* use the motion handler to handle this, to avoid duplicating
     a bunch of code */
  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->display);

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
}

gboolean
gimp_rectangle_tool_no_movement (GimpRectangleTool *rectangle)
{
  gint                      pressx, pressy;
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  g_object_get (rectangle,
                "pressx", &pressx,
                "pressy", &pressy,
                NULL);

  return (private->lastx == pressx && private->lasty == pressy);
}
