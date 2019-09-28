/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpprocedureconfig-private.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PROCEDURE_CONFIG_PRIVATE_H__
#define __GIMP_PROCEDURE_CONFIG_PRIVATE_H__


gboolean   gimp_procedure_config_load_default  (GimpProcedureConfig  *config,
                                                GError              **error);
gboolean   gimp_procedure_config_save_default  (GimpProcedureConfig  *config,
                                                GError              **error);

gboolean   gimp_procedure_config_load_last     (GimpProcedureConfig  *config,
                                                GError              **error);
gboolean   gimp_procedure_config_save_last     (GimpProcedureConfig  *config,
                                                GError              **error);

gboolean   gimp_procedure_config_load_parasite (GimpProcedureConfig  *config,
                                                GimpImage            *image,
                                                GError              **error);
gboolean   gimp_procedure_config_save_parasite (GimpProcedureConfig  *config,
                                                GimpImage            *image,
                                                GError              **error);


#endif /* __GIMP_PROCEDURE_CONFIG_PRIVATE_H__ */
