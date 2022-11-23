/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-colormap.h"
#include "core/ligmamarshal.h"
#include "core/ligmapalette.h"
#include "core/ligmaprojection.h"

#include "ligmacolordialog.h"
#include "ligmacolormapselection.h"
#include "ligmadialogfactory.h"
#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmamenufactory.h"
#include "ligmapaletteview.h"
#include "ligmauimanager.h"
#include "ligmaviewrendererpalette.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT
};

enum
{
  COLOR_CLICKED,
  COLOR_ACTIVATED,
  LAST_SIGNAL
};

#define BORDER      6
#define RGB_EPSILON 1e-6

#define HAVE_COLORMAP(image) \
        (image != NULL && \
         ligma_image_get_base_type (image) == LIGMA_INDEXED && \
         ligma_image_get_colormap_palette (image) != NULL)


static void   ligma_colormap_selection_set_property    (GObject               *object,
                                                       guint                  property_id,
                                                       const GValue          *value,
                                                       GParamSpec            *pspec);
static void   ligma_colormap_selection_get_property    (GObject               *object,
                                                       guint                  property_id,
                                                       GValue                *value,
                                                       GParamSpec            *pspec);
static void   ligma_colormap_selection_dispose         (GObject               *object);
static void   ligma_colormap_selection_finalize        (GObject               *object);

static void   ligma_colormap_selection_unmap           (GtkWidget             *widget);

static PangoLayout *
              ligma_colormap_selection_create_layout   (GtkWidget             *widget);

static void   ligma_colormap_selection_update_entries  (LigmaColormapSelection *selection);

static gboolean
              ligma_colormap_selection_preview_draw    (GtkWidget             *widget,
                                                       cairo_t               *cr,
                                                       LigmaColormapSelection *selection);

static void   ligma_colormap_selection_entry_clicked   (LigmaPaletteView       *view,
                                                       LigmaPaletteEntry      *entry,
                                                       GdkModifierType       state,
                                                       LigmaColormapSelection *selection);
static void   ligma_colormap_selection_entry_selected  (LigmaPaletteView       *view,
                                                       LigmaPaletteEntry      *entry,
                                                       LigmaColormapSelection *selection);
static void   ligma_colormap_selection_entry_activated (LigmaPaletteView       *view,
                                                       LigmaPaletteEntry      *entry,
                                                       LigmaColormapSelection *selection);
static void   ligma_colormap_selection_color_dropped   (LigmaPaletteView       *view,
                                                       LigmaPaletteEntry      *entry,
                                                       const LigmaRGB         *color,
                                                       LigmaColormapSelection *selection);

static void   ligma_colormap_adjustment_changed        (GtkAdjustment         *adjustment,
                                                       LigmaColormapSelection *selection);
static void   ligma_colormap_hex_entry_changed         (LigmaColorHexEntry     *entry,
                                                       LigmaColormapSelection *selection);

static void   ligma_colormap_selection_set_context     (LigmaColormapSelection *selection,
                                                       LigmaContext           *context);
static void   ligma_colormap_selection_image_changed   (LigmaColormapSelection *selection,
                                                       LigmaImage             *image);
static void   ligma_colormap_selection_set_palette     (LigmaColormapSelection *selection);

G_DEFINE_TYPE (LigmaColormapSelection, ligma_colormap_selection, GTK_TYPE_BOX)

#define parent_class ligma_colormap_selection_parent_class

static guint signals[LAST_SIGNAL] = { 0 };

