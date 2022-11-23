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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <math.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmabrush.h"
#include "core/ligmabuffer.h"
#include "core/ligmachannel.h"
#include "core/ligmacontainer.h"
#include "core/ligmadatafactory.h"
#include "core/ligmadrawable.h"
#include "core/ligmagradient.h"
#include "core/ligmaimage.h"
#include "core/ligmaimagefile.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"
#include "core/ligmapalette.h"
#include "core/ligmapattern.h"
#include "core/ligmatemplate.h"
#include "core/ligmatoolitem.h"

#include "text/ligmafont.h"

#include "vectors/ligmavectors.h"

#include "ligmadnd.h"
#include "ligmadnd-xds.h"
#include "ligmapixbuf.h"
#include "ligmaselectiondata.h"
#include "ligmaview.h"
#include "ligmaviewrendererimage.h"

#include "ligma-log.h"
#include "ligma-intl.h"


#define DRAG_PREVIEW_SIZE  LIGMA_VIEW_SIZE_LARGE
#define DRAG_ICON_OFFSET   -8


typedef GtkWidget * (* LigmaDndGetIconFunc)  (GtkWidget        *widget,
                                             GdkDragContext   *context,
                                             GCallback         get_data_func,
                                             gpointer          get_data_data);
typedef void        (* LigmaDndDragDataFunc) (GtkWidget        *widget,
                                             GdkDragContext   *context,
                                             GCallback         get_data_func,
                                             gpointer          get_data_data,
                                             GtkSelectionData *selection);
typedef gboolean    (* LigmaDndDropDataFunc) (GtkWidget        *widget,
                                             gint              x,
                                             gint              y,
                                             GCallback         set_data_func,
                                             gpointer          set_data_data,
                                             GtkSelectionData *selection);


typedef struct _LigmaDndDataDef LigmaDndDataDef;

struct _LigmaDndDataDef
{
  GtkTargetEntry       target_entry;

  const gchar         *get_data_func_name;
  const gchar         *get_data_data_name;

  const gchar         *set_data_func_name;
  const gchar         *set_data_data_name;

  LigmaDndGetIconFunc   get_icon_func;
  LigmaDndDragDataFunc  get_data_func;
  LigmaDndDropDataFunc  set_data_func;
};


static GtkWidget * ligma_dnd_get_viewable_list_icon (GtkWidget      *widget,
                                                    GdkDragContext *context,
                                                    GCallback       get_viewable_list_func,
                                                    gpointer        get_viewable_list_data);
static GtkWidget * ligma_dnd_get_viewable_icon   (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_viewable_func,
                                                 gpointer          get_viewable_data);
static GtkWidget * ligma_dnd_get_component_icon  (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_comp_func,
                                                 gpointer          get_comp_data);
static GtkWidget * ligma_dnd_get_color_icon      (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_color_func,
                                                 gpointer          get_color_data);

