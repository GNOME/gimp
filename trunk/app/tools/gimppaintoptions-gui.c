/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"
#include "widgets/gtkhwrapbox.h"

#include "gimpairbrushtool.h"
#include "gimpblendtool.h"
#include "gimpbucketfilltool.h"
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


static GtkWidget * pressure_options_gui (GimpPressureOptions *pressure,
                                         GimpPaintOptions    *paint_options,
                                         GType                tool_type);
static GtkWidget * fade_options_gui     (GimpFadeOptions     *fade,
                                         GimpPaintOptions    *paint_options,
                                         GType                tool_type);
static GtkWidget * gradient_options_gui (GimpGradientOptions *gradient,
                                         GimpPaintOptions    *paint_options,
                                         GType                tool_type,
                                         GtkWidget           *incremental_toggle);
static GtkWidget * jitter_options_gui   (GimpJitterOptions   *jitter,
                                         GimpPaintOptions    *paint_options,
                                         GType                tool_type);

/*  public functions  */

GtkWidget *
gimp_paint_options_gui (GimpToolOptions *tool_options)
{
  GObject          *config  = G_OBJECT (tool_options);
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (tool_options);
  GtkWidget        *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget        *frame;
  GtkWidget        *table;
  GtkWidget        *menu;
  GtkWidget        *label;
  GtkWidget        *button;
  GtkWidget        *incremental_toggle = NULL;
  gint              table_row          = 0;
  GType             tool_type;

  tool_type = tool_options->tool_info->tool_type;

  /*  the main table  */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  g_object_set_data (G_OBJECT (vbox), GIMP_PAINT_OPTIONS_TABLE_KEY, table);

  /*  the paint mode menu  */
  menu  = gimp_prop_paint_mode_menu_new (config, "paint-mode", TRUE, FALSE);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, table_row++,
                                     _("Mode:"), 0.0, 0.5,
                                     menu, 2, FALSE);

  if (tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      gtk_widget_set_sensitive (menu, FALSE);
      gtk_widget_set_sensitive (label, FALSE);
    }

  /*  the opacity scale  */
  gimp_prop_opacity_entry_new (config, "opacity",
                               GTK_TABLE (table), 0, table_row++,
                               _("Opacity:"));

  /*  the brush  */
  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL))
    {
      button = gimp_prop_brush_box_new (NULL, GIMP_CONTEXT (tool_options), 2,
                                        "brush-view-type", "brush-view-size");
      gimp_table_attach_aligned (GTK_TABLE (table), 0, table_row++,
                                 _("Brush:"), 0.0, 0.5,
                                 button, 2, FALSE);

      if (tool_type != GIMP_TYPE_SMUDGE_TOOL)
        {
          GtkObject *adj;

          adj = gimp_prop_scale_entry_new (config, "brush-scale",
                                           GTK_TABLE (table), 0, table_row++,
                                           _("Scale:"),
                                           0.01, 0.1, 2,
                                           FALSE, 0.0, 0.0);

          gimp_scale_entry_set_logarithmic (adj, TRUE);
        }
    }

  /*  the gradient  */
  if (tool_type == GIMP_TYPE_BLEND_TOOL)
    {
      button = gimp_prop_gradient_box_new (NULL, GIMP_CONTEXT (tool_options), 2,
                                           "gradient-view-type",
                                           "gradient-view-size",
                                           "gradient-reverse");
      gimp_table_attach_aligned (GTK_TABLE (table), 0, table_row++,
                                 _("Gradient:"), 0.0, 0.5,
                                 button, 2, TRUE);
    }

  frame = pressure_options_gui (options->pressure_options,
                                options, tool_type);
  if (frame)
    {
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }

  frame = fade_options_gui (options->fade_options,
                            options, tool_type);
  if (frame)
    {
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }

  frame = jitter_options_gui (options->jitter_options,
                              options, tool_type);
  if (frame)
    {
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }

  /*  the "incremental" toggle  */
  if (tool_type == GIMP_TYPE_PENCIL_TOOL     ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_ERASER_TOOL)
    {
      incremental_toggle =
        gimp_prop_enum_check_button_new (config,
                                         "application-mode",
                                         _("Incremental"),
                                         GIMP_PAINT_CONSTANT,
                                         GIMP_PAINT_INCREMENTAL);
      gtk_box_pack_start (GTK_BOX (vbox), incremental_toggle, FALSE, FALSE, 0);
      gtk_widget_show (incremental_toggle);
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
      button = gimp_prop_check_button_new (config, "hard", _("Hard edge"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  frame = gradient_options_gui (options->gradient_options,
                                options, tool_type,
                                incremental_toggle);
  if (frame)
    {
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }

  return vbox;
}


/*  private functions  */

static GtkWidget *
pressure_options_gui (GimpPressureOptions *pressure,
                      GimpPaintOptions    *paint_options,
                      GType                tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame  = NULL;
  GtkWidget *wbox   = NULL;
  GtkWidget *button;

  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL))
    {
      GtkWidget *inner_frame;

      frame = gimp_prop_expander_new (G_OBJECT (paint_options),
                                      "pressure-expanded",
                                      _("Pressure sensitivity"));

      inner_frame = gimp_frame_new ("<expander>");
      gtk_container_add (GTK_CONTAINER (frame), inner_frame);
      gtk_widget_show (inner_frame);

      wbox = gtk_hwrap_box_new (FALSE);
      gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (wbox), 4);
      gtk_container_add (GTK_CONTAINER (inner_frame), wbox);
      gtk_widget_show (wbox);
    }

  /*  the opacity toggle  */
  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL) ||
      tool_type == GIMP_TYPE_CLONE_TOOL                  ||
      tool_type == GIMP_TYPE_HEAL_TOOL                   ||
      tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL      ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL             ||
      tool_type == GIMP_TYPE_ERASER_TOOL)
    {
      button = gimp_prop_check_button_new (config, "pressure-opacity",
                                           _("Opacity"));
      gtk_container_add (GTK_CONTAINER (wbox), button);
      gtk_widget_show (button);
    }

  /*  the pressure toggle  */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL          ||
      tool_type == GIMP_TYPE_CLONE_TOOL             ||
      tool_type == GIMP_TYPE_HEAL_TOOL              ||
      tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL          ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL        ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL        ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      button = gimp_prop_check_button_new (config, "pressure-hardness",
                                           _("Hardness"));
      gtk_container_add (GTK_CONTAINER (wbox), button);
      gtk_widget_show (button);
    }

  /*  the rate toggle */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      button = gimp_prop_check_button_new (config, "pressure-rate",
                                           _("Rate"));
      gtk_container_add (GTK_CONTAINER (wbox), button);
      gtk_widget_show (button);
    }

  /*  the size toggle  */
  if (tool_type == GIMP_TYPE_CLONE_TOOL             ||
      tool_type == GIMP_TYPE_HEAL_TOOL              ||
      tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL          ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL        ||
      tool_type == GIMP_TYPE_ERASER_TOOL            ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL        ||
      tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      button = gimp_prop_check_button_new (config, "pressure-size",
                                           _("Size"));
      gtk_container_add (GTK_CONTAINER (wbox), button);
      gtk_widget_show (button);
    }

  /* the inverse size toggle */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL)
    {
      button = gimp_prop_check_button_new (config, "pressure-inverse-size",
                                           _("Size"));
      gtk_container_add (GTK_CONTAINER (wbox), button);
      gtk_widget_show (button);
    }

  /*  the color toggle  */
  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL))
    {
      button = gimp_prop_check_button_new (config, "pressure-color",
                                           _("Color"));
      gtk_container_add (GTK_CONTAINER (wbox), button);
      gtk_widget_show (button);
    }

  return frame;
}

