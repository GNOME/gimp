/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawablechooser.h
 * Copyright (C) 2023 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimpdrawablechooser.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimpdrawablechooser
 * @title: GimpDrawableChooser
 * @short_description: A widget allowing to select a drawable.
 *
 * The chooser contains an optional label and a button which queries the core
 * process to pop up a drawable selection dialog.
 *
 * Since: 3.0
 **/

#define CELL_SIZE 40

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_LABEL,
  PROP_DRAWABLE,
  PROP_DRAWABLE_TYPE,
  N_PROPS
};

struct _GimpDrawableChooser
{
  GtkBox        parent_instance;

  GType         drawable_type;
  GimpDrawable *drawable;
  gchar        *title;
  gchar        *label;
  gchar        *callback;

  GBytes       *thumbnail;
  GimpDrawable *thumbnail_drawable;
  gint          width;
  gint          height;
  gint          bpp;

  GtkWidget    *label_widget;
  GtkWidget    *preview_frame;
  GtkWidget    *preview;
  GtkWidget    *preview_title;
};


/*  local function prototypes  */

static void             gimp_drawable_chooser_constructed       (GObject             *object);
static void             gimp_drawable_chooser_dispose           (GObject             *object);
static void             gimp_drawable_chooser_finalize          (GObject             *object);

static void             gimp_drawable_chooser_set_property      (GObject             *object,
                                                                 guint                property_id,
                                                                 const GValue        *value,
                                                                 GParamSpec          *pspec);
static void             gimp_drawable_chooser_get_property      (GObject             *object,
                                                                 guint                property_id,
                                                                 GValue              *value,
                                                                 GParamSpec          *pspec);

static void             gimp_drawable_chooser_clicked           (GimpDrawableChooser *chooser);

static GimpValueArray * gimp_temp_callback_run                  (GimpProcedure       *procedure,
                                                                 GimpProcedureConfig *config,
                                                                 GimpDrawableChooser *chooser);
static gboolean         gimp_drawable_select_remove_after_run   (const gchar *procedure_name);

static void             gimp_drawable_chooser_draw              (GimpDrawableChooser *chooser);
static void             gimp_drawable_chooser_get_thumbnail     (GimpDrawableChooser *chooser,
                                                                 gint              width,
                                                                 gint              height);


static GParamSpec *drawable_button_props[N_PROPS] = { NULL, };

G_DEFINE_FINAL_TYPE (GimpDrawableChooser, gimp_drawable_chooser, GTK_TYPE_BOX)


