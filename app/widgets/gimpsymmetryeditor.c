/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsymmetryeditor.c
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-symmetry.h"
#include "core/gimpsymmetry.h"

#include "gimpdocked.h"
#include "gimpmenufactory.h"
#include "gimppropwidgets.h"
#include "gimpspinscale.h"
#include "gimpsymmetryeditor.h"

#include "gimp-intl.h"

enum
{
  PROP_0,
  PROP_GIMP,
};

struct _GimpSymmetryEditorPrivate
{
  Gimp            *gimp;
  GimpImage       *image;

  GtkWidget       *menu;
  GtkWidget       *options_frame;
};

static void        gimp_symmetry_editor_docked_iface_init (GimpDockedInterface   *iface);

/* Signal handlers on the GObject. */
static void        gimp_symmetry_editor_constructed       (GObject               *object);
static void        gimp_symmetry_editor_dispose           (GObject               *object);
static void        gimp_symmetry_editor_set_property      (GObject               *object,
                                                           guint                  property_id,
                                                           const GValue          *value,
                                                           GParamSpec            *pspec);
static void        gimp_symmetry_editor_get_property      (GObject               *object,
                                                           guint                  property_id,
                                                           GValue                *value,
                                                           GParamSpec            *pspec);

/* Signal handlers on the context. */
static void        gimp_symmetry_editor_image_changed     (GimpContext           *context,
                                                           GimpImage             *image,
                                                           GimpSymmetryEditor    *editor);

/* Signal handlers on the contextual image. */
static void        gimp_symmetry_editor_symmetry_notify   (GimpImage             *image,
                                                           GParamSpec            *pspec,
                                                           GimpSymmetryEditor    *editor);

/* Signal handlers on the symmetry. */
static void        gimp_symmetry_editor_symmetry_updated  (GimpSymmetry          *symmetry,
                                                           GimpImage             *image,
                                                           GimpSymmetryEditor    *editor);

/* Private functions. */
static void        gimp_symmetry_editor_set_options       (GimpSymmetryEditor    *editor,
                                                           GimpSymmetry          *symmetry);


G_DEFINE_TYPE_WITH_CODE (GimpSymmetryEditor, gimp_symmetry_editor,
                         GIMP_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_symmetry_editor_docked_iface_init))

#define parent_class gimp_symmetry_editor_parent_class

static void
gimp_symmetry_editor_class_init (GimpSymmetryEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_symmetry_editor_constructed;
  object_class->dispose      = gimp_symmetry_editor_dispose;
  object_class->set_property = gimp_symmetry_editor_set_property;
  object_class->get_property = gimp_symmetry_editor_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpSymmetryEditorPrivate));
}

static void
gimp_symmetry_editor_docked_iface_init (GimpDockedInterface *docked_iface)
{
}

static void
gimp_symmetry_editor_init (GimpSymmetryEditor *editor)
{
  GtkScrolledWindow *scrolled_window;

  editor->p = G_TYPE_INSTANCE_GET_PRIVATE (editor,
                                           GIMP_TYPE_SYMMETRY_EDITOR,
                                           GimpSymmetryEditorPrivate);

  gtk_widget_set_size_request (GTK_WIDGET (editor), -1, 200);

  /* Scrolled window to keep the dock size reasonable. */
  scrolled_window = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));

  gtk_scrolled_window_set_policy (scrolled_window,
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (editor),
                      GTK_WIDGET (scrolled_window),
                      TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (scrolled_window));

  /* A frame to hold the symmetry options. */
  editor->p->options_frame = gimp_frame_new ("");
  gtk_scrolled_window_add_with_viewport (scrolled_window,
                                         editor->p->options_frame);
}

static void
gimp_symmetry_editor_constructed (GObject *object)
{
  GimpSymmetryEditor *editor = GIMP_SYMMETRY_EDITOR (object);
  GimpContext           *user_context;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  user_context = gimp_get_user_context (editor->p->gimp);

  g_signal_connect_object (user_context, "image-changed",
                           G_CALLBACK (gimp_symmetry_editor_image_changed),
                           editor,
                           0);

  gimp_symmetry_editor_image_changed (user_context,
                                      gimp_context_get_image (user_context),
                                      editor);
}

