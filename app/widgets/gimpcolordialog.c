/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color_dialog module (C) 1998 Austin Donnelly <austin@greenend.org.uk>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligma-palettes.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-color-profile.h"
#include "core/ligmaimage-colormap.h"
#include "core/ligmamarshal.h"
#include "core/ligmapalettemru.h"
#include "core/ligmaprojection.h"

#include "ligmaactiongroup.h"
#include "ligmacolordialog.h"
#include "ligmacolorhistory.h"
#include "ligmacolormapselection.h"
#include "ligmadialogfactory.h"
#include "ligmahelp-ids.h"
#include "ligmawidgets-constructors.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


#define RESPONSE_RESET   1
#define COLOR_AREA_SIZE 20


enum
{
  UPDATE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_USER_CONTEXT_AWARE
};

static void   ligma_color_dialog_constructed      (GObject            *object);
static void   ligma_color_dialog_finalize         (GObject            *object);
static void   ligma_color_dialog_set_property     (GObject            *object,
                                                  guint               property_id,
                                                  const GValue       *value,
                                                  GParamSpec         *pspec);
static void   ligma_color_dialog_get_property     (GObject            *object,
                                                  guint               property_id,
                                                  GValue             *value,
                                                  GParamSpec         *pspec);

static void   ligma_color_dialog_response         (GtkDialog          *dialog,
                                                  gint                response_id);

static void   ligma_color_dialog_help_func        (const gchar        *help_id,
                                                  gpointer            help_data);

static void   ligma_color_dialog_colormap_clicked (LigmaColorDialog    *dialog,
                                                  LigmaPaletteEntry   *entry,
                                                  GdkModifierType     state);
static void
        ligma_color_dialog_colormap_edit_activate (LigmaColorDialog    *dialog);
static void
        ligma_color_dialog_colormap_add_activate  (LigmaColorDialog    *dialog);

static void   ligma_color_dialog_color_changed    (LigmaColorSelection *selection,
                                                  LigmaColorDialog    *dialog);

static void   ligma_color_history_add_clicked     (GtkWidget          *widget,
                                                  LigmaColorDialog    *dialog);

static void   ligma_color_dialog_history_selected (LigmaColorHistory   *history,
                                                  const LigmaRGB      *rgb,
                                                  LigmaColorDialog    *dialog);

static void   ligma_color_dialog_image_changed    (LigmaContext        *context,
                                                  LigmaImage          *image,
                                                  LigmaColorDialog    *dialog);
static void   ligma_color_dialog_update_simulation(LigmaImage          *image,
                                                  LigmaColorDialog    *dialog);
static void   ligma_color_dialog_update           (LigmaColorDialog    *dialog);
static void   ligma_color_dialog_show             (LigmaColorDialog    *dialog);
static void   ligma_color_dialog_hide             (LigmaColorDialog    *dialog);


G_DEFINE_TYPE (LigmaColorDialog, ligma_color_dialog, LIGMA_TYPE_VIEWABLE_DIALOG)

#define parent_class ligma_color_dialog_parent_class

static guint color_dialog_signals[LAST_SIGNAL] = { 0, };


