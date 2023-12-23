/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenu_model.h
 * Copyright (C) 2023 Jehan
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

#ifndef __GIMP_MENU_MODEL_H__
#define __GIMP_MENU_MODEL_H__


#define GIMP_TYPE_MENU_MODEL            (gimp_menu_model_get_type ())
#define GIMP_MENU_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MENU_MODEL, GimpMenuModel))
#define GIMP_MENU_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MENU_MODEL, GimpMenuModelClass))
#define GIMP_IS_MENU_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MENU_MODEL))
#define GIMP_IS_MENU_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MENU_MODEL))
#define GIMP_MENU_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MENU_MODEL, GimpMenuModelClass))


typedef struct _GimpMenuModelPrivate GimpMenuModelPrivate;
typedef struct _GimpMenuModelClass   GimpMenuModelClass;

struct _GimpMenuModel
{
  GMenuModel             parent_instance;

  GimpMenuModelPrivate  *priv;
};

struct _GimpMenuModelClass
{
  GMenuModelClass        parent_class;
};


GType            gimp_menu_model_get_type   (void) G_GNUC_CONST;

GimpMenuModel  * gimp_menu_model_new        (GimpUIManager *manager,
                                             GMenuModel    *model);

GimpMenuModel  * gimp_menu_model_get_submodel (GimpMenuModel *model,
                                               const gchar   *path);

const gchar    * gimp_menu_model_get_path     (GimpMenuModel *model);

void             gimp_menu_model_set_title    (GimpMenuModel *model,
                                               const gchar   *path,
                                               const gchar   *title);
void             gimp_menu_model_set_color    (GimpMenuModel *model,
                                               const gchar   *path,
                                               GeglColor     *color);


#endif /* __GIMP_MENU_MODEL_H__ */
