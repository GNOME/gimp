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

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpdatafactory.h"
#include "core/gimptooloptions.h"

#include "widgets/gimpcontainerentry.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpview.h"
#include "widgets/gimpviewablebutton.h"
#include "widgets/gimpviewrenderergradient.h"

#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void gimp_tool_options_gradient_reverse_notify (GimpToolOptions *tool_options,
                                                       GParamSpec      *pspec,
                                                       GimpView        *view);


/*  public functions  */

GtkWidget *
gimp_tool_options_gui (GimpToolOptions *tool_options)
{
  GtkWidget *vbox;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), NULL);

  vbox = gtk_vbox_new (FALSE, 6);

  return vbox;
}

GtkWidget *
gimp_tool_options_brush_box_new (GimpToolOptions *tool_options)
{
  GimpContext       *context = GIMP_CONTEXT (tool_options);
  GimpDialogFactory *dialog_factory;
  GtkWidget         *hbox;
  GtkWidget         *button;
  GtkWidget         *entry;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), NULL);

  dialog_factory = gimp_dialog_factory_from_name ("dock");

  hbox = gtk_hbox_new (FALSE, 2);

  button = gimp_viewable_button_new (context->gimp->brush_factory->container,
                                     context,
                                     GIMP_VIEW_SIZE_SMALL, 1,
                                     dialog_factory,
                                     "gimp-brush-grid|gimp-brush-list",
                                     GIMP_STOCK_BRUSH,
                                     _("Open the brush selection dialog"));

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  entry = gimp_container_entry_new (context->gimp->brush_factory->container,
                                    context,
                                    GIMP_VIEW_SIZE_SMALL, 1);
  /*  set a silly smally size request on the entry to disable
   *  GtkEntry's minimal width of 150 pixels.
   */
  gtk_widget_set_size_request (entry, 10, -1);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  return hbox;
}

GtkWidget *
gimp_tool_options_pattern_box_new (GimpToolOptions *tool_options)
{
  GimpContext       *context = GIMP_CONTEXT (tool_options);
  GimpDialogFactory *dialog_factory;
  GtkWidget         *hbox;
  GtkWidget         *button;
  GtkWidget         *entry;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), NULL);

  dialog_factory = gimp_dialog_factory_from_name ("dock");

  hbox = gtk_hbox_new (FALSE, 2);

  button = gimp_viewable_button_new (context->gimp->pattern_factory->container,
                                     context,
                                     GIMP_VIEW_SIZE_SMALL, 1,
                                     dialog_factory,
                                     "gimp-pattern-grid|gimp-pattern-list",
                                     GIMP_STOCK_PATTERN,
                                     _("Open the pattern selection dialog"));

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  entry = gimp_container_entry_new (context->gimp->pattern_factory->container,
                                    context,
                                    GIMP_VIEW_SIZE_SMALL, 1);
  /*  set a silly smally size request on the entry to disable
   *  GtkEntry's minimal width of 150 pixels.
   */
  gtk_widget_set_size_request (entry, 10, -1);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  return hbox;
}

GtkWidget *
gimp_tool_options_gradient_box_new (GimpToolOptions *tool_options)
{
  GObject           *config  = G_OBJECT (tool_options);
  GimpContext       *context = GIMP_CONTEXT (tool_options);
  GimpDialogFactory *dialog_factory;
  GtkWidget         *hbox;
  GtkWidget         *button;
  GtkWidget         *toggle;
  GtkWidget         *preview;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), NULL);

  dialog_factory = gimp_dialog_factory_from_name ("dock");

  hbox = gtk_hbox_new (FALSE, 2);

  button = gimp_viewable_button_new (context->gimp->gradient_factory->container,
                                     context,
                                     GIMP_VIEW_SIZE_LARGE, 1,
                                     dialog_factory,
                                     "gimp-gradient-list|gimp-gradient-grid",
                                     GIMP_STOCK_GRADIENT,
                                     _("Open the gradient selection dialog"));
  GIMP_VIEWABLE_BUTTON (button)->preview_size = GIMP_VIEW_SIZE_SMALL;
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  toggle = gimp_prop_check_button_new (config, "gradient-reverse",
                                       _("Reverse"));
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  gtk_widget_show (toggle);

  preview = GTK_BIN (button)->child;

  g_signal_connect_object (config, "notify::gradient-reverse",
                           G_CALLBACK (gimp_tool_options_gradient_reverse_notify),
                           G_OBJECT (preview), 0);

  gimp_tool_options_gradient_reverse_notify (tool_options,
                                             NULL,
                                             GIMP_VIEW (preview));

  return hbox;
}

