//
// Created by wsl on 4/19/17.
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

int fratio_master(dlg_app_info *app) {

    Data::MPIData mpiData;
    int ntasks = 0;
    vector <struct MsIndex> msIndexs;

    /*------------------------------------share data file-------------------------------------------------------------*/
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
        struct MsIndex msIndex;
        fread(&msIndex, sizeof(struct MsIndex), 1, sp);
        msIndexs.push_back(msIndex);
    }
 
    if (sp) {
        fclose(sp);
    }
    /*------------------------------------input ----------------------------------------------------------------------*/
    double *fratio;
    if ((fratio = (double *) calloc((size_t) mpiData.Nms, sizeof(double))) == 0) {
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
    read_share_XYZ(Data::shareDir, "master", fratio, mpiData.Nms, "fratio");
    read_share_XYZ(Data::shareDir, "master", B, Npoly * mpiData.Nms, "B");
    read_share_XYZ(Data::shareDir, "master", Bi, Npoly * Npoly, "Bi");

    int flag = 0;
    for (unsigned int cm = 0; cm < app->n_inputs; cm++) {
        app->inputs[cm].read((char *)&flag, sizeof(int));
        if (flag == 0) {
            Data::IOData data;
            load_iodata_dn_noprefix(&(app->inputs[cm]), &data);
//            cout << "[fratio_master]========, data.N/M/Mt/Nms:" << data.N << "/" << data.M  << "/"
//                 << data.Mt << "/" << data.Nms << ", data.freq0:" << data.freq0/ 1e6<< "Mhz" << endl;
            char short_name[128], ms_full_name[256];
            sprintf(ms_full_name,"%s", data.msname);
            ms_short_name(ms_full_name, short_name);
            for (unsigned int sm = 0; sm < msIndexs.size(); sm++) {
                if(strcmp(short_name, msIndexs[sm].ms)==0) {
                    fratio[msIndexs[sm].cm] = data.fratio;
                    break;
                }
            }
            Data::freeData(data);
        }
    }
    /*---------------------------------------find_prod_inverse--------------------------------------------------------*/
    /* interpolation polynomial */

    find_prod_inverse(B, Bi, Npoly, mpiData.Nms, fratio);

    /*----------------------------------------share output------------------------------------------------------------*/
    write_share_XYZ(Data::shareDir, "master", fratio, mpiData.Nms, "fratio");
    write_share_XYZ(Data::shareDir, "master", B, Npoly * mpiData.Nms, "B");
    write_share_XYZ(Data::shareDir, "master", Bi, Npoly * Npoly, "Bi");

    /*-------------------------------------------output---------------------------------------------------------------*/
    dump_mpidata_dn(&(app->outputs[0]), &mpiData);
    /*---------------------------------------------   free  ----------------------------------------------------------*/
    free(B);
    free(Bi);
    free(fratio);
    delete[] mpiData.freqs;

    cout << "[fratio_master]========,Done." << endl;
    return 0;
}

