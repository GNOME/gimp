/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppreview-utils.c
 * Copyright (C) 2001-2002 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/gimpbrush.h"
#include "core/gimpbuffer.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "gimpbrushpreview.h"
#include "gimpbufferpreview.h"
#include "gimpdrawablepreview.h"
#include "gimpgradientpreview.h"
#include "gimpimagepreview.h"
#include "gimpimagefilepreview.h"
#include "gimppalettepreview.h"
#include "gimppatternpreview.h"
#include "gimptoolinfopreview.h"


GType
gimp_preview_type_from_viewable (GimpViewable *viewable)
{
  GType type = GIMP_TYPE_PREVIEW;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), G_TYPE_NONE);

  if (GIMP_IS_BRUSH (viewable))
    {
      type = GIMP_TYPE_BRUSH_PREVIEW;
    }
  else if (GIMP_IS_DRAWABLE (viewable))
    {
      type = GIMP_TYPE_DRAWABLE_PREVIEW;
    }
  else if (GIMP_IS_IMAGE (viewable))
    {
      type = GIMP_TYPE_IMAGE_PREVIEW;
    }
  else if (GIMP_IS_PATTERN (viewable))
    {
      type = GIMP_TYPE_PATTERN_PREVIEW;
    }
  else if (GIMP_IS_GRADIENT (viewable))
    {
      type = GIMP_TYPE_GRADIENT_PREVIEW;
    }
  else if (GIMP_IS_PALETTE (viewable))
    {
      type = GIMP_TYPE_PALETTE_PREVIEW;
    }
  else if (GIMP_IS_BUFFER (viewable))
    {
      type = GIMP_TYPE_BUFFER_PREVIEW;
    }
  else if (GIMP_IS_TOOL_INFO (viewable))
    {
      type = GIMP_TYPE_TOOL_INFO_PREVIEW;
    }
  else if (GIMP_IS_IMAGEFILE (viewable))
    {
      type = GIMP_TYPE_IMAGEFILE_PREVIEW;
    }

  return type;
}
