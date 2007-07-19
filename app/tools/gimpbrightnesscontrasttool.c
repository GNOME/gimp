/* GIMP - The GNU Image Manipulation Program
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

#include "base/gimplut.h"
#include "base/lut-funcs.h"

#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpbrightnesscontrasttool.h"
#include "gimpimagemapoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define SLIDER_WIDTH 200

#define BRIGHTNESS  0x1
#define CONTRAST    0x2
#define ALL        (BRIGHTNESS | CONTRAST)


static void   gimp_brightness_contrast_tool_finalize       (GObject               *object);

static gboolean gimp_brightness_contrast_tool_initialize   (GimpTool              *tool,
                                                            GimpDisplay           *display,
                                                            GError               **error);

static void   gimp_brightness_contrast_tool_button_press   (GimpTool              *tool,
                                                            GimpCoords            *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            GimpDisplay           *display);
static void   gimp_brightness_contrast_tool_button_release (GimpTool              *tool,
                                                            GimpCoords            *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            GimpButtonReleaseType  release_type,
                                                            GimpDisplay           *display);
static void   gimp_brightness_contrast_tool_motion         (GimpTool              *tool,
                                                            GimpCoords            *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            GimpDisplay           *display);

static void   gimp_brightness_contrast_tool_map            (GimpImageMapTool      *image_map_tool);
static void   gimp_brightness_contrast_tool_dialog         (GimpImageMapTool      *image_map_tool);
static void   gimp_brightness_contrast_tool_reset          (GimpImageMapTool      *image_map_tool);

static void   brightness_contrast_update                   (GimpBrightnessContrastTool *bc_tool,
                                                            gint                        update);
static void   brightness_contrast_brightness_changed       (GtkAdjustment              *adj,
                                                            GimpBrightnessContrastTool *bc_tool);
static void   brightness_contrast_contrast_changed         (GtkAdjustment              *adj,
                                                            GimpBrightnessContrastTool *bc_tool);


G_DEFINE_TYPE (GimpBrightnessContrastTool, gimp_brightness_contrast_tool,
               GIMP_TYPE_IMAGE_MAP_TOOL)

#define parent_class gimp_brightness_contrast_tool_parent_class


void
gimp_brightness_contrast_tool_register (GimpToolRegisterCallback  callback,
                                        gpointer                  data)
{
  (* callback) (GIMP_TYPE_BRIGHTNESS_CONTRAST_TOOL,
                GIMP_TYPE_IMAGE_MAP_OPTIONS, NULL,
                0,
                "gimp-brightness-contrast-tool",
                _("Brightness-Contrast"),
                _("Brightness/Contrast Tool: Adjust brightness and contrast"),
                N_("B_rightness-Contrast..."), NULL,
                NULL, GIMP_HELP_TOOL_BRIGHTNESS_CONTRAST,
                GIMP_STOCK_TOOL_BRIGHTNESS_CONTRAST,
                data);
}

static void
gimp_brightness_contrast_tool_class_init (GimpBrightnessContrastToolClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpToolClass         *tool_class    = GIMP_TOOL_CLASS (klass);
  GimpImageMapToolClass *im_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  object_class->finalize     = gimp_brightness_contrast_tool_finalize;

  tool_class->initialize     = gimp_brightness_contrast_tool_initialize;
  tool_class->button_press   = gimp_brightness_contrast_tool_button_press;
  tool_class->button_release = gimp_brightness_contrast_tool_button_release;
  tool_class->motion         = gimp_brightness_contrast_tool_motion;

  im_tool_class->shell_desc  = _("Adjust Brightness and Contrast");

  im_tool_class->map         = gimp_brightness_contrast_tool_map;
  im_tool_class->dialog      = gimp_brightness_contrast_tool_dialog;
  im_tool_class->reset       = gimp_brightness_contrast_tool_reset;
}

static void
gimp_brightness_contrast_tool_init (GimpBrightnessContrastTool *bc_tool)
{
  bc_tool->brightness = 0.0;
  bc_tool->contrast   = 0.0;
  bc_tool->lut        = gimp_lut_new ();
}

static void
gimp_brightness_contrast_tool_finalize (GObject *object)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (object);

  if (bc_tool->lut)
    {
      gimp_lut_free (bc_tool->lut);
      bc_tool->lut = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_brightness_contrast_tool_initialize (GimpTool     *tool,
                                          GimpDisplay  *display,
                                          GError      **error)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (tool);
  GimpDrawable               *drawable;

  drawable = gimp_image_get_active_drawable (display->image);

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error (error, 0, 0,
                   _("Brightness-Contrast does not operate on indexed layers."));
      return FALSE;
    }

  bc_tool->brightness = 0.0;
  bc_tool->contrast   = 0.0;

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);

  brightness_contrast_update (bc_tool, ALL);

  return TRUE;
}

static void
gimp_brightness_contrast_tool_map (GimpImageMapTool *im_tool)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (im_tool);

  brightness_contrast_lut_setup (bc_tool->lut,
                                 bc_tool->brightness / 255.0,
                                 bc_tool->contrast / 127.0,
                                 gimp_drawable_bytes (im_tool->drawable));
  gimp_image_map_apply (im_tool->image_map,
                        (GimpImageMapApplyFunc) gimp_lut_process,
                        bc_tool->lut);
}


static void
gimp_brightness_contrast_tool_button_press (GimpTool        *tool,
                                            GimpCoords      *coords,
                                            guint32          time,
                                            GdkModifierType  state,
                                            GimpDisplay     *display)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (tool);

  bc_tool->x = coords->x - bc_tool->contrast;
  bc_tool->y = coords->y + bc_tool->brightness;
  bc_tool->dx =   bc_tool->contrast;
  bc_tool->dy = - bc_tool->brightness;

  gimp_tool_control_activate (tool->control);
  tool->display = display;
}

static void
gimp_brightness_contrast_tool_button_release (GimpTool              *tool,
                                              GimpCoords            *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              GimpButtonReleaseType  release_type,
                                              GimpDisplay           *display)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (tool);
  GimpImageMapTool           *im_tool = GIMP_IMAGE_MAP_TOOL (tool);

  gimp_tool_control_halt (tool->control);

  if (bc_tool->dx == 0 && bc_tool->dy == 0)
    return;

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    gimp_brightness_contrast_tool_reset (im_tool);

  gimp_image_map_tool_preview (im_tool);
}


static void
gimp_brightness_contrast_tool_motion (GimpTool        *tool,
                                      GimpCoords      *coords,
                                      guint32          time,
                                      GdkModifierType  state,
                                      GimpDisplay     *display)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (tool);
  GimpImageMapTool           *im_tool = GIMP_IMAGE_MAP_TOOL (tool);

  gimp_tool_control_pause (tool->control);

  bc_tool->dx =   (coords->x - bc_tool->x);
  bc_tool->dy = - (coords->y - bc_tool->y);

  bc_tool->brightness = CLAMP (bc_tool->dy, -127.0, 127.0);
  bc_tool->contrast   = CLAMP (bc_tool->dx, -127.0, 127.0);

  brightness_contrast_update (bc_tool, ALL);
  gimp_image_map_tool_preview (im_tool);

  gimp_tool_control_resume (tool->control);
}


/********************************/
/*  Brightness Contrast dialog  */
/********************************/

