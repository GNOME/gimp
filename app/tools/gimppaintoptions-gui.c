/* The GIMP -- an image manipulation program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpview.h"
#include "widgets/gimpviewrenderergradient.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gtkhwrapbox.h"
#include "widgets/gimpviewablebutton.h"

#include "gimpairbrushtool.h"
#include "gimpblendtool.h"
#include "gimpbucketfilltool.h"
#include "gimpclonetool.h"
#include "gimpconvolvetool.h"
#include "gimpdodgeburntool.h"
#include "gimpdodgeburntool.h"
#include "gimperasertool.h"
#include "gimpinktool.h"
#include "gimppaintbrushtool.h"
#include "gimppaintoptions-gui.h"
#include "gimppenciltool.h"
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

static void gradient_options_reverse_notify (GimpPaintOptions *paint_options,
                                             GParamSpec       *pspec,
                                             GimpView         *view);


GtkWidget *
gimp_paint_options_gui (GimpToolOptions *tool_options)
{
  GimpPaintOptions  *options;
  GimpContext       *context;
  GObject           *config;
  GtkWidget         *vbox;
  GtkWidget         *frame;
  GtkWidget         *table;
  GtkWidget         *optionmenu;
  GtkWidget         *mode_label;
  GtkWidget         *button;
  GimpDialogFactory *dialog_factory;
  GtkWidget         *incremental_toggle = NULL;
  gint               table_row          = 0;
  GType              tool_type;

  options = GIMP_PAINT_OPTIONS (tool_options);
  context = GIMP_CONTEXT (tool_options);
  config  = G_OBJECT (tool_options);

  vbox = gimp_tool_options_gui (tool_options);

  dialog_factory = gimp_dialog_factory_from_name ("dock");

  tool_type = tool_options->tool_info->tool_type;

  /*  the main table  */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  g_object_set_data (G_OBJECT (vbox), GIMP_PAINT_OPTIONS_TABLE_KEY, table);

  /*  the opacity scale  */
  gimp_prop_opacity_entry_new (config, "opacity",
                               GTK_TABLE (table), 0, table_row++,
                               _("Opacity:"));

  /*  the paint mode menu  */
  optionmenu = gimp_prop_paint_mode_menu_new (config, "paint-mode", TRUE);
  mode_label = gimp_table_attach_aligned (GTK_TABLE (table), 0, table_row++,
                                          _("Mode:"), 0.0, 0.5,
                                          optionmenu, 2, TRUE);

  if (tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      gtk_widget_set_sensitive (optionmenu, FALSE);
      gtk_widget_set_sensitive (mode_label, FALSE);
    }

  /*  the brush view  */
  if (tool_type != GIMP_TYPE_BUCKET_FILL_TOOL &&
      tool_type != GIMP_TYPE_BLEND_TOOL       &&
      tool_type != GIMP_TYPE_INK_TOOL)
    {
      button = gimp_viewable_button_new (context->gimp->brush_factory->container,
                                         context,
                                         GIMP_VIEW_SIZE_SMALL, 1,
                                         dialog_factory,
                                         "gimp-brush-grid|gimp-brush-list",
                                         GIMP_STOCK_BRUSH,
                                         _("Open the brush selection dialog"));

      gimp_table_attach_aligned (GTK_TABLE (table), 0, table_row++,
                                 _("Brush:"), 0.0, 0.5,
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

  /*  the "incremental" toggle  */
  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL) ||
      tool_options->tool_info->tool_type == GIMP_TYPE_ERASER_TOOL)
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
  if (tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_CLONE_TOOL      ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      button = gimp_prop_check_button_new (config, "hard", _("Hard edge"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  /*  the pattern view table  */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  the pattern view  */
  if (tool_type == GIMP_TYPE_BUCKET_FILL_TOOL ||
      tool_type == GIMP_TYPE_CLONE_TOOL)
    {
      button = gimp_viewable_button_new (context->gimp->pattern_factory->container,
                                         context,
                                         GIMP_VIEW_SIZE_SMALL, 1,
                                         dialog_factory,
                                         "gimp-pattern-grid|gimp-pattern-list",
                                         GIMP_STOCK_PATTERN,
                                         _("Open the pattern selection dialog"));

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("Pattern:"), 0.0, 0.5,
                                 button, 2, TRUE);
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
  GObject   *config;
  GtkWidget *frame = NULL;
  GtkWidget *wbox  = NULL;
  GtkWidget *button;

  config = G_OBJECT (paint_options);

  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL) ||
      tool_type == GIMP_TYPE_CLONE_TOOL                  ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL               ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL             ||
      tool_type == GIMP_TYPE_ERASER_TOOL                 ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      GtkWidget *inner_frame;

      frame = gtk_expander_new (_("Pressure sensitivity"));
      gtk_expander_set_expanded (GTK_EXPANDER (frame), FALSE);

      inner_frame = gimp_frame_new ("<expander>");
      gtk_container_add (GTK_CONTAINER (frame), inner_frame);
      gtk_widget_show (inner_frame);

      wbox = gtk_hwrap_box_new (FALSE);
      gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (wbox), 7);
      gtk_container_add (GTK_CONTAINER (inner_frame), wbox);
      gtk_widget_show (wbox);
    }

  /*  the opacity toggle  */
  if (tool_type == GIMP_TYPE_CLONE_TOOL      ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL ||
      tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      button = gimp_prop_check_button_new (config, "pressure-opacity",
                                           _("Opacity"));
      gtk_container_add (GTK_CONTAINER (wbox), button);
      gtk_widget_show (button);
    }

  /*  the pressure toggle  */
  if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL   ||
      tool_type == GIMP_TYPE_CLONE_TOOL      ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
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
  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL) ||
      tool_type == GIMP_TYPE_CLONE_TOOL                  ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL               ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL             ||
      tool_type == GIMP_TYPE_ERASER_TOOL)
    {
      button = gimp_prop_check_button_new (config, "pressure-size",
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
  GObject   *config;
  GtkWidget *frame = NULL;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkWidget *button;
  GtkWidget *unitmenu;

  config = G_OBJECT (paint_options);

  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL) ||
      tool_type == GIMP_TYPE_CLONE_TOOL                  ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL               ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL             ||
      tool_type == GIMP_TYPE_ERASER_TOOL                 ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      frame = gimp_frame_new (NULL);

      table = gtk_table_new (1, 3, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (table), 2);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_container_add (GTK_CONTAINER (frame), table);
      gtk_widget_show (table);

      button = gimp_prop_check_button_new (config, "use-fade",
                                           _("Fade out"));
      gtk_frame_set_label_widget (GTK_FRAME (frame), button);
      gtk_widget_show (button);

      gtk_widget_set_sensitive (table, fade->use_fade);
      g_object_set_data (G_OBJECT (button), "set_sensitive", table);

      /*  the fade-out sizeentry  */
      spinbutton = gimp_prop_spin_button_new (config, "fade-length",
                                              1.0, 50.0, 0);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("Length:"), 0.0, 0.5,
                                 spinbutton, 1, FALSE);

      /*  the fade-out unitmenu  */
      unitmenu = gimp_prop_unit_menu_new (config, "fade-unit", "%a");
      gtk_table_attach (GTK_TABLE (table), unitmenu, 2, 3, 0, 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (unitmenu);

      g_object_set_data (G_OBJECT (unitmenu), "set_digits", spinbutton);
    }

  return frame;
}

