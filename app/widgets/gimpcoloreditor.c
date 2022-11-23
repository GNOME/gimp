/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacoloreditor.c
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "ligmacoloreditor.h"
#include "ligmacolorhistory.h"
#include "ligmadocked.h"
#include "ligmafgbgeditor.h"
#include "ligmafgbgview.h"
#include "ligmasessioninfo-aux.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT
};


static void   ligma_color_editor_docked_iface_init (LigmaDockedInterface  *iface);

static void   ligma_color_editor_constructed     (GObject           *object);
static void   ligma_color_editor_dispose         (GObject           *object);
static void   ligma_color_editor_set_property    (GObject           *object,
                                                 guint              property_id,
                                                 const GValue      *value,
                                                 GParamSpec        *pspec);
static void   ligma_color_editor_get_property    (GObject           *object,
                                                 guint              property_id,
                                                 GValue            *value,
                                                 GParamSpec        *pspec);

static void   ligma_color_editor_style_updated   (GtkWidget         *widget);

static void   ligma_color_editor_set_aux_info    (LigmaDocked        *docked,
                                                 GList             *aux_info);
static GList *ligma_color_editor_get_aux_info     (LigmaDocked       *docked);
static GtkWidget *ligma_color_editor_get_preview (LigmaDocked        *docked,
                                                 LigmaContext       *context,
                                                 GtkIconSize        size);
static void   ligma_color_editor_set_context     (LigmaDocked        *docked,
                                                 LigmaContext       *context);

static void   ligma_color_editor_fg_changed      (LigmaContext       *context,
                                                 const LigmaRGB     *rgb,
                                                 LigmaColorEditor   *editor);
static void   ligma_color_editor_bg_changed      (LigmaContext       *context,
                                                 const LigmaRGB     *rgb,
                                                 LigmaColorEditor   *editor);
static void   ligma_color_editor_color_changed   (LigmaColorSelector *selector,
                                                 const LigmaRGB     *rgb,
                                                 const LigmaHSV     *hsv,
                                                 LigmaColorEditor   *editor);
static void   ligma_color_editor_tab_toggled     (GtkWidget         *widget,
                                                 LigmaColorEditor   *editor);
static void   ligma_color_editor_fg_bg_notify    (GtkWidget         *widget,
                                                 GParamSpec        *pspec,
                                                 LigmaColorEditor   *editor);
static void   ligma_color_editor_color_picked    (GtkWidget         *widget,
                                                 const LigmaRGB     *rgb,
                                                 LigmaColorEditor   *editor);
static void   ligma_color_editor_entry_changed   (LigmaColorHexEntry *entry,
                                                 LigmaColorEditor   *editor);

static void  ligma_color_editor_history_selected (LigmaColorHistory *history,
                                                 const LigmaRGB    *rgb,
                                                 LigmaColorEditor  *editor);

G_DEFINE_TYPE_WITH_CODE (LigmaColorEditor, ligma_color_editor, LIGMA_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_color_editor_docked_iface_init))

#define parent_class ligma_color_editor_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_color_editor_class_init (LigmaColorEditorClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = ligma_color_editor_constructed;
  object_class->dispose       = ligma_color_editor_dispose;
  object_class->set_property  = ligma_color_editor_set_property;
  object_class->get_property  = ligma_color_editor_get_property;

  widget_class->style_updated = ligma_color_editor_style_updated;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        G_PARAM_CONSTRUCT |
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_color_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->get_preview  = ligma_color_editor_get_preview;
  iface->set_aux_info = ligma_color_editor_set_aux_info;
  iface->get_aux_info = ligma_color_editor_get_aux_info;
  iface->set_context  = ligma_color_editor_set_context;
}

