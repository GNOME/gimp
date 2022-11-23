/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolornotebook.c
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"

#include "ligmawidgetstypes.h"

#include "ligmacolornotebook.h"
#include "ligmacolorscales.h"
#include "ligmahelpui.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmacolornotebook
 * @title: LigmaColorNotebook
 * @short_description: A #LigmaColorSelector implementation.
 *
 * The #LigmaColorNotebook widget is an implementation of a
 * #LigmaColorSelector. It serves as a container for
 * #LigmaColorSelectors.
 **/


#define DEFAULT_TAB_ICON_SIZE  GTK_ICON_SIZE_BUTTON


struct _LigmaColorNotebookPrivate
{
  GtkWidget         *notebook;

  GList             *selectors;
  LigmaColorSelector *cur_page;
};

#define GET_PRIVATE(obj) (((LigmaColorNotebook *) obj)->priv)


static void   ligma_color_notebook_style_updated   (GtkWidget         *widget);

static void   ligma_color_notebook_togg_visible    (LigmaColorSelector *selector,
                                                   gboolean           visible);
static void   ligma_color_notebook_togg_sensitive  (LigmaColorSelector *selector,
                                                   gboolean           sensitive);
static void   ligma_color_notebook_set_show_alpha  (LigmaColorSelector *selector,
                                                   gboolean           show_alpha);
static void   ligma_color_notebook_set_color       (LigmaColorSelector *selector,
                                                   const LigmaRGB     *rgb,
                                                   const LigmaHSV     *hsv);
static void   ligma_color_notebook_set_channel     (LigmaColorSelector *selector,
                                                   LigmaColorSelectorChannel channel);
static void   ligma_color_notebook_set_model_visible
                                                  (LigmaColorSelector *selector,
                                                   LigmaColorSelectorModel model,
                                                   gboolean           gboolean);
static void   ligma_color_notebook_set_config      (LigmaColorSelector *selector,
                                                   LigmaColorConfig   *config);


static void   ligma_color_notebook_switch_page     (GtkNotebook       *gtk_notebook,
                                                   gpointer           page,
                                                   guint              page_num,
                                                   LigmaColorNotebook *notebook);

static void   ligma_color_notebook_color_changed   (LigmaColorSelector *page,
                                                   const LigmaRGB     *rgb,
                                                   const LigmaHSV     *hsv,
                                                   LigmaColorNotebook *notebook);
static void   ligma_color_notebook_channel_changed (LigmaColorSelector *page,
                                                   LigmaColorSelectorChannel channel,
                                                   LigmaColorNotebook *notebook);
static void   ligma_color_notebook_model_visible_changed
                                                  (LigmaColorSelector *page,
                                                   LigmaColorSelectorModel model,
                                                   gboolean           visible,
                                                   LigmaColorNotebook *notebook);

static GtkWidget * ligma_color_notebook_add_page   (LigmaColorNotebook *notebook,
                                                   GType              page_type);
static void   ligma_color_notebook_remove_selector (GtkContainer      *container,
                                                   GtkWidget         *widget,
                                                   LigmaColorNotebook *notebook);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaColorNotebook, ligma_color_notebook,
                            LIGMA_TYPE_COLOR_SELECTOR)

#define parent_class ligma_color_notebook_parent_class


static void
ligma_color_notebook_class_init (LigmaColorNotebookClass *klass)
{
  GtkWidgetClass         *widget_class   = GTK_WIDGET_CLASS (klass);
  LigmaColorSelectorClass *selector_class = LIGMA_COLOR_SELECTOR_CLASS (klass);

  widget_class->style_updated           = ligma_color_notebook_style_updated;

  selector_class->name                  = "Notebook";
  selector_class->help_id               = "ligma-colorselector-notebook";
  selector_class->set_toggles_visible   = ligma_color_notebook_togg_visible;
  selector_class->set_toggles_sensitive = ligma_color_notebook_togg_sensitive;
  selector_class->set_show_alpha        = ligma_color_notebook_set_show_alpha;
  selector_class->set_color             = ligma_color_notebook_set_color;
  selector_class->set_channel           = ligma_color_notebook_set_channel;
  selector_class->set_model_visible     = ligma_color_notebook_set_model_visible;
  selector_class->set_config            = ligma_color_notebook_set_config;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tab-icon-size",
                                                              NULL,
                                                              "Size for icons displayed in the tab",
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_TAB_ICON_SIZE,
                                                              G_PARAM_READABLE));

  gtk_widget_class_set_css_name (widget_class, "LigmaColorNotebook");
}

