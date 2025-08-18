/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprow.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpdnd.h"
#include "gimpeditor.h"
#include "gimprow.h"
#include "gimprow-utils.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"

#include "gimp-intl.h"


enum
{
  EDIT_NAME,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_VIEWABLE,
  PROP_VIEW_SIZE,
  PROP_VIEW_BORDER_WIDTH,
  N_PROPS
};

static GParamSpec *obj_props[N_PROPS] = { NULL, };


typedef struct _GimpRowPrivate GimpRowPrivate;

struct _GimpRowPrivate
{
  GimpContext  *context;
  GimpViewable *viewable;

  gint          view_size;
  gint          view_border_width;

  GtkWidget    *box;

  GtkWidget    *icon;
  GtkWidget    *view;
  GtkWidget    *label;

  GtkWidget    *rename_popover;
  GtkGesture   *rename_long_press;
};

#define GET_PRIVATE(obj) \
  ((GimpRowPrivate *) gimp_row_get_instance_private ((GimpRow *) obj))


static void       gimp_row_constructed           (GObject          *object);
static void       gimp_row_dispose               (GObject          *object);
static void       gimp_row_set_property          (GObject          *object,
                                                  guint             property_id,
                                                  const GValue     *value,
                                                  GParamSpec       *pspec);
static void       gimp_row_get_property          (GObject          *object,
                                                  guint             property_id,
                                                  GValue           *value,
                                                  GParamSpec       *pspec);

static gboolean   gimp_row_button_press_event    (GtkWidget        *widget,
                                                  GdkEventButton   *bevent);
static void       gimp_row_style_updated         (GtkWidget        *widget);
static gboolean   gimp_row_query_tooltip         (GtkWidget        *widget,
                                                  gint              x,
                                                  gint              y,
                                                  gboolean          keyboard_tooltip,
                                                  GtkTooltip       *tooltip);

static void       gimp_row_real_set_context      (GimpRow          *row,
                                                  GimpContext      *context);
static void       gimp_row_real_set_viewable     (GimpRow          *row,
                                                  GimpViewable     *viewable);
static void       gimp_row_real_set_view_size    (GimpRow          *row);
static void       gimp_row_real_icon_changed     (GimpRow          *row);
static void       gimp_row_real_name_changed     (GimpRow          *row);
static void       gimp_row_real_monitor_changed  (GimpRow          *row);
static void       gimp_row_real_edit_name        (GimpRow          *row);
static gboolean   gimp_row_real_name_edited      (GimpRow          *row,
                                                  const gchar      *new_name);

static void       gimp_row_icon_changed          (GimpViewable     *viewable,
                                                  const GParamSpec *pspec,
                                                  GimpRow          *row);
static void       gimp_row_name_changed          (GimpViewable     *viewable,
                                                  GimpRow          *row);

static void       gimp_row_label_long_pressed    (GtkGesture       *gesture,
                                                  gdouble           x,
                                                  gdouble           y,
                                                  GimpRow          *row);
static void       gimp_row_rename_entry_activate (GtkEntry         *entry,
                                                  GimpRow          *row);

static GimpViewable *
                  gimp_row_drag_viewable         (GtkWidget        *widget,
                                                  GimpContext     **context,
                                                  gpointer          data);
static GdkPixbuf *gimp_row_drag_pixbuf           (GtkWidget        *widget,
                                                  gpointer          data);


G_DEFINE_TYPE_WITH_PRIVATE (GimpRow, gimp_row, GTK_TYPE_LIST_BOX_ROW)

#define parent_class gimp_row_parent_class

static guint row_signals[LAST_SIGNAL] = { 0 };


