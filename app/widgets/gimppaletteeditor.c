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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"
#include "core/ligmapalette.h"

#include "ligmacolordialog.h"
#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmadialogfactory.h"
#include "ligmahelp-ids.h"
#include "ligmapaletteeditor.h"
#include "ligmapaletteview.h"
#include "ligmasessioninfo-aux.h"
#include "ligmauimanager.h"
#include "ligmaviewrendererpalette.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


#define ENTRY_WIDTH  12
#define ENTRY_HEIGHT 10
#define SPACING       1
#define COLUMNS      16
#define ROWS         11

#define PREVIEW_WIDTH  ((ENTRY_WIDTH  + SPACING) * COLUMNS + 1)
#define PREVIEW_HEIGHT ((ENTRY_HEIGHT + SPACING) * ROWS    + 1)


/*  local function prototypes  */

static void   ligma_palette_editor_docked_iface_init (LigmaDockedInterface *face);

static void   ligma_palette_editor_constructed      (GObject           *object);
static void   ligma_palette_editor_dispose          (GObject           *object);

static void   ligma_palette_editor_unmap            (GtkWidget         *widget);

static void   ligma_palette_editor_set_data         (LigmaDataEditor    *editor,
                                                    LigmaData          *data);

static void   ligma_palette_editor_set_context      (LigmaDocked        *docked,
                                                    LigmaContext       *context);
static void   ligma_palette_editor_set_aux_info     (LigmaDocked        *docked,
                                                    GList             *aux_info);
static GList *ligma_palette_editor_get_aux_info     (LigmaDocked        *docked);

static void   palette_editor_invalidate_preview    (LigmaPalette       *palette,
                                                    LigmaPaletteEditor *editor);

static void   palette_editor_viewport_size_allocate(GtkWidget         *widget,
                                                    GtkAllocation     *allocation,
                                                    LigmaPaletteEditor *editor);

static void   palette_editor_drop_palette          (GtkWidget         *widget,
                                                    gint               x,
                                                    gint               y,
                                                    LigmaViewable      *viewable,
                                                    gpointer           data);
static void   palette_editor_drop_color            (GtkWidget         *widget,
                                                    gint               x,
                                                    gint               y,
                                                    const LigmaRGB     *color,
                                                    gpointer           data);

static void   palette_editor_entry_clicked         (LigmaPaletteView   *view,
                                                    LigmaPaletteEntry  *entry,
                                                    GdkModifierType    state,
                                                    LigmaPaletteEditor *editor);
static void   palette_editor_entry_selected        (LigmaPaletteView   *view,
                                                    LigmaPaletteEntry  *entry,
                                                    LigmaPaletteEditor *editor);
static void   palette_editor_entry_activated       (LigmaPaletteView   *view,
                                                    LigmaPaletteEntry  *entry,
                                                    LigmaPaletteEditor *editor);
static gboolean palette_editor_button_press_event  (GtkWidget *widget,
                                                    GdkEvent  *event,
                                                    gpointer   user_data);
static gboolean palette_editor_popup_menu          (GtkWidget *widget,
                                                    gpointer   user_data);
static void   palette_editor_color_dropped         (LigmaPaletteView   *view,
                                                    LigmaPaletteEntry  *entry,
                                                    const LigmaRGB     *color,
                                                    LigmaPaletteEditor *editor);

static void   palette_editor_color_name_changed    (GtkWidget         *widget,
                                                    LigmaPaletteEditor *editor);
static void   palette_editor_columns_changed       (GtkAdjustment     *adj,
                                                    LigmaPaletteEditor *editor);

static void   palette_editor_resize                (LigmaPaletteEditor *editor,
                                                    gint               width,
                                                    gdouble            zoom_factor);
static void   palette_editor_scroll_top_left       (LigmaPaletteEditor *editor);
static void   palette_editor_edit_color_update     (LigmaColorDialog   *dialog,
                                                    const LigmaRGB     *color,
                                                    LigmaColorDialogState state,
                                                    LigmaPaletteEditor *editor);


G_DEFINE_TYPE_WITH_CODE (LigmaPaletteEditor, ligma_palette_editor,
                         LIGMA_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_palette_editor_docked_iface_init))