static void
ligma_color_dialog_class_init (LigmaColorDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->constructed  = ligma_color_dialog_constructed;
  object_class->finalize     = ligma_color_dialog_finalize;
  object_class->set_property = ligma_color_dialog_set_property;
  object_class->get_property = ligma_color_dialog_get_property;

  dialog_class->response     = ligma_color_dialog_response;

  color_dialog_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaColorDialogClass, update),
                  NULL, NULL,
                  ligma_marshal_VOID__BOXED_ENUM,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_RGB,
                  LIGMA_TYPE_COLOR_DIALOG_STATE);

  g_object_class_install_property (object_class, PROP_USER_CONTEXT_AWARE,
                                   g_param_spec_boolean ("user-context-aware",
                                                        NULL, NULL, FALSE,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_color_dialog_init (LigmaColorDialog *dialog)
{
  dialog->stack = gtk_stack_new ();
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      dialog->stack, TRUE, TRUE, 0);
  gtk_widget_show (dialog->stack);
}

static void
ligma_color_dialog_constructed (GObject *object)
{
  LigmaColorDialog    *dialog          = LIGMA_COLOR_DIALOG (object);
  LigmaViewableDialog *viewable_dialog = LIGMA_VIEWABLE_DIALOG (object);
  GtkWidget          *hbox;
  GtkWidget          *history;
  GtkWidget          *button;
  GtkWidget          *image;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /** Tab: colormap selection. **/
  dialog->colormap_selection = ligma_colormap_selection_new (viewable_dialog->context->ligma->user_context);
  gtk_stack_add_named (GTK_STACK (dialog->stack),
                       dialog->colormap_selection, "colormap");
  g_signal_connect_swapped (dialog->colormap_selection, "color-clicked",
                            G_CALLBACK (ligma_color_dialog_colormap_clicked),
                            dialog);
  g_signal_connect_swapped (dialog->colormap_selection, "color-activated",
                            G_CALLBACK (ligma_color_dialog_colormap_edit_activate),
                            dialog);
  gtk_widget_show (dialog->colormap_selection);

  /* Edit color button */
  button = ligma_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), FALSE);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_EDIT, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  /* XXX: I use the same icon, tooltip and help id as in
   * colormap-actions.c. I wanted to just load these strings from
   * the action, but the group is only registered when the Colormap
   * dockable is opened.
   */
  ligma_help_set_help_data_with_markup (button,
                                       NC_("colormap-action", "Edit this color"),
                                       LIGMA_HELP_INDEXED_PALETTE_EDIT);
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (ligma_color_dialog_colormap_edit_activate),
                            dialog);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (LIGMA_COLORMAP_SELECTION (dialog->colormap_selection)->right_vbox), button,
                      FALSE, FALSE, 0);

  /* Add Color button */
  button = ligma_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), FALSE);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_LIST_ADD, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  ligma_help_set_help_data_with_markup (button,
                                       NC_("colormap-action", "Add current foreground color"),
                                       LIGMA_HELP_INDEXED_PALETTE_ADD);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (ligma_color_dialog_colormap_add_activate),
                            dialog);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (LIGMA_COLORMAP_SELECTION (dialog->colormap_selection)->right_vbox), button,
                      FALSE, FALSE, 0);

  /** Tab: color selection. **/
  dialog->selection = ligma_color_selection_new ();
  gtk_container_set_border_width (GTK_CONTAINER (dialog->selection), 12);
  gtk_stack_add_named (GTK_STACK (dialog->stack), dialog->selection, "color");
  gtk_widget_show (dialog->selection);

  g_signal_connect (dialog->selection, "color-changed",
                    G_CALLBACK (ligma_color_dialog_color_changed),
                    dialog);

  /* Color history box. */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_end (GTK_BOX (ligma_color_selection_get_right_vbox (LIGMA_COLOR_SELECTION (dialog->selection))),
                    hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Button for adding to color history. */
  button = ligma_icon_button_new (LIGMA_ICON_LIST_ADD, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (button), FALSE, FALSE, 0);
  ligma_help_set_help_data (button,
                           _("Add the current color to the color history"),
                           NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_color_history_add_clicked),
                    dialog);

  /* Color history table. */
  history = ligma_color_history_new (viewable_dialog->context, 12);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (history), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (history));

  g_signal_connect (history, "color-selected",
                    G_CALLBACK (ligma_color_dialog_history_selected),
                    dialog);

  g_signal_connect (dialog, "show",
                    G_CALLBACK (ligma_color_dialog_show),
                    NULL);
  g_signal_connect (dialog, "hide",
                    G_CALLBACK (ligma_color_dialog_hide),
                    NULL);
  ligma_color_dialog_show (dialog);
}

