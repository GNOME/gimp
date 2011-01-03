/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextEditor
 * Copyright (C) 2002-2003, 2008  Sven Neumann <sven@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpmarshal.h"

#include "text/gimptext.h"

#include "gimphelp-ids.h"
#include "gimpmenufactory.h"
#include "gimptextbuffer.h"
#include "gimptexteditor.h"
#include "gimptextstyleeditor.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


enum
{
  TEXT_CHANGED,
  DIR_CHANGED,
  LAST_SIGNAL
};


static void   gimp_text_editor_finalize     (GObject         *object);

static void   gimp_text_editor_text_changed (GtkTextBuffer   *buffer,
                                             GimpTextEditor  *editor);
static void   gimp_text_editor_font_toggled (GtkToggleButton *button,
                                             GimpTextEditor  *editor);


G_DEFINE_TYPE (GimpTextEditor, gimp_text_editor, GIMP_TYPE_DIALOG)

#define parent_class gimp_text_editor_parent_class

static guint text_editor_signals[LAST_SIGNAL] = { 0 };


static void
gimp_text_editor_class_init (GimpTextEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_text_editor_finalize;

  klass->text_changed    = NULL;
  klass->dir_changed     = NULL;

  text_editor_signals[TEXT_CHANGED] =
    g_signal_new ("text-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpTextEditorClass, text_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  text_editor_signals[DIR_CHANGED] =
    g_signal_new ("dir-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpTextEditorClass, dir_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
gimp_text_editor_init (GimpTextEditor *editor)
{
  editor->view        = NULL;
  editor->file_dialog = NULL;
  editor->ui_manager  = NULL;

  switch (gtk_widget_get_default_direction ())
    {
    case GTK_TEXT_DIR_NONE:
    case GTK_TEXT_DIR_LTR:
      editor->base_dir = GIMP_TEXT_DIRECTION_LTR;
      break;
    case GTK_TEXT_DIR_RTL:
      editor->base_dir = GIMP_TEXT_DIRECTION_RTL;
      break;
    }
}

static void
gimp_text_editor_finalize (GObject *object)
{
  GimpTextEditor *editor = GIMP_TEXT_EDITOR (object);

  if (editor->font_name)
    g_free (editor->font_name);

  if (editor->ui_manager)
    g_object_unref (editor->ui_manager);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

GtkWidget *
gimp_text_editor_new (const gchar     *title,
                      GtkWindow       *parent,
                      Gimp            *gimp,
                      GimpMenuFactory *menu_factory,
                      GimpText        *text,
                      GimpTextBuffer  *text_buffer,
                      gdouble          xres,
                      gdouble          yres)
{
  GimpTextEditor *editor;
  GtkWidget      *content_area;
  GtkWidget      *toolbar;
  GtkWidget      *style_editor;
  GtkWidget      *scrolled_window;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT_BUFFER (text_buffer), NULL);

  editor = g_object_new (GIMP_TYPE_TEXT_EDITOR,
                         "title",         title,
                         "role",          "gimp-text-editor",
                         "transient-for", parent,
                         "help-func",     gimp_standard_help_func,
                         "help-id",       GIMP_HELP_TEXT_EDITOR_DIALOG,
                         NULL);

  gtk_dialog_add_button (GTK_DIALOG (editor),
                         _("_Close"), GTK_RESPONSE_CLOSE);

  g_signal_connect (editor, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  g_signal_connect_object (text_buffer, "changed",
                           G_CALLBACK (gimp_text_editor_text_changed),
                           editor, 0);

  editor->ui_manager = gimp_menu_factory_manager_new (menu_factory,
                                                      "<TextEditor>",
                                                      editor, FALSE);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (editor));

  toolbar = gtk_ui_manager_get_widget (GTK_UI_MANAGER (editor->ui_manager),
                                       "/text-editor-toolbar");

  if (toolbar)
    {
      gtk_box_pack_start (GTK_BOX (content_area), toolbar, FALSE, FALSE, 0);
      gtk_widget_show (toolbar);
    }

  style_editor = gimp_text_style_editor_new (gimp, text, text_buffer,
                                             gimp->fonts,
                                             xres, yres);
  gtk_box_pack_start (GTK_BOX (content_area), style_editor, FALSE, FALSE, 0);
  gtk_widget_show (style_editor);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
  gtk_box_pack_start (GTK_BOX (content_area), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  editor->view = gtk_text_view_new_with_buffer (GTK_TEXT_BUFFER (text_buffer));
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (editor->view),
                               GTK_WRAP_WORD_CHAR);
  gtk_container_add (GTK_CONTAINER (scrolled_window), editor->view);
  gtk_widget_show (editor->view);

  switch (editor->base_dir)
    {
    case GIMP_TEXT_DIRECTION_LTR:
      gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_LTR);
      break;
    case GIMP_TEXT_DIRECTION_RTL:
      gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_RTL);
      break;
    }

  gtk_widget_set_size_request (editor->view, 200, 64);

  editor->font_toggle =
    gtk_check_button_new_with_mnemonic (_("_Use selected font"));
  gtk_box_pack_start (GTK_BOX (content_area), editor->font_toggle,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->font_toggle);

  g_signal_connect (editor->font_toggle, "toggled",
                    G_CALLBACK (gimp_text_editor_font_toggled),
                    editor);

  gtk_widget_grab_focus (editor->view);

  gimp_ui_manager_update (editor->ui_manager, editor);

  return GTK_WIDGET (editor);
}

void
gimp_text_editor_set_text (GimpTextEditor *editor,
                           const gchar    *text,
                           gint            len)
{
  GtkTextBuffer *buffer;

  g_return_if_fail (GIMP_IS_TEXT_EDITOR (editor));
  g_return_if_fail (text != NULL || len == 0);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->view));

  if (text)
    gtk_text_buffer_set_text (buffer, text, len);
  else
    gtk_text_buffer_set_text (buffer, "", 0);
}

