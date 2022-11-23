/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "gegl/ligma-babl.h"

#include "core/ligma.h"
#include "core/ligmatemplate.h"
#include "core/ligma-utils.h"

#include "ligmapropwidgets.h"
#include "ligmatemplateeditor.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


#define SB_WIDTH            8
#define MAX_COMMENT_LENGTH  512  /* arbitrary */


enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_TEMPLATE
};


typedef struct _LigmaTemplateEditorPrivate LigmaTemplateEditorPrivate;

struct _LigmaTemplateEditorPrivate
{
  Ligma          *ligma;

  LigmaTemplate  *template;

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
        ((LigmaTemplateEditorPrivate *) ligma_template_editor_get_instance_private ((LigmaTemplateEditor *) (editor)))


static void    ligma_template_editor_constructed    (GObject            *object);
static void    ligma_template_editor_finalize       (GObject            *object);
static void    ligma_template_editor_set_property   (GObject            *object,
                                                    guint               property_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void    ligma_template_editor_get_property   (GObject            *object,
                                                    guint               property_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);

static void ligma_template_editor_precision_changed (GtkWidget          *widget,
                                                    LigmaTemplateEditor *editor);
static void ligma_template_editor_simulation_intent_changed
                                                   (GtkWidget          *widget,
                                                    LigmaTemplateEditor *editor);
static void ligma_template_editor_simulation_bpc_toggled
                                                   (GtkWidget          *widget,
                                                    LigmaTemplateEditor *editor);

static void ligma_template_editor_aspect_callback   (GtkWidget          *widget,
                                                    LigmaTemplateEditor *editor);
static void ligma_template_editor_template_notify   (LigmaTemplate       *template,
                                                    GParamSpec         *param_spec,
                                                    LigmaTemplateEditor *editor);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaTemplateEditor, ligma_template_editor,
                            GTK_TYPE_BOX)

#define parent_class ligma_template_editor_parent_class


static void
ligma_template_editor_class_init (LigmaTemplateEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_template_editor_constructed;
  object_class->finalize     = ligma_template_editor_finalize;
  object_class->set_property = ligma_template_editor_set_property;
  object_class->get_property = ligma_template_editor_get_property;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma", NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TEMPLATE,
                                   g_param_spec_object ("template", NULL, NULL,
                                                        LIGMA_TYPE_TEMPLATE,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_template_editor_init (LigmaTemplateEditor *editor)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 12);
}

static void
ligma_template_editor_constructed (GObject *object)
{
  LigmaTemplateEditor        *editor  = LIGMA_TEMPLATE_EDITOR (object);
  LigmaTemplateEditorPrivate *private = GET_PRIVATE (object);
  LigmaTemplate              *template;
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
  GList                     *focus_chain = NULL;
  gchar                     *text;
  gint                       row;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (private->ligma != NULL);
  ligma_assert (private->template != NULL);

  template = private->template;

  /*  Image size frame  */
  frame = ligma_frame_new (_("Image Size"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  width = ligma_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (width), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (width), SB_WIDTH);

  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  height = ligma_spin_button_new (adjustment, 1.0, 2);
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

  private->size_se = ligma_size_entry_new (0,
                                          ligma_template_get_unit (template),
                                          _("%p"),
                                          TRUE, FALSE, FALSE, SB_WIDTH,
                                          LIGMA_SIZE_ENTRY_UPDATE_SIZE);

  gtk_box_pack_start (GTK_BOX (hbox), private->size_se, FALSE, FALSE, 0);
  gtk_widget_show (private->size_se);

  ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (private->size_se),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_grid_attach (GTK_GRID (private->size_se), height, 0, 1, 1, 1);
  gtk_widget_show (height);

  ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (private->size_se),
                             GTK_SPIN_BUTTON (width), NULL);
  gtk_grid_attach (GTK_GRID (private->size_se), width, 0, 0, 1, 1);
  gtk_widget_show (width);

  ligma_prop_coordinates_connect (G_OBJECT (template),
                                 "width", "height", "unit",
                                 private->size_se, NULL,
                                 ligma_template_get_resolution_x (template),
                                 ligma_template_get_resolution_y (template));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 2, 2, 1);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  aspect_box = ligma_enum_icon_box_new (LIGMA_TYPE_ASPECT_TYPE,
                                       "ligma", GTK_ICON_SIZE_MENU,
                                       G_CALLBACK (ligma_template_editor_aspect_callback),
                                       editor, NULL,
                                       &private->aspect_button);
  gtk_widget_hide (private->aspect_button); /* hide "square" */

