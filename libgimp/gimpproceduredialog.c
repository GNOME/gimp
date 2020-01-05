/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpproceduredialog.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#include "gimp.h"
#include "gimpui.h"

#include "gimpprocedureconfig-private.h"

#include "libgimp-intl.h"


enum
{
  PROP_0,
  PROP_PROCEDURE,
  PROP_CONFIG,
  N_PROPS
};

#define RESPONSE_RESET 1


struct _GimpProcedureDialogPrivate
{
  GimpProcedure       *procedure;
  GimpProcedureConfig *config;
  GimpProcedureConfig *initial_config;

  GtkWidget           *reset_popover;
};


static void   gimp_procedure_dialog_dispose       (GObject      *object);
static void   gimp_procedure_dialog_set_property  (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void   gimp_procedure_dialog_get_property  (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);

static void   gimp_procedure_dialog_reset_initial (GtkWidget           *button,
                                                   GimpProcedureDialog *dialog);
static void   gimp_procedure_dialog_reset_factory (GtkWidget           *button,
                                                   GimpProcedureDialog *dialog);
static void   gimp_procedure_dialog_load_defaults (GtkWidget           *button,
                                                   GimpProcedureDialog *dialog);
static void   gimp_procedure_dialog_save_defaults (GtkWidget           *button,
                                                   GimpProcedureDialog *dialog);


G_DEFINE_TYPE_WITH_PRIVATE (GimpProcedureDialog, gimp_procedure_dialog,
                            GIMP_TYPE_DIALOG)

#define parent_class gimp_procedure_dialog_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_procedure_dialog_class_init (GimpProcedureDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gimp_procedure_dialog_dispose;
  object_class->get_property = gimp_procedure_dialog_get_property;
  object_class->set_property = gimp_procedure_dialog_set_property;

  props[PROP_PROCEDURE] =
    g_param_spec_object ("procedure",
                         "Procedure",
                         "The GimpProcedure this dialog is used with",
                         GIMP_TYPE_PROCEDURE,
                         GIMP_PARAM_READWRITE);

  props[PROP_CONFIG] =
    g_param_spec_object ("config",
                         "Config",
                         "The GimpProcedureConfig this dialog is editing",
                         GIMP_TYPE_PROCEDURE_CONFIG,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_procedure_dialog_init (GimpProcedureDialog *dialog)
{
  dialog->priv = gimp_procedure_dialog_get_instance_private (dialog);
}

static void
gimp_procedure_dialog_dispose (GObject *object)
{
  GimpProcedureDialog *dialog = GIMP_PROCEDURE_DIALOG (object);

  g_clear_object (&dialog->priv->procedure);
  g_clear_object (&dialog->priv->config);
  g_clear_object (&dialog->priv->initial_config);

  g_clear_pointer (&dialog->priv->reset_popover, gtk_widget_destroy);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_procedure_dialog_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpProcedureDialog *dialog = GIMP_PROCEDURE_DIALOG (object);

  switch (property_id)
    {
    case PROP_PROCEDURE:
      dialog->priv->procedure = g_value_dup_object (value);
      break;

    case PROP_CONFIG:
      dialog->priv->config = g_value_dup_object (value);

      if (dialog->priv->config)
        dialog->priv->initial_config =
          gimp_config_duplicate (GIMP_CONFIG (dialog->priv->config));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_procedure_dialog_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpProcedureDialog *dialog = GIMP_PROCEDURE_DIALOG (object);

  switch (property_id)
    {
    case PROP_PROCEDURE:
      g_value_set_object (value, dialog->priv->procedure);
      break;

    case PROP_CONFIG:
      g_value_set_object (value, dialog->priv->config);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_procedure_dialog_new (GimpProcedure       *procedure,
                           GimpProcedureConfig *config,
                           const gchar         *title)
{
  GtkWidget   *dialog;
  gchar       *role;
  const gchar *help_id;
  const gchar *ok_label;
  gboolean     use_header_bar;
  GtkWidget   *hbox;
  GtkWidget   *button;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (gimp_procedure_config_get_procedure (config) ==
                        procedure, NULL);
  g_return_val_if_fail (title != NULL, NULL);

  role = g_strdup_printf ("gimp-%s", gimp_procedure_get_name (procedure));

  help_id = gimp_procedure_get_help_id (procedure);

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (GIMP_TYPE_PROCEDURE_DIALOG,
                         "procedure",      procedure,
                         "config",         config,
                         "title",          title,
                         "role",           role,
                         "help-func",      gimp_standard_help_func,
                         "help-id",        help_id,
                         "use-header-bar", use_header_bar,
                         NULL);

  g_free (role);

  if (GIMP_IS_LOAD_PROCEDURE (procedure))
    ok_label = _("_Open");
  else if (GIMP_IS_SAVE_PROCEDURE (procedure))
    ok_label = _("_Export");
  else
    ok_label = _("_OK");

  gimp_dialog_add_buttons (GIMP_DIALOG (dialog),
                           _("_Reset"),  RESPONSE_RESET,
                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                           ok_label,     GTK_RESPONSE_OK,
                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            RESPONSE_RESET,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  hbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_set_spacing (GTK_BOX (hbox), 6);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_START);
  gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                    hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Load Defaults"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_procedure_dialog_load_defaults),
                    dialog);

  button = gtk_button_new_with_mnemonic (_("_Save Defaults"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_procedure_dialog_save_defaults),
                    dialog);

  return GTK_WIDGET (dialog);
}

gboolean
gimp_procedure_dialog_run (GimpProcedureDialog *dialog)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), FALSE);

  while (TRUE)
    {
      gint response = gimp_dialog_run (GIMP_DIALOG (dialog));

      if (response == RESPONSE_RESET)
        {
          if (! dialog->priv->reset_popover)
            {
              GtkWidget *button;
              GtkWidget *vbox;

              button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog),
                                                           response);

              dialog->priv->reset_popover = gtk_popover_new (button);

              vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
              gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
              gtk_container_add (GTK_CONTAINER (dialog->priv->reset_popover),
                                 vbox);
              gtk_widget_show (vbox);

              button = gtk_button_new_with_mnemonic (_("Reset to _Initial "
                                                       "Values"));
              gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
              gtk_widget_show (button);

              g_signal_connect (button, "clicked",
                                G_CALLBACK (gimp_procedure_dialog_reset_initial),
                                dialog);

              button = gtk_button_new_with_mnemonic (_("Reset to _Factory "
                                                       "Defaults"));
              gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
              gtk_widget_show (button);

              g_signal_connect (button, "clicked",
                                G_CALLBACK (gimp_procedure_dialog_reset_factory),
                                dialog);
            }

          gtk_popover_popup (GTK_POPOVER (dialog->priv->reset_popover));
        }
      else
        {
          return response == GTK_RESPONSE_OK;
        }
    }
}


