/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcoloreditor.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"

#include "gimpcoloreditor.h"
#include "gimpdocked.h"
#include "gimpsessioninfo.h"

#include "gimp-intl.h"


static void   gimp_color_editor_class_init        (GimpColorEditorClass *klass);
static void   gimp_color_editor_init              (GimpColorEditor      *editor);
static void   gimp_color_editor_docked_iface_init (GimpDockedInterface  *docked_iface);

static void   gimp_color_editor_destroy         (GtkObject         *object);
static void   gimp_color_editor_style_set       (GtkWidget         *widget,
                                                 GtkStyle          *prev_style);

static void   gimp_color_editor_set_aux_info     (GimpDocked       *docked,
                                                  GList            *aux_info);
static GList *gimp_color_editor_get_aux_info     (GimpDocked       *docked);
static void gimp_color_editor_set_docked_context (GimpDocked       *docked,
                                                  GimpContext      *context,
                                                  GimpContext      *prev_context);

static void   gimp_color_editor_destroy         (GtkObject         *object);

static void   gimp_color_editor_fg_changed      (GimpContext       *context,
                                                 const GimpRGB     *rgb,
                                                 GimpColorEditor   *editor);
static void   gimp_color_editor_bg_changed      (GimpContext       *context,
                                                 const GimpRGB     *rgb,
                                                 GimpColorEditor   *editor);
static void   gimp_color_editor_color_changed   (GimpColorSelector *selector,
                                                 const GimpRGB     *rgb,
                                                 const GimpHSV     *hsv,
                                                 GimpColorEditor   *editor);
static void   gimp_color_editor_area_changed    (GimpColorArea     *color_area,
                                                 GimpColorEditor   *editor);
static void   gimp_color_editor_tab_toggled     (GtkWidget         *widget,
                                                 GimpColorEditor   *editor);
static void   gimp_color_editor_fg_bg_toggled   (GtkWidget         *widget,
                                                 GimpColorEditor   *editor);
static void   gimp_color_editor_color_picked    (GtkWidget         *widget,
                                                 const GimpRGB     *rgb,
                                                 GimpColorEditor   *editor);


static GimpEditorClass *parent_class = NULL;


GType
gimp_color_editor_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo editor_info =
      {
        sizeof (GimpColorEditorClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_editor_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorEditor),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_editor_init,
      };
      static const GInterfaceInfo docked_iface_info =
      {
        (GInterfaceInitFunc) gimp_color_editor_docked_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      type = g_type_register_static (GIMP_TYPE_EDITOR,
                                     "GimpColorEditor",
                                     &editor_info, 0);

      g_type_add_interface_static (type, GIMP_TYPE_DOCKED,
                                   &docked_iface_info);
    }

  return type;
}

static void
gimp_color_editor_class_init (GimpColorEditorClass* klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = gimp_color_editor_destroy;

  widget_class->style_set = gimp_color_editor_style_set;
}

