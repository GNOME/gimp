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

#include <gtk/gtk.h>

#include "widgets/widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpbrush.h"
#include "core/gimpimage.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpcontainerview-utils.h"
#include "widgets/gimplistitem.h"
#include "widgets/gimpmenuitem.h"
#include "widgets/gimppreview.h"


static gchar * gimp_container_view_tool_name_func    (GtkWidget *widget);
static gchar * gimp_container_view_image_name_func   (GtkWidget *widget);
static gchar * gimp_container_view_brush_name_func   (GtkWidget *widget);
static gchar * gimp_container_view_pattern_name_func (GtkWidget *widget);
static gchar * gimp_container_view_palette_name_func (GtkWidget *widget);


/*  public functions  */

GimpItemGetNameFunc
gimp_container_view_get_built_in_name_func (GtkType  type)
{
  if (type == GIMP_TYPE_TOOL_INFO)
    {
      return gimp_container_view_tool_name_func;
    }
  else if (type == GIMP_TYPE_IMAGE)
    {
      return gimp_container_view_image_name_func;
    }
  else if (type == GIMP_TYPE_BRUSH)
    {
      return gimp_container_view_brush_name_func;
    }
  else if (type == GIMP_TYPE_PATTERN)
    {
      return gimp_container_view_pattern_name_func;
    }
  else if (type == GIMP_TYPE_PALETTE)
    {
      return gimp_container_view_palette_name_func;
    }

  return NULL;
}

gboolean
gimp_container_view_is_built_in_name_func (GimpItemGetNameFunc  get_name_func)
{
  if (get_name_func == gimp_container_view_tool_name_func    ||
      get_name_func == gimp_container_view_image_name_func   ||
      get_name_func == gimp_container_view_brush_name_func   ||
      get_name_func == gimp_container_view_pattern_name_func ||
      get_name_func == gimp_container_view_palette_name_func)
    {
      return TRUE;
    }

  return FALSE;
}


/*  private functions  */

static GimpPreview *
gimp_container_view_get_name_func_preview (GtkWidget *widget)
{
  if (GIMP_IS_PREVIEW (widget))
    {
      return GIMP_PREVIEW (widget);
    }
  else if (GIMP_IS_LIST_ITEM (widget))
    {
      return GIMP_PREVIEW (GIMP_LIST_ITEM (widget)->preview);
    }
  else if (GIMP_IS_MENU_ITEM (widget))
    {
      return GIMP_PREVIEW (GIMP_MENU_ITEM (widget)->preview);
    }

  g_warning ("gimp_container_view_get_name_func_preview(): "
	     "widget type does not match");

  return NULL;
}

static gchar *
gimp_container_view_tool_name_func (GtkWidget *widget)
{
  GimpPreview  *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpToolInfo *tool_info;

      tool_info = GIMP_TOOL_INFO (preview->viewable);

      if (tool_info)
	return g_strdup (tool_info->blurb);
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_image_name_func (GtkWidget *widget)
{
  GimpPreview  *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpImage *gimage;

      gimage = GIMP_IMAGE (preview->viewable);

      if (gimage)
	return g_strdup_printf ("%s-%d",
				g_basename (gimp_image_filename (gimage)),
				gimp_image_get_ID (gimage));
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_brush_name_func (GtkWidget *widget)
{
  GimpPreview  *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpBrush *brush;

      brush = GIMP_BRUSH (preview->viewable);

      if (brush)
	return g_strdup_printf ("%s (%d x %d)",
				GIMP_OBJECT (brush)->name,
				brush->mask->width,
				brush->mask->height);
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_pattern_name_func (GtkWidget *widget)
{
  GimpPreview  *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpPattern *pattern;

      pattern = GIMP_PATTERN (preview->viewable);

      if (pattern)
	return g_strdup_printf ("%s (%d x %d)",
				GIMP_OBJECT (pattern)->name,
				pattern->mask->width,
				pattern->mask->height);
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_palette_name_func (GtkWidget *widget)
{
  GimpPreview  *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpPalette *palette;

      palette = GIMP_PALETTE (preview->viewable);

      if (palette)
	return g_strdup_printf ("%s (%d)",
				GIMP_OBJECT (palette)->name,
				palette->n_colors);
    }

  return g_strdup ("EEK");
}
