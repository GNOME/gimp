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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"
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

#include "file/file-open.h"
#include "file/file-utils.h"

#include "gimpdnd.h"
#include "gimppreview.h"

#include "gimp-intl.h"


#define DRAG_PREVIEW_SIZE 32
#define DRAG_ICON_OFFSET  -8

/* #define DEBUG_DND */

#ifdef DEBUG_DND
#define D(stmnt) stmnt
#else
#define D(stmnt)
#endif


typedef GtkWidget * (* GimpDndGetIconFunc)  (GtkWidget *widget,
					     GCallback  get_data_func,
					     gpointer   get_data_data);
typedef guchar    * (* GimpDndDragDataFunc) (GtkWidget *widget,
					     GCallback  get_data_func,
					     gpointer   get_data_data,
					     gint      *format,
					     gint      *length);
typedef gboolean    (* GimpDndDropDataFunc) (GtkWidget *widget,
					     GCallback  set_data_func,
					     gpointer   set_data_data,
					     guchar    *vals,
					     gint       format,
					     gint       length);


typedef struct _GimpDndDataDef GimpDndDataDef;

struct _GimpDndDataDef
{
  GtkTargetEntry       target_entry;

  gchar               *get_data_func_name;
  gchar               *get_data_data_name;

  gchar               *set_data_func_name;
  gchar               *set_data_data_name;

  GimpDndGetIconFunc   get_icon_func;
  GimpDndDragDataFunc  get_data_func;
  GimpDndDropDataFunc  set_data_func;
};


static GtkWidget * gimp_dnd_get_viewable_icon  (GtkWidget *widget,
					        GCallback  get_viewable_func,
					        gpointer   get_viewable_data);
static GtkWidget * gimp_dnd_get_color_icon     (GtkWidget *widget,
					        GCallback  get_color_func,
					        gpointer   get_color_data);

static guchar    * gimp_dnd_get_file_data      (GtkWidget *widget,
					        GCallback  get_file_func,
					        gpointer   get_file_data,
					        gint      *format,
					        gint      *length);
static gboolean    gimp_dnd_set_file_data      (GtkWidget *widget,
					        GCallback  set_file_func,
					        gpointer   set_file_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);

static guchar    * gimp_dnd_get_color_data     (GtkWidget *widget,
					        GCallback  get_color_func,
					        gpointer   get_color_data,
					        gint      *format,
					        gint      *length);
static gboolean    gimp_dnd_set_color_data     (GtkWidget *widget,
					        GCallback  set_color_func,
					        gpointer   set_color_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);

static guchar    * gimp_dnd_get_svg_data       (GtkWidget *widget,
					        GCallback  get_svg_func,
					        gpointer   get_svg_data,
					        gint      *format,
					        gint      *length);
static gboolean    gimp_dnd_set_svg_data       (GtkWidget *widget,
					        GCallback  set_svg_func,
					        gpointer   set_svg_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);

static guchar    * gimp_dnd_get_image_data     (GtkWidget *widget,
					        GCallback  get_image_func,
					        gpointer   get_image_data,
					        gint      *format,
					        gint      *length);
static gboolean    gimp_dnd_set_image_data     (GtkWidget *widget,
					        GCallback  set_image_func,
					        gpointer   set_image_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);

static guchar    * gimp_dnd_get_item_data      (GtkWidget *widget,
					        GCallback  get_item_func,
					        gpointer   get_item_data,
					        gint      *format,
					        gint      *length);
static gboolean    gimp_dnd_set_item_data      (GtkWidget *widget,
					        GCallback  set_item_func,
					        gpointer   set_item_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);

static guchar    * gimp_dnd_get_data_data      (GtkWidget *widget,
					        GCallback  get_data_func,
					        gpointer   get_data_data,
					        gint      *format,
					        gint      *length);
static gboolean    gimp_dnd_set_brush_data     (GtkWidget *widget,
					        GCallback  set_brush_func,
					        gpointer   set_brush_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);
static gboolean    gimp_dnd_set_pattern_data   (GtkWidget *widget,
					        GCallback  set_pattern_func,
					        gpointer   set_pattern_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);
static gboolean    gimp_dnd_set_gradient_data  (GtkWidget *widget,
					        GCallback  set_gradient_func,
					        gpointer   set_gradient_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);
static gboolean    gimp_dnd_set_palette_data   (GtkWidget *widget,
					        GCallback  set_palette_func,
					        gpointer   set_palette_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);
static gboolean    gimp_dnd_set_font_data      (GtkWidget *widget,
					        GCallback  set_font_func,
					        gpointer   set_font_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);
static gboolean    gimp_dnd_set_buffer_data    (GtkWidget *widget,
					        GCallback  set_buffer_func,
					        gpointer   set_buffer_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);
static gboolean    gimp_dnd_set_imagefile_data (GtkWidget *widget,
					        GCallback  set_imagefile_func,
					        gpointer   set_imagefile_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);
static gboolean    gimp_dnd_set_template_data  (GtkWidget *widget,
                                                GCallback  set_template_func,
					        gpointer   set_template_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);
static gboolean    gimp_dnd_set_tool_data      (GtkWidget *widget,
					        GCallback  set_tool_func,
					        gpointer   set_tool_data,
					        guchar    *vals,
					        gint       format,
					        gint       length);