#define parent_class ligma_palette_editor_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_palette_editor_class_init (LigmaPaletteEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass      *widget_class = GTK_WIDGET_CLASS (klass);
  LigmaDataEditorClass *editor_class = LIGMA_DATA_EDITOR_CLASS (klass);

  object_class->constructed = ligma_palette_editor_constructed;
  object_class->dispose     = ligma_palette_editor_dispose;

  widget_class->unmap       = ligma_palette_editor_unmap;

  editor_class->set_data    = ligma_palette_editor_set_data;
  editor_class->title       = _("Palette Editor");
}

static void
ligma_palette_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_context  = ligma_palette_editor_set_context;
  iface->set_aux_info = ligma_palette_editor_set_aux_info;
  iface->get_aux_info = ligma_palette_editor_get_aux_info;
}

static void
ligma_palette_editor_init (LigmaPaletteEditor *editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);
  GtkWidget      *viewport;
  GtkWidget      *hbox;
  GtkWidget      *icon;
  GtkWidget      *spinbutton;

  editor->zoom_factor = 1.0;
  editor->col_width   = 0;
  editor->last_width  = 0;
  editor->columns     = COLUMNS;

  data_editor->view = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (data_editor->view, -1, PREVIEW_HEIGHT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (data_editor->view),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (editor), data_editor->view, TRUE, TRUE, 0);
  gtk_widget_show (data_editor->view);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (data_editor->view), viewport);
  gtk_widget_show (viewport);

  editor->view = ligma_view_new_full_by_types (NULL,
                                              LIGMA_TYPE_PALETTE_VIEW,
                                              LIGMA_TYPE_PALETTE,
                                              PREVIEW_WIDTH, PREVIEW_HEIGHT, 0,
                                              FALSE, TRUE, FALSE);
  ligma_view_renderer_palette_set_cell_size
    (LIGMA_VIEW_RENDERER_PALETTE (LIGMA_VIEW (editor->view)->renderer), -1);
  ligma_view_renderer_palette_set_draw_grid
    (LIGMA_VIEW_RENDERER_PALETTE (LIGMA_VIEW (editor->view)->renderer), TRUE);
  gtk_container_add (GTK_CONTAINER (viewport), editor->view);
  gtk_widget_show (editor->view);

  g_signal_connect (gtk_widget_get_parent (editor->view), "size-allocate",
                    G_CALLBACK (palette_editor_viewport_size_allocate),
                    editor);

  g_signal_connect (editor->view, "entry-clicked",
                    G_CALLBACK (palette_editor_entry_clicked),
                    editor);
  g_signal_connect (editor->view, "entry-selected",
                    G_CALLBACK (palette_editor_entry_selected),
                    editor);
  g_signal_connect (editor->view, "entry-activated",
                    G_CALLBACK (palette_editor_entry_activated),
                    editor);
  g_signal_connect (editor->view, "color-dropped",
                    G_CALLBACK (palette_editor_color_dropped),
                    editor);
  g_signal_connect (editor->view, "button-press-event",
                    G_CALLBACK (palette_editor_button_press_event),
                    editor);
  g_signal_connect (editor->view, "popup-menu",
                    G_CALLBACK (palette_editor_popup_menu),
                    editor);

  ligma_dnd_viewable_dest_add (editor->view,
                              LIGMA_TYPE_PALETTE,
                              palette_editor_drop_palette,
                              editor);
  ligma_dnd_viewable_dest_add (gtk_widget_get_parent (editor->view),
                              LIGMA_TYPE_PALETTE,
                              palette_editor_drop_palette,
                              editor);

  ligma_dnd_color_dest_add (gtk_widget_get_parent (editor->view),
                           palette_editor_drop_color,
                           editor);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The color index number  */
  editor->index_label = gtk_label_new ("####");
  gtk_box_pack_start (GTK_BOX (hbox), editor->index_label, FALSE, FALSE, 0);
  ligma_label_set_attributes (GTK_LABEL (editor->index_label),
                             PANGO_ATTR_FAMILY, "Monospace", -1);
  gtk_widget_show (editor->index_label);

  /*  The color name entry  */
  editor->color_name = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), editor->color_name, TRUE, TRUE, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (editor->color_name), 1);
  gtk_entry_set_text (GTK_ENTRY (editor->color_name), _("Undefined"));
  gtk_editable_set_editable (GTK_EDITABLE (editor->color_name), FALSE);
  gtk_widget_show (editor->color_name);

  g_signal_connect (editor->color_name, "changed",
                    G_CALLBACK (palette_editor_color_name_changed),
                    editor);

  icon = gtk_image_new_from_icon_name (LIGMA_ICON_GRID, GTK_ICON_SIZE_MENU);
  gtk_widget_set_margin_start (icon, 2);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
  gtk_widget_show (icon);

  editor->columns_adj = gtk_adjustment_new (0, 0, 64, 1, 4, 0);
  spinbutton = ligma_spin_button_new (editor->columns_adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  ligma_help_set_help_data (spinbutton, _("Set the number of columns"), NULL);

  g_signal_connect (editor->columns_adj, "value-changed",
                    G_CALLBACK (palette_editor_columns_changed),
                    editor);
}