static void
ligma_color_notebook_init (LigmaColorNotebook *notebook)
{
  LigmaColorNotebookPrivate *private = GET_PRIVATE (notebook);
  GType                    *selector_types;
  guint                     n_selector_types;
  guint                     i;

  notebook->priv = ligma_color_notebook_get_instance_private (notebook);

  private = notebook->priv;

  private->notebook = gtk_notebook_new ();
  gtk_notebook_popup_enable (GTK_NOTEBOOK (private->notebook));
  gtk_box_pack_start (GTK_BOX (notebook), private->notebook, TRUE, TRUE, 0);
  gtk_widget_show (private->notebook);

  g_signal_connect (private->notebook, "switch-page",
                    G_CALLBACK (ligma_color_notebook_switch_page),
                    notebook);
  g_signal_connect (private->notebook, "remove",
                    G_CALLBACK (ligma_color_notebook_remove_selector),
                    notebook);

  selector_types = g_type_children (LIGMA_TYPE_COLOR_SELECTOR,
                                    &n_selector_types);

  if (n_selector_types == 2)
    {
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (private->notebook), FALSE);
      gtk_notebook_set_show_border (GTK_NOTEBOOK (private->notebook), FALSE);
    }

  for (i = 0; i < n_selector_types; i++)
    {
      /*  skip ourselves  */
      if (g_type_is_a (selector_types[i], LIGMA_TYPE_COLOR_NOTEBOOK))
        continue;

      /*  skip the "Scales" color selector  */
      if (g_type_is_a (selector_types[i], LIGMA_TYPE_COLOR_SCALES))
        continue;

      ligma_color_notebook_add_page (notebook, selector_types[i]);
    }

  g_free (selector_types);
}

static void
ligma_color_notebook_style_updated (GtkWidget *widget)
{
  LigmaColorNotebookPrivate *private = GET_PRIVATE (widget);
  GList                    *list;
  GtkIconSize               icon_size;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "tab-icon-size", &icon_size,
                        NULL);

  for (list = private->selectors; list; list = g_list_next (list))
    {
      LigmaColorSelectorClass *selector_class;
      GtkWidget              *image;

      selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (list->data);

      image = gtk_image_new_from_icon_name (selector_class->icon_name,
                                            icon_size);
      ligma_help_set_help_data (image, gettext (selector_class->name), NULL);

      gtk_notebook_set_tab_label (GTK_NOTEBOOK (private->notebook),
                                  GTK_WIDGET (list->data),
                                  image);
    }
}

static void
ligma_color_notebook_togg_visible (LigmaColorSelector *selector,
                                  gboolean           visible)
{
  LigmaColorNotebookPrivate *private = GET_PRIVATE (selector);
  GList                    *list;

  for (list = private->selectors; list; list = g_list_next (list))
    {
      LigmaColorSelector *child = list->data;

      ligma_color_selector_set_toggles_visible (child, visible);
    }
}

static void
ligma_color_notebook_togg_sensitive (LigmaColorSelector *selector,
                                    gboolean           sensitive)
{
  LigmaColorNotebookPrivate *private = GET_PRIVATE (selector);
  GList                    *list;

  for (list = private->selectors; list; list = g_list_next (list))
    {
      LigmaColorSelector *child = list->data;

      ligma_color_selector_set_toggles_sensitive (child, sensitive);
    }
}

