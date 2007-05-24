/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimppalette.h"

#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimphelp-ids.h"
#include "gimppaletteeditor.h"
#include "gimppaletteview.h"
#include "gimpsessioninfoaux.h"
#include "gimpuimanager.h"
#include "gimpviewrendererpalette.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define ENTRY_WIDTH  12
#define ENTRY_HEIGHT 10
#define SPACING       1
#define COLUMNS      16
#define ROWS         11

#define PREVIEW_WIDTH  ((ENTRY_WIDTH  + SPACING) * COLUMNS + 1)
#define PREVIEW_HEIGHT ((ENTRY_HEIGHT + SPACING) * ROWS    + 1)

/*  local function prototypes  */

static void   gimp_palette_editor_docked_iface_init (GimpDockedInterface *face);

static GObject * gimp_palette_editor_constructor   (GType              type,
                                                    guint              n_params,
                                                    GObjectConstructParam *params);

static void   gimp_palette_editor_destroy          (GtkObject         *object);
static void   gimp_palette_editor_unmap            (GtkWidget         *widget);

static void   gimp_palette_editor_set_data         (GimpDataEditor    *editor,
                                                    GimpData          *data);

static void   gimp_palette_editor_set_context      (GimpDocked        *docked,
                                                    GimpContext       *context);
static void   gimp_palette_editor_set_aux_info     (GimpDocked        *docked,
                                                    GList             *aux_info);
static GList *gimp_palette_editor_get_aux_info     (GimpDocked        *docked);

static void   palette_editor_invalidate_preview    (GimpPalette       *palette,
                                                    GimpPaletteEditor *editor);

static void   palette_editor_viewport_size_allocate(GtkWidget         *widget,
                                                    GtkAllocation     *allocation,
                                                    GimpPaletteEditor *editor);

static gint   palette_editor_eventbox_button_press (GtkWidget         *widget,
                                                    GdkEventButton    *bevent,
                                                    GimpPaletteEditor *editor);
static void   palette_editor_drop_color            (GtkWidget         *widget,
                                                    gint               x,
                                                    gint               y,
                                                    const GimpRGB     *color,
                                                    gpointer           data);
static void   palette_editor_drop_palette          (GtkWidget         *widget,
                                                    gint               x,
                                                    gint               y,
                                                    GimpViewable      *viewable,
                                                    gpointer           data);

static void   palette_editor_entry_clicked         (GimpPaletteView   *view,
                                                    GimpPaletteEntry  *entry,
                                                    GdkModifierType    state,
                                                    GimpPaletteEditor *editor);
static void   palette_editor_entry_selected        (GimpPaletteView   *view,
                                                    GimpPaletteEntry  *entry,
                                                    GimpPaletteEditor *editor);
static void   palette_editor_entry_activated       (GimpPaletteView   *view,
                                                    GimpPaletteEntry  *entry,
                                                    GimpPaletteEditor *editor);
static void   palette_editor_entry_context         (GimpPaletteView   *view,
                                                    GimpPaletteEntry  *entry,
                                                    GimpPaletteEditor *editor);
static void   palette_editor_color_dropped         (GimpPaletteView   *view,
                                                    GimpPaletteEntry  *entry,
                                                    const GimpRGB     *color,
                                                    GimpPaletteEditor *editor);

static void   palette_editor_color_name_changed    (GtkWidget         *widget,
                                                    GimpPaletteEditor *editor);
static void   palette_editor_columns_changed       (GtkAdjustment     *adj,
                                                    GimpPaletteEditor *editor);

static void   palette_editor_resize                (GimpPaletteEditor *editor,
                                                    gint               width,
                                                    gdouble            zoom_factor);
static void   palette_editor_scroll_top_left       (GimpPaletteEditor *editor);


G_DEFINE_TYPE_WITH_CODE (GimpPaletteEditor, gimp_palette_editor,
                         GIMP_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_palette_editor_docked_iface_init))

#define parent_class gimp_palette_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_palette_editor_class_init (GimpPaletteEditorClass *klass)
{
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass      *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass      *widget_class     = GTK_WIDGET_CLASS (klass);
  GimpDataEditorClass *editor_class     = GIMP_DATA_EDITOR_CLASS (klass);

  object_class->constructor = gimp_palette_editor_constructor;

  gtk_object_class->destroy = gimp_palette_editor_destroy;

  widget_class->unmap       = gimp_palette_editor_unmap;

  editor_class->set_data    = gimp_palette_editor_set_data;
  editor_class->title       = _("Palette Editor");
}

