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

#include "core/gimptoolinfo.h"

#include "text/gimptext.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpfontselection.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimptextoptions.h"

#include "libgimp/gimpintl.h"


static void   gimp_text_options_init       (GimpTextOptions      *options);
static void   gimp_text_options_class_init (GimpTextOptionsClass *options_class);

static void   gimp_text_options_reset      (GimpToolOptions      *tool_options);


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

  object_class  = G_OBJECT_CLASS (klass);
  options_class = GIMP_TOOL_OPTIONS_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  options_class->reset = gimp_text_options_reset;
}

static void
gimp_text_options_init (GimpTextOptions *options)
{
  GObject *text;

  text = g_object_new (GIMP_TYPE_TEXT, NULL);

  options->text = GIMP_TEXT (text);

  options->buffer = gimp_prop_text_buffer_new (text, "text", -1);
}

static void
gimp_text_options_reset (GimpToolOptions *tool_options)
{
  GimpTextOptions *options = GIMP_TEXT_OPTIONS (tool_options);

  gimp_config_reset (G_OBJECT (options->text));

  GIMP_TOOL_OPTIONS_CLASS (parent_class)->reset (tool_options);
}

GtkWidget *
gimp_text_options_gui (GimpToolOptions *tool_options)
{
  GimpTextOptions *options;
  GtkWidget       *vbox;
  GtkWidget       *table;
  GtkWidget       *button;
  GtkWidget       *unit_menu;
  GtkWidget       *font_selection;
  GtkWidget       *box;
  GtkWidget       *spinbutton;
  gint             digits;

  options = GIMP_TEXT_OPTIONS (tool_options);

  vbox = gimp_tool_options_gui (tool_options);

  table = gtk_table_new (4, 6, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (table), FALSE, FALSE, 0);
  gtk_widget_show (table);

  font_selection = gimp_prop_font_selection_new (G_OBJECT (options->text),
                                                 "font");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Font:"), 1.0, 0.5,
                             font_selection, 3, FALSE);

  digits = gimp_unit_get_digits (options->text->font_size_unit);
  spinbutton = gimp_prop_spin_button_new (G_OBJECT (options->text),
                                          "font-size",
					  1.0, 10.0, digits); 
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("_Size:"), 1.0, 0.5,
                             spinbutton, 2, FALSE);

  unit_menu = gimp_prop_unit_menu_new (G_OBJECT (options->text),
                                       "font-size-unit", "%a");
  g_object_set_data (G_OBJECT (unit_menu), "set_digits", spinbutton);
  gtk_table_attach (GTK_TABLE (table), unit_menu, 3, 4, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (unit_menu);

  button = gimp_prop_color_button_new (G_OBJECT (options->text),
                                       "color", _("Text Color"),
				       -1, 24, GIMP_COLOR_AREA_FLAT);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (button),
                                GIMP_CONTEXT (options));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Color:"), 1.0, 0.5, button, 1, FALSE);

  box = gimp_prop_enum_stock_box_new (G_OBJECT (options->text),
                                      "justify", "gtk-justify", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("Justify:"), 1.0, 0.5, box, 3, TRUE);

  spinbutton = gimp_prop_spin_button_new (G_OBJECT (options->text),
                                          "indent", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gimp_table_attach_stock (GTK_TABLE (table), 4,
			   _("Indent:"), 0.5, spinbutton, NULL);

  spinbutton = gimp_prop_spin_button_new (G_OBJECT (options->text),
                                          "line-spacing", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gimp_table_attach_stock (GTK_TABLE (table), 5,
                           _("Line\nSpacing:"), 0.0, spinbutton,
			   GIMP_STOCK_LINE_SPACING);

  return vbox;
}
