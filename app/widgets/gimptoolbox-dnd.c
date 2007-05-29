/* GIMP - The GNU Image Manipulation Program
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimp-edit.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimptoolinfo.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "gimpdnd.h"
#include "gimptoolbox.h"
#include "gimptoolbox-dnd.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_toolbox_drop_uri_list  (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GList           *uri_list,
                                           gpointer         data);
static void   gimp_toolbox_drop_drawable  (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GimpViewable    *viewable,
                                           gpointer         data);
static void   gimp_toolbox_drop_tool      (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GimpViewable    *viewable,
                                           gpointer         data);
static void   gimp_toolbox_drop_buffer    (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GimpViewable    *viewable,
                                           gpointer         data);
static void   gimp_toolbox_drop_component (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GimpImage       *image,
                                           GimpChannelType  component,
                                           gpointer         data);
static void   gimp_toolbox_drop_pixbuf    (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GdkPixbuf       *pixbuf,
                                           gpointer         data);


/*  public functions  */

void
gimp_toolbox_dnd_init (GimpToolbox *toolbox)
{
  GimpDock *dock;

  g_return_if_fail (GIMP_IS_TOOLBOX (toolbox));

  dock = GIMP_DOCK (toolbox);

  gimp_dnd_uri_list_dest_add (GTK_WIDGET (toolbox),
                              gimp_toolbox_drop_uri_list,
                              dock->context);
  gimp_dnd_uri_list_dest_add (toolbox->tool_wbox,
                              gimp_toolbox_drop_uri_list,
                              dock->context);

  gimp_dnd_viewable_dest_add (toolbox->tool_wbox, GIMP_TYPE_LAYER,
                              gimp_toolbox_drop_drawable,
                              dock->context);
  gimp_dnd_viewable_dest_add (toolbox->tool_wbox, GIMP_TYPE_LAYER_MASK,
                              gimp_toolbox_drop_drawable,
                              dock->context);
  gimp_dnd_viewable_dest_add (toolbox->tool_wbox, GIMP_TYPE_CHANNEL,
                              gimp_toolbox_drop_drawable,
                              dock->context);
  gimp_dnd_viewable_dest_add (toolbox->tool_wbox, GIMP_TYPE_TOOL_INFO,
                              gimp_toolbox_drop_tool,
                              dock->context);
  gimp_dnd_viewable_dest_add (toolbox->tool_wbox, GIMP_TYPE_BUFFER,
                              gimp_toolbox_drop_buffer,
                              dock->context);

  gimp_dnd_component_dest_add (toolbox->tool_wbox,
                               gimp_toolbox_drop_component,
                               dock->context);
  gimp_dnd_pixbuf_dest_add    (toolbox->tool_wbox,
                               gimp_toolbox_drop_pixbuf,
                               dock->context);
}


/*  private functions  */

static void
gimp_toolbox_drop_uri_list (GtkWidget *widget,
                            gint       x,
                            gint       y,
                            GList     *uri_list,
                            gpointer   data)
{
  GimpContext *context = GIMP_CONTEXT (data);
  GList       *list;

  if (context->gimp->busy)
    return;

  for (list = uri_list; list; list = g_list_next (list))
    {
      const gchar       *uri   = list->data;
      GimpImage         *image;
      GimpPDBStatusType  status;
      GError            *error = NULL;

      image = file_open_with_display (context->gimp, context, NULL,
                                      uri, FALSE, &status, &error);

      if (! image && status != GIMP_PDB_CANCEL)
        {
          gchar *filename = file_utils_uri_display_name (uri);

          gimp_message (context->gimp, G_OBJECT (widget), GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        filename, error->message);

          g_clear_error (&error);
          g_free (filename);
        }
    }
}

static void
gimp_toolbox_drop_drawable (GtkWidget    *widget,
                            gint          x,
                            gint          y,
                            GimpViewable *viewable,
                            gpointer      data)
{
  GimpContext       *context = GIMP_CONTEXT (data);
  GimpDrawable      *drawable;
  GimpItem          *item;
  GimpImage         *image;
  GimpImage         *new_image;
  GimpLayer         *new_layer;
  GType              new_type;
  gint               width, height;
  gint               off_x, off_y;
  gint               bytes;
  GimpImageBaseType  type;

  if (context->gimp->busy)
    return;

  drawable = GIMP_DRAWABLE (viewable);
  item     = GIMP_ITEM (viewable);
  image    = gimp_item_get_image (item);

  width  = gimp_item_width  (item);
  height = gimp_item_height (item);
  bytes  = gimp_drawable_bytes (drawable);

  type = GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable));

  new_image = gimp_create_image (image->gimp, width, height, type, TRUE);
  gimp_image_undo_disable (new_image);

  if (type == GIMP_INDEXED)
    gimp_image_set_colormap (new_image,
                             gimp_image_get_colormap (image),
                             gimp_image_get_colormap_size (image),
                             FALSE);

  gimp_image_set_resolution (new_image, image->xresolution, image->yresolution);
  gimp_image_set_unit (new_image, gimp_image_get_unit (image));

  if (GIMP_IS_LAYER (drawable))
    new_type = G_TYPE_FROM_INSTANCE (drawable);
  else
    new_type = GIMP_TYPE_LAYER;

  new_layer = GIMP_LAYER (gimp_item_convert (GIMP_ITEM (drawable), new_image,
                                             new_type, FALSE));

  gimp_object_set_name (GIMP_OBJECT (new_layer),
                        gimp_object_get_name (GIMP_OBJECT (drawable)));

  gimp_item_offsets (GIMP_ITEM (new_layer), &off_x, &off_y);
  gimp_item_translate (GIMP_ITEM (new_layer), -off_x, -off_y, FALSE);
  gimp_item_set_visible (GIMP_ITEM (new_layer), TRUE, FALSE);
  gimp_item_set_linked (GIMP_ITEM (new_layer), FALSE, FALSE);
  gimp_layer_set_mode (new_layer, GIMP_NORMAL_MODE, FALSE);
  gimp_layer_set_opacity (new_layer, GIMP_OPACITY_OPAQUE, FALSE);
  gimp_layer_set_lock_alpha (new_layer, FALSE, FALSE);

  gimp_image_add_layer (new_image, new_layer, 0);

  gimp_image_undo_enable (new_image);

  gimp_create_display (image->gimp, new_image, GIMP_UNIT_PIXEL, 1.0);
  g_object_unref (new_image);
}

