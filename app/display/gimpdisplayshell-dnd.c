/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "core/ligma.h"
#include "core/ligma-edit.h"
#include "core/ligmabuffer.h"
#include "core/ligmadrawable-edit.h"
#include "core/ligmafilloptions.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-new.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmalayer.h"
#include "core/ligmalayer-new.h"
#include "core/ligmalayermask.h"
#include "core/ligmapattern.h"
#include "core/ligmaprogress.h"

#include "file/file-open.h"

#include "text/ligmatext.h"
#include "text/ligmatextlayer.h"

#include "vectors/ligmavectors.h"
#include "vectors/ligmavectors-import.h"

#include "widgets/ligmadnd.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-dnd.h"
#include "ligmadisplayshell-transform.h"

#include "ligma-log.h"
#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_display_shell_drop_drawable  (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 LigmaViewable    *viewable,
                                                 gpointer         data);
static void   ligma_display_shell_drop_vectors   (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 LigmaViewable    *viewable,
                                                 gpointer         data);
static void   ligma_display_shell_drop_svg       (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 const guchar    *svg_data,
                                                 gsize            svg_data_length,
                                                 gpointer         data);
static void   ligma_display_shell_drop_pattern   (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 LigmaViewable    *viewable,
                                                 gpointer         data);
static void   ligma_display_shell_drop_color     (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 const LigmaRGB   *color,
                                                 gpointer         data);
static void   ligma_display_shell_drop_buffer    (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 LigmaViewable    *viewable,
                                                 gpointer         data);
static void   ligma_display_shell_drop_uri_list  (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GList           *uri_list,
                                                 gpointer         data);
static void   ligma_display_shell_drop_component (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 LigmaImage       *image,
                                                 LigmaChannelType  component,
                                                 gpointer         data);
static void   ligma_display_shell_drop_pixbuf    (GtkWidget       *widget,
                                                 gint             x,
                                                 gint             y,
                                                 GdkPixbuf       *pixbuf,
                                                 gpointer         data);


/*  public functions  */

void
ligma_display_shell_dnd_init (LigmaDisplayShell *shell)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  ligma_dnd_viewable_dest_add  (shell->canvas, LIGMA_TYPE_LAYER,
                               ligma_display_shell_drop_drawable,
                               shell);
  ligma_dnd_viewable_dest_add  (shell->canvas, LIGMA_TYPE_LAYER_MASK,
                               ligma_display_shell_drop_drawable,
                               shell);
  ligma_dnd_viewable_dest_add  (shell->canvas, LIGMA_TYPE_CHANNEL,
                               ligma_display_shell_drop_drawable,
                               shell);
  ligma_dnd_viewable_dest_add  (shell->canvas, LIGMA_TYPE_VECTORS,
                               ligma_display_shell_drop_vectors,
                               shell);
  ligma_dnd_viewable_dest_add  (shell->canvas, LIGMA_TYPE_PATTERN,
                               ligma_display_shell_drop_pattern,
                               shell);
  ligma_dnd_viewable_dest_add  (shell->canvas, LIGMA_TYPE_BUFFER,
                               ligma_display_shell_drop_buffer,
                               shell);
  ligma_dnd_color_dest_add     (shell->canvas,
                               ligma_display_shell_drop_color,
                               shell);
  ligma_dnd_component_dest_add (shell->canvas,
                               ligma_display_shell_drop_component,
                               shell);
  ligma_dnd_uri_list_dest_add  (shell->canvas,
                               ligma_display_shell_drop_uri_list,
                               shell);
  ligma_dnd_svg_dest_add       (shell->canvas,
                               ligma_display_shell_drop_svg,
                               shell);
  ligma_dnd_pixbuf_dest_add    (shell->canvas,
                               ligma_display_shell_drop_pixbuf,
                               shell);
}


/*  private functions  */

/*
 * Position the dropped item in the middle of the viewport.
 */