static void
gimp_symmetry_editor_dispose (GObject *object)
{
  GimpSymmetryEditor *editor = GIMP_SYMMETRY_EDITOR (object);

  g_clear_object (&editor->p->image);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_symmetry_editor_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpSymmetryEditor *editor = GIMP_SYMMETRY_EDITOR (object);

  switch (property_id)
    {
    case PROP_GIMP:
      editor->p->gimp = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_symmetry_editor_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpSymmetryEditor *editor = GIMP_SYMMETRY_EDITOR (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, editor->p->gimp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_symmetry_editor_image_changed (GimpContext        *context,
                                    GimpImage          *image,
                                    GimpSymmetryEditor *editor)
{
  GimpGuiConfig *guiconfig;

  if (image == editor->p->image)
    return;

  guiconfig = GIMP_GUI_CONFIG (editor->p->gimp->config);

  /* Disconnect and unref the previous image. */
  if (editor->p->image)
    {
      g_signal_handlers_disconnect_by_func (editor->p->image,
                                            G_CALLBACK (gimp_symmetry_editor_symmetry_notify),
                                            editor);
      g_object_unref (editor->p->image);
      editor->p->image = NULL;
    }

  /* Destroy the previous menu. */
  if (editor->p->menu)
    gtk_widget_destroy (editor->p->menu);
  editor->p->menu = NULL;

  if (image && guiconfig->playground_symmetry)
    {
      GtkListStore *store;
      GtkTreeIter   iter;
      GList        *syms;
      GList        *sym_iter;
      GimpSymmetry *symmetry;

      store = gimp_int_store_new ();

      /* The menu of available symmetries. */
      syms = gimp_image_symmetry_list ();
      for (sym_iter = syms; sym_iter; sym_iter = g_list_next (sym_iter))
        {
          GimpSymmetryClass *klass;
          GType              type;

          type = (GType) sym_iter->data;
          klass = g_type_class_ref (type);

          gtk_list_store_prepend (store, &iter);
          gtk_list_store_set (store, &iter,
                              GIMP_INT_STORE_LABEL,
                              klass->label,
                              GIMP_INT_STORE_USER_DATA,
                              sym_iter->data,
                              -1);
          g_type_class_unref (klass);
        }
      g_list_free (syms);

      gtk_list_store_prepend (store, &iter);
      gtk_list_store_set (store, &iter,
                          GIMP_INT_STORE_LABEL, _("None"),
                          GIMP_INT_STORE_USER_DATA, GIMP_TYPE_SYMMETRY,
                          -1);
      editor->p->menu = gimp_prop_pointer_combo_box_new (G_OBJECT (image),
                                                         "symmetry",
                                                         GIMP_INT_STORE (store));
      g_object_unref (store);

      gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (editor->p->menu),
                                    _("Symmetry"));
      g_object_set (editor->p->menu, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

      gtk_box_pack_start (GTK_BOX (editor), editor->p->menu,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (editor), editor->p->menu, 0);
      gtk_widget_show (editor->p->menu);

      /* Connect to symmetry change. */
      g_signal_connect (image, "notify::symmetry",
                        G_CALLBACK (gimp_symmetry_editor_symmetry_notify),
                        editor);

      /* Update the symmetry options. */
      symmetry = gimp_image_symmetry_selected (image);
      gimp_symmetry_editor_set_options (editor, symmetry);
      editor->p->image = g_object_ref (image);
    }
  else if (! guiconfig->playground_symmetry)
    {
      GtkWidget *label;

      /* Display a text when the feature is disabled. */
      label = gtk_label_new (_("Symmetry Painting is disabled.\n"
                               "You can enable the feature in the "
                               "\"Experimental Playground\" section of \"Preferences\"."));
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                 -1);
      gtk_container_add (GTK_CONTAINER (editor->p->options_frame), label);
      gtk_widget_show (label);
      gtk_widget_show (editor->p->options_frame);
    }
}

static void
gimp_symmetry_editor_symmetry_notify (GimpImage           *image,
                                      GParamSpec          *pspec,
                                      GimpSymmetryEditor  *editor)
{
  GimpSymmetry *symmetry = NULL;

  if (image &&
      (symmetry = gimp_image_symmetry_selected (image)))
    {
      g_signal_connect (symmetry, "update-ui",
                        G_CALLBACK (gimp_symmetry_editor_symmetry_updated),
                        editor);
    }

  gimp_symmetry_editor_set_options (editor, symmetry);
}

static void
gimp_symmetry_editor_symmetry_updated (GimpSymmetry       *symmetry,
                                       GimpImage          *image,
                                       GimpSymmetryEditor *editor)
{
  GimpContext *context;

  g_return_if_fail (GIMP_IS_SYMMETRY (symmetry));

  context = gimp_get_user_context (editor->p->gimp);
  if (image != context->image ||
      symmetry != gimp_image_symmetry_selected (image))
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (symmetry),
                                            gimp_symmetry_editor_symmetry_updated,
                                            editor);
      return;
    }

  gimp_symmetry_editor_set_options (editor, symmetry);
}

