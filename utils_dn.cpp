#include "utils_dn.h"
#include <stdio.h>

int len_iodata(Data::IOData *iodata) {
    int len = 0;
    len += 5*sizeof(int);
    len += sizeof(char) * strlen(iodata->msname);
    len += 5*sizeof(int);
    len += sizeof(int);
    len += sizeof(int) * iodata->Nms;

    len += sizeof(double);
    len += sizeof(int);
    len += 2*sizeof(double);

    len += 3*sizeof(int);
    len += 3*sizeof(double) * iodata->Nbase*iodata->tilesz;

    len += sizeof(int);
    len += 8*sizeof(double) * iodata->Nbase*iodata->tilesz;

    len += sizeof(int);
    len += 8*iodata->Nchan*sizeof(double) * iodata->Nbase*iodata->tilesz;

    len += sizeof(int);
    len += sizeof(double)*iodata->Nchan;
    len += sizeof(double);

    len += sizeof(int);
    len += sizeof(double)*iodata->Nbase*iodata->tilesz;
    len += 2*sizeof(double);

    return len;
}

void write_data(char **dst, void *src, int len) {
    memcpy(*dst, src, len);
    *dst = *dst + len;
}
void read_data(void *dst, char **src, int len) {
    memcpy(dst, *src, len);
    *src = *src + len;
}

void dump_iodata_dn(dlg_output_info *output, Data::IOData *iodata) {

    char *data, *data0;
    int total_len = len_iodata(iodata);

    if ((data=(char*)malloc(total_len))==0) {
        fprintf(stderr,"%s: %d: no free memory for iodata\n",__FILE__,__LINE__);
        return;
    }
    data0 = data;

    int len = 0;/* this is a flag that indicate share-data file 0*/
    write_data(&data, &len, sizeof(int));
    write_data(&data, &total_len, sizeof(int));
    write_data(&data, &(iodata->M), sizeof(int));
    write_data(&data, &(iodata->Mt), sizeof(int));

    len = strlen(iodata->msname);
    write_data(&data, &len, sizeof(int));
    write_data(&data, iodata->msname, sizeof(char) * strlen(iodata->msname));
    write_data(&data, &(iodata->N), sizeof(int));
    write_data(&data, &(iodata->Nbase), sizeof(int));
    write_data(&data, &(iodata->tilesz), sizeof(int));
    write_data(&data, &(iodata->Nchan), sizeof(int));
    write_data(&data, &(iodata->Nms), sizeof(int));

    len = iodata->Nms;
    write_data(&data, &len, sizeof(int));
    write_data(&data, &(iodata->NchanMS), sizeof(int) * iodata->Nms);

    write_data(&data, &(iodata->deltat), sizeof(double));
    write_data(&data, &(iodata->totalt), sizeof(int));
    write_data(&data, &(iodata->ra0), sizeof(double));
    write_data(&data, &(iodata->dec0), sizeof(double));

    len = iodata->Nbase*iodata->tilesz;
    write_data(&data, &len, sizeof(int));
    write_data(&data, iodata->u, sizeof(double) * len);
    write_data(&data, &len, sizeof(int));
    write_data(&data, iodata->v, sizeof(double) * len);
    write_data(&data, &len, sizeof(int));
    write_data(&data, iodata->w, sizeof(double) * len);

    len = 8*len;
    write_data(&data, &len, sizeof(int));
    write_data(&data, iodata->x, sizeof(double) * len);

    len = len*iodata->Nchan;
    write_data(&data, &len, sizeof(int));
    write_data(&data, iodata->xo, sizeof(double) * len);

    len = iodata->Nchan;
    write_data(&data, &len, sizeof(int));
    write_data(&data, iodata->freqs, sizeof(double) * len);
    write_data(&data, &(iodata->freq0), sizeof(double));

    len = iodata->Nbase*iodata->tilesz;
    write_data(&data, &len, sizeof(int));
    write_data(&data, iodata->flag, sizeof(double) * len);
    write_data(&data, &(iodata->deltaf), sizeof(double));
    write_data(&data, &(iodata->fratio), sizeof(double));

    output->write(data0, total_len);
    free(data0);
    data = NULL;
    data0 = NULL;
}