static void
ligma_display_shell_dnd_position_item (LigmaDisplayShell *shell,
                                      LigmaImage        *image,
                                      LigmaItem         *item)
{
  gint item_width  = ligma_item_get_width  (item);
  gint item_height = ligma_item_get_height (item);
  gint off_x, off_y;

  if (item_width  >= ligma_image_get_width  (image) &&
      item_height >= ligma_image_get_height (image))
    {
      off_x = (ligma_image_get_width  (image) - item_width)  / 2;
      off_y = (ligma_image_get_height (image) - item_height) / 2;
    }
  else
    {
      gint x, y;
      gint width, height;

      ligma_display_shell_untransform_viewport (
        shell,
        ! ligma_display_shell_get_infinite_canvas (shell),
        &x, &y, &width, &height);

      off_x = x + (width  - item_width)  / 2;
      off_y = y + (height - item_height) / 2;
    }

  ligma_item_translate (item,
                       off_x - ligma_item_get_offset_x (item),
                       off_y - ligma_item_get_offset_y (item),
                       FALSE);
}

static void
ligma_display_shell_dnd_flush (LigmaDisplayShell *shell,
                              LigmaImage        *image)
{
  ligma_display_shell_present (shell);

  ligma_image_flush (image);

  ligma_context_set_display (ligma_get_user_context (shell->display->ligma),
                            shell->display);
}

static void
ligma_display_shell_drop_drawable (GtkWidget    *widget,
                                  gint          x,
                                  gint          y,
                                  LigmaViewable *viewable,
                                  gpointer      data)
{
  LigmaDisplayShell *shell     = LIGMA_DISPLAY_SHELL (data);
  LigmaImage        *image     = ligma_display_get_image (shell->display);
  GType             new_type;
  LigmaItem         *new_item;

  LIGMA_LOG (DND, NULL);

  if (shell->display->ligma->busy)
    return;

  if (! image)
    {
      image = ligma_image_new_from_drawable (shell->display->ligma,
                                            LIGMA_DRAWABLE (viewable));
      ligma_create_display (shell->display->ligma, image, LIGMA_UNIT_PIXEL, 1.0,
                           G_OBJECT (ligma_widget_get_monitor (widget)));
      g_object_unref (image);

      return;
    }

  if (LIGMA_IS_LAYER (viewable))
    new_type = G_TYPE_FROM_INSTANCE (viewable);
  else
    new_type = LIGMA_TYPE_LAYER;

  new_item = ligma_item_convert (LIGMA_ITEM (viewable), image, new_type);

  if (new_item)
    {
      LigmaLayer *new_layer = LIGMA_LAYER (new_item);

      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Layer"));

      ligma_display_shell_dnd_position_item (shell, image, new_item);

      ligma_item_set_visible (new_item, TRUE, FALSE);

      ligma_image_add_layer (image, new_layer,
                            LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);

      ligma_image_undo_group_end (image);

      ligma_display_shell_dnd_flush (shell, image);
    }
}

static void
ligma_display_shell_drop_vectors (GtkWidget    *widget,
                                 gint          x,
                                 gint          y,
                                 LigmaViewable *viewable,
                                 gpointer      data)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (data);
  LigmaImage        *image = ligma_display_get_image (shell->display);
  LigmaItem         *new_item;

  LIGMA_LOG (DND, NULL);

  if (shell->display->ligma->busy)
    return;

  if (! image)
    return;

  new_item = ligma_item_convert (LIGMA_ITEM (viewable),
                                image, G_TYPE_FROM_INSTANCE (viewable));

  if (new_item)
    {
      LigmaVectors *new_vectors = LIGMA_VECTORS (new_item);

      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Path"));

      ligma_image_add_vectors (image, new_vectors,
                              LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);

      ligma_image_undo_group_end (image);

      ligma_display_shell_dnd_flush (shell, image);
    }
}

static void
ligma_display_shell_drop_svg (GtkWidget     *widget,
                             gint           x,
                             gint           y,
                             const guchar  *svg_data,
                             gsize          svg_data_len,
                             gpointer       data)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (data);
  LigmaImage        *image = ligma_display_get_image (shell->display);
  GError           *error  = NULL;

  LIGMA_LOG (DND, NULL);

  if (shell->display->ligma->busy)
    return;

  if (! image)
    return;

  if (! ligma_vectors_import_buffer (image,
                                    (const gchar *) svg_data, svg_data_len,
                                    TRUE, FALSE,
                                    LIGMA_IMAGE_ACTIVE_PARENT, -1,
                                    NULL, &error))
    {
      ligma_message_literal (shell->display->ligma, G_OBJECT (shell->display),
                            LIGMA_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);
    }
  else
    {
      ligma_display_shell_dnd_flush (shell, image);
    }
}

