/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpimagechooser.c
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
#include "gimpimagechooser.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimpimagechooser
 * @title: GimpImageChooser
 * @short_description: A widget allowing to select a image.
 *
 * The chooser contains an optional label and a button which queries the core
 * process to pop up an image selection dialog.
 *
 * Since: 3.0
 **/

#define CELL_SIZE   40
#define BUTTON_SIZE 96

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_LABEL,
  PROP_IMAGE,
  N_PROPS
};

struct _GimpImageChooser
{
  GtkBox        parent_instance;

  GimpImage    *image;
  gchar        *title;
  gchar        *label;
  gchar        *callback;

  GBytes       *thumbnail;
  gint          width;
  gint          height;
  gint          bpp;

  GtkWidget    *label_widget;
  GtkWidget    *preview_frame;
  GtkWidget    *preview;
  GtkWidget    *preview_title;
};


/*  local function prototypes  */

static void             gimp_image_chooser_constructed       (GObject             *object);
static void             gimp_image_chooser_dispose           (GObject             *object);
static void             gimp_image_chooser_finalize          (GObject             *object);

static void             gimp_image_chooser_set_property      (GObject             *object,
                                                              guint                property_id,
                                                              const GValue        *value,
                                                              GParamSpec          *pspec);
static void             gimp_image_chooser_get_property      (GObject             *object,
                                                              guint                property_id,
                                                              GValue              *value,
                                                              GParamSpec          *pspec);

static void             gimp_image_chooser_clicked           (GimpImageChooser    *chooser);

static GimpValueArray * gimp_temp_callback_run               (GimpProcedure       *procedure,
                                                              GimpProcedureConfig *config,
                                                              GimpImageChooser    *chooser);
static gboolean         gimp_image_select_remove_after_run   (const gchar         *procedure_name);

static void             gimp_image_chooser_draw              (GimpImageChooser    *chooser);
static void             gimp_image_chooser_get_thumbnail     (GimpImageChooser    *chooser,
                                                              gint                 width,
                                                              gint                 height);


static GParamSpec *image_button_props[N_PROPS] = { NULL, };

G_DEFINE_FINAL_TYPE (GimpImageChooser, gimp_image_chooser, GTK_TYPE_BOX)


static void
gimp_image_chooser_class_init (GimpImageChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_image_chooser_constructed;
  object_class->dispose      = gimp_image_chooser_dispose;
  object_class->finalize     = gimp_image_chooser_finalize;
  object_class->set_property = gimp_image_chooser_set_property;
  object_class->get_property = gimp_image_chooser_get_property;

  /**
   * GimpImageChooser:title:
   *
   * The title to be used for the image selection popup dialog.
   *
   * Since: 3.0
   */
  image_button_props[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title to be used for the image selection popup dialog",
                         "Image Selection",
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * GimpImageChooser:label:
   *
   * Label text with mnemonic.
   *
   * Since: 3.0
   */
  image_button_props[PROP_LABEL] =
    g_param_spec_string ("label",
                         "Label",
                         "The label to be used next to the button",
                         NULL,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * GimpImageChooser:image:
   *
   * The currently selected image.
   *
   * Since: 3.0
   */
  image_button_props[PROP_IMAGE] =
    gimp_param_spec_image ("image",
                           "Image",
                           "The currently selected image",
                           TRUE,
                           GIMP_PARAM_READWRITE |
                           G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class,
                                     N_PROPS, image_button_props);
}

static void
gimp_image_chooser_init (GimpImageChooser *chooser)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (chooser),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (chooser), 6);

  chooser->thumbnail = NULL;
}

