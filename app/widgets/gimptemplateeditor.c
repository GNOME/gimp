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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpconfig-utils.h"
#include "config/gimpconfig.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimpcontext.h"
#include "core/gimptemplate.h"

#include "gimptemplateeditor.h"
#include "gimpenummenu.h"
#include "gimppropwidgets.h"
#include "gimpviewablebutton.h"

#include "gimp-intl.h"


#define SB_WIDTH 10


static void gimp_template_editor_class_init (GimpTemplateEditorClass *klass);
static void gimp_template_editor_init       (GimpTemplateEditor      *editor);

static void gimp_template_editor_finalize        (GObject            *object);

static void gimp_template_editor_aspect_callback (GtkWidget          *widget,
                                                  GimpTemplateEditor *editor);
static void gimp_template_editor_template_notify (GimpTemplate       *template,
                                                  GParamSpec         *param_spec,
                                                  GimpTemplateEditor *editor);
static void gimp_template_editor_icon_changed    (GimpContext        *context,
                                                  GimpTemplate       *template,
                                                  GimpTemplateEditor *editor);


static GimpEditorClass *parent_class = NULL;


GType
gimp_template_editor_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpTemplateEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_template_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpTemplateEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_template_editor_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_EDITOR,
                                          "GimpTemplateEditor",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_template_editor_class_init (GimpTemplateEditorClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_template_editor_finalize;
}

