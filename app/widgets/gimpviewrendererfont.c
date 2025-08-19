/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererfont.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "gimpviewrendererfont.h"


G_DEFINE_TYPE (GimpViewRendererFont,
               gimp_view_renderer_font,
               GIMP_TYPE_VIEW_RENDERER)

#define parent_class gimp_view_renderer_font_parent_class


static void
gimp_view_renderer_font_class_init (GimpViewRendererFontClass *klass)
{
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  renderer_class->default_bg      = GIMP_VIEW_BG_WHITE;
  renderer_class->follow_theme_bg = GIMP_VIEW_BG_STYLE;
}

static void
gimp_view_renderer_font_init (GimpViewRendererFont *renderer)
{
}
