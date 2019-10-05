#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdatomic.h>

struct string_pair_t {
    char *country;
    char *capital;
};

const struct string_pair_t european_capital_cities[] = {
    {"Albania", "Tirana"},
    {"Armenia", "Yerevan"},
    {"Austria", "Vienna"},
    {"Azerbaijan", "Baku"},
    {"Belarus", "Minsk"},
    {"Belgium", "Brussels"},
    {"Bulgaria", "Sofia"},
    {"Croatia", "Zagreb"},
    {"Cyprus", "Nicosia"},
    {"Czechia", "Prague"},
    {"Denmark", "Copenhagen"},
    {"Estonia", "Tallinn"},
    {"Finland", "Helsinki"},
    {"France", "Paris"},
    {"Georgia", "Tbilisi"},
    {"Germany", "Berlin"},
    {"Greece", "Athens"},
    {"Hungary", "Budapest"},
    {"Iceland", "Reykjavik"},
    {"Ireland", "Dublin"},
    {"Italy", "Rome"},
    {"Latvia", "Riga"},
    {"Liechtenstein", "Vaduz"},
    {"Lithuania", "Vilnius"},
    {"Luxembourg", "Luxembourg"},
    {"Malta", "Valletta"},
    {"Moldova", "Chisinau"},
    {"Monaco", "Monaco"},
    {"Montenegro", "Podgorica"},
    {"Netherlands", "Amsterdam"},
    {"Norway", "Oslo"},
    {"Poland", "Warsaw"},
    {"Portugal", "Lisbon"},
    {"Romania", "Bucharest"},
    {"Russia", "Moscow"},
    {"Serbia", "Belgrade"},
    {"Slovakia", "Bratislava"},
    {"Slovenia", "Ljubljana"},
    {"Spain", "Madrid"},
    {"Sweden", "Stockholm"},
    {"Switzerland", "Bern"},
    {"Ukraine", "Kyiv"}
};

// const struct string_pair_t european_capital_cities[] = {
//     {"Albania", "Tirana"},
//     {"Armenia", "Yerevan"},
//     {"Austria", "Vienna"},
//     {"Azerbaijan", "Baku"},
//     {"Belarus", "Minsk"},
//     {"Belgium", "Brussels"},
//     {"Bulgaria", "Sofia"},
//     {"Croatia", "Zagreb"},
//     {"Cyprus", "Nicosia"},
//     {"Czechia", "Prague"},
//     {"Denmark", "Copenhagen"},
//     {"Estonia", "Tallinn"},
//     {"Finland", "Helsinki"},
//     {"France", "Paris"},
//     {"Georgia", "Tbilisi"},
//     {"Germany", "Berlin"},
//     {"Greece", "Athens"},
//     {"Hungary", "Budapest"},
//     {"Iceland", "Reykjavik"},
//     {"Ireland", "Dublin"},
//     {"Italy", "Rome"},
//     {"Latvia", "Riga"},
//     {"Liechtenstein", "Vaduz"},
//     {"Lithuania", "Vilnius"},
//     {"Luxembourg", "Luxembourg"},
//     {"Malta", "Valletta"},
//     {"Moldova", "Chisinau"},
//     {"Monaco", "Monaco"},
//     {"Montenegro", "Podgorica"},
//     {"Netherlands", "Amsterdam"},
//     {"Norway", "Oslo"},
//     {"Poland", "Warsaw"},
//     {"Portugal", "Lisbon"},
//     {"Romania", "Bucharest"},
//     {"Russia", "Moscow"},
//     {"Serbia", "Belgrade"},
//     {"Slovakia", "Bratislava"},
//     {"Slovenia", "Ljubljana"},
//     {"Spain", "Madrid"},
//     {"Sweden", "Stockholm"},
//     {"Switzerland", "Bern"},
//     {"Ukraine", "Kyiv"}
// };


#define MEMORY_SZ (100 * 1024 * 1024)
#define CACHE_SZ (63)

size_t store_buffer(char *memory, char *buf, size_t len) {
    size_t written = 0;

    _Atomic(size_t) *header_len;
    atomic_init(&header_len, (_Atomic size_t *) memory);
    atomic_store_explicit(header_len, len, memory_order_release);
    written += sizeof(size_t);

    memcpy(memory + written, buf, len);
    written += len;

    memory += written;

    _Atomic(unsigned char) *canary;
    // canary = memory;
    atomic_init(&canary, (_Atomic char *) memory);
    atomic_store_explicit(canary, 0xff, memory_order_release);
    written += sizeof(char);

    // Find the next multiple of CACHE_SZ+1
    return (written + CACHE_SZ) & (~CACHE_SZ);
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

    memset(str, 0, MEMORY_SZ);

    sleep(10);

    char *offset = str;
    for (int i = 0; i < sizeof(european_capital_cities) / sizeof(struct string_pair_t); i++) {
        char tmp[50];
        int len = sprintf(tmp, "*3\r\n$3\r\nSET\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n",
            strlen(european_capital_cities[i].country),
            european_capital_cities[i].country,
            strlen(european_capital_cities[i].capital),
            european_capital_cities[i].capital);

        printf("%d ~ Writing %s - %s\n", sizeof(european_capital_cities), european_capital_cities[i].country,
            european_capital_cities[i].capital);

        offset += store_buffer(offset, tmp, len);

        // sleep(2);
    }

    // char *offset = str;
    // for (int i = 0; i < 4; i++) {

    //     char tmp[50];
    //     int rand_len = 16 + rand() % 20;
    //     for (int j = 0; j < rand_len; j++) {
    //         tmp[j] = 'a' + i;
    //     }

    //     // printf("Offset %p\n", offset);

    //     offset += store_buffer(offset, tmp, rand_len);

    //     // sleep(2);
    // }

    printf("Finished\n");

    // printf("Data written in memory: %s\n",str);

    // for (int i = 0; i < 100; i++) {
    //     printf("%d, ", str[i]);
    // }
    // printf("\n");

    //detach from shared memory
    shmdt(str);

    return 0;
}
