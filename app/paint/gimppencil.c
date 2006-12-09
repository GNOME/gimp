/* GIMP - The GNU Image Manipulation Program
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

#include <glib-object.h>

#include "paint-types.h"

#include "gimppencil.h"
#include "gimppenciloptions.h"

#include "gimp-intl.h"


G_DEFINE_TYPE (GimpPencil, gimp_pencil, GIMP_TYPE_PAINTBRUSH)


void
gimp_pencil_register (Gimp                      *gimp,
                      GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_PENCIL,
                GIMP_TYPE_PENCIL_OPTIONS,
                "gimp-pencil",
                _("Pencil"),
                "gimp-tool-pencil");
}

static void
gimp_pencil_class_init (GimpPencilClass *klass)
{
}

static void
gimp_pencil_init (GimpPencil *pencil)
{
}
