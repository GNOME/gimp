/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
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
#include "gimpwidgets-utils.h"
#include "gimpwindowstrategy.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GtkWidget * gimp_viewable_box_new       (GimpContainer *container,
                                                GimpContext   *context,
                                                const gchar   *label,
                                                gint           spacing,
                                                GimpViewType   view_type,
                                                GimpViewSize   button_view_size,
                                                GimpViewSize   view_size,
                                                const gchar   *dialog_identifier,
                                                const gchar   *dialog_icon_name,
                                                const gchar   *dialog_tooltip,
                                                const gchar   *editor_id,
                                                const gchar   *editor_tooltip);
static GtkWidget * view_props_connect          (GtkWidget     *box,
                                                GimpContext   *context,
                                                const gchar   *view_type_prop,
                                                const gchar   *view_size_prop);
static void   gimp_viewable_box_edit_clicked   (GtkWidget          *widget,
                                                GimpViewableButton *button);
static void   gimp_gradient_box_reverse_notify (GObject       *object,
                                                GParamSpec    *pspec,
                                                GimpView      *view);
static void   gimp_gradient_box_blend_notify   (GObject       *object,
                                                GParamSpec    *pspec,
                                                GimpView      *view);


/*  brush boxes  */

static GtkWidget *
brush_box_new (GimpContainer *container,
               GimpContext   *context,
               const gchar   *label,
               gint           spacing,
               GimpViewType   view_type,
               GimpViewSize   view_size,
               const gchar   *editor_id,
               const gchar   *editor_tooltip)
{
  if (! container)
    container = gimp_data_factory_get_container (context->gimp->brush_factory);

  return gimp_viewable_box_new (container, context, label, spacing,
                                view_type, GIMP_VIEW_SIZE_SMALL, view_size,
                                "gimp-brush-grid|gimp-brush-list",
                                GIMP_ICON_BRUSH,
                                _("Open the brush selection dialog"),
                                editor_id, editor_tooltip);
}

GtkWidget *
gimp_brush_box_new (GimpContainer *container,
                    GimpContext   *context,
                    const gchar   *label,
                    gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return brush_box_new (container, context, label, spacing,
                        GIMP_VIEW_TYPE_GRID, GIMP_VIEW_SIZE_SMALL,
                        NULL, NULL);
}

GtkWidget *
gimp_prop_brush_box_new (GimpContainer *container,
                         GimpContext   *context,
                         const gchar   *label,
                         gint           spacing,
                         const gchar   *view_type_prop,
                         const gchar   *view_size_prop,
                         const gchar   *editor_id,
                         const gchar   *editor_tooltip)
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

  return view_props_connect (brush_box_new (container, context, label, spacing,
                                            view_type, view_size,
                                            editor_id, editor_tooltip),
                             context,
                             view_type_prop, view_size_prop);
}


/*  dynamics boxes  */

static GtkWidget *
dynamics_box_new (GimpContainer *container,
                  GimpContext   *context,
                  const gchar   *label,
                  gint           spacing,
                  GimpViewType   view_type,
                  GimpViewSize   view_size,
                  const gchar   *editor_id,
                  const gchar   *editor_tooltip)
{
  if (! container)
    container = gimp_data_factory_get_container (context->gimp->dynamics_factory);

  return gimp_viewable_box_new (container, context, label, spacing,
                                view_type, GIMP_VIEW_SIZE_SMALL, view_size,
                                "gimp-dynamics-list|gimp-dynamics-grid",
                                GIMP_ICON_DYNAMICS,
                                _("Open the dynamics selection dialog"),
                                editor_id, editor_tooltip);
}

GtkWidget *
gimp_dynamics_box_new (GimpContainer *container,
                       GimpContext   *context,
                       const gchar   *label,
                       gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return dynamics_box_new (container, context, label, spacing,
                           GIMP_VIEW_TYPE_LIST, GIMP_VIEW_SIZE_SMALL,
                           NULL, NULL);
}

GtkWidget *
gimp_prop_dynamics_box_new (GimpContainer *container,
                            GimpContext   *context,
                            const gchar   *label,
                            gint           spacing,
                            const gchar   *view_type_prop,
                            const gchar   *view_size_prop,
                            const gchar   *editor_id,
                            const gchar   *editor_tooltip)
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

  return view_props_connect (dynamics_box_new (container, context, label,
                                               spacing,
                                               view_type, view_size,
                                               editor_id, editor_tooltip),
                             context,
                             view_type_prop, view_size_prop);
}


/*  brush boxes  */

