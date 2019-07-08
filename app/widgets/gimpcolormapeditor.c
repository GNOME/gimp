/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimppalette.h"
#include "core/gimpprojection.h"

#include "gimpcolordialog.h"
#include "gimpcolormapeditor.h"
#include "gimpcolormapselection.h"
#include "gimpdialogfactory.h"
#include "gimpdocked.h"
#include "gimpmenufactory.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void gimp_colormap_editor_docked_iface_init (GimpDockedInterface  *face);

static void   gimp_colormap_editor_constructed     (GObject              *object);
static void   gimp_colormap_editor_dispose         (GObject              *object);

static void   gimp_colormap_editor_unmap           (GtkWidget            *widget);

static void   gimp_colormap_editor_set_context     (GimpDocked           *docked,
                                                    GimpContext          *context);

static void   gimp_colormap_editor_color_update    (GimpColorDialog      *dialog,
                                                    const GimpRGB        *color,
                                                    GimpColorDialogState  state,
                                                    GimpColormapEditor   *editor);

static void   gimp_colormap_editor_entry_popup     (GimpEditor           *editor);
static void   gimp_colormap_editor_color_clicked   (GimpColormapEditor   *editor,
                                                    GimpPaletteEntry     *entry,
                                                    GdkModifierType       state);

G_DEFINE_TYPE_WITH_CODE (GimpColormapEditor, gimp_colormap_editor,
                         GIMP_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_colormap_editor_docked_iface_init))

#define parent_class gimp_colormap_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_colormap_editor_class_init (GimpColormapEditorClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed    = gimp_colormap_editor_constructed;
  object_class->dispose        = gimp_colormap_editor_dispose;

  widget_class->unmap          = gimp_colormap_editor_unmap;
}

static void
gimp_colormap_editor_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_context = gimp_colormap_editor_set_context;
}

static void
gimp_colormap_editor_init (GimpColormapEditor *editor)
{
}

static void
gimp_colormap_editor_constructed (GObject *object)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (object);
  GdkModifierType     extend_mask;
  GdkModifierType     modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* Editor buttons. */
  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "colormap",
                                 "colormap-edit-color",
                                 NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "colormap",
                                 "colormap-add-color-from-fg",
                                 "colormap-add-color-from-bg",
                                 gimp_get_toggle_behavior_mask (),
                                 NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "colormap",
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
gimp_colormap_editor_dispose (GObject *object)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (object);

  g_clear_pointer (&editor->color_dialog, gtk_widget_destroy);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_colormap_editor_unmap (GtkWidget *widget)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (widget);

  if (editor->color_dialog)
    gtk_widget_hide (editor->color_dialog);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_colormap_editor_set_context (GimpDocked  *docked,
                                  GimpContext *context)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  if (editor->selection)
    gtk_widget_destroy (editor->selection);
  editor->selection = NULL;

  /* Main selection widget. */
  if (context)
    {
      editor->selection = gimp_colormap_selection_new (context);
      gtk_box_pack_start (GTK_BOX (editor), editor->selection, TRUE, TRUE, 0);
      gtk_widget_show (editor->selection);

      g_signal_connect_swapped (editor->selection, "color-context",
                                G_CALLBACK (gimp_colormap_editor_entry_popup),
                                editor);
      g_signal_connect_swapped (editor->selection, "color-clicked",
                                G_CALLBACK (gimp_colormap_editor_color_clicked),
                                editor);
      g_signal_connect_swapped (editor->selection, "color-activated",
                                G_CALLBACK (gimp_colormap_editor_edit_color),
                                editor);
    }
}


/*  public functions  */

GtkWidget *
gimp_colormap_editor_new (GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (GIMP_TYPE_COLORMAP_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<Colormap>",
                       "ui-path",         "/colormap-popup",
                       NULL);
}