static void        ligma_dnd_get_uri_list_data   (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_uri_list_func,
                                                 gpointer          get_uri_list_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_uri_list_data   (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_uri_list_func,
                                                 gpointer          set_uri_list_data,
                                                 GtkSelectionData *selection);

static void        ligma_dnd_get_xds_data        (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_image_func,
                                                 gpointer          get_image_data,
                                                 GtkSelectionData *selection);

static void        ligma_dnd_get_color_data      (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_color_func,
                                                 gpointer          get_color_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_color_data      (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_color_func,
                                                 gpointer          set_color_data,
                                                 GtkSelectionData *selection);

static void        ligma_dnd_get_stream_data     (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_stream_func,
                                                 gpointer          get_stream_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_stream_data     (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_stream_func,
                                                 gpointer          set_stream_data,
                                                 GtkSelectionData *selection);

static void        ligma_dnd_get_pixbuf_data     (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_pixbuf_func,
                                                 gpointer          get_pixbuf_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_pixbuf_data     (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_pixbuf_func,
                                                 gpointer          set_pixbuf_data,
                                                 GtkSelectionData *selection);
static void        ligma_dnd_get_component_data  (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_comp_func,
                                                 gpointer          get_comp_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_component_data  (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_comp_func,
                                                 gpointer          set_comp_data,
                                                 GtkSelectionData *selection);

static void        ligma_dnd_get_image_data      (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_image_func,
                                                 gpointer          get_image_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_image_data      (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_image_func,
                                                 gpointer          set_image_data,
                                                 GtkSelectionData *selection);

static void        ligma_dnd_get_item_data       (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_item_func,
                                                 gpointer          get_item_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_item_data       (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_item_func,
                                                 gpointer          set_item_data,
                                                 GtkSelectionData *selection);

static void        ligma_dnd_get_item_list_data  (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_item_func,
                                                 gpointer          get_item_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_item_list_data  (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_item_func,
                                                 gpointer          set_item_data,
                                                 GtkSelectionData *selection);

static void        ligma_dnd_get_object_data     (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 GCallback         get_object_func,
                                                 gpointer          get_object_data,
                                                 GtkSelectionData *selection);

static gboolean    ligma_dnd_set_brush_data      (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_brush_func,
                                                 gpointer          set_brush_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_pattern_data    (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_pattern_func,
                                                 gpointer          set_pattern_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_gradient_data   (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_gradient_func,
                                                 gpointer          set_gradient_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_palette_data    (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_palette_func,
                                                 gpointer          set_palette_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_font_data       (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_font_func,
                                                 gpointer          set_font_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_buffer_data     (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_buffer_func,
                                                 gpointer          set_buffer_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_imagefile_data  (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_imagefile_func,
                                                 gpointer          set_imagefile_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_template_data   (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_template_func,
                                                 gpointer          set_template_data,
                                                 GtkSelectionData *selection);
static gboolean    ligma_dnd_set_tool_item_data  (GtkWidget        *widget,
                                                 gint              x,
                                                 gint              y,
                                                 GCallback         set_tool_item_func,
                                                 gpointer          set_tool_item_data,
                                                 GtkSelectionData *selection);



static const LigmaDndDataDef dnd_data_defs[] =
{
  {
    { NULL, 0, -1 },

    NULL,
    NULL,

    NULL,
    NULL,
    NULL
  },

  {
    LIGMA_TARGET_URI_LIST,

    "ligma-dnd-get-uri-list-func",
    "ligma-dnd-get-uri-list-data",

    "ligma-dnd-set-uri-list-func",
    "ligma-dnd-set-uri-list-data",

    NULL,
    ligma_dnd_get_uri_list_data,
    ligma_dnd_set_uri_list_data
  },

  {
    LIGMA_TARGET_TEXT_PLAIN,

    NULL,
    NULL,

    "ligma-dnd-set-uri-list-func",
    "ligma-dnd-set-uri-list-data",

    NULL,
    NULL,
    ligma_dnd_set_uri_list_data
  },

  {
    LIGMA_TARGET_NETSCAPE_URL,

    NULL,
    NULL,

    "ligma-dnd-set-uri-list-func",
    "ligma-dnd-set-uri-list-data",

    NULL,
    NULL,
    ligma_dnd_set_uri_list_data
  },

  {
    LIGMA_TARGET_XDS,

    "ligma-dnd-get-xds-func",
    "ligma-dnd-get-xds-data",

    NULL,
    NULL,

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_xds_data,
    NULL
  },

  {
    LIGMA_TARGET_COLOR,

    "ligma-dnd-get-color-func",
    "ligma-dnd-get-color-data",

    "ligma-dnd-set-color-func",
    "ligma-dnd-set-color-data",

    ligma_dnd_get_color_icon,
    ligma_dnd_get_color_data,
    ligma_dnd_set_color_data
  },

  {
    LIGMA_TARGET_SVG,

    "ligma-dnd-get-svg-func",
    "ligma-dnd-get-svg-data",

    "ligma-dnd-set-svg-func",
    "ligma-dnd-set-svg-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_stream_data,
    ligma_dnd_set_stream_data
  },

  {
    LIGMA_TARGET_SVG_XML,

    "ligma-dnd-get-svg-xml-func",
    "ligma-dnd-get-svg-xml-data",

    "ligma-dnd-set-svg-xml-func",
    "ligma-dnd-set-svg-xml-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_stream_data,
    ligma_dnd_set_stream_data
  },

  {
    LIGMA_TARGET_PIXBUF,

    "ligma-dnd-get-pixbuf-func",
    "ligma-dnd-get-pixbuf-data",

    "ligma-dnd-set-pixbuf-func",
    "ligma-dnd-set-pixbuf-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_pixbuf_data,
    ligma_dnd_set_pixbuf_data
  },

  {
    LIGMA_TARGET_IMAGE,

    "ligma-dnd-get-image-func",
    "ligma-dnd-get-image-data",

    "ligma-dnd-set-image-func",
    "ligma-dnd-set-image-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_image_data,
    ligma_dnd_set_image_data,
  },

  {
    LIGMA_TARGET_COMPONENT,

    "ligma-dnd-get-component-func",
    "ligma-dnd-get-component-data",

    "ligma-dnd-set-component-func",
    "ligma-dnd-set-component-data",

    ligma_dnd_get_component_icon,
    ligma_dnd_get_component_data,
    ligma_dnd_set_component_data,
  },

  {
    LIGMA_TARGET_LAYER,

    "ligma-dnd-get-layer-func",
    "ligma-dnd-get-layer-data",

    "ligma-dnd-set-layer-func",
    "ligma-dnd-set-layer-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_item_data,
    ligma_dnd_set_item_data,
  },

  {
    LIGMA_TARGET_CHANNEL,

    "ligma-dnd-get-channel-func",
    "ligma-dnd-get-channel-data",

    "ligma-dnd-set-channel-func",
    "ligma-dnd-set-channel-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_item_data,
    ligma_dnd_set_item_data,
  },

  {
    LIGMA_TARGET_LAYER_MASK,

    "ligma-dnd-get-layer-mask-func",
    "ligma-dnd-get-layer-mask-data",

    "ligma-dnd-set-layer-mask-func",
    "ligma-dnd-set-layer-mask-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_item_data,
    ligma_dnd_set_item_data,
  },

  {
    LIGMA_TARGET_VECTORS,

    "ligma-dnd-get-vectors-func",
    "ligma-dnd-get-vectors-data",

    "ligma-dnd-set-vectors-func",
    "ligma-dnd-set-vectors-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_item_data,
    ligma_dnd_set_item_data,
  },

  {
    LIGMA_TARGET_BRUSH,

    "ligma-dnd-get-brush-func",
    "ligma-dnd-get-brush-data",

    "ligma-dnd-set-brush-func",
    "ligma-dnd-set-brush-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_object_data,
    ligma_dnd_set_brush_data
  },

  {
    LIGMA_TARGET_PATTERN,

    "ligma-dnd-get-pattern-func",
    "ligma-dnd-get-pattern-data",

    "ligma-dnd-set-pattern-func",
    "ligma-dnd-set-pattern-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_object_data,
    ligma_dnd_set_pattern_data
  },

  {
    LIGMA_TARGET_GRADIENT,

    "ligma-dnd-get-gradient-func",
    "ligma-dnd-get-gradient-data",

    "ligma-dnd-set-gradient-func",
    "ligma-dnd-set-gradient-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_object_data,
    ligma_dnd_set_gradient_data
  },

  {
    LIGMA_TARGET_PALETTE,

    "ligma-dnd-get-palette-func",
    "ligma-dnd-get-palette-data",

    "ligma-dnd-set-palette-func",
    "ligma-dnd-set-palette-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_object_data,
    ligma_dnd_set_palette_data
  },

  {
    LIGMA_TARGET_FONT,

    "ligma-dnd-get-font-func",
    "ligma-dnd-get-font-data",

    "ligma-dnd-set-font-func",
    "ligma-dnd-set-font-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_object_data,
    ligma_dnd_set_font_data
  },

  {
    LIGMA_TARGET_BUFFER,

    "ligma-dnd-get-buffer-func",
    "ligma-dnd-get-buffer-data",

    "ligma-dnd-set-buffer-func",
    "ligma-dnd-set-buffer-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_object_data,
    ligma_dnd_set_buffer_data
  },

  {
    LIGMA_TARGET_IMAGEFILE,

    "ligma-dnd-get-imagefile-func",
    "ligma-dnd-get-imagefile-data",

    "ligma-dnd-set-imagefile-func",
    "ligma-dnd-set-imagefile-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_object_data,
    ligma_dnd_set_imagefile_data
  },

  {
    LIGMA_TARGET_TEMPLATE,

    "ligma-dnd-get-template-func",
    "ligma-dnd-get-template-data",

    "ligma-dnd-set-template-func",
    "ligma-dnd-set-template-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_object_data,
    ligma_dnd_set_template_data
  },

  {
    LIGMA_TARGET_TOOL_ITEM,

    "ligma-dnd-get-tool-item-func",
    "ligma-dnd-get-tool-item-data",

    "ligma-dnd-set-tool-item-func",
    "ligma-dnd-set-tool-item-data",

    ligma_dnd_get_viewable_icon,
    ligma_dnd_get_object_data,
    ligma_dnd_set_tool_item_data
  },

  {
    LIGMA_TARGET_NOTEBOOK_TAB,

    NULL,
    NULL,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL
  },

  {
    LIGMA_TARGET_LAYER_LIST,

    "ligma-dnd-get-layer-list-func",
    "ligma-dnd-get-layer-list-data",

    "ligma-dnd-set-layer-list-func",
    "ligma-dnd-set-layer-list-data",

    ligma_dnd_get_viewable_list_icon,
    ligma_dnd_get_item_list_data,
    ligma_dnd_set_item_list_data,
  },

  {
    LIGMA_TARGET_CHANNEL_LIST,

    "ligma-dnd-get-channel-list-func",
    "ligma-dnd-get-channel-list-data",

    "ligma-dnd-set-channel-list-func",
    "ligma-dnd-set-channel-list-data",

    ligma_dnd_get_viewable_list_icon,
    ligma_dnd_get_item_list_data,
    ligma_dnd_set_item_list_data,
  },

  {
    LIGMA_TARGET_VECTORS_LIST,

    "ligma-dnd-get-vectors-list-func",
    "ligma-dnd-get-vectors-list-data",

    "ligma-dnd-set-vectors-list-func",
    "ligma-dnd-set-vectors-list-data",

    ligma_dnd_get_viewable_list_icon,
    ligma_dnd_get_item_list_data,
    ligma_dnd_set_item_list_data,
  },

};


static Ligma *the_dnd_ligma = NULL;


void
ligma_dnd_init (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (the_dnd_ligma == NULL);

  the_dnd_ligma = ligma;
}


/**********************/
/*  helper functions  */
/**********************/

static void
ligma_dnd_target_list_add (GtkTargetList        *list,
                          const GtkTargetEntry *entry)
{
  GdkAtom atom = gdk_atom_intern (entry->target, FALSE);
  guint   info;

  if (! gtk_target_list_find (list, atom, &info) || info != entry->info)
    {
      gtk_target_list_add (list, atom, entry->flags, entry->info);
    }
}


/********************************/
/*  general data dnd functions  */
/********************************/

static void
ligma_dnd_data_drag_begin (GtkWidget      *widget,
                          GdkDragContext *context,
                          gpointer        data)
{
  const LigmaDndDataDef *dnd_data;
  LigmaDndType           data_type;
  GCallback             get_data_func = NULL;
  gpointer              get_data_data = NULL;
  GtkWidget            *icon_widget;

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "ligma-dnd-get-data-type"));

  LIGMA_LOG (DND, "data type %d", data_type);

  if (! data_type)
    return;

  dnd_data = dnd_data_defs + data_type;

  if (dnd_data->get_data_func_name)
    get_data_func = g_object_get_data (G_OBJECT (widget),
                                       dnd_data->get_data_func_name);

  if (dnd_data->get_data_data_name)
    get_data_data = g_object_get_data (G_OBJECT (widget),
                                       dnd_data->get_data_data_name);

  if (! get_data_func)
    return;

  icon_widget = dnd_data->get_icon_func (widget,
                                         context,
                                         get_data_func,
                                         get_data_data);

  if (icon_widget)
    {
      GtkWidget *frame;
      GtkWidget *window;

      window = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DND);
      gtk_window_set_screen (GTK_WINDOW (window),
                             gtk_widget_get_screen (widget));

      gtk_widget_realize (window);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (window), frame);
      gtk_widget_show (frame);

      gtk_container_add (GTK_CONTAINER (frame), icon_widget);
      gtk_widget_show (icon_widget);

      g_object_set_data_full (G_OBJECT (widget), "ligma-dnd-data-widget",
                              window, (GDestroyNotify) gtk_widget_destroy);

      gtk_drag_set_icon_widget (context, window,
                                DRAG_ICON_OFFSET, DRAG_ICON_OFFSET);

      /*  remember for which drag context the widget was made  */
      g_object_set_data (G_OBJECT (window), "ligma-gdk-drag-context", context);
    }
}

static void
ligma_dnd_data_drag_end (GtkWidget      *widget,
                        GdkDragContext *context)
{
  LigmaDndType  data_type;
  GtkWidget   *icon_widget;

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "ligma-dnd-get-data-type"));

  LIGMA_LOG (DND, "data type %d", data_type);

  icon_widget = g_object_get_data (G_OBJECT (widget), "ligma-dnd-data-widget");

  if (icon_widget)
    {
      /*  destroy the icon_widget only if it was made for this drag
       *  context. See bug #139337.
       */
      if (g_object_get_data (G_OBJECT (icon_widget),
                             "ligma-gdk-drag-context") ==
          (gpointer) context)
        {
          g_object_set_data (G_OBJECT (widget), "ligma-dnd-data-widget", NULL);
        }
    }
}

static void
ligma_dnd_data_drag_handle (GtkWidget        *widget,
                           GdkDragContext   *context,
                           GtkSelectionData *selection_data,
                           guint             info,
                           guint             time,
                           gpointer          data)
{
  GCallback    get_data_func = NULL;
  gpointer     get_data_data = NULL;
  LigmaDndType  data_type;

  LIGMA_LOG (DND, "data type %d", info);

  for (data_type = LIGMA_DND_TYPE_NONE + 1;
       data_type <= LIGMA_DND_TYPE_LAST;
       data_type++)
    {
      const LigmaDndDataDef *dnd_data = dnd_data_defs + data_type;

      if (dnd_data->target_entry.info == info)
        {
          LIGMA_LOG (DND, "target %s", dnd_data->target_entry.target);

          if (dnd_data->get_data_func_name)
            get_data_func = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->get_data_func_name);

          if (dnd_data->get_data_data_name)
            get_data_data = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->get_data_data_name);

          if (! get_data_func)
            return;

          dnd_data->get_data_func (widget,
                                   context,
                                   get_data_func,
                                   get_data_data,
                                   selection_data);

          return;
        }
    }
}

static void
ligma_dnd_data_drop_handle (GtkWidget        *widget,
                           GdkDragContext   *context,
                           gint              x,
                           gint              y,
                           GtkSelectionData *selection_data,
                           guint             info,
                           guint             time,
                           gpointer          data)
{
  LigmaDndType data_type;

  LIGMA_LOG (DND, "data type %d", info);

  if (gtk_selection_data_get_length (selection_data) <= 0)
    {
      gtk_drag_finish (context, FALSE, FALSE, time);
      return;
    }

  for (data_type = LIGMA_DND_TYPE_NONE + 1;
       data_type <= LIGMA_DND_TYPE_LAST;
       data_type++)
    {
      const LigmaDndDataDef *dnd_data = dnd_data_defs + data_type;

      if (dnd_data->target_entry.info == info)
        {
          GCallback set_data_func = NULL;
          gpointer  set_data_data = NULL;

          LIGMA_LOG (DND, "target %s", dnd_data->target_entry.target);

          if (dnd_data->set_data_func_name)
            set_data_func = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->set_data_func_name);

          if (dnd_data->set_data_data_name)
            set_data_data = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->set_data_data_name);

          if (set_data_func &&
              dnd_data->set_data_func (widget, x, y,
                                       set_data_func,
                                       set_data_data,
                                       selection_data))
            {
              gtk_drag_finish (context, TRUE, FALSE, time);
              return;
            }

          gtk_drag_finish (context, FALSE, FALSE, time);
          return;
        }
    }
}

static void
ligma_dnd_data_source_add (LigmaDndType  data_type,
                          GtkWidget   *widget,
                          GCallback    get_data_func,
                          gpointer     get_data_data)
{
  const LigmaDndDataDef *dnd_data;
  gboolean              drag_connected;

  dnd_data = dnd_data_defs + data_type;

  /*  set a default drag source if not already done  */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-site-data"))
    gtk_drag_source_set (widget, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                         NULL, 0,
                         GDK_ACTION_COPY | GDK_ACTION_MOVE);

  drag_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "ligma-dnd-drag-connected"));

  if (! drag_connected)
    {
      g_signal_connect (widget, "drag-begin",
                        G_CALLBACK (ligma_dnd_data_drag_begin),
                        NULL);
      g_signal_connect (widget, "drag-end",
                        G_CALLBACK (ligma_dnd_data_drag_end),
                        NULL);
      g_signal_connect (widget, "drag-data-get",
                        G_CALLBACK (ligma_dnd_data_drag_handle),
                        NULL);

      g_object_set_data (G_OBJECT (widget), "ligma-dnd-drag-connected",
                         GINT_TO_POINTER (TRUE));
    }

  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_func_name,
                     get_data_func);
  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_data_name,
                     get_data_data);

  /*  remember the first set source type for drag view creation  */
  if (! g_object_get_data (G_OBJECT (widget), "ligma-dnd-get-data-type"))
    g_object_set_data (G_OBJECT (widget), "ligma-dnd-get-data-type",
                       GINT_TO_POINTER (data_type));

  if (dnd_data->target_entry.target)
    {
      GtkTargetList *target_list;

      target_list = gtk_drag_source_get_target_list (widget);

      if (target_list)
        {
          ligma_dnd_target_list_add (target_list, &dnd_data->target_entry);
        }
      else
        {
          target_list = gtk_target_list_new (&dnd_data->target_entry, 1);

          gtk_drag_source_set_target_list (widget, target_list);
          gtk_target_list_unref (target_list);
        }
    }
}

static gboolean
ligma_dnd_data_source_remove (LigmaDndType  data_type,
                             GtkWidget   *widget)
{
  const LigmaDndDataDef *dnd_data;
  gboolean              drag_connected;
  gboolean              list_changed = FALSE;

  drag_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "ligma-dnd-drag-connected"));

  if (! drag_connected)
    return FALSE;

  dnd_data = dnd_data_defs + data_type;

  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_func_name, NULL);
  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_data_name, NULL);

  /*  remove the dnd type remembered for the dnd icon  */
  if (data_type ==
      GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                          "ligma-dnd-get-data-type")))
    g_object_set_data (G_OBJECT (widget), "ligma-dnd-get-data-type", NULL);

  if (dnd_data->target_entry.target)
    {
      /* Don't just remove the target from the existing list, create a
       * new list without the target and replace the old list. The
       * source's target list is part of a drag operation's state, but
       * only by reference, it's not copied. So when we change the
       * list, we would change the state of that ongoing drag, making
       * it impossible to drop anything. See bug #676522.
       */
      GtkTargetList *target_list = gtk_drag_source_get_target_list (widget);

      if (target_list)
        {
          GtkTargetList  *new_list;
          GtkTargetEntry *targets;
          gint            n_targets_old;
          gint            n_targets_new;
          gint            i;

          targets = gtk_target_table_new_from_list (target_list, &n_targets_old);

          new_list = gtk_target_list_new (NULL, 0);

          for (i = 0; i < n_targets_old; i++)
            {
              if (targets[i].info != data_type)
                {
                  gtk_target_list_add (new_list,
                                       gdk_atom_intern (targets[i].target, FALSE),
                                       targets[i].flags,
                                       targets[i].info);
                }
            }

          gtk_target_table_free (targets, n_targets_old);

          targets = gtk_target_table_new_from_list (new_list, &n_targets_new);
          gtk_target_table_free (targets, n_targets_new);

          if (n_targets_old != n_targets_new)
            {
              list_changed = TRUE;

              if (n_targets_new > 0)
                gtk_drag_source_set_target_list (widget, new_list);
              else
                gtk_drag_source_set_target_list (widget, NULL);
            }

          gtk_target_list_unref (new_list);
        }
    }

  return list_changed;
}

