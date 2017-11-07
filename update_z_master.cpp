//
// Created by wsl on 11/07/17.
//
#include "data.h"
#include "cmd_master.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>

#include "sagecal.h"
#include "utils.h"
#include "utils_dn.h"

using namespace std;
using namespace Data;

int update_z_master(dlg_app_info *app) {

    vector <struct MsIndex> msIndexs;
    Data::MPIData mpiData;
    int ntasks = 0;
    int admm = get_last_iter(app->uid), ct = get_second_last_iter(app->uid);

    cout << "[update_z_master]=======, admm=" << admm << ",ct=" << ct << endl;
    /*------------------------------------read share------------------------------------------------------------------*/
    char shareFile[256];
    sprintf(shareFile,"%s/v.mastershare",Data::shareDir);

    FILE *sp = 0;
    if ((sp = fopen(shareFile, "rb")) == 0) {
        fprintf(stderr, "%s: %d: no output file\n", __FILE__, __LINE__);
        return 1;
    }
    load_mpidata(sp, &mpiData);
    fread(&ntasks, sizeof(int), 1, sp);

    for (unsigned int cm = 0; cm <ntasks; cm++) {
        struct MsIndex msIndex = {};
        fread(&msIndex, sizeof(struct MsIndex), 1, sp);
        msIndexs.push_back(msIndex);
    }
    if (sp) {
        fclose(sp);
    }
    /*------------------------------------var init--------------------------------------------------------------------*/
    double *arho;
    if ((arho = (double *) calloc((size_t) mpiData.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    double *B, *Bi;
    /* Npoly terms, for each frequency, so Npoly x Nms */
    if ((B = (double *) calloc((size_t) Npoly * mpiData.Nms, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    /* pseudoinverse */
    if ((Bi = (double *) calloc((size_t) Npoly * Npoly, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    double *Z, *Y, *z;
    /* Z: 2Nx2 x Npoly x M */
    /* keep ordered by M (one direction together) */
    if ((Z = (double *) calloc((size_t) mpiData.N * 8 * Npoly * mpiData.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    /* z : 2Nx2 x M x Npoly vector, so each block is 8NM */
    if ((z = (double *) calloc((size_t) mpiData.N * 8 * Npoly * mpiData.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    /* Y, 2Nx2 , M times */
    if ((Y = (double *) calloc((size_t) mpiData.N * 8 * mpiData.M * mpiData.Nms, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    /* need a copy to calculate dual residual */
    double *Zold;
    if ((Zold = (double *) calloc((size_t) mpiData.N * 8 * Npoly * mpiData.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }

    read_share_XYZ(Data::shareDir, "master", arho, mpiData.M, "arho");
    read_share_XYZ(Data::shareDir, "master", B, Npoly * mpiData.Nms, "B");
    read_share_XYZ(Data::shareDir, "master", Bi, Npoly * Npoly, "Bi");
    read_share_XYZ(Data::shareDir, "master", Z , mpiData.N * 8 * Npoly * mpiData.M, "Z");
    read_share_XYZ(Data::shareDir, "master", z, mpiData.N * 8 * Npoly * mpiData.M, "z");
    read_share_XYZ(Data::shareDir, "master", Y, mpiData.N * 8 * mpiData.M * mpiData.Nms, "Y");
    read_share_XYZ(Data::shareDir, "master", Zold, mpiData.N * 8 * Npoly * mpiData.M, "Zold");
    /*------------------------------------input slave update Y--------------------------------------------------------*/

    int flag = 0;
    for (unsigned int cm = 0; cm < app->n_inputs; cm++) {
        app->inputs[cm].read((char *)&flag, sizeof(int));
        if (flag == 0) {
            Data::IOData iodata;
            load_iodata_dn_noprefix(&(app->inputs[cm]), &iodata);
//            cout << "[update_z_master]======,"<< iodata.msname <<",iodata.N/M/Mt/Nms:" << iodata.N << "/"
//                 << iodata.M  << "/" << iodata.Mt << "/" << iodata.Nms
//                 << ", iodata.freq0:" << iodata.freq0/ 1e6<< "Mhz" << endl;
            char short_name[128], ms_full_name[256];
            sprintf(ms_full_name,"%s", iodata.msname);
            ms_short_name(ms_full_name, short_name);

            for (unsigned int sm = 0; sm < msIndexs.size(); sm++) {
                if(strcmp(short_name, msIndexs[sm].ms)==0) {
                    cout << "[update_z_master]======, find:" << msIndexs[sm].ms << ", idx:" << msIndexs[sm].cm << endl;
                    app->inputs[cm].read((char *)&Y[msIndexs[sm].cm * mpiData.N * 8 * mpiData.M], sizeof(double) * mpiData.N * 8 * mpiData.M);
                    break;
                }
            }
            Data::freeData(iodata);
        }
    }

    /*-----------------------------------process and share output-----------------------------------------------------*/
    if(admm==0) {
        calculate_manifold_average(mpiData.N, mpiData.M, mpiData.Nms, Y, 20, Data::Nt);
        /* send updated Y back to each slave */
        for (int im = 0; im < mpiData.Nms; im++) {
            write_share_XYZ(Data::shareDir, msIndexs[im].ms, &Y[msIndexs[im].cm * mpiData.N * 8 * mpiData.M], mpiData.N * 8 * mpiData.M, "Y");
        }
    }

    /* update Z */
    /* add to 8NM vector, multiplied by Npoly different scalars, Nms times */
    for (int ci = 0; ci < Npoly; ci++) {
        my_dcopy(8 * mpiData.N * mpiData.M, Y, 1, &z[ci * 8 * mpiData.N * mpiData.M], 1);
        my_dscal(8 * mpiData.N * mpiData.M, B[ci], &z[ci * 8 * mpiData.N * mpiData.M]);
    }
    for (int im = 1; im < mpiData.Nms; im++) {
        for (int ci = 0; ci < Npoly; ci++) {
            my_daxpy(8 * mpiData.N * mpiData.M, &Y[im * 8 * mpiData.N * mpiData.M], B[im * Npoly + ci],
                     &z[ci * 8 * mpiData.N * mpiData.M]);
        }
    }
    /* also scale by 1/rho, only if rho>0, otherwise set it to 0.0*/
    for (int im = 0; im < mpiData.M; im++) {
        double invscale = 0.0;
        if (arho[im] > 0.0) {
            invscale = 1.0 / arho[im];
        }
        for (int ci = 0; ci < Npoly; ci++) {
            my_dscal(8 * mpiData.N, invscale, &z[8 * mpiData.N * mpiData.M * ci + 8 * mpiData.N * im]);
        }
    }
    /* find product z_tilde x Bi^T, z_tilde with proper reshaping */
    my_dcopy(mpiData.N * 8 * Npoly * mpiData.M, Z, 1, Zold, 1);
    update_global_z(Z, mpiData.N, mpiData.M, Npoly, z, Bi);
    /* find dual error ||Zold-Znew|| */
    my_daxpy(mpiData.N * 8 * Npoly * mpiData.M, Z, -1.0, Zold);
    /* dual residual per one real parameter */
    if (Data::verbose) {
        cout << "ADMM : " << admm << " dual residual="
             << my_dnrm2(mpiData.N * 8 * Npoly * mpiData.M, Zold) / sqrt((double) 8 * mpiData.N * Npoly * mpiData.M)
             << endl;
    } else {
        cout << "Timeslot:" << ct << " ADMM:" << admm << endl;
    }

    /* send B_i Z to each slave */
    for (int im = 0; im < mpiData.Nms; im++) {
        for (int p = 0; p < mpiData.M; p++) {
            memset(&z[8 * mpiData.N * p], 0, sizeof(double) * (size_t) mpiData.N * 8);
            for (int ci = 0; ci < Npoly; ci++) {
                my_daxpy(8 * mpiData.N, &Z[p * 8 * mpiData.N * Npoly + ci * 8 * mpiData.N], B[im * Npoly + ci],
                         &z[8 * mpiData.N * p]);
            }
        }

        write_share_XYZ(Data::shareDir, msIndexs[im].ms, z, mpiData.N * 8 * mpiData.M, "Z");
    }
    /*-----------------------------------------share output-----------------------------------------------------------*/
    write_share_XYZ(Data::shareDir, "master", B, Npoly * mpiData.Nms, "B");
    write_share_XYZ(Data::shareDir, "master", Bi, Npoly * Npoly, "Bi");
    write_share_XYZ(Data::shareDir, "master", Z , mpiData.N * 8 * Npoly * mpiData.M, "Z");
    write_share_XYZ(Data::shareDir, "master", z, mpiData.N * 8 * Npoly * mpiData.M, "z");
    write_share_XYZ(Data::shareDir, "master", Y, mpiData.N * 8 * mpiData.M * mpiData.Nms, "Y");
    write_share_XYZ(Data::shareDir, "master", Zold, mpiData.N * 8 * Npoly * mpiData.M, "Zold");

    /*-----------------------------------------------output-----------------------------------------------------------*/
    dump_mpidata_dn(&(app->outputs[0]), &mpiData);
    /*-----------------------------------------------free-------------------------------------------------------------*/
    delete[] mpiData.freqs;
    free(Z);
    free(Zold);
    free(z);
    free(Y);
    free(B);
    free(Bi);
    free(arho);
    cout << "[update_z_master]=======, Done." << endl;
    return 0;
}

