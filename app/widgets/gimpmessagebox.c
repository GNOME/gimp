/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamessagebox.c
 * Copyright (C) 2004 Sven Neumann <sven@ligma.org>
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
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "ligmamessagebox.h"

#include "ligma-intl.h"


#define LIGMA_MESSAGE_BOX_SPACING  12

enum
{
  PROP_0,
  PROP_ICON_NAME
};


static void   ligma_message_box_constructed          (GObject        *object);
static void   ligma_message_box_dispose              (GObject        *object);
static void   ligma_message_box_finalize             (GObject        *object);
static void   ligma_message_box_set_property         (GObject        *object,
                                                     guint           property_id,
                                                     const GValue   *value,
                                                     GParamSpec     *pspec);
static void   ligma_message_box_get_property         (GObject        *object,
                                                     guint           property_id,
                                                     GValue         *value,
                                                     GParamSpec     *pspec);

static void   ligma_message_box_get_preferred_width  (GtkWidget      *widget,
                                                     gint           *minimum_width,
                                                     gint           *natural_width);
static void   ligma_message_box_get_preferred_height (GtkWidget      *widget,
                                                     gint           *minimum_height,
                                                     gint           *natural_height);
static void   ligma_message_box_get_preferred_width_for_height
                                                    (GtkWidget      *widget,
                                                     gint            height,
                                                     gint           *minimum_width,
                                                     gint           *natural_width);
static void   ligma_message_box_get_preferred_height_for_width
                                                    (GtkWidget      *widget,
                                                     gint            width,
                                                     gint           *minimum_height,
                                                     gint           *natural_height);
static void   ligma_message_box_size_allocate        (GtkWidget      *widget,
                                                     GtkAllocation  *allocation);

static void   ligma_message_box_forall               (GtkContainer   *container,
                                                     gboolean        include_internals,
                                                     GtkCallback     callback,
                                                     gpointer        callback_data);

static void   ligma_message_box_set_label_text       (LigmaMessageBox *box,
                                                     gint            n,
                                                     const gchar    *format,
                                                     va_list         args) G_GNUC_PRINTF (3, 0);
static void   ligma_message_box_set_label_markup     (LigmaMessageBox *box,
                                                     gint            n,
                                                     const gchar    *format,
                                                     va_list         args) G_GNUC_PRINTF (3, 0);

static gboolean ligma_message_box_update             (gpointer        data);


G_DEFINE_TYPE (LigmaMessageBox, ligma_message_box, GTK_TYPE_BOX)

#define parent_class ligma_message_box_parent_class