static void
gimp_row_class_init (GimpRowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet  *binding_set;

  object_class->constructed        = gimp_row_constructed;
  object_class->dispose            = gimp_row_dispose;
  object_class->set_property       = gimp_row_set_property;
  object_class->get_property       = gimp_row_get_property;

  widget_class->button_press_event = gimp_row_button_press_event;
  widget_class->style_updated      = gimp_row_style_updated;
  widget_class->query_tooltip      = gimp_row_query_tooltip;

  klass->set_context               = gimp_row_real_set_context;
  klass->set_viewable              = gimp_row_real_set_viewable;
  klass->set_view_size             = gimp_row_real_set_view_size;
  klass->icon_changed              = gimp_row_real_icon_changed;
  klass->name_changed              = gimp_row_real_name_changed;
  klass->monitor_changed           = gimp_row_real_monitor_changed;
  klass->edit_name                 = gimp_row_real_edit_name;
  klass->name_edited               = gimp_row_real_name_edited;

  row_signals[EDIT_NAME] =
    g_signal_new ("edit-name",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpRowClass, edit_name),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  binding_set = gtk_binding_set_by_class (klass);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_F2, 0,
                                "edit-name", 0);

  obj_props[PROP_CONTEXT] =
    g_param_spec_object ("context",
                         NULL, NULL,
                         GIMP_TYPE_CONTEXT,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT);

  obj_props[PROP_VIEWABLE] =
    g_param_spec_object ("viewable",
                         NULL, NULL,
                         GIMP_TYPE_VIEWABLE,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT);

  obj_props[PROP_VIEW_SIZE] =
    g_param_spec_int ("view-size",
                      NULL, NULL,
                      1, GIMP_VIEWABLE_MAX_PREVIEW_SIZE,
                      GIMP_VIEW_SIZE_MEDIUM,
                      GIMP_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT);

  obj_props[PROP_VIEW_BORDER_WIDTH] =
    g_param_spec_int ("view-border-width",
                      NULL, NULL,
                      0, GIMP_VIEW_MAX_BORDER_WIDTH,
                      1,
                      GIMP_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, N_PROPS, obj_props);
}

static void
gimp_row_init (GimpRow *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);
  GtkWidget      *ebox;

  gtk_widget_set_has_tooltip (GTK_WIDGET (row), TRUE);

  priv->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (row), priv->box);
  gtk_widget_set_visible (priv->box, TRUE);

  priv->icon = gtk_image_new_from_icon_name (NULL, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (priv->box), priv->icon, FALSE, FALSE, 0);
  gtk_widget_set_visible (priv->icon, TRUE);

  ebox = gtk_event_box_new ();
  gtk_box_pack_end (GTK_BOX (priv->box), ebox, TRUE, TRUE, 0);
  gtk_widget_show (ebox);

  priv->label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (priv->label), 0.0);
  gtk_container_add (GTK_CONTAINER (ebox), priv->label);
  gtk_widget_set_visible (priv->label, TRUE);

  priv->rename_long_press = gtk_gesture_long_press_new (ebox);
  gtk_event_controller_set_propagation_phase
    (GTK_EVENT_CONTROLLER (priv->rename_long_press),
     GTK_PHASE_TARGET);

  g_signal_connect (priv->rename_long_press, "pressed",
                    G_CALLBACK (gimp_row_label_long_pressed),
                    row);
}

static void
gimp_row_constructed (GObject *object)
{
  GimpRowPrivate *priv    = GET_PRIVATE (object);
  gboolean        preview = FALSE;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  priv->view = gimp_view_new (priv->context,
                              priv->viewable,
                              priv->view_size,
                              priv->view_border_width,
                              FALSE);
  GIMP_VIEW (priv->view)->eat_button_events = FALSE;
  GIMP_VIEW (priv->view)->show_popup        = TRUE;
  gtk_box_pack_start (GTK_BOX (priv->box), priv->view, FALSE, FALSE, 0);

  if (priv->viewable)
    preview = gimp_viewable_has_preview (priv->viewable);

  gtk_widget_set_visible (priv->icon, ! preview);
  gtk_widget_set_visible (priv->view, preview);
}

