/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaImageCommentEditor
 * Copyright (C) 2007  Sven Neumann <sven@ligma.org>
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

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmatemplate.h"

#include "ligmaimagecommenteditor.h"

#include "ligma-intl.h"

#define LIGMA_IMAGE_COMMENT_PARASITE "ligma-comment"


static void  ligma_image_comment_editor_update              (LigmaImageParasiteView  *view);

static void  ligma_image_comment_editor_buffer_changed      (GtkTextBuffer          *buffer,
                                                            LigmaImageCommentEditor *editor);
static void  ligma_image_comment_editor_use_default_comment (GtkWidget              *button,
                                                            LigmaImageCommentEditor *editor);


G_DEFINE_TYPE (LigmaImageCommentEditor,
               ligma_image_comment_editor, LIGMA_TYPE_IMAGE_PARASITE_VIEW)

static void
ligma_image_comment_editor_class_init (LigmaImageCommentEditorClass *klass)
{
  LigmaImageParasiteViewClass *view_class;

  view_class = LIGMA_IMAGE_PARASITE_VIEW_CLASS (klass);

  view_class->update = ligma_image_comment_editor_update;
}

static void
ligma_image_comment_editor_init (LigmaImageCommentEditor *editor)
{
  GtkWidget *vbox;
  GtkWidget *scrolled_window;
  GtkWidget *text_view;
  GtkWidget *button;

  /* Init */
  editor->recoursing = FALSE;

  /* Vbox */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (editor), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* Scrolled winow */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 2);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  /* Text view */
  text_view = gtk_text_view_new ();

  gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), TRUE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);

  gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (text_view), 6);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (text_view), 6);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (text_view), 6);

  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  /* Button */
  button = gtk_button_new_with_mnemonic (_("Use _default comment"));
  ligma_help_set_help_data (GTK_WIDGET (button),
                           _("Replace the current image comment with the "
                             "default comment set in "
                             "Edit→Preferences→Default Image."),
                           NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_image_comment_editor_use_default_comment),
                    editor);

  /* Buffer */
  editor->buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

  g_signal_connect (editor->buffer, "changed",
                    G_CALLBACK (ligma_image_comment_editor_buffer_changed),
                    editor);
}


/*  public functions  */

GtkWidget *
ligma_image_comment_editor_new (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return g_object_new (LIGMA_TYPE_IMAGE_COMMENT_EDITOR,
                       "image",    image,
                       "parasite", LIGMA_IMAGE_COMMENT_PARASITE,
                       NULL);
}


/*  private functions  */

static void
ligma_image_comment_editor_update (LigmaImageParasiteView *view)
{
  LigmaImageCommentEditor *editor = LIGMA_IMAGE_COMMENT_EDITOR (view);
  const LigmaParasite     *parasite;

  if (editor->recoursing)
    return;

  g_signal_handlers_block_by_func (editor->buffer,
                                   ligma_image_comment_editor_buffer_changed,
                                   editor);

  parasite = ligma_image_parasite_view_get_parasite (view);

  if (parasite)
    {
      gchar   *text;
      guint32  text_size;

      text = (gchar *) ligma_parasite_get_data (parasite, &text_size);
      text = g_strndup (text, text_size);

      if (! g_utf8_validate (text, -1, NULL))
        {
          gchar *tmp = ligma_any_to_utf8 (text, -1, NULL);

          g_free (text);
          text = tmp;
        }

      gtk_text_buffer_set_text (editor->buffer, text, -1);
      g_free (text);
    }
  else
    {
      gtk_text_buffer_set_text (editor->buffer, "", 0);
    }

  g_signal_handlers_unblock_by_func (editor->buffer,
                                     ligma_image_comment_editor_buffer_changed,
                                     editor);
}

static void
ligma_image_comment_editor_buffer_changed (GtkTextBuffer          *buffer,
                                          LigmaImageCommentEditor *editor)
{
  LigmaImage   *image;
  gchar       *text;
  gint         len;
  GtkTextIter  start;
  GtkTextIter  end;

  image =
    ligma_image_parasite_view_get_image (LIGMA_IMAGE_PARASITE_VIEW (editor));

  gtk_text_buffer_get_bounds (buffer, &start, &end);

  text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

  len = text ? strlen (text) : 0;

  editor->recoursing = TRUE;

  if (len > 0)
    {
      LigmaParasite *parasite;

      parasite = ligma_parasite_new (LIGMA_IMAGE_COMMENT_PARASITE,
                                    LIGMA_PARASITE_PERSISTENT,
                                    len + 1, text);

      ligma_image_parasite_attach (image, parasite, TRUE);
      ligma_parasite_free (parasite);
    }
  else
    {
      ligma_image_parasite_detach (image, LIGMA_IMAGE_COMMENT_PARASITE, TRUE);
    }

  editor->recoursing = FALSE;

  g_free (text);
}

static void
ligma_image_comment_editor_use_default_comment (GtkWidget              *button,
                                               LigmaImageCommentEditor *editor)
{
  LigmaImage   *image;
  const gchar *comment = NULL;

  image = ligma_image_parasite_view_get_image (LIGMA_IMAGE_PARASITE_VIEW (editor));

  if (image)
    {
      LigmaTemplate *template = image->ligma->config->default_image;

      comment = ligma_template_get_comment (template);
    }

  if (comment)
    gtk_text_buffer_set_text (editor->buffer, comment, -1);
  else
    gtk_text_buffer_set_text (editor->buffer, "", -1);
}