static void
gimp_drawable_chooser_class_init (GimpDrawableChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_drawable_chooser_constructed;
  object_class->dispose      = gimp_drawable_chooser_dispose;
  object_class->finalize     = gimp_drawable_chooser_finalize;
  object_class->set_property = gimp_drawable_chooser_set_property;
  object_class->get_property = gimp_drawable_chooser_get_property;

  /**
   * GimpDrawableChooser:title:
   *
   * The title to be used for the drawable selection popup dialog.
   *
   * Since: 3.0
   */
  drawable_button_props[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title to be used for the drawable selection popup dialog",
                         "Drawable Selection",
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * GimpDrawableChooser:label:
   *
   * Label text with mnemonic.
   *
   * Since: 3.0
   */
  drawable_button_props[PROP_LABEL] =
    g_param_spec_string ("label",
                         "Label",
                         "The label to be used next to the button",
                         NULL,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * GimpDrawableChooser:drawable:
   *
   * The currently selected drawable.
   *
   * Since: 3.0
   */
  drawable_button_props[PROP_DRAWABLE] =
    gimp_param_spec_drawable ("drawable",
                              "Drawable",
                              "The currently selected drawable",
                              TRUE,
                              GIMP_PARAM_READWRITE |
                              G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GimpDrawableChooser:drawable-type:
   *
   * Allowed drawable types, which must be either GIMP_TYPE_DRAWABLE or a
   * subtype.
   *
   * Since: 3.0
   */
  drawable_button_props[PROP_DRAWABLE_TYPE] =
    g_param_spec_gtype ("drawable-type",
                        "Allowed drawable Type",
                        "The GType of the drawable property",
                        GIMP_TYPE_DRAWABLE,
                        G_PARAM_CONSTRUCT_ONLY |
                        GIMP_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     N_PROPS, drawable_button_props);
}

static void
gimp_drawable_chooser_init (GimpDrawableChooser *chooser)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (chooser),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (chooser), 6);

  chooser->thumbnail_drawable = NULL;
  chooser->thumbnail          = NULL;
}

static void
gimp_drawable_chooser_constructed (GObject *object)
{
  GimpDrawableChooser *chooser = GIMP_DRAWABLE_CHOOSER (object);
  GtkWidget           *button;
  GtkWidget           *box;
  gint                 scale_factor;

  scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (chooser));

  chooser->label_widget = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (chooser), chooser->label_widget, FALSE, FALSE, 0);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (chooser->label_widget), chooser->label);
  gtk_widget_show (GTK_WIDGET (chooser->label_widget));

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (chooser), button, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (chooser->label_widget), button);
  gtk_widget_show (button);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_add (GTK_CONTAINER (button), box);
  gtk_widget_show (box);

  chooser->preview_frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (chooser->preview_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), chooser->preview_frame, FALSE, FALSE, 0);
  gtk_widget_show (chooser->preview_frame);

  chooser->preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (chooser->preview, scale_factor * CELL_SIZE, scale_factor * CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (chooser->preview_frame), chooser->preview);
  gtk_widget_show (chooser->preview);

  chooser->preview_title = gtk_label_new (_("Browse..."));
  gtk_box_pack_start (GTK_BOX (box), chooser->preview_title, FALSE, FALSE, 0);
  gtk_widget_show (chooser->preview_title);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_drawable_chooser_clicked),
                            chooser);

  G_OBJECT_CLASS (gimp_drawable_chooser_parent_class)->constructed (object);
}

static void
gimp_drawable_chooser_dispose (GObject *object)
{
  GimpDrawableChooser *chooser = GIMP_DRAWABLE_CHOOSER (object);

  if (chooser->callback)
    {
      gimp_drawables_close_popup (chooser->callback);

      gimp_plug_in_remove_temp_procedure (gimp_get_plug_in (), chooser->callback);
      g_clear_pointer (&chooser->callback, g_free);
    }

  G_OBJECT_CLASS (gimp_drawable_chooser_parent_class)->dispose (object);
}

static void
gimp_drawable_chooser_finalize (GObject *object)
{
  GimpDrawableChooser *chooser = GIMP_DRAWABLE_CHOOSER (object);

  g_clear_pointer (&chooser->title, g_free);
  g_clear_pointer (&chooser->label, g_free);
  g_clear_pointer (&chooser->thumbnail, g_bytes_unref);

  G_OBJECT_CLASS (gimp_drawable_chooser_parent_class)->finalize (object);
}

