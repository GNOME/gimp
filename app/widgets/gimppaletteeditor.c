/* The GIMP -- an image manipulation program
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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"
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
#include "gimpsessioninfo.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define ENTRY_WIDTH  12
#define ENTRY_HEIGHT 10
#define SPACING       1
#define COLUMNS      16
#define ROWS         11

#define PREVIEW_WIDTH  ((ENTRY_WIDTH  * COLUMNS) + (SPACING * (COLUMNS + 1)))
#define PREVIEW_HEIGHT ((ENTRY_HEIGHT * ROWS)    + (SPACING * (ROWS    + 1)))

#define PALETTE_EVENT_MASK (GDK_EXPOSURE_MASK     | \
                            GDK_BUTTON_PRESS_MASK | \
                            GDK_ENTER_NOTIFY_MASK)


/*  local function prototypes  */

static void   gimp_palette_editor_class_init (GimpPaletteEditorClass *klass);
static void   gimp_palette_editor_init       (GimpPaletteEditor      *editor);

static void   gimp_palette_editor_docked_iface_init (GimpDockedInterface *docked_iface);

static GObject * gimp_palette_editor_constructor   (GType              type,
                                                    guint              n_params,
                                                    GObjectConstructParam *params);

static void   gimp_palette_editor_set_aux_info     (GimpDocked        *docked,
                                                    GList             *aux_info);
static GList *gimp_palette_editor_get_aux_info     (GimpDocked        *docked);

static void   gimp_palette_editor_destroy          (GtkObject         *object);
static void   gimp_palette_editor_unmap            (GtkWidget         *widget);

static void   gimp_palette_editor_set_data         (GimpDataEditor    *editor,
                                                    GimpData          *data);

static gint   palette_editor_eventbox_button_press (GtkWidget         *widget,
						    GdkEventButton    *bevent,
						    GimpPaletteEditor *editor);
static gint palette_editor_color_area_button_press (GtkWidget         *widget,
						    GdkEventButton    *bevent,
						    GimpPaletteEditor *editor);
static void   palette_editor_draw_entries          (GimpPaletteEditor *editor,
                                                    gint               row_start,
                                                    gint               column_highlight);
static void   palette_editor_redraw                (GimpPaletteEditor *editor,
                                                    gint               width,
                                                    gdouble            zoom_factor);
static void   palette_editor_scroll_top_left       (GimpPaletteEditor *editor);

static void   palette_editor_select_entry          (GimpPaletteEditor *editor,
                                                    GimpPaletteEntry  *entry);
static void   palette_editor_color_name_changed    (GtkWidget         *widget,
                                                    GimpPaletteEditor *editor);
static void   palette_editor_columns_changed       (GtkAdjustment     *adj,
                                                    GimpPaletteEditor *editor);

static void   palette_editor_drag_color            (GtkWidget         *widget,
                                                    GimpRGB           *color,
                                                    gpointer           data);
static void   palette_editor_drop_color            (GtkWidget         *widget,
                                                    const GimpRGB     *color,
                                                    gpointer           data);
static void   palette_editor_drop_palette          (GtkWidget         *widget,
                                                    GimpViewable      *viewable,
                                                    gpointer           data);
static void   palette_editor_invalidate_preview    (GimpPalette       *palette,
                                                    GimpPaletteEditor *editor);
static void   palette_editor_viewport_resized      (GtkWidget         *widget,
                                                    GtkAllocation     *allocation,
                                                    GimpPaletteEditor *editor);
static void   palette_editor_viewport_realize      (GtkWidget         *widget,
                                                    GimpPaletteEditor *editor);


static GimpDataEditorClass *parent_class        = NULL;
static GimpDockedInterface *parent_docked_iface = NULL;


GType
gimp_palette_editor_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpPaletteEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_palette_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpPaletteEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_palette_editor_init,
      };
      static const GInterfaceInfo docked_iface_info =
      {
        (GInterfaceInitFunc) gimp_palette_editor_docked_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      type = g_type_register_static (GIMP_TYPE_DATA_EDITOR,
                                     "GimpPaletteEditor",
                                     &info, 0);

      g_type_add_interface_static (type, GIMP_TYPE_DOCKED,
                                   &docked_iface_info);
    }

  return type;
}

