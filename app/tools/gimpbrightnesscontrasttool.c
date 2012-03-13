/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/gimplut.h"
#include "base/lut-funcs.h"

#include "gegl/gimpbrightnesscontrastconfig.h"

#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-constructors.h"

#include "display/gimpdisplay.h"

#include "gimpbrightnesscontrasttool.h"
#include "gimpimagemapoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define SLIDER_WIDTH 200


static void   gimp_brightness_contrast_tool_finalize       (GObject               *object);

static gboolean gimp_brightness_contrast_tool_initialize   (GimpTool              *tool,
                                                            GimpDisplay           *display,
                                                            GError               **error);

static void   gimp_brightness_contrast_tool_button_press   (GimpTool              *tool,
                                                            const GimpCoords      *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            GimpButtonPressType    press_type,
                                                            GimpDisplay           *display);
static void   gimp_brightness_contrast_tool_button_release (GimpTool              *tool,
                                                            const GimpCoords      *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            GimpButtonReleaseType  release_type,
                                                            GimpDisplay           *display);
static void   gimp_brightness_contrast_tool_motion         (GimpTool              *tool,
                                                            const GimpCoords      *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            GimpDisplay           *display);

static GeglNode *
              gimp_brightness_contrast_tool_get_operation  (GimpImageMapTool      *image_map_tool,
                                                            GObject              **config);
static void   gimp_brightness_contrast_tool_map            (GimpImageMapTool      *image_map_tool);
static void   gimp_brightness_contrast_tool_dialog         (GimpImageMapTool      *image_map_tool);

static void   brightness_contrast_config_notify            (GObject                    *object,
                                                            GParamSpec                 *pspec,
                                                            GimpBrightnessContrastTool *bc_tool);

static void   brightness_contrast_brightness_changed       (GtkAdjustment              *adj,
                                                            GimpBrightnessContrastTool *bc_tool);
static void   brightness_contrast_contrast_changed         (GtkAdjustment              *adj,
                                                            GimpBrightnessContrastTool *bc_tool);

static void   brightness_contrast_to_levels_callback       (GtkWidget                  *widget,
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

  object_class->finalize             = gimp_brightness_contrast_tool_finalize;

  tool_class->initialize             = gimp_brightness_contrast_tool_initialize;
  tool_class->button_press           = gimp_brightness_contrast_tool_button_press;
  tool_class->button_release         = gimp_brightness_contrast_tool_button_release;
  tool_class->motion                 = gimp_brightness_contrast_tool_motion;

  im_tool_class->dialog_desc         = _("Adjust Brightness and Contrast");
  im_tool_class->settings_name       = "brightness-contrast";
  im_tool_class->import_dialog_title = _("Import Brightness-Contrast settings");
  im_tool_class->export_dialog_title = _("Export Brightness-Contrast settings");

  im_tool_class->get_operation       = gimp_brightness_contrast_tool_get_operation;
  im_tool_class->map                 = gimp_brightness_contrast_tool_map;
  im_tool_class->dialog              = gimp_brightness_contrast_tool_dialog;
}

static void
gimp_brightness_contrast_tool_init (GimpBrightnessContrastTool *bc_tool)
{
  GimpImageMapTool *im_tool = GIMP_IMAGE_MAP_TOOL (bc_tool);

  bc_tool->lut = gimp_lut_new ();

  im_tool->apply_func = (GimpImageMapApplyFunc) gimp_lut_process;
  im_tool->apply_data = bc_tool->lut;
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
  GimpBrightnessContrastTool *bc_tool  = GIMP_BRIGHTNESS_CONTRAST_TOOL (tool);
  GimpImage                  *image    = gimp_display_get_image (display);
  GimpDrawable               *drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Brightness-Contrast does not operate "
			     "on indexed layers."));
      return FALSE;
    }

  gimp_config_reset (GIMP_CONFIG (bc_tool->config));

  return GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);
}

static GeglNode *
gimp_brightness_contrast_tool_get_operation (GimpImageMapTool  *im_tool,
                                             GObject          **config)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (im_tool);
  GeglNode                   *node;

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", "gimp:brightness-contrast",
                       NULL);

  bc_tool->config = g_object_new (GIMP_TYPE_BRIGHTNESS_CONTRAST_CONFIG, NULL);

  *config = G_OBJECT (bc_tool->config);

  g_signal_connect_object (bc_tool->config, "notify",
                           G_CALLBACK (brightness_contrast_config_notify),
                           G_OBJECT (bc_tool), 0);

  gegl_node_set (node,
                 "config", bc_tool->config,
                 NULL);

  return node;
}

static void
gimp_brightness_contrast_tool_map (GimpImageMapTool *im_tool)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (im_tool);

  brightness_contrast_lut_setup (bc_tool->lut,
                                 bc_tool->config->brightness / 2.0,
                                 bc_tool->config->contrast,
                                 gimp_drawable_bytes (im_tool->drawable));
}

