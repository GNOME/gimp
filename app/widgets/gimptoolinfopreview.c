/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolinfopreview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "core/gimptoolinfo.h"

#include "gimptoolinfopreview.h"


/* FIXME: make tool icons nicer */
#define TOOL_INFO_WIDTH  22
#define TOOL_INFO_HEIGHT 22


static void   gimp_tool_info_preview_class_init (GimpToolInfoPreviewClass *klass);
static void   gimp_tool_info_preview_init       (GimpToolInfoPreview      *preview);

static void        gimp_tool_info_preview_state_changed (GtkWidget    *widget,
							 GtkStateType  previous_state);
static void        gimp_tool_info_preview_render        (GimpPreview  *preview);
static GtkWidget * gimp_tool_info_preview_create_popup  (GimpPreview  *preview);
static gboolean    gimp_tool_info_preview_needs_popup   (GimpPreview  *preview);


static GimpPreviewClass *parent_class = NULL;


GType
gimp_tool_info_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpToolInfoPreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_tool_info_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpToolInfoPreview),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_tool_info_preview_init,
      };

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpToolInfoPreview",
                                             &preview_info, 0);
    }
  
  return preview_type;
}

static void
gimp_tool_info_preview_class_init (GimpToolInfoPreviewClass *klass)
{
  GtkWidgetClass   *widget_class;
  GimpPreviewClass *preview_class;

  widget_class  = GTK_WIDGET_CLASS (klass);
  preview_class = GIMP_PREVIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->state_changed = gimp_tool_info_preview_state_changed;

  preview_class->render       = gimp_tool_info_preview_render;
  preview_class->create_popup = gimp_tool_info_preview_create_popup;
  preview_class->needs_popup  = gimp_tool_info_preview_needs_popup;
}

static void
gimp_tool_info_preview_init (GimpToolInfoPreview *tool_info_preview)
{
}

static void
gimp_tool_info_preview_state_changed (GtkWidget    *widget,
				      GtkStateType  previous_state)
{
  gimp_preview_render (GIMP_PREVIEW (widget));
}

static void
gimp_tool_info_preview_render (GimpPreview *preview)
{
  GtkWidget    *widget;
  GimpToolInfo *tool_info;
  TempBuf      *temp_buf;
  TempBuf      *render_buf;
  guchar        color[3];
  gint          width;
  gint          height;
  gint          tool_info_width;
  gint          tool_info_height;
  gint          x, y;
  guchar       *src;
  guchar       *dest;
  gboolean      new_buf = FALSE;

  widget = GTK_WIDGET (preview);

  tool_info        = GIMP_TOOL_INFO (preview->viewable);
  tool_info_width  = TOOL_INFO_WIDTH;
  tool_info_height = TOOL_INFO_HEIGHT;

  width  = preview->width;
  height = preview->height;

  if (width  == tool_info_width &&
      height == tool_info_height)
    {
      temp_buf = gimp_viewable_get_preview (preview->viewable,
					    width, height);
    }
  else
    {
      temp_buf = gimp_viewable_get_new_preview (preview->viewable,
						width, height);
      new_buf = TRUE;
    }

  color[0] = widget->style->bg[widget->state].red   >> 8;
  color[1] = widget->style->bg[widget->state].green >> 8;
  color[2] = widget->style->bg[widget->state].blue  >> 8;

  render_buf = temp_buf_new (width, height, 3, 0, 0, color);

  src  = temp_buf_data (temp_buf);
  dest = temp_buf_data (render_buf);

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
	{
	  if (src[3] != 0)
	    {
	      *dest++ = *src++;
	      *dest++ = *src++;
	      *dest++ = *src++;

	      src++;
	    }
	  else
	    {
	      src  += 4;
	      dest += 3;
	    }
	}
    }

  if (new_buf)
    temp_buf_free (temp_buf);

  gimp_preview_render_and_flush (preview,
                                 render_buf,
                                 -1);

  temp_buf_free (render_buf);
}

static GtkWidget *
gimp_tool_info_preview_create_popup (GimpPreview *preview)
{
  gint popup_width;
  gint popup_height;

  popup_width  = TOOL_INFO_WIDTH;
  popup_height = TOOL_INFO_HEIGHT;

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
  tool_info_width  = TOOL_INFO_WIDTH;
  tool_info_height = TOOL_INFO_HEIGHT;

  if (tool_info_width > preview->width || tool_info_height > preview->height)
    return TRUE;

  return FALSE;
}
