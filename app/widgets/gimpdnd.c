/* GIMP - The GNU Image Manipulation Program
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpbuffer.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"
#include "core/gimptemplate.h"
#include "core/gimptoolinfo.h"

#include "text/gimpfont.h"

#include "vectors/gimpvectors.h"

/*  #define DEBUG_DND  */

#include "gimpdnd.h"
#include "gimpdnd-xds.h"
#include "gimppixbuf.h"
#include "gimpselectiondata.h"
#include "gimpview.h"
#include "gimpviewrendererimage.h"

#include "gimp-intl.h"


#define DRAG_PREVIEW_SIZE  GIMP_VIEW_SIZE_LARGE
#define DRAG_ICON_OFFSET   -8


#ifdef DEBUG_DND
#define D(stmnt) stmnt
#else
#define D(stmnt)
#endif


typedef GtkWidget * (* GimpDndGetIconFunc)  (GtkWidget        *widget,
                                             GCallback         get_data_func,
                                             gpointer          get_data_data);
typedef void        (* GimpDndDragDataFunc) (GtkWidget        *widget,
                                             GdkDragContext   *context,
                                             GCallback         get_data_func,
                                             gpointer          get_data_data,
                                             GtkSelectionData *selection);
typedef gboolean    (* GimpDndDropDataFunc) (GtkWidget        *widget,
                                             gint              x,
                                             gint              y,
                                             GCallback         set_data_func,
                                             gpointer          set_data_data,
                                             GtkSelectionData *selection);


typedef struct _GimpDndDataDef GimpDndDataDef;

struct _GimpDndDataDef
{
  GtkTargetEntry       target_entry;

  const gchar         *get_data_func_name;
  const gchar         *get_data_data_name;

  const gchar         *set_data_func_name;
  const gchar         *set_data_data_name;

  GimpDndGetIconFunc   get_icon_func;
  GimpDndDragDataFunc  get_data_func;
  GimpDndDropDataFunc  set_data_func;
};


static GtkWidget * gimp_dnd_get_viewable_icon  (GtkWidget        *widget,
                                                GCallback         get_viewable_func,
                                                gpointer          get_viewable_data);
static GtkWidget * gimp_dnd_get_component_icon (GtkWidget        *widget,
                                                GCallback         get_comp_func,
                                                gpointer          get_comp_data);
static GtkWidget * gimp_dnd_get_color_icon     (GtkWidget        *widget,
                                                GCallback         get_color_func,
                                                gpointer          get_color_data);