static void
gimp_color_editor_init (GimpColorEditor *editor)
{
  GtkWidget   *notebook;
  gint         content_spacing;
  gint         button_spacing;
  GtkIconSize  button_icon_size;
  GimpRGB      rgb;
  GimpHSV      hsv;
  GList       *list;
  GSList      *group;

  editor->context = NULL;
  editor->edit_bg = FALSE;

  gimp_rgba_set (&rgb, 0.0, 0.0, 0.0, 1.0);
  gimp_rgb_to_hsv (&rgb, &hsv);

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "content_spacing",  &content_spacing,
			"button_spacing",   &button_spacing,
                        "button_icon_size", &button_icon_size,
			NULL);

  editor->hbox = gtk_hbox_new (TRUE, button_spacing);
  gtk_box_pack_start (GTK_BOX (editor), editor->hbox, FALSE, FALSE, 0);
  gtk_widget_show (editor->hbox);

  editor->notebook = gimp_color_selector_new (GIMP_TYPE_COLOR_NOTEBOOK,
                                              &rgb, &hsv,
                                              GIMP_COLOR_SELECTOR_HUE);
  gimp_color_selector_set_show_alpha (GIMP_COLOR_SELECTOR (editor->notebook),
                                      FALSE);
  gtk_box_pack_start (GTK_BOX (editor), editor->notebook,
                      TRUE, TRUE, content_spacing);
  gtk_widget_show (editor->notebook);

  g_signal_connect (editor->notebook, "color_changed",
                    G_CALLBACK (gimp_color_editor_color_changed),
                    editor);

  notebook = GIMP_COLOR_NOTEBOOK (editor->notebook)->notebook;

  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

  gimp_color_notebook_set_has_page (GIMP_COLOR_NOTEBOOK (editor->notebook),
                                    GIMP_TYPE_COLOR_SCALES, TRUE);

  group = NULL;

  for (list = GIMP_COLOR_NOTEBOOK (editor->notebook)->selectors;
       list;
       list = g_list_next (list))
    {
      GimpColorSelector      *selector;
      GimpColorSelectorClass *selector_class;
      GtkWidget              *button;
      GtkWidget              *image;

      selector       = GIMP_COLOR_SELECTOR (list->data);
      selector_class = GIMP_COLOR_SELECTOR_GET_CLASS (selector);

      button = gtk_radio_button_new (group);
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_box_pack_start (GTK_BOX (editor->hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      image = gtk_image_new_from_stock (selector_class->stock_id,
                                        GTK_ICON_SIZE_BUTTON);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      gimp_help_set_help_data (button,
			       selector_class->name,
			       selector_class->help_page);

      g_object_set_data (G_OBJECT (button),   "selector", selector);
      g_object_set_data (G_OBJECT (selector), "button",   button);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (gimp_color_editor_tab_toggled),
                        editor);
    }

  /*  The color picker  */
  {
    GtkWidget *button;

    button = gimp_pick_button_new ();
    gtk_box_pack_start (GTK_BOX (editor->hbox), button, TRUE, TRUE, 0);
    gtk_widget_show (button);

    g_signal_connect (button, "color_picked",
                      G_CALLBACK (gimp_color_editor_color_picked),
                      editor);
  }

  /*  FG/BG toggles  */
  {
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *frame;
    gint       i;

    static const gchar *labels[] =
    {
      N_("FG"), N_("BG")
    };
    static const gchar *tips[] =
    {
      N_("Edit Foreground Color"), N_("Edit Background Color")
    };

    hbox = gtk_hbox_new (FALSE, button_spacing);
    gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    vbox = gtk_vbox_new (FALSE, button_spacing);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
    gtk_widget_show (vbox);

    group = NULL;

    for (i = 0; i < G_N_ELEMENTS (labels); i++)
      {
        GtkWidget *button;

        button = gtk_radio_button_new_with_mnemonic (group,
                                                     gettext (labels[i]));
        group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
        gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
        gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
        gtk_widget_show (button);

        gimp_help_set_help_data (button, gettext (tips[i]), NULL);

        g_object_set_data (G_OBJECT (button), "edit_bg",
                           GINT_TO_POINTER (i == 1));

        if ((i == 1) == editor->edit_bg)
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

        g_signal_connect (button, "toggled",
                          G_CALLBACK (gimp_color_editor_fg_bg_toggled),
                          editor);
      }

    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
    gtk_box_pack_end (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
    gtk_widget_show (frame);

    editor->color_area = gimp_color_area_new (&rgb, GIMP_COLOR_AREA_FLAT,
                                              GDK_BUTTON1_MASK |
                                              GDK_BUTTON2_MASK);
    gtk_container_add (GTK_CONTAINER (frame), editor->color_area);
    gtk_widget_show (editor->color_area);

    g_signal_connect (editor->color_area, "color_changed",
                      G_CALLBACK (gimp_color_editor_area_changed),
                      editor);
  }
}

static void
gimp_color_editor_docked_iface_init (GimpDockedInterface *docked_iface)
{
  docked_iface->set_aux_info = gimp_color_editor_set_aux_info;
  docked_iface->get_aux_info = gimp_color_editor_get_aux_info;
  docked_iface->set_context  = gimp_color_editor_set_docked_context;
}

#define AUX_INFO_CURRENT_PAGE "current-page"

static void
gimp_color_editor_set_aux_info (GimpDocked *docked,
                                GList      *aux_info)
{
  GimpColorEditor *editor   = GIMP_COLOR_EDITOR (docked);
  GtkWidget       *notebook = GIMP_COLOR_NOTEBOOK (editor->notebook)->notebook;
  GList           *list;

  for (list = aux_info; list; list = g_list_next (list))
    {
      GimpSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_CURRENT_PAGE))
        {
          GList *children;
          GList *child;

          children = gtk_container_get_children (GTK_CONTAINER (notebook));

          for (child = children; child; child = g_list_next (child))
            {
              if (! strcmp (G_OBJECT_TYPE_NAME (child->data), aux->value))
                {
                  GtkWidget *button;

                  button = g_object_get_data (G_OBJECT (child->data), "button");

                  if (button)
                    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                                  TRUE);

                  break;
                }
            }

          g_list_free (children);
        }
    }
}