static void
ligma_palette_editor_constructed (GObject *object)
{
  LigmaPaletteEditor *editor = LIGMA_PALETTE_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "palette-editor",
                                 "palette-editor-edit-color", NULL);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "palette-editor",
                                 "palette-editor-new-color-fg",
                                 "palette-editor-new-color-bg",
                                 ligma_get_toggle_behavior_mask (),
                                 NULL);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "palette-editor",
                                 "palette-editor-delete-color", NULL);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "palette-editor",
                                 "palette-editor-zoom-out", NULL);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "palette-editor",
                                 "palette-editor-zoom-in", NULL);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "palette-editor",
                                 "palette-editor-zoom-all", NULL);
}

static void
ligma_palette_editor_dispose (GObject *object)
{
  LigmaPaletteEditor *editor = LIGMA_PALETTE_EDITOR (object);

  g_clear_pointer (&editor->color_dialog, gtk_widget_destroy);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_palette_editor_unmap (GtkWidget *widget)
{
  LigmaPaletteEditor *editor = LIGMA_PALETTE_EDITOR (widget);

  if (editor->color_dialog)
    gtk_widget_hide (editor->color_dialog);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
ligma_palette_editor_set_data (LigmaDataEditor *editor,
                              LigmaData       *data)
{
  LigmaPaletteEditor *palette_editor = LIGMA_PALETTE_EDITOR (editor);

  g_signal_handlers_block_by_func (palette_editor->columns_adj,
                                   palette_editor_columns_changed,
                                   editor);

  if (editor->data)
    {
      if (palette_editor->color_dialog)
        {
          gtk_widget_destroy (palette_editor->color_dialog);
          palette_editor->color_dialog = NULL;
        }

      g_signal_handlers_disconnect_by_func (editor->data,
                                            palette_editor_invalidate_preview,
                                            editor);

      gtk_adjustment_set_value (palette_editor->columns_adj, 0);
    }

  LIGMA_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  ligma_view_set_viewable (LIGMA_VIEW (palette_editor->view),
                          LIGMA_VIEWABLE (data));

  if (editor->data)
    {
      LigmaPalette *palette = LIGMA_PALETTE (editor->data);

      g_signal_connect (editor->data, "invalidate-preview",
                        G_CALLBACK (palette_editor_invalidate_preview),
                        editor);

      gtk_adjustment_set_value (palette_editor->columns_adj,
                                ligma_palette_get_columns (palette));

      palette_editor_scroll_top_left (palette_editor);

      palette_editor_invalidate_preview (LIGMA_PALETTE (editor->data),
                                         palette_editor);
    }

  g_signal_handlers_unblock_by_func (palette_editor->columns_adj,
                                     palette_editor_columns_changed,
                                     editor);
}

static void
ligma_palette_editor_set_context (LigmaDocked  *docked,
                                 LigmaContext *context)
{
  LigmaPaletteEditor *editor = LIGMA_PALETTE_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  ligma_view_renderer_set_context (LIGMA_VIEW (editor->view)->renderer,
                                  context);
}

#define AUX_INFO_ZOOM_FACTOR "zoom-factor"

static void
ligma_palette_editor_set_aux_info (LigmaDocked *docked,
                                  GList      *aux_info)
{
  LigmaPaletteEditor *editor = LIGMA_PALETTE_EDITOR (docked);
  GList             *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      LigmaSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_ZOOM_FACTOR))
        {
          gdouble zoom_factor;

          zoom_factor = g_ascii_strtod (aux->value, NULL);

          editor->zoom_factor = CLAMP (zoom_factor, 0.1, 4.0);
        }
    }
}