static void
gimp_palette_editor_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_context  = gimp_palette_editor_set_context;
  iface->set_aux_info = gimp_palette_editor_set_aux_info;
  iface->get_aux_info = gimp_palette_editor_get_aux_info;
}

static void
gimp_palette_editor_init (GimpPaletteEditor *editor)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);
  GtkWidget      *eventbox;
  GtkWidget      *alignment;
  GtkWidget      *hbox;
  GtkWidget      *label;
  GtkWidget      *spinbutton;
  GtkObject      *adj;

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

  eventbox = gtk_event_box_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (data_editor->view),
                                         eventbox);
  gtk_widget_show (eventbox);

  g_signal_connect (eventbox, "button-press-event",
                    G_CALLBACK (palette_editor_eventbox_button_press),
                    editor);
  g_signal_connect (eventbox->parent, "size-allocate",
                    G_CALLBACK (palette_editor_viewport_size_allocate),
                    editor);

  gimp_dnd_color_dest_add (eventbox, palette_editor_drop_color, editor);
  gimp_dnd_viewable_dest_add (eventbox, GIMP_TYPE_PALETTE,
                              palette_editor_drop_palette,
                              editor);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (eventbox), alignment);
  gtk_widget_show (alignment);

  editor->view = gimp_view_new_full_by_types (NULL,
                                              GIMP_TYPE_PALETTE_VIEW,
                                              GIMP_TYPE_PALETTE,
                                              PREVIEW_WIDTH, PREVIEW_HEIGHT, 0,
                                              FALSE, TRUE, FALSE);
  gimp_view_renderer_palette_set_cell_size
    (GIMP_VIEW_RENDERER_PALETTE (GIMP_VIEW (editor->view)->renderer), -1);
  gimp_view_renderer_palette_set_draw_grid
    (GIMP_VIEW_RENDERER_PALETTE (GIMP_VIEW (editor->view)->renderer), TRUE);
  gtk_container_add (GTK_CONTAINER (alignment), editor->view);
  gtk_widget_show (editor->view);

  g_signal_connect (editor->view, "entry-clicked",
                    G_CALLBACK (palette_editor_entry_clicked),
                    editor);
  g_signal_connect (editor->view, "entry-selected",
                    G_CALLBACK (palette_editor_entry_selected),
                    editor);
  g_signal_connect (editor->view, "entry-activated",
                    G_CALLBACK (palette_editor_entry_activated),
                    editor);
  g_signal_connect (editor->view, "entry-context",
                    G_CALLBACK (palette_editor_entry_context),
                    editor);
  g_signal_connect (editor->view, "color-dropped",
                    G_CALLBACK (palette_editor_color_dropped),
                    editor);

  gimp_dnd_viewable_dest_add (editor->view, GIMP_TYPE_PALETTE,
                              palette_editor_drop_palette,
                              editor);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The color name entry  */
  editor->color_name = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), editor->color_name, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (editor->color_name), _("Undefined"));
  gtk_editable_set_editable (GTK_EDITABLE (editor->color_name), FALSE);
  gtk_widget_show (editor->color_name);

  g_signal_connect (editor->color_name, "changed",
                    G_CALLBACK (palette_editor_color_name_changed),
                    editor);

  label = gtk_label_new (_("Columns:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&adj, 0, 0, 64, 1, 4, 4, 1, 0);
  editor->columns_data = GTK_ADJUSTMENT (adj);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (editor->columns_data, "value-changed",
                    G_CALLBACK (palette_editor_columns_changed),
                    editor);
}

static GObject *
gimp_palette_editor_constructor (GType                  type,
                                 guint                  n_params,
                                 GObjectConstructParam *params)
{
  GObject           *object;
  GimpPaletteEditor *editor;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_PALETTE_EDITOR (object);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                 "palette-editor-edit-color", NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                 "palette-editor-new-color-fg",
                                 "palette-editor-new-color-bg",
                                 GDK_CONTROL_MASK,
                                 NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                 "palette-editor-delete-color", NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                 "palette-editor-zoom-out", NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                 "palette-editor-zoom-in", NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                 "palette-editor-zoom-all", NULL);

  return object;
}

