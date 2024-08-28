/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimp-palettes.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"
#include "core/gimpimage-colormap.h"
#include "core/gimpmarshal.h"
#include "core/gimppalettemru.h"
#include "core/gimpprojection.h"

#include "gimpactiongroup.h"
#include "gimpcolordialog.h"
#include "gimpcolorhistory.h"
#include "gimpcolormapselection.h"
#include "gimpdialogfactory.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-constructors.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


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

static void   gimp_color_dialog_constructed      (GObject            *object);
static void   gimp_color_dialog_finalize         (GObject            *object);
static void   gimp_color_dialog_set_property     (GObject            *object,
                                                  guint               property_id,
                                                  const GValue       *value,
                                                  GParamSpec         *pspec);
static void   gimp_color_dialog_get_property     (GObject            *object,
                                                  guint               property_id,
                                                  GValue             *value,
                                                  GParamSpec         *pspec);

static void   gimp_color_dialog_style_updated    (GtkWidget          *widget);

static void   gimp_color_dialog_response         (GtkDialog          *dialog,
                                                  gint                response_id);

static void   gimp_color_dialog_help_func        (const gchar        *help_id,
                                                  gpointer            help_data);

static void   gimp_color_dialog_colormap_clicked (GimpColorDialog    *dialog,
                                                  GimpPaletteEntry   *entry,
                                                  GdkModifierType     state);
static void
        gimp_color_dialog_colormap_edit_activate (GimpColorDialog    *dialog);
static void
        gimp_color_dialog_colormap_add_activate  (GimpColorDialog    *dialog);

static void   gimp_color_dialog_color_changed    (GimpColorSelection *selection,
                                                  GimpColorDialog    *dialog);

static void   gimp_color_history_add_clicked     (GtkWidget          *widget,
                                                  GimpColorDialog    *dialog);

static void   gimp_color_dialog_image_changed    (GimpContext        *context,
                                                  GimpImage          *image,
                                                  GimpColorDialog    *dialog);
static void   gimp_color_dialog_update_format    (GimpImage          *image,
                                                  GimpColorDialog    *dialog);
static void   gimp_color_dialog_update_simulation(GimpImage          *image,
                                                  GimpColorDialog    *dialog);
static void   gimp_color_dialog_update_base_type (GimpColorDialog    *dialog);
static void   gimp_color_dialog_show             (GimpColorDialog    *dialog);
static void   gimp_color_dialog_hide             (GimpColorDialog    *dialog);


G_DEFINE_TYPE (GimpColorDialog, gimp_color_dialog, GIMP_TYPE_VIEWABLE_DIALOG)

#define parent_class gimp_color_dialog_parent_class

static guint color_dialog_signals[LAST_SIGNAL] = { 0, };