static void
ligma_dnd_data_dest_add (LigmaDndType  data_type,
                        GtkWidget   *widget,
                        gpointer     set_data_func,
                        gpointer     set_data_data)
{
  const LigmaDndDataDef *dnd_data;
  gboolean              drop_connected;

  /*  set a default drag dest if not already done  */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-drag-dest"))
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);

  drop_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "ligma-dnd-drop-connected"));

  if (set_data_func && ! drop_connected)
    {
      g_signal_connect (widget, "drag-data-received",
                        G_CALLBACK (ligma_dnd_data_drop_handle),
                        NULL);

      g_object_set_data (G_OBJECT (widget), "ligma-dnd-drop-connected",
                         GINT_TO_POINTER (TRUE));
    }

  dnd_data = dnd_data_defs + data_type;

  if (set_data_func)
    {
      g_object_set_data (G_OBJECT (widget), dnd_data->set_data_func_name,
                         set_data_func);
      g_object_set_data (G_OBJECT (widget), dnd_data->set_data_data_name,
                         set_data_data);
    }

  if (dnd_data->target_entry.target)
    {
      GtkTargetList *target_list;

      target_list = gtk_drag_dest_get_target_list (widget);

      if (target_list)
        {
          ligma_dnd_target_list_add (target_list, &dnd_data->target_entry);
        }
      else
        {
          target_list = gtk_target_list_new (&dnd_data->target_entry, 1);

          gtk_drag_dest_set_target_list (widget, target_list);
          gtk_target_list_unref (target_list);
        }
    }
}