static void
gimp_palette_editor_class_init (GimpPaletteEditorClass *klass)
{
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass      *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass      *widget_class     = GTK_WIDGET_CLASS (klass);
  GimpDataEditorClass *editor_class     = GIMP_DATA_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_palette_editor_constructor;

  gtk_object_class->destroy  = gimp_palette_editor_destroy;

  widget_class->unmap        = gimp_palette_editor_unmap;

  editor_class->set_data     = gimp_palette_editor_set_data;
}

static void
gimp_palette_editor_init (GimpPaletteEditor *editor)
{
  GtkWidget *scrolled_win;
  GtkWidget *eventbox;
  GtkWidget *alignment;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *spinbutton;

  editor->zoom_factor   = 1.0;
  editor->col_width     = 0;
  editor->last_width    = 0;
  editor->columns       = COLUMNS;
  editor->columns_valid = TRUE;

  editor->scrolled_window = scrolled_win =
    gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_win, -1, PREVIEW_HEIGHT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (editor), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  eventbox = gtk_event_box_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
					 eventbox);
  gtk_widget_show (eventbox);

  g_signal_connect (eventbox, "button_press_event",
		    G_CALLBACK (palette_editor_eventbox_button_press),
		    editor);
  g_signal_connect (eventbox->parent, "size_allocate",
                    G_CALLBACK (palette_editor_viewport_resized),
                    editor);
  g_signal_connect (eventbox->parent, "realize",
                    G_CALLBACK (palette_editor_viewport_realize),
                    editor);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (eventbox), alignment);
  gtk_widget_show (alignment);

  editor->color_area = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (editor->color_area), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (editor->color_area),
                    PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (editor->color_area, PALETTE_EVENT_MASK);
  gtk_container_add (GTK_CONTAINER (alignment), editor->color_area);
  gtk_widget_show (editor->color_area);

  g_signal_connect (editor->color_area, "button_press_event",
		    G_CALLBACK (palette_editor_color_area_button_press),
		    editor);

  gimp_dnd_color_source_add (editor->color_area,
                             palette_editor_drag_color,
			     editor);
  gimp_dnd_color_dest_add (eventbox, palette_editor_drop_color, editor);
  gimp_dnd_viewable_dest_add (eventbox, GIMP_TYPE_PALETTE,
			      palette_editor_drop_palette,
                              editor);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The color name entry  */
  editor->color_name = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), editor->color_name, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (editor->color_name), _("Undefined"));
  gtk_widget_set_sensitive (editor->color_name, FALSE);
  gtk_widget_show (editor->color_name);

  g_signal_connect (editor->color_name, "changed",
                    G_CALLBACK (palette_editor_color_name_changed),
                    editor);

  label = gtk_label_new (_("Columns:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new ((GtkObject **) &editor->columns_data,
                                     0, 0, 64, 1, 4, 4, 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (editor->columns_data, "value_changed",
                    G_CALLBACK (palette_editor_columns_changed),
                    editor);
}

static void
gimp_palette_editor_docked_iface_init (GimpDockedInterface *docked_iface)
{
  parent_docked_iface = g_type_interface_peek_parent (docked_iface);

  docked_iface->set_aux_info = gimp_palette_editor_set_aux_info;
  docked_iface->get_aux_info = gimp_palette_editor_get_aux_info;
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

  editor->edit_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                   "palette-editor-edit-color", NULL);

  editor->new_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                   "palette-editor-new-color-fg",
                                   "palette-editor-new-color-bg",
                                   GDK_CONTROL_MASK,
                                   NULL);

  editor->delete_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                   "palette-editor-delete-color", NULL);

  editor->zoom_out_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                   "palette-editor-zoom-out", NULL);

  editor->zoom_in_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                   "palette-editor-zoom-in", NULL);

  editor->zoom_all_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "palette-editor",
                                   "palette-editor-zoom-all", NULL);

  return object;
}

