EXEC=read_from_regular_write_to_log
SOURCE=${EXEC}.cpp
OBJ=${EXEC}.o
CC=mpicc

LOG_VOL=/homes/zhd1108/LOG-VOL/git_install
HDF5=/homes/zhd1108/HDF5/1.13.0

all: ${OBJ}
	$(CC) ${OBJ} -o $(EXEC) -L$(LOG_VOL)/lib -lH5VL_log -L$(HDF5)/lib -lhdf5

${OBJ}: $(SOURCE)
	$(CC) -c $(SOURCE) -o ${OBJ} -I$(LOG_VOL)/include -I$(HDF5)/include
	

run:
	LD_LIBRARY_PATH="$(LOG_VOL)/lib:$(HDF5)/lib" \
		mpirun -n 1 ./$(EXEC)

clean:
	rm -f $(EXEC) *.o *.h5 core.*