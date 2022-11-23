/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_DND_H__
#define __LIGMA_DND_H__


#define LIGMA_TARGET_URI_LIST \
        { "text/uri-list", 0, LIGMA_DND_TYPE_URI_LIST }

#define LIGMA_TARGET_TEXT_PLAIN \
        { "text/plain", 0, LIGMA_DND_TYPE_TEXT_PLAIN }

#define LIGMA_TARGET_NETSCAPE_URL \
        { "_NETSCAPE_URL", 0, LIGMA_DND_TYPE_NETSCAPE_URL }

#define LIGMA_TARGET_XDS \
        { "XdndDirectSave0", 0, LIGMA_DND_TYPE_XDS }

#define LIGMA_TARGET_COLOR \
        { "application/x-color", 0, LIGMA_DND_TYPE_COLOR }

#define LIGMA_TARGET_SVG \
        { "image/svg", 0, LIGMA_DND_TYPE_SVG }

#define LIGMA_TARGET_SVG_XML \
        { "image/svg+xml", 0, LIGMA_DND_TYPE_SVG_XML }

/* just here for documentation purposes, the actual list of targets
 * is created dynamically from available GdkPixbuf loaders
 */
#define LIGMA_TARGET_PIXBUF \
        { NULL, 0, LIGMA_DND_TYPE_PIXBUF }

#define LIGMA_TARGET_IMAGE \
        { "application/x-ligma-image-id", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_IMAGE }

#define LIGMA_TARGET_COMPONENT \
        { "application/x-ligma-component", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_COMPONENT }

#define LIGMA_TARGET_LAYER \
        { "application/x-ligma-layer-id", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_LAYER }

#define LIGMA_TARGET_CHANNEL \
        { "application/x-ligma-channel-id", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_CHANNEL }

#define LIGMA_TARGET_LAYER_MASK \
        { "application/x-ligma-layer-mask-id", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_LAYER_MASK }

#define LIGMA_TARGET_VECTORS \
        { "application/x-ligma-vectors-id", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_VECTORS }

#define LIGMA_TARGET_BRUSH \
        { "application/x-ligma-brush-name", 0, LIGMA_DND_TYPE_BRUSH }

#define LIGMA_TARGET_PATTERN \
        { "application/x-ligma-pattern-name", 0, LIGMA_DND_TYPE_PATTERN }

#define LIGMA_TARGET_GRADIENT \
        { "application/x-ligma-gradient-name", 0, LIGMA_DND_TYPE_GRADIENT }

#define LIGMA_TARGET_PALETTE \
        { "application/x-ligma-palette-name", 0, LIGMA_DND_TYPE_PALETTE }

#define LIGMA_TARGET_FONT \
        { "application/x-ligma-font-name", 0, LIGMA_DND_TYPE_FONT }

#define LIGMA_TARGET_BUFFER \
        { "application/x-ligma-buffer-name", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_BUFFER }

#define LIGMA_TARGET_IMAGEFILE \
        { "application/x-ligma-imagefile-name", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_IMAGEFILE }

#define LIGMA_TARGET_TEMPLATE \
        { "application/x-ligma-template-name", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_TEMPLATE }

#define LIGMA_TARGET_TOOL_ITEM \
        { "application/x-ligma-tool-item-name", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_TOOL_ITEM }

#define LIGMA_TARGET_NOTEBOOK_TAB \
        { "GTK_NOTEBOOK_TAB", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_NOTEBOOK_TAB }

#define LIGMA_TARGET_LAYER_LIST \
        { "application/x-ligma-layer-list", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_LAYER_LIST }

#define LIGMA_TARGET_CHANNEL_LIST \
        { "application/x-ligma-channel-list", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_CHANNEL_LIST }

#define LIGMA_TARGET_VECTORS_LIST \
        { "application/x-ligma-vectors-list", GTK_TARGET_SAME_APP, LIGMA_DND_TYPE_VECTORS_LIST }