static void        gimp_dnd_get_uri_list_data  (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                GCallback         get_uri_list_func,
                                                gpointer          get_uri_list_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_uri_list_data  (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_uri_list_func,
                                                gpointer          set_uri_list_data,
                                                GtkSelectionData *selection);

static void        gimp_dnd_get_xds_data       (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                GCallback         get_image_func,
                                                gpointer          get_image_data,
                                                GtkSelectionData *selection);

static void        gimp_dnd_get_color_data     (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                GCallback         get_color_func,
                                                gpointer          get_color_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_color_data     (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_color_func,
                                                gpointer          set_color_data,
                                                GtkSelectionData *selection);

static void        gimp_dnd_get_stream_data    (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                GCallback         get_stream_func,
                                                gpointer          get_stream_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_stream_data    (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_stream_func,
                                                gpointer          set_stream_data,
                                                GtkSelectionData *selection);

static void        gimp_dnd_get_pixbuf_data    (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                GCallback         get_pixbuf_func,
                                                gpointer          get_pixbuf_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_pixbuf_data    (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_pixbuf_func,
                                                gpointer          set_pixbuf_data,
                                                GtkSelectionData *selection);
static void        gimp_dnd_get_component_data (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                GCallback         get_comp_func,
                                                gpointer          get_comp_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_component_data (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_comp_func,
                                                gpointer          set_comp_data,
                                                GtkSelectionData *selection);

static void        gimp_dnd_get_image_data     (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                GCallback         get_image_func,
                                                gpointer          get_image_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_image_data     (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_image_func,
                                                gpointer          set_image_data,
                                                GtkSelectionData *selection);

static void        gimp_dnd_get_item_data      (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                GCallback         get_item_func,
                                                gpointer          get_item_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_item_data      (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_item_func,
                                                gpointer          set_item_data,
                                                GtkSelectionData *selection);

static void        gimp_dnd_get_object_data    (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                GCallback         get_object_func,
                                                gpointer          get_object_data,
                                                GtkSelectionData *selection);

static gboolean    gimp_dnd_set_brush_data     (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_brush_func,
                                                gpointer          set_brush_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_pattern_data   (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_pattern_func,
                                                gpointer          set_pattern_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_gradient_data  (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_gradient_func,
                                                gpointer          set_gradient_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_palette_data   (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_palette_func,
                                                gpointer          set_palette_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_font_data      (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_font_func,
                                                gpointer          set_font_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_buffer_data    (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_buffer_func,
                                                gpointer          set_buffer_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_imagefile_data (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_imagefile_func,
                                                gpointer          set_imagefile_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_template_data  (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_template_func,
                                                gpointer          set_template_data,
                                                GtkSelectionData *selection);
static gboolean    gimp_dnd_set_tool_info_data (GtkWidget        *widget,
                                                gint              x,
                                                gint              y,
                                                GCallback         set_tool_info_func,
                                                gpointer          set_tool_info_data,
                                                GtkSelectionData *selection);



static const GimpDndDataDef dnd_data_defs[] =
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
    GIMP_TARGET_URI_LIST,

    "gimp-dnd-get-uri-list-func",
    "gimp-dnd-get-uri-list-data",

    "gimp-dnd-set-uri-list-func",
    "gimp-dnd-set-uri-list-data",

    NULL,
    gimp_dnd_get_uri_list_data,
    gimp_dnd_set_uri_list_data
  },

  {
    GIMP_TARGET_TEXT_PLAIN,

    NULL,
    NULL,

    "gimp-dnd-set-uri-list-func",
    "gimp-dnd-set-uri-list-data",

    NULL,
    NULL,
    gimp_dnd_set_uri_list_data
  },

  {
    GIMP_TARGET_NETSCAPE_URL,

    NULL,
    NULL,

    "gimp-dnd-set-uri-list-func",
    "gimp-dnd-set-uri-list-data",

    NULL,
    NULL,
    gimp_dnd_set_uri_list_data
  },

  {
    GIMP_TARGET_XDS,

    "gimp-dnd-get-xds-func",
    "gimp-dnd-get-xds-data",

    NULL,
    NULL,

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_xds_data,
    NULL
  },

  {
    GIMP_TARGET_COLOR,

    "gimp-dnd-get-color-func",
    "gimp-dnd-get-color-data",

    "gimp-dnd-set-color-func",
    "gimp-dnd-set-color-data",

    gimp_dnd_get_color_icon,
    gimp_dnd_get_color_data,
    gimp_dnd_set_color_data
  },

  {
    GIMP_TARGET_SVG,

    "gimp-dnd-get-svg-func",
    "gimp-dnd-get-svg-data",

    "gimp-dnd-set-svg-func",
    "gimp-dnd-set-svg-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_stream_data,
    gimp_dnd_set_stream_data
  },

  {
    GIMP_TARGET_SVG_XML,

    "gimp-dnd-get-svg-xml-func",
    "gimp-dnd-get-svg-xml-data",

    "gimp-dnd-set-svg-xml-func",
    "gimp-dnd-set-svg-xml-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_stream_data,
    gimp_dnd_set_stream_data
  },

  {
    GIMP_TARGET_PIXBUF,

    "gimp-dnd-get-pixbuf-func",
    "gimp-dnd-get-pixbuf-data",

    "gimp-dnd-set-pixbuf-func",
    "gimp-dnd-set-pixbuf-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_pixbuf_data,
    gimp_dnd_set_pixbuf_data
  },

  {
    GIMP_TARGET_IMAGE,

    "gimp-dnd-get-image-func",
    "gimp-dnd-get-image-data",

    "gimp-dnd-set-image-func",
    "gimp-dnd-set-image-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_image_data,
    gimp_dnd_set_image_data,
  },

  {
    GIMP_TARGET_COMPONENT,

    "gimp-dnd-get-component-func",
    "gimp-dnd-get-component-data",

    "gimp-dnd-set-component-func",
    "gimp-dnd-set-component-data",

    gimp_dnd_get_component_icon,
    gimp_dnd_get_component_data,
    gimp_dnd_set_component_data,
  },

  {
    GIMP_TARGET_LAYER,

    "gimp-dnd-get-layer-func",
    "gimp-dnd-get-layer-data",

    "gimp-dnd-set-layer-func",
    "gimp-dnd-set-layer-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_item_data,
    gimp_dnd_set_item_data,
  },

  {
    GIMP_TARGET_CHANNEL,

    "gimp-dnd-get-channel-func",
    "gimp-dnd-get-channel-data",

    "gimp-dnd-set-channel-func",
    "gimp-dnd-set-channel-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_item_data,
    gimp_dnd_set_item_data,
  },

  {
    GIMP_TARGET_LAYER_MASK,

    "gimp-dnd-get-layer-mask-func",
    "gimp-dnd-get-layer-mask-data",

    "gimp-dnd-set-layer-mask-func",
    "gimp-dnd-set-layer-mask-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_item_data,
    gimp_dnd_set_item_data,
  },

  {
    GIMP_TARGET_VECTORS,

    "gimp-dnd-get-vectors-func",
    "gimp-dnd-get-vectors-data",

    "gimp-dnd-set-vectors-func",
    "gimp-dnd-set-vectors-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_item_data,
    gimp_dnd_set_item_data,
  },

  {
    GIMP_TARGET_BRUSH,

    "gimp-dnd-get-brush-func",
    "gimp-dnd-get-brush-data",

    "gimp-dnd-set-brush-func",
    "gimp-dnd-set-brush-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_object_data,
    gimp_dnd_set_brush_data
  },

  {
    GIMP_TARGET_PATTERN,

    "gimp-dnd-get-pattern-func",
    "gimp-dnd-get-pattern-data",

    "gimp-dnd-set-pattern-func",
    "gimp-dnd-set-pattern-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_object_data,
    gimp_dnd_set_pattern_data
  },

  {
    GIMP_TARGET_GRADIENT,

    "gimp-dnd-get-gradient-func",
    "gimp-dnd-get-gradient-data",

    "gimp-dnd-set-gradient-func",
    "gimp-dnd-set-gradient-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_object_data,
    gimp_dnd_set_gradient_data
  },

  {
    GIMP_TARGET_PALETTE,

    "gimp-dnd-get-palette-func",
    "gimp-dnd-get-palette-data",

    "gimp-dnd-set-palette-func",
    "gimp-dnd-set-palette-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_object_data,
    gimp_dnd_set_palette_data
  },

  {
    GIMP_TARGET_FONT,

    "gimp-dnd-get-font-func",
    "gimp-dnd-get-font-data",

    "gimp-dnd-set-font-func",
    "gimp-dnd-set-font-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_object_data,
    gimp_dnd_set_font_data
  },

  {
    GIMP_TARGET_BUFFER,

    "gimp-dnd-get-buffer-func",
    "gimp-dnd-get-buffer-data",

    "gimp-dnd-set-buffer-func",
    "gimp-dnd-set-buffer-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_object_data,
    gimp_dnd_set_buffer_data
  },

  {
    GIMP_TARGET_IMAGEFILE,

    "gimp-dnd-get-imagefile-func",
    "gimp-dnd-get-imagefile-data",

    "gimp-dnd-set-imagefile-func",
    "gimp-dnd-set-imagefile-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_object_data,
    gimp_dnd_set_imagefile_data
  },

  {
    GIMP_TARGET_TEMPLATE,

    "gimp-dnd-get-template-func",
    "gimp-dnd-get-template-data",

    "gimp-dnd-set-template-func",
    "gimp-dnd-set-template-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_object_data,
    gimp_dnd_set_template_data
  },

  {
    GIMP_TARGET_TOOL_INFO,

    "gimp-dnd-get-tool-info-func",
    "gimp-dnd-get-tool-info-data",

    "gimp-dnd-set-tool-info-func",
    "gimp-dnd-set-tool-info-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_object_data,
    gimp_dnd_set_tool_info_data
  },

  {
    GIMP_TARGET_DIALOG,

    NULL,
    NULL,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL
  }
};


static Gimp *the_dnd_gimp = NULL;


void
gimp_dnd_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (the_dnd_gimp == NULL);

  the_dnd_gimp = gimp;
}


/**********************/
/*  helper functions  */
/**********************/

static void
gimp_dnd_target_list_add (GtkTargetList        *list,
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
gimp_dnd_data_drag_begin (GtkWidget      *widget,
                          GdkDragContext *context,
                          gpointer        data)
{
  const GimpDndDataDef *dnd_data;
  GimpDndType           data_type;
  GCallback             get_data_func = NULL;
  gpointer              get_data_data = NULL;
  GtkWidget            *icon_widget;

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "gimp-dnd-get-data-type"));

  D (g_printerr ("\ngimp_dnd_data_drag_begin (%d)\n", data_type));

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

      g_object_set_data_full (G_OBJECT (widget), "gimp-dnd-data-widget",
                              window, (GDestroyNotify) gtk_widget_destroy);

      gtk_drag_set_icon_widget (context, window,
                                DRAG_ICON_OFFSET, DRAG_ICON_OFFSET);

      /*  remember for which drag context the widget was made  */
      g_object_set_data (G_OBJECT (window), "gimp-gdk-drag-context", context);
    }
}

static void
gimp_dnd_data_drag_end (GtkWidget      *widget,
                        GdkDragContext *context)
{
  GtkWidget *icon_widget;

  D (g_printerr ("\ngimp_dnd_data_drag_end\n"));

  icon_widget = g_object_get_data (G_OBJECT (widget), "gimp-dnd-data-widget");

  if (icon_widget)
    {
      /*  destroy the icon_widget only if it was made for this drag
       *  context. See bug #139337.
       */
      if (g_object_get_data (G_OBJECT (icon_widget),
                             "gimp-gdk-drag-context") ==
          (gpointer) context)
        {
          g_object_set_data (G_OBJECT (widget), "gimp-dnd-data-widget", NULL);
        }
    }
}

static void
gimp_dnd_data_drag_handle (GtkWidget        *widget,
                           GdkDragContext   *context,
                           GtkSelectionData *selection_data,
                           guint             info,
                           guint             time,
                           gpointer          data)
{
  GCallback    get_data_func = NULL;
  gpointer     get_data_data = NULL;
  GimpDndType  data_type;

  D (g_printerr ("\ngimp_dnd_data_drag_handle(%d)\n", info));

  for (data_type = GIMP_DND_TYPE_NONE + 1;
       data_type <= GIMP_DND_TYPE_LAST;
       data_type++)
    {
      const GimpDndDataDef *dnd_data = dnd_data_defs + data_type;

      if (dnd_data->target_entry.info == info)
        {
          D (g_printerr ("gimp_dnd_data_drag_handle(%s)\n",
                         dnd_data->target_entry.target));

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
gimp_dnd_data_drop_handle (GtkWidget        *widget,
                           GdkDragContext   *context,
                           gint              x,
                           gint              y,
                           GtkSelectionData *selection_data,
                           guint             info,
                           guint             time,
                           gpointer          data)
{
  GimpDndType data_type;

  D (g_printerr ("\ngimp_dnd_data_drop_handle(%d)\n", info));

  if (selection_data->length <= 0)
    {
      gtk_drag_finish (context, FALSE, FALSE, time);
      return;
    }

  for (data_type = GIMP_DND_TYPE_NONE + 1;
       data_type <= GIMP_DND_TYPE_LAST;
       data_type++)
    {
      const GimpDndDataDef *dnd_data = dnd_data_defs + data_type;

      if (dnd_data->target_entry.info == info)
        {
          GCallback set_data_func = NULL;
          gpointer  set_data_data = NULL;

          D (g_printerr ("gimp_dnd_data_drop_handle(%s)\n",
                         dnd_data->target_entry.target));

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
gimp_dnd_data_source_add (GimpDndType  data_type,
                          GtkWidget   *widget,
                          GCallback    get_data_func,
                          gpointer     get_data_data)
{
  const GimpDndDataDef *dnd_data;
  gboolean              drag_connected;

  dnd_data = dnd_data_defs + data_type;

  /*  set a default drag source if not already done  */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-site-data"))
    gtk_drag_source_set (widget, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                         NULL, 0,
                         GDK_ACTION_COPY | GDK_ACTION_MOVE);

  drag_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "gimp-dnd-drag-connected"));

  if (! drag_connected)
    {
      g_signal_connect (widget, "drag-begin",
                        G_CALLBACK (gimp_dnd_data_drag_begin),
                        NULL);
      g_signal_connect (widget, "drag-end",
                        G_CALLBACK (gimp_dnd_data_drag_end),
                        NULL);
      g_signal_connect (widget, "drag-data-get",
                        G_CALLBACK (gimp_dnd_data_drag_handle),
                        NULL);

      g_object_set_data (G_OBJECT (widget), "gimp-dnd-drag-connected",
                         GINT_TO_POINTER (TRUE));
    }

  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_func_name,
                     get_data_func);
  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_data_name,
                     get_data_data);

  /*  remember the first set source type for drag view creation  */
  if (! g_object_get_data (G_OBJECT (widget), "gimp-dnd-get-data-type"))
    g_object_set_data (G_OBJECT (widget), "gimp-dnd-get-data-type",
                       GINT_TO_POINTER (data_type));

  if (dnd_data->target_entry.target)
    {
      GtkTargetList *target_list;

      target_list = gtk_drag_source_get_target_list (widget);

      if (target_list)
        {
          gimp_dnd_target_list_add (target_list, &dnd_data->target_entry);
        }
      else
        {
          target_list = gtk_target_list_new (&dnd_data->target_entry, 1);

          gtk_drag_source_set_target_list (widget, target_list);
          gtk_target_list_unref (target_list);
        }
    }
}

static void
gimp_dnd_data_source_remove (GimpDndType  data_type,
                             GtkWidget   *widget)
{
  const GimpDndDataDef *dnd_data;
  gboolean              drag_connected;

  drag_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "gimp-dnd-drag-connected"));

  if (! drag_connected)
    return;

  dnd_data = dnd_data_defs + data_type;

  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_func_name, NULL);
  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_data_name, NULL);

  /*  remove the dnd type remembered for the dnd icon  */
  if (data_type ==
      GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                          "gimp-dnd-get-data-type")))
    g_object_set_data (G_OBJECT (widget), "gimp-dnd-get-data-type", NULL);

  if (dnd_data->target_entry.target)
    {
      GtkTargetList *target_list;

      target_list = gtk_drag_source_get_target_list (widget);

      if (target_list)
        {
          GdkAtom atom = gdk_atom_intern (dnd_data->target_entry.target, TRUE);

          if (atom != GDK_NONE)
            gtk_target_list_remove (target_list, atom);
        }
    }
}

static void
gimp_dnd_data_dest_add (GimpDndType  data_type,
                        GtkWidget   *widget,
                        gpointer     set_data_func,
                        gpointer     set_data_data)
{
  const GimpDndDataDef *dnd_data;
  gboolean              drop_connected;

  /*  set a default drag dest if not already done  */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-drag-dest"))
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);

  drop_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "gimp-dnd-drop-connected"));

  if (set_data_func && ! drop_connected)
    {
      g_signal_connect (widget, "drag-data-received",
                        G_CALLBACK (gimp_dnd_data_drop_handle),
                        NULL);

      g_object_set_data (G_OBJECT (widget), "gimp-dnd-drop-connected",
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
          gimp_dnd_target_list_add (target_list, &dnd_data->target_entry);
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
gimp_dnd_data_dest_remove (GimpDndType  data_type,
                           GtkWidget   *widget)
{
  const GimpDndDataDef *dnd_data;

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
gimp_dnd_get_uri_list_data (GtkWidget        *widget,
                            GdkDragContext   *context,
                            GCallback         get_uri_list_func,
                            gpointer          get_uri_list_data,
                            GtkSelectionData *selection)
{
  GList *uri_list;

  uri_list = (* (GimpDndDragUriListFunc) get_uri_list_func) (widget,
                                                             get_uri_list_data);

  if (uri_list)
    {
      gimp_selection_data_set_uri_list (selection, uri_list);

      g_list_foreach (uri_list, (GFunc) g_free, NULL);
      g_list_free (uri_list);
    }
}

static gboolean
gimp_dnd_set_uri_list_data (GtkWidget        *widget,
                            gint              x,
                            gint              y,
                            GCallback         set_uri_list_func,
                            gpointer          set_uri_list_data,
                            GtkSelectionData *selection)
{
  GList *uri_list = gimp_selection_data_get_uri_list (selection);

  if (! uri_list)
    return FALSE;

  (* (GimpDndDropUriListFunc) set_uri_list_func) (widget, x, y, uri_list,
                                                  set_uri_list_data);

  g_list_foreach (uri_list, (GFunc) g_free, NULL);
  g_list_free (uri_list);

  return TRUE;
}

void
gimp_dnd_uri_list_source_add (GtkWidget              *widget,
                              GimpDndDragUriListFunc  get_uri_list_func,
                              gpointer                data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_add (GIMP_DND_TYPE_URI_LIST, widget,
                            G_CALLBACK (get_uri_list_func),
                            data);
}

void
gimp_dnd_uri_list_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_remove (GIMP_DND_TYPE_URI_LIST, widget);
}

void
gimp_dnd_uri_list_dest_add (GtkWidget              *widget,
                            GimpDndDropUriListFunc  set_uri_list_func,
                            gpointer                data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  /*  Set a default drag dest if not already done. Explicitely set
   *  COPY and MOVE for file drag destinations. Some file managers
   *  such as Konqueror only offer MOVE by default.
   */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-drag-dest"))
    gtk_drag_dest_set (widget,
                       GTK_DEST_DEFAULT_ALL, NULL, 0,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);

  gimp_dnd_data_dest_add (GIMP_DND_TYPE_URI_LIST, widget,
                          G_CALLBACK (set_uri_list_func),
                          data);
  gimp_dnd_data_dest_add (GIMP_DND_TYPE_TEXT_PLAIN, widget,
                          G_CALLBACK (set_uri_list_func),
                          data);
  gimp_dnd_data_dest_add (GIMP_DND_TYPE_NETSCAPE_URL, widget,
                          G_CALLBACK (set_uri_list_func),
                          data);
}

void
gimp_dnd_uri_list_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_URI_LIST, widget);
  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_TEXT_PLAIN, widget);
  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_NETSCAPE_URL, widget);
}


/******************************/
/* Direct Save Protocol (XDS) */
/******************************/

static void
gimp_dnd_get_xds_data (GtkWidget        *widget,
                       GdkDragContext   *context,
                       GCallback         get_image_func,
                       gpointer          get_image_data,
                       GtkSelectionData *selection)
{
  GimpImage   *image;
  GimpContext *gimp_context;

  image = (GimpImage *)
    (* (GimpDndDragViewableFunc) get_image_func) (widget, &gimp_context,
                                                  get_image_data);

  if (image)
    gimp_dnd_xds_save_image (context, image, selection);
}

static void
gimp_dnd_xds_drag_begin (GtkWidget      *widget,
                         GdkDragContext *context)
{
  const GimpDndDataDef *dnd_data = dnd_data_defs + GIMP_DND_TYPE_XDS;
  GCallback             get_data_func;
  gpointer              get_data_data;

  get_data_func = g_object_get_data (G_OBJECT (widget),
                                     dnd_data->get_data_func_name);
  get_data_data = g_object_get_data (G_OBJECT (widget),
                                     dnd_data->get_data_data_name);

  if (get_data_func)
    {
      GimpImage   *image;
      GimpContext *gimp_context;

      image = (GimpImage *)
        (* (GimpDndDragViewableFunc) get_data_func) (widget, &gimp_context,
                                                     get_data_data);

      gimp_dnd_xds_source_set (context, image);
    }
}

static void
gimp_dnd_xds_drag_end (GtkWidget      *widget,
                       GdkDragContext *context)
{
  gimp_dnd_xds_source_set (context, NULL);
}

void
gimp_dnd_xds_source_add (GtkWidget               *widget,
                         GimpDndDragViewableFunc  get_image_func,
                         gpointer                 data)
{
  gulong handler;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_add (GIMP_DND_TYPE_XDS, widget,
                            G_CALLBACK (get_image_func),
                            data);

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "gimp-dnd-xds-drag-begin"));

  if (! handler)
    {
      handler = g_signal_connect (widget, "drag-begin",
                                  G_CALLBACK (gimp_dnd_xds_drag_begin),
                                  NULL);
      g_object_set_data (G_OBJECT (widget), "gimp-dnd-xds-drag-begin",
                         GUINT_TO_POINTER (handler));
    }

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "gimp-dnd-xds-drag-end"));

  if (! handler)
    {
      handler = g_signal_connect (widget, "drag-end",
                                  G_CALLBACK (gimp_dnd_xds_drag_end),
                                  NULL);
      g_object_set_data (G_OBJECT (widget), "gimp-dnd-xds-drag-end",
                         GUINT_TO_POINTER (handler));
    }
}