static GList *
ligma_palette_editor_get_aux_info (LigmaDocked *docked)
{
  LigmaPaletteEditor *editor = LIGMA_PALETTE_EDITOR (docked);
  GList             *aux_info;

  aux_info = parent_docked_iface->get_aux_info (docked);

  if (editor->zoom_factor != 1.0)
    {
      LigmaSessionInfoAux *aux;
      gchar               value[G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_formatd (value, sizeof (value), "%.2f", editor->zoom_factor);

      aux = ligma_session_info_aux_new (AUX_INFO_ZOOM_FACTOR, value);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}


/*  public functions  */

GtkWidget *
ligma_palette_editor_new (LigmaContext     *context,
                         LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return g_object_new (LIGMA_TYPE_PALETTE_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<PaletteEditor>",
                       "ui-path",         "/palette-editor-popup",
                       "data-factory",    context->ligma->palette_factory,
                       "context",         context,
                       "data",            ligma_context_get_palette (context),
                       NULL);
}

void
ligma_palette_editor_edit_color (LigmaPaletteEditor *editor)
{
  LigmaDataEditor *data_editor;
  LigmaPalette    *palette;

  g_return_if_fail (LIGMA_IS_PALETTE_EDITOR (editor));

  data_editor = LIGMA_DATA_EDITOR (editor);

  if (! (data_editor->data_editable && editor->color))
    return;

  palette = LIGMA_PALETTE (ligma_data_editor_get_data (data_editor));

  if (! editor->color_dialog)
    {
      editor->color_dialog =
        ligma_color_dialog_new (LIGMA_VIEWABLE (palette),
                               data_editor->context,
                               FALSE,
                               _("Edit Palette Color"),
                               LIGMA_ICON_PALETTE,
                               _("Edit Color Palette Entry"),
                               GTK_WIDGET (editor),
                               ligma_dialog_factory_get_singleton (),
                               "ligma-palette-editor-color-dialog",
                               &editor->color->color,
                               FALSE, FALSE);

      g_signal_connect (editor->color_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &editor->color_dialog);

      g_signal_connect (editor->color_dialog, "update",
                        G_CALLBACK (palette_editor_edit_color_update),
                        editor);
    }
  else
    {
      ligma_viewable_dialog_set_viewables (LIGMA_VIEWABLE_DIALOG (editor->color_dialog),
                                          g_list_prepend (NULL, palette),
                                          data_editor->context);
      ligma_color_dialog_set_color (LIGMA_COLOR_DIALOG (editor->color_dialog),
                                   &editor->color->color);

      if (! gtk_widget_get_visible (editor->color_dialog))
        ligma_dialog_factory_position_dialog (ligma_dialog_factory_get_singleton (),
                                             "ligma-palette-editor-color-dialog",
                                             editor->color_dialog,
                                             ligma_widget_get_monitor (GTK_WIDGET (editor)));
    }

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

void
ligma_palette_editor_pick_color (LigmaPaletteEditor  *editor,
                                const LigmaRGB      *color,
                                LigmaColorPickState  pick_state)
{
  g_return_if_fail (LIGMA_IS_PALETTE_EDITOR (editor));
  g_return_if_fail (color != NULL);

  if (LIGMA_DATA_EDITOR (editor)->data_editable)
    {
      LigmaPaletteEntry *entry;
      LigmaData         *data;
      gint              index = -1;

      data = ligma_data_editor_get_data (LIGMA_DATA_EDITOR (editor));
      index = ligma_palette_get_entry_position (LIGMA_PALETTE (data),
                                               editor->color);

      switch (pick_state)
        {
        case LIGMA_COLOR_PICK_STATE_START:
          if (editor->color)
            index += 1;

          entry = ligma_palette_add_entry (LIGMA_PALETTE (data), index,
                                          NULL, color);
          ligma_palette_view_select_entry (LIGMA_PALETTE_VIEW (editor->view),
                                          entry);
          break;

        case LIGMA_COLOR_PICK_STATE_UPDATE:
        case LIGMA_COLOR_PICK_STATE_END:
          ligma_palette_set_entry_color (LIGMA_PALETTE (data),
                                        index, color);
          break;
        }
    }
}

void
ligma_palette_editor_zoom (LigmaPaletteEditor  *editor,
                          LigmaZoomType        zoom_type)
{
  LigmaPalette *palette;
  gdouble      zoom_factor;

  g_return_if_fail (LIGMA_IS_PALETTE_EDITOR (editor));

  palette = LIGMA_PALETTE (LIGMA_DATA_EDITOR (editor)->data);

  if (! palette)
    return;

  zoom_factor = editor->zoom_factor;

  switch (zoom_type)
    {
    case LIGMA_ZOOM_IN_MAX:
    case LIGMA_ZOOM_IN_MORE:
    case LIGMA_ZOOM_IN:
      zoom_factor += 0.1;
      break;

    case LIGMA_ZOOM_OUT_MORE:
    case LIGMA_ZOOM_OUT:
      zoom_factor -= 0.1;
      break;

    case LIGMA_ZOOM_OUT_MAX:
    case LIGMA_ZOOM_TO: /* abused as ZOOM_ALL */
      {
        GtkWidget     *scrolled_win = LIGMA_DATA_EDITOR (editor)->view;
        GtkWidget     *viewport     = gtk_bin_get_child (GTK_BIN (scrolled_win));
        GtkAllocation  allocation;
        gint           columns;
        gint           rows;

        gtk_widget_get_allocation (viewport, &allocation);

        columns = ligma_palette_get_columns (palette);
        if (columns == 0)
          columns = COLUMNS;

        rows = ligma_palette_get_n_colors (palette) / columns;
        if (ligma_palette_get_n_colors (palette) % columns)
          rows += 1;

        rows = MAX (1, rows);

        zoom_factor = (((gdouble) allocation.height - 2 * SPACING) /
                       (gdouble) rows - SPACING) / ENTRY_HEIGHT;
      }
      break;

    case LIGMA_ZOOM_SMOOTH:
    case LIGMA_ZOOM_PINCH: /* can't happen */
      g_return_if_reached ();
    }

  zoom_factor = CLAMP (zoom_factor, 0.1, 4.0);

  editor->columns = ligma_palette_get_columns (palette);
  if (editor->columns == 0)
    editor->columns = COLUMNS;

  palette_editor_resize (editor, editor->last_width, zoom_factor);

  palette_editor_scroll_top_left (editor);
}

gint
ligma_palette_editor_get_index (LigmaPaletteEditor *editor,
                               const LigmaRGB     *search)
{
  LigmaPalette *palette;

  g_return_val_if_fail (LIGMA_IS_PALETTE_EDITOR (editor), -1);
  g_return_val_if_fail (search != NULL, -1);

  palette = LIGMA_PALETTE (LIGMA_DATA_EDITOR (editor)->data);

  if (palette && ligma_palette_get_n_colors (palette) > 0)
    {
      LigmaPaletteEntry *entry;

      entry = ligma_palette_find_entry (palette, search, editor->color);

      if (entry)
        return ligma_palette_get_entry_position (palette, entry);
    }

  return -1;
}

gboolean
ligma_palette_editor_set_index (LigmaPaletteEditor *editor,
                               gint               index,
                               LigmaRGB           *color)
{
  LigmaPalette *palette;

  g_return_val_if_fail (LIGMA_IS_PALETTE_EDITOR (editor), FALSE);

  palette = LIGMA_PALETTE (LIGMA_DATA_EDITOR (editor)->data);

  if (palette && ligma_palette_get_n_colors (palette) > 0)
    {
      LigmaPaletteEntry *entry;

      index = CLAMP (index, 0, ligma_palette_get_n_colors (palette) - 1);

      entry = ligma_palette_get_entry (palette, index);

      ligma_palette_view_select_entry (LIGMA_PALETTE_VIEW (editor->view),
                                      entry);

      if (color)
        *color = editor->color->color;

      return TRUE;
    }

  return FALSE;
}

gint
ligma_palette_editor_max_index (LigmaPaletteEditor *editor)
{
  LigmaPalette *palette;

  g_return_val_if_fail (LIGMA_IS_PALETTE_EDITOR (editor), -1);

  palette = LIGMA_PALETTE (LIGMA_DATA_EDITOR (editor)->data);

  if (palette && ligma_palette_get_n_colors (palette) > 0)
    {
      return ligma_palette_get_n_colors (palette) - 1;
    }

  return -1;
}


/*  private functions  */

static void
palette_editor_invalidate_preview (LigmaPalette       *palette,
                                   LigmaPaletteEditor *editor)
{
  editor->columns = ligma_palette_get_columns (palette);
  if (editor->columns == 0)
    editor->columns = COLUMNS;

  palette_editor_resize (editor, editor->last_width, editor->zoom_factor);
}

static void
palette_editor_viewport_size_allocate (GtkWidget         *widget,
                                       GtkAllocation     *allocation,
                                       LigmaPaletteEditor *editor)
{
  if (allocation->width != editor->last_width)
    {
      palette_editor_resize (editor, allocation->width,
                             editor->zoom_factor);
    }
}

static void
palette_editor_drop_palette (GtkWidget    *widget,
                             gint          x,
                             gint          y,
                             LigmaViewable *viewable,
                             gpointer      data)
{
  ligma_data_editor_set_data (LIGMA_DATA_EDITOR (data), LIGMA_DATA (viewable));
}

static void
palette_editor_drop_color (GtkWidget     *widget,
                           gint           x,
                           gint           y,
                           const LigmaRGB *color,
                           gpointer       data)
{
  LigmaPaletteEditor *editor = data;

  if (LIGMA_DATA_EDITOR (editor)->data_editable)
    {
      LigmaPalette      *palette = LIGMA_PALETTE (LIGMA_DATA_EDITOR (editor)->data);
      LigmaPaletteEntry *entry;

      entry = ligma_palette_add_entry (palette, -1, NULL, color);
      ligma_palette_view_select_entry (LIGMA_PALETTE_VIEW (editor->view), entry);
    }
}


/*  palette view callbacks  */

static void
palette_editor_entry_clicked (LigmaPaletteView   *view,
                              LigmaPaletteEntry  *entry,
                              GdkModifierType    state,
                              LigmaPaletteEditor *editor)
{
  if (entry)
    {
      LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);

      if (state & ligma_get_toggle_behavior_mask ())
        ligma_context_set_background (data_editor->context, &entry->color);
      else
        ligma_context_set_foreground (data_editor->context, &entry->color);
    }
}

static void
palette_editor_entry_selected (LigmaPaletteView   *view,
                               LigmaPaletteEntry  *entry,
                               LigmaPaletteEditor *editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);

  if (editor->color != entry)
    {
      gchar index[8];

      editor->color = entry;

      if (entry)
        {
          LigmaPalette *palette = LIGMA_PALETTE (data_editor->data);
          gint         pos;

          pos = ligma_palette_get_entry_position (palette, entry);
          g_snprintf (index, sizeof (index), "%04i", pos);
        }
      else
        {
          g_snprintf (index, sizeof (index), "####");
        }

      gtk_label_set_text (GTK_LABEL (editor->index_label), index);

      g_signal_handlers_block_by_func (editor->color_name,
                                       palette_editor_color_name_changed,
                                       editor);

      gtk_entry_set_text (GTK_ENTRY (editor->color_name),
                          entry ? entry->name : _("Undefined"));

      g_signal_handlers_unblock_by_func (editor->color_name,
                                         palette_editor_color_name_changed,
                                         editor);

      gtk_editable_set_editable (GTK_EDITABLE (editor->color_name),
                                 entry && data_editor->data_editable);

      ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                              ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
    }
}

static void
palette_editor_entry_activated (LigmaPaletteView   *view,
                                LigmaPaletteEntry  *entry,
                                LigmaPaletteEditor *editor)
{
  if (LIGMA_DATA_EDITOR (editor)->data_editable && entry == editor->color)
    {
      ligma_ui_manager_activate_action (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                                       "palette-editor",
                                       "palette-editor-edit-color");
    }
}

static gboolean
palette_editor_button_press_event (GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   user_data)
{
  LigmaPaletteEditor *editor = LIGMA_PALETTE_EDITOR (user_data);

  if (gdk_event_triggers_context_menu (event))
    {
      ligma_editor_popup_menu_at_pointer (LIGMA_EDITOR (editor), event);
      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static gboolean
palette_editor_popup_menu (GtkWidget *widget,
                           gpointer   user_data)
{
  LigmaPaletteEditor *editor = LIGMA_PALETTE_EDITOR (user_data);
  LigmaPaletteEntry *selected;
  GdkRectangle rect;

  selected = ligma_palette_view_get_selected_entry (LIGMA_PALETTE_VIEW (editor->view));
  if (!selected)
    return GDK_EVENT_PROPAGATE;

  ligma_palette_view_get_entry_rect (LIGMA_PALETTE_VIEW (editor->view), selected, &rect);
  return ligma_editor_popup_menu_at_rect (LIGMA_EDITOR (editor),
                                         gtk_widget_get_window (GTK_WIDGET (editor->view)),
                                         &rect, GDK_GRAVITY_CENTER, GDK_GRAVITY_NORTH_WEST,
                                         NULL);
}

static void
palette_editor_color_dropped (LigmaPaletteView   *view,
                              LigmaPaletteEntry  *entry,
                              const LigmaRGB     *color,
                              LigmaPaletteEditor *editor)
{
  if (LIGMA_DATA_EDITOR (editor)->data_editable)
    {
      LigmaPalette *palette = LIGMA_PALETTE (LIGMA_DATA_EDITOR (editor)->data);
      gint         pos     = -1;

      if (entry)
        pos = ligma_palette_get_entry_position (palette, entry);

      entry = ligma_palette_add_entry (palette, pos, NULL, color);
      ligma_palette_view_select_entry (LIGMA_PALETTE_VIEW (editor->view), entry);
    }
}


/*  color name and columns callbacks  */

static void
palette_editor_color_name_changed (GtkWidget         *widget,
                                   LigmaPaletteEditor *editor)
{
  if (LIGMA_DATA_EDITOR (editor)->data)
    {
      LigmaPalette *palette = LIGMA_PALETTE (LIGMA_DATA_EDITOR (editor)->data);
      const gchar *name;
      gint         pos;

      name = gtk_entry_get_text (GTK_ENTRY (editor->color_name));

      pos = ligma_palette_get_entry_position (palette, editor->color);
      ligma_palette_set_entry_name (palette, pos, name);
    }
}

static void
palette_editor_columns_changed (GtkAdjustment     *adj,
                                LigmaPaletteEditor *editor)
{
  if (LIGMA_DATA_EDITOR (editor)->data)
    {
      LigmaPalette *palette = LIGMA_PALETTE (LIGMA_DATA_EDITOR (editor)->data);

      ligma_palette_set_columns (palette,
                                ROUND (gtk_adjustment_get_value (adj)));
    }
}


/*  misc utils  */

static void
palette_editor_resize (LigmaPaletteEditor *editor,
                       gint               width,
                       gdouble            zoom_factor)
{
  LigmaPalette *palette;
  gint         rows;
  gint         preview_width;
  gint         preview_height;

  palette = LIGMA_PALETTE (LIGMA_DATA_EDITOR (editor)->data);

  if (! palette)
    return;

  editor->zoom_factor = zoom_factor;
  editor->last_width  = width;
  editor->col_width   = width / (editor->columns + 1) - SPACING;

  if (editor->col_width < 0)
    editor->col_width = 0;

  rows = ligma_palette_get_n_colors (palette) / editor->columns;
  if (ligma_palette_get_n_colors (palette) % editor->columns)
    rows += 1;

  preview_width  = (editor->col_width + SPACING) * editor->columns;
  preview_height = (rows *
                    (SPACING + (gint) (ENTRY_HEIGHT * editor->zoom_factor)));

  if (preview_height > LIGMA_VIEWABLE_MAX_PREVIEW_SIZE)
    preview_height = ((LIGMA_VIEWABLE_MAX_PREVIEW_SIZE - SPACING) / rows) * rows;

  ligma_view_renderer_set_size_full (LIGMA_VIEW (editor->view)->renderer,
                                    preview_width  + SPACING,
                                    preview_height + SPACING, 0);
}

static void
palette_editor_scroll_top_left (LigmaPaletteEditor *palette_editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (palette_editor);
  GtkAdjustment  *hadj;
  GtkAdjustment  *vadj;

  if (! data_editor->view)
    return;

  hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (data_editor->view));
  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (data_editor->view));

  if (hadj)
    gtk_adjustment_set_value (hadj, 0.0);
  if (vadj)
    gtk_adjustment_set_value (vadj, 0.0);
}

static void
palette_editor_edit_color_update (LigmaColorDialog      *dialog,
                                  const LigmaRGB        *color,
                                  LigmaColorDialogState  state,
                                  LigmaPaletteEditor    *editor)
{
  LigmaPalette *palette = LIGMA_PALETTE (LIGMA_DATA_EDITOR (editor)->data);

  switch (state)
    {
    case LIGMA_COLOR_DIALOG_UPDATE:
      break;

    case LIGMA_COLOR_DIALOG_OK:
      if (editor->color)
        {
          editor->color->color = *color;
          ligma_data_dirty (LIGMA_DATA (palette));
        }
      /* Fallthrough */

    case LIGMA_COLOR_DIALOG_CANCEL:
      gtk_widget_hide (editor->color_dialog);
      break;
    }
}
