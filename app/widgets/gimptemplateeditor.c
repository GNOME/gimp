/* GIMP - The GNU Image Manipulation Program
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimpcontext.h"
#include "core/gimptemplate.h"

#include "gimptemplateeditor.h"
#include "gimpviewablebutton.h"

#include "gimp-intl.h"


#define SB_WIDTH            8
#define MAX_COMMENT_LENGTH  512  /* arbitrary */

enum
{
  PROP_0,
  PROP_TEMPLATE
};


static GObject *gimp_template_editor_constructor (GType               type,
                                                  guint               n_params,
                                                  GObjectConstructParam *params);
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
static void gimp_template_editor_icon_changed    (GimpContext        *context,
                                                  GimpTemplate       *template,
                                                  GimpTemplateEditor *editor);


G_DEFINE_TYPE (GimpTemplateEditor, gimp_template_editor, GTK_TYPE_VBOX)

#define parent_class gimp_template_editor_parent_class


static void
gimp_template_editor_class_init (GimpTemplateEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_template_editor_constructor;
  object_class->finalize     = gimp_template_editor_finalize;
  object_class->set_property = gimp_template_editor_set_property;
  object_class->get_property = gimp_template_editor_get_property;

  g_object_class_install_property (object_class, PROP_TEMPLATE,
                                   g_param_spec_object ("template", NULL, NULL,
                                                        GIMP_TYPE_TEMPLATE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_template_editor_init (GimpTemplateEditor *editor)
{
  editor->template = NULL;
}

static GObject *
gimp_template_editor_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GimpTemplateEditor *editor;
  GObject            *object;
  GtkWidget          *aspect_box;
  GtkWidget          *frame;
  GtkWidget          *hbox;
  GtkWidget          *vbox;
  GtkWidget          *table;
  GtkWidget          *label;
  GtkObject          *adjustment;
  GtkWidget          *width;
  GtkWidget          *height;
  GtkWidget          *xres;
  GtkWidget          *yres;
  GtkWidget          *chainbutton;
  GtkWidget          *combo;
  GtkWidget          *scrolled_window;
  GtkWidget          *text_view;
  GtkTextBuffer      *text_buffer;
  GList              *focus_chain = NULL;
  gchar              *text;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_TEMPLATE_EDITOR (object);

  g_assert (editor->template != NULL);

  gtk_box_set_spacing (GTK_BOX (editor), 12);

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

  /*  the image size labels  */
  label = gtk_label_new_with_mnemonic (_("_Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), width);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("H_eight:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), height);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  create the sizeentry which keeps it all together  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 0, 2);
  gtk_widget_show (hbox);

  editor->size_se = gimp_size_entry_new (0, editor->template->unit,_("%p"),
                                         TRUE, FALSE, FALSE, SB_WIDTH,
                                         GIMP_SIZE_ENTRY_UPDATE_SIZE);

  gtk_table_set_row_spacing (GTK_TABLE (editor->size_se), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (editor->size_se), 1, 6);

  gtk_box_pack_start (GTK_BOX (hbox), editor->size_se, FALSE, FALSE, 0);
  gtk_widget_show (editor->size_se);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (editor->size_se),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (editor->size_se), height, 0, 1, 1, 2);
  gtk_widget_show (height);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (editor->size_se),
                             GTK_SPIN_BUTTON (width), NULL);
  gtk_table_attach_defaults (GTK_TABLE (editor->size_se), width, 0, 1, 0, 1);
  gtk_widget_show (width);

  gimp_prop_coordinates_connect (G_OBJECT (editor->template),
                                 "width", "height", "unit",
                                 editor->size_se, NULL,
                                 editor->template->xresolution,
                                 editor->template->yresolution);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 3, 2, 3);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (0, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  aspect_box = gimp_enum_stock_box_new (GIMP_TYPE_ASPECT_TYPE,
                                        "gimp", GTK_ICON_SIZE_MENU,
                                        G_CALLBACK (gimp_template_editor_aspect_callback),
                                        editor,
                                        &editor->aspect_button);
  gtk_widget_hide (editor->aspect_button); /* hide "square" */

  gtk_box_pack_start (GTK_BOX (vbox), aspect_box, FALSE, FALSE, 0);
  gtk_widget_show (aspect_box);

  vbox = gtk_vbox_new (2, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  editor->pixel_label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (editor->pixel_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (editor->pixel_label), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), editor->pixel_label, FALSE, FALSE, 0);
  gtk_widget_show (editor->pixel_label);

  editor->more_label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (editor->more_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (editor->more_label), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), editor->more_label, FALSE, FALSE, 0);
  gtk_widget_show (editor->more_label);