static void
ligma_color_dialog_finalize (GObject *object)
{
  LigmaColorDialog    *dialog          = LIGMA_COLOR_DIALOG (object);
  LigmaViewableDialog *viewable_dialog = LIGMA_VIEWABLE_DIALOG (dialog);

  if (dialog->user_context_aware && viewable_dialog->context)
    {
      LigmaContext *user_context = viewable_dialog->context->ligma->user_context;

      g_signal_handlers_disconnect_by_func (user_context,
                                            G_CALLBACK (ligma_color_dialog_image_changed),
                                            dialog);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_color_dialog_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaColorDialog *dialog = LIGMA_COLOR_DIALOG (object);

  switch (property_id)
    {
    case PROP_USER_CONTEXT_AWARE:
      dialog->user_context_aware = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_dialog_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaColorDialog *dialog = LIGMA_COLOR_DIALOG (object);

  switch (property_id)
    {
    case PROP_USER_CONTEXT_AWARE:
      g_value_set_boolean (value, dialog->user_context_aware);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_dialog_response (GtkDialog *gtk_dialog,
                            gint       response_id)
{
  LigmaColorDialog       *dialog          = LIGMA_COLOR_DIALOG (gtk_dialog);
  LigmaViewableDialog    *viewable_dialog = LIGMA_VIEWABLE_DIALOG (dialog);
  LigmaImage             *image           = NULL;
  LigmaColormapSelection *colormap_selection;
  gint                   col_index;
  LigmaRGB                color;

  colormap_selection = LIGMA_COLORMAP_SELECTION (dialog->colormap_selection);
  col_index = ligma_colormap_selection_get_index (colormap_selection, NULL);

  if (dialog->colormap_editing && viewable_dialog->context)
    {
      LigmaContext *user_context = viewable_dialog->context->ligma->user_context;

      image = ligma_context_get_image (user_context);
    }

  switch (response_id)
    {
    case RESPONSE_RESET:
      ligma_color_selection_reset (LIGMA_COLOR_SELECTION (dialog->selection));
      break;

    case GTK_RESPONSE_OK:
      ligma_color_selection_get_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                      &color);

      if (dialog->colormap_editing && image)
        {
          LigmaRGB old_color;

          dialog->colormap_editing = FALSE;

          /* Restore old color for undo */
          ligma_color_selection_get_old_color (LIGMA_COLOR_SELECTION (dialog->selection), &old_color);
          ligma_image_set_colormap_entry (image, col_index, &old_color, FALSE);
          ligma_image_set_colormap_entry (image, col_index, &color, TRUE);
          ligma_image_flush (image);

          gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "colormap");
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         &color, LIGMA_COLOR_DIALOG_UPDATE);
        }
      else
        {
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         &color, LIGMA_COLOR_DIALOG_OK);
        }
      break;

    default:
      ligma_color_selection_get_old_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                          &color);

      if (dialog->colormap_editing && image)
        {
          dialog->colormap_editing = FALSE;

          ligma_image_set_colormap_entry (image, col_index, &color, FALSE);
          ligma_projection_flush (ligma_image_get_projection (image));

          gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "colormap");
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         &color, LIGMA_COLOR_DIALOG_UPDATE);
        }
      else
        {
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         &color, LIGMA_COLOR_DIALOG_CANCEL);
        }
      break;
    }
}


/*  public functions  */