gchar *
gimp_text_editor_get_text (GimpTextEditor *editor)
{
  GtkTextBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_TEXT_EDITOR (editor), NULL);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor->view));

  return gimp_text_buffer_get_text (GIMP_TEXT_BUFFER (buffer));
}

void
gimp_text_editor_set_direction (GimpTextEditor    *editor,
                                GimpTextDirection  base_dir)
{
  g_return_if_fail (GIMP_IS_TEXT_EDITOR (editor));

  if (editor->base_dir == base_dir)
    return;

  editor->base_dir = base_dir;

  if (editor->view)
    {
      switch (editor->base_dir)
        {
        case GIMP_TEXT_DIRECTION_LTR:
          gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_LTR);
          break;
        case GIMP_TEXT_DIRECTION_RTL:
          gtk_widget_set_direction (editor->view, GTK_TEXT_DIR_RTL);
          break;
        }
    }

  gimp_ui_manager_update (editor->ui_manager, editor);

  g_signal_emit (editor, text_editor_signals[DIR_CHANGED], 0);
}

GimpTextDirection
gimp_text_editor_get_direction (GimpTextEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_TEXT_EDITOR (editor), GIMP_TEXT_DIRECTION_LTR);

  return editor->base_dir;
}

void
gimp_text_editor_set_font_name (GimpTextEditor *editor,
                                const gchar    *font_name)
{
  g_return_if_fail (GIMP_IS_TEXT_EDITOR (editor));

  if (editor->font_name)
    g_free (editor->font_name);

  editor->font_name = g_strdup (font_name);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (editor->font_toggle)))
    {
      PangoFontDescription *font_desc = NULL;

      if (font_name)
        font_desc = pango_font_description_from_string (font_name);

      gtk_widget_override_font (editor->view, font_desc);

      if (font_desc)
        pango_font_description_free (font_desc);
    }
}

const gchar *
gimp_text_editor_get_font_name (GimpTextEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_TEXT_EDITOR (editor), NULL);

  return editor->font_name;
}


/*  private functions  */

static void
gimp_text_editor_text_changed (GtkTextBuffer  *buffer,
                               GimpTextEditor *editor)
{
  g_signal_emit (editor, text_editor_signals[TEXT_CHANGED], 0);
}

static void
gimp_text_editor_font_toggled (GtkToggleButton *button,
                               GimpTextEditor  *editor)
{
  PangoFontDescription *font_desc = NULL;

  if (gtk_toggle_button_get_active (button) && editor->font_name)
    font_desc = pango_font_description_from_string (editor->font_name);

  gtk_widget_override_font (editor->view, font_desc);

  if (font_desc)
    pango_font_description_free (font_desc);
}
