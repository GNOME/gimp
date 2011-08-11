/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimptemplate.h"

#include "gimppropwidgets.h"
#include "gimptemplateeditor.h"

#include "gimp-intl.h"


#define SB_WIDTH            8
#define MAX_COMMENT_LENGTH  512  /* arbitrary */


enum
{
  PROP_0,
  PROP_TEMPLATE
};


typedef struct _GimpTemplateEditorPrivate GimpTemplateEditorPrivate;

struct _GimpTemplateEditorPrivate
{
  GimpTemplate          *template;

  GtkWidget             *aspect_button;
  gboolean               block_aspect;

  GtkWidget             *expander;
  GimpUnitEntries       *unit_entries;
  GtkWidget             *memsize_label;
  GtkWidget             *pixel_label;
  GtkWidget             *more_label;
  GimpUnitEntries       *resolution_entries;
};

#define GET_PRIVATE(editor) \
        G_TYPE_INSTANCE_GET_PRIVATE (editor, \
                                     GIMP_TYPE_TEMPLATE_EDITOR, \
                                     GimpTemplateEditorPrivate)


static void    gimp_template_editor_constructed  (GObject            *object);
static void    gimp_template_editor_finalize     (GObject            *object);
static void    gimp_template_editor_set_property (GObject            *object,
                                                  guint               property_id,
                                                  const GValue       *value,
                                                  GParamSpec         *pspec);
static void    gimp_template_editor_get_property (GObject            *object,
                                                  guint               property_id,
                                                  GValue             *value,
                                                  GParamSpec         *pspec);

static void gimp_template_editor_aspect_callback (GtkWidget          *widget,
                                                  GimpTemplateEditor *editor);
static void gimp_template_editor_template_notify (GimpTemplate       *template,
                                                  GParamSpec         *param_spec,
                                                  GimpTemplateEditor *editor);


G_DEFINE_TYPE (GimpTemplateEditor, gimp_template_editor, GTK_TYPE_BOX)

#define parent_class gimp_template_editor_parent_class