static GimpDndDataDef dnd_data_defs[] =
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

    "gimp-dnd-get-file-func",
    "gimp-dnd-get-file-data",

    "gimp-dnd-set-file-func",
    "gimp-dnd-set-file-data",

    NULL,
    gimp_dnd_get_file_data,
    gimp_dnd_set_file_data
  },

  {
    GIMP_TARGET_TEXT_PLAIN,

    NULL,
    NULL,

    "gimp-dnd-set-file-func",
    "gimp-dnd-set-file-data",

    NULL,
    NULL,
    gimp_dnd_set_file_data
  },

  {
    GIMP_TARGET_NETSCAPE_URL,

    NULL,
    NULL,

    "gimp-dnd-set-file-func",
    "gimp-dnd-set-file-data",

    NULL,
    NULL,
    gimp_dnd_set_file_data
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
    gimp_dnd_get_svg_data,
    gimp_dnd_set_svg_data
  },

  {
    GIMP_TARGET_SVG_XML,

    "gimp-dnd-get-svg-xml-func",
    "gimp-dnd-get-svg-xml-data",

    "gimp-dnd-set-svg-xml-func",
    "gimp-dnd-set-svg-xml-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_svg_data,
    gimp_dnd_set_svg_data
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
    GIMP_TARGET_COMPONENT,

    NULL,
    NULL,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
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
    gimp_dnd_get_data_data,
    gimp_dnd_set_brush_data
  },

  {
    GIMP_TARGET_PATTERN,

    "gimp-dnd-get-pattern-func",
    "gimp-dnd-get-pattern-data",

    "gimp-dnd-set-pattern-func",
    "gimp-dnd-set-pattern-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_data_data,
    gimp_dnd_set_pattern_data
  },

  {
    GIMP_TARGET_GRADIENT,

    "gimp-dnd-get-gradient-func",
    "gimp-dnd-get-gradient-data",

    "gimp-dnd-set-gradient-func",
    "gimp-dnd-set-gradient-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_data_data,
    gimp_dnd_set_gradient_data
  },

  {
    GIMP_TARGET_PALETTE,

    "gimp-dnd-get-palette-func",
    "gimp-dnd-get-palette-data",

    "gimp-dnd-set-palette-func",
    "gimp-dnd-set-palette-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_data_data,
    gimp_dnd_set_palette_data
  },

  {
    GIMP_TARGET_FONT,

    "gimp-dnd-get-font-func",
    "gimp-dnd-get-font-data",

    "gimp-dnd-set-font-func",
    "gimp-dnd-set-font-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_data_data,
    gimp_dnd_set_font_data
  },

  {
    GIMP_TARGET_BUFFER,

    "gimp-dnd-get-buffer-func",
    "gimp-dnd-get-buffer-data",

    "gimp-dnd-set-buffer-func",
    "gimp-dnd-set-buffer-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_data_data,
    gimp_dnd_set_buffer_data
  },

  {
    GIMP_TARGET_IMAGEFILE,

    "gimp-dnd-get-imagefile-func",
    "gimp-dnd-get-imagefile-data",

    "gimp-dnd-set-imagefile-func",
    "gimp-dnd-set-imagefile-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_data_data,
    gimp_dnd_set_imagefile_data
  },

  {
    GIMP_TARGET_TEMPLATE,

    "gimp-dnd-get-template-func",
    "gimp-dnd-get-template-data",

    "gimp-dnd-set-template-func",
    "gimp-dnd-set-template-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_data_data,
    gimp_dnd_set_template_data
  },

  {
    GIMP_TARGET_TOOL,

    "gimp-dnd-get-tool-func",
    "gimp-dnd-get-tool-data",

    "gimp-dnd-set-tool-func",
    "gimp-dnd-set-tool-data",

    gimp_dnd_get_viewable_icon,
    gimp_dnd_get_data_data,
    gimp_dnd_set_tool_data
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


/********************************/
/*  general data dnd functions  */
/********************************/

static void
gimp_dnd_data_drag_begin (GtkWidget      *widget,
			  GdkDragContext *context,
			  gpointer        data)
{
  GimpDndType     data_type;
  GimpDndDataDef *dnd_data;
  GCallback       get_data_func = NULL;
  gpointer        get_data_data = NULL;
  GtkWidget      *icon_widget;

  data_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "gimp-dnd-get-data-type"));

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
    }
}

