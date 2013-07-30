/* GIMP - The GNU Image Manipulation Program
 *
 * gimpnpointdeformationoptions.h
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "gimpnpointdeformationoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


static void   gimp_n_point_deformation_options_set_property (GObject      *object,
                                                             guint         property_id,
                                                             const GValue *value,
                                                             GParamSpec   *pspec);
static void   gimp_n_point_deformation_options_get_property (GObject      *object,
                                                             guint         property_id,
                                                             GValue       *value,
                                                             GParamSpec   *pspec);


G_DEFINE_TYPE (GimpNPointDeformationOptions, gimp_n_point_deformation_options,
               GIMP_TYPE_TOOL_OPTIONS)

#define parent_class gimp_n_point_deformation_options_parent_class


static void
gimp_n_point_deformation_options_class_init (GimpNPointDeformationOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_n_point_deformation_options_set_property;
  object_class->get_property = gimp_n_point_deformation_options_get_property;

}

static void
gimp_n_point_deformation_options_init (GimpNPointDeformationOptions *options)
{
}

static void
gimp_n_point_deformation_options_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
//  GimpNPointDeformationOptions *options = GIMP_N_POINT_DEFORMATION_OPTIONS (object);

}

static void
gimp_n_point_deformation_options_get_property (GObject    *object,
                                               guint       property_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
//  GimpNPointDeformationOptions *options = GIMP_N_POINT_DEFORMATION_OPTIONS (object);
}

GtkWidget *
gimp_n_point_deformation_options_gui (GimpToolOptions *tool_options)
{
//  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_tool_options_gui (tool_options);

  return vbox;
}