static void
gimp_template_editor_init (GimpTemplateEditor *editor)
{
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *separator;
  GtkWidget *label;
  GtkObject *adjustment;
  GtkWidget *spinbutton;
  GtkWidget *spinbutton2;
  GtkWidget *chainbutton;

  editor->template = gimp_template_new ("GimpTemplateEditor");
  editor->memsize  = 0;

  /*  Image size frame  */
  frame = gtk_frame_new (_("Image Size"));
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (7, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 4, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  the pixel size labels  */
  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  a separator after the pixel section  */
  separator = gtk_hseparator_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), separator, 0, 2, 2, 3);
  gtk_widget_show (separator);

  /*  the unit size labels  */
  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  create the sizeentry which keeps it all together  */
  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 2, 3, 5);
  gtk_widget_show (abox);

  editor->size_se = gimp_size_entry_new (0, editor->template->unit, "%a",
                                         FALSE, FALSE, TRUE, SB_WIDTH,
                                         GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (editor->size_se), 1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (editor->size_se), 1, 2);
  gtk_container_add (GTK_CONTAINER (abox), editor->size_se);
  gtk_widget_show (editor->size_se);

  /*  height in units  */
  spinbutton = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 0,
                                     1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);
  /*  add the "height in units" spinbutton to the sizeentry  */
  gtk_table_attach_defaults (GTK_TABLE (editor->size_se), spinbutton,
			     0, 1, 2, 3);
  gtk_widget_show (spinbutton);

  /*  height in pixels  */
  hbox = gtk_hbox_new (FALSE, 4);

  spinbutton2 = gimp_spin_button_new (&adjustment,
                                      1, 1, 1, 1, 10, 0,
                                      1, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton2), SB_WIDTH);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton2, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton2);

  label = gtk_label_new (_("Pixels"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  editor->memsize_label = gtk_label_new (NULL);
  gtk_box_pack_end (GTK_BOX (hbox), editor->memsize_label, FALSE, FALSE, 0);
  gtk_widget_show (editor->memsize_label);

  /*  add the "height in pixels" spinbutton to the main table  */
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 1, 2);
  gtk_widget_show (hbox);

  /*  register the height spinbuttons with the sizeentry  */
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (editor->size_se),
                             GTK_SPIN_BUTTON (spinbutton),
			     GTK_SPIN_BUTTON (spinbutton2));

  /*  width in units  */
  spinbutton = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 0,
                                     1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);
  /*  add the "width in units" spinbutton to the sizeentry  */
  gtk_table_attach_defaults (GTK_TABLE (editor->size_se), spinbutton,
			     0, 1, 1, 2);
  gtk_widget_show (spinbutton);

  /*  width in pixels  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 0, 1);
  gtk_widget_show (hbox);

  spinbutton2 = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 0,
                                     1, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton2), SB_WIDTH);
  /*  add the "width in pixels" spinbutton to the main table  */
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton2, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton2);

  hbox2 = gimp_enum_stock_box_new (GIMP_TYPE_ASPECT_TYPE,
                                   "gimp", GTK_ICON_SIZE_MENU,
                                   G_CALLBACK (gimp_template_editor_aspect_callback),
                                   editor,
                                   &editor->aspect_button);
  gtk_widget_hide (editor->aspect_button); /* hide "square" */
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  /*  register the width spinbuttons with the sizeentry  */
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (editor->size_se),
                             GTK_SPIN_BUTTON (spinbutton),
			     GTK_SPIN_BUTTON (spinbutton2));

  /*  initialize the sizeentry  */
  gimp_prop_size_entry_connect (G_OBJECT (editor->template),
                                "width", "height", "unit",
                                editor->size_se, NULL,
                                editor->template->xresolution,
                                editor->template->yresolution);

  /*  the resolution labels  */
  label = gtk_label_new (_("Resolution X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 6, 7,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the resolution sizeentry  */
  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 2, 5, 7);
  gtk_widget_show (abox);

  spinbutton = gimp_spin_button_new (&adjustment,
                                     1, 1, 1, 1, 10, 0,
                                     1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);

  editor->resolution_se = gimp_size_entry_new (1, editor->template->resolution_unit,
                                             _("pixels/%a"),
                                             FALSE, FALSE, FALSE, SB_WIDTH,
                                             GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  gtk_table_set_col_spacing (GTK_TABLE (editor->resolution_se), 1, 2);
  gtk_table_set_col_spacing (GTK_TABLE (editor->resolution_se), 2, 2);
  gtk_table_set_row_spacing (GTK_TABLE (editor->resolution_se), 0, 2);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (editor->resolution_se),
			     GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (editor->resolution_se), spinbutton,
			     1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gtk_container_add (GTK_CONTAINER (abox), editor->resolution_se);
  gtk_widget_show (editor->resolution_se);

  /*  the resolution chainbutton  */
  chainbutton = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gtk_table_attach_defaults (GTK_TABLE (editor->resolution_se), chainbutton,
                             2, 3, 0, 2);
  gtk_widget_show (chainbutton);

  gimp_prop_size_entry_connect (G_OBJECT (editor->template),
                                "xresolution", "yresolution",
                                "resolution-unit",
                                editor->resolution_se, chainbutton,
                                1.0, 1.0);

  /*  hbox containing the Image type and fill type frames  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  frame for Image Type  */
  frame = gimp_prop_enum_radio_frame_new (G_OBJECT (editor->template),
                                          "image-type",
                                          _("Image Type"),
                                          GIMP_RGB, GIMP_GRAY);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* frame for Fill Type */
  frame = gimp_prop_enum_radio_frame_new (G_OBJECT (editor->template),
                                          "fill-type",
                                          _("Fill Type"),
                                          -1, -1);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);
}

