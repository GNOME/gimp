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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpconfig-params.h"
#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimprotatetool.h"
#include "gimpscaletool.h"
#include "gimptooloptions-gui.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TYPE,
  PROP_DIRECTION,
  PROP_INTERPOLATION,
  PROP_SUPERSAMPLE,
  PROP_RECURSION_LEVEL,
  PROP_CLIP,
  PROP_SHOW_PREVIEW,
  PROP_GRID_TYPE,
  PROP_GRID_SIZE,
  PROP_CONSTRAIN_1,
  PROP_CONSTRAIN_2
};


static void   gimp_transform_options_init       (GimpTransformOptions      *options);
static void   gimp_transform_options_class_init (GimpTransformOptionsClass *options_class);

static void   gimp_transform_options_set_property (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void   gimp_transform_options_get_property (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);

static void   gimp_transform_options_reset        (GimpToolOptions *tool_options);static void   gimp_transform_options_set_defaults (GimpToolOptions *tool_options);

static void   gimp_transform_options_grid_notify  (GimpTransformOptions *options,
                                                   GParamSpec           *pspec,
                                                   GtkWidget            *density_box);


static GimpToolOptionsClass *parent_class = NULL;


GType
gimp_transform_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpTransformOptionsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_transform_options_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpTransformOptions),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_transform_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpTransformOptions",
                                     &info, 0);
    }

  return type;
}

static void
gimp_transform_options_class_init (GimpTransformOptionsClass *klass)
{
  GObjectClass         *object_class;
  GimpToolOptionsClass *options_class;

  object_class  = G_OBJECT_CLASS (klass);
  options_class = GIMP_TOOL_OPTIONS_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_transform_options_set_property;
  object_class->get_property = gimp_transform_options_get_property;

  options_class->reset       = gimp_transform_options_reset;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_TYPE,
                                 "type", NULL,
                                 GIMP_TYPE_TRANSFORM_TYPE,
                                 GIMP_TRANSFORM_TYPE_LAYER,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_DIRECTION,
                                 "direction", NULL,
                                 GIMP_TYPE_TRANSFORM_DIRECTION,
                                 GIMP_TRANSFORM_FORWARD,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_INTERPOLATION,
                                 "interpolation", NULL,
                                 GIMP_TYPE_INTERPOLATION_TYPE,
                                 GIMP_INTERPOLATION_LINEAR,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SUPERSAMPLE,
                                    "supersample", NULL,
                                    FALSE,
                                    0);

#if 0
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_RECURSION_LEVEL,
                                "recursion-level", NULL,
                                1, 5, 3,
                                0);
#endif

  g_object_class_install_property (object_class, PROP_RECURSION_LEVEL,
                                   g_param_spec_int ("recursion-level",
                                                     NULL, NULL,
                                                     1, 5, 3,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_CLIP,
                                    "clip", NULL,
                                    FALSE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SHOW_PREVIEW,
                                    "show-preview", NULL,
                                    FALSE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_GRID_TYPE,
                                 "grid-type", NULL,
                                 GIMP_TYPE_TRANSFORM_GRID_TYPE,
                                 GIMP_TRANSFORM_GRID_TYPE_N_LINES,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_GRID_SIZE,
                                "grid-size", NULL,
                                1, 128, 15,
                                0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_CONSTRAIN_1,
                                    "constrain-1", NULL,
                                    FALSE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_CONSTRAIN_2,
                                    "constrain-2", NULL,
                                    FALSE,
                                    0);
}

static void
gimp_transform_options_init (GimpTransformOptions *options)
{
}

static void
gimp_transform_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpTransformOptions *options;

  options = GIMP_TRANSFORM_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TYPE:
      options->type = g_value_get_enum (value);
      break;
    case PROP_DIRECTION:
      options->direction = g_value_get_enum (value);
      break;
    case PROP_INTERPOLATION:
      options->interpolation = g_value_get_enum (value);
      break;
    case PROP_SUPERSAMPLE:
      options->supersample = g_value_get_boolean (value);
      break;
    case PROP_RECURSION_LEVEL:
      options->recursion_level = g_value_get_int (value);
      break;
    case PROP_CLIP:
      options->clip = g_value_get_boolean (value);
      break;
    case PROP_SHOW_PREVIEW:
      options->show_preview = g_value_get_boolean (value);
      break;
    case PROP_GRID_TYPE:
      options->grid_type = g_value_get_enum (value);
      break;
    case PROP_GRID_SIZE:
      options->grid_size = g_value_get_int (value);
      break;
    case PROP_CONSTRAIN_1:
      options->constrain_1 = g_value_get_boolean (value);
      break;
    case PROP_CONSTRAIN_2:
      options->constrain_2 = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_transform_options_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpTransformOptions *options;

  options = GIMP_TRANSFORM_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, options->type);
      break;
    case PROP_DIRECTION:
      g_value_set_enum (value, options->direction);
      break;
    case PROP_INTERPOLATION:
      g_value_set_enum (value, options->interpolation);
      break;
    case PROP_SUPERSAMPLE:
      g_value_set_boolean (value, options->supersample);
      break;
    case PROP_RECURSION_LEVEL:
      g_value_set_int (value, options->recursion_level);
      break;
    case PROP_CLIP:
      g_value_set_boolean (value, options->clip);
      break;
    case PROP_SHOW_PREVIEW:
      g_value_set_boolean (value, options->show_preview);
      break;
    case PROP_GRID_TYPE:
      g_value_set_enum (value, options->grid_type);
      break;
    case PROP_GRID_SIZE:
      g_value_set_int (value, options->grid_size);
      break;
    case PROP_CONSTRAIN_1:
      g_value_set_boolean (value, options->constrain_1);
      break;
    case PROP_CONSTRAIN_2:
      g_value_set_boolean (value, options->constrain_2);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_transform_options_reset (GimpToolOptions *tool_options)
{
  gimp_transform_options_set_defaults (tool_options);

  GIMP_TOOL_OPTIONS_CLASS (parent_class)->reset (tool_options);
}