static void
gimp_dnd_data_drag_end (GtkWidget      *widget,
			GdkDragContext *context)
{
  g_object_set_data (G_OBJECT (widget), "gimp-dnd-data-widget", NULL);
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

  D (g_print ("\ngimp_dnd_data_drag_handle(%d)\n", info));

  for (data_type = GIMP_DND_TYPE_NONE + 1;
       data_type <= GIMP_DND_TYPE_LAST;
       data_type++)
    {
      GimpDndDataDef *dnd_data = dnd_data_defs + data_type;

      if (dnd_data->target_entry.info == info)
	{
          gint    format;
          guchar *vals;
          gint    length;

          D (g_print ("gimp_dnd_data_drag_handle(%s)\n",
                      dnd_data->target_entry.target));

          if (dnd_data->get_data_func_name)
            get_data_func = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->get_data_func_name);

          if (dnd_data->get_data_data_name)
            get_data_data = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->get_data_data_name);

          if (! get_data_func)
            return;

          vals = dnd_data->get_data_func (widget,
                                          get_data_func,
                                          get_data_data,
                                          &format,
                                          &length);

          if (vals)
            {
              GdkAtom atom;

              atom = gdk_atom_intern (dnd_data->target_entry.target, FALSE);

              gtk_selection_data_set (selection_data, atom,
                                      format, vals, length);
              g_free (vals);
            }

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

  D (g_print ("\ngimp_dnd_data_drop_handle(%d)\n", info));

  if (selection_data->length <= 0)
    {
      gtk_drag_finish (context, FALSE, FALSE, time);
      return;
    }

  for (data_type = GIMP_DND_TYPE_NONE + 1;
       data_type <= GIMP_DND_TYPE_LAST;
       data_type++)
    {
      GimpDndDataDef *dnd_data = dnd_data_defs + data_type;

      if (dnd_data->target_entry.info == info)
	{
          GCallback set_data_func = NULL;
          gpointer  set_data_data = NULL;

          D (g_print ("gimp_dnd_data_drop_handle(%s)\n",
                      dnd_data->target_entry.target));

          if (dnd_data->set_data_func_name)
            set_data_func = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->set_data_func_name);

          if (dnd_data->set_data_data_name)
            set_data_data = g_object_get_data (G_OBJECT (widget),
                                               dnd_data->set_data_data_name);

	  if (set_data_func &&
              dnd_data->set_data_func (widget,
                                       set_data_func,
                                       set_data_data,
                                       selection_data->data,
                                       selection_data->format,
                                       selection_data->length))
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
  GimpDndDataDef *dnd_data;
  GtkTargetList  *target_list;
  gboolean        drag_connected;

  dnd_data = dnd_data_defs + data_type;

  /*  set a default drag source if not already done  */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-site-data"))
    gtk_drag_source_set (widget, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                         &dnd_data->target_entry, 1,
                         GDK_ACTION_COPY | GDK_ACTION_MOVE);

  drag_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "gimp-dnd-drag-connected"));

  if (! drag_connected)
    {
      g_signal_connect (widget, "drag_begin",
                        G_CALLBACK (gimp_dnd_data_drag_begin),
                        NULL);
      g_signal_connect (widget, "drag_end",
                        G_CALLBACK (gimp_dnd_data_drag_end),
                        NULL);
      g_signal_connect (widget, "drag_data_get",
                        G_CALLBACK (gimp_dnd_data_drag_handle),
                        NULL);

      g_object_set_data (G_OBJECT (widget), "gimp-dnd-drag-connected",
                         GINT_TO_POINTER (TRUE));
    }

  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_func_name,
                     get_data_func);
  g_object_set_data (G_OBJECT (widget), dnd_data->get_data_data_name,
                     get_data_data);

  /*  remember the first set source type for drag preview creation  */
  if (! g_object_get_data (G_OBJECT (widget), "gimp-dnd-get-data-type"))
    g_object_set_data (G_OBJECT (widget), "gimp-dnd-get-data-type",
                       GINT_TO_POINTER (data_type));

  target_list = gtk_drag_source_get_target_list (widget);

  if (target_list)
    {
      gtk_target_list_add_table (target_list, &dnd_data->target_entry, 1);
    }
  else
    {
      target_list = gtk_target_list_new (&dnd_data->target_entry, 1);

      gtk_drag_source_set_target_list (widget, target_list);
      gtk_target_list_unref (target_list);
    }
}

static void
gimp_dnd_data_source_remove (GimpDndType  data_type,
                             GtkWidget   *widget)
{
  GimpDndDataDef *dnd_data;
  GtkTargetList  *target_list;
  gboolean        drag_connected;

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

  target_list = gtk_drag_source_get_target_list (widget);

  if (target_list)
    {
      GdkAtom atom;

      atom = gdk_atom_intern (dnd_data->target_entry.target, TRUE);

      if (atom != GDK_NONE)
        gtk_target_list_remove (target_list, atom);
    }
}

static void
gimp_dnd_data_dest_add (GimpDndType  data_type,
			GtkWidget   *widget,
			gpointer     set_data_func,
			gpointer     set_data_data)
{
  GimpDndDataDef *dnd_data;
  GtkTargetList  *target_list;
  gboolean        drop_connected;

  /*  set a default drag dest if not already done  */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-drag-dest"))
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY);

  drop_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "gimp-dnd-drop-connected"));

  if (! drop_connected)
    {
      g_signal_connect (widget, "drag_data_received",
                        G_CALLBACK (gimp_dnd_data_drop_handle),
                        NULL);

      g_object_set_data (G_OBJECT (widget), "gimp-dnd-drop-connected",
                         GINT_TO_POINTER (TRUE));
    }

  dnd_data = dnd_data_defs + data_type;

  g_object_set_data (G_OBJECT (widget), dnd_data->set_data_func_name,
                     set_data_func);
  g_object_set_data (G_OBJECT (widget), dnd_data->set_data_data_name,
                     set_data_data);

  target_list = gtk_drag_dest_get_target_list (widget);

  if (target_list)
    {
      gtk_target_list_add_table (target_list, &dnd_data->target_entry, 1);
    }
  else
    {
      target_list = gtk_target_list_new (&dnd_data->target_entry, 1);

      gtk_drag_dest_set_target_list (widget, target_list);
      gtk_target_list_unref (target_list);
    }
}

