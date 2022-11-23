/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextStyleEditor
 * Copyright (C) 2010  Michael Natterer <mitch@ligma.org>
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
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"

#include "text/ligmatext.h"

#include "ligmacolorpanel.h"
#include "ligmacontainerentry.h"
#include "ligmacontainerview.h"
#include "ligmatextbuffer.h"
#include "ligmatextstyleeditor.h"
#include "ligmatexttag.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_TEXT,
  PROP_BUFFER,
  PROP_FONTS,
  PROP_RESOLUTION_X,
  PROP_RESOLUTION_Y
};


static void      ligma_text_style_editor_constructed       (GObject             *object);
static void      ligma_text_style_editor_dispose           (GObject             *object);
static void      ligma_text_style_editor_finalize          (GObject             *object);
static void      ligma_text_style_editor_set_property      (GObject             *object,
                                                           guint                property_id,
                                                           const GValue        *value,
                                                           GParamSpec          *pspec);
static void      ligma_text_style_editor_get_property      (GObject             *object,
                                                           guint                property_id,
                                                           GValue              *value,
                                                           GParamSpec          *pspec);

static GtkWidget * ligma_text_style_editor_create_toggle   (LigmaTextStyleEditor *editor,
                                                           GtkTextTag          *tag,
                                                           const gchar         *icon_name,
                                                           const gchar         *tooltip);

static void      ligma_text_style_editor_clear_tags        (GtkButton           *button,
                                                           LigmaTextStyleEditor *editor);

static void      ligma_text_style_editor_font_changed      (LigmaContext         *context,
                                                           LigmaFont            *font,
                                                           LigmaTextStyleEditor *editor);
static void      ligma_text_style_editor_set_font          (LigmaTextStyleEditor *editor,
                                                           GtkTextTag          *font_tag);
static void      ligma_text_style_editor_set_default_font  (LigmaTextStyleEditor *editor);

static void      ligma_text_style_editor_color_changed     (LigmaColorButton     *button,
                                                           LigmaTextStyleEditor *editor);
static void      ligma_text_style_editor_set_color         (LigmaTextStyleEditor *editor,
                                                           GtkTextTag          *color_tag);
static void      ligma_text_style_editor_set_default_color (LigmaTextStyleEditor *editor);

static void      ligma_text_style_editor_tag_toggled       (GtkToggleButton     *toggle,
                                                           LigmaTextStyleEditor *editor);
static void      ligma_text_style_editor_set_toggle        (LigmaTextStyleEditor *editor,
                                                           GtkToggleButton     *toggle,
                                                           gboolean             active);

static void      ligma_text_style_editor_size_changed      (LigmaSizeEntry       *entry,
                                                           LigmaTextStyleEditor *editor);
static void      ligma_text_style_editor_set_size          (LigmaTextStyleEditor *editor,
                                                           GtkTextTag          *size_tag);
static void      ligma_text_style_editor_set_default_size  (LigmaTextStyleEditor *editor);

static void      ligma_text_style_editor_baseline_changed  (GtkAdjustment       *adjustment,
                                                           LigmaTextStyleEditor *editor);
static void      ligma_text_style_editor_set_baseline      (LigmaTextStyleEditor *editor,
                                                           GtkTextTag          *baseline_tag);

static void      ligma_text_style_editor_kerning_changed   (GtkAdjustment       *adjustment,
                                                           LigmaTextStyleEditor *editor);
static void      ligma_text_style_editor_set_kerning       (LigmaTextStyleEditor *editor,
                                                           GtkTextTag          *kerning_tag);

static void      ligma_text_style_editor_update            (LigmaTextStyleEditor *editor);
static gboolean  ligma_text_style_editor_update_idle       (LigmaTextStyleEditor *editor);


G_DEFINE_TYPE (LigmaTextStyleEditor, ligma_text_style_editor,
               GTK_TYPE_BOX)

#define parent_class ligma_text_style_editor_parent_class