static void
ligma_display_shell_dnd_fill (LigmaDisplayShell *shell,
                             LigmaFillOptions  *options,
                             const gchar      *undo_desc)
{
  LigmaImage    *image = ligma_display_get_image (shell->display);
  GList        *drawables;
  GList        *iter;

  if (shell->display->ligma->busy)
    return;

  if (! image)
    return;

  drawables = ligma_image_get_selected_drawables (image);

  if (! drawables)
    return;

  for (iter = drawables; iter; iter = iter->next)
    {
      if (ligma_viewable_get_children (iter->data))
        {
          ligma_message_literal (shell->display->ligma, G_OBJECT (shell->display),
                                LIGMA_MESSAGE_ERROR,
                                _("Cannot modify the pixels of layer groups."));
          g_list_free (drawables);
          return;
        }

      if (ligma_item_is_content_locked (iter->data, NULL))
        {
          ligma_message_literal (shell->display->ligma, G_OBJECT (shell->display),
                                LIGMA_MESSAGE_ERROR,
                                _("A selected layer's pixels are locked."));
          g_list_free (drawables);
          return;
        }
    }

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_PAINT, undo_desc);

  for (iter = drawables; iter; iter = iter->next)
    {
      /* FIXME: there should be a virtual method for this that the
       *        LigmaTextLayer can override.
       */
      if (ligma_fill_options_get_style (options) == LIGMA_FILL_STYLE_SOLID &&
          ligma_item_is_text_layer (iter->data))
        {
          LigmaRGB color;

          ligma_context_get_foreground (LIGMA_CONTEXT (options), &color);

          ligma_text_layer_set (iter->data, NULL,
                               "color", &color,
                               NULL);
        }
      else
        {
          ligma_drawable_edit_fill (iter->data, options, undo_desc);
        }
    }

  g_list_free (drawables);
  ligma_image_undo_group_end (image);
  ligma_display_shell_dnd_flush (shell, image);
}

static void
ligma_display_shell_drop_pattern (GtkWidget    *widget,
                                 gint          x,
                                 gint          y,
                                 LigmaViewable *viewable,
                                 gpointer      data)
{
  LigmaDisplayShell *shell   = LIGMA_DISPLAY_SHELL (data);
  LigmaFillOptions  *options = ligma_fill_options_new (shell->display->ligma,
                                                     NULL, FALSE);

  LIGMA_LOG (DND, NULL);

  ligma_fill_options_set_style (options, LIGMA_FILL_STYLE_PATTERN);
  ligma_context_set_pattern (LIGMA_CONTEXT (options), LIGMA_PATTERN (viewable));

  ligma_display_shell_dnd_fill (shell, options,
                               C_("undo-type", "Drop pattern to layer"));

  g_object_unref (options);
}

static void
ligma_display_shell_drop_color (GtkWidget     *widget,
                               gint           x,
                               gint           y,
                               const LigmaRGB *color,
                               gpointer       data)
{
  LigmaDisplayShell *shell   = LIGMA_DISPLAY_SHELL (data);
  LigmaFillOptions  *options = ligma_fill_options_new (shell->display->ligma,
                                                     NULL, FALSE);

  LIGMA_LOG (DND, NULL);

  ligma_fill_options_set_style (options, LIGMA_FILL_STYLE_SOLID);
  ligma_context_set_foreground (LIGMA_CONTEXT (options), color);

  ligma_display_shell_dnd_fill (shell, options,
                               C_("undo-type", "Drop color to layer"));

  g_object_unref (options);
}

