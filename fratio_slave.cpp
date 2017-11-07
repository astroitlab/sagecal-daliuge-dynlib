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

int fratio_slave(dlg_app_info *app) {
    /*-----------------------------------------------input------------------------------------------------------------*/
    Data::IOData old_iodata, iodata;
    Data::LBeam beam;
    Data::MPIData mpiData;

    int sources_precessed = get_last_iter(app->uid);
    openblas_set_num_threads(1);

    int flag = 0;
    app->inputs[0].read((char *)&flag, sizeof(int));
    if (flag != 0) {
        load_iodata_dn(&(app->inputs[1]), &old_iodata);
        load_mpidata_dn(&(app->inputs[1]),&mpiData);
    } else {
        load_iodata_dn_noprefix(&(app->inputs[0]), &old_iodata);
        load_mpidata_dn(&(app->inputs[0]),&mpiData);
    }
    load_share_iodata(Data::shareDir, old_iodata.msname, &iodata);
    cout << "[fratio_slave]========, iodata.N/M/Mt/Nms:" << iodata.N << "/" << iodata.M  << "/" << iodata.Mt << "/" << iodata.Nms
        << ", iodata.freq0:" << iodata.freq0/ 1e6<< "Mhz" << endl;

    if (Data::doBeam) {
        load_share_beam(Data::shareDir,iodata.msname, &beam);
    }

    /*----------------------------------------------------------------------------------------------------------------*/
    Block<int> sort(1);
    sort[0] = MS::TIME; /* note: only sort over TIME for ms iterator to work */
    vector < MSIter * > msitr;
    vector < MeasurementSet * > msvector;

    MeasurementSet *ms = new MeasurementSet(iodata.msname, Table::Update);
    MSIter *mi = new MSIter(*ms, sort, iodata.deltat * (double) iodata.tilesz);
    msitr.push_back(mi);
    msvector.push_back(ms);
    for (int cm = 0; cm < iodata.Nms; cm++) {
        msitr[cm]->origin();
    }

    if(sources_precessed!=0) {
        if (!Data::doBeam) {
            Data::loadData(msitr[0]->table(), old_iodata, &old_iodata.fratio);
        } else {
            Data::loadData(msitr[0]->table(), old_iodata, beam, &old_iodata.fratio);
        }
        Data::writeData(msitr[0]->table(), iodata);
        /* advance to next data chunk */
        for (int cm = 0; cm < iodata.Nms; cm++) {
            for(int it=0;it<sources_precessed;it++)
                (*msitr[cm])++;
        }
    }

    if (!Data::doBeam) {
        Data::loadData(msitr[0]->table(), iodata, &iodata.fratio);
    } else {
        Data::loadData(msitr[0]->table(), iodata, beam, &iodata.fratio);
    }
    /*----------------------------------------------------------------------------------------------------------------*/
    /* downweight factor for regularization, depending on amount of data flagged,
       0.0 means all data are flagged */
    iodata.fratio = 1.0 - iodata.fratio;

    if (Data::verbose) {
        cout << iodata.msname << ": downweight ratio (" << iodata.fratio << ") based on flags." << endl;
    }
    /*----------------------------------------share output------------------------------------------------------------*/
    dump_share_iodata(Data::shareDir,iodata.msname,&iodata);
    if (Data::doBeam) {
        dump_share_beam(Data::shareDir,iodata.msname,&iodata,&beam);
    }
    /*-------------------------------------output---------------------------------------------------------------------*/
    dump_iodata_dn(&(app->outputs[0]), &iodata);
    dump_mpidata_dn(&(app->outputs[0]), &mpiData);
    /*------------------------------------free ----------------------------------------------------------------------*/
    for (int cm = 0; cm < iodata.Nms; cm++) {
        delete msitr[cm];
        delete msvector[cm];
    }

    if (!doBeam) {
        Data::freeData(iodata);
    } else {
        Data::freeData(iodata, beam);
    }
    Data::freeData(old_iodata);
    delete[] mpiData.freqs;
    cout << "[fratio_slave]========, Done." << endl;
    return 0;
}