static void
gimp_color_dialog_class_init (GimpColorDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = gimp_color_dialog_constructed;
  object_class->finalize     = gimp_color_dialog_finalize;
  object_class->set_property = gimp_color_dialog_set_property;
  object_class->get_property = gimp_color_dialog_get_property;

  dialog_class->response      = gimp_color_dialog_response;

  widget_class->style_updated = gimp_color_dialog_style_updated;

  color_dialog_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpColorDialogClass, update),
                  NULL, NULL,
                  gimp_marshal_VOID__BOXED_ENUM,
                  G_TYPE_NONE, 2,
                  GEGL_TYPE_COLOR,
                  GIMP_TYPE_COLOR_DIALOG_STATE);

  g_object_class_install_property (object_class, PROP_USER_CONTEXT_AWARE,
                                   g_param_spec_boolean ("user-context-aware",
                                                        NULL, NULL, FALSE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_color_dialog_init (GimpColorDialog *dialog)
{
  dialog->stack = gtk_stack_new ();
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      dialog->stack, TRUE, TRUE, 0);
  gtk_widget_show (dialog->stack);
}

static void
gimp_color_dialog_constructed (GObject *object)
{
  GimpColorDialog    *dialog          = GIMP_COLOR_DIALOG (object);
  GimpViewableDialog *viewable_dialog = GIMP_VIEWABLE_DIALOG (object);
  GtkWidget          *hbox;
  GtkWidget          *history;
  GtkWidget          *button;
  GtkWidget          *image;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /** Tab: colormap selection. **/
  dialog->colormap_selection = gimp_colormap_selection_new (viewable_dialog->context->gimp->user_context);
  gtk_stack_add_named (GTK_STACK (dialog->stack),
                       dialog->colormap_selection, "colormap");
  g_signal_connect_swapped (dialog->colormap_selection, "color-clicked",
                            G_CALLBACK (gimp_color_dialog_colormap_clicked),
                            dialog);
  g_signal_connect_swapped (dialog->colormap_selection, "color-activated",
                            G_CALLBACK (gimp_color_dialog_colormap_edit_activate),
                            dialog);
  gtk_widget_show (dialog->colormap_selection);

  /* Edit color button */
  button = gimp_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), FALSE);

  image = gtk_image_new_from_icon_name (GIMP_ICON_EDIT, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  /* XXX: I use the same icon, tooltip and help id as in
   * colormap-actions.c. I wanted to just load these strings from
   * the action, but the group is only registered when the Colormap
   * dockable is opened.
   */
  gimp_help_set_help_data_with_markup (button,
                                       C_("colormap-action", "Edit this color"),
                                       GIMP_HELP_INDEXED_PALETTE_EDIT);
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_color_dialog_colormap_edit_activate),
                            dialog);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (GIMP_COLORMAP_SELECTION (dialog->colormap_selection)->right_vbox), button,
                      FALSE, FALSE, 0);

  /* Add Color button */
  button = gimp_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), FALSE);

  image = gtk_image_new_from_icon_name (GIMP_ICON_LIST_ADD, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data_with_markup (button,
                                       C_("colormap-action", "Add current foreground color"),
                                       GIMP_HELP_INDEXED_PALETTE_ADD);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_color_dialog_colormap_add_activate),
                            dialog);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (GIMP_COLORMAP_SELECTION (dialog->colormap_selection)->right_vbox), button,
                      FALSE, FALSE, 0);

  /** Tab: color selection. **/
  dialog->selection = gimp_color_selection_new ();
  gtk_container_set_border_width (GTK_CONTAINER (dialog->selection), 12);
  gtk_stack_add_named (GTK_STACK (dialog->stack), dialog->selection, "color");
  gtk_widget_show (dialog->selection);

  g_signal_connect (dialog->selection, "color-changed",
                    G_CALLBACK (gimp_color_dialog_color_changed),
                    dialog);

  /* Color history box. */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_end (GTK_BOX (gimp_color_selection_get_right_vbox (GIMP_COLOR_SELECTION (dialog->selection))),
                    hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Button for adding to color history. */
  button = gimp_icon_button_new (GIMP_ICON_LIST_ADD, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (button), FALSE, FALSE, 0);
  gimp_help_set_help_data (button,
                           _("Add the current color to the color history"),
                           NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_color_history_add_clicked),
                    dialog);

  /* Color history table. */
  history = gimp_color_history_new (viewable_dialog->context, 12);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (history), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (history));

  g_signal_connect_swapped (history, "color-selected",
                            G_CALLBACK (gimp_color_selection_set_color),
                            dialog->selection);

  g_signal_connect (dialog, "show",
                    G_CALLBACK (gimp_color_dialog_show),
                    NULL);
  g_signal_connect (dialog, "hide",
                    G_CALLBACK (gimp_color_dialog_hide),
                    NULL);
  gimp_color_dialog_show (dialog);
}

