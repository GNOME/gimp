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

#include "widgets-types.h"

#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "core/gimpbrush.h"
#include "core/gimpbuffer.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "gimpcontainerview.h"
#include "gimpcontainerview-utils.h"
#include "gimplistitem.h"
#include "gimpmenuitem.h"
#include "gimppreview.h"


typedef struct _GimpNameFuncEntry GimpNameFuncEntry;

struct _GimpNameFuncEntry
{
  GType               type;
  GimpItemGetNameFunc name_func;
};


static gchar * gimp_container_view_tool_name_func      (GtkWidget  *widget,
							gchar     **tooltip);
static gchar * gimp_container_view_image_name_func     (GtkWidget  *widget,
							gchar     **tooltip);
static gchar * gimp_container_view_brush_name_func     (GtkWidget  *widget,
							gchar     **tooltip);
static gchar * gimp_container_view_pattern_name_func   (GtkWidget  *widget,
							gchar     **tooltip);
static gchar * gimp_container_view_palette_name_func   (GtkWidget  *widget,
							gchar     **tooltip);
static gchar * gimp_container_view_buffer_name_func    (GtkWidget  *widget,
							gchar     **tooltip);
static gchar * gimp_container_view_imagefile_name_func (GtkWidget  *widget,
							gchar     **tooltip);


static GimpNameFuncEntry name_func_entries[] =
{
  { G_TYPE_NONE, gimp_container_view_tool_name_func      },
  { G_TYPE_NONE, gimp_container_view_image_name_func     },
  { G_TYPE_NONE, gimp_container_view_brush_name_func     },
  { G_TYPE_NONE, gimp_container_view_pattern_name_func   },
  { G_TYPE_NONE, gimp_container_view_palette_name_func   },
  { G_TYPE_NONE, gimp_container_view_buffer_name_func    },
  { G_TYPE_NONE, gimp_container_view_imagefile_name_func }
};


/*  public functions  */

GimpItemGetNameFunc
gimp_container_view_get_built_in_name_func (GType  type)
{
  gint i;

  if (name_func_entries[0].type == G_TYPE_NONE)
    {
      name_func_entries[0].type = GIMP_TYPE_TOOL_INFO;
      name_func_entries[1].type = GIMP_TYPE_IMAGE;
      name_func_entries[2].type = GIMP_TYPE_BRUSH;
      name_func_entries[3].type = GIMP_TYPE_PATTERN;
      name_func_entries[4].type = GIMP_TYPE_PALETTE;
      name_func_entries[5].type = GIMP_TYPE_BUFFER;
      name_func_entries[6].type = GIMP_TYPE_IMAGEFILE;
    }

  for (i = 0; i < G_N_ELEMENTS (name_func_entries); i++)
    {
      if (type == name_func_entries[i].type)
	return name_func_entries[i].name_func;
    }

  return NULL;
}

gboolean
gimp_container_view_is_built_in_name_func (GimpItemGetNameFunc get_name_func)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (name_func_entries); i++)
    {
      if (get_name_func == name_func_entries[i].name_func)
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
gimp_container_view_tool_name_func (GtkWidget  *widget,
				    gchar     **tooltip)
{
  GimpPreview *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpToolInfo *tool_info;

      tool_info = GIMP_TOOL_INFO (preview->viewable);

      if (tool_info)
	{
	  if (tooltip)
	    *tooltip = NULL;

	  return g_strdup (tool_info->blurb);
	}
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_image_name_func (GtkWidget  *widget,
				     gchar     **tooltip)
{
  GimpPreview *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpImage *gimage;

      gimage = GIMP_IMAGE (preview->viewable);

      if (gimage)
	{
	  gchar *basename;
	  gchar *retval;

	  basename = g_path_get_basename (gimp_image_filename (gimage));

	  retval = g_strdup_printf ("%s-%d",
				    basename,
				    gimp_image_get_ID (gimage));

	  g_free (basename);

	  if (tooltip)
	    *tooltip = g_strdup (gimp_image_filename (gimage));

	  return retval;
	}
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_brush_name_func (GtkWidget  *widget,
				     gchar     **tooltip)
{
  GimpPreview *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpBrush *brush;

      brush = GIMP_BRUSH (preview->viewable);

      if (brush)
	{
	  if (tooltip)
	    *tooltip = NULL;

	  return g_strdup_printf ("%s (%d x %d)",
				  GIMP_OBJECT (brush)->name,
				  brush->mask->width,
				  brush->mask->height);
	}
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_pattern_name_func (GtkWidget  *widget,
				       gchar     **tooltip)
{
  GimpPreview *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpPattern *pattern;

      pattern = GIMP_PATTERN (preview->viewable);

      if (pattern)
	{
	  if (tooltip)
	    *tooltip = NULL;

	  return g_strdup_printf ("%s (%d x %d)",
				  GIMP_OBJECT (pattern)->name,
				  pattern->mask->width,
				  pattern->mask->height);
	}
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_palette_name_func (GtkWidget  *widget,
				       gchar     **tooltip)
{
  GimpPreview *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpPalette *palette;

      palette = GIMP_PALETTE (preview->viewable);

      if (palette)
	{
	  if (tooltip)
	    *tooltip = NULL;

	  return g_strdup_printf ("%s (%d)",
				  GIMP_OBJECT (palette)->name,
				  palette->n_colors);
	}
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_buffer_name_func (GtkWidget  *widget,
				      gchar     **tooltip)
{
  GimpPreview *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpBuffer *buffer;

      buffer = GIMP_BUFFER (preview->viewable);

      if (buffer)
	{
	  if (tooltip)
	    *tooltip = NULL;

	  return g_strdup_printf ("%s (%d x %d)",
				  GIMP_OBJECT (buffer)->name,
				  tile_manager_width (buffer->tiles),
				  tile_manager_height (buffer->tiles));
	}
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_imagefile_name_func (GtkWidget  *widget,
					 gchar     **tooltip)
{
  GimpPreview *preview;

  preview = gimp_container_view_get_name_func_preview (widget);

  if (preview)
    {
      GimpImagefile *imagefile;

      imagefile = GIMP_IMAGEFILE (preview->viewable);

      if (imagefile)
	{
	  gchar *basename;

	  basename = g_path_get_basename (GIMP_OBJECT (imagefile)->name);

	  if (tooltip)
	    *tooltip = g_strdup (GIMP_OBJECT (imagefile)->name);

	  if (imagefile->width > 0 && imagefile->height > 0)
	    {
	      return g_strdup_printf ("%s (%d x %d)",
				      basename,
				      imagefile->width,
				      imagefile->height);
	    }
	  else
	    {
	      return g_strdup (basename);
	    }

	  g_free (basename);
	}
    }

  return g_strdup ("EEK");
}