#ifdef ENABLE_MEMSIZE_LABEL
  editor->memsize_label = gtk_label_new (NULL);
  gimp_label_set_attributes (GTK_LABEL (editor->memsize_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_misc_set_alignment (GTK_MISC (editor->memsize_label), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), editor->memsize_label, FALSE, FALSE, 0);
  gtk_widget_show (editor->memsize_label);
#endif

  text = g_strdup_printf ("<b>%s</b>", _("_Advanced Options"));
  editor->expander = g_object_new (GTK_TYPE_EXPANDER,
                                   "label",         text,
                                   "use-markup",    TRUE,
                                   "use-underline", TRUE,
                                   NULL);
  g_free (text);

  gtk_box_pack_start (GTK_BOX (editor), editor->expander, TRUE, TRUE, 0);
  gtk_widget_show (editor->expander);

  frame = gimp_frame_new ("<expander>");
  gtk_container_add (GTK_CONTAINER (editor->expander), frame);
  gtk_widget_show (frame);

  table = gtk_table_new (5, 2, FALSE);
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

  /*  the resolution labels  */
  label = gtk_label_new_with_mnemonic (_("_X resolution:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), xres);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("_Y resolution:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), yres);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the resolution sizeentry  */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 0, 2);
  gtk_widget_show (hbox);

  editor->resolution_se =
    gimp_size_entry_new (0, editor->template->resolution_unit, _("pixels/%a"),
                         FALSE, FALSE, FALSE, SB_WIDTH,
                         GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);

  gtk_table_set_row_spacing (GTK_TABLE (editor->resolution_se), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (editor->resolution_se), 1, 2);
  gtk_table_set_col_spacing (GTK_TABLE (editor->resolution_se), 2, 2);

  gtk_box_pack_start (GTK_BOX (hbox), editor->resolution_se, FALSE, FALSE, 0);
  gtk_widget_show (editor->resolution_se);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (editor->resolution_se),
                             GTK_SPIN_BUTTON (yres), NULL);
  gtk_table_attach_defaults (GTK_TABLE (editor->resolution_se), yres,
                             0, 1, 1, 2);
  gtk_widget_show (yres);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (editor->resolution_se),
                             GTK_SPIN_BUTTON (xres), NULL);
  gtk_table_attach_defaults (GTK_TABLE (editor->resolution_se), xres,
                             0, 1, 0, 1);
  gtk_widget_show (xres);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_se), 0,
                                  editor->template->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_se), 1,
                                  editor->template->yresolution, FALSE);

  /*  the resolution chainbutton  */
  chainbutton = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gtk_table_attach_defaults (GTK_TABLE (editor->resolution_se), chainbutton,
                             1, 2, 0, 2);
  gtk_widget_show (chainbutton);

  gimp_prop_coordinates_connect (G_OBJECT (editor->template),
                                 "xresolution", "yresolution",
                                 "resolution-unit",
                                 editor->resolution_se, chainbutton,
                                 1.0, 1.0);

  focus_chain = g_list_prepend (focus_chain,
                                GIMP_SIZE_ENTRY (editor->resolution_se)->unitmenu);
  focus_chain = g_list_prepend (focus_chain, chainbutton);
  focus_chain = g_list_prepend (focus_chain, yres);
  focus_chain = g_list_prepend (focus_chain, xres);

  gtk_container_set_focus_chain (GTK_CONTAINER (editor->resolution_se),
                                 focus_chain);
  g_list_free (focus_chain);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (editor->template),
                                        "image-type",
                                        GIMP_RGB, GIMP_GRAY);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Color _space:"), 0.0, 0.5,
                             combo, 1, FALSE);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (editor->template),
                                        "fill-type",
                                        GIMP_FOREGROUND_FILL,
                                        GIMP_TRANSPARENT_FILL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                             _("_Fill with:"), 0.0, 0.5,
                             combo, 1, FALSE);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
                             _("Comme_nt:"), 0.0, 0.0,
                             scrolled_window, 1, FALSE);

  text_buffer = gimp_prop_text_buffer_new (G_OBJECT (editor->template),
                                           "comment", MAX_COMMENT_LENGTH);

  text_view = gtk_text_view_new_with_buffer (text_buffer);
  g_object_unref (text_buffer);

  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  g_signal_connect_object (editor->template, "notify",
                           G_CALLBACK (gimp_template_editor_template_notify),
                           editor, 0);

  /*  call the notify callback once to get the labels set initially  */
  gimp_template_editor_template_notify (editor->template, NULL, editor);

  return object;
}

