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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"

#include "widgets/gimpview.h"
#include "widgets/gimpviewrenderer.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-layer-select.h"

#include "gimp-intl.h"


typedef struct
{
  GtkWidget *window;
  GtkWidget *view;
  GtkWidget *label;

  GimpImage *image;
  GimpLayer *orig_layer;
} LayerSelect;


/*  local function prototypes  */

static LayerSelect * layer_select_new       (GimpDisplayShell *shell,
                                             GimpImage        *image,
                                             GimpLayer        *layer,
                                             gint              view_size);
static void          layer_select_destroy   (LayerSelect      *layer_select,
                                             guint32           time);
static void          layer_select_advance   (LayerSelect      *layer_select,
                                             gint              move);
static gboolean      layer_select_events    (GtkWidget        *widget,
                                             GdkEvent         *event,
                                             LayerSelect      *layer_select);


/*  public functions  */

void
gimp_display_shell_layer_select_init (GimpDisplayShell *shell,
                                      gint              move,
                                      guint32           time)
{
  LayerSelect   *layer_select;
  GimpImage     *image;
  GimpLayer     *layer;
  GdkGrabStatus  status;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  layer = gimp_image_get_active_layer (image);

  if (! layer)
    return;

  layer_select = layer_select_new (shell, image, layer,
                                   image->gimp->config->layer_preview_size);
  layer_select_advance (layer_select, move);

  gtk_window_set_screen (GTK_WINDOW (layer_select->window),
                         gtk_widget_get_screen (GTK_WIDGET (shell)));

  gtk_widget_show (layer_select->window);

  status = gdk_keyboard_grab (gtk_widget_get_window (layer_select->window), FALSE, time);
  if (status != GDK_GRAB_SUCCESS)
    g_printerr ("gdk_keyboard_grab failed with status %d\n", status);
}


/*  private functions  */

static LayerSelect *
layer_select_new (GimpDisplayShell *shell,
                  GimpImage        *image,
                  GimpLayer        *layer,
                  gint              view_size)
{
  LayerSelect *layer_select;
  GtkWidget   *frame1;
  GtkWidget   *frame2;
  GtkWidget   *hbox;

  layer_select = g_slice_new0 (LayerSelect);

  layer_select->image      = image;
  layer_select->orig_layer = layer;

  layer_select->window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_role (GTK_WINDOW (layer_select->window), "gimp-layer-select");
  gtk_window_set_title (GTK_WINDOW (layer_select->window), _("Layer Select"));
  gtk_window_set_position (GTK_WINDOW (layer_select->window), GTK_WIN_POS_MOUSE);
  gtk_widget_set_events (layer_select->window,
                         GDK_KEY_PRESS_MASK   |
                         GDK_KEY_RELEASE_MASK |
                         GDK_BUTTON_PRESS_MASK);

  g_signal_connect (layer_select->window, "event",
                    G_CALLBACK (layer_select_events),
                    layer_select);

  frame1 = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame1), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (layer_select->window), frame1);
  gtk_widget_show (frame1);

  frame2 = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (frame1), frame2);
  gtk_widget_show (frame2);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (frame2), hbox);
  gtk_widget_show (hbox);

  /*  the view  */
  layer_select->view =
    gimp_view_new_by_types (gimp_get_user_context (image->gimp),
                            GIMP_TYPE_VIEW,
                            GIMP_TYPE_LAYER,
                            view_size, 1, FALSE);
  gimp_view_renderer_set_color_config (GIMP_VIEW (layer_select->view)->renderer,
                                       gimp_display_shell_get_color_config (shell));
  gimp_view_set_viewable (GIMP_VIEW (layer_select->view),
                          GIMP_VIEWABLE (layer));
  gtk_box_pack_start (GTK_BOX (hbox), layer_select->view, FALSE, FALSE, 0);
  gtk_widget_show (layer_select->view);

  /*  the layer name label */
  layer_select->label = gtk_label_new (gimp_object_get_name (layer));
  gtk_box_pack_start (GTK_BOX (hbox), layer_select->label, FALSE, FALSE, 0);
  gtk_widget_show (layer_select->label);

  return layer_select;
}

static void
layer_select_destroy (LayerSelect *layer_select,
                      guint32      time)
{
  gdk_display_keyboard_ungrab (gtk_widget_get_display (layer_select->window),
                                                       time);

  gtk_widget_destroy (layer_select->window);

  if (layer_select->orig_layer !=
      gimp_image_get_active_layer (layer_select->image))
    {
      gimp_image_flush (layer_select->image);
    }

  g_slice_free (LayerSelect, layer_select);
}

static void
layer_select_advance (LayerSelect *layer_select,
                      gint         move)
{
  GimpLayer *active_layer;
  GimpLayer *next_layer;
  GList     *layers;
  gint       n_layers;
  gint       index;

  if (move == 0)
    return;

  /*  If there is a floating selection, allow no advancement  */
  if (gimp_image_get_floating_selection (layer_select->image))
    return;

  active_layer = gimp_image_get_active_layer (layer_select->image);

  layers   = gimp_image_get_layer_list (layer_select->image);
  n_layers = g_list_length (layers);

  index = g_list_index (layers, active_layer);
  index += move;

  if (index < 0)
    index = n_layers - 1;
  else if (index >= n_layers)
    index = 0;

  next_layer = g_list_nth_data (layers, index);

  g_list_free (layers);

  if (next_layer && next_layer != active_layer)
    {
      active_layer = gimp_image_set_active_layer (layer_select->image,
                                                  next_layer);

      if (active_layer)
        {
          gimp_view_set_viewable (GIMP_VIEW (layer_select->view),
                                  GIMP_VIEWABLE (active_layer));
          gtk_label_set_text (GTK_LABEL (layer_select->label),
                              gimp_object_get_name (active_layer));
        }
    }
}

static gboolean
layer_select_events (GtkWidget   *widget,
                     GdkEvent    *event,
                     LayerSelect *layer_select)
{
  GdkEventKey    *kevent;
  GdkEventButton *bevent;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      layer_select_destroy (layer_select, bevent->time);
      break;

    case GDK_KEY_PRESS:
      kevent = (GdkEventKey *) event;

      switch (kevent->keyval)
        {
        case GDK_KEY_Tab:
          layer_select_advance (layer_select, 1);
          break;
        case GDK_KEY_ISO_Left_Tab:
          layer_select_advance (layer_select, -1);
          break;
        }
      return TRUE;
      break;

    case GDK_KEY_RELEASE:
      kevent = (GdkEventKey *) event;

      switch (kevent->keyval)
        {
        case GDK_KEY_Alt_L: case GDK_KEY_Alt_R:
          kevent->state &= ~GDK_MOD1_MASK;
          break;
        case GDK_KEY_Control_L: case GDK_KEY_Control_R:
          kevent->state &= ~GDK_CONTROL_MASK;
          break;
        case GDK_KEY_Shift_L: case GDK_KEY_Shift_R:
          kevent->state &= ~GDK_SHIFT_MASK;
          break;
        }

      if (! (kevent->state & GDK_CONTROL_MASK))
        layer_select_destroy (layer_select, kevent->time);

      return TRUE;
      break;

    default:
      break;
    }

  return FALSE;
}