static void
gimp_transform_options_set_defaults (GimpToolOptions *tool_options)
{
  GParamSpec *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tool_options),
                                        "interpolation");

  if (pspec)
    G_PARAM_SPEC_ENUM (pspec)->default_value =
      tool_options->tool_info->gimp->config->interpolation_type;
}

static void
gimp_scale_constraints_callback (GtkWidget *widget,
                                 GObject   *config)
{
  gint      value;
  gboolean  c0;
  gboolean  c1;

  value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                              "gimp-item-data"));
  
  c0 = c1 = FALSE;

  if (value == 1 || value == 3)
    c0 = TRUE;
  if (value == 2 || value == 3)
    c1 = TRUE;
  
  g_object_set (config,
                "constrain-1", c0,
                "constrain-2", c1,
                NULL);
}

GtkWidget *
gimp_transform_options_gui (GimpToolOptions *tool_options)
{
  GObject              *config;
  GimpTransformOptions *options;
  GtkWidget            *vbox;
  GtkWidget            *hbox;
  GtkWidget            *label;
  GtkWidget            *frame;
  GtkWidget            *table;
  GtkWidget            *combo;
  GtkWidget            *button;

  config  = G_OBJECT (tool_options);
  options = GIMP_TRANSFORM_OPTIONS (tool_options);

  vbox = gimp_tool_options_gui (tool_options);

  hbox = gimp_prop_enum_stock_box_new (config, "type", "gimp", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Affect:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (hbox), label, 0);
  gtk_widget_show (label);

  frame = gimp_prop_enum_radio_frame_new (config, "direction",
                                          _("Transform Direction"), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the interpolation menu  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Interpolation:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_prop_enum_combo_box_new (config, "interpolation", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  /*  the supersample toggle button  */
  button = gimp_prop_check_button_new (config, "supersample",
                                       _("Supersampling"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the clip resulting image toggle button  */
  button = gimp_prop_check_button_new (config, "clip", _("Clip result"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* the preview toggle button */
  button = gimp_prop_check_button_new (config, "show-preview", _("Preview"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  
  /*  the grid frame  */
  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the grid type menu  */
  combo = gimp_prop_enum_combo_box_new (config, "grid-type", 0, 0);
  gtk_frame_set_label_widget (GTK_FRAME (frame), combo);
  gtk_widget_show (combo);

  /*  the grid density scale  */
  table = gtk_table_new (1, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  g_signal_connect (config, "notify::grid-type",
                    G_CALLBACK (gimp_transform_options_grid_notify),
                    table);

  gimp_prop_scale_entry_new (config, "grid-size",
                             GTK_TABLE (table), 0, 0,
                             _("Density:"),
                             1.0, 8.0, 0,
                             FALSE, 0.0, 0.0);

  if (tool_options->tool_info->tool_type == GIMP_TYPE_ROTATE_TOOL ||
      tool_options->tool_info->tool_type == GIMP_TYPE_SCALE_TOOL)
    {
      GtkWidget *vbox2;
      GtkWidget *vbox3;
      gchar     *str;
      gchar     *str1;
      gchar     *str2;
      gchar     *str3;

      /*  the constraints frame  */
      frame = gimp_frame_new (_("Constraints"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      vbox2 = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);
      gtk_widget_show (vbox2);

      if (tool_options->tool_info->tool_type == GIMP_TYPE_ROTATE_TOOL)
        {
          str = g_strdup_printf (_("15 degrees  %s"),
                                 gimp_get_mod_name_control ());

          button = gimp_prop_check_button_new (config, "constrain-1", str);
          gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
          gtk_widget_show (button);

          g_free (str);
        }
      else if (tool_options->tool_info->tool_type == GIMP_TYPE_SCALE_TOOL)
        {
          g_object_set (config, 
                        "constrain-1", FALSE,
                        "constrain-2", FALSE,
                        NULL);

          str1 = g_strdup_printf (_("Keep height  %s"),
                                 gimp_get_mod_name_control ());
          str2 = g_strdup_printf (_("Keep width  %s"),
                                 gimp_get_mod_name_alt ());
          str3 = g_strdup_printf (_("Keep aspect  %s-%s"),
                                 gimp_get_mod_name_control (), gimp_get_mod_name_alt ());

          vbox3 = gimp_int_radio_group_new (FALSE, NULL,
                                            G_CALLBACK (gimp_scale_constraints_callback),
                                            config, 0,

                                            _("None"), 0, NULL,
                                            str1,      1, NULL,
                                            str2,      2, NULL,
                                            str3,      3, NULL,

                                            NULL);
                                            
          gtk_box_pack_start (GTK_BOX (vbox2), vbox3, FALSE, FALSE, 0);
          gtk_widget_show (vbox3);

          g_free (str1);
          g_free (str2);
          g_free (str3);
        }
    }

  gimp_transform_options_set_defaults (tool_options);

  return vbox;
}


/*  private functions  */

static void
gimp_transform_options_grid_notify (GimpTransformOptions *options,
                                    GParamSpec           *pspec,
                                    GtkWidget            *density_box)
{
  gtk_widget_set_sensitive (density_box,
                            options->grid_type !=
                            GIMP_TRANSFORM_GRID_TYPE_NONE);
}
