#ifndef gimp_composite_context_h
#define gimp_composite_context_h

extern void gimp_composite_mmx_init(void);

#ifdef USE_MMX
#if __GNUC__ >= 3
/*
 *
 */
extern void gimp_composite_addition_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_burn_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_coloronly_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_darken_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_difference_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_dissolve_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_divide_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_dodge_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_grainextract_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_grainmerge_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_hardlight_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_hueonly_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_lighten_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_multiply_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_overlay_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_replace_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_saturationonly_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_screen_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_softlight_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_subtract_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_swap_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);
extern void gimp_composite_valueonly_rgba8_rgba8_rgba8_mmx(GimpCompositeContext *);

extern void gimp_composite_addition_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_burn_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_coloronly_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_darken_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_difference_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_dissolve_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_divide_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_dodge_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_grainextract_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_grainmerge_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_hardlight_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_hueonly_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_lighten_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_multiply_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_overlay_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_replace_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_saturationonly_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_screen_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_softlight_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_subtract_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_swap_va8_va8_va8_mmx(GimpCompositeContext *);
extern void gimp_composite_valueonly_va8_va8_va8_mmx(GimpCompositeContext *);
#endif /* __GNUC__ > 3 */
#endif /* USE_MMX */
#endif
