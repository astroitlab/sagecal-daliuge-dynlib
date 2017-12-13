//
// Created by wsl on 2017-7-30.
//
#include "data.h"
#include "cmd_slave.h"
#include <fstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "sagecal.h"

using namespace std;
using namespace Data;

typedef struct _app_data {
    char func[40];
    char cmdfile[200];
} app_data;

static inline app_data *to_app_data(dlg_app_info *app) {
    return (app_data *)app->data;
}

static inline unsigned long usecs(struct timeval *start, struct timeval *end) {
    return (end->tv_sec - start->tv_sec) * 1000000 + (end->tv_usec - start->tv_usec);
}

static inline void ParseCmdLine(char *cmd_file) {
    ifstream infile(cmd_file);
    if (!infile.good()) {
        cout << "File " << cmd_file << " does not exist." << endl;
        exit(1);
    }
    int argc=0;
    char **args = new char *[108];
    string buffer;
    if (infile.is_open()) {
        while (infile.good()) {
            std::getline(infile, buffer);
            if (buffer.length() > 0) {
                char *cmdline = new char[buffer.size() + 1];
                strcpy(cmdline, buffer.c_str());
                cmdline[buffer.size()] = '\0';
                char * pch = strtok (cmdline, " ");
                args[argc++] = pch;
                while ( pch!= NULL) {
                    pch = strtok (NULL, " ");
                    args[argc++] = pch;
                }
            }
        }
        for(int i=0;i<argc-2;i=i+2) {
            if( args[i][0]!= '-') {
                i = i - 1;
                continue;
            }
            char c = args[i][1];
            switch (c) {
                case 's':
                    Data::SkyModel = args[i+1];
                    break;
                case 'c':
                    Data::Clusters = args[i+1];
                    break;
                case 'p':
                    Data::solfile = args[i+1];
                    break;
                case 'g':
                    Data::max_iter = atoi(args[i+1]);
                    break;
                case 'a':
                    Data::DoSim = atoi(args[i+1]);
                    if (Data::DoSim < 0) { Data::DoSim = 1; }
                    break;
                case 'b':
                    Data::doChan = atoi(args[i+1]);
                    if (Data::doChan > 1) { Data::doChan = 1; }
                    break;
                case 'B':
                    Data::doBeam = atoi(args[i+1]);
                    if (Data::doBeam > 1) { Data::doBeam = 1; }
                    break;
                case 'S':
                    Data::shareDir = args[i+1];
                    break;
                case 'F':
                    Data::format = atoi(args[i+1]);
                    if (Data::format > 1) { Data::format = 1; }
                    break;
                case 'e':
                    Data::max_emiter = atoi(args[i+1]);
                    break;
                case 'l':
                    Data::max_lbfgs = atoi(args[i+1]);
                    break;
                case 'm':
                    Data::lbfgs_m = atoi(args[i+1]);
                    break;
                case 'j':
                    Data::solver_mode = atoi(args[i+1]);
                    break;
                case 't':
                    Data::TileSize = atoi(args[i+1]);
                    break;
                case 'I':
                    Data::DataField = args[i+1];
                    break;
                case 'O':
                    Data::OutField = args[i+1];
                    break;
                case 'n':
                    Data::Nt = atoi(args[i+1]);
                    break;
                case 'k':
                    Data::ccid = atoi(args[i+1]);
                    break;
                case 'o':
                    Data::rho = atof(args[i+1]);
                    break;
                case 'L':
                    Data::nulow = atof(args[i+1]);
                    break;
                case 'H':
                    Data::nuhigh = atof(args[i+1]);
                    break;
                case 'R':
                    Data::randomize = atoi(args[i+1]);
                    break;
                case 'W':
                    Data::whiten = atoi(args[i+1]);
                    break;
                case 'J':
                    Data::phaseOnly = atoi(args[i+1]);
                    break;
                case 'x':
                    Data::min_uvcut = atof(args[i+1]);
                    break;
                case 'y':
                    Data::max_uvcut = atof(args[i+1]);
                    break;
                case 'z':
                    Data::ignorefile = args[i+1];
                    break;
                case 'v':
                    Data::verbose = 1;
                    break;
                case 'D':
                    Data::DoDiag = atoi(args[i+1]);
                    if (Data::DoDiag < 0) { Data::DoDiag = 0; }
                    break;
            }
        }
    }
}
int init(dlg_app_info *app, const char ***params)
{
    const char **param;
    while (1) {
        param = *params;
        // Sentinel
        if (param == NULL) {
            break;
        }
        if (strcmp(param[0], "func") == 0) {
            Data::func = (char *)param[1];
        } else if(strcmp(param[0], "cmdfile") == 0) {
            Data::cmdFile = (char *)param[1];
        }
        params++;
    }
    if(Data::func==NULL||Data::cmdFile==NULL) {
        printf("func or cmdfile not configured\n");
        return 1;
    }

    app->data = malloc(sizeof(app_data));
    strcpy(to_app_data(app)->func,Data::func);
    strcpy(to_app_data(app)->cmdfile,Data::cmdFile);
    return 0;
}

int run(dlg_app_info *app) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    printf("------------------slave begin run %s, [%s]------------------\n", app->uid, to_app_data(app)->func);
    printf("------------------cmdfile: %s------------------\n",to_app_data(app)->cmdfile);
    ParseCmdLine(to_app_data(app)->cmdfile);

    double duration = 0.;
    if(strcmp(to_app_data(app)->func, "aux_reader") == 0) {
        aux_reader(app);
    } else if(strcmp(to_app_data(app)->func, "fratio_slave") == 0) {
        fratio_slave(app);
    } else if(strcmp(to_app_data(app)->func, "admm_slave") == 0) {
        admm_slave(app);
    } else if(strcmp(to_app_data(app)->func, "coh_slave") == 0) {
        coh_slave(app);
    } else if(strcmp(to_app_data(app)->func, "sagefit_slave") == 0) {
        sagefit_slave(app);
    } else if(strcmp(to_app_data(app)->func, "update_y_slave") == 0) {
        update_y_slave(app);
    } else if(strcmp(to_app_data(app)->func, "write_residual_slave") == 0) {
        write_residual_slave(app);
    } else {
        printf("------------------slave wrong func parameter:%s------------------\n", to_app_data(app)->func);
        free(app->data);
        return 1;
    }

    gettimeofday(&end, NULL);
    duration = usecs(&start, &end) / 1000000.;
    printf("------------------slave end run %s, [%s] used %.3f's------------------\n", app->uid, to_app_data(app)->func, duration);
    return 0;
}

void drop_completed(dlg_app_info *app, const char *uid, drop_status status) {
    app->done(APP_FINISHED);
    free(app->data);
}