static void
gimp_color_dialog_finalize (GObject *object)
{
  GimpColorDialog    *dialog          = GIMP_COLOR_DIALOG (object);
  GimpViewableDialog *viewable_dialog = GIMP_VIEWABLE_DIALOG (dialog);

  g_clear_weak_pointer (&dialog->active_image);

  if (dialog->user_context_aware && viewable_dialog->context)
    {
      GimpContext *user_context = viewable_dialog->context->gimp->user_context;

      g_signal_handlers_disconnect_by_func (user_context,
                                            G_CALLBACK (gimp_color_dialog_image_changed),
                                            dialog);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_dialog_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpColorDialog *dialog = GIMP_COLOR_DIALOG (object);

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
gimp_color_dialog_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpColorDialog *dialog = GIMP_COLOR_DIALOG (object);

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
gimp_color_dialog_style_updated (GtkWidget *widget)
{
  GimpColorDialog   *dialog = GIMP_COLOR_DIALOG (widget);
  GimpColorNotebook *notebook;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  notebook =
    GIMP_COLOR_NOTEBOOK (gimp_color_selection_get_notebook (GIMP_COLOR_SELECTION (dialog->selection)));
  GTK_WIDGET_GET_CLASS (notebook)->style_updated (GTK_WIDGET (notebook));
}

static void
gimp_color_dialog_response (GtkDialog *gtk_dialog,
                            gint       response_id)
{
  GimpColorDialog       *dialog          = GIMP_COLOR_DIALOG (gtk_dialog);
  GimpViewableDialog    *viewable_dialog = GIMP_VIEWABLE_DIALOG (dialog);
  GimpImage             *image           = NULL;
  GimpColormapSelection *colormap_selection;
  gint                   col_index;
  GeglColor             *color           = NULL;

  colormap_selection = GIMP_COLORMAP_SELECTION (dialog->colormap_selection);
  col_index = gimp_colormap_selection_get_index (colormap_selection, NULL);

  if (dialog->colormap_editing && viewable_dialog->context)
    {
      GimpContext *user_context = viewable_dialog->context->gimp->user_context;

      image = gimp_context_get_image (user_context);
    }

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_color_selection_reset (GIMP_COLOR_SELECTION (dialog->selection));
      break;

    case GTK_RESPONSE_OK:
      color = gimp_color_selection_get_color (GIMP_COLOR_SELECTION (dialog->selection));

      if (dialog->colormap_editing && image)
        {
          GeglColor *old_color;

          dialog->colormap_editing = FALSE;

          /* Restore old color for undo */
          old_color = gimp_color_selection_get_old_color (GIMP_COLOR_SELECTION (dialog->selection));
          gimp_image_set_colormap_entry (image, col_index, old_color, FALSE);
          gimp_image_set_colormap_entry (image, col_index, color, TRUE);
          gimp_image_flush (image);

          gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "colormap");
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         color, GIMP_COLOR_DIALOG_UPDATE);

          g_object_unref (old_color);
        }
      else
        {
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         color, GIMP_COLOR_DIALOG_OK);
        }

      g_object_unref (color);
      break;

    default:
      color = gimp_color_selection_get_old_color (GIMP_COLOR_SELECTION (dialog->selection));

      if (dialog->colormap_editing && image)
        {
          dialog->colormap_editing = FALSE;

          gimp_image_set_colormap_entry (image, col_index, color, FALSE);
          gimp_projection_flush (gimp_image_get_projection (image));

          gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "colormap");
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         color, GIMP_COLOR_DIALOG_UPDATE);
        }
      else
        {
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         color, GIMP_COLOR_DIALOG_CANCEL);
        }

      g_object_unref (color);
      break;
    }
}


/*  public functions  */

