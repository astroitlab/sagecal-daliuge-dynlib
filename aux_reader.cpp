#include "data.h"
#include "cmd_slave.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "sagecal.h"
#include "utils.h"
#include "utils_dn.h"

using namespace std;
using namespace Data;

int aux_reader(dlg_app_info *app) {
    char table_name[256];
    int len = app->inputs[0].read(table_name,255);
    cout << "read len:" << len << endl;

    table_name[len] = '\0';

    Data::IOData iodata;
    Data::LBeam beam;
    iodata.tilesz=Data::TileSize;
    iodata.deltat=1.0;
    iodata.msname = (char *)malloc(sizeof(char) * (strlen(table_name)-1));
    snprintf(iodata.msname, strlen(table_name)-1, "%s", table_name);
    iodata.msname[strlen(table_name)-1] = '\0';
    if (Data::SkyModel && Data::Clusters) {
        cout << "Using SkyModel: " << Data::SkyModel << ", Clusters: " << Clusters << ", table_name: " << iodata.msname << endl;
    } else {
        cout << "ERROR,  SkyModel: " << Data::SkyModel << ", Clusters: " << Clusters << ", table_name: " << iodata.msname << endl;
    }
    if (!Data::SkyModel || !Data::Clusters) {
        exit(1);
    }
    cout << "Selecting baselines > " << Data::min_uvcut << " and < " << Data::max_uvcut << " wavelengths." << endl;
    if (!DoSim) {
        cout << "Using ";
        if (Data::solver_mode == SM_LM_LBFGS || Data::solver_mode == SM_OSLM_LBFGS || Data::solver_mode == SM_RTR_OSLM_LBFGS ||
            Data::solver_mode == SM_NSD_RLBFGS) {
            cout << "Gaussian noise model for solver." << endl;
        } else {
            cout << "Robust noise model for solver with degrees of freedom [" << Data::nulow << "," << Data::nuhigh << "]." << endl;
        }
    } else {
        cout << "Only doing simulation (with possible correction for cluster id " << Data::ccid << ")." << endl;
    }

    openblas_set_num_threads(1);

    if (!doBeam) {
        readAuxData(iodata.msname, &iodata);
    } else {
        readAuxData(iodata.msname, &iodata, &beam);
    }

    /**********************************************************/
    int M, Mt, ci, cj, ck;
    double *p, *pinit;
    double **pm;

    clus_source_t *carr;
    baseline_t *barr;
    read_sky_cluster(Data::SkyModel, Data::Clusters, &carr, &M, iodata.freq0, iodata.ra0, iodata.dec0, Data::format);
    /* exit if there are 0 clusters (incorrect sky model/ cluster file)*/
    if (M <= 0) {
        fprintf(stderr, "%s: %d: no clusters to solve\n", __FILE__, __LINE__);
        exit(1);
    } else {
        printf("Got %d clusters\n", M);
    }
    /* array to store baseline->sta1,sta2 map */
    if ((barr = (baseline_t *) calloc((size_t) iodata.Nbase * iodata.tilesz, sizeof(baseline_t))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    generate_baselines(iodata.Nbase, iodata.tilesz, iodata.N, barr, Data::Nt);

    /* calculate actual no of parameters needed,
     this could be > M */
    Mt = 0;
    for (ci = 0; ci < M; ci++) {
        //printf("cluster %d has %d time chunks\n",carr[ci].id,carr[ci].nchunk);
        Mt += carr[ci].nchunk;
    }
    printf("Total effective clusters: %d\n", Mt);

    /* parameters 8*N*M ==> 8*N*Mt */
    if ((p = (double *) calloc((size_t) iodata.N * 8 * Mt, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    /* update cluster array with correct pointers to parameters */
    cj = 0;
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
    /* pointers to parameters */
    if ((pm = (double **) calloc((size_t) Mt, sizeof(double *))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    /* setup the pointers */
    for (ci = 0; ci < Mt; ci++) {
        pm[ci] = &(p[ci * 8 * iodata.N]);
    }
    /* initilize parameters to [1,0,0,0,0,0,1,0] */
    for (ci = 0; ci < Mt; ci++) {
        for (cj = 0; cj < iodata.N; cj++) {
            pm[ci][8 * cj] = 1.0;
            pm[ci][8 * cj + 6] = 1.0;
        }
    }
    /* backup of default initial values */
    if ((pinit = (double *) calloc((size_t) iodata.N * 8 * Mt, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memcpy(pinit, p, (size_t) iodata.N * 8 * Mt * sizeof(double));

    /**********************************************************/
    /* timeinterval in seconds */
    cout << "For " << iodata.tilesz << " samples, solution time interval (s): "
         << iodata.deltat * (double) iodata.tilesz << endl;
    cout << "Freq: " << iodata.freq0 / 1e6 << " MHz, Chan: " << iodata.Nchan << " Bandwidth: " << iodata.deltaf / 1e6
         << " MHz" << ", Nms: " << iodata.Nms << endl;

    iodata.M = M;
    iodata.Mt = Mt;
    /*------------------------------------share data file-------------------------------------------------------------*/

    dump_share_iodata(Data::shareDir, iodata.msname, &iodata);
    if (doBeam) {
        dump_share_beam(Data::shareDir, iodata.msname, &iodata, &beam);
    }
    dump_share_barr(Data::shareDir, iodata.msname, &iodata, barr);
    write_share_XYZ(Data::shareDir, iodata.msname, p, iodata.N * 8 * Mt, "p");
    write_share_XYZ(Data::shareDir, iodata.msname, pinit, iodata.N * 8 * Mt, "pinit");

    /*------------------------------------output----------------------------------------------------------------------*/

    dump_iodata_dn(&(app->outputs[0]), &iodata);
    if (Data::doBeam) {
        dump_beam_dn(&(app->outputs[0]), &iodata, &beam);
    }

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
    free(p);
    free(pinit);
    free(pm);
    /* free data memory */
    if (!doBeam) {
        Data::freeData(iodata);
    } else {
        Data::freeData(iodata, beam);
    }

    cout << "[aux_reader]========, Done." << endl;
    return 0;
}

