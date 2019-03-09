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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"

#include "text/gimptext.h"

#include "gimpcolorpanel.h"
#include "gimpcontainerentry.h"
#include "gimpcontainerview.h"
#include "gimptextbuffer.h"
#include "gimptextstyleeditor.h"
#include "gimptexttag.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_TEXT,
  PROP_BUFFER,
  PROP_FONTS,
  PROP_RESOLUTION_X,
  PROP_RESOLUTION_Y
};


static void      gimp_text_style_editor_constructed       (GObject             *object);
static void      gimp_text_style_editor_dispose           (GObject             *object);
static void      gimp_text_style_editor_finalize          (GObject             *object);
static void      gimp_text_style_editor_set_property      (GObject             *object,
                                                           guint                property_id,
                                                           const GValue        *value,
                                                           GParamSpec          *pspec);
static void      gimp_text_style_editor_get_property      (GObject             *object,
                                                           guint                property_id,
                                                           GValue              *value,
                                                           GParamSpec          *pspec);

static GtkWidget * gimp_text_style_editor_create_toggle   (GimpTextStyleEditor *editor,
                                                           GtkTextTag          *tag,
                                                           const gchar         *icon_name,
                                                           const gchar         *tooltip);

static void      gimp_text_style_editor_clear_tags        (GtkButton           *button,
                                                           GimpTextStyleEditor *editor);

static void      gimp_text_style_editor_font_changed      (GimpContext         *context,
                                                           GimpFont            *font,
                                                           GimpTextStyleEditor *editor);
static void      gimp_text_style_editor_set_font          (GimpTextStyleEditor *editor,
                                                           GtkTextTag          *font_tag);
static void      gimp_text_style_editor_set_default_font  (GimpTextStyleEditor *editor);

static void      gimp_text_style_editor_color_changed     (GimpColorButton     *button,
                                                           GimpTextStyleEditor *editor);
static void      gimp_text_style_editor_set_color         (GimpTextStyleEditor *editor,
                                                           GtkTextTag          *color_tag);
static void      gimp_text_style_editor_set_default_color (GimpTextStyleEditor *editor);

static void      gimp_text_style_editor_tag_toggled       (GtkToggleButton     *toggle,
                                                           GimpTextStyleEditor *editor);
static void      gimp_text_style_editor_set_toggle        (GimpTextStyleEditor *editor,
                                                           GtkToggleButton     *toggle,
                                                           gboolean             active);

static void      gimp_text_style_editor_size_changed      (GimpSizeEntry       *entry,
                                                           GimpTextStyleEditor *editor);
static void      gimp_text_style_editor_set_size          (GimpTextStyleEditor *editor,
                                                           GtkTextTag          *size_tag);
static void      gimp_text_style_editor_set_default_size  (GimpTextStyleEditor *editor);

static void      gimp_text_style_editor_baseline_changed  (GtkAdjustment       *adjustment,
                                                           GimpTextStyleEditor *editor);
static void      gimp_text_style_editor_set_baseline      (GimpTextStyleEditor *editor,
                                                           GtkTextTag          *baseline_tag);

static void      gimp_text_style_editor_kerning_changed   (GtkAdjustment       *adjustment,
                                                           GimpTextStyleEditor *editor);
static void      gimp_text_style_editor_set_kerning       (GimpTextStyleEditor *editor,
                                                           GtkTextTag          *kerning_tag);

static void      gimp_text_style_editor_update            (GimpTextStyleEditor *editor);
static gboolean  gimp_text_style_editor_update_idle       (GimpTextStyleEditor *editor);


G_DEFINE_TYPE (GimpTextStyleEditor, gimp_text_style_editor,
               GTK_TYPE_BOX)

#define parent_class gimp_text_style_editor_parent_class


