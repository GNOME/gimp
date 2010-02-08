/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimp-edit.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable-bucket-fill.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-new.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimppattern.h"
#include "core/gimpprogress.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "text/gimptext.h"
#include "text/gimptextlayer.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpvectors-import.h"

#include "widgets/gimpdnd.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-dnd.h"
#include "gimpdisplayshell-transform.h"

#include "gimp-log.h"
#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_display_shell_drop_drawable  (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GimpViewable    *viewable,
                                                 gpointer         data);
static void   gimp_display_shell_drop_vectors   (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GimpViewable    *viewable,
                                                 gpointer         data);
static void   gimp_display_shell_drop_svg       (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 const guchar    *svg_data,
                                                 gsize            svg_data_length,
                                                 gpointer         data);
static void   gimp_display_shell_drop_pattern   (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GimpViewable    *viewable,
                                                 gpointer         data);
static void   gimp_display_shell_drop_color     (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 const GimpRGB   *color,
                                                 gpointer         data);
static void   gimp_display_shell_drop_buffer    (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GimpViewable    *viewable,
                                                 gpointer         data);
static void   gimp_display_shell_drop_uri_list  (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GList           *uri_list,
                                                 gpointer         data);
static void   gimp_display_shell_drop_component (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GimpImage       *image,
                                                 GimpChannelType  component,
                                                 gpointer         data);
static void   gimp_display_shell_drop_pixbuf    (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GdkPixbuf       *pixbuf,
                                                 gpointer         data);


/*  public functions  */

void
gimp_display_shell_dnd_init (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_dnd_uri_list_dest_add  (shell->canvas,
                               gimp_display_shell_drop_uri_list,
                               shell);
  gimp_dnd_viewable_dest_add  (shell->canvas, GIMP_TYPE_LAYER,
                               gimp_display_shell_drop_drawable,
                               shell);
  gimp_dnd_viewable_dest_add  (shell->canvas, GIMP_TYPE_LAYER_MASK,
                               gimp_display_shell_drop_drawable,
                               shell);
  gimp_dnd_viewable_dest_add  (shell->canvas, GIMP_TYPE_CHANNEL,
                               gimp_display_shell_drop_drawable,
                               shell);
  gimp_dnd_viewable_dest_add  (shell->canvas, GIMP_TYPE_VECTORS,
                               gimp_display_shell_drop_vectors,
                               shell);
  gimp_dnd_viewable_dest_add  (shell->canvas, GIMP_TYPE_PATTERN,
                               gimp_display_shell_drop_pattern,
                               shell);
  gimp_dnd_viewable_dest_add  (shell->canvas, GIMP_TYPE_BUFFER,
                               gimp_display_shell_drop_buffer,
                               shell);
  gimp_dnd_color_dest_add     (shell->canvas,
                               gimp_display_shell_drop_color,
                               shell);
  gimp_dnd_svg_dest_add       (shell->canvas,
                               gimp_display_shell_drop_svg,
                               shell);
  gimp_dnd_component_dest_add (shell->canvas,
                               gimp_display_shell_drop_component,
                               shell);
  gimp_dnd_pixbuf_dest_add    (shell->canvas,
                               gimp_display_shell_drop_pixbuf,
                               shell);
}


/*  private functions  */

/*
 * Position the dropped item in the middle of the viewport.
 */
static void
gimp_display_shell_dnd_position_item (GimpDisplayShell *shell,
                                      GimpItem         *item)
{
  gint x, y;
  gint width, height;
  gint off_x, off_y;

  gimp_display_shell_untransform_viewport (shell, &x, &y, &width, &height);

  gimp_item_get_offset (item, &off_x, &off_y);

  off_x = x + (width  - gimp_item_get_width  (item)) / 2 - off_x;
  off_y = y + (height - gimp_item_get_height (item)) / 2 - off_y;

  gimp_item_translate (item, off_x, off_y, FALSE);
}

static void
gimp_display_shell_dnd_flush (GimpDisplayShell *shell,
                              GimpImage        *image)
{
  gimp_display_shell_present (shell);

  gimp_image_flush (image);

  gimp_context_set_display (gimp_get_user_context (shell->display->gimp),
                            shell->display);
}

