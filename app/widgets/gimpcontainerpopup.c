/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainerpopup.c
 * Copyright (C) 2003-2014 Michael Natterer <mitch@ligma.org>
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
#include <gdk/gdkkeysyms.h>

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmacontainer.h"
#include "core/ligmaviewable.h"

#include "ligmacontainerbox.h"
#include "ligmacontainereditor.h"
#include "ligmacontainerpopup.h"
#include "ligmacontainertreeview.h"
#include "ligmacontainerview.h"
#include "ligmadialogfactory.h"
#include "ligmaviewrenderer.h"
#include "ligmawidgets-utils.h"
#include "ligmawindowstrategy.h"

#include "ligma-intl.h"


static void   ligma_container_popup_finalize         (GObject            *object);

static void   ligma_container_popup_confirm          (LigmaPopup          *popup);

static void   ligma_container_popup_create_view      (LigmaContainerPopup *popup);

static void   ligma_container_popup_smaller_clicked  (GtkWidget          *button,
                                                     LigmaContainerPopup *popup);
static void   ligma_container_popup_larger_clicked   (GtkWidget          *button,
                                                     LigmaContainerPopup *popup);
static void   ligma_container_popup_view_type_toggled(GtkWidget          *button,
                                                     LigmaContainerPopup *popup);
static void   ligma_container_popup_dialog_clicked   (GtkWidget          *button,
                                                     LigmaContainerPopup *popup);

static void   ligma_container_popup_context_changed  (LigmaContext        *context,
                                                     LigmaViewable       *viewable,
                                                     LigmaContainerPopup *popup);


G_DEFINE_TYPE (LigmaContainerPopup, ligma_container_popup, LIGMA_TYPE_POPUP)

#define parent_class ligma_container_popup_parent_class


static void
ligma_container_popup_class_init (LigmaContainerPopupClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  LigmaPopupClass *popup_class  = LIGMA_POPUP_CLASS (klass);

  object_class->finalize = ligma_container_popup_finalize;

  popup_class->confirm   = ligma_container_popup_confirm;
}

static void
ligma_container_popup_init (LigmaContainerPopup *popup)
{
  popup->view_type         = LIGMA_VIEW_TYPE_LIST;
  popup->default_view_size = LIGMA_VIEW_SIZE_SMALL;
  popup->view_size         = LIGMA_VIEW_SIZE_SMALL;
  popup->view_border_width = 1;

  popup->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (popup->frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (popup), popup->frame);
  gtk_widget_show (popup->frame);
}

static void
ligma_container_popup_finalize (GObject *object)
{
  LigmaContainerPopup *popup = LIGMA_CONTAINER_POPUP (object);

  g_clear_object (&popup->context);

  g_clear_pointer (&popup->dialog_identifier, g_free);
  g_clear_pointer (&popup->dialog_icon_name,  g_free);
  g_clear_pointer (&popup->dialog_tooltip,    g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_container_popup_confirm (LigmaPopup *popup)
{
  LigmaContainerPopup *c_popup = LIGMA_CONTAINER_POPUP (popup);
  LigmaObject         *object;

  object = ligma_context_get_by_type (c_popup->context,
                                     ligma_container_get_children_type (c_popup->container));
  ligma_context_set_by_type (c_popup->orig_context,
                            ligma_container_get_children_type (c_popup->container),
                            object);

  LIGMA_POPUP_CLASS (parent_class)->confirm (popup);
}

GtkWidget *
ligma_container_popup_new (LigmaContainer     *container,
                          LigmaContext       *context,
                          LigmaViewType       view_type,
                          gint               default_view_size,
                          gint               view_size,
                          gint               view_border_width,
                          LigmaDialogFactory *dialog_factory,
                          const gchar       *dialog_identifier,
                          const gchar       *dialog_icon_name,
                          const gchar       *dialog_tooltip)
{
  LigmaContainerPopup *popup;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (default_view_size >  0 &&
                        default_view_size <= LIGMA_VIEWABLE_MAX_POPUP_SIZE,
                        NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_POPUP_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= LIGMA_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        LIGMA_IS_DIALOG_FACTORY (dialog_factory), NULL);
  if (dialog_factory)
    {
      g_return_val_if_fail (dialog_identifier != NULL, NULL);
      g_return_val_if_fail (dialog_icon_name != NULL, NULL);
      g_return_val_if_fail (dialog_tooltip != NULL, NULL);
    }

  popup = g_object_new (LIGMA_TYPE_CONTAINER_POPUP,
                        "type", GTK_WINDOW_POPUP,
                        NULL);
  gtk_window_set_resizable (GTK_WINDOW (popup), FALSE);

  popup->container    = container;
  popup->orig_context = context;
  popup->context      = ligma_context_new (context->ligma, "popup", context);

  popup->view_type         = view_type;
  popup->default_view_size = default_view_size;
  popup->view_size         = view_size;
  popup->view_border_width = view_border_width;

  g_signal_connect (popup->context,
                    ligma_context_type_to_signal_name (ligma_container_get_children_type (container)),
                    G_CALLBACK (ligma_container_popup_context_changed),
                    popup);

  if (dialog_factory)
    {
      popup->dialog_factory    = dialog_factory;
      popup->dialog_identifier = g_strdup (dialog_identifier);
      popup->dialog_icon_name  = g_strdup (dialog_icon_name);
      popup->dialog_tooltip    = g_strdup (dialog_tooltip);
    }

  ligma_container_popup_create_view (popup);

  return GTK_WIDGET (popup);
}

LigmaViewType
ligma_container_popup_get_view_type (LigmaContainerPopup *popup)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER_POPUP (popup), LIGMA_VIEW_TYPE_LIST);

  return popup->view_type;
}