GtkWidget *
ligma_color_dialog_new (LigmaViewable      *viewable,
                       LigmaContext       *context,
                       gboolean           user_context_aware,
                       const gchar       *title,
                       const gchar       *icon_name,
                       const gchar       *desc,
                       GtkWidget         *parent,
                       LigmaDialogFactory *dialog_factory,
                       const gchar       *dialog_identifier,
                       const LigmaRGB     *color,
                       gboolean           wants_updates,
                       gboolean           show_alpha)
{
  LigmaColorDialog *dialog;
  const gchar     *role;
  gboolean         use_header_bar;

  g_return_val_if_fail (viewable == NULL || LIGMA_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        LIGMA_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (dialog_factory == NULL || dialog_identifier != NULL,
                        NULL);
  g_return_val_if_fail (color != NULL, NULL);

  role = dialog_identifier ? dialog_identifier : "ligma-color-selector";

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (LIGMA_TYPE_COLOR_DIALOG,
                         "title",              title,
                         "role",               role,
                         "help-func",          ligma_color_dialog_help_func,
                         "help-id",            LIGMA_HELP_COLOR_DIALOG,
                         "icon-name",          icon_name,
                         "description",        desc,
                         "context",            context,
                         "user-context-aware", user_context_aware,
                         "parent",             gtk_widget_get_toplevel (parent),
                         "use-header-bar",     use_header_bar,
                         NULL);

  ligma_dialog_add_buttons (LIGMA_DIALOG (dialog),

                           _("_Reset"),  RESPONSE_RESET,
                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                           _("_OK"),     GTK_RESPONSE_OK,

                           NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            RESPONSE_RESET,
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  if (viewable)
    {
      ligma_viewable_dialog_set_viewables (LIGMA_VIEWABLE_DIALOG (dialog),
                                          g_list_prepend (NULL, viewable), context);
    }
  else
    {
      GtkWidget *parent;

      parent = gtk_widget_get_parent (LIGMA_VIEWABLE_DIALOG (dialog)->icon);
      parent = gtk_widget_get_parent (parent);

      gtk_widget_hide (parent);
    }

  dialog->wants_updates = wants_updates;

  if (dialog_factory)
    {
      ligma_dialog_factory_add_foreign (dialog_factory, dialog_identifier,
                                       GTK_WIDGET (dialog),
                                       ligma_widget_get_monitor (parent));
    }

  ligma_color_selection_set_show_alpha (LIGMA_COLOR_SELECTION (dialog->selection),
                                       show_alpha);

  g_object_set_data (G_OBJECT (context->ligma->config->color_management),
                     "ligma-context", context);

  ligma_color_selection_set_config (LIGMA_COLOR_SELECTION (dialog->selection),
                                   context->ligma->config->color_management);

  g_object_set_data (G_OBJECT (context->ligma->config->color_management),
                     "ligma-context", NULL);

  ligma_color_selection_set_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                  color);
  ligma_color_selection_set_old_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                      color);

  return GTK_WIDGET (dialog);
}

void
ligma_color_dialog_set_color (LigmaColorDialog *dialog,
                             const LigmaRGB   *color)
{
  g_return_if_fail (LIGMA_IS_COLOR_DIALOG (dialog));
  g_return_if_fail (color != NULL);

  g_signal_handlers_block_by_func (dialog->selection,
                                   ligma_color_dialog_color_changed,
                                   dialog);

  ligma_color_selection_set_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                  color);
  ligma_color_selection_set_old_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                      color);

  g_signal_handlers_unblock_by_func (dialog->selection,
                                     ligma_color_dialog_color_changed,
                                     dialog);
}

void
ligma_color_dialog_get_color (LigmaColorDialog *dialog,
                             LigmaRGB         *color)
{
  g_return_if_fail (LIGMA_IS_COLOR_DIALOG (dialog));
  g_return_if_fail (color != NULL);

  ligma_color_selection_get_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                  color);
}


/*  private functions  */

static void
ligma_color_dialog_help_func (const gchar *help_id,
                             gpointer     help_data)
{
  LigmaColorDialog   *dialog = LIGMA_COLOR_DIALOG (help_data);
  LigmaColorNotebook *notebook;
  LigmaColorSelector *current;

  notebook =
    LIGMA_COLOR_NOTEBOOK (ligma_color_selection_get_notebook (LIGMA_COLOR_SELECTION (dialog->selection)));

  current = ligma_color_notebook_get_current_selector (notebook);

  help_id = LIGMA_COLOR_SELECTOR_GET_CLASS (current)->help_id;

  ligma_standard_help_func (help_id, NULL);
}