static void
gimp_display_shell_drop_drawable (GtkWidget    *widget,
                                  gint          x,
                                  gint          y,
                                  GimpViewable *viewable,
                                  gpointer      data)
{
  GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (data);
  GimpImage        *image     = gimp_display_get_image (shell->display);
  GType             new_type;
  GimpItem         *new_item;

  GIMP_LOG (DND, NULL);

  if (shell->display->gimp->busy)
    return;

  if (! image)
    {
      image = gimp_image_new_from_drawable (shell->display->gimp,
                                            GIMP_DRAWABLE (viewable));
      gimp_create_display (shell->display->gimp, image, GIMP_UNIT_PIXEL, 1.0);
      g_object_unref (image);

      return;
    }

  if (GIMP_IS_LAYER (viewable))
    new_type = G_TYPE_FROM_INSTANCE (viewable);
  else
    new_type = GIMP_TYPE_LAYER;

  new_item = gimp_item_convert (GIMP_ITEM (viewable), image, new_type);

  if (new_item)
    {
      GimpLayer *new_layer = GIMP_LAYER (new_item);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Layer"));

      gimp_display_shell_dnd_position_item (shell, new_item);

      gimp_item_set_visible (new_item, TRUE, FALSE);
      gimp_item_set_linked (new_item, FALSE, FALSE);

      gimp_image_add_layer (image, new_layer,
                            GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

      gimp_image_undo_group_end (image);

      gimp_display_shell_dnd_flush (shell, image);
    }
}

static void
gimp_display_shell_drop_vectors (GtkWidget    *widget,
                                 gint          x,
                                 gint          y,
                                 GimpViewable *viewable,
                                 gpointer      data)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);
  GimpImage        *image = gimp_display_get_image (shell->display);
  GimpItem         *new_item;

  GIMP_LOG (DND, NULL);

  if (shell->display->gimp->busy)
    return;

  if (! image)
    return;

  new_item = gimp_item_convert (GIMP_ITEM (viewable),
                                image, G_TYPE_FROM_INSTANCE (viewable));

  if (new_item)
    {
      GimpVectors *new_vectors = GIMP_VECTORS (new_item);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Path"));

      gimp_image_add_vectors (image, new_vectors,
                              GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

      gimp_image_undo_group_end (image);

      gimp_display_shell_dnd_flush (shell, image);
    }
}

static void
gimp_display_shell_drop_svg (GtkWidget     *widget,
                             gint           x,
                             gint           y,
                             const guchar  *svg_data,
                             gsize          svg_data_len,
                             gpointer       data)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);
  GimpImage        *image = gimp_display_get_image (shell->display);
  GError           *error  = NULL;

  GIMP_LOG (DND, NULL);

  if (shell->display->gimp->busy)
    return;

  if (! image)
    return;

  if (! gimp_vectors_import_buffer (image,
                                    (const gchar *) svg_data, svg_data_len,
                                    TRUE, TRUE,
                                    GIMP_IMAGE_ACTIVE_PARENT, -1,
                                    NULL, &error))
    {
      gimp_message_literal (shell->display->gimp, G_OBJECT (shell->display),
			    GIMP_MESSAGE_ERROR,
			    error->message);
      g_clear_error (&error);
    }
  else
    {
      gimp_display_shell_dnd_flush (shell, image);
    }
}

static void
gimp_display_shell_dnd_bucket_fill (GimpDisplayShell   *shell,
                                    GimpBucketFillMode  fill_mode,
                                    const GimpRGB      *color,
                                    GimpPattern        *pattern)
{
  GimpImage    *image = gimp_display_get_image (shell->display);
  GimpDrawable *drawable;

  if (shell->display->gimp->busy)
    return;

  if (! image)
    return;

  drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    return;

  if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
    {
      gimp_message_literal (shell->display->gimp, G_OBJECT (shell->display),
                            GIMP_MESSAGE_ERROR,
                            _("Cannot modify the pixels of layer groups."));
      return;
    }

  if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
    {
      gimp_message_literal (shell->display->gimp, G_OBJECT (shell->display),
                            GIMP_MESSAGE_ERROR,
                            _("The active layer's pixels are locked."));
      return;
    }

  /* FIXME: there should be a virtual method for this that the
   *        GimpTextLayer can override.
   */
  if (color && gimp_drawable_is_text_layer (drawable))
    {
      gimp_text_layer_set (GIMP_TEXT_LAYER (drawable), NULL,
                           "color", color,
                           NULL);
    }
  else
    {
      gimp_drawable_bucket_fill_full (drawable,
                                      fill_mode,
                                      GIMP_NORMAL_MODE, GIMP_OPACITY_OPAQUE,
                                      FALSE,             /* no seed fill */
                                      FALSE,             /* don't fill transp */
                                      GIMP_SELECT_CRITERION_COMPOSITE,
                                      0.0, FALSE,        /* fill params  */
                                      0.0, 0.0,          /* ignored      */
                                      color, pattern);
    }

  gimp_display_shell_dnd_flush (shell, image);
}