static void
gimp_dnd_data_dest_remove (GimpDndType  data_type,
                           GtkWidget   *widget)
{
  GimpDndDataDef *dnd_data;
  GtkTargetList  *target_list;
  gboolean        drop_connected;

  drop_connected =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                        "gimp-dnd-drop-connected"));

  if (! drop_connected)
    return;

  dnd_data = dnd_data_defs + data_type;

  g_object_set_data (G_OBJECT (widget), dnd_data->set_data_func_name, NULL);
  g_object_set_data (G_OBJECT (widget), dnd_data->set_data_data_name, NULL);

  target_list = gtk_drag_dest_get_target_list (widget);

  if (target_list)
    {
      GdkAtom atom;

      atom = gdk_atom_intern (dnd_data->target_entry.target, TRUE);

      if (atom != GDK_NONE)
        gtk_target_list_remove (target_list, atom);
    }
}


/************************/
/*  file dnd functions  */
/************************/

static guchar *
gimp_dnd_get_file_data (GtkWidget *widget,
                        GCallback  get_file_func,
                        gpointer   get_file_data,
                        gint      *format,
                        gint      *length)
{
  GList *file_list;
  GList *list;
  gchar *vals = NULL;

  file_list = (* (GimpDndDragFileFunc) get_file_func) (widget, get_file_data);

  if (! file_list)
    return NULL;

  for (list = file_list; list; list = g_list_next (list))
    {
      if (vals)
        {
          gchar *tmp = g_strconcat (vals,
                                    list->data,
                                    list->next ? "\n" : NULL,
                                    NULL);
          g_free (vals);
          vals = tmp;
        }
      else
        {
          vals = g_strconcat (list->data,
                              list->next ? "\n" : NULL,
                              NULL);
        }
    }

  g_list_free (file_list);

  *format = 8;
  *length = strlen (vals) + 1;

  return (guchar *) vals;
}

static gboolean
gimp_dnd_set_file_data (GtkWidget *widget,
			GCallback  set_file_func,
			gpointer   set_file_data,
			guchar    *vals,
			gint       format,
			gint       length)
{
  GList *files = NULL;
  gchar *buffer;

  if (format != 8)
    {
      g_warning ("Received invalid file data!");
      return FALSE;
    }

  buffer = (gchar *) vals;

  D (g_print ("gimp_dnd_set_file_data: raw buffer >>%s<<\n", buffer));

  {
    gchar name_buffer[1024];

    while (*buffer && (buffer - (gchar *) vals < length))
      {
	gchar *name = name_buffer;
	gint   len  = 0;

	while (len < sizeof (name_buffer) && *buffer && *buffer != '\n')
	  {
	    *name++ = *buffer++;
	    len++;
	  }
	if (len == 0)
	  break;

	if (*(name - 1) == 0xd)   /* gmc uses RETURN+NEWLINE as delimiter */
	  len--;

	if (len > 2)
	  files = g_list_append (files, g_strndup (name_buffer, len));

	if (*buffer)
	  buffer++;
      }
  }

  if (! files)
    return FALSE;

  (* (GimpDndDropFileFunc) set_file_func) (widget, files, set_file_data);

  g_list_foreach (files, (GFunc) g_free, NULL);
  g_list_free (files);

  return TRUE;
}

void
gimp_dnd_file_source_add (GtkWidget           *widget,
                          GimpDndDragFileFunc  get_file_func,
                          gpointer             data)
{
  gimp_dnd_data_source_add (GIMP_DND_TYPE_URI_LIST, widget,
			    G_CALLBACK (get_file_func),
			    data);
}

void
gimp_dnd_file_source_remove (GtkWidget *widget)
{
  gimp_dnd_data_source_remove (GIMP_DND_TYPE_URI_LIST, widget);
}

void
gimp_dnd_file_dest_add (GtkWidget           *widget,
			GimpDndDropFileFunc  set_file_func,
			gpointer             data)
{
  /*  Set a default drag dest if not already done. Explicitely set
   *  COPY and MOVE for file drag destinations. Some file managers
   *  such as Konqueror only offer MOVE by default.
   */
  if (! g_object_get_data (G_OBJECT (widget), "gtk-drag-dest"))
    gtk_drag_dest_set (widget,
                       GTK_DEST_DEFAULT_ALL, NULL, 0,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);

  gimp_dnd_data_dest_add (GIMP_DND_TYPE_NETSCAPE_URL, widget,
			  G_CALLBACK (set_file_func),
			  data);
  gimp_dnd_data_dest_add (GIMP_DND_TYPE_TEXT_PLAIN, widget,
			  G_CALLBACK (set_file_func),
			  data);
  gimp_dnd_data_dest_add (GIMP_DND_TYPE_URI_LIST, widget,
			  G_CALLBACK (set_file_func),
			  data);
}

void
gimp_dnd_file_dest_remove (GtkWidget *widget)
{
  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_URI_LIST, widget);
  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_TEXT_PLAIN, widget);
  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_NETSCAPE_URL, widget);
}

