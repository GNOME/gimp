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
#ifndef __COLOR_BALANCE_H__
#define __COLOR_BALANCE_H__

#include <gtk/gtk.h>
#include "gimpdrawableF.h"
#include "image_map.h"
#include "tools.h"

typedef enum
{
  SHADOWS,
  MIDTONES,
  HIGHLIGHTS
} TransferMode;

typedef struct _ColorBalanceDialog ColorBalanceDialog;
struct _ColorBalanceDialog
{
  GtkWidget   *shell;
  GtkWidget   *cyan_red_text;
  GtkWidget   *magenta_green_text;
  GtkWidget   *yellow_blue_text;
  GtkAdjustment  *cyan_red_data;
  GtkAdjustment  *magenta_green_data;
  GtkAdjustment  *yellow_blue_data;

  GimpDrawable *drawable;
  ImageMap     image_map;

  double       cyan_red[3];
  double       magenta_green[3];
  double       yellow_blue[3];

  guchar       r_lookup[256];
  guchar       g_lookup[256];
  guchar       b_lookup[256];

  gint         preserve_luminosity;
  gint         preview;
  gint         application_mode;
};

/*  by_color select functions  */
Tool *        tools_new_color_balance  (void);
void          tools_free_color_balance (Tool *);

void          color_balance_initialize (GDisplay *);
void          color_balance            (PixelRegion *, PixelRegion *, void *);

void          color_balance_create_lookup_tables (ColorBalanceDialog *);

#endif  /*  __COLOR_BALANCE_H__  */
