
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

extern "C" {

enum EvictPol {
    lfu = 0, lru = 1, fifo = 2, rnd = 3
};

static struct option options[] =
        {
                {"policy", required_argument, NULL, 'p'},
                {"name",   required_argument, NULL, 'n'},
                {0,        0,                 0,    0}
        };

typedef unsigned long long ULL;

typedef struct line {
    ULL blockAddr;
    ULL tag;
    int meta;
    short valid;
} line_t;

typedef struct set {
    line_t *lines;
    int recentRate;
    int placementRate;
} set_t;

typedef struct cache {
    int setBits;
    int linesPerSet;
    int blockBits;
    int evictionCount;
    int hitCount;
    int missCount;
    int policy;
    char *name;
    set_t *sets;
} cache_t;

void cacheSetup(cache_t *, int, int, int, int, char *);
void cacheDeallocate(cache_t *);
void operateCache(const ULL, cache_t *);

int parsePolStr(char *);
void runTrace(FILE *, cache_t *);

int main(int argc, char **argv) {
    int opt;
    char *trace = NULL;
    char *name = NULL;
    char *polStr = NULL;
    FILE *tfile;
    int pol, setBits = -1, linesPer = -1, blockBits = -1;
    cache_t *cache = (cache_t *) malloc(sizeof(cache_t));

    while ((opt = getopt_long(argc, argv, "s:E:b:t:h", options, NULL)) != -1) {
        switch (opt) {
            case 's':
                setBits = atoi(optarg);
                break;
            case 'E':
                linesPer = atoi(optarg);
                break;
            case 'b':
                blockBits = atoi(optarg);
                break;
            case 't':
                trace = optarg;
                break;
            case 'p':
                polStr = optarg;
                break;
            case 'n':
                name = optarg;
                break;
            case 'h':
            default:
                goto optsErr;
        }
    }
    if (setBits < 0 || linesPer < 0 || blockBits < 0) {
        goto optsErr;
    }

    if (trace) {
        tfile = fopen(trace, "r");
        if (!tfile) {
            goto fileErr;
        }
    } else goto optsErr;

    if (polStr) {
        if ((pol = parsePolStr(polStr)) < 0) {
            goto optsErr;
        }
    } else goto optsErr;

    if (!name) {
        name = polStr;
    }

    cacheSetup(cache, setBits, linesPer, blockBits, pol, name);
    runTrace(tfile, cache);
    cacheDeallocate(cache);
    free(cache);

    exit(EXIT_SUCCESS);

    optsErr:
    fprintf(stderr, "Usage:\n"
                    "%s [options]\n"
                    "Options:\n"
                    "\t-h Print this help message.\n"
                    "\t-s<num> REQUIRED - Number of set index bits.\n"
                    "\t-E<num> REQUIRED - Number of lines per set.\n"
                    "\t-b<num> REQUIRED - Number of block offset bits.\n"
                    "\t--name=<string> OPTIONAL - Name of the cache.\n"
                    "\t--policy=<value> REQUIRED - cache replacement policy.\n"
                    "\t\t=lru - least recently used.\n"
                    "\t\t=lfu - least frequently used.\n"
                    "\t\t=fifo - first in first out.\n"
                    "\t\t=rand - random.\n"
                    "\t-t<file> REQUIRED - trace file to run.\n", argv[0]);
    free(cache);
    exit(EXIT_FAILURE);

    fileErr:
    fprintf(stderr, "Trace File [%s] does not exist\n", trace);
    free(cache);
    exit(EXIT_FAILURE);
}

int parsePolStr(char *polStr) {
    if (!strcmp(polStr, "lru")) {
        return lru;
    } else if (!strcmp(polStr, "lfu")) {
        return lfu;
    } else if (!strcmp(polStr, "rand")) {
        return rnd;
    } else if (!strcmp(polStr, "fifo")) {
        return fifo;
    } else return -1;
}

void runTrace(FILE *trace, cache_t *cache) {
    int size;
    char op;
    ULL address;
    while (fscanf(trace, " %c %llx,%d", &op, &address, &size) == 3) {
        if (op != 'M' && op != 'L' && op != 'S') {
            continue;
        }
        //printf(" %c %llx, ", op, address);
        operateCache(address, cache);
        if (op == 'M') {
            cache->hitCount++;
        }
    }
    fclose(trace);
}
}

