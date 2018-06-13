//
// Created by wsl on 2017-7-30.
//
#include <sys/time.h>

#include "data.h"
#include "cmd_master.h"

using namespace std;
using namespace Data;

typedef struct _app_data {
    char func[40];
    char cmdfile[200];
} app_data;

static inline app_data *to_app_data(dlg_app_info *app)
{
    return (app_data *)app->data;
}
static inline unsigned long usecs(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) * 1000000 + (end->tv_usec - start->tv_usec);
}

void ParseCmdLine() {
    ifstream infile(Data::cmdFile);
    /* check if the file exists and readable */
    if (!infile.good()) {
        cout << "File " << Data::cmdFile << " does not exist." << endl;
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
                case 't':
                    Data::TileSize = atoi(args[i+1]);
                    break;
                case 'p':
                    Data::solfile = args[i+1];
                    break;
                case 'r':
                    admm_rho= atof(args[i+1]);
                    break;
                case 'G':
                    admm_rho_file= args[i+1];
                case 'N':
                    Data::Npoly = atoi(args[i+1]);
                    break;
                case 'S':
                    Data::shareDir = args[i+1];
                    break;
                case 'Q':
                    PolyType= atoi(args[i+1]);
                    break;
                case 'v':
                    Data::verbose = 1;
                    break;
            }
        }
    }

}

int init(dlg_app_info *app, const char ***params) {
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
        printf("Arg01 or Arg02 not configured\n");
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
    printf("====================master begin run %s, [%s]====================\n", app->uid, to_app_data(app)->func);
    Data::func = to_app_data(app)->func;
    Data::cmdFile = to_app_data(app)->cmdfile;
    ParseCmdLine();
    double duration = 0.;

    if(strcmp(to_app_data(app)->func, "admm_master") == 0) {
        admm_master(app);
    } else if(strcmp(to_app_data(app)->func, "fratio_master") == 0) {
        fratio_master(app);
    } else if(strcmp(to_app_data(app)->func, "update_z_master") == 0) {
        update_z_master(app);
    } else if(strcmp(to_app_data(app)->func, "write_z_master") == 0) {
        write_z_master(app);
    } else {
        printf("------------------master wrong func parameter:%s------------------\n", Data::func);
        return 1;
    }

    gettimeofday(&end, NULL);
    duration = usecs(&start, &end) / 1000000.;
    printf("====================master end run %s, [%s] used %.3f's====================\n", app->uid, Data::func, duration);
    return 0;
}

void drop_completed(dlg_app_info *app, const char *uid, drop_status status) {
    app->done(APP_FINISHED);
    free(app->data);
}