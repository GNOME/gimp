/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmodifierseditor.c
 * Copyright (C) 2022 Jehan
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
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "display/display-types.h"
#include "display/gimpmodifiersmanager.h"

#include "gimpmodifierseditor.h"
#include "gimpshortcutbutton.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_MANAGER,
};

struct _GimpModifiersEditorPrivate
{
  GdkDevice            *device;
  guint                 button;

  GtkWidget            *header;
  GtkWidget            *warning;
  GtkWidget            *select_button;

  GtkWidget            *stack;
  GtkWidget            *current_settings;
  GtkWidget            *plus_button;

  GtkSizeGroup         *mod_size_group;
  GtkSizeGroup         *action_size_group;
  GtkSizeGroup         *minus_size_group;

  GimpModifiersManager *manager;
};


static void     gimp_modifiers_editor_constructed          (GObject             *object);
static void     gimp_modifiers_editor_finalize             (GObject             *object);
static void     gimp_modifiers_editor_set_property         (GObject             *object,
                                                            guint                property_id,
                                                            const GValue        *value,
                                                            GParamSpec          *pspec);
static void     gimp_modifiers_editor_get_property         (GObject             *object,
                                                            guint                property_id,
                                                            GValue              *value,
                                                            GParamSpec          *pspec);

static gboolean gimp_modifiers_editor_button_press_event   (GtkWidget           *editor,
                                                            GdkEventButton      *event,
                                                            gpointer             user_data);
static void     gimp_modifiers_editor_plus_button_clicked  (GtkButton           *button,
                                                            GimpModifiersEditor *editor);
static void     gimp_modifiers_editor_minus_button_clicked (GtkButton           *minus_button,
                                                            GimpModifiersEditor *editor);
static void     gimp_modifiers_editor_notify_accelerator   (GtkWidget           *widget,
                                                            const GParamSpec    *pspec,
                                                            GimpModifiersEditor *editor);

static void     gimp_modifiers_editor_show_settings         (GimpModifiersEditor *editor,
                                                             GdkDevice           *device,
                                                             guint                button);
static void     gimp_modifiers_editor_add_mapping           (GimpModifiersEditor *editor,
                                                             GdkModifierType      modifiers,
                                                             GimpModifierAction   mod_action);

G_DEFINE_TYPE_WITH_PRIVATE (GimpModifiersEditor, gimp_modifiers_editor,
                            GIMP_TYPE_FRAME)

#define parent_class gimp_modifiers_editor_parent_class


static void
gimp_modifiers_editor_class_init (GimpModifiersEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_modifiers_editor_constructed;
  object_class->finalize     = gimp_modifiers_editor_finalize;
  object_class->get_property = gimp_modifiers_editor_get_property;
  object_class->set_property = gimp_modifiers_editor_set_property;

  g_object_class_install_property (object_class, PROP_MANAGER,
                                   g_param_spec_object ("manager", NULL, NULL,
                                                        GIMP_TYPE_MODIFIERS_MANAGER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_modifiers_editor_init (GimpModifiersEditor *editor)
{
  GtkWidget *grid;
  GtkWidget *hbox;
  GtkWidget *hint;
  GtkWidget *image;
  gchar     *text;

  editor->priv = gimp_modifiers_editor_get_instance_private (editor);
  editor->priv->device            = NULL;
  editor->priv->plus_button       = NULL;
  editor->priv->current_settings  = NULL;
  editor->priv->mod_size_group    = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  editor->priv->action_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  editor->priv->minus_size_group  = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* Setup the title. */
  gtk_frame_set_label_align (GTK_FRAME (editor), 0.5, 0.5);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);

  editor->priv->header = gtk_label_new (NULL);
  gtk_grid_attach (GTK_GRID (grid), editor->priv->header, 0, 0, 2, 1);
  gtk_widget_show (editor->priv->header);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_show (hbox);

  hint = gtk_label_new (NULL);
  text = g_strdup_printf ("<i>%s</i>", _("Click here to set a button's modifiers"));
  gtk_label_set_markup (GTK_LABEL (hint), text);
  g_free (text);
  gtk_box_pack_start (GTK_BOX (hbox), hint, TRUE, TRUE, 2);
  gtk_widget_show (hint);

  image = gtk_image_new_from_icon_name ("gimp-cursor", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 2);
  gtk_widget_show (image);

  editor->priv->warning = gtk_label_new (NULL);
  text = g_strdup_printf ("<b>%s</b>",
                          _("Modifiers cannot be customized on the primary button."));
  gtk_label_set_markup (GTK_LABEL (editor->priv->warning), text);
  g_free (text);
  gtk_grid_attach (GTK_GRID (grid), editor->priv->warning, 0, 2, 2, 1);
  gtk_widget_hide (editor->priv->warning);

  editor->priv->select_button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (editor->priv->select_button), hbox);
  gtk_grid_attach (GTK_GRID (grid), editor->priv->select_button, 0, 1, 1, 1);
  gtk_widget_show (editor->priv->select_button);

  gtk_frame_set_label_widget (GTK_FRAME (editor), grid);
  gtk_widget_show (grid);

  /* Setup the stack. */
  editor->priv->stack = gtk_stack_new ();
  gtk_container_add (GTK_CONTAINER (editor), editor->priv->stack);
  gtk_widget_show (editor->priv->stack);
}

static void
gimp_modifiers_editor_constructed (GObject *object)
{
  GimpModifiersEditor *editor = GIMP_MODIFIERS_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect (editor->priv->select_button, "button-press-event",
                    G_CALLBACK (gimp_modifiers_editor_button_press_event),
                    object);
}

static void
gimp_modifiers_editor_finalize (GObject *object)
{
  GimpModifiersEditor *editor = GIMP_MODIFIERS_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);

  g_clear_object (&editor->priv->device);
  g_object_unref (editor->priv->mod_size_group);
  g_object_unref (editor->priv->action_size_group);
  g_object_unref (editor->priv->minus_size_group);
}