GtkWidget *
gimp_color_dialog_new (GimpViewable      *viewable,
                       GimpContext       *context,
                       gboolean           user_context_aware,
                       const gchar       *title,
                       const gchar       *icon_name,
                       const gchar       *desc,
                       GtkWidget         *parent,
                       GimpDialogFactory *dialog_factory,
                       const gchar       *dialog_identifier,
                       GeglColor         *color,
                       gboolean           wants_updates,
                       gboolean           show_alpha)
{
  GimpColorDialog *dialog;
  const gchar     *role;
  gboolean         use_header_bar;

  g_return_val_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (dialog_factory == NULL || dialog_identifier != NULL,
                        NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  role = dialog_identifier ? dialog_identifier : "gimp-color-selector";

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (GIMP_TYPE_COLOR_DIALOG,
                         "title",              title,
                         "role",               role,
                         "help-func",          gimp_color_dialog_help_func,
                         "help-id",            GIMP_HELP_COLOR_DIALOG,
                         "icon-name",          icon_name,
                         "description",        desc,
                         "context",            context,
                         "user-context-aware", user_context_aware,
                         "parent",             gtk_widget_get_toplevel (parent),
                         "use-header-bar",     use_header_bar,
                         NULL);

  gimp_dialog_add_buttons (GIMP_DIALOG (dialog),

                           _("_Reset"),  RESPONSE_RESET,
                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                           _("_OK"),     GTK_RESPONSE_OK,

                           NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            RESPONSE_RESET,
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  if (viewable)
    {
      gimp_viewable_dialog_set_viewables (GIMP_VIEWABLE_DIALOG (dialog),
                                          g_list_prepend (NULL, viewable), context);
    }
  else
    {
      GtkWidget *parent;

      parent = gtk_widget_get_parent (GIMP_VIEWABLE_DIALOG (dialog)->icon);
      parent = gtk_widget_get_parent (parent);

      gtk_widget_hide (parent);
    }

  dialog->wants_updates = wants_updates;

  if (dialog_factory)
    {
      gimp_dialog_factory_add_foreign (dialog_factory, dialog_identifier,
                                       GTK_WIDGET (dialog),
                                       gimp_widget_get_monitor (parent));
    }

  gimp_color_selection_set_show_alpha (GIMP_COLOR_SELECTION (dialog->selection),
                                       show_alpha);

  g_object_set_data (G_OBJECT (context->gimp->config->color_management),
                     "gimp-context", context);

  gimp_color_selection_set_config (GIMP_COLOR_SELECTION (dialog->selection),
                                   context->gimp->config->color_management);

  g_object_set_data (G_OBJECT (context->gimp->config->color_management),
                     "gimp-context", NULL);

  gimp_color_selection_set_color (GIMP_COLOR_SELECTION (dialog->selection),
                                  color);
  gimp_color_selection_set_old_color (GIMP_COLOR_SELECTION (dialog->selection),
                                      color);

  return GTK_WIDGET (dialog);
}

void
gimp_color_dialog_set_color (GimpColorDialog *dialog,
                             GeglColor       *color)
{
  g_return_if_fail (GIMP_IS_COLOR_DIALOG (dialog));
  g_return_if_fail (GEGL_IS_COLOR (color));

  g_signal_handlers_block_by_func (dialog->selection,
                                   gimp_color_dialog_color_changed,
                                   dialog);

  gimp_color_selection_set_color (GIMP_COLOR_SELECTION (dialog->selection),
                                  color);
  gimp_color_selection_set_old_color (GIMP_COLOR_SELECTION (dialog->selection),
                                      color);

  g_signal_handlers_unblock_by_func (dialog->selection,
                                     gimp_color_dialog_color_changed,
                                     dialog);
}

GeglColor *
gimp_color_dialog_get_color (GimpColorDialog *dialog)
{
  g_return_val_if_fail (GIMP_IS_COLOR_DIALOG (dialog), NULL);

  return gimp_color_selection_get_color (GIMP_COLOR_SELECTION (dialog->selection));
}


/*  private functions  */

static void
gimp_color_dialog_help_func (const gchar *help_id,
                             gpointer     help_data)
{
  GimpColorDialog   *dialog = GIMP_COLOR_DIALOG (help_data);
  GimpColorNotebook *notebook;
  GimpColorSelector *current;

  notebook =
    GIMP_COLOR_NOTEBOOK (gimp_color_selection_get_notebook (GIMP_COLOR_SELECTION (dialog->selection)));

  current = gimp_color_notebook_get_current_selector (notebook);

  help_id = GIMP_COLOR_SELECTOR_GET_CLASS (current)->help_id;

  gimp_standard_help_func (help_id, NULL);
}

static void
gimp_color_dialog_colormap_clicked (GimpColorDialog  *dialog,
                                    GimpPaletteEntry *entry,
                                    GdkModifierType   state)
{
  gimp_color_dialog_set_color (dialog, entry->color);

  if (dialog->wants_updates)
    g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                   entry->color, GIMP_COLOR_DIALOG_UPDATE);
}

static void
gimp_color_dialog_colormap_edit_activate (GimpColorDialog *dialog)
{
  GimpColormapSelection *colormap_selection;
  GeglColor             *color;
  gint                   col_index;

  dialog->colormap_editing = TRUE;

  colormap_selection = GIMP_COLORMAP_SELECTION (dialog->colormap_selection);
  col_index = gimp_colormap_selection_get_index (colormap_selection, NULL);
  color = gimp_image_get_colormap_entry (dialog->active_image, col_index);
  gimp_color_dialog_set_color (dialog, color);

  gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "color");
}

