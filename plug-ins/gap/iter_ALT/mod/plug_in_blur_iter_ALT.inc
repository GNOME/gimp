/* ----------------------------------------------------------------------
 * p_plug_in_blur_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_blur_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_blur_Vals 
    {
      gdouble blur_pct;     /* likelihood of randomization (as %age) */
      gdouble blur_rcount;  /* repeat count */
      gint seed_type;       /* seed init. type - current time or user value */
      gint blur_seed;       /* seed value for rand() function */
    } t_plug_in_blur_Vals; 

    t_plug_in_blur_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_blur_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_blur_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_blur_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_blur_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_blur_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_blur_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_blur_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.blur_pct, buf_from->blur_pct, buf_to->blur_pct, total_steps, current_step);
    p_delta_gdouble(&buf.blur_rcount, buf_from->blur_rcount, buf_to->blur_rcount, total_steps, current_step);
    p_delta_gint(&buf.seed_type, buf_from->seed_type, buf_to->seed_type, total_steps, current_step);
    p_delta_gint(&buf.blur_seed, buf_from->blur_seed, buf_to->blur_seed, total_steps, current_step);

    gimp_set_data("plug_in_blur", &buf, sizeof(buf)); 

    return 0; /* OK */
}