void load_iodata_dn_noprefix(dlg_input_info *input, Data::IOData *iodata){
    int len = 0;

    int total_len = 0;
    input->read((char *)&total_len, sizeof(int));

    total_len = total_len - 2*sizeof(int);//skip flag
    char *data, *data0;
    if ((data=(char*)malloc(total_len))==0) {
        fprintf(stderr,"%s: %d: no free memory for iodata\n",__FILE__,__LINE__);
        return;
    }
    data0 = data;
    input->read(data, total_len);

    read_data(&(iodata->M), &data, sizeof(int));

    read_data(&(iodata->Mt), &data, sizeof(int));
    read_data(&len, &data, sizeof(int));

    iodata->msname = (char *)malloc(sizeof(char)*(len+1));
    read_data(iodata->msname, &data, sizeof(char) * len);
    iodata->msname[len] = '\0';

    read_data(&(iodata->N), &data, sizeof(int));
    read_data(&(iodata->Nbase), &data, sizeof(int));
    read_data(&(iodata->tilesz), &data, sizeof(int));
    read_data(&(iodata->Nchan), &data, sizeof(int));
    read_data(&(iodata->Nms), &data, sizeof(int));

    read_data(&len, &data, sizeof(int));
    iodata->NchanMS = new int[len];
    read_data(iodata->NchanMS, &data, sizeof(int) * len);

    read_data(&(iodata->deltat), &data, sizeof(double));
    read_data(&(iodata->totalt), &data, sizeof(int));
    read_data(&(iodata->ra0), &data, sizeof(double));
    read_data(&(iodata->dec0), &data, sizeof(double));

    read_data(&len, &data, sizeof(int));
    iodata->u = new double[len];
    read_data(iodata->u, &data, sizeof(double) * len);

    read_data(&len, &data, sizeof(int));
    iodata->v = new double[len];
    read_data(iodata->v, &data, sizeof(double) * len);

    read_data(&len, &data, sizeof(int));
    iodata->w = new double[len];
    read_data(iodata->w, &data, sizeof(double) * len);

    read_data(&len, &data, sizeof(int));
    iodata->x = new double[len];
    read_data(iodata->x, &data, sizeof(double) * len);

    read_data(&len, &data, sizeof(int));
    iodata->xo = new double[len];
    read_data(iodata->xo, &data, sizeof(double) * len);

    read_data(&len, &data, sizeof(int));
    iodata->freqs = new double[len];
    read_data(iodata->freqs, &data, sizeof(double) * len);

    read_data(&(iodata->freq0), &data, sizeof(double));

    read_data(&len, &data, sizeof(int));
    iodata->flag = new double[len];
    read_data(iodata->flag, &data, sizeof(double) * len);

    read_data(&(iodata->deltaf), &data, sizeof(double));
    read_data(&(iodata->fratio), &data, sizeof(double));

    free(data0);
    data = NULL;
    data0 = NULL;
}
void load_iodata_dn(dlg_input_info *input, Data::IOData *iodata){
    int len = 0;
    input->read((char *)&len, sizeof(int));
    load_iodata_dn_noprefix(input, iodata);
}
void dump_beam_dn(dlg_output_info *output, Data::IOData *iodata, Data::LBeam *lBeam) {
    int len = iodata->tilesz;
    int total_len = 2 * sizeof(int) + sizeof(double) * len
                    + 2 * sizeof(int) * iodata->N
                    + 3 * sizeof(double) * iodata->N;

    for(int i=0;i< iodata->N;i++) {
        total_len += 3*sizeof(double) * lBeam->len_xyz[i];
    }
    char *data, *data0;
    if ((data=(char*)malloc(total_len))==0) {
        fprintf(stderr,"%s: %d: no free memory for iodata\n",__FILE__,__LINE__);
        return;
    }
    data0 = data;
    write_data(&data, &total_len, sizeof(int));
    write_data(&data, &len, sizeof(int));
    write_data(&data, lBeam->time_utc, sizeof(double) * len);

    len = iodata->N;
    write_data(&data, &len, sizeof(int));
    write_data(&data, lBeam->Nelem, sizeof(int) * len);
    write_data(&data, lBeam->sx, sizeof(double) * len);
    write_data(&data, lBeam->sy, sizeof(double) * len);
    write_data(&data, lBeam->sz, sizeof(double) * len);
    write_data(&data, lBeam->len_xyz, sizeof(int) * len);

    for(int i=0;i< iodata->N;i++) {
        write_data(&data, lBeam->xx[i], sizeof(double) * lBeam->len_xyz[i]);
        write_data(&data, lBeam->yy[i], sizeof(double) * lBeam->len_xyz[i]);
        write_data(&data, lBeam->zz[i], sizeof(double) * lBeam->len_xyz[i]);
    }

    write_data(&data, &(lBeam->p_ra0), sizeof(double));
    write_data(&data, &(lBeam->p_dec0), sizeof(double));

    output->write(data0, total_len);
    free(data0);
    data = NULL;
    data0 = NULL;
}

