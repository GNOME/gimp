/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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
#ifndef __GIMP_DND_H__
#define __GIMP_DND_H__

#include <gtk/gtk.h>

#include "gimpbrush.h"
#include "gimpdrawable.h"
#include "palette_entries.h"
#include "patterns.h"
#include "gradient.h"
#include "toolsF.h"

enum
{
  GIMP_DND_TYPE_URI_LIST,
  GIMP_DND_TYPE_TEXT_PLAIN,
  GIMP_DND_TYPE_NETSCAPE_URL,
  GIMP_DND_TYPE_IMAGE,
  GIMP_DND_TYPE_LAYER,
  GIMP_DND_TYPE_CHANNEL,
  GIMP_DND_TYPE_LAYER_MASK,
  GIMP_DND_TYPE_COMPONENT,
  GIMP_DND_TYPE_PATH,
  GIMP_DND_TYPE_COLOR,
  GIMP_DND_TYPE_BRUSH,
  GIMP_DND_TYPE_PATTERN,
  GIMP_DND_TYPE_GRADIENT,
  GIMP_DND_TYPE_PALETTE,
  GIMP_DND_TYPE_TOOL
};

#define GIMP_TARGET_URI_LIST \
        { "text/uri-list", 0, GIMP_DND_TYPE_URI_LIST }

#define GIMP_TARGET_TEXT_PLAIN \
        { "text/plain", 0, GIMP_DND_TYPE_TEXT_PLAIN }

#define GIMP_TARGET_NETSCAPE_URL \
        { "_NETSCAPE_URL", 0, GIMP_DND_TYPE_NETSCAPE_URL }

#define GIMP_TARGET_IMAGE \
        { "GIMP_IMAGE", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_IMAGE }

#define GIMP_TARGET_LAYER \
        { "GIMP_LAYER", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_LAYER }

#define GIMP_TARGET_CHANNEL \
        { "GIMP_CHANNEL", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_CHANNEL }

#define GIMP_TARGET_LAYER_MASK \
        { "GIMP_LAYER_MASK", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_LAYER_MASK }

#define GIMP_TARGET_COMPONENT \
        { "GIMP_COMPONENT", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_COMPONENT }

#define GIMP_TARGET_PATH \
        { "GIMP_PATH", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_PATH }

#define GIMP_TARGET_COLOR \
        { "application/x-color", 0, GIMP_DND_TYPE_COLOR }

#define GIMP_TARGET_BRUSH \
        { "GIMP_BRUSH", 0, GIMP_DND_TYPE_BRUSH }

#define GIMP_TARGET_PATTERN \
        { "GIMP_PATTERN", 0, GIMP_DND_TYPE_PATTERN }

#define GIMP_TARGET_GRADIENT \
        { "GIMP_GRADIENT", 0, GIMP_DND_TYPE_GRADIENT }

#define GIMP_TARGET_PALETTE \
        { "GIMP_PALETTE", 0, GIMP_DND_TYPE_PALETTE }

#define GIMP_TARGET_TOOL \
        { "GIMP_TOOL", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_TOOL }

typedef enum
{
  GIMP_DROP_NONE,
  GIMP_DROP_ABOVE,
  GIMP_DROP_BELOW
} GimpDropType;

/*  color dnd functions  */

typedef void (* GimpDndDropColorFunc) (GtkWidget *widget,
				       guchar     r,
				       guchar     g,
				       guchar     b,
				       gpointer   data);
typedef void (* GimpDndDragColorFunc) (GtkWidget *widget,
				       guchar    *r,
				       guchar    *g,
				       guchar    *b,
				       gpointer   data);

void  gimp_dnd_color_source_set    (GtkWidget            *widget,
				    GimpDndDragColorFunc  get_color_func,
				    gpointer              data);
void  gimp_dnd_color_dest_set      (GtkWidget            *widget,
				    GimpDndDropColorFunc  set_color_func,
				    gpointer              data);

/*  brush dnd functions  */

typedef void        (* GimpDndDropBrushFunc) (GtkWidget *widget,
					      GimpBrush *brush,
					      gpointer   data);
typedef GimpBrush * (* GimpDndDragBrushFunc) (GtkWidget *widget,
					      gpointer   data);

void  gimp_dnd_brush_source_set    (GtkWidget            *widget,
				    GimpDndDragBrushFunc  get_brush_func,
				    gpointer              data);
void  gimp_dnd_brush_dest_set      (GtkWidget            *widget,
				    GimpDndDropBrushFunc  set_brush_func,
				    gpointer              data);

/*  pattern dnd functions  */

typedef void       (* GimpDndDropPatternFunc) (GtkWidget *widget,
					       GPattern  *pattern,
					       gpointer   data);
typedef GPattern * (* GimpDndDragPatternFunc) (GtkWidget *widget,
					       gpointer   data);

void  gimp_dnd_pattern_source_set  (GtkWidget              *widget,
				    GimpDndDragPatternFunc  get_pattern_func,
				    gpointer                data);
void  gimp_dnd_pattern_dest_set    (GtkWidget              *widget,
				    GimpDndDropPatternFunc  set_pattern_func,
				    gpointer                data);

/*  gradient dnd functions  */

typedef void         (* GimpDndDropGradientFunc) (GtkWidget  *widget,
						  gradient_t *gradient,
						  gpointer    data);
typedef gradient_t * (* GimpDndDragGradientFunc) (GtkWidget  *widget,
						  gpointer    data);

void  gimp_dnd_gradient_source_set (GtkWidget               *widget,
				    GimpDndDragGradientFunc  get_gradient_func,
				    gpointer                 data);
void  gimp_dnd_gradient_dest_set   (GtkWidget               *widget,
				    GimpDndDropGradientFunc  set_gradient_func,
				    gpointer                 data);

/*  palette dnd functions  */

typedef void             (* GimpDndDropPaletteFunc) (GtkWidget      *widget,
						     PaletteEntries *palette,
						     gpointer        data);
typedef PaletteEntries * (* GimpDndDragPaletteFunc) (GtkWidget      *widget,
						     gpointer        data);

void  gimp_dnd_palette_source_set  (GtkWidget              *widget,
				    GimpDndDragPaletteFunc  get_palette_func,
				    gpointer                data);
void  gimp_dnd_palette_dest_set    (GtkWidget              *widget,
				    GimpDndDropPaletteFunc  set_palette_func,
				    gpointer                data);

/*  tool dnd functions  */

typedef void     (* GimpDndDropToolFunc) (GtkWidget *widget,
					  ToolType   tool,
					  gpointer   data);
typedef ToolType (* GimpDndDragToolFunc) (GtkWidget *widget,
					  gpointer   data); 

void  gimp_dnd_tool_source_set     (GtkWidget           *widget,
				    GimpDndDragToolFunc  get_tool_func,
				    gpointer             data);
void  gimp_dnd_tool_dest_set       (GtkWidget           *widget,
				    GimpDndDropToolFunc  set_tool_func,
				    gpointer             data);

/*  drawable dnd functions  */

void  gimp_dnd_set_drawable_preview_icon (GtkWidget      *widget,
					  GdkDragContext *context,
					  GimpDrawable   *drawable);

/*  file / url dnd functions  */

void  gimp_dnd_file_dest_set (GtkWidget *widget);

#endif /* __GIMP_DND_H__ */