static void
gimp_template_editor_class_init (GimpTemplateEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_template_editor_constructed;
  object_class->finalize     = gimp_template_editor_finalize;
  object_class->set_property = gimp_template_editor_set_property;
  object_class->get_property = gimp_template_editor_get_property;

  g_object_class_install_property (object_class, PROP_TEMPLATE,
                                   g_param_spec_object ("template", NULL, NULL,
                                                        GIMP_TYPE_TEMPLATE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (object_class, sizeof (GimpTemplateEditorPrivate));
}

static void
gimp_template_editor_init (GimpTemplateEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 12);
}

static void
gimp_template_editor_constructed (GObject *object)
{
  GimpTemplateEditor        *editor  = GIMP_TEMPLATE_EDITOR (object);
  GimpTemplateEditorPrivate *private = GET_PRIVATE (object);
  GimpTemplate              *template;
  GtkWidget                 *aspect_box;
  GtkWidget                 *frame;
  GtkWidget                 *hbox;
  GtkWidget                 *vbox;
  GtkWidget                 *table;
  GtkObject                 *adjustment;
  GtkWidget                 *width;
  GtkWidget                 *height;
  GtkWidget                 *xres;
  GtkWidget                 *yres;
  GtkWidget                 *chainbutton;
  GtkWidget                 *combo;
  GtkWidget                 *scrolled_window;
  GtkWidget                 *text_view;
  GtkTextBuffer             *text_buffer;
  gchar                     *text;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (private->template != NULL);

  template = private->template;

  /*  Image size frame  */
  frame = gimp_frame_new (_("Image Size"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  width = gimp_spin_button_new (&adjustment,
                                1, 1, 1, 1, 10, 0,
                                1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (width), SB_WIDTH);

  height = gimp_spin_button_new (&adjustment,
                                 1, 1, 1, 1, 10, 0,
                                 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (height), SB_WIDTH);

  /*  create the sizeentry which keeps it all together  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (hbox);

  private->unit_entries = GIMP_UNIT_ENTRIES (gimp_unit_entries_new ());
                        
  gtk_table_attach_defaults (GTK_TABLE (table), gimp_unit_entries_get_table (private->unit_entries), 0, 2, 0, 2);

  gimp_unit_entries_add_entry (private->unit_entries, 
                                        GIMP_UNIT_ENTRIES_WIDTH, 
                                        _("Width:"));
  gimp_unit_entries_add_entry (private->unit_entries,
                                        GIMP_UNIT_ENTRIES_HEIGHT,
                                        _("Height:"));

  gimp_prop_coordinates_connect (G_OBJECT (template),
                                 GIMP_UNIT_ENTRIES_WIDTH, GIMP_UNIT_ENTRIES_HEIGHT, "unit",
                                 G_OBJECT (private->unit_entries), NULL,
                                 gimp_template_get_resolution_x (template),
                                 gimp_template_get_resolution_y (template));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 0, 3, 2, 3);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  aspect_box = gimp_enum_stock_box_new (GIMP_TYPE_ASPECT_TYPE,
                                        "gimp", GTK_ICON_SIZE_MENU,
                                        G_CALLBACK (gimp_template_editor_aspect_callback),
                                        editor,
                                        &private->aspect_button);
  gtk_widget_hide (private->aspect_button); /* hide "square" */

  gtk_box_pack_start (GTK_BOX (vbox), aspect_box, FALSE, FALSE, 0);
  gtk_widget_show (aspect_box);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  private->pixel_label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (private->pixel_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (private->pixel_label), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), private->pixel_label, FALSE, FALSE, 0);
  gtk_widget_show (private->pixel_label);

  private->more_label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (private->more_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (private->more_label), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), private->more_label, FALSE, FALSE, 0);
  gtk_widget_show (private->more_label);

#ifdef ENABLE_MEMSIZE_LABEL
  private->memsize_label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (private->memsize_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (private->memsize_label), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), private->memsize_label, FALSE, FALSE, 0);
  gtk_widget_show (private->memsize_label);
#endif

  text = g_strdup_printf ("<b>%s</b>", _("_Advanced Options"));
  private->expander = g_object_new (GTK_TYPE_EXPANDER,
                                    "label",         text,
                                    "use-markup",    TRUE,
                                    "use-underline", TRUE,
                                    NULL);
  g_free (text);

  gtk_box_pack_start (GTK_BOX (editor), private->expander, TRUE, TRUE, 0);
  gtk_widget_show (private->expander);

  frame = gimp_frame_new ("<expander>");
  gtk_container_add (GTK_CONTAINER (private->expander), frame);
  gtk_widget_show (frame);

  table = gtk_table_new (6, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  xres = gimp_spin_button_new (&adjustment,
                               1, 1, 1, 1, 10, 0,
                               1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (xres), SB_WIDTH);

  yres = gimp_spin_button_new (&adjustment,
                               1, 1, 1, 1, 10, 0,
                               1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (yres), SB_WIDTH);

  /*  the resolution sizeentry  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 0, 2, 0, 2);
  gtk_widget_show (hbox);

  private->resolution_entries = GIMP_UNIT_ENTRIES (gimp_unit_entries_new ()); 

  gimp_unit_entries_add_entry (private->resolution_entries,
                                           "xres",
                                           _("X resolution:"));
  gimp_unit_entries_add_entry (private->resolution_entries,
                                           "yres",
                                           _("Y resolution:"));
  gimp_unit_entries_set_mode (private->resolution_entries, GIMP_UNIT_ENTRY_MODE_RESOLUTION);                              

  gtk_box_pack_start (GTK_BOX (hbox), gimp_unit_entries_get_table (private->resolution_entries), TRUE, TRUE, 0);
  gtk_widget_show (gimp_unit_entries_get_table (private->resolution_entries));

  gimp_unit_adjustment_set_resolution (gimp_unit_entries_get_adjustment (private->unit_entries, GIMP_UNIT_ENTRIES_WIDTH),
                                       gimp_template_get_resolution_x (template));
  gimp_unit_adjustment_set_resolution (gimp_unit_entries_get_adjustment (private->unit_entries, GIMP_UNIT_ENTRIES_HEIGHT),
                                       gimp_template_get_resolution_y (template));

  /*  the resolution chainbutton  */
  chainbutton = gimp_unit_entries_add_chain_button (private->resolution_entries,
                                                        "xres", "yres");

  gimp_prop_coordinates_connect (G_OBJECT (template),
                                 "xresolution", "yresolution",
                                 "resolution-unit",
                                 G_OBJECT (private->resolution_entries), chainbutton,
                                 1.0, 1.0);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (template),
                                        "image-type",
                                        GIMP_RGB, GIMP_GRAY);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Color _space:"), 0.0, 0.5,
                             combo, 1, FALSE);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (template),
                                        "precision", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("_Precision:"), 0.0, 0.5,
                             combo, 1, FALSE);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (template),
                                        "fill-type",
                                        GIMP_FOREGROUND_FILL,
                                        GIMP_PATTERN_FILL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
                             _("_Fill with:"), 0.0, 0.5,
                             combo, 1, FALSE);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 5,
                             _("Comme_nt:"), 0.0, 0.0,
                             scrolled_window, 1, FALSE);

  text_buffer = gimp_prop_text_buffer_new (G_OBJECT (template),
                                           "comment", MAX_COMMENT_LENGTH);

  text_view = gtk_text_view_new_with_buffer (text_buffer);
  g_object_unref (text_buffer);

  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  g_signal_connect_object (template, "notify",
                           G_CALLBACK (gimp_template_editor_template_notify),
                           editor, 0);

  /*  call the notify callback once to get the labels set initially  */
  gimp_template_editor_template_notify (template, NULL, editor);
}

