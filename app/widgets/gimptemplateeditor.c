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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"

#include "core/gimp.h"
#include "core/gimptemplate.h"
#include "core/gimp-utils.h"

#include "gimppropwidgets.h"
#include "gimptemplateeditor.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define SB_WIDTH            8
#define MAX_COMMENT_LENGTH  512  /* arbitrary */


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_TEMPLATE
};


typedef struct _GimpTemplateEditorPrivate GimpTemplateEditorPrivate;

struct _GimpTemplateEditorPrivate
{
  Gimp          *gimp;

  GimpTemplate  *template;

  GtkWidget     *aspect_button;
  gboolean       block_aspect;

  GtkWidget     *expander;
  GtkWidget     *size_se;
  GtkWidget     *memsize_label;
  GtkWidget     *pixel_label;
  GtkWidget     *more_label;
  GtkWidget     *resolution_se;
  GtkWidget     *chain_button;
  GtkWidget     *precision_combo;
  GtkWidget     *profile_combo;
  GtkWidget     *simulation_profile_combo;
  GtkWidget     *simulation_intent_combo;
  GtkWidget     *simulation_bpc_toggle;
};

#define GET_PRIVATE(editor) \
        ((GimpTemplateEditorPrivate *) gimp_template_editor_get_instance_private ((GimpTemplateEditor *) (editor)))


