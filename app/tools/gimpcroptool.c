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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimprectangleoptions.h"
#include "gimprectangletool.h"
#include "gimpcropoptions.h"
#include "gimpcroptool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void gimp_crop_tool_class_init     (GimpCropToolClass *klass);
static void gimp_crop_tool_init           (GimpCropTool      *crop_tool);
static void gimp_crop_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *rectangle_iface);

static GObject * gimp_crop_tool_constructor (GType                  type,
                                             guint                  n_params,
                                             GObjectConstructParam *params);
static void gimp_crop_tool_finalize         (GObject *object);
static void gimp_crop_tool_control          (GimpTool       *tool,
                                             GimpToolAction  action,
                                             GimpDisplay    *gdisp);

static void gimp_crop_tool_cursor_update    (GimpTool                   *tool,
                                             GimpCoords                 *coords,
                                             GdkModifierType             state,
                                             GimpDisplay                *gdisp);

static gboolean gimp_crop_tool_execute      (GimpRectangleTool       *rectangle,
                                             gint                        x,
                                             gint                        y,
                                             gint                        w,
                                             gint                        h);

void        gimp_crop_tool_set_controls (GimpRectangleTool *tool,
                                         GtkWidget         *controls);
GtkWidget * gimp_crop_tool_get_controls (GimpRectangleTool *tool);
void        gimp_crop_tool_set_entry    (GimpRectangleTool *tool,
                                         GtkWidget         *entry);
GtkWidget * gimp_crop_tool_get_entry    (GimpRectangleTool *tool);

void        gimp_crop_tool_set_startx   (GimpRectangleTool *tool,
                                         gint               startx);
gint        gimp_crop_tool_get_startx   (GimpRectangleTool *tool);
void        gimp_crop_tool_set_starty   (GimpRectangleTool *tool,
                                         gint               starty);
gint        gimp_crop_tool_get_starty   (GimpRectangleTool *tool);

void        gimp_crop_tool_set_lastx    (GimpRectangleTool *tool,
                                         gint               lastx);
gint        gimp_crop_tool_get_lastx    (GimpRectangleTool *tool);
void        gimp_crop_tool_set_lasty    (GimpRectangleTool *tool,
                                         gint               lasty);
gint        gimp_crop_tool_get_lasty    (GimpRectangleTool *tool);

void        gimp_crop_tool_set_pressx   (GimpRectangleTool *tool,
                                         gint               pressx);
gint        gimp_crop_tool_get_pressx   (GimpRectangleTool *tool);
void        gimp_crop_tool_set_pressy   (GimpRectangleTool *tool,
                                         gint               pressy);
gint        gimp_crop_tool_get_pressy   (GimpRectangleTool *tool);

void        gimp_crop_tool_set_x1       (GimpRectangleTool *tool,
                                         gint               x1);
gint        gimp_crop_tool_get_x1       (GimpRectangleTool *tool);
void        gimp_crop_tool_set_y1       (GimpRectangleTool *tool,
                                         gint               y1);
gint        gimp_crop_tool_get_y1       (GimpRectangleTool *tool);
void        gimp_crop_tool_set_x2       (GimpRectangleTool *tool,
                                         gint               x2);
gint        gimp_crop_tool_get_x2       (GimpRectangleTool *tool);
void        gimp_crop_tool_set_y2       (GimpRectangleTool *tool,
                                         gint               y2);
gint        gimp_crop_tool_get_y2       (GimpRectangleTool *tool);

void        gimp_crop_tool_set_function (GimpRectangleTool *tool,
                                         guint              function);
guint       gimp_crop_tool_get_function (GimpRectangleTool *tool);

void        gimp_crop_tool_set_dx1      (GimpRectangleTool *tool,
                                         gint               dx1);
gint        gimp_crop_tool_get_dx1      (GimpRectangleTool *tool);
void        gimp_crop_tool_set_dy1      (GimpRectangleTool *tool,
                                         gint               dy1);
gint        gimp_crop_tool_get_dy1      (GimpRectangleTool *tool);
void        gimp_crop_tool_set_dx2      (GimpRectangleTool *tool,
                                         gint               dx2);
gint        gimp_crop_tool_get_dx2      (GimpRectangleTool *tool);
void        gimp_crop_tool_set_dy2      (GimpRectangleTool *tool,
                                         gint               dy2);
gint        gimp_crop_tool_get_dy2      (GimpRectangleTool *tool);

void        gimp_crop_tool_set_dcw      (GimpRectangleTool *tool,
                                         gint               dcw);
