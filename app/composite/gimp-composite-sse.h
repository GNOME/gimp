#ifndef gimp_composite_sse_h
#define gimp_composite_sse_h

extern void gimp_composite_sse_init(void);

#ifdef USE_MMX
#ifdef ARCH_X86
#if __GNUC__ >= 3
/*
 *
 */
extern void gimp_composite_addition_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_burn_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_coloronly_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_darken_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_difference_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_dissolve_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_divide_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_dodge_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_grain_extract_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_grain_merge_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_hardlight_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_hueonly_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_lighten_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_multiply_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_overlay_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_replace_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_saturationonly_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_screen_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_softlight_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_subtract_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_swap_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_valueonly_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);
extern void gimp_composite_scale_rgba8_rgba8_rgba8_sse(GimpCompositeContext *);

extern void gimp_composite_addition_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_burn_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_coloronly_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_darken_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_difference_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_dissolve_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_divide_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_dodge_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_grain_extract_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_grain_merge_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_hardlight_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_hueonly_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_lighten_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_multiply_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_overlay_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_replace_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_saturationonly_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_screen_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_softlight_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_subtract_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_swap_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_valueonly_va8_va8_va8_sse(GimpCompositeContext *);
extern void gimp_composite_scale_va8_va8_va8_sse(GimpCompositeContext *);
#endif /* __GNUC__ > 3 */
#endif /* ARCH_X86 */
#endif /* USE_MMX */
#endif