static void    gimp_template_editor_constructed    (GObject            *object);
static void    gimp_template_editor_finalize       (GObject            *object);
static void    gimp_template_editor_set_property   (GObject            *object,
                                                    guint               property_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void    gimp_template_editor_get_property   (GObject            *object,
                                                    guint               property_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);

static void gimp_template_editor_precision_changed (GtkWidget          *widget,
                                                    GimpTemplateEditor *editor);
static void gimp_template_editor_simulation_intent_changed
                                                   (GtkWidget          *widget,
                                                    GimpTemplateEditor *editor);
static void gimp_template_editor_simulation_bpc_toggled
                                                   (GtkWidget          *widget,
                                                    GimpTemplateEditor *editor);

static void gimp_template_editor_aspect_callback   (GtkWidget          *widget,
                                                    GimpTemplateEditor *editor);
static void gimp_template_editor_template_notify   (GimpTemplate       *template,
                                                    GParamSpec         *param_spec,
                                                    GimpTemplateEditor *editor);


G_DEFINE_TYPE_WITH_PRIVATE (GimpTemplateEditor, gimp_template_editor,
                            GTK_TYPE_BOX)

#define parent_class gimp_template_editor_parent_class


static void
gimp_template_editor_class_init (GimpTemplateEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_template_editor_constructed;
  object_class->finalize     = gimp_template_editor_finalize;
  object_class->set_property = gimp_template_editor_set_property;
  object_class->get_property = gimp_template_editor_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TEMPLATE,
                                   g_param_spec_object ("template", NULL, NULL,
                                                        GIMP_TYPE_TEMPLATE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
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
  GtkWidget                 *grid;
  GtkWidget                 *label;
  GtkAdjustment             *adjustment;
  GtkWidget                 *width;
  GtkWidget                 *height;
  GtkWidget                 *xres;
  GtkWidget                 *yres;
  GtkWidget                 *combo;
  GtkWidget                 *scrolled_window;
  GtkWidget                 *text_view;
  GtkTextBuffer             *text_buffer;
  GtkListStore              *store;
  gchar                     *text;
  gint                       row;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (private->gimp != NULL);
  gimp_assert (private->template != NULL);

  template = private->template;

  /*  Image size frame  */
  frame = gimp_frame_new (_("Image Size"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  width = gimp_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (width), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (width), SB_WIDTH);

  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  height = gimp_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (height), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (height), SB_WIDTH);

  /*  the image size labels  */
  label = gtk_label_new_with_mnemonic (_("_Width:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), width);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("H_eight:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), height);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_show (label);

  /*  create the sizeentry which keeps it all together  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 0, 1, 2);
  gtk_widget_show (hbox);

  private->size_se = gimp_size_entry_new (0,
                                          gimp_template_get_unit (template),
                                          _("%n"),
                                          TRUE, FALSE, FALSE, SB_WIDTH,
                                          GIMP_SIZE_ENTRY_UPDATE_SIZE);

  gtk_box_pack_start (GTK_BOX (hbox), private->size_se, FALSE, FALSE, 0);
  gtk_widget_show (private->size_se);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->size_se),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_grid_attach (GTK_GRID (private->size_se), height, 0, 1, 1, 1);
  gtk_widget_show (height);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->size_se),
                             GTK_SPIN_BUTTON (width), NULL);
  gtk_grid_attach (GTK_GRID (private->size_se), width, 0, 0, 1, 1);
  gtk_widget_show (width);

  gimp_prop_coordinates_connect (G_OBJECT (template),
                                 "width", "height", "unit",
                                 private->size_se, NULL,
                                 gimp_template_get_resolution_x (template),
                                 gimp_template_get_resolution_y (template));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 2, 2, 1);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  aspect_box = gimp_enum_icon_box_new (GIMP_TYPE_ASPECT_TYPE,
                                       "gimp", GTK_ICON_SIZE_MENU,
                                       G_CALLBACK (gimp_template_editor_aspect_callback),
                                       editor, NULL,
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
  gtk_label_set_xalign (GTK_LABEL (private->pixel_label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), private->pixel_label, FALSE, FALSE, 0);
  gtk_widget_show (private->pixel_label);

  private->more_label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (private->more_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_label_set_xalign (GTK_LABEL (private->more_label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), private->more_label, FALSE, FALSE, 0);
  gtk_widget_show (private->more_label);

#ifdef ENABLE_MEMSIZE_LABEL
  private->memsize_label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (private->memsize_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_label_set_xalign (GTK_LABEL (private->memsize_label), 0.0);
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

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_window, -1, 300);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_OUT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
  gtk_widget_show (scrolled_window);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 16);
  gtk_container_add (GTK_CONTAINER (scrolled_window), grid);
  gtk_widget_show (grid);

  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  xres = gimp_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (xres), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (xres), SB_WIDTH);

  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  yres = gimp_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (yres), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (yres), SB_WIDTH);

  /*  the resolution labels  */
  label = gtk_label_new_with_mnemonic (_("_X resolution:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), xres);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("_Y resolution:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), yres);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_show (label);

  /*  the resolution sizeentry  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 0, 1, 2);
  gtk_widget_show (hbox);

  private->resolution_se =
    gimp_size_entry_new (0,
                         gimp_template_get_resolution_unit (template),
                         _("pixels/%a"),
                         FALSE, FALSE, FALSE, SB_WIDTH,
                         GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);

  gtk_box_pack_start (GTK_BOX (hbox), private->resolution_se, FALSE, FALSE, 0);
  gtk_widget_show (private->resolution_se);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->resolution_se),
                             GTK_SPIN_BUTTON (yres), NULL);
  gtk_grid_attach (GTK_GRID (private->resolution_se), yres, 0, 1, 1, 1);
  gtk_widget_show (yres);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->resolution_se),
                             GTK_SPIN_BUTTON (xres), NULL);
  gtk_grid_attach (GTK_GRID (private->resolution_se), xres, 0, 0, 1, 1);
  gtk_widget_show (xres);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se), 0,
                                  gimp_template_get_resolution_x (template),
                                  FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se), 1,
                                  gimp_template_get_resolution_y (template),
                                  FALSE);

  /*  the resolution chainbutton  */
  private->chain_button = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gtk_grid_attach (GTK_GRID (private->resolution_se), private->chain_button, 1, 0, 1, 2);
  gtk_widget_show (private->chain_button);

  gimp_prop_coordinates_connect (G_OBJECT (template),
                                 "xresolution", "yresolution",
                                 "resolution-unit",
                                 private->resolution_se, private->chain_button,
                                 1.0, 1.0);

  row = 2;

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (template),
                                        "image-type",
                                        GIMP_RGB, GIMP_GRAY);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Color _space:"), 0.0, 0.5,
                            combo, 1);

  /* construct the precision combo manually, instead of using
   * gimp_prop_enum_combo_box_new(), so that we only reset the gamma combo when
   * the precision is changed through the ui.  see issue #3025.
   */
  store = gimp_enum_store_new_with_range (GIMP_TYPE_COMPONENT_TYPE,
                                          GIMP_COMPONENT_TYPE_U8,
                                          GIMP_COMPONENT_TYPE_FLOAT);

  private->precision_combo = g_object_new (GIMP_TYPE_ENUM_COMBO_BOX,
                                           "model", store,
                                           NULL);
  g_object_unref (store);

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Precision:"), 0.0, 0.5,
                            private->precision_combo, 1);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (private->precision_combo),
                                 gimp_babl_component_type (
                                   gimp_template_get_precision (template)));

  g_signal_connect (private->precision_combo, "changed",
                    G_CALLBACK (gimp_template_editor_precision_changed),
                    editor);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (template), "trc",
                                        GIMP_TRC_LINEAR,
                                        GIMP_TRC_NON_LINEAR);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Gamma:"), 0.0, 0.5,
                            combo, 1);

  private->profile_combo =
    gimp_prop_profile_combo_box_new (G_OBJECT (template),
                                     "color-profile",
                                     NULL,
                                     _("Choose A Color Profile"),
                                     G_OBJECT (private->gimp->config),
                                     "color-profile-path");
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Co_lor profile:"), 0.0, 0.5,
                            private->profile_combo, 1);

  private->simulation_profile_combo =
    gimp_prop_profile_combo_box_new (G_OBJECT (template),
                                     "simulation-profile",
                                     NULL,
                                     _("Choose A Soft-Proofing Color Profile"),
                                     G_OBJECT (private->gimp->config),
                                     "color-profile-path");
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Soft-proofing color profile:"), 0.0, 0.5,
                            private->simulation_profile_combo, 1);

  private->simulation_intent_combo =
    gimp_prop_enum_combo_box_new (G_OBJECT (template),
                                  "simulation-intent",
                                  0, 0);

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Soft-proofing rendering intent:"), 0.0, 0.5,
                            private->simulation_intent_combo, 1);

  g_signal_connect (private->simulation_intent_combo, "changed",
                    G_CALLBACK (gimp_template_editor_simulation_intent_changed),
                    editor);

  private->simulation_bpc_toggle =
    gimp_prop_check_button_new (G_OBJECT (template), "simulation-bpc",
                                _("_Use Black Point Compensation"));

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            NULL, 0.0, 0.5,
                            private->simulation_bpc_toggle, 1);

  g_signal_connect (private->simulation_bpc_toggle, "toggled",
                    G_CALLBACK (gimp_template_editor_simulation_bpc_toggled),
                    editor);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (template),
                                        "fill-type",
                                        0, 0);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Fill with:"), 0.0, 0.5,
                            combo, 1);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  label = gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                    _("Comme_nt:"), 0.0, 0.0,
                                    scrolled_window, 1);

  text_buffer = gimp_prop_text_buffer_new (G_OBJECT (template),
                                           "comment", MAX_COMMENT_LENGTH);

  text_view = gtk_text_view_new_with_buffer (text_buffer);
  g_object_unref (text_buffer);

  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), text_view);

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

  g_clear_object (&private->template);

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
    case PROP_GIMP:
      private->gimp = g_value_get_object (value); /* don't ref */
      break;

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
    case PROP_GIMP:
      g_value_set_object (value, private->gimp);
      break;

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

  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  editor = g_object_new (GIMP_TYPE_TEMPLATE_EDITOR,
                         "gimp",     gimp,
                         "template", template,
                         NULL);

  private = GET_PRIVATE (editor);

  if (edit_template)
    {
      GtkWidget   *grid;
      GtkWidget   *entry;
      GtkWidget   *icon_picker;

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_box_pack_start (GTK_BOX (editor), grid, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (editor), grid, 0);
      gtk_widget_show (grid);

      entry = gimp_prop_entry_new (G_OBJECT (private->template), "name", 128);

      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                _("_Name:"), 1.0, 0.5,
                                entry, 1);

      icon_picker = gimp_prop_icon_picker_new (GIMP_VIEWABLE (private->template),
                                               gimp);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                _("_Icon:"), 1.0, 0.5,
                                icon_picker, 1);
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

