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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
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
#include "core/gimpprojection.h"

#include "gimpcolordialog.h"
#include "gimpcolormapselection.h"
#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimpmenufactory.h"
#include "gimppaletteview.h"
#include "gimpuimanager.h"
#include "gimpviewrendererpalette.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_INDEX
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
         gimp_image_get_base_type (image) == GIMP_INDEXED && \
         gimp_image_get_colormap_palette (image) != NULL)


static void   gimp_colormap_selection_set_property     (GObject               *object,
                                                        guint                  property_id,
                                                        const GValue          *value,
                                                        GParamSpec            *pspec);
static void   gimp_colormap_selection_get_property     (GObject               *object,
                                                        guint                  property_id,
                                                        GValue                *value,
                                                        GParamSpec            *pspec);
static void   gimp_colormap_selection_dispose          (GObject               *object);
static void   gimp_colormap_selection_finalize         (GObject               *object);

static void   gimp_colormap_selection_unmap            (GtkWidget             *widget);

static PangoLayout *
              gimp_colormap_selection_create_layout    (GtkWidget             *widget);

static void   gimp_colormap_selection_update_entries   (GimpColormapSelection *selection);

static gboolean
              gimp_colormap_selection_preview_draw     (GtkWidget             *widget,
                                                        cairo_t               *cr,
                                                        GimpColormapSelection *selection);

static void   gimp_colormap_selection_entry_clicked    (GimpPaletteView       *view,
                                                        GimpPaletteEntry      *entry,
                                                        GdkModifierType       state,
                                                        GimpColormapSelection *selection);
static void   gimp_colormap_selection_entry_selected   (GimpPaletteView       *view,
                                                        GimpPaletteEntry      *entry,
                                                        GimpColormapSelection *selection);
static void   gimp_colormap_selection_entry_activated  (GimpPaletteView       *view,
                                                        GimpPaletteEntry      *entry,
                                                        GimpColormapSelection *selection);
static void   gimp_colormap_selection_color_dropped    (GimpPaletteView       *view,
                                                        GimpPaletteEntry      *entry,
                                                        GeglColor             *color,
                                                        GimpColormapSelection *selection);

static void   gimp_colormap_adjustment_changed         (GtkAdjustment         *adjustment,
                                                        GimpColormapSelection *selection);
static void   gimp_colormap_hex_entry_changed          (GimpColorHexEntry     *entry,
                                                        GimpColormapSelection *selection);

static void   gimp_colormap_selection_set_context      (GimpColormapSelection *selection,
                                                        GimpContext           *context);
static void   gimp_colormap_selection_image_changed    (GimpColormapSelection *selection,
                                                        GimpImage             *image);
static void   gimp_colormap_selection_set_palette      (GimpColormapSelection *selection);
static void   gimp_colormap_selection_colormap_changed (GimpColormapSelection *selection);

G_DEFINE_TYPE (GimpColormapSelection, gimp_colormap_selection, GTK_TYPE_BOX)

#define parent_class gimp_colormap_selection_parent_class

static guint signals[LAST_SIGNAL] = { 0 };

static void
gimp_colormap_selection_class_init (GimpColormapSelectionClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  signals[COLOR_CLICKED] =
    g_signal_new ("color-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColormapSelectionClass, color_clicked),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER_ENUM,
                  G_TYPE_NONE, 2,
                  G_TYPE_POINTER,
                  GDK_TYPE_MODIFIER_TYPE);
  signals[COLOR_ACTIVATED] =
    g_signal_new ("color-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColormapSelectionClass, color_activated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  object_class->set_property    = gimp_colormap_selection_set_property;
  object_class->get_property    = gimp_colormap_selection_get_property;
  object_class->dispose         = gimp_colormap_selection_dispose;
  object_class->finalize        = gimp_colormap_selection_finalize;

  widget_class->unmap           = gimp_colormap_selection_unmap;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_INDEX,
                                   g_param_spec_int ("index", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READABLE));
}