  gtk_box_pack_start (GTK_BOX (vbox), aspect_box, FALSE, FALSE, 0);
  gtk_widget_show (aspect_box);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  private->pixel_label = gtk_label_new (NULL);
  ligma_label_set_attributes (GTK_LABEL (private->pixel_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_label_set_xalign (GTK_LABEL (private->pixel_label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), private->pixel_label, FALSE, FALSE, 0);
  gtk_widget_show (private->pixel_label);

  private->more_label = gtk_label_new (NULL);
  ligma_label_set_attributes (GTK_LABEL (private->more_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_label_set_xalign (GTK_LABEL (private->more_label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), private->more_label, FALSE, FALSE, 0);
  gtk_widget_show (private->more_label);

#ifdef ENABLE_MEMSIZE_LABEL
  private->memsize_label = gtk_label_new (NULL);
  ligma_label_set_attributes (GTK_LABEL (private->memsize_label),
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

  frame = ligma_frame_new ("<expander>");
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
  xres = ligma_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (xres), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (xres), SB_WIDTH);

  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  yres = ligma_spin_button_new (adjustment, 1.0, 2);
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
    ligma_size_entry_new (0,
                         ligma_template_get_resolution_unit (template),
                         _("pixels/%s"),
                         FALSE, FALSE, FALSE, SB_WIDTH,
                         LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION);

  gtk_box_pack_start (GTK_BOX (hbox), private->resolution_se, FALSE, FALSE, 0);
  gtk_widget_show (private->resolution_se);

  ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (private->resolution_se),
                             GTK_SPIN_BUTTON (yres), NULL);
  gtk_grid_attach (GTK_GRID (private->resolution_se), yres, 0, 1, 1, 1);
  gtk_widget_show (yres);

  ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (private->resolution_se),
                             GTK_SPIN_BUTTON (xres), NULL);
  gtk_grid_attach (GTK_GRID (private->resolution_se), xres, 0, 0, 1, 1);
  gtk_widget_show (xres);

  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (private->size_se), 0,
                                  ligma_template_get_resolution_x (template),
                                  FALSE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (private->size_se), 1,
                                  ligma_template_get_resolution_y (template),
                                  FALSE);

  /*  the resolution chainbutton  */
  private->chain_button = ligma_chain_button_new (LIGMA_CHAIN_RIGHT);
  gtk_grid_attach (GTK_GRID (private->resolution_se), private->chain_button, 1, 0, 1, 2);
  gtk_widget_show (private->chain_button);

  ligma_prop_coordinates_connect (G_OBJECT (template),
                                 "xresolution", "yresolution",
                                 "resolution-unit",
                                 private->resolution_se, private->chain_button,
                                 1.0, 1.0);

  focus_chain = g_list_prepend (focus_chain,
                                ligma_size_entry_get_unit_combo (LIGMA_SIZE_ENTRY (private->resolution_se)));
  focus_chain = g_list_prepend (focus_chain, private->chain_button);
  focus_chain = g_list_prepend (focus_chain, yres);
  focus_chain = g_list_prepend (focus_chain, xres);

  gtk_container_set_focus_chain (GTK_CONTAINER (private->resolution_se),
                                 focus_chain);
  g_list_free (focus_chain);

  row = 2;

  combo = ligma_prop_enum_combo_box_new (G_OBJECT (template),
                                        "image-type",
                                        LIGMA_RGB, LIGMA_GRAY);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Color _space:"), 0.0, 0.5,
                            combo, 1);

  /* construct the precision combo manually, instead of using
   * ligma_prop_enum_combo_box_new(), so that we only reset the gamma combo when
   * the precision is changed through the ui.  see issue #3025.
   */
  store = ligma_enum_store_new_with_range (LIGMA_TYPE_COMPONENT_TYPE,
                                          LIGMA_COMPONENT_TYPE_U8,
                                          LIGMA_COMPONENT_TYPE_FLOAT);