static void
gimp_text_style_editor_class_init (GimpTextStyleEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_text_style_editor_constructed;
  object_class->dispose      = gimp_text_style_editor_dispose;
  object_class->finalize     = gimp_text_style_editor_finalize;
  object_class->set_property = gimp_text_style_editor_set_property;
  object_class->get_property = gimp_text_style_editor_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TEXT,
                                   g_param_spec_object ("text",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TEXT_BUFFER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_FONTS,
                                   g_param_spec_object ("fonts",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTAINER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_RESOLUTION_X,
                                   g_param_spec_double ("resolution-x",
                                                        NULL, NULL,
                                                        GIMP_MIN_RESOLUTION,
                                                        GIMP_MAX_RESOLUTION,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RESOLUTION_Y,
                                   g_param_spec_double ("resolution-y",
                                                        NULL, NULL,
                                                        GIMP_MIN_RESOLUTION,
                                                        GIMP_MAX_RESOLUTION,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_text_style_editor_init (GimpTextStyleEditor *editor)
{
  GtkWidget *image;
  GimpRGB    color;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);
  gtk_box_set_spacing (GTK_BOX (editor), 2);

  /*  upper row  */

  editor->upper_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (editor), editor->upper_hbox, FALSE, FALSE, 0);
  gtk_widget_show (editor->upper_hbox);

  editor->font_entry = gimp_container_entry_new (NULL, NULL,
                                                 GIMP_VIEW_SIZE_SMALL, 1);
  gtk_box_pack_start (GTK_BOX (editor->upper_hbox), editor->font_entry,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->font_entry);

  gimp_help_set_help_data (editor->font_entry,
                           _("Change font of selected text"), NULL);

  editor->size_entry =
    gimp_size_entry_new (1, 0, "%a", TRUE, FALSE, FALSE, 10,
                         GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (editor->size_entry), 1, 0);
  gtk_box_pack_start (GTK_BOX (editor->upper_hbox), editor->size_entry,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->size_entry);

  gimp_help_set_help_data (editor->size_entry,
                           _("Change size of selected text"), NULL);

  g_signal_connect (editor->size_entry, "value-changed",
                    G_CALLBACK (gimp_text_style_editor_size_changed),
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

  gimp_help_set_help_data (editor->clear_button,
                           _("Clear style of selected text"), NULL);

  g_signal_connect (editor->clear_button, "clicked",
                    G_CALLBACK (gimp_text_style_editor_clear_tags),
                    editor);

  image = gtk_image_new_from_icon_name ("edit-clear", GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (editor->clear_button), image);
  gtk_widget_show (image);

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);
  editor->color_button = gimp_color_panel_new (_("Change color of selected text"),
                                               &color,
                                               GIMP_COLOR_AREA_FLAT, 20, 20);
  gimp_widget_set_fully_opaque (editor->color_button, TRUE);

  gtk_box_pack_end (GTK_BOX (editor->lower_hbox), editor->color_button,
                    FALSE, FALSE, 0);
  gtk_widget_show (editor->color_button);

  gimp_help_set_help_data (editor->color_button,
                           _("Change color of selected text"), NULL);

  g_signal_connect (editor->color_button, "color-changed",
                    G_CALLBACK (gimp_text_style_editor_color_changed),
                    editor);

  editor->kerning_adjustment =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, -1000.0, 1000.0, 1.0, 10.0, 0.0));
  editor->kerning_spinbutton =
    gimp_spin_button_new (editor->kerning_adjustment, 1.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (editor->kerning_spinbutton), 5);
  gtk_box_pack_end (GTK_BOX (editor->lower_hbox), editor->kerning_spinbutton,
                    FALSE, FALSE, 0);
  gtk_widget_show (editor->kerning_spinbutton);

  gimp_help_set_help_data (editor->kerning_spinbutton,
                           _("Change kerning of selected text"), NULL);

  g_signal_connect (editor->kerning_adjustment, "value-changed",
                    G_CALLBACK (gimp_text_style_editor_kerning_changed),
                    editor);

  editor->baseline_adjustment =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, -1000.0, 1000.0, 1.0, 10.0, 0.0));
  editor->baseline_spinbutton =
    gimp_spin_button_new (editor->baseline_adjustment, 1.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (editor->baseline_spinbutton), 5);
  gtk_box_pack_end (GTK_BOX (editor->lower_hbox), editor->baseline_spinbutton,
                    FALSE, FALSE, 0);
  gtk_widget_show (editor->baseline_spinbutton);

  gimp_help_set_help_data (editor->baseline_spinbutton,
                           _("Change baseline of selected text"), NULL);

  g_signal_connect (editor->baseline_adjustment, "value-changed",
                    G_CALLBACK (gimp_text_style_editor_baseline_changed),
                    editor);
}