/*  dnd initialization  */

void  ligma_dnd_init (Ligma *ligma);


/*  uri list dnd functions  */

typedef GList * (* LigmaDndDragUriListFunc) (GtkWidget *widget,
                                            gpointer   data);
typedef void    (* LigmaDndDropUriListFunc) (GtkWidget *widget,
                                            gint       x,
                                            gint       y,
                                            GList     *uri_list,
                                            gpointer   data);

void  ligma_dnd_uri_list_source_add    (GtkWidget              *widget,
                                       LigmaDndDragUriListFunc  get_uri_list_func,
                                       gpointer                data);
void  ligma_dnd_uri_list_source_remove (GtkWidget              *widget);

void  ligma_dnd_uri_list_dest_add      (GtkWidget              *widget,
                                       LigmaDndDropUriListFunc  set_uri_list_func,
                                       gpointer                data);
void  ligma_dnd_uri_list_dest_remove   (GtkWidget              *widget);


/*  color dnd functions  */

typedef void (* LigmaDndDragColorFunc) (GtkWidget     *widget,
                                       LigmaRGB       *color,
                                       gpointer       data);
typedef void (* LigmaDndDropColorFunc) (GtkWidget     *widget,
                                       gint           x,
                                       gint           y,
                                       const LigmaRGB *color,
                                       gpointer       data);

void  ligma_dnd_color_source_add    (GtkWidget            *widget,
                                    LigmaDndDragColorFunc  get_color_func,
                                    gpointer              data);
void  ligma_dnd_color_source_remove (GtkWidget            *widget);

void  ligma_dnd_color_dest_add      (GtkWidget            *widget,
                                    LigmaDndDropColorFunc  set_color_func,
                                    gpointer              data);
void  ligma_dnd_color_dest_remove   (GtkWidget            *widget);


/*  stream dnd functions  */

typedef guchar * (* LigmaDndDragStreamFunc) (GtkWidget    *widget,
                                            gsize        *stream_len,
                                            gpointer      data);
typedef void     (* LigmaDndDropStreamFunc) (GtkWidget    *widget,
                                            gint          x,
                                            gint          y,
                                            const guchar *stream,
                                            gsize         stream_len,
                                            gpointer      data);

void  ligma_dnd_svg_source_add    (GtkWidget              *widget,
                                  LigmaDndDragStreamFunc   get_svg_func,
                                  gpointer                data);
void  ligma_dnd_svg_source_remove (GtkWidget              *widget);

void  ligma_dnd_svg_dest_add      (GtkWidget              *widget,
                                  LigmaDndDropStreamFunc   set_svg_func,
                                  gpointer                data);
void  ligma_dnd_svg_dest_remove   (GtkWidget              *widget);


/*  pixbuf dnd functions  */

typedef GdkPixbuf * (* LigmaDndDragPixbufFunc) (GtkWidget    *widget,
                                               gpointer      data);
typedef void        (* LigmaDndDropPixbufFunc) (GtkWidget    *widget,
                                               gint          x,
                                               gint          y,
                                               GdkPixbuf    *pixbuf,
                                               gpointer      data);

void  ligma_dnd_pixbuf_source_add    (GtkWidget              *widget,
                                     LigmaDndDragPixbufFunc   get_pixbuf_func,
                                     gpointer                data);
void  ligma_dnd_pixbuf_source_remove (GtkWidget              *widget);

void  ligma_dnd_pixbuf_dest_add      (GtkWidget              *widget,
                                     LigmaDndDropPixbufFunc   set_pixbuf_func,
                                     gpointer                data);
void  ligma_dnd_pixbuf_dest_remove   (GtkWidget              *widget);


/*  component dnd functions  */

typedef LigmaImage * (* LigmaDndDragComponentFunc) (GtkWidget       *widget,
                                                  LigmaContext    **context,
                                                  LigmaChannelType *channel,
                                                  gpointer         data);