void load_beam_dn(dlg_input_info *input, Data::LBeam *lBeam){

    int total_len = 0;

    input->read((char *)&total_len, sizeof(int));
    total_len = total_len - sizeof(int);

    char *data, *data0;
    if ((data=(char*)malloc(total_len))==0) {
        fprintf(stderr,"%s: %d: no free memory for iodata\n",__FILE__,__LINE__);
        return;
    }
    data0 = data;
    input->read(data, total_len);

    int len = 0;
    read_data(&len, &data, sizeof(int));
    lBeam->time_utc = new double[len];
    read_data(lBeam->time_utc, &data, sizeof(double) * len);

    read_data(&len, &data, sizeof(int));
    lBeam->Nelem = new int[len];
    read_data(lBeam->Nelem, &data, sizeof(int) * len);

    lBeam->sx = new double[len];
    read_data(lBeam->sx, &data, sizeof(double) * len);

    lBeam->sy = new double[len];
    read_data(lBeam->sy, &data, sizeof(double) * len);

    lBeam->sz = new double[len];
    read_data(lBeam->sz, &data, sizeof(double) * len);

    lBeam->len_xyz = new int[len];
    read_data(lBeam->len_xyz, &data, sizeof(int) * len);

    lBeam->xx = new double *[len];
    lBeam->yy = new double *[len];
    lBeam->zz = new double *[len];

    for(int i=0; i < len;i++) {
        lBeam->xx[i] = new double[lBeam->len_xyz[i]];
        lBeam->yy[i] = new double[lBeam->len_xyz[i]];
        lBeam->zz[i] = new double[lBeam->len_xyz[i]];
        read_data(lBeam->xx[i], &data, sizeof(double) * lBeam->len_xyz[i]);
        read_data(lBeam->yy[i], &data, sizeof(double) * lBeam->len_xyz[i]);
        read_data(lBeam->zz[i], &data, sizeof(double) * lBeam->len_xyz[i]);
    }

    read_data(&(lBeam->p_ra0), &data, sizeof(double));
    read_data(&(lBeam->p_dec0), &data, sizeof(double));
    free(data0);
    data = NULL;
    data0 = NULL;
}

void dump_barr_dn(dlg_output_info *output, Data::IOData *iodata, baseline_t *barr){
    int total_len = iodata->Nbase * iodata->tilesz * 2 * sizeof(int)
        + iodata->Nbase * iodata->tilesz * sizeof(unsigned char);

    char *data, *data0;
    if ((data=(char*)malloc(total_len))==0) {
        fprintf(stderr,"%s: %d: no free memory for barr\n",__FILE__,__LINE__);
        return;
    } 
    data0 = data;    

    for(int i=0;i<iodata->Nbase * iodata->tilesz;i++) {
        write_data(&data, &(barr[i].sta1), sizeof(int));
        write_data(&data, &(barr[i].sta2), sizeof(int));
        write_data(&data, &(barr[i].flag), sizeof(unsigned char));
    }

    output->write(data0, total_len);
    free(data0);
    data = NULL;
    data0 = NULL;
}