static void
gimp_text_style_editor_constructed (GObject *object)
{
  GimpTextStyleEditor *editor = GIMP_TEXT_STYLE_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (editor->gimp));
  gimp_assert (GIMP_IS_CONTAINER (editor->fonts));
  gimp_assert (GIMP_IS_TEXT (editor->text));
  gimp_assert (GIMP_IS_TEXT_BUFFER (editor->buffer));

  editor->context = gimp_context_new (editor->gimp, "text style editor", NULL);

  g_signal_connect (editor->context, "font-changed",
                    G_CALLBACK (gimp_text_style_editor_font_changed),
                    editor);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_entry), 0,
                                  editor->resolution_y, TRUE);

  /* use the global user context so we get the global FG/BG colors */
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (editor->color_button),
                                gimp_get_user_context (editor->gimp));

  gimp_container_view_set_container (GIMP_CONTAINER_VIEW (editor->font_entry),
                                     editor->fonts);
  gimp_container_view_set_context (GIMP_CONTAINER_VIEW (editor->font_entry),
                                   editor->context);

  gimp_text_style_editor_create_toggle (editor, editor->buffer->bold_tag,
                                        GIMP_ICON_FORMAT_TEXT_BOLD,
                                        _("Bold"));
  gimp_text_style_editor_create_toggle (editor, editor->buffer->italic_tag,
                                        GIMP_ICON_FORMAT_TEXT_ITALIC,
                                        _("Italic"));
  gimp_text_style_editor_create_toggle (editor, editor->buffer->underline_tag,
                                        GIMP_ICON_FORMAT_TEXT_UNDERLINE,
                                        _("Underline"));
  gimp_text_style_editor_create_toggle (editor, editor->buffer->strikethrough_tag,
                                        GIMP_ICON_FORMAT_TEXT_STRIKETHROUGH,
                                        _("Strikethrough"));

  g_signal_connect_swapped (editor->text, "notify::font",
                            G_CALLBACK (gimp_text_style_editor_update),
                            editor);
  g_signal_connect_swapped (editor->text, "notify::font-size",
                            G_CALLBACK (gimp_text_style_editor_update),
                            editor);
  g_signal_connect_swapped (editor->text, "notify::font-size-unit",
                            G_CALLBACK (gimp_text_style_editor_update),
                            editor);
  g_signal_connect_swapped (editor->text, "notify::color",
                            G_CALLBACK (gimp_text_style_editor_update),
                            editor);

  g_signal_connect_data (editor->buffer, "changed",
                         G_CALLBACK (gimp_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_data (editor->buffer, "apply-tag",
                         G_CALLBACK (gimp_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_data (editor->buffer, "remove-tag",
                         G_CALLBACK (gimp_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);
  g_signal_connect_data (editor->buffer, "mark-set",
                         G_CALLBACK (gimp_text_style_editor_update),
                         editor, 0,
                         G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  gimp_text_style_editor_update (editor);
}

static void
gimp_text_style_editor_dispose (GObject *object)
{
  GimpTextStyleEditor *editor = GIMP_TEXT_STYLE_EDITOR (object);

  if (editor->text)
    {
      g_signal_handlers_disconnect_by_func (editor->text,
                                            gimp_text_style_editor_update,
                                            editor);
    }

  if (editor->buffer)
    {
      g_signal_handlers_disconnect_by_func (editor->buffer,
                                            gimp_text_style_editor_update,
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
gimp_text_style_editor_finalize (GObject *object)
{
  GimpTextStyleEditor *editor = GIMP_TEXT_STYLE_EDITOR (object);

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
gimp_text_style_editor_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpTextStyleEditor *editor = GIMP_TEXT_STYLE_EDITOR (object);

  switch (property_id)
    {
    case PROP_GIMP:
      editor->gimp = g_value_get_object (value); /* don't ref */
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
        gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_entry), 0,
                                        editor->resolution_y, TRUE);
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
    case PROP_GIMP:
      g_value_set_object (value, editor->gimp);
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
gimp_text_style_editor_new (Gimp           *gimp,
                            GimpText       *text,
                            GimpTextBuffer *buffer,
                            GimpContainer  *fonts,
                            gdouble         resolution_x,
                            gdouble         resolution_y)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (resolution_x > 0.0, NULL);
  g_return_val_if_fail (resolution_y > 0.0, NULL);

  return g_object_new (GIMP_TYPE_TEXT_STYLE_EDITOR,
                       "gimp",         gimp,
                       "text",         text,
                       "buffer",       buffer,
                       "fonts",        fonts,
                       "resolution-x", resolution_x,
                       "resolution-y", resolution_y,
                       NULL);
}

GList *
gimp_text_style_editor_list_tags (GimpTextStyleEditor  *editor,
                                  GList               **remove_tags)
{
  GList *toggles;
  GList *tags = NULL;

  g_return_val_if_fail (GIMP_IS_TEXT_STYLE_EDITOR (editor), NULL);
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

    pixels = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (editor->size_entry), 0);

    if (pixels != 0.0)
      {
        GtkTextTag *tag;
        gdouble     points;

        points = gimp_units_to_points (pixels,
                                       GIMP_UNIT_PIXEL,
                                       editor->resolution_y);
        tag = gimp_text_buffer_get_size_tag (editor->buffer,
                                             PANGO_SCALE * points);
        tags = g_list_prepend (tags, tag);
      }
  }

  {
    GList       *list;
    const gchar *font_name;

    for (list = editor->buffer->font_tags; list; list = g_list_next (list))
      *remove_tags = g_list_prepend (*remove_tags, list->data);

    font_name = gimp_context_get_font_name (editor->context);

    if (font_name)
      {
        GtkTextTag  *tag;

        tag = gimp_text_buffer_get_font_tag (editor->buffer, font_name);
        tags = g_list_prepend (tags, tag);
      }
  }

  {
    GList   *list;
    GimpRGB  color;

    for (list = editor->buffer->color_tags; list; list = g_list_next (list))
      *remove_tags = g_list_prepend (*remove_tags, list->data);

    gimp_color_button_get_color (GIMP_COLOR_BUTTON (editor->color_button),
                                 &color);

    if (TRUE) /* FIXME should have "inconsistent" state as for font and size */
      {
        GtkTextTag *tag;

        tag = gimp_text_buffer_get_color_tag (editor->buffer, &color);
        tags = g_list_prepend (tags, tag);
      }
  }

  *remove_tags = g_list_reverse (*remove_tags);

  return g_list_reverse (tags);
}


/*  private functions  */

static GtkWidget *
gimp_text_style_editor_create_toggle (GimpTextStyleEditor *editor,
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

  gimp_help_set_help_data (toggle, tooltip, NULL);

  editor->toggles = g_list_append (editor->toggles, toggle);
  g_object_set_data (G_OBJECT (toggle), "tag", tag);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_text_style_editor_tag_toggled),
                    editor);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
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

      gtk_text_buffer_begin_user_action (buffer);

      gtk_text_buffer_remove_all_tags (buffer, &start, &end);

      gtk_text_buffer_end_user_action (buffer);
    }
}

static void
gimp_text_style_editor_font_changed (GimpContext         *context,
                                     GimpFont            *font,
                                     GimpTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GList         *insert_tags;
  GList         *remove_tags;

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      gimp_text_buffer_set_font (editor->buffer, &start, &end,
                                 gimp_context_get_font_name (context));
    }

  insert_tags = gimp_text_style_editor_list_tags (editor, &remove_tags);
  gimp_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
gimp_text_style_editor_set_font (GimpTextStyleEditor *editor,
                                 GtkTextTag          *font_tag)
{
  gchar *font = NULL;

  if (font_tag)
    font = gimp_text_tag_get_font (font_tag);

  g_signal_handlers_block_by_func (editor->context,
                                   gimp_text_style_editor_font_changed,
                                   editor);

  gimp_context_set_font_name (editor->context, font);

  g_signal_handlers_unblock_by_func (editor->context,
                                     gimp_text_style_editor_font_changed,
                                     editor);

  g_free (font);
}

static void
gimp_text_style_editor_set_default_font (GimpTextStyleEditor *editor)
{
  g_signal_handlers_block_by_func (editor->context,
                                   gimp_text_style_editor_font_changed,
                                   editor);

  gimp_context_set_font_name (editor->context, editor->text->font);

  g_signal_handlers_unblock_by_func (editor->context,
                                     gimp_text_style_editor_font_changed,
                                     editor);
}

static void
gimp_text_style_editor_color_changed (GimpColorButton     *button,
                                      GimpTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GList         *insert_tags;
  GList         *remove_tags;

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;
      GimpRGB     color;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      gimp_color_button_get_color (button, &color);
      gimp_text_buffer_set_color (editor->buffer, &start, &end, &color);
    }

  insert_tags = gimp_text_style_editor_list_tags (editor, &remove_tags);
  gimp_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
gimp_text_style_editor_set_color (GimpTextStyleEditor *editor,
                                  GtkTextTag          *color_tag)
{
  GimpRGB color;

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);

  if (color_tag)
    gimp_text_tag_get_fg_color (color_tag, &color);

  g_signal_handlers_block_by_func (editor->color_button,
                                   gimp_text_style_editor_color_changed,
                                   editor);

  gimp_color_button_set_color (GIMP_COLOR_BUTTON (editor->color_button),
                               &color);

  /* FIXME should have "inconsistent" state as for font and size */

  g_signal_handlers_unblock_by_func (editor->color_button,
                                     gimp_text_style_editor_color_changed,
                                     editor);
}

static void
gimp_text_style_editor_set_default_color (GimpTextStyleEditor *editor)
{
  g_signal_handlers_block_by_func (editor->color_button,
                                   gimp_text_style_editor_color_changed,
                                   editor);

  gimp_color_button_set_color (GIMP_COLOR_BUTTON (editor->color_button),
                               &editor->text->color);

  g_signal_handlers_unblock_by_func (editor->color_button,
                                     gimp_text_style_editor_color_changed,
                                     editor);
}

static void
gimp_text_style_editor_tag_toggled (GtkToggleButton     *toggle,
                                    GimpTextStyleEditor *editor)
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

  insert_tags = gimp_text_style_editor_list_tags (editor, &remove_tags);
  gimp_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
gimp_text_style_editor_set_toggle (GimpTextStyleEditor *editor,
                                   GtkToggleButton     *toggle,
                                   gboolean             active)
{
  g_signal_handlers_block_by_func (toggle,
                                   gimp_text_style_editor_tag_toggled,
                                   editor);

  gtk_toggle_button_set_active (toggle, active);

  g_signal_handlers_unblock_by_func (toggle,
                                     gimp_text_style_editor_tag_toggled,
                                     editor);
}

static void
gimp_text_style_editor_size_changed (GimpSizeEntry       *entry,
                                     GimpTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GList         *insert_tags;
  GList         *remove_tags;

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start, end;
      gdouble     points;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      points = gimp_units_to_points (gimp_size_entry_get_refval (entry, 0),
                                     GIMP_UNIT_PIXEL,
                                     editor->resolution_y);

      gimp_text_buffer_set_size (editor->buffer, &start, &end,
                                 PANGO_SCALE * points);
    }

  insert_tags = gimp_text_style_editor_list_tags (editor, &remove_tags);
  gimp_text_buffer_set_insert_tags (editor->buffer, insert_tags, remove_tags);
}

