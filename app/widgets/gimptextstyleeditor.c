/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextStyleEditor
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "gimptextbuffer.h"
#include "gimptextstyleeditor.h"


enum
{
  PROP_0,
  PROP_BUFFER
};


static GObject * gimp_text_style_editor_constructor  (GType                  type,
                                                      guint                  n_params,
                                                      GObjectConstructParam *params);
static void      gimp_text_style_editor_dispose      (GObject               *object);
static void      gimp_text_style_editor_finalize     (GObject               *object);
static void      gimp_text_style_editor_set_property (GObject               *object,
                                                      guint                  property_id,
                                                      const GValue          *value,
                                                      GParamSpec            *pspec);
static void      gimp_text_style_editor_get_property (GObject               *object,
                                                      guint                  property_id,
                                                      GValue                *value,
                                                      GParamSpec            *pspec);

static GtkWidget *
                gimp_text_style_editor_create_toggle (GimpTextStyleEditor *editor,
                                                      GtkTextTag          *tag,
                                                      const gchar         *stock_id);

static void      gimp_text_style_editor_clear_tags   (GtkButton           *button,
                                                      GimpTextStyleEditor *editor);
static void      gimp_text_style_editor_tag_toggled  (GtkToggleButton     *toggle,
                                                      GimpTextStyleEditor *editor);

static void      gimp_text_style_editor_mark_set     (GtkTextBuffer       *buffer,
                                                      GtkTextIter         *location,
                                                      GtkTextMark         *mark,
                                                      GimpTextStyleEditor *editor);


G_DEFINE_TYPE (GimpTextStyleEditor, gimp_text_style_editor,
               GTK_TYPE_HBOX)

#define parent_class gimp_text_style_editor_parent_class