static GtkWidget *
mybrush_box_new (GimpContainer *container,
                 GimpContext   *context,
                 const gchar   *label,
                 gint           spacing,
                 GimpViewType   view_type,
                 GimpViewSize   view_size)
{
  if (! container)
    container = gimp_data_factory_get_container (context->gimp->mybrush_factory);

  return gimp_viewable_box_new (container, context, label, spacing,
                                view_type, GIMP_VIEW_SIZE_LARGE, view_size,
                                "gimp-mypaint-brush-grid|gimp-mypaint-brush-list",
                                GIMP_ICON_BRUSH,
                                _("Open the MyPaint brush selection dialog"),
                                NULL, NULL);
}

GtkWidget *
gimp_mybrush_box_new (GimpContainer *container,
                      GimpContext   *context,
                      const gchar   *label,
                      gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return mybrush_box_new (container, context, label, spacing,
                          GIMP_VIEW_TYPE_GRID, GIMP_VIEW_SIZE_LARGE);
}

GtkWidget *
gimp_prop_mybrush_box_new (GimpContainer *container,
                           GimpContext   *context,
                           const gchar   *label,
                           gint           spacing,
                           const gchar   *view_type_prop,
                           const gchar   *view_size_prop)
{
  GimpViewType view_type = GIMP_VIEW_TYPE_GRID;
  GimpViewSize view_size = GIMP_VIEW_SIZE_LARGE;

  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (view_type_prop && view_size_prop)
    g_object_get (context,
                  view_type_prop, &view_type,
                  view_size_prop, &view_size,
                  NULL);

  return view_props_connect (mybrush_box_new (container, context, label, spacing,
                                              view_type, view_size),
                             context,
                             view_type_prop, view_size_prop);
}


/*  pattern boxes  */

static GtkWidget *
pattern_box_new (GimpContainer *container,
                 GimpContext   *context,
                 const gchar   *label,
                 gint           spacing,
                 GimpViewType   view_type,
                 GimpViewSize   view_size)
{
  if (! container)
    container = gimp_data_factory_get_container (context->gimp->pattern_factory);

  return gimp_viewable_box_new (container, context, label, spacing,
                                view_type, GIMP_VIEW_SIZE_SMALL, view_size,
                                "gimp-pattern-grid|gimp-pattern-list",
                                GIMP_ICON_PATTERN,
                                _("Open the pattern selection dialog"),
                                NULL, NULL);
}

GtkWidget *
gimp_pattern_box_new (GimpContainer *container,
                      GimpContext   *context,
                      const gchar   *label,
                      gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return pattern_box_new (container, context, label, spacing,
                          GIMP_VIEW_TYPE_GRID, GIMP_VIEW_SIZE_SMALL);
}

GtkWidget *
gimp_prop_pattern_box_new (GimpContainer *container,
                           GimpContext   *context,
                           const gchar   *label,
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

  return view_props_connect (pattern_box_new (container, context, label, spacing,
                                              view_type, view_size),
                             context,
                             view_type_prop, view_size_prop);
}


/*  gradient boxes  */

static GtkWidget *
gradient_box_new (GimpContainer *container,
                  GimpContext   *context,
                  const gchar   *label,
                  gint           spacing,
                  GimpViewType   view_type,
                  GimpViewSize   view_size,
                  const gchar   *reverse_prop,
                  const gchar   *blend_color_space_prop,
                  const gchar   *editor_id,
                  const gchar   *editor_tooltip)
{
  GtkWidget *hbox;
  GtkWidget *button;
  GList     *children;

  if (! container)
    container = gimp_data_factory_get_container (context->gimp->gradient_factory);

  hbox = gimp_viewable_box_new (container, context, label, spacing,
                                view_type, GIMP_VIEW_SIZE_SMALL, view_size,
                                "gimp-gradient-list|gimp-gradient-grid",
                                GIMP_ICON_GRADIENT,
                                _("Open the gradient selection dialog"),
                                editor_id, editor_tooltip);

  children = gtk_container_get_children (GTK_CONTAINER (hbox));
  button = children->data;
  g_list_free (children);

  GIMP_VIEWABLE_BUTTON (button)->button_view_size = GIMP_VIEW_SIZE_SMALL;

  if (reverse_prop)
    {
      GtkWidget *toggle;
      GtkWidget *view;
      GtkWidget *image;
      gchar     *signal_name;

      toggle = gimp_prop_check_button_new (G_OBJECT (context), reverse_prop,
                                           NULL);
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (toggle), FALSE);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (hbox), toggle, 1);
      gtk_widget_show (toggle);

      gimp_help_set_help_data (toggle, _("Reverse"), NULL);

      image = gtk_image_new_from_icon_name (GIMP_ICON_OBJECT_FLIP_HORIZONTAL,
                                            GTK_ICON_SIZE_MENU);
      /* gimp_prop_check_button_new() adds the property nick as label of
       * the button by default. */
      gtk_container_remove (GTK_CONTAINER (toggle),
                            gtk_bin_get_child (GTK_BIN (toggle)));
      gtk_container_add (GTK_CONTAINER (toggle), image);
      gtk_widget_show (image);

      view = gtk_bin_get_child (GTK_BIN (button));

      signal_name = g_strconcat ("notify::", reverse_prop, NULL);
      g_signal_connect_object (context, signal_name,
                               G_CALLBACK (gimp_gradient_box_reverse_notify),
                               G_OBJECT (view), 0);
      g_free (signal_name);

      gimp_gradient_box_reverse_notify (G_OBJECT (context),
                                        NULL,
                                        GIMP_VIEW (view));
    }

  if (blend_color_space_prop)
    {
      GtkWidget *view;
      gchar     *signal_name;

      view = gtk_bin_get_child (GTK_BIN (button));

      signal_name = g_strconcat ("notify::", blend_color_space_prop, NULL);
      g_signal_connect_object (context, signal_name,
                               G_CALLBACK (gimp_gradient_box_blend_notify),
                               G_OBJECT (view), 0);
      g_free (signal_name);

      gimp_gradient_box_blend_notify (G_OBJECT (context),
                                      NULL,
                                      GIMP_VIEW (view));
    }

  return hbox;
}