static void
gimp_template_editor_finalize (GObject *object)
{
  GimpTemplateEditorPrivate *private = GET_PRIVATE (object);

  if (private->template)
    {
      g_object_unref (private->template);
      private->template = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_template_editor_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpTemplateEditorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TEMPLATE:
      private->template = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_template_editor_get_property (GObject      *object,
                                   guint         property_id,
                                   GValue       *value,
                                   GParamSpec   *pspec)
{
  GimpTemplateEditorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TEMPLATE:
      g_value_set_object (value, private->template);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_template_editor_new (GimpTemplate *template,
                          Gimp         *gimp,
                          gboolean      edit_template)
{
  GimpTemplateEditor        *editor;
  GimpTemplateEditorPrivate *private;

  g_return_val_if_fail (!edit_template || GIMP_IS_GIMP (gimp), NULL);

  editor = g_object_new (GIMP_TYPE_TEMPLATE_EDITOR,
                         "template", template,
                         NULL);

  private = GET_PRIVATE (editor);

  if (edit_template)
    {
      GtkWidget   *table;
      GtkWidget   *entry;
      GtkWidget   *icon_picker;

      table = gtk_table_new (2, 2, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 6);
      gtk_table_set_row_spacings (GTK_TABLE (table), 6);
      gtk_box_pack_start (GTK_BOX (editor), table, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (editor), table, 0);
      gtk_widget_show (table);

      entry = gimp_prop_entry_new (G_OBJECT (private->template), "name", 128);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("_Name:"), 1.0, 0.5,
                                 entry, 1, FALSE);

      icon_picker = gimp_prop_icon_picker_new (GIMP_VIEWABLE (private->template),
                                               gimp);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                 _("_Icon:"), 1.0, 0.5,
                                 icon_picker, 1, TRUE);
    }

  return GTK_WIDGET (editor);
}

GimpTemplate *
gimp_template_editor_get_template (GimpTemplateEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE_EDITOR (editor), NULL);

  return GET_PRIVATE (editor)->template;
}

void
gimp_template_editor_show_advanced (GimpTemplateEditor *editor,
                                    gboolean            expanded)
{
  GimpTemplateEditorPrivate *private;

  g_return_if_fail (GIMP_IS_TEMPLATE_EDITOR (editor));

  private = GET_PRIVATE (editor);

  gtk_expander_set_expanded (GTK_EXPANDER (private->expander), expanded);
}

GimpUnitEntries *
gimp_template_editor_get_unit_entries (GimpTemplateEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE_EDITOR (editor), NULL);

  return GET_PRIVATE (editor)->unit_entries;
}


/*  private functions  */

static void
gimp_template_editor_set_pixels (GimpTemplateEditor *editor,
                                 GimpTemplate       *template)
{
  GimpTemplateEditorPrivate *private = GET_PRIVATE (editor);
  gchar                     *text;

  text = g_strdup_printf (ngettext ("%d × %d pixel",
                                    "%d × %d pixels",
                                    gimp_template_get_height (template)),
                          gimp_template_get_width (template),
                          gimp_template_get_height (template));
  gtk_label_set_text (GTK_LABEL (private->pixel_label), text);
  g_free (text);
}

