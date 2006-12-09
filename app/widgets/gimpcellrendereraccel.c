/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrendereraccel.c
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
 *
 * Derived from: libegg/libegg/treeviewutils/eggcellrendererkeys.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms.h>

#include "widgets-types.h"

#include "core/gimpmarshal.h"

#include "gimpcellrendereraccel.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  ACCEL_EDITED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ACCEL_KEY,
  PROP_ACCEL_MASK
};

#define GIMP_CELL_RENDERER_ACCEL_PATH "gimp-cell-renderer-accel-path"


static void   gimp_cell_renderer_accel_set_property (GObject         *object,
                                                     guint            param_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void   gimp_cell_renderer_accel_get_property (GObject         *object,
                                                     guint            param_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static void   gimp_cell_renderer_accel_get_size     (GtkCellRenderer *cell,
                                                     GtkWidget       *widget,
                                                     GdkRectangle    *cell_area,
                                                     gint            *x_offset,
                                                     gint            *y_offset,
                                                     gint            *width,
                                                     gint            *height);
static GtkCellEditable *
             gimp_cell_renderer_accel_start_editing (GtkCellRenderer *cell,
                                                     GdkEvent        *event,
                                                     GtkWidget       *widget,
                                                     const gchar     *path,
                                                     GdkRectangle    *background_area,
                                                     GdkRectangle    *cell_area,
                                                     GtkCellRendererState  flags);


G_DEFINE_TYPE (GimpCellRendererAccel, gimp_cell_renderer_accel,
               GTK_TYPE_CELL_RENDERER_TEXT)

#define parent_class gimp_cell_renderer_accel_parent_class

static guint accel_cell_signals[LAST_SIGNAL] = { 0 };


static void
gimp_cell_renderer_accel_class_init (GimpCellRendererAccelClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  accel_cell_signals[ACCEL_EDITED] =
    g_signal_new ("accel-edited",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpCellRendererAccelClass, accel_edited),
                  NULL, NULL,
                  gimp_marshal_VOID__STRING_BOOLEAN_UINT_FLAGS,
                  G_TYPE_NONE, 4,
                  G_TYPE_STRING,
                  G_TYPE_BOOLEAN,
                  G_TYPE_UINT,
                  GDK_TYPE_MODIFIER_TYPE);

  object_class->set_property = gimp_cell_renderer_accel_set_property;
  object_class->get_property = gimp_cell_renderer_accel_get_property;

  cell_class->get_size       = gimp_cell_renderer_accel_get_size;
  cell_class->start_editing  = gimp_cell_renderer_accel_start_editing;

  g_object_class_install_property (object_class, PROP_ACCEL_KEY,
                                   g_param_spec_uint ("accel-key", NULL, NULL,
                                                      0, G_MAXINT, 0,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ACCEL_MASK,
                                   g_param_spec_flags ("accel-mask", NULL, NULL,
                                                       GDK_TYPE_MODIFIER_TYPE,
                                                       0,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));
}

static void
gimp_cell_renderer_accel_init (GimpCellRendererAccel *cell)
{
}

static void
gimp_cell_renderer_accel_set_property (GObject      *object,
                                       guint         param_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpCellRendererAccel *accel = GIMP_CELL_RENDERER_ACCEL (object);

  switch (param_id)
    {
    case PROP_ACCEL_KEY:
      gimp_cell_renderer_accel_set_accelerator (accel,
                                                g_value_get_uint (value),
                                                accel->accel_mask);
      break;
    case PROP_ACCEL_MASK:
      gimp_cell_renderer_accel_set_accelerator (accel,
                                                accel->accel_key,
                                                g_value_get_flags (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
gimp_cell_renderer_accel_get_property (GObject    *object,
                                       guint       param_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpCellRendererAccel *accel = GIMP_CELL_RENDERER_ACCEL (object);

  switch (param_id)
    {
    case PROP_ACCEL_KEY:
      g_value_set_uint (value, accel->accel_key);
      break;
    case PROP_ACCEL_MASK:
      g_value_set_flags (value, accel->accel_mask);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
gimp_cell_renderer_accel_get_size (GtkCellRenderer *cell,
                                   GtkWidget       *widget,
                                   GdkRectangle    *cell_area,
                                   gint            *x_offset,
                                   gint            *y_offset,
                                   gint            *width,
                                   gint            *height)
{
  GimpCellRendererAccel *accel = GIMP_CELL_RENDERER_ACCEL (cell);
  GtkRequisition         requisition;

  if (! accel->sizing_label)
    accel->sizing_label = gtk_label_new (_("Type a new accelerator, "
                                           "or press Backspace to clear"));

  gtk_widget_size_request (accel->sizing_label, &requisition);

  GTK_CELL_RENDERER_CLASS (parent_class)->get_size (cell, widget, cell_area,
                                                    x_offset, y_offset,
                                                    width, height);

  /* FIXME: need to take the cell_area et al. into account */
  if (width)
    *width = MAX (*width, requisition.width);
  if (height)
    *height = MAX (*height, requisition.height);
}

/* FIXME: Currently we don't differentiate between a 'bogus' key (like tab in
 * GTK mode) and a removed key.
 */

static gboolean
grab_key_callback (GtkWidget             *widget,
                   GdkEventKey           *event,
                   GimpCellRendererAccel *accel)
{
  GdkDisplay      *display;
  guint            accel_key;
  GdkModifierType  accel_mods;
  gchar           *path;
  gboolean         delete = FALSE;
  gboolean         edited = FALSE;
  GdkModifierType  consumed_modifiers;

  switch (event->keyval)
    {
    case GDK_Shift_L:
    case GDK_Shift_R:
    case GDK_Control_L:
    case GDK_Control_R:
    case GDK_Alt_L:
    case GDK_Alt_R:
      return TRUE;

    case GDK_Delete:
    case GDK_KP_Delete:
    case GDK_BackSpace:
      delete = TRUE;
      break;
    default:
      break;
    }

  display = gtk_widget_get_display (widget);

  gdk_keymap_translate_keyboard_state (gdk_keymap_get_for_display (display),
                                       event->hardware_keycode,
                                       event->state,
                                       event->group,
                                       NULL, NULL, NULL, &consumed_modifiers);

  accel_key  = gdk_keyval_to_lower (event->keyval);
  accel_mods = (event->state                            &
                gtk_accelerator_get_default_mod_mask () &
                ~consumed_modifiers);

  if (accel_key == GDK_ISO_Left_Tab)
    accel_key = GDK_Tab;

  /* If lowercasing affects the keysym, then we need to include SHIFT
   * in the modifiers, We re-upper case when we match against the
   * keyval, but display and save in caseless form.
   */
  if (accel_key != event->keyval)
    accel_mods |= GDK_SHIFT_MASK;

  if (accel_key != GDK_Escape)
    {
      if (delete || ! gtk_accelerator_valid (accel_key, accel_mods))
        {
          accel_key  = 0;
          accel_mods = 0;
        }

      edited = TRUE;
    }

  path = g_strdup (g_object_get_data (G_OBJECT (accel->edit_widget),
                                      GIMP_CELL_RENDERER_ACCEL_PATH));

  gdk_display_keyboard_ungrab (display, event->time);
  gdk_display_pointer_ungrab (display, event->time);

  gtk_cell_editable_editing_done (GTK_CELL_EDITABLE (accel->edit_widget));
  gtk_cell_editable_remove_widget (GTK_CELL_EDITABLE (accel->edit_widget));
  accel->edit_widget = NULL;
  accel->grab_widget = NULL;

  if (edited)
    g_signal_emit (accel, accel_cell_signals[ACCEL_EDITED], 0,
                   path, delete, accel_key, accel_mods);

  g_free (path);

  return TRUE;
}

static void
ungrab_stuff (GtkWidget             *widget,
              GimpCellRendererAccel *accel)
{
  GdkDisplay *display = gtk_widget_get_display (widget);

  gdk_display_keyboard_ungrab (display, GDK_CURRENT_TIME);
  gdk_display_pointer_ungrab (display, GDK_CURRENT_TIME);

  g_signal_handlers_disconnect_by_func (accel->grab_widget,
                                        G_CALLBACK (grab_key_callback),
                                        accel);
}

static void
pointless_eventbox_start_editing (GtkCellEditable *cell_editable,
                                  GdkEvent        *event)
{
  /* do nothing, because we are pointless */
}

static void
pointless_eventbox_cell_editable_init (GtkCellEditableIface *iface)
{
  iface->start_editing = pointless_eventbox_start_editing;
}

static GType
pointless_eventbox_subclass_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GtkEventBoxClass),
        NULL,           /* base_init      */
        NULL,           /* base_finalize  */
        NULL,           /* class_init     */
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GtkEventBox),
        0,              /* n_preallocs    */
        NULL            /* instance init  */
      };

      const GInterfaceInfo editable_info =
      {
        (GInterfaceInitFunc) pointless_eventbox_cell_editable_init,
        NULL, NULL
      };

      type = g_type_register_static (GTK_TYPE_EVENT_BOX,
                                     "GimpCellEditableEventBox",
                                     &info, 0);

      g_type_add_interface_static (type, GTK_TYPE_CELL_EDITABLE,
                                   &editable_info);
    }

  return type;
}

static GtkCellEditable *
gimp_cell_renderer_accel_start_editing (GtkCellRenderer      *cell,
                                        GdkEvent             *event,
                                        GtkWidget            *widget,
                                        const gchar          *path,
                                        GdkRectangle         *background_area,
                                        GdkRectangle         *cell_area,
                                        GtkCellRendererState  flags)
{
  GtkCellRendererText   *celltext = GTK_CELL_RENDERER_TEXT (cell);
  GimpCellRendererAccel *accel    = GIMP_CELL_RENDERER_ACCEL (cell);
  GtkWidget             *label;
  GtkWidget             *eventbox;

  if (! celltext->editable)
    return NULL;

  g_return_val_if_fail (widget->window != NULL, NULL);

  if (gdk_keyboard_grab (widget->window, FALSE,
                         gdk_event_get_time (event)) != GDK_GRAB_SUCCESS)
    return NULL;

  if (gdk_pointer_grab (widget->window, FALSE,
                        GDK_BUTTON_PRESS_MASK,
                        NULL, NULL,
                        gdk_event_get_time (event)) != GDK_GRAB_SUCCESS)
    {
      gdk_display_keyboard_ungrab (gtk_widget_get_display (widget),
                                   gdk_event_get_time (event));
      return NULL;
    }

  accel->grab_widget = widget;

  g_signal_connect (widget, "key-press-event",
                    G_CALLBACK (grab_key_callback),
                    accel);

  eventbox = g_object_new (pointless_eventbox_subclass_get_type (), NULL);
  accel->edit_widget = eventbox;
  g_object_add_weak_pointer (G_OBJECT (accel->edit_widget),
                             (gpointer) &accel->edit_widget);

  label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_widget_modify_bg (eventbox, GTK_STATE_NORMAL,
                        &widget->style->bg[GTK_STATE_SELECTED]);

  gtk_widget_modify_fg (label, GTK_STATE_NORMAL,
                        &widget->style->fg[GTK_STATE_SELECTED]);

  if (accel->accel_key != 0)
    gtk_label_set_text (GTK_LABEL (label),
                        _("Type a new accelerator, or press Backspace to clear"));
  else
    gtk_label_set_text (GTK_LABEL (label),
                        _("Type a new accelerator"));

  gtk_container_add (GTK_CONTAINER (eventbox), label);

  g_object_set_data_full (G_OBJECT (accel->edit_widget),
                          GIMP_CELL_RENDERER_ACCEL_PATH,
                          g_strdup (path), g_free);

  gtk_widget_show_all (accel->edit_widget);

  g_signal_connect (accel->edit_widget, "unrealize",
                    G_CALLBACK (ungrab_stuff),
                    accel);

  accel->edit_key = accel->accel_key;

  return GTK_CELL_EDITABLE (accel->edit_widget);
}


/*  public functions  */

GtkCellRenderer *
gimp_cell_renderer_accel_new (void)
{
  return g_object_new (GIMP_TYPE_CELL_RENDERER_ACCEL, NULL);
}

void
gimp_cell_renderer_accel_set_accelerator (GimpCellRendererAccel *accel,
                                          guint                  accel_key,
                                          GdkModifierType        accel_mask)
{
  gboolean changed = FALSE;

  g_return_if_fail (GIMP_IS_CELL_RENDERER_ACCEL (accel));

  g_object_freeze_notify (G_OBJECT (accel));

  if (accel_key != accel->accel_key)
    {
      accel->accel_key = accel_key;

      g_object_notify (G_OBJECT (accel), "accel-key");
      changed = TRUE;
    }

  if (accel_mask != accel->accel_mask)
    {
      accel->accel_mask = accel_mask;

      g_object_notify (G_OBJECT (accel), "accel-mask");
      changed = TRUE;
    }

  g_object_thaw_notify (G_OBJECT (accel));

  if (changed)
    {
      gchar *text = gimp_get_accel_string (accel->accel_key,
                                           accel->accel_mask);
      g_object_set (accel, "text", text, NULL);
      g_free (text);
    }
}

void
gimp_cell_renderer_accel_get_accelerator (GimpCellRendererAccel *accel,
                                          guint                 *accel_key,
                                          GdkModifierType       *accel_mask)
{
  g_return_if_fail (GIMP_IS_CELL_RENDERER_ACCEL (accel));

  if (accel_key)
    *accel_key = accel->accel_key;

  if (accel_mask)
    *accel_mask = accel->accel_mask;
}
