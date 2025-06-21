/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcoloreditor.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpwidgets/gimpwidgets-private.h"
#include "libgimpconfig/gimpconfig.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"

#include "display/display-types.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpcoloreditor.h"
#include "gimpcolorhistory.h"
#include "gimpdocked.h"
#include "gimpfgbgeditor.h"
#include "gimpfgbgview.h"
#include "gimpsessioninfo-aux.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT
};


static void   gimp_color_editor_docked_iface_init (GimpDockedInterface  *iface);

static void   gimp_color_editor_constructed       (GObject           *object);
static void   gimp_color_editor_dispose           (GObject           *object);
static void   gimp_color_editor_set_property      (GObject           *object,
                                                   guint              property_id,
                                                   const GValue      *value,
                                                   GParamSpec        *pspec);
static void   gimp_color_editor_get_property      (GObject           *object,
                                                   guint              property_id,
                                                   GValue            *value,
                                                   GParamSpec        *pspec);

static void   gimp_color_editor_style_updated     (GtkWidget         *widget);

static void   gimp_color_editor_set_aux_info      (GimpDocked        *docked,
                                                   GList             *aux_info);
static GList *gimp_color_editor_get_aux_info       (GimpDocked       *docked);
static GtkWidget *gimp_color_editor_get_preview   (GimpDocked        *docked,
                                                   GimpContext       *context,
                                                   GtkIconSize        size);
static void   gimp_color_editor_set_context       (GimpDocked        *docked,
                                                   GimpContext       *context);

static void   gimp_color_editor_fg_changed        (GimpContext       *context,
                                                   GeglColor         *color,
                                                   GimpColorEditor   *editor);
static void   gimp_color_editor_bg_changed        (GimpContext       *context,
                                                   GeglColor         *color,
                                                   GimpColorEditor   *editor);
static void   gimp_color_editor_color_changed     (GimpColorSelector *selector,
                                                   GeglColor         *color,
                                                   GimpColorEditor   *editor);
static void   gimp_color_editor_tab_toggled       (GtkWidget         *widget,
                                                   GimpColorEditor   *editor);
static void   gimp_color_editor_fg_bg_notify      (GtkWidget         *widget,
                                                   GParamSpec        *pspec,
                                                   GimpColorEditor   *editor);
static void   gimp_color_editor_color_picked      (GtkWidget         *widget,
                                                   const GeglColor   *color,
                                                   GimpColorEditor   *editor);
static void   gimp_color_editor_entry_changed     (GimpColorHexEntry *entry,
                                                   GimpColorEditor   *editor);

static void  gimp_color_editor_history_selected   (GimpColorHistory *history,
                                                   GeglColor        *color,
                                                   GimpColorEditor  *editor);

static void  gimp_color_editor_display_changed    (GimpContext     *context,
                                                   GimpDisplay     *display,
                                                   GimpColorEditor *editor);
static void  gimp_color_editor_image_changed      (GimpDisplay     *display,
                                                   GParamSpec      *pspec,
                                                   GimpColorEditor *editor);
static void  gimp_color_editor_shell_changed      (GimpDisplay     *display,
                                                   GParamSpec      *pspec,
                                                   GimpColorEditor *editor);
static void  gimp_color_editor_update_simulation  (GimpImage       *image,
                                                   GimpColorEditor *editor);
static void  gimp_color_editor_enable_simulation  (GimpColorEditor *editor);
static void  gimp_color_editor_update_format      (GimpColorEditor *editor);


G_DEFINE_TYPE_WITH_CODE (GimpColorEditor, gimp_color_editor, GIMP_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_color_editor_docked_iface_init))

#define parent_class gimp_color_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_color_editor_class_init (GimpColorEditorClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = gimp_color_editor_constructed;
  object_class->dispose       = gimp_color_editor_dispose;
  object_class->set_property  = gimp_color_editor_set_property;
  object_class->get_property  = gimp_color_editor_get_property;

  widget_class->style_updated = gimp_color_editor_style_updated;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        G_PARAM_CONSTRUCT |
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_color_editor_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->get_preview  = gimp_color_editor_get_preview;
  iface->set_aux_info = gimp_color_editor_set_aux_info;
  iface->get_aux_info = gimp_color_editor_get_aux_info;
  iface->set_context  = gimp_color_editor_set_context;
}