static void
ligma_colormap_selection_class_init (LigmaColormapSelectionClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  signals[COLOR_CLICKED] =
    g_signal_new ("color-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColormapSelectionClass, color_clicked),
                  NULL, NULL,
                  ligma_marshal_VOID__POINTER_ENUM,
                  G_TYPE_NONE, 2,
                  G_TYPE_POINTER,
                  GDK_TYPE_MODIFIER_TYPE);
  signals[COLOR_ACTIVATED] =
    g_signal_new ("color-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColormapSelectionClass, color_activated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  object_class->set_property    = ligma_colormap_selection_set_property;
  object_class->get_property    = ligma_colormap_selection_get_property;
  object_class->dispose         = ligma_colormap_selection_dispose;
  object_class->finalize        = ligma_colormap_selection_finalize;

  widget_class->unmap           = ligma_colormap_selection_unmap;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_colormap_selection_init (LigmaColormapSelection *selection)
{
  GtkWidget *frame;
  GtkWidget *grid;

  gtk_box_set_homogeneous (GTK_BOX (selection), FALSE);

  /* Main colormap frame. */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (selection), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  selection->view = ligma_view_new_full_by_types (NULL,
                                                 LIGMA_TYPE_PALETTE_VIEW,
                                                 LIGMA_TYPE_PALETTE,
                                                 1, 1, 0,
                                                 FALSE, TRUE, FALSE);
  ligma_view_set_expand (LIGMA_VIEW (selection->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), selection->view);
  gtk_widget_show (selection->view);

  g_signal_connect (selection->view, "draw",
                    G_CALLBACK (ligma_colormap_selection_preview_draw),
                    selection);

  g_signal_connect (selection->view, "entry-clicked",
                    G_CALLBACK (ligma_colormap_selection_entry_clicked),
                    selection);
  g_signal_connect (selection->view, "entry-selected",
                    G_CALLBACK (ligma_colormap_selection_entry_selected),
                    selection);
  g_signal_connect (selection->view, "entry-activated",
                    G_CALLBACK (ligma_colormap_selection_entry_activated),
                    selection);
  g_signal_connect (selection->view, "color-dropped",
                    G_CALLBACK (ligma_colormap_selection_color_dropped),
                    selection);

  /* Bottom horizontal box for additional widgets. */
  selection->right_vbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_set_homogeneous (GTK_BOX (selection->right_vbox), TRUE);
  gtk_box_pack_end (GTK_BOX (selection), selection->right_vbox,
                    FALSE, FALSE, 0);
  gtk_widget_show (selection->right_vbox);

  /*  Some helpful hints  */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_end (GTK_BOX (selection), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  selection->index_adjustment = (GtkAdjustment *)
    gtk_adjustment_new (0, 0, 0, 1, 10, 0);
  selection->index_spinbutton = ligma_spin_button_new (selection->index_adjustment,
                                                      1.0, 0);
  gtk_widget_set_halign (selection->index_spinbutton, GTK_ALIGN_START);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (selection->index_spinbutton),
                               TRUE);

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Color index:"), 0.0, 0.5,
                            selection->index_spinbutton, 1);

  g_signal_connect (selection->index_adjustment, "value-changed",
                    G_CALLBACK (ligma_colormap_adjustment_changed),
                    selection);

  selection->color_entry = ligma_color_hex_entry_new ();
  gtk_widget_set_halign (selection->color_entry, GTK_ALIGN_START);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("HTML notation:"), 0.0, 0.5,
                            selection->color_entry, 1);

  g_signal_connect (selection->color_entry, "color-changed",
                    G_CALLBACK (ligma_colormap_hex_entry_changed),
                    selection);
}

