/* GIMP - The GNU Image Manipulation Program
 *
 * gimpseamlesscloneoptions.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"

#include "gimpseamlesscloneoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_MAX_REFINE_SCALE,
};


static void   gimp_seamless_clone_options_set_property (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);
static void   gimp_seamless_clone_options_get_property (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);


G_DEFINE_TYPE (GimpSeamlessCloneOptions, gimp_seamless_clone_options,
               GIMP_TYPE_TOOL_OPTIONS)

#define parent_class gimp_seamless_clone_options_parent_class


static void
gimp_seamless_clone_options_class_init (GimpSeamlessCloneOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_seamless_clone_options_set_property;
  object_class->get_property = gimp_seamless_clone_options_get_property;

  GIMP_CONFIG_PROP_INT  (object_class, PROP_MAX_REFINE_SCALE,
                         "max-refine-scale",
                         _("Refinement scale"),
                         _("Maximal scale of refinement points to be "
                           "used for the interpolation mesh"),
                         0, 50, 5,
                         GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_seamless_clone_options_init (GimpSeamlessCloneOptions *options)
{
}

static void
gimp_seamless_clone_options_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GimpSeamlessCloneOptions *options = GIMP_SEAMLESS_CLONE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MAX_REFINE_SCALE:
      options->max_refine_scale = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_seamless_clone_options_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GimpSeamlessCloneOptions *options = GIMP_SEAMLESS_CLONE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MAX_REFINE_SCALE:
      g_value_set_int (value, options->max_refine_scale);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_seamless_clone_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget *scale;

  scale = gimp_prop_spin_scale_new (config, "max-refine-scale", NULL,
                                    1.0, 10.0, 0);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale), 0.0, 50.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  return vbox;
}
