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

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpdrawable-foreground-extract.h"
#include "core/gimpimage.h"
#include "core/gimpscanconvert.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpforegroundselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void   gimp_foreground_select_tool_class_init (GimpForegroundSelectToolClass *klass);
static void   gimp_foreground_select_tool_init       (GimpForegroundSelectTool      *fg_select);
static void   gimp_foreground_select_tool_finalize       (GObject         *object);

static void   gimp_foreground_select_tool_button_press   (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static void   gimp_foreground_select_tool_button_release (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);
static void   gimp_foreground_select_tool_motion         (GimpTool        *tool,
                                                          GimpCoords      *coords,
                                                          guint32          time,
                                                          GdkModifierType  state,
                                                          GimpDisplay     *gdisp);

static void   gimp_foreground_select_tool_draw           (GimpDrawTool    *draw_tool);

static void   gimp_foreground_select_tool_select      (GimpFreeSelectTool *free_sel,
                                                       GimpImage          *gimage);


static GimpFreeSelectToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_foreground_select_tool_register (GimpToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (GIMP_TYPE_FOREGROUND_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-foreground-select-tool",
                _("Foreground Select"),
                _("Extract foreground"),
                N_("_Foreground Select"), NULL,
                NULL, GIMP_HELP_TOOL_FOREGROUND_SELECT,
                GIMP_STOCK_TOOL_FUZZY_SELECT,
                data);
}

GType
gimp_foreground_select_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpForegroundSelectToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_foreground_select_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpForegroundSelectTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_foreground_select_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_FREE_SELECT_TOOL,
                                          "GimpForegroundSelectTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

/*  private functions  */

static void
gimp_foreground_select_tool_class_init (GimpForegroundSelectToolClass *klass)
{
  GObjectClass            *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass           *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass       *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  GimpFreeSelectToolClass *free_select_tool_class = GIMP_FREE_SELECT_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize         = gimp_foreground_select_tool_finalize;

  tool_class->button_press       = gimp_foreground_select_tool_button_press;
  tool_class->button_release     = gimp_foreground_select_tool_button_release;
  tool_class->motion             = gimp_foreground_select_tool_motion;

  draw_tool_class->draw          = gimp_foreground_select_tool_draw;

  free_select_tool_class->select = gimp_foreground_select_tool_select;
}

static void
gimp_foreground_select_tool_init (GimpForegroundSelectTool *fg_select)
{
  GimpTool *tool = GIMP_TOOL (fg_select);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_FREE_SELECT);

  fg_select->mask = NULL;
}

static void
gimp_foreground_select_tool_finalize (GObject *object)
{
  GimpForegroundSelectTool *fg_select = GIMP_FOREGROUND_SELECT_TOOL (object);

  if (fg_select->mask)
    {
      g_object_unref (fg_select->mask);
      fg_select->mask = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_foreground_select_tool_button_press (GimpTool        *tool,
                                          GimpCoords      *coords,
                                          guint32          time,
                                          GdkModifierType  state,
                                          GimpDisplay     *gdisp)
{
  GIMP_TOOL_CLASS (parent_class)->button_press (tool,
                                                coords, time, state, gdisp);
}

static void
gimp_foreground_select_tool_button_release (GimpTool        *tool,
                                            GimpCoords      *coords,
                                            guint32          time,
                                            GdkModifierType  state,
                                            GimpDisplay     *gdisp)
{
  GIMP_TOOL_CLASS (parent_class)->button_release (tool,
                                                  coords, time, state, gdisp);
}

static void
gimp_foreground_select_tool_motion (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  GIMP_TOOL_CLASS (parent_class)->motion (tool,
                                          coords, time, state, gdisp);
}

static void
gimp_foreground_select_tool_draw (GimpDrawTool *draw_tool)
{
  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
gimp_foreground_select_tool_select (GimpFreeSelectTool *free_sel,
                                    GimpImage          *gimage)
{
  GimpTool             *tool     = GIMP_TOOL (free_sel);
  GimpDrawable         *drawable = gimp_image_active_drawable (gimage);
  GimpScanConvert      *scan_convert;
  GimpChannel          *mask;
  GimpSelectionOptions *options;

  if (! drawable)
    return;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_set_busy (gimage->gimp);

  scan_convert = gimp_scan_convert_new ();
  gimp_scan_convert_add_polyline (scan_convert,
                                  free_sel->num_points, free_sel->points, TRUE);

  mask = gimp_channel_new (gimage,
                           gimp_image_get_width (gimage),
                           gimp_image_get_height (gimage),
                           "foreground-extraction", NULL);

  gimp_scan_convert_render_value (scan_convert,
                                  gimp_drawable_data (GIMP_DRAWABLE (mask)),
                                  0, 0, 127);
  gimp_scan_convert_free (scan_convert);

  gimp_drawable_foreground_extract (drawable,
                                    GIMP_FOREGROUND_EXTRACT_SIOX,
                                    GIMP_DRAWABLE (mask));

  gimp_channel_select_channel (gimp_image_get_mask (gimage),
                               tool->tool_info->blurb,
                               mask, 0, 0,
                               GIMP_SELECTION_TOOL (tool)->op,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius);

  g_object_unref (mask);

  gimp_unset_busy (gimage->gimp);
}
