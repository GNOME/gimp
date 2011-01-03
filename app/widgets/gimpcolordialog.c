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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimp-palettes.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"
#include "core/gimppalettemru.h"

#include "gimpcolordialog.h"
#include "gimpcolorhistory.h"
#include "gimpdialogfactory.h"
#include "gimphelp-ids.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define RESPONSE_RESET   1
#define COLOR_AREA_SIZE 20


enum
{
  UPDATE,
  LAST_SIGNAL
};


static void   gimp_color_dialog_constructed      (GObject            *object);

static void   gimp_color_dialog_response         (GtkDialog          *dialog,
                                                  gint                response_id);

static void   gimp_color_dialog_help_func        (const gchar        *help_id,
                                                  gpointer            help_data);
static void   gimp_color_dialog_color_changed    (GimpColorSelection *selection,
                                                  GimpColorDialog    *dialog);

static void   gimp_color_history_add_clicked     (GtkWidget          *widget,
                                                  GimpColorDialog    *dialog);

static void   gimp_color_dialog_history_selected (GimpColorHistory   *history,
                                                  const GimpRGB      *rgb,
                                                  GimpColorDialog    *dialog);

G_DEFINE_TYPE (GimpColorDialog, gimp_color_dialog, GIMP_TYPE_VIEWABLE_DIALOG)

#define parent_class gimp_color_dialog_parent_class

static guint color_dialog_signals[LAST_SIGNAL] = { 0, };


static void
gimp_color_dialog_class_init (GimpColorDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->constructed = gimp_color_dialog_constructed;

  dialog_class->response    = gimp_color_dialog_response;

  color_dialog_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpColorDialogClass, update),
                  NULL, NULL,
                  gimp_marshal_VOID__BOXED_ENUM,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_RGB,
                  GIMP_TYPE_COLOR_DIALOG_STATE);
}

static void
gimp_color_dialog_init (GimpColorDialog *dialog)
{
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),

                          _("_Reset"),  RESPONSE_RESET,
                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                          _("_OK"),     GTK_RESPONSE_OK,

                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  dialog->selection = gimp_color_selection_new ();
  gtk_container_set_border_width (GTK_CONTAINER (dialog->selection), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      dialog->selection, TRUE, TRUE, 0);
  gtk_widget_show (dialog->selection);

  g_signal_connect (dialog->selection, "color-changed",
                    G_CALLBACK (gimp_color_dialog_color_changed),
                    dialog);
}

static void
gimp_color_dialog_constructed (GObject *object)
{
  GimpColorDialog    *dialog          = GIMP_COLOR_DIALOG (object);
  GimpViewableDialog *viewable_dialog = GIMP_VIEWABLE_DIALOG (object);
  GtkWidget          *hbox;
  GtkWidget          *history;
  GtkWidget          *button;
  GtkWidget          *arrow;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* Color history box. */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_end (GTK_BOX (gimp_color_selection_get_right_vbox (GIMP_COLOR_SELECTION (dialog->selection))),
                    hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Button for adding to color history. */
  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (button), FALSE, FALSE, 0);
  gimp_help_set_help_data (button,
                           _("Add the current color to the color history"),
                           NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_color_history_add_clicked),
                    dialog);

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (button), arrow);
  gtk_widget_show (arrow);

  /* Color history table. */
  history = gimp_color_history_new (viewable_dialog->context, 12);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (history), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (history));

  g_signal_connect (history, "color-selected",
                    G_CALLBACK (gimp_color_dialog_history_selected),
                    dialog);
}

static void
gimp_color_dialog_response (GtkDialog *gtk_dialog,
                            gint       response_id)
{
  GimpColorDialog *dialog = GIMP_COLOR_DIALOG (gtk_dialog);
  GimpRGB          color;

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_color_selection_reset (GIMP_COLOR_SELECTION (dialog->selection));
      break;

    case GTK_RESPONSE_OK:
      gimp_color_selection_get_color (GIMP_COLOR_SELECTION (dialog->selection),
                                      &color);

      g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                     &color, GIMP_COLOR_DIALOG_OK);
      break;

    default:
      gimp_color_selection_get_old_color (GIMP_COLOR_SELECTION (dialog->selection),
                                          &color);

      g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                     &color, GIMP_COLOR_DIALOG_CANCEL);
      break;
    }
}


/*  public functions  */

GtkWidget *
gimp_color_dialog_new (GimpViewable      *viewable,
                       GimpContext       *context,
                       const gchar       *title,
                       const gchar       *icon_name,
                       const gchar       *desc,
                       GtkWidget         *parent,
                       GimpDialogFactory *dialog_factory,
                       const gchar       *dialog_identifier,
                       const GimpRGB     *color,
                       gboolean           wants_updates,
                       gboolean           show_alpha)
{
  GimpColorDialog *dialog;
  const gchar     *role;

  g_return_val_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (dialog_factory == NULL || dialog_identifier != NULL,
                        NULL);
  g_return_val_if_fail (color != NULL, NULL);

  role = dialog_identifier ? dialog_identifier : "gimp-color-selector";

  dialog = g_object_new (GIMP_TYPE_COLOR_DIALOG,
                         "title",       title,
                         "role",        role,
                         "help-func",   gimp_color_dialog_help_func,
                         "help-id",     GIMP_HELP_COLOR_DIALOG,
                         "icon-name",   icon_name,
                         "description", desc,
                         "context",     context,
                         "parent",      parent,
                         NULL);

  if (viewable)
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (dialog),
                                         viewable, context);
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
                                       gtk_widget_get_screen (parent),
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
                             const GimpRGB   *color)
{
  g_return_if_fail (GIMP_IS_COLOR_DIALOG (dialog));
  g_return_if_fail (color != NULL);

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

void
gimp_color_dialog_get_color (GimpColorDialog *dialog,
                             GimpRGB         *color)
{
  g_return_if_fail (GIMP_IS_COLOR_DIALOG (dialog));
  g_return_if_fail (color != NULL);

  gimp_color_selection_get_color (GIMP_COLOR_SELECTION (dialog->selection),
                                  color);
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
gimp_color_dialog_color_changed (GimpColorSelection *selection,
                                 GimpColorDialog    *dialog)
{
  if (dialog->wants_updates)
    {
      GimpRGB color;

      gimp_color_selection_get_color (selection, &color);

      g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                     &color, GIMP_COLOR_DIALOG_UPDATE);
    }
}


/* History-adding button callback */

static void
gimp_color_history_add_clicked (GtkWidget       *widget,
                                GimpColorDialog *dialog)
{
  GimpViewableDialog *viewable_dialog = GIMP_VIEWABLE_DIALOG (dialog);
  GimpPalette        *history;
  GimpRGB             color;

  history = gimp_palettes_get_color_history (viewable_dialog->context->gimp);

  gimp_color_selection_get_color (GIMP_COLOR_SELECTION (dialog->selection),
                                  &color);

  gimp_palette_mru_add (GIMP_PALETTE_MRU (history), &color);
}

/* Color history callback  */

static void
gimp_color_dialog_history_selected (GimpColorHistory *history,
                                    const GimpRGB    *rgb,
                                    GimpColorDialog  *dialog)
{
  gimp_color_selection_set_color (GIMP_COLOR_SELECTION (dialog->selection),
                                  rgb);
}