static void
ligma_dnd_data_dest_remove (LigmaDndType  data_type,
                           GtkWidget   *widget)
{
  const LigmaDndDataDef *dnd_data;

  dnd_data = dnd_data_defs + data_type;

  g_object_set_data (G_OBJECT (widget), dnd_data->set_data_func_name, NULL);
  g_object_set_data (G_OBJECT (widget), dnd_data->set_data_data_name, NULL);

  if (dnd_data->target_entry.target)
    {
      GtkTargetList *target_list;

      target_list = gtk_drag_dest_get_target_list (widget);

      if (target_list)
        {
          GdkAtom atom = gdk_atom_intern (dnd_data->target_entry.target, TRUE);

          if (atom != GDK_NONE)
            gtk_target_list_remove (target_list, atom);
        }
    }
}


/****************************/
/*  uri list dnd functions  */
/****************************/

static void
ligma_dnd_get_uri_list_data (GtkWidget        *widget,
                            GdkDragContext   *context,
                            GCallback         get_uri_list_func,
                            gpointer          get_uri_list_data,
                            GtkSelectionData *selection)
{
  GList *uri_list;

  uri_list = (* (LigmaDndDragUriListFunc) get_uri_list_func) (widget,
                                                             get_uri_list_data);

  LIGMA_LOG (DND, "uri_list %p", uri_list);

  if (uri_list)
    {
      ligma_selection_data_set_uri_list (selection, uri_list);

      g_list_free_full (uri_list, (GDestroyNotify) g_free);
    }
}

static gboolean
ligma_dnd_set_uri_list_data (GtkWidget        *widget,
                            gint              x,
                            gint              y,
                            GCallback         set_uri_list_func,
                            gpointer          set_uri_list_data,
                            GtkSelectionData *selection)
{
  GList *uri_list = ligma_selection_data_get_uri_list (selection);

  LIGMA_LOG (DND, "uri_list %p", uri_list);

  if (! uri_list)
    return FALSE;

  (* (LigmaDndDropUriListFunc) set_uri_list_func) (widget, x, y, uri_list,
                                                  set_uri_list_data);

  g_list_free_full (uri_list, (GDestroyNotify) g_free);

  return TRUE;
}

void
ligma_dnd_uri_list_source_add (GtkWidget              *widget,
                              LigmaDndDragUriListFunc  get_uri_list_func,
                              gpointer                data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_add (LIGMA_DND_TYPE_URI_LIST, widget,
                            G_CALLBACK (get_uri_list_func),
                            data);
}

void
ligma_dnd_uri_list_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_remove (LIGMA_DND_TYPE_URI_LIST, widget);
}

void
ligma_dnd_uri_list_dest_add (GtkWidget              *widget,
                            LigmaDndDropUriListFunc  set_uri_list_func,
                            gpointer                data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  /*  Set a default drag dest if not already done. Explicitly set
   *  COPY and MOVE for file drag destinations. Some file managers
   *  such as Konqueror only offer MOVE by default.
   */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-drag-dest"))
    gtk_drag_dest_set (widget,
                       GTK_DEST_DEFAULT_ALL, NULL, 0,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);

  ligma_dnd_data_dest_add (LIGMA_DND_TYPE_URI_LIST, widget,
                          G_CALLBACK (set_uri_list_func),
                          data);
  ligma_dnd_data_dest_add (LIGMA_DND_TYPE_TEXT_PLAIN, widget,
                          G_CALLBACK (set_uri_list_func),
                          data);
  ligma_dnd_data_dest_add (LIGMA_DND_TYPE_NETSCAPE_URL, widget,
                          G_CALLBACK (set_uri_list_func),
                          data);
}

void
ligma_dnd_uri_list_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_dest_remove (LIGMA_DND_TYPE_URI_LIST, widget);
  ligma_dnd_data_dest_remove (LIGMA_DND_TYPE_TEXT_PLAIN, widget);
  ligma_dnd_data_dest_remove (LIGMA_DND_TYPE_NETSCAPE_URL, widget);
}


/******************************/
/* Direct Save Protocol (XDS) */
/******************************/

static void
ligma_dnd_get_xds_data (GtkWidget        *widget,
                       GdkDragContext   *context,
                       GCallback         get_image_func,
                       gpointer          get_image_data,
                       GtkSelectionData *selection)
{
  LigmaImage   *image;
  LigmaContext *ligma_context;

  image = g_object_get_data (G_OBJECT (context), "ligma-dnd-viewable");

  if (! image)
    image = (LigmaImage *)
      (* (LigmaDndDragViewableFunc) get_image_func) (widget, &ligma_context,
                                                    get_image_data);

  LIGMA_LOG (DND, "image %p", image);

  if (image)
    ligma_dnd_xds_save_image (context, image, selection);
}

static void
ligma_dnd_xds_drag_begin (GtkWidget      *widget,
                         GdkDragContext *context)
{
  const LigmaDndDataDef *dnd_data = dnd_data_defs + LIGMA_DND_TYPE_XDS;
  GCallback             get_data_func;
  gpointer              get_data_data;

  get_data_func = g_object_get_data (G_OBJECT (widget),
                                     dnd_data->get_data_func_name);
  get_data_data = g_object_get_data (G_OBJECT (widget),
                                     dnd_data->get_data_data_name);

  if (get_data_func)
    {
      LigmaImage   *image;
      LigmaContext *ligma_context;

      image = (LigmaImage *)
        (* (LigmaDndDragViewableFunc) get_data_func) (widget, &ligma_context,
                                                     get_data_data);

      LIGMA_LOG (DND, "image %p", image);

      ligma_dnd_xds_source_set (context, image);
    }
}

static void
ligma_dnd_xds_drag_end (GtkWidget      *widget,
                       GdkDragContext *context)
{
  ligma_dnd_xds_source_set (context, NULL);
}

void
ligma_dnd_xds_source_add (GtkWidget               *widget,
                         LigmaDndDragViewableFunc  get_image_func,
                         gpointer                 data)
{
  gulong handler;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_add (LIGMA_DND_TYPE_XDS, widget,
                            G_CALLBACK (get_image_func),
                            data);

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "ligma-dnd-xds-drag-begin"));

  if (! handler)
    {
      handler = g_signal_connect (widget, "drag-begin",
                                  G_CALLBACK (ligma_dnd_xds_drag_begin),
                                  NULL);
      g_object_set_data (G_OBJECT (widget), "ligma-dnd-xds-drag-begin",
                         GUINT_TO_POINTER (handler));
    }

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "ligma-dnd-xds-drag-end"));

  if (! handler)
    {
      handler = g_signal_connect (widget, "drag-end",
                                  G_CALLBACK (ligma_dnd_xds_drag_end),
                                  NULL);
      g_object_set_data (G_OBJECT (widget), "ligma-dnd-xds-drag-end",
                         GUINT_TO_POINTER (handler));
    }
}

void
ligma_dnd_xds_source_remove (GtkWidget *widget)
{
  gulong handler;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "ligma-dnd-xds-drag-begin"));
  if (handler)
    {
      g_signal_handler_disconnect (widget, handler);
      g_object_set_data (G_OBJECT (widget), "ligma-dnd-xds-drag-begin", NULL);
    }

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "ligma-dnd-xds-drag-end"));
  if (handler)
    {
      g_signal_handler_disconnect (widget, handler);
      g_object_set_data (G_OBJECT (widget), "ligma-dnd-xds-drag-end", NULL);
    }

  ligma_dnd_data_source_remove (LIGMA_DND_TYPE_XDS, widget);
}


/*************************/
/*  color dnd functions  */
/*************************/

static GtkWidget *
ligma_dnd_get_color_icon (GtkWidget      *widget,
                         GdkDragContext *context,
                         GCallback       get_color_func,
                         gpointer        get_color_data)
{
  GtkWidget *color_area;
  LigmaRGB    color;

  (* (LigmaDndDragColorFunc) get_color_func) (widget, &color, get_color_data);

  LIGMA_LOG (DND, "called");

  g_object_set_data_full (G_OBJECT (context),
                          "ligma-dnd-color", g_memdup2 (&color, sizeof (LigmaRGB)),
                          (GDestroyNotify) g_free);

  color_area = ligma_color_area_new (&color, LIGMA_COLOR_AREA_SMALL_CHECKS, 0);
  ligma_color_area_set_color_config (LIGMA_COLOR_AREA (color_area),
                                    the_dnd_ligma->config->color_management);
  gtk_widget_set_size_request (color_area,
                               DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);

  return color_area;
}

static void
ligma_dnd_get_color_data (GtkWidget        *widget,
                         GdkDragContext   *context,
                         GCallback         get_color_func,
                         gpointer          get_color_data,
                         GtkSelectionData *selection)
{
  LigmaRGB *c;
  LigmaRGB  color;

  c = g_object_get_data (G_OBJECT (context), "ligma-dnd-color");

  if (c)
    color = *c;
  else
    (* (LigmaDndDragColorFunc) get_color_func) (widget, &color, get_color_data);

  LIGMA_LOG (DND, "called");

  ligma_selection_data_set_color (selection, &color);
}

