//
// Created by wsl on 4/19/17.
//

#ifndef SAGECAL_DALIUGE_UTILS_DN_H
#define SAGECAL_DALIUGE_UTILS_DN_H

#include "data.h"
#include "dlg_app.h"
#include <sagecal.h>

void dump_iodata_dn(dlg_output_info *output, Data::IOData *iodata);

void load_iodata_dn(dlg_input_info *input, Data::IOData *iodata);

void load_iodata_dn_noprefix(dlg_input_info *input, Data::IOData *iodata);

void dump_beam_dn(dlg_output_info *output, Data::IOData *iodata, Data::LBeam *lBeam);

void load_beam_dn(dlg_input_info *input, Data::LBeam *lBeam);

void dump_barr_dn(dlg_output_info *output, Data::IOData *iodata, baseline_t *barr);

void load_barr_dn(dlg_input_info *input,Data::IOData *iodata, baseline_t *barr);

void dump_coh_dn(dlg_output_info *output, Data::IOData *iodata, complex double *coh);

void load_coh_dn(dlg_input_info *input, Data::IOData *iodata, complex double *coh);

void dump_mpidata_dn(dlg_output_info *output, Data::MPIData *iodata);

void load_mpidata_dn(dlg_input_info *input, Data::MPIData *iodata);

void load_mpidata_dn_noprefix(dlg_input_info *input, Data::MPIData *iodata);


#endif //SAGECAL_DALIUGE_UTILS_H