static void
ligma_colormap_selection_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  LigmaColormapSelection *selection = LIGMA_COLORMAP_SELECTION (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      ligma_colormap_selection_set_context (selection, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_colormap_selection_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  LigmaColormapSelection *selection = LIGMA_COLORMAP_SELECTION (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, selection->context);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_colormap_selection_dispose (GObject *object)
{
  LigmaColormapSelection *selection = LIGMA_COLORMAP_SELECTION (object);

  g_clear_pointer (&selection->color_dialog, gtk_widget_destroy);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_colormap_selection_finalize (GObject *object)
{
  LigmaColormapSelection *selection = LIGMA_COLORMAP_SELECTION (object);

  if (selection->context)
    {
      g_signal_handlers_disconnect_by_func (selection->context,
                                            gtk_widget_queue_draw,
                                            selection);
      g_signal_handlers_disconnect_by_func (selection->context,
                                            G_CALLBACK (ligma_colormap_selection_image_changed),
                                            selection);
    }
  if (selection->active_image)
    {
      g_signal_handlers_disconnect_by_func (selection->active_image,
                                            G_CALLBACK (gtk_widget_queue_draw),
                                            selection);
      g_signal_handlers_disconnect_by_func (selection->active_image,
                                            G_CALLBACK (ligma_colormap_selection_set_palette),
                                            selection);
    }
  if (selection->active_palette)
    {
      g_signal_handlers_disconnect_by_func (selection->active_palette,
                                            G_CALLBACK (gtk_widget_queue_draw),
                                            selection);
    }

  g_clear_object (&selection->layout);
  g_clear_object (&selection->context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_colormap_selection_unmap (GtkWidget *widget)
{
  LigmaColormapSelection *selection = LIGMA_COLORMAP_SELECTION (widget);

  if (selection->color_dialog)
    gtk_widget_hide (selection->color_dialog);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

/*  public functions  */

GtkWidget *
ligma_colormap_selection_new (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return g_object_new (LIGMA_TYPE_COLORMAP_SELECTION,
                       "context",     context,
                       "orientation", GTK_ORIENTATION_VERTICAL,
                       NULL);
}

gint
ligma_colormap_selection_get_index (LigmaColormapSelection *selection,
                                   const LigmaRGB         *search)
{
  LigmaImage *image;
  gint       index;

  g_return_val_if_fail (LIGMA_IS_COLORMAP_SELECTION (selection), 0);

  image = ligma_context_get_image (selection->context);

  if (! HAVE_COLORMAP (image))
    return -1;

  index = selection->col_index;

  if (search)
    {
      LigmaRGB temp;

      ligma_image_get_colormap_entry (image, index, &temp);

      if (ligma_rgb_distance (&temp, search) > RGB_EPSILON)
        {
          gint n_colors = ligma_image_get_colormap_size (image);
          gint i;

          for (i = 0; i < n_colors; i++)
            {
              ligma_image_get_colormap_entry (image, i, &temp);

              if (ligma_rgb_distance (&temp, search) < RGB_EPSILON)
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
ligma_colormap_selection_set_index (LigmaColormapSelection *selection,
                                   gint                   index,
                                   LigmaRGB               *color)
{
  LigmaImage *image;
  gint       size;

  g_return_val_if_fail (LIGMA_IS_COLORMAP_SELECTION (selection), FALSE);

  image = ligma_context_get_image (selection->context);

  if (! HAVE_COLORMAP (image))
    return FALSE;

  size = ligma_image_get_colormap_size (image);

  if (size < 1)
    return FALSE;

  index = CLAMP (index, 0, size - 1);

  if (index != selection->col_index)
    {
      LigmaPalette *palette = ligma_image_get_colormap_palette (image);

      selection->col_index = index;

      ligma_palette_view_select_entry (LIGMA_PALETTE_VIEW (selection->view),
                                      ligma_palette_get_entry (palette, index));

      ligma_colormap_selection_update_entries (selection);
    }

  if (color)
    ligma_image_get_colormap_entry (image, index, color);

  return TRUE;
}

gint
ligma_colormap_selection_max_index (LigmaColormapSelection *selection)
{
  LigmaImage *image;

  g_return_val_if_fail (LIGMA_IS_COLORMAP_SELECTION (selection), -1);

  image = ligma_context_get_image (selection->context);

  if (! HAVE_COLORMAP (image))
    return -1;

  return MAX (0, ligma_image_get_colormap_size (image) - 1);
}

LigmaPaletteEntry *
ligma_colormap_selection_get_selected_entry (LigmaColormapSelection *selection)
{
  g_return_val_if_fail (LIGMA_IS_COLORMAP_SELECTION (selection), NULL);

  return ligma_palette_view_get_selected_entry (LIGMA_PALETTE_VIEW (selection->view));
}

void
ligma_colormap_selection_get_entry_rect (LigmaColormapSelection *selection,
                                        LigmaPaletteEntry      *entry,
                                        GdkRectangle          *rect)
{
  GtkAllocation allocation;

  g_return_if_fail (LIGMA_IS_COLORMAP_SELECTION (selection));
  g_return_if_fail (entry);
  g_return_if_fail (rect);

  ligma_palette_view_get_entry_rect (LIGMA_PALETTE_VIEW (selection->view),
                                    entry, rect);
  gtk_widget_get_allocation (GTK_WIDGET (selection), &allocation);
  /* rect->x += allocation.x; */
  /* rect->y += allocation.y; */
}


/*  private functions  */

static PangoLayout *
ligma_colormap_selection_create_layout (GtkWidget *widget)
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
ligma_colormap_selection_preview_draw (GtkWidget             *widget,
                                      cairo_t               *cr,
                                      LigmaColormapSelection *selection)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  LigmaImage       *image;
  GtkAllocation    allocation;
  GdkRGBA          color;
  gint             width, height;
  gint             y;

  image = ligma_context_get_image (selection->context);

  if (image == NULL || ligma_image_get_base_type (image) == LIGMA_INDEXED)
    return FALSE;

  gtk_style_context_get_color (style, gtk_widget_get_state_flags (widget),
                               &color);
  gdk_cairo_set_source_rgba (cr, &color);

  gtk_widget_get_allocation (widget, &allocation);

  if (! selection->layout)
    selection->layout = ligma_colormap_selection_create_layout (selection->view);

  pango_layout_set_width (selection->layout,
                          PANGO_SCALE * (allocation.width - 2 * BORDER));

  pango_layout_get_pixel_size (selection->layout, &width, &height);

  y = (allocation.height - height) / 2;

  cairo_move_to (cr, BORDER, MAX (y, 0));
  pango_cairo_show_layout (cr, selection->layout);

  return TRUE;
}

static void
ligma_colormap_selection_update_entries (LigmaColormapSelection *selection)
{
  LigmaImage *image = ligma_context_get_image (selection->context);

  if (! HAVE_COLORMAP (image) ||
      ! ligma_image_get_colormap_size (image))
    {
      gtk_widget_set_sensitive (selection->index_spinbutton, FALSE);
      gtk_widget_set_sensitive (selection->color_entry, FALSE);

      gtk_adjustment_set_value (selection->index_adjustment, 0);
      gtk_entry_set_text (GTK_ENTRY (selection->color_entry), "");
    }
  else
    {
      LigmaRGB  color;
      guchar   r, g, b;
      gchar   *string;

      gtk_adjustment_set_value (selection->index_adjustment,
                                selection->col_index);
      ligma_image_get_colormap_entry (image, selection->col_index, &color);
      ligma_rgb_get_uchar (&color, &r, &g, &b);

      string = g_strdup_printf ("%02x%02x%02x", r, g, b);
      gtk_entry_set_text (GTK_ENTRY (selection->color_entry), string);
      g_free (string);

      gtk_widget_set_sensitive (selection->index_spinbutton, TRUE);
      gtk_widget_set_sensitive (selection->color_entry, TRUE);
    }
}

static void
ligma_colormap_selection_entry_clicked (LigmaPaletteView       *view,
                                       LigmaPaletteEntry      *entry,
                                       GdkModifierType        state,
                                       LigmaColormapSelection *selection)
{
  LigmaPalette *palette;
  gint         index;

  palette = ligma_image_get_colormap_palette (selection->active_image);
  index = ligma_palette_get_entry_position (palette, entry);
  ligma_colormap_selection_set_index (selection, index, NULL);

  g_signal_emit (selection, signals[COLOR_CLICKED], 0, entry, state);
}

static void
ligma_colormap_selection_entry_selected (LigmaPaletteView       *view,
                                        LigmaPaletteEntry      *entry,
                                        LigmaColormapSelection *selection)
{
  LigmaPalette *palette;
  gint         index = 0;

  palette = ligma_image_get_colormap_palette (selection->active_image);
  if (entry)
    index = ligma_palette_get_entry_position (palette, entry);

  ligma_colormap_selection_set_index (selection, index, NULL);
}

static void
ligma_colormap_selection_entry_activated (LigmaPaletteView       *view,
                                         LigmaPaletteEntry      *entry,
                                         LigmaColormapSelection *selection)
{
  LigmaPalette *palette;
  gint         index;

  palette = ligma_image_get_colormap_palette (selection->active_image);
  index = ligma_palette_get_entry_position (palette, entry);
  ligma_colormap_selection_set_index (selection, index, NULL);

  g_signal_emit (selection, signals[COLOR_ACTIVATED], 0, entry);
}

static void
ligma_colormap_selection_color_dropped (LigmaPaletteView       *view,
                                       LigmaPaletteEntry      *entry,
                                       const LigmaRGB         *color,
                                       LigmaColormapSelection *selection)
{
}

static void
ligma_colormap_adjustment_changed (GtkAdjustment         *adjustment,
                                  LigmaColormapSelection *selection)
{
  LigmaImage *image = ligma_context_get_image (selection->context);

  if (HAVE_COLORMAP (image))
    {
      gint index = ROUND (gtk_adjustment_get_value (adjustment));

      ligma_colormap_selection_set_index (selection, index, NULL);

      ligma_colormap_selection_update_entries (selection);
    }
}

static void
ligma_colormap_hex_entry_changed (LigmaColorHexEntry     *entry,
                                 LigmaColormapSelection *selection)
{
  LigmaImage *image = ligma_context_get_image (selection->context);

  if (image)
    {
      LigmaRGB color;

      ligma_color_hex_entry_get_color (entry, &color);

      ligma_image_set_colormap_entry (image, selection->col_index, &color, TRUE);
      ligma_image_flush (image);
    }
}

static void
ligma_colormap_selection_set_context (LigmaColormapSelection *selection,
                                     LigmaContext           *context)
{
  g_return_if_fail (LIGMA_IS_COLORMAP_SELECTION (selection));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  if (context != selection->context)
    {
      if (selection->context)
        {
          g_signal_handlers_disconnect_by_func (selection->context,
                                                gtk_widget_queue_draw,
                                                selection);
          g_signal_handlers_disconnect_by_func (selection->context,
                                                G_CALLBACK (ligma_colormap_selection_image_changed),
                                                selection);
          g_object_unref (selection->context);
        }

      selection->context = context;

      if (context)
        {
          g_object_ref (context);

          g_signal_connect_swapped (context, "foreground-changed",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    selection);
          g_signal_connect_swapped (context, "background-changed",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    selection);
          g_signal_connect_swapped (context, "image-changed",
                                    G_CALLBACK (ligma_colormap_selection_image_changed),
                                    selection);
          ligma_colormap_selection_image_changed (selection, ligma_context_get_image (context));
        }

      ligma_view_renderer_set_context (LIGMA_VIEW (selection->view)->renderer,
                                      context);
      g_object_notify (G_OBJECT (selection), "context");
    }
}

static void
ligma_colormap_selection_image_changed (LigmaColormapSelection *selection,
                                       LigmaImage             *image)
{
  if (selection->active_image)
    {
      g_object_remove_weak_pointer (G_OBJECT (selection->active_image),
                                    (gpointer) &selection->active_image);
      g_signal_handlers_disconnect_by_func (selection->active_image,
                                            G_CALLBACK (gtk_widget_queue_draw),
                                            selection);
      g_signal_handlers_disconnect_by_func (selection->active_image,
                                            G_CALLBACK (ligma_colormap_selection_set_palette),
                                            selection);
      if (ligma_image_get_base_type (selection->active_image) == LIGMA_INDEXED)
        {
          LigmaPalette *palette;

          palette = ligma_image_get_colormap_palette (selection->active_image);
          g_signal_handlers_disconnect_by_func (palette,
                                                G_CALLBACK (gtk_widget_queue_draw),
                                                selection);
        }

      if (selection->color_dialog)
        gtk_widget_hide (selection->color_dialog);

      if (! HAVE_COLORMAP (image))
        {
          gtk_adjustment_set_upper (selection->index_adjustment, 0);

          if (gtk_widget_get_mapped (GTK_WIDGET (selection)))
            {
              if (selection->active_palette)
                g_signal_handlers_disconnect_by_func (selection->active_palette,
                                                      G_CALLBACK (gtk_widget_queue_draw),
                                                      selection);
              ligma_view_set_viewable (LIGMA_VIEW (selection->view), NULL);
              gtk_adjustment_set_upper (selection->index_adjustment, 0);
              selection->active_palette = NULL;
            }
        }
    }
  selection->active_image = image;
  if (image)
    {
      g_object_add_weak_pointer (G_OBJECT (selection->active_image),
                                 (gpointer) &selection->active_image);
      g_signal_connect_swapped (image, "colormap-changed",
                                G_CALLBACK (gtk_widget_queue_draw),
                                selection);
      g_signal_connect_swapped (image, "mode-changed",
                                G_CALLBACK (ligma_colormap_selection_set_palette),
                                selection);
    }
  ligma_colormap_selection_set_palette (selection);
  gtk_widget_queue_draw (GTK_WIDGET (selection));
}

static void
ligma_colormap_selection_set_palette (LigmaColormapSelection *selection)
{
  LigmaPalette *palette = NULL;

  if (selection->active_image)
    palette = ligma_image_get_colormap_palette (selection->active_image);

  if (palette != selection->active_palette)
    {
      if (selection->active_palette)
        {
          g_object_remove_weak_pointer (G_OBJECT (selection->active_palette),
                                        (gpointer) &selection->active_palette);
          g_signal_handlers_disconnect_by_func (selection->active_palette,
                                                G_CALLBACK (gtk_widget_queue_draw),
                                                selection);
          ligma_view_set_viewable (LIGMA_VIEW (selection->view), NULL);
          gtk_adjustment_set_upper (selection->index_adjustment, 0);
        }
      selection->active_palette = palette;
      if (palette)
        {
          g_object_add_weak_pointer (G_OBJECT (selection->active_palette),
                                     (gpointer) &selection->active_palette);
          g_signal_connect_swapped (palette, "dirty",
                                    G_CALLBACK (gtk_widget_queue_draw),
                                    selection);
          ligma_view_set_viewable (LIGMA_VIEW (selection->view),
                                  LIGMA_VIEWABLE (palette));

          gtk_adjustment_set_upper (selection->index_adjustment,
                                    ligma_image_get_colormap_size (selection->active_image) - 1);
        }
    }
}
