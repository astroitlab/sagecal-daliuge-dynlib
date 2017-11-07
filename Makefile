OUTPUT=
CXX=g++
CXXFLAGS=-O3 -Wall -g -fPIC
CASA_LIBDIR=/usr/local/lib
CASA_INCDIR=-I/usr/local/include/casacore
CASA_LIBS=-lcasa_casa -lcasa_tables -lcasa_measures -lcasa_ms
LAPACK=-lopenblas -lgfortran -lpthread
LAPACK_DIR=/opt/OpenBLAS/lib

LDFLAGS=-Wl,--rpath,/opt/OpenBLAS/lib,--rpath,${CASA_LIBDIR}

MY_LIBS=-lm -lsagecal
INCLUDES=-I. -I./lib $(CASA_INCDIR) -I/usr/include
LIBPATH=-L$(LAPACK_DIR) -L$(CASA_LIBDIR)  -L./lib

#### glib
GLIBI=-I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include
GLIBL=-lglib-2.0

default: all

data.o: data.cpp data.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

utils.o: utils.cpp utils.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

utils_dn.o: utils_dn.cpp utils_dn.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

aux_reader.o: aux_reader.cpp cmd_slave.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

admm_master.o: admm_master.cpp cmd_master.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

admm_slave.o: admm_slave.cpp cmd_slave.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

fratio_master.o: fratio_master.cpp cmd_master.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

fratio_slave.o: fratio_slave.cpp cmd_slave.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

coh_slave.o: coh_slave.cpp cmd_slave.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

sagefit_slave.o: sagefit_slave.cpp cmd_slave.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

update_z_master.o: update_z_master.cpp cmd_master.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

update_y_slave.o: update_y_slave.cpp cmd_slave.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

write_z_master.o: write_z_master.cpp cmd_master.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

write_residual_slave.o: write_residual_slave.cpp cmd_slave.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

dlg_app_slave.o: dlg_app_slave.cpp dlg_app.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

dlg_app_master.o: dlg_app_master.cpp dlg_app.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GLIBI) -c $<

OBJS_SLAVE = dlg_app_slave.o aux_reader.o admm_slave.o fratio_slave.o coh_slave.o sagefit_slave.o update_y_slave.o write_residual_slave.o data.o utils.o utils_dn.o
lib_slave: ${OBJS_SLAVE} ./lib/libsagecal.a
	$(CXX) -shared -o libsagecal_slave.so ${OBJS_SLAVE} $(MY_LIBS) $(LAPACK) $(CASA_LIBS) $(GLIBL) $(LIBPATH)
	cp libsagecal_slave.so /astrodata/sagecal/build/bin

OBJS_MASTER = dlg_app_master.o admm_master.o fratio_master.o update_z_master.o write_z_master.o data.o utils.o utils_dn.o
lib_master: ${OBJS_MASTER}
	$(CXX) -shared -o libsagecal_master.so ${OBJS_MASTER} $(MY_LIBS) $(LAPACK) $(CASA_LIBS) $(GLIBL) $(LIBPATH)
	cp libsagecal_master.so /astrodata/sagecal/build/bin

all: lib_slave lib_master
clean:
	rm *.o *.a *.so
