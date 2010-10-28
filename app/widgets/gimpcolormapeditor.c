/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimpmarshal.h"
#include "core/gimppalette.h"

#include "gimpcolormapeditor.h"
#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimpmenufactory.h"
#include "gimppaletteview.h"
#include "gimpuimanager.h"
#include "gimpviewrendererpalette.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define BORDER  6
#define EPSILON 1e-10

#define HAVE_COLORMAP(image) \
        (image != NULL && \
         gimp_image_get_base_type (image) == GIMP_INDEXED && \
         gimp_image_get_colormap (image) != NULL)


static void gimp_colormap_editor_docked_iface_init (GimpDockedInterface *face);

static void   gimp_colormap_editor_constructed     (GObject            *object);
static void   gimp_colormap_editor_dispose         (GObject            *object);
static void   gimp_colormap_editor_finalize        (GObject            *object);

static void   gimp_colormap_editor_unmap           (GtkWidget          *widget);

static void   gimp_colormap_editor_set_image       (GimpImageEditor    *editor,
                                                    GimpImage          *image);

static void   gimp_colormap_editor_set_context     (GimpDocked        *docked,
                                                    GimpContext       *context);

static PangoLayout *
              gimp_colormap_editor_create_layout   (GtkWidget          *widget);

static void   gimp_colormap_editor_update_entries  (GimpColormapEditor *editor);

static gboolean gimp_colormap_preview_draw         (GtkWidget          *widget,
                                                    cairo_t            *cr,
                                                    GimpColormapEditor *editor);

