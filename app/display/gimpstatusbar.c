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

#include <gtk/gtk.h>

#include "display-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-unit.h"

#include "widgets/gimpunitstore.h"
#include "widgets/gimpunitcombobox.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpstatusbar.h"

#include "gimp-intl.h"


/* maximal width of the string holding the cursor-coordinates for
 * the status line
 */
#define CURSOR_STR_LENGTH 256


static void   gimp_statusbar_class_init   (GimpStatusbarClass *klass);
static void   gimp_statusbar_init         (GimpStatusbar      *statusbar);

static void   gimp_statusbar_update       (GtkStatusbar       *gtk_statusbar,
                                           guint               context_id,
                                           const gchar        *text);
static void   gimp_statusbar_set_cursor   (GimpStatusbar      *statusbar,
                                           gdouble             x,
                                           gdouble             y);
static void   gimp_statusbar_unit_changed (GtkWidget          *combo,
                                           GimpStatusbar      *statusbar);


static GtkStatusbarClass *parent_class = NULL;


GType
gimp_statusbar_get_type (void)
{
  static GType statusbar_type = 0;

  if (! statusbar_type)
    {
      static const GTypeInfo statusbar_info =
      {
        sizeof (GimpStatusbarClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_statusbar_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpStatusbar),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_statusbar_init,
      };

      statusbar_type = g_type_register_static (GTK_TYPE_STATUSBAR,
                                               "GimpStatusbar",
                                               &statusbar_info, 0);
    }

  return statusbar_type;
}

static void
gimp_statusbar_class_init (GimpStatusbarClass *klass)
{
  GtkStatusbarClass *gtk_statusbar_class;

  parent_class = g_type_class_peek_parent (klass);

  gtk_statusbar_class = GTK_STATUSBAR_CLASS (klass);

  gtk_statusbar_class->text_pushed = gimp_statusbar_update;
  gtk_statusbar_class->text_popped = gimp_statusbar_update;
}

static void
gimp_statusbar_init (GimpStatusbar *statusbar)
{
  GtkStatusbar  *gtk_statusbar;
  GtkBox        *box;
  GtkWidget     *hbox;
  GimpUnitStore *store;
  GtkShadowType  shadow_type;

  gtk_statusbar = GTK_STATUSBAR (statusbar);
  box           = GTK_BOX (statusbar);

  gtk_widget_hide (gtk_statusbar->frame);

  statusbar->shell                 = NULL;
  statusbar->cursor_format_str[0]  = '\0';
  statusbar->progressid            = 0;

  gtk_widget_style_get (GTK_WIDGET (statusbar),
                        "shadow_type", &shadow_type,
                        NULL);

  statusbar->cursor_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (statusbar->cursor_frame), shadow_type);
  gtk_box_pack_start (box, statusbar->cursor_frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (box, statusbar->cursor_frame, 0);
  gtk_widget_show (statusbar->cursor_frame);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (statusbar->cursor_frame), hbox);
  gtk_widget_show (hbox);

  statusbar->cursor_label = gtk_label_new ("0, 0");
  gtk_misc_set_alignment (GTK_MISC (statusbar->cursor_label), 0.5, 0.5);
  gtk_container_add (GTK_CONTAINER (hbox), statusbar->cursor_label);
  gtk_widget_show (statusbar->cursor_label);

  store = gimp_unit_store_new (2);
  statusbar->unit_combo = gimp_unit_combo_box_new_with_model (store);
  g_object_unref (store);

  gtk_container_add (GTK_CONTAINER (hbox), statusbar->unit_combo);
  gtk_widget_show (statusbar->unit_combo);

  g_signal_connect (statusbar->unit_combo, "changed",
                    G_CALLBACK (gimp_statusbar_unit_changed),
                    statusbar);

  statusbar->progressbar = gtk_progress_bar_new ();
  gtk_box_pack_start (box, statusbar->progressbar, TRUE, TRUE, 0);
  gtk_widget_show (statusbar->progressbar);

  GTK_PROGRESS_BAR (statusbar->progressbar)->progress.x_align = 0.0;
  GTK_PROGRESS_BAR (statusbar->progressbar)->progress.y_align = 0.5;

  statusbar->cancelbutton = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_sensitive (statusbar->cancelbutton, FALSE);
  gtk_box_pack_start (box, statusbar->cancelbutton, FALSE, FALSE, 0);
  gtk_widget_show (statusbar->cancelbutton);

  /* Update the statusbar once to work around a canvas size problem:
   *
   *  The first update of the statusbar used to queue a resize which
   *  in term caused the canvas to be resized. That made it shrink by
   *  one pixel in height resulting in the last row not being displayed.
   *  Shrink-wrapping the display used to fix this reliably. With the
   *  next call the resize doesn't seem to happen any longer.
   */

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (statusbar->progressbar),
                             "GIMP");
}

static void
gimp_statusbar_update (GtkStatusbar *gtk_statusbar,
                       guint         context_id,
                       const gchar  *text)
{
  GimpStatusbar *statusbar;

  statusbar = GIMP_STATUSBAR (gtk_statusbar);

  if (! text)
    text = "";

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (statusbar->progressbar), text);
}

GtkWidget *
gimp_statusbar_new (GimpDisplayShell *shell)
{
  GimpStatusbar *statusbar;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  statusbar = g_object_new (GIMP_TYPE_STATUSBAR, NULL);

  statusbar->shell = shell;

  return GTK_WIDGET (statusbar);
}

void
gimp_statusbar_push (GimpStatusbar *statusbar,
                     const gchar   *context_id,
                     const gchar   *message)
{
  guint context_uint;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  context_uint = gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar),
                                               context_id);

  gtk_statusbar_push (GTK_STATUSBAR (statusbar), context_uint, message);
}