void
gimp_dnd_xds_source_remove (GtkWidget *widget)
{
  gulong handler;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "gimp-dnd-xds-drag-begin"));
  if (handler)
    {
      g_signal_handler_disconnect (widget, handler);
      g_object_set_data (G_OBJECT (widget), "gimp-dnd-xds-drag-begin", NULL);
    }

  handler = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                 "gimp-dnd-xds-drag-end"));
  if (handler)
    {
      g_signal_handler_disconnect (widget, handler);
      g_object_set_data (G_OBJECT (widget), "gimp-dnd-xds-drag-end", NULL);
    }

  gimp_dnd_data_source_remove (GIMP_DND_TYPE_XDS, widget);
}


/*************************/
/*  color dnd functions  */
/*************************/

static GtkWidget *
gimp_dnd_get_color_icon (GtkWidget *widget,
                         GCallback  get_color_func,
                         gpointer   get_color_data)
{
  GtkWidget *color_area;
  GimpRGB    color;

  (* (GimpDndDragColorFunc) get_color_func) (widget, &color, get_color_data);

  color_area = gimp_color_area_new (&color, GIMP_COLOR_AREA_SMALL_CHECKS, 0);
  gtk_widget_set_size_request (color_area,
                               DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);

  return color_area;
}

static void
gimp_dnd_get_color_data (GtkWidget        *widget,
                         GdkDragContext   *context,
                         GCallback         get_color_func,
                         gpointer          get_color_data,
                         GtkSelectionData *selection)
{
  GimpRGB color;

  (* (GimpDndDragColorFunc) get_color_func) (widget, &color, get_color_data);

  gimp_selection_data_set_color (selection, &color);
}