static void
gimp_text_style_editor_set_size (GimpTextStyleEditor *editor,
                                 GtkTextTag          *size_tag)
{
  gint    size = 0;
  gdouble pixels;

  if (size_tag)
    size = gimp_text_tag_get_size (size_tag);

  g_signal_handlers_block_by_func (editor->size_entry,
                                   gimp_text_style_editor_size_changed,
                                   editor);

  pixels = gimp_units_to_pixels ((gdouble) size / PANGO_SCALE,
                                 GIMP_UNIT_POINT,
                                 editor->resolution_y);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (editor->size_entry), 0, pixels);

  if (size == 0)
    {
      GtkWidget *spinbutton;

      spinbutton = gimp_size_entry_get_help_widget (GIMP_SIZE_ENTRY (editor->size_entry), 0);

      gtk_entry_set_text (GTK_ENTRY (spinbutton), "");
    }

  g_signal_handlers_unblock_by_func (editor->size_entry,
                                     gimp_text_style_editor_size_changed,
                                     editor);
}

static void
gimp_text_style_editor_set_default_size (GimpTextStyleEditor *editor)
{
  gdouble pixels = gimp_units_to_pixels (editor->text->font_size,
                                         editor->text->unit,
                                         editor->resolution_y);

  g_signal_handlers_block_by_func (editor->size_entry,
                                   gimp_text_style_editor_size_changed,
                                   editor);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (editor->size_entry), 0, pixels);

  g_signal_handlers_unblock_by_func (editor->size_entry,
                                     gimp_text_style_editor_size_changed,
                                     editor);
}

