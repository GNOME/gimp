/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorselector.h
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
 *
 * based on:
 * Colour selector module
 * Copyright (C) 1999 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_SELECTOR_H__
#define __LIGMA_COLOR_SELECTOR_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


/**
 * LIGMA_COLOR_SELECTOR_SIZE:
 *
 * The suggested size for a color area in a #LigmaColorSelector
 * implementation.
 **/
#define LIGMA_COLOR_SELECTOR_SIZE     150

/**
 * LIGMA_COLOR_SELECTOR_BAR_SIZE:
 *
 * The suggested width for a color bar in a #LigmaColorSelector
 * implementation.
 **/
#define LIGMA_COLOR_SELECTOR_BAR_SIZE 15


#define LIGMA_TYPE_COLOR_SELECTOR            (ligma_color_selector_get_type ())
#define LIGMA_COLOR_SELECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_SELECTOR, LigmaColorSelector))
#define LIGMA_COLOR_SELECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_SELECTOR, LigmaColorSelectorClass))
#define LIGMA_IS_COLOR_SELECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_SELECTOR))
#define LIGMA_IS_COLOR_SELECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_SELECTOR))
#define LIGMA_COLOR_SELECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_SELECTOR, LigmaColorSelectorClass))

typedef struct _LigmaColorSelectorPrivate LigmaColorSelectorPrivate;
typedef struct _LigmaColorSelectorClass   LigmaColorSelectorClass;

struct _LigmaColorSelector
{
  GtkBox                    parent_instance;

  LigmaColorSelectorPrivate *priv;

  /* FIXME MOVE TO PRIVATE */
  gboolean                  toggles_visible;
  gboolean                  toggles_sensitive;
  gboolean                  show_alpha;

  LigmaRGB                   rgb;
  LigmaHSV                   hsv;

  LigmaColorSelectorChannel  channel;
};

struct _LigmaColorSelectorClass
{
  GtkBoxClass  parent_class;

  const gchar *name;
  const gchar *help_id;
  const gchar *icon_name;

  /*  virtual functions  */
  void (* set_toggles_visible)   (LigmaColorSelector        *selector,
                                  gboolean                  visible);
  void (* set_toggles_sensitive) (LigmaColorSelector        *selector,
                                  gboolean                  sensitive);
  void (* set_show_alpha)        (LigmaColorSelector        *selector,
                                  gboolean                  show_alpha);
  void (* set_color)             (LigmaColorSelector        *selector,
                                  const LigmaRGB            *rgb,
                                  const LigmaHSV            *hsv);
  void (* set_channel)           (LigmaColorSelector        *selector,
                                  LigmaColorSelectorChannel  channel);
  void (* set_model_visible)     (LigmaColorSelector        *selector,
                                  LigmaColorSelectorModel    model,
                                  gboolean                  visible);
  void (* set_config)            (LigmaColorSelector        *selector,
                                  LigmaColorConfig          *config);

  void (* set_simulation)        (LigmaColorSelector        *selector,
                                  LigmaColorProfile         *profile,
                                  LigmaColorRenderingIntent  intent,
                                  gboolean                  bpc);

  /*  signals  */
  void (* color_changed)         (LigmaColorSelector        *selector,
                                  const LigmaRGB            *rgb,
                                  const LigmaHSV            *hsv);
  void (* channel_changed)       (LigmaColorSelector        *selector,
                                  LigmaColorSelectorChannel  channel);
  void (* model_visible_changed) (LigmaColorSelector        *selector,
                                  LigmaColorSelectorModel    model,
                                  gboolean                  visible);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType       ligma_color_selector_get_type         (void) G_GNUC_CONST;
GtkWidget * ligma_color_selector_new              (GType              selector_type,
                                                  const LigmaRGB     *rgb,
                                                  const LigmaHSV     *hsv,
                                                  LigmaColorSelectorChannel  channel);

void     ligma_color_selector_set_toggles_visible   (LigmaColorSelector *selector,
                                                    gboolean           visible);
gboolean ligma_color_selector_get_toggles_visible   (LigmaColorSelector *selector);

void     ligma_color_selector_set_toggles_sensitive (LigmaColorSelector *selector,
                                                    gboolean           sensitive);
gboolean ligma_color_selector_get_toggles_sensitive (LigmaColorSelector *selector);

void     ligma_color_selector_set_show_alpha        (LigmaColorSelector *selector,
                                                    gboolean           show_alpha);
gboolean ligma_color_selector_get_show_alpha        (LigmaColorSelector *selector);

void     ligma_color_selector_set_color             (LigmaColorSelector *selector,
                                                    const LigmaRGB     *rgb,
                                                    const LigmaHSV     *hsv);
void     ligma_color_selector_get_color             (LigmaColorSelector *selector,
                                                    LigmaRGB           *rgb,
                                                    LigmaHSV           *hsv);

void     ligma_color_selector_set_channel           (LigmaColorSelector *selector,
                                                    LigmaColorSelectorChannel  channel);
LigmaColorSelectorChannel
         ligma_color_selector_get_channel           (LigmaColorSelector *selector);

void     ligma_color_selector_set_model_visible     (LigmaColorSelector *selector,
                                                    LigmaColorSelectorModel model,
                                                    gboolean           visible);
gboolean ligma_color_selector_get_model_visible     (LigmaColorSelector *selector,
                                                    LigmaColorSelectorModel model);

void     ligma_color_selector_emit_color_changed         (LigmaColorSelector *selector);
void     ligma_color_selector_emit_channel_changed       (LigmaColorSelector *selector);
void     ligma_color_selector_emit_model_visible_changed (LigmaColorSelector *selector,
                                                         LigmaColorSelectorModel model);

void     ligma_color_selector_set_config            (LigmaColorSelector *selector,
                                                    LigmaColorConfig   *config);

void     ligma_color_selector_set_simulation        (LigmaColorSelector *selector,
                                                    LigmaColorProfile  *profile,
                                                    LigmaColorRenderingIntent intent,
                                                    gboolean           bpc);


G_END_DECLS

#endif /* __LIGMA_COLOR_SELECTOR_H__ */