static void
gimp_image_chooser_constructed (GObject *object)
{
  GimpImageChooser *chooser = GIMP_IMAGE_CHOOSER (object);
  GtkWidget        *button;
  GtkWidget        *box;
  gint              scale_factor;

  scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (chooser));

  chooser->label_widget = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (chooser), chooser->label_widget, FALSE, FALSE, 0);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (chooser->label_widget), chooser->label);
  gtk_widget_set_visible (GTK_WIDGET (chooser->label_widget), TRUE);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (chooser), button, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (chooser->label_widget), button);
  gtk_widget_set_visible (button, TRUE);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_add (GTK_CONTAINER (button), box);
  gtk_widget_set_visible (box, TRUE);

  chooser->preview_frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (chooser->preview_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box), chooser->preview_frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (chooser->preview_frame, TRUE);

  chooser->preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (chooser->preview, scale_factor * CELL_SIZE, scale_factor * CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (chooser->preview_frame), chooser->preview);
  gtk_widget_set_visible (chooser->preview, TRUE);

  chooser->preview_title = gtk_label_new (_("Browse..."));
  gtk_box_pack_start (GTK_BOX (box), chooser->preview_title, FALSE, FALSE, 0);
  gtk_widget_set_visible (chooser->preview_title, TRUE);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_image_chooser_clicked),
                            chooser);

  G_OBJECT_CLASS (gimp_image_chooser_parent_class)->constructed (object);
}

static void
gimp_image_chooser_dispose (GObject *object)
{
  GimpImageChooser *chooser = GIMP_IMAGE_CHOOSER (object);

  if (chooser->callback)
    {
      gimp_images_close_popup (chooser->callback);

      gimp_plug_in_remove_temp_procedure (gimp_get_plug_in (), chooser->callback);
      g_clear_pointer (&chooser->callback, g_free);
    }

  G_OBJECT_CLASS (gimp_image_chooser_parent_class)->dispose (object);
}

static void
gimp_image_chooser_finalize (GObject *object)
{
  GimpImageChooser *chooser = GIMP_IMAGE_CHOOSER (object);

  g_clear_pointer (&chooser->title, g_free);
  g_clear_pointer (&chooser->label, g_free);
  g_clear_pointer (&chooser->thumbnail, g_bytes_unref);

  G_OBJECT_CLASS (gimp_image_chooser_parent_class)->finalize (object);
}

static void
gimp_image_chooser_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *gvalue,
                                 GParamSpec   *pspec)
{
  GimpImageChooser *chooser = GIMP_IMAGE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_TITLE:
      chooser->title = g_value_dup_string (gvalue);
      break;

    case PROP_LABEL:
      chooser->label = g_value_dup_string (gvalue);
      break;

    case PROP_IMAGE:
      gimp_image_chooser_set_image (chooser, g_value_get_object (gvalue));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_chooser_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpImageChooser *chooser = GIMP_IMAGE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, chooser->title);
      break;

    case PROP_LABEL:
      g_value_set_string (value, chooser->label);
      break;

    case PROP_IMAGE:
      g_value_set_object (value, chooser->image);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_image_chooser_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @label: (nullable): Button label or %NULL for no label.
 * @image: (nullable): Initial image.
 *
 * Creates a new #GtkWidget that lets a user choose an image.
 *
 * When @image is %NULL, initial choice is from context.
 *
 * Returns: A [class@GimpUi.ImageChooser.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_image_chooser_new (const gchar *title,
                        const gchar *label,
                        GimpImage   *image)
{
  GtkWidget *chooser;

  g_return_val_if_fail (image == NULL, NULL);

  chooser = g_object_new (GIMP_TYPE_IMAGE_CHOOSER,
                          "title", title,
                          "label", label,
                          "image", image,
                          NULL);

  return chooser;
}

/**
 * gimp_image_chooser_get_image:
 * @chooser: A #GimpImageChooser
 *
 * Gets the currently selected image.
 *
 * Returns: (transfer none): an internal copy of the image which must not be freed.
 *
 * Since: 3.0
 */
GimpImage *
gimp_image_chooser_get_image (GimpImageChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_CHOOSER (chooser), NULL);

  return chooser->image;
}

/**
 * gimp_image_chooser_set_image:
 * @chooser: A #GimpImageChooser
 * @image: Image to set.
 *
 * Sets the currently selected image.
 * This will select the image in both the button and any chooser popup.
 *
 * Since: 3.0
 */