gint        gimp_crop_tool_get_dcw      (GimpRectangleTool *tool);
void        gimp_crop_tool_set_dch      (GimpRectangleTool *tool,
                                         gint               dch);
gint        gimp_crop_tool_get_dch      (GimpRectangleTool *tool);
void        gimp_crop_tool_set_origx    (GimpRectangleTool *tool,
                                         gdouble            origx);
gdouble     gimp_crop_tool_get_origx    (GimpRectangleTool *tool);
void        gimp_crop_tool_set_origy    (GimpRectangleTool *tool,
                                         gdouble            origy);
gdouble     gimp_crop_tool_get_origy    (GimpRectangleTool *tool);

void        gimp_crop_tool_set_sizew    (GimpRectangleTool *tool,
                                         gdouble            sizew);
gdouble     gimp_crop_tool_get_sizew    (GimpRectangleTool *tool);
void        gimp_crop_tool_set_sizeh    (GimpRectangleTool *tool,
                                         gdouble            sizeh);
gdouble     gimp_crop_tool_get_sizeh    (GimpRectangleTool *tool);

static GimpDrawToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_crop_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_CROP_TOOL,
                GIMP_TYPE_CROP_OPTIONS,
                gimp_crop_options_gui,
                0,
                "gimp-crop-tool",
                _("Crop & Resize"),
                _("Crop or Resize an image"),
                N_("_Crop & Resize"), NULL,
                NULL, GIMP_HELP_TOOL_CROP,
                GIMP_STOCK_TOOL_CROP,
                data);
}

GType
gimp_crop_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpCropToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_crop_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpCropTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_crop_tool_init,
      };
      static const GInterfaceInfo rectangle_info =
      {
        (GInterfaceInitFunc) gimp_crop_tool_rectangle_tool_iface_init,           /* interface_init */
        NULL,           /* interface_finalize */
        NULL            /* interface_data */
      };
      
      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
                                          "GimpCropTool",
                                          &tool_info, 0);
      g_type_add_interface_static (tool_type,
                                   GIMP_TYPE_RECTANGLE_TOOL,
                                   &rectangle_info);
    }

  return tool_type;
}

static void
gimp_crop_tool_class_init (GimpCropToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_crop_tool_constructor;
  object_class->finalize     = gimp_crop_tool_finalize;

  tool_class->initialize     = gimp_rectangle_tool_initialize;
  tool_class->control        = gimp_crop_tool_control;
  tool_class->button_press   = gimp_rectangle_tool_button_press;
  tool_class->button_release = gimp_rectangle_tool_button_release;
  tool_class->motion         = gimp_rectangle_tool_motion;
  tool_class->key_press      = gimp_rectangle_tool_key_press;
  tool_class->modifier_key   = gimp_rectangle_tool_modifier_key;
  tool_class->oper_update    = gimp_rectangle_tool_oper_update;
  tool_class->cursor_update  = gimp_crop_tool_cursor_update;

  draw_tool_class->draw      = gimp_rectangle_tool_draw;
}

static void
gimp_crop_tool_init (GimpCropTool *crop_tool)
{
  GimpTool           *tool      = GIMP_TOOL (crop_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_CROP);
}

