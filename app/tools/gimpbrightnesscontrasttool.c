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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "operations/ligmabrightnesscontrastconfig.h"

#include "core/ligmadrawable.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmawidgets-constructors.h"

#include "display/ligmadisplay.h"

#include "ligmabrightnesscontrasttool.h"
#include "ligmafilteroptions.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


#define SLIDER_WIDTH 200


static gboolean   ligma_brightness_contrast_tool_initialize (LigmaTool              *tool,
                                                            LigmaDisplay           *display,
                                                            GError               **error);
static void   ligma_brightness_contrast_tool_button_press   (LigmaTool              *tool,
                                                            const LigmaCoords      *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            LigmaButtonPressType    press_type,
                                                            LigmaDisplay           *display);
static void   ligma_brightness_contrast_tool_button_release (LigmaTool              *tool,
                                                            const LigmaCoords      *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            LigmaButtonReleaseType  release_type,
                                                            LigmaDisplay           *display);
static void   ligma_brightness_contrast_tool_motion         (LigmaTool              *tool,
                                                            const LigmaCoords      *coords,
                                                            guint32                time,
                                                            GdkModifierType        state,
                                                            LigmaDisplay           *display);

static gchar *
              ligma_brightness_contrast_tool_get_operation  (LigmaFilterTool        *filter_tool,
                                                            gchar                **description);
static void   ligma_brightness_contrast_tool_dialog         (LigmaFilterTool        *filter_tool);

static void   brightness_contrast_to_levels_callback       (GtkWidget             *widget,
                                                            LigmaFilterTool        *filter_tool);


G_DEFINE_TYPE (LigmaBrightnessContrastTool, ligma_brightness_contrast_tool,
               LIGMA_TYPE_FILTER_TOOL)

#define parent_class ligma_brightness_contrast_tool_parent_class


void
ligma_brightness_contrast_tool_register (LigmaToolRegisterCallback  callback,
                                        gpointer                  data)
{
  (* callback) (LIGMA_TYPE_BRIGHTNESS_CONTRAST_TOOL,
                LIGMA_TYPE_FILTER_OPTIONS, NULL,
                0,
                "ligma-brightness-contrast-tool",
                _("Brightness-Contrast"),
                _("Adjust brightness and contrast"),
                N_("B_rightness-Contrast..."), NULL,
                NULL, LIGMA_HELP_TOOL_BRIGHTNESS_CONTRAST,
                LIGMA_ICON_TOOL_BRIGHTNESS_CONTRAST,
                data);
}

static void
ligma_brightness_contrast_tool_class_init (LigmaBrightnessContrastToolClass *klass)
{
  LigmaToolClass       *tool_class        = LIGMA_TOOL_CLASS (klass);
  LigmaFilterToolClass *filter_tool_class = LIGMA_FILTER_TOOL_CLASS (klass);

  tool_class->initialize           = ligma_brightness_contrast_tool_initialize;
  tool_class->button_press         = ligma_brightness_contrast_tool_button_press;
  tool_class->button_release       = ligma_brightness_contrast_tool_button_release;
  tool_class->motion               = ligma_brightness_contrast_tool_motion;

  filter_tool_class->get_operation = ligma_brightness_contrast_tool_get_operation;
  filter_tool_class->dialog        = ligma_brightness_contrast_tool_dialog;
}

static void
ligma_brightness_contrast_tool_init (LigmaBrightnessContrastTool *bc_tool)
{
}