static void
gimp_template_editor_finalize (GObject *object)
{
  GimpTemplateEditor *editor = GIMP_TEMPLATE_EDITOR (object);

  if (editor->template)
    {
      g_object_unref (editor->template);
      editor->template = NULL;
    }

  if (editor->stock_id_container)
    {
      g_object_unref (editor->stock_id_container);
      editor->stock_id_container = NULL;
    }

  if (editor->stock_id_context)
    {
      g_object_unref (editor->stock_id_context);
      editor->stock_id_context = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_template_editor_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpTemplateEditor *editor = GIMP_TEMPLATE_EDITOR (object);

  switch (property_id)
    {
    case PROP_TEMPLATE:
      editor->template = GIMP_TEMPLATE (g_value_dup_object (value));
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
  GimpTemplateEditor *editor = GIMP_TEMPLATE_EDITOR (object);

  switch (property_id)
    {
    case PROP_TEMPLATE:
      g_value_set_object (value, editor->template);
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
  GimpTemplateEditor *editor;

  g_return_val_if_fail (!edit_template || GIMP_IS_GIMP (gimp), NULL);

  editor = g_object_new (GIMP_TYPE_TEMPLATE_EDITOR,
                         "template", template,
                         NULL);

  if (edit_template)
    {
      GtkWidget   *table;
      GtkWidget   *entry;
      GtkWidget   *button;
      GSList      *stock_list;
      GSList      *list;
      const gchar *stock_id;

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (editor->template));

      editor->stock_id_container = gimp_list_new (GIMP_TYPE_TEMPLATE, FALSE);
      editor->stock_id_context = gimp_context_new (gimp, "foo", NULL);

      g_signal_connect (editor->stock_id_context, "template-changed",
                        G_CALLBACK (gimp_template_editor_icon_changed),
                        editor);

      stock_list = gtk_stock_list_ids ();

      for (list = stock_list; list; list = g_slist_next (list))
        {
          GimpObject *object = g_object_new (GIMP_TYPE_TEMPLATE,
                                             "name",     list->data,
                                             "stock-id", list->data,
                                             NULL);

          gimp_container_add (editor->stock_id_container, object);
          g_object_unref (object);

          if (strcmp (list->data, stock_id) == 0)
            gimp_context_set_template (editor->stock_id_context,
                                       GIMP_TEMPLATE (object));
        }

      g_slist_foreach (stock_list, (GFunc) g_free, NULL);
      g_slist_free (stock_list);

      table = gtk_table_new (2, 2, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 6);
      gtk_table_set_row_spacings (GTK_TABLE (table), 6);
      gtk_box_pack_start (GTK_BOX (editor), table, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (editor), table, 0);
      gtk_widget_show (table);

      entry = gimp_prop_entry_new (G_OBJECT (editor->template), "name", 128);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("_Name:"), 1.0, 0.5,
                                 entry, 1, FALSE);

      button = gimp_viewable_button_new (editor->stock_id_container,
                                         editor->stock_id_context,
                                         GIMP_VIEW_TYPE_LIST,
                                         GIMP_VIEW_SIZE_SMALL,
                                         GIMP_VIEW_SIZE_SMALL, 0,
                                         NULL, NULL, NULL, NULL);
      gimp_viewable_button_set_view_type (GIMP_VIEWABLE_BUTTON (button),
                                          GIMP_VIEW_TYPE_GRID);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                 _("_Icon:"), 1.0, 0.5,
                                 button, 1, TRUE);
    }

  return GTK_WIDGET (editor);
}

