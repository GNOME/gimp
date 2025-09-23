/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpgpparams.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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
#include <gegl-paramspecs.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpprotocol.h"

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpdisplay.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimpgradient.h"
#include "core/gimpgrouplayer.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplinklayer.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"
#include "core/gimpselection.h"
#include "core/gimpunit.h"

#include "path/gimppath.h"
#include "path/gimpvectorlayer.h"

#include "text/gimpfont.h"
#include "text/gimptextlayer.h"

#include "core/gimpparamspecs.h"

#include "pdb/gimppdb-utils.h"

#include "libgimp/gimpgpparams.h"


/*  include the implementation, they are shared between app/ and
 *  libgimp/ but need different headers.
 */
#include "../../libgimp/gimpgpparams-body.c"