static void
gimp_drawable_chooser_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *gvalue,
                                    GParamSpec   *pspec)
{
  GimpDrawableChooser *chooser = GIMP_DRAWABLE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_TITLE:
      chooser->title = g_value_dup_string (gvalue);
      break;

    case PROP_LABEL:
      chooser->label = g_value_dup_string (gvalue);
      break;

    case PROP_DRAWABLE:
      g_return_if_fail (g_value_get_object (gvalue) == NULL ||
                        g_type_is_a (G_TYPE_FROM_INSTANCE (g_value_get_object (gvalue)),
                                     chooser->drawable_type));
      gimp_drawable_chooser_set_drawable (chooser, g_value_get_object (gvalue));
      break;

    case PROP_DRAWABLE_TYPE:
      chooser->drawable_type = g_value_get_gtype (gvalue);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_chooser_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpDrawableChooser *chooser = GIMP_DRAWABLE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, chooser->title);
      break;

    case PROP_LABEL:
      g_value_set_string (value, chooser->label);
      break;

    case PROP_DRAWABLE:
      g_value_set_object (value, chooser->drawable);
      break;

    case PROP_DRAWABLE_TYPE:
      g_value_set_gtype (value, chooser->drawable_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_drawable_chooser_new:
 * @title:    (nullable): Title of the dialog to use or %NULL to use the default title.
 * @label:    (nullable): Button label or %NULL for no label.
 * @drawable_type: the acceptable subtype of choosable drawables.
 * @drawable: (nullable): Initial drawable.
 *
 * Creates a new #GtkWidget that lets a user choose a drawable which must be of
 * type @drawable_type. @drawable_type of values %G_TYPE_NONE and
 * %GIMP_TYPE_DRAWABLE are equivalent. Otherwise it must be a subtype of
 * %GIMP_TYPE_DRAWABLE.
 *
 * When @drawable is %NULL, initial choice is from context.
 *
 * Returns: A [class@GimpUi.DrawableChooser.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_drawable_chooser_new (const gchar  *title,
                           const gchar  *label,
                           GType         drawable_type,
                           GimpDrawable *drawable)
{
  GtkWidget *chooser;

  if (drawable_type == G_TYPE_NONE)
    drawable_type = GIMP_TYPE_DRAWABLE;

  g_return_val_if_fail (g_type_is_a (drawable_type, GIMP_TYPE_DRAWABLE), NULL);
  g_return_val_if_fail (drawable == NULL ||
                        g_type_is_a (G_TYPE_FROM_INSTANCE (drawable), drawable_type),
                        NULL);

  chooser = g_object_new (GIMP_TYPE_DRAWABLE_CHOOSER,
                          "title",         title,
                          "label",         label,
                          "drawable",      drawable,
                          "drawable-type", drawable_type,
                          NULL);

  return chooser;
}

/**
 * gimp_drawable_chooser_get_drawable:
 * @chooser: A #GimpDrawableChooser
 *
 * Gets the currently selected drawable.
 *
 * Returns: (transfer none): an internal copy of the drawable which must not be freed.
 *
 * Since: 3.0
 */
GimpDrawable *
gimp_drawable_chooser_get_drawable (GimpDrawableChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_CHOOSER (chooser), NULL);

  return chooser->drawable;
}

/**
 * gimp_drawable_chooser_set_drawable:
 * @chooser: A #GimpDrawableChooser
 * @drawable: Drawable to set.
 *
 * Sets the currently selected drawable.
 * This will select the drawable in both the button and any chooser popup.
 *
 * Since: 3.0
 */
void
gimp_drawable_chooser_set_drawable (GimpDrawableChooser *chooser,
                                    GimpDrawable        *drawable)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_CHOOSER (chooser));
  g_return_if_fail (drawable == NULL || GIMP_IS_DRAWABLE (drawable));

  chooser->drawable = drawable;

  if (chooser->callback)
    gimp_drawables_set_popup (chooser->callback, chooser->drawable);

  g_object_notify_by_pspec (G_OBJECT (chooser), drawable_button_props[PROP_DRAWABLE]);

  gimp_drawable_chooser_draw (chooser);
}

/**
 * gimp_drawable_chooser_get_label:
 * @widget: A [class@DrawableChooser].
 *
 * Returns the label widget.
 *
 * Returns: (transfer none): the [class@Gtk.Widget] showing the label text.
 * Since: 3.0
 */
GtkWidget *
gimp_drawable_chooser_get_label (GimpDrawableChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_CHOOSER (chooser), NULL);

  return chooser->label_widget;
}


/*  private functions  */

static GimpValueArray *
gimp_temp_callback_run (GimpProcedure       *procedure,
                        GimpProcedureConfig *config,
                        GimpDrawableChooser *chooser)
{
  GimpDrawable *drawable;
  gboolean      closing;

  g_object_get (config,
                "drawable", &drawable,
                "closing",  &closing,
                NULL);

  g_object_set (chooser, "drawable", drawable, NULL);

  if (closing)
    {
      g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                       (GSourceFunc) gimp_drawable_select_remove_after_run,
                       g_strdup (gimp_procedure_get_name (procedure)),
                       g_free);
      g_clear_pointer (&chooser->callback, g_free);
    }

  g_clear_object (&drawable);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_drawable_select_remove_after_run (const gchar *procedure_name)
{
  gimp_plug_in_remove_temp_procedure (gimp_get_plug_in (), procedure_name);

  return G_SOURCE_REMOVE;
}

