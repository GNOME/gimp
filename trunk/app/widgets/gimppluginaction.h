/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginaction.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PLUG_IN_ACTION_H__
#define __GIMP_PLUG_IN_ACTION_H__


#include "gimpaction.h"


#define GIMP_TYPE_PLUG_IN_ACTION            (gimp_plug_in_action_get_type ())
#define GIMP_PLUG_IN_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PLUG_IN_ACTION, GimpPlugInAction))
#define GIMP_PLUG_IN_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PLUG_IN_ACTION, GimpPlugInActionClass))
#define GIMP_IS_PLUG_IN_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PLUG_IN_ACTION))
#define GIMP_IS_PLUG_IN_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GIMP_TYPE_PLUG_IN_ACTION))
#define GIMP_PLUG_IN_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMP_TYPE_PLUG_IN_ACTION, GimpPlugInActionClass))


typedef struct _GimpPlugInActionClass GimpPlugInActionClass;

struct _GimpPlugInAction
{
  GimpAction           parent_instance;

  GimpPlugInProcedure *procedure;
};

struct _GimpPlugInActionClass
{
  GimpActionClass parent_class;

  void (* selected) (GimpPlugInAction    *action,
                     GimpPlugInProcedure *proc);
};


GType              gimp_plug_in_action_get_type (void) G_GNUC_CONST;

GimpPlugInAction * gimp_plug_in_action_new      (const gchar         *name,
                                                 const gchar         *label,
                                                 const gchar         *tooltip,
                                                 const gchar         *stock_id,
                                                 GimpPlugInProcedure *procedure);
void               gimp_plug_in_action_selected (GimpPlugInAction    *action,
                                                 GimpPlugInProcedure *procedure);


#endif  /* __GIMP_PLUG_IN_ACTION_H__ */