static void
gimp_crop_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *rectangle_iface)
{
  rectangle_iface->set_controls = gimp_crop_tool_set_controls;
  rectangle_iface->get_controls = gimp_crop_tool_get_controls;
  rectangle_iface->set_entry    = gimp_crop_tool_set_entry;
  rectangle_iface->get_entry    = gimp_crop_tool_get_entry;
  rectangle_iface->set_startx   = gimp_crop_tool_set_startx;
  rectangle_iface->get_startx   = gimp_crop_tool_get_startx;
  rectangle_iface->set_starty   = gimp_crop_tool_set_starty;
  rectangle_iface->get_starty   = gimp_crop_tool_get_starty;
  rectangle_iface->set_lastx    = gimp_crop_tool_set_lastx;
  rectangle_iface->get_lastx    = gimp_crop_tool_get_lastx;
  rectangle_iface->set_lasty    = gimp_crop_tool_set_lasty;
  rectangle_iface->get_lasty    = gimp_crop_tool_get_lasty;
  rectangle_iface->set_pressx   = gimp_crop_tool_set_pressx;
  rectangle_iface->get_pressx   = gimp_crop_tool_get_pressx;
  rectangle_iface->set_pressy   = gimp_crop_tool_set_pressy;
  rectangle_iface->get_pressy   = gimp_crop_tool_get_pressy;
  rectangle_iface->set_x1       = gimp_crop_tool_set_x1;
  rectangle_iface->get_x1       = gimp_crop_tool_get_x1;
  rectangle_iface->set_y1       = gimp_crop_tool_set_y1;
  rectangle_iface->get_y1       = gimp_crop_tool_get_y1;
  rectangle_iface->set_x2       = gimp_crop_tool_set_x2;
  rectangle_iface->get_x2       = gimp_crop_tool_get_x2;
  rectangle_iface->set_y2       = gimp_crop_tool_set_y2;
  rectangle_iface->get_y2       = gimp_crop_tool_get_y2;
  rectangle_iface->set_function = gimp_crop_tool_set_function;
  rectangle_iface->get_function = gimp_crop_tool_get_function;
  rectangle_iface->set_dx1      = gimp_crop_tool_set_dx1;
  rectangle_iface->get_dx1      = gimp_crop_tool_get_dx1;
  rectangle_iface->set_dy1      = gimp_crop_tool_set_dy1;
  rectangle_iface->get_dy1      = gimp_crop_tool_get_dy1;
  rectangle_iface->set_dx2      = gimp_crop_tool_set_dx2;
  rectangle_iface->get_dx2      = gimp_crop_tool_get_dx2;
  rectangle_iface->set_dy2      = gimp_crop_tool_set_dy2;
  rectangle_iface->get_dy2      = gimp_crop_tool_get_dy2;
  rectangle_iface->set_dcw      = gimp_crop_tool_set_dcw;
  rectangle_iface->get_dcw      = gimp_crop_tool_get_dcw;
  rectangle_iface->set_dch      = gimp_crop_tool_set_dch;
  rectangle_iface->get_dch      = gimp_crop_tool_get_dch;
  rectangle_iface->set_origx    = gimp_crop_tool_set_origx;
  rectangle_iface->get_origx    = gimp_crop_tool_get_origx;
  rectangle_iface->set_origy    = gimp_crop_tool_set_origy;
  rectangle_iface->get_origy    = gimp_crop_tool_get_origy;
  rectangle_iface->set_sizew    = gimp_crop_tool_set_sizew;
  rectangle_iface->get_sizew    = gimp_crop_tool_get_sizew;
  rectangle_iface->set_sizeh    = gimp_crop_tool_set_sizeh;
  rectangle_iface->get_sizeh    = gimp_crop_tool_get_sizeh;
  rectangle_iface->execute      = gimp_crop_tool_execute;
}

static GObject *
gimp_crop_tool_constructor (GType                  type,
                            guint                  n_params,
                            GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_rectangle_tool_constructor (object);

  return object;
}

static void
gimp_crop_tool_finalize (GObject *object)
{
  gimp_rectangle_tool_finalize (object);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_crop_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *gdisp)
{
  gimp_rectangle_tool_control (tool, action, gdisp);

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_crop_tool_cursor_update (GimpTool        *tool,
                              GimpCoords      *coords,
                              GdkModifierType  state,
                              GimpDisplay     *gdisp)
{
  gimp_rectangle_tool_cursor_update (tool, coords, state, gdisp);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_CROP);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static gboolean
gimp_crop_tool_execute (GimpRectangleTool  *rectangle,
                        gint                x,
                        gint                y,
                        gint                w,
                        gint                h)
{
  GimpTool        *tool      = GIMP_TOOL (rectangle);
  GimpCropOptions *options;
  GimpImage       *gimage;
  gboolean         rectangle_exists;

  options = GIMP_CROP_OPTIONS (tool->tool_info->tool_options);

  gimage = tool->gdisp->gimage;

  rectangle_exists = (w > 0 && h > 0);

  /* if rectangle exists, crop it */
  if (rectangle_exists)
    {
      gimp_image_crop (gimage, GIMP_CONTEXT (options),
                       x, y, w + x, h + y,
                       options->layer_only,
                       options->crop_mode == GIMP_CROP_MODE_CROP);
      
      gimp_image_flush (gimage);

      return TRUE;
    }

  return TRUE;
}

void
gimp_crop_tool_set_controls (GimpRectangleTool *tool,
                             GtkWidget         *controls)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->controls = controls;
}

GtkWidget *
gimp_crop_tool_get_controls (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->controls;
}

void
gimp_crop_tool_set_entry (GimpRectangleTool *tool,
                          GtkWidget         *entry)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->dimensions_entry = entry;
}