static void
gimp_text_style_editor_baseline_changed (GtkAdjustment       *adjustment,
                                         GimpTextStyleEditor *editor)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (editor->buffer);
  GtkTextIter    start, end;

  if (! gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                        gtk_text_buffer_get_insert (buffer));
      gtk_text_buffer_get_end_iter (buffer, &end);
    }

  gimp_text_buffer_set_baseline (editor->buffer, &start, &end,
                                 gtk_adjustment_get_value (adjustment) *
                                 PANGO_SCALE);
}

static void
gimp_text_style_editor_set_baseline (GimpTextStyleEditor *editor,
                                     GtkTextTag          *baseline_tag)
{
  gint baseline = 0;

  if (baseline_tag)
    baseline = gimp_text_tag_get_baseline (baseline_tag);

  g_signal_handlers_block_by_func (editor->baseline_adjustment,
                                   gimp_text_style_editor_baseline_changed,
                                   editor);

  gtk_adjustment_set_value (editor->baseline_adjustment,
                            (gdouble) baseline / PANGO_SCALE);
  /* make sure the "" really gets replaced */
  gtk_adjustment_value_changed (editor->baseline_adjustment);

  g_signal_handlers_unblock_by_func (editor->baseline_adjustment,
                                     gimp_text_style_editor_baseline_changed,
                                     editor);
}

