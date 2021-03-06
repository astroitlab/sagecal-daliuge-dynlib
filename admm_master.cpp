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


int admm_master(dlg_app_info *app) {

    Data::MPIData iodata;

    iodata.tilesz = Data::TileSize;
    vector <string> msnames;
    vector <string> inputs;
    vector <struct MsIndex> msIndexs;

    int ntasks = 0;
    /**********************************************************/
    ntasks = app->n_inputs;
    iodata.Nms = app->n_inputs;
    /**** get info from slaves ***************************************/

    iodata.freqs = new double[iodata.Nms];
    iodata.freq0 = 0.0;
    iodata.N = iodata.M = iodata.totalt = 0;
    int Mo = 0;
    /* use iodata to store the results, also check for consistency of results */

    for (unsigned int cm = 0; cm < app->n_inputs; cm++) {
        Data::IOData data;
        load_iodata_dn(&(app->inputs[cm]), &data);

        cout << "Slave [" << data.msname << "] N=" << data.N << " M/Mt=" << data.M << "/" << data.Mt << " tilesz="
             << data.tilesz << " totaltime=" << data.totalt << " Freq=" << data.freq0 / 1e6<< "Mhz"<< endl;

        msnames.push_back(string(data.msname));
        struct MsIndex msIndex;
        msIndex.cm = cm;

        char ms_full_name[256];
        sprintf(ms_full_name,"%s", data.msname);
        ms_short_name(ms_full_name, msIndex.ms);
        msIndexs.push_back(msIndex);

        if (cm == 0) { /* update data */
            iodata.N = data.N;
            Mo = data.M;
            iodata.M = data.Mt;
            iodata.tilesz = data.tilesz;
            iodata.totalt = data.totalt;
        } else { /* compare against others */
            if ((iodata.N != data.N) || (iodata.M != data.Mt) || (iodata.tilesz != data.tilesz)) {
                cout << "Slave " << data.msname << " parameters do not match  N=" << data.N << " M=" << data.M
                     << " tilesz=" << data.tilesz << endl;
            }
            if (iodata.totalt < data.totalt) {
                /* use max value as total time */
                iodata.totalt = data.totalt;
            }
        }
        iodata.freqs[cm] = data.freq0;
        iodata.freq0 += data.freq0;

        Data::freeData(data);
    }
    for (std::vector <struct MsIndex>::iterator iter = msIndexs.begin (); iter != msIndexs.end (); iter++) {
        cout << (*iter).cm << "," << (*iter).ms << endl;
    }
    iodata.freq0 /= (double) iodata.Nms;

    cout << "Reference frequency (MHz)=" << iodata.freq0 * 1.0e-6 << endl;

    /* regularization factor array, size Mx1 one per each hybrid cluster */
    double *arho, *arhoslave;
    if ((arho = (double *) calloc((size_t) iodata.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    if ((arhoslave = (double *) calloc((size_t) Mo, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }

    /* if text file is given, read it and update rho array */
    if (Data::admm_rho_file) {
        read_arho_fromfile(Data::admm_rho_file, iodata.M, arho, Mo, arhoslave);
    } else {
        /* copy common value */
        /* setup regularization factor array */
        for (int p = 0; p < iodata.M; p++) {
            arho[p] = admm_rho;
        }
        for (int p = 0; p < Mo; p++) {
            arhoslave[p] = admm_rho;
        }
    }
    /* interpolation polynomial */
    double *B, *Bi;
    /* Npoly terms, for each frequency, so Npoly x Nms */
    if ((B = (double *) calloc((size_t) Npoly * iodata.Nms, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(B, 0, sizeof(double) * Npoly * iodata.Nms);
    /* pseudoinverse */
    if ((Bi = (double *) calloc((size_t) Npoly * Npoly, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(Bi, 0, sizeof(double) * Npoly * Npoly);

    setup_polynomials(B, Npoly, iodata.Nms, iodata.freqs, iodata.freq0, (Npoly == 1 ? 1 : PolyType));
    /* determine how many iterations are needed */
    int Ntime = (iodata.totalt + iodata.tilesz - 1) / iodata.tilesz;
    /* override if input limit is given */
    if (Nmaxtime > 0 && Ntime > Nmaxtime) {
        Ntime = Nmaxtime;
    }
    cout << "Master total timeslots=" << Ntime << endl;
    if (!Data::admm_rho_file) {
        cout << "ADMM iterations=" << Nadmm << " polynomial order=" << Npoly << " regularization=" << admm_rho << endl;
    } else {
        cout << "ADMM iterations=" << Nadmm << " polynomial order=" << Npoly << " regularization given by text file "
             << Data::admm_rho_file << endl;
    }
    /* send array to slaves */
    /* update rho on each slave */
    iodata.Mo = Mo;

    /*------------------------------------share data file-------------------------------------------------------------*/
    char shareFile[256];
    sprintf(shareFile,"%s/v.mastershare",Data::shareDir);

    FILE *sp = 0;
    if ((sp = fopen(shareFile, "wb")) == 0) {
        fprintf(stderr, "%s: %d: no output file\n", __FILE__, __LINE__);
        return 1;
    }
    dump_mpidata(sp, &iodata);

    fwrite(&ntasks, sizeof(int), 1, sp);
    for (unsigned int cm = 0; cm < msIndexs.size(); cm++) {
        fwrite(&(msIndexs[cm]), sizeof(struct MsIndex), 1, sp);
    }
    if (sp) {
        fclose(sp);
    }
    /* dump single mpidata*/
    dump_share_mpidata(Data::shareDir, &iodata);
    write_share_XYZ(Data::shareDir, "master", arho, iodata.M, "arho");
    write_share_XYZ(Data::shareDir, "master", B, Npoly * iodata.Nms, "B");
    write_share_XYZ(Data::shareDir, "master", Bi, Npoly * Npoly, "Bi");

    /* ADMM memory */
    double *Z, *Y, *z;
    /* Z: 2Nx2 x Npoly x M */
    /* keep ordered by M (one direction together) */
    if ((Z = (double *) calloc((size_t) iodata.N * 8 * Npoly * iodata.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(Z, 0, sizeof(double) * iodata.N * 8 * Npoly * iodata.M);

    /* z : 2Nx2 x M x Npoly vector, so each block is 8NM */
    if ((z = (double *) calloc((size_t) iodata.N * 8 * Npoly * iodata.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(z, 0, sizeof(double) * iodata.N * 8 * Npoly * iodata.M);
    /* copy of Y+rho J, M times, for each slave */
    /* keep ordered by M (one direction together) */
    if ((Y = (double *) calloc((size_t) iodata.N * 8 * iodata.M * iodata.Nms, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(Y, 0, sizeof(double) * iodata.N * 8 * iodata.M * iodata.Nms);

    /* need a copy to calculate dual residual */
    double *Zold;
    if ((Zold = (double *) calloc((size_t) iodata.N * 8 * Npoly * iodata.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(Zold, 0, sizeof(double) * iodata.N * 8 * Npoly * iodata.M);

    double *fratio;
    if ((fratio = (double *) calloc((size_t) iodata.Nms, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(fratio, 0, sizeof(double) * iodata.Nms);

    write_share_XYZ(Data::shareDir, "master", Z, iodata.N * 8 * Npoly * iodata.M, "Z");
    write_share_XYZ(Data::shareDir, "master", z, iodata.N * 8 * Npoly * iodata.M, "z");
    write_share_XYZ(Data::shareDir, "master", Y, iodata.N * 8 * iodata.M * iodata.Nms, "Y");
    write_share_XYZ(Data::shareDir, "master", Zold, iodata.N * 8 * Npoly * iodata.M, "Zold");
    write_share_XYZ(Data::shareDir, "master", fratio, iodata.Nms, "fratio");
    /*------------------------------------output data file------------------------------------------------------------*/
    dump_mpidata_dn(&(app->outputs[0]), &iodata);
    app->outputs[0].write((char *) arhoslave, sizeof(double) * iodata.Mo);
    /*-----------------------------------------------free ------------------------------------------------------------*/
    delete[] iodata.freqs;
    free(B);
    free(arhoslave);
    free(arho);
    free(Z);
    free(Zold);
    free(z);
    free(fratio);
    cout << "[admm_master]=========, Done." << endl;
    return 0;
}

