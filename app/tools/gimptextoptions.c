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

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimptoolinfo.h"

#include "text/gimpfont.h"
#include "text/gimptext.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpcontainerpopup.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpfontselection.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimptextoptions.h"
#include "paint_options.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TEXT,
};


static void  gimp_text_options_init       (GimpTextOptions      *options);
static void  gimp_text_options_class_init (GimpTextOptionsClass *options_class);

static void  gimp_text_options_get_property (GObject     *object,
                                             guint        property_id,
                                             GValue      *value,
                                             GParamSpec  *pspec);

static void  gimp_text_options_notify_font  (GimpContext *context,
                                             GParamSpec  *pspec,
                                             GimpText    *text);
static void  gimp_text_notify_font          (GimpText    *text,
                                             GParamSpec  *pspec,
                                             GimpContext *context);

static void     gimp_text_options_font_clicked  (GtkWidget      *widget, 
                                                 GimpContext    *context);
static gboolean gimp_text_options_font_scrolled (GtkWidget      *widget, 
                                                 GdkEventScroll *sevent,
                                                 GimpContext    *context);


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
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->get_property = gimp_text_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_TEXT,
                                   "text", NULL,
                                   GIMP_TYPE_TEXT,
                                   0);
}

static void
gimp_text_options_init (GimpTextOptions *options)
{
  GObject *text;

  text = g_object_new (GIMP_TYPE_TEXT, NULL);

  options->text   = GIMP_TEXT (text);
  options->buffer = gimp_prop_text_buffer_new (text, "text", -1);

  g_signal_connect_object (options, "notify::font",
                           G_CALLBACK (gimp_text_options_notify_font),
                           text, 0);
  g_signal_connect_object (text, "notify::font",
                           G_CALLBACK (gimp_text_notify_font),
                           options, 0);
}

static void
gimp_text_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpTextOptions *options;

  options = GIMP_TEXT_OPTIONS (object);

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
gimp_text_options_notify_font (GimpContext *context,
                               GParamSpec  *pspec,
                               GimpText    *text)
{
  GimpFont *font = gimp_context_get_font (context);

  g_signal_handlers_block_by_func (text,
                                   gimp_text_notify_font,
                                   context);

  g_object_set (text,
                "font", gimp_object_get_name (GIMP_OBJECT (font)),
                NULL);

  g_signal_handlers_unblock_by_func (text,
                                     gimp_text_notify_font,
                                     context);  
}
                               
static void
gimp_text_notify_font (GimpText    *text,
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
  GimpTextOptions *options;
  GObject         *config;
  GtkWidget       *vbox;
  GtkWidget       *table;
  GtkWidget       *button;
  GtkWidget       *auto_button;
  GtkWidget       *preview;
  GtkWidget       *menu;
  GtkWidget       *font_selection;
  GtkWidget       *box;
  GtkWidget       *spinbutton;
  gint             digits;

  options = GIMP_TEXT_OPTIONS (tool_options);
  config = G_OBJECT (options->text);

  vbox = gimp_tool_options_gui (tool_options);

  table = gtk_table_new (10, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  button = gtk_button_new ();
  preview = gimp_prop_preview_new (G_OBJECT (options), "font", 24);
  gtk_container_add (GTK_CONTAINER (button), preview);
  gtk_widget_show (preview);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Font:"), 1.0, 0.5,
                             button, 2, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_text_options_font_clicked),
                    options);
  g_signal_connect (button, "scroll_event",
                    G_CALLBACK (gimp_text_options_font_scrolled),
                    options);

  font_selection = gimp_prop_font_selection_new (config, "font");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("_Font:"), 1.0, 0.5,
                             font_selection, 2, FALSE);

  digits = gimp_unit_get_digits (options->text->font_size_unit);
  spinbutton = gimp_prop_spin_button_new (config, "font-size",
					  1.0, 10.0, digits); 
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("_Size:"), 1.0, 0.5,
                             spinbutton, 1, FALSE);

  menu = gimp_prop_unit_menu_new (config, "font-size-unit", "%a");
  g_object_set_data (G_OBJECT (menu), "set_digits", spinbutton);
  gtk_table_attach (GTK_TABLE (table), menu, 2, 3, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (menu);

  button = gimp_prop_check_button_new (config, "hinting", _("_Hinting"));
  gtk_table_attach (GTK_TABLE (table), button, 1, 3, 3, 4,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);

  auto_button = gimp_prop_check_button_new (config, "autohint",
					    _("Force Auto-Hinter"));
  gtk_table_attach (GTK_TABLE (table), auto_button, 1, 3, 4, 5,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (auto_button);

  gtk_widget_set_sensitive (auto_button, options->text->hinting);
  g_object_set_data (G_OBJECT (button), "set_sensitive", auto_button);

  button = gimp_prop_check_button_new (config, "antialias", _("Antialiasing"));
  gtk_table_attach (GTK_TABLE (table), button, 1, 3, 5, 6,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);

  button = gimp_prop_color_button_new (config, "color",
				       _("Text Color"),
				       -1, 24, GIMP_COLOR_AREA_FLAT);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (button),
                                GIMP_CONTEXT (options));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 6,
                             _("Color:"), 1.0, 0.5,
			     button, 1, FALSE);

  box = gimp_prop_enum_stock_box_new (config, "justify", "gtk-justify", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 7,
                             _("Justify:"), 1.0, 0.5,
			     box, 2, TRUE);

  spinbutton = gimp_prop_spin_button_new (config, "indent", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 8,
                             _("Indent:"), 1.0, 0.5,
                             spinbutton, 1, FALSE);

  spinbutton = gimp_prop_spin_button_new (config, "line-spacing", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gimp_table_attach_stock (GTK_TABLE (table), 9,
                           _("Line\nSpacing:"), 0.0,
			   spinbutton, 1, GIMP_STOCK_LINE_SPACING);

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

static void
gimp_text_options_font_clicked (GtkWidget   *widget, 
                                GimpContext *context)
{
  GtkWidget *toplevel;
  GtkWidget *popup;

  toplevel = gtk_widget_get_toplevel (widget);

  popup = gimp_container_popup_new (context->gimp->fonts, context,
                                    GIMP_DOCK (toplevel)->dialog_factory,
                                    "gimp-font-list",
                                    GTK_STOCK_SELECT_FONT,
                                    _("Open the font selection dialog"));
  gimp_container_popup_show (GIMP_CONTAINER_POPUP (popup), widget);
}

static gboolean
gimp_text_options_font_scrolled (GtkWidget      *widget, 
                                 GdkEventScroll *sevent,
                                 GimpContext    *context)
{
  paint_options_container_scrolled (context->gimp->fonts,
                                    context, GIMP_TYPE_FONT, sevent);

  return TRUE;
}
