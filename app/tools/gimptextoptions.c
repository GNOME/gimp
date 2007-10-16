/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpconfig-utils.h"

#include "core/gimpviewable.h"

#include "text/gimptext.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimptextoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_FONT_SIZE,
  PROP_UNIT,
  PROP_HINTING,
  PROP_AUTOHINT,
  PROP_ANTIALIAS,
  PROP_LANGUAGE,
  PROP_BASE_DIR,
  PROP_JUSTIFICATION,
  PROP_INDENTATION,
  PROP_LINE_SPACING,
  PROP_LETTER_SPACING,

  PROP_FONT_VIEW_TYPE,
  PROP_FONT_VIEW_SIZE
};


static void  gimp_text_options_set_property       (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void  gimp_text_options_get_property       (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);

static void  gimp_text_options_notify_font        (GimpContext  *context,
                                                   GParamSpec   *pspec,
                                                   GimpText     *text);
static void  gimp_text_options_notify_text_font   (GimpText     *text,
                                                   GParamSpec   *pspec,
                                                   GimpContext  *context);
static void  gimp_text_options_notify_color       (GimpContext  *context,
                                                   GParamSpec   *pspec,
                                                   GimpText     *text);
static void  gimp_text_options_notify_text_color  (GimpText     *text,
                                                   GParamSpec   *pspec,
                                                   GimpContext  *context);


G_DEFINE_TYPE (GimpTextOptions, gimp_text_options, GIMP_TYPE_TOOL_OPTIONS)


static void
gimp_text_options_class_init (GimpTextOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_text_options_set_property;
  object_class->get_property = gimp_text_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_UNIT,
                                 "font-size-unit", NULL,
                                 TRUE, FALSE, GIMP_UNIT_PIXEL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_FONT_SIZE,
                                   "font-size", NULL,
                                   0.0, 8192.0, 18.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_HINTING,
                                    "hinting",
                                    N_("Hinting alters the font outline to "
                                       "produce a crisp bitmap at small "
                                       "sizes"),
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_AUTOHINT,
                                    "autohint",
                                    N_("If available, hints from the font are "
                                       "used but you may prefer to always use "
                                       "the automatic hinter"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                                    "antialias", NULL,
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_LANGUAGE,
                                   "language", NULL,
                                   (const gchar *) gtk_get_default_language (),
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_BASE_DIR,
                                "base-direction", NULL,
                                 GIMP_TYPE_TEXT_DIRECTION,
                                 GIMP_TEXT_DIRECTION_LTR,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_JUSTIFICATION,
                                "justify", NULL,
                                 GIMP_TYPE_TEXT_JUSTIFICATION,
                                 GIMP_TEXT_JUSTIFY_LEFT,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_INDENTATION,
                                   "indent",
                                   N_("Indentation of the first line"),
                                   -8192.0, 8192.0, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_LINE_SPACING,
                                   "line-spacing",
                                   N_("Adjust line spacing"),
                                   -8192.0, 8192.0, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_LETTER_SPACING,
                                   "letter-spacing",
                                   N_("Adjust letter spacing"),
                                   -8192.0, 8192.0, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_DEFAULTS);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_FONT_VIEW_TYPE,
                                 "font-view-type", NULL,
                                 GIMP_TYPE_VIEW_TYPE,
                                 GIMP_VIEW_TYPE_LIST,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_FONT_VIEW_SIZE,
                                "font-view-size", NULL,
                                GIMP_VIEW_SIZE_TINY,
                                GIMP_VIEWABLE_MAX_BUTTON_SIZE,
                                GIMP_VIEW_SIZE_SMALL,
                                GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_text_options_init (GimpTextOptions *options)
{
  options->size_entry           = NULL;
  options->to_vectors_button    = NULL;
  options->along_vectors_button = NULL;
}

static void
gimp_text_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpTextOptions *options = GIMP_TEXT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FONT_SIZE:
      g_value_set_double (value, options->font_size);
      break;
    case PROP_UNIT:
      g_value_set_int (value, options->unit);
      break;
    case PROP_HINTING:
      g_value_set_boolean (value, options->hinting);
      break;
    case PROP_AUTOHINT:
      g_value_set_boolean (value, options->autohint);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, options->antialias);
      break;
    case PROP_LANGUAGE:
      g_value_set_string (value, options->language);
      break;
    case PROP_BASE_DIR:
      g_value_set_enum (value, options->base_dir);
      break;
    case PROP_JUSTIFICATION:
      g_value_set_enum (value, options->justify);
      break;
    case PROP_INDENTATION:
      g_value_set_double (value, options->indent);
      break;
    case PROP_LINE_SPACING:
      g_value_set_double (value, options->line_spacing);
      break;
    case PROP_LETTER_SPACING:
      g_value_set_double (value, options->letter_spacing);
      break;

    case PROP_FONT_VIEW_TYPE:
      g_value_set_enum (value, options->font_view_type);
      break;
    case PROP_FONT_VIEW_SIZE:
      g_value_set_int (value, options->font_view_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_text_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpTextOptions *options = GIMP_TEXT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FONT_SIZE:
      options->font_size = g_value_get_double (value);
      break;
    case PROP_UNIT:
      options->unit = g_value_get_int (value);
      break;
    case PROP_HINTING:
      options->hinting = g_value_get_boolean (value);
      break;
    case PROP_AUTOHINT:
      options->autohint = g_value_get_boolean (value);
      break;
    case PROP_ANTIALIAS:
      options->antialias = g_value_get_boolean (value);
      break;
    case PROP_BASE_DIR:
      options->base_dir = g_value_get_enum (value);
      break;
    case PROP_LANGUAGE:
      g_free (options->language);
      options->language = g_value_dup_string (value);
      break;
    case PROP_JUSTIFICATION:
      options->justify = g_value_get_enum (value);
      break;
    case PROP_INDENTATION:
      options->indent = g_value_get_double (value);
      break;
    case PROP_LINE_SPACING:
      options->line_spacing = g_value_get_double (value);
      break;
    case PROP_LETTER_SPACING:
      options->letter_spacing = g_value_get_double (value);
      break;

    case PROP_FONT_VIEW_TYPE:
      options->font_view_type = g_value_get_enum (value);
      break;
    case PROP_FONT_VIEW_SIZE:
      options->font_view_size = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_text_options_notify_font (GimpContext *context,
                               GParamSpec  *pspec,
                               GimpText    *text)
{
  g_signal_handlers_block_by_func (text,
                                   gimp_text_options_notify_text_font,
                                   context);

  g_object_set (text, "font", gimp_context_get_font_name (context), NULL);

  g_signal_handlers_unblock_by_func (text,
                                     gimp_text_options_notify_text_font,
                                     context);
}

static void
gimp_text_options_notify_text_font (GimpText    *text,
                                    GParamSpec  *pspec,
                                    GimpContext *context)
{
  g_signal_handlers_block_by_func (context,
                                   gimp_text_options_notify_font, text);

  gimp_context_set_font_name (context, text->font);

  g_signal_handlers_unblock_by_func (context,
                                     gimp_text_options_notify_font, text);
}

static void
gimp_text_options_notify_color (GimpContext *context,
                                GParamSpec  *pspec,
                                GimpText    *text)
{
  GimpRGB  color;

  gimp_context_get_foreground (context, &color);

  g_signal_handlers_block_by_func (text,
                                   gimp_text_options_notify_text_color,
                                   context);

  g_object_set (text, "color", &color, NULL);

  g_signal_handlers_unblock_by_func (text,
                                     gimp_text_options_notify_text_color,
                                     context);
}

static void
gimp_text_options_notify_text_color (GimpText    *text,
                                     GParamSpec  *pspec,
                                     GimpContext *context)
{
  g_signal_handlers_block_by_func (context,
                                   gimp_text_options_notify_color, text);

  gimp_context_set_foreground (context, &text->color);

  g_signal_handlers_unblock_by_func (context,
                                     gimp_text_options_notify_color, text);
}

/*  This function could live in gimptexttool.c also.
 *  But it takes some bloat out of that file...
 */
void
gimp_text_options_connect_text (GimpTextOptions *options,
                                GimpText        *text)
{
  GimpContext *context;
  GimpRGB      color;

  g_return_if_fail (GIMP_IS_TEXT_OPTIONS (options));
  g_return_if_fail (GIMP_IS_TEXT (text));

  context = GIMP_CONTEXT (options);

  gimp_context_get_foreground (context, &color);

  gimp_config_sync (G_OBJECT (options), G_OBJECT (text), 0);

  g_object_set (text,
                "color", &color,
                "font",  gimp_context_get_font_name (context),
                NULL);

  gimp_config_connect (G_OBJECT (options), G_OBJECT (text), NULL);

  g_signal_connect_object (options, "notify::font",
                           G_CALLBACK (gimp_text_options_notify_font),
                           text, 0);
  g_signal_connect_object (text, "notify::font",
                           G_CALLBACK (gimp_text_options_notify_text_font),
                           options, 0);

  g_signal_connect_object (options, "notify::foreground",
                           G_CALLBACK (gimp_text_options_notify_color),
                           text, 0);
  g_signal_connect_object (text, "notify::color",
                           G_CALLBACK (gimp_text_options_notify_text_color),
                           options, 0);
}

GtkWidget *
gimp_text_options_gui (GimpToolOptions *tool_options)
{
  GObject         *config  = G_OBJECT (tool_options);
  GimpTextOptions *options = GIMP_TEXT_OPTIONS (tool_options);
  GtkWidget       *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget       *table;
  GtkWidget       *hbox;
  GtkWidget       *button;
  GtkWidget       *auto_button;
  GtkWidget       *entry;
  GtkWidget       *box;
  GtkWidget       *spinbutton;
  gint             row = 0;

  table = gtk_table_new (10, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  hbox = gimp_prop_font_box_new (NULL, GIMP_CONTEXT (tool_options), 2,
                                 "font-view-type", "font-view-size");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Font:"), 0.0, 0.5,
                             hbox, 2, FALSE);

  entry = gimp_prop_size_entry_new (config,
                                    "font-size", FALSE, "font-size-unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 72.0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Size:"), 0.0, 0.5,
                             entry, 2, FALSE);

  options->size_entry = entry;

  button = gimp_prop_check_button_new (config, "hinting", _("Hinting"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 3, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);
  row++;

  auto_button = gimp_prop_check_button_new (config, "autohint",
                                            _("Force auto-hinter"));
  gtk_table_attach (GTK_TABLE (table), auto_button, 0, 3, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (auto_button);
  row++;

  gtk_widget_set_sensitive (auto_button, options->hinting);
  g_object_set_data (G_OBJECT (button), "set_sensitive", auto_button);

  button = gimp_prop_check_button_new (config, "antialias", _("Antialiasing"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 3, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);
  row++;

  button = gimp_prop_color_button_new (config, "foreground", _("Text Color"),
                                       -1, 24, GIMP_COLOR_AREA_FLAT);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (button),
                                GIMP_CONTEXT (options));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Color:"), 0.0, 0.5,
                             button, 1, FALSE);

  box = gimp_prop_enum_stock_box_new (config, "justify", "gtk-justify", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Justify:"), 0.0, 0.5,
                             box, 2, TRUE);

  spinbutton = gimp_prop_spin_button_new (config, "indent", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gimp_table_attach_stock (GTK_TABLE (table), row++,
                           GTK_STOCK_INDENT, spinbutton, 1, TRUE);

  spinbutton = gimp_prop_spin_button_new (config, "line-spacing", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gimp_table_attach_stock (GTK_TABLE (table), row++,
                           GIMP_STOCK_LINE_SPACING, spinbutton, 1, TRUE);

  spinbutton = gimp_prop_spin_button_new (config,
                                          "letter-spacing", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gimp_table_attach_stock (GTK_TABLE (table), row++,
                           GIMP_STOCK_LETTER_SPACING, spinbutton, 1, TRUE);

  /*  Create a path from the current text  */
  button = gtk_button_new_with_label (_("Path from Text"));
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  gtk_widget_show (button);

  options->to_vectors_button = button;

  button = gtk_button_new_with_label (_("Text along Path"));
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  gtk_widget_show (button);

  options->along_vectors_button = button;

  return vbox;
}

static void
gimp_text_options_editor_dir_changed (GimpTextEditor  *editor,
                                      GimpTextOptions *options)
{
  g_object_set (options,
                "base-direction", editor->base_dir,
                NULL);
}

static void
gimp_text_options_editor_notify_dir (GimpTextOptions *options,
                                     GParamSpec      *pspec,
                                     GimpTextEditor  *editor)
{
  GimpTextDirection  dir;

  g_object_get (options,
                "base-direction", &dir,
                NULL);

  gimp_text_editor_set_direction (editor, dir);
}

static void
gimp_text_options_editor_notify_font (GimpTextOptions *options,
                                      GParamSpec      *pspec,
                                      GimpTextEditor  *editor)
{
  const gchar *font_name;

  font_name = gimp_context_get_font_name (GIMP_CONTEXT (options));

  gimp_text_editor_set_font_name (editor, font_name);
}

GtkWidget *
gimp_text_options_editor_new (GtkWindow       *parent,
                              GimpTextOptions *options,
                              GimpMenuFactory *menu_factory,
                              const gchar     *title)
{
  GtkWidget   *editor;
  const gchar *font_name;

  g_return_val_if_fail (GIMP_IS_TEXT_OPTIONS (options), NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (title != NULL, NULL);

  editor = gimp_text_editor_new (title, parent, menu_factory);

  font_name = gimp_context_get_font_name (GIMP_CONTEXT (options));

  gimp_text_editor_set_direction (GIMP_TEXT_EDITOR (editor),
                                  options->base_dir);
  gimp_text_editor_set_font_name (GIMP_TEXT_EDITOR (editor),
                                  font_name);

  g_signal_connect_object (editor, "dir-changed",
                           G_CALLBACK (gimp_text_options_editor_dir_changed),
                           options, 0);
  g_signal_connect_object (options, "notify::base-direction",
                           G_CALLBACK (gimp_text_options_editor_notify_dir),
                           editor, 0);
  g_signal_connect_object (options, "notify::font",
                           G_CALLBACK (gimp_text_options_editor_notify_font),
                           editor, 0);

  return editor;
}