static void
ligma_color_editor_init (LigmaColorEditor *editor)
{
  GtkWidget   *notebook;
  GtkWidget   *hbox;
  GtkWidget   *button;
  gint         content_spacing;
  gint         button_spacing;
  GtkIconSize  button_icon_size;
  LigmaRGB      rgb;
  LigmaHSV      hsv;
  GList       *list;
  GSList      *group;
  gint         icon_width  = 40;
  gint         icon_height = 38;

  editor->context = NULL;
  editor->edit_bg = FALSE;

  ligma_rgba_set (&rgb, 0.0, 0.0, 0.0, 1.0);
  ligma_rgb_to_hsv (&rgb, &hsv);

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "content-spacing",  &content_spacing,
                        "button-spacing",   &button_spacing,
                        "button-icon-size", &button_icon_size,
                        NULL);

  editor->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);
  gtk_box_set_homogeneous (GTK_BOX (editor->hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (editor), editor->hbox, FALSE, FALSE, 0);
  gtk_widget_show (editor->hbox);

  editor->notebook = ligma_color_selector_new (LIGMA_TYPE_COLOR_NOTEBOOK,
                                              &rgb, &hsv,
                                              LIGMA_COLOR_SELECTOR_RED);
  ligma_color_selector_set_show_alpha (LIGMA_COLOR_SELECTOR (editor->notebook),
                                      FALSE);
  gtk_box_pack_start (GTK_BOX (editor), editor->notebook,
                      TRUE, TRUE, content_spacing);
  gtk_widget_show (editor->notebook);

  g_signal_connect (editor->notebook, "color-changed",
                    G_CALLBACK (ligma_color_editor_color_changed),
                    editor);

  notebook = ligma_color_notebook_get_notebook (LIGMA_COLOR_NOTEBOOK (editor->notebook));

  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

  ligma_color_notebook_set_has_page (LIGMA_COLOR_NOTEBOOK (editor->notebook),
                                    LIGMA_TYPE_COLOR_SCALES, TRUE);

  group = NULL;

  for (list = ligma_color_notebook_get_selectors (LIGMA_COLOR_NOTEBOOK (editor->notebook));
       list;
       list = g_list_next (list))
    {
      LigmaColorSelector      *selector;
      LigmaColorSelectorClass *selector_class;
      GtkWidget              *button;
      GtkWidget              *image;

      selector       = LIGMA_COLOR_SELECTOR (list->data);
      selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (selector);

      button = gtk_radio_button_new (group);
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_box_pack_start (GTK_BOX (editor->hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      image = gtk_image_new_from_icon_name (selector_class->icon_name,
                                            GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      ligma_help_set_help_data (button,
                               selector_class->name, selector_class->help_id);

      g_object_set_data (G_OBJECT (button),   "selector", selector);
      g_object_set_data (G_OBJECT (selector), "button",   button);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (ligma_color_editor_tab_toggled),
                        editor);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  FG/BG editor  */
  editor->fg_bg = ligma_fg_bg_editor_new (NULL);
  gtk_icon_size_lookup (button_icon_size, &icon_width, &icon_height);
  gtk_widget_set_size_request (editor->fg_bg,
                               (gint) (icon_width * 1.75),
                               (gint) (icon_height * 1.75));
  gtk_box_pack_start (GTK_BOX (hbox), editor->fg_bg, FALSE, FALSE, 0);
  gtk_widget_show (editor->fg_bg);

  g_signal_connect (editor->fg_bg, "notify::active-color",
                    G_CALLBACK (ligma_color_editor_fg_bg_notify),
                    editor);

  /*  The color picker  */
  button = ligma_pick_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "color-picked",
                    G_CALLBACK (ligma_color_editor_color_picked),
                    editor);

  /*  The hex triplet entry  */
  editor->hex_entry = ligma_color_hex_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), editor->hex_entry, TRUE, TRUE, 0);
  gtk_widget_show (editor->hex_entry);

  g_signal_connect (editor->hex_entry, "color-changed",
                    G_CALLBACK (ligma_color_editor_entry_changed),
                    editor);
}

static void
ligma_color_editor_constructed (GObject *object)
{
  LigmaColorEditor *editor = LIGMA_COLOR_EDITOR (object);
  GtkWidget       *history;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* The color history */
  history = ligma_color_history_new (editor->context, 12);
  gtk_box_pack_end (GTK_BOX (editor), history, FALSE, FALSE, 0);
  gtk_widget_show (history);

  g_signal_connect (history, "color-selected",
                    G_CALLBACK (ligma_color_editor_history_selected),
                    editor);
}

