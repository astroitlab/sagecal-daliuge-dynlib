#ifndef __CMD_SLAVE_H__
#define __CMD_SLAVE_H__

#include "dlg_app.h"

int aux_reader(dlg_app_info *app);

int admm_slave(dlg_app_info *app);

int fratio_slave(dlg_app_info *app);

int coh_slave(dlg_app_info *app);

int sagefit_slave(dlg_app_info *app);

int update_y_slave(dlg_app_info *app);

int write_residual_slave(dlg_app_info *app);
#endif /* __CMD_SLAVE_H__ */