static void   gimp_colormap_editor_entry_clicked   (GimpPaletteView    *view,
                                                    GimpPaletteEntry   *entry,
                                                    GdkModifierType    state,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_editor_entry_selected  (GimpPaletteView    *view,
                                                    GimpPaletteEntry   *entry,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_editor_entry_activated (GimpPaletteView    *view,
                                                    GimpPaletteEntry   *entry,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_editor_entry_context   (GimpPaletteView    *view,
                                                    GimpPaletteEntry   *entry,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_editor_color_dropped   (GimpPaletteView    *view,
                                                    GimpPaletteEntry   *entry,
                                                    const GimpRGB      *color,
                                                    GimpColormapEditor *editor);

static void   gimp_colormap_adjustment_changed     (GtkAdjustment      *adjustment,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_hex_entry_changed      (GimpColorHexEntry  *entry,
                                                    GimpColormapEditor *editor);

static void   gimp_colormap_image_mode_changed     (GimpImage          *image,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_image_colormap_changed (GimpImage          *image,
                                                    gint                ncol,
                                                    GimpColormapEditor *editor);


G_DEFINE_TYPE_WITH_CODE (GimpColormapEditor, gimp_colormap_editor,
                         GIMP_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_colormap_editor_docked_iface_init))

#define parent_class gimp_colormap_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_colormap_editor_class_init (GimpColormapEditorClass* klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class       = GTK_WIDGET_CLASS (klass);
  GimpImageEditorClass *image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

  object_class->constructed     = gimp_colormap_editor_constructed;
  object_class->dispose         = gimp_colormap_editor_dispose;
  object_class->finalize        = gimp_colormap_editor_finalize;

  widget_class->unmap           = gimp_colormap_editor_unmap;

  image_editor_class->set_image = gimp_colormap_editor_set_image;
}

static void
gimp_colormap_editor_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_context = gimp_colormap_editor_set_context;
}

static void
gimp_colormap_editor_init (GimpColormapEditor *editor)
{
  GtkWidget *frame;
  GtkWidget *table;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  editor->view = gimp_view_new_full_by_types (NULL,
                                              GIMP_TYPE_PALETTE_VIEW,
                                              GIMP_TYPE_PALETTE,
                                              1, 1, 0,
                                              FALSE, TRUE, FALSE);
  gimp_view_set_expand (GIMP_VIEW (editor->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), editor->view);
  gtk_widget_show (editor->view);

  g_signal_connect (editor->view, "draw",
                    G_CALLBACK (gimp_colormap_preview_draw),
                    editor);

  g_signal_connect (editor->view, "entry-clicked",
                    G_CALLBACK (gimp_colormap_editor_entry_clicked),
                    editor);
  g_signal_connect (editor->view, "entry-selected",
                    G_CALLBACK (gimp_colormap_editor_entry_selected),
                    editor);
  g_signal_connect (editor->view, "entry-activated",
                    G_CALLBACK (gimp_colormap_editor_entry_activated),
                    editor);
  g_signal_connect (editor->view, "entry-context",
                    G_CALLBACK (gimp_colormap_editor_entry_context),
                    editor);
  g_signal_connect (editor->view, "color-dropped",
                    G_CALLBACK (gimp_colormap_editor_color_dropped),
                    editor);

  /*  Some helpful hints  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_end (GTK_BOX (editor), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  editor->index_adjustment = gtk_adjustment_new (0, 0, 0, 1, 10, 0);
  editor->index_spinbutton = gtk_spin_button_new (editor->index_adjustment,
                                                  1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (editor->index_spinbutton),
                               TRUE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Color index:"), 0.0, 0.5,
                             editor->index_spinbutton, 1, TRUE);

  g_signal_connect (editor->index_adjustment, "value-changed",
                    G_CALLBACK (gimp_colormap_adjustment_changed),
                    editor);

  editor->color_entry = gimp_color_hex_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (editor->color_entry), 12);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("HTML notation:"), 0.0, 0.5,
                             editor->color_entry, 1, TRUE);

  g_signal_connect (editor->color_entry, "color-changed",
                    G_CALLBACK (gimp_colormap_hex_entry_changed),
                    editor);
}

static void
gimp_colormap_editor_constructed (GObject *object)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (object);
  GdkModifierType     extend_mask;
  GdkModifierType     modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "colormap",
                                 "colormap-edit-color",
                                 NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "colormap",
                                 "colormap-add-color-from-fg",
                                 "colormap-add-color-from-bg",
                                 gimp_get_toggle_behavior_mask (),
                                 NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "colormap",
                                 "colormap-selection-replace",
                                 "colormap-selection-add",
                                 extend_mask,
                                 "colormap-selection-subtract",
                                 modify_mask,
                                 "colormap-selection-intersect",
                                 extend_mask | modify_mask,
                                 NULL);
}

static void
gimp_colormap_editor_dispose (GObject *object)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (object);

  if (editor->color_dialog)
    {
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_colormap_editor_finalize (GObject *object)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (object);

  if (editor->layout)
    {
      g_object_unref (editor->layout);
      editor->layout = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_colormap_editor_unmap (GtkWidget *widget)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (widget);

  if (editor->color_dialog)
    gtk_widget_hide (editor->color_dialog);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_colormap_editor_set_image (GimpImageEditor *image_editor,
                                GimpImage       *image)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (image_editor);

  if (image_editor->image)
    {
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_colormap_image_mode_changed,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_colormap_image_colormap_changed,
                                            editor);

      if (editor->color_dialog)
        gtk_widget_hide (editor->color_dialog);

      if (! HAVE_COLORMAP (image))
        {
          gtk_adjustment_set_upper (editor->index_adjustment, 0);

          if (gtk_widget_get_mapped (GTK_WIDGET (editor)))
            gimp_view_set_viewable (GIMP_VIEW (editor->view), NULL);
        }
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  editor->col_index = 0;

  if (image)
    {
      g_signal_connect (image, "mode-changed",
                        G_CALLBACK (gimp_colormap_image_mode_changed),
                        editor);
      g_signal_connect (image, "colormap-changed",
                        G_CALLBACK (gimp_colormap_image_colormap_changed),
                        editor);

      if (HAVE_COLORMAP (image))
        {
          gimp_view_set_viewable (GIMP_VIEW (editor->view),
                                  GIMP_VIEWABLE (gimp_image_get_colormap_palette (image)));

          gtk_adjustment_set_upper (editor->index_adjustment,
                                    gimp_image_get_colormap_size (image) - 1);
        }
    }

  gimp_colormap_editor_update_entries (editor);
}

static void
gimp_colormap_editor_set_context (GimpDocked  *docked,
                                  GimpContext *context)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  gimp_view_renderer_set_context (GIMP_VIEW (editor->view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
gimp_colormap_editor_new (GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (GIMP_TYPE_COLORMAP_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<Colormap>",
                       "ui-path",         "/colormap-popup",
                       NULL);
}

gint
gimp_colormap_editor_get_index (GimpColormapEditor *editor,
                                const GimpRGB      *search)
{
  GimpImage *image;
  gint       index;

  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), 01);

  image = GIMP_IMAGE_EDITOR (editor)->image;

  if (! HAVE_COLORMAP (image))
    return -1;

  index = editor->col_index;

  if (search)
    {
      GimpRGB temp;

      gimp_image_get_colormap_entry (image, index, &temp);

      if (gimp_rgb_distance (&temp, search) > EPSILON)
        {
          gint n_colors = gimp_image_get_colormap_size (image);
          gint i;

          for (i = 0; i < n_colors; i++)
            {
              gimp_image_get_colormap_entry (image, i, &temp);

              if (gimp_rgb_distance (&temp, search) < EPSILON)
                {
                  index = i;
                  break;
                }
            }
        }
    }

  return index;
}

gboolean
gimp_colormap_editor_set_index (GimpColormapEditor *editor,
                                gint                index,
                                GimpRGB            *color)
{
  GimpImage *image;
  gint       size;

  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), FALSE);

  image = GIMP_IMAGE_EDITOR (editor)->image;

  if (! HAVE_COLORMAP (image))
    return FALSE;

  size = gimp_image_get_colormap_size (image);

  if (size < 1)
    return FALSE;

  index = CLAMP (index, 0, size - 1);

  if (index != editor->col_index)
    {
      GimpPalette *palette = gimp_image_get_colormap_palette (image);

      editor->col_index = index;

      gimp_palette_view_select_entry (GIMP_PALETTE_VIEW (editor->view),
                                      gimp_palette_get_entry (palette, index));

      gimp_colormap_editor_update_entries (editor);
    }

  if (color)
    gimp_image_get_colormap_entry (GIMP_IMAGE_EDITOR (editor)->image,
                                   index, color);

  return TRUE;
}

gint
gimp_colormap_editor_max_index (GimpColormapEditor *editor)
{
  GimpImage *image;

  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), -1);

  image = GIMP_IMAGE_EDITOR (editor)->image;

  if (! HAVE_COLORMAP (image))
    return -1;

  return MAX (0, gimp_image_get_colormap_size (image) - 1);
}


/*  private functions  */

static PangoLayout *
gimp_colormap_editor_create_layout (GtkWidget *widget)
{
  PangoLayout    *layout;
  PangoAttrList  *attrs;
  PangoAttribute *attr;

  layout = gtk_widget_create_pango_layout (widget,
                                           _("Only indexed images have "
                                             "a colormap."));

  pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);

  attrs = pango_attr_list_new ();

  attr = pango_attr_style_new (PANGO_STYLE_ITALIC);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);

  return layout;
}

static gboolean
gimp_colormap_preview_draw (GtkWidget          *widget,
                            cairo_t            *cr,
                            GimpColormapEditor *editor)
{
  GimpImageEditor *image_editor = GIMP_IMAGE_EDITOR (editor);
  GtkStyle        *style;
  GtkAllocation    allocation;
  gint             width, height;
  gint             y;

  if (image_editor->image == NULL ||
      gimp_image_get_base_type (image_editor->image) == GIMP_INDEXED)
    return FALSE;

  style = gtk_widget_get_style (widget);
  gdk_cairo_set_source_color (cr, &style->fg[gtk_widget_get_state (widget)]);

  gtk_widget_get_allocation (widget, &allocation);

  if (! editor->layout)
    editor->layout = gimp_colormap_editor_create_layout (editor->view);

  pango_layout_set_width (editor->layout,
                          PANGO_SCALE * (allocation.width - 2 * BORDER));

  pango_layout_get_pixel_size (editor->layout, &width, &height);

  y = (allocation.height - height) / 2;

  cairo_move_to (cr, BORDER, MAX (y, 0));
  pango_cairo_show_layout (cr, editor->layout);

  return TRUE;
}

static void
gimp_colormap_editor_update_entries (GimpColormapEditor *editor)
{
  GimpImage *image = GIMP_IMAGE_EDITOR (editor)->image;

  if (! HAVE_COLORMAP (image) ||
      ! gimp_image_get_colormap_size (image))
    {
      gtk_widget_set_sensitive (editor->index_spinbutton, FALSE);
      gtk_widget_set_sensitive (editor->color_entry, FALSE);

      gtk_adjustment_set_value (editor->index_adjustment, 0);
      gtk_entry_set_text (GTK_ENTRY (editor->color_entry), "");
    }
  else
    {
      const guchar *colormap = gimp_image_get_colormap (image);
      const guchar *col;
      gchar        *string;

      gtk_adjustment_set_value (editor->index_adjustment, editor->col_index);

      col = colormap + editor->col_index * 3;

      string = g_strdup_printf ("%02x%02x%02x", col[0], col[1], col[2]);
      gtk_entry_set_text (GTK_ENTRY (editor->color_entry), string);
      g_free (string);

      gtk_widget_set_sensitive (editor->index_spinbutton, TRUE);
      gtk_widget_set_sensitive (editor->color_entry, TRUE);
    }
}

static void
gimp_colormap_editor_entry_clicked (GimpPaletteView    *view,
                                    GimpPaletteEntry   *entry,
                                    GdkModifierType     state,
                                    GimpColormapEditor *editor)
{
  GimpImageEditor *image_editor = GIMP_IMAGE_EDITOR (editor);

  gimp_colormap_editor_set_index (editor, entry->position, NULL);

  if (state & gimp_get_toggle_behavior_mask ())
    gimp_context_set_background (image_editor->context, &entry->color);
  else
    gimp_context_set_foreground (image_editor->context, &entry->color);
}

static void
gimp_colormap_editor_entry_selected (GimpPaletteView    *view,
                                     GimpPaletteEntry   *entry,
                                     GimpColormapEditor *editor)
{
  gint index = entry ? entry->position : 0;

  gimp_colormap_editor_set_index (editor, index, NULL);
}

static void
gimp_colormap_editor_entry_activated (GimpPaletteView    *view,
                                      GimpPaletteEntry   *entry,
                                      GimpColormapEditor *editor)
{
  gimp_colormap_editor_set_index (editor, entry->position, NULL);

  gimp_ui_manager_activate_action (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                                   "colormap",
                                   "colormap-edit-color");
}

static void
gimp_colormap_editor_entry_context (GimpPaletteView    *view,
                                    GimpPaletteEntry   *entry,
                                    GimpColormapEditor *editor)
{
  gimp_colormap_editor_set_index (editor, entry->position, NULL);

  gimp_editor_popup_menu (GIMP_EDITOR (editor), NULL, NULL);
}

static void
gimp_colormap_editor_color_dropped (GimpPaletteView    *view,
                                    GimpPaletteEntry   *entry,
                                    const GimpRGB      *color,
                                    GimpColormapEditor *editor)
{
}

static void
gimp_colormap_adjustment_changed (GtkAdjustment      *adjustment,
                                  GimpColormapEditor *editor)
{
  GimpImage *image = GIMP_IMAGE_EDITOR (editor)->image;

  if (HAVE_COLORMAP (image))
    {
      gint index = ROUND (gtk_adjustment_get_value (adjustment));

      gimp_colormap_editor_set_index (editor, index, NULL);

      gimp_colormap_editor_update_entries (editor);
    }
}

static void
gimp_colormap_hex_entry_changed (GimpColorHexEntry  *entry,
                                 GimpColormapEditor *editor)
{
  GimpImage *image = GIMP_IMAGE_EDITOR (editor)->image;

  if (image)
    {
      GimpRGB color;

      gimp_color_hex_entry_get_color (entry, &color);

      gimp_image_set_colormap_entry (image, editor->col_index, &color, TRUE);
      gimp_image_flush (image);
    }
}

static void
gimp_colormap_image_mode_changed (GimpImage          *image,
                                  GimpColormapEditor *editor)
{
  if (editor->color_dialog)
    gtk_widget_hide (editor->color_dialog);

  gimp_colormap_image_colormap_changed (image, -1, editor);
}

static void
gimp_colormap_image_colormap_changed (GimpImage          *image,
                                      gint                ncol,
                                      GimpColormapEditor *editor)
{
  if (HAVE_COLORMAP (image))
    {
      gimp_view_set_viewable (GIMP_VIEW (editor->view),
                              GIMP_VIEWABLE (gimp_image_get_colormap_palette (image)));

      gtk_adjustment_set_upper (editor->index_adjustment,
                                gimp_image_get_colormap_size (image) - 1);
    }
  else
    {
      gimp_view_set_viewable (GIMP_VIEW (editor->view), NULL);
    }

  if (ncol == editor->col_index || ncol == -1)
    gimp_colormap_editor_update_entries (editor);
}