static void
gimp_row_dispose (GObject *object)
{
  GimpRowPrivate *priv = GET_PRIVATE (object);

  gimp_row_set_viewable (GIMP_ROW (object), NULL);

  g_clear_pointer (&priv->rename_popover, gtk_widget_destroy);
  g_clear_object  (&priv->rename_long_press);

  gimp_row_set_context  (GIMP_ROW (object), NULL);
  gimp_row_set_viewable (GIMP_ROW (object), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_row_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  GimpRow *row = GIMP_ROW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      gimp_row_set_context (row, g_value_get_object (value));
      break;
    case PROP_VIEWABLE:
      gimp_row_set_viewable (row, g_value_get_object (value));
      break;
    case PROP_VIEW_SIZE:
    case PROP_VIEW_BORDER_WIDTH:
      {
        gint size;
        gint border = 0;

        size = gimp_row_get_view_size (row, &border);

        if (property_id == PROP_VIEW_SIZE)
          size = g_value_get_int (value);
        else
          border = g_value_get_int (value);

        gimp_row_set_view_size (row, size, border);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_row_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  GimpRow *row = GIMP_ROW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, gimp_row_get_context (row));
      break;
    case PROP_VIEWABLE:
      g_value_set_object (value, gimp_row_get_viewable (row));
      break;
    case PROP_VIEW_SIZE:
    case PROP_VIEW_BORDER_WIDTH:
      {
        gint size;
        gint border = 0;

        size = gimp_row_get_view_size (row, &border);

        if (property_id == PROP_VIEW_SIZE)
          g_value_set_int (value, size);
        else
          g_value_set_int (value, border);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_row_button_press_event (GtkWidget      *widget,
                             GdkEventButton *bevent)
{
  GdkEvent *event = (GdkEvent *) bevent;
  gboolean  context_menu;

  GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

  context_menu = gdk_event_triggers_context_menu (event);

  if (bevent->button == 1 || context_menu)
    {
      GtkWidget *list_box = gtk_widget_get_parent (widget);

      if (GTK_IS_LIST_BOX (list_box))
        {
          gtk_list_box_select_row (GTK_LIST_BOX (list_box),
                                   GTK_LIST_BOX_ROW (widget));

          if (context_menu)
            {
              GtkWidget *editor = gtk_widget_get_ancestor (widget,
                                                           GIMP_TYPE_EDITOR);

              if (editor)
                {
                  return gimp_editor_popup_menu_at_pointer (GIMP_EDITOR (editor),
                                                            event);
               }
            }
        }
    }

  return FALSE;
}

static void
gimp_row_style_updated (GtkWidget *widget)
{
  GimpRowPrivate *priv = GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (priv->view)
    gimp_view_renderer_invalidate (GIMP_VIEW (priv->view)->renderer);
}

static gboolean
gimp_row_query_tooltip (GtkWidget  *widget,
                        gint        x,
                        gint        y,
                        gboolean    keyboard_tooltip,
                        GtkTooltip *tooltip)
{
  GimpRowPrivate *priv     = GET_PRIVATE (widget);
  gboolean        show_tip = FALSE;

  if (priv->viewable)
    {
      gchar *desc;
      gchar *tip;

      desc = gimp_viewable_get_description (priv->viewable, &tip);

      if (tip)
        {
          gtk_tooltip_set_text (tooltip, tip);

          show_tip = TRUE;

          g_free (tip);
        }

      g_free (desc);
    }

  return show_tip;
}

static void
gimp_row_real_set_context (GimpRow     *row,
                           GimpContext *context)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  g_set_object (&priv->context, context);

  if (priv->view)
    gimp_view_renderer_set_context (GIMP_VIEW (priv->view)->renderer,
                                    priv->context);
}

static void
gimp_row_real_set_viewable (GimpRow      *row,
                            GimpViewable *viewable)
{
  GimpRowPrivate *priv          = GET_PRIVATE (row);
  GType           viewable_type = G_TYPE_NONE;
  gboolean        preview       = FALSE;

  if (viewable)
    viewable_type = G_TYPE_FROM_INSTANCE (viewable);

  if (priv->viewable)
    {
      g_signal_handlers_disconnect_by_func (priv->viewable,
                                            gimp_row_icon_changed,
                                            row);
      g_signal_handlers_disconnect_by_func (priv->viewable,
                                            gimp_row_name_changed,
                                            row);

      if (! viewable)
        {
          if (gimp_dnd_viewable_source_remove (GTK_WIDGET (row),
                                               G_TYPE_FROM_INSTANCE (priv->viewable)))
            {
              if (gimp_viewable_get_size (priv->viewable, NULL, NULL))
                gimp_dnd_pixbuf_source_remove (GTK_WIDGET (row));

              gtk_drag_source_unset (GTK_WIDGET (row));
            }
        }
    }
  else if (viewable)
    {
      if (gimp_dnd_drag_source_set_by_type (GTK_WIDGET (row),
                                            GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                            viewable_type,
                                            GDK_ACTION_COPY))
        {
          gimp_dnd_viewable_source_add (GTK_WIDGET (row),
                                        viewable_type,
                                        gimp_row_drag_viewable,
                                        NULL);

          if (gimp_viewable_get_size (viewable, NULL, NULL))
            gimp_dnd_pixbuf_source_add (GTK_WIDGET (row),
                                        gimp_row_drag_pixbuf,
                                        NULL);
        }
    }

  g_set_weak_pointer (&priv->viewable, viewable);

  if (priv->viewable)
    {
      g_signal_connect (priv->viewable,
                        GIMP_VIEWABLE_GET_CLASS (viewable)->name_changed_signal,
                        G_CALLBACK (gimp_row_name_changed),
                        row);
      g_signal_connect (priv->viewable, "notify::icon-name",
                        G_CALLBACK (gimp_row_icon_changed),
                        row);

      preview = gimp_viewable_has_preview (priv->viewable);
    }

  gimp_row_name_changed (priv->viewable, row);

  gtk_widget_set_visible (priv->icon, ! preview);
  gimp_row_icon_changed (priv->viewable, NULL, row);

  if (priv->view)
    {
      gtk_widget_set_visible (priv->view, preview);
      gimp_view_set_viewable (GIMP_VIEW (priv->view), priv->viewable);
    }
}

static void
gimp_row_real_set_view_size (GimpRow *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  gtk_image_set_pixel_size (GTK_IMAGE (priv->icon),
                            priv->view_size);

  if (priv->view)
    gimp_view_renderer_set_size (GIMP_VIEW (priv->view)->renderer,
                                 priv->view_size,
                                 priv->view_border_width);
}

static void
gimp_row_real_icon_changed (GimpRow *row)
{
  GimpRowPrivate *priv      = GET_PRIVATE (row);
  const gchar    *icon_name = NULL;

  if (priv->viewable)
    icon_name = gimp_viewable_get_icon_name (priv->viewable);

  gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon), icon_name,
                                GTK_ICON_SIZE_BUTTON);
}

