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

#include <glib-object.h>

#include "paint-types.h"

#include "gimppencil.h"
#include "gimppenciloptions.h"

#include "gimp-intl.h"


void
gimp_pencil_register (Gimp                      *gimp,
                      GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_PENCIL,
                GIMP_TYPE_PENCIL_OPTIONS,
                _("Pencil"));
}

GType
gimp_pencil_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpPencilClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        NULL,           /* class_init     */
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpPencil),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
      };

      type = g_type_register_static (GIMP_TYPE_PAINTBRUSH,
                                     "GimpPencil",
                                     &info, 0);
    }

  return type;
}