static void
gimp_text_style_editor_class_init (GimpTextStyleEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_text_style_editor_constructor;
  object_class->dispose      = gimp_text_style_editor_dispose;
  object_class->finalize     = gimp_text_style_editor_finalize;
  object_class->set_property = gimp_text_style_editor_set_property;
  object_class->get_property = gimp_text_style_editor_get_property;

  g_object_class_install_property (object_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TEXT_BUFFER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_text_style_editor_init (GimpTextStyleEditor *editor)
{
  GtkWidget *image;

  editor->tag_to_toggle_hash = g_hash_table_new (g_direct_hash,
                                                 g_direct_equal);

  editor->clear_button = gtk_button_new ();
  gtk_widget_set_can_focus (editor->clear_button, FALSE);
  gtk_box_pack_start (GTK_BOX (editor), editor->clear_button, FALSE, FALSE, 0);
  gtk_widget_show (editor->clear_button);

  g_signal_connect (editor->clear_button, "clicked",
                    G_CALLBACK (gimp_text_style_editor_clear_tags),
                    editor);

  image = gtk_image_new_from_stock (GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (editor->clear_button), image);
  gtk_widget_show (image);
}

static GObject *
gimp_text_style_editor_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject             *object;
  GimpTextStyleEditor *editor;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_TEXT_STYLE_EDITOR (object);

  g_assert (GIMP_IS_TEXT_BUFFER (editor->buffer));

  gimp_text_style_editor_create_toggle (editor, editor->buffer->bold_tag,
                                        GTK_STOCK_BOLD);
  gimp_text_style_editor_create_toggle (editor, editor->buffer->italic_tag,
                                        GTK_STOCK_ITALIC);
  gimp_text_style_editor_create_toggle (editor, editor->buffer->underline_tag,
                                        GTK_STOCK_UNDERLINE);
  gimp_text_style_editor_create_toggle (editor, editor->buffer->strikethrough_tag,
                                        GTK_STOCK_STRIKETHROUGH);

  g_signal_connect (editor->buffer, "mark-set",
                    G_CALLBACK (gimp_text_style_editor_mark_set),
                    editor);

  return object;
}

static void
gimp_text_style_editor_dispose (GObject *object)
{
  GimpTextStyleEditor *editor = GIMP_TEXT_STYLE_EDITOR (object);

  if (editor->buffer)
    {
      g_signal_handlers_disconnect_by_func (editor->buffer,
                                            gimp_text_style_editor_mark_set,
                                            editor);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_text_style_editor_finalize (GObject *object)
{
  GimpTextStyleEditor *editor = GIMP_TEXT_STYLE_EDITOR (object);

  if (editor->tag_to_toggle_hash)
    {
      g_hash_table_unref (editor->tag_to_toggle_hash);
      editor->tag_to_toggle_hash = NULL;
    }

  if (editor->buffer)
    {
      g_object_unref (editor->buffer);
      editor->buffer = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_text_style_editor_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpTextStyleEditor *editor = GIMP_TEXT_STYLE_EDITOR (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      editor->buffer = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_text_style_editor_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpTextStyleEditor *editor = GIMP_TEXT_STYLE_EDITOR (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, editor->buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GtkWidget *
gimp_text_style_editor_new (GimpTextBuffer *buffer)
{
  g_return_val_if_fail (GIMP_IS_TEXT_BUFFER (buffer), NULL);

  return g_object_new (GIMP_TYPE_TEXT_STYLE_EDITOR,
                       "buffer", buffer,
                       NULL);
}


/*  private functions  */

static GtkWidget *
gimp_text_style_editor_create_toggle (GimpTextStyleEditor *editor,
                                      GtkTextTag          *tag,
                                      const gchar         *stock_id)
{
  GtkWidget *toggle;
  GtkWidget *image;

  toggle = gtk_toggle_button_new ();
  gtk_widget_set_can_focus (toggle, FALSE);
  gtk_box_pack_start (GTK_BOX (editor), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_object_set_data (G_OBJECT (toggle), "tag", tag);
  g_hash_table_insert (editor->tag_to_toggle_hash, tag, toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_text_style_editor_tag_toggled),
                    editor);

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (toggle), image);
  gtk_widget_show (image);

  return toggle;
}

static void
gimp_text_style_editor_clear_tags (GtkButton           *button,
                                   GimpTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      gtk_text_buffer_remove_all_tags (buffer, &start, &end);
    }
}

static void
gimp_text_style_editor_tag_toggled (GtkToggleButton     *toggle,
                                    GimpTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GtkTextTag    *tag    = g_object_get_data (G_OBJECT (toggle), "tag");

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      if (gtk_toggle_button_get_active (toggle))
        {
          gtk_text_buffer_apply_tag (buffer, tag, &start, &end);
        }
      else
        {
          gtk_text_buffer_remove_tag (buffer, tag, &start, &end);
        }
    }
}

static void
gimp_text_style_editor_enable_toggle (GtkTextTag          *tag,
                                      GtkToggleButton     *toggle,
                                      GimpTextStyleEditor *editor)
{
  g_signal_handlers_block_by_func (toggle,
                                   gimp_text_style_editor_tag_toggled,
                                   editor);

  gtk_toggle_button_set_active (toggle, TRUE);

  g_signal_handlers_unblock_by_func (toggle,
                                     gimp_text_style_editor_tag_toggled,
                                     editor);
}

typedef struct
{
  GimpTextStyleEditor *editor;
  GSList              *tags;
  GSList              *tags_on;
  GSList              *tags_off;
  GtkTextIter          iter;
  gboolean             any_active;
} UpdateTogglesData;

static void
gimp_text_style_editor_update_selection (GtkTextTag        *tag,
                                         GtkToggleButton   *toggle,
                                         UpdateTogglesData *data)
{
  if (! gtk_text_iter_has_tag (&data->iter, tag))
    {
      g_signal_handlers_block_by_func (toggle,
                                       gimp_text_style_editor_tag_toggled,
                                       data->editor);

      gtk_toggle_button_set_active (toggle, FALSE);

      g_signal_handlers_unblock_by_func (toggle,
                                         gimp_text_style_editor_tag_toggled,
                                         data->editor);
    }
  else
    {
      data->any_active = TRUE;
    }
}

static void
gimp_text_style_editor_update_cursor (GtkTextTag        *tag,
                                      GtkToggleButton   *toggle,
                                      UpdateTogglesData *data)
{
  g_signal_handlers_block_by_func (toggle,
                                   gimp_text_style_editor_tag_toggled,
                                   data->editor);

  gtk_toggle_button_set_active (toggle,
                                (g_slist_find (data->tags, tag) &&
                                 ! g_slist_find (data->tags_on, tag)) ||
                                g_slist_find (data->tags_off, tag));

  g_signal_handlers_unblock_by_func (toggle,
                                     gimp_text_style_editor_tag_toggled,
                                     data->editor);
}

static void
gimp_text_style_editor_mark_set (GtkTextBuffer       *buffer,
                                 GtkTextIter         *location,
                                 GtkTextMark         *mark,
                                 GimpTextStyleEditor *editor)
{
  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;
      GtkTextIter iter;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
      gtk_text_iter_order (&start, &end);

      /*  first, switch all toggles on  */
      g_hash_table_foreach (editor->tag_to_toggle_hash,
                            (GHFunc) gimp_text_style_editor_enable_toggle,
                            editor);

      for (iter = start;
           gtk_text_iter_in_range (&iter, &start, &end);
           gtk_text_iter_forward_cursor_position (&iter))
        {
          UpdateTogglesData data;

          data.editor     = editor;
          data.iter       = iter;
          data.any_active = FALSE;

          g_hash_table_foreach (editor->tag_to_toggle_hash,
                                (GHFunc) gimp_text_style_editor_update_selection,
                                &data);

          if (! data.any_active)
            break;
       }

    }
  else
    {
      UpdateTogglesData data;
      GtkTextIter       cursor;

      gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                        gtk_text_buffer_get_insert (buffer));

      data.editor   = editor;
      data.tags     = gtk_text_iter_get_tags (&cursor);
      data.tags_on  = gtk_text_iter_get_toggled_tags (&cursor, TRUE);
      data.tags_off = gtk_text_iter_get_toggled_tags (&cursor, FALSE);

      g_hash_table_foreach (editor->tag_to_toggle_hash,
                            (GHFunc) gimp_text_style_editor_update_cursor,
                            &data);

      g_slist_free (data.tags);
      g_slist_free (data.tags_on);
      g_slist_free (data.tags_off);
    }
}