void
gimp_colormap_editor_edit_color (GimpColormapEditor *editor)
{
  GimpImage    *image;
  const guchar *colormap;
  GimpRGB       color;
  gchar        *desc;
  gint          index;

  g_return_if_fail (GIMP_IS_COLORMAP_EDITOR (editor));

  image = GIMP_IMAGE_EDITOR (editor)->image;
  index = gimp_colormap_selection_get_index (GIMP_COLORMAP_SELECTION (editor->selection),
                                             NULL);

  if (index == -1)
    /* No colormap. */
    return;

  colormap = gimp_image_get_colormap (image);

  gimp_rgba_set_uchar (&color,
                       colormap[index * 3],
                       colormap[index * 3 + 1],
                       colormap[index * 3 + 2],
                       255);

  desc = g_strdup_printf (_("Edit colormap entry #%d"), index);

  if (! editor->color_dialog)
    {
      editor->color_dialog =
        gimp_color_dialog_new (GIMP_VIEWABLE (image),
                               GIMP_IMAGE_EDITOR (editor)->context,
                               FALSE,
                               _("Edit Colormap Entry"),
                               GIMP_ICON_COLORMAP,
                               desc,
                               GTK_WIDGET (editor),
                               gimp_dialog_factory_get_singleton (),
                               "gimp-colormap-editor-color-dialog",
                               (const GimpRGB *) &color,
                               TRUE, FALSE);

      g_signal_connect (editor->color_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &editor->color_dialog);

      g_signal_connect (editor->color_dialog, "update",
                        G_CALLBACK (gimp_colormap_editor_color_update),
                        editor);
    }
  else
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (editor->color_dialog),
                                         GIMP_VIEWABLE (image),
                                         GIMP_IMAGE_EDITOR (editor)->context);
      g_object_set (editor->color_dialog, "description", desc, NULL);
      gimp_color_dialog_set_color (GIMP_COLOR_DIALOG (editor->color_dialog),
                                   &color);

      if (! gtk_widget_get_visible (editor->color_dialog))
        gimp_dialog_factory_position_dialog (gimp_dialog_factory_get_singleton (),
                                             "gimp-colormap-editor-color-dialog",
                                             editor->color_dialog,
                                             gimp_widget_get_monitor (GTK_WIDGET (editor)));
    }

  g_free (desc);

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

gint
gimp_colormap_editor_get_index (GimpColormapEditor *editor,
                                const GimpRGB      *search)
{
  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), 0);

  return gimp_colormap_selection_get_index (GIMP_COLORMAP_SELECTION (editor->selection), search);
}

gboolean
gimp_colormap_editor_set_index (GimpColormapEditor *editor,
                                gint                index,
                                GimpRGB            *color)
{
  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), FALSE);

  return gimp_colormap_selection_set_index (GIMP_COLORMAP_SELECTION (editor->selection), index, color);
}

gint
gimp_colormap_editor_max_index (GimpColormapEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), -1);

  return gimp_colormap_selection_max_index (GIMP_COLORMAP_SELECTION (editor->selection));
}

static void
gimp_colormap_editor_color_update (GimpColorDialog      *dialog,
                                   const GimpRGB        *color,
                                   GimpColorDialogState  state,
                                   GimpColormapEditor   *editor)
{
  GimpImageEditor *image_editor = GIMP_IMAGE_EDITOR (editor);
  GimpImage       *image        = image_editor->image;
  gboolean         push_undo    = FALSE;

  switch (state)
    {
    case GIMP_COLOR_DIALOG_OK:
      push_undo = TRUE;

      if (state & gimp_get_toggle_behavior_mask ())
        gimp_context_set_background (image_editor->context, color);
      else
        gimp_context_set_foreground (image_editor->context, color);
      /* Fall through */

    case GIMP_COLOR_DIALOG_CANCEL:
      gtk_widget_hide (editor->color_dialog);
      break;

    case GIMP_COLOR_DIALOG_UPDATE:
      break;
    }

  if (image)
    {
      gint col_index;

      col_index = gimp_colormap_selection_get_index (GIMP_COLORMAP_SELECTION (editor->selection),
                                                     NULL);
      if (push_undo)
        {
          GimpRGB old_color;

          gimp_color_selection_get_old_color (
            GIMP_COLOR_SELECTION (dialog->selection), &old_color);

          /* Restore old color for undo */
          gimp_image_set_colormap_entry (image, col_index, &old_color,
                                         FALSE);
        }

      gimp_image_set_colormap_entry (image, col_index, color,
                                     push_undo);

      if (push_undo)
        gimp_image_flush (image);
      else
        gimp_projection_flush (gimp_image_get_projection (image));
    }
}

static void
gimp_colormap_editor_entry_popup (GimpEditor *editor)
{
  gimp_editor_popup_menu (editor, NULL, NULL);
}

static void
gimp_colormap_editor_color_clicked (GimpColormapEditor *editor,
                                    GimpPaletteEntry   *entry,
                                    GdkModifierType     state)
{
  GimpImageEditor *image_editor = GIMP_IMAGE_EDITOR (editor);

  if (state & gimp_get_toggle_behavior_mask ())
    gimp_context_set_background (image_editor->context, &entry->color);
  else
    gimp_context_set_foreground (image_editor->context, &entry->color);
}