#define AUX_INFO_ZOOM_FACTOR "zoom-factor"

static void
gimp_palette_editor_set_aux_info (GimpDocked *docked,
                                  GList      *aux_info)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (docked);
  GList             *list;

  if (parent_docked_iface->set_aux_info)
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
  GimpPaletteEditor *editor   = GIMP_PALETTE_EDITOR (docked);
  GList             *aux_info = NULL;

  if (parent_docked_iface->get_aux_info)
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

      palette_editor->columns_valid = FALSE;

      palette_editor_select_entry (palette_editor, NULL);

      gtk_adjustment_set_value (palette_editor->columns_data, 0);
    }

  GIMP_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    {
      GimpPalette *palette = GIMP_PALETTE (editor->data);

      g_signal_connect (editor->data, "invalidate_preview",
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

/*  public functions  */

GtkWidget *
gimp_palette_editor_new (Gimp            *gimp,
                         GimpMenuFactory *menu_factory)
{
  GimpPalette *palette;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  palette = gimp_context_get_palette (gimp_get_user_context (gimp));

  return g_object_new (GIMP_TYPE_PALETTE_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<PaletteEditor>",
                       "ui-path",         "/palette-editor-popup",
                       "data-factory",    gimp->palette_factory,
                       "data",            palette,
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
      GimpData *data = gimp_data_editor_get_data (GIMP_DATA_EDITOR (editor));

      switch (pick_state)
        {
        case GIMP_COLOR_PICK_STATE_NEW:
          editor->color = gimp_palette_add_entry (GIMP_PALETTE (data),
                                                  NULL, color);
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
    case GIMP_ZOOM_IN:
      zoom_factor += 0.1;
      break;

    case GIMP_ZOOM_OUT:
      zoom_factor -= 0.1;
      break;

    case GIMP_ZOOM_TO: /* abused as ZOOM_ALL */
      {
        gint height;
        gint rows;

        height = editor->color_area->parent->parent->parent->allocation.height;

        if (palette->n_columns)
          rows = palette->n_colors / palette->n_columns;
        else
          rows = palette->n_colors / COLUMNS;

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

  editor->columns_valid = FALSE;
  palette_editor_redraw (editor, editor->last_width, zoom_factor);

  palette_editor_scroll_top_left (editor);
}


/*  private functions  */

/*  the color area event callbacks  ******************************************/

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

static gboolean
palette_editor_color_area_button_press (GtkWidget         *widget,
                                        GdkEventButton    *bevent,
                                        GimpPaletteEditor *editor)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);
  GimpPalette    *palette;
  GimpContext    *user_context;
  GList          *list;
  gint            entry_width;
  gint            entry_height;
  gint            row, col;
  gint            pos;

  palette = GIMP_PALETTE (data_editor->data);

  user_context = gimp_get_user_context (data_editor->data_factory->gimp);

  entry_width  = editor->col_width + SPACING;
  entry_height = (ENTRY_HEIGHT * editor->zoom_factor) +  SPACING;

  col = (bevent->x - 1) / entry_width;
  row = (bevent->y - 1) / entry_height;
  pos = row * editor->columns + col;

  if (palette)
    list = g_list_nth (palette->colors, pos);
  else
    list = NULL;

  if (list)
    editor->dnd_color = list->data;
  else
    editor->dnd_color = NULL;

  if (! palette)
    return FALSE;

  if (bevent->type == GDK_BUTTON_PRESS &&
      (bevent->button == 1 || bevent->button == 3))
    {
      palette_editor_select_entry (editor, list ? list->data : NULL);

      if (list)
        {
          if (bevent->state & GDK_CONTROL_MASK)
            gimp_context_set_background (user_context,
                                         &editor->color->color);
          else
            gimp_context_set_foreground (user_context,
                                         &editor->color->color);

          palette_editor_draw_entries (editor, row, col);
        }
    }
  else if (data_editor->data_editable      &&
           list                            &&
           list->data     == editor->color &&
           bevent->button == 1             &&
           bevent->type   == GDK_2BUTTON_PRESS)
    {
      GtkAction *action;

      action = gimp_ui_manager_find_action (GIMP_EDITOR (editor)->ui_manager,
                                            "palette-editor",
                                            "palette-editor-edit-color");

      if (action)
        gtk_action_activate (action);
    }

  return FALSE; /* continue with eventbox_button_press */
}

/*  functions for drawing & updating the palette dialog color area  **********/

static gint
palette_editor_draw_color_row (guchar            *colors,
			       gint               n_colors,
			       gint               y,
			       gint               column_highlight,
			       guchar            *buffer,
			       GimpPaletteEditor *palette_editor)
{
  guchar    *p;
  guchar     bcolor;
  gint       width, height;
  gint       entry_width;
  gint       entry_height;
  gint       vsize;
  gint       vspacing;
  gint       i, j;
  GtkWidget *preview;

  preview = palette_editor->color_area;

  bcolor = 0;

  width        = preview->requisition.width;
  height       = preview->requisition.height;
  entry_width  = palette_editor->col_width;
  entry_height = (ENTRY_HEIGHT * palette_editor->zoom_factor);

  if ((y >= 0) && ((y + SPACING) < height))
    vspacing = SPACING;
  else if (y < 0)
    vspacing = SPACING + y;
  else
    vspacing = height - y;

  if (vspacing > 0)
    {
      if (y < 0)
	y += SPACING - vspacing;

      for (i = SPACING - vspacing; i < SPACING; i++, y++)
	{
	  p = buffer;
	  for (j = 0; j < width; j++)
	    {
	      *p++ = bcolor;
	      *p++ = bcolor;
	      *p++ = bcolor;
	    }

	  if (column_highlight >= 0)
	    {
	      guchar *ph;

	      ph = &buffer[3 * column_highlight * (entry_width + SPACING)];

	      for (j = 0 ; j <= entry_width + SPACING; j++)
		{
		  *ph++ = ~bcolor;
		  *ph++ = ~bcolor;
		  *ph++ = ~bcolor;
		}

 	      gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0,
				    y + entry_height + 1, width);
	    }

	  gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, y, width);
	}

      if (y > SPACING)
	y += SPACING - vspacing;
    }
  else
    y += SPACING;

  vsize = (y >= 0) ? (entry_height) : (entry_height + y);

  if ((y >= 0) && ((y + entry_height) < height))
    vsize = entry_height;
  else if (y < 0)
    vsize = entry_height + y;
  else
    vsize = height - y;

  if (vsize > 0)
    {
      p = buffer;
      for (i = 0; i < n_colors; i++)
	{
	  for (j = 0; j < SPACING; j++)
	    {
	      *p++ = bcolor;
	      *p++ = bcolor;
	      *p++ = bcolor;
	    }

	  for (j = 0; j < entry_width; j++)
	    {
	      *p++ = colors[i * 3];
	      *p++ = colors[i * 3 + 1];
	      *p++ = colors[i * 3 + 2];
	    }
	}

      for (i = 0; i < (palette_editor->columns - n_colors); i++)
	{
	  for (j = 0; j < (SPACING + entry_width); j++)
	    {
	      *p++ = 0;
	      *p++ = 0;
	      *p++ = 0;
	    }
	}

      for (j = 0; j < SPACING; j++)
	{
	  if (n_colors == column_highlight)
	    {
	      *p++ = ~bcolor;
	      *p++ = ~bcolor;
	      *p++ = ~bcolor;
	    }
	  else
	    {
	      *p++ = bcolor;
	      *p++ = bcolor;
	      *p++ = bcolor;
	    }
	}

      if (y < 0)
	y += entry_height - vsize;
      for (i = 0; i < vsize; i++, y++)
	{
	  if (column_highlight >= 0)
	    {
	      guchar *ph;

	      ph = &buffer[3 * column_highlight * (entry_width + SPACING)];

	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      ph += 3 * (entry_width);
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	    }

	  gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, y, width);
	}
      if (y > entry_height)
	y += entry_height - vsize;
    }
  else
    y += entry_height;

  return y;
}