static gboolean
ligma_dnd_set_color_data (GtkWidget        *widget,
                         gint              x,
                         gint              y,
                         GCallback         set_color_func,
                         gpointer          set_color_data,
                         GtkSelectionData *selection)
{
  LigmaRGB color;

  LIGMA_LOG (DND, "called");

  if (! ligma_selection_data_get_color (selection, &color))
    return FALSE;

  (* (LigmaDndDropColorFunc) set_color_func) (widget, x, y, &color,
                                             set_color_data);

  return TRUE;
}

void
ligma_dnd_color_source_add (GtkWidget            *widget,
                           LigmaDndDragColorFunc  get_color_func,
                           gpointer              data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_add (LIGMA_DND_TYPE_COLOR, widget,
                            G_CALLBACK (get_color_func),
                            data);
}

void
ligma_dnd_color_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_remove (LIGMA_DND_TYPE_COLOR, widget);
}

void
ligma_dnd_color_dest_add (GtkWidget            *widget,
                         LigmaDndDropColorFunc  set_color_func,
                         gpointer              data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_dest_add (LIGMA_DND_TYPE_COLOR, widget,
                          G_CALLBACK (set_color_func),
                          data);
}

void
ligma_dnd_color_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_dest_remove (LIGMA_DND_TYPE_COLOR, widget);
}


/**************************/
/*  stream dnd functions  */
/**************************/

static void
ligma_dnd_get_stream_data (GtkWidget        *widget,
                          GdkDragContext   *context,
                          GCallback         get_stream_func,
                          gpointer          get_stream_data,
                          GtkSelectionData *selection)
{
  guchar *stream;
  gsize   stream_length;

  stream = (* (LigmaDndDragStreamFunc) get_stream_func) (widget, &stream_length,
                                                        get_stream_data);

  LIGMA_LOG (DND, "stream %p, length %" G_GSIZE_FORMAT, stream, stream_length);

  if (stream)
    {
      ligma_selection_data_set_stream (selection, stream, stream_length);
      g_free (stream);
    }
}

static gboolean
ligma_dnd_set_stream_data (GtkWidget        *widget,
                          gint              x,
                          gint              y,
                          GCallback         set_stream_func,
                          gpointer          set_stream_data,
                          GtkSelectionData *selection)
{
  const guchar *stream;
  gsize         stream_length;

  stream = ligma_selection_data_get_stream (selection, &stream_length);

  LIGMA_LOG (DND, "stream %p, length %" G_GSIZE_FORMAT, stream, stream_length);

  if (! stream)
    return FALSE;

  (* (LigmaDndDropStreamFunc) set_stream_func) (widget, x, y,
                                               stream, stream_length,
                                               set_stream_data);

  return TRUE;
}

void
ligma_dnd_svg_source_add (GtkWidget             *widget,
                         LigmaDndDragStreamFunc  get_svg_func,
                         gpointer               data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_add (LIGMA_DND_TYPE_SVG, widget,
                            G_CALLBACK (get_svg_func),
                            data);
  ligma_dnd_data_source_add (LIGMA_DND_TYPE_SVG_XML, widget,
                            G_CALLBACK (get_svg_func),
                            data);
}

void
ligma_dnd_svg_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_remove (LIGMA_DND_TYPE_SVG, widget);
  ligma_dnd_data_source_remove (LIGMA_DND_TYPE_SVG_XML, widget);
}

void
ligma_dnd_svg_dest_add (GtkWidget             *widget,
                       LigmaDndDropStreamFunc  set_svg_func,
                       gpointer               data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_dest_add (LIGMA_DND_TYPE_SVG, widget,
                          G_CALLBACK (set_svg_func),
                          data);
  ligma_dnd_data_dest_add (LIGMA_DND_TYPE_SVG_XML, widget,
                          G_CALLBACK (set_svg_func),
                          data);
}

void
ligma_dnd_svg_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_dest_remove (LIGMA_DND_TYPE_SVG, widget);
  ligma_dnd_data_dest_remove (LIGMA_DND_TYPE_SVG_XML, widget);
}


/**************************/
/*  pixbuf dnd functions  */
/**************************/

static void
ligma_dnd_get_pixbuf_data (GtkWidget        *widget,
                          GdkDragContext   *context,
                          GCallback         get_pixbuf_func,
                          gpointer          get_pixbuf_data,
                          GtkSelectionData *selection)
{
  GdkPixbuf *pixbuf;

  pixbuf = (* (LigmaDndDragPixbufFunc) get_pixbuf_func) (widget,
                                                        get_pixbuf_data);

  LIGMA_LOG (DND, "pixbuf %p", pixbuf);

  if (pixbuf)
    {
      ligma_set_busy (the_dnd_ligma);

      gtk_selection_data_set_pixbuf (selection, pixbuf);
      g_object_unref (pixbuf);

      ligma_unset_busy (the_dnd_ligma);
    }
}

static gboolean
ligma_dnd_set_pixbuf_data (GtkWidget        *widget,
                          gint              x,
                          gint              y,
                          GCallback         set_pixbuf_func,
                          gpointer          set_pixbuf_data,
                          GtkSelectionData *selection)
{
  GdkPixbuf *pixbuf;

  ligma_set_busy (the_dnd_ligma);

  pixbuf = gtk_selection_data_get_pixbuf (selection);

  ligma_unset_busy (the_dnd_ligma);

  LIGMA_LOG (DND, "pixbuf %p", pixbuf);

  if (! pixbuf)
    return FALSE;

  (* (LigmaDndDropPixbufFunc) set_pixbuf_func) (widget, x, y,
                                               pixbuf,
                                               set_pixbuf_data);

  g_object_unref (pixbuf);

  return TRUE;
}

void
ligma_dnd_pixbuf_source_add (GtkWidget             *widget,
                            LigmaDndDragPixbufFunc  get_pixbuf_func,
                            gpointer               data)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_add (LIGMA_DND_TYPE_PIXBUF, widget,
                            G_CALLBACK (get_pixbuf_func),
                            data);

  target_list = gtk_drag_source_get_target_list (widget);

  if (target_list)
    gtk_target_list_ref (target_list);
  else
    target_list = gtk_target_list_new (NULL, 0);

  ligma_pixbuf_targets_add (target_list, LIGMA_DND_TYPE_PIXBUF, TRUE);

  gtk_drag_source_set_target_list (widget, target_list);
  gtk_target_list_unref (target_list);
}

void
ligma_dnd_pixbuf_source_remove (GtkWidget *widget)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_remove (LIGMA_DND_TYPE_PIXBUF, widget);

  target_list = gtk_drag_source_get_target_list (widget);

  if (target_list)
    ligma_pixbuf_targets_remove (target_list);
}

void
ligma_dnd_pixbuf_dest_add (GtkWidget              *widget,
                          LigmaDndDropPixbufFunc   set_pixbuf_func,
                          gpointer                data)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_dest_add (LIGMA_DND_TYPE_PIXBUF, widget,
                          G_CALLBACK (set_pixbuf_func),
                          data);

  target_list = gtk_drag_dest_get_target_list (widget);

  if (target_list)
    gtk_target_list_ref (target_list);
  else
    target_list = gtk_target_list_new (NULL, 0);

  ligma_pixbuf_targets_add (target_list, LIGMA_DND_TYPE_PIXBUF, FALSE);

  gtk_drag_dest_set_target_list (widget, target_list);
  gtk_target_list_unref (target_list);
}

void
ligma_dnd_pixbuf_dest_remove (GtkWidget *widget)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_dest_remove (LIGMA_DND_TYPE_PIXBUF, widget);

  target_list = gtk_drag_dest_get_target_list (widget);

  if (target_list)
    ligma_pixbuf_targets_remove (target_list);
}


/*****************************/
/*  component dnd functions  */
/*****************************/

static GtkWidget *
ligma_dnd_get_component_icon (GtkWidget      *widget,
                             GdkDragContext *context,
                             GCallback       get_comp_func,
                             gpointer        get_comp_data)
{
  GtkWidget       *view;
  LigmaImage       *image;
  LigmaContext     *ligma_context;
  LigmaChannelType  channel;

  image = (* (LigmaDndDragComponentFunc) get_comp_func) (widget, &ligma_context,
                                                        &channel,
                                                        get_comp_data);

  LIGMA_LOG (DND, "image %p, component %d", image, channel);

  if (! image)
    return NULL;

  g_object_set_data_full (G_OBJECT (context),
                          "ligma-dnd-viewable", g_object_ref (image),
                          (GDestroyNotify) g_object_unref);
  g_object_set_data (G_OBJECT (context),
                     "ligma-dnd-component", GINT_TO_POINTER (channel));

  view = ligma_view_new (ligma_context, LIGMA_VIEWABLE (image),
                        DRAG_PREVIEW_SIZE, 0, TRUE);

  LIGMA_VIEW_RENDERER_IMAGE (LIGMA_VIEW (view)->renderer)->channel = channel;

  return view;
}

