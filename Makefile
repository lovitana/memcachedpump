all:
	gcc -std=c11 shm_writer.c -o shm_writer
	gcc -std=c11  shm_reader_uds.c -o shm_reader_uds  -lpthread