static void
ligma_text_style_editor_class_init (LigmaTextStyleEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = ligma_text_style_editor_constructed;
  object_class->dispose      = ligma_text_style_editor_dispose;
  object_class->finalize     = ligma_text_style_editor_finalize;
  object_class->set_property = ligma_text_style_editor_set_property;
  object_class->get_property = ligma_text_style_editor_get_property;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TEXT,
                                   g_param_spec_object ("text",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_TEXT,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_TEXT_BUFFER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_FONTS,
                                   g_param_spec_object ("fonts",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTAINER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_RESOLUTION_X,
                                   g_param_spec_double ("resolution-x",
                                                        NULL, NULL,
                                                        LIGMA_MIN_RESOLUTION,
                                                        LIGMA_MAX_RESOLUTION,
                                                        1.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RESOLUTION_Y,
                                   g_param_spec_double ("resolution-y",
                                                        NULL, NULL,
                                                        LIGMA_MIN_RESOLUTION,
                                                        LIGMA_MAX_RESOLUTION,
                                                        1.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  gtk_widget_class_set_css_name (widget_class, "LigmaTextStyleEditor");
}

static void
ligma_text_style_editor_init (LigmaTextStyleEditor *editor)
{
  GtkWidget *image;
  LigmaRGB    color;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);
  gtk_box_set_spacing (GTK_BOX (editor), 2);

  /*  upper row  */

  editor->upper_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (editor), editor->upper_hbox, FALSE, FALSE, 0);
  gtk_widget_show (editor->upper_hbox);

  editor->font_entry = ligma_container_entry_new (NULL, NULL,
                                                 LIGMA_VIEW_SIZE_SMALL, 1);
  gtk_box_pack_start (GTK_BOX (editor->upper_hbox), editor->font_entry,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->font_entry);

  ligma_help_set_help_data (editor->font_entry,
                           _("Change font of selected text"), NULL);

  editor->size_entry =
    ligma_size_entry_new (1, 0, "%a", TRUE, FALSE, FALSE, 10,
                         LIGMA_SIZE_ENTRY_UPDATE_SIZE);
  gtk_box_pack_start (GTK_BOX (editor->upper_hbox), editor->size_entry,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->size_entry);

  ligma_help_set_help_data (editor->size_entry,
                           _("Change size of selected text"), NULL);

  g_signal_connect (editor->size_entry, "value-changed",
                    G_CALLBACK (ligma_text_style_editor_size_changed),
                    editor);

  /*  lower row  */

  editor->lower_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (editor), editor->lower_hbox, FALSE, FALSE, 0);
  gtk_widget_show (editor->lower_hbox);

  editor->clear_button = gtk_button_new ();
  gtk_widget_set_can_focus (editor->clear_button, FALSE);
  gtk_box_pack_start (GTK_BOX (editor->lower_hbox), editor->clear_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->clear_button);

  ligma_help_set_help_data (editor->clear_button,
                           _("Clear style of selected text"), NULL);

  g_signal_connect (editor->clear_button, "clicked",
                    G_CALLBACK (ligma_text_style_editor_clear_tags),
                    editor);

  image = gtk_image_new_from_icon_name ("edit-clear", GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (editor->clear_button), image);
  gtk_widget_show (image);

  ligma_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);
  editor->color_button = ligma_color_panel_new (_("Change color of selected text"),
                                               &color,
                                               LIGMA_COLOR_AREA_FLAT, 20, 20);
  ligma_widget_set_fully_opaque (editor->color_button, TRUE);

  gtk_box_pack_end (GTK_BOX (editor->lower_hbox), editor->color_button,
                    FALSE, FALSE, 0);
  gtk_widget_show (editor->color_button);

  ligma_help_set_help_data (editor->color_button,
                           _("Change color of selected text"), NULL);

  g_signal_connect (editor->color_button, "color-changed",
                    G_CALLBACK (ligma_text_style_editor_color_changed),
                    editor);

  editor->kerning_adjustment = gtk_adjustment_new (0.0, -1000.0, 1000.0,
                                                   1.0, 10.0, 0.0);
  editor->kerning_spinbutton =
    ligma_spin_button_new (editor->kerning_adjustment, 1.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (editor->kerning_spinbutton), 5);
  gtk_box_pack_end (GTK_BOX (editor->lower_hbox), editor->kerning_spinbutton,
                    FALSE, FALSE, 0);
  gtk_widget_show (editor->kerning_spinbutton);

  ligma_help_set_help_data (editor->kerning_spinbutton,
                           _("Change kerning of selected text"), NULL);

  g_signal_connect (editor->kerning_adjustment, "value-changed",
                    G_CALLBACK (ligma_text_style_editor_kerning_changed),
                    editor);

  editor->baseline_adjustment = gtk_adjustment_new (0.0, -1000.0, 1000.0,
                                                    1.0, 10.0, 0.0);
  editor->baseline_spinbutton =
    ligma_spin_button_new (editor->baseline_adjustment, 1.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (editor->baseline_spinbutton), 5);
  gtk_box_pack_end (GTK_BOX (editor->lower_hbox), editor->baseline_spinbutton,
                    FALSE, FALSE, 0);
  gtk_widget_show (editor->baseline_spinbutton);

  ligma_help_set_help_data (editor->baseline_spinbutton,
                           _("Change baseline of selected text"), NULL);

  g_signal_connect (editor->baseline_adjustment, "value-changed",
                    G_CALLBACK (ligma_text_style_editor_baseline_changed),
                    editor);
}