static void
ligma_dnd_get_component_data (GtkWidget        *widget,
                             GdkDragContext   *context,
                             GCallback         get_comp_func,
                             gpointer          get_comp_data,
                             GtkSelectionData *selection)
{
  LigmaImage       *image;
  LigmaContext     *ligma_context;
  LigmaChannelType  channel = 0;

  image = g_object_get_data (G_OBJECT (context), "ligma-dnd-viewable");
  channel = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (context),
                                                "ligma-dnd-component"));

  if (! image)
    image = (* (LigmaDndDragComponentFunc) get_comp_func) (widget, &ligma_context,
                                                          &channel,
                                                          get_comp_data);

  LIGMA_LOG (DND, "image %p, component %d", image, channel);

  if (image)
    ligma_selection_data_set_component (selection, image, channel);
}

static gboolean
ligma_dnd_set_component_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_comp_func,
                             gpointer          set_comp_data,
                             GtkSelectionData *selection)
{
  LigmaImage       *image;
  LigmaChannelType  channel = 0;

  image = ligma_selection_data_get_component (selection, the_dnd_ligma,
                                             &channel);

  LIGMA_LOG (DND, "image %p, component %d", image, channel);

  if (! image)
    return FALSE;

  (* (LigmaDndDropComponentFunc) set_comp_func) (widget, x, y,
                                                image, channel,
                                                set_comp_data);

  return TRUE;
}

void
ligma_dnd_component_source_add (GtkWidget                *widget,
                               LigmaDndDragComponentFunc  get_comp_func,
                               gpointer                  data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_add (LIGMA_DND_TYPE_COMPONENT, widget,
                            G_CALLBACK (get_comp_func),
                            data);
}

void
ligma_dnd_component_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_source_remove (LIGMA_DND_TYPE_COMPONENT, widget);
}

void
ligma_dnd_component_dest_add (GtkWidget                 *widget,
                             LigmaDndDropComponentFunc   set_comp_func,
                             gpointer                   data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_dest_add (LIGMA_DND_TYPE_COMPONENT, widget,
                          G_CALLBACK (set_comp_func),
                          data);
}

void
ligma_dnd_component_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  ligma_dnd_data_dest_remove (LIGMA_DND_TYPE_COMPONENT, widget);
}


/*******************************************/
/*  LigmaViewable (by GType) dnd functions  */
/*******************************************/

static GtkWidget *
ligma_dnd_get_viewable_icon (GtkWidget      *widget,
                            GdkDragContext *context,
                            GCallback       get_viewable_func,
                            gpointer        get_viewable_data)
{
  LigmaViewable *viewable;
  LigmaContext  *ligma_context;
  GtkWidget    *view;
  gchar        *desc;

  viewable = (* (LigmaDndDragViewableFunc) get_viewable_func) (widget,
                                                              &ligma_context,
                                                              get_viewable_data);

  LIGMA_LOG (DND, "viewable %p", viewable);

  if (! viewable)
    return NULL;

  g_object_set_data_full (G_OBJECT (context),
                          "ligma-dnd-viewable", g_object_ref (viewable),
                          (GDestroyNotify) g_object_unref);

  view = ligma_view_new (ligma_context, viewable,
                        DRAG_PREVIEW_SIZE, 0, TRUE);

  desc = ligma_viewable_get_description (viewable, NULL);

  if (desc)
    {
      GtkWidget *hbox;
      GtkWidget *label;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 3);
      gtk_box_pack_start (GTK_BOX (hbox), view, FALSE, FALSE, 0);
      gtk_widget_show (view);

      label = g_object_new (GTK_TYPE_LABEL,
                            "label",           desc,
                            "xalign",          0.0,
                            "yalign",          0.5,
                            "max-width-chars", 30,
                            "width-chars",     MIN (strlen (desc), 10),
                            "ellipsize",       PANGO_ELLIPSIZE_END,
                            NULL);

      g_free (desc);

      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      gtk_widget_show (label);

      return hbox;
    }

  return view;
}

static LigmaDndType
ligma_dnd_data_type_get_by_g_type (GType    type,
                                  gboolean list)
{
  LigmaDndType dnd_type = LIGMA_DND_TYPE_NONE;

  if (g_type_is_a (type, LIGMA_TYPE_IMAGE) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_IMAGE;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_LAYER))
    {
      dnd_type = list ? LIGMA_DND_TYPE_LAYER_LIST : LIGMA_DND_TYPE_LAYER;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_LAYER_MASK) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_LAYER_MASK;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_CHANNEL))
    {
      dnd_type = list ? LIGMA_DND_TYPE_CHANNEL_LIST : LIGMA_DND_TYPE_CHANNEL;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_VECTORS))
    {
      dnd_type = list ? LIGMA_DND_TYPE_VECTORS_LIST : LIGMA_DND_TYPE_VECTORS;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_BRUSH) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_BRUSH;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_PATTERN) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_PATTERN;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_GRADIENT) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_GRADIENT;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_PALETTE) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_PALETTE;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_FONT) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_FONT;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_BUFFER) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_BUFFER;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_IMAGEFILE) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_IMAGEFILE;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_TEMPLATE) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_TEMPLATE;
    }
  else if (g_type_is_a (type, LIGMA_TYPE_TOOL_ITEM) && ! list)
    {
      dnd_type = LIGMA_DND_TYPE_TOOL_ITEM;
    }

  return dnd_type;
}

gboolean
ligma_dnd_drag_source_set_by_type (GtkWidget       *widget,
                                  GdkModifierType  start_button_mask,
                                  GType            type,
                                  GdkDragAction    actions)
{
  LigmaDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = ligma_dnd_data_type_get_by_g_type (type, FALSE);

  if (dnd_type == LIGMA_DND_TYPE_NONE)
    return FALSE;

  gtk_drag_source_set (widget, start_button_mask,
                       &dnd_data_defs[dnd_type].target_entry, 1,
                       actions);

  return TRUE;
}

gboolean
ligma_dnd_drag_dest_set_by_type (GtkWidget       *widget,
                                GtkDestDefaults  flags,
                                GType            type,
                                gboolean         list_accepted,
                                GdkDragAction    actions)
{
  GtkTargetEntry target_entries[2];
  LigmaDndType    dnd_type;
  gint           target_entries_n = 0;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  if (list_accepted)
    {
      dnd_type = ligma_dnd_data_type_get_by_g_type (type, TRUE);

      if (dnd_type != LIGMA_DND_TYPE_NONE)
        {
          target_entries[target_entries_n] = dnd_data_defs[dnd_type].target_entry;
          target_entries_n++;
        }
    }

  dnd_type = ligma_dnd_data_type_get_by_g_type (type, FALSE);
  if (dnd_type != LIGMA_DND_TYPE_NONE)
    {
      target_entries[target_entries_n] = dnd_data_defs[dnd_type].target_entry;
      target_entries_n++;
    }

  if (target_entries_n == 0)
    return FALSE;

  gtk_drag_dest_set (widget, flags,
                     (const GtkTargetEntry *) &target_entries,
                     target_entries_n,
                     actions);

  return TRUE;
}

/**
 * ligma_dnd_viewable_source_add:
 * @widget:
 * @type:
 * @get_viewable_func:
 * @data:
 *
 * Sets up @widget as a drag source for a #LigmaViewable object, as
 * returned by @get_viewable_func on @widget and @data.
 *
 * @type must be a list type for drag operations.
 */
gboolean
ligma_dnd_viewable_source_add (GtkWidget               *widget,
                              GType                    type,
                              LigmaDndDragViewableFunc  get_viewable_func,
                              gpointer                 data)
{
  LigmaDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (get_viewable_func != NULL, FALSE);

  dnd_type = ligma_dnd_data_type_get_by_g_type (type, FALSE);

  if (dnd_type == LIGMA_DND_TYPE_NONE)
    return FALSE;

  ligma_dnd_data_source_add (dnd_type, widget,
                            G_CALLBACK (get_viewable_func),
                            data);

  return TRUE;
}

gboolean
ligma_dnd_viewable_source_remove (GtkWidget *widget,
                                 GType      type)
{
  LigmaDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = ligma_dnd_data_type_get_by_g_type (type, FALSE);

  if (dnd_type == LIGMA_DND_TYPE_NONE)
    return FALSE;

  return ligma_dnd_data_source_remove (dnd_type, widget);
}

