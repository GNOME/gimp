/* GIMP - The GNU Image Manipulation Program
 *
 * gimpcageoptions.c
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcagetool.h"
#include "gimpcageoptions.h"

#include "gimp-intl.h"


G_DEFINE_TYPE (GimpCageOptions, gimp_cage_options,
               GIMP_TYPE_TRANSFORM_OPTIONS)

#define parent_class gimp_cage_options_parent_class


static void
gimp_cage_options_class_init (GimpCageOptionsClass *klass)
{
 
}

static void
gimp_cage_options_init (GimpCageOptions *options)
{
	
}