static void
gimp_color_dialog_colormap_add_activate (GimpColorDialog *dialog)
{
  GimpViewableDialog *viewable_dialog = GIMP_VIEWABLE_DIALOG (dialog);

  if (dialog->active_image &&
      gimp_palette_get_n_colors (gimp_image_get_colormap_palette (dialog->active_image)) < 256 &&
      viewable_dialog->context)
    {
      GimpContext *user_context = viewable_dialog->context->gimp->user_context;
      GeglColor   *color        = gimp_context_get_foreground (user_context);

      gimp_image_add_colormap_entry (dialog->active_image, color);
      gimp_image_flush (dialog->active_image);
    }
}

static void
gimp_color_dialog_color_changed (GimpColorSelection *selection,
                                 GimpColorDialog    *dialog)
{
  GimpViewableDialog *viewable_dialog = GIMP_VIEWABLE_DIALOG (dialog);
  GeglColor          *color;

  color = gimp_color_selection_get_color (selection);

  if (dialog->colormap_editing && viewable_dialog->context)
    {
      GimpContext *user_context = viewable_dialog->context->gimp->user_context;
      GimpImage   *image;

      image = gimp_context_get_image (user_context);
      if (image)
        {
          GimpColormapSelection *colormap_selection;
          gboolean               push_undo = FALSE;
          gint                   col_index;

          colormap_selection = GIMP_COLORMAP_SELECTION (dialog->colormap_selection);
          col_index = gimp_colormap_selection_get_index (colormap_selection, NULL);
          if (push_undo)
            {
              GeglColor *old_color;

              old_color = gimp_color_selection_get_old_color (GIMP_COLOR_SELECTION (dialog->selection));

              /* Restore old color for undo */
              gimp_image_set_colormap_entry (image, col_index, old_color, FALSE);

              g_object_unref (old_color);
            }

          gimp_image_set_colormap_entry (image, col_index, color, push_undo);

          if (push_undo)
            gimp_image_flush (image);
          else
            gimp_projection_flush (gimp_image_get_projection (image));
        }
    }

  if (dialog->wants_updates)
    g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                   color, GIMP_COLOR_DIALOG_UPDATE);

  g_object_unref (color);
}


/* History-adding button callback */

static void
gimp_color_history_add_clicked (GtkWidget       *widget,
                                GimpColorDialog *dialog)
{
  GimpViewableDialog *viewable_dialog = GIMP_VIEWABLE_DIALOG (dialog);
  GimpPalette        *history;
  GeglColor          *color;

  history = gimp_palettes_get_color_history (viewable_dialog->context->gimp);
  color   = gimp_color_selection_get_color (GIMP_COLOR_SELECTION (dialog->selection));
  gimp_palette_mru_add (GIMP_PALETTE_MRU (history), color);
}

/* Context-related callbacks */

