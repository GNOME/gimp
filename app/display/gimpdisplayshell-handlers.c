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

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimpimage.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-handlers.h"
#include "gimpdisplayshell-scale.h"

#include "gimprc.h"


/*  local function prototypes  */

static void   gimp_display_shell_update_title_handler       (GimpImage        *gimage,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_selection_control_handler  (GimpImage        *gimage,
                                                             GimpSelectionControl control,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_size_changed_handler       (GimpImage        *gimage,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_resolution_changed_handler (GimpImage        *gimage,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_unit_changed_handler       (GimpImage        *gimage,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_qmask_changed_handler      (GimpImage        *gimage,
                                                             GimpDisplayShell *shell);
static void   gimp_display_shell_update_guide_handler       (GimpImage        *gimage,
                                                             GimpGuide        *guide,
                                                             GimpDisplayShell *shell);


/*  public functions  */

void
gimp_display_shell_connect (GimpDisplayShell *shell)
{
  GimpImage *gimage;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->gdisp));
  g_return_if_fail (GIMP_IS_IMAGE (shell->gdisp->gimage));

  gimage = shell->gdisp->gimage;

  g_signal_connect (G_OBJECT (gimage), "dirty",
                    G_CALLBACK (gimp_display_shell_update_title_handler),
                    shell);
  g_signal_connect (G_OBJECT (gimage), "clean",
                    G_CALLBACK (gimp_display_shell_update_title_handler),
                    shell);
  g_signal_connect (G_OBJECT (gimage), "name_changed",
                    G_CALLBACK (gimp_display_shell_update_title_handler),
                    shell);

  g_signal_connect (G_OBJECT (gimage), "selection_control",
                    G_CALLBACK (gimp_display_shell_selection_control_handler),
                    shell);
  g_signal_connect (G_OBJECT (gimage), "size_changed",
                    G_CALLBACK (gimp_display_shell_size_changed_handler),
                    shell);
  g_signal_connect (G_OBJECT (gimage), "resolution_changed",
                    G_CALLBACK (gimp_display_shell_resolution_changed_handler),
                    shell);
  g_signal_connect (G_OBJECT (gimage), "unit_changed",
                    G_CALLBACK (gimp_display_shell_unit_changed_handler),
                    shell);
  g_signal_connect (G_OBJECT (gimage), "qmask_changed",
                    G_CALLBACK (gimp_display_shell_qmask_changed_handler),
                    shell);
  g_signal_connect (G_OBJECT (gimage), "update_guide",
                    G_CALLBACK (gimp_display_shell_update_guide_handler),
                    shell);
}

void
gimp_display_shell_disconnect (GimpDisplayShell *shell)
{
  GimpImage *gimage;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->gdisp));
  g_return_if_fail (GIMP_IS_IMAGE (shell->gdisp->gimage));

  gimage = shell->gdisp->gimage;

  g_signal_handlers_disconnect_by_func (G_OBJECT (gimage),
                                        gimp_display_shell_update_guide_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gimage),
                                        gimp_display_shell_qmask_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gimage),
                                        gimp_display_shell_unit_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gimage),
                                        gimp_display_shell_resolution_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gimage),
                                        gimp_display_shell_size_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (G_OBJECT (gimage),
                                        gimp_display_shell_selection_control_handler,
                                        shell);

  g_signal_handlers_disconnect_by_func (G_OBJECT (gimage),
                                        gimp_display_shell_update_title_handler,
                                        shell);
}


/*  private functions  */

static void
gimp_display_shell_update_title_handler (GimpImage        *gimage,
                                         GimpDisplayShell *shell)
{
  gimp_display_shell_update_title (shell);
}

static void
gimp_display_shell_selection_control_handler (GimpImage            *gimage,
                                              GimpSelectionControl  control,
                                              GimpDisplayShell     *shell)
{
  gimp_display_shell_selection_visibility (shell, control);
}

static void
gimp_display_shell_size_changed_handler (GimpImage        *gimage,
                                         GimpDisplayShell *shell)
{
  gimp_display_shell_resize_cursor_label (shell);
  gimp_display_shell_scale_resize (shell, gimprc.resize_windows_on_resize, TRUE);
}

static void
gimp_display_shell_resolution_changed_handler (GimpImage        *gimage,
                                               GimpDisplayShell *shell)
{
  gimp_display_shell_scale_setup (shell);
  gimp_display_shell_resize_cursor_label (shell);
}

static void
gimp_display_shell_unit_changed_handler (GimpImage        *gimage,
                                         GimpDisplayShell *shell)
{
  gimp_display_shell_scale_setup (shell);
  gimp_display_shell_resize_cursor_label (shell);
}

static void
gimp_display_shell_qmask_changed_handler (GimpImage        *gimage,
                                          GimpDisplayShell *shell)
{
  GtkImage *image;

  image = GTK_IMAGE (GTK_BIN (shell->qmask)->child);

  g_signal_handlers_block_by_func (G_OBJECT (shell->qmask),
                                   gimp_display_shell_qmask_toggled,
                                   shell);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shell->qmask),
                                shell->gdisp->gimage->qmask_state);

  if (shell->gdisp->gimage->qmask_state)
    gtk_image_set_from_stock (image, GIMP_STOCK_QMASK_ON, GTK_ICON_SIZE_MENU);
  else
    gtk_image_set_from_stock (image, GIMP_STOCK_QMASK_OFF, GTK_ICON_SIZE_MENU);

  g_signal_handlers_unblock_by_func (G_OBJECT (shell->qmask),
                                     gimp_display_shell_qmask_toggled,
                                     shell);
}

static void
gimp_display_shell_update_guide_handler (GimpImage        *gimage,
                                         GimpGuide        *guide,
                                         GimpDisplayShell *shell)
{
  gimp_display_shell_expose_guide (shell, guide);
}
