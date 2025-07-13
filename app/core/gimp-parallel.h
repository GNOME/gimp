/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-parallel.h
 * Copyright (C) 2018 Ell
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

#pragma once


void        gimp_parallel_init                       (Gimp             *gimp);
void        gimp_parallel_exit                       (Gimp             *gimp);

GimpAsync * gimp_parallel_run_async                  (GimpRunAsyncFunc  func,
                                                      gpointer          user_data);
GimpAsync * gimp_parallel_run_async_full             (gint              priority,
                                                      GimpRunAsyncFunc  func,
                                                      gpointer          user_data,
                                                      GDestroyNotify    user_data_destroy_func);
GimpAsync * gimp_parallel_run_async_independent      (GimpRunAsyncFunc  func,
                                                      gpointer          user_data);
GimpAsync * gimp_parallel_run_async_independent_full (gint              priority,
                                                      GimpRunAsyncFunc  func,
                                                      gpointer          user_data);


#ifdef __cplusplus

extern "C++"
{

#include <new>

template <class RunAsyncFunc>
inline GimpAsync *
gimp_parallel_run_async (RunAsyncFunc func)
{
  RunAsyncFunc *func_copy = g_new (RunAsyncFunc, 1);

  new (func_copy) RunAsyncFunc (func);

  return gimp_parallel_run_async_full (0,
                                       [] (GimpAsync *async,
                                           gpointer   user_data)
                                       {
                                         RunAsyncFunc *func_copy =
                                           (RunAsyncFunc *) user_data;

                                         (*func_copy) (async);

                                         func_copy->~RunAsyncFunc ();
                                         g_free (func_copy);
                                       },
                                       func_copy,
                                       [] (gpointer user_data)
                                       {
                                         RunAsyncFunc *func_copy =
                                           (RunAsyncFunc *) user_data;

                                         func_copy->~RunAsyncFunc ();
                                         g_free (func_copy);
                                       });
}

template <class RunAsyncFunc,
          class DestroyFunc>
inline GimpAsync *
gimp_parallel_run_async_full (gint         priority,
                              RunAsyncFunc func,
                              DestroyFunc  destroy_func)
{
  typedef struct
  {
    RunAsyncFunc func;
    DestroyFunc  destroy_func;
  } Funcs;

  Funcs *funcs_copy = g_new (Funcs, 1);

  new (funcs_copy) Funcs {func, destroy_func};

  return gimp_parallel_run_async_full (priority,
                                       [] (GimpAsync *async,
                                           gpointer   user_data)
                                       {
                                         Funcs *funcs_copy =
                                           (Funcs *) user_data;

                                         funcs_copy->func (async);

                                         funcs_copy->~Funcs ();
                                         g_free (funcs_copy);
                                       },
                                       funcs_copy,
                                       [] (gpointer user_data)
                                       {
                                         Funcs *funcs_copy =
                                           (Funcs *) user_data;

                                         funcs_copy->destroy_func ();

                                         funcs_copy->~Funcs ();
                                         g_free (funcs_copy);
                                       });
}

template <class RunAsyncFunc>
inline GimpAsync *
gimp_parallel_run_async_independent_full (gint         priority,
                                          RunAsyncFunc func)
{
  RunAsyncFunc *func_copy = g_new (RunAsyncFunc, 1);

  new (func_copy) RunAsyncFunc (func);

  return gimp_parallel_run_async_independent_full (priority,
                                                   [] (GimpAsync *async,
                                                       gpointer   user_data)
                                                   {
                                                     RunAsyncFunc *func_copy =
                                                       (RunAsyncFunc *) user_data;

                                                     (*func_copy) (async);

                                                     func_copy->~RunAsyncFunc ();
                                                     g_free (func_copy);
                                                   },
                                                   func_copy);
}

template <class RunAsyncFunc>
inline GimpAsync *
gimp_parallel_run_async_independent (RunAsyncFunc func)
{
  return gimp_parallel_run_async_independent_full (0, func);
}

}

#endif /* __cplusplus */