  private->precision_combo = g_object_new (LIGMA_TYPE_ENUM_COMBO_BOX,
                                           "model", store,
                                           NULL);
  g_object_unref (store);

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Precision:"), 0.0, 0.5,
                            private->precision_combo, 1);

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (private->precision_combo),
                                 ligma_babl_component_type (
                                   ligma_template_get_precision (template)));

  g_signal_connect (private->precision_combo, "changed",
                    G_CALLBACK (ligma_template_editor_precision_changed),
                    editor);

  combo = ligma_prop_enum_combo_box_new (G_OBJECT (template), "trc",
                                        LIGMA_TRC_LINEAR,
                                        LIGMA_TRC_NON_LINEAR);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Gamma:"), 0.0, 0.5,
                            combo, 1);

  private->profile_combo =
    ligma_prop_profile_combo_box_new (G_OBJECT (template),
                                     "color-profile",
                                     NULL,
                                     _("Choose A Color Profile"),
                                     G_OBJECT (private->ligma->config),
                                     "color-profile-path");
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Co_lor profile:"), 0.0, 0.5,
                            private->profile_combo, 1);

  private->simulation_profile_combo =
    ligma_prop_profile_combo_box_new (G_OBJECT (template),
                                     "simulation-profile",
                                     NULL,
                                     _("Choose A Soft-Proofing Color Profile"),
                                     G_OBJECT (private->ligma->config),
                                     "color-profile-path");
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Soft-proofing color profile:"), 0.0, 0.5,
                            private->simulation_profile_combo, 1);

  private->simulation_intent_combo =
    ligma_enum_combo_box_new (LIGMA_TYPE_COLOR_RENDERING_INTENT);

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Soft-proofing rendering intent:"), 0.0, 0.5,
                            private->simulation_intent_combo, 1);

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (private->simulation_intent_combo),
                                 LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC);

  g_signal_connect (private->simulation_intent_combo, "changed",
                    G_CALLBACK (ligma_template_editor_simulation_intent_changed),
                    editor);

  private->simulation_bpc_toggle =
    gtk_check_button_new_with_mnemonic (_("_Use Black Point Compensation"));

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            NULL, 0.0, 0.5,
                            private->simulation_bpc_toggle, 1);

  g_signal_connect (private->simulation_bpc_toggle, "toggled",
                    G_CALLBACK (ligma_template_editor_simulation_bpc_toggled),
                    editor);

  combo = ligma_prop_enum_combo_box_new (G_OBJECT (template),
                                        "fill-type",
                                        0, 0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Fill with:"), 0.0, 0.5,
                            combo, 1);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                    _("Comme_nt:"), 0.0, 0.0,
                                    scrolled_window, 1);

  text_buffer = ligma_prop_text_buffer_new (G_OBJECT (template),
                                           "comment", MAX_COMMENT_LENGTH);

  text_view = gtk_text_view_new_with_buffer (text_buffer);
  g_object_unref (text_buffer);

  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), text_view);

  g_signal_connect_object (template, "notify",
                           G_CALLBACK (ligma_template_editor_template_notify),
                           editor, 0);

  /*  call the notify callback once to get the labels set initially  */
  ligma_template_editor_template_notify (template, NULL, editor);
}

static void
ligma_template_editor_finalize (GObject *object)
{
  LigmaTemplateEditorPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->template);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_template_editor_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaTemplateEditorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      private->ligma = g_value_get_object (value); /* don't ref */
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
ligma_template_editor_get_property (GObject      *object,
                                   guint         property_id,
                                   GValue       *value,
                                   GParamSpec   *pspec)
{
  LigmaTemplateEditorPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, private->ligma);
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
ligma_template_editor_new (LigmaTemplate *template,
                          Ligma         *ligma,
                          gboolean      edit_template)
{
  LigmaTemplateEditor        *editor;
  LigmaTemplateEditorPrivate *private;

  g_return_val_if_fail (LIGMA_IS_TEMPLATE (template), NULL);
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  editor = g_object_new (LIGMA_TYPE_TEMPLATE_EDITOR,
                         "ligma",     ligma,
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

      entry = ligma_prop_entry_new (G_OBJECT (private->template), "name", 128);

      ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                _("_Name:"), 1.0, 0.5,
                                entry, 1);

      icon_picker = ligma_prop_icon_picker_new (LIGMA_VIEWABLE (private->template),
                                               ligma);
      ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                _("_Icon:"), 1.0, 0.5,
                                icon_picker, 1);
    }

  return GTK_WIDGET (editor);
}

LigmaTemplate *
ligma_template_editor_get_template (LigmaTemplateEditor *editor)
{
  g_return_val_if_fail (LIGMA_IS_TEMPLATE_EDITOR (editor), NULL);

  return GET_PRIVATE (editor)->template;
}