static void
gimp_row_real_name_changed (GimpRow *row)
{
  GimpRowPrivate *priv        = GET_PRIVATE (row);
  const gchar    *description = NULL;

  if (priv->viewable)
    description = gimp_viewable_get_description (priv->viewable, NULL);

  gtk_label_set_text (GTK_LABEL (priv->label), description);
}

static void
gimp_row_real_monitor_changed (GimpRow *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  if (priv->view)
    gimp_view_renderer_free_color_transform (GIMP_VIEW (priv->view)->renderer);
}

static void
gimp_row_real_edit_name (GimpRow *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);
  GtkWidget      *box;
  GtkWidget      *label;
  GtkWidget      *entry;
  const gchar    *default_name;
  gchar          *title;

  if (! priv->viewable ||
      ! gimp_viewable_is_name_editable (priv->viewable))
    {
      gtk_widget_error_bell (GTK_WIDGET (row));
      return;
    }

  priv->rename_popover = gtk_popover_new (priv->label);
  gtk_popover_set_modal (GTK_POPOVER (priv->rename_popover), TRUE);

  g_object_add_weak_pointer (G_OBJECT (priv->rename_popover),
                             (gpointer) &priv->rename_popover);

  g_signal_connect (priv->rename_popover, "closed",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (priv->rename_popover), box);
  gtk_widget_show (box);

  default_name = GIMP_VIEWABLE_GET_CLASS (priv->viewable)->default_name;

  title = g_strdup_printf (_("Rename %s"), default_name);
  label = gtk_label_new (title);
  g_free (title);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), gimp_object_get_name (priv->viewable));
  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  g_signal_connect (entry, "activate",
                    G_CALLBACK (gimp_row_rename_entry_activate),
                    row);

  gtk_popover_popup (GTK_POPOVER (priv->rename_popover));
}

static gboolean
gimp_row_real_name_edited (GimpRow     *row,
                           const gchar *new_name)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  gimp_object_set_name (GIMP_OBJECT (priv->viewable), new_name);

  return TRUE;
}


/*  public functions  */

GtkWidget *
gimp_row_new (GimpContext  *context,
              GimpViewable *viewable,
              gint          view_size,
              gint          view_border_width)
{
  GType row_type;

  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  row_type = gimp_row_type_from_viewable (viewable);

  return g_object_new (row_type,
                       "context",           context,
                       "viewable",          viewable,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       NULL);
}

void
gimp_row_set_context (GimpRow     *row,
                      GimpContext *context)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  g_return_if_fail (GIMP_IS_ROW (row));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (context != priv->context)
    {
      GIMP_ROW_GET_CLASS (row)->set_context (row, context);

      g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_CONTEXT]);
    }
}

GimpContext *
gimp_row_get_context (GimpRow *row)
{
  g_return_val_if_fail (GIMP_IS_ROW (row), NULL);

  return GET_PRIVATE (row)->context;
}

