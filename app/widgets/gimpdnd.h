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


typedef enum
{
  GIMP_DND_TYPE_NONE         = 0,
  GIMP_DND_TYPE_URI_LIST     = 1,
  GIMP_DND_TYPE_TEXT_PLAIN   = 2,
  GIMP_DND_TYPE_NETSCAPE_URL = 3,
  GIMP_DND_TYPE_COLOR        = 4,
  GIMP_DND_TYPE_SVG          = 5,
  GIMP_DND_TYPE_SVG_XML      = 6,
  GIMP_DND_TYPE_IMAGE        = 7,
  GIMP_DND_TYPE_LAYER        = 8,
  GIMP_DND_TYPE_CHANNEL      = 9,
  GIMP_DND_TYPE_LAYER_MASK   = 10,
  GIMP_DND_TYPE_COMPONENT    = 11,
  GIMP_DND_TYPE_VECTORS      = 12,
  GIMP_DND_TYPE_BRUSH        = 13,
  GIMP_DND_TYPE_PATTERN      = 14,
  GIMP_DND_TYPE_GRADIENT     = 15,
  GIMP_DND_TYPE_PALETTE      = 16,
  GIMP_DND_TYPE_FONT         = 17,
  GIMP_DND_TYPE_BUFFER       = 18,
  GIMP_DND_TYPE_IMAGEFILE    = 19,
  GIMP_DND_TYPE_TEMPLATE     = 20,
  GIMP_DND_TYPE_TOOL         = 21,
  GIMP_DND_TYPE_DIALOG       = 22,

  GIMP_DND_TYPE_LAST         = GIMP_DND_TYPE_DIALOG
} GimpDndType;


#define GIMP_TARGET_URI_LIST \
        { "text/uri-list", 0, GIMP_DND_TYPE_URI_LIST }

#define GIMP_TARGET_TEXT_PLAIN \
        { "text/plain", 0, GIMP_DND_TYPE_TEXT_PLAIN }

#define GIMP_TARGET_NETSCAPE_URL \
        { "_NETSCAPE_URL", 0, GIMP_DND_TYPE_NETSCAPE_URL }

#define GIMP_TARGET_COLOR \
        { "application/x-color", 0, GIMP_DND_TYPE_COLOR }

#define GIMP_TARGET_SVG \
        { "image/svg", 0, GIMP_DND_TYPE_SVG }

#define GIMP_TARGET_SVG_XML \
        { "image/svg+xml", 0, GIMP_DND_TYPE_SVG_XML }

#define GIMP_TARGET_IMAGE \
        { "application/x-gimp-image-id", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_IMAGE }

#define GIMP_TARGET_LAYER \
        { "application/x-gimp-layer-id", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_LAYER }

#define GIMP_TARGET_CHANNEL \
        { "application/x-gimp-channel-id", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_CHANNEL }

#define GIMP_TARGET_LAYER_MASK \
        { "application/x-gimp-layer-mask-id", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_LAYER_MASK }

#define GIMP_TARGET_COMPONENT \
        { "application/x-gimp-component", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_COMPONENT }

#define GIMP_TARGET_VECTORS \
        { "application/x-gimp-vectors-id", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_VECTORS }

#define GIMP_TARGET_BRUSH \
        { "application/x-gimp-brush-name", 0, GIMP_DND_TYPE_BRUSH }

#define GIMP_TARGET_PATTERN \
        { "application/x-gimp-pattern-name", 0, GIMP_DND_TYPE_PATTERN }

#define GIMP_TARGET_GRADIENT \
        { "application/x-gimp-gradient-name", 0, GIMP_DND_TYPE_GRADIENT }

#define GIMP_TARGET_PALETTE \
        { "application/x-gimp-palette-name", 0, GIMP_DND_TYPE_PALETTE }

#define GIMP_TARGET_FONT \
        { "application/x-gimp-font-name", 0, GIMP_DND_TYPE_FONT }

#define GIMP_TARGET_BUFFER \
        { "application/x-gimp-buffer-name", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_BUFFER }

#define GIMP_TARGET_IMAGEFILE \
        { "application/x-gimp-imagefile-name", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_IMAGEFILE }

#define GIMP_TARGET_TEMPLATE \
        { "application/x-gimp-template-name", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_TEMPLATE }

