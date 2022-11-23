/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadisplayshell-items.h
 * Copyright (C) 2010  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DISPLAY_SHELL_ITEMS_H__
#define __LIGMA_DISPLAY_SHELL_ITEMS_H__


void   ligma_display_shell_items_init            (LigmaDisplayShell *shell);
void   ligma_display_shell_items_free            (LigmaDisplayShell *shell);

void   ligma_display_shell_add_item              (LigmaDisplayShell *shell,
                                                 LigmaCanvasItem   *item);
void   ligma_display_shell_remove_item           (LigmaDisplayShell *shell,
                                                 LigmaCanvasItem   *item);

void   ligma_display_shell_add_preview_item      (LigmaDisplayShell *shell,
                                                 LigmaCanvasItem   *item);
void   ligma_display_shell_remove_preview_item   (LigmaDisplayShell *shell,
                                                 LigmaCanvasItem   *item);

void   ligma_display_shell_add_unrotated_item    (LigmaDisplayShell *shell,
                                                 LigmaCanvasItem   *item);
void   ligma_display_shell_remove_unrotated_item (LigmaDisplayShell *shell,
                                                 LigmaCanvasItem   *item);

void   ligma_display_shell_add_tool_item         (LigmaDisplayShell *shell,
                                                 LigmaCanvasItem   *item);
void   ligma_display_shell_remove_tool_item      (LigmaDisplayShell *shell,
                                                 LigmaCanvasItem   *item);


#endif /* __LIGMA_DISPLAY_SHELL_ITEMS_H__ */