/*  private functions  */

static void
gimp_procedure_dialog_reset_initial (GtkWidget           *button,
                                     GimpProcedureDialog *dialog)
{
  gimp_config_copy (GIMP_CONFIG (dialog->priv->initial_config),
                    GIMP_CONFIG (dialog->priv->config),
                    0);

  gtk_popover_popdown (GTK_POPOVER (dialog->priv->reset_popover));
}

static void
gimp_procedure_dialog_reset_factory (GtkWidget           *button,
                                     GimpProcedureDialog *dialog)
{
  gimp_config_reset (GIMP_CONFIG (dialog->priv->config));

  gtk_popover_popdown (GTK_POPOVER (dialog->priv->reset_popover));
}

static void
gimp_procedure_dialog_load_defaults (GtkWidget           *button,
                                     GimpProcedureDialog *dialog)
{
  GError *error = NULL;

  if (! gimp_procedure_config_load_default (dialog->priv->config, &error))
    {
      if (error)
        {
          g_printerr ("Loading default values from disk failed: %s\n",
                      error->message);
          g_clear_error (&error);
        }
      else
        {
          g_printerr ("No default values found on disk\n");
        }
    }
}

static void
gimp_procedure_dialog_save_defaults (GtkWidget           *button,
                                     GimpProcedureDialog *dialog)
{
  GError *error = NULL;

  if (! gimp_procedure_config_save_default (dialog->priv->config, &error))
    {
      g_printerr ("Saving default values to disk failed: %s\n",
                  error->message);
      g_clear_error (&error);
    }
}