static void
gimp_palette_editor_destroy (GtkObject *object)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (object);

  if (editor->color_dialog)
    {
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_palette_editor_unmap (GtkWidget *widget)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (widget);

  if (editor->color_dialog)
    gtk_widget_hide (editor->color_dialog);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_palette_editor_set_data (GimpDataEditor *editor,
                              GimpData       *data)
{
  GimpPaletteEditor *palette_editor = GIMP_PALETTE_EDITOR (editor);

  g_signal_handlers_block_by_func (palette_editor->columns_data,
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

      gtk_adjustment_set_value (palette_editor->columns_data, 0);
    }

  GIMP_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  gimp_view_set_viewable (GIMP_VIEW (palette_editor->view),
                          GIMP_VIEWABLE (data));

  if (editor->data)
    {
      GimpPalette *palette = GIMP_PALETTE (editor->data);

      g_signal_connect (editor->data, "invalidate-preview",
                        G_CALLBACK (palette_editor_invalidate_preview),
                        editor);

      gtk_adjustment_set_value (palette_editor->columns_data,
                                palette->n_columns);

      palette_editor_scroll_top_left (palette_editor);

      palette_editor_invalidate_preview (GIMP_PALETTE (editor->data),
                                         palette_editor);
    }

  g_signal_handlers_unblock_by_func (palette_editor->columns_data,
                                     palette_editor_columns_changed,
                                     editor);
}

static void
gimp_palette_editor_set_context (GimpDocked  *docked,
                                 GimpContext *context)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  gimp_view_renderer_set_context (GIMP_VIEW (editor->view)->renderer,
                                  context);
}

#define AUX_INFO_ZOOM_FACTOR "zoom-factor"

static void
gimp_palette_editor_set_aux_info (GimpDocked *docked,
                                  GList      *aux_info)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (docked);
  GList             *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      GimpSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_ZOOM_FACTOR))
        {
          gdouble zoom_factor;

          zoom_factor = g_ascii_strtod (aux->value, NULL);

          editor->zoom_factor = CLAMP (zoom_factor, 0.1, 4.0);
        }
    }
}