static void
gimp_display_shell_drop_pattern (GtkWidget    *widget,
                                 gint          x,
                                 gint          y,
                                 GimpViewable *viewable,
                                 gpointer      data)
{
  GIMP_LOG (DND, NULL);

  if (GIMP_IS_PATTERN (viewable))
    gimp_display_shell_dnd_bucket_fill (GIMP_DISPLAY_SHELL (data),
                                        GIMP_PATTERN_BUCKET_FILL,
                                        NULL, GIMP_PATTERN (viewable));
}

static void
gimp_display_shell_drop_color (GtkWidget     *widget,
                               gint           x,
                               gint           y,
                               const GimpRGB *color,
                               gpointer       data)
{
  GIMP_LOG (DND, NULL);

  gimp_display_shell_dnd_bucket_fill (GIMP_DISPLAY_SHELL (data),
                                      GIMP_FG_BUCKET_FILL,
                                      color, NULL);
}

static void
gimp_display_shell_drop_buffer (GtkWidget    *widget,
                                gint          drop_x,
                                gint          drop_y,
                                GimpViewable *viewable,
                                gpointer      data)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);
  GimpImage        *image = gimp_display_get_image (shell->display);
  GimpDrawable     *drawable;
  GimpBuffer       *buffer;
  gint              x, y, width, height;

  GIMP_LOG (DND, NULL);

  if (shell->display->gimp->busy)
    return;

  if (! image)
    return;

  drawable = gimp_image_get_active_drawable (image);

  if (drawable)
    {
      if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
        {
          gimp_message_literal (shell->display->gimp, G_OBJECT (shell->display),
                                GIMP_MESSAGE_ERROR,
                                _("Cannot modify the pixels of layer groups."));
          return;
        }

      if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
        {
          gimp_message_literal (shell->display->gimp, G_OBJECT (shell->display),
                                GIMP_MESSAGE_ERROR,
                                _("The active layer's pixels are locked."));
          return;
        }
    }

  buffer = GIMP_BUFFER (viewable);

  gimp_display_shell_untransform_viewport (shell, &x, &y, &width, &height);

  /* FIXME: popup a menu for selecting "Paste Into" */

  gimp_edit_paste (image, drawable, buffer, FALSE,
                   x, y, width, height);

  gimp_display_shell_dnd_flush (shell, image);
}