static void
gimp_color_editor_init (GimpColorEditor *editor)
{
  GtkWidget   *notebook;
  GtkWidget   *hbox;
  GtkWidget   *button;
  gint         content_spacing;
  gint         button_spacing;
  GtkIconSize  button_icon_size;
  GeglColor   *color;
  GList       *list;
  GSList      *group;
  gint         icon_width  = 40;
  gint         icon_height = 38;

  editor->context = NULL;
  editor->edit_bg = FALSE;

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "content-spacing",  &content_spacing,
                        "button-spacing",   &button_spacing,
                        "button-icon-size", &button_icon_size,
                        NULL);

  editor->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);
  gtk_box_set_homogeneous (GTK_BOX (editor->hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (editor), editor->hbox, FALSE, FALSE, 0);
  gtk_widget_show (editor->hbox);

  color = gegl_color_new ("black");
  editor->notebook = gimp_color_selector_new (GIMP_TYPE_COLOR_NOTEBOOK, color,
                                              GIMP_COLOR_SELECTOR_HUE);
  g_object_unref (color);
  gimp_color_selector_set_show_alpha (GIMP_COLOR_SELECTOR (editor->notebook),
                                      FALSE);
  gtk_box_pack_start (GTK_BOX (editor), editor->notebook,
                      TRUE, TRUE, content_spacing);
  gtk_widget_show (editor->notebook);

  g_signal_connect (editor->notebook, "color-changed",
                    G_CALLBACK (gimp_color_editor_color_changed),
                    editor);

  notebook = gimp_color_notebook_get_notebook (GIMP_COLOR_NOTEBOOK (editor->notebook));

  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

  gimp_color_notebook_set_has_page (GIMP_COLOR_NOTEBOOK (editor->notebook),
                                    GIMP_TYPE_COLOR_SCALES, TRUE);

  group = NULL;

  for (list = gimp_color_notebook_get_selectors (GIMP_COLOR_NOTEBOOK (editor->notebook));
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

      image = gtk_image_new_from_icon_name (selector_class->icon_name,
                                            GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      gimp_help_set_help_data (button,
                               selector_class->name, selector_class->help_id);
      gimp_widget_set_identifier (button, selector_class->name);

      g_object_set_data (G_OBJECT (button),   "selector", selector);
      g_object_set_data (G_OBJECT (selector), "button",   button);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (gimp_color_editor_tab_toggled),
                        editor);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  FG/BG editor  */
  editor->fg_bg = gimp_fg_bg_editor_new (NULL);
  gtk_icon_size_lookup (button_icon_size, &icon_width, &icon_height);
  gtk_widget_set_size_request (editor->fg_bg,
                               (gint) (icon_width * 1.75),
                               (gint) (icon_height * 1.75));
  gtk_box_pack_start (GTK_BOX (hbox), editor->fg_bg, FALSE, FALSE, 0);
  gtk_widget_show (editor->fg_bg);

  g_signal_connect (editor->fg_bg, "notify::active-color",
                    G_CALLBACK (gimp_color_editor_fg_bg_notify),
                    editor);

  /*  The color picker  */
  button = gimp_pick_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "color-picked",
                    G_CALLBACK (gimp_color_editor_color_picked),
                    editor);

  /*  The hex triplet entry  */
  editor->hex_entry = gimp_color_hex_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), editor->hex_entry, TRUE, TRUE, 0);
  gtk_widget_show (editor->hex_entry);

  g_signal_connect (editor->hex_entry, "color-changed",
                    G_CALLBACK (gimp_color_editor_entry_changed),
                    editor);
}