void
ligma_template_editor_show_advanced (LigmaTemplateEditor *editor,
                                    gboolean            expanded)
{
  LigmaTemplateEditorPrivate *private;

  g_return_if_fail (LIGMA_IS_TEMPLATE_EDITOR (editor));

  private = GET_PRIVATE (editor);

  gtk_expander_set_expanded (GTK_EXPANDER (private->expander), expanded);
}

GtkWidget *
ligma_template_editor_get_size_se (LigmaTemplateEditor *editor)
{
  g_return_val_if_fail (LIGMA_IS_TEMPLATE_EDITOR (editor), NULL);

  return GET_PRIVATE (editor)->size_se;
}

GtkWidget *
ligma_template_editor_get_resolution_se (LigmaTemplateEditor *editor)
{
  g_return_val_if_fail (LIGMA_IS_TEMPLATE_EDITOR (editor), NULL);

  return GET_PRIVATE (editor)->resolution_se;
}

GtkWidget *
ligma_template_editor_get_resolution_chain (LigmaTemplateEditor *editor)
{
  g_return_val_if_fail (LIGMA_IS_TEMPLATE_EDITOR (editor), NULL);

  return GET_PRIVATE (editor)->chain_button;
}


/*  private functions  */

static void
ligma_template_editor_precision_changed (GtkWidget          *widget,
                                        LigmaTemplateEditor *editor)
{
  LigmaTemplateEditorPrivate *private = GET_PRIVATE (editor);
  LigmaComponentType          component_type;
  LigmaTRCType                trc;

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget),
                                 (gint *) &component_type);

  g_object_get (private->template,
                "trc", &trc,
                NULL);

  trc = ligma_suggest_trc_for_component_type (component_type, trc);

  g_object_set (private->template,
                "component-type", component_type,
                "trc",            trc,
                NULL);
}

static void
ligma_template_editor_simulation_intent_changed (GtkWidget          *widget,
                                                LigmaTemplateEditor *editor)
{
  LigmaTemplateEditorPrivate *private = GET_PRIVATE (editor);
  LigmaColorRenderingIntent   intent;

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget),
                                 (gint *) &intent);

  g_object_set (private->template,
                "simulation-intent", intent,
                NULL);
}

static void
ligma_template_editor_simulation_bpc_toggled (GtkWidget          *widget,
                                             LigmaTemplateEditor *editor)
{
  LigmaTemplateEditorPrivate *private = GET_PRIVATE (editor);
  gboolean                   bpc     = FALSE;

  bpc = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  g_object_set (private->template,
                "simulation-bpc", bpc,
                NULL);
}

static void
ligma_template_editor_set_pixels (LigmaTemplateEditor *editor,
                                 LigmaTemplate       *template)
{
  LigmaTemplateEditorPrivate *private = GET_PRIVATE (editor);
  gchar                     *text;

  text = g_strdup_printf (ngettext ("%d × %d pixel",
                                    "%d × %d pixels",
                                    ligma_template_get_height (template)),
                          ligma_template_get_width (template),
                          ligma_template_get_height (template));
  gtk_label_set_text (GTK_LABEL (private->pixel_label), text);
  g_free (text);
}

static void
ligma_template_editor_aspect_callback (GtkWidget          *widget,
                                      LigmaTemplateEditor *editor)
{
  LigmaTemplateEditorPrivate *private = GET_PRIVATE (editor);

  if (! private->block_aspect &&
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      LigmaTemplate *template    = private->template;
      gint          width       = ligma_template_get_width (template);
      gint          height      = ligma_template_get_height (template);
      gdouble       xresolution = ligma_template_get_resolution_x (template);
      gdouble       yresolution = ligma_template_get_resolution_y (template);

      if (width == height)
        {
          private->block_aspect = TRUE;
          ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (private->aspect_button),
                                           LIGMA_ASPECT_SQUARE);
          private->block_aspect = FALSE;
          return;
       }

      g_signal_handlers_block_by_func (template,
                                       ligma_template_editor_template_notify,
                                       editor);

      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (private->size_se), 0,
                                      yresolution, FALSE);
      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (private->size_se), 1,
                                      xresolution, FALSE);

      g_object_set (template,
                    "width",       height,
                    "height",      width,
                    "xresolution", yresolution,
                    "yresolution", xresolution,
                    NULL);

      g_signal_handlers_unblock_by_func (template,
                                         ligma_template_editor_template_notify,
                                         editor);

      ligma_template_editor_set_pixels (editor, template);
    }
}

