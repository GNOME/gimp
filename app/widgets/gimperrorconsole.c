/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmaerrorconsole.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
 *
 * partly based on errorconsole.c
 * Copyright (C) 1998 Nick Fetchak <nuke@bayside.net>
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"

#include "ligmadocked.h"
#include "ligmaerrorconsole.h"
#include "ligmamenufactory.h"
#include "ligmasessioninfo-aux.h"
#include "ligmatextbuffer.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


static const gboolean default_highlight[] =
{
  [LIGMA_MESSAGE_ERROR]   = TRUE,
  [LIGMA_MESSAGE_WARNING] = TRUE,
  [LIGMA_MESSAGE_INFO]    = FALSE
};


static void       ligma_error_console_docked_iface_init (LigmaDockedInterface *iface);

static void       ligma_error_console_constructed       (GObject          *object);
static void       ligma_error_console_dispose           (GObject          *object);

static void       ligma_error_console_unmap             (GtkWidget        *widget);

static gboolean   ligma_error_console_button_press      (GtkWidget        *widget,
                                                        GdkEventButton   *event,
                                                        LigmaErrorConsole *console);

static void       ligma_error_console_set_aux_info      (LigmaDocked       *docked,
                                                        GList            *aux_info);
static GList    * ligma_error_console_get_aux_info      (LigmaDocked       *docked);


G_DEFINE_TYPE_WITH_CODE (LigmaErrorConsole, ligma_error_console, LIGMA_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_error_console_docked_iface_init))

#define parent_class ligma_error_console_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_error_console_class_init (LigmaErrorConsoleClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = ligma_error_console_constructed;
  object_class->dispose     = ligma_error_console_dispose;

  widget_class->unmap       = ligma_error_console_unmap;
}

static void
ligma_error_console_init (LigmaErrorConsole *console)
{
  GtkWidget *scrolled_window;

  console->text_buffer = GTK_TEXT_BUFFER (ligma_text_buffer_new ());

  gtk_text_buffer_create_tag (console->text_buffer, "title",
                              "scale",  PANGO_SCALE_LARGE,
                              "weight", PANGO_WEIGHT_BOLD,
                              NULL);
  gtk_text_buffer_create_tag (console->text_buffer, "message",
                              NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (console), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  console->text_view = gtk_text_view_new_with_buffer (console->text_buffer);
  g_object_unref (console->text_buffer);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (console->text_view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (console->text_view),
                               GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), console->text_view);
  gtk_widget_show (console->text_view);

  g_signal_connect (console->text_view, "button-press-event",
                    G_CALLBACK (ligma_error_console_button_press),
                    console);

  console->file_dialog = NULL;

  memcpy (console->highlight, default_highlight, sizeof (default_highlight));
}

static void
ligma_error_console_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_aux_info = ligma_error_console_set_aux_info;
  iface->get_aux_info = ligma_error_console_get_aux_info;
}

static void
ligma_error_console_constructed (GObject *object)
{
  LigmaErrorConsole *console = LIGMA_ERROR_CONSOLE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  console->clear_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (console), "error-console",
                                   "error-console-clear", NULL);

  console->save_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (console), "error-console",
                                   "error-console-save-all",
                                   "error-console-save-selection",
                                   GDK_SHIFT_MASK,
                                   NULL);
}

