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

#include "core/gimpbrush.h"
#include "core/gimpbuffer.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "file/file-utils.h"

#include "gimpcontainereditor.h"
#include "gimpcontainerview.h"
#include "gimpcontainerview-utils.h"
#include "gimpdockable.h"
#include "gimplistitem.h"
#include "gimpmenuitem.h"
#include "gimppreview.h"


typedef struct _GimpNameFuncEntry GimpNameFuncEntry;

struct _GimpNameFuncEntry
{
  GType               type;
  GimpItemGetNameFunc name_func;
};


static gchar * gimp_container_view_tool_name_func      (GObject    *object,
							gchar     **tooltip);
static gchar * gimp_container_view_image_name_func     (GObject    *object,
							gchar     **tooltip);
static gchar * gimp_container_view_brush_name_func     (GObject    *object,
							gchar     **tooltip);
static gchar * gimp_container_view_pattern_name_func   (GObject    *object,
							gchar     **tooltip);
static gchar * gimp_container_view_palette_name_func   (GObject    *object,
							gchar     **tooltip);
static gchar * gimp_container_view_buffer_name_func    (GObject    *object,
							gchar     **tooltip);
static gchar * gimp_container_view_imagefile_name_func (GObject    *object,
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

GimpContainerView *
gimp_container_view_get_by_dockable (GimpDockable *dockable)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  if (GTK_BOX (dockable)->children)
    {
      GtkWidget *child;

      child = ((GtkBoxChild *) GTK_BOX (dockable)->children->data)->widget;

      if (GIMP_IS_CONTAINER_EDITOR (child))
        {
          return GIMP_CONTAINER_EDITOR (child)->view;
        }
      else if (GIMP_IS_CONTAINER_VIEW (child))
        {
          return GIMP_CONTAINER_VIEW (child);
        }
    }

  return NULL;
}

GimpItemGetNameFunc
gimp_container_view_get_built_in_name_func (GType type)
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

static GimpViewable *
gimp_container_view_get_name_func_viewable (GObject *object)
{
  if (GIMP_IS_VIEWABLE (object))
    {
      return GIMP_VIEWABLE (object);
    }
  if (GIMP_IS_PREVIEW (object))
    {
      return GIMP_PREVIEW (object)->viewable;
    }
  else if (GIMP_IS_LIST_ITEM (object))
    {
      return GIMP_PREVIEW (GIMP_LIST_ITEM (object)->preview)->viewable;
    }
  else if (GIMP_IS_MENU_ITEM (object))
    {
      return GIMP_PREVIEW (GIMP_MENU_ITEM (object)->preview)->viewable;
    }

  g_warning ("%s: can't figure GimpViewable from type %s",
             G_STRLOC, g_type_name (G_TYPE_FROM_INSTANCE (object)));

  return NULL;
}

static gchar *
gimp_container_view_tool_name_func (GObject  *object,
				    gchar   **tooltip)
{
  GimpViewable *viewable;

  viewable = gimp_container_view_get_name_func_viewable (object);

  if (viewable)
    {
      GimpToolInfo *tool_info;

      tool_info = GIMP_TOOL_INFO (viewable);

      if (tooltip)
        *tooltip = NULL;

      return g_strdup (tool_info->blurb);
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_image_name_func (GObject  *object,
                                     gchar   **tooltip)
{
  GimpViewable *viewable;

  viewable = gimp_container_view_get_name_func_viewable (object);

  if (viewable)
    {
      GimpImage   *gimage;
      const gchar *uri;
      gchar       *basename;
      gchar       *retval;

      gimage = GIMP_IMAGE (viewable);

      uri = gimp_image_get_uri (GIMP_IMAGE (gimage));

      basename = file_utils_uri_to_utf8_basename (uri);

      if (tooltip)
        {
          gchar *filename;

          filename = file_utils_uri_to_utf8_filename (uri);

          *tooltip = filename;
        }

      retval = g_strdup_printf ("%s-%d",
                                basename,
                                gimp_image_get_ID (gimage));

      g_free (basename);

      return retval;
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_brush_name_func (GObject  *object,
                                     gchar   **tooltip)
{
  GimpViewable *viewable;

  viewable = gimp_container_view_get_name_func_viewable (object);

  if (viewable)
    {
      GimpBrush *brush;

      brush = GIMP_BRUSH (viewable);

      if (tooltip)
        *tooltip = NULL;

      return g_strdup_printf ("%s (%d x %d)",
                              GIMP_OBJECT (brush)->name,
                              brush->mask->width,
                              brush->mask->height);
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_pattern_name_func (GObject  *object,
                                       gchar   **tooltip)
{
  GimpViewable *viewable;

  viewable = gimp_container_view_get_name_func_viewable (object);

  if (viewable)
    {
      GimpPattern *pattern;

      pattern = GIMP_PATTERN (viewable);

      if (tooltip)
        *tooltip = NULL;

      return g_strdup_printf ("%s (%d x %d)",
                              GIMP_OBJECT (pattern)->name,
                              pattern->mask->width,
                              pattern->mask->height);
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_palette_name_func (GObject  *object,
                                       gchar   **tooltip)
{
  GimpViewable *viewable;

  viewable = gimp_container_view_get_name_func_viewable (object);

  if (viewable)
    {
      GimpPalette *palette;

      palette = GIMP_PALETTE (viewable);

      if (tooltip)
        *tooltip = NULL;

      return g_strdup_printf ("%s (%d)",
                              GIMP_OBJECT (palette)->name,
                              palette->n_colors);
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_buffer_name_func (GObject  *object,
                                      gchar   **tooltip)
{
  GimpViewable *viewable;

  viewable = gimp_container_view_get_name_func_viewable (object);

  if (viewable)
    {
      GimpBuffer *buffer;

      buffer = GIMP_BUFFER (viewable);

      if (tooltip)
        *tooltip = NULL;

      return g_strdup_printf ("%s (%d x %d)",
                              GIMP_OBJECT (buffer)->name,
                              gimp_buffer_get_width (buffer),
                              gimp_buffer_get_height (buffer));
    }

  return g_strdup ("EEK");
}

static gchar *
gimp_container_view_imagefile_name_func (GObject  *object,
                                         gchar   **tooltip)
{
  GimpViewable *viewable;

  viewable = gimp_container_view_get_name_func_viewable (object);

  if (viewable)
    {
      GimpImagefile *imagefile;
      const gchar   *uri;
      gchar         *basename;

      imagefile = GIMP_IMAGEFILE (viewable);

      uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

      basename = file_utils_uri_to_utf8_basename (uri);

      if (tooltip)
        {
          gchar       *filename;
          const gchar *desc;

          filename = file_utils_uri_to_utf8_filename (uri);
          desc     = gimp_imagefile_get_description (imagefile);

          if (desc)
            {
              *tooltip = g_strdup_printf ("%s\n%s", filename, desc);
              g_free (filename);
            }
          else
            {
              *tooltip = filename;
            }
        }

      if (imagefile->width > 0 && imagefile->height > 0)
        {
          gchar *tmp = basename;

          basename = g_strdup_printf ("%s (%d x %d)",
                                      tmp,
                                      imagefile->width,
                                      imagefile->height);
          g_free (tmp);
        }

      return basename;
    }

  return g_strdup ("EEK");
}