gboolean
ligma_dnd_viewable_dest_add (GtkWidget               *widget,
                            GType                    type,
                            LigmaDndDropViewableFunc  set_viewable_func,
                            gpointer                 data)
{
  LigmaDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = ligma_dnd_data_type_get_by_g_type (type, FALSE);

  if (dnd_type == LIGMA_DND_TYPE_NONE)
    return FALSE;

  ligma_dnd_data_dest_add (dnd_type, widget,
                          G_CALLBACK (set_viewable_func),
                          data);

  return TRUE;
}

gboolean
ligma_dnd_viewable_dest_remove (GtkWidget *widget,
                               GType      type)
{
  LigmaDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = ligma_dnd_data_type_get_by_g_type (type, FALSE);

  if (dnd_type == LIGMA_DND_TYPE_NONE)
    return FALSE;

  ligma_dnd_data_dest_remove (dnd_type, widget);

  return TRUE;
}

LigmaViewable *
ligma_dnd_get_drag_viewable (GtkWidget *widget)
{
  const LigmaDndDataDef    *dnd_data;
  LigmaDndType              data_type;
  LigmaDndDragViewableFunc  get_data_func = NULL;
  gpointer                 get_data_data = NULL;
  LigmaContext             *context;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "ligma-dnd-get-data-type"));

  if (! data_type)
    return NULL;

  dnd_data = dnd_data_defs + data_type;

  if (dnd_data->get_data_func_name)
    get_data_func = g_object_get_data (G_OBJECT (widget),
                                       dnd_data->get_data_func_name);

  if (dnd_data->get_data_data_name)
    get_data_data = g_object_get_data (G_OBJECT (widget),
                                       dnd_data->get_data_data_name);

  if (! get_data_func)
    return NULL;

  return (LigmaViewable *) (* get_data_func) (widget, &context, get_data_data);
}


/*************************************************/
/*  LigmaViewable (by GType) GList dnd functions  */
/*************************************************/

static GtkWidget *
ligma_dnd_get_viewable_list_icon (GtkWidget      *widget,
                                 GdkDragContext *context,
                                 GCallback       get_list_func,
                                 gpointer        get_list_data)
{
  GList        *viewables;
  LigmaViewable *viewable;
  LigmaContext  *ligma_context;
  GtkWidget    *view;
  gchar        *desc;
  gboolean      desc_use_markup = FALSE;
  gfloat        desc_yalign     = 0.5f;
  gint          desc_width_chars;

  viewables = (* (LigmaDndDragViewableListFunc) get_list_func) (widget,
                                                               &ligma_context,
                                                               get_list_data);

  if (! viewables)
    return NULL;

  viewable = viewables->data;

  LIGMA_LOG (DND, "viewable %p", viewable);

  g_object_set_data_full (G_OBJECT (context),
                          "ligma-dnd-viewable", g_object_ref (viewable),
                          (GDestroyNotify) g_object_unref);

  view = ligma_view_new (ligma_context, viewable,
                        DRAG_PREVIEW_SIZE, 0, TRUE);

  if (g_list_length (viewables) > 1)
    {
      /* When dragging multiple viewable, just show the first of them in the
       * icon box, and the number of viewables being dragged in the
       * description label (instead of the viewable name).
       */
      desc = g_strdup_printf ("<sup><i>(%d)</i></sup>", g_list_length (viewables));
      desc_use_markup  = TRUE;
      desc_yalign      = 0.0f;
      desc_width_chars = (gint) log10 (g_list_length (viewables)) + 3;
    }
  else
    {
      desc = ligma_viewable_get_description (viewable, NULL);
      desc_width_chars =  MIN (strlen (desc), 10);
    }
  g_list_free (viewables);

  if (desc)
    {
      GtkWidget *hbox;
      GtkWidget *label;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 3);
      gtk_box_pack_start (GTK_BOX (hbox), view, FALSE, FALSE, 0);
      gtk_widget_show (view);

      label = g_object_new (GTK_TYPE_LABEL,
                            "label",           desc,
                            "use-markup",      desc_use_markup,
                            "xalign",          0.0,
                            "yalign",          desc_yalign,
                            "max-width-chars", 30,
                            "width-chars",     desc_width_chars,
                            "ellipsize",       PANGO_ELLIPSIZE_END,
                            NULL);

      g_free (desc);

      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      gtk_widget_show (label);

      return hbox;
    }

  return view;
}

/**
 * ligma_dnd_viewable_list_source_add:
 * @widget:
 * @type:
 * @get_viewable_func:
 * @data:
 *
 * Sets up @widget as a drag source for a #GList of #LigmaViewable
 * object, as returned by @get_viewable_func on @widget and @data.
 *
 * @type must be a list type for drag operations (only LigmaLayer so
 * far).
 */
gboolean
ligma_dnd_viewable_list_source_add (GtkWidget                   *widget,
                                   GType                        type,
                                   LigmaDndDragViewableListFunc  get_viewable_list_func,
                                   gpointer                     data)
{
  LigmaDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (get_viewable_list_func != NULL, FALSE);

  dnd_type = ligma_dnd_data_type_get_by_g_type (type, TRUE);

  if (dnd_type == LIGMA_DND_TYPE_NONE)
    return FALSE;

  ligma_dnd_data_source_add (dnd_type, widget,
                            G_CALLBACK (get_viewable_list_func),
                            data);

  return TRUE;
}

gboolean
ligma_dnd_viewable_list_source_remove (GtkWidget *widget,
                                      GType      type)
{
  LigmaDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = ligma_dnd_data_type_get_by_g_type (type, TRUE);

  if (dnd_type == LIGMA_DND_TYPE_NONE)
    return FALSE;

  return ligma_dnd_data_source_remove (dnd_type, widget);
}

gboolean
ligma_dnd_viewable_list_dest_add (GtkWidget                   *widget,
                                 GType                        type,
                                 LigmaDndDropViewableListFunc  set_viewable_func,
                                 gpointer                     data)
{
  LigmaDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = ligma_dnd_data_type_get_by_g_type (type, TRUE);

  if (dnd_type == LIGMA_DND_TYPE_NONE)
    return FALSE;

  ligma_dnd_data_dest_add (dnd_type, widget,
                          G_CALLBACK (set_viewable_func),
                          data);

  return TRUE;
}

gboolean
ligma_dnd_viewable_list_dest_remove (GtkWidget *widget,
                                    GType      type)
{
  LigmaDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = ligma_dnd_data_type_get_by_g_type (type, TRUE);

  if (dnd_type == LIGMA_DND_TYPE_NONE)
    return FALSE;

  ligma_dnd_data_dest_remove (dnd_type, widget);

  return TRUE;
}

GList *
ligma_dnd_get_drag_list (GtkWidget *widget)
{
  const LigmaDndDataDef    *dnd_data;
  LigmaDndType              data_type;
  LigmaDndDragViewableListFunc  get_data_func = NULL;
  gpointer                 get_data_data = NULL;
  LigmaContext             *context;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "ligma-dnd-get-data-type"));

  if (! data_type)
    return NULL;

  dnd_data = dnd_data_defs + data_type;

  if (dnd_data->get_data_func_name)
    get_data_func = g_object_get_data (G_OBJECT (widget),
                                       dnd_data->get_data_func_name);

  if (dnd_data->get_data_data_name)
    get_data_data = g_object_get_data (G_OBJECT (widget),
                                       dnd_data->get_data_data_name);

  if (! get_data_func)
    return NULL;

  return (GList *) (* get_data_func) (widget, &context, get_data_data);
}


/*****************************/
/*  LigmaImage dnd functions  */
/*****************************/

static void
ligma_dnd_get_image_data (GtkWidget        *widget,
                         GdkDragContext   *context,
                         GCallback         get_image_func,
                         gpointer          get_image_data,
                         GtkSelectionData *selection)
{
  LigmaImage   *image;
  LigmaContext *ligma_context;

  image = g_object_get_data (G_OBJECT (context), "ligma-dnd-viewable");

  if (! image)
    image = (LigmaImage *)
      (* (LigmaDndDragViewableFunc) get_image_func) (widget, &ligma_context,
                                                    get_image_data);

  LIGMA_LOG (DND, "image %p", image);

  if (image)
    ligma_selection_data_set_image (selection, image);
}

static gboolean
ligma_dnd_set_image_data (GtkWidget        *widget,
                         gint              x,
                         gint              y,
                         GCallback         set_image_func,
                         gpointer          set_image_data,
                         GtkSelectionData *selection)
{
  LigmaImage *image = ligma_selection_data_get_image (selection, the_dnd_ligma);

  LIGMA_LOG (DND, "image %p", image);

  if (! image)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_image_func) (widget, x, y,
                                                LIGMA_VIEWABLE (image),
                                                set_image_data);

  return TRUE;
}


/****************************/
/*  LigmaItem dnd functions  */
/****************************/