static void
ligma_display_shell_drop_buffer (GtkWidget    *widget,
                                gint          drop_x,
                                gint          drop_y,
                                LigmaViewable *viewable,
                                gpointer      data)
{
  LigmaDisplayShell *shell    = LIGMA_DISPLAY_SHELL (data);
  LigmaImage        *image    = ligma_display_get_image (shell->display);
  LigmaContext      *context;
  GList            *drawables;
  LigmaBuffer       *buffer;
  LigmaPasteType     paste_type;
  gint              x, y, width, height;

  LIGMA_LOG (DND, NULL);

  if (shell->display->ligma->busy)
    return;

  if (! image)
    {
      image = ligma_image_new_from_buffer (shell->display->ligma,
                                          LIGMA_BUFFER (viewable));
      ligma_create_display (image->ligma, image, LIGMA_UNIT_PIXEL, 1.0,
                           G_OBJECT (ligma_widget_get_monitor (widget)));
      g_object_unref (image);

      return;
    }

  paste_type = LIGMA_PASTE_TYPE_NEW_LAYER_OR_FLOATING;
  drawables  = ligma_image_get_selected_drawables (image);
  context    = ligma_get_user_context (shell->display->ligma);
  buffer     = LIGMA_BUFFER (viewable);

  ligma_display_shell_untransform_viewport (
    shell,
    ! ligma_display_shell_get_infinite_canvas (shell),
    &x, &y, &width, &height);

  /* FIXME: popup a menu for selecting "Paste Into" */

  g_list_free (ligma_edit_paste (image, drawables, LIGMA_OBJECT (buffer),
                                paste_type, context, FALSE,
                                x, y, width, height));

  g_list_free (drawables);
  ligma_display_shell_dnd_flush (shell, image);
}