static void
ligma_text_style_editor_constructed (GObject *object)
{
  LigmaTextStyleEditor *editor = LIGMA_TEXT_STYLE_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (editor->ligma));
  ligma_assert (LIGMA_IS_CONTAINER (editor->fonts));
  ligma_assert (LIGMA_IS_TEXT (editor->text));
  ligma_assert (LIGMA_IS_TEXT_BUFFER (editor->buffer));

  editor->context = ligma_context_new (editor->ligma, "text style editor", NULL);

  g_signal_connect (editor->context, "font-changed",
                    G_CALLBACK (ligma_text_style_editor_font_changed),
                    editor);

  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (editor->size_entry), 0,
                                  editor->resolution_y, TRUE);

  /* use the global user context so we get the global FG/BG colors */
  ligma_color_panel_set_context (LIGMA_COLOR_PANEL (editor->color_button),
                                ligma_get_user_context (editor->ligma));

  ligma_container_view_set_container (LIGMA_CONTAINER_VIEW (editor->font_entry),
                                     editor->fonts);
  ligma_container_view_set_context (LIGMA_CONTAINER_VIEW (editor->font_entry),
                                   editor->context);

  ligma_text_style_editor_create_toggle (editor, editor->buffer->bold_tag,
                                        LIGMA_ICON_FORMAT_TEXT_BOLD,
                                        _("Bold"));
  ligma_text_style_editor_create_toggle (editor, editor->buffer->italic_tag,
                                        LIGMA_ICON_FORMAT_TEXT_ITALIC,
                                        _("Italic"));
  ligma_text_style_editor_create_toggle (editor, editor->buffer->underline_tag,
                                        LIGMA_ICON_FORMAT_TEXT_UNDERLINE,
                                        _("Underline"));
  ligma_text_style_editor_create_toggle (editor, editor->buffer->strikethrough_tag,
                                        LIGMA_ICON_FORMAT_TEXT_STRIKETHROUGH,
                                        _("Strikethrough"));

  g_signal_connect_swapped (editor->text, "notify::font",
                            G_CALLBACK (ligma_text_style_editor_update),
                            editor);
  g_signal_connect_swapped (editor->text, "notify::font-size",
                            G_CALLBACK (ligma_text_style_editor_update),
                            editor);
  g_signal_connect_swapped (editor->text, "notify::font-size-unit",
                            G_CALLBACK (ligma_text_style_editor_update),
                            editor);
  g_signal_connect_swapped (editor->text, "notify::color",
                            G_CALLBACK (ligma_text_style_editor_update),
                            editor);

  g_signal_connect_data (editor->buffer, "changed",
                         G_CALLBACK (ligma_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_data (editor->buffer, "apply-tag",
                         G_CALLBACK (ligma_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_data (editor->buffer, "remove-tag",
                         G_CALLBACK (ligma_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_data (editor->buffer, "mark-set",
                         G_CALLBACK (ligma_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  ligma_text_style_editor_update (editor);
}

static void
ligma_text_style_editor_dispose (GObject *object)
{
  LigmaTextStyleEditor *editor = LIGMA_TEXT_STYLE_EDITOR (object);

  if (editor->text)
    {
      g_signal_handlers_disconnect_by_func (editor->text,
                                            ligma_text_style_editor_update,
                                            editor);
    }

  if (editor->buffer)
    {
      g_signal_handlers_disconnect_by_func (editor->buffer,
                                            ligma_text_style_editor_update,
                                            editor);
    }

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            ligma_text_style_editor_font_changed,
                                            editor);
    }

  if (editor->update_idle_id)
    {
      g_source_remove (editor->update_idle_id);
      editor->update_idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_text_style_editor_finalize (GObject *object)
{
  LigmaTextStyleEditor *editor = LIGMA_TEXT_STYLE_EDITOR (object);

  g_clear_object (&editor->context);
  g_clear_object (&editor->text);
  g_clear_object (&editor->buffer);
  g_clear_object (&editor->fonts);

  if (editor->toggles)
    {
      g_list_free (editor->toggles);
      editor->toggles = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_text_style_editor_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LigmaTextStyleEditor *editor = LIGMA_TEXT_STYLE_EDITOR (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      editor->ligma = g_value_get_object (value); /* don't ref */
      break;
    case PROP_TEXT:
      editor->text = g_value_dup_object (value);
      break;
    case PROP_BUFFER:
      editor->buffer = g_value_dup_object (value);
      break;
    case PROP_FONTS:
      editor->fonts = g_value_dup_object (value);
      break;
    case PROP_RESOLUTION_X:
      editor->resolution_x = g_value_get_double (value);
      break;
    case PROP_RESOLUTION_Y:
      editor->resolution_y = g_value_get_double (value);
      if (editor->size_entry)
        ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (editor->size_entry), 0,
                                        editor->resolution_y, TRUE);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_text_style_editor_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  LigmaTextStyleEditor *editor = LIGMA_TEXT_STYLE_EDITOR (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, editor->ligma);
      break;
    case PROP_TEXT:
      g_value_set_object (value, editor->text);
      break;
    case PROP_BUFFER:
      g_value_set_object (value, editor->buffer);
      break;
    case PROP_FONTS:
      g_value_set_object (value, editor->fonts);
      break;
    case PROP_RESOLUTION_X:
      g_value_set_double (value, editor->resolution_x);
      break;
    case PROP_RESOLUTION_Y:
      g_value_set_double (value, editor->resolution_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GtkWidget *
ligma_text_style_editor_new (Ligma           *ligma,
                            LigmaText       *text,
                            LigmaTextBuffer *buffer,
                            LigmaContainer  *fonts,
                            gdouble         resolution_x,
                            gdouble         resolution_y)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_TEXT (text), NULL);
  g_return_val_if_fail (LIGMA_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (resolution_x > 0.0, NULL);
  g_return_val_if_fail (resolution_y > 0.0, NULL);

  return g_object_new (LIGMA_TYPE_TEXT_STYLE_EDITOR,
                       "ligma",         ligma,
                       "text",         text,
                       "buffer",       buffer,
                       "fonts",        fonts,
                       "resolution-x", resolution_x,
                       "resolution-y", resolution_y,
                       NULL);
}

GList *
ligma_text_style_editor_list_tags (LigmaTextStyleEditor  *editor,
                                  GList               **remove_tags)
{
  GList *toggles;
  GList *tags = NULL;

  g_return_val_if_fail (LIGMA_IS_TEXT_STYLE_EDITOR (editor), NULL);
  g_return_val_if_fail (remove_tags != NULL, NULL);

  *remove_tags = NULL;

  for (toggles = editor->toggles; toggles; toggles = g_list_next (toggles))
    {
      GtkTextTag *tag = g_object_get_data (toggles->data, "tag");

      if (gtk_toggle_button_get_active (toggles->data))
        {
          tags = g_list_prepend (tags, tag);
        }
      else
        {
          *remove_tags = g_list_prepend (*remove_tags, tag);
        }
    }

  {
    GList   *list;
    gdouble  pixels;

    for (list = editor->buffer->size_tags; list; list = g_list_next (list))
      *remove_tags = g_list_prepend (*remove_tags, list->data);

    pixels = ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (editor->size_entry), 0);

    if (pixels != 0.0)
      {
        GtkTextTag *tag;
        gdouble     points;

        points = ligma_units_to_points (pixels,
                                       LIGMA_UNIT_PIXEL,
                                       editor->resolution_y);
        tag = ligma_text_buffer_get_size_tag (editor->buffer,
                                             PANGO_SCALE * points);
        tags = g_list_prepend (tags, tag);
      }
  }

  {
    GList       *list;
    const gchar *font_name;

    for (list = editor->buffer->font_tags; list; list = g_list_next (list))
      *remove_tags = g_list_prepend (*remove_tags, list->data);

    font_name = ligma_context_get_font_name (editor->context);

    if (font_name)
      {
        GtkTextTag  *tag;

        tag = ligma_text_buffer_get_font_tag (editor->buffer, font_name);
        tags = g_list_prepend (tags, tag);
      }
  }

  {
    GList   *list;
    LigmaRGB  color;

    for (list = editor->buffer->color_tags; list; list = g_list_next (list))
      *remove_tags = g_list_prepend (*remove_tags, list->data);

    ligma_color_button_get_color (LIGMA_COLOR_BUTTON (editor->color_button),
                                 &color);

    if (TRUE) /* FIXME should have "inconsistent" state as for font and size */
      {
        GtkTextTag *tag;

        tag = ligma_text_buffer_get_color_tag (editor->buffer, &color);
        tags = g_list_prepend (tags, tag);
      }
  }

  *remove_tags = g_list_reverse (*remove_tags);

  return g_list_reverse (tags);
}


/*  private functions  */

static GtkWidget *
ligma_text_style_editor_create_toggle (LigmaTextStyleEditor *editor,
                                      GtkTextTag          *tag,
                                      const gchar         *icon_name,
                                      const gchar         *tooltip)
{
  GtkWidget *toggle;
  GtkWidget *image;

  toggle = gtk_toggle_button_new ();
  gtk_widget_set_can_focus (toggle, FALSE);
  gtk_box_pack_start (GTK_BOX (editor->lower_hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  ligma_help_set_help_data (toggle, tooltip, NULL);

  editor->toggles = g_list_append (editor->toggles, toggle);
  g_object_set_data (G_OBJECT (toggle), "tag", tag);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (ligma_text_style_editor_tag_toggled),
                    editor);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (toggle), image);
  gtk_widget_show (image);

  return toggle;
}

static void
ligma_text_style_editor_clear_tags (GtkButton           *button,
                                   LigmaTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      gtk_text_buffer_begin_user_action (buffer);

      gtk_text_buffer_remove_all_tags (buffer, &start, &end);

      gtk_text_buffer_end_user_action (buffer);
    }
}

static void
ligma_text_style_editor_font_changed (LigmaContext         *context,
                                     LigmaFont            *font,
                                     LigmaTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GList         *insert_tags;
  GList         *remove_tags;

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      ligma_text_buffer_set_font (editor->buffer, &start, &end,
                                 ligma_context_get_font_name (context));
    }

  insert_tags = ligma_text_style_editor_list_tags (editor, &remove_tags);
  ligma_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
ligma_text_style_editor_set_font (LigmaTextStyleEditor *editor,
                                 GtkTextTag          *font_tag)
{
  gchar *font = NULL;

  if (font_tag)
    font = ligma_text_tag_get_font (font_tag);

  g_signal_handlers_block_by_func (editor->context,
                                   ligma_text_style_editor_font_changed,
                                   editor);

  ligma_context_set_font_name (editor->context, font);

  g_signal_handlers_unblock_by_func (editor->context,
                                     ligma_text_style_editor_font_changed,
                                     editor);

  g_free (font);
}

static void
ligma_text_style_editor_set_default_font (LigmaTextStyleEditor *editor)
{
  g_signal_handlers_block_by_func (editor->context,
                                   ligma_text_style_editor_font_changed,
                                   editor);

  ligma_context_set_font_name (editor->context, editor->text->font);

  g_signal_handlers_unblock_by_func (editor->context,
                                     ligma_text_style_editor_font_changed,
                                     editor);
}

static void
ligma_text_style_editor_color_changed (LigmaColorButton     *button,
                                      LigmaTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GList         *insert_tags;
  GList         *remove_tags;

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;
      LigmaRGB     color;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      ligma_color_button_get_color (button, &color);
      ligma_text_buffer_set_color (editor->buffer, &start, &end, &color);
    }

  insert_tags = ligma_text_style_editor_list_tags (editor, &remove_tags);
  ligma_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
ligma_text_style_editor_set_color (LigmaTextStyleEditor *editor,
                                  GtkTextTag          *color_tag)
{
  LigmaRGB color;

  ligma_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);

  if (color_tag)
    ligma_text_tag_get_fg_color (color_tag, &color);

  g_signal_handlers_block_by_func (editor->color_button,
                                   ligma_text_style_editor_color_changed,
                                   editor);

  ligma_color_button_set_color (LIGMA_COLOR_BUTTON (editor->color_button),
                               &color);

  /* FIXME should have "inconsistent" state as for font and size */

  g_signal_handlers_unblock_by_func (editor->color_button,
                                     ligma_text_style_editor_color_changed,
                                     editor);
}

static void
ligma_text_style_editor_set_default_color (LigmaTextStyleEditor *editor)
{
  g_signal_handlers_block_by_func (editor->color_button,
                                   ligma_text_style_editor_color_changed,
                                   editor);

  ligma_color_button_set_color (LIGMA_COLOR_BUTTON (editor->color_button),
                               &editor->text->color);

  g_signal_handlers_unblock_by_func (editor->color_button,
                                     ligma_text_style_editor_color_changed,
                                     editor);
}

static void
ligma_text_style_editor_tag_toggled (GtkToggleButton     *toggle,
                                    LigmaTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GtkTextTag    *tag    = g_object_get_data (G_OBJECT (toggle), "tag");
  GList         *insert_tags;
  GList         *remove_tags;

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      gtk_text_buffer_begin_user_action (buffer);

      if (gtk_toggle_button_get_active (toggle))
        {
          gtk_text_buffer_apply_tag (buffer, tag, &start, &end);
        }
      else
        {
          gtk_text_buffer_remove_tag (buffer, tag, &start, &end);
        }

      gtk_text_buffer_end_user_action (buffer);
    }

  insert_tags = ligma_text_style_editor_list_tags (editor, &remove_tags);
  ligma_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
ligma_text_style_editor_set_toggle (LigmaTextStyleEditor *editor,
                                   GtkToggleButton     *toggle,
                                   gboolean             active)
{
  g_signal_handlers_block_by_func (toggle,
                                   ligma_text_style_editor_tag_toggled,
                                   editor);

  gtk_toggle_button_set_active (toggle, active);

  g_signal_handlers_unblock_by_func (toggle,
                                     ligma_text_style_editor_tag_toggled,
                                     editor);
}

static void
ligma_text_style_editor_size_changed (LigmaSizeEntry       *entry,
                                     LigmaTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GList         *insert_tags;
  GList         *remove_tags;
  GtkTextIter    start, end;
  gdouble        points;

  points = ligma_units_to_points (ligma_size_entry_get_refval (entry, 0),
                                 LIGMA_UNIT_PIXEL, editor->resolution_y);

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
    }
  else
    {
      gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
      gtk_text_buffer_get_iter_at_offset (buffer, &end, -1);
    }

  ligma_text_buffer_set_size (editor->buffer, &start, &end,
                             PANGO_SCALE * points);

  insert_tags = ligma_text_style_editor_list_tags (editor, &remove_tags);
  ligma_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
ligma_text_style_editor_set_size (LigmaTextStyleEditor *editor,
                                 GtkTextTag          *size_tag)
{
  gint    size = 0;
  gdouble pixels;

  if (size_tag)
    size = ligma_text_tag_get_size (size_tag);

  g_signal_handlers_block_by_func (editor->size_entry,
                                   ligma_text_style_editor_size_changed,
                                   editor);

  pixels = ligma_units_to_pixels ((gdouble) size / PANGO_SCALE,
                                 LIGMA_UNIT_POINT,
                                 editor->resolution_y);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (editor->size_entry), 0, pixels);

  if (size == 0)
    {
      GtkWidget *spinbutton;

      spinbutton = ligma_size_entry_get_help_widget (LIGMA_SIZE_ENTRY (editor->size_entry), 0);

      gtk_entry_set_text (GTK_ENTRY (spinbutton), "");
    }

  g_signal_handlers_unblock_by_func (editor->size_entry,
                                     ligma_text_style_editor_size_changed,
                                     editor);
}

static void
ligma_text_style_editor_set_default_size (LigmaTextStyleEditor *editor)
{
  gdouble pixels = ligma_units_to_pixels (editor->text->font_size,
                                         editor->text->unit,
                                         editor->resolution_y);

  g_signal_handlers_block_by_func (editor->size_entry,
                                   ligma_text_style_editor_size_changed,
                                   editor);

  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (editor->size_entry), 0, pixels);

  g_signal_handlers_unblock_by_func (editor->size_entry,
                                     ligma_text_style_editor_size_changed,
                                     editor);
}

static void
ligma_text_style_editor_baseline_changed (GtkAdjustment       *adjustment,
                                         LigmaTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GtkTextIter    start, end;

  if (! gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                        gtk_text_buffer_get_insert (buffer));
      gtk_text_buffer_get_end_iter (buffer, &end);
    }

  ligma_text_buffer_set_baseline (editor->buffer, &start, &end,
                                 gtk_adjustment_get_value (adjustment) *
                                 PANGO_SCALE);
}

static void
ligma_text_style_editor_set_baseline (LigmaTextStyleEditor *editor,
                                     GtkTextTag          *baseline_tag)
{
  gint baseline = 0;

  if (baseline_tag)
    baseline = ligma_text_tag_get_baseline (baseline_tag);

  g_signal_handlers_block_by_func (editor->baseline_adjustment,
                                   ligma_text_style_editor_baseline_changed,
                                   editor);

  if (gtk_adjustment_get_value (editor->baseline_adjustment) !=
      (gdouble) baseline / PANGO_SCALE)
    {
      gtk_adjustment_set_value (editor->baseline_adjustment,
                                (gdouble) baseline / PANGO_SCALE);
    }
  else
    {
      /* make sure the "" really gets replaced */
      g_signal_emit_by_name (editor->baseline_adjustment, "value-changed");
    }

  g_signal_handlers_unblock_by_func (editor->baseline_adjustment,
                                     ligma_text_style_editor_baseline_changed,
                                     editor);
}

static void
ligma_text_style_editor_kerning_changed (GtkAdjustment       *adjustment,
                                        LigmaTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GtkTextIter    start, end;

  if (! gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                        gtk_text_buffer_get_insert (buffer));
      end = start;
      gtk_text_iter_forward_char (&end);
    }

  ligma_text_buffer_set_kerning (editor->buffer, &start, &end,
                                gtk_adjustment_get_value (adjustment) *
                                PANGO_SCALE);
}

static void
ligma_text_style_editor_set_kerning (LigmaTextStyleEditor *editor,
                                    GtkTextTag          *kerning_tag)
{
  gint kerning = 0;

  if (kerning_tag)
    kerning = ligma_text_tag_get_kerning (kerning_tag);

  g_signal_handlers_block_by_func (editor->kerning_adjustment,
                                   ligma_text_style_editor_kerning_changed,
                                   editor);

  if (gtk_adjustment_get_value (editor->baseline_adjustment) !=
      (gdouble) kerning / PANGO_SCALE)
    {
      gtk_adjustment_set_value (editor->kerning_adjustment,
                                (gdouble) kerning / PANGO_SCALE);
    }
  else
    {
      /* make sure the "" really gets replaced */
      g_signal_emit_by_name (editor->kerning_adjustment, "value-changed");
    }

  g_signal_handlers_unblock_by_func (editor->kerning_adjustment,
                                     ligma_text_style_editor_kerning_changed,
                                     editor);
}

static void
ligma_text_style_editor_update (LigmaTextStyleEditor *editor)
{
  if (editor->update_idle_id)
    g_source_remove (editor->update_idle_id);

  editor->update_idle_id =
    gdk_threads_add_idle ((GSourceFunc) ligma_text_style_editor_update_idle,
                          editor);
}

static gboolean
ligma_text_style_editor_update_idle (LigmaTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);

  if (editor->update_idle_id)
    {
      g_source_remove (editor->update_idle_id);
      editor->update_idle_id = 0;
    }

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter  start, end;
      GtkTextIter  iter;
      GList       *list;
      gboolean     any_toggle_active = TRUE;
      gboolean     font_differs      = FALSE;
      gboolean     color_differs     = FALSE;
      gboolean     size_differs      = FALSE;
      gboolean     baseline_differs  = FALSE;
      gboolean     kerning_differs   = FALSE;
      GtkTextTag  *font_tag          = NULL;
      GtkTextTag  *color_tag         = NULL;
      GtkTextTag  *size_tag          = NULL;
      GtkTextTag  *baseline_tag      = NULL;
      GtkTextTag  *kerning_tag       = NULL;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
      gtk_text_iter_order (&start, &end);

      /*  first, switch all toggles on  */
      for (list = editor->toggles; list; list = g_list_next (list))
        {
          GtkToggleButton *toggle = list->data;

          ligma_text_style_editor_set_toggle (editor, toggle, TRUE);
        }

      /*  and get some initial values  */
      font_tag     = ligma_text_buffer_get_iter_font (editor->buffer,
                                                     &start, NULL);
      color_tag    = ligma_text_buffer_get_iter_color (editor->buffer,
                                                      &start, NULL);
      size_tag     = ligma_text_buffer_get_iter_size (editor->buffer,
                                                     &start, NULL);
      baseline_tag = ligma_text_buffer_get_iter_baseline (editor->buffer,
                                                         &start, NULL);
      kerning_tag  = ligma_text_buffer_get_iter_kerning (editor->buffer,
                                                        &start, NULL);

      for (iter = start;
           gtk_text_iter_in_range (&iter, &start, &end);
           gtk_text_iter_forward_cursor_position (&iter))
        {
          if (any_toggle_active)
            {
              any_toggle_active = FALSE;

              for (list = editor->toggles; list; list = g_list_next (list))
                {
                  GtkToggleButton *toggle = list->data;
                  GtkTextTag      *tag    = g_object_get_data (G_OBJECT (toggle),
                                                               "tag");

                  if (! gtk_text_iter_has_tag (&iter, tag))
                    {
                      ligma_text_style_editor_set_toggle (editor, toggle, FALSE);
                    }
                  else
                    {
                      any_toggle_active = TRUE;
                    }
                }
            }

          if (! font_differs)
            {
              GtkTextTag *tag;

              tag = ligma_text_buffer_get_iter_font (editor->buffer, &iter,
                                                    NULL);

              if (tag != font_tag)
                font_differs = TRUE;
            }

          if (! color_differs)
            {
              GtkTextTag *tag;

              tag = ligma_text_buffer_get_iter_color (editor->buffer, &iter,
                                                     NULL);

              if (tag != color_tag)
                color_differs = TRUE;
            }

          if (! size_differs)
            {
              GtkTextTag *tag;

              tag = ligma_text_buffer_get_iter_size (editor->buffer, &iter,
                                                    NULL);

              if (tag != size_tag)
                size_differs = TRUE;
            }

          if (! baseline_differs)
            {
              GtkTextTag *tag;

              tag = ligma_text_buffer_get_iter_baseline (editor->buffer, &iter,
                                                        NULL);

              if (tag != baseline_tag)
                baseline_differs = TRUE;
            }

          if (! kerning_differs)
            {
              GtkTextTag *tag;

              tag = ligma_text_buffer_get_iter_kerning (editor->buffer, &iter,
                                                       NULL);

              if (tag != kerning_tag)
                kerning_differs = TRUE;
            }

          if (! any_toggle_active &&
              color_differs       &&
              font_differs        &&
              size_differs        &&
              baseline_differs    &&
              kerning_differs)
            break;
       }

      if (font_differs)
        ligma_text_style_editor_set_font (editor, NULL);
      else if (font_tag)
        ligma_text_style_editor_set_font (editor, font_tag);
      else
        ligma_text_style_editor_set_default_font (editor);

      if (color_differs)
        ligma_text_style_editor_set_color (editor, NULL);
      else if (color_tag)
        ligma_text_style_editor_set_color (editor, color_tag);
      else
        ligma_text_style_editor_set_default_color (editor);

      if (size_differs)
        ligma_text_style_editor_set_size (editor, NULL);
      else if (size_tag)
        ligma_text_style_editor_set_size (editor, size_tag);
      else
        ligma_text_style_editor_set_default_size (editor);

      if (baseline_differs)
        gtk_entry_set_text (GTK_ENTRY (editor->baseline_spinbutton), "");
      else
        ligma_text_style_editor_set_baseline (editor, baseline_tag);

      if (kerning_differs)
        gtk_entry_set_text (GTK_ENTRY (editor->kerning_spinbutton), "");
      else
        ligma_text_style_editor_set_kerning (editor, kerning_tag);
    }
  else /* no selection */
    {
      GtkTextIter  cursor;
      GSList      *tags;
      GSList      *tags_on;
      GSList      *tags_off;
      GList       *list;

      gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                        gtk_text_buffer_get_insert (buffer));

      tags     = gtk_text_iter_get_tags (&cursor);
      tags_on  = gtk_text_iter_get_toggled_tags (&cursor, TRUE);
      tags_off = gtk_text_iter_get_toggled_tags (&cursor, FALSE);

      for (list = editor->buffer->font_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              ligma_text_style_editor_set_font (editor, tag);
              break;
            }
        }

      if (! list)
        ligma_text_style_editor_set_default_font (editor);

      for (list = editor->buffer->color_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              ligma_text_style_editor_set_color (editor, tag);
              break;
            }
        }

      if (! list)
        ligma_text_style_editor_set_default_color (editor);

      for (list = editor->buffer->size_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              ligma_text_style_editor_set_size (editor, tag);
              break;
            }
        }

      if (! list)
        ligma_text_style_editor_set_default_size (editor);

      for (list = editor->buffer->baseline_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              ligma_text_style_editor_set_baseline (editor, tag);
              break;
            }
        }

      if (! list)
        ligma_text_style_editor_set_baseline (editor, NULL);

      for (list = editor->toggles; list; list = g_list_next (list))
        {
          GtkToggleButton *toggle = list->data;
          GtkTextTag      *tag    = g_object_get_data (G_OBJECT (toggle),
                                                       "tag");

          ligma_text_style_editor_set_toggle (editor, toggle,
                                             (g_slist_find (tags, tag) &&
                                              ! g_slist_find (tags_on, tag)) ||
                                             g_slist_find (tags_off, tag));
        }

      {
        GtkTextTag *tag;

        tag = ligma_text_buffer_get_iter_kerning (editor->buffer, &cursor, NULL);
        ligma_text_style_editor_set_kerning (editor, tag);
      }

      g_slist_free (tags);
      g_slist_free (tags_on);
      g_slist_free (tags_off);
    }

  if (editor->context->font_name &&
      g_strcmp0 (editor->context->font_name,
                 ligma_object_get_name (ligma_context_get_font (editor->context))))
    {
      /* A font is set, but is unavailable; change the help text. */
      gchar *help_text;

      help_text = g_strdup_printf (_("Font \"%s\" unavailable on this system"),
                                   editor->context->font_name);
      ligma_help_set_help_data (editor->font_entry, help_text, NULL);
      g_free (help_text);
    }
  else
    {
      ligma_help_set_help_data (editor->font_entry,
                               _("Change font of selected text"), NULL);
    }

  return FALSE;
}