static void
palette_editor_draw_entries (GimpPaletteEditor *editor,
			     gint               row_start,
			     gint               column_highlight)
{
  GimpPalette      *palette;
  GimpPaletteEntry *entry;
  guchar           *buffer;
  guchar           *colors;
  GList            *list;
  gint              width, height;
  gint              entry_width;
  gint              entry_height;
  gint              index, y;

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  if (! palette)
    return;

  width  = editor->color_area->requisition.width;
  height = editor->color_area->requisition.height;

  entry_width  = editor->col_width;
  entry_height = (ENTRY_HEIGHT * editor->zoom_factor);

  if (entry_width <= 0)
    return;

  colors = g_new (guchar, editor->columns * 3);
  buffer = g_new (guchar, width * 3);

  if (row_start < 0)
    {
      y = 0;
      list = palette->colors;
      column_highlight = -1;
    }
  else
    {
      y = (entry_height + SPACING) * row_start;
      list = g_list_nth (palette->colors,
			 row_start * editor->columns);
    }

  index = 0;

  for (; list; list = g_list_next (list))
    {
      entry = (GimpPaletteEntry *) list->data;

      gimp_rgb_get_uchar (&entry->color,
			  &colors[index * 3],
			  &colors[index * 3 + 1],
			  &colors[index * 3 + 2]);
      index++;

      if (index == editor->columns)
	{
	  index = 0;
	  y = palette_editor_draw_color_row (colors,
                                             editor->columns, y,
					     column_highlight, buffer,
                                             editor);

	  if (y >= height || row_start >= 0)
	    {
	      /* This row only */
	      gtk_widget_queue_draw (editor->color_area);
	      g_free (buffer);
	      g_free (colors);
	      return;
	    }
	}
    }

  while (y < height)
    {
      y = palette_editor_draw_color_row (colors, index, y, column_highlight,
					 buffer, editor);
      index = 0;
      if (row_start >= 0)
	break;
    }

  g_free (buffer);
  g_free (colors);

  gtk_widget_queue_draw (editor->color_area);
}