GtkWidget *
gimp_template_editor_get_size_se (GimpTemplateEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE_EDITOR (editor), NULL);

  return GET_PRIVATE (editor)->size_se;
}

GtkWidget *
gimp_template_editor_get_resolution_se (GimpTemplateEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE_EDITOR (editor), NULL);

  return GET_PRIVATE (editor)->resolution_se;
}

GtkWidget *
gimp_template_editor_get_resolution_chain (GimpTemplateEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE_EDITOR (editor), NULL);

  return GET_PRIVATE (editor)->chain_button;
}


/*  private functions  */

static void
gimp_template_editor_precision_changed (GtkWidget          *widget,
                                        GimpTemplateEditor *editor)
{
  GimpTemplateEditorPrivate *private = GET_PRIVATE (editor);
  GimpComponentType          component_type;
  GimpTRCType                trc;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                 (gint *) &component_type);

  g_object_get (private->template,
                "trc", &trc,
                NULL);

  trc = gimp_suggest_trc_for_component_type (component_type, trc);

  g_object_set (private->template,
                "component-type", component_type,
                "trc",            trc,
                NULL);
}

static void
gimp_template_editor_simulation_intent_changed (GtkWidget          *widget,
                                                GimpTemplateEditor *editor)
{
  GimpTemplateEditorPrivate *private = GET_PRIVATE (editor);
  GimpColorRenderingIntent   intent;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget),
                                 (gint *) &intent);

  g_object_set (private->template,
                "simulation-intent", intent,
                NULL);
}

