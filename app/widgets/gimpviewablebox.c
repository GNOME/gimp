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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

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

static GtkWidget      * gimp_viewable_box_new  (GimpContainer *container,
                                                GimpContext   *context,
                                                gint           spacing,
                                                GimpViewType   view_type,
                                                GimpViewSize   view_size,
                                                const gchar   *dialog_identifier,
                                                const gchar   *dialog_stock_id,
                                                const gchar   *dialog_tooltip);
static void   gimp_gradient_box_reverse_notify (GObject       *object,
                                                GParamSpec    *pspec,
                                                GimpView      *view);


/*  public functions  */

GtkWidget *
gimp_brush_box_new (GimpContainer *container,
                    GimpContext   *context,
                    gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (! container)
    container = context->gimp->brush_factory->container;

  return gimp_viewable_box_new (container, context, spacing,
                                GIMP_VIEW_TYPE_GRID, GIMP_VIEW_SIZE_SMALL,
                                "gimp-brush-grid|gimp-brush-list",
                                GIMP_STOCK_BRUSH,
                                _("Open the brush selection dialog"));
}

GtkWidget *
gimp_pattern_box_new (GimpContainer *container,
                      GimpContext   *context,
                      gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (! container)
    container = context->gimp->pattern_factory->container;

  return gimp_viewable_box_new (container, context, spacing,
                                GIMP_VIEW_TYPE_GRID, GIMP_VIEW_SIZE_SMALL,
                                "gimp-pattern-grid|gimp-pattern-list",
                                GIMP_STOCK_PATTERN,
                                _("Open the pattern selection dialog"));
}

GtkWidget *
gimp_gradient_box_new (GimpContainer *container,
                       GimpContext   *context,
                       const gchar   *reverse_prop,
                       gint           spacing)
{
  GtkWidget *hbox;
  GtkWidget *button;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (! container)
    container = context->gimp->gradient_factory->container;

  hbox = gtk_hbox_new (FALSE, spacing);

  button = gimp_viewable_button_new (container, context,
                                     GIMP_VIEW_TYPE_LIST,
                                     GIMP_VIEW_SIZE_LARGE, 1,
                                     gimp_dialog_factory_from_name ("dock"),
                                     "gimp-gradient-list|gimp-gradient-grid",
                                     GIMP_STOCK_GRADIENT,
                                     _("Open the gradient selection dialog"));
  GIMP_VIEWABLE_BUTTON (button)->preview_size = GIMP_VIEW_SIZE_SMALL;
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  if (reverse_prop)
    {
      GtkWidget *toggle;
      GtkWidget *preview;
      gchar     *signal_name;

      toggle = gimp_prop_check_button_new (G_OBJECT (context), reverse_prop,
                                           _("Reverse"));
      gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
      gtk_widget_show (toggle);

      preview = GTK_BIN (button)->child;

      signal_name = g_strconcat ("notify::", reverse_prop, NULL);
      g_signal_connect_object (context, signal_name,
                               G_CALLBACK (gimp_gradient_box_reverse_notify),
                               G_OBJECT (preview), 0);
      g_free (signal_name);

      gimp_gradient_box_reverse_notify (G_OBJECT (context),
                                        NULL,
                                        GIMP_VIEW (preview));
    }

  return hbox;
}

GtkWidget *
gimp_palette_box_new (GimpContainer *container,
                      GimpContext   *context,
                      gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (! container)
    container = context->gimp->palette_factory->container;

  return gimp_viewable_box_new (container, context, spacing,
                                GIMP_VIEW_TYPE_LIST, GIMP_VIEW_SIZE_MEDIUM,
                                "gimp-palette-list|gimp-palette-grid",
                                GIMP_STOCK_PALETTE,
                                _("Open the palette selection dialog"));
}

GtkWidget *
gimp_font_box_new (GimpContainer *container,
                   GimpContext   *context,
                   gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (! container)
    container = context->gimp->fonts;

  return gimp_viewable_box_new (container, context, spacing,
                                GIMP_VIEW_TYPE_LIST, GIMP_VIEW_SIZE_SMALL,
                                "gimp-font-list|gimp-font-grid",
                                GIMP_STOCK_FONT,
                                _("Open the font selection dialog"));
}


/*  private functions  */

static GtkWidget *
gimp_viewable_box_new (GimpContainer *container,
                       GimpContext   *context,
                       gint           spacing,
                       GimpViewType   view_type,
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
                                     view_type, view_size, 1,
                                     gimp_dialog_factory_from_name ("dock"),
                                     dialog_identifier,
                                     dialog_stock_id,
                                     dialog_tooltip);

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