GtkWidget *
gimp_gradient_box_new (GimpContainer *container,
                       GimpContext   *context,
                       const gchar   *label,
                       gint           spacing,
                       const gchar   *reverse_prop,
                       const gchar   *blend_color_space_prop)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return gradient_box_new (container, context, label, spacing,
                           GIMP_VIEW_TYPE_LIST, GIMP_VIEW_SIZE_LARGE,
                           reverse_prop, blend_color_space_prop,
                           NULL, NULL);
}

GtkWidget *
gimp_prop_gradient_box_new (GimpContainer *container,
                            GimpContext   *context,
                            const gchar   *label,
                            gint           spacing,
                            const gchar   *view_type_prop,
                            const gchar   *view_size_prop,
                            const gchar   *reverse_prop,
                            const gchar   *blend_color_space_prop,
                            const gchar   *editor_id,
                            const gchar   *editor_tooltip)
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

  return view_props_connect (gradient_box_new (container, context, label, spacing,
                                               view_type, view_size,
                                               reverse_prop,
                                               blend_color_space_prop,
                                               editor_id, editor_tooltip),
                             context,
                             view_type_prop, view_size_prop);
}


/*  palette boxes  */

static GtkWidget *
palette_box_new (GimpContainer *container,
                 GimpContext   *context,
                 const gchar   *label,
                 gint           spacing,
                 GimpViewType   view_type,
                 GimpViewSize   view_size,
                 const gchar   *editor_id,
                 const gchar   *editor_tooltip)
{
  if (! container)
    container = gimp_data_factory_get_container (context->gimp->palette_factory);

  return gimp_viewable_box_new (container, context, label, spacing,
                                view_type, GIMP_VIEW_SIZE_MEDIUM, view_size,
                                "gimp-palette-list|gimp-palette-grid",
                                GIMP_ICON_PALETTE,
                                _("Open the palette selection dialog"),
                                editor_id, editor_tooltip);
}

GtkWidget *
gimp_palette_box_new (GimpContainer *container,
                      GimpContext   *context,
                      const gchar   *label,
                      gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return palette_box_new (container, context, label, spacing,
                          GIMP_VIEW_TYPE_LIST, GIMP_VIEW_SIZE_MEDIUM,
                          NULL, NULL);
}

GtkWidget *
gimp_prop_palette_box_new (GimpContainer *container,
                           GimpContext   *context,
                           const gchar   *label,
                           gint           spacing,
                           const gchar   *view_type_prop,
                           const gchar   *view_size_prop,
                           const gchar   *editor_id,
                           const gchar   *editor_tooltip)
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

  return view_props_connect (palette_box_new (container, context, label, spacing,
                                              view_type, view_size,
                                              editor_id, editor_tooltip),
                             context,
                             view_type_prop, view_size_prop);
}


/*  font boxes  */

static GtkWidget *
font_box_new (GimpContainer *container,
              GimpContext   *context,
              const gchar   *label,
              gint           spacing,
              GimpViewType   view_type,
              GimpViewSize   view_size)
{
  if (! container)
    container = gimp_data_factory_get_container (context->gimp->font_factory);

  return gimp_viewable_box_new (container, context, label, spacing,
                                view_type, GIMP_VIEW_SIZE_SMALL, view_size,
                                "gimp-font-list|gimp-font-grid",
                                GIMP_ICON_FONT,
                                _("Open the font selection dialog"),
                                NULL, NULL);
}

GtkWidget *
gimp_font_box_new (GimpContainer *container,
                   GimpContext   *context,
                   const gchar   *label,
                   gint           spacing)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return font_box_new (container, context, label, spacing,
                       GIMP_VIEW_TYPE_LIST, GIMP_VIEW_SIZE_SMALL);
}

