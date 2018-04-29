/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerpopup.c
 * Copyright (C) 2003-2014 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpviewable.h"

#include "gimpcontainerbox.h"
#include "gimpcontainereditor.h"
#include "gimpcontainerpopup.h"
#include "gimpcontainertreeview.h"
#include "gimpcontainerview.h"
#include "gimpdialogfactory.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"
#include "gimpwindowstrategy.h"

#include "gimp-intl.h"


static void   gimp_container_popup_finalize         (GObject            *object);

static void   gimp_container_popup_confirm          (GimpPopup          *popup);

static void   gimp_container_popup_create_view      (GimpContainerPopup *popup);

static void   gimp_container_popup_smaller_clicked  (GtkWidget          *button,
                                                     GimpContainerPopup *popup);
static void   gimp_container_popup_larger_clicked   (GtkWidget          *button,
                                                     GimpContainerPopup *popup);
static void   gimp_container_popup_view_type_toggled(GtkWidget          *button,
                                                     GimpContainerPopup *popup);
static void   gimp_container_popup_dialog_clicked   (GtkWidget          *button,
                                                     GimpContainerPopup *popup);


G_DEFINE_TYPE (GimpContainerPopup, gimp_container_popup, GIMP_TYPE_POPUP)

#define parent_class gimp_container_popup_parent_class


static void
gimp_container_popup_class_init (GimpContainerPopupClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GimpPopupClass *popup_class  = GIMP_POPUP_CLASS (klass);

  object_class->finalize = gimp_container_popup_finalize;

  popup_class->confirm   = gimp_container_popup_confirm;
}

static void
gimp_container_popup_init (GimpContainerPopup *popup)
{
  popup->view_type         = GIMP_VIEW_TYPE_LIST;
  popup->default_view_size = GIMP_VIEW_SIZE_SMALL;
  popup->view_size         = GIMP_VIEW_SIZE_SMALL;
  popup->view_border_width = 1;

  popup->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (popup->frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (popup), popup->frame);
  gtk_widget_show (popup->frame);
}

