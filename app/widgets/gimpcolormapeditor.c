/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-colormap.h"
#include "core/ligmapalette.h"
#include "core/ligmaprojection.h"

#include "ligmacolordialog.h"
#include "ligmacolormapeditor.h"
#include "ligmacolormapselection.h"
#include "ligmadialogfactory.h"
#include "ligmadocked.h"
#include "ligmamenufactory.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


static void ligma_colormap_editor_docked_iface_init (LigmaDockedInterface  *face);

static void   ligma_colormap_editor_constructed     (GObject              *object);
static void   ligma_colormap_editor_dispose         (GObject              *object);

static void   ligma_colormap_editor_unmap           (GtkWidget            *widget);

static void   ligma_colormap_editor_set_context     (LigmaDocked           *docked,
                                                    LigmaContext          *context);

static void   ligma_colormap_editor_color_update    (LigmaColorDialog      *dialog,
                                                    const LigmaRGB        *color,
                                                    LigmaColorDialogState  state,
                                                    LigmaColormapEditor   *editor);

static gboolean   ligma_colormap_editor_entry_button_press (GtkWidget     *widget,
                                                           GdkEvent      *event,
                                                           gpointer       user_data);
static gboolean   ligma_colormap_editor_entry_popup     (GtkWidget            *widget,
                                                        gpointer              user_data);
static void   ligma_colormap_editor_color_clicked   (LigmaColormapEditor   *editor,
                                                    LigmaPaletteEntry     *entry,
                                                    GdkModifierType       state);

G_DEFINE_TYPE_WITH_CODE (LigmaColormapEditor, ligma_colormap_editor,
                         LIGMA_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_colormap_editor_docked_iface_init))

#define parent_class ligma_colormap_editor_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_colormap_editor_class_init (LigmaColormapEditorClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed    = ligma_colormap_editor_constructed;
  object_class->dispose        = ligma_colormap_editor_dispose;

  widget_class->unmap          = ligma_colormap_editor_unmap;
}

static void
ligma_colormap_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_context = ligma_colormap_editor_set_context;
}

static void
ligma_colormap_editor_init (LigmaColormapEditor *editor)
{
}

static void
ligma_colormap_editor_constructed (GObject *object)
{
  LigmaColormapEditor *editor = LIGMA_COLORMAP_EDITOR (object);
  GdkModifierType     extend_mask;
  GdkModifierType     modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* Editor buttons. */
  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "colormap",
                                 "colormap-edit-color",
                                 NULL);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "colormap",
                                 "colormap-add-color-from-fg",
                                 "colormap-add-color-from-bg",
                                 ligma_get_toggle_behavior_mask (),
                                 NULL);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "colormap",
                                 "colormap-selection-replace",
                                 "colormap-selection-add",
                                 extend_mask,
                                 "colormap-selection-subtract",
                                 modify_mask,
                                 "colormap-selection-intersect",
                                 extend_mask | modify_mask,
                                 NULL);
}