static void
gimp_text_style_editor_kerning_changed (GtkAdjustment       *adjustment,
                                        GimpTextStyleEditor *editor)
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

  gimp_text_buffer_set_kerning (editor->buffer, &start, &end,
                                gtk_adjustment_get_value (adjustment) *
                                PANGO_SCALE);
}

static void
gimp_text_style_editor_set_kerning (GimpTextStyleEditor *editor,
                                    GtkTextTag          *kerning_tag)
{
  gint kerning = 0;

  if (kerning_tag)
    kerning = gimp_text_tag_get_kerning (kerning_tag);

  g_signal_handlers_block_by_func (editor->kerning_adjustment,
                                   gimp_text_style_editor_kerning_changed,
                                   editor);

  gtk_adjustment_set_value (editor->kerning_adjustment,
                            (gdouble) kerning / PANGO_SCALE);
  /* make sure the "" really gets replaced */
  gtk_adjustment_value_changed (editor->kerning_adjustment);

  g_signal_handlers_unblock_by_func (editor->kerning_adjustment,
                                     gimp_text_style_editor_kerning_changed,
                                     editor);
}

static void
gimp_text_style_editor_update (GimpTextStyleEditor *editor)
{
  if (editor->update_idle_id)
    g_source_remove (editor->update_idle_id);

  editor->update_idle_id =
    gdk_threads_add_idle ((GSourceFunc) gimp_text_style_editor_update_idle,
                          editor);
}