static void
palette_editor_scroll_top_left (GimpPaletteEditor *palette_editor)
{
  GtkAdjustment *hadj;
  GtkAdjustment *vadj;

  if (! palette_editor->scrolled_window)
    return;

  hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (palette_editor->scrolled_window));
  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (palette_editor->scrolled_window));

  if (hadj)
    gtk_adjustment_set_value (hadj, 0.0);
  if (vadj)
    gtk_adjustment_set_value (vadj, 0.0);
}

static void
palette_editor_redraw (GimpPaletteEditor *editor,
                       gint               width,
                       gdouble            zoom_factor)
{
  GimpPalette *palette;
  gint         vsize;
  gint         nrows;
  gint         n_entries;
  gint         preview_width;

  palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

  if (! palette)
    return;

  if (editor->columns_valid)
    return;

  editor->zoom_factor = zoom_factor;
  editor->last_width  = width;
  editor->col_width   = width / (editor->columns + 1) - SPACING;

  if (editor->col_width < 0)
    editor->col_width = 0;

  editor->columns_valid = TRUE;

  n_entries = palette->n_colors;

  nrows = n_entries / editor->columns;
  if (n_entries % editor->columns)
    nrows += 1;

  vsize = nrows * (SPACING + (gint) (ENTRY_HEIGHT * editor->zoom_factor)) + SPACING;

  preview_width = (editor->col_width + SPACING) * editor->columns + SPACING;

  gtk_preview_size (GTK_PREVIEW (editor->color_area), preview_width, vsize);
  gtk_widget_queue_resize (editor->color_area);

  palette_editor_draw_entries (editor, -1, -1);

  if (editor->color)
    palette_editor_draw_entries (editor,
                                 editor->color->position / editor->columns,
                                 editor->color->position % editor->columns);
}