void
gimp_row_set_viewable (GimpRow      *row,
                       GimpViewable *viewable)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  g_return_if_fail (GIMP_IS_ROW (row));
  g_return_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable));

  if (viewable != priv->viewable)
    {
      GIMP_ROW_GET_CLASS (row)->set_viewable (row, viewable);

      g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_VIEWABLE]);
    }
}

GimpViewable *
gimp_row_get_viewable (GimpRow *row)
{
  g_return_val_if_fail (GIMP_IS_ROW (row), NULL);

  return GET_PRIVATE (row)->viewable;
}

void
gimp_row_set_view_size (GimpRow *row,
                        gint     view_size,
                        gint     view_border_width)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  g_return_if_fail (GIMP_IS_ROW (row));
  g_return_if_fail (view_size >  0 &&
                    view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (view_border_width >= 0 &&
                    view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH);

  if (priv->view_size         != view_size ||
      priv->view_border_width != view_border_width)
    {
      priv->view_size         = view_size;
      priv->view_border_width = view_border_width;

      GIMP_ROW_GET_CLASS (row)->set_view_size (row);

      g_object_freeze_notify (G_OBJECT (row));
      g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_VIEW_SIZE]);
      g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_VIEW_BORDER_WIDTH]);
      g_object_thaw_notify (G_OBJECT (row));
    }
}

gint
gimp_row_get_view_size (GimpRow *row,
                        gint    *view_border_width)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (GIMP_IS_ROW (row), 1);

  if (view_border_width)
    *view_border_width = priv->view_border_width;

  return priv->view_size;
}

void
gimp_row_monitor_changed (GimpRow *row)
{
  g_return_if_fail (GIMP_IS_ROW (row));

  GIMP_ROW_GET_CLASS (row)->monitor_changed (row);
}

GtkWidget *
_gimp_row_get_box (GimpRow *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (GIMP_IS_ROW (row), NULL);

  return priv->box;
}

GtkWidget *
_gimp_row_get_icon (GimpRow *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (GIMP_IS_ROW (row), NULL);

  return priv->icon;
}

GtkWidget *
_gimp_row_get_view (GimpRow *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (GIMP_IS_ROW (row), NULL);

  return priv->view;
}

GtkWidget *
_gimp_row_get_label (GimpRow *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  g_return_val_if_fail (GIMP_IS_ROW (row), NULL);

  return priv->label;
}


/*  private functions  */

static void
gimp_row_icon_changed (GimpViewable     *viewable,
                       const GParamSpec *pspec,
                       GimpRow          *row)
{
  GIMP_ROW_GET_CLASS (row)->icon_changed (row);
}

static void
gimp_row_name_changed (GimpViewable *viewable,
                       GimpRow      *row)
{
  GIMP_ROW_GET_CLASS (row)->name_changed (row);
}

static void
gimp_row_label_long_pressed (GtkGesture *gesture,
                             gdouble     x,
                             gdouble     y,
                             GimpRow    *row)
{
  g_signal_emit (row, row_signals[EDIT_NAME], 0);
}

static void
gimp_row_rename_entry_activate (GtkEntry *entry,
                                GimpRow  *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);
  const gchar    *old_name;
  gchar          *new_name;

  old_name = gimp_object_get_name (priv->viewable);

  /*  Dup the new name, and popdown before renaming, because it can
   *  destroy the row and the popup.
   */
  new_name = g_strdup (gtk_entry_get_text (entry));
  gtk_popover_popdown (GTK_POPOVER (priv->rename_popover));

  if (g_strcmp0 (old_name, new_name))
    {
      if (! GIMP_ROW_GET_CLASS (row)->name_edited (row, new_name))
        {
          gtk_widget_error_bell (GTK_WIDGET (row));
        }
    }

  g_free (new_name);
}

static GimpViewable *
gimp_row_drag_viewable (GtkWidget    *widget,
                        GimpContext **context,
                        gpointer      data)
{
  GimpRowPrivate *priv = GET_PRIVATE (widget);

  if (context)
    *context = priv->context;

  return priv->viewable;
}

static GdkPixbuf *
gimp_row_drag_pixbuf (GtkWidget *widget,
                      gpointer   data)
{
  GimpRowPrivate *priv = GET_PRIVATE (widget);
  gint            width;
  gint            height;

  g_printerr ("drag pixbuf\n");

  if (priv->viewable &&
      gimp_viewable_get_size (priv->viewable, &width, &height))
    {
      return gimp_viewable_get_new_pixbuf (priv->viewable,
                                           priv->context,
                                           width, height, NULL);
    }

  g_printerr ("failed\n");

  return NULL;
}