#define GIMP_TARGET_TOOL \
        { "application/x-gimp-tool-name", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_TOOL }

#define GIMP_TARGET_DIALOG \
        { "application/x-gimp-dialog", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_DIALOG }


/*  dnd initialization  */

void  gimp_dnd_init (Gimp *gimp);


/*  file / url dnd functions  */

typedef GList * (* GimpDndDragFileFunc) (GtkWidget *widget,
                                         gpointer   data);
typedef void    (* GimpDndDropFileFunc) (GtkWidget *widget,
                                         GList     *files,
                                         gpointer   data);

void  gimp_dnd_file_source_add    (GtkWidget           *widget,
                                   GimpDndDragFileFunc  get_file_func,
                                   gpointer             data);
void  gimp_dnd_file_source_remove (GtkWidget           *widget);

void  gimp_dnd_file_dest_add      (GtkWidget           *widget,
                                   GimpDndDropFileFunc  set_file_func,
                                   gpointer             data);
void  gimp_dnd_file_dest_remove   (GtkWidget           *widget);

/*  standard callback  */
void  gimp_dnd_open_files         (GtkWidget           *widget,
                                   GList               *files,
                                   gpointer             data);


/*  color dnd functions  */

typedef void (* GimpDndDragColorFunc) (GtkWidget     *widget,
				       GimpRGB       *color,
				       gpointer       data);
typedef void (* GimpDndDropColorFunc) (GtkWidget     *widget,
				       const GimpRGB *color,
				       gpointer       data);

void  gimp_dnd_color_source_add    (GtkWidget            *widget,
				    GimpDndDragColorFunc  get_color_func,
				    gpointer              data);
void  gimp_dnd_color_source_remove (GtkWidget            *widget);

void  gimp_dnd_color_dest_add      (GtkWidget            *widget,
				    GimpDndDropColorFunc  set_color_func,
				    gpointer              data);
void  gimp_dnd_color_dest_remove   (GtkWidget            *widget);


/*  svg dnd functions  */

typedef guchar * (* GimpDndDragSvgFunc) (GtkWidget    *widget,
                                         gint         *svg_data_len,
                                         gpointer      data);
typedef void     (* GimpDndDropSvgFunc) (GtkWidget    *widget,
                                         const guchar *svg_data,
                                         gint          svg_data_len,
                                         gpointer      data);

void  gimp_dnd_svg_source_add    (GtkWidget          *widget,
                                  GimpDndDragSvgFunc  get_svg_func,
                                  gpointer            data);
void  gimp_dnd_svg_source_remove (GtkWidget          *widget);

void  gimp_dnd_svg_dest_add      (GtkWidget          *widget,
                                  GimpDndDropSvgFunc  set_svg_func,
                                  gpointer            data);
void  gimp_dnd_svg_dest_remove   (GtkWidget          *widget);


/*  GimpViewable (by GType) dnd functions  */

typedef GimpViewable * (* GimpDndDragViewableFunc) (GtkWidget     *widget,
						    gpointer       data);
typedef void           (* GimpDndDropViewableFunc) (GtkWidget     *widget,
						    GimpViewable  *viewable,
						    gpointer       data);


gboolean gimp_dnd_drag_source_set_by_type (GtkWidget               *widget,
                                           GdkModifierType          start_button_mask,
                                           GType                    type,
                                           GdkDragAction            actions);
gboolean gimp_dnd_viewable_source_add     (GtkWidget               *widget,
                                           GType                    type,
                                           GimpDndDragViewableFunc  get_viewable_func,
                                           gpointer                 data);
gboolean gimp_dnd_viewable_source_remove  (GtkWidget               *widget,
                                           GType                    type);

gboolean gimp_dnd_drag_dest_set_by_type   (GtkWidget               *widget,
                                           GtkDestDefaults          flags,
                                           GType                    type,
                                           GdkDragAction            actions);

gboolean gimp_dnd_viewable_dest_add       (GtkWidget               *widget,
                                           GType                    type,
                                           GimpDndDropViewableFunc  set_viewable_func,
                                           gpointer                 data);
gboolean gimp_dnd_viewable_dest_remove    (GtkWidget               *widget,
                                           GType                    type);

GimpViewable * gimp_dnd_get_drag_data     (GtkWidget               *widget);


#endif /* __GIMP_DND_H__ */