static void
ligma_color_dialog_colormap_clicked (LigmaColorDialog  *dialog,
                                    LigmaPaletteEntry *entry,
                                    GdkModifierType   state)
{
  ligma_color_dialog_set_color (dialog, &entry->color);

  if (dialog->wants_updates)
    {
      g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                     &entry->color, LIGMA_COLOR_DIALOG_UPDATE);
    }
}

static void
ligma_color_dialog_colormap_edit_activate (LigmaColorDialog *dialog)
{
  LigmaColormapSelection *colormap_selection;
  LigmaRGB                color;
  gint                   col_index;

  dialog->colormap_editing = TRUE;

  colormap_selection = LIGMA_COLORMAP_SELECTION (dialog->colormap_selection);
  col_index = ligma_colormap_selection_get_index (colormap_selection, NULL);
  ligma_image_get_colormap_entry (dialog->active_image, col_index, &color);
  ligma_color_dialog_set_color (dialog, &color);

  gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "color");
}

static void
ligma_color_dialog_colormap_add_activate (LigmaColorDialog *dialog)
{
  LigmaViewableDialog *viewable_dialog = LIGMA_VIEWABLE_DIALOG (dialog);

  if (dialog->active_image &&
      ligma_image_get_colormap_size (dialog->active_image) < 256 &&
      viewable_dialog->context)
    {
      LigmaContext *user_context = viewable_dialog->context->ligma->user_context;
      LigmaRGB      color;

      ligma_context_get_foreground (user_context, &color);

      ligma_image_add_colormap_entry (dialog->active_image, &color);
      ligma_image_flush (dialog->active_image);
    }
}

static void
ligma_color_dialog_color_changed (LigmaColorSelection *selection,
                                 LigmaColorDialog    *dialog)
{
  LigmaViewableDialog *viewable_dialog = LIGMA_VIEWABLE_DIALOG (dialog);
  LigmaRGB             color;

  ligma_color_selection_get_color (selection, &color);

  if (dialog->colormap_editing && viewable_dialog->context)
    {
      LigmaContext *user_context = viewable_dialog->context->ligma->user_context;
      LigmaImage   *image;

      image = ligma_context_get_image (user_context);
      if (image)
        {
          LigmaColormapSelection *colormap_selection;
          gboolean               push_undo = FALSE;
          gint                   col_index;

          colormap_selection = LIGMA_COLORMAP_SELECTION (dialog->colormap_selection);
          col_index = ligma_colormap_selection_get_index (colormap_selection, NULL);
          if (push_undo)
            {
              LigmaRGB old_color;

              ligma_color_selection_get_old_color (LIGMA_COLOR_SELECTION (dialog->selection), &old_color);

              /* Restore old color for undo */
              ligma_image_set_colormap_entry (image, col_index, &old_color,
                                             FALSE);
            }

          ligma_image_set_colormap_entry (image, col_index, &color,
                                         push_undo);

          if (push_undo)
            ligma_image_flush (image);
          else
            ligma_projection_flush (ligma_image_get_projection (image));
        }
    }

  if (dialog->wants_updates)
    {
      g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                     &color, LIGMA_COLOR_DIALOG_UPDATE);
    }
}


/* History-adding button callback */

static void
ligma_color_history_add_clicked (GtkWidget       *widget,
                                LigmaColorDialog *dialog)
{
  LigmaViewableDialog *viewable_dialog = LIGMA_VIEWABLE_DIALOG (dialog);
  LigmaPalette        *history;
  LigmaRGB             color;

  history = ligma_palettes_get_color_history (viewable_dialog->context->ligma);

  ligma_color_selection_get_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                  &color);

  ligma_palette_mru_add (LIGMA_PALETTE_MRU (history), &color);
}

/* Color history callback  */