/*  private functions  */

static void
gimp_symmetry_editor_set_options (GimpSymmetryEditor *editor,
                                  GimpSymmetry       *symmetry)
{
  GimpSymmetryClass  *klass;
  GtkWidget          *frame;
  GtkWidget          *vbox;
  GParamSpec        **specs;
  gint                n_properties;
  gint                i;

  frame = editor->p->options_frame;

  /* Clean the old frame */
  gtk_widget_hide (frame);
  gtk_container_foreach (GTK_CONTAINER (frame),
                         (GtkCallback) gtk_widget_destroy, NULL);

  if (! symmetry || symmetry->type == GIMP_TYPE_SYMMETRY)
    return;

  klass = g_type_class_ref (symmetry->type);
  gtk_frame_set_label (GTK_FRAME (frame),
                       klass->label);
  g_type_class_unref (klass);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  specs = gimp_symmetry_get_settings (symmetry, &n_properties);

  for (i = 0; i < n_properties; i++)
    {
      GParamSpec  *spec;
      const gchar *name;
      const gchar *blurb;

      if (specs[i] == NULL)
        {
          GtkWidget *separator;

          separator = gtk_hseparator_new ();
          gtk_box_pack_start (GTK_BOX (vbox), separator,
                              FALSE, FALSE, 0);
          gtk_widget_show (separator);
          continue;
        }
      spec = G_PARAM_SPEC (specs[i]);

      name = g_param_spec_get_name (spec);
      blurb = g_param_spec_get_blurb (spec);

      switch (spec->value_type)
        {
        case G_TYPE_BOOLEAN:
            {
              GtkWidget *checkbox;

              checkbox = gimp_prop_check_button_new (G_OBJECT (symmetry),
                                                     name,
                                                     blurb);
              gtk_box_pack_start (GTK_BOX (vbox), checkbox,
                                  FALSE, FALSE, 0);
              gtk_widget_show (checkbox);
            }
          break;
        case G_TYPE_DOUBLE:
        case G_TYPE_INT:
        case G_TYPE_UINT:
            {
              GtkWidget *scale;
              gdouble    minimum;
              gdouble    maximum;

              if (spec->value_type == G_TYPE_DOUBLE)
                {
                  minimum = G_PARAM_SPEC_DOUBLE (spec)->minimum;
                  maximum = G_PARAM_SPEC_DOUBLE (spec)->maximum;
                }
              else if (spec->value_type == G_TYPE_INT)
                {
                  minimum = G_PARAM_SPEC_INT (spec)->minimum;
                  maximum = G_PARAM_SPEC_INT (spec)->maximum;
                }
              else
                {
                  minimum = G_PARAM_SPEC_UINT (spec)->minimum;
                  maximum = G_PARAM_SPEC_UINT (spec)->maximum;
                }

              scale = gimp_prop_spin_scale_new (G_OBJECT (symmetry),
                                                name, blurb,
                                                1.0, 10.0, 1);
              gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (scale),
                                                minimum,
                                                maximum);
              gtk_box_pack_start (GTK_BOX (vbox), scale, TRUE, TRUE, 0);
              gtk_widget_show (scale);
            }
          break;
        default:
          /* Type of parameter we haven't handled yet. */
          continue;
        }
    }

  g_free (specs);

  /* Finally show the frame. */
  gtk_widget_show (frame);
}

/*  public functions  */

GtkWidget *
gimp_symmetry_editor_new (Gimp            *gimp,
                          GimpImage       *image,
                          GimpMenuFactory *menu_factory)
{
  GimpSymmetryEditor *editor;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (image == NULL || GIMP_IS_IMAGE (image), NULL);

  editor = g_object_new (GIMP_TYPE_SYMMETRY_EDITOR,
                         "gimp",            gimp,
                         "menu-factory",    menu_factory,
                         NULL);

  return GTK_WIDGET (editor);
}