static void
ligma_color_notebook_set_show_alpha (LigmaColorSelector *selector,
                                    gboolean           show_alpha)
{
  LigmaColorNotebookPrivate *private = GET_PRIVATE (selector);
  GList                    *list;

  for (list = private->selectors; list; list = g_list_next (list))
    {
      LigmaColorSelector *child = list->data;

      ligma_color_selector_set_show_alpha (child, show_alpha);
    }
}

static void
ligma_color_notebook_set_color (LigmaColorSelector *selector,
                               const LigmaRGB     *rgb,
                               const LigmaHSV     *hsv)
{
  LigmaColorNotebookPrivate *private = GET_PRIVATE (selector);

  g_signal_handlers_block_by_func (private->cur_page,
                                   ligma_color_notebook_color_changed,
                                   selector);

  ligma_color_selector_set_color (private->cur_page, rgb, hsv);

  g_signal_handlers_unblock_by_func (private->cur_page,
                                     ligma_color_notebook_color_changed,
                                     selector);
}

static void
ligma_color_notebook_set_channel (LigmaColorSelector        *selector,
                                 LigmaColorSelectorChannel  channel)
{
  LigmaColorNotebookPrivate *private = GET_PRIVATE (selector);

  g_signal_handlers_block_by_func (private->cur_page,
                                   ligma_color_notebook_channel_changed,
                                   selector);

  ligma_color_selector_set_channel (private->cur_page, channel);

  g_signal_handlers_unblock_by_func (private->cur_page,
                                     ligma_color_notebook_channel_changed,
                                     selector);
}

static void
ligma_color_notebook_set_model_visible (LigmaColorSelector      *selector,
                                       LigmaColorSelectorModel  model,
                                       gboolean                visible)
{
  LigmaColorNotebookPrivate *private = GET_PRIVATE (selector);

  g_signal_handlers_block_by_func (private->cur_page,
                                   ligma_color_notebook_model_visible_changed,
                                   selector);

  ligma_color_selector_set_model_visible (private->cur_page, model, visible);

  g_signal_handlers_unblock_by_func (private->cur_page,
                                     ligma_color_notebook_model_visible_changed,
                                     selector);
}

static void
ligma_color_notebook_set_config (LigmaColorSelector *selector,
                                LigmaColorConfig   *config)
{
  LigmaColorNotebookPrivate *private = GET_PRIVATE (selector);
  GList                    *list;

  for (list = private->selectors; list; list = g_list_next (list))
    {
      LigmaColorSelector *child = list->data;

      ligma_color_selector_set_config (child, config);
    }
}

static void
ligma_color_notebook_switch_page (GtkNotebook       *gtk_notebook,
                                 gpointer           page,
                                 guint              page_num,
                                 LigmaColorNotebook *notebook)
{
  LigmaColorNotebookPrivate *private  = GET_PRIVATE (notebook);
  LigmaColorSelector        *selector = LIGMA_COLOR_SELECTOR (notebook);
  GtkWidget                *page_widget;
  LigmaColorSelectorModel    model;

  page_widget = gtk_notebook_get_nth_page (gtk_notebook, page_num);

  private->cur_page = LIGMA_COLOR_SELECTOR (page_widget);

  g_signal_handlers_block_by_func (private->cur_page,
                                   ligma_color_notebook_color_changed,
                                   notebook);
  g_signal_handlers_block_by_func (private->cur_page,
                                   ligma_color_notebook_channel_changed,
                                   notebook);
  g_signal_handlers_block_by_func (private->cur_page,
                                   ligma_color_notebook_model_visible_changed,
                                   notebook);

  ligma_color_selector_set_color (private->cur_page,
                                 &selector->rgb,
                                 &selector->hsv);
  ligma_color_selector_set_channel (private->cur_page,
                                   ligma_color_selector_get_channel (selector));

  for (model = LIGMA_COLOR_SELECTOR_MODEL_RGB;
       model <= LIGMA_COLOR_SELECTOR_MODEL_HSV;
       model++)
    {
      gboolean visible = ligma_color_selector_get_model_visible (selector, model);

      ligma_color_selector_set_model_visible (private->cur_page, model,
                                             visible);
    }

  g_signal_handlers_unblock_by_func (private->cur_page,
                                     ligma_color_notebook_color_changed,
                                     notebook);
  g_signal_handlers_unblock_by_func (private->cur_page,
                                     ligma_color_notebook_channel_changed,
                                     notebook);
  g_signal_handlers_unblock_by_func (private->cur_page,
                                     ligma_color_notebook_model_visible_changed,
                                     notebook);
}