/*  the next two functions are straight cut'n'paste from glib/glib/gconvert.c,
 *  except that gimp_unescape_uri_string() does not try to UTF-8 validate
 *  the unescaped result.
 */
static int
unescape_character (const char *scanner)
{
  int first_digit;
  int second_digit;

  first_digit = g_ascii_xdigit_value (scanner[0]);
  if (first_digit < 0)
    return -1;

  second_digit = g_ascii_xdigit_value (scanner[1]);
  if (second_digit < 0)
    return -1;

  return (first_digit << 4) | second_digit;
}

static gchar *
gimp_unescape_uri_string (const char *escaped,
                          int         len,
                          const char *illegal_escaped_characters,
                          gboolean    ascii_must_not_be_escaped)
{
  const gchar *in, *in_end;
  gchar *out, *result;
  int c;

  if (escaped == NULL)
    return NULL;

  if (len < 0)
    len = strlen (escaped);

  result = g_malloc (len + 1);

  out = result;
  for (in = escaped, in_end = escaped + len; in < in_end; in++)
    {
      c = *in;

      if (c == '%')
        {
          /* catch partial escape sequences past the end of the substring */
          if (in + 3 > in_end)
            break;

          c = unescape_character (in + 1);

          /* catch bad escape sequences and NUL characters */
          if (c <= 0)
            break;

          /* catch escaped ASCII */
          if (ascii_must_not_be_escaped && c <= 0x7F)
            break;

          /* catch other illegal escaped characters */
          if (strchr (illegal_escaped_characters, c) != NULL)
            break;

          in += 2;
        }

      *out++ = c;
    }

  g_assert (out - result <= len);
  *out = '\0';

  if (in != in_end)
    {
      g_free (result);
      return NULL;
    }

  return result;
}

void
gimp_dnd_open_files (GtkWidget *widget,
		     GList     *files,
		     gpointer   data)
{
  GList *list;

  for (list = files; list; list = g_list_next (list))
    {
      const gchar *dnd_crap = list->data;
      gchar       *filename;
      gchar       *uri   = NULL;
      GError      *error = NULL;

      if (!dnd_crap)
        continue;

      D (g_print ("gimp_dnd_open_files: trying to convert "
                  "\"%s\" to an uri.\n", dnd_crap));

      filename = g_filename_from_uri (dnd_crap, NULL, NULL);

      if (filename)
        {
          /*  if we got a correctly encoded "file:" uri...  */

          uri = g_filename_to_uri (filename, NULL, NULL);

          g_free (filename);
        }
      else if (g_file_test (dnd_crap, G_FILE_TEST_EXISTS))
        {
          /*  ...else if we got a valid local filename...  */

          uri = g_filename_to_uri (dnd_crap, NULL, NULL);
        }
      else
        {
          /*  ...otherwise do evil things...  */

          const gchar *start = dnd_crap;

          if (! strncmp (dnd_crap, "file://", strlen ("file://")))
            {
              start += strlen ("file://");
            }
          else if (! strncmp (dnd_crap, "file:", strlen ("file:")))
            {
              start += strlen ("file:");
            }

          if (start != dnd_crap)
            {
              /*  try if we got a "file:" uri in the local filename encoding  */
              gchar  *unescaped_filename;

              if (strstr (dnd_crap, "%"))
                {
                  unescaped_filename = gimp_unescape_uri_string (start, -1,
                                                                 "/", FALSE);
                }
              else
                {
                  unescaped_filename = g_strdup (start);
                }

              uri = g_filename_to_uri (unescaped_filename, NULL, &error);

              if (! uri)
                {
                  gchar *escaped_filename = g_strescape (unescaped_filename,
                                                         NULL);

                  g_message (_("The filename '%s' couldn't be converted to a "
                               "valid URI:\n\n%s"),
                             escaped_filename,
                             error->message ? error->message
                                            : _("Invalid UTF-8"));
                  g_free (escaped_filename);
                  g_clear_error (&error);

                  g_free (unescaped_filename);
                  continue;
                }

              g_free (unescaped_filename);
            }
          else
            {
              /*  otherwise try the crap passed anyway, in case it's
               *  a "http:" or whatever uri a plug-in might handle
               */
              uri = g_strdup (dnd_crap);
            }
        }

      D (g_print ("gimp_dnd_open_files: ...trying to open "
                  "resulting uri \"%s\"\n", uri));

      {
        GimpImage         *gimage;
        GimpPDBStatusType  status;

        gimage = file_open_with_display (the_dnd_gimp,
                                         gimp_get_user_context (the_dnd_gimp),
                                         uri, &status, &error);

        if (! gimage && status != GIMP_PDB_CANCEL)
          {
            filename = file_utils_uri_to_utf8_filename (uri);

            g_message (_("Opening '%s' failed:\n\n%s"),
                       filename, error->message);

            g_clear_error (&error);
            g_free (filename);
          }
      }

      g_free (uri);
    }
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

  (* (GimpDndDragColorFunc) get_color_func) (widget, &color,
					     get_color_data);

  color_area = gimp_color_area_new (&color, TRUE, 0);
  gtk_widget_set_size_request (color_area, DRAG_PREVIEW_SIZE, DRAG_PREVIEW_SIZE);

  return color_area;
}