typedef void        (* LigmaDndDropComponentFunc) (GtkWidget       *widget,
                                                  gint             x,
                                                  gint             y,
                                                  LigmaImage       *image,
                                                  LigmaChannelType  channel,
                                                  gpointer         data);

void  ligma_dnd_component_source_add    (GtkWidget                 *widget,
                                        LigmaDndDragComponentFunc   get_comp_func,
                                        gpointer                   data);
void  ligma_dnd_component_source_remove (GtkWidget                 *widget);

void  ligma_dnd_component_dest_add      (GtkWidget                 *widget,
                                        LigmaDndDropComponentFunc   set_comp_func,
                                        gpointer                   data);
void  ligma_dnd_component_dest_remove   (GtkWidget                 *widget);


/*  LigmaViewable (by GType) dnd functions  */

typedef LigmaViewable * (* LigmaDndDragViewableFunc) (GtkWidget     *widget,
                                                    LigmaContext  **context,
                                                    gpointer       data);
typedef void           (* LigmaDndDropViewableFunc) (GtkWidget     *widget,
                                                    gint           x,
                                                    gint           y,
                                                    LigmaViewable  *viewable,
                                                    gpointer       data);


gboolean ligma_dnd_drag_source_set_by_type (GtkWidget               *widget,
                                           GdkModifierType          start_button_mask,
                                           GType                    type,
                                           GdkDragAction            actions);
gboolean ligma_dnd_viewable_source_add     (GtkWidget               *widget,
                                           GType                    type,
                                           LigmaDndDragViewableFunc  get_viewable_func,
                                           gpointer                 data);
gboolean ligma_dnd_viewable_source_remove  (GtkWidget               *widget,
                                           GType                    type);

gboolean ligma_dnd_drag_dest_set_by_type   (GtkWidget               *widget,
                                           GtkDestDefaults          flags,
                                           GType                    type,
                                           gboolean                 list_accepted,
                                           GdkDragAction            actions);

gboolean ligma_dnd_viewable_dest_add       (GtkWidget               *widget,
                                           GType                    type,
                                           LigmaDndDropViewableFunc  set_viewable_func,
                                           gpointer                 data);
gboolean ligma_dnd_viewable_dest_remove    (GtkWidget               *widget,
                                           GType                    type);

LigmaViewable * ligma_dnd_get_drag_viewable (GtkWidget               *widget);

/*  LigmaViewable (by GType) GList dnd functions  */

typedef GList * (* LigmaDndDragViewableListFunc) (GtkWidget     *widget,
                                                 LigmaContext  **context,
                                                 gpointer       data);
typedef void    (* LigmaDndDropViewableListFunc) (GtkWidget     *widget,
                                                 gint           x,
                                                 gint           y,
                                                 GList         *viewables,
                                                 gpointer       data);

gboolean   ligma_dnd_viewable_list_source_add    (GtkWidget                   *widget,
                                                 GType                        type,
                                                 LigmaDndDragViewableListFunc  get_viewable_list_func,
                                                 gpointer                     data);
gboolean   ligma_dnd_viewable_list_source_remove (GtkWidget                   *widget,
                                                 GType                        type);
gboolean   ligma_dnd_viewable_list_dest_add      (GtkWidget                   *widget,
                                                 GType                        type,
                                                 LigmaDndDropViewableListFunc  set_viewable_func,
                                                 gpointer                     data);
gboolean   ligma_dnd_viewable_list_dest_remove   (GtkWidget                   *widget,
                                                 GType                        type);
GList    * ligma_dnd_get_drag_list               (GtkWidget                   *widget);

/*  Direct Save Protocol (XDS)  */

void  ligma_dnd_xds_source_add    (GtkWidget               *widget,
                                  LigmaDndDragViewableFunc  get_image_func,
                                  gpointer                 data);
void  ligma_dnd_xds_source_remove (GtkWidget               *widget);


#endif /* __LIGMA_DND_H__ */
