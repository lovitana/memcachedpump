#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>

#include <sys/types.h>  /* Type definitions used by many programs */
#include <stdio.h>      /* Standard I/O functions */
#include <stdlib.h>     /* Prototypes of commonly used library functions,
                           plus EXIT_SUCCESS and EXIT_FAILURE constants */
#include <unistd.h>     /* Prototypes for many system calls */
#include <errno.h>      /* Declares errno and defines error constants */
#include <string.h>     /* Commonly used string-handling functions */
#include <stdbool.h>    /* 'bool' type plus 'true' and 'false' constants */

#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdint.h>

#include <pthread.h>

#define SV_SOCK_PATH "redis/trunk/pkg/redis/usr/bin/redis.sock"
// #define SV_SOCK_PATH "uds_socket"
#define REDIS_REPLY_OFF "*3\r\n$6\r\nCLIENT\r\n$5\r\nREPLY\r\n$3\r\nOFF\r\n"

#define MEMORY_SZ (100 * 1024 * 1024)
#define CACHE_SZ (63)

#define READ_DUMP_SZ 1048576
#define WRITE_BUF_SZ 1

char read_dump[READ_DUMP_SZ] __attribute__ ((aligned (16)));
// char dump[WRITE_BUF_SZ] __attribute__ ((aligned (16)));

size_t load_buffer(char *memory, char **buf, size_t *len) {
    size_t header_len;
    size_t read_bytes = 0;

    _Atomic(size_t) *offset;
    atomic_init(&offset, (_Atomic size_t *) memory);
    while ((header_len = atomic_load_explicit(offset, memory_order_acquire)) == 0);
    *len = header_len;

    read_bytes += sizeof(size_t);
    *buf = memory + read_bytes;

    _Atomic(unsigned char) * canary;
    atomic_init(&canary, (_Atomic char *) (memory + read_bytes + header_len) );
    while (atomic_load_explicit(canary, memory_order_acquire) != 0xff);
    read_bytes += header_len;

    return (read_bytes + CACHE_SZ) & (~CACHE_SZ);
}

void blocking_write(int fd, const char *buf, ssize_t len) {
    ssize_t bytes_written = write(fd, buf, len);
    if (bytes_written == -1) {
        perror("write");
    }

    while (bytes_written < len) {
        ssize_t ret = write(fd, buf + bytes_written, len - bytes_written);
        if (ret == -1) {
            perror("write");
        }

        bytes_written += ret;
    }
}

static void printchar(unsigned char theChar) {

    switch (theChar) {

        case '\n':
            fprintf(stderr, "\\n");
            break;
        case '\r':
            fprintf(stderr, "\\r");
            break;
        case '\t':
            fprintf(stderr, "\\t");
            break;
        default:
            if ((theChar < 0x20) || (theChar > 0x7f)) {
                fprintf(stderr, "\\%03o", (unsigned char)theChar);
            } else {
                fprintf(stderr, "%c", theChar);
            }
        break;
   }
}

void * threadFunc(void *arg)
{
    char tmp[1024];
    printf("Reading\n");

    int fd = (int)arg;

    while (true) {
        int ret = read(fd, tmp, 1024);
        if (ret == -1) {
            perror("read");
        }
    }

	// Return value from thread
	return NULL;
}

int launch_drain_thread(int sfd) {
	// Thread id
	pthread_t threadId;

	// Create a thread that will funtion threadFunc()
	int err = pthread_create(&threadId, NULL, &threadFunc, (void *)sfd);
	// Check if thread is created sucessfuly
	if (err)
	{
		printf("Thread creation failed : %s\n", strerror(err));
		return err;
	}
	// else
	// 	std::cout << "Thread Created with ID : " << threadId << std::endl;
	// Do some stuff

	err = pthread_detach(threadId);
	if (err)
		printf("Failed to detach Thread : %s\n", strerror(err));

	return 0;
}

int main()
{
	// ftok to generate unique key
	key_t key = ftok("shmfiles", 65);

	// shmget returns an identifier in shmid
	int shmid = shmget(key, MEMORY_SZ, 0666|IPC_CREAT);

    if (shmid == -1) {
        perror("shmid");
    }

	// shmat to attach to shared memory
	char *str = (char*) shmat(shmid, (void*)0, 0);


	// Configure the unix socket
   	struct sockaddr_un addr;
	// Create client socket
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1)
        perror("socket");

	// // Change the socket into non-blocking state
    // fcntl(sfd, F_SETFL, O_NONBLOCK);

    // Construct server address
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

	// Make the connection
    if (connect(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
        perror("connect");

    // blocking_write(sfd, REDIS_REPLY_OFF, sizeof(REDIS_REPLY_OFF));

    launch_drain_thread(sfd);



    char *next_buffer = str;
    // for (int i = 0; i < 4; i++) {
    while (true) {
        char *buf = NULL;
        size_t len = 0;
        size_t offset = load_buffer(next_buffer, &buf, &len);

        // printf("Found entry at location %p, %p, with content: %d\n", next_buffer, buf, len);
        // for (int j = 0; j < len+1; j++) {
        //     printchar(*(unsigned char *)(buf + j));
        // }
        // printf("\n");

        printf("Writing\n");
        blocking_write(sfd, buf, len);

        // char tmp[1024];
        // printf("Reading\n");
        // int ret = read(sfd, tmp, 1024);
        // if (ret == -1) {
        //     perror("read");
        // }

        // tmp[ret] = 0;
        // printf("Redis returned %s\n", tmp);

        next_buffer += offset;
    }

    printf("Finished\n");

	//detach from shared memory
	shmdt(str);

	// destroy the shared memory
	shmctl(shmid, IPC_RMID, NULL);

	return 0;
}
