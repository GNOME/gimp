/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpeditor.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpeditor.h"
#include "gimpdnd.h"


static void   gimp_editor_class_init (GimpEditorClass *klass);
static void   gimp_editor_init       (GimpEditor      *panel);

static void   gimp_editor_style_set  (GtkWidget       *widget,
                                      GtkStyle        *prev_style);


static GtkVBoxClass *parent_class = NULL;


GType
gimp_editor_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_editor_init,
      };

      type = g_type_register_static (GTK_TYPE_VBOX,
                                     "GimpEditor",
                                     &info, 0);
    }

  return type;
}

static void
gimp_editor_class_init (GimpEditorClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->style_set = gimp_editor_style_set;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content_spacing",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             G_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("button_spacing",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             G_PARAM_READABLE));
}

static void
gimp_editor_init (GimpEditor *editor)
{
}

static void
gimp_editor_style_set (GtkWidget *widget,
                       GtkStyle  *prev_style)
{
  GimpEditor *editor;
  gint        content_spacing;
  gint        button_spacing;

  editor = GIMP_EDITOR (widget);

  gtk_widget_style_get (widget,
                        "content_spacing", &content_spacing,
			"button_spacing",  &button_spacing,
			NULL);

  gtk_box_set_spacing (GTK_BOX (widget), content_spacing);

  if (editor->button_box)
    {
      gtk_box_set_spacing (GTK_BOX (editor->button_box), button_spacing);
    }

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

GtkWidget *
gimp_editor_new (void)
{
  GimpEditor *editor;

  editor = g_object_new (GIMP_TYPE_EDITOR, NULL);

  return GTK_WIDGET (editor);
}

GtkWidget *
gimp_editor_add_button (GimpEditor  *editor,
                        const gchar *stock_id,
                        const gchar *tooltip,
                        const gchar *help_data,
                        GCallback    callback,
                        GCallback    extended_callback,
                        gpointer     callback_data)
{
  GtkWidget *button;
  GtkWidget *image;

  g_return_val_if_fail (GIMP_IS_EDITOR (editor), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  if (! editor->button_box)
    {
      editor->button_box = gtk_hbox_new (TRUE, 2);
      gtk_box_pack_end (GTK_BOX (editor), editor->button_box, FALSE, FALSE, 0);
      gtk_widget_show (editor->button_box);
    }

  button = gimp_button_new ();
  gtk_box_pack_start (GTK_BOX (editor->button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  if (tooltip || help_data)
    gimp_help_set_help_data (button, tooltip, help_data);

  if (callback)
    g_signal_connect (G_OBJECT (button), "clicked",
		      callback,
		      callback_data);

  if (extended_callback)
    g_signal_connect (G_OBJECT (button), "extended_clicked",
		      extended_callback,
		      callback_data);

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  return button;
}
