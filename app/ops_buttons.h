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
#ifndef __OPS_BUTTONS_H__
#define __OPS_BUTTONS_H__

typedef struct _OpsButton OpsButton;

typedef enum
{
  OPS_BUTTON_MODIFIER_NONE,
  OPS_BUTTON_MODIFIER_SHIFT,
  OPS_BUTTON_MODIFIER_CTRL,
  OPS_BUTTON_MODIFIER_ALT,
  OPS_BUTTON_MODIFIER_SHIFT_CTRL,
  OPS_BUTTON_MODIFIER_LAST
} OpsButtonModifier;

typedef enum
{
  OPS_BUTTON_NORMAL,
  OPS_BUTTON_RADIO,
} OpsButtonType;

struct _OpsButton 
{
  gchar         **xpm_data;       /*  xpm data for the button  */
  GtkSignalFunc   callback;       /*  callback function        */
  GtkSignalFunc  *ext_callbacks;  /*  callback functions when modifiers are pressed  */
  gchar          *tooltip;
  gchar          *private_tip;
  GtkWidget      *widget;         /*  the button widget        */
  gint            modifier;
};

/* Function declarations */

GtkWidget * ops_button_box_new (GtkWidget     *parent,
				OpsButton     *ops_button,
				OpsButtonType  ops_type);

#endif /* __OPS_BUTTONS_H__ */

