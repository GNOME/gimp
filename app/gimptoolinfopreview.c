/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpToolInfoPreview Widget
 * Copyright (C) 2001 Michael Natterer
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

#include "apptypes.h"

#include "tools/gimptoolinfo.h"

#include "gimpdnd.h"
#include "gimptoolinfopreview.h"
#include "temp_buf.h"


/* FIXME: make tool icons nicer */
#define TOOL_INFO_SIZE 22


static void     gimp_tool_info_preview_class_init (GimpToolInfoPreviewClass *klass);
static void     gimp_tool_info_preview_init       (GimpToolInfoPreview      *preview);

static void           gimp_tool_info_preview_render        (GimpPreview *preview);
static GtkWidget    * gimp_tool_info_preview_create_popup  (GimpPreview *preview);
static gboolean       gimp_tool_info_preview_needs_popup   (GimpPreview *preview);

static GimpViewable * gimp_tool_info_preview_drag_viewable (GtkWidget   *widget,
							    gpointer     data);


static GimpPreviewClass *parent_class = NULL;


GtkType
gimp_tool_info_preview_get_type (void)
{
  static GtkType preview_type = 0;

  if (! preview_type)
    {
      GtkTypeInfo preview_info =
      {
	"GimpToolInfoPreview",
	sizeof (GimpToolInfoPreview),
	sizeof (GimpToolInfoPreviewClass),
	(GtkClassInitFunc) gimp_tool_info_preview_class_init,
	(GtkObjectInitFunc) gimp_tool_info_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      preview_type = gtk_type_unique (GIMP_TYPE_PREVIEW, &preview_info);
    }
  
  return preview_type;
}

static void
gimp_tool_info_preview_class_init (GimpToolInfoPreviewClass *klass)
{
  GtkObjectClass   *object_class;
  GimpPreviewClass *preview_class;

  object_class  = (GtkObjectClass *) klass;
  preview_class = (GimpPreviewClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PREVIEW);

  preview_class->render       = gimp_tool_info_preview_render;
  preview_class->create_popup = gimp_tool_info_preview_create_popup;
  preview_class->needs_popup  = gimp_tool_info_preview_needs_popup;
}

static void
gimp_tool_info_preview_init (GimpToolInfoPreview *tool_info_preview)
{
  gimp_gtk_drag_source_set_by_type (GTK_WIDGET (tool_info_preview),
				    GDK_BUTTON2_MASK,
				    GIMP_TYPE_TOOL_INFO,
				    GDK_ACTION_COPY);
  gimp_dnd_viewable_source_set (GTK_WIDGET (tool_info_preview),
				GIMP_TYPE_TOOL_INFO,
				gimp_tool_info_preview_drag_viewable,
				NULL);
}

static void
gimp_tool_info_preview_render (GimpPreview *preview)
{
  GimpToolInfo *tool_info;
  TempBuf      *temp_buf;
  gint          width;
  gint          height;
  gint          tool_info_width;
  gint          tool_info_height;

  tool_info        = GIMP_TOOL_INFO (preview->viewable);
  tool_info_width  = TOOL_INFO_SIZE;
  tool_info_height = TOOL_INFO_SIZE;

  width  = preview->width;
  height = preview->height;

  if (width  == tool_info_width &&
      height == tool_info_height)
    {
      /* TODO once tool icons are finished */
    }
  else if (width  <= tool_info_width &&
	   height <= tool_info_height)
    {
      /* dito */
    }

  temp_buf = gimp_viewable_get_new_preview (preview->viewable,
					    width, height);

  gimp_preview_render_and_flush (preview,
                                 temp_buf,
                                 -1);

  temp_buf_free (temp_buf);
}

static GtkWidget *
gimp_tool_info_preview_create_popup (GimpPreview *preview)
{
  gint popup_width;
  gint popup_height;

  popup_width  = TOOL_INFO_SIZE;
  popup_height = TOOL_INFO_SIZE;

  return gimp_preview_new_full (preview->viewable,
				popup_width,
				popup_height,
				0,
				TRUE, FALSE, FALSE);
}

static gboolean
gimp_tool_info_preview_needs_popup (GimpPreview *preview)
{
  GimpToolInfo *tool_info;
  gint          tool_info_width;
  gint          tool_info_height;

  tool_info        = GIMP_TOOL_INFO (preview->viewable);
  tool_info_width  = TOOL_INFO_SIZE;
  tool_info_height = TOOL_INFO_SIZE;

  if (tool_info_width > preview->width || tool_info_height > preview->height)
    return TRUE;

  return FALSE;
}

static GimpViewable *
gimp_tool_info_preview_drag_viewable (GtkWidget *widget,
				      gpointer   data)
{
  return GIMP_PREVIEW (widget)->viewable;
}