void
gimp_statusbar_push_coords (GimpStatusbar *statusbar,
                            const gchar   *context_id,
                            const gchar   *title,
                            gdouble        x,
                            const gchar   *separator,
                            gdouble        y)
{
  GimpImage *gimage;
  gchar      buf[CURSOR_STR_LENGTH];

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (title != NULL);
  g_return_if_fail (separator != NULL);

  gimage = statusbar->shell->gdisp->gimage;

  if (gimage->unit == GIMP_UNIT_PIXEL)
    {
      g_snprintf (buf, sizeof (buf), statusbar->cursor_format_str,
		  title,
                  ROUND (x),
                  separator,
                  ROUND (y));
    }
  else /* show real world units */
    {
      gdouble unit_factor = gimp_image_unit_get_factor (gimage);

      g_snprintf (buf, sizeof (buf), statusbar->cursor_format_str,
		  title,
		  x * unit_factor / gimage->xresolution,
		  separator,
		  y * unit_factor / gimage->yresolution);
    }

  gimp_statusbar_push (statusbar, context_id, buf);
}

void
gimp_statusbar_pop (GimpStatusbar *statusbar,
                    const gchar   *context_id)
{
  guint context_uint;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  context_uint = gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar),
                                               context_id);

  gtk_statusbar_pop (GTK_STATUSBAR (statusbar), context_uint);
}

void
gimp_statusbar_update_cursor (GimpStatusbar *statusbar,
                              gdouble        x,
                              gdouble        y)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  shell = statusbar->shell;

  if (x < 0 ||
      y < 0 ||
      x >= shell->gdisp->gimage->width ||
      y >= shell->gdisp->gimage->height)
    {
      gtk_label_set_text (GTK_LABEL (statusbar->cursor_label), "");
    }
  else
    {
      gimp_statusbar_set_cursor (statusbar, x, y);
    }
}

void
gimp_statusbar_resize_cursor (GimpStatusbar *statusbar)
{
  static PangoLayout *layout = NULL;

  GimpDisplayShell *shell;
  GimpImage        *gimage;
  GtkTreeModel     *model;
  const gchar      *text;
  gint              cursor_label_width;
  gint              label_frame_size_difference;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  shell  = statusbar->shell;
  gimage = shell->gdisp->gimage;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (statusbar->unit_combo));
  gimp_unit_store_set_resolutions (GIMP_UNIT_STORE (model),
                                   gimage->xresolution, gimage->yresolution);

  g_signal_handlers_block_by_func (statusbar->unit_combo,
                                   gimp_statusbar_unit_changed, statusbar);
  gtk_combo_box_set_active (GTK_COMBO_BOX (statusbar->unit_combo),
                            gimage->unit);
  g_signal_handlers_unblock_by_func (statusbar->unit_combo,
                                     gimp_statusbar_unit_changed, statusbar);

  if (gimage->unit == GIMP_UNIT_PIXEL)
    {
      g_snprintf (statusbar->cursor_format_str,
                  sizeof (statusbar->cursor_format_str),
		  "%%s%%d%%s%%d");
    }
  else /* show real world units */
    {
      g_snprintf (statusbar->cursor_format_str,
                  sizeof (statusbar->cursor_format_str),
		  "%%s%%.%df%%s%%.%df",
                  gimp_image_unit_get_digits (gimage),
                  gimp_image_unit_get_digits (gimage));
    }

  gimp_statusbar_set_cursor (statusbar, gimage->width, gimage->height);

  text = gtk_label_get_text (GTK_LABEL (statusbar->cursor_label));

  /* one static layout for all displays should be fine */
  if (! layout)
    layout = gtk_widget_create_pango_layout (statusbar->cursor_label, text);
  else
    pango_layout_set_text (layout, text, -1);

  pango_layout_get_pixel_size (layout, &cursor_label_width, NULL);

  /*  find out how many pixels the label's parent frame is bigger than
   *  the label itself
   */
  label_frame_size_difference = (statusbar->cursor_frame->allocation.width -
                                 statusbar->cursor_label->allocation.width);

  gtk_widget_set_size_request (statusbar->cursor_label, cursor_label_width, -1);

  /* don't resize if this is a new display */
  if (label_frame_size_difference)
    gtk_widget_set_size_request (statusbar->cursor_frame,
                                 cursor_label_width +
                                 label_frame_size_difference,
                                 -1);
}

static void
gimp_statusbar_set_cursor (GimpStatusbar *statusbar,
                           gdouble        x,
                           gdouble        y)
{
  GimpImage     *image = statusbar->shell->gdisp->gimage;
  GtkTreeModel  *model;
  GimpUnitStore *store;
  gchar          buffer[CURSOR_STR_LENGTH];

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (statusbar->unit_combo));
  store = GIMP_UNIT_STORE (model);

  gimp_unit_store_set_pixel_values (store, x, y);

  if (image->unit == GIMP_UNIT_PIXEL)
    {
      g_snprintf (buffer, sizeof (buffer), statusbar->cursor_format_str,
		  "", ROUND (x), ", ", ROUND (y));
    }
  else /* show real world units */
    {
      gimp_unit_store_get_values (store, image->unit, &x, &y);

      g_snprintf (buffer, sizeof (buffer), statusbar->cursor_format_str,
		  "", x, ", ", y);
    }

  gtk_label_set_text (GTK_LABEL (statusbar->cursor_label), buffer);
}

static void
gimp_statusbar_unit_changed (GtkWidget     *combo,
                             GimpStatusbar *statusbar)
{
  gimp_image_set_unit (statusbar->shell->gdisp->gimage,
                       gtk_combo_box_get_active (GTK_COMBO_BOX (combo)));
}