static void
gimp_color_editor_constructed (GObject *object)
{
  GimpColorEditor *editor       = GIMP_COLOR_EDITOR (object);
  GimpContext     *user_context = editor->context->gimp->user_context;
  GimpDisplay     *display      = gimp_context_get_display (user_context);
  GtkWidget       *history;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* The color history */
  history = gimp_color_history_new (editor->context, 12);
  gtk_box_pack_end (GTK_BOX (editor), history, FALSE, FALSE, 0);
  gtk_widget_show (history);

  g_signal_connect (history, "color-selected",
                    G_CALLBACK (gimp_color_editor_history_selected),
                    editor);

  g_signal_connect_object (user_context, "display-changed",
                           G_CALLBACK (gimp_color_editor_display_changed),
                           editor, 0);

  gimp_color_editor_display_changed (user_context, display, editor);
}

static void
gimp_color_editor_dispose (GObject *object)
{
  GimpColorEditor *editor = GIMP_COLOR_EDITOR (object);

  g_clear_weak_pointer (&editor->active_display);
  g_clear_weak_pointer (&editor->active_image);
  g_clear_weak_pointer (&editor->active_shell);

  if (editor->context)
    gimp_docked_set_context (GIMP_DOCKED (editor), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_color_editor_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_CONTEXT:
      gimp_docked_set_context (GIMP_DOCKED (object),
                               g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_editor_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpColorEditor *editor = GIMP_COLOR_EDITOR (object);

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
gimp_color_editor_get_preview (GimpDocked  *docked,
                               GimpContext *context,
                               GtkIconSize  size)
{
  GtkWidget *preview;
  gint       width;
  gint       height;

  preview = gimp_fg_bg_view_new (context);

  if (gtk_icon_size_lookup (size, &width, &height))
    gtk_widget_set_size_request (preview, width, height);

  return preview;
}

#define AUX_INFO_CURRENT_PAGE "current-page"

static void
gimp_color_editor_set_aux_info (GimpDocked *docked,
                                GList      *aux_info)
{
  GimpColorEditor *editor   = GIMP_COLOR_EDITOR (docked);
  GtkWidget       *notebook;
  GList           *list;

  notebook = gimp_color_notebook_get_notebook (GIMP_COLOR_NOTEBOOK (editor->notebook));

  parent_docked_iface->set_aux_info (docked, aux_info);

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
  GimpColorSelector  *current;
  GList              *aux_info;

  aux_info = parent_docked_iface->get_aux_info (docked);

  current = gimp_color_notebook_get_current_selector (notebook);

  if (current)
    {
      GimpSessionInfoAux *aux;

      aux = gimp_session_info_aux_new (AUX_INFO_CURRENT_PAGE,
                                       G_OBJECT_TYPE_NAME (current));
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
gimp_color_editor_set_context (GimpDocked  *docked,
                               GimpContext *context)
{
  GimpColorEditor *editor = GIMP_COLOR_EDITOR (docked);

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

      g_signal_handlers_disconnect_by_func (editor->context->gimp->config,
                                            G_CALLBACK (gimp_color_editor_style_updated),
                                            editor);

      g_object_unref (editor->context);
    }

  editor->context = context;

  if (editor->context)
    {
      GeglColor *color;

      g_object_ref (editor->context);

      g_signal_connect_object (editor->context, "foreground-changed",
                               G_CALLBACK (gimp_color_editor_fg_changed),
                               editor, 0);
      g_signal_connect_object (editor->context, "background-changed",
                               G_CALLBACK (gimp_color_editor_bg_changed),
                               editor, 0);

      g_signal_connect_object (editor->context->gimp->config,
                               "notify::theme",
                               G_CALLBACK (gimp_color_editor_style_updated),
                               editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->gimp->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (gimp_color_editor_style_updated),
                               editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (context->gimp->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (gimp_color_editor_style_updated),
                               editor, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

      if (editor->edit_bg)
        {
          color = gimp_context_get_background (editor->context);
          gimp_color_editor_bg_changed (editor->context, color, editor);
        }
      else
        {
          color = gimp_context_get_foreground (editor->context);
          gimp_color_editor_fg_changed (editor->context, color, editor);
        }

      g_object_set_data (G_OBJECT (context->gimp->config->color_management),
                         "gimp-context", editor->context);

      gimp_color_selector_set_config (GIMP_COLOR_SELECTOR (editor->notebook),
                                      context->gimp->config->color_management);

      g_object_set_data (G_OBJECT (context->gimp->config->color_management),
                         "gimp-context", NULL);
    }

  gimp_fg_bg_editor_set_context (GIMP_FG_BG_EDITOR (editor->fg_bg), context);
}

GtkWidget *
gimp_color_editor_new (GimpContext *context)
{
  return g_object_new (GIMP_TYPE_COLOR_EDITOR,
                       "context", context,
                       NULL);
}

static void
gimp_color_editor_style_updated (GtkWidget *widget)
{
  GimpColorEditor   *editor   = GIMP_COLOR_EDITOR (widget);
  GimpColorNotebook *notebook = GIMP_COLOR_NOTEBOOK (editor->notebook);
  GtkIconSize        button_icon_size;
  gint               icon_width  = 40;
  gint               icon_height = 38;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (editor->hbox)
    gimp_editor_set_box_style (GIMP_EDITOR (editor), GTK_BOX (editor->hbox));

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "button-icon-size", &button_icon_size,
                        NULL);
  gtk_icon_size_lookup (button_icon_size, &icon_width, &icon_height);
  gtk_widget_set_size_request (editor->fg_bg,
                               (gint) (icon_width * 1.75),
                               (gint) (icon_height * 1.75));

  GTK_WIDGET_GET_CLASS (notebook)->style_updated (GTK_WIDGET (notebook));
}


static void
gimp_color_editor_set_color (GimpColorEditor *editor,
                             GeglColor       *color)
{
  g_signal_handlers_block_by_func (editor->notebook,
                                   gimp_color_editor_color_changed,
                                   editor);

  gimp_color_selector_set_color (GIMP_COLOR_SELECTOR (editor->notebook), color);

  g_signal_handlers_unblock_by_func (editor->notebook,
                                     gimp_color_editor_color_changed,
                                     editor);

  g_signal_handlers_block_by_func (editor->hex_entry,
                                   gimp_color_editor_entry_changed,
                                   editor);

  gimp_color_hex_entry_set_color (GIMP_COLOR_HEX_ENTRY (editor->hex_entry), color);

  g_signal_handlers_unblock_by_func (editor->hex_entry,
                                     gimp_color_editor_entry_changed,
                                     editor);
}

static void
gimp_color_editor_fg_changed (GimpContext     *context,
                              GeglColor       *color,
                              GimpColorEditor *editor)
{
  if (! editor->edit_bg)
    gimp_color_editor_set_color (editor, color);
}

static void
gimp_color_editor_bg_changed (GimpContext     *context,
                              GeglColor       *color,
                              GimpColorEditor *editor)
{
  if (editor->edit_bg)
    gimp_color_editor_set_color (editor, color);
}

static void
gimp_color_editor_color_changed (GimpColorSelector *selector,
                                 GeglColor         *color,
                                 GimpColorEditor   *editor)
{
  if (editor->context)
    {
      if (editor->edit_bg)
        {
          g_signal_handlers_block_by_func (editor->context,
                                           gimp_color_editor_bg_changed,
                                           editor);

          gimp_context_set_background (editor->context, color);

          g_signal_handlers_unblock_by_func (editor->context,
                                             gimp_color_editor_bg_changed,
                                             editor);
        }
      else
        {
          g_signal_handlers_block_by_func (editor->context,
                                           gimp_color_editor_fg_changed,
                                           editor);

          gimp_context_set_foreground (editor->context, color);

          g_signal_handlers_unblock_by_func (editor->context,
                                             gimp_color_editor_fg_changed,
                                             editor);
        }
    }

  g_signal_handlers_block_by_func (editor->hex_entry,
                                   gimp_color_editor_entry_changed,
                                   editor);

  gimp_color_hex_entry_set_color (GIMP_COLOR_HEX_ENTRY (editor->hex_entry), color);

  g_signal_handlers_unblock_by_func (editor->hex_entry,
                                     gimp_color_editor_entry_changed,
                                     editor);
}

static void
gimp_color_editor_tab_toggled (GtkWidget       *widget,
                               GimpColorEditor *editor)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GtkWidget *selector;

      selector = g_object_get_data (G_OBJECT (widget), "selector");

      if (selector)
        {
          GtkWidget *notebook;
          gint       page_num;

          notebook = gimp_color_notebook_get_notebook (GIMP_COLOR_NOTEBOOK (editor->notebook));

          page_num = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), selector);

          if (page_num >= 0)
            gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), page_num);
        }
    }
}

