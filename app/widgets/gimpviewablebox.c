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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpconfig-utils.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"

#include "gimpcontainerentry.h"
#include "gimpdialogfactory.h"
#include "gimppropwidgets.h"
#include "gimpview.h"
#include "gimpviewablebutton.h"
#include "gimpviewablebox.h"
#include "gimpviewrenderergradient.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GtkWidget * gimp_viewable_box_new       (GimpContainer *container,
                                                GimpContext   *context,
                                                gint           spacing,
                                                GimpViewType   view_type,
                                                GimpViewType   button_view_size,
                                                GimpViewSize   view_size,
                                                const gchar   *dialog_identifier,
                                                const gchar   *dialog_stock_id,
                                                const gchar   *dialog_tooltip);
static GtkWidget * view_props_connect          (GtkWidget     *box,
                                                GimpContext   *context,
                                                const gchar   *view_type_prop,
                                                const gchar   *view_size_prop);
static void   gimp_gradient_box_reverse_notify (GObject       *object,
                                                GParamSpec    *pspec,
                                                GimpView      *view);


/*  brush boxes  */

static GtkWidget *
brush_box_new (GimpContainer *container,
               GimpContext   *context,
               gint           spacing,
               GimpViewType   view_type,
               GimpViewSize   view_size)
{
  if (! container)
    container = context->gimp->brush_factory->container;

  return gimp_viewable_box_new (container, context, spacing,
                                view_type, GIMP_VIEW_SIZE_SMALL, view_size,
                                "gimp-brush-grid|gimp-brush-list",
                                GIMP_STOCK_BRUSH,
                                _("Open the brush selection dialog"));
}

GtkWidget *
gimp_brush_box_new (GimpContainer *container,
                    GimpContext   *context,
                    gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return brush_box_new (container, context, spacing,
                        GIMP_VIEW_TYPE_GRID, GIMP_VIEW_SIZE_SMALL);
}
GtkWidget *
gimp_prop_brush_box_new (GimpContainer *container,
                         GimpContext   *context,
                         gint           spacing,
                         const gchar   *view_type_prop,
                         const gchar   *view_size_prop)
{
  GimpViewType view_type;
  GimpViewSize view_size;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (brush_box_new (container, context, spacing,
                                            view_type, view_size),
                             context,
                             view_type_prop, view_size_prop);
}


/*  pattern boxes  */

static GtkWidget *
pattern_box_new (GimpContainer *container,
                 GimpContext   *context,
                 gint           spacing,
                 GimpViewType   view_type,
                 GimpViewSize   view_size)
{
  if (! container)
    container = context->gimp->pattern_factory->container;

  return gimp_viewable_box_new (container, context, spacing,
                                view_type, GIMP_VIEW_SIZE_SMALL, view_size,
                                "gimp-pattern-grid|gimp-pattern-list",
                                GIMP_STOCK_PATTERN,
                                _("Open the pattern selection dialog"));
}

GtkWidget *
gimp_pattern_box_new (GimpContainer *container,
                      GimpContext   *context,
                      gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return pattern_box_new (container, context, spacing,
                          GIMP_VIEW_TYPE_GRID, GIMP_VIEW_SIZE_SMALL);
}

GtkWidget *
gimp_prop_pattern_box_new (GimpContainer *container,
                           GimpContext   *context,
                           gint           spacing,
                           const gchar   *view_type_prop,
                           const gchar   *view_size_prop)
{
  GimpViewType view_type;
  GimpViewSize view_size;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (pattern_box_new (container, context, spacing,
                                              view_type, view_size),
                             context,
                             view_type_prop, view_size_prop);
}


/*  gradient boxes  */

static GtkWidget *
gradient_box_new (GimpContainer *container,
                  GimpContext   *context,
                  gint           spacing,
                  GimpViewType   view_type,
                  GimpViewSize   view_size,
                  const gchar   *reverse_prop)
{
  GtkWidget *hbox;
  GtkWidget *button;

  if (! container)
    container = context->gimp->gradient_factory->container;

  hbox = gtk_hbox_new (FALSE, spacing);

  button = gimp_viewable_button_new (container, context,
                                     view_type,
                                     GIMP_VIEW_SIZE_LARGE, view_size, 1,
                                     gimp_dialog_factory_from_name ("dock"),
                                     "gimp-gradient-list|gimp-gradient-grid",
                                     GIMP_STOCK_GRADIENT,
                                     _("Open the gradient selection dialog"));
  GIMP_VIEWABLE_BUTTON (button)->button_view_size = GIMP_VIEW_SIZE_SMALL;

  g_object_set_data (G_OBJECT (hbox), "viewable-button", button);

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  if (reverse_prop)
    {
      GtkWidget *toggle;
      GtkWidget *view;
      GtkWidget *image;
      gchar     *signal_name;

      toggle = gimp_prop_check_button_new (G_OBJECT (context), reverse_prop,
                                           NULL);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
      gtk_widget_show (toggle);

      gimp_help_set_help_data (toggle, _("Reverse"), NULL);

      image = gtk_image_new_from_stock (GIMP_STOCK_FLIP_HORIZONTAL,
                                        GTK_ICON_SIZE_BUTTON);
      gtk_container_add (GTK_CONTAINER (toggle), image);
      gtk_widget_show (image);

      view = GTK_BIN (button)->child;

      signal_name = g_strconcat ("notify::", reverse_prop, NULL);
      g_signal_connect_object (context, signal_name,
                               G_CALLBACK (gimp_gradient_box_reverse_notify),
                               G_OBJECT (view), 0);
      g_free (signal_name);

      gimp_gradient_box_reverse_notify (G_OBJECT (context),
                                        NULL,
                                        GIMP_VIEW (view));
    }

  return hbox;
}