static void
ligma_colormap_editor_dispose (GObject *object)
{
  LigmaColormapEditor *editor = LIGMA_COLORMAP_EDITOR (object);

  g_clear_pointer (&editor->color_dialog, gtk_widget_destroy);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_colormap_editor_unmap (GtkWidget *widget)
{
  LigmaColormapEditor *editor = LIGMA_COLORMAP_EDITOR (widget);

  if (editor->color_dialog)
    gtk_widget_hide (editor->color_dialog);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
ligma_colormap_editor_set_context (LigmaDocked  *docked,
                                  LigmaContext *context)
{
  LigmaColormapEditor *editor = LIGMA_COLORMAP_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  if (editor->selection)
    gtk_widget_destroy (editor->selection);
  editor->selection = NULL;

  /* Main selection widget. */
  if (context)
    {
      editor->selection = ligma_colormap_selection_new (context);
      gtk_box_pack_start (GTK_BOX (editor), editor->selection, TRUE, TRUE, 0);
      gtk_widget_show (editor->selection);

      g_signal_connect_swapped (editor->selection, "color-clicked",
                                G_CALLBACK (ligma_colormap_editor_color_clicked),
                                editor);
      g_signal_connect_swapped (editor->selection, "color-activated",
                                G_CALLBACK (ligma_colormap_editor_edit_color),
                                editor);
      g_signal_connect (editor->selection, "button-press-event",
                        G_CALLBACK (ligma_colormap_editor_entry_button_press),
                        editor);
      g_signal_connect (editor->selection, "popup-menu",
                        G_CALLBACK (ligma_colormap_editor_entry_popup),
                        editor);
    }
}


/*  public functions  */

GtkWidget *
ligma_colormap_editor_new (LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (LIGMA_TYPE_COLORMAP_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<Colormap>",
                       "ui-path",         "/colormap-popup",
                       NULL);
}

void
ligma_colormap_editor_edit_color (LigmaColormapEditor *editor)
{
  LigmaImage *image;
  LigmaRGB    color;
  gchar     *desc;
  gint       index;

  g_return_if_fail (LIGMA_IS_COLORMAP_EDITOR (editor));

  image = LIGMA_IMAGE_EDITOR (editor)->image;
  index = ligma_colormap_selection_get_index (LIGMA_COLORMAP_SELECTION (editor->selection),
                                             NULL);

  if (index == -1)
    /* No colormap. */
    return;

  ligma_image_get_colormap_entry (image, index, &color);

  desc = g_strdup_printf (_("Edit colormap entry #%d"), index);

  if (! editor->color_dialog)
    {
      editor->color_dialog =
        ligma_color_dialog_new (LIGMA_VIEWABLE (image),
                               LIGMA_IMAGE_EDITOR (editor)->context,
                               FALSE,
                               _("Edit Colormap Entry"),
                               LIGMA_ICON_COLORMAP,
                               desc,
                               GTK_WIDGET (editor),
                               ligma_dialog_factory_get_singleton (),
                               "ligma-colormap-editor-color-dialog",
                               (const LigmaRGB *) &color,
                               TRUE, FALSE);

      g_signal_connect (editor->color_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &editor->color_dialog);

      g_signal_connect (editor->color_dialog, "update",
                        G_CALLBACK (ligma_colormap_editor_color_update),
                        editor);
    }
  else
    {
      ligma_viewable_dialog_set_viewables (LIGMA_VIEWABLE_DIALOG (editor->color_dialog),
                                          g_list_prepend (NULL, image),
                                          LIGMA_IMAGE_EDITOR (editor)->context);
      g_object_set (editor->color_dialog, "description", desc, NULL);
      ligma_color_dialog_set_color (LIGMA_COLOR_DIALOG (editor->color_dialog),
                                   &color);

      if (! gtk_widget_get_visible (editor->color_dialog))
        ligma_dialog_factory_position_dialog (ligma_dialog_factory_get_singleton (),
                                             "ligma-colormap-editor-color-dialog",
                                             editor->color_dialog,
                                             ligma_widget_get_monitor (GTK_WIDGET (editor)));
    }

  g_free (desc);

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

gint
ligma_colormap_editor_get_index (LigmaColormapEditor *editor,
                                const LigmaRGB      *search)
{
  g_return_val_if_fail (LIGMA_IS_COLORMAP_EDITOR (editor), 0);

  return ligma_colormap_selection_get_index (LIGMA_COLORMAP_SELECTION (editor->selection), search);
}

gboolean
ligma_colormap_editor_set_index (LigmaColormapEditor *editor,
                                gint                index,
                                LigmaRGB            *color)
{
  g_return_val_if_fail (LIGMA_IS_COLORMAP_EDITOR (editor), FALSE);

  return ligma_colormap_selection_set_index (LIGMA_COLORMAP_SELECTION (editor->selection), index, color);
}

gint
ligma_colormap_editor_max_index (LigmaColormapEditor *editor)
{
  g_return_val_if_fail (LIGMA_IS_COLORMAP_EDITOR (editor), -1);

  return ligma_colormap_selection_max_index (LIGMA_COLORMAP_SELECTION (editor->selection));
}

static void
ligma_colormap_editor_color_update (LigmaColorDialog      *dialog,
                                   const LigmaRGB        *color,
                                   LigmaColorDialogState  state,
                                   LigmaColormapEditor   *editor)
{
  LigmaImageEditor *image_editor = LIGMA_IMAGE_EDITOR (editor);
  LigmaImage       *image        = image_editor->image;
  gboolean         push_undo    = FALSE;

  switch (state)
    {
    case LIGMA_COLOR_DIALOG_OK:
      push_undo = TRUE;

      if (state & ligma_get_toggle_behavior_mask ())
        ligma_context_set_background (image_editor->context, color);
      else
        ligma_context_set_foreground (image_editor->context, color);
      /* Fall through */

    case LIGMA_COLOR_DIALOG_CANCEL:
      gtk_widget_hide (editor->color_dialog);
      break;

    case LIGMA_COLOR_DIALOG_UPDATE:
      break;
    }

  if (image)
    {
      gint col_index;

      col_index = ligma_colormap_selection_get_index (LIGMA_COLORMAP_SELECTION (editor->selection),
                                                     NULL);
      if (push_undo)
        {
          LigmaRGB old_color;

          ligma_color_selection_get_old_color (
            LIGMA_COLOR_SELECTION (dialog->selection), &old_color);

          /* Restore old color for undo */
          ligma_image_set_colormap_entry (image, col_index, &old_color,
                                         FALSE);
        }

      ligma_image_set_colormap_entry (image, col_index, color,
                                     push_undo);

      if (push_undo)
        ligma_image_flush (image);
      else
        ligma_projection_flush (ligma_image_get_projection (image));
    }
}

static gboolean
ligma_colormap_editor_entry_button_press (GtkWidget *widget,
                                         GdkEvent  *event,
                                         gpointer   user_data)
{
  if (gdk_event_triggers_context_menu (event))
    {
      ligma_editor_popup_menu_at_pointer (LIGMA_EDITOR (user_data), event);
      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static gboolean
ligma_colormap_editor_entry_popup (GtkWidget *widget,
                                  gpointer   user_data)
{
  LigmaColormapEditor *editor = LIGMA_COLORMAP_EDITOR (user_data);
  LigmaColormapSelection *selection = LIGMA_COLORMAP_SELECTION (widget);
  LigmaPaletteEntry *selected;
  GdkRectangle rect;

  selected = ligma_colormap_selection_get_selected_entry (selection);
  if (!selected)
    return GDK_EVENT_PROPAGATE;

  ligma_colormap_selection_get_entry_rect (selection, selected, &rect);
  return ligma_editor_popup_menu_at_rect (LIGMA_EDITOR (editor),
                                         gtk_widget_get_window (widget),
                                         &rect, GDK_GRAVITY_CENTER, GDK_GRAVITY_NORTH_WEST,
                                         NULL);
}

static void
ligma_colormap_editor_color_clicked (LigmaColormapEditor *editor,
                                    LigmaPaletteEntry   *entry,
                                    GdkModifierType     state)
{
  LigmaImageEditor *image_editor = LIGMA_IMAGE_EDITOR (editor);

  if (state & ligma_get_toggle_behavior_mask ())
    ligma_context_set_background (image_editor->context, &entry->color);
  else
    ligma_context_set_foreground (image_editor->context, &entry->color);
}
