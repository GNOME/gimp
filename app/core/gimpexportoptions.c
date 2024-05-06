/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpexportoptions.h
 * Copyright (C) 2024 Michael Natterer <mitch@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpexportoptions.h"

#include "gimp-intl.h"


static void   gimp_export_options_finalize            (GObject             *object);


G_DEFINE_TYPE_WITH_CODE (GimpExportOptions, gimp_export_options, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_export_options_parent_class


static void
gimp_export_options_class_init (GimpExportOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_export_options_finalize;
}

static void
gimp_export_options_init (GimpExportOptions *options)
{
}

static void
gimp_export_options_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

GimpExportOptions *
gimp_export_options_new (void)
{
  GimpExportOptions *options;

  options = g_object_new (GIMP_TYPE_EXPORT_OPTIONS,
                          NULL);

  return options;
}