static GtkWidget *
fade_options_gui (GimpFadeOptions  *fade,
                  GimpPaintOptions *paint_options,
                  GType             tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame  = NULL;

  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL))
    {
      GtkWidget *table;
      GtkWidget *spinbutton;
      GtkWidget *menu;

      table = gtk_table_new (1, 3, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);

      frame = gimp_prop_expanding_frame_new (config, "use-fade",
                                             _("Fade out"),
                                             table, NULL);

      /*  the fade-out sizeentry  */
      spinbutton = gimp_prop_spin_button_new (config, "fade-length",
                                              1.0, 50.0, 0);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("Length:"), 0.0, 0.5,
                                 spinbutton, 1, FALSE);

      /*  the fade-out unitmenu  */
      menu = gimp_prop_unit_menu_new (config, "fade-unit", "%a");
      gtk_table_attach (GTK_TABLE (table), menu, 2, 3, 0, 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (menu);

      g_object_set_data (G_OBJECT (menu), "set_digits", spinbutton);
      gimp_unit_menu_set_pixel_digits (GIMP_UNIT_MENU (menu), 0);
    }

  return frame;
}

static GtkWidget *
jitter_options_gui (GimpJitterOptions  *jitter,
                    GimpPaintOptions   *paint_options,
                    GType               tool_type)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame  = NULL;

  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL))
    {
      GtkWidget *table;

      table = gtk_table_new (1, 3, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);

      frame = gimp_prop_expanding_frame_new (config, "use-jitter",
                                             _("Apply Jitter"),
                                             table, NULL);

      gimp_prop_scale_entry_new (config, "jitter-amount",
                                 GTK_TABLE (table), 0, 0,
                                 _("Amount:"),
                                 0.01, 0.1, 2,
                                 TRUE, 0.0, 5.0);
    }

  return frame;
}