static void
gimp_brightness_contrast_tool_button_press (GimpTool            *tool,
                                            const GimpCoords    *coords,
                                            guint32              time,
                                            GdkModifierType      state,
                                            GimpButtonPressType  press_type,
                                            GimpDisplay         *display)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (tool);

  bc_tool->x  = coords->x - bc_tool->config->contrast   * 127.0;
  bc_tool->y  = coords->y + bc_tool->config->brightness * 127.0;
  bc_tool->dx =   bc_tool->config->contrast   * 127.0;
  bc_tool->dy = - bc_tool->config->brightness * 127.0;

  gimp_tool_control_activate (tool->control);
  tool->display = display;
}

static void
gimp_brightness_contrast_tool_button_release (GimpTool              *tool,
                                              const GimpCoords      *coords,
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
    gimp_config_reset (GIMP_CONFIG (bc_tool->config));

  gimp_image_map_tool_preview (im_tool);
}

static void
gimp_brightness_contrast_tool_motion (GimpTool         *tool,
                                      const GimpCoords *coords,
                                      guint32           time,
                                      GdkModifierType   state,
                                      GimpDisplay      *display)
{
  GimpBrightnessContrastTool *bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (tool);

  bc_tool->dx =   (coords->x - bc_tool->x);
  bc_tool->dy = - (coords->y - bc_tool->y);

  g_object_set (bc_tool->config,
                "brightness", CLAMP (bc_tool->dy, -127.0, 127.0) / 127.0,
                "contrast",   CLAMP (bc_tool->dx, -127.0, 127.0) / 127.0,
                NULL);
}


/********************************/
/*  Brightness Contrast dialog  */
/********************************/

static void
gimp_brightness_contrast_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpBrightnessContrastTool   *bc_tool;
  GimpBrightnessContrastConfig *config;
  GtkWidget                    *main_vbox;
  GtkWidget                    *table;
  GtkWidget                    *button;
  GtkObject                    *data;

  bc_tool = GIMP_BRIGHTNESS_CONTRAST_TOOL (image_map_tool);
  config  = bc_tool->config;

  main_vbox = gimp_image_map_tool_dialog_get_vbox (image_map_tool);

  /*  The table containing sliders  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Create the brightness scale widget  */
  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                               _("_Brightness:"), SLIDER_WIDTH, -1,
                               config->brightness * 127.0,
                               -127.0, 127.0, 1.0, 10.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  bc_tool->brightness_data = GTK_ADJUSTMENT (data);

  g_signal_connect (data, "value-changed",
                    G_CALLBACK (brightness_contrast_brightness_changed),
                    bc_tool);

  /*  Create the contrast scale widget  */
  data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                               _("Con_trast:"), SLIDER_WIDTH, -1,
                               config->contrast * 127.0,
                               -127.0, 127.0, 1.0, 10.0, 0,
                               TRUE, 0.0, 0.0,
                               NULL, NULL);
  bc_tool->contrast_data = GTK_ADJUSTMENT (data);

  g_signal_connect (data, "value-changed",
                    G_CALLBACK (brightness_contrast_contrast_changed),
                    bc_tool);

  button = gimp_stock_button_new (GIMP_STOCK_TOOL_LEVELS,
                                  _("Edit these Settings as Levels"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (brightness_contrast_to_levels_callback),
                    bc_tool);
}

static void
brightness_contrast_config_notify (GObject                    *object,
                                   GParamSpec                 *pspec,
                                   GimpBrightnessContrastTool *bc_tool)
{
  GimpBrightnessContrastConfig *config;

  config = GIMP_BRIGHTNESS_CONTRAST_CONFIG (object);

  if (! bc_tool->brightness_data)
    return;

  if (! strcmp (pspec->name, "brightness"))
    {
      gtk_adjustment_set_value (bc_tool->brightness_data,
                                config->brightness * 127.0);
    }
  else if (! strcmp (pspec->name, "contrast"))
    {
      gtk_adjustment_set_value (bc_tool->contrast_data,
                                config->contrast * 127.0);
    }

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (bc_tool));
}

static void
brightness_contrast_brightness_changed (GtkAdjustment              *adjustment,
                                        GimpBrightnessContrastTool *bc_tool)
{
  GimpBrightnessContrastConfig *config = bc_tool->config;
  gdouble                       value;

  value = gtk_adjustment_get_value (adjustment) / 127.0;

  if (config->brightness != value)
    {
      g_object_set (config,
                    "brightness", value,
                    NULL);
    }
}

static void
brightness_contrast_contrast_changed (GtkAdjustment              *adjustment,
                                      GimpBrightnessContrastTool *bc_tool)
{
  GimpBrightnessContrastConfig *config = bc_tool->config;
  gdouble                       value;

  value = gtk_adjustment_get_value (adjustment) / 127.0;

  if (config->contrast != value)
    {
      g_object_set (config,
                    "contrast", value,
                    NULL);
    }
}

static void
brightness_contrast_to_levels_callback (GtkWidget                  *widget,
                                        GimpBrightnessContrastTool *bc_tool)
{
  GimpLevelsConfig *levels;

  levels = gimp_brightness_contrast_config_to_levels_config (bc_tool->config);

  gimp_image_map_tool_edit_as (GIMP_IMAGE_MAP_TOOL (bc_tool),
                               "gimp-levels-tool",
                               GIMP_CONFIG (levels));

  g_object_unref (levels);
}
