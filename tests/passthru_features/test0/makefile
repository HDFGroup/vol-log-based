EXEC=create_open
SOURCE=create_open.cpp
CC=mpicc

LOG_VOL=/homes/zhd1108/LOG-VOL/git_install
HDF5=/homes/zhd1108/HDF5/1.13.0
# RECORDER=/homes/zhd1108/RECORDER/2.3-h5_dev1.13

all: create_open.o
	cp regular_bkp.h5 regular.h5 
	$(CC) create_open.o -o $(EXEC) -L$(LOG_VOL)/lib -lH5VL_log -L$(HDF5)/lib -lhdf5

create_open.o: $(SOURCE)
	$(CC) -c $(SOURCE) -o create_open.o -I$(LOG_VOL)/include -I$(HDF5)/include
	

run:
	LD_LIBRARY_PATH="$(LOG_VOL)/lib:$(HDF5)/lib" \
		mpirun -n 1 ./$(EXEC)

clean:
	rm -f $(EXEC) *.o test.h5 log-based.h5 core.*
	rm -rf recorder-*