/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#pragma once


#define GIMP_TARGET_URI_LIST \
        { "text/uri-list", 0, GIMP_DND_TYPE_URI_LIST }

#define GIMP_TARGET_TEXT_PLAIN \
        { "text/plain", 0, GIMP_DND_TYPE_TEXT_PLAIN }

#define GIMP_TARGET_NETSCAPE_URL \
        { "_NETSCAPE_URL", 0, GIMP_DND_TYPE_NETSCAPE_URL }

#define GIMP_TARGET_XDS \
        { "XdndDirectSave0", 0, GIMP_DND_TYPE_XDS }

#define GIMP_TARGET_COLOR \
        { "application/x-geglcolor", 0, GIMP_DND_TYPE_COLOR }

#define GIMP_TARGET_SVG \
        { "image/svg", 0, GIMP_DND_TYPE_SVG }

#define GIMP_TARGET_SVG_XML \
        { "image/svg+xml", 0, GIMP_DND_TYPE_SVG_XML }

/* just here for documentation purposes, the actual list of targets
 * is created dynamically from available GdkPixbuf loaders
 */
#define GIMP_TARGET_PIXBUF \
        { NULL, 0, GIMP_DND_TYPE_PIXBUF }

#define GIMP_TARGET_IMAGE \
        { "application/x-gimp-image-id", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_IMAGE }

#define GIMP_TARGET_COMPONENT \
        { "application/x-gimp-component", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_COMPONENT }

#define GIMP_TARGET_LAYER \
        { "application/x-gimp-layer-id", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_LAYER }

#define GIMP_TARGET_CHANNEL \
        { "application/x-gimp-channel-id", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_CHANNEL }

#define GIMP_TARGET_LAYER_MASK \
        { "application/x-gimp-layer-mask-id", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_LAYER_MASK }

#define GIMP_TARGET_PATH \
        { "application/x-gimp-path-id", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_PATH }

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

#define GIMP_TARGET_TOOL_ITEM \
        { "application/x-gimp-tool-item-name", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_TOOL_ITEM }

#define GIMP_TARGET_NOTEBOOK_TAB \
        { "GTK_NOTEBOOK_TAB", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_NOTEBOOK_TAB }

#define GIMP_TARGET_LAYER_LIST \
        { "application/x-gimp-layer-list", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_LAYER_LIST }

#define GIMP_TARGET_CHANNEL_LIST \
        { "application/x-gimp-channel-list", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_CHANNEL_LIST }

#define GIMP_TARGET_PATH_LIST \
        { "application/x-gimp-path-list", GTK_TARGET_SAME_APP, GIMP_DND_TYPE_PATH_LIST }


/*  dnd initialization  */

void  gimp_dnd_init (Gimp *gimp);


/*  uri list dnd functions  */

typedef GList * (* GimpDndDragUriListFunc) (GtkWidget *widget,
                                            gpointer   data);
typedef void    (* GimpDndDropUriListFunc) (GtkWidget *widget,
                                            gint       x,
                                            gint       y,
                                            GList     *uri_list,
                                            gpointer   data);

void  gimp_dnd_uri_list_source_add    (GtkWidget              *widget,
                                       GimpDndDragUriListFunc  get_uri_list_func,
                                       gpointer                data);
void  gimp_dnd_uri_list_source_remove (GtkWidget              *widget);

void  gimp_dnd_uri_list_dest_add      (GtkWidget              *widget,
                                       GimpDndDropUriListFunc  set_uri_list_func,
                                       gpointer                data);
void  gimp_dnd_uri_list_dest_remove   (GtkWidget              *widget);


/*  color dnd functions  */

typedef void (* GimpDndDragColorFunc) (GtkWidget     *widget,
                                       GeglColor    **color,
                                       gpointer       data);
typedef void (* GimpDndDropColorFunc) (GtkWidget     *widget,
                                       gint           x,
                                       gint           y,
                                       GeglColor     *color,
                                       gpointer       data);

void  gimp_dnd_color_source_add    (GtkWidget            *widget,
                                    GimpDndDragColorFunc  get_color_func,
                                    gpointer              data);
void  gimp_dnd_color_source_remove (GtkWidget            *widget);

void  gimp_dnd_color_dest_add      (GtkWidget            *widget,
                                    GimpDndDropColorFunc  set_color_func,
                                    gpointer              data);
void  gimp_dnd_color_dest_remove   (GtkWidget            *widget);


/*  stream dnd functions  */

typedef guchar * (* GimpDndDragStreamFunc) (GtkWidget    *widget,
                                            gsize        *stream_len,
                                            gpointer      data);
typedef void     (* GimpDndDropStreamFunc) (GtkWidget    *widget,
                                            gint          x,
                                            gint          y,
                                            const guchar *stream,
                                            gsize         stream_len,
                                            gpointer      data);