static GtkWidget *
gradient_options_gui (GimpGradientOptions *gradient,
                      GimpPaintOptions    *paint_options,
                      GType                tool_type,
                      GtkWidget           *incremental_toggle)
{
  GObject   *config = G_OBJECT (paint_options);
  GtkWidget *frame  = NULL;

  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL))
    {
      GtkWidget *table;
      GtkWidget *spinbutton;
      GtkWidget *button;
      GtkWidget *menu;
      GtkWidget *combo;

      table = gtk_table_new (3, 3, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);

      frame = gimp_prop_expanding_frame_new (config, "use-gradient",
                                             _("Use color from gradient"),
                                             table, &button);

      if (incremental_toggle)
        {
          gtk_widget_set_sensitive (incremental_toggle,
                                    ! gradient->use_gradient);
          g_object_set_data (G_OBJECT (button), "inverse_sensitive",
                             incremental_toggle);
        }

      /*  the gradient view  */
      button = gimp_prop_gradient_box_new (NULL, GIMP_CONTEXT (config), 2,
                                           "gradient-view-type",
                                           "gradient-view-size",
                                           "gradient-reverse");
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("Gradient:"), 0.0, 0.5,
                                 button, 2, TRUE);

      /*  the gradient length scale  */
      spinbutton = gimp_prop_spin_button_new (config, "gradient-length",
                                              1.0, 50.0, 0);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                 _("Length:"), 0.0, 0.5,
                                 spinbutton, 1, FALSE);

      /*  the gradient unitmenu  */
      menu = gimp_prop_unit_menu_new (config, "gradient-unit", "%a");
      gtk_table_attach (GTK_TABLE (table), menu, 2, 3, 1, 2,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (menu);

      g_object_set_data (G_OBJECT (menu), "set_digits", spinbutton);
      gimp_unit_menu_set_pixel_digits (GIMP_UNIT_MENU (menu), 0);

      /*  the repeat type  */
      combo = gimp_prop_enum_combo_box_new (config, "gradient-repeat", 0, 0);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                                 _("Repeat:"), 0.0, 0.5,
                                 combo, 2, FALSE);
    }

  return frame;
}