static void
gimp_template_editor_simulation_bpc_toggled (GtkWidget          *widget,
                                             GimpTemplateEditor *editor)
{
  GimpTemplateEditorPrivate *private = GET_PRIVATE (editor);
  gboolean                   bpc     = FALSE;

  bpc = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  g_object_set (private->template,
                "simulation-bpc", bpc,
                NULL);
}

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

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se), 0,
                                      yresolution, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se), 1,
                                      xresolution, FALSE);

      g_object_set (template,
                    "width",       height,
                    "height",      width,
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
          gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se), 0,
                                          gimp_template_get_resolution_x (template),
                                          FALSE);
        }
      else if (! strcmp (param_spec->name, "yresolution"))
        {
          gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se), 1,
                                          gimp_template_get_resolution_y (template),
                                          FALSE);
        }
      else if (! strcmp (param_spec->name, "component-type"))
        {
          g_signal_handlers_block_by_func (private->precision_combo,
                                           gimp_template_editor_precision_changed,
                                           editor);

          gimp_int_combo_box_set_active (
            GIMP_INT_COMBO_BOX (private->precision_combo),
            gimp_babl_component_type (gimp_template_get_precision (template)));

          g_signal_handlers_unblock_by_func (private->precision_combo,
                                             gimp_template_editor_precision_changed,
                                             editor);
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

  xres = ROUND (gimp_template_get_resolution_x (template));
  yres = ROUND (gimp_template_get_resolution_y (template));

  if (xres != yres)
    text = g_strdup_printf (_("%d × %d ppi, %s"), xres, yres, desc);
  else
    text = g_strdup_printf (_("%d ppi, %s"), yres, desc);

  gtk_label_set_text (GTK_LABEL (private->more_label), text);
  g_free (text);

  if (! param_spec                              ||
      ! strcmp (param_spec->name, "image-type") ||
      ! strcmp (param_spec->name, "precision"))
    {
      GimpColorProfile        *profile;
      GtkListStore            *profile_store;
      GFile                   *file;
      gchar                   *path;

      file = gimp_directory_file ("profilerc", NULL);
      profile_store = gimp_color_profile_store_new (file);
      g_object_unref (file);

      gimp_color_profile_store_add_defaults (GIMP_COLOR_PROFILE_STORE (profile_store),
                                             private->gimp->config->color_management,
                                             gimp_template_get_base_type (template),
                                             gimp_template_get_precision (template),
                                             NULL);

      gtk_combo_box_set_model (GTK_COMBO_BOX (private->profile_combo),
                               GTK_TREE_MODEL (profile_store));

      /* Simulation Profile should not be set by default */
      file = gimp_directory_file ("profilerc", NULL);
      profile_store = gimp_color_profile_store_new (file);
      g_object_unref (file);

      gimp_color_profile_store_add_file (GIMP_COLOR_PROFILE_STORE (profile_store),
                                         NULL, NULL);
      /* Add Preferred CMYK profile if it exists */
      profile =
        gimp_color_config_get_cmyk_color_profile (GIMP_COLOR_CONFIG (private->gimp->config->color_management),
                                                  NULL);
      if (profile)
        {
          g_object_get (G_OBJECT (private->gimp->config->color_management),
                        "cmyk-profile", &path, NULL);
          file = gimp_file_new_for_config_path (path, NULL);
          g_free (path);
          text = g_strdup_printf (_("Preferred CMYK (%s)"),
                                  gimp_color_profile_get_label (profile));
          g_object_unref (profile);
          gimp_color_profile_store_add_file (GIMP_COLOR_PROFILE_STORE (profile_store),
                                             file, text);
          g_object_unref (file);
          g_free (text);
        }

      gtk_combo_box_set_model (GTK_COMBO_BOX (private->simulation_profile_combo),
                               GTK_TREE_MODEL (profile_store));

      g_object_unref (profile_store);

      g_object_get (template,
                    "color-profile", &file,
                    NULL);

      gimp_color_profile_combo_box_set_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (private->profile_combo),
                                                    file, NULL);

      if (file)
        g_object_unref (file);

      g_object_get (template,
                    "simulation-profile", &file,
                    NULL);

      gimp_color_profile_combo_box_set_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (private->simulation_profile_combo),
                                                    file, NULL);

      if (file)
        g_object_unref (file);
    }
}