void  gimp_dnd_svg_source_add    (GtkWidget              *widget,
                                  GimpDndDragStreamFunc   get_svg_func,
                                  gpointer                data);
void  gimp_dnd_svg_source_remove (GtkWidget              *widget);

void  gimp_dnd_svg_dest_add      (GtkWidget              *widget,
                                  GimpDndDropStreamFunc   set_svg_func,
                                  gpointer                data);
void  gimp_dnd_svg_dest_remove   (GtkWidget              *widget);


/*  pixbuf dnd functions  */

typedef GdkPixbuf * (* GimpDndDragPixbufFunc) (GtkWidget    *widget,
                                               gpointer      data);
typedef void        (* GimpDndDropPixbufFunc) (GtkWidget    *widget,
                                               gint          x,
                                               gint          y,
                                               GdkPixbuf    *pixbuf,
                                               gpointer      data);

void  gimp_dnd_pixbuf_source_add    (GtkWidget              *widget,
                                     GimpDndDragPixbufFunc   get_pixbuf_func,
                                     gpointer                data);
void  gimp_dnd_pixbuf_source_remove (GtkWidget              *widget);

void  gimp_dnd_pixbuf_dest_add      (GtkWidget              *widget,
                                     GimpDndDropPixbufFunc   set_pixbuf_func,
                                     gpointer                data);
void  gimp_dnd_pixbuf_dest_remove   (GtkWidget              *widget);


/*  component dnd functions  */

typedef GimpImage * (* GimpDndDragComponentFunc) (GtkWidget       *widget,
                                                  GimpContext    **context,
                                                  GimpChannelType *channel,
                                                  gpointer         data);
typedef void        (* GimpDndDropComponentFunc) (GtkWidget       *widget,
                                                  gint             x,
                                                  gint             y,
                                                  GimpImage       *image,
                                                  GimpChannelType  channel,
                                                  gpointer         data);

void  gimp_dnd_component_source_add    (GtkWidget                 *widget,
                                        GimpDndDragComponentFunc   get_comp_func,
                                        gpointer                   data);
void  gimp_dnd_component_source_remove (GtkWidget                 *widget);

void  gimp_dnd_component_dest_add      (GtkWidget                 *widget,
                                        GimpDndDropComponentFunc   set_comp_func,
                                        gpointer                   data);
void  gimp_dnd_component_dest_remove   (GtkWidget                 *widget);


/*  GimpViewable (by GType) dnd functions  */

typedef GimpViewable * (* GimpDndDragViewableFunc) (GtkWidget     *widget,
                                                    GimpContext  **context,
                                                    gpointer       data);
typedef void           (* GimpDndDropViewableFunc) (GtkWidget     *widget,
                                                    gint           x,
                                                    gint           y,
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
                                           gboolean                 list_accepted,
                                           GdkDragAction            actions);

gboolean gimp_dnd_viewable_dest_add       (GtkWidget               *widget,
                                           GType                    type,
                                           GimpDndDropViewableFunc  set_viewable_func,
                                           gpointer                 data);
gboolean gimp_dnd_viewable_dest_remove    (GtkWidget               *widget,
                                           GType                    type);

GimpViewable * gimp_dnd_get_drag_viewable (GtkWidget               *widget);

/*  GimpViewable (by GType) GList dnd functions  */

typedef GList * (* GimpDndDragViewableListFunc) (GtkWidget     *widget,
                                                 GimpContext  **context,
                                                 gpointer       data);
typedef void    (* GimpDndDropViewableListFunc) (GtkWidget     *widget,
                                                 gint           x,
                                                 gint           y,
                                                 GList         *viewables,
                                                 gpointer       data);

gboolean   gimp_dnd_viewable_list_source_add    (GtkWidget                   *widget,
                                                 GType                        type,
                                                 GimpDndDragViewableListFunc  get_viewable_list_func,
                                                 gpointer                     data);
gboolean   gimp_dnd_viewable_list_source_remove (GtkWidget                   *widget,
                                                 GType                        type);
gboolean   gimp_dnd_viewable_list_dest_add      (GtkWidget                   *widget,
                                                 GType                        type,
                                                 GimpDndDropViewableListFunc  set_viewable_func,
                                                 gpointer                     data);
gboolean   gimp_dnd_viewable_list_dest_remove   (GtkWidget                   *widget,
                                                 GType                        type);
GList    * gimp_dnd_get_drag_list               (GtkWidget                   *widget);

/*  Direct Save Protocol (XDS)  */

void  gimp_dnd_xds_source_add    (GtkWidget               *widget,
                                  GimpDndDragViewableFunc  get_image_func,
                                  gpointer                 data);
void  gimp_dnd_xds_source_remove (GtkWidget               *widget);