static void
gimp_color_editor_fg_bg_notify (GtkWidget       *widget,
                                GParamSpec      *pspec,
                                GimpColorEditor *editor)
{
  gboolean edit_bg;

  edit_bg = (GIMP_FG_BG_EDITOR (widget)->active_color ==
             GIMP_ACTIVE_COLOR_BACKGROUND);

  if (edit_bg != editor->edit_bg)
    {
      editor->edit_bg = edit_bg;

      if (editor->context)
        {
          GeglColor *color;

          if (edit_bg)
            {
              color = gimp_context_get_background (editor->context);
              gimp_color_editor_bg_changed (editor->context, color, editor);
            }
          else
            {
              color = gimp_context_get_foreground (editor->context);
              gimp_color_editor_fg_changed (editor->context, color, editor);
            }
        }
    }
}

static void
gimp_color_editor_color_picked (GtkWidget       *widget,
                                const GeglColor *color,
                                GimpColorEditor *editor)
{
  if (editor->context && color)
    {
      if (editor->edit_bg)
        gimp_context_set_background (editor->context, GEGL_COLOR (color));
      else
        gimp_context_set_foreground (editor->context, GEGL_COLOR (color));
    }
}

static void
gimp_color_editor_entry_changed (GimpColorHexEntry *entry,
                                 GimpColorEditor   *editor)
{
  GeglColor *color;

  color = gimp_color_hex_entry_get_color (entry);

  if (editor->context)
    {
      if (editor->edit_bg)
        gimp_context_set_background (editor->context, color);
      else
        gimp_context_set_foreground (editor->context, color);
    }

  g_object_unref (color);
}