void
gimp_template_editor_show_advanced (GimpTemplateEditor *editor,
                                    gboolean            expanded)
{
  g_return_if_fail (GIMP_IS_TEMPLATE_EDITOR (editor));

  gtk_expander_set_expanded (GTK_EXPANDER (editor->expander), expanded);
}


/*  private functions  */

static void
gimp_template_editor_set_pixels (GimpTemplateEditor *editor,
                                 GimpTemplate       *template)
{
  gchar *text = g_strdup_printf (ngettext ("%d × %d pixel",
                                           "%d × %d pixels", template->height),
                                 template->width, template->height);
  gtk_label_set_text (GTK_LABEL (editor->pixel_label), text);
  g_free (text);
}

static void
gimp_template_editor_aspect_callback (GtkWidget          *widget,
                                      GimpTemplateEditor *editor)
{
  if (! editor->block_aspect && GTK_TOGGLE_BUTTON (widget)->active)
    {
      GimpTemplate *template    = editor->template;
      gint          width       = template->width;
      gint          height      = template->height;
      gdouble       xresolution = template->xresolution;
      gdouble       yresolution = template->yresolution;

      if (template->width == template->height)
        {
          editor->block_aspect = TRUE;
          gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (editor->aspect_button),
                                           GIMP_ASPECT_SQUARE);
          editor->block_aspect = FALSE;
          return;
       }

      g_signal_handlers_block_by_func (template,
                                       gimp_template_editor_template_notify,
                                       editor);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_se), 0,
                                      yresolution, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_se), 1,
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
  GimpAspectType  aspect;
  const gchar    *desc;
  gchar          *text;
  gint            xres;
  gint            yres;

  if (param_spec)
    {
      if (! strcmp (param_spec->name, "xresolution"))
        {
          gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_se), 0,
                                          template->xresolution, FALSE);
        }
      else if (! strcmp (param_spec->name, "yresolution"))
        {
          gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_se), 1,
                                          template->yresolution, FALSE);
        }
    }

#ifdef ENABLE_MEMSIZE_LABEL
  text = gimp_memsize_to_string (template->initial_size);
  gtk_label_set_text (GTK_LABEL (editor->memsize_label), text);
  g_free (text);
#endif

  gimp_template_editor_set_pixels (editor, template);

  if (template->width > template->height)
    aspect = GIMP_ASPECT_LANDSCAPE;
  else if (template->height > template->width)
    aspect = GIMP_ASPECT_PORTRAIT;
  else
    aspect = GIMP_ASPECT_SQUARE;

  editor->block_aspect = TRUE;
  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (editor->aspect_button),
                                   aspect);
  editor->block_aspect = FALSE;

  gimp_enum_get_value (GIMP_TYPE_IMAGE_BASE_TYPE, template->image_type,
                       NULL, NULL, &desc, NULL);

  xres = ROUND (template->xresolution);
  yres = ROUND (template->yresolution);

  if (xres != yres)
    text = g_strdup_printf (_("%d × %d ppi, %s"), xres, yres, desc);
  else
    text = g_strdup_printf (_("%d ppi, %s"), yres, desc);

  gtk_label_set_text (GTK_LABEL (editor->more_label), text);
  g_free (text);

  if (editor->stock_id_container)
    {
      GimpObject  *object;
      const gchar *stock_id;

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (template));

      object = gimp_container_get_child_by_name (editor->stock_id_container,
                                                 stock_id);

      gimp_context_set_template (editor->stock_id_context,
                                 (GimpTemplate *) object);
    }
}

static void
gimp_template_editor_icon_changed (GimpContext        *context,
                                   GimpTemplate       *template,
                                   GimpTemplateEditor *editor)
{
  g_object_set (editor->template,
                "stock-id", GIMP_OBJECT (template)->name,
                NULL);
}
