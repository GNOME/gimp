/* ----------------------------------------------------------------------
 * p_plug_in_refract_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_refract_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_refract_Vals 
    {
      gint32    lens_id;
      gint32    refl_id;
      long      thick;
      long      refr_dist;
      long      refl_dist;
      gdouble   na;
      gdouble   nb;
      long      edge;
      long      newl;
      long      xofs;
      long      yofs;
    } t_plug_in_refract_Vals; 

    t_plug_in_refract_Vals  buf, buf_from, buf_to; 

    if(len_struct != sizeof(t_plug_in_refract_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_refract_iter  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_refract_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_refract_ITER_FROM", &buf_from); 
    gimp_get_data("plug_in_refract_ITER_TO",   &buf_to); 
    memcpy(&buf, &buf_from, sizeof(buf));

    p_delta_drawable(&buf.lens_id, buf_from.lens_id, buf_to.lens_id, total_steps, current_step);
    p_delta_drawable(&buf.refl_id, buf_from.refl_id, buf_to.refl_id, total_steps, current_step);
    p_delta_long(&buf.thick, buf_from.thick, buf_to.thick, total_steps, current_step);
    p_delta_long(&buf.refr_dist, buf_from.refr_dist, buf_to.refr_dist, total_steps, current_step);
    p_delta_long(&buf.refl_dist, buf_from.refl_dist, buf_to.refl_dist, total_steps, current_step);
    p_delta_gdouble(&buf.na, buf_from.na, buf_to.na, total_steps, current_step);
    p_delta_gdouble(&buf.nb, buf_from.nb, buf_to.nb, total_steps, current_step);
    p_delta_long(&buf.edge, buf_from.edge, buf_to.edge, total_steps, current_step);
    p_delta_long(&buf.newl, buf_from.newl, buf_to.newl, total_steps, current_step);
    p_delta_long(&buf.xofs, buf_from.xofs, buf_to.xofs, total_steps, current_step);
    p_delta_long(&buf.yofs, buf_from.yofs, buf_to.yofs, total_steps, current_step);

    gimp_set_data("plug_in_refract", &buf, sizeof(buf)); 

    return 0; /* OK */
}