static guchar *
gimp_dnd_get_color_data (GtkWidget *widget,
			 GCallback  get_color_func,
			 gpointer   get_color_data,
			 gint      *format,
			 gint      *length)
{
  guint16 *vals;
  GimpRGB  color;
  guchar   r, g, b, a;

  (* (GimpDndDragColorFunc) get_color_func) (widget, &color,
					     get_color_data);

  vals = g_new (guint16, 4);

  gimp_rgba_get_uchar (&color, &r, &g, &b, &a);

  vals[0] = r + (r << 8);
  vals[1] = g + (g << 8);
  vals[2] = b + (b << 8);
  vals[3] = a + (a << 8);

  *format = 16;
  *length = 8;

  return (guchar *) vals;
}

static gboolean
gimp_dnd_set_color_data (GtkWidget *widget,
			 GCallback  set_color_func,
			 gpointer   set_color_data,
			 guchar    *vals,
			 gint       format,
			 gint       length)
{
  guint16 *color_vals;
  GimpRGB  color;

  if ((format != 16) || (length != 8))
    {
      g_warning ("Received invalid color data!");
      return FALSE;
    }

  color_vals = (guint16 *) vals;

  gimp_rgba_set_uchar (&color,
		       (guchar) (color_vals[0] >> 8),
		       (guchar) (color_vals[1] >> 8),
		       (guchar) (color_vals[2] >> 8),
		       (guchar) (color_vals[3] >> 8));

  (* (GimpDndDropColorFunc) set_color_func) (widget, &color,
					     set_color_data);

  return TRUE;
}

void
gimp_dnd_color_source_add (GtkWidget            *widget,
			   GimpDndDragColorFunc  get_color_func,
			   gpointer              data)
{
  gimp_dnd_data_source_add (GIMP_DND_TYPE_COLOR, widget,
			    G_CALLBACK (get_color_func),
			    data);
}

void
gimp_dnd_color_source_remove (GtkWidget *widget)
{
  gimp_dnd_data_source_remove (GIMP_DND_TYPE_COLOR, widget);
}

void
gimp_dnd_color_dest_add (GtkWidget            *widget,
			 GimpDndDropColorFunc  set_color_func,
			 gpointer              data)
{
  gimp_dnd_data_dest_add (GIMP_DND_TYPE_COLOR, widget,
			  G_CALLBACK (set_color_func),
			  data);
}

void
gimp_dnd_color_dest_remove (GtkWidget *widget)
{
  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_COLOR, widget);
}


/***********************/
/*  svg dnd functions  */
/***********************/

static guchar *
gimp_dnd_get_svg_data (GtkWidget *widget,
                       GCallback  get_svg_func,
                       gpointer   get_svg_data,
                       gint      *format,
                       gint      *length)
{
  gchar *svg_data;
  gint   svg_data_length;

  svg_data = (* (GimpDndDragSvgFunc) get_svg_func) (widget, &svg_data_length,
                                                    get_svg_data);

  *format = 8;
  *length = svg_data_length;

  return (guchar *) svg_data;
}

static gboolean
gimp_dnd_set_svg_data (GtkWidget *widget,
                       GCallback  set_svg_func,
                       gpointer   set_svg_data,
                       guchar    *vals,
                       gint       format,
                       gint       length)
{
  if (format != 8)
    {
      g_warning ("Received invalid SVG data!");
      return FALSE;
    }

  (* (GimpDndDropSvgFunc) set_svg_func) (widget, (gchar *) vals, length,
                                         set_svg_data);

  return TRUE;
}

void
gimp_dnd_svg_source_add (GtkWidget          *widget,
                         GimpDndDragSvgFunc  get_svg_func,
                         gpointer            data)
{
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
  gimp_dnd_data_source_remove (GIMP_DND_TYPE_SVG, widget);
  gimp_dnd_data_source_remove (GIMP_DND_TYPE_SVG_XML, widget);
}

void
gimp_dnd_svg_dest_add (GtkWidget          *widget,
                       GimpDndDropSvgFunc  set_svg_func,
                       gpointer            data)
{
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
  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_SVG, widget);
  gimp_dnd_data_dest_remove (GIMP_DND_TYPE_SVG_XML, widget);
}


/*******************************************/
/*  GimpViewable (by GType) dnd functions  */
/*******************************************/

static GtkWidget *
gimp_dnd_get_viewable_icon (GtkWidget *widget,
			    GCallback  get_viewable_func,
			    gpointer   get_viewable_data)
{
  GtkWidget    *preview;
  GimpViewable *viewable;

  viewable = (* (GimpDndDragViewableFunc) get_viewable_func) (widget,
							      get_viewable_data);

  if (! viewable)
    return NULL;

  preview = gimp_preview_new (viewable, DRAG_PREVIEW_SIZE, 0, TRUE);

  return preview;
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
      dnd_type = GIMP_DND_TYPE_TOOL;
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
  g_return_val_if_fail (set_viewable_func != NULL, FALSE);

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
  GimpDndType              data_type;
  GimpDndDataDef          *dnd_data;
  GimpDndDragViewableFunc  get_data_func = NULL;
  gpointer                 get_data_data = NULL;

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

  return (GimpViewable *) (* get_data_func) (widget, get_data_data);

}