static void
gimp_color_dialog_image_changed (GimpContext     *context,
                                 GimpImage       *image,
                                 GimpColorDialog *dialog)
{
  if (dialog->active_image != image)
    {
      if (dialog->active_image)
        {
          g_signal_handlers_disconnect_by_func (dialog->active_image,
                                                G_CALLBACK (gimp_color_dialog_update_base_type),
                                                dialog);
          g_signal_handlers_disconnect_by_func (dialog->active_image,
                                                gimp_color_dialog_update_simulation,
                                                dialog);
        }

      g_set_weak_pointer (&dialog->active_image, image);

      if (image)
        {
          g_signal_connect_object (image, "notify::base-type",
                                   G_CALLBACK (gimp_color_dialog_update_base_type),
                                   dialog, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "profile-changed",
                                   G_CALLBACK (gimp_color_dialog_update_format),
                                   dialog, 0);
          g_signal_connect_object (image, "simulation-profile-changed",
                                   G_CALLBACK (gimp_color_dialog_update_simulation),
                                   dialog, 0);
          g_signal_connect_object (image, "simulation-intent-changed",
                                   G_CALLBACK (gimp_color_dialog_update_simulation),
                                   dialog, 0);
          g_signal_connect_object (image, "simulation-bpc-changed",
                                   G_CALLBACK (gimp_color_dialog_update_simulation),
                                   dialog, 0);

          gimp_color_dialog_update_simulation (image, dialog);
        }

      gimp_color_dialog_update_base_type (dialog);
      gimp_color_dialog_update_format (image, dialog);
    }
}

static void
gimp_color_dialog_update_format (GimpImage       *image,
                                 GimpColorDialog *dialog)
{
  const Babl *format = NULL;

  g_return_if_fail (GIMP_IS_COLOR_DIALOG (dialog));

  if (image)
    format = gimp_image_get_layer_format (image, FALSE);

  gimp_color_selection_set_format (GIMP_COLOR_SELECTION (dialog->selection), format);
}

static void
gimp_color_dialog_update_simulation (GimpImage       *image,
                                     GimpColorDialog *dialog)
{
  GimpColorProfile         *profile = NULL;
  GimpColorRenderingIntent  intent  = GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  gboolean                  bpc     = FALSE;

  g_return_if_fail (GIMP_IS_COLOR_DIALOG (dialog));

  if (image)
    {
      profile = gimp_image_get_simulation_profile (image);
      intent  = gimp_image_get_simulation_intent (image);
      bpc     = gimp_image_get_simulation_bpc (image);
    }

  gimp_color_selection_set_simulation (GIMP_COLOR_SELECTION (dialog->selection),
                                       profile, intent, bpc);
}

static void
gimp_color_dialog_update_base_type (GimpColorDialog *dialog)
{
  if (dialog->active_image &&
      gimp_image_get_base_type (dialog->active_image) == GIMP_INDEXED)
    gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "colormap");
  else
    gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "color");
}

static void
gimp_color_dialog_show (GimpColorDialog *dialog)
{
  GimpViewableDialog *viewable_dialog = GIMP_VIEWABLE_DIALOG (dialog);

  dialog->colormap_editing = FALSE;

  if (dialog->user_context_aware && viewable_dialog->context)
    {
      GimpContext *user_context = viewable_dialog->context->gimp->user_context;
      GimpImage   *image        = gimp_context_get_image (user_context);

      g_signal_connect_object (user_context, "image-changed",
                               G_CALLBACK (gimp_color_dialog_image_changed),
                               dialog, 0);
      g_signal_connect_object (viewable_dialog->context->gimp->config,
                               "notify::theme",
                               G_CALLBACK (gimp_color_dialog_style_updated),
                               dialog, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (viewable_dialog->context->gimp->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (gimp_color_dialog_style_updated),
                               dialog, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (viewable_dialog->context->gimp->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (gimp_color_dialog_style_updated),
                               dialog, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

      gimp_color_dialog_image_changed (viewable_dialog->context,
                                       image, dialog);
      gimp_color_dialog_update_base_type (dialog);
      gimp_color_dialog_style_updated (GTK_WIDGET (dialog));
    }
  else
    {
      gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "color");
    }
}

static void
gimp_color_dialog_hide (GimpColorDialog *dialog)
{
  GimpViewableDialog *viewable_dialog = GIMP_VIEWABLE_DIALOG (dialog);

  if (dialog->user_context_aware && viewable_dialog->context)
    {
      GimpContext *user_context = viewable_dialog->context->gimp->user_context;
      g_signal_handlers_disconnect_by_func (user_context,
                                            G_CALLBACK (gimp_color_dialog_image_changed),
                                            dialog);
      g_signal_handlers_disconnect_by_func (viewable_dialog->context->gimp->config,
                                            G_CALLBACK (gimp_color_dialog_style_updated),
                                            dialog);
    }
}
