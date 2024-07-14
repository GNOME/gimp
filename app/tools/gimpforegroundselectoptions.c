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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-constructors.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpforegroundselectoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


/*
 * for matting-global: iterations int
 * for matting-levin:  levels int, active levels int
 */

enum
{
  PROP_0,
  PROP_DRAW_MODE,
  PROP_PREVIEW_MODE,
  PROP_STROKE_WIDTH,
  PROP_MASK_COLOR,
  PROP_ENGINE,
  PROP_ITERATIONS,
  PROP_LEVELS,
  PROP_ACTIVE_LEVELS
};


static void   gimp_foreground_select_options_finalize     (GObject      *object);
static void   gimp_foreground_select_options_set_property (GObject      *object,
                                                           guint         property_id,
                                                           const GValue *value,
                                                           GParamSpec   *pspec);
static void   gimp_foreground_select_options_get_property (GObject      *object,
                                                           guint         property_id,
                                                           GValue       *value,
                                                           GParamSpec   *pspec);


G_DEFINE_TYPE (GimpForegroundSelectOptions, gimp_foreground_select_options,
               GIMP_TYPE_SELECTION_OPTIONS)


#define parent_class gimp_foreground_select_options_parent_class

static void
gimp_foreground_select_options_class_init (GimpForegroundSelectOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglColor    *blue         = gegl_color_new ("blue");

  object_class->finalize     = gimp_foreground_select_options_finalize;
  object_class->set_property = gimp_foreground_select_options_set_property;
  object_class->get_property = gimp_foreground_select_options_get_property;

  /*  override the antialias default value from GimpSelectionOptions  */

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_DRAW_MODE,
                         "draw-mode",
                         _("Draw Mode"),
                         _("Paint over areas to mark color values for "
                           "inclusion or exclusion from selection"),
                         GIMP_TYPE_MATTING_DRAW_MODE,
                         GIMP_MATTING_DRAW_MODE_FOREGROUND,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_PREVIEW_MODE,
                         "preview-mode",
                         _("Preview Mode"),
                         _("Preview Mode"),
                         GIMP_TYPE_MATTING_PREVIEW_MODE,
                         GIMP_MATTING_PREVIEW_MODE_ON_COLOR,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT  (object_class, PROP_STROKE_WIDTH,
                         "stroke-width",
                         _("Stroke width"),
                         _("Size of the brush used for refinements"),
                         1, 6000, 10,
                         GIMP_PARAM_STATIC_STRINGS);

  gimp_color_set_alpha (blue, 0.5);
  GIMP_CONFIG_PROP_COLOR  (object_class, PROP_MASK_COLOR,
                           "mask-color",
                           _("Preview color"),
                           _("Color of selection preview mask"),
                           TRUE, blue,
                           GIMP_PARAM_STATIC_STRINGS);
  g_object_unref (blue);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_ENGINE,
                         "engine",
                         _("Engine"),
                         _("Matting engine to use"),
                         GIMP_TYPE_MATTING_ENGINE,
                         GIMP_MATTING_ENGINE_LEVIN,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT  (object_class, PROP_LEVELS,
                         "levels",
                         C_("measurement", "Levels"),
                         _("Number of downsampled levels to use"),
                         1, 10, 2,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT  (object_class, PROP_ACTIVE_LEVELS,
                         "active-levels",
                         _("Active levels"),
                         _("Number of levels to perform solving"),
                         1, 10, 2,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT  (object_class, PROP_ITERATIONS,
                         "iterations",
                         _("Iterations"),
                         _("Number of iterations to perform"),
                         1, 10, 2,
                         GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_foreground_select_options_init (GimpForegroundSelectOptions *options)
{
  options->mask_color = gegl_color_new ("blue");
}

static void
gimp_foreground_select_options_finalize (GObject *object)
{
  g_clear_object (&(GIMP_FOREGROUND_SELECT_OPTIONS (object)->mask_color));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_foreground_select_options_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  GimpForegroundSelectOptions *options = GIMP_FOREGROUND_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_DRAW_MODE:
      options->draw_mode = g_value_get_enum (value);
      break;

    case PROP_PREVIEW_MODE:
      options->preview_mode = g_value_get_enum (value);
      break;

    case PROP_STROKE_WIDTH:
      options->stroke_width = g_value_get_int (value);
      break;

    case PROP_MASK_COLOR:
      g_clear_object (&options->mask_color);
      options->mask_color = gegl_color_duplicate (g_value_get_object (value));
      break;

    case PROP_ENGINE:
      options->engine = g_value_get_enum (value);
      if ((options->engine == GIMP_MATTING_ENGINE_LEVIN) &&
          !(gegl_has_operation ("gegl:matting-levin")))
        {
          options->engine = GIMP_MATTING_ENGINE_GLOBAL;
        }
      break;

    case PROP_LEVELS:
      options->levels = g_value_get_int (value);
      break;

    case PROP_ACTIVE_LEVELS:
      options->active_levels = g_value_get_int (value);
      break;

    case PROP_ITERATIONS:
      options->iterations = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_foreground_select_options_get_property (GObject    *object,
                                             guint       property_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  GimpForegroundSelectOptions *options = GIMP_FOREGROUND_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_DRAW_MODE:
      g_value_set_enum (value, options->draw_mode);
      break;

    case PROP_PREVIEW_MODE:
      g_value_set_enum (value, options->preview_mode);
      break;

    case PROP_STROKE_WIDTH:
      g_value_set_int (value, options->stroke_width);
      break;

    case PROP_MASK_COLOR:
      g_value_set_object (value, options->mask_color);
      break;

    case PROP_ENGINE:
      g_value_set_enum (value, options->engine);
      break;

     case PROP_LEVELS:
      g_value_set_int (value, options->levels);
      break;

    case PROP_ACTIVE_LEVELS:
      g_value_set_int (value, options->active_levels);
      break;

    case PROP_ITERATIONS:
      g_value_set_int (value, options->iterations);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_foreground_select_options_reset_stroke_width (GtkWidget       *button,
                                                   GimpToolOptions *tool_options)
{
  g_object_set (tool_options, "stroke-width", 10, NULL);
}

static gboolean
gimp_foreground_select_options_sync_engine (GBinding     *binding,
                                            const GValue *source_value,
                                            GValue       *target_value,
                                            gpointer      user_data)
{
  gint type = g_value_get_enum (source_value);

  g_value_set_boolean (target_value,
                       type == GPOINTER_TO_INT (user_data));

  return TRUE;
}

GtkWidget *
gimp_foreground_select_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_selection_options_gui (tool_options);
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *scale;
  GtkWidget *combo;
  GtkWidget *inner_vbox;
  GtkWidget *antialias_toggle;

  antialias_toggle = GIMP_SELECTION_OPTIONS (tool_options)->antialias_toggle;
  gtk_widget_hide (antialias_toggle);

  frame = gimp_prop_enum_radio_frame_new (config, "draw-mode", NULL,
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* stroke width */
  scale = gimp_prop_spin_scale_new (config, "stroke-width",
                                    1.0, 10.0, 2);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 1.0, 1000.0);
  gimp_spin_scale_set_gamma (GIMP_SPIN_SCALE (scale), 1.7);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);

  button = gimp_icon_button_new (GIMP_ICON_RESET, NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_image_set_from_icon_name (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                GIMP_ICON_RESET, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_foreground_select_options_reset_stroke_width),
                    tool_options);

  gimp_help_set_help_data (button,
                           _("Reset stroke width native size"), NULL);

  /* preview mode */

  frame = gimp_prop_enum_radio_frame_new (config, "preview-mode", NULL,
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  /*  mask color */
  button = gimp_prop_color_button_new (config, "mask-color",
                                       NULL,
                                       128, 24,
                                       GIMP_COLOR_AREA_SMALL_CHECKS);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (button),
                                GIMP_CONTEXT (config));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /* engine */
  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  combo = gimp_prop_enum_combo_box_new (config, "engine", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Engine"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), combo);

  if (! gegl_has_operation ("gegl:matting-levin"))
    gtk_widget_set_sensitive (combo, FALSE);

  inner_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), inner_vbox);
  gtk_widget_show (inner_vbox);

  /*  engine parameters  */
  scale = gimp_prop_spin_scale_new (config, "levels",
                                    1.0, 1.0, 0);
  gtk_box_pack_start (GTK_BOX (inner_vbox), scale, FALSE, FALSE, 0);

  g_object_bind_property_full (config, "engine",
                               scale,  "visible",
                               G_BINDING_SYNC_CREATE,
                               gimp_foreground_select_options_sync_engine,
                               NULL,
                               GINT_TO_POINTER (GIMP_MATTING_ENGINE_LEVIN),
                               NULL);

  scale = gimp_prop_spin_scale_new (config, "active-levels",
                                    1.0, 1.0, 0);
  gtk_box_pack_start (GTK_BOX (inner_vbox), scale, FALSE, FALSE, 0);

  g_object_bind_property_full (config, "engine",
                               scale,  "visible",
                               G_BINDING_SYNC_CREATE,
                               gimp_foreground_select_options_sync_engine,
                               NULL,
                               GINT_TO_POINTER (GIMP_MATTING_ENGINE_LEVIN),
                               NULL);

  scale = gimp_prop_spin_scale_new (config, "iterations",
                                    1.0, 1.0, 0);
  gtk_box_pack_start (GTK_BOX (inner_vbox), scale, FALSE, FALSE, 0);

  g_object_bind_property_full (config, "engine",
                               scale,  "visible",
                               G_BINDING_SYNC_CREATE,
                               gimp_foreground_select_options_sync_engine,
                               NULL,
                               GINT_TO_POINTER (GIMP_MATTING_ENGINE_GLOBAL),
                               NULL);

  return vbox;
}