/*************************/
/*  image dnd functions  */
/*************************/

static guchar *
gimp_dnd_get_image_data (GtkWidget *widget,
			 GCallback  get_image_func,
			 gpointer   get_image_data,
			 gint      *format,
			 gint      *length)
{
  GimpImage *gimage;
  gchar     *id;

  gimage = (GimpImage *)
    (* (GimpDndDragViewableFunc) get_image_func) (widget, get_image_data);

  if (! gimage)
    return NULL;

  id = g_strdup_printf ("%d", gimp_image_get_ID (gimage));

  *format = 8;
  *length = strlen (id) + 1;

  return (guchar *) id;
}

static gboolean
gimp_dnd_set_image_data (GtkWidget *widget,
			 GCallback  set_image_func,
			 gpointer   set_image_data,
			 guchar    *vals,
			 gint       format,
			 gint       length)
{
  GimpImage *gimage;
  gchar     *id;
  gint       ID;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid image ID data!");
      return FALSE;
    }

  id = (gchar *) vals;
  ID = atoi (id);

  if (! ID)
    return FALSE;

  gimage = gimp_image_get_by_ID (the_dnd_gimp, ID);

  if (! gimage)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_image_func) (widget,
                                                GIMP_VIEWABLE (gimage),
                                                set_image_data);

  return TRUE;
}


/************************/
/*  item dnd functions  */
/************************/

static guchar *
gimp_dnd_get_item_data (GtkWidget *widget,
                        GCallback  get_item_func,
                        gpointer   get_item_data,
                        gint      *format,
                        gint      *length)
{
  GimpItem *item;
  gchar    *id;

  item = (GimpItem *)
    (* (GimpDndDragViewableFunc) get_item_func) (widget, get_item_data);

  if (! item)
    return NULL;

  id = g_strdup_printf ("%d", gimp_item_get_ID (item));

  *format = 8;
  *length = strlen (id) + 1;

  return (guchar *) id;
}

static gboolean
gimp_dnd_set_item_data (GtkWidget *widget,
                        GCallback  set_item_func,
                        gpointer   set_item_data,
                        guchar    *vals,
                        gint       format,
                        gint       length)
{
  GimpItem *item;
  gchar    *id;
  gint      ID;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid item ID data!");
      return FALSE;
    }

  id = (gchar *) vals;
  ID = atoi (id);

  if (! ID)
    return FALSE;

  item = gimp_item_get_by_ID (the_dnd_gimp, ID);

  if (! item)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_item_func) (widget,
                                               GIMP_VIEWABLE (item),
                                               set_item_data);

  return TRUE;
}


/****************************/
/*  GimpData dnd functions  */
/****************************/

static guchar *
gimp_dnd_get_data_data (GtkWidget *widget,
			GCallback  get_data_func,
			gpointer   get_data_data,
			gint      *format,
			gint      *length)
{
  GimpData *data;
  gchar    *name;

  data = (GimpData *)
    (* (GimpDndDragViewableFunc) get_data_func) (widget, get_data_data);

  if (! data)
    return NULL;

  name = g_strdup (gimp_object_get_name (GIMP_OBJECT (data)));

  if (! name)
    return NULL;

  *format = 8;
  *length = strlen (name) + 1;

  return (guchar *) name;
}


/*************************/
/*  brush dnd functions  */
/*************************/

static gboolean
gimp_dnd_set_brush_data (GtkWidget *widget,
			 GCallback  set_brush_func,
			 gpointer   set_brush_data,
			 guchar    *vals,
			 gint       format,
			 gint       length)
{
  GimpBrush *brush;
  gchar     *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid brush data!");
      return FALSE;
    }

  name = (gchar *) vals;

  if (strcmp (name, "Standard") == 0)
    brush = GIMP_BRUSH (gimp_brush_get_standard ());
  else
    brush = (GimpBrush *)
      gimp_container_get_child_by_name (the_dnd_gimp->brush_factory->container,
					name);

  if (! brush)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_brush_func) (widget,
                                                GIMP_VIEWABLE (brush),
                                                set_brush_data);

  return TRUE;
}


/***************************/
/*  pattern dnd functions  */
/***************************/

static gboolean
gimp_dnd_set_pattern_data (GtkWidget *widget,
			   GCallback  set_pattern_func,
			   gpointer   set_pattern_data,
			   guchar    *vals,
			   gint       format,
			   gint       length)
{
  GimpPattern *pattern;
  gchar       *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid pattern data!");
      return FALSE;
    }

  name = (gchar *) vals;

  if (strcmp (name, "Standard") == 0)
    pattern = GIMP_PATTERN (gimp_pattern_get_standard ());
  else
    pattern = (GimpPattern *)
      gimp_container_get_child_by_name (the_dnd_gimp->pattern_factory->container,
					name);

  if (! pattern)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_pattern_func) (widget,
                                                  GIMP_VIEWABLE (pattern),
                                                  set_pattern_data);

  return TRUE;
}


/****************************/
/*  gradient dnd functions  */
/****************************/