static GList *
gimp_color_editor_get_aux_info (GimpDocked *docked)
{
  GimpColorEditor    *editor   = GIMP_COLOR_EDITOR (docked);
  GimpColorNotebook  *notebook = GIMP_COLOR_NOTEBOOK (editor->notebook);
  GList              *aux_info = NULL;

  if (notebook->cur_page)
    {
      GimpSessionInfoAux *aux;

      aux = gimp_session_info_aux_new (AUX_INFO_CURRENT_PAGE,
                                       G_OBJECT_TYPE_NAME (notebook->cur_page));
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
gimp_color_editor_set_docked_context (GimpDocked  *docked,
                                      GimpContext *context,
                                      GimpContext *prev_context)
{
  gimp_color_editor_set_context (GIMP_COLOR_EDITOR (docked), context);
}

static void
gimp_color_editor_destroy (GtkObject *object)
{
  GimpColorEditor *editor = GIMP_COLOR_EDITOR (object);

  if (editor->context)
    gimp_color_editor_set_context (editor, NULL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


/*  public functions  */

GtkWidget *
gimp_color_editor_new (GimpContext *context)
{
  GimpColorEditor *editor;

  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);

  editor = g_object_new (GIMP_TYPE_COLOR_EDITOR, NULL);

  if (context)
    gimp_color_editor_set_context (editor, context);

  return GTK_WIDGET (editor);
}

static void
gimp_color_editor_style_set (GtkWidget *widget,
                             GtkStyle  *prev_style)
{
  GimpColorEditor *editor;
  GtkIconSize      button_icon_size;
  gint             button_spacing;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  editor = GIMP_COLOR_EDITOR (widget);

  gtk_widget_style_get (widget,
                        "button_icon_size", &button_icon_size,
			"button_spacing",   &button_spacing,
			NULL);

  if (editor->hbox)
    {
      GList  *children;
      GList  *list;

      gtk_box_set_spacing (GTK_BOX (editor->hbox), button_spacing);

      children = gtk_container_get_children (GTK_CONTAINER (editor->hbox));

      for (list = children; list; list = g_list_next (list))
        {
          GtkBin *bin;
          gchar  *stock_id;

          bin = GTK_BIN (list->data);

          gtk_image_get_stock (GTK_IMAGE (bin->child), &stock_id, NULL);
          gtk_image_set_from_stock (GTK_IMAGE (bin->child),
                                    stock_id,
                                    button_icon_size);
        }

      g_list_free (children);
    }
}

void
gimp_color_editor_set_context (GimpColorEditor *editor,
                               GimpContext     *context)
{
  g_return_if_fail (GIMP_IS_COLOR_EDITOR (editor));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (context == editor->context)
    return;

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
					    gimp_color_editor_fg_changed,
					    editor);
      g_signal_handlers_disconnect_by_func (editor->context,
					    gimp_color_editor_bg_changed,
					    editor);

      g_object_unref (editor->context);
      editor->context = NULL;
    }

  if (context)
    {
      GimpRGB rgb;

      editor->context = g_object_ref (context);

      g_signal_connect (editor->context, "foreground_changed",
			G_CALLBACK (gimp_color_editor_fg_changed),
			editor);
      g_signal_connect (editor->context, "background_changed",
			G_CALLBACK (gimp_color_editor_bg_changed),
			editor);

      if (editor->edit_bg)
        {
          gimp_context_get_background (editor->context, &rgb);
          gimp_color_editor_bg_changed (editor->context, &rgb, editor);
        }
      else
        {
          gimp_context_get_foreground (editor->context, &rgb);
          gimp_color_editor_fg_changed (editor->context, &rgb, editor);
        }
    }
}


/*  public functions  */

static void
gimp_color_editor_fg_changed (GimpContext     *context,
                              const GimpRGB   *rgb,
                              GimpColorEditor *editor)
{
  if (! editor->edit_bg)
    {
      GimpHSV hsv;

      gimp_rgb_to_hsv (rgb, &hsv);

      g_signal_handlers_block_by_func (editor->notebook,
                                       gimp_color_editor_color_changed,
                                       editor);
      g_signal_handlers_block_by_func (editor->color_area,
                                       gimp_color_editor_area_changed,
                                       editor);

      gimp_color_selector_set_color (GIMP_COLOR_SELECTOR (editor->notebook),
                                     rgb, &hsv);
      gimp_color_area_set_color (GIMP_COLOR_AREA (editor->color_area), rgb);

      g_signal_handlers_unblock_by_func (editor->notebook,
                                         gimp_color_editor_color_changed,
                                         editor);
      g_signal_handlers_unblock_by_func (editor->color_area,
                                         gimp_color_editor_area_changed,
                                         editor);
    }
}

static void
gimp_color_editor_bg_changed (GimpContext     *context,
                              const GimpRGB   *rgb,
                              GimpColorEditor *editor)
{
  if (editor->edit_bg)
    {
      GimpHSV hsv;

      gimp_rgb_to_hsv (rgb, &hsv);

      g_signal_handlers_block_by_func (editor->notebook,
                                       gimp_color_editor_color_changed,
                                       editor);
      g_signal_handlers_block_by_func (editor->color_area,
                                       gimp_color_editor_area_changed,
                                       editor);

      gimp_color_selector_set_color (GIMP_COLOR_SELECTOR (editor->notebook),
                                     rgb, &hsv);
      gimp_color_area_set_color (GIMP_COLOR_AREA (editor->color_area), rgb);

      g_signal_handlers_unblock_by_func (editor->notebook,
                                         gimp_color_editor_color_changed,
                                         editor);
      g_signal_handlers_unblock_by_func (editor->color_area,
                                         gimp_color_editor_area_changed,
                                         editor);
    }
}

static void
gimp_color_editor_color_changed (GimpColorSelector *selector,
                                 const GimpRGB     *rgb,
                                 const GimpHSV     *hsv,
                                 GimpColorEditor   *editor)
{
  if (editor->context)
    {
      if (editor->edit_bg)
        {
          g_signal_handlers_block_by_func (editor->context,
                                           gimp_color_editor_bg_changed,
                                           editor);

          gimp_context_set_background (editor->context, rgb);

          g_signal_handlers_unblock_by_func (editor->context,
                                             gimp_color_editor_bg_changed,
                                             editor);
        }
      else
        {
          g_signal_handlers_block_by_func (editor->context,
                                           gimp_color_editor_fg_changed,
                                           editor);

          gimp_context_set_foreground (editor->context, rgb);

          g_signal_handlers_unblock_by_func (editor->context,
                                             gimp_color_editor_fg_changed,
                                             editor);
        }
    }

  g_signal_handlers_block_by_func (editor->color_area,
                                   gimp_color_editor_area_changed,
                                   editor);

  gimp_color_area_set_color (GIMP_COLOR_AREA (editor->color_area), rgb);

  g_signal_handlers_unblock_by_func (editor->color_area,
                                     gimp_color_editor_area_changed,
                                     editor);
}

static void
gimp_color_editor_area_changed (GimpColorArea   *color_area,
                                GimpColorEditor *editor)
{
  GimpRGB rgb;
  GimpHSV hsv;

  gimp_color_area_get_color (color_area, &rgb);
  gimp_rgb_to_hsv (&rgb, &hsv);

  gimp_color_selector_set_color (GIMP_COLOR_SELECTOR (editor->notebook),
                                 &rgb, &hsv);
}

static void
gimp_color_editor_tab_toggled (GtkWidget       *widget,
                               GimpColorEditor *editor)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      GtkWidget *selector;

      selector = g_object_get_data (G_OBJECT (widget), "selector");

      if (selector)
        {
          GtkWidget *notebook;
          gint       page_num;

          notebook = GIMP_COLOR_NOTEBOOK (editor->notebook)->notebook;

          page_num = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), selector);

          if (page_num >= 0)
            gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page_num);
        }
    }
}

static void
gimp_color_editor_fg_bg_toggled (GtkWidget       *widget,
                                 GimpColorEditor *editor)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      gboolean edit_bg;

      edit_bg = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                    "edit_bg"));

      if (edit_bg != editor->edit_bg)
        {
          editor->edit_bg = edit_bg;

          if (editor->context)
            {
              GimpRGB rgb;

              if (edit_bg)
                {
                  gimp_context_get_background (editor->context, &rgb);
                  gimp_color_editor_bg_changed (editor->context, &rgb, editor);
                }
              else
                {
                  gimp_context_get_foreground (editor->context, &rgb);
                  gimp_color_editor_fg_changed (editor->context, &rgb, editor);
                }
            }
        }
    }
}

static void
gimp_color_editor_color_picked (GtkWidget       *widget,
                                const GimpRGB   *rgb,
                                GimpColorEditor *editor)
{
  gimp_color_area_set_color (GIMP_COLOR_AREA (editor->color_area), rgb);
}