static void
gimp_colormap_selection_init (GimpColormapSelection *selection)
{
  GtkWidget *frame;
  GtkWidget *grid;

  gtk_box_set_homogeneous (GTK_BOX (selection), FALSE);

  /* Main colormap frame. */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (selection), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  selection->view = gimp_view_new_full_by_types (NULL,
                                                 GIMP_TYPE_PALETTE_VIEW,
                                                 GIMP_TYPE_PALETTE,
                                                 1, 1, 0,
                                                 FALSE, TRUE, FALSE);
  gimp_view_set_expand (GIMP_VIEW (selection->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), selection->view);
  gtk_widget_show (selection->view);

  g_signal_connect (selection->view, "draw",
                    G_CALLBACK (gimp_colormap_selection_preview_draw),
                    selection);

  g_signal_connect (selection->view, "entry-clicked",
                    G_CALLBACK (gimp_colormap_selection_entry_clicked),
                    selection);
  g_signal_connect (selection->view, "entry-selected",
                    G_CALLBACK (gimp_colormap_selection_entry_selected),
                    selection);
  g_signal_connect (selection->view, "entry-activated",
                    G_CALLBACK (gimp_colormap_selection_entry_activated),
                    selection);
  g_signal_connect (selection->view, "color-dropped",
                    G_CALLBACK (gimp_colormap_selection_color_dropped),
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
  selection->index_spinbutton = gimp_spin_button_new (selection->index_adjustment,
                                                      1.0, 0);
  gtk_widget_set_halign (selection->index_spinbutton, GTK_ALIGN_START);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (selection->index_spinbutton),
                               TRUE);

  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Color index:"), 0.0, 0.5,
                            selection->index_spinbutton, 1);

  g_signal_connect (selection->index_adjustment, "value-changed",
                    G_CALLBACK (gimp_colormap_adjustment_changed),
                    selection);

  selection->color_entry = gimp_color_hex_entry_new ();
  gtk_widget_set_halign (selection->color_entry, GTK_ALIGN_START);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("HTML notation (sRGB):"), 0.0, 0.5,
                            selection->color_entry, 1);

  g_signal_connect (selection->color_entry, "color-changed",
                    G_CALLBACK (gimp_colormap_hex_entry_changed),
                    selection);
}