static GList *
gimp_palette_editor_get_aux_info (GimpDocked *docked)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (docked);
  GList             *aux_info;

  aux_info = parent_docked_iface->get_aux_info (docked);

  if (editor->zoom_factor != 1.0)
    {
      GimpSessionInfoAux *aux;
      gchar               value[G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_formatd (value, sizeof (value), "%.2f", editor->zoom_factor);

      aux = gimp_session_info_aux_new (AUX_INFO_ZOOM_FACTOR, value);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}


/*  public functions  */

GtkWidget *
gimp_palette_editor_new (GimpContext     *context,
                         GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return g_object_new (GIMP_TYPE_PALETTE_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<PaletteEditor>",
                       "ui-path",         "/palette-editor-popup",
                       "data-factory",    context->gimp->palette_factory,
                       "context",         context,
                       "data",            gimp_context_get_palette (context),
                       NULL);
}

void
gimp_palette_editor_pick_color (GimpPaletteEditor  *editor,
                                const GimpRGB      *color,
                                GimpColorPickState  pick_state)
{
  g_return_if_fail (GIMP_IS_PALETTE_EDITOR (editor));
  g_return_if_fail (color != NULL);

  if (GIMP_DATA_EDITOR (editor)->data_editable)
    {
      GimpPaletteEntry *entry;
      GimpData         *data;
      gint              index = -1;

      data = gimp_data_editor_get_data (GIMP_DATA_EDITOR (editor));

      switch (pick_state)
        {
        case GIMP_COLOR_PICK_STATE_NEW:
          if (editor->color)
            index = editor->color->position + 1;

          entry = gimp_palette_add_entry (GIMP_PALETTE (data), index,
                                          NULL, color);
          gimp_palette_view_select_entry (GIMP_PALETTE_VIEW (editor->view),
                                          entry);
          break;

        case GIMP_COLOR_PICK_STATE_UPDATE:
          editor->color->color = *color;
          gimp_data_dirty (data);
          break;
        }
    }
}

void
gimp_palette_editor_zoom (GimpPaletteEditor  *editor,
                          GimpZoomType        zoom_type)
{
  GimpPalette *palette;
  gdouble      zoom_factor;

  g_return_if_fail (GIMP_IS_PALETTE_EDITOR (editor));

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  if (! palette)
    return;

  zoom_factor = editor->zoom_factor;

  switch (zoom_type)
    {
    case GIMP_ZOOM_IN_MAX:
    case GIMP_ZOOM_IN_MORE:
    case GIMP_ZOOM_IN:
      zoom_factor += 0.1;
      break;

    case GIMP_ZOOM_OUT_MORE:
    case GIMP_ZOOM_OUT:
      zoom_factor -= 0.1;
      break;

    case GIMP_ZOOM_OUT_MAX:
    case GIMP_ZOOM_TO: /* abused as ZOOM_ALL */
      {
        gint height  = editor->view->parent->parent->parent->allocation.height;
        gint columns = palette->n_columns ? palette->n_columns : COLUMNS;
        gint rows;

        rows = palette->n_colors / columns;

        if (palette->n_colors % columns)
          rows += 1;

        rows = MAX (1, rows);

        zoom_factor = (((gdouble) height - 2 * SPACING) /
                       (gdouble) rows - SPACING) / ENTRY_HEIGHT;
      }
      break;
    }

  zoom_factor = CLAMP (zoom_factor, 0.1, 4.0);

  if (palette->n_columns)
    editor->columns = palette->n_columns;
  else
    editor->columns = COLUMNS;

  palette_editor_resize (editor, editor->last_width, zoom_factor);

  palette_editor_scroll_top_left (editor);
}

gint
gimp_palette_editor_get_index (GimpPaletteEditor *editor,
                               const GimpRGB     *search)
{
  GimpPalette *palette;

  g_return_val_if_fail (GIMP_IS_PALETTE_EDITOR (editor), -1);
  g_return_val_if_fail (search != NULL, -1);

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  if (palette && palette->n_colors > 0)
    {
      GimpPaletteEntry *entry;

      entry = gimp_palette_find_entry (palette, search, editor->color);

      if (entry)
        return entry->position;
    }

  return -1;
}

gboolean
gimp_palette_editor_set_index (GimpPaletteEditor *editor,
                               gint               index,
                               GimpRGB           *color)
{
  GimpPalette *palette;
  GList       *list;

  g_return_val_if_fail (GIMP_IS_PALETTE_EDITOR (editor), FALSE);

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  if (! palette || palette->n_colors == 0)
    return FALSE;

  index = CLAMP (index, 0, palette->n_colors - 1);

  list = g_list_nth (palette->colors, index);

  gimp_palette_view_select_entry (GIMP_PALETTE_VIEW (editor->view),
                                  list->data);

  if (color)
    *color = editor->color->color;

  return TRUE;
}

gint
gimp_palette_editor_max_index (GimpPaletteEditor *editor)
{
  GimpPalette *palette;

  g_return_val_if_fail (GIMP_IS_PALETTE_EDITOR (editor), -1);

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  if (! palette || palette->n_colors == 0)
    return -1;

  return palette->n_colors - 1;
}


/*  private functions  */

static void
palette_editor_invalidate_preview (GimpPalette       *palette,
                                   GimpPaletteEditor *editor)
{
  if (palette->n_columns)
    editor->columns = palette->n_columns;
  else
    editor->columns = COLUMNS;

  palette_editor_resize (editor, editor->last_width, editor->zoom_factor);
}

static void
palette_editor_viewport_size_allocate (GtkWidget         *widget,
                                       GtkAllocation     *allocation,
                                       GimpPaletteEditor *editor)
{
  if (widget->allocation.width != editor->last_width)
    {
      palette_editor_resize (editor, widget->allocation.width,
                             editor->zoom_factor);
    }
}

static gboolean
palette_editor_eventbox_button_press (GtkWidget         *widget,
                                      GdkEventButton    *bevent,
                                      GimpPaletteEditor *editor)
{
  if (bevent->button == 3 && bevent->type == GDK_BUTTON_PRESS)
    {
      return gimp_editor_popup_menu (GIMP_EDITOR (editor), NULL, NULL);
    }

  return TRUE;
}

static void
palette_editor_drop_color (GtkWidget     *widget,
                           gint           x,
                           gint           y,
                           const GimpRGB *color,
                           gpointer       data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  if (GIMP_DATA_EDITOR (editor)->data_editable)
    {
      GimpPalette      *palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);
      GimpPaletteEntry *entry;

      entry = gimp_palette_add_entry (palette, -1, NULL, color);
      gimp_palette_view_select_entry (GIMP_PALETTE_VIEW (editor->view), entry);
    }
}

static void
palette_editor_drop_palette (GtkWidget    *widget,
                             gint          x,
                             gint          y,
                             GimpViewable *viewable,
                             gpointer      data)
{
  gimp_data_editor_set_data (GIMP_DATA_EDITOR (data), GIMP_DATA (viewable));
}


