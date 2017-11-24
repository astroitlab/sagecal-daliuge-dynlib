#include "utils_dn.h"
#include <stdio.h>

void dump_iodata_dn(dlg_output_info *output, Data::IOData *iodata) {

    int len = 0;
    output->write((char *)&len, sizeof(int));/* this is a flag that indicate share-data file 0*/
    output->write((char *)&(iodata->M), sizeof(int));
    output->write((char *)&(iodata->Mt), sizeof(int));

    len = strlen(iodata->msname);
    output->write((char *)&len, sizeof(int));
    output->write(iodata->msname, sizeof(char) * strlen(iodata->msname));

    output->write((char *)&(iodata->N), sizeof(int));
    output->write((char *)&(iodata->Nbase), sizeof(int));
    output->write((char *)&(iodata->tilesz), sizeof(int));
    output->write((char *)&(iodata->Nchan), sizeof(int));
    output->write((char *)&(iodata->Nms), sizeof(int));

    len = iodata->Nms;
    output->write((char *)&len, sizeof(int));
    output->write((char *)iodata->NchanMS, sizeof(int) * iodata->Nms);

    output->write((char *)&(iodata->deltat), sizeof(double));
    output->write((char *)&(iodata->totalt), sizeof(int));
    output->write((char *)&(iodata->ra0), sizeof(double));
    output->write((char *)&(iodata->dec0), sizeof(double));

    len = iodata->Nbase*iodata->tilesz;
    output->write((char *)&len, sizeof(int));
    output->write((char *)iodata->u, sizeof(double) * len);
    output->write((char *)&len, sizeof(int));
    output->write((char *)iodata->v, sizeof(double) * len);
    output->write((char *)&len, sizeof(int));
    output->write((char *)iodata->w, sizeof(double) * len);

    len = 8*len;
    output->write((char *)&len, sizeof(int));
    output->write((char *)iodata->x, sizeof(double) * len);

    len = len*iodata->Nchan;
    output->write((char *)&len, sizeof(int));
    output->write((char *)iodata->xo, sizeof(double) * len);

    len = iodata->Nchan;
    output->write((char *)&len, sizeof(int));
    output->write((char *)iodata->freqs, sizeof(double) * len);

    output->write((char *)&(iodata->freq0), sizeof(double));

    len = iodata->Nbase*iodata->tilesz;
    output->write((char *)&len, sizeof(int));
    output->write((char *)iodata->flag, sizeof(double) * len);

    output->write((char *)&(iodata->deltaf), sizeof(double));
    output->write((char *)&(iodata->fratio), sizeof(double));
}
void load_iodata_dn_noprefix(dlg_input_info *input, Data::IOData *iodata){
    int len = 0;
    input->read((char *)&(iodata->M), sizeof(int));
    input->read((char *)&(iodata->Mt), sizeof(int));

    input->read((char *)&len, sizeof(int));
    iodata->msname = (char *)malloc(sizeof(char)*(len+1));
    input->read((char *)iodata->msname, sizeof(char) * len);
    iodata->msname[len] = '\0';

    input->read((char *)&(iodata->N), sizeof(int));
    input->read((char *)&(iodata->Nbase), sizeof(int));
    input->read((char *)&(iodata->tilesz), sizeof(int));
    input->read((char *)&(iodata->Nchan), sizeof(int));
    input->read((char *)&(iodata->Nms), sizeof(int));

    input->read((char *)&len, sizeof(int));
    iodata->NchanMS = new int[len];
    input->read((char *)iodata->NchanMS, sizeof(int) * len);

    input->read((char *)&(iodata->deltat), sizeof(double));
    input->read((char *)&(iodata->totalt), sizeof(int));
    input->read((char *)&(iodata->ra0), sizeof(double));
    input->read((char *)&(iodata->dec0), sizeof(double));

    input->read((char *)&len, sizeof(int));
    iodata->u = new double[len];
    input->read((char *)iodata->u, sizeof(double) * len);

    input->read((char *)&len, sizeof(int));
    iodata->v = new double[len];
    input->read((char *)iodata->v, sizeof(double) * len);

    input->read((char *)&len, sizeof(int));
    iodata->w = new double[len];
    input->read((char *)iodata->w, sizeof(double) * len);

    input->read((char *)&len, sizeof(int));
    iodata->x = new double[len];
    input->read((char *)iodata->x, sizeof(double) * len);

    input->read((char *)&len, sizeof(int));
    iodata->xo = new double[len];
    input->read((char *)iodata->xo, sizeof(double) * len);

    input->read((char *)&len, sizeof(int));
    iodata->freqs = new double[len];
    input->read((char *)iodata->freqs, sizeof(double) * len);

    input->read((char *)&(iodata->freq0), sizeof(double));

    input->read((char *)&len, sizeof(int));
    iodata->flag = new double[len];
    input->read((char *)iodata->flag, sizeof(double) * len);

    input->read((char *)&(iodata->deltaf), sizeof(double));
    input->read((char *)&(iodata->fratio), sizeof(double));
}
void load_iodata_dn(dlg_input_info *input, Data::IOData *iodata){
    int len = 0;
    input->read((char *)&len, sizeof(int));
    load_iodata_dn_noprefix(input,iodata);
}
void dump_beam_dn(dlg_output_info *output, Data::IOData *iodata, Data::LBeam *lBeam){

    int len = iodata->tilesz;
    output->write((char *)&len, sizeof(int));
    output->write((char *)lBeam->time_utc, sizeof(double) * len);

    len = iodata->N;
    output->write((char *)&len, sizeof(int));
    output->write((char *)lBeam->Nelem, sizeof(int) * len);

    output->write((char *)lBeam->sx, sizeof(double) * len);

    output->write((char *)lBeam->sy, sizeof(double) * len);

    output->write((char *)lBeam->sz, sizeof(double) * len);

    output->write((char *)lBeam->len_xyz, sizeof(int) * len);

    for(int i=0;i< iodata->N;i++) {
        output->write((char *)lBeam->xx[i], sizeof(double) * lBeam->len_xyz[i]);
        output->write((char *)lBeam->yy[i], sizeof(double) * lBeam->len_xyz[i]);
        output->write((char *)lBeam->zz[i], sizeof(double) * lBeam->len_xyz[i]);
    }

    output->write((char *)&(lBeam->p_ra0), sizeof(double));
    output->write((char *)&(lBeam->p_dec0), sizeof(double));
}