static void
ligma_error_console_dispose (GObject *object)
{
  LigmaErrorConsole *console = LIGMA_ERROR_CONSOLE (object);

  if (console->file_dialog)
    gtk_widget_destroy (console->file_dialog);

  console->ligma->message_handler = LIGMA_MESSAGE_BOX;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_error_console_unmap (GtkWidget *widget)
{
  LigmaErrorConsole *console = LIGMA_ERROR_CONSOLE (widget);

  if (console->file_dialog)
    gtk_widget_destroy (console->file_dialog);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

GtkWidget *
ligma_error_console_new (Ligma            *ligma,
                        LigmaMenuFactory *menu_factory)
{
  LigmaErrorConsole *console;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  console = g_object_new (LIGMA_TYPE_ERROR_CONSOLE,
                          "menu-factory",   menu_factory,
                          "menu-identifier", "<ErrorConsole>",
                          "ui-path",        "/error-console-popup",
                          NULL);

  console->ligma = ligma;

  console->ligma->message_handler = LIGMA_ERROR_CONSOLE;

  return GTK_WIDGET (console);
}

void
ligma_error_console_add (LigmaErrorConsole    *console,
                        LigmaMessageSeverity  severity,
                        const gchar         *domain,
                        const gchar         *message)
{
  const gchar        *desc;
  GtkTextIter         end;
  GtkTextMark        *end_mark;
  GtkTextChildAnchor *anchor;
  GtkWidget          *image;
  gchar              *str;

  g_return_if_fail (LIGMA_IS_ERROR_CONSOLE (console));
  g_return_if_fail (domain != NULL);
  g_return_if_fail (message != NULL);

  ligma_enum_get_value (LIGMA_TYPE_MESSAGE_SEVERITY, severity,
                       NULL, NULL, &desc, NULL);

  gtk_text_buffer_get_end_iter (console->text_buffer, &end);

  anchor = gtk_text_child_anchor_new ();
  gtk_text_buffer_insert_child_anchor (console->text_buffer, &end, anchor);

  image = gtk_image_new_from_icon_name (ligma_get_message_icon_name (severity),
                                        GTK_ICON_SIZE_BUTTON);
  gtk_text_view_add_child_at_anchor (GTK_TEXT_VIEW (console->text_view),
                                     image, anchor);
  gtk_widget_show (image);

  g_object_unref (anchor);

  gtk_text_buffer_insert (console->text_buffer, &end, "  ", -1);

  str = g_strdup_printf ("%s %s", domain, desc);
  gtk_text_buffer_insert_with_tags_by_name (console->text_buffer, &end,
                                            str, -1,
                                            "title",
                                            NULL);
  g_free (str);

  gtk_text_buffer_insert (console->text_buffer, &end, "\n", -1);

  gtk_text_buffer_insert_with_tags_by_name (console->text_buffer, &end,
                                            message, -1,
                                            "message",
                                            NULL);

  gtk_text_buffer_insert (console->text_buffer, &end, "\n\n", -1);

  end_mark = gtk_text_buffer_create_mark (console->text_buffer,
                                          NULL, &end, TRUE);
  gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (console->text_view), end_mark,
                                FALSE, TRUE, 1.0, 0.0);
  gtk_text_buffer_delete_mark (console->text_buffer, end_mark);
}


/*  private functions  */

static gboolean
ligma_error_console_button_press (GtkWidget        *widget,
                                 GdkEventButton   *bevent,
                                 LigmaErrorConsole *console)
{
  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      return ligma_editor_popup_menu_at_pointer (LIGMA_EDITOR (console), (GdkEvent *) bevent);
    }

  return FALSE;
}

static const gchar * const aux_info_highlight[] =
{
  [LIGMA_MESSAGE_ERROR]   = "highlight-error",
  [LIGMA_MESSAGE_WARNING] = "highlight-warning",
  [LIGMA_MESSAGE_INFO]    = "highlight-info"
};

static void
ligma_error_console_set_aux_info (LigmaDocked *docked,
                                 GList      *aux_info)
{
  LigmaErrorConsole *console = LIGMA_ERROR_CONSOLE (docked);
  GList            *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      LigmaSessionInfoAux *aux = list->data;
      gint                i;

      for (i = 0; i < G_N_ELEMENTS (aux_info_highlight); i++)
        {
          if (! strcmp (aux->name, aux_info_highlight[i]))
            {
              console->highlight[i] = ! strcmp (aux->value, "yes");
              break;
            }
        }
    }
}

static GList *
ligma_error_console_get_aux_info (LigmaDocked *docked)
{
  LigmaErrorConsole   *console = LIGMA_ERROR_CONSOLE (docked);
  GList              *aux_info;
  gint                i;

  aux_info = parent_docked_iface->get_aux_info (docked);

  for (i = 0; i < G_N_ELEMENTS (aux_info_highlight); i++)
    {
      LigmaSessionInfoAux *aux;

      aux = ligma_session_info_aux_new (aux_info_highlight[i],
                                       console->highlight[i] ? "yes" : "no");

      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}