static void
gimp_container_popup_finalize (GObject *object)
{
  GimpContainerPopup *popup = GIMP_CONTAINER_POPUP (object);

  g_clear_object (&popup->context);

  g_clear_pointer (&popup->dialog_identifier, g_free);
  g_clear_pointer (&popup->dialog_icon_name,  g_free);
  g_clear_pointer (&popup->dialog_tooltip,    g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_container_popup_confirm (GimpPopup *popup)
{
  GimpContainerPopup *c_popup = GIMP_CONTAINER_POPUP (popup);
  GimpObject         *object;

  object = gimp_context_get_by_type (c_popup->context,
                                     gimp_container_get_children_type (c_popup->container));
  gimp_context_set_by_type (c_popup->orig_context,
                            gimp_container_get_children_type (c_popup->container),
                            object);

  GIMP_POPUP_CLASS (parent_class)->confirm (popup);
}

static void
gimp_container_popup_context_changed (GimpContext        *context,
                                      GimpViewable       *viewable,
                                      GimpContainerPopup *popup)
{
  GdkEvent *current_event;
  gboolean  confirm = FALSE;

  current_event = gtk_get_current_event ();

  if (current_event)
    {
      if (((GdkEventAny *) current_event)->type == GDK_BUTTON_PRESS ||
          ((GdkEventAny *) current_event)->type == GDK_BUTTON_RELEASE)
        confirm = TRUE;

      gdk_event_free (current_event);
    }

  if (confirm)
    g_signal_emit_by_name (popup, "confirm");
}

GtkWidget *
gimp_container_popup_new (GimpContainer     *container,
                          GimpContext       *context,
                          GimpViewType       view_type,
                          gint               default_view_size,
                          gint               view_size,
                          gint               view_border_width,
                          GimpDialogFactory *dialog_factory,
                          const gchar       *dialog_identifier,
                          const gchar       *dialog_icon_name,
                          const gchar       *dialog_tooltip)
{
  GimpContainerPopup *popup;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (default_view_size >  0 &&
                        default_view_size <= GIMP_VIEWABLE_MAX_POPUP_SIZE,
                        NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_POPUP_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  if (dialog_factory)
    {
      g_return_val_if_fail (dialog_identifier != NULL, NULL);
      g_return_val_if_fail (dialog_icon_name != NULL, NULL);
      g_return_val_if_fail (dialog_tooltip != NULL, NULL);
    }

  popup = g_object_new (GIMP_TYPE_CONTAINER_POPUP,
                        "type", GTK_WINDOW_POPUP,
                        NULL);
  gtk_window_set_resizable (GTK_WINDOW (popup), FALSE);

  popup->container    = container;
  popup->orig_context = context;
  popup->context      = gimp_context_new (context->gimp, "popup", context);

  popup->view_type         = view_type;
  popup->default_view_size = default_view_size;
  popup->view_size         = view_size;
  popup->view_border_width = view_border_width;

  g_signal_connect (popup->context,
                    gimp_context_type_to_signal_name (gimp_container_get_children_type (container)),
                    G_CALLBACK (gimp_container_popup_context_changed),
                    popup);

  if (dialog_factory)
    {
      popup->dialog_factory    = dialog_factory;
      popup->dialog_identifier = g_strdup (dialog_identifier);
      popup->dialog_icon_name  = g_strdup (dialog_icon_name);
      popup->dialog_tooltip    = g_strdup (dialog_tooltip);
    }

  gimp_container_popup_create_view (popup);

  return GTK_WIDGET (popup);
}

GimpViewType
gimp_container_popup_get_view_type (GimpContainerPopup *popup)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER_POPUP (popup), GIMP_VIEW_TYPE_LIST);

  return popup->view_type;
}

void
gimp_container_popup_set_view_type (GimpContainerPopup *popup,
                                    GimpViewType        view_type)
{
  g_return_if_fail (GIMP_IS_CONTAINER_POPUP (popup));

  if (view_type != popup->view_type)
    {
      popup->view_type = view_type;

      gtk_widget_destroy (GTK_WIDGET (popup->editor));
      gimp_container_popup_create_view (popup);
    }
}

gint
gimp_container_popup_get_view_size (GimpContainerPopup *popup)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER_POPUP (popup), GIMP_VIEW_SIZE_SMALL);

  return popup->view_size;
}

void
gimp_container_popup_set_view_size (GimpContainerPopup *popup,
                                    gint                view_size)
{
  GtkWidget     *scrolled_win;
  GtkWidget     *viewport;
  GtkAllocation  allocation;

  g_return_if_fail (GIMP_IS_CONTAINER_POPUP (popup));

  scrolled_win = GIMP_CONTAINER_BOX (popup->editor->view)->scrolled_win;
  viewport     = gtk_bin_get_child (GTK_BIN (scrolled_win));

  gtk_widget_get_allocation (viewport, &allocation);

  view_size = CLAMP (view_size, GIMP_VIEW_SIZE_TINY,
                     MIN (GIMP_VIEW_SIZE_GIGANTIC,
                          allocation.width - 2 * popup->view_border_width));

  if (view_size != popup->view_size)
    {
      popup->view_size = view_size;

      gimp_container_view_set_view_size (popup->editor->view,
                                         popup->view_size,
                                         popup->view_border_width);
    }
}


/*  private functions  */