static void
gimp_display_shell_drop_uri_list (GtkWidget *widget,
                                  gint       x,
                                  gint       y,
                                  GList     *uri_list,
                                  gpointer   data)
{
  GimpDisplayShell *shell   = GIMP_DISPLAY_SHELL (data);
  GimpImage        *image   = gimp_display_get_image (shell->display);
  GimpContext      *context = gimp_get_user_context (shell->display->gimp);
  GList            *list;
  gboolean          open_as_layers;

  GIMP_LOG (DND, NULL);

  open_as_layers = (image != NULL);

  for (list = uri_list; list; list = g_list_next (list))
    {
      const gchar       *uri   = list->data;
      GimpPDBStatusType  status;
      GError            *error = NULL;
      gboolean           warn  = FALSE;

      if (open_as_layers)
        {
          GList *new_layers;

          new_layers = file_open_layers (shell->display->gimp, context,
                                         GIMP_PROGRESS (shell->display),
                                         image, FALSE,
                                         uri, GIMP_RUN_INTERACTIVE, NULL,
                                         &status, &error);

          if (new_layers)
            {
              gint x, y;
              gint width, height;

              gimp_display_shell_untransform_viewport (shell, &x, &y,
                                                       &width, &height);

              gimp_image_add_layers (image, new_layers,
                                     GIMP_IMAGE_ACTIVE_PARENT, -1,
                                     x, y, width, height,
                                     _("Drop layers"));

              g_list_free (new_layers);
            }
          else if (status != GIMP_PDB_CANCEL)
            {
              warn = TRUE;
            }
        }
      else if (gimp_display_get_image (shell->display))
        {
          /*  open any subsequent images in a new display  */
          GimpImage *new_image;

          new_image = file_open_with_display (shell->display->gimp, context,
                                              NULL,
                                              uri, FALSE,
                                              &status, &error);

          if (! new_image && status != GIMP_PDB_CANCEL)
            warn = TRUE;
        }
      else
        {
          /*  open the first image in the empty display  */
          image = file_open_with_display (shell->display->gimp, context,
                                          GIMP_PROGRESS (shell->display),
                                          uri, FALSE,
                                          &status, &error);

          if (! image && status != GIMP_PDB_CANCEL)
            warn = TRUE;
        }

      if (warn)
        {
          gchar *filename = file_utils_uri_display_name (uri);

          gimp_message (shell->display->gimp, G_OBJECT (shell->display),
                        GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        filename, error->message);

          g_clear_error (&error);
          g_free (filename);
        }
    }

  if (image)
    gimp_display_shell_dnd_flush (shell, image);
}

static void
gimp_display_shell_drop_component (GtkWidget       *widget,
                                   gint             x,
                                   gint             y,
                                   GimpImage       *image,
                                   GimpChannelType  component,
                                   gpointer         data)
{
  GimpDisplayShell *shell      = GIMP_DISPLAY_SHELL (data);
  GimpImage        *dest_image = gimp_display_get_image (shell->display);
  GimpChannel      *channel;
  GimpItem         *new_item;
  const gchar      *desc;

  GIMP_LOG (DND, NULL);

  if (shell->display->gimp->busy)
    return;

  if (! dest_image)
    return;

  channel = gimp_channel_new_from_component (image, component, NULL, NULL);

  new_item = gimp_item_convert (GIMP_ITEM (channel),
                                dest_image, GIMP_TYPE_LAYER);
  g_object_unref (channel);

  if (new_item)
    {
      GimpLayer *new_layer = GIMP_LAYER (new_item);

      gimp_enum_get_value (GIMP_TYPE_CHANNEL_TYPE, component,
                           NULL, NULL, &desc, NULL);
      gimp_object_take_name (GIMP_OBJECT (new_layer),
                             g_strdup_printf (_("%s Channel Copy"), desc));

      gimp_image_undo_group_start (dest_image, GIMP_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Layer"));

      gimp_display_shell_dnd_position_item (shell, new_item);

      gimp_image_add_layer (dest_image, new_layer,
                            GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

      gimp_image_undo_group_end (dest_image);

      gimp_display_shell_dnd_flush (shell, dest_image);
    }
}

static void
gimp_display_shell_drop_pixbuf (GtkWidget *widget,
                                gint       x,
                                gint       y,
                                GdkPixbuf *pixbuf,
                                gpointer   data)
{
  GimpDisplayShell *shell     = GIMP_DISPLAY_SHELL (data);
  GimpImage        *image     = gimp_display_get_image (shell->display);
  GimpLayer        *new_layer;
  GimpImageType     image_type;

  GIMP_LOG (DND, NULL);

  if (shell->display->gimp->busy)
    return;

  if (! image)
    {
      image = gimp_image_new_from_pixbuf (shell->display->gimp, pixbuf,
                                          _("Dropped Buffer"));
      gimp_create_display (image->gimp, image, GIMP_UNIT_PIXEL, 1.0);
      g_object_unref (image);

      return;
    }

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

  new_layer =
    gimp_layer_new_from_pixbuf (pixbuf, image, image_type,
                                _("Dropped Buffer"),
                                GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

  if (new_layer)
    {
      GimpItem *new_item = GIMP_ITEM (new_layer);

      new_item = GIMP_ITEM (new_layer);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Layer"));

      gimp_display_shell_dnd_position_item (shell, new_item);

      gimp_image_add_layer (image, new_layer,
                            GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

      gimp_image_undo_group_end (image);

      gimp_display_shell_dnd_flush (shell, image);
    }
}