static void
ligma_color_editor_dispose (GObject *object)
{
  LigmaColorEditor *editor = LIGMA_COLOR_EDITOR (object);

  if (editor->context)
    ligma_docked_set_context (LIGMA_DOCKED (editor), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_color_editor_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_CONTEXT:
      ligma_docked_set_context (LIGMA_DOCKED (object),
                               g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_editor_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaColorEditor *editor = LIGMA_COLOR_EDITOR (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, editor->context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
ligma_color_editor_get_preview (LigmaDocked  *docked,
                               LigmaContext *context,
                               GtkIconSize  size)
{
  GtkWidget *preview;
  gint       width;
  gint       height;

  preview = ligma_fg_bg_view_new (context);

  if (gtk_icon_size_lookup (size, &width, &height))
    gtk_widget_set_size_request (preview, width, height);

  return preview;
}

#define AUX_INFO_CURRENT_PAGE "current-page"

static void
ligma_color_editor_set_aux_info (LigmaDocked *docked,
                                GList      *aux_info)
{
  LigmaColorEditor *editor   = LIGMA_COLOR_EDITOR (docked);
  GtkWidget       *notebook;
  GList           *list;

  notebook = ligma_color_notebook_get_notebook (LIGMA_COLOR_NOTEBOOK (editor->notebook));

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      LigmaSessionInfoAux *aux = list->data;

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
ligma_color_editor_get_aux_info (LigmaDocked *docked)
{
  LigmaColorEditor    *editor   = LIGMA_COLOR_EDITOR (docked);
  LigmaColorNotebook  *notebook = LIGMA_COLOR_NOTEBOOK (editor->notebook);
  LigmaColorSelector  *current;
  GList              *aux_info;

  aux_info = parent_docked_iface->get_aux_info (docked);

  current = ligma_color_notebook_get_current_selector (notebook);

  if (current)
    {
      LigmaSessionInfoAux *aux;

      aux = ligma_session_info_aux_new (AUX_INFO_CURRENT_PAGE,
                                       G_OBJECT_TYPE_NAME (current));
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
ligma_color_editor_set_context (LigmaDocked  *docked,
                               LigmaContext *context)
{
  LigmaColorEditor *editor = LIGMA_COLOR_EDITOR (docked);

  if (context == editor->context)
    return;

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            ligma_color_editor_fg_changed,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->context,
                                            ligma_color_editor_bg_changed,
                                            editor);

      g_signal_handlers_disconnect_by_func (editor->context->ligma->config,
                                            G_CALLBACK (ligma_color_editor_style_updated),
                                            editor);

      g_object_unref (editor->context);
    }

  editor->context = context;

  if (editor->context)
    {
      LigmaRGB rgb;

      g_object_ref (editor->context);

      g_signal_connect (editor->context, "foreground-changed",
                        G_CALLBACK (ligma_color_editor_fg_changed),
                        editor);
      g_signal_connect (editor->context, "background-changed",
                        G_CALLBACK (ligma_color_editor_bg_changed),
                        editor);

      g_signal_connect_object (editor->context->ligma->config,
                               "notify::theme",
                               G_CALLBACK (ligma_color_editor_style_updated),
                               editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->ligma->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (ligma_color_editor_style_updated),
                               editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->ligma->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (ligma_color_editor_style_updated),
                               editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

      if (editor->edit_bg)
        {
          ligma_context_get_background (editor->context, &rgb);
          ligma_color_editor_bg_changed (editor->context, &rgb, editor);
        }
      else
        {
          ligma_context_get_foreground (editor->context, &rgb);
          ligma_color_editor_fg_changed (editor->context, &rgb, editor);
        }

      g_object_set_data (G_OBJECT (context->ligma->config->color_management),
                         "ligma-context", editor->context);

      ligma_color_selector_set_config (LIGMA_COLOR_SELECTOR (editor->notebook),
                                      context->ligma->config->color_management);

      g_object_set_data (G_OBJECT (context->ligma->config->color_management),
                         "ligma-context", NULL);
    }

  ligma_fg_bg_editor_set_context (LIGMA_FG_BG_EDITOR (editor->fg_bg), context);
}

GtkWidget *
ligma_color_editor_new (LigmaContext *context)
{
  return g_object_new (LIGMA_TYPE_COLOR_EDITOR,
                       "context", context,
                       NULL);
}

static void
ligma_color_editor_style_updated (GtkWidget *widget)
{
  LigmaColorEditor *editor = LIGMA_COLOR_EDITOR (widget);
  GtkIconSize      button_icon_size;
  gint             icon_width  = 40;
  gint             icon_height = 38;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (editor->hbox)
    ligma_editor_set_box_style (LIGMA_EDITOR (editor), GTK_BOX (editor->hbox));

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "button-icon-size", &button_icon_size,
                        NULL);
  gtk_icon_size_lookup (button_icon_size, &icon_width, &icon_height);
  gtk_widget_set_size_request (editor->fg_bg,
                               (gint) (icon_width * 1.75),
                               (gint) (icon_height * 1.75));
}


static void
ligma_color_editor_set_color (LigmaColorEditor *editor,
                             const LigmaRGB   *rgb)
{
  LigmaHSV hsv;

  ligma_rgb_to_hsv (rgb, &hsv);

  g_signal_handlers_block_by_func (editor->notebook,
                                   ligma_color_editor_color_changed,
                                   editor);

  ligma_color_selector_set_color (LIGMA_COLOR_SELECTOR (editor->notebook),
                                 rgb, &hsv);

  g_signal_handlers_unblock_by_func (editor->notebook,
                                     ligma_color_editor_color_changed,
                                     editor);

  g_signal_handlers_block_by_func (editor->hex_entry,
                                   ligma_color_editor_entry_changed,
                                   editor);

  ligma_color_hex_entry_set_color (LIGMA_COLOR_HEX_ENTRY (editor->hex_entry),
                                  rgb);

  g_signal_handlers_unblock_by_func (editor->hex_entry,
                                     ligma_color_editor_entry_changed,
                                     editor);
}

static void
ligma_color_editor_fg_changed (LigmaContext     *context,
                              const LigmaRGB   *rgb,
                              LigmaColorEditor *editor)
{
  if (! editor->edit_bg)
    ligma_color_editor_set_color (editor, rgb);
}

static void
ligma_color_editor_bg_changed (LigmaContext     *context,
                              const LigmaRGB   *rgb,
                              LigmaColorEditor *editor)
{
  if (editor->edit_bg)
    ligma_color_editor_set_color (editor, rgb);
}

static void
ligma_color_editor_color_changed (LigmaColorSelector *selector,
                                 const LigmaRGB     *rgb,
                                 const LigmaHSV     *hsv,
                                 LigmaColorEditor   *editor)
{
  if (editor->context)
    {
      if (editor->edit_bg)
        {
          g_signal_handlers_block_by_func (editor->context,
                                           ligma_color_editor_bg_changed,
                                           editor);

          ligma_context_set_background (editor->context, rgb);

          g_signal_handlers_unblock_by_func (editor->context,
                                             ligma_color_editor_bg_changed,
                                             editor);
        }
      else
        {
          g_signal_handlers_block_by_func (editor->context,
                                           ligma_color_editor_fg_changed,
                                           editor);

          ligma_context_set_foreground (editor->context, rgb);

          g_signal_handlers_unblock_by_func (editor->context,
                                             ligma_color_editor_fg_changed,
                                             editor);
        }
    }

  g_signal_handlers_block_by_func (editor->hex_entry,
                                   ligma_color_editor_entry_changed,
                                   editor);

  ligma_color_hex_entry_set_color (LIGMA_COLOR_HEX_ENTRY (editor->hex_entry),
                                  rgb);

  g_signal_handlers_unblock_by_func (editor->hex_entry,
                                     ligma_color_editor_entry_changed,
                                     editor);
}

static void
ligma_color_editor_tab_toggled (GtkWidget       *widget,
                               LigmaColorEditor *editor)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GtkWidget *selector;

      selector = g_object_get_data (G_OBJECT (widget), "selector");

      if (selector)
        {
          GtkWidget *notebook;
          gint       page_num;

          notebook = ligma_color_notebook_get_notebook (LIGMA_COLOR_NOTEBOOK (editor->notebook));

          page_num = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), selector);

          if (page_num >= 0)
            gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page_num);
        }
    }
}