static void
ligma_message_box_class_init (LigmaMessageBoxClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class    = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->constructed          = ligma_message_box_constructed;
  object_class->dispose              = ligma_message_box_dispose;
  object_class->finalize             = ligma_message_box_finalize;
  object_class->set_property         = ligma_message_box_set_property;
  object_class->get_property         = ligma_message_box_get_property;

  widget_class->get_preferred_width  = ligma_message_box_get_preferred_width;
  widget_class->get_preferred_height = ligma_message_box_get_preferred_height;
  widget_class->get_preferred_width_for_height = ligma_message_box_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ligma_message_box_get_preferred_height_for_width;
  widget_class->size_allocate        = ligma_message_box_size_allocate;

  container_class->forall            = ligma_message_box_forall;

  gtk_container_class_handle_border_width (container_class);

  g_object_class_install_property (object_class, PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name", NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_message_box_init (LigmaMessageBox *box)
{
  gint i;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (box), 12);
  gtk_container_set_border_width (GTK_CONTAINER (box), 12);

  /*  Unset the focus chain to keep the labels from being in the focus
   *  chain.  Users of LigmaMessageBox that add focusable widgets should
   *  either unset the focus chain or (better) explicitly set one.
   */
  gtk_container_set_focus_chain (GTK_CONTAINER (box), NULL);

  for (i = 0; i < 2; i++)
    {
      GtkWidget *label = g_object_new (GTK_TYPE_LABEL,
                                       "wrap",            TRUE,
                                       "wrap-mode",       PANGO_WRAP_WORD_CHAR,
                                       "max-width-chars", 80,
                                       "selectable",      TRUE,
                                       "xalign",          0.0,
                                       "yalign",          0.5,
                                       NULL);

      if (i == 0)
        ligma_label_set_attributes (GTK_LABEL (label),
                                   PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                                   PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                   -1);

      gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

      box->label[i] = label;
    }

  box->repeat   = 0;
  box->label[2] = NULL;
  box->idle_id  = 0;
}

static void
ligma_message_box_constructed (GObject *object)
{
  LigmaMessageBox *box = LIGMA_MESSAGE_BOX (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (box->icon_name)
    {
      box->image = gtk_image_new_from_icon_name (box->icon_name,
                                                 GTK_ICON_SIZE_DIALOG);

      gtk_widget_set_halign (box->image, GTK_ALIGN_START);
      gtk_widget_set_valign (box->image, GTK_ALIGN_START);
      gtk_widget_set_parent (box->image, GTK_WIDGET (box));
      gtk_widget_show (box->image);
    }
}

static void
ligma_message_box_dispose (GObject *object)
{
  LigmaMessageBox *box = LIGMA_MESSAGE_BOX (object);

  if (box->image)
    {
      gtk_widget_unparent (box->image);
      box->image = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_message_box_finalize (GObject *object)
{
  LigmaMessageBox *box = LIGMA_MESSAGE_BOX (object);

  if (box->idle_id)
    {
      g_source_remove (box->idle_id);
      box->idle_id = 0;
    }

  g_clear_pointer (&box->icon_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_message_box_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaMessageBox *box = LIGMA_MESSAGE_BOX (object);

  switch (property_id)
    {
    case PROP_ICON_NAME:
      box->icon_name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_message_box_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaMessageBox *box = LIGMA_MESSAGE_BOX (object);

  switch (property_id)
    {
    case PROP_ICON_NAME:
      g_value_set_string (value, box->icon_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_message_box_get_preferred_width (GtkWidget *widget,
                                      gint      *minimum_width,
                                      gint      *natural_width)
{
  LigmaMessageBox *box = LIGMA_MESSAGE_BOX (widget);

  GTK_WIDGET_CLASS (parent_class)->get_preferred_width (widget,
                                                        minimum_width,
                                                        natural_width);

  if (box->image && gtk_widget_get_visible (box->image))
    {
      gint image_minimum;
      gint image_natural;

      gtk_widget_get_preferred_width (box->image,
                                      &image_minimum, &image_natural);

      *minimum_width += image_minimum + LIGMA_MESSAGE_BOX_SPACING;
      *natural_width += image_natural + LIGMA_MESSAGE_BOX_SPACING;
    }
}

static void
ligma_message_box_get_preferred_height (GtkWidget *widget,
                                       gint      *minimum_height,
                                       gint      *natural_height)
{
  LigmaMessageBox *box = LIGMA_MESSAGE_BOX (widget);

  GTK_WIDGET_CLASS (parent_class)->get_preferred_height (widget,
                                                         minimum_height,
                                                         natural_height);

  if (box->image && gtk_widget_get_visible (box->image))
    {
      gint image_minimum;
      gint image_natural;

      gtk_widget_get_preferred_height (box->image,
                                       &image_minimum, &image_natural);

      *minimum_height = MAX (*minimum_height, image_minimum);
      *natural_height = MAX (*natural_height, image_natural);
    }
}

static void
ligma_message_box_get_preferred_width_for_height (GtkWidget *widget,
                                                 gint       height,
                                                 gint      *minimum_width,
                                                 gint      *natural_width)
{
  GTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget,
                                                      minimum_width,
                                                      natural_width);
}

static void
ligma_message_box_get_preferred_height_for_width (GtkWidget *widget,
                                                 gint       width,
                                                 gint      *minimum_height,
                                                 gint      *natural_height)
{
  GTK_WIDGET_GET_CLASS (widget)->get_preferred_height (widget,
                                                       minimum_height,
                                                       natural_height);
}

static void
ligma_message_box_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
  LigmaMessageBox *box   = LIGMA_MESSAGE_BOX (widget);
  gint            width = 0;
  gboolean        rtl;

  rtl = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL);

  if (box->image && gtk_widget_get_visible (box->image))
    {
      GtkRequisition child_requisition;
      GtkAllocation  child_allocation;
      gint           height;

      gtk_widget_get_preferred_size (box->image, &child_requisition, NULL);

      width  = MIN (allocation->width,
                    child_requisition.width + LIGMA_MESSAGE_BOX_SPACING);
      width  = MAX (1, width);

      height = allocation->height;

      if (rtl)
        child_allocation.x = allocation->width - child_requisition.width;
      else
        child_allocation.x = allocation->x;

      child_allocation.y      = allocation->y;
      child_allocation.width  = width;
      child_allocation.height = height;

      gtk_widget_size_allocate (box->image, &child_allocation);
    }

  allocation->x     += rtl ? 0 : width;
  allocation->width -= width;

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  allocation->x      -= rtl ? 0 : width;
  allocation->width  += width;

  gtk_widget_set_allocation (widget, allocation);
}

static void
ligma_message_box_forall (GtkContainer *container,
                         gboolean      include_internals,
                         GtkCallback   callback,
                         gpointer      callback_data)
{
  if (include_internals)
    {
      LigmaMessageBox *box = LIGMA_MESSAGE_BOX (container);

      if (box->image)
        (* callback) (box->image, callback_data);
    }

  GTK_CONTAINER_CLASS (parent_class)->forall (container, include_internals,
                                              callback, callback_data);
}

static void
ligma_message_box_set_label_text (LigmaMessageBox *box,
                                 gint            n,
                                 const gchar    *format,
                                 va_list         args)
{
  GtkWidget *label = box->label[n];

  if (format)
    {
      gchar *text = g_strdup_vprintf (format, args);
      gchar *utf8 = ligma_any_to_utf8 (text, -1, "Cannot convert text to utf8.");

      gtk_label_set_text (GTK_LABEL (label), utf8);
      gtk_widget_show (label);

      g_free (utf8);
      g_free (text);
    }
  else
    {
      gtk_widget_hide (label);
      gtk_label_set_text (GTK_LABEL (label), NULL);
    }
}

static void
ligma_message_box_set_label_markup (LigmaMessageBox *box,
                                   gint            n,
                                   const gchar    *format,
                                   va_list         args)
{
  GtkWidget *label = box->label[n];

  if (format)
    {
      gchar *text = g_markup_vprintf_escaped (format, args);

      gtk_label_set_markup (GTK_LABEL (label), text);
      gtk_widget_show (label);

      g_free (text);
    }
  else
    {
      gtk_widget_hide (label);
      gtk_label_set_text (GTK_LABEL (label), NULL);
    }
}

static gboolean
ligma_message_box_update (gpointer data)
{
  LigmaMessageBox *box = data;
  gchar          *message;

  box->idle_id = 0;

  message = g_strdup_printf (ngettext ("Message repeated once.",
                                       "Message repeated %d times.",
                                       box->repeat),
                             box->repeat);

  if (box->label[2])
    {
      gtk_label_set_text (GTK_LABEL (box->label[2]), message);
    }
  else
    {
      GtkWidget *label = box->label[2] = gtk_label_new (message);

      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      ligma_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_OBLIQUE,
                                 -1);
      gtk_box_pack_end (GTK_BOX (box), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
    }

  g_free (message);

  return G_SOURCE_REMOVE;
}

/*  public functions  */

GtkWidget *
ligma_message_box_new (const gchar *icon_name)
{
  return g_object_new (LIGMA_TYPE_MESSAGE_BOX,
                       "icon-name", icon_name,
                       NULL);
}

void
ligma_message_box_set_primary_text (LigmaMessageBox *box,
                                   const gchar    *format,
                                   ...)
{
  va_list args;

  g_return_if_fail (LIGMA_IS_MESSAGE_BOX (box));

  va_start (args, format);
  ligma_message_box_set_label_text (box, 0, format, args);
  va_end (args);
}

void
ligma_message_box_set_text (LigmaMessageBox *box,
                           const gchar    *format,
                           ...)
{
  va_list args;

  g_return_if_fail (LIGMA_IS_MESSAGE_BOX (box));

  va_start (args, format);
  ligma_message_box_set_label_text (box, 1, format, args);
  va_end (args);
}

void
ligma_message_box_set_markup (LigmaMessageBox *box,
                             const gchar    *format,
                             ...)
{
  va_list args;

  g_return_if_fail (LIGMA_IS_MESSAGE_BOX (box));

  va_start (args, format);
  ligma_message_box_set_label_markup (box, 1,format, args);
  va_end (args);
}

gint
ligma_message_box_repeat (LigmaMessageBox *box)
{
  g_return_val_if_fail (LIGMA_IS_MESSAGE_BOX (box), 0);

  box->repeat++;

  if (box->idle_id == 0)
    {
      /* When a same message is repeated dozens of thousands of times in
       * a short span of time, updating the GUI at each increment is
       * extremely slow (like really really slow, your GUI gets stuck
       * for 10 minutes). So let's just delay GUI update as a low
       * priority idle task.
       */
      box->idle_id = g_idle_add_full (G_PRIORITY_LOW,
                                      ligma_message_box_update,
                                      box, NULL);
    }

  return box->repeat;
}