static void
ligma_color_notebook_color_changed (LigmaColorSelector *page,
                                   const LigmaRGB     *rgb,
                                   const LigmaHSV     *hsv,
                                   LigmaColorNotebook *notebook)
{
  LigmaColorSelector *selector = LIGMA_COLOR_SELECTOR (notebook);

  selector->rgb = *rgb;
  selector->hsv = *hsv;

  ligma_color_selector_emit_color_changed (selector);
}

static void
ligma_color_notebook_channel_changed (LigmaColorSelector        *page,
                                     LigmaColorSelectorChannel  channel,
                                     LigmaColorNotebook        *notebook)
{
  LigmaColorSelector *selector = LIGMA_COLOR_SELECTOR (notebook);

  ligma_color_selector_set_channel (selector, channel);
}

static void
ligma_color_notebook_model_visible_changed (LigmaColorSelector      *page,
                                           LigmaColorSelectorModel  model,
                                           gboolean                visible,
                                           LigmaColorNotebook      *notebook)
{
  LigmaColorSelector *selector = LIGMA_COLOR_SELECTOR (notebook);

  ligma_color_selector_set_model_visible (selector, model, visible);
}

static GtkWidget *
ligma_color_notebook_add_page (LigmaColorNotebook *notebook,
                              GType              page_type)
{
  LigmaColorNotebookPrivate *private  = GET_PRIVATE (notebook);
  LigmaColorSelector        *selector = LIGMA_COLOR_SELECTOR (notebook);
  LigmaColorSelectorClass   *selector_class;
  GtkWidget                *page;
  GtkWidget                *menu_widget;
  GtkWidget                *image;
  GtkWidget                *label;
  gboolean                  show_alpha;

  page = ligma_color_selector_new (page_type,
                                  &selector->rgb,
                                  &selector->hsv,
                                  ligma_color_selector_get_channel (selector));

  if (! page)
    return NULL;

  selector_class = LIGMA_COLOR_SELECTOR_GET_CLASS (page);

  show_alpha = ligma_color_selector_get_show_alpha (LIGMA_COLOR_SELECTOR (notebook));
  ligma_color_selector_set_show_alpha (LIGMA_COLOR_SELECTOR (page), show_alpha);

  menu_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  image = gtk_image_new_from_icon_name (selector_class->icon_name,
                                        GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (menu_widget), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  label = gtk_label_new (gettext (selector_class->name));
  gtk_box_pack_start (GTK_BOX (menu_widget), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  image = gtk_image_new_from_icon_name (selector_class->icon_name,
                                        DEFAULT_TAB_ICON_SIZE);
  ligma_help_set_help_data (image, gettext (selector_class->name), NULL);

  gtk_notebook_append_page_menu (GTK_NOTEBOOK (private->notebook),
                                 page, image, menu_widget);

  if (! private->cur_page)
    private->cur_page = LIGMA_COLOR_SELECTOR (page);

  private->selectors = g_list_append (private->selectors, page);

  gtk_widget_show (page);

  g_signal_connect (page, "color-changed",
                    G_CALLBACK (ligma_color_notebook_color_changed),
                    notebook);
  g_signal_connect (page, "channel-changed",
                    G_CALLBACK (ligma_color_notebook_channel_changed),
                    notebook);
  g_signal_connect (page, "model-visible-changed",
                    G_CALLBACK (ligma_color_notebook_model_visible_changed),
                    notebook);

  return page;
}

static void
ligma_color_notebook_remove_selector (GtkContainer      *container,
                                     GtkWidget         *widget,
                                     LigmaColorNotebook *notebook)
{
  notebook->priv->selectors = g_list_remove (notebook->priv->selectors,
                                             widget);

  if (! notebook->priv->selectors)
    notebook->priv->cur_page = NULL;
}


/**
 * ligma_color_notebook_set_has_page:
 * @notebook:  A #LigmaColorNotebook widget.
 * @page_type: The #GType of the notebook page to add or remove.
 * @has_page:  Whether the page should be added or removed.
 *
 * This function adds and removed pages to / from a #LigmaColorNotebook.
 * The @page_type passed must be a #LigmaColorSelector subtype.
 *
 * Returns: (transfer none): The new page widget, if @has_page was
 *               %TRUE, or %NULL if @has_page was %FALSE.
 **/
GtkWidget *
ligma_color_notebook_set_has_page (LigmaColorNotebook *notebook,
                                  GType              page_type,
                                  gboolean           has_page)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_COLOR_NOTEBOOK (notebook), NULL);
  g_return_val_if_fail (g_type_is_a (page_type, LIGMA_TYPE_COLOR_SELECTOR),
                        NULL);
  g_return_val_if_fail (! g_type_is_a (page_type, LIGMA_TYPE_COLOR_NOTEBOOK),
                        NULL);

  for (list = notebook->priv->selectors; list; list = g_list_next (list))
    {
      LigmaColorSelector *page = list->data;

      if (G_TYPE_FROM_INSTANCE (page) == page_type)
        {
          if (has_page)
            return GTK_WIDGET (page);

          gtk_container_remove (GTK_CONTAINER (notebook->priv->notebook),
                                GTK_WIDGET (page));

          return NULL;
        }
    }

  if (! has_page)
    return NULL;

  return ligma_color_notebook_add_page (notebook, page_type);
}