static gboolean
gimp_dnd_set_color_data (GtkWidget        *widget,
                         gint              x,
                         gint              y,
                         GCallback         set_color_func,
                         gpointer          set_color_data,
                         GtkSelectionData *selection)
{
  GimpRGB color;

  if (! gimp_selection_data_get_color (selection, &color))
    return FALSE;

  (* (GimpDndDropColorFunc) set_color_func) (widget, x, y, &color,
                                             set_color_data);

  return TRUE;
}

void
gimp_dnd_color_source_add (GtkWidget            *widget,
                           GimpDndDragColorFunc  get_color_func,
                           gpointer              data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_add (GIMP_DND_TYPE_COLOR, widget,
                            G_CALLBACK (get_color_func),
                            data);
}

void
gimp_dnd_color_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_remove (GIMP_DND_TYPE_COLOR, widget);
}

void
gimp_dnd_color_dest_add (GtkWidget            *widget,
                         GimpDndDropColorFunc  set_color_func,
                         gpointer              data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_dest_add (GIMP_DND_TYPE_COLOR, widget,
                          G_CALLBACK (set_color_func),
                          data);
}

void
gimp_dnd_color_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_COLOR, widget);
}


/**************************/
/*  stream dnd functions  */
/**************************/

static void
gimp_dnd_get_stream_data (GtkWidget        *widget,
                          GdkDragContext   *context,
                          GCallback         get_stream_func,
                          gpointer          get_stream_data,
                          GtkSelectionData *selection)
{
  guchar *stream;
  gsize   stream_length;

  stream = (* (GimpDndDragStreamFunc) get_stream_func) (widget, &stream_length,
                                                        get_stream_data);

  if (stream)
    {
      gimp_selection_data_set_stream (selection, stream, stream_length);
      g_free (stream);
    }
}

