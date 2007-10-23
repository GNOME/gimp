/* A small test program for the CPU detection code */

#include "config.h"

#include <stdlib.h>

#include <glib.h>

#include "gimpcpuaccel.h"


static void
cpu_accel_print_results (void)
{
  GimpCpuAccelFlags  support;

  g_printerr ("Testing CPU features...\n");

  support = gimp_cpu_accel_get_support ();

#ifdef ARCH_X86
  g_printerr ("  mmx     : %s\n",
              (support & GIMP_CPU_ACCEL_X86_MMX)     ? "yes" : "no");
  g_printerr ("  3dnow   : %s\n",
              (support & GIMP_CPU_ACCEL_X86_3DNOW)   ? "yes" : "no");
  g_printerr ("  mmxext  : %s\n",
              (support & GIMP_CPU_ACCEL_X86_MMXEXT)  ? "yes" : "no");
  g_printerr ("  sse     : %s\n",
              (support & GIMP_CPU_ACCEL_X86_SSE)     ? "yes" : "no");
  g_printerr ("  sse2    : %s\n",
              (support & GIMP_CPU_ACCEL_X86_SSE2)    ? "yes" : "no");
  g_printerr ("  sse3    : %s\n",
              (support & GIMP_CPU_ACCEL_X86_SSE3)    ? "yes" : "no");
#endif
#ifdef ARCH_PPC
  g_printerr ("  altivec : %s\n",
              (support & GIMP_CPU_ACCEL_PPC_ALTIVEC) ? "yes" : "no");
#endif
  g_printerr ("\n");
}

int
main (void)
{
  cpu_accel_print_results ();

  return EXIT_SUCCESS;
}