static void
gimp_modifiers_editor_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpModifiersEditor *editor = GIMP_MODIFIERS_EDITOR (object);

  switch (property_id)
    {
    case PROP_MANAGER:
      editor->priv->manager = g_value_get_object (value);
      break;


   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_modifiers_editor_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpModifiersEditor *editor = GIMP_MODIFIERS_EDITOR (object);

  switch (property_id)
    {
    case PROP_MANAGER:
      g_value_set_object (value, editor->priv->manager);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*  public functions  */

GtkWidget *
gimp_modifiers_editor_new (GimpModifiersManager *manager)
{
  GimpModifiersEditor *editor;

  g_return_val_if_fail (GIMP_IS_MODIFIERS_MANAGER (manager), NULL);

  editor = g_object_new (GIMP_TYPE_MODIFIERS_EDITOR,
                         "manager", manager,
                         NULL);

  return GTK_WIDGET (editor);
}

void
gimp_modifiers_editor_clear (GimpModifiersEditor *editor)
{
  gimp_modifiers_manager_clear (editor->priv->manager);
  gtk_container_foreach (GTK_CONTAINER (editor->priv->stack),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);
  gimp_modifiers_editor_show_settings (editor, editor->priv->device, editor->priv->button);
}

/*  private functions  */

static void
gimp_modifiers_editor_show_settings (GimpModifiersEditor *editor,
                                     GdkDevice           *device,
                                     guint                button)
{
  const gchar *vendor_id;
  const gchar *product_id;
  gchar       *title;
  gchar       *text;

  vendor_id  = gdk_device_get_vendor_id (device);
  product_id = gdk_device_get_product_id (device);

  if (device != editor->priv->device)
    {
      g_clear_object (&editor->priv->device);
      editor->priv->device = g_object_ref (device);
    }
  editor->priv->button = button;

  /* Update header. */
  if (gdk_device_get_name (device) != NULL)
    text = g_strdup_printf (_("Editing modifiers for button %d of %s"),
                            editor->priv->button,
                            gdk_device_get_name (device));
  else
    text = g_strdup_printf (_("Editing modifiers for button %d"),
                            editor->priv->button);

  title = g_strdup_printf ("<b><big>%s</big></b>", text);
  gtk_label_set_markup (GTK_LABEL (editor->priv->header), title);

  g_free (title);
  g_free (text);

  /* Update modifier settings. */
  text = g_strdup_printf ("%s:%s-%d",
                          vendor_id ? vendor_id : "*",
                          product_id ? product_id : "*",
                          button);
  editor->priv->current_settings = gtk_stack_get_child_by_name (GTK_STACK (editor->priv->stack), text);

  if (! editor->priv->current_settings)
    {
      GtkWidget *plus_button;
      GList     *modifiers;
      GList     *iter;

      editor->priv->current_settings = gtk_list_box_new ();
      gtk_stack_add_named (GTK_STACK (editor->priv->stack), editor->priv->current_settings, text);

      modifiers = gimp_modifiers_manager_get_modifiers (editor->priv->manager,
                                                        device, editor->priv->button);
      for (iter = modifiers; iter; iter = iter->next)
        {
          GdkModifierType    mods = GPOINTER_TO_INT (iter->data);
          GimpModifierAction action;

          action = gimp_modifiers_manager_get_action (editor->priv->manager, device,
                                                      editor->priv->button, mods);
          gimp_modifiers_editor_add_mapping (editor, mods, action);
        }

      plus_button = gtk_button_new_from_icon_name ("list-add", GTK_ICON_SIZE_LARGE_TOOLBAR);
      gtk_list_box_insert (GTK_LIST_BOX (editor->priv->current_settings), plus_button, -1);
      gtk_widget_show (plus_button);

      g_signal_connect (plus_button, "clicked",
                        G_CALLBACK (gimp_modifiers_editor_plus_button_clicked),
                        editor);
      g_object_set_data (G_OBJECT (editor->priv->current_settings), "plus-button", plus_button);

      if (g_list_length (modifiers) == 0)
        gimp_modifiers_editor_plus_button_clicked (GTK_BUTTON (plus_button), editor);

      gtk_widget_show (editor->priv->current_settings);
      g_list_free (modifiers);
    }

  gtk_stack_set_visible_child (GTK_STACK (editor->priv->stack), editor->priv->current_settings);

  g_free (text);
}

static gboolean
gimp_modifiers_editor_button_press_event (GtkWidget      *widget,
                                          GdkEventButton *event,
                                          gpointer        user_data)
{
  GimpModifiersEditor *editor = GIMP_MODIFIERS_EDITOR (user_data);
  GdkDevice           *device = gdk_event_get_source_device ((GdkEvent *) event);

  /* Update warning. */
  if (event->button == GDK_BUTTON_PRIMARY)
    {
      gtk_widget_show (editor->priv->warning);
      gimp_widget_blink (editor->priv->warning);
    }
  else
    {
      gtk_widget_hide (editor->priv->warning);
    }

  if (event->button != GDK_BUTTON_PRIMARY &&
      (event->button != editor->priv->button                       ||
       editor->priv->device == NULL                                ||
       g_strcmp0 (gdk_device_get_vendor_id (editor->priv->device),
                  gdk_device_get_vendor_id (device)) != 0          ||
       g_strcmp0 (gdk_device_get_product_id (editor->priv->device),
                  gdk_device_get_product_id (device)) != 0))
    {
      gimp_modifiers_editor_show_settings (editor, device, event->button);
    }

  return FALSE;
}

static void
gimp_modifiers_editor_add_mapping (GimpModifiersEditor *editor,
                                   GdkModifierType      modifiers,
                                   GimpModifierAction   mod_action)
{
  GtkWidget   *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  GtkWidget   *box_row;
  GtkWidget   *combo;
  GtkWidget   *shortcut;
  GtkWidget   *minus_button;
  GtkWidget   *plus_button;

  plus_button = g_object_get_data (G_OBJECT (editor->priv->current_settings), "plus-button");

  shortcut = gimp_shortcut_button_new (NULL);
  gimp_shortcut_button_accepts_modifier (GIMP_SHORTCUT_BUTTON (shortcut),
                                         TRUE, FALSE);
  gimp_shortcut_button_set_accelerator (GIMP_SHORTCUT_BUTTON (shortcut), NULL, 0, modifiers);
  gtk_box_pack_start (GTK_BOX (box), shortcut, FALSE, FALSE, 0);
  gtk_size_group_add_widget (editor->priv->mod_size_group, shortcut);
  gtk_widget_show (shortcut);

  combo = gimp_enum_combo_box_new (GIMP_TYPE_MODIFIER_ACTION);
  gtk_box_pack_start (GTK_BOX (box), combo, FALSE, FALSE, 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), mod_action);
  gtk_size_group_add_widget (editor->priv->action_size_group, combo);
  gtk_widget_show (combo);

  minus_button = gtk_button_new_from_icon_name ("list-remove", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_size_group_add_widget (editor->priv->minus_size_group, minus_button);
  gtk_box_pack_start (GTK_BOX (box), minus_button, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (minus_button), "shortcut-button", shortcut);
  g_signal_connect (minus_button, "clicked",
                    G_CALLBACK (gimp_modifiers_editor_minus_button_clicked),
                    editor);
  gtk_widget_show (minus_button);

  g_object_set_data (G_OBJECT (shortcut), "shortcut-button", shortcut);
  g_object_set_data (G_OBJECT (shortcut), "shortcut-action", combo);
  g_object_set_data (G_OBJECT (combo),    "shortcut-button", shortcut);
  g_object_set_data (G_OBJECT (combo),    "shortcut-action", combo);
  g_signal_connect (shortcut, "notify::accelerator",
                    G_CALLBACK (gimp_modifiers_editor_notify_accelerator),
                    editor);
  g_signal_connect (combo, "notify::active",
                    G_CALLBACK (gimp_modifiers_editor_notify_accelerator),
                    editor);

  gtk_list_box_insert (GTK_LIST_BOX (editor->priv->current_settings), box, -1);

  if (plus_button)
    {
      g_object_ref (plus_button);
      box_row = gtk_widget_get_parent (GTK_WIDGET (plus_button));
      gtk_container_remove (GTK_CONTAINER (box_row), GTK_WIDGET (plus_button));
      gtk_container_remove (GTK_CONTAINER (editor->priv->current_settings), box_row);
      gtk_list_box_insert (GTK_LIST_BOX (editor->priv->current_settings), GTK_WIDGET (plus_button), -1);
    }

  gtk_widget_show (box);
}

static void
gimp_modifiers_editor_plus_button_clicked (GtkButton           *plus_button,
                                           GimpModifiersEditor *editor)
{
  gimp_modifiers_editor_add_mapping (editor, 0, GIMP_MODIFIER_ACTION_NONE);
}

static void
gimp_modifiers_editor_minus_button_clicked (GtkButton           *minus_button,
                                            GimpModifiersEditor *editor)
{
  GtkWidget       *shortcut = g_object_get_data (G_OBJECT (minus_button), "shortcut-button");
  GdkModifierType  modifiers;

  gimp_shortcut_button_get_keys (GIMP_SHORTCUT_BUTTON (shortcut), NULL, &modifiers);
  gimp_modifiers_manager_remove (editor->priv->manager, editor->priv->device, editor->priv->button, modifiers);

  /* Always leave at least 1 row. Simply reset it instead. Since
   * GtkListBox doesn't have an API for length, I get the row at
   * position 2. If there is none, then there is just 1 row (+ 1 row for
   * the plus button).
   */
  if (gtk_list_box_get_row_at_index (GTK_LIST_BOX (editor->priv->current_settings), 2) == NULL)
    {
      GtkWidget *combo = g_object_get_data (G_OBJECT (minus_button), "shortcut-action");

      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), GIMP_MODIFIER_ACTION_NONE);
      gimp_shortcut_button_set_accelerator (GIMP_SHORTCUT_BUTTON (shortcut), NULL, 0, 0);
    }
  else
    {
      GtkWidget *box_row;

      box_row = gtk_widget_get_parent (GTK_WIDGET (minus_button));
      box_row = gtk_widget_get_parent (box_row);
      gtk_container_remove (GTK_CONTAINER (editor->priv->current_settings), box_row);
    }
}

static void
gimp_modifiers_editor_notify_accelerator (GtkWidget           *widget,
                                          const GParamSpec    *pspec,
                                          GimpModifiersEditor *editor)
{
  GtkWidget          *shortcut;
  GtkWidget          *combo;
  GimpModifierAction  action = GIMP_MODIFIER_ACTION_NONE;

  GdkModifierType  modifiers;

  shortcut = g_object_get_data (G_OBJECT (widget), "shortcut-button");
  combo    = g_object_get_data (G_OBJECT (widget), "shortcut-action");

  gimp_shortcut_button_get_keys (GIMP_SHORTCUT_BUTTON (shortcut), NULL, &modifiers);

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), (gint *) &action))
    gimp_modifiers_manager_set (editor->priv->manager, editor->priv->device,
                                editor->priv->button, modifiers,
                                action);
}
