#include "data.h"
#include "cmd_slave.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>

#include <sagecal.h>
#include "utils.h"
#include "utils_dn.h"
using namespace std;
using namespace Data;

int coh_slave(dlg_app_info *app) {
    /*-----------------------------------------------input------------------------------------------------------------*/
    Data::IOData iodata;
    Data::LBeam beam;
    Data::MPIData mpiData;
    clus_source_t *carr;
    baseline_t *barr;
    double *arho, *arho0;
    int sources_precessed = get_last_iter(app->uid);
    openblas_set_num_threads(1);

    if(doBeam && sources_precessed!=0)
        sources_precessed = 1;

    load_iodata_dn(&(app->inputs[0]), &iodata);
    load_mpidata_dn(&(app->inputs[0]),&mpiData);

    cout << "[coh_slave]========, iodata.N/M/Mt/Nms:" << iodata.N << "/" << iodata.M  << "/" << iodata.Mt << "/" << iodata.Nms
        << ", iodata.freq0:" << iodata.freq0/ 1e6<< "Mhz" << endl;

    if (Data::doBeam) {
        load_share_beam(Data::shareDir,iodata.msname, &beam);
    }
    if ((barr = (baseline_t *) calloc((size_t) iodata.Nbase * iodata.tilesz, sizeof(baseline_t))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }

    load_share_barr(Data::shareDir,iodata.msname, &iodata, barr);

    int M = mpiData.Mo;

    if ((arho = (double *) calloc((size_t) M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    read_share_XYZ(Data::shareDir, iodata.msname, arho, M, "arho");

    if ((arho0 = (double *) calloc((size_t) M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    read_share_XYZ(Data::shareDir, iodata.msname, arho0, M, "arho0");

    /*----------------------------------------------------------------------------------------------------------------*/
    read_sky_cluster(Data::SkyModel, Data::Clusters, &carr, &M, iodata.freq0, iodata.ra0, iodata.dec0, Data::format);
    if (M <= 0) {
        fprintf(stderr, "%s: %d: no clusters to solve\n", __FILE__, __LINE__);
        exit(1);
    } else {
        printf("%s:Got %d clusters\n", __FILE__, M);
    }
    /* update cluster array with correct pointers to parameters */
    int ci = 0, ck = 0,cj = 0;
    for (ci = 0; ci < M; ci++) {
        if ((carr[ci].p = (int *) calloc((size_t) carr[ci].nchunk, sizeof(int))) == 0) {
            fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
            exit(1);
        }
        for (ck = 0; ck < carr[ci].nchunk; ck++) {
            carr[ci].p[ck] = cj * 8 * iodata.N;
            cj++;
        }
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    double *xbackup = 0;
    if (iodata.Nchan > 1 || Data::whiten) {
        if ((xbackup = (double *) calloc((size_t) iodata.Nbase * 8 * iodata.tilesz, sizeof(double))) == 0) {
            fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
            exit(1);
        }
        read_share_XYZ(Data::shareDir, iodata.msname, xbackup, iodata.Nbase * 8 * iodata.tilesz, "xbackup");
    }

    complex double *coh;
    if ((coh = (complex double *) calloc((size_t)(iodata.M * iodata.Nbase * iodata.tilesz * 4), sizeof(complex double)))==0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    load_share_coh(Data::shareDir, iodata.msname, &iodata, coh);
    /*----------------------------------------------------------------------------------------------------------------*/
    double inv_c = 1.0 / CONST_C;
    /* reweight regularization factors with weight based on flags */
    memcpy(arho, arho0, (size_t) M * sizeof(double));
    my_dscal(M, iodata.fratio, arho);

    /* rescale u,v,w by 1/c NOT to wavelengths, that is done later in prediction */
    my_dscal(iodata.Nbase * iodata.tilesz, inv_c, iodata.u);
    my_dscal(iodata.Nbase * iodata.tilesz, inv_c, iodata.v);
    my_dscal(iodata.Nbase * iodata.tilesz, inv_c, iodata.w);

    /**********************************************************/
    /* update baseline flags */
    /* and set x[]=0 for flagged values */
    preset_flags_and_data(iodata.Nbase * iodata.tilesz, iodata.flag, barr, iodata.x, Data::Nt);
    /* if data is being whitened, whiten x here before copying */
    if (Data::whiten) {
        whiten_data(iodata.Nbase * iodata.tilesz, iodata.x, iodata.u, iodata.v, iodata.freq0, Data::Nt);
    }
    if (iodata.Nchan > 1 || Data::whiten) { /* keep fresh copy of raw data */
        my_dcopy(iodata.Nbase * 8 * iodata.tilesz, iodata.x, 1, xbackup, 1);
    }
    /* precess source locations (also beam pointing) from J2000 to JAPP if we do any beam predictions,
     using first time slot as epoch */
    if (doBeam && !sources_precessed) {
        precess_source_locations(beam.time_utc[iodata.tilesz / 2], carr, M, &beam.p_ra0, &beam.p_dec0, Data::Nt);
        sources_precessed = 1;
    }
    if (!doBeam) {
        precalculate_coherencies(iodata.u, iodata.v, iodata.w, coh, iodata.N, iodata.Nbase * iodata.tilesz, barr,
                                 carr, M, iodata.freq0, iodata.deltaf, iodata.deltat, iodata.dec0, Data::min_uvcut,
                                 Data::max_uvcut, Data::Nt);
    } else {
        precalculate_coherencies_withbeam(iodata.u, iodata.v, iodata.w, coh, iodata.N, iodata.Nbase * iodata.tilesz,
                                          barr, carr, M, iodata.freq0, iodata.deltaf, iodata.deltat, iodata.dec0,
                                          Data::min_uvcut, Data::max_uvcut,
                                          beam.p_ra0, beam.p_dec0, iodata.freq0, beam.sx, beam.sy, beam.time_utc,
                                          iodata.tilesz, beam.Nelem, beam.xx, beam.yy, beam.zz, Data::Nt);
    }

    /*---------------------------------share output-------------------------------------------------------------------*/
    cout << "Data::shareDir:" << Data::shareDir << ",msname:" << iodata.msname << endl;
    dump_share_iodata(Data::shareDir,iodata.msname,&iodata);

    if (Data::doBeam) {
        dump_share_beam(Data::shareDir,iodata.msname,&iodata,&beam);
    }
    dump_share_coh(Data::shareDir,iodata.msname,&iodata,coh);
    dump_share_barr(Data::shareDir,iodata.msname,&iodata,barr);
    write_share_XYZ(Data::shareDir, iodata.msname, arho, iodata.M, "arho");
    if (iodata.Nchan > 1 || Data::whiten) {
        write_share_XYZ(Data::shareDir, iodata.msname, xbackup, iodata.Nbase * 8 * iodata.tilesz, "xbackup");
    }
    /*--------------------------------output  ------------------------------------------------------------------------*/
    dump_iodata_dn(&(app->outputs[0]), &iodata);
    dump_mpidata_dn(&(app->outputs[0]), &mpiData);
    /*------------------------------------free ----------------------------------------------------------------------*/
    exinfo_gaussian *exg;
    exinfo_disk *exd;
    exinfo_ring *exr;
    exinfo_shapelet *exs;
    for (ci = 0; ci < M; ci++) {
        free(carr[ci].ll);
        free(carr[ci].mm);
        free(carr[ci].nn);
        free(carr[ci].sI);
        free(carr[ci].p);
        free(carr[ci].ra);
        free(carr[ci].dec);
        for (cj = 0; cj < carr[ci].N; cj++) {
            /* do a proper typecast before freeing */
            switch (carr[ci].stype[cj]) {
                case STYPE_GAUSSIAN:
                    exg = (exinfo_gaussian *) carr[ci].ex[cj];
                    if (exg) free(exg);
                    break;
                case STYPE_DISK:
                    exd = (exinfo_disk *) carr[ci].ex[cj];
                    if (exd) free(exd);
                    break;
                case STYPE_RING:
                    exr = (exinfo_ring *) carr[ci].ex[cj];
                    if (exr) free(exr);
                    break;
                case STYPE_SHAPELET:
                    exs = (exinfo_shapelet *) carr[ci].ex[cj];
                    if (exs) {
                        if (exs->modes) {
                            free(exs->modes);
                        }
                        free(exs);
                    }
                    break;
                default:
                    break;
            }
        }
        free(carr[ci].ex);
        free(carr[ci].stype);
        free(carr[ci].sI0);
        free(carr[ci].f0);
        free(carr[ci].spec_idx);
        free(carr[ci].spec_idx1);
        free(carr[ci].spec_idx2);
    }
    free(carr);
    free(barr);
    if (iodata.Nchan > 1 || Data::whiten) {
        free(xbackup);
    }
    free(coh);
    free(arho);
    free(arho0);
    if (!doBeam) {
        Data::freeData(iodata);
    } else {
        Data::freeData(iodata, beam);
    }
    delete[] mpiData.freqs;
    cout << "[coh_slave]========,Done." << endl;
    return 0;
}

