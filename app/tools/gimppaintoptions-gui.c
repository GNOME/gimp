/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-constructors.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpairbrushtool.h"
#include "gimpclonetool.h"
#include "gimpconvolvetool.h"
#include "gimpdodgeburntool.h"
#include "gimperasertool.h"
#include "gimphealtool.h"
#include "gimpinktool.h"
#include "gimppaintoptions-gui.h"
#include "gimppenciltool.h"
#include "gimpperspectiveclonetool.h"
#include "gimpsmudgetool.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


static void gimp_paint_options_gui_reset_size  (GtkWidget        *button,
                                                GimpPaintOptions *paint_options);
static void gimp_paint_options_gui_reset_aspect_ratio
                                               (GtkWidget        *button,
                                                GimpPaintOptions *paint_options);
static void gimp_paint_options_gui_reset_angle (GtkWidget        *button,
                                                GimpPaintOptions *paint_options);

static GtkWidget * dynamics_options_gui        (GimpPaintOptions *paint_options,
                                                GType             tool_type);
static GtkWidget * jitter_options_gui          (GimpPaintOptions *paint_options,
                                                GType             tool_type);
static GtkWidget * smoothing_options_gui       (GimpPaintOptions *paint_options,
                                                GType             tool_type);


/*  public functions  */