static gboolean
ligma_brightness_contrast_tool_initialize (LigmaTool     *tool,
                                          LigmaDisplay  *display,
                                          GError      **error)
{
  LigmaBrightnessContrastTool *bc_tool   = LIGMA_BRIGHTNESS_CONTRAST_TOOL (tool);
  LigmaImage                  *image     = ligma_display_get_image (display);
  GList                      *drawables;

  if (! LIGMA_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  drawables = ligma_image_get_selected_drawables (image);
  /* Single drawable selection has been checked in parent initialize(). */
  g_return_val_if_fail (g_list_length (drawables) == 1, FALSE);

  if (ligma_drawable_get_component_type (drawables->data) == LIGMA_COMPONENT_TYPE_U8)
    {
      ligma_prop_widget_set_factor (bc_tool->brightness_scale,
                                   127.0, 1.0, 8.0, 0);
      ligma_prop_widget_set_factor (bc_tool->contrast_scale,
                                   127.0, 1.0, 8.0, 0);
    }
  else
    {
      ligma_prop_widget_set_factor (bc_tool->brightness_scale,
                                   0.5, 0.01, 0.1, 3);
      ligma_prop_widget_set_factor (bc_tool->contrast_scale,
                                   0.5, 0.01, 0.1, 3);
    }

  g_list_free (drawables);

  return TRUE;
}

static gchar *
ligma_brightness_contrast_tool_get_operation (LigmaFilterTool  *filter_tool,
                                             gchar          **description)
{
  *description = g_strdup (_("Adjust Brightness and Contrast"));

  return g_strdup ("ligma:brightness-contrast");
}

static void
ligma_brightness_contrast_tool_button_press (LigmaTool            *tool,
                                            const LigmaCoords    *coords,
                                            guint32              time,
                                            GdkModifierType      state,
                                            LigmaButtonPressType  press_type,
                                            LigmaDisplay         *display)
{
  LigmaBrightnessContrastTool *bc_tool = LIGMA_BRIGHTNESS_CONTRAST_TOOL (tool);

  bc_tool->dragging = ! ligma_filter_tool_on_guide (LIGMA_FILTER_TOOL (tool),
                                                   coords, display);

  if (! bc_tool->dragging)
    {
      LIGMA_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else
    {
      gdouble brightness;
      gdouble contrast;

      g_object_get (LIGMA_FILTER_TOOL (tool)->config,
                    "brightness", &brightness,
                    "contrast",   &contrast,
                    NULL);

      bc_tool->x  = coords->x - contrast   * 127.0;
      bc_tool->y  = coords->y + brightness * 127.0;
      bc_tool->dx =   contrast   * 127.0;
      bc_tool->dy = - brightness * 127.0;

      tool->display = display;

      ligma_tool_control_activate (tool->control);
    }
}

static void
ligma_brightness_contrast_tool_button_release (LigmaTool              *tool,
                                              const LigmaCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              LigmaButtonReleaseType  release_type,
                                              LigmaDisplay           *display)
{
  LigmaBrightnessContrastTool *bc_tool = LIGMA_BRIGHTNESS_CONTRAST_TOOL (tool);

  if (! bc_tool->dragging)
    {
      LIGMA_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
  else
    {
      ligma_tool_control_halt (tool->control);

      bc_tool->dragging = FALSE;

      if (bc_tool->dx == 0 && bc_tool->dy == 0)
        return;

      if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
        ligma_config_reset (LIGMA_CONFIG (LIGMA_FILTER_TOOL (tool)->config));
    }
}

static void
ligma_brightness_contrast_tool_motion (LigmaTool         *tool,
                                      const LigmaCoords *coords,
                                      guint32           time,
                                      GdkModifierType   state,
                                      LigmaDisplay      *display)
{
  LigmaBrightnessContrastTool *bc_tool = LIGMA_BRIGHTNESS_CONTRAST_TOOL (tool);

  if (! bc_tool->dragging)
    {
      LIGMA_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
  else
    {
      bc_tool->dx =   (coords->x - bc_tool->x);
      bc_tool->dy = - (coords->y - bc_tool->y);

      g_object_set (LIGMA_FILTER_TOOL (tool)->config,
                    "brightness", CLAMP (bc_tool->dy, -127.0, 127.0) / 127.0,
                    "contrast",   CLAMP (bc_tool->dx, -127.0, 127.0) / 127.0,
                    NULL);
    }
}


/********************************/
/*  Brightness Contrast dialog  */
/********************************/

static void
ligma_brightness_contrast_tool_dialog (LigmaFilterTool *filter_tool)
{
  LigmaBrightnessContrastTool *bc_tool = LIGMA_BRIGHTNESS_CONTRAST_TOOL (filter_tool);
  GtkWidget                  *main_vbox;
  GtkWidget                  *scale;
  GtkWidget                  *button;

  main_vbox = ligma_filter_tool_dialog_get_vbox (filter_tool);

  /*  Create the brightness scale widget  */
  scale = ligma_prop_spin_scale_new (filter_tool->config, "brightness",
                                    0.01, 0.1, 3);
  ligma_spin_scale_set_label (LIGMA_SPIN_SCALE (scale), _("_Brightness"));
  gtk_box_pack_start (GTK_BOX (main_vbox), scale, FALSE, FALSE, 0);

  bc_tool->brightness_scale = scale;

  /*  Create the contrast scale widget  */
  scale = ligma_prop_spin_scale_new (filter_tool->config, "contrast",
                                    0.01, 0.1, 3);
  ligma_spin_scale_set_label (LIGMA_SPIN_SCALE (scale), _("_Contrast"));
  gtk_box_pack_start (GTK_BOX (main_vbox), scale, FALSE, FALSE, 0);

  bc_tool->contrast_scale = scale;

  button = ligma_icon_button_new (LIGMA_ICON_TOOL_LEVELS,
                                 _("Edit these Settings as Levels"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (brightness_contrast_to_levels_callback),
                    filter_tool);
}

static void
brightness_contrast_to_levels_callback (GtkWidget      *widget,
                                        LigmaFilterTool *filter_tool)
{
  LigmaLevelsConfig *levels;

  levels = ligma_brightness_contrast_config_to_levels_config (LIGMA_BRIGHTNESS_CONTRAST_CONFIG (filter_tool->config));

  ligma_filter_tool_edit_as (filter_tool,
                            "ligma-levels-tool",
                            LIGMA_CONFIG (levels));

  g_object_unref (levels);
}