static void
ligma_template_editor_template_notify (LigmaTemplate       *template,
                                      GParamSpec         *param_spec,
                                      LigmaTemplateEditor *editor)
{
  LigmaTemplateEditorPrivate *private = GET_PRIVATE (editor);
  LigmaAspectType             aspect;
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
          ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (private->size_se), 0,
                                          ligma_template_get_resolution_x (template),
                                          FALSE);
        }
      else if (! strcmp (param_spec->name, "yresolution"))
        {
          ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (private->size_se), 1,
                                          ligma_template_get_resolution_y (template),
                                          FALSE);
        }
      else if (! strcmp (param_spec->name, "component-type"))
        {
          g_signal_handlers_block_by_func (private->precision_combo,
                                           ligma_template_editor_precision_changed,
                                           editor);

          ligma_int_combo_box_set_active (
            LIGMA_INT_COMBO_BOX (private->precision_combo),
            ligma_babl_component_type (ligma_template_get_precision (template)));

          g_signal_handlers_unblock_by_func (private->precision_combo,
                                             ligma_template_editor_precision_changed,
                                             editor);
        }
    }

#ifdef ENABLE_MEMSIZE_LABEL
  text = g_format_size (ligma_template_get_initial_size (template));
  gtk_label_set_text (GTK_LABEL (private->memsize_label), text);
  g_free (text);
#endif

  ligma_template_editor_set_pixels (editor, template);

  width  = ligma_template_get_width (template);
  height = ligma_template_get_height (template);

  if (width > height)
    aspect = LIGMA_ASPECT_LANDSCAPE;
  else if (height > width)
    aspect = LIGMA_ASPECT_PORTRAIT;
  else
    aspect = LIGMA_ASPECT_SQUARE;

  private->block_aspect = TRUE;
  ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (private->aspect_button),
                                   aspect);
  private->block_aspect = FALSE;

  ligma_enum_get_value (LIGMA_TYPE_IMAGE_BASE_TYPE,
                       ligma_template_get_base_type (template),
                       NULL, NULL, &desc, NULL);

  xres = ROUND (ligma_template_get_resolution_x (template));
  yres = ROUND (ligma_template_get_resolution_y (template));

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
      LigmaColorProfile        *profile;
      GtkListStore            *profile_store;
      GFile                   *file;
      gchar                   *path;

      file = ligma_directory_file ("profilerc", NULL);
      profile_store = ligma_color_profile_store_new (file);
      g_object_unref (file);

      ligma_color_profile_store_add_defaults (LIGMA_COLOR_PROFILE_STORE (profile_store),
                                             private->ligma->config->color_management,
                                             ligma_template_get_base_type (template),
                                             ligma_template_get_precision (template),
                                             NULL);

      gtk_combo_box_set_model (GTK_COMBO_BOX (private->profile_combo),
                               GTK_TREE_MODEL (profile_store));

      /* Simulation Profile should not be set by default */
      file = ligma_directory_file ("profilerc", NULL);
      profile_store = ligma_color_profile_store_new (file);
      g_object_unref (file);

      ligma_color_profile_store_add_file (LIGMA_COLOR_PROFILE_STORE (profile_store),
                                         NULL, NULL);
      /* Add Preferred CMYK profile if it exists */
      profile =
        ligma_color_config_get_cmyk_color_profile (LIGMA_COLOR_CONFIG (private->ligma->config->color_management),
                                                  NULL);
      if (profile)
        {
          g_object_get (G_OBJECT (private->ligma->config->color_management),
                        "cmyk-profile", &path, NULL);
          file = ligma_file_new_for_config_path (path, NULL);
          g_free (path);
          text = g_strdup_printf (_("Preferred CMYK (%s)"),
                                  ligma_color_profile_get_label (profile));
          g_object_unref (profile);
          ligma_color_profile_store_add_file (LIGMA_COLOR_PROFILE_STORE (profile_store),
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

      ligma_color_profile_combo_box_set_active_file (LIGMA_COLOR_PROFILE_COMBO_BOX (private->profile_combo),
                                                    file, NULL);

      if (file)
        g_object_unref (file);

      g_object_get (template,
                    "simulation-profile", &file,
                    NULL);

      ligma_color_profile_combo_box_set_active_file (LIGMA_COLOR_PROFILE_COMBO_BOX (private->simulation_profile_combo),
                                                    file, NULL);

      if (file)
        g_object_unref (file);
    }
}
