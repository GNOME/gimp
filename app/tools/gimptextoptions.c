/* The GIMP -- an image manipulation program
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
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-params.h"
#include "config/gimpconfig-utils.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimptoolinfo.h"

#include "text/gimptext.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimpviewablebutton.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimptextoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TEXT
};


static void  gimp_text_options_init        (GimpTextOptions      *options);
static void  gimp_text_options_class_init  (GimpTextOptionsClass *options_class);

static void  gimp_text_options_finalize         (GObject         *object);
static void  gimp_text_options_set_property     (GObject         *object,
                                                 guint            property_id,
                                                 const GValue    *value,
                                                 GParamSpec      *pspec);
static void  gimp_text_options_get_property     (GObject         *object,
                                                 guint            property_id,
                                                 GValue          *value,
                                                 GParamSpec      *pspec);

static void  gimp_text_options_reset            (GimpToolOptions *options);

static void  gimp_text_options_notify_font      (GimpContext     *context,
                                                 GParamSpec      *pspec,
                                                 GimpText        *text);
static void  gimp_text_options_notify_text_font (GimpText        *text,
                                                 GParamSpec      *pspec,
                                                 GimpContext     *context);


static GimpToolOptionsClass *parent_class = NULL;


GType
gimp_text_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpTextOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_text_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpTextOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_text_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpTextOptions",
                                     &info, 0);
    }

  return type;
}

static void
gimp_text_options_class_init (GimpTextOptionsClass *klass)
{
  GObjectClass         *object_class;
  GimpToolOptionsClass *options_class;

  object_class = G_OBJECT_CLASS (klass);
  options_class = GIMP_TOOL_OPTIONS_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_text_options_finalize;
  object_class->set_property = gimp_text_options_set_property;
  object_class->get_property = gimp_text_options_get_property;

  options_class->reset       = gimp_text_options_reset;

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_TEXT,
                                   "text", NULL,
                                   GIMP_TYPE_TEXT,
                                   GIMP_PARAM_AGGREGATE);
}

static void
gimp_text_options_init (GimpTextOptions *options)
{
  options->text   = g_object_new (GIMP_TYPE_TEXT, NULL);
  options->buffer = gimp_prop_text_buffer_new (G_OBJECT (options->text),
                                               "text", -1);

  g_signal_connect_object (options, "notify::font",
                           G_CALLBACK (gimp_text_options_notify_font),
                           options->text, 0);
  g_signal_connect_object (options->text, "notify::font",
                           G_CALLBACK (gimp_text_options_notify_text_font),
                           options, 0);
}

static void
gimp_text_options_finalize (GObject *object)
{
  GimpTextOptions *options = GIMP_TEXT_OPTIONS (object);

  if (options->buffer)
    {
      g_object_unref (options->buffer);
      options->buffer = NULL;
    }

  if (options->text)
    {
      g_object_unref (options->text);
      options->text = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
    case PROP_TEXT:
      if (g_value_get_object (value))
        gimp_config_sync (GIMP_CONFIG (g_value_get_object (value)),
                          GIMP_CONFIG (options->text), 0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
    case PROP_TEXT:
      g_value_set_object (value, options->text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_text_options_reset (GimpToolOptions *options)
{
  GimpTextOptions *text_options = GIMP_TEXT_OPTIONS (options);
  gchar           *text;

  g_object_freeze_notify (G_OBJECT (text_options->text));

  text = text_options->text->text;
  text_options->text->text = NULL;

  GIMP_TOOL_OPTIONS_CLASS (parent_class)->reset (options);

  text_options->text->text = text;

  g_object_thaw_notify (G_OBJECT (text_options->text));
}

static void
gimp_text_options_notify_font (GimpContext *context,
                               GParamSpec  *pspec,
                               GimpText    *text)
{
  GimpFont *font = gimp_context_get_font (context);

  g_signal_handlers_block_by_func (text,
                                   gimp_text_options_notify_text_font,
                                   context);

  g_object_set (text,
                "font", font ? gimp_object_get_name (GIMP_OBJECT (font)) : NULL,
                NULL);

  g_signal_handlers_unblock_by_func (text,
                                     gimp_text_options_notify_text_font,
                                     context);
}

static void
gimp_text_options_notify_text_font (GimpText    *text,
                                    GParamSpec  *pspec,
                                    GimpContext *context)
{
  GimpObject *font;
  gchar      *value;

  g_object_get (text,
                "font", &value,
                NULL);

  font = gimp_container_get_child_by_name (context->gimp->fonts, value);

  g_free (value);

  g_signal_handlers_block_by_func (context,
                                   gimp_text_options_notify_font,
                                   text);

  g_object_set (context,
                "font", font,
                NULL);

  g_signal_handlers_unblock_by_func (context,
                                     gimp_text_options_notify_font,
                                     text);
}

GtkWidget *
gimp_text_options_gui (GimpToolOptions *tool_options)
{
  GimpTextOptions   *options;
  GObject           *config;
  GtkWidget         *vbox;
  GtkWidget         *table;
  GtkWidget         *button;
  GimpDialogFactory *dialog_factory;
  GtkWidget         *auto_button;
  GtkWidget         *menu;
  GtkWidget         *box;
  GtkWidget         *spinbutton;
  gint               digits;
  gint               row = 0;

  options = GIMP_TEXT_OPTIONS (tool_options);
  config = G_OBJECT (options->text);

  vbox = gimp_tool_options_gui (tool_options);

  dialog_factory = gimp_dialog_factory_from_name ("dock");

  table = gtk_table_new (9, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  button = gimp_viewable_button_new (GIMP_CONTEXT (options)->gimp->fonts,
                                     GIMP_CONTEXT (options),
                                     GIMP_PREVIEW_SIZE_SMALL, 1,
                                     dialog_factory,
                                     "gimp-font-list|gimp-font-grid",
                                     GIMP_STOCK_FONT,
                                     _("Open the font selection dialog"));

  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("_Font:"), 1.0, 0.5,
                             button, 2, TRUE);

  digits = gimp_unit_get_digits (options->text->unit);
  spinbutton = gimp_prop_spin_button_new (config, "font-size",
					  1.0, 10.0, digits);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row,
                             _("_Size:"), 1.0, 0.5,
                             spinbutton, 1, FALSE);

  menu = gimp_prop_unit_menu_new (config, "font-size-unit", "%a");
  g_object_set_data (G_OBJECT (menu), "set_digits", spinbutton);
  gtk_table_attach (GTK_TABLE (table), menu, 2, 3, row, row + 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (menu);
  row++;

  button = gimp_prop_check_button_new (config, "hinting", _("_Hinting"));
  gtk_table_attach (GTK_TABLE (table), button, 1, 3, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);
  row++;

  auto_button = gimp_prop_check_button_new (config, "autohint",
					    _("Force Auto-Hinter"));
  gtk_table_attach (GTK_TABLE (table), auto_button, 1, 3, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (auto_button);
  row++;

  gtk_widget_set_sensitive (auto_button, options->text->hinting);
  g_object_set_data (G_OBJECT (button), "set_sensitive", auto_button);

  button = gimp_prop_check_button_new (config, "antialias", _("Antialiasing"));
  gtk_table_attach (GTK_TABLE (table), button, 1, 3, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);
  row++;

  button = gimp_prop_color_button_new (config, "color",
				       _("Text Color"),
				       -1, 24, GIMP_COLOR_AREA_FLAT);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (button),
                                GIMP_CONTEXT (options));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Color:"), 1.0, 0.5,
			     button, 1, FALSE);

  box = gimp_prop_enum_stock_box_new (config, "justify", "gtk-justify", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Justify:"), 1.0, 0.5,
			     box, 2, TRUE);

  spinbutton = gimp_prop_spin_button_new (config, "indent", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Indent:"), 1.0, 0.5,
                             spinbutton, 1, FALSE);

  spinbutton = gimp_prop_spin_button_new (config, "line-spacing", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gimp_table_attach_stock (GTK_TABLE (table), row++,
                           _("Line\nSpacing:"), 0.0,
			   spinbutton, 1, GIMP_STOCK_LINE_SPACING);

  button = gtk_button_new_with_label (_("Create Path from Text"));
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (tool_options),
                     "gimp-text-to-vectors", button);

  return vbox;
}


static void
gimp_text_options_dir_changed (GimpTextEditor *editor,
                               GimpText       *text)
{
  g_object_set (text,
                "base-direction", editor->base_dir,
                NULL);
}

static void
gimp_text_options_notify_dir (GimpText       *text,
                              GParamSpec     *pspec,
                              GimpTextEditor *editor)
{
  GimpTextDirection  dir;

  g_object_get (text,
                "base-direction", &dir,
                NULL);

  gimp_text_editor_set_direction (editor, dir);
}

GtkWidget *
gimp_text_options_editor_new (GimpTextOptions *options,
                              const gchar     *title)
{
  GtkWidget *editor;

  g_return_val_if_fail (GIMP_IS_TEXT_OPTIONS (options), NULL);

  editor = gimp_text_editor_new (title, options->buffer);

  gimp_text_editor_set_direction (GIMP_TEXT_EDITOR (editor),
                                  options->text->base_dir);

  g_signal_connect_object (editor, "dir_changed",
                           G_CALLBACK (gimp_text_options_dir_changed),
                           options->text, 0);
  g_signal_connect_object (options->text, "notify::base-direction",
                           G_CALLBACK (gimp_text_options_notify_dir),
                           editor, 0);

  return editor;
}
