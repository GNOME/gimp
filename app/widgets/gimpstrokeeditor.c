/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "core/gimpstrokeoptions.h"

#include "gimpenummenu.h"
#include "gimppropwidgets.h"
#include "gimpstrokeeditor.h"

#include "gimp-intl.h"


#define SB_WIDTH 10


static void  gimp_stroke_editor_class_init (GimpStrokeEditorClass *klass);
static void  gimp_stroke_editor_init       (GimpStrokeEditor      *editor);

static void  gimp_stroke_editor_finalize   (GObject               *object);


static GtkVBoxClass *parent_class = NULL;


GType
gimp_stroke_editor_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpStrokeEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_stroke_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpStrokeEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_stroke_editor_init,
      };

      view_type = g_type_register_static (GTK_TYPE_VBOX,
                                          "GimpStrokeEditor",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_stroke_editor_class_init (GimpStrokeEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_stroke_editor_finalize;
}

static void
gimp_stroke_editor_init (GimpStrokeEditor *editor)
{
}

static void
gimp_stroke_editor_finalize (GObject *object)
{
  GimpStrokeEditor *editor = GIMP_STROKE_EDITOR (object);

  if (editor->options)
    {
      g_object_unref (editor->options);
      editor->options = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
gimp_stroke_editor_new (GimpStrokeOptions *options)
{
  GimpStrokeEditor *editor;
  GtkWidget        *table;
  GtkWidget        *menu;
  GtkWidget        *spinbutton;
  GtkWidget        *button;
  gint              digits;
  gint              row = 0;

  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), NULL);

  editor = g_object_new (GIMP_TYPE_STROKE_EDITOR, NULL);

  editor->options = options;
  g_object_ref (options);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (editor), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  digits = gimp_unit_get_digits (options->width_unit);
  spinbutton = gimp_prop_spin_button_new (G_OBJECT (options), "width",
                                          1.0, 10.0, digits);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row,
                             _("Stroke _Width:"), 1.0, 0.5,
                             spinbutton, 1, FALSE);

  menu = gimp_prop_unit_menu_new (G_OBJECT (options), "width-unit", "%a");
  g_object_set_data (G_OBJECT (menu), "set_digits", spinbutton);
  gtk_table_attach (GTK_TABLE (table), menu, 2, 3, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (menu);
  row++;

  menu = gimp_prop_enum_option_menu_new (G_OBJECT (options), "cap-style",
                                         0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("_Cap Style:"), 1.0, 0.5,
                             menu, 2, TRUE);

  menu = gimp_prop_enum_option_menu_new (G_OBJECT (options), "join-style",
                                         0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("_Join Style:"), 1.0, 0.5,
                             menu, 2, TRUE);

  gimp_prop_scale_entry_new (G_OBJECT (options), "miter",
                             GTK_TABLE (table), 0, row++,
                             _("_Miter Limit:"),
                             1.0, 1.0, 1,
                             FALSE, 0.0, 0.0);

  button = gimp_prop_check_button_new (G_OBJECT (options), "antialias",
                                       _("Antialiasing"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);
  row++;

  return GTK_WIDGET (editor);
}