void
ligma_container_popup_set_view_type (LigmaContainerPopup *popup,
                                    LigmaViewType        view_type)
{
  g_return_if_fail (LIGMA_IS_CONTAINER_POPUP (popup));

  if (view_type != popup->view_type)
    {
      popup->view_type = view_type;

      gtk_widget_destroy (GTK_WIDGET (popup->editor));
      ligma_container_popup_create_view (popup);
    }
}

gint
ligma_container_popup_get_view_size (LigmaContainerPopup *popup)
{
  g_return_val_if_fail (LIGMA_IS_CONTAINER_POPUP (popup), LIGMA_VIEW_SIZE_SMALL);

  return popup->view_size;
}

void
ligma_container_popup_set_view_size (LigmaContainerPopup *popup,
                                    gint                view_size)
{
  GtkWidget     *scrolled_win;
  GtkWidget     *viewport;
  GtkAllocation  allocation;

  g_return_if_fail (LIGMA_IS_CONTAINER_POPUP (popup));

  scrolled_win = LIGMA_CONTAINER_BOX (popup->editor->view)->scrolled_win;
  viewport     = gtk_bin_get_child (GTK_BIN (scrolled_win));

  gtk_widget_get_allocation (viewport, &allocation);

  view_size = CLAMP (view_size, LIGMA_VIEW_SIZE_TINY,
                     MIN (LIGMA_VIEW_SIZE_GIGANTIC,
                          allocation.width - 2 * popup->view_border_width));

  if (view_size != popup->view_size)
    {
      popup->view_size = view_size;

      ligma_container_view_set_view_size (popup->editor->view,
                                         popup->view_size,
                                         popup->view_border_width);
    }
}


/*  private functions  */