static void
ligma_dnd_get_item_data (GtkWidget        *widget,
                        GdkDragContext   *context,
                        GCallback         get_item_func,
                        gpointer          get_item_data,
                        GtkSelectionData *selection)
{
  LigmaItem    *item;
  LigmaContext *ligma_context;

  item = g_object_get_data (G_OBJECT (context), "ligma-dnd-viewable");

  if (! item)
    item = (LigmaItem *)
      (* (LigmaDndDragViewableFunc) get_item_func) (widget, &ligma_context,
                                                   get_item_data);

  LIGMA_LOG (DND, "item %p", item);

  if (item)
    ligma_selection_data_set_item (selection, item);
}

static gboolean
ligma_dnd_set_item_data (GtkWidget        *widget,
                        gint              x,
                        gint              y,
                        GCallback         set_item_func,
                        gpointer          set_item_data,
                        GtkSelectionData *selection)
{
  LigmaItem *item = ligma_selection_data_get_item (selection, the_dnd_ligma);

  LIGMA_LOG (DND, "item %p", item);

  if (! item)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_item_func) (widget, x, y,
                                               LIGMA_VIEWABLE (item),
                                               set_item_data);

  return TRUE;
}


/**********************************/
/*  LigmaItem GList dnd functions  */
/**********************************/

static void
ligma_dnd_get_item_list_data (GtkWidget        *widget,
                             GdkDragContext   *context,
                             GCallback         get_item_func,
                             gpointer          get_item_data,
                             GtkSelectionData *selection)
{
  GList       *items;
  LigmaContext *ligma_context;

  items = (* (LigmaDndDragViewableListFunc) get_item_func) (widget, &ligma_context,
                                                           get_item_data);

  if (items)
    ligma_selection_data_set_item_list (selection, items);
  g_list_free (items);
}

static gboolean
ligma_dnd_set_item_list_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_item_func,
                             gpointer          set_item_data,
                             GtkSelectionData *selection)
{
  GList *items = ligma_selection_data_get_item_list (selection, the_dnd_ligma);

  if (! items)
    return FALSE;

  (* (LigmaDndDropViewableListFunc) set_item_func) (widget, x, y, items,
                                                   set_item_data);
  g_list_free (items);

  return TRUE;
}

/******************************/
/*  LigmaObject dnd functions  */
/******************************/

static void
ligma_dnd_get_object_data (GtkWidget        *widget,
                          GdkDragContext   *context,
                          GCallback         get_object_func,
                          gpointer          get_object_data,
                          GtkSelectionData *selection)
{
  LigmaObject  *object;
  LigmaContext *ligma_context;

  object = g_object_get_data (G_OBJECT (context), "ligma-dnd-viewable");

  if (! object)
    object = (LigmaObject *)
      (* (LigmaDndDragViewableFunc) get_object_func) (widget, &ligma_context,
                                                     get_object_data);

  LIGMA_LOG (DND, "object %p", object);

  if (LIGMA_IS_OBJECT (object))
    ligma_selection_data_set_object (selection, object);
}


/*****************************/
/*  LigmaBrush dnd functions  */
/*****************************/

static gboolean
ligma_dnd_set_brush_data (GtkWidget        *widget,
                         gint              x,
                         gint              y,
                         GCallback         set_brush_func,
                         gpointer          set_brush_data,
                         GtkSelectionData *selection)
{
  LigmaBrush *brush = ligma_selection_data_get_brush (selection, the_dnd_ligma);

  LIGMA_LOG (DND, "brush %p", brush);

  if (! brush)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_brush_func) (widget, x, y,
                                                LIGMA_VIEWABLE (brush),
                                                set_brush_data);

  return TRUE;
}


/*******************************/
/*  LigmaPattern dnd functions  */
/*******************************/

static gboolean
ligma_dnd_set_pattern_data (GtkWidget        *widget,
                           gint              x,
                           gint              y,
                           GCallback         set_pattern_func,
                           gpointer          set_pattern_data,
                           GtkSelectionData *selection)
{
  LigmaPattern *pattern = ligma_selection_data_get_pattern (selection,
                                                          the_dnd_ligma);

  LIGMA_LOG (DND, "pattern %p", pattern);

  if (! pattern)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_pattern_func) (widget, x, y,
                                                  LIGMA_VIEWABLE (pattern),
                                                  set_pattern_data);

  return TRUE;
}


/********************************/
/*  LigmaGradient dnd functions  */
/********************************/

static gboolean
ligma_dnd_set_gradient_data (GtkWidget        *widget,
                            gint              x,
                            gint              y,
                            GCallback         set_gradient_func,
                            gpointer          set_gradient_data,
                            GtkSelectionData *selection)
{
  LigmaGradient *gradient = ligma_selection_data_get_gradient (selection,
                                                             the_dnd_ligma);

  LIGMA_LOG (DND, "gradient %p", gradient);

  if (! gradient)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_gradient_func) (widget, x, y,
                                                   LIGMA_VIEWABLE (gradient),
                                                   set_gradient_data);

  return TRUE;
}


/*******************************/
/*  LigmaPalette dnd functions  */
/*******************************/

static gboolean
ligma_dnd_set_palette_data (GtkWidget        *widget,
                           gint              x,
                           gint              y,
                           GCallback         set_palette_func,
                           gpointer          set_palette_data,
                           GtkSelectionData *selection)
{
  LigmaPalette *palette = ligma_selection_data_get_palette (selection,
                                                          the_dnd_ligma);

  LIGMA_LOG (DND, "palette %p", palette);

  if (! palette)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_palette_func) (widget, x, y,
                                                  LIGMA_VIEWABLE (palette),
                                                  set_palette_data);

  return TRUE;
}


/****************************/
/*  LigmaFont dnd functions  */
/****************************/

static gboolean
ligma_dnd_set_font_data (GtkWidget        *widget,
                        gint              x,
                        gint              y,
                        GCallback         set_font_func,
                        gpointer          set_font_data,
                        GtkSelectionData *selection)
{
  LigmaFont *font = ligma_selection_data_get_font (selection, the_dnd_ligma);

  LIGMA_LOG (DND, "font %p", font);

  if (! font)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_font_func) (widget, x, y,
                                               LIGMA_VIEWABLE (font),
                                               set_font_data);

  return TRUE;
}


/******************************/
/*  LigmaBuffer dnd functions  */
/******************************/

static gboolean
ligma_dnd_set_buffer_data (GtkWidget        *widget,
                          gint              x,
                          gint              y,
                          GCallback         set_buffer_func,
                          gpointer          set_buffer_data,
                          GtkSelectionData *selection)
{
  LigmaBuffer *buffer = ligma_selection_data_get_buffer (selection, the_dnd_ligma);

  LIGMA_LOG (DND, "buffer %p", buffer);

  if (! buffer)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_buffer_func) (widget, x, y,
                                                 LIGMA_VIEWABLE (buffer),
                                                 set_buffer_data);

  return TRUE;
}


/*********************************/
/*  LigmaImagefile dnd functions  */
/*********************************/

static gboolean
ligma_dnd_set_imagefile_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_imagefile_func,
                             gpointer          set_imagefile_data,
                             GtkSelectionData *selection)
{
  LigmaImagefile *imagefile = ligma_selection_data_get_imagefile (selection,
                                                                the_dnd_ligma);

  LIGMA_LOG (DND, "imagefile %p", imagefile);

  if (! imagefile)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_imagefile_func) (widget, x, y,
                                                    LIGMA_VIEWABLE (imagefile),
                                                    set_imagefile_data);

  return TRUE;
}


/********************************/
/*  LigmaTemplate dnd functions  */
/********************************/

static gboolean
ligma_dnd_set_template_data (GtkWidget        *widget,
                            gint              x,
                            gint              y,
                            GCallback         set_template_func,
                            gpointer          set_template_data,
                            GtkSelectionData *selection)
{
  LigmaTemplate *template = ligma_selection_data_get_template (selection,
                                                             the_dnd_ligma);

  LIGMA_LOG (DND, "template %p", template);

  if (! template)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_template_func) (widget, x, y,
                                                   LIGMA_VIEWABLE (template),
                                                   set_template_data);

  return TRUE;
}


/*********************************/
/*  LigmaToolEntry dnd functions  */
/*********************************/

static gboolean
ligma_dnd_set_tool_item_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_tool_item_func,
                             gpointer          set_tool_item_data,
                             GtkSelectionData *selection)
{
  LigmaToolItem *tool_item = ligma_selection_data_get_tool_item (selection,
                                                               the_dnd_ligma);

  LIGMA_LOG (DND, "tool_item %p", tool_item);

  if (! tool_item)
    return FALSE;

  (* (LigmaDndDropViewableFunc) set_tool_item_func) (widget, x, y,
                                                    LIGMA_VIEWABLE (tool_item),
                                                    set_tool_item_data);

  return TRUE;
}
