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

#include "gimpdasheditor.h"
#include "gimpenumcombobox.h"
#include "gimppropwidgets.h"
#include "gimpstrokeeditor.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_OPTIONS,
  PROP_RESOLUTION
};

static void      gimp_stroke_editor_class_init   (GimpStrokeEditorClass *klass);
static GObject * gimp_stroke_editor_constructor  (GType                  type,
                                                  guint                  n_params,
                                                  GObjectConstructParam *params);
static void      gimp_stroke_editor_set_property (GObject         *object,
                                                  guint            property_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);
static void      gimp_stroke_editor_get_property (GObject         *object,
                                                  guint            property_id,
                                                  GValue          *value,
                                                  GParamSpec      *pspec);
static void      gimp_stroke_editor_finalize     (GObject         *object);
static gboolean  gimp_stroke_editor_paint_button (GtkWidget       *widget,
                                                  GdkEventExpose  *event,
                                                  gpointer         data);
static void      gimp_stroke_editor_dash_preset  (GtkWidget       *widget,
                                                  gpointer         data);



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
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        (GClassInitFunc) gimp_stroke_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpStrokeEditor),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
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

  object_class->constructor  = gimp_stroke_editor_constructor;
  object_class->set_property = gimp_stroke_editor_set_property;
  object_class->get_property = gimp_stroke_editor_get_property;
  object_class->finalize     = gimp_stroke_editor_finalize;

  g_object_class_install_property (object_class, PROP_OPTIONS,
                                   g_param_spec_object ("options", NULL, NULL,
                                                        GIMP_TYPE_STROKE_OPTIONS,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_RESOLUTION,
                                   g_param_spec_double ("resolution", NULL, NULL,
                                                        GIMP_MIN_RESOLUTION,
                                                        GIMP_MAX_RESOLUTION,
                                                        72.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_stroke_editor_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpStrokeEditor *editor = GIMP_STROKE_EDITOR (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      editor->options = GIMP_STROKE_OPTIONS (g_value_dup_object (value));
      break;
    case PROP_RESOLUTION:
      editor->resolution = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_stroke_editor_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpStrokeEditor *editor = GIMP_STROKE_EDITOR (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      g_value_set_object (value, editor->options);
      break;
    case PROP_RESOLUTION:
      g_value_set_double (value, editor->resolution);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GObject *
gimp_stroke_editor_constructor (GType                   type,
                                guint                   n_params,
                                GObjectConstructParam  *params)
{
  GimpStrokeEditor *editor;
  GtkWidget        *box;
  GtkWidget        *size;
  GtkWidget        *label;
  GtkWidget        *frame;
  GtkWidget        *table;
  GtkWidget        *expander;
  GtkWidget        *dash_editor;
  GtkWidget        *button;
  GObject          *object;
  gint              row = 0;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_STROKE_EDITOR (object);

  g_assert (editor->options != NULL);

  gtk_box_set_spacing (GTK_BOX (editor), 6);

  box = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (editor), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  label = gtk_label_new (_("Line Width:"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  size = gimp_prop_size_entry_new (G_OBJECT (editor->options), "width", "unit",
                                   "%a", GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                   editor->resolution);
  gimp_size_entry_set_pixel_digits (GIMP_SIZE_ENTRY (size), 1);
  gtk_box_pack_start (GTK_BOX (box), size, FALSE, FALSE, 0);
  gtk_widget_show (size);

  expander = gtk_expander_new_with_mnemonic (_("_Line Style"));
  gtk_box_pack_start (GTK_BOX (editor), expander, FALSE, FALSE, 0);
  gtk_widget_show (expander);

  frame = gimp_frame_new ("<expander>");
  gtk_container_add (GTK_CONTAINER (expander), frame);
  gtk_widget_show (frame);

  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 4, 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  box = gimp_prop_enum_stock_box_new (G_OBJECT (editor->options), "cap-style",
                                      "gimp-cap", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("_Cap style:"), 0.0, 0.5,
                             box, 2, TRUE);

  box = gimp_prop_enum_stock_box_new (G_OBJECT (editor->options), "join-style",
                                      "gimp-join", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("_Join style:"), 0.0, 0.5,
                             box, 2, TRUE);

  gimp_prop_scale_entry_new (G_OBJECT (editor->options), "miter",
                             GTK_TABLE (table), 0, row++,
                             _("_Miter limit:"),
                             1.0, 1.0, 1,
                             FALSE, 0.0, 0.0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Dash pattern:"), 0.0, 0.5,
                             frame, 2, FALSE);

  box = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  dash_editor = gimp_dash_editor_new (editor->options);

  button = g_object_new (GTK_TYPE_BUTTON,
                         "width-request", 14,
                         NULL);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect_object (button, "clicked",
                           G_CALLBACK (gimp_dash_editor_shift_left),
                           dash_editor, G_CONNECT_SWAPPED);
  g_signal_connect_after (button, "expose-event",
                          G_CALLBACK (gimp_stroke_editor_paint_button),
                          button);

  gtk_box_pack_start (GTK_BOX (box), dash_editor, TRUE, TRUE, 0);
  gtk_widget_show (dash_editor);

  button = g_object_new (GTK_TYPE_BUTTON,
                         "width-request", 14,
                         NULL);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect_object (button, "clicked",
                           G_CALLBACK (gimp_dash_editor_shift_right),
                           dash_editor, G_CONNECT_SWAPPED);
  g_signal_connect_after (button, "expose-event",
                          G_CALLBACK (gimp_stroke_editor_paint_button),
                          NULL);


  box = gimp_enum_combo_box_new (GIMP_TYPE_DASH_PRESET);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (box), GIMP_DASH_CUSTOM);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Dash preset:"), 0.0, 0.5,
                             box, 2, TRUE);

  g_signal_connect (box, "changed",
                    G_CALLBACK (gimp_stroke_editor_dash_preset),
                    editor->options);
  g_signal_connect_object (editor->options, "dash_info_changed",
                           G_CALLBACK (gimp_int_combo_box_set_active),
                           box, G_CONNECT_SWAPPED);


  button = gimp_prop_check_button_new (G_OBJECT (editor->options), "antialias",
                                       _("_Antialiasing"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 2, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);
  row++;

  box = gimp_prop_enum_radio_box_new (G_OBJECT (editor->options), "style",
                                      0, 0);
  gtk_box_pack_start (GTK_BOX (editor), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  return object;
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
gimp_stroke_editor_new (GimpStrokeOptions *options,
                        gdouble            resolution)
{
  g_return_val_if_fail (GIMP_IS_STROKE_OPTIONS (options), NULL);

  return g_object_new (GIMP_TYPE_STROKE_EDITOR,
                       "options",    options,
                       "resolution", resolution,
                       NULL);
}

static gboolean
gimp_stroke_editor_paint_button (GtkWidget       *widget,
                                 GdkEventExpose  *event,
                                 gpointer         data)
{
  GtkAllocation *alloc = &widget->allocation;
  gint           w     = MIN (alloc->width, alloc->height) * 2 / 3;

  gtk_paint_arrow (widget->style, widget->window,
                   widget->state, GTK_SHADOW_IN,
                   &event->area, widget, NULL,
                   data ? GTK_ARROW_LEFT : GTK_ARROW_RIGHT, TRUE,
                   alloc->x + (alloc->width - w) / 2,
                   alloc->y + (alloc->height - w) / 2,
                   w, w);
  return FALSE;
}

static void
gimp_stroke_editor_dash_preset (GtkWidget *widget,
                                gpointer   data)
{
  gint value;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value))
    gimp_stroke_options_set_dash_preset (GIMP_STROKE_OPTIONS (data), value);
}
