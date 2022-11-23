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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmadisplayconfig.h"

#include "core/ligma.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmawidgets-utils.h"

#include "ligmamagnifyoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_AUTO_RESIZE,
  PROP_ZOOM_TYPE
};


static void   ligma_magnify_options_config_iface_init (LigmaConfigInterface *config_iface);

static void   ligma_magnify_options_set_property (GObject         *object,
                                                 guint            property_id,
                                                 const GValue    *value,
                                                 GParamSpec      *pspec);
static void   ligma_magnify_options_get_property (GObject         *object,
                                                 guint            property_id,
                                                 GValue          *value,
                                                 GParamSpec      *pspec);

static void   ligma_magnify_options_reset        (LigmaConfig      *config);


G_DEFINE_TYPE_WITH_CODE (LigmaMagnifyOptions, ligma_magnify_options,
                         LIGMA_TYPE_TOOL_OPTIONS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_magnify_options_config_iface_init))

#define parent_class ligma_magnify_options_parent_class

static LigmaConfigInterface *parent_config_iface = NULL;


static void
ligma_magnify_options_class_init (LigmaMagnifyOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_magnify_options_set_property;
  object_class->get_property = ligma_magnify_options_get_property;

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_AUTO_RESIZE,
                            "auto-resize",
                            _("Auto-resize window"),
                            _("Resize image window to accommodate "
                              "new zoom level"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_ZOOM_TYPE,
                         "zoom-type",
                         _("Direction"),
                         _("Direction of magnification"),
                         LIGMA_TYPE_ZOOM_TYPE,
                         LIGMA_ZOOM_IN,
                         LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_magnify_options_config_iface_init (LigmaConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = ligma_magnify_options_reset;
}

static void
ligma_magnify_options_init (LigmaMagnifyOptions *options)
{
}

static void
ligma_magnify_options_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaMagnifyOptions *options = LIGMA_MAGNIFY_OPTIONS (object);

  switch (property_id)
    {
    case PROP_AUTO_RESIZE:
      options->auto_resize = g_value_get_boolean (value);
      break;
    case PROP_ZOOM_TYPE:
      options->zoom_type = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_magnify_options_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaMagnifyOptions *options = LIGMA_MAGNIFY_OPTIONS (object);

  switch (property_id)
    {
    case PROP_AUTO_RESIZE:
      g_value_set_boolean (value, options->auto_resize);
      break;
    case PROP_ZOOM_TYPE:
      g_value_set_enum (value, options->zoom_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_magnify_options_reset (LigmaConfig *config)
{
  LigmaToolOptions *tool_options = LIGMA_TOOL_OPTIONS (config);
  GParamSpec      *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        "auto-resize");

  if (pspec)
    G_PARAM_SPEC_BOOLEAN (pspec)->default_value =
      LIGMA_DISPLAY_CONFIG (tool_options->tool_info->ligma->config)->resize_windows_on_zoom;

  parent_config_iface->reset (config);
}

GtkWidget *
ligma_magnify_options_gui (LigmaToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = ligma_tool_options_gui (tool_options);
  GtkWidget       *frame;
  GtkWidget       *button;
  gchar           *str;
  GdkModifierType  toggle_mask;

  toggle_mask = ligma_get_toggle_behavior_mask ();

  /*  the auto_resize toggle button  */
  button = ligma_prop_check_button_new (config, "auto-resize", NULL);
  gtk_box_pack_start (GTK_BOX (vbox),  button, FALSE, FALSE, 0);

  /*  tool toggle  */
  str = g_strdup_printf (_("Direction  (%s)"),
                         ligma_get_mod_string (toggle_mask));
  frame = ligma_prop_enum_radio_frame_new (config, "zoom-type",
                                          str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  g_free (str);

  return vbox;
}