static gboolean
gimp_dnd_set_stream_data (GtkWidget        *widget,
                          gint              x,
                          gint              y,
                          GCallback         set_stream_func,
                          gpointer          set_stream_data,
                          GtkSelectionData *selection)
{
  const guchar *stream;
  gsize         stream_length;

  stream = gimp_selection_data_get_stream (selection, &stream_length);

  if (! stream)
    return FALSE;

  (* (GimpDndDropStreamFunc) set_stream_func) (widget, x, y,
                                               stream, stream_length,
                                               set_stream_data);

  return TRUE;
}

void
gimp_dnd_svg_source_add (GtkWidget             *widget,
                         GimpDndDragStreamFunc  get_svg_func,
                         gpointer               data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_add (GIMP_DND_TYPE_SVG, widget,
                            G_CALLBACK (get_svg_func),
                            data);
  gimp_dnd_data_source_add (GIMP_DND_TYPE_SVG_XML, widget,
                            G_CALLBACK (get_svg_func),
                            data);
}

void
gimp_dnd_svg_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_remove (GIMP_DND_TYPE_SVG, widget);
  gimp_dnd_data_source_remove (GIMP_DND_TYPE_SVG_XML, widget);
}

void
gimp_dnd_svg_dest_add (GtkWidget             *widget,
                       GimpDndDropStreamFunc  set_svg_func,
                       gpointer               data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_dest_add (GIMP_DND_TYPE_SVG, widget,
                          G_CALLBACK (set_svg_func),
                          data);
  gimp_dnd_data_dest_add (GIMP_DND_TYPE_SVG_XML, widget,
                          G_CALLBACK (set_svg_func),
                          data);
}

void
gimp_dnd_svg_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_SVG, widget);
  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_SVG_XML, widget);
}


/**************************/
/*  pixbuf dnd functions  */
/**************************/

static void
gimp_dnd_get_pixbuf_data (GtkWidget        *widget,
                          GdkDragContext   *context,
                          GCallback         get_pixbuf_func,
                          gpointer          get_pixbuf_data,
                          GtkSelectionData *selection)
{
  GdkPixbuf *pixbuf;

  pixbuf = (* (GimpDndDragPixbufFunc) get_pixbuf_func) (widget,
                                                        get_pixbuf_data);

  if (pixbuf)
    {
      gimp_set_busy (the_dnd_gimp);

      gtk_selection_data_set_pixbuf (selection, pixbuf);
      g_object_unref (pixbuf);

      gimp_unset_busy (the_dnd_gimp);
    }
}

static gboolean
gimp_dnd_set_pixbuf_data (GtkWidget        *widget,
                          gint              x,
                          gint              y,
                          GCallback         set_pixbuf_func,
                          gpointer          set_pixbuf_data,
                          GtkSelectionData *selection)
{
  GdkPixbuf *pixbuf;

  gimp_set_busy (the_dnd_gimp);

  pixbuf = gtk_selection_data_get_pixbuf (selection);

  gimp_unset_busy (the_dnd_gimp);

  if (! pixbuf)
    return FALSE;

  (* (GimpDndDropPixbufFunc) set_pixbuf_func) (widget, x, y,
                                               pixbuf,
                                               set_pixbuf_data);

  g_object_unref (pixbuf);

  return TRUE;
}