static void
ligma_display_shell_drop_uri_list (GtkWidget *widget,
                                  gint       x,
                                  gint       y,
                                  GList     *uri_list,
                                  gpointer   data)
{
  LigmaDisplayShell *shell   = LIGMA_DISPLAY_SHELL (data);
  LigmaImage        *image;
  LigmaContext      *context;
  GList            *list;
  gboolean          open_as_layers;

  /* If the app is already being torn down, shell->display might be
   * NULL here.  Play it safe.
   */
  if (! shell->display)
    return;

  image = ligma_display_get_image (shell->display);
  context = ligma_get_user_context (shell->display->ligma);

  LIGMA_LOG (DND, NULL);

  open_as_layers = (image != NULL);

  if (image)
    g_object_ref (image);

  for (list = uri_list; list; list = g_list_next (list))
    {
      GFile             *file  = g_file_new_for_uri (list->data);
      LigmaPDBStatusType  status;
      GError            *error = NULL;
      gboolean           warn  = FALSE;

      if (! shell->display)
        {
          /* It seems as if LIGMA is being torn down for quitting. Bail out. */
          g_object_unref (file);
          g_clear_object (&image);
          return;
        }

      if (open_as_layers)
        {
          GList *new_layers;

          new_layers = file_open_layers (shell->display->ligma, context,
                                         LIGMA_PROGRESS (shell->display),
                                         image, FALSE,
                                         file, LIGMA_RUN_INTERACTIVE, NULL,
                                         &status, &error);

          if (new_layers)
            {
              gint x      = 0;
              gint y      = 0;
              gint width  = ligma_image_get_width  (image);
              gint height = ligma_image_get_height (image);

              if (ligma_display_get_image (shell->display))
                {
                  ligma_display_shell_untransform_viewport (
                    shell,
                    ! ligma_display_shell_get_infinite_canvas (shell),
                    &x, &y, &width, &height);
                }

              ligma_image_add_layers (image, new_layers,
                                     LIGMA_IMAGE_ACTIVE_PARENT, -1,
                                     x, y, width, height,
                                     _("Drop layers"));

              g_list_free (new_layers);
            }
          else if (status != LIGMA_PDB_CANCEL && status != LIGMA_PDB_SUCCESS)
            {
              warn = TRUE;
            }
        }
      else if (ligma_display_get_image (shell->display))
        {
          /*  open any subsequent images in a new display  */
          LigmaImage *new_image;

          new_image = file_open_with_display (shell->display->ligma, context,
                                              NULL,
                                              file, FALSE,
                                              G_OBJECT (ligma_widget_get_monitor (widget)),
                                              &status, &error);

          if (! new_image && status != LIGMA_PDB_CANCEL && status != LIGMA_PDB_SUCCESS)
            warn = TRUE;
        }
      else
        {
          /*  open the first image in the empty display  */
          image = file_open_with_display (shell->display->ligma, context,
                                          LIGMA_PROGRESS (shell->display),
                                          file, FALSE,
                                          G_OBJECT (ligma_widget_get_monitor (widget)),
                                          &status, &error);

          if (image)
            {
              g_object_ref (image);
            }
          else if (status != LIGMA_PDB_CANCEL && status != LIGMA_PDB_SUCCESS)
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
          ligma_message (shell->display->ligma, G_OBJECT (shell->display),
                        LIGMA_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        ligma_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }

  if (image)
    ligma_display_shell_dnd_flush (shell, image);

  g_clear_object (&image);
}

static void
ligma_display_shell_drop_component (GtkWidget       *widget,
                                   gint             x,
                                   gint             y,
                                   LigmaImage       *image,
                                   LigmaChannelType  component,
                                   gpointer         data)
{
  LigmaDisplayShell *shell      = LIGMA_DISPLAY_SHELL (data);
  LigmaImage        *dest_image = ligma_display_get_image (shell->display);
  LigmaChannel      *channel;
  LigmaItem         *new_item;
  const gchar      *desc;

  LIGMA_LOG (DND, NULL);

  if (shell->display->ligma->busy)
    return;

  if (! dest_image)
    {
      dest_image = ligma_image_new_from_component (image->ligma,
                                                  image, component);
      ligma_create_display (dest_image->ligma, dest_image, LIGMA_UNIT_PIXEL, 1.0,
                           G_OBJECT (ligma_widget_get_monitor (widget)));
      g_object_unref (dest_image);

      return;
    }

  channel = ligma_channel_new_from_component (image, component, NULL, NULL);

  new_item = ligma_item_convert (LIGMA_ITEM (channel),
                                dest_image, LIGMA_TYPE_LAYER);
  g_object_unref (channel);

  if (new_item)
    {
      LigmaLayer *new_layer = LIGMA_LAYER (new_item);

      ligma_enum_get_value (LIGMA_TYPE_CHANNEL_TYPE, component,
                           NULL, NULL, &desc, NULL);
      ligma_object_take_name (LIGMA_OBJECT (new_layer),
                             g_strdup_printf (_("%s Channel Copy"), desc));

      ligma_image_undo_group_start (dest_image, LIGMA_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Layer"));

      ligma_display_shell_dnd_position_item (shell, image, new_item);

      ligma_image_add_layer (dest_image, new_layer,
                            LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);

      ligma_image_undo_group_end (dest_image);

      ligma_display_shell_dnd_flush (shell, dest_image);
    }
}

static void
ligma_display_shell_drop_pixbuf (GtkWidget *widget,
                                gint       x,
                                gint       y,
                                GdkPixbuf *pixbuf,
                                gpointer   data)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (data);
  LigmaImage        *image = ligma_display_get_image (shell->display);
  LigmaLayer        *new_layer;
  gboolean          has_alpha = FALSE;

  LIGMA_LOG (DND, NULL);

  if (shell->display->ligma->busy)
    return;

  if (! image)
    {
      image = ligma_image_new_from_pixbuf (shell->display->ligma, pixbuf,
                                          _("Dropped Buffer"));
      ligma_create_display (image->ligma, image, LIGMA_UNIT_PIXEL, 1.0,
                           G_OBJECT (ligma_widget_get_monitor (widget)));
      g_object_unref (image);

      return;
    }

  if (gdk_pixbuf_get_n_channels (pixbuf) == 2 ||
      gdk_pixbuf_get_n_channels (pixbuf) == 4)
    {
      has_alpha = TRUE;
    }

  new_layer =
    ligma_layer_new_from_pixbuf (pixbuf, image,
                                ligma_image_get_layer_format (image, has_alpha),
                                _("Dropped Buffer"),
                                LIGMA_OPACITY_OPAQUE,
                                ligma_image_get_default_new_layer_mode (image));

  if (new_layer)
    {
      LigmaItem *new_item = LIGMA_ITEM (new_layer);

      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_EDIT_PASTE,
                                   _("Drop New Layer"));

      ligma_display_shell_dnd_position_item (shell, image, new_item);

      ligma_image_add_layer (image, new_layer,
                            LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);

      ligma_image_undo_group_end (image);

      ligma_display_shell_dnd_flush (shell, image);
    }
}
