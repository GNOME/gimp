/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimperrorconsole.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * partly based on errorconsole.c
 * Copyright (C) 1998 Nick Fetchak <nuke@bayside.net>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"

#include "gimperrorconsole.h"
#include "gimpmenufactory.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static GObject * gimp_error_console_constructor  (GType             type,
                                                  guint             n_params,
                                                  GObjectConstructParam *params);

static void      gimp_error_console_destroy      (GtkObject        *object);
static void      gimp_error_console_unmap        (GtkWidget        *widget);

static gboolean  gimp_error_console_button_press (GtkWidget        *widget,
                                                  GdkEventButton   *event,
                                                  GimpErrorConsole *console);


G_DEFINE_TYPE (GimpErrorConsole, gimp_error_console, GIMP_TYPE_EDITOR)

#define parent_class gimp_error_console_parent_class


static void
gimp_error_console_class_init (GimpErrorConsoleClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class     = GTK_WIDGET_CLASS (klass);

  object_class->constructor = gimp_error_console_constructor;
  gtk_object_class->destroy = gimp_error_console_destroy;
  widget_class->unmap       = gimp_error_console_unmap;
}

static void
gimp_error_console_init (GimpErrorConsole *console)
{
  GtkWidget *scrolled_window;

  console->text_buffer = gtk_text_buffer_new (NULL);

  gtk_text_buffer_create_tag (console->text_buffer, "title",
                              "weight", PANGO_WEIGHT_BOLD,
                              NULL);
  gtk_text_buffer_create_tag (console->text_buffer, "message",
                              "scale",  PANGO_SCALE_SMALL,
                              NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (console), scrolled_window);
  gtk_widget_show (scrolled_window);

  console->text_view = gtk_text_view_new_with_buffer (console->text_buffer);
  g_object_unref (console->text_buffer);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (console->text_view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (console->text_view),
                               GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), console->text_view);
  gtk_widget_show (console->text_view);

  g_signal_connect (console->text_view, "button-press-event",
                    G_CALLBACK (gimp_error_console_button_press),
                    console);

  console->file_dialog = NULL;
}

static GObject *
gimp_error_console_constructor (GType                  type,
                                guint                  n_params,
                                GObjectConstructParam *params)
{
  GObject          *object;
  GimpErrorConsole *console;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  console = GIMP_ERROR_CONSOLE (object);

  console->clear_button =
    gimp_editor_add_action_button (GIMP_EDITOR (console), "error-console",
                                   "error-console-clear", NULL);

  console->save_button =
    gimp_editor_add_action_button (GIMP_EDITOR (console), "error-console",
                                   "error-console-save-all",
                                   "error-console-save-selection",
                                   GDK_SHIFT_MASK,
                                   NULL);

  return object;
}

static void
gimp_error_console_destroy (GtkObject *object)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (object);

  if (console->file_dialog)
    gtk_widget_destroy (console->file_dialog);

  console->gimp->message_handler = GIMP_MESSAGE_BOX;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_error_console_unmap (GtkWidget *widget)
{
  GimpErrorConsole *console = GIMP_ERROR_CONSOLE (widget);

  if (console->file_dialog)
    gtk_widget_destroy (console->file_dialog);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

GtkWidget *
gimp_error_console_new (Gimp            *gimp,
                        GimpMenuFactory *menu_factory)
{
  GimpErrorConsole *console;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  console = g_object_new (GIMP_TYPE_ERROR_CONSOLE,
                          "menu-factory",   menu_factory,
                          "menu-identifier", "<ErrorConsole>",
                          "ui-path",        "/error-console-popup",
                          NULL);

  console->gimp = gimp;

  console->gimp->message_handler = GIMP_ERROR_CONSOLE;

  return GTK_WIDGET (console);
}

void
gimp_error_console_add (GimpErrorConsole    *console,
                        GimpMessageSeverity  severity,
                        const gchar         *domain,
                        const gchar         *message)
{
  const gchar *desc;
  GtkTextIter  end;
  GtkTextMark *end_mark;
  GdkPixbuf   *pixbuf;
  gchar       *str;

  g_return_if_fail (GIMP_IS_ERROR_CONSOLE (console));
  g_return_if_fail (domain != NULL);
  g_return_if_fail (message != NULL);

  gimp_enum_get_value (GIMP_TYPE_MESSAGE_SEVERITY, severity,
                       NULL, NULL, &desc, NULL);

  gtk_text_buffer_get_end_iter (console->text_buffer, &end);

  pixbuf = gtk_widget_render_icon (console->text_view,
                                   gimp_get_message_stock_id (severity),
                                   GTK_ICON_SIZE_MENU, NULL);
  gtk_text_buffer_insert_pixbuf (console->text_buffer, &end, pixbuf);
  g_object_unref (pixbuf);

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
gimp_error_console_button_press (GtkWidget        *widget,
                                 GdkEventButton   *bevent,
                                 GimpErrorConsole *console)
{
  if (bevent->button == 3 && bevent->type == GDK_BUTTON_PRESS)
    {
      return gimp_editor_popup_menu (GIMP_EDITOR (console), NULL, NULL);
    }

  return FALSE;
}