void
gimp_dnd_pixbuf_source_add (GtkWidget             *widget,
                            GimpDndDragPixbufFunc  get_pixbuf_func,
                            gpointer               data)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_add (GIMP_DND_TYPE_PIXBUF, widget,
                            G_CALLBACK (get_pixbuf_func),
                            data);

  target_list = gtk_drag_source_get_target_list (widget);

  if (target_list)
    gtk_target_list_ref (target_list);
  else
    target_list = gtk_target_list_new (NULL, 0);

  gimp_pixbuf_targets_add (target_list, GIMP_DND_TYPE_PIXBUF, TRUE);

  gtk_drag_source_set_target_list (widget, target_list);
  gtk_target_list_unref (target_list);
}

void
gimp_dnd_pixbuf_source_remove (GtkWidget *widget)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_remove (GIMP_DND_TYPE_PIXBUF, widget);

  target_list = gtk_drag_source_get_target_list (widget);

  if (target_list)
    gimp_pixbuf_targets_remove (target_list);
}

void
gimp_dnd_pixbuf_dest_add (GtkWidget              *widget,
                          GimpDndDropPixbufFunc   set_pixbuf_func,
                          gpointer                data)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_dest_add (GIMP_DND_TYPE_PIXBUF, widget,
                          G_CALLBACK (set_pixbuf_func),
                          data);

  target_list = gtk_drag_dest_get_target_list (widget);

  if (target_list)
    gtk_target_list_ref (target_list);
  else
    target_list = gtk_target_list_new (NULL, 0);

  gimp_pixbuf_targets_add (target_list, GIMP_DND_TYPE_PIXBUF, FALSE);

  gtk_drag_dest_set_target_list (widget, target_list);
  gtk_target_list_unref (target_list);
}

void
gimp_dnd_pixbuf_dest_remove (GtkWidget *widget)
{
  GtkTargetList *target_list;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_PIXBUF, widget);

  target_list = gtk_drag_dest_get_target_list (widget);

  if (target_list)
    gimp_pixbuf_targets_remove (target_list);
}


/*****************************/
/*  component dnd functions  */
/*****************************/

static GtkWidget *
gimp_dnd_get_component_icon (GtkWidget *widget,
                             GCallback  get_comp_func,
                             gpointer   get_comp_data)
{
  GtkWidget       *view;
  GimpImage       *image;
  GimpContext     *context;
  GimpChannelType  channel;

  image = (* (GimpDndDragComponentFunc) get_comp_func) (widget, &context,
                                                        &channel,
                                                        get_comp_data);

  if (! image)
    return NULL;

  view = gimp_view_new (context, GIMP_VIEWABLE (image),
                        DRAG_PREVIEW_SIZE, 0, TRUE);

  GIMP_VIEW_RENDERER_IMAGE (GIMP_VIEW (view)->renderer)->channel = channel;

  return view;
}

static void
gimp_dnd_get_component_data (GtkWidget        *widget,
                             GdkDragContext   *context,
                             GCallback         get_comp_func,
                             gpointer          get_comp_data,
                             GtkSelectionData *selection)
{
  GimpImage       *image;
  GimpContext     *gimp_context;
  GimpChannelType  channel = 0;

  image = (* (GimpDndDragComponentFunc) get_comp_func) (widget, &gimp_context,
                                                        &channel,
                                                        get_comp_data);

  if (image)
    gimp_selection_data_set_component (selection, image, channel);
}

static gboolean
gimp_dnd_set_component_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_comp_func,
                             gpointer          set_comp_data,
                             GtkSelectionData *selection)
{
  GimpImage       *image;
  GimpChannelType  channel = 0;

  image = gimp_selection_data_get_component (selection, the_dnd_gimp,
                                             &channel);

  if (! image)
    return FALSE;

  (* (GimpDndDropComponentFunc) set_comp_func) (widget, x, y,
                                                image, channel,
                                                set_comp_data);

  return TRUE;
}

void
gimp_dnd_component_source_add (GtkWidget                *widget,
                               GimpDndDragComponentFunc  get_comp_func,
                               gpointer                  data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_add (GIMP_DND_TYPE_COMPONENT, widget,
                            G_CALLBACK (get_comp_func),
                            data);
}

void
gimp_dnd_component_source_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_source_remove (GIMP_DND_TYPE_COMPONENT, widget);
}

void
gimp_dnd_component_dest_add (GtkWidget                 *widget,
                             GimpDndDropComponentFunc   set_comp_func,
                             gpointer                   data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_dest_add (GIMP_DND_TYPE_COMPONENT, widget,
                          G_CALLBACK (set_comp_func),
                          data);
}

void
gimp_dnd_component_dest_remove (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_COMPONENT, widget);
}


/*******************************************/
/*  GimpViewable (by GType) dnd functions  */
/*******************************************/

static GtkWidget *
gimp_dnd_get_viewable_icon (GtkWidget *widget,
                            GCallback  get_viewable_func,
                            gpointer   get_viewable_data)
{
  GimpViewable *viewable;
  GimpContext  *context;
  GtkWidget    *view;
  gchar        *desc;

  viewable = (* (GimpDndDragViewableFunc) get_viewable_func) (widget, &context,
                                                              get_viewable_data);

  if (! viewable)
    return NULL;

  view = gimp_view_new (context, viewable,
                        DRAG_PREVIEW_SIZE, 0, TRUE);

  desc = gimp_viewable_get_description (viewable, NULL);

  if (desc)
    {
      GtkWidget *hbox;
      GtkWidget *label;

      hbox = gtk_hbox_new (FALSE, 3);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 3);
      gtk_box_pack_start (GTK_BOX (hbox), view, FALSE, FALSE, 0);
      gtk_widget_show (view);

      label = g_object_new (GTK_TYPE_LABEL,
                            "label",           desc,
                            "xpad",            3,
                            "xalign",          0.0,
                            "yalign",          0.5,
                            "max-width-chars", 30,
                            "ellipsize",       PANGO_ELLIPSIZE_END,
                            NULL);

      g_free (desc);

      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      gtk_widget_show (label);

      return hbox;
    }

  return view;
}

