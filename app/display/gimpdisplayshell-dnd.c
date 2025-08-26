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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimp-edit.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable-edit.h"
#include "core/gimpfilloptions.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimplayer-new.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimppattern.h"
#include "core/gimpprogress.h"

#include "file/file-open.h"

#include "path/gimppath.h"
#include "path/gimppath-import.h"

#include "text/gimptext.h"
#include "text/gimptextlayer.h"

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
static void   gimp_display_shell_drop_path      (GtkWidget       *widget,
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
                                                 GeglColor       *color,
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

  gimp_dnd_viewable_dest_add  (shell->canvas, GIMP_TYPE_LAYER,
                               gimp_display_shell_drop_drawable,
                               shell);
  gimp_dnd_viewable_dest_add  (shell->canvas, GIMP_TYPE_LAYER_MASK,
                               gimp_display_shell_drop_drawable,
                               shell);
  gimp_dnd_viewable_dest_add  (shell->canvas, GIMP_TYPE_CHANNEL,
                               gimp_display_shell_drop_drawable,
                               shell);
  gimp_dnd_viewable_dest_add  (shell->canvas, GIMP_TYPE_PATH,
                               gimp_display_shell_drop_path,
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
  gimp_dnd_component_dest_add (shell->canvas,
                               gimp_display_shell_drop_component,
                               shell);
  gimp_dnd_uri_list_dest_add  (shell->canvas,
                               gimp_display_shell_drop_uri_list,
                               shell);
  gimp_dnd_svg_dest_add       (shell->canvas,
                               gimp_display_shell_drop_svg,
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
                                      GimpImage        *image,
                                      GimpItem         *item)
{
  gint item_width  = gimp_item_get_width  (item);
  gint item_height = gimp_item_get_height (item);
  gint off_x, off_y;

  if (item_width  >= gimp_image_get_width  (image) &&
      item_height >= gimp_image_get_height (image))
    {
      off_x = (gimp_image_get_width  (image) - item_width)  / 2;
      off_y = (gimp_image_get_height (image) - item_height) / 2;
    }
  else
    {
      gint x, y;
      gint width, height;

      gimp_display_shell_untransform_viewport (
        shell,
        ! gimp_display_shell_get_infinite_canvas (shell),
        &x, &y, &width, &height);

      off_x = x + (width  - item_width)  / 2;
      off_y = y + (height - item_height) / 2;
    }

  gimp_item_translate (item,
                       off_x - gimp_item_get_offset_x (item),
                       off_y - gimp_item_get_offset_y (item),
                       FALSE);
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
      gimp_create_display (shell->display->gimp, image, gimp_unit_pixel (), 1.0,
                           G_OBJECT (gimp_widget_get_monitor (widget)));
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

      gimp_display_shell_dnd_position_item (shell, image, new_item);

      gimp_item_set_visible (new_item, TRUE, FALSE);

      gimp_image_add_layer (image, new_layer,
                            GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
      gimp_drawable_enable_resize_undo (GIMP_DRAWABLE (new_layer));

      gimp_image_undo_group_end (image);

      gimp_display_shell_dnd_flush (shell, image);
    }
}

static void
gimp_display_shell_drop_path (GtkWidget    *widget,
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
      GimpPath *new_path = GIMP_PATH (new_item);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Path"));

      gimp_image_add_path (image, new_path,
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

  if (! gimp_path_import_buffer (image,
                                 (const gchar *) svg_data, svg_data_len,
                                 TRUE, FALSE,
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
gimp_display_shell_dnd_fill (GimpDisplayShell *shell,
                             GimpFillOptions  *options,
                             const gchar      *undo_desc)
{
  GimpImage     *image  = gimp_display_get_image (shell->display);
  GimpGuiConfig *config = GIMP_GUI_CONFIG (shell->display->gimp->config);
  GList         *drawables;
  GList         *iter;

  if (shell->display->gimp->busy)
    return;

  if (! image)
    return;

  drawables = gimp_image_get_selected_drawables (image);

  if (! drawables)
    return;

  for (iter = drawables; iter; iter = iter->next)
    {
      if (gimp_viewable_get_children (iter->data))
        {
          gimp_message_literal (shell->display->gimp, G_OBJECT (shell->display),
                                GIMP_MESSAGE_ERROR,
                                _("Cannot modify the pixels of layer groups."));
          g_list_free (drawables);
          return;
        }

      if (gimp_item_is_content_locked (iter->data, NULL))
        {
          gimp_message_literal (shell->display->gimp, G_OBJECT (shell->display),
                                GIMP_MESSAGE_ERROR,
                                _("A selected layer's pixels are locked."));
          g_list_free (drawables);
          return;
        }

      if (! gimp_item_is_visible (GIMP_ITEM (iter->data)) &&
          ! config->edit_non_visible)
        {
          gimp_message_literal (shell->display->gimp, G_OBJECT (shell->display),
                                GIMP_MESSAGE_ERROR,
                                _("A selected layer is not visible."));
          g_list_free (drawables);
          return;
        }
    }

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_PAINT, undo_desc);

  for (iter = drawables; iter; iter = iter->next)
    {
      /* FIXME: there should be a virtual method for this that the
       *        GimpTextLayer can override.
       */
      if (gimp_item_is_text_layer (iter->data) &&
          (gimp_fill_options_get_style (options) == GIMP_FILL_STYLE_FG_COLOR ||
           gimp_fill_options_get_style (options) == GIMP_FILL_STYLE_BG_COLOR))
        {
          GeglColor *color;

          if (gimp_fill_options_get_style (options) == GIMP_FILL_STYLE_FG_COLOR)
            color = gimp_context_get_foreground (GIMP_CONTEXT (options));
          else
            color = gimp_context_get_background (GIMP_CONTEXT (options));

          gimp_text_layer_set (iter->data, NULL,
                               "color", color,
                               NULL);
        }
      else
        {
          gimp_drawable_edit_fill (iter->data, options, undo_desc);
        }
    }

  g_list_free (drawables);
  gimp_image_undo_group_end (image);
  gimp_display_shell_dnd_flush (shell, image);
}

static void
gimp_display_shell_drop_pattern (GtkWidget    *widget,
                                 gint          x,
                                 gint          y,
                                 GimpViewable *viewable,
                                 gpointer      data)
{
  GimpDisplayShell *shell   = GIMP_DISPLAY_SHELL (data);
  GimpFillOptions  *options = gimp_fill_options_new (shell->display->gimp,
                                                     NULL, FALSE);

  GIMP_LOG (DND, NULL);

  gimp_fill_options_set_style (options, GIMP_FILL_STYLE_PATTERN);
  gimp_context_set_pattern (GIMP_CONTEXT (options), GIMP_PATTERN (viewable));

  gimp_display_shell_dnd_fill (shell, options,
                               C_("undo-type", "Drop pattern to layer"));

  g_object_unref (options);
}

static void
gimp_display_shell_drop_color (GtkWidget     *widget,
                               gint           x,
                               gint           y,
                               GeglColor     *color,
                               gpointer       data)
{
  GimpDisplayShell *shell   = GIMP_DISPLAY_SHELL (data);
  GimpFillOptions  *options = gimp_fill_options_new (shell->display->gimp,
                                                     NULL, FALSE);

  GIMP_LOG (DND, NULL);

  gimp_fill_options_set_style (options, GIMP_FILL_STYLE_FG_COLOR);
  gimp_context_set_foreground (GIMP_CONTEXT (options), color);

  gimp_display_shell_dnd_fill (shell, options,
                               C_("undo-type", "Drop color to layer"));

  g_object_unref (options);
}

static void
gimp_display_shell_drop_buffer (GtkWidget    *widget,
                                gint          drop_x,
                                gint          drop_y,
                                GimpViewable *viewable,
                                gpointer      data)
{
  GimpDisplayShell *shell    = GIMP_DISPLAY_SHELL (data);
  GimpImage        *image    = gimp_display_get_image (shell->display);
  GimpContext      *context;
  GList            *drawables;
  GimpBuffer       *buffer;
  GimpPasteType     paste_type;
  gint              x, y, width, height;

  GIMP_LOG (DND, NULL);

  if (shell->display->gimp->busy)
    return;

  if (! image)
    {
      image = gimp_image_new_from_buffer (shell->display->gimp,
                                          GIMP_BUFFER (viewable));
      gimp_create_display (image->gimp, image, gimp_unit_pixel (), 1.0,
                           G_OBJECT (gimp_widget_get_monitor (widget)));
      g_object_unref (image);

      return;
    }

  paste_type = GIMP_PASTE_TYPE_NEW_LAYER_OR_FLOATING;
  drawables  = gimp_image_get_selected_drawables (image);
  context    = gimp_get_user_context (shell->display->gimp);
  buffer     = GIMP_BUFFER (viewable);

  gimp_display_shell_untransform_viewport (
    shell,
    ! gimp_display_shell_get_infinite_canvas (shell),
    &x, &y, &width, &height);

  /* FIXME: popup a menu for selecting "Paste Into" */

  g_list_free (gimp_edit_paste (image, drawables, GIMP_OBJECT (buffer),
                                paste_type, context, FALSE,
                                x, y, width, height));

  g_list_free (drawables);
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
  GimpImage        *image;
  GimpContext      *context;
  GList            *list;
  gboolean          open_as_layers;

  /* If the app is already being torn down, shell->display might be
   * NULL here.  Play it safe.
   */
  if (! shell->display)
    return;

  image = gimp_display_get_image (shell->display);
  context = gimp_get_user_context (shell->display->gimp);

  GIMP_LOG (DND, NULL);

  open_as_layers = (image != NULL);

  if (image)
    g_object_ref (image);

  for (list = uri_list; list; list = g_list_next (list))
    {
      GFile             *file  = g_file_new_for_uri (list->data);
      GimpPDBStatusType  status;
      GError            *error = NULL;
      gboolean           warn  = FALSE;

      if (! shell->display)
        {
          /* It seems as if GIMP is being torn down for quitting. Bail out. */
          g_object_unref (file);
          g_clear_object (&image);
          return;
        }

      if (open_as_layers)
        {
          GList *new_layers;

          new_layers = file_open_layers (shell->display->gimp, context,
                                         GIMP_PROGRESS (shell->display),
                                         image, FALSE, FALSE,
                                         file, GIMP_RUN_INTERACTIVE, NULL,
                                         &status, &error);

          if (new_layers)
            {
              gint x      = 0;
              gint y      = 0;
              gint width  = gimp_image_get_width  (image);
              gint height = gimp_image_get_height (image);

              if (gimp_display_get_image (shell->display))
                {
                  gimp_display_shell_untransform_viewport (
                    shell,
                    ! gimp_display_shell_get_infinite_canvas (shell),
                    &x, &y, &width, &height);
                }

              gimp_image_add_layers (image, new_layers,
                                     GIMP_IMAGE_ACTIVE_PARENT, -1,
                                     x, y, width, height,
                                     _("Drop layers"));

              g_list_free (new_layers);
            }
          else if (status != GIMP_PDB_CANCEL && status != GIMP_PDB_SUCCESS)
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
                                              file, FALSE,
                                              G_OBJECT (gimp_widget_get_monitor (widget)),
                                              &status, &error);

          if (! new_image && status != GIMP_PDB_CANCEL && status != GIMP_PDB_SUCCESS)
            warn = TRUE;
        }
      else
        {
          /*  open the first image in the empty display  */
          image = file_open_with_display (shell->display->gimp, context,
                                          GIMP_PROGRESS (shell->display),
                                          file, FALSE,
                                          G_OBJECT (gimp_widget_get_monitor (widget)),
                                          &status, &error);

          if (image)
            {
              g_object_ref (image);
            }
          else if (status != GIMP_PDB_CANCEL && status != GIMP_PDB_SUCCESS)
            {
              warn = TRUE;
            }
        }

      /* Something above might have run a few rounds of the main loop. Check
       * that shell->display is still there, otherwise ignore this as the app
       * is being torn down for quitting.
       */
      if (warn && shell->display)
        {
          gimp_message (shell->display->gimp, G_OBJECT (shell->display),
                        GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        gimp_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }

  if (image)
    gimp_display_shell_dnd_flush (shell, image);

  g_clear_object (&image);
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
    {
      dest_image = gimp_image_new_from_component (image->gimp,
                                                  image, component);
      gimp_create_display (dest_image->gimp, dest_image, gimp_unit_pixel (), 1.0,
                           G_OBJECT (gimp_widget_get_monitor (widget)));
      g_object_unref (dest_image);

      return;
    }

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

      gimp_display_shell_dnd_position_item (shell, image, new_item);

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
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);
  GimpImage        *image = gimp_display_get_image (shell->display);
  GimpLayer        *new_layer;
  gboolean          has_alpha = FALSE;

  GIMP_LOG (DND, NULL);

  if (shell->display->gimp->busy)
    return;

  if (! image)
    {
      image = gimp_image_new_from_pixbuf (shell->display->gimp, pixbuf,
                                          _("Dropped Buffer"));
      gimp_create_display (image->gimp, image, gimp_unit_pixel (), 1.0,
                           G_OBJECT (gimp_widget_get_monitor (widget)));
      g_object_unref (image);

      return;
    }

  if (gdk_pixbuf_get_n_channels (pixbuf) == 2 ||
      gdk_pixbuf_get_n_channels (pixbuf) == 4)
    {
      has_alpha = TRUE;
    }

  new_layer =
    gimp_layer_new_from_pixbuf (pixbuf, image,
                                gimp_image_get_layer_format (image, has_alpha),
                                _("Dropped Buffer"),
                                GIMP_OPACITY_OPAQUE,
                                gimp_image_get_default_new_layer_mode (image));

  if (new_layer)
    {
      GimpItem *new_item = GIMP_ITEM (new_layer);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Layer"));

      gimp_display_shell_dnd_position_item (shell, image, new_item);

      gimp_image_add_layer (image, new_layer,
                            GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

      gimp_image_undo_group_end (image);

      gimp_display_shell_dnd_flush (shell, image);
    }
}