static void
gimp_toolbox_drop_tool (GtkWidget    *widget,
                        gint          x,
                        gint          y,
                        GimpViewable *viewable,
                        gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);

  if (context->gimp->busy)
    return;

  gimp_context_set_tool (context, GIMP_TOOL_INFO (viewable));
}

static void
gimp_toolbox_drop_buffer (GtkWidget    *widget,
                          gint          x,
                          gint          y,
                          GimpViewable *viewable,
                          gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);
  GimpImage   *image;

  if (context->gimp->busy)
    return;

  image = gimp_edit_paste_as_new (context->gimp, NULL, GIMP_BUFFER (viewable));

  if (image)
    {
      gimp_create_display (image->gimp, image, GIMP_UNIT_PIXEL, 1.0);
      g_object_unref (image);
    }
}

static void
gimp_toolbox_drop_component (GtkWidget       *widget,
                             gint             x,
                             gint             y,
                             GimpImage       *image,
                             GimpChannelType  component,
                             gpointer         data)
{
  GimpContext *context = GIMP_CONTEXT (data);
  GimpChannel *channel;
  GimpImage   *new_image;
  GimpLayer   *new_layer;
  const gchar *desc;

  if (context->gimp->busy)
    return;

  new_image = gimp_create_image (image->gimp,
                                 gimp_image_get_width  (image),
                                 gimp_image_get_height (image),
                                 GIMP_GRAY, TRUE);

  gimp_image_undo_disable (new_image);

  gimp_image_set_resolution (new_image, image->xresolution, image->yresolution);
  gimp_image_set_unit (new_image, gimp_image_get_unit (image));

  channel = gimp_channel_new_from_component (image, component, NULL, NULL);

  new_layer = GIMP_LAYER (gimp_item_convert (GIMP_ITEM (channel), new_image,
                                             GIMP_TYPE_LAYER, FALSE));

  g_object_unref (channel);

  gimp_enum_get_value (GIMP_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  gimp_object_take_name (GIMP_OBJECT (new_layer),
                         g_strdup_printf (_("%s Channel Copy"), desc));

  gimp_image_add_layer (new_image, new_layer, 0);

  gimp_image_undo_enable (new_image);

  gimp_create_display (new_image->gimp, new_image, GIMP_UNIT_PIXEL, 1.0);
  g_object_unref (new_image);
}

static void
gimp_toolbox_drop_pixbuf (GtkWidget *widget,
                          gint       x,
                          gint       y,
                          GdkPixbuf *pixbuf,
                          gpointer   data)
{
  GimpContext   *context = GIMP_CONTEXT (data);
  GimpImageType  image_type;
  GimpImage     *new_image;
  GimpLayer     *new_layer;

  if (context->gimp->busy)
    return;

  switch (gdk_pixbuf_get_n_channels (pixbuf))
    {
    case 1: image_type = GIMP_GRAY_IMAGE;  break;
    case 2: image_type = GIMP_GRAYA_IMAGE; break;
    case 3: image_type = GIMP_RGB_IMAGE;   break;
    case 4: image_type = GIMP_RGBA_IMAGE;  break;
      break;

    default:
      g_return_if_reached ();
      break;
    }

  new_image = gimp_create_image (context->gimp,
                                 gdk_pixbuf_get_width  (pixbuf),
                                 gdk_pixbuf_get_height (pixbuf),
                                 GIMP_IMAGE_TYPE_BASE_TYPE (image_type),
                                 FALSE);

  gimp_image_undo_disable (new_image);

  new_layer =
    gimp_layer_new_from_pixbuf (pixbuf, new_image, image_type,
                                _("Dropped Buffer"),
                                GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

  gimp_image_add_layer (new_image, new_layer, 0);

  gimp_image_undo_enable (new_image);

  gimp_create_display (new_image->gimp, new_image, GIMP_UNIT_PIXEL, 1.0);
  g_object_unref (new_image);
}