static GimpDndType
gimp_dnd_data_type_get_by_g_type (GType type)
{
  GimpDndType dnd_type = GIMP_DND_TYPE_NONE;

  if (g_type_is_a (type, GIMP_TYPE_IMAGE))
    {
      dnd_type = GIMP_DND_TYPE_IMAGE;
    }
  else if (g_type_is_a (type, GIMP_TYPE_LAYER))
    {
      dnd_type = GIMP_DND_TYPE_LAYER;
    }
  else if (g_type_is_a (type, GIMP_TYPE_LAYER_MASK))
    {
      dnd_type = GIMP_DND_TYPE_LAYER_MASK;
    }
  else if (g_type_is_a (type, GIMP_TYPE_CHANNEL))
    {
      dnd_type = GIMP_DND_TYPE_CHANNEL;
    }
  else if (g_type_is_a (type, GIMP_TYPE_VECTORS))
    {
      dnd_type = GIMP_DND_TYPE_VECTORS;
    }
  else if (g_type_is_a (type, GIMP_TYPE_BRUSH))
    {
      dnd_type = GIMP_DND_TYPE_BRUSH;
    }
  else if (g_type_is_a (type, GIMP_TYPE_PATTERN))
    {
      dnd_type = GIMP_DND_TYPE_PATTERN;
    }
  else if (g_type_is_a (type, GIMP_TYPE_GRADIENT))
    {
      dnd_type = GIMP_DND_TYPE_GRADIENT;
    }
  else if (g_type_is_a (type, GIMP_TYPE_PALETTE))
    {
      dnd_type = GIMP_DND_TYPE_PALETTE;
    }
  else if (g_type_is_a (type, GIMP_TYPE_FONT))
    {
      dnd_type = GIMP_DND_TYPE_FONT;
    }
  else if (g_type_is_a (type, GIMP_TYPE_BUFFER))
    {
      dnd_type = GIMP_DND_TYPE_BUFFER;
    }
  else if (g_type_is_a (type, GIMP_TYPE_IMAGEFILE))
    {
      dnd_type = GIMP_DND_TYPE_IMAGEFILE;
    }
  else if (g_type_is_a (type, GIMP_TYPE_TEMPLATE))
    {
      dnd_type = GIMP_DND_TYPE_TEMPLATE;
    }
  else if (g_type_is_a (type, GIMP_TYPE_TOOL_INFO))
    {
      dnd_type = GIMP_DND_TYPE_TOOL_INFO;
    }

  return dnd_type;
}

gboolean
gimp_dnd_drag_source_set_by_type (GtkWidget       *widget,
                                  GdkModifierType  start_button_mask,
                                  GType            type,
                                  GdkDragAction    actions)
{
  GimpDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = gimp_dnd_data_type_get_by_g_type (type);

  if (dnd_type == GIMP_DND_TYPE_NONE)
    return FALSE;

  gtk_drag_source_set (widget, start_button_mask,
                       &dnd_data_defs[dnd_type].target_entry, 1,
                       actions);

  return TRUE;
}

gboolean
gimp_dnd_drag_dest_set_by_type (GtkWidget       *widget,
                                GtkDestDefaults  flags,
                                GType            type,
                                GdkDragAction    actions)
{
  GimpDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = gimp_dnd_data_type_get_by_g_type (type);

  if (dnd_type == GIMP_DND_TYPE_NONE)
    return FALSE;

  gtk_drag_dest_set (widget, flags,
                     &dnd_data_defs[dnd_type].target_entry, 1,
                     actions);

  return TRUE;
}

gboolean
gimp_dnd_viewable_source_add (GtkWidget               *widget,
                              GType                    type,
                              GimpDndDragViewableFunc  get_viewable_func,
                              gpointer                 data)
{
  GimpDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (get_viewable_func != NULL, FALSE);

  dnd_type = gimp_dnd_data_type_get_by_g_type (type);

  if (dnd_type == GIMP_DND_TYPE_NONE)
    return FALSE;

  gimp_dnd_data_source_add (dnd_type, widget,
                            G_CALLBACK (get_viewable_func),
                            data);

  return TRUE;
}

gboolean
gimp_dnd_viewable_source_remove (GtkWidget *widget,
                                 GType      type)
{
  GimpDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = gimp_dnd_data_type_get_by_g_type (type);

  if (dnd_type == GIMP_DND_TYPE_NONE)
    return FALSE;

  gimp_dnd_data_source_remove (dnd_type, widget);

  return TRUE;
}

gboolean
gimp_dnd_viewable_dest_add (GtkWidget               *widget,
                            GType                    type,
                            GimpDndDropViewableFunc  set_viewable_func,
                            gpointer                 data)
{
  GimpDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = gimp_dnd_data_type_get_by_g_type (type);

  if (dnd_type == GIMP_DND_TYPE_NONE)
    return FALSE;

  gimp_dnd_data_dest_add (dnd_type, widget,
                          G_CALLBACK (set_viewable_func),
                          data);

  return TRUE;
}

gboolean
gimp_dnd_viewable_dest_remove (GtkWidget *widget,
                               GType      type)
{
  GimpDndType dnd_type;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

  dnd_type = gimp_dnd_data_type_get_by_g_type (type);

  if (dnd_type == GIMP_DND_TYPE_NONE)
    return FALSE;

  gimp_dnd_data_dest_remove (dnd_type, widget);

  return TRUE;
}

GimpViewable *
gimp_dnd_get_drag_data (GtkWidget *widget)
{
  const GimpDndDataDef    *dnd_data;
  GimpDndType              data_type;
  GimpDndDragViewableFunc  get_data_func = NULL;
  gpointer                 get_data_data = NULL;
  GimpContext             *context;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "gimp-dnd-get-data-type"));

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

  return (GimpViewable *) (* get_data_func) (widget, &context, get_data_data);

}


/*****************************/
/*  GimpImage dnd functions  */
/*****************************/

static void
gimp_dnd_get_image_data (GtkWidget        *widget,
                         GdkDragContext   *context,
                         GCallback         get_image_func,
                         gpointer          get_image_data,
                         GtkSelectionData *selection)
{
  GimpImage   *image;
  GimpContext *gimp_context;

  image = (GimpImage *)
    (* (GimpDndDragViewableFunc) get_image_func) (widget, &gimp_context,
                                                  get_image_data);

  if (image)
    gimp_selection_data_set_image (selection, image);
}

static gboolean
gimp_dnd_set_image_data (GtkWidget        *widget,
                         gint              x,
                         gint              y,
                         GCallback         set_image_func,
                         gpointer          set_image_data,
                         GtkSelectionData *selection)
{
  GimpImage *image = gimp_selection_data_get_image (selection, the_dnd_gimp);

  if (! image)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_image_func) (widget, x, y,
                                                GIMP_VIEWABLE (image),
                                                set_image_data);

  return TRUE;
}


/****************************/
/*  GimpItem dnd functions  */
/****************************/

static void
gimp_dnd_get_item_data (GtkWidget        *widget,
                        GdkDragContext   *context,
                        GCallback         get_item_func,
                        gpointer          get_item_data,
                        GtkSelectionData *selection)
{
  GimpItem    *item;
  GimpContext *gimp_context;

  item = (GimpItem *)
    (* (GimpDndDragViewableFunc) get_item_func) (widget, &gimp_context,
                                                 get_item_data);

  if (item)
    gimp_selection_data_set_item (selection, item);
}