static void
gimp_drawable_chooser_clicked (GimpDrawableChooser *chooser)
{
  if (chooser->callback)
    {
      /* Popup already created.  Calling setter raises the popup. */
      gimp_drawables_set_popup (chooser->callback, chooser->drawable);
    }
  else
    {
      GimpPlugIn    *plug_in  = gimp_get_plug_in ();
      GtkWidget     *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (chooser));
      GBytes        *handle   = NULL;
      gchar         *callback_name;
      GimpProcedure *callback_procedure;

      if (GIMP_IS_DIALOG (toplevel))
        handle = gimp_dialog_get_native_handle (GIMP_DIALOG (toplevel));

      callback_name = gimp_pdb_temp_procedure_name (gimp_get_pdb ());
      callback_procedure = gimp_procedure_new (plug_in,
                                               callback_name,
                                               GIMP_PDB_PROC_TYPE_TEMPORARY,
                                               (GimpRunFunc) gimp_temp_callback_run,
                                               g_object_ref (chooser),
                                               (GDestroyNotify) g_object_unref);
      gimp_procedure_add_drawable_argument (callback_procedure, "drawable",
                                            "Drawable", "The selected drawable",
                                            TRUE, G_PARAM_READWRITE);
      gimp_procedure_add_boolean_argument (callback_procedure, "closing",
                                           "Closing", "If the dialog was closing",
                                           FALSE, G_PARAM_READWRITE);

      gimp_plug_in_add_temp_procedure (plug_in, callback_procedure);
      g_object_unref (callback_procedure);
      g_free (callback_name);

      if (gimp_drawables_popup (gimp_procedure_get_name (callback_procedure), chooser->title,
                                g_type_name (chooser->drawable_type), chooser->drawable, handle))
        {
          /* Allow callbacks to be watched */
          gimp_plug_in_persistent_enable (plug_in);

          chooser->callback = g_strdup (gimp_procedure_get_name (callback_procedure));
        }
      else
        {
          g_warning ("%s: failed to open remote drawable select dialog.", G_STRFUNC);
          gimp_plug_in_remove_temp_procedure (plug_in, gimp_procedure_get_name (callback_procedure));
          return;
        }
      gimp_drawables_set_popup (chooser->callback, chooser->drawable);
    }
}

static void
gimp_drawable_chooser_draw (GimpDrawableChooser *chooser)
{
  GimpPreviewArea *area = GIMP_PREVIEW_AREA (chooser->preview);
  GtkAllocation    allocation;
  gint             x = 0;
  gint             y = 0;

  gtk_widget_get_allocation (chooser->preview, &allocation);

  gimp_drawable_chooser_get_thumbnail (chooser, allocation.width, allocation.height);

  if (chooser->width < allocation.width ||
      chooser->height < allocation.height)
    {
      x = ((allocation.width  - chooser->width)  / 2);
      y = ((allocation.height - chooser->height) / 2);
    }
  gimp_preview_area_reset (area);

  if (chooser->thumbnail)
    {
      GimpImageType type;
      gint          rowstride;

      rowstride = chooser->width * chooser->bpp;
      switch (chooser->bpp)
        {
        case 1:
          type = GIMP_GRAY_IMAGE;
          break;
        case 2:
          type = GIMP_GRAYA_IMAGE;
          break;
        case 3:
          type = GIMP_RGB_IMAGE;
          break;
        case 4:
          type = GIMP_RGBA_IMAGE;
          break;
        default:
          g_return_if_reached ();
        }

      gimp_preview_area_draw (area, x, y, chooser->width, chooser->height, type,
                              g_bytes_get_data (chooser->thumbnail, NULL), rowstride);
    }
}

static void
gimp_drawable_chooser_get_thumbnail (GimpDrawableChooser *chooser,
                                     gint                 width,
                                     gint                 height)
{
  if (chooser->drawable == chooser->thumbnail_drawable &&
      chooser->width    == width                       &&
      chooser->height   == height)
    /* Let's assume drawable contents is not changing in a single run. */
    return;

  g_clear_pointer (&chooser->thumbnail, g_bytes_unref);
  chooser->width = chooser->height = 0;

  chooser->thumbnail_drawable = chooser->drawable;
  if (chooser->drawable)
    chooser->thumbnail = gimp_drawable_get_thumbnail_data (chooser->drawable,
                                                           width, height,
                                                           &chooser->width,
                                                           &chooser->height,
                                                           &chooser->bpp);
}