static void
gimp_color_editor_history_selected (GimpColorHistory *history,
                                    GeglColor        *color,
                                    GimpColorEditor  *editor)
{
  if (editor->context)
    {
      if (editor->edit_bg)
        gimp_context_set_background (editor->context, color);
      else
        gimp_context_set_foreground (editor->context, color);
    }
}

static void
gimp_color_editor_display_changed (GimpContext     *context,
                                   GimpDisplay     *display,
                                   GimpColorEditor *editor)
{
  if (editor->active_display != display)
    {
      if (editor->active_display)
        {
          g_signal_handlers_disconnect_by_func (editor->active_display,
                                                gimp_color_editor_image_changed,
                                                editor);
          g_signal_handlers_disconnect_by_func (editor->active_display,
                                                gimp_color_editor_shell_changed,
                                                editor);
        }

      g_set_weak_pointer (&editor->active_display, display);

      if (display)
        {
          g_signal_connect_object (display, "notify::image",
                                   G_CALLBACK (gimp_color_editor_image_changed),
                                   editor, 0);
          g_signal_connect_object (display, "notify::shell",
                                   G_CALLBACK (gimp_color_editor_shell_changed),
                                   editor, 0);
        }

      gimp_color_editor_image_changed (display, NULL, editor);
      gimp_color_editor_shell_changed (display, NULL, editor);
    }
}