GtkWidget *
gimp_prop_font_box_new (GimpContainer *container,
                        GimpContext   *context,
                        const gchar   *label,
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

  return view_props_connect (font_box_new (container, context, label, spacing,
                                           view_type, view_size),
                             context,
                             view_type_prop, view_size_prop);
}


/*  private functions  */

static GtkWidget *
gimp_viewable_box_new (GimpContainer *container,
                       GimpContext   *context,
                       const gchar   *label,
                       gint           spacing,
                       GimpViewType   view_type,
                       GimpViewSize   button_view_size,
                       GimpViewSize   view_size,
                       const gchar   *dialog_identifier,
                       const gchar   *dialog_icon_name,
                       const gchar   *dialog_tooltip,
                       const gchar   *editor_id,
                       const gchar   *editor_tooltip)
{
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *vbox;
  GtkWidget *l;
  GtkWidget *entry;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, spacing);

  button = gimp_viewable_button_new (container, context,
                                     view_type, button_view_size, view_size, 1,
                                     gimp_dialog_factory_get_singleton (),
                                     dialog_identifier,
                                     dialog_icon_name,
                                     dialog_tooltip);

  gimp_view_renderer_set_size_full (GIMP_VIEW (GIMP_VIEWABLE_BUTTON (button)->view)->renderer,
                                    button_view_size, button_view_size, 1);

  g_object_set_data (G_OBJECT (hbox), "viewable-button", button);

  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  if (label)
    {
      l = gtk_label_new_with_mnemonic (label);
      gtk_label_set_xalign (GTK_LABEL (l), 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), l, FALSE, FALSE, 0);
      gtk_widget_show (l);
    }

  entry = gimp_container_entry_new (container, context, view_size, 1);

  /*  set a silly smally size request on the entry to disable
   *  GtkEntry's minimal width of 150 pixels.
   */
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 4);
  gtk_box_pack_end (GTK_BOX (vbox), entry, label ? FALSE : TRUE, FALSE, 0);
  gtk_widget_show (entry);

  if (editor_id)
    {
      GtkWidget *edit_button;
      GtkWidget *image;

      edit_button = gtk_button_new ();
      gtk_button_set_relief (GTK_BUTTON (edit_button), GTK_RELIEF_NONE);
      gtk_box_pack_end (GTK_BOX (hbox), edit_button, FALSE, FALSE, 0);
      gtk_widget_show (edit_button);

      if (editor_tooltip)
        gimp_help_set_help_data (edit_button, editor_tooltip, NULL);

      image = gtk_image_new_from_icon_name (GIMP_ICON_EDIT,
                                            GTK_ICON_SIZE_BUTTON);
      gtk_misc_set_alignment (GTK_MISC (image), 0.5, 1.0);
      gtk_container_add (GTK_CONTAINER (edit_button), image);
      gtk_widget_show (image);

      g_object_set_data_full (G_OBJECT (button),
                              "gimp-viewable-box-editor",
                              g_strdup (editor_id),
                              (GDestroyNotify) g_free);

      g_signal_connect (edit_button, "clicked",
                        G_CALLBACK (gimp_viewable_box_edit_clicked),
                        button);
    }

  return hbox;
}

static GtkWidget *
view_props_connect (GtkWidget   *box,
                    GimpContext *context,
                    const gchar *view_type_prop,
                    const gchar *view_size_prop)
{
  GtkWidget *button = g_object_get_data (G_OBJECT (box), "viewable-button");

  if (view_type_prop)
    gimp_config_connect_full (G_OBJECT (context), G_OBJECT (button),
                              view_type_prop, "popup-view-type");

  if (view_size_prop)
    gimp_config_connect_full (G_OBJECT (context), G_OBJECT (button),
                              view_size_prop, "popup-view-size");

  return box;
}

static void
gimp_viewable_box_edit_clicked (GtkWidget          *widget,
                                GimpViewableButton *button)
{
  const gchar *editor_id = g_object_get_data (G_OBJECT (button),
                                              "gimp-viewable-box-editor");

  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (button->context->gimp)),
                                             button->context->gimp,
                                             gimp_dialog_factory_get_singleton (),
                                             gtk_widget_get_screen (widget),
                                             gimp_widget_get_monitor (widget),
                                             editor_id);
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

static void
gimp_gradient_box_blend_notify (GObject    *object,
                                GParamSpec *pspec,
                                GimpView   *view)
{
  GimpViewRendererGradient    *rendergrad;
  GimpGradientBlendColorSpace  blend_color_space;

  rendergrad = GIMP_VIEW_RENDERER_GRADIENT (view->renderer);

  g_object_get (object,
                "gradient-blend-color-space", &blend_color_space,
                NULL);

  gimp_view_renderer_gradient_set_blend_color_space (rendergrad,
                                                     blend_color_space);
}