void
gimp_image_chooser_set_image (GimpImageChooser *chooser,
                              GimpImage        *image)
{
  g_return_if_fail (GIMP_IS_IMAGE_CHOOSER (chooser));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  chooser->image = image;

  if (chooser->callback)
    gimp_images_set_popup (chooser->callback, chooser->image);

  g_object_notify_by_pspec (G_OBJECT (chooser), image_button_props[PROP_IMAGE]);

  gimp_image_chooser_draw (chooser);
}

/**
 * gimp_image_chooser_get_label:
 * @widget: A [class@ImageChooser].
 *
 * Returns the label widget.
 *
 * Returns: (transfer none): the [class@Gtk.Widget] showing the label text.
 * Since: 3.0
 */
GtkWidget *
gimp_image_chooser_get_label (GimpImageChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_CHOOSER (chooser), NULL);

  return chooser->label_widget;
}


/*  private functions  */

static GimpValueArray *
gimp_temp_callback_run (GimpProcedure       *procedure,
                        GimpProcedureConfig *config,
                        GimpImageChooser    *chooser)
{
  GimpImage *image;
  gboolean   closing;

  g_object_get (config,
                "image",   &image,
                "closing", &closing,
                NULL);

  g_object_set (chooser, "image", image, NULL);

  if (closing)
    {
      g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                       (GSourceFunc) gimp_image_select_remove_after_run,
                       g_strdup (gimp_procedure_get_name (procedure)),
                       g_free);
      g_clear_pointer (&chooser->callback, g_free);
    }

  g_clear_object (&image);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_image_select_remove_after_run (const gchar *procedure_name)
{
  gimp_plug_in_remove_temp_procedure (gimp_get_plug_in (), procedure_name);

  return G_SOURCE_REMOVE;
}

static void
gimp_image_chooser_clicked (GimpImageChooser *chooser)
{
  if (chooser->callback)
    {
      /* Popup already created.  Calling setter raises the popup. */
      gimp_images_set_popup (chooser->callback, chooser->image);
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
      gimp_procedure_add_image_argument (callback_procedure, "image",
                                         "Image", "The selected image",
                                         TRUE, G_PARAM_READWRITE);
      gimp_procedure_add_boolean_argument (callback_procedure, "closing",
                                           "Closing", "If the dialog was closing",
                                           FALSE, G_PARAM_READWRITE);

      gimp_plug_in_add_temp_procedure (plug_in, callback_procedure);
      g_object_unref (callback_procedure);
      g_free (callback_name);

      if (gimp_images_popup (gimp_procedure_get_name (callback_procedure), chooser->title,
                             chooser->image, handle))
        {
          /* Allow callbacks to be watched */
          gimp_plug_in_persistent_enable (plug_in);

          chooser->callback = g_strdup (gimp_procedure_get_name (callback_procedure));
        }
      else
        {
          g_warning ("%s: failed to open remote image select dialog.", G_STRFUNC);
          gimp_plug_in_remove_temp_procedure (plug_in, gimp_procedure_get_name (callback_procedure));
          return;
        }
      gimp_images_set_popup (chooser->callback, chooser->image);
    }
}

static void
gimp_image_chooser_draw (GimpImageChooser *chooser)
{
  GimpPreviewArea *area = GIMP_PREVIEW_AREA (chooser->preview);
  GtkAllocation    allocation;
  gint             x = 0;
  gint             y = 0;

  gtk_widget_get_allocation (chooser->preview, &allocation);

  gimp_image_chooser_get_thumbnail (chooser, allocation.width, allocation.height);

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
gimp_image_chooser_get_thumbnail (GimpImageChooser *chooser,
                                  gint              width,
                                  gint              height)
{
  if (chooser->width  == width &&
      chooser->height == height)
    /* Let's assume image contents is not changing in a single run. */
    return;

  g_clear_pointer (&chooser->thumbnail, g_bytes_unref);

  if (chooser->image)
    {
      chooser->width  = BUTTON_SIZE;
      chooser->height = BUTTON_SIZE;

      chooser->thumbnail = gimp_image_get_thumbnail_data (chooser->image,
                                                          &chooser->width,
                                                          &chooser->height,
                                                          &chooser->bpp);
    }
}
