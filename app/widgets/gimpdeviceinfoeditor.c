/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpdeviceinfoeditor.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpdeviceinfo.h"
#include "gimpdeviceinfoeditor.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_INFO
};


typedef struct _GimpInputKeyInfo GimpInputKeyInfo;

struct _GimpInputKeyInfo
{
  gint                  index;
  GtkWidget            *entry;
  GimpDeviceInfoEditor *editor;
};


typedef struct _GimpDeviceInfoEditorPrivate GimpDeviceInfoEditorPrivate;

struct _GimpDeviceInfoEditorPrivate
{
  GimpDeviceInfo *info;

  GtkWidget      *axis_combos[GDK_AXIS_LAST - GDK_AXIS_X];
};

#define GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE(editor) \
        G_TYPE_INSTANCE_GET_PRIVATE (editor, \
                                     GIMP_TYPE_DEVICE_INFO_EDITOR, \
                                     GimpDeviceInfoEditorPrivate)


static GObject * gimp_device_info_editor_constructor  (GType                  type,
                                                       guint                  n_params,
                                                       GObjectConstructParam *params);
static void      gimp_device_info_editor_finalize     (GObject               *object);
static void      gimp_device_info_editor_set_property (GObject               *object,
                                                       guint                  property_id,
                                                       const GValue          *value,
                                                       GParamSpec            *pspec);
static void      gimp_device_info_editor_get_property (GObject               *object,
                                                       guint                  property_id,
                                                       GValue                *value,
                                                       GParamSpec            *pspec);

void             gimp_device_info_editor_axis_changed (GimpIntComboBox       *combo,
                                                       GimpDeviceInfoEditor  *editor);

static gboolean  gimp_device_info_editor_key_press    (GtkWidget        *widget,
                                                       GdkEventKey      *event,
                                                       GimpInputKeyInfo *key);
static void      gimp_device_info_editor_clear_key    (GtkWidget        *widget,
                                                       GimpInputKeyInfo *key);
static void      gimp_device_info_editor_set_key      (GimpInputKeyInfo *key,
                                                       guint             keyval,
                                                       GdkModifierType   modifiers);


G_DEFINE_TYPE (GimpDeviceInfoEditor, gimp_device_info_editor, GTK_TYPE_VBOX)

#define parent_class gimp_device_info_editor_parent_class


static void
gimp_device_info_editor_class_init (GimpDeviceInfoEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_device_info_editor_constructor;
  object_class->finalize     = gimp_device_info_editor_finalize;
  object_class->set_property = gimp_device_info_editor_set_property;
  object_class->get_property = gimp_device_info_editor_get_property;

  g_object_class_install_property (object_class, PROP_INFO,
                                   g_param_spec_object ("info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DEVICE_INFO,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (object_class, sizeof (GimpDeviceInfoEditorPrivate));
}

static void
gimp_device_info_editor_init (GimpDeviceInfoEditor *editor)
{
  gtk_box_set_spacing (GTK_BOX (editor), 6);
}

static GObject *
gimp_device_info_editor_constructor (GType                   type,
                                     guint                   n_params,
                                     GObjectConstructParam  *params)
{
  GObject                     *object;
  GimpDeviceInfoEditor        *editor;
  GimpDeviceInfoEditorPrivate *private;
  GtkWidget                   *hbox;
  GtkWidget                   *label;
  GtkWidget                   *combo;
  GtkWidget                   *table;
  gint                         n_keys;
  gint                         i;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor  = GIMP_DEVICE_INFO_EDITOR (object);
  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  g_assert (GIMP_IS_DEVICE_INFO (private->info));

  /*  the mode menu  */

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Mode:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (private->info), "mode",
                                        0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  /*  the axes  */

  table = gtk_table_new (GDK_AXIS_LAST, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (editor), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  for (i = GDK_AXIS_X; i < GDK_AXIS_LAST; i++)
    {
      static const gchar *const axis_use_strings[] =
      {
        N_("_X:"),
        N_("_Y:"),
        N_("_Pressure:"),
        N_("X _tilt:"),
        N_("Y t_ilt:"),
        N_("_Wheel:")
      };

      gint n_axes = gimp_device_info_get_n_axes (private->info);
      gint j;

      combo = gimp_int_combo_box_new (_("none"), -1,
                                      NULL);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), -1);
      private->axis_combos[i - 1] = combo;
      gimp_table_attach_aligned (GTK_TABLE (table), 0, i - 1,
                                 axis_use_strings[i - 1], 0.0, 0.5,
                                 combo, 1, FALSE);

      for (j = 0; j < n_axes; j++)
        {
          gchar string[16];

          g_snprintf (string, sizeof (string), "%d", j + 1);

          gimp_int_combo_box_append (GIMP_INT_COMBO_BOX (combo),
                                     GIMP_INT_STORE_VALUE, j,
                                     GIMP_INT_STORE_LABEL, string,
                                     -1);

          if (gimp_device_info_get_axis_use (private->info, j) == i)
            gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), j);
        }

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_device_info_editor_axis_changed),
                        editor);
    }

  /*  the keys  */

  n_keys = gimp_device_info_get_n_keys (private->info);

  table = gtk_table_new (n_keys, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (editor), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  for (i = 0; i < n_keys; i++)
    {
      GimpInputKeyInfo *key = g_new (GimpInputKeyInfo, 1);
      GtkWidget        *button;
      gchar             string[32];
      guint             keyval;
      GdkModifierType   modifiers;

      key->index  = i;
      key->editor = editor;

      g_snprintf (string, sizeof (string), "_%d:", i + 1);

      hbox = gtk_hbox_new (FALSE, 6);

      key->entry = gtk_entry_new ();
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), key->entry);
      gtk_box_pack_start (GTK_BOX (hbox), key->entry, TRUE, TRUE, 0);

      gimp_table_attach_aligned (GTK_TABLE (table), 0, i - 1,
                                 string, 0.0, 0.5,
                                 hbox, 1, FALSE);

      g_signal_connect (key->entry, "key-press-event",
                        G_CALLBACK (gimp_device_info_editor_key_press),
                        key);
      g_object_set_data_full (G_OBJECT (key->entry), "key", key,
                              (GDestroyNotify) g_free);

      button = gtk_button_new_with_mnemonic (_("Cl_ear"));
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (gimp_device_info_editor_clear_key),
                        key);

      gimp_device_info_get_key (private->info, i, &keyval, &modifiers);

      gimp_device_info_editor_set_key (key, keyval, modifiers);
    }

  return object;
}