static void
ligma_color_dialog_history_selected (LigmaColorHistory *history,
                                    const LigmaRGB    *rgb,
                                    LigmaColorDialog  *dialog)
{
  ligma_color_selection_set_color (LIGMA_COLOR_SELECTION (dialog->selection),
                                  rgb);
}

/* Context-related callbacks */

static void
ligma_color_dialog_image_changed (LigmaContext     *context,
                                 LigmaImage       *image,
                                 LigmaColorDialog *dialog)
{
  if (dialog->active_image != image)
    {
      if (dialog->active_image)
        {
          g_object_remove_weak_pointer (G_OBJECT (dialog->active_image),
                                        (gpointer) &dialog->active_image);
          g_signal_handlers_disconnect_by_func (dialog->active_image,
                                                G_CALLBACK (ligma_color_dialog_update),
                                                dialog);
          g_signal_handlers_disconnect_by_func (dialog->active_image,
                                                ligma_color_dialog_update_simulation,
                                                dialog);
        }
      dialog->active_image = image;
      if (image)
        {
          g_object_add_weak_pointer (G_OBJECT (dialog->active_image),
                                     (gpointer) &dialog->active_image);
          g_signal_connect_swapped (image, "notify::base-type",
                                    G_CALLBACK (ligma_color_dialog_update),
                                    dialog);
          g_signal_connect (image, "simulation-profile-changed",
                            G_CALLBACK (ligma_color_dialog_update_simulation),
                            dialog);
          g_signal_connect (image, "simulation-intent-changed",
                            G_CALLBACK (ligma_color_dialog_update_simulation),
                            dialog);
          g_signal_connect (image, "simulation-bpc-changed",
                            G_CALLBACK (ligma_color_dialog_update_simulation),
                            dialog);

          ligma_color_dialog_update_simulation (image, dialog);
        }
      ligma_color_dialog_update (dialog);
    }
}

static void
ligma_color_dialog_update_simulation (LigmaImage       *image,
                                     LigmaColorDialog *dialog)
{
  g_return_if_fail (LIGMA_IS_COLOR_DIALOG (dialog));

  if (image && LIGMA_IS_COLOR_DIALOG (dialog))
    {
      ligma_color_selection_set_simulation (LIGMA_COLOR_SELECTION (dialog->selection),
                                           ligma_image_get_simulation_profile (image),
                                           ligma_image_get_simulation_intent (image),
                                           ligma_image_get_simulation_bpc (image));
    }
}


static void
ligma_color_dialog_update (LigmaColorDialog *dialog)
{
  if (dialog->active_image &&
      ligma_image_get_base_type (dialog->active_image) == LIGMA_INDEXED)
    gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "colormap");
  else
    gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "color");
}

static void
ligma_color_dialog_show (LigmaColorDialog *dialog)
{
  LigmaViewableDialog *viewable_dialog = LIGMA_VIEWABLE_DIALOG (dialog);

  dialog->colormap_editing = FALSE;

  if (dialog->user_context_aware && viewable_dialog->context)
    {
      LigmaContext *user_context = viewable_dialog->context->ligma->user_context;
      LigmaImage   *image        = ligma_context_get_image (user_context);

      g_signal_connect (user_context, "image-changed",
                        G_CALLBACK (ligma_color_dialog_image_changed),
                        dialog);

      ligma_color_dialog_image_changed (viewable_dialog->context,
                                       image, dialog);
      ligma_color_dialog_update (dialog);
    }
  else
    {
      gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "color");
    }
}

static void
ligma_color_dialog_hide (LigmaColorDialog *dialog)
{
  LigmaViewableDialog *viewable_dialog = LIGMA_VIEWABLE_DIALOG (dialog);

  if (dialog->user_context_aware && viewable_dialog->context)
    {
      LigmaContext *user_context = viewable_dialog->context->ligma->user_context;
      g_signal_handlers_disconnect_by_func (user_context,
                                            G_CALLBACK (ligma_color_dialog_image_changed),
                                            dialog);
    }
}