static gboolean
gimp_dnd_set_item_data (GtkWidget        *widget,
                        gint              x,
                        gint              y,
                        GCallback         set_item_func,
                        gpointer          set_item_data,
                        GtkSelectionData *selection)
{
  GimpItem *item = gimp_selection_data_get_item (selection, the_dnd_gimp);

  if (! item)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_item_func) (widget, x, y,
                                               GIMP_VIEWABLE (item),
                                               set_item_data);

  return TRUE;
}


/******************************/
/*  GimpObject dnd functions  */
/******************************/

static void
gimp_dnd_get_object_data (GtkWidget        *widget,
                          GdkDragContext   *context,
                          GCallback         get_object_func,
                          gpointer          get_object_data,
                          GtkSelectionData *selection)
{
  GimpObject  *object;
  GimpContext *gimp_context;

  object = (GimpObject *)
    (* (GimpDndDragViewableFunc) get_object_func) (widget, &gimp_context,
                                                   get_object_data);

  if (GIMP_IS_OBJECT (object))
    gimp_selection_data_set_object (selection, object);
}


/*****************************/
/*  GimpBrush dnd functions  */
/*****************************/

static gboolean
gimp_dnd_set_brush_data (GtkWidget        *widget,
                         gint              x,
                         gint              y,
                         GCallback         set_brush_func,
                         gpointer          set_brush_data,
                         GtkSelectionData *selection)
{
  GimpBrush *brush = gimp_selection_data_get_brush (selection, the_dnd_gimp);

  if (! brush)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_brush_func) (widget, x, y,
                                                GIMP_VIEWABLE (brush),
                                                set_brush_data);

  return TRUE;
}


/*******************************/
/*  GimpPattern dnd functions  */
/*******************************/

static gboolean
gimp_dnd_set_pattern_data (GtkWidget        *widget,
                           gint              x,
                           gint              y,
                           GCallback         set_pattern_func,
                           gpointer          set_pattern_data,
                           GtkSelectionData *selection)
{
  GimpPattern *pattern = gimp_selection_data_get_pattern (selection,
                                                          the_dnd_gimp);

  if (! pattern)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_pattern_func) (widget, x, y,
                                                  GIMP_VIEWABLE (pattern),
                                                  set_pattern_data);

  return TRUE;
}


/********************************/
/*  GimpGradient dnd functions  */
/********************************/

static gboolean
gimp_dnd_set_gradient_data (GtkWidget        *widget,
                            gint              x,
                            gint              y,
                            GCallback         set_gradient_func,
                            gpointer          set_gradient_data,
                            GtkSelectionData *selection)
{
  GimpGradient *gradient = gimp_selection_data_get_gradient (selection,
                                                             the_dnd_gimp);

  if (! gradient)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_gradient_func) (widget, x, y,
                                                   GIMP_VIEWABLE (gradient),
                                                   set_gradient_data);

  return TRUE;
}


/*******************************/
/*  GimpPalette dnd functions  */
/*******************************/

static gboolean
gimp_dnd_set_palette_data (GtkWidget        *widget,
                           gint              x,
                           gint              y,
                           GCallback         set_palette_func,
                           gpointer          set_palette_data,
                           GtkSelectionData *selection)
{
  GimpPalette *palette = gimp_selection_data_get_palette (selection,
                                                          the_dnd_gimp);

  if (! palette)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_palette_func) (widget, x, y,
                                                  GIMP_VIEWABLE (palette),
                                                  set_palette_data);

  return TRUE;
}


/****************************/
/*  GimpFont dnd functions  */
/****************************/

static gboolean
gimp_dnd_set_font_data (GtkWidget        *widget,
                        gint              x,
                        gint              y,
                        GCallback         set_font_func,
                        gpointer          set_font_data,
                        GtkSelectionData *selection)
{
  GimpFont *font = gimp_selection_data_get_font (selection, the_dnd_gimp);

  if (! font)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_font_func) (widget, x, y,
                                               GIMP_VIEWABLE (font),
                                               set_font_data);

  return TRUE;
}


/******************************/
/*  GimpBuffer dnd functions  */
/******************************/

static gboolean
gimp_dnd_set_buffer_data (GtkWidget        *widget,
                          gint              x,
                          gint              y,
                          GCallback         set_buffer_func,
                          gpointer          set_buffer_data,
                          GtkSelectionData *selection)
{
  GimpBuffer *buffer = gimp_selection_data_get_buffer (selection, the_dnd_gimp);

  if (! buffer)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_buffer_func) (widget, x, y,
                                                 GIMP_VIEWABLE (buffer),
                                                 set_buffer_data);

  return TRUE;
}


/*********************************/
/*  GimpImagefile dnd functions  */
/*********************************/

static gboolean
gimp_dnd_set_imagefile_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_imagefile_func,
                             gpointer          set_imagefile_data,
                             GtkSelectionData *selection)
{
  GimpImagefile *imagefile = gimp_selection_data_get_imagefile (selection,
                                                                the_dnd_gimp);

  if (! imagefile)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_imagefile_func) (widget, x, y,
                                                    GIMP_VIEWABLE (imagefile),
                                                    set_imagefile_data);

  return TRUE;
}


/********************************/
/*  GimpTemplate dnd functions  */
/********************************/

static gboolean
gimp_dnd_set_template_data (GtkWidget        *widget,
                            gint              x,
                            gint              y,
                            GCallback         set_template_func,
                            gpointer          set_template_data,
                            GtkSelectionData *selection)
{
  GimpTemplate *template = gimp_selection_data_get_template (selection,
                                                             the_dnd_gimp);

  if (! template)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_template_func) (widget, x, y,
                                                   GIMP_VIEWABLE (template),
                                                   set_template_data);

  return TRUE;
}


/********************************/
/*  GimpToolInfo dnd functions  */
/********************************/

static gboolean
gimp_dnd_set_tool_info_data (GtkWidget        *widget,
                             gint              x,
                             gint              y,
                             GCallback         set_tool_info_func,
                             gpointer          set_tool_info_data,
                             GtkSelectionData *selection)
{
  GimpToolInfo *tool_info = gimp_selection_data_get_tool_info (selection,
                                                               the_dnd_gimp);

  if (! tool_info)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_tool_info_func) (widget, x, y,
                                                    GIMP_VIEWABLE (tool_info),
                                                    set_tool_info_data);

  return TRUE;
}