GtkWidget *
gimp_gradient_box_new (GimpContainer *container,
                       GimpContext   *context,
                       gint           spacing,
                       const gchar   *reverse_prop)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return gradient_box_new (container, context, spacing,
                           GIMP_VIEW_TYPE_LIST, GIMP_VIEW_SIZE_LARGE,
                           reverse_prop);
}

GtkWidget *
gimp_prop_gradient_box_new (GimpContainer *container,
                            GimpContext   *context,
                            gint           spacing,
                            const gchar   *view_type_prop,
                            const gchar   *view_size_prop,
                            const gchar   *reverse_prop)
{
  GimpViewType view_type;
  GimpViewSize view_size;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (gradient_box_new (container, context, spacing,
                                               view_type, view_size,
                                               reverse_prop),
                             context,
                             view_type_prop, view_size_prop);
}


/*  palette boxes  */

static GtkWidget *
palette_box_new (GimpContainer *container,
                 GimpContext   *context,
                 gint           spacing,
                 GimpViewType   view_type,
                 GimpViewSize   view_size)
{
  if (! container)
    container = context->gimp->palette_factory->container;

  return gimp_viewable_box_new (container, context, spacing,
                                view_type, GIMP_VIEW_SIZE_MEDIUM, view_size,
                                "gimp-palette-list|gimp-palette-grid",
                                GIMP_STOCK_PALETTE,
                                _("Open the palette selection dialog"));
}

GtkWidget *
gimp_palette_box_new (GimpContainer *container,
                      GimpContext   *context,
                      gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return palette_box_new (container, context, spacing,
                          GIMP_VIEW_TYPE_LIST, GIMP_VIEW_SIZE_MEDIUM);
}

GtkWidget *
gimp_prop_palette_box_new (GimpContainer *container,
                           GimpContext   *context,
                           gint           spacing,
                           const gchar   *view_type_prop,
                           const gchar   *view_size_prop)
{
  GimpViewType view_type;
  GimpViewSize view_size;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (palette_box_new (container, context, spacing,
                                              view_type, view_size),
                             context,
                             view_type_prop, view_size_prop);
}


/*  font boxes  */

static GtkWidget *
font_box_new (GimpContainer *container,
              GimpContext   *context,
              gint           spacing,
              GimpViewType   view_type,
              GimpViewSize   view_size)
{
  if (! container)
    container = context->gimp->fonts;

  return gimp_viewable_box_new (container, context, spacing,
                                view_type, GIMP_VIEW_SIZE_SMALL, view_size,
                                "gimp-font-list|gimp-font-grid",
                                GIMP_STOCK_FONT,
                                _("Open the font selection dialog"));
}

GtkWidget *
gimp_font_box_new (GimpContainer *container,
                   GimpContext   *context,
                   gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return font_box_new (container, context, spacing,
                       GIMP_VIEW_TYPE_LIST, GIMP_VIEW_SIZE_SMALL);
}

GtkWidget *
gimp_prop_font_box_new (GimpContainer *container,
                        GimpContext   *context,
                        gint           spacing,
                        const gchar   *view_type_prop,
                        const gchar   *view_size_prop)
{
  GimpViewType view_type;
  GimpViewSize view_size;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  g_object_get (context,
                view_type_prop, &view_type,
                view_size_prop, &view_size,
                NULL);

  return view_props_connect (font_box_new (container, context, spacing,
                                           view_type, view_size),
                             context,
                             view_type_prop, view_size_prop);
}


/*  private functions  */

static GtkWidget *
gimp_viewable_box_new (GimpContainer *container,
                       GimpContext   *context,
                       gint           spacing,
                       GimpViewType   view_type,
                       GimpViewType   button_view_size,
                       GimpViewSize   view_size,
                       const gchar   *dialog_identifier,
                       const gchar   *dialog_stock_id,
                       const gchar   *dialog_tooltip)
{
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *entry;

  hbox = gtk_hbox_new (FALSE, spacing);

  button = gimp_viewable_button_new (container, context,
                                     view_type, button_view_size, view_size, 1,
                                     gimp_dialog_factory_from_name ("dock"),
                                     dialog_identifier,
                                     dialog_stock_id,
                                     dialog_tooltip);

  g_object_set_data (G_OBJECT (hbox), "viewable-button", button);

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  entry = gimp_container_entry_new (container, context, view_size, 1);

  /*  set a silly smally size request on the entry to disable
   *  GtkEntry's minimal width of 150 pixels.
   */
  gtk_widget_set_size_request (entry, 10, -1);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  return hbox;
}

static GtkWidget *
view_props_connect (GtkWidget   *box,
                    GimpContext *context,
                    const gchar *view_type_prop,
                    const gchar *view_size_prop)
{
  GtkWidget *button = g_object_get_data (G_OBJECT (box), "viewable-button");

  gimp_config_connect_full (G_OBJECT (context), G_OBJECT (button),
                            view_type_prop, "popup-view-type");
  gimp_config_connect_full (G_OBJECT (context), G_OBJECT (button),
                            view_size_prop, "popup-view-size");

  return box;
}

static void
gimp_gradient_box_reverse_notify (GObject    *object,
                                  GParamSpec *pspec,
                                  GimpView   *view)
{
  GimpViewRendererGradient *rendergrad;
  gboolean                  reverse;

  rendergrad = GIMP_VIEW_RENDERER_GRADIENT (view->renderer);

  g_object_get (object, "gradient-reverse", &reverse, NULL);

  gimp_view_renderer_gradient_set_reverse (rendergrad, reverse);
}
