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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "paint-types.h"

#include "ligmapencil.h"
#include "ligmapenciloptions.h"

#include "ligma-intl.h"


G_DEFINE_TYPE (LigmaPencil, ligma_pencil, LIGMA_TYPE_PAINTBRUSH)


void
ligma_pencil_register (Ligma                      *ligma,
                      LigmaPaintRegisterCallback  callback)
{
  (* callback) (ligma,
                LIGMA_TYPE_PENCIL,
                LIGMA_TYPE_PENCIL_OPTIONS,
                "ligma-pencil",
                _("Pencil"),
                "ligma-tool-pencil");
}

static void
ligma_pencil_class_init (LigmaPencilClass *klass)
{
}

static void
ligma_pencil_init (LigmaPencil *pencil)
{
}