GtkWidget *
gimp_tool_options_font_box_new (GimpToolOptions *tool_options)
{
  GimpContext       *context = GIMP_CONTEXT (tool_options);
  GimpDialogFactory *dialog_factory;
  GtkWidget         *hbox;
  GtkWidget         *button;
  GtkWidget         *entry;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), NULL);

  dialog_factory = gimp_dialog_factory_from_name ("dock");

  hbox = gtk_hbox_new (FALSE, 2);

  button = gimp_viewable_button_new (context->gimp->fonts,
                                     context,
                                     GIMP_VIEW_SIZE_SMALL, 1,
                                     dialog_factory,
                                     "gimp-font-list|gimp-font-grid",
                                     GIMP_STOCK_FONT,
                                     _("Open the font selection dialog"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  entry = gimp_container_entry_new (context->gimp->fonts,
                                    context,
                                    GIMP_VIEW_SIZE_SMALL, 1);
  /*  set a silly smally size request on the entry to disable
   *  GtkEntry's minimal width of 150 pixels.
   */
  gtk_widget_set_size_request (entry, 10, -1);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  return hbox;
}

void
gimp_tool_options_radio_frame_add_box (GtkWidget *frame,
                                       GtkWidget *hbox,
                                       gint       enum_value)
{
  GtkWidget *radio;
  GtkWidget *spacer;
  gint       indicator_size;
  gint       indicator_spacing;
  gint       focus_width;
  gint       focus_padding;
  GSList    *list;

  radio = g_object_get_data (G_OBJECT (frame), "radio-button");
  gtk_widget_style_get (radio,
                        "indicator-size",    &indicator_size,
                        "indicator-spacing", &indicator_spacing,
                        "focus-line-width",  &focus_width,
                        "focus-padding",     &focus_padding,
                        NULL);

  spacer = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_size_request (spacer,
                               indicator_size +
                               3 * indicator_spacing +
                               focus_width +
                               focus_padding +
                               GTK_CONTAINER (radio)->border_width -
                               gtk_box_get_spacing (GTK_BOX (hbox)),
                               -1);
  gtk_box_pack_start (GTK_BOX (hbox), spacer, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (hbox), spacer, 0);
  gtk_widget_show (spacer);

  for (list = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
       list;
       list = g_slist_next (list))
    {
      if (GPOINTER_TO_INT (g_object_get_data (list->data, "gimp-item-data")) ==
          enum_value)
        {
          g_object_set_data (list->data, "set_sensitive", hbox);
          g_signal_connect (list->data, "toggled",
                            G_CALLBACK (gimp_toggle_button_sensitive_update),
                            NULL);

          gtk_widget_set_sensitive (hbox,
                                    GTK_TOGGLE_BUTTON (list->data)->active);

          break;
        }
    }

  gtk_box_pack_start (GTK_BOX (GTK_BIN (frame)->child), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
}


/*  private functions  */

static void
gimp_tool_options_gradient_reverse_notify (GimpToolOptions *tool_options,
                                           GParamSpec      *pspec,
                                           GimpView        *view)
{
  GimpViewRendererGradient *rendergrad;
  gboolean                  reverse;

  rendergrad = GIMP_VIEW_RENDERER_GRADIENT (view->renderer);

  g_object_get (tool_options, "gradient-reverse", &reverse, NULL);

  gimp_view_renderer_gradient_set_reverse (rendergrad, reverse);
}