static void
gimp_colormap_selection_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  GimpColormapSelection *selection = GIMP_COLORMAP_SELECTION (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      gimp_colormap_selection_set_context (selection, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_colormap_selection_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  GimpColormapSelection *selection = GIMP_COLORMAP_SELECTION (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, selection->context);
      break;
    case PROP_INDEX:
      g_value_set_int (value, selection->col_index);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_colormap_selection_dispose (GObject *object)
{
  GimpColormapSelection *selection = GIMP_COLORMAP_SELECTION (object);

  g_clear_pointer (&selection->color_dialog, gtk_widget_destroy);
  g_clear_weak_pointer (&selection->active_image);
  g_clear_weak_pointer (&selection->active_palette);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_colormap_selection_finalize (GObject *object)
{
  GimpColormapSelection *selection = GIMP_COLORMAP_SELECTION (object);

  if (selection->context)
    {
      g_signal_handlers_disconnect_by_func (selection->context,
                                            gtk_widget_queue_draw,
                                            selection);
      g_signal_handlers_disconnect_by_func (selection->context,
                                            G_CALLBACK (gimp_colormap_selection_image_changed),
                                            selection);
    }
  if (selection->active_image)
    {
      g_signal_handlers_disconnect_by_func (selection->active_image,
                                            G_CALLBACK (gtk_widget_queue_draw),
                                            selection);
      g_signal_handlers_disconnect_by_func (selection->active_image,
                                            G_CALLBACK (gimp_colormap_selection_set_palette),
                                            selection);
      g_signal_handlers_disconnect_by_func (selection->active_image,
                                            G_CALLBACK (gimp_colormap_selection_colormap_changed),
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
gimp_colormap_selection_unmap (GtkWidget *widget)
{
  GimpColormapSelection *selection = GIMP_COLORMAP_SELECTION (widget);

  if (selection->color_dialog)
    gtk_widget_hide (selection->color_dialog);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

/*  public functions  */

GtkWidget *
gimp_colormap_selection_new (GimpContext *context)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return g_object_new (GIMP_TYPE_COLORMAP_SELECTION,
                       "context",     context,
                       "orientation", GTK_ORIENTATION_VERTICAL,
                       NULL);
}

gint
gimp_colormap_selection_get_index (GimpColormapSelection *selection,
                                   GeglColor             *search)
{
  GimpImage *image;
  gint       index;

  g_return_val_if_fail (GIMP_IS_COLORMAP_SELECTION (selection), 0);

  image = gimp_context_get_image (selection->context);

  if (! HAVE_COLORMAP (image))
    return -1;

  index = selection->col_index;

  if (search)
    {
      GeglColor *temp;

      temp = gimp_image_get_colormap_entry (image, index);

      if (! gimp_color_is_perceptually_identical (temp, search))
        {
          gint n_colors;

          n_colors = gimp_palette_get_n_colors (gimp_image_get_colormap_palette (image));
          for (gint i = 0; i < n_colors; i++)
            {
              temp = gimp_image_get_colormap_entry (image, i);

              if (gimp_color_is_perceptually_identical (temp, search))
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
gimp_colormap_selection_set_index (GimpColormapSelection *selection,
                                   gint                   index,
                                   GeglColor             *color)
{
  GimpImage *image;
  gint       size;

  g_return_val_if_fail (GIMP_IS_COLORMAP_SELECTION (selection), FALSE);
  g_return_val_if_fail (color == NULL || GEGL_IS_COLOR (color), FALSE);

  image = gimp_context_get_image (selection->context);

  if (! HAVE_COLORMAP (image))
    return FALSE;

  size = gimp_palette_get_n_colors (gimp_image_get_colormap_palette (image));

  if (size < 1)
    return FALSE;

  index = CLAMP (index, 0, size - 1);

  if (index != selection->col_index)
    {
      GimpPalette *palette = gimp_image_get_colormap_palette (image);

      selection->col_index = index;

      g_object_notify (G_OBJECT (selection), "index");
      gimp_palette_view_select_entry (GIMP_PALETTE_VIEW (selection->view),
                                      gimp_palette_get_entry (palette, index));

      gimp_colormap_selection_update_entries (selection);
    }

  if (color)
    {
      GeglColor  *c = gimp_image_get_colormap_entry (image, index);
      const Babl *format;
      guchar      pixel[40];

      format = gegl_color_get_format (c);
      gegl_color_get_pixel (c, format, pixel);
      gegl_color_set_pixel (color, format, pixel);
    }

  return TRUE;
}

gint
gimp_colormap_selection_max_index (GimpColormapSelection *selection)
{
  GimpImage *image;
  gint       n_colors;

  g_return_val_if_fail (GIMP_IS_COLORMAP_SELECTION (selection), -1);

  image = gimp_context_get_image (selection->context);

  if (! HAVE_COLORMAP (image))
    return -1;

  n_colors = gimp_palette_get_n_colors (gimp_image_get_colormap_palette (image));

  return MAX (0, n_colors - 1);
}

GimpPaletteEntry *
gimp_colormap_selection_get_selected_entry (GimpColormapSelection *selection)
{
  g_return_val_if_fail (GIMP_IS_COLORMAP_SELECTION (selection), NULL);

  return gimp_palette_view_get_selected_entry (GIMP_PALETTE_VIEW (selection->view));
}

void
gimp_colormap_selection_get_entry_rect (GimpColormapSelection *selection,
                                        GimpPaletteEntry      *entry,
                                        GdkRectangle          *rect)
{
  GtkAllocation allocation;

  g_return_if_fail (GIMP_IS_COLORMAP_SELECTION (selection));
  g_return_if_fail (entry);
  g_return_if_fail (rect);

  gimp_palette_view_get_entry_rect (GIMP_PALETTE_VIEW (selection->view),
                                    entry, rect);
  gtk_widget_get_allocation (GTK_WIDGET (selection), &allocation);
  /* rect->x += allocation.x; */
  /* rect->y += allocation.y; */
}


/*  private functions  */

static PangoLayout *
gimp_colormap_selection_create_layout (GtkWidget *widget)
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
gimp_colormap_selection_preview_draw (GtkWidget             *widget,
                                      cairo_t               *cr,
                                      GimpColormapSelection *selection)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GimpImage       *image;
  GtkAllocation    allocation;
  GdkRGBA          color;
  gint             width, height;
  gint             y;

  image = gimp_context_get_image (selection->context);

  if (image == NULL || gimp_image_get_base_type (image) == GIMP_INDEXED)
    return FALSE;

  gtk_style_context_get_color (style, gtk_widget_get_state_flags (widget),
                               &color);
  gdk_cairo_set_source_rgba (cr, &color);

  gtk_widget_get_allocation (widget, &allocation);

  if (! selection->layout)
    selection->layout = gimp_colormap_selection_create_layout (selection->view);

  pango_layout_set_width (selection->layout,
                          PANGO_SCALE * (allocation.width - 2 * BORDER));

  pango_layout_get_pixel_size (selection->layout, &width, &height);

  y = (allocation.height - height) / 2;

  cairo_move_to (cr, BORDER, MAX (y, 0));
  pango_cairo_show_layout (cr, selection->layout);

  return TRUE;
}

static void
gimp_colormap_selection_update_entries (GimpColormapSelection *selection)
{
  GimpImage *image    = gimp_context_get_image (selection->context);
  gint       n_colors = 0;

  if (gimp_image_get_colormap_palette (image))
    n_colors = gimp_palette_get_n_colors (gimp_image_get_colormap_palette (image));

  if (! HAVE_COLORMAP (image) || ! n_colors)
    {
      gtk_widget_set_sensitive (selection->index_spinbutton, FALSE);
      gtk_widget_set_sensitive (selection->color_entry, FALSE);

      gtk_adjustment_set_value (selection->index_adjustment, 0);
      gtk_entry_set_text (GTK_ENTRY (selection->color_entry), "");
    }
  else
    {
      GeglColor  *color;
      guchar      rgb[3];
      gchar      *string;

      gtk_adjustment_set_value (selection->index_adjustment,
                                selection->col_index);
      color = gimp_image_get_colormap_entry (image, selection->col_index);
      /* The color entry shows a CSS/SVG notation, which so far means
       * sRGB. But is it really what we want? Don't we want to edit
       * colors in the image's specific RGB color space? TODO
       */
      gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), rgb);

      string = g_strdup_printf ("%02x%02x%02x", rgb[0], rgb[1], rgb[2]);
      gtk_entry_set_text (GTK_ENTRY (selection->color_entry), string);
      g_free (string);

      gtk_widget_set_sensitive (selection->index_spinbutton, TRUE);
      gtk_widget_set_sensitive (selection->color_entry, TRUE);
    }
}

static void
gimp_colormap_selection_entry_clicked (GimpPaletteView       *view,
                                       GimpPaletteEntry      *entry,
                                       GdkModifierType        state,
                                       GimpColormapSelection *selection)
{
  GimpPalette *palette;
  gint         index;

  palette = gimp_image_get_colormap_palette (selection->active_image);
  index = gimp_palette_get_entry_position (palette, entry);
  gimp_colormap_selection_set_index (selection, index, NULL);

  g_signal_emit (selection, signals[COLOR_CLICKED], 0, entry, state);
}

static void
gimp_colormap_selection_entry_selected (GimpPaletteView       *view,
                                        GimpPaletteEntry      *entry,
                                        GimpColormapSelection *selection)
{
  GimpPalette *palette;
  gint         index = 0;

  palette = gimp_image_get_colormap_palette (selection->active_image);
  if (entry)
    index = gimp_palette_get_entry_position (palette, entry);

  gimp_colormap_selection_set_index (selection, index, NULL);
}

static void
gimp_colormap_selection_entry_activated (GimpPaletteView       *view,
                                         GimpPaletteEntry      *entry,
                                         GimpColormapSelection *selection)
{
  GimpPalette *palette;
  gint         index;

  palette = gimp_image_get_colormap_palette (selection->active_image);
  index = gimp_palette_get_entry_position (palette, entry);
  gimp_colormap_selection_set_index (selection, index, NULL);

  g_signal_emit (selection, signals[COLOR_ACTIVATED], 0, entry);
}

static void
gimp_colormap_selection_color_dropped (GimpPaletteView       *view,
                                       GimpPaletteEntry      *entry,
                                       GeglColor             *color,
                                       GimpColormapSelection *selection)
{
}

static void
gimp_colormap_adjustment_changed (GtkAdjustment         *adjustment,
                                  GimpColormapSelection *selection)
{
  GimpImage *image = gimp_context_get_image (selection->context);

  if (HAVE_COLORMAP (image))
    {
      gint index = ROUND (gtk_adjustment_get_value (adjustment));

      gimp_colormap_selection_set_index (selection, index, NULL);

      gimp_colormap_selection_update_entries (selection);
    }
}

static void
gimp_colormap_hex_entry_changed (GimpColorHexEntry     *entry,
                                 GimpColormapSelection *selection)
{
  GimpImage *image = gimp_context_get_image (selection->context);

  if (image)
    {
      GeglColor *color;

      color = gimp_color_hex_entry_get_color (entry);

      gimp_image_set_colormap_entry (image, selection->col_index, color, TRUE);
      gimp_image_flush (image);
      g_object_unref (color);
    }
}

static void
gimp_colormap_selection_set_context (GimpColormapSelection *selection,
                                     GimpContext           *context)
{
  g_return_if_fail (GIMP_IS_COLORMAP_SELECTION (selection));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (context != selection->context)
    {
      if (selection->context)
        {
          g_signal_handlers_disconnect_by_func (selection->context,
                                                gtk_widget_queue_draw,
                                                selection);
          g_signal_handlers_disconnect_by_func (selection->context,
                                                G_CALLBACK (gimp_colormap_selection_image_changed),
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
                                    G_CALLBACK (gimp_colormap_selection_image_changed),
                                    selection);
          gimp_colormap_selection_image_changed (selection, gimp_context_get_image (context));
        }

      gimp_view_renderer_set_context (GIMP_VIEW (selection->view)->renderer,
                                      context);
      g_object_notify (G_OBJECT (selection), "context");
    }
}

static void
gimp_colormap_selection_image_changed (GimpColormapSelection *selection,
                                       GimpImage             *image)
{
  if (selection->active_image)
    {
      g_signal_handlers_disconnect_by_func (selection->active_image,
                                            G_CALLBACK (gimp_colormap_selection_colormap_changed),
                                            selection);
      g_signal_handlers_disconnect_by_func (selection->active_image,
                                            G_CALLBACK (gimp_colormap_selection_set_palette),
                                            selection);
      if (gimp_image_get_base_type (selection->active_image) == GIMP_INDEXED)
        {
          GimpPalette *palette;

          palette = gimp_image_get_colormap_palette (selection->active_image);
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
              gimp_view_set_viewable (GIMP_VIEW (selection->view), NULL);
              gtk_adjustment_set_upper (selection->index_adjustment, 0);
              selection->active_palette = NULL;
            }
        }
    }

  g_set_weak_pointer (&selection->active_image, image);

  if (image)
    {
      g_signal_connect_object (image, "colormap-changed",
                               G_CALLBACK (gimp_colormap_selection_colormap_changed),
                               selection, G_CONNECT_SWAPPED);
      g_signal_connect_object (image, "mode-changed",
                               G_CALLBACK (gimp_colormap_selection_set_palette),
                               selection, G_CONNECT_SWAPPED);
    }

  gimp_colormap_selection_set_palette (selection);
  gtk_widget_queue_draw (GTK_WIDGET (selection));
}

static void
gimp_colormap_selection_set_palette (GimpColormapSelection *selection)
{
  GimpPalette *palette = NULL;

  if (selection->active_image)
    palette = gimp_image_get_colormap_palette (selection->active_image);

  if (palette != selection->active_palette)
    {
      if (selection->active_palette)
        {
          gimp_colormap_selection_set_index (selection, 0, NULL);

          g_signal_handlers_disconnect_by_func (selection->active_palette,
                                                G_CALLBACK (gtk_widget_queue_draw),
                                                selection);
          gimp_view_set_viewable (GIMP_VIEW (selection->view), NULL);
          gtk_adjustment_set_upper (selection->index_adjustment, 0);
        }

      g_set_weak_pointer (&selection->active_palette, palette);

      if (palette)
        {
          gint n_colors;

          n_colors = gimp_palette_get_n_colors (palette);
          g_signal_connect_object (palette, "dirty",
                                   G_CALLBACK (gtk_widget_queue_draw),
                                   selection, G_CONNECT_SWAPPED);
          gimp_view_set_viewable (GIMP_VIEW (selection->view),
                                  GIMP_VIEWABLE (palette));
          gtk_adjustment_set_upper (selection->index_adjustment, n_colors - 1);
        }
    }
}

static void
gimp_colormap_selection_colormap_changed (GimpColormapSelection *selection)
{
  GimpPalette *palette = NULL;

  if (selection->active_image)
    palette = gimp_image_get_colormap_palette (selection->active_image);

  if (palette)
    {
      gint n_colors;

      n_colors = gimp_palette_get_n_colors (palette);
      gtk_adjustment_set_upper (selection->index_adjustment, n_colors - 1);

      gimp_colormap_selection_update_entries (selection);
    }

  gtk_widget_queue_draw (GTK_WIDGET (selection));
}