void load_barr_dn(dlg_input_info *input, Data::IOData *iodata, baseline_t *barr){
    int total_len = iodata->Nbase * iodata->tilesz * 2 * sizeof(int)
        + iodata->Nbase * iodata->tilesz * sizeof(unsigned char);

    char *data, *data0;
    if ((data=(char*)malloc(total_len))==0) {
        fprintf(stderr,"%s: %d: no free memory for barr\n",__FILE__,__LINE__);
        return;
    }   
    data0 = data;

    input->read(data, total_len);
    for(int i=0;i<iodata->Nbase * iodata->tilesz;i++) {
        read_data(&(barr[i].sta1), &data, sizeof(int));
        read_data(&(barr[i].sta2), &data, sizeof(int));
        read_data(&(barr[i].flag), &data, sizeof(unsigned char));
    }
    free(data0);
    data = NULL;
    data0 = NULL;
}

void dump_coh_dn(dlg_output_info *output, Data::IOData *iodata, complex double *coh) {
    output->write((char *)coh, sizeof(complex double) * iodata->M * iodata->Nbase * iodata->tilesz * 4);
}

void load_coh_dn(dlg_input_info *input, Data::IOData *iodata, complex double *coh) {
    input->read((char *)coh, sizeof(complex double) * iodata->M * iodata->Nbase * iodata->tilesz * 4);
}

void dump_mpidata_dn(dlg_output_info *output, Data::MPIData *iodata) {

    int total_len = 9 * sizeof(int)
        + iodata->Nms * sizeof(double) + sizeof(double);

    char *data, *data0;
    if ((data=(char*)malloc(total_len))==0) {
        fprintf(stderr,"%s: %d: no free memory for MPIData\n",__FILE__,__LINE__);
        return;
    }  
    data0 = data;

    int len = 1;
    write_data(&data, &len, sizeof(int));/* this is a flag that indicate master share-data file 1*/
    write_data(&data, &total_len, sizeof(int));

    write_data(&data, &(iodata->N), sizeof(int));
    write_data(&data, &(iodata->M), sizeof(int));
    write_data(&data, &(iodata->tilesz), sizeof(int));
    write_data(&data, &(iodata->Nms), sizeof(int));
    write_data(&data, &(iodata->totalt), sizeof(int));

    len = iodata->Nms;
    write_data(&data, &len, sizeof(int));
    write_data(&data, iodata->freqs, sizeof(double) * len);

    write_data(&data, &(iodata->freq0), sizeof(double));
    write_data(&data, &(iodata->Mo), sizeof(int));

    output->write(data0, total_len);

    free(data0);
    data = NULL;
    data0 = NULL;
}

void load_mpidata_dn_noprefix(dlg_input_info *input, Data::MPIData *iodata) {
    int len = 0;
    int total_len = 0;
    input->read((char *)&total_len, sizeof(int));
    total_len = total_len - 2*sizeof(int);
    char *data, *data0;
    if ((data=(char*)malloc(total_len))==0) {
        fprintf(stderr,"%s: %d: no free memory for MPIData\n",__FILE__,__LINE__);
        return;
    }  
    data0 = data;
    input->read(data, total_len);

    read_data((char *)&(iodata->N), &data, sizeof(int));
    read_data((char *)&(iodata->M), &data, sizeof(int));
    read_data((char *)&(iodata->tilesz), &data, sizeof(int));
    read_data((char *)&(iodata->Nms), &data, sizeof(int));
    read_data((char *)&(iodata->totalt), &data, sizeof(int));

    read_data((char *)&len, &data, sizeof(int));
    iodata->freqs = new double[len];
    read_data((char *)iodata->freqs, &data, sizeof(double) * len);
    read_data((char *)&(iodata->freq0), &data, sizeof(double));
    read_data((char *)&(iodata->Mo), &data, sizeof(int));

    free(data0);
    data = NULL;
    data0 = NULL;
}

void load_mpidata_dn(dlg_input_info *input, Data::MPIData *iodata) {
    int len = 0;
    input->read((char *)&len, sizeof(int));/*skip flag*/
    load_mpidata_dn_noprefix(input, iodata);
}