static gboolean
gimp_dnd_set_gradient_data (GtkWidget *widget,
			    GCallback  set_gradient_func,
			    gpointer   set_gradient_data,
			    guchar    *vals,
			    gint       format,
			    gint       length)
{
  GimpGradient *gradient;
  gchar        *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid gradient data!");
      return FALSE;
    }

  name = (gchar *) vals;

  if (strcmp (name, "Standard") == 0)
    gradient = GIMP_GRADIENT (gimp_gradient_get_standard ());
  else
    gradient = (GimpGradient *)
      gimp_container_get_child_by_name (the_dnd_gimp->gradient_factory->container,
					name);

  if (! gradient)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_gradient_func) (widget,
                                                   GIMP_VIEWABLE (gradient),
                                                   set_gradient_data);

  return TRUE;
}


/***************************/
/*  palette dnd functions  */
/***************************/

static gboolean
gimp_dnd_set_palette_data (GtkWidget *widget,
			   GCallback  set_palette_func,
			   gpointer   set_palette_data,
			   guchar    *vals,
			   gint       format,
			   gint       length)
{
  GimpPalette *palette;
  gchar       *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid palette data!");
      return FALSE;
    }

  name = (gchar *) vals;

  if (strcmp (name, "Standard") == 0)
    palette = GIMP_PALETTE (gimp_palette_get_standard ());
  else
    palette = (GimpPalette *)
      gimp_container_get_child_by_name (the_dnd_gimp->palette_factory->container,
					name);

  if (! palette)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_palette_func) (widget,
                                                  GIMP_VIEWABLE (palette),
                                                  set_palette_data);

  return TRUE;
}


/************************/
/*  font dnd functions  */
/************************/

static gboolean
gimp_dnd_set_font_data (GtkWidget *widget,
                        GCallback  set_font_func,
                        gpointer   set_font_data,
                        guchar    *vals,
                        gint       format,
                        gint       length)
{
  GimpFont *font;
  gchar    *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid font data!");
      return FALSE;
    }

  name = (gchar *) vals;

  if (strcmp (name, "Standard") == 0)
    font = gimp_font_get_standard ();
  else
    font = (GimpFont *)
      gimp_container_get_child_by_name (the_dnd_gimp->fonts, name);

  if (! font)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_font_func) (widget,
                                               GIMP_VIEWABLE (font),
                                               set_font_data);

  return TRUE;
}


/**************************/
/*  buffer dnd functions  */
/**************************/

static gboolean
gimp_dnd_set_buffer_data (GtkWidget *widget,
			  GCallback  set_buffer_func,
			  gpointer   set_buffer_data,
			  guchar    *vals,
			  gint       format,
			  gint       length)
{
  GimpBuffer *buffer;
  gchar      *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid buffer data!");
      return FALSE;
    }

  name = (gchar *) vals;

  buffer = (GimpBuffer *)
    gimp_container_get_child_by_name (the_dnd_gimp->named_buffers, name);

  if (! buffer)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_buffer_func) (widget,
                                                 GIMP_VIEWABLE (buffer),
                                                 set_buffer_data);

  return TRUE;
}


/*****************************/
/*  imagefile dnd functions  */
/*****************************/

static gboolean
gimp_dnd_set_imagefile_data (GtkWidget *widget,
			     GCallback  set_imagefile_func,
			     gpointer   set_imagefile_data,
			     guchar    *vals,
			     gint       format,
			     gint       length)
{
  GimpImagefile *imagefile;
  gchar         *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid imagefile data!");
      return FALSE;
    }

  name = (gchar *) vals;

  imagefile = (GimpImagefile *)
    gimp_container_get_child_by_name (the_dnd_gimp->documents, name);

  if (! imagefile)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_imagefile_func) (widget,
                                                    GIMP_VIEWABLE (imagefile),
                                                    set_imagefile_data);

  return TRUE;
}


/*****************************/
/*  template dnd functions  */
/*****************************/

static gboolean
gimp_dnd_set_template_data (GtkWidget *widget,
                            GCallback  set_template_func,
                            gpointer   set_template_data,
                            guchar    *vals,
                            gint       format,
                            gint       length)
{
  GimpTemplate *template;
  gchar        *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid template data!");
      return FALSE;
    }

  name = (gchar *) vals;

  template = (GimpTemplate *)
    gimp_container_get_child_by_name (the_dnd_gimp->templates, name);

  if (! template)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_template_func) (widget,
                                                   GIMP_VIEWABLE (template),
                                                   set_template_data);

  return TRUE;
}


/************************/
/*  tool dnd functions  */
/************************/

static gboolean
gimp_dnd_set_tool_data (GtkWidget *widget,
			GCallback  set_tool_func,
			gpointer   set_tool_data,
			guchar    *vals,
			gint       format,
			gint       length)
{
  GimpToolInfo *tool_info;
  gchar        *name;

  if ((format != 8) || (length < 1))
    {
      g_warning ("Received invalid tool data!");
      return FALSE;
    }

  name = (gchar *) vals;

  if (strcmp (name, "gimp-standard-tool") == 0)
    tool_info = gimp_tool_info_get_standard (the_dnd_gimp);
  else
    tool_info = (GimpToolInfo *)
      gimp_container_get_child_by_name (the_dnd_gimp->tool_info_list, name);

  if (! tool_info)
    return FALSE;

  (* (GimpDndDropViewableFunc) set_tool_func) (widget,
                                               GIMP_VIEWABLE (tool_info),
                                               set_tool_data);

  return TRUE;
}