void load_beam_dn(dlg_input_info *input, Data::LBeam *lBeam){
    int len = 0;
    input->read((char *)&len, sizeof(int));
    lBeam->time_utc = new double[len];
    input->read((char *)lBeam->time_utc, sizeof(double) * len);

    input->read((char *)&len, sizeof(int));
    lBeam->Nelem = new int[len];
    input->read((char *)lBeam->Nelem, sizeof(int) * len);

    lBeam->sx = new double[len];
    input->read((char *)lBeam->sx, sizeof(double) * len);

    lBeam->sy = new double[len];
    input->read((char *)lBeam->sy, sizeof(double) * len);

    lBeam->sz = new double[len];
    input->read((char *)lBeam->sz, sizeof(double) * len);

    lBeam->len_xyz = new int[len];
    input->read((char *)lBeam->len_xyz, sizeof(int) * len);

    lBeam->xx = new double *[len];
    lBeam->yy = new double *[len];
    lBeam->zz = new double *[len];

    for(int i=0; i < len;i++) {
        lBeam->xx[i] = new double[lBeam->len_xyz[i]];
        lBeam->yy[i] = new double[lBeam->len_xyz[i]];
        lBeam->zz[i] = new double[lBeam->len_xyz[i]];
        input->read((char *)lBeam->xx[i], sizeof(double) * lBeam->len_xyz[i]);
        input->read((char *)lBeam->yy[i], sizeof(double) * lBeam->len_xyz[i]);
        input->read((char *)lBeam->zz[i], sizeof(double) * lBeam->len_xyz[i]);
    }

    input->read((char *)&(lBeam->p_ra0), sizeof(double));
    input->read((char *)&(lBeam->p_dec0), sizeof(double));
}

void dump_barr_dn(dlg_output_info *output, Data::IOData *iodata, baseline_t *barr){
    for(int i=0;i<iodata->Nbase * iodata->tilesz;i++) {
        output->write((char *)&(barr[i].sta1), sizeof(int));
        output->write((char *)&(barr[i].sta2), sizeof(int));
        output->write((char *)&(barr[i].flag), sizeof(unsigned char));
    }
}

void load_barr_dn(dlg_input_info *input, Data::IOData *iodata, baseline_t *barr){
    for(int i=0;i<iodata->Nbase * iodata->tilesz;i++) {
        input->read((char *)&(barr[i].sta1), sizeof(int));
        input->read((char *)&(barr[i].sta2), sizeof(int));
        input->read((char *)&(barr[i].flag), sizeof(unsigned char));
    }
}

void dump_coh_dn(dlg_output_info *output, Data::IOData *iodata, complex double *coh) {
    output->write((char *)coh, sizeof(complex double) * iodata->M * iodata->Nbase * iodata->tilesz * 4);
}

void load_coh_dn(dlg_input_info *input, Data::IOData *iodata, complex double *coh) {
    input->read((char *)coh, sizeof(complex double) * iodata->M * iodata->Nbase * iodata->tilesz * 4);
}

void dump_mpidata_dn(dlg_output_info *output, Data::MPIData *iodata) {
    int len = 1;
    output->write((char *)&len, sizeof(int));/* this is a flag that indicate master share-data file 1*/

    output->write((char *)&(iodata->N), sizeof(int));
    output->write((char *)&(iodata->M), sizeof(int));
    output->write((char *)&(iodata->tilesz), sizeof(int));
    output->write((char *)&(iodata->Nms), sizeof(int));
    output->write((char *)&(iodata->totalt), sizeof(int));

    len = iodata->Nms;
    output->write((char *)&len, sizeof(int));
    output->write((char *)iodata->freqs, sizeof(double) * len);

    output->write((char *)&(iodata->freq0), sizeof(double));
    output->write((char *)&(iodata->Mo), sizeof(int));
}

void load_mpidata_dn_noprefix(dlg_input_info *input, Data::MPIData *iodata) {
    int len = 0;

    input->read((char *)&(iodata->N), sizeof(int));
    input->read((char *)&(iodata->M), sizeof(int));
    input->read((char *)&(iodata->tilesz), sizeof(int));
    input->read((char *)&(iodata->Nms), sizeof(int));
    input->read((char *)&(iodata->totalt), sizeof(int));

    input->read((char *)&len, sizeof(int));
    iodata->freqs = new double[len];
    input->read((char *)iodata->freqs, sizeof(double) * len);
    input->read((char *)&(iodata->freq0), sizeof(double));
    input->read((char *)&(iodata->Mo), sizeof(int));
}

void load_mpidata_dn(dlg_input_info *input, Data::MPIData *iodata) {
    int len = 0;
    input->read((char *)&len, sizeof(int));/*skip flag*/
    load_mpidata_dn_noprefix(input, iodata);
}