static void
gimp_container_popup_create_view (GimpContainerPopup *popup)
{
  GimpEditor *editor;
  GtkWidget  *button;
  gint        rows;
  gint        columns;

  popup->editor = g_object_new (GIMP_TYPE_CONTAINER_EDITOR,
                                "view-type",         popup->view_type,
                                "container",         popup->container,
                                "context",           popup->context,
                                "view-size",         popup->view_size,
                                "view-border-width", popup->view_border_width,
                                NULL);

  gimp_container_view_set_reorderable (GIMP_CONTAINER_VIEW (popup->editor->view),
                                       FALSE);

  if (popup->view_type == GIMP_VIEW_TYPE_LIST)
    {
      GtkWidget *search_entry;

      search_entry = gtk_entry_new ();
      gtk_box_pack_end (GTK_BOX (popup->editor->view), search_entry,
                        FALSE, FALSE, 0);
      gtk_tree_view_set_search_entry (GTK_TREE_VIEW (GIMP_CONTAINER_TREE_VIEW (GIMP_CONTAINER_VIEW (popup->editor->view))->view),
                                      GTK_ENTRY (search_entry));
      gtk_widget_show (search_entry);
    }

  /* lame workaround for bug #761998 */
  if (popup->default_view_size >= GIMP_VIEW_SIZE_LARGE)
    {
      rows    = 6;
      columns = 6;
    }
  else
    {
      rows    = 10;
      columns = 6;
    }

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (popup->editor->view),
                                       columns * (popup->default_view_size +
                                                  2 * popup->view_border_width),
                                       rows    * (popup->default_view_size +
                                                  2 * popup->view_border_width));

  if (GIMP_IS_EDITOR (popup->editor->view))
    gimp_editor_set_show_name (GIMP_EDITOR (popup->editor->view), FALSE);

  gtk_container_add (GTK_CONTAINER (popup->frame), GTK_WIDGET (popup->editor));
  gtk_widget_show (GTK_WIDGET (popup->editor));

  editor = GIMP_EDITOR (popup->editor->view);

  gimp_editor_add_button (editor, "zoom-out",
                          _("Smaller Previews"), NULL,
                          G_CALLBACK (gimp_container_popup_smaller_clicked),
                          NULL,
                          popup);
  gimp_editor_add_button (editor, "zoom-in",
                          _("Larger Previews"), NULL,
                          G_CALLBACK (gimp_container_popup_larger_clicked),
                          NULL,
                          popup);

  button = gimp_editor_add_icon_box (editor, GIMP_TYPE_VIEW_TYPE, "gimp",
                                     G_CALLBACK (gimp_container_popup_view_type_toggled),
                                     popup);
  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button), popup->view_type);

  if (popup->dialog_factory)
    gimp_editor_add_button (editor, popup->dialog_icon_name,
                            popup->dialog_tooltip, NULL,
                            G_CALLBACK (gimp_container_popup_dialog_clicked),
                            NULL,
                            popup);

  gtk_widget_grab_focus (GTK_WIDGET (popup->editor));
}

static void
gimp_container_popup_smaller_clicked (GtkWidget          *button,
                                      GimpContainerPopup *popup)
{
  gint view_size;

  view_size = gimp_container_view_get_view_size (popup->editor->view, NULL);

  gimp_container_popup_set_view_size (popup, view_size * 0.8);
}

static void
gimp_container_popup_larger_clicked (GtkWidget          *button,
                                     GimpContainerPopup *popup)
{
  gint view_size;

  view_size = gimp_container_view_get_view_size (popup->editor->view, NULL);

  gimp_container_popup_set_view_size (popup, view_size * 1.2);
}

static void
gimp_container_popup_view_type_toggled (GtkWidget          *button,
                                        GimpContainerPopup *popup)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      GimpViewType view_type;

      view_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                                      "gimp-item-data"));

      gimp_container_popup_set_view_type (popup, view_type);
    }
}

static void
gimp_container_popup_dialog_clicked (GtkWidget          *button,
                                     GimpContainerPopup *popup)
{
  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (popup->context->gimp)),
                                             popup->context->gimp,
                                             popup->dialog_factory,
                                             gimp_widget_get_monitor (button),
                                             popup->dialog_identifier);
  g_signal_emit_by_name (popup, "confirm");
}