static void
ligma_container_popup_create_view (LigmaContainerPopup *popup)
{
  LigmaEditor  *editor;
  GtkWidget   *button;
  const gchar *signal_name;
  GType        children_type;
  gint         rows;
  gint         columns;

  popup->editor = g_object_new (LIGMA_TYPE_CONTAINER_EDITOR,
                                "view-type",         popup->view_type,
                                "container",         popup->container,
                                "context",           popup->context,
                                "view-size",         popup->view_size,
                                "view-border-width", popup->view_border_width,
                                NULL);

  ligma_container_view_set_reorderable (LIGMA_CONTAINER_VIEW (popup->editor->view),
                                       FALSE);

  if (popup->view_type == LIGMA_VIEW_TYPE_LIST)
    {
      GtkWidget *search_entry;

      search_entry = gtk_entry_new ();
      gtk_box_pack_end (GTK_BOX (popup->editor->view), search_entry,
                        FALSE, FALSE, 0);
      gtk_tree_view_set_search_entry (GTK_TREE_VIEW (LIGMA_CONTAINER_TREE_VIEW (LIGMA_CONTAINER_VIEW (popup->editor->view))->view),
                                      GTK_ENTRY (search_entry));
      gtk_widget_show (search_entry);
    }

  /* lame workaround for bug #761998 */
  if (popup->default_view_size >= LIGMA_VIEW_SIZE_LARGE)
    {
      rows    = 6;
      columns = 6;
    }
  else
    {
      rows    = 10;
      columns = 6;
    }

  ligma_container_box_set_size_request (LIGMA_CONTAINER_BOX (popup->editor->view),
                                       columns * (popup->default_view_size +
                                                  2 * popup->view_border_width),
                                       rows    * (popup->default_view_size +
                                                  2 * popup->view_border_width));

  if (LIGMA_IS_EDITOR (popup->editor->view))
    ligma_editor_set_show_name (LIGMA_EDITOR (popup->editor->view), FALSE);

  gtk_container_add (GTK_CONTAINER (popup->frame), GTK_WIDGET (popup->editor));
  gtk_widget_show (GTK_WIDGET (popup->editor));

  editor = LIGMA_EDITOR (popup->editor->view);

  ligma_editor_add_button (editor, "zoom-out",
                          _("Smaller Previews"), NULL,
                          G_CALLBACK (ligma_container_popup_smaller_clicked),
                          NULL,
                          popup);
  ligma_editor_add_button (editor, "zoom-in",
                          _("Larger Previews"), NULL,
                          G_CALLBACK (ligma_container_popup_larger_clicked),
                          NULL,
                          popup);

  button = ligma_editor_add_icon_box (editor, LIGMA_TYPE_VIEW_TYPE, "ligma",
                                     G_CALLBACK (ligma_container_popup_view_type_toggled),
                                     popup);
  ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (button), popup->view_type);

  if (popup->dialog_factory)
    ligma_editor_add_button (editor, popup->dialog_icon_name,
                            popup->dialog_tooltip, NULL,
                            G_CALLBACK (ligma_container_popup_dialog_clicked),
                            NULL,
                            popup);

  gtk_widget_grab_focus (GTK_WIDGET (popup->editor));

  /* Special-casing the object types managed by the context to make sure
   * the right items are selected when opening the popup.
   */
  children_type = ligma_container_get_children_type (popup->container);
  signal_name   = ligma_context_type_to_signal_name (children_type);

  if (signal_name)
    {
      LigmaObject *object;
      GList      *items = NULL;

      object = ligma_context_get_by_type (popup->orig_context, children_type);
      if (object)
        items = g_list_prepend (NULL, object);

      ligma_container_view_select_items (popup->editor->view, items);

      g_list_free (items);
    }
}

static void
ligma_container_popup_smaller_clicked (GtkWidget          *button,
                                      LigmaContainerPopup *popup)
{
  gint view_size;

  view_size = ligma_container_view_get_view_size (popup->editor->view, NULL);

  ligma_container_popup_set_view_size (popup, view_size * 0.8);
}

static void
ligma_container_popup_larger_clicked (GtkWidget          *button,
                                     LigmaContainerPopup *popup)
{
  gint view_size;

  view_size = ligma_container_view_get_view_size (popup->editor->view, NULL);

  ligma_container_popup_set_view_size (popup, view_size * 1.2);
}

static void
ligma_container_popup_view_type_toggled (GtkWidget          *button,
                                        LigmaContainerPopup *popup)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      LigmaViewType view_type;

      view_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                                      "ligma-item-data"));

      ligma_container_popup_set_view_type (popup, view_type);
    }
}

static void
ligma_container_popup_dialog_clicked (GtkWidget          *button,
                                     LigmaContainerPopup *popup)
{
  ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (popup->context->ligma)),
                                             popup->context->ligma,
                                             popup->dialog_factory,
                                             ligma_widget_get_monitor (button),
                                             popup->dialog_identifier);
  g_signal_emit_by_name (popup, "confirm");
}

static void
ligma_container_popup_context_changed (LigmaContext        *context,
                                      LigmaViewable       *viewable,
                                      LigmaContainerPopup *popup)
{
  GdkEvent  *current_event;
  GtkWidget *current_widget = GTK_WIDGET (popup);
  gboolean   confirm        = FALSE;

  current_event = gtk_get_current_event ();

  if (current_event && gtk_widget_get_window (current_widget))
    {
      GdkWindow *event_window = gdk_window_get_effective_toplevel (((GdkEventAny *) current_event)->window);
      GdkWindow *popup_window = gdk_window_get_effective_toplevel (gtk_widget_get_window (current_widget));

      /* We need to differentiate a context change as a consequence of
       * an event on another widget.
       */
      if ((((GdkEventAny *) current_event)->type == GDK_BUTTON_PRESS ||
           ((GdkEventAny *) current_event)->type == GDK_BUTTON_RELEASE) &&
          event_window == popup_window)
        confirm = TRUE;

      gdk_event_free (current_event);
    }

  if (confirm)
    g_signal_emit_by_name (popup, "confirm");
}