/**
 * ligma_color_notebook_get_notebook:
 * @notebook:  A #LigmaColorNotebook widget.
 *
 * Returns: (transfer none) (type GtkNotebook): The #GtkNotebook inside.
 *
 * Since: 3.0
 **/
GtkWidget *
ligma_color_notebook_get_notebook (LigmaColorNotebook *notebook)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_NOTEBOOK (notebook), NULL);

  return notebook->priv->notebook;
}

/**
 * ligma_color_notebook_get_selectors:
 * @notebook:  A #LigmaColorNotebook widget.
 *
 * Returns: (element-type LigmaColorSelector) (transfer none): The
 *               notebook's list of #LigmaColorSelector's.
 *
 * Since: 3.0
 **/
GList *
ligma_color_notebook_get_selectors (LigmaColorNotebook *notebook)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_NOTEBOOK (notebook), NULL);

  return notebook->priv->selectors;
}

/**
 * ligma_color_notebook_get_current_selector:
 * @notebook:  A #LigmaColorNotebook widget.
 *
 * Returns: (transfer none): The active page's #LigmaColorSelector.
 *
 * Since: 3.0
 **/
LigmaColorSelector *
ligma_color_notebook_get_current_selector (LigmaColorNotebook *notebook)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_NOTEBOOK (notebook), NULL);

  return notebook->priv->cur_page;
}

/**
 * ligma_color_notebook_set_simulation:
 * @notebook:  A #LigmaColorNotebook widget.
 * @profile:   A #LigmaColorProfile object.
 * @intent:    A #LigmaColorRenderingIntent enum.
 * @bpc:       A gboolean.
 *
 * Updates all selectors with the current simulation settings.
 *
 * Since: 3.0
 **/
void
ligma_color_notebook_set_simulation (LigmaColorNotebook *notebook,
                                    LigmaColorProfile  *profile,
                                    LigmaColorRenderingIntent intent,
                                    gboolean           bpc)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_COLOR_NOTEBOOK (notebook));
  g_return_if_fail (profile == NULL || LIGMA_IS_COLOR_PROFILE (profile));

  for (list = notebook->priv->selectors; list; list = g_list_next (list))
    {
      LigmaColorSelector *selector = list->data;

      if (selector)
        ligma_color_selector_set_simulation (selector, profile, intent, bpc);
    }
}
