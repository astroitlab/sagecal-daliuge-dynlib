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

int write_z_master(dlg_app_info *app) {
    int first_iter = get_last_iter(app->uid);
    Data::MPIData iodata;
    load_share_mpidata(Data::shareDir, &iodata);

    FILE *sfp = 0;
    if (solfile) {
        char solutionFile[256];
        sprintf(solutionFile,"%s/master.solution",Data::solfile);
        if ((sfp = fopen(solutionFile, "a+")) == 0) {
            fprintf(stderr, "%s: %d: no file\n", __FILE__, __LINE__);
            return 1;
        }
    }
    /* write additional info to solution file */
    if (solfile && first_iter == 0) {
        fprintf(sfp, "# solution file (Z) created by SAGECal-Daliuge\n");
        fprintf(sfp, "# reference_freq(MHz) polynomial_order stations clusters effective_clusters\n");
        fprintf(sfp, "%lf %d %d %d %d\n", iodata.freq0 * 1e-6, Npoly, iodata.N, iodata.Mo, iodata.M);
        fclose(sfp);
        delete[] iodata.freqs;
        cout << "[write_z_master]=======, Done." << endl;
        return 0;
    }

    /*------------------------------------share ----------------------------------------------------------------------*/
    double *Z;
    /* Z: 2Nx2 x Npoly x M */
    /* keep ordered by M (one direction together) */
    if ((Z = (double *) calloc((size_t) iodata.N * 8 * Npoly * iodata.M, sizeof(double))) == 0) {
        fprintf(stderr, "%s: %d: no free memory\n", __FILE__, __LINE__);
        exit(1);
    }

    read_share_XYZ(Data::shareDir, (char *)"master", Z, iodata.N * 8 * Npoly * iodata.M, "Z");
    /*----------------------------------------processing--------------------------------------------------------------*/

    /* write Z to solution file, same format as J, but we have Npoly times more
       values per timeslot per column */
    if (solfile) {
        for (int p = 0; p < iodata.N * 8 * Npoly; p++) {
            fprintf(sfp, "%d ", p);
            for (int pp = 0; pp < iodata.M; pp++) {
                fprintf(sfp, " %e", Z[pp * iodata.N * 8 * Npoly + p]);
            }
            fprintf(sfp, "\n");
        }
        fclose(sfp);
    }
    int resetcount = 0, msgcode = 0;
    for (int cm = 0; cm < iodata.Nms; cm++) {
        /* to-do : how to fetch msgcode from slave*/
        if (msgcode == CTRL_RESET) {
            resetcount++;
        }
    }
    if (resetcount > iodata.Nms / 2) {
        /* if most slaves have reset, print a warning only */
        //memset(Z,0,sizeof(double)*(size_t)iodata.N*8*Npoly*iodata.M);
        cout << "Resetting Global Solution" << endl;
    }

    /*-------------------------------------------output---------------- ----------------------------------------------*/
    int flag = 2;
    app->outputs[0].write((char *)&flag, sizeof(int));
    /*-------------------------------------------free  ---------------------------------------------------------------*/
    delete[] iodata.freqs;
    free(Z);
    cout << "[write_z_master]=======, Done." << endl;
    return 0;
}