static void
ligma_color_editor_fg_bg_notify (GtkWidget       *widget,
                                GParamSpec      *pspec,
                                LigmaColorEditor *editor)
{
  gboolean edit_bg;

  edit_bg = (LIGMA_FG_BG_EDITOR (widget)->active_color ==
             LIGMA_ACTIVE_COLOR_BACKGROUND);

  if (edit_bg != editor->edit_bg)
    {
      editor->edit_bg = edit_bg;

      if (editor->context)
        {
          LigmaRGB rgb;

          if (edit_bg)
            {
              ligma_context_get_background (editor->context, &rgb);
              ligma_color_editor_bg_changed (editor->context, &rgb, editor);
            }
          else
            {
              ligma_context_get_foreground (editor->context, &rgb);
              ligma_color_editor_fg_changed (editor->context, &rgb, editor);
            }
        }
    }
}

static void
ligma_color_editor_color_picked (GtkWidget       *widget,
                                const LigmaRGB   *rgb,
                                LigmaColorEditor *editor)
{
  if (editor->context)
    {
      if (editor->edit_bg)
        ligma_context_set_background (editor->context, rgb);
      else
        ligma_context_set_foreground (editor->context, rgb);
    }
}

static void
ligma_color_editor_entry_changed (LigmaColorHexEntry *entry,
                                 LigmaColorEditor   *editor)
{
  LigmaRGB  rgb;

  ligma_color_hex_entry_get_color (entry, &rgb);

  if (editor->context)
    {
      if (editor->edit_bg)
        ligma_context_set_background (editor->context, &rgb);
      else
        ligma_context_set_foreground (editor->context, &rgb);
    }
}

static void
ligma_color_editor_history_selected (LigmaColorHistory *history,
                                    const LigmaRGB    *rgb,
                                    LigmaColorEditor  *editor)
{
  if (editor->context)
    {
      if (editor->edit_bg)
        ligma_context_set_background (editor->context, rgb);
      else
        ligma_context_set_foreground (editor->context, rgb);
    }
}