static void
gimp_color_editor_image_changed (GimpDisplay     *display,
                                 GParamSpec      *pspec,
                                 GimpColorEditor *editor)
{
  GimpImage *image = display ? gimp_display_get_image (display) : NULL;

  if (editor->active_image != image)
    {
      if (editor->active_image)
        {
          g_signal_handlers_disconnect_by_func (editor->active_image,
                                                gimp_color_editor_update_simulation,
                                                editor);
          g_signal_handlers_disconnect_by_func (editor->active_image,
                                                gimp_color_editor_update_format,
                                                editor);
        }

      g_set_weak_pointer (&editor->active_image, image);

      if (image)
        {
          g_signal_connect_object (image, "simulation-profile-changed",
                                   G_CALLBACK (gimp_color_editor_update_simulation),
                                   editor, 0);
          g_signal_connect_object (image, "simulation-intent-changed",
                                   G_CALLBACK (gimp_color_editor_update_simulation),
                                   editor, 0);
          g_signal_connect_object (image, "simulation-bpc-changed",
                                   G_CALLBACK (gimp_color_editor_update_simulation),
                                   editor, 0);

          g_signal_connect_object (image, "profile-changed",
                                   G_CALLBACK (gimp_color_editor_update_format),
                                   editor, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "precision-changed",
                                   G_CALLBACK (gimp_color_editor_update_format),
                                   editor, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "notify::base-type",
                                   G_CALLBACK (gimp_color_editor_update_format),
                                   editor, G_CONNECT_SWAPPED);
        }

      gimp_color_editor_update_simulation (image, editor);
      gimp_color_editor_update_format (editor);
    }
}

static void
gimp_color_editor_shell_changed (GimpDisplay     *display,
                                 GParamSpec      *pspec,
                                 GimpColorEditor *editor)
{
  GimpDisplayShell *shell = display ? gimp_display_get_shell (display) : NULL;

  if (editor->active_shell != shell)
    {
      GimpColorConfig *config;

      if (editor->active_shell)
        {
          config = gimp_display_shell_get_color_config (editor->active_shell);

          g_signal_handlers_disconnect_by_func (config,
                                                gimp_color_editor_enable_simulation,
                                                editor);
        }

      g_set_weak_pointer (&editor->active_shell, shell);

      if (shell)
        {
          config = gimp_display_shell_get_color_config (shell);

          g_signal_connect_object (config, "notify::mode",
                                   G_CALLBACK (gimp_color_editor_enable_simulation),
                                   editor, G_CONNECT_SWAPPED);
        }

      gimp_color_editor_enable_simulation (editor);
    }
}

static void
gimp_color_editor_update_simulation (GimpImage       *image,
                                     GimpColorEditor *editor)
{
  g_return_if_fail (GIMP_IS_COLOR_EDITOR (editor));

  if (image)
    gimp_color_notebook_set_simulation (GIMP_COLOR_NOTEBOOK (editor->notebook),
                                        gimp_image_get_simulation_profile (image),
                                        gimp_image_get_simulation_intent (image),
                                        gimp_image_get_simulation_bpc (image));
  else
    gimp_color_notebook_set_simulation (GIMP_COLOR_NOTEBOOK (editor->notebook),
                                        NULL,
                                        GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                        FALSE);
}

static void
gimp_color_editor_enable_simulation (GimpColorEditor *editor)
{
  g_return_if_fail (GIMP_IS_COLOR_EDITOR (editor));

  if (editor->active_shell && editor->active_image)
    {
      GimpColorConfig         *config;
      GimpColorManagementMode  mode;

      config = gimp_display_shell_get_color_config (editor->active_shell);
      mode   = gimp_color_config_get_mode (config);
      gimp_color_notebook_enable_simulation (GIMP_COLOR_NOTEBOOK (editor->notebook),
                                             (mode == GIMP_COLOR_MANAGEMENT_SOFTPROOF));
    }
  else
    {
      gimp_color_notebook_enable_simulation (GIMP_COLOR_NOTEBOOK (editor->notebook), FALSE);
    }
}

static void
gimp_color_editor_update_format (GimpColorEditor *editor)
{
  g_return_if_fail (GIMP_IS_COLOR_EDITOR (editor));

  if (editor->active_image)
    gimp_color_notebook_set_format (GIMP_COLOR_NOTEBOOK (editor->notebook),
                                    gimp_image_get_layer_format (editor->active_image, FALSE));
  else
    gimp_color_notebook_set_format (GIMP_COLOR_NOTEBOOK (editor->notebook), NULL);
}