static void
gimp_template_editor_aspect_callback (GtkWidget          *widget,
                                      GimpTemplateEditor *editor)
{
  GimpTemplateEditorPrivate *private = GET_PRIVATE (editor);

  if (! private->block_aspect &&
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GimpTemplate *template    = private->template;
      gint          width       = gimp_template_get_width (template);
      gint          height      = gimp_template_get_height (template);
      gdouble       xresolution = gimp_template_get_resolution_x (template);
      gdouble       yresolution = gimp_template_get_resolution_y (template);

      if (width == height)
        {
          private->block_aspect = TRUE;
          gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (private->aspect_button),
                                           GIMP_ASPECT_SQUARE);
          private->block_aspect = FALSE;
          return;
       }

      g_signal_handlers_block_by_func (template,
                                       gimp_template_editor_template_notify,
                                       editor);

      gimp_unit_adjustment_set_resolution (gimp_unit_entries_get_adjustment (private->unit_entries,
                                                                             GIMP_UNIT_ENTRIES_WIDTH),
                                           xresolution); 
      gimp_unit_adjustment_set_resolution (gimp_unit_entries_get_adjustment (private->unit_entries,
                                                                             GIMP_UNIT_ENTRIES_HEIGHT),
                                           yresolution);                                                                

      g_object_set (template,
                    GIMP_UNIT_ENTRIES_WIDTH,  height,
                    GIMP_UNIT_ENTRIES_HEIGHT, width,
                    "xresolution", yresolution,
                    "yresolution", xresolution,
                    NULL);

      g_signal_handlers_unblock_by_func (template,
                                         gimp_template_editor_template_notify,
                                         editor);

      gimp_template_editor_set_pixels (editor, template);
    }
}

static void
gimp_template_editor_template_notify (GimpTemplate       *template,
                                      GParamSpec         *param_spec,
                                      GimpTemplateEditor *editor)
{
  GimpTemplateEditorPrivate *private = GET_PRIVATE (editor);
  GimpAspectType             aspect;
  const gchar               *desc;
  gchar                     *text;
  gint                       width;
  gint                       height;
  gint                       xres;
  gint                       yres;

  if (param_spec)
    {
      if (! strcmp (param_spec->name, "xresolution"))
        {
          gimp_unit_adjustment_set_resolution (gimp_unit_entries_get_adjustment (private->unit_entries,
                                                                                 GIMP_UNIT_ENTRIES_HEIGHT),
                                               gimp_template_get_resolution_x (template));
        }
      else if (! strcmp (param_spec->name, "yresolution"))
        {
          gimp_unit_adjustment_set_resolution (gimp_unit_entries_get_adjustment (private->unit_entries, 
                                                                                 GIMP_UNIT_ENTRIES_WIDTH),
                                               gimp_template_get_resolution_y (template));                               
        }
    }

#ifdef ENABLE_MEMSIZE_LABEL
  text = g_format_size (gimp_template_get_initial_size (template));
  gtk_label_set_text (GTK_LABEL (private->memsize_label), text);
  g_free (text);
#endif

  gimp_template_editor_set_pixels (editor, template);

  width  = gimp_template_get_width (template);
  height = gimp_template_get_height (template);

  if (width > height)
    aspect = GIMP_ASPECT_LANDSCAPE;
  else if (height > width)
    aspect = GIMP_ASPECT_PORTRAIT;
  else
    aspect = GIMP_ASPECT_SQUARE;

  private->block_aspect = TRUE;
  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (private->aspect_button),
                                   aspect);
  private->block_aspect = FALSE;

  gimp_enum_get_value (GIMP_TYPE_IMAGE_BASE_TYPE,
                       gimp_template_get_base_type (template),
                       NULL, NULL, &desc, NULL);

  xres = ROUND (gimp_template_get_resolution_y (template));
  yres = ROUND (gimp_template_get_resolution_x (template));

  if (xres != yres)
    text = g_strdup_printf (_("%d × %d ppi, %s"), xres, yres, desc);
  else
    text = g_strdup_printf (_("%d ppi, %s"), yres, desc);

  gtk_label_set_text (GTK_LABEL (private->more_label), text);
  g_free (text);
}