static void
gimp_template_editor_finalize (GObject *object)
{
  GimpTemplateEditor *editor;

  editor = GIMP_TEMPLATE_EDITOR (object);

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

GtkWidget *
gimp_template_editor_new (Gimp     *gimp,
                          gboolean  edit_stock_id)
{
  GimpTemplateEditor *editor;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  editor = g_object_new (GIMP_TYPE_TEMPLATE_EDITOR, NULL);

  if (edit_stock_id)
    {
      GtkWidget *table;
      GtkWidget *button;
      GSList    *stock_list;
      GSList    *list;

      editor->stock_id_container = gimp_list_new (GIMP_TYPE_TEMPLATE,
                                                  GIMP_CONTAINER_POLICY_STRONG);
      editor->stock_id_context = gimp_context_new (gimp, "foo", NULL);

      g_signal_connect (editor->stock_id_context, "template_changed",
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
        }

      g_slist_foreach (stock_list, (GFunc) g_free, NULL);
      g_slist_free (stock_list);

      table = gtk_table_new (1, 2, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_box_pack_start (GTK_BOX (editor), table, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (editor), table, 0);
      gtk_widget_show (table);

      button = gimp_viewable_button_new (editor->stock_id_container,
                                         editor->stock_id_context,
                                         GIMP_PREVIEW_SIZE_SMALL,
                                         NULL, NULL, NULL, NULL);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                                 _("_Icon:"), 1.0, 0.5,
                                 button, 1, TRUE);
    }

  return GTK_WIDGET (editor);
}

void
gimp_template_editor_set_template (GimpTemplateEditor *editor,
                                   GimpTemplate       *template)
{
  g_return_if_fail (GIMP_IS_TEMPLATE_EDITOR (editor));
  g_return_if_fail (GIMP_IS_TEMPLATE (template));

  g_signal_handlers_disconnect_by_func (editor->template,
                                        gimp_template_editor_template_notify,
                                        editor);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_se), 0,
                                  template->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_se), 1,
                                  template->yresolution, FALSE);

  gimp_config_copy_properties (G_OBJECT (template),
                               G_OBJECT (editor->template));

  g_signal_connect (editor->template, "notify",
                    G_CALLBACK (gimp_template_editor_template_notify),
                    editor);
  gimp_template_editor_template_notify (editor->template, NULL, editor);
}

GimpTemplate *
gimp_template_editor_get_template (GimpTemplateEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE_EDITOR (editor), NULL);

  return GIMP_TEMPLATE (gimp_config_duplicate (G_OBJECT (editor->template)));
}


/*  private functions  */

static void
gimp_template_editor_aspect_callback (GtkWidget          *widget,
                                      GimpTemplateEditor *editor)
{
  if (! editor->block_aspect && GTK_TOGGLE_BUTTON (widget)->active)
    {
      gint    width;
      gint    height;
      gdouble xresolution;
      gdouble yresolution;

      width       = editor->template->width;
      height      = editor->template->height;
      xresolution = editor->template->xresolution;
      yresolution = editor->template->yresolution;

      if (editor->template->width == editor->template->height)
        {
          editor->block_aspect = TRUE;
          gimp_radio_group_set_active (GTK_RADIO_BUTTON (editor->aspect_button),
                                       GINT_TO_POINTER (GIMP_ASPECT_SQUARE));
          editor->block_aspect = FALSE;
          return;
       }

      g_signal_handlers_block_by_func (editor->template,
                                       gimp_template_editor_template_notify,
                                       editor);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_se), 0,
                                      yresolution, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (editor->size_se), 1,
                                      xresolution, FALSE);

      g_object_set (editor->template,
                    "width",       height,
                    "height",      width,
                    "xresolution", yresolution,
                    "yresolution", xresolution,
                    NULL);

      g_signal_handlers_unblock_by_func (editor->template,
                                         gimp_template_editor_template_notify,
                                         editor);
    }
}

static void
gimp_template_editor_template_notify (GimpTemplate       *template,
                                      GParamSpec         *param_spec,
                                      GimpTemplateEditor *editor)
{
  GimpAspectType aspect;

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

  if (editor->memsize != template->initial_size)
    {
      gchar *text;

      editor->memsize = template->initial_size;

      if (template->initial_size_too_large)
        text = g_strdup (_("Too large!"));
      else
        text = gimp_memsize_to_string (editor->memsize);

      gtk_label_set_text (GTK_LABEL (editor->memsize_label), text);
      g_free (text);
    }

  if (editor->template->width > editor->template->height)
    aspect = GIMP_ASPECT_LANDSCAPE;
  else if (editor->template->height > editor->template->width)
    aspect = GIMP_ASPECT_PORTRAIT;
  else
    aspect = GIMP_ASPECT_SQUARE;

  editor->block_aspect = TRUE;
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (editor->aspect_button),
                               GINT_TO_POINTER (aspect));
  editor->block_aspect = FALSE;

  if (editor->stock_id_container)
    {
      GimpObject  *object;
      const gchar *stock_id;

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (editor->template));

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

