/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Manish Singh
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

#ifndef __GIMP_DISPLAY_SHELL_FILTER_H__
#define __GIMP_DISPLAY_SHELL_FILTER_H__


typedef struct _ColorDisplayNode ColorDisplayNode;

struct _ColorDisplayNode
{
  GimpColorDisplay *color_display;
  gchar            *cd_name;
};


ColorDisplayNode *
       gimp_display_shell_filter_attach           (GimpDisplayShell *shell,
                                                   GType             type);
ColorDisplayNode *
       gimp_display_shell_filter_attach_clone     (GimpDisplayShell *shell,
                                                   ColorDisplayNode *node);
void   gimp_display_shell_filter_detach           (GimpDisplayShell *shell,
                                                   ColorDisplayNode *node);
void   gimp_display_shell_filter_detach_destroy   (GimpDisplayShell *shell,
                                                   ColorDisplayNode *node);
void   gimp_display_shell_filter_detach_all       (GimpDisplayShell *shell);
void   gimp_display_shell_filter_reorder_up       (GimpDisplayShell *shell,
                                                   ColorDisplayNode *node);
void   gimp_display_shell_filter_reorder_down     (GimpDisplayShell *shell,
                                                   ColorDisplayNode *node);

void   gimp_display_shell_filter_configure        (ColorDisplayNode *node,
                                                   GFunc             ok_func,
                                                   gpointer          ok_data,
                                                   GFunc             cancel_func,
                                                   gpointer          cancel_data);
void   gimp_display_shell_filter_configure_cancel (ColorDisplayNode *node);


#endif /* __GIMP_DISPLAY_SHELL_FILTER_H__ */