static GtkWidget *
gradient_options_gui (GimpGradientOptions *gradient,
                      GimpPaintOptions    *paint_options,
                      GType                tool_type,
                      GtkWidget           *incremental_toggle)
{
  GObject   *config;
  GtkWidget *frame = NULL;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkWidget *button;
  GtkWidget *unitmenu;
  GtkWidget *combo;
  GimpContext       *context;
  GimpDialogFactory *dialog_factory;
  GtkWidget *hbox;
  GtkWidget *gradient_button;
  GtkWidget *preview;

  config = G_OBJECT (paint_options);
  context = GIMP_CONTEXT (paint_options);
  dialog_factory = gimp_dialog_factory_from_name ("dock");

  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL))
   {
      frame = gimp_frame_new (NULL);

      table = gtk_table_new (3, 3, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (table), 2);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_table_set_row_spacings (GTK_TABLE (table), 2);
      gtk_container_add (GTK_CONTAINER (frame), table);
      gtk_widget_show (table);

      button = gimp_prop_check_button_new (config, "use-gradient",
                                           _("Use color from gradient"));
      gtk_frame_set_label_widget (GTK_FRAME (frame), button);
      gtk_widget_show (button);

      gtk_widget_set_sensitive (table, gradient->use_gradient);
      g_object_set_data (G_OBJECT (button), "set_sensitive", table);
      g_object_set_data (G_OBJECT (button), "inverse_sensitive",
                         incremental_toggle);

      /*  the gradient view  */
      gradient_button =
        gimp_viewable_button_new (context->gimp->gradient_factory->container,
                                  context,
                                  GIMP_VIEW_SIZE_LARGE, 1,
                                  dialog_factory,
                                  "gimp-gradient-list|gimp-gradient-grid",
                                  GIMP_STOCK_GRADIENT,
                                  _("Open the gradient selection dialog"));

      /*  use smaller previews for the popup  */
      GIMP_VIEWABLE_BUTTON (gradient_button)->preview_size =
        GIMP_VIEW_SIZE_SMALL;

      hbox = gtk_hbox_new (FALSE, 4);

      gtk_box_pack_start (GTK_BOX (hbox), gradient_button, FALSE, FALSE, 0);
      gtk_widget_show (gradient_button);

      button = gimp_prop_check_button_new (config, "gradient-reverse",
                                           _("Reverse"));
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("Gradient:"), 0.0, 0.5,
                                 hbox, 2, FALSE);

      preview = GTK_BIN (gradient_button)->child;

      g_signal_connect_object (config, "notify::gradient-reverse",
                               G_CALLBACK (gradient_options_reverse_notify),
                               G_OBJECT (preview), 0);

      gradient_options_reverse_notify (GIMP_PAINT_OPTIONS (config),
                                       NULL,
                                       GIMP_VIEW (preview));

      /*  the gradient length scale  */
      spinbutton = gimp_prop_spin_button_new (config, "gradient-length",
                                              1.0, 50.0, 0);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                 _("Length:"), 0.0, 0.5,
                                 spinbutton, 1, FALSE);

      /*  the gradient unitmenu  */
      unitmenu = gimp_prop_unit_menu_new (config, "gradient-unit", "%a");
      gtk_table_attach (GTK_TABLE (table), unitmenu, 2, 3, 1, 2,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (unitmenu);

      g_object_set_data (G_OBJECT (unitmenu), "set_digits", spinbutton);

      /*  the repeat type  */
      combo = gimp_prop_enum_combo_box_new (config, "gradient-repeat", 0, 0);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                                 _("Repeat:"), 0.0, 0.5,
                                 combo, 2, TRUE);

      gtk_widget_show (table);
    }

  return frame;
}

static void
gradient_options_reverse_notify (GimpPaintOptions *paint_options,
                                 GParamSpec       *pspec,
                                 GimpView         *view)
{
  GimpViewRendererGradient *rendergrad;

  rendergrad = GIMP_VIEW_RENDERER_GRADIENT (view->renderer);

  gimp_view_renderer_gradient_set_reverse (rendergrad,
                                           paint_options->gradient_options->gradient_reverse);
}