/*  palette view callbacks  */

static void
palette_editor_entry_clicked (GimpPaletteView   *view,
                              GimpPaletteEntry  *entry,
                              GdkModifierType    state,
                              GimpPaletteEditor *editor)
{
  if (entry)
    {
      GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);

      if (state & GDK_CONTROL_MASK)
        gimp_context_set_background (data_editor->context, &entry->color);
      else
        gimp_context_set_foreground (data_editor->context, &entry->color);
    }
}

static void
palette_editor_entry_selected (GimpPaletteView   *view,
                               GimpPaletteEntry  *entry,
                               GimpPaletteEditor *editor)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);
  GimpPalette    *palette;

  palette = GIMP_PALETTE (data_editor->data);

  if (editor->color != entry)
    {
      editor->color = entry;

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

      gimp_ui_manager_update (GIMP_EDITOR (editor)->ui_manager,
                              GIMP_EDITOR (editor)->popup_data);
    }
}

static void
palette_editor_entry_activated (GimpPaletteView   *view,
                                GimpPaletteEntry  *entry,
                                GimpPaletteEditor *editor)
{
  if (GIMP_DATA_EDITOR (editor)->data_editable && entry == editor->color)
    {
      gimp_ui_manager_activate_action (GIMP_EDITOR (editor)->ui_manager,
                                       "palette-editor",
                                       "palette-editor-edit-color");
    }
}

static void
palette_editor_entry_context (GimpPaletteView   *view,
                              GimpPaletteEntry  *entry,
                              GimpPaletteEditor *editor)
{
  gimp_editor_popup_menu (GIMP_EDITOR (editor), NULL, NULL);
}

static void
palette_editor_color_dropped (GimpPaletteView   *view,
                              GimpPaletteEntry  *entry,
                              const GimpRGB     *color,
                              GimpPaletteEditor *editor)
{
  if (GIMP_DATA_EDITOR (editor)->data_editable)
    {
      GimpPalette *palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);
      gint         pos     = -1;

      if (entry)
        pos = entry->position;

      entry = gimp_palette_add_entry (palette, pos, NULL, color);
      gimp_palette_view_select_entry (GIMP_PALETTE_VIEW (editor->view), entry);
    }
}


/*  color name and columns callbacks  */

static void
palette_editor_color_name_changed (GtkWidget         *widget,
                                   GimpPaletteEditor *editor)
{
  if (GIMP_DATA_EDITOR (editor)->data)
    {
      GimpPalette *palette;

      palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

      if (editor->color->name)
        g_free (editor->color->name);

      editor->color->name =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (editor->color_name)));

      gimp_data_dirty (GIMP_DATA (palette));
    }
}

static void
palette_editor_columns_changed (GtkAdjustment     *adj,
                                GimpPaletteEditor *editor)
{
  if (GIMP_DATA_EDITOR (editor)->data)
    {
      GimpPalette *palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

      gimp_palette_set_columns (palette, ROUND (adj->value));
    }
}


/*  misc utils  */

static void
palette_editor_resize (GimpPaletteEditor *editor,
                       gint               width,
                       gdouble            zoom_factor)
{
  GimpPalette *palette;
  gint         rows;
  gint         preview_width;
  gint         preview_height;

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  if (! palette)
    return;

  editor->zoom_factor = zoom_factor;
  editor->last_width  = width;
  editor->col_width   = width / (editor->columns + 1) - SPACING;

  if (editor->col_width < 0)
    editor->col_width = 0;

  rows = palette->n_colors / editor->columns;
  if (palette->n_colors % editor->columns)
    rows += 1;

  preview_width  = (editor->col_width + SPACING) * editor->columns;
  preview_height = (rows *
                    (SPACING + (gint) (ENTRY_HEIGHT * editor->zoom_factor)));

  if (preview_height > GIMP_VIEWABLE_MAX_PREVIEW_SIZE)
    preview_height = ((GIMP_VIEWABLE_MAX_PREVIEW_SIZE - SPACING) / rows) * rows;

  gimp_view_renderer_set_size_full (GIMP_VIEW (editor->view)->renderer,
                                    preview_width  + SPACING,
                                    preview_height + SPACING, 0);
}

static void
palette_editor_scroll_top_left (GimpPaletteEditor *palette_editor)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (palette_editor);
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