GtkWidget *
gimp_paint_options_gui (GimpToolOptions *tool_options)
{
  GObject          *config  = G_OBJECT (tool_options);
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (tool_options);
  GtkWidget        *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget        *hbox;
  GtkWidget        *menu;
  GtkWidget        *label;
  GtkWidget        *scale;
  GType             tool_type;

  tool_type = tool_options->tool_info->tool_type;

  /*  the paint mode menu  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Mode:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  menu = gimp_prop_paint_mode_menu_new (config, "paint-mode", TRUE, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), menu, TRUE, TRUE, 0);
  gtk_widget_show (menu);

  if (tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL ||
      tool_type == GIMP_TYPE_HEAL_TOOL       ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      gtk_widget_set_sensitive (menu, FALSE);
      gtk_widget_set_sensitive (label, FALSE);
    }

  /*  the opacity scale  */
  scale = gimp_prop_opacity_spin_scale_new (config, "opacity",
                                            _("Opacity"));
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  the brush  */
  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL))
    {
      GtkWidget *button;
      GtkWidget *hbox;
      GtkWidget *frame;

      button = gimp_prop_brush_box_new (NULL, GIMP_CONTEXT (tool_options),
                                        _("Brush"), 2,
                                        "brush-view-type", "brush-view-size",
                                        "gimp-brush-editor");
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      scale = gimp_prop_spin_scale_new (config, "brush-size",
                                        _("Size"),
                                        1.0, 10.0, 2);
      gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 1000.0);
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

      button = gimp_stock_button_new (GIMP_STOCK_RESET, NULL);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_image_set_from_stock (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (gimp_paint_options_gui_reset_size),
                        options);

      gimp_help_set_help_data (button,
                               _("Reset size to brush's native size"), NULL);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      scale = gimp_prop_spin_scale_new (config, "brush-aspect-ratio",
                                        _("Aspect Ratio"),
                                        0.1, 1.0, 2);
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

      button = gimp_stock_button_new (GIMP_STOCK_RESET, NULL);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_image_set_from_stock (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (gimp_paint_options_gui_reset_aspect_ratio),
                        options);

      gimp_help_set_help_data (button,
                               _("Reset aspect ratio to brush's native"), NULL);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      scale = gimp_prop_spin_scale_new (config, "brush-angle",
                                        _("Angle"),
                                        1.0, 5.0, 2);
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

      button = gimp_stock_button_new (GIMP_STOCK_RESET, NULL);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_image_set_from_stock (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (gimp_paint_options_gui_reset_angle),
                        options);

      gimp_help_set_help_data (button,
                               _("Reset angle to zero"), NULL);

      button = gimp_prop_dynamics_box_new (NULL, GIMP_CONTEXT (tool_options),
                                           _("Dynamics"), 2,
                                           "dynamics-view-type",
                                           "dynamics-view-size",
                                           "gimp-dynamics-editor");
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      frame = dynamics_options_gui (options, tool_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      frame = jitter_options_gui (options, tool_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }

  /*  the "smooth stroke" options  */
  if (g_type_is_a (tool_type, GIMP_TYPE_PAINT_TOOL))
    {
      GtkWidget *frame;

      frame = smoothing_options_gui (options, tool_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }

  /*  the "incremental" toggle  */
  if (tool_type == GIMP_TYPE_PENCIL_TOOL     ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_ERASER_TOOL)
    {
      GtkWidget *button;

      button = gimp_prop_enum_check_button_new (config,
                                                "application-mode",
                                                _("Incremental"),
                                                GIMP_PAINT_CONSTANT,
                                                GIMP_PAINT_INCREMENTAL);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  /* the "hard edge" toggle */
  if (tool_type == GIMP_TYPE_ERASER_TOOL            ||
      tool_type == GIMP_TYPE_CLONE_TOOL             ||
      tool_type == GIMP_TYPE_HEAL_TOOL              ||
      tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL          ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL        ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      GtkWidget *button;

      button = gimp_prop_check_button_new (config, "hard", _("Hard edge"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  return vbox;
}


/*  private functions  */

static GtkWidget *
dynamics_options_gui (GimpPaintOptions *paint_options,
                      GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame;
  GtkWidget *inner_frame;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *menu;
  GtkWidget *combo;
  GtkWidget *checkbox;
  GtkWidget *vbox;
  GtkWidget *inner_vbox;
  GtkWidget *hbox;
  GtkWidget *box;

  frame = gimp_prop_expander_new (config, "dynamics-expanded",
                                  _("Dynamics Options"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  inner_frame = gimp_frame_new (_("Fade Options"));
  gtk_box_pack_start (GTK_BOX (vbox), inner_frame, FALSE, FALSE, 0);
  gtk_widget_show (inner_frame);

  inner_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (inner_frame), inner_vbox);
  gtk_widget_show (inner_vbox);

  /*  the fade-out scale & unitmenu  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (inner_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  scale = gimp_prop_spin_scale_new (config, "fade-length",
                                    _("Fade length"), 1.0, 50.0, 0);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 1000.0);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  menu = gimp_prop_unit_combo_box_new (config, "fade-unit");
  gtk_box_pack_start (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

#if 0
  /* FIXME pixel digits */
  g_object_set_data (G_OBJECT (menu), "set_digits", spinbutton);
  gimp_unit_menu_set_pixel_digits (GIMP_UNIT_MENU (menu), 0);
#endif

  /*  the repeat type  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (inner_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Repeat:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_prop_enum_combo_box_new (config, "fade-repeat", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  checkbox = gimp_prop_check_button_new (config, "fade-reverse",
                                         _("Reverse"));
  gtk_box_pack_start (GTK_BOX (inner_vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show (checkbox);

  /* Color UI */
  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL))
    {
      inner_frame = gimp_frame_new (_("Color Options"));
      gtk_box_pack_start (GTK_BOX (vbox), inner_frame, FALSE, FALSE, 0);
      gtk_widget_show (inner_frame);

      box = gimp_prop_gradient_box_new (NULL, GIMP_CONTEXT (config),
                                        _("Gradient"), 2,
                                        "gradient-view-type",
                                        "gradient-view-size",
                                        "gradient-reverse",
                                        "gimp-gradient-editor");
      gtk_container_add (GTK_CONTAINER (inner_frame), box);
      gtk_widget_show (box);
    }

  return frame;
}

static GtkWidget *
jitter_options_gui (GimpPaintOptions *paint_options,
                    GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame;
  GtkWidget *scale;

  scale = gimp_prop_spin_scale_new (config, "jitter-amount",
                                    _("Amount"),
                                    0.01, 1.0, 2);

  frame = gimp_prop_expanding_frame_new (config, "use-jitter",
                                         _("Apply Jitter"),
                                         scale, NULL);

  return frame;
}

static GtkWidget *
smoothing_options_gui (GimpPaintOptions *paint_options,
                       GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *scale;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  frame = gimp_prop_expanding_frame_new (config, "use-smoothing",
                                         _("Smooth stroke"),
                                         vbox, NULL);

  scale = gimp_prop_spin_scale_new (config, "smoothing-quality",
                                    _("Quality"),
                                    1, 10, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, "smoothing-factor",
                                    _("Weight"),
                                    1, 10, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  return frame;
}

static void
gimp_paint_options_gui_reset_size (GtkWidget        *button,
                                   GimpPaintOptions *paint_options)
{
 GimpBrush *brush = gimp_context_get_brush (GIMP_CONTEXT (paint_options));

 if (brush)
   gimp_paint_options_set_default_brush_size (paint_options, brush);
}

static void
gimp_paint_options_gui_reset_aspect_ratio (GtkWidget        *button,
                                           GimpPaintOptions *paint_options)
{
   g_object_set (paint_options,
                 "brush-aspect-ratio", 0.0,
                 NULL);
}

static void
gimp_paint_options_gui_reset_angle (GtkWidget        *button,
                                    GimpPaintOptions *paint_options)
{
   g_object_set (paint_options,
                 "brush-angle", 0.0,
                 NULL);
}