static void
palette_editor_select_entry (GimpPaletteEditor *editor,
                             GimpPaletteEntry  *entry)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);
  GimpPalette    *palette;

  palette = GIMP_PALETTE (data_editor->data);

  if (editor->color != entry)
    {
      if (editor->color)
        palette_editor_draw_entries (editor, -1, -1);

      editor->color = entry;

      g_signal_handlers_block_by_func (editor->color_name,
                                       palette_editor_color_name_changed,
                                       editor);

      gtk_entry_set_text (GTK_ENTRY (editor->color_name),
                          entry ? entry->name : _("Undefined"));

      g_signal_handlers_unblock_by_func (editor->color_name,
                                         palette_editor_color_name_changed,
                                         editor);

      gtk_widget_set_sensitive (editor->color_name,
                                entry && data_editor->data_editable);

      gimp_ui_manager_update (GIMP_EDITOR (editor)->ui_manager,
                              GIMP_EDITOR (editor)->popup_data);
    }
}


/*  the color name entry callback  *******************************************/

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

/*  the columns spinbutton callback  *****************************************/

static void
palette_editor_columns_changed (GtkAdjustment     *adj,
                                GimpPaletteEditor *editor)
{
  if (GIMP_DATA_EDITOR (editor)->data)
    {
      GimpPalette *palette = GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data);

      gimp_palette_set_n_columns (palette, ROUND (adj->value));
    }
}

/*  the palette dialog color dnd callbacks  **********************************/

static void
palette_editor_drag_color (GtkWidget *widget,
			   GimpRGB   *color,
			   gpointer   data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  if (GIMP_DATA_EDITOR (editor)->data && editor->dnd_color)
    {
      *color = editor->dnd_color->color;
    }
  else
    {
      gimp_rgba_set (color, 0.0, 0.0, 0.0, 1.0);
    }
}

static void
palette_editor_drop_color (GtkWidget     *widget,
			   const GimpRGB *color,
			   gpointer       data)
{
  GimpPaletteEditor *editor = GIMP_PALETTE_EDITOR (data);

  if (GIMP_DATA_EDITOR (editor)->data_editable)
    {
      editor->color =
	gimp_palette_add_entry (GIMP_PALETTE (GIMP_DATA_EDITOR (editor)->data),
                                NULL,
				(GimpRGB *) color);
    }
}

static void
palette_editor_drop_palette (GtkWidget    *widget,
			     GimpViewable *viewable,
			     gpointer      data)
{
  gimp_data_editor_set_data (GIMP_DATA_EDITOR (data), GIMP_DATA (viewable));
}

static void
palette_editor_invalidate_preview (GimpPalette       *palette,
				   GimpPaletteEditor *editor)
{
  if (editor->color)
    {
      GimpPaletteEntry *entry = editor->color;

      if (g_list_find (palette->colors, entry))
        {
          editor->color = NULL;
          palette_editor_select_entry (editor, entry);
        }
      else
        {
          palette_editor_select_entry (editor, NULL);
        }
    }

  if (editor->dnd_color && ! g_list_find (palette->colors, editor->dnd_color))
    editor->dnd_color = NULL;

  if (palette->n_columns)
    editor->columns = palette->n_columns;
  else
    editor->columns = COLUMNS;

  editor->columns_valid = FALSE;
  palette_editor_redraw (editor, editor->last_width, editor->zoom_factor);
}

static void
palette_editor_viewport_resized (GtkWidget         *widget,
                                 GtkAllocation     *allocation,
                                 GimpPaletteEditor *editor)
{
  if (widget->allocation.width != editor->last_width)
    {
      editor->columns_valid = FALSE;
      palette_editor_redraw (editor, widget->allocation.width,
                             editor->zoom_factor);
    }
}

static void
palette_editor_viewport_realize (GtkWidget         *widget,
                                 GimpPaletteEditor *editor)
{
  editor->columns_valid = FALSE;
  palette_editor_redraw (editor, widget->allocation.width, editor->zoom_factor);
}