static void
gimp_device_info_editor_finalize (GObject *object)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  if (private->info)
    {
      g_object_unref (private->info);
      private->info = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_device_info_editor_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_INFO:
      private->info = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_device_info_editor_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_INFO:
      g_value_set_object (value, private->info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_device_info_editor_axis_changed (GimpIntComboBox      *combo,
                                      GimpDeviceInfoEditor *editor)
{
  GimpDeviceInfoEditorPrivate *private;
  GdkAxisUse                   new_use;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (editor);

  for (new_use = GDK_AXIS_X; new_use < GDK_AXIS_LAST; new_use++)
    {
      if (private->axis_combos[new_use - 1] == (GtkWidget *) combo)
        {
          GdkAxisUse old_use  = GDK_AXIS_IGNORE;
          gint       new_axis = -1;
          gint       old_axis = -1;
          gint       n_axes;
          gint       i;

          gimp_int_combo_box_get_active (combo, &new_axis);

          n_axes = gimp_device_info_get_n_axes (private->info);

          for (i = 0; i < n_axes; i++)
            if (gimp_device_info_get_axis_use (private->info, i) == new_use)
              {
                old_axis = i;
                break;
              }

          if (new_axis == old_axis)
            return;

          if (new_axis != -1)
            old_use = gimp_device_info_get_axis_use (private->info, new_axis);

          /* we must always have an x and a y axis */
          if ((new_axis == -1 && (new_use == GDK_AXIS_X ||
                                  new_use == GDK_AXIS_Y)) ||
              (old_axis == -1 && (old_use == GDK_AXIS_X ||
                                  old_use == GDK_AXIS_Y)))
            {
              gimp_int_combo_box_set_active (combo, old_axis);
            }
          else
            {
              if (new_axis != -1)
                gimp_device_info_set_axis_use (private->info, new_axis, new_use);

              if (old_axis != -1)
                gimp_device_info_set_axis_use (private->info, old_axis, old_use);

              if (old_use != GDK_AXIS_IGNORE)
                gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (private->axis_combos[old_use - 1]),
                                               old_axis);
            }

          break;
        }
    }
}

static gboolean
gimp_device_info_editor_key_press (GtkWidget        *widget,
                                   GdkEventKey      *event,
                                   GimpInputKeyInfo *key)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (key->editor);

  gimp_device_info_editor_set_key (key, event->keyval, event->state & 0xFF);

  gimp_device_info_set_key (private->info, key->index,
                            event->keyval, event->state & 0xFF);

  return TRUE;
}

static void
gimp_device_info_editor_clear_key (GtkWidget        *widget,
                                   GimpInputKeyInfo *key)
{
  GimpDeviceInfoEditorPrivate *private;

  private = GIMP_DEVICE_INFO_EDITOR_GET_PRIVATE (key->editor);

  gimp_device_info_editor_set_key (key, 0, 0);

  gimp_device_info_set_key (private->info, key->index, 0, 0);
}

static void
gimp_device_info_editor_set_key (GimpInputKeyInfo *key,
                                 guint             keyval,
                                 GdkModifierType   modifiers)
{
  GString *str;
  gchar    chars[2];

  if (keyval)
    {
      str = g_string_new (NULL);

      if (modifiers & GDK_SHIFT_MASK)
        g_string_append (str, "Shift+");
      if (modifiers & GDK_CONTROL_MASK)
        g_string_append (str, "Ctrl+");
      if (modifiers & GDK_MOD1_MASK)
        g_string_append (str, "Alt+");

      if ((keyval >= 0x20) && (keyval <= 0xFF))
        {
          chars[0] = keyval;
          chars[1] = 0;
          g_string_append (str, chars);
        }
      else
        g_string_append (str, _("(unknown)"));

      gtk_entry_set_text (GTK_ENTRY (key->entry),
                          g_string_free (str, FALSE));
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (key->entry), _("(disabled)"));
    }
}


/*  public functions  */

GtkWidget *
gimp_device_info_editor_new (GimpDeviceInfo *info)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_INFO (info), NULL);

  return g_object_new (GIMP_TYPE_DEVICE_INFO_EDITOR,
                       "info", info,
                       NULL);
}