static void
gimp_brightness_contrast_tool_dialog (GimpImageMapTool *im_tool)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (im_tool);
  GtkWidget                  *table;
  GtkWidget                  *slider;
  GtkObject                  *data;

  /*  The table containing sliders  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (im_tool->main_vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Create the brightness scale widget  */
  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                               _("_Brightness:"), SLIDER_WIDTH, -1,
                               bc_tool->brightness,
                               -127.0, 127.0, 1.0, 10.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  bc_tool->brightness_data = GTK_ADJUSTMENT (data);
  slider = GIMP_SCALE_ENTRY_SCALE (data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);

  g_signal_connect (data, "value-changed",
                    G_CALLBACK (brightness_contrast_brightness_changed),
                    bc_tool);

  /*  Create the contrast scale widget  */
  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                               _("Con_trast:"), SLIDER_WIDTH, -1,
                               bc_tool->contrast,
                               -127.0, 127.0, 1.0, 10.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  bc_tool->contrast_data = GTK_ADJUSTMENT (data);
  slider = GIMP_SCALE_ENTRY_SCALE (data);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_CONTINUOUS);

  g_signal_connect (data, "value-changed",
                    G_CALLBACK (brightness_contrast_contrast_changed),
                    bc_tool);
}

static void
gimp_brightness_contrast_tool_reset (GimpImageMapTool *im_tool)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (im_tool);

  bc_tool->brightness = 0.0;
  bc_tool->contrast   = 0.0;

  brightness_contrast_update (bc_tool, ALL);
}

static void
brightness_contrast_update (GimpBrightnessContrastTool *bc_tool,
                            gint                        update)
{
  if (update & BRIGHTNESS)
    gtk_adjustment_set_value (bc_tool->brightness_data, bc_tool->brightness);

  if (update & CONTRAST)
    gtk_adjustment_set_value (bc_tool->contrast_data, bc_tool->contrast);
}

static void
brightness_contrast_brightness_changed (GtkAdjustment              *adjustment,
                                        GimpBrightnessContrastTool *bc_tool)
{
  if (bc_tool->brightness != adjustment->value)
    {
      bc_tool->brightness = adjustment->value;

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (bc_tool));
    }
}

static void
brightness_contrast_contrast_changed (GtkAdjustment              *adjustment,
                                      GimpBrightnessContrastTool *bc_tool)
{
  if (bc_tool->contrast != adjustment->value)
    {
      bc_tool->contrast = adjustment->value;

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (bc_tool));
    }
}
