/* ----------------------------------------------------------------------
 * p_plug_in_diffraction_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_diffraction_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_diffraction_Vals 
    {
      gdouble   lam_r;
      gdouble   lam_g;
      gdouble   lam_b;
      gdouble   contour_r;
      gdouble   contour_g;
      gdouble   contour_b;
      gdouble   edges_r;
      gdouble   edges_g;
      gdouble   edges_b;
      gdouble   brightness;
      gdouble   scattering;
      gdouble   polarization;
    } t_plug_in_diffraction_Vals; 

    t_plug_in_diffraction_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_diffraction_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_diffraction_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_diffraction_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_diffraction_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_diffraction_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_diffraction_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_diffraction_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.lam_r, buf_from->lam_r, buf_to->lam_r, total_steps, current_step);
    p_delta_gdouble(&buf.lam_g, buf_from->lam_g, buf_to->lam_g, total_steps, current_step);
    p_delta_gdouble(&buf.lam_b, buf_from->lam_b, buf_to->lam_b, total_steps, current_step);
    p_delta_gdouble(&buf.contour_r, buf_from->contour_r, buf_to->contour_r, total_steps, current_step);
    p_delta_gdouble(&buf.contour_g, buf_from->contour_g, buf_to->contour_g, total_steps, current_step);
    p_delta_gdouble(&buf.contour_b, buf_from->contour_b, buf_to->contour_b, total_steps, current_step);
    p_delta_gdouble(&buf.edges_r, buf_from->edges_r, buf_to->edges_r, total_steps, current_step);
    p_delta_gdouble(&buf.edges_g, buf_from->edges_g, buf_to->edges_g, total_steps, current_step);
    p_delta_gdouble(&buf.edges_b, buf_from->edges_b, buf_to->edges_b, total_steps, current_step);
    p_delta_gdouble(&buf.brightness, buf_from->brightness, buf_to->brightness, total_steps, current_step);
    p_delta_gdouble(&buf.scattering, buf_from->scattering, buf_to->scattering, total_steps, current_step);
    p_delta_gdouble(&buf.polarization, buf_from->polarization, buf_to->polarization, total_steps, current_step);

    gimp_set_data("plug_in_diffraction", &buf, sizeof(buf)); 

    return 0; /* OK */
}