static gboolean
gimp_text_style_editor_update_idle (GimpTextStyleEditor *editor)
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

          gimp_text_style_editor_set_toggle (editor, toggle, TRUE);
        }

      /*  and get some initial values  */
      font_tag     = gimp_text_buffer_get_iter_font (editor->buffer,
                                                     &start, NULL);
      color_tag    = gimp_text_buffer_get_iter_color (editor->buffer,
                                                      &start, NULL);
      size_tag     = gimp_text_buffer_get_iter_size (editor->buffer,
                                                     &start, NULL);
      baseline_tag = gimp_text_buffer_get_iter_baseline (editor->buffer,
                                                         &start, NULL);
      kerning_tag  = gimp_text_buffer_get_iter_kerning (editor->buffer,
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
                      gimp_text_style_editor_set_toggle (editor, toggle, FALSE);
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

              tag = gimp_text_buffer_get_iter_font (editor->buffer, &iter,
                                                    NULL);

              if (tag != font_tag)
                font_differs = TRUE;
            }

          if (! color_differs)
            {
              GtkTextTag *tag;

              tag = gimp_text_buffer_get_iter_color (editor->buffer, &iter,
                                                     NULL);

              if (tag != color_tag)
                color_differs = TRUE;
            }

          if (! size_differs)
            {
              GtkTextTag *tag;

              tag = gimp_text_buffer_get_iter_size (editor->buffer, &iter,
                                                    NULL);

              if (tag != size_tag)
                size_differs = TRUE;
            }

          if (! baseline_differs)
            {
              GtkTextTag *tag;

              tag = gimp_text_buffer_get_iter_baseline (editor->buffer, &iter,
                                                        NULL);

              if (tag != baseline_tag)
                baseline_differs = TRUE;
            }

          if (! kerning_differs)
            {
              GtkTextTag *tag;

              tag = gimp_text_buffer_get_iter_kerning (editor->buffer, &iter,
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
        gimp_text_style_editor_set_font (editor, NULL);
      else if (font_tag)
        gimp_text_style_editor_set_font (editor, font_tag);
      else
        gimp_text_style_editor_set_default_font (editor);

      if (color_differs)
        gimp_text_style_editor_set_color (editor, NULL);
      else if (color_tag)
        gimp_text_style_editor_set_color (editor, color_tag);
      else
        gimp_text_style_editor_set_default_color (editor);

      if (size_differs)
        gimp_text_style_editor_set_size (editor, NULL);
      else if (size_tag)
        gimp_text_style_editor_set_size (editor, size_tag);
      else
        gimp_text_style_editor_set_default_size (editor);

      if (baseline_differs)
        gtk_entry_set_text (GTK_ENTRY (editor->baseline_spinbutton), "");
      else
        gimp_text_style_editor_set_baseline (editor, baseline_tag);

      if (kerning_differs)
        gtk_entry_set_text (GTK_ENTRY (editor->kerning_spinbutton), "");
      else
        gimp_text_style_editor_set_kerning (editor, kerning_tag);
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
              gimp_text_style_editor_set_font (editor, tag);
              break;
            }
        }

      if (! list)
        gimp_text_style_editor_set_default_font (editor);

      for (list = editor->buffer->color_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              gimp_text_style_editor_set_color (editor, tag);
              break;
            }
        }

      if (! list)
        gimp_text_style_editor_set_default_color (editor);

      for (list = editor->buffer->size_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              gimp_text_style_editor_set_size (editor, tag);
              break;
            }
        }

      if (! list)
        gimp_text_style_editor_set_default_size (editor);

      for (list = editor->buffer->baseline_tags; list; list = g_list_next (list))
        {
          GtkTextTag *tag = list->data;

          if ((g_slist_find (tags, tag) &&
               ! g_slist_find (tags_on, tag)) ||
              g_slist_find (tags_off, tag))
            {
              gimp_text_style_editor_set_baseline (editor, tag);
              break;
            }
        }

      if (! list)
        gimp_text_style_editor_set_baseline (editor, NULL);

      for (list = editor->toggles; list; list = g_list_next (list))
        {
          GtkToggleButton *toggle = list->data;
          GtkTextTag      *tag    = g_object_get_data (G_OBJECT (toggle),
                                                       "tag");

          gimp_text_style_editor_set_toggle (editor, toggle,
                                             (g_slist_find (tags, tag) &&
                                              ! g_slist_find (tags_on, tag)) ||
                                             g_slist_find (tags_off, tag));
        }

      {
        GtkTextTag *tag;

        tag = gimp_text_buffer_get_iter_kerning (editor->buffer, &cursor, NULL);
        gimp_text_style_editor_set_kerning (editor, tag);
      }

      g_slist_free (tags);
      g_slist_free (tags_on);
      g_slist_free (tags_off);
    }

  if (editor->context->font_name &&
      g_strcmp0 (editor->context->font_name,
                 gimp_object_get_name (gimp_context_get_font (editor->context))))
    {
      /* A font is set, but is unavailable; change the help text. */
      gchar *help_text;

      help_text = g_strdup_printf (_("Font \"%s\" unavailable on this system"),
                                   editor->context->font_name);
      gimp_help_set_help_data (editor->font_entry, help_text, NULL);
      g_free (help_text);
    }
  else
    {
      gimp_help_set_help_data (editor->font_entry,
                               _("Change font of selected text"), NULL);
    }

  return FALSE;
}
