#include "data.h"
#include "cmd_slave.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>

#include "sagecal.h"
#include "utils.h"
#include "utils_dn.h"

using namespace std;
using namespace Data;

int admm_slave(dlg_app_info *app) {
    /*---------------------------------------------2 input------------------------------------------------------------*/
    Data::IOData iodata;
    Data::MPIData mpiData;

    int len = 0;
    app->inputs->read((char *)&len, sizeof(int));
    if(len != 0) {
        load_mpidata_dn_noprefix(app->inputs, &mpiData);
        load_iodata_dn(++(app->inputs), &iodata);
        --(app->inputs);
    } else {
        load_iodata_dn_noprefix(app->inputs, &iodata);
        load_mpidata_dn(++(app->inputs), &mpiData);
    }

    cout << "[admm_slave]===, iodata.N/M/Mt:" << iodata.N << "/" << iodata.M  << "/" << iodata.Mt << ", iodata.freq0:" << iodata.freq0/ 1e6<< "Mhz" << endl;
    cout << "[admm_slave]===, mpiData.N:" << mpiData.N << ", mpiData.freq0:" << mpiData.freq0/ 1e6<< "Mhz" << endl;

    double *arho, *arho0;
    if ((arho = (double *) calloc((size_t) mpiData.Mo, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    (app->inputs)->read((char *)arho, sizeof(double) * mpiData.Mo);

    if ((arho0 = (double *) calloc((size_t) mpiData.Mo, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memcpy(arho0, arho, (size_t) mpiData.Mo * sizeof(double));

    /*------------------------------------output----------------------------------------------------------------------*/
    dump_iodata_dn(&(app->outputs[0]), &iodata);
    dump_mpidata_dn(&(app->outputs[0]), &mpiData);
    app->outputs[0].write((char *)arho, sizeof(double) * mpiData.Mo);
    /*--------------------------------------------  share -------------------------------------------------------*/
    write_share_XYZ(Data::shareDir, iodata.msname, arho, iodata.M, "arho");
    write_share_XYZ(Data::shareDir, iodata.msname, arho0, iodata.M, "arho0");

    double res_0, res_1, res_00, res_01,mean_nu;
    int start_iter = 1, tilex = 0;
    res_0 = res_1 = res_00 = res_01 = mean_nu = 0.0;
    dump_share_res(Data::shareDir, iodata.msname, &start_iter, &res_0, &res_1, &res_00, &res_01, &mean_nu, &tilex);
    double *Z, *Y;
    /* Z: (store B_f Z) 2Nx2 x M */
    if ((Z = (double *) calloc((size_t) iodata.N * 8 * mpiData.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(Z, 0, sizeof(double) * iodata.N * 8 * mpiData.M);
    /* Y, 2Nx2 , M times */
    if ((Y = (double *) calloc((size_t) iodata.N * 8 * mpiData.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(Y, 0, sizeof(double) * iodata.N * 8 * mpiData.M);

    double *pres;
    if ((pres = (double *) calloc((size_t) iodata.N * 8 * mpiData.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(pres, 0, sizeof(double) * iodata.N * 8 * mpiData.M);

    complex double *coh;
    /* coherencies */
    if ((coh = (complex double *) calloc((size_t)(iodata.M * iodata.Nbase * iodata.tilesz * 4), sizeof(complex double)))==0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    double *xbackup = 0;
    if (iodata.Nchan > 1 || Data::whiten) {
        if ((xbackup = (double *) calloc((size_t) iodata.Nbase * 8 * iodata.tilesz, sizeof(double))) == 0) {
            fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
            exit(1);
        }
        memset(xbackup, 0, sizeof(double) * iodata.Nbase * 8 * iodata.tilesz);
        write_share_XYZ(Data::shareDir, iodata.msname, xbackup, iodata.Nbase * 8 * iodata.tilesz, "xbackup");
    }

    write_share_XYZ(Data::shareDir, iodata.msname, Y, iodata.N * 8 * mpiData.M, "Y");
    write_share_XYZ(Data::shareDir, iodata.msname, Z, iodata.N * 8 * mpiData.M, "Z");

    write_share_XYZ(Data::shareDir, iodata.msname, pres, iodata.N * 8 * mpiData.M, "pres");

    dump_share_coh(Data::shareDir,iodata.msname,&iodata,coh);
    /*--------------------------------------------       free  -------------------------------------------------------*/
    free(Y);
    free(Z);
    free(arho);
    free(pres);
    free(coh);
    free(xbackup);
    Data::freeData(iodata);
    delete[] mpiData.freqs;
    cout << "[admm_slave]=========, Done." << endl;
    return 0;
}

