/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextProxy
 * Copyright (C) 2009-2010  Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "widgets-types.h"

#include "gimptextproxy.h"


enum
{
  CHANGE_SIZE,
  CHANGE_BASELINE,
  CHANGE_KERNING,
  LAST_SIGNAL
};


static void   gimp_text_proxy_move_cursor        (GtkTextView     *text_view,
                                                  GtkMovementStep  step,
                                                  gint             count,
                                                  gboolean         extend_selection);
static void   gimp_text_proxy_insert_at_cursor   (GtkTextView     *text_view,
                                                  const gchar     *str);
static void   gimp_text_proxy_delete_from_cursor (GtkTextView     *text_view,
                                                  GtkDeleteType    type,
                                                  gint             count);
static void   gimp_text_proxy_backspace          (GtkTextView     *text_view);
static void   gimp_text_proxy_cut_clipboard      (GtkTextView     *text_view);
static void   gimp_text_proxy_copy_clipboard     (GtkTextView     *text_view);
static void   gimp_text_proxy_paste_clipboard    (GtkTextView     *text_view);
static void   gimp_text_proxy_toggle_overwrite   (GtkTextView     *text_view);


G_DEFINE_TYPE (GimpTextProxy, gimp_text_proxy, GTK_TYPE_TEXT_VIEW)

static guint proxy_signals[LAST_SIGNAL] = { 0 };


static void
gimp_text_proxy_class_init (GimpTextProxyClass *klass)
{
  GtkTextViewClass *tv_class = GTK_TEXT_VIEW_CLASS (klass);
  GtkBindingSet    *binding_set;

  tv_class->move_cursor        = gimp_text_proxy_move_cursor;
  tv_class->insert_at_cursor   = gimp_text_proxy_insert_at_cursor;
  tv_class->delete_from_cursor = gimp_text_proxy_delete_from_cursor;
  tv_class->backspace          = gimp_text_proxy_backspace;
  tv_class->cut_clipboard      = gimp_text_proxy_cut_clipboard;
  tv_class->copy_clipboard     = gimp_text_proxy_copy_clipboard;
  tv_class->paste_clipboard    = gimp_text_proxy_paste_clipboard;
  tv_class->toggle_overwrite   = gimp_text_proxy_toggle_overwrite;

  proxy_signals[CHANGE_SIZE] =
    g_signal_new ("change-size",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpTextProxyClass, change_size),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_DOUBLE);

  proxy_signals[CHANGE_BASELINE] =
    g_signal_new ("change-baseline",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpTextProxyClass, change_baseline),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_DOUBLE);

  proxy_signals[CHANGE_KERNING] =
    g_signal_new ("change-kerning",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpTextProxyClass, change_kerning),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_DOUBLE);

  binding_set = gtk_binding_set_by_class (klass);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_plus, GDK_MOD1_MASK,
                                "change-size", 1,
                                G_TYPE_DOUBLE, 1.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_minus, GDK_MOD1_MASK,
                                "change-size", 1,
                                G_TYPE_DOUBLE, -1.0);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Up, GDK_MOD1_MASK,
                                "change-baseline", 1,
                                G_TYPE_DOUBLE, 1.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Down, GDK_MOD1_MASK,
                                "change-baseline", 1,
                                G_TYPE_DOUBLE, -1.0);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Left, GDK_MOD1_MASK,
                                "change-kerning", 1,
                                G_TYPE_DOUBLE, -1.0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_Right, GDK_MOD1_MASK,
                                "change-kerning", 1,
                                G_TYPE_DOUBLE, 1.0);
}

static void
gimp_text_proxy_init (GimpTextProxy *text_proxy)
{
}

static void
gimp_text_proxy_move_cursor (GtkTextView     *text_view,
                             GtkMovementStep  step,
                             gint             count,
                             gboolean         extend_selection)
{
}

static void
gimp_text_proxy_insert_at_cursor (GtkTextView *text_view,
                                  const gchar *str)
{
}

static void
gimp_text_proxy_delete_from_cursor (GtkTextView   *text_view,
                                    GtkDeleteType  type,
                                    gint           count)
{
}

static void
gimp_text_proxy_backspace (GtkTextView *text_view)
{
}

static void
gimp_text_proxy_cut_clipboard (GtkTextView *text_view)
{
}

static void
gimp_text_proxy_copy_clipboard (GtkTextView *text_view)
{
}

static void
gimp_text_proxy_paste_clipboard (GtkTextView *text_view)
{
}

static void
gimp_text_proxy_toggle_overwrite (GtkTextView *text_view)
{
}


/*  public functions  */

GtkWidget *
gimp_text_proxy_new (void)
{
  GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
  GtkWidget     *proxy;

  proxy = g_object_new (GIMP_TYPE_TEXT_PROXY,
                        "buffer", buffer,
                        NULL);

  g_object_unref (buffer);

  return proxy;
}