GtkWidget *
gimp_crop_tool_get_entry (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->dimensions_entry;
}

void
gimp_crop_tool_set_startx (GimpRectangleTool *tool,
                           gint               startx)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->startx = startx;
}

gint
gimp_crop_tool_get_startx (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->startx;
}

void
gimp_crop_tool_set_starty (GimpRectangleTool *tool,
                           gint               starty)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->starty = starty;
}

gint
gimp_crop_tool_get_starty (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->starty;
}

void
gimp_crop_tool_set_lastx (GimpRectangleTool *tool,
                          gint               lastx)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->lastx = lastx;
}

gint
gimp_crop_tool_get_lastx (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->lastx;
}

void
gimp_crop_tool_set_lasty (GimpRectangleTool *tool,
                          gint               lasty)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->lasty = lasty;
}

gint
gimp_crop_tool_get_lasty (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->lasty;
}

void
gimp_crop_tool_set_pressx (GimpRectangleTool *tool,
                           gint               pressx)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->pressx = pressx;
}

gint
gimp_crop_tool_get_pressx (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->pressx;
}

void
gimp_crop_tool_set_pressy (GimpRectangleTool *tool,
                           gint               pressy)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->pressy = pressy;
}

gint
gimp_crop_tool_get_pressy (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->pressy;
}

void
gimp_crop_tool_set_x1 (GimpRectangleTool *tool,
                       gint               x1)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->x1 = x1;
}

gint
gimp_crop_tool_get_x1 (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->x1;
}

void
gimp_crop_tool_set_y1 (GimpRectangleTool *tool,
                       gint               y1)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->y1 = y1;
}

gint
gimp_crop_tool_get_y1 (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->y1;
}

void
gimp_crop_tool_set_x2 (GimpRectangleTool *tool,
                       gint               x2)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->x2 = x2;
}

gint
gimp_crop_tool_get_x2 (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->x2;
}

void
gimp_crop_tool_set_y2 (GimpRectangleTool *tool,
                       gint               y2)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->y2 = y2;
}

gint
gimp_crop_tool_get_y2 (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->y2;
}

void
gimp_crop_tool_set_function (GimpRectangleTool *tool,
                             guint              function)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->function = function;
}

guint
gimp_crop_tool_get_function (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->function;
}

void
gimp_crop_tool_set_dx1 (GimpRectangleTool *tool,
                        gint               dx1)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->dx1 = dx1;
}

gint
gimp_crop_tool_get_dx1 (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->dx1;
}

void
gimp_crop_tool_set_dy1 (GimpRectangleTool *tool,
                        gint               dy1)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->dy1 = dy1;
}

gint
gimp_crop_tool_get_dy1 (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->dy1;
}

void
gimp_crop_tool_set_dx2 (GimpRectangleTool *tool,
                        gint               dx2)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->dx2 = dx2;
}

gint
gimp_crop_tool_get_dx2 (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->dx2;
}

void
gimp_crop_tool_set_dy2 (GimpRectangleTool *tool,
                        gint               dy2)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->dy2 = dy2;
}

gint
gimp_crop_tool_get_dy2 (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->dy2;
}

void
gimp_crop_tool_set_dcw (GimpRectangleTool *tool,
                        gint               dcw)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->dcw = dcw;
}

gint
gimp_crop_tool_get_dcw (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->dcw;
}

void
gimp_crop_tool_set_dch (GimpRectangleTool *tool,
                        gint               dch)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->dch = dch;
}

gint
gimp_crop_tool_get_dch (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->dch;
}

void
gimp_crop_tool_set_origx (GimpRectangleTool *tool,
                          gdouble            origx)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->origx = origx;
}

gdouble
gimp_crop_tool_get_origx (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->origx;
}

void
gimp_crop_tool_set_origy (GimpRectangleTool *tool,
                          gdouble            origy)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->origy = origy;
}

gdouble
gimp_crop_tool_get_origy (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->origy;
}

void
gimp_crop_tool_set_sizew (GimpRectangleTool *tool,
                          gdouble            sizew)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->sizew = sizew;
}

gdouble
gimp_crop_tool_get_sizew (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->sizew;
}

void
gimp_crop_tool_set_sizeh (GimpRectangleTool *tool,
                          gdouble            sizeh)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  crop_tool->sizeh = sizeh;
}

gdouble
gimp_crop_tool_get_sizeh (GimpRectangleTool *tool)
{
  GimpCropTool *crop_tool = GIMP_CROP_TOOL (tool);
  return crop_tool->sizeh;
}

