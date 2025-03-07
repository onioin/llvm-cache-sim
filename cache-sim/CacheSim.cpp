extern "C" {

#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <stdio.h>
#include <assert.h>

//TODO: lift policy enum out of this file
#define LEAST_FREQUENT      0
#define LEAST_RECENT        1
#define FIFO                2
#define RANDOM              3

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

ULL addr2Block(const ULL address, const cache_t *cache){
    return ((address >> (cache->blockBits)) << (cache->blockBits));
}


ULL cacheTag(const ULL address, const cache_t *cache){
    return ((address >> (cache->blockBits + cache->setBits)) << (cache->blockBits + cache->setBits));
}

ULL cacheSet(const ULL address, const cache_t *cache){
    return ((address << (64 - (cache->blockBits + cache->setBits))) >> (64 - cache->setBits));
}

void accessCache(const ULL address, const cache_t *cache){
    ULL set = cacheSet(address, cache);
    ULL tag = cacheTag(address, cache);
    set_t theSet = cache->sets[set];
    int recent;
    switch(cache->policy){
        case LEAST_RECENT:
            for(recent = 0; recent < cache->linesPerSet; recent++){
                if(theSet.lines[recent].valid && theSet.lines[recent].tag == tag){
                    break;
                }
            }
            recent = theSet.lines[recent].meta;
            for(int i = 0; i < cache->linesPerSet; i++){
                if(!theSet.lines[i].valid){
                    continue;
                }
                if(theSet.lines[i].tag == tag){
                    theSet.lines[i].meta = 0;
                }else if(theSet.lines[i].meta <= recent){
                    theSet.lines[i].meta++;
                }
            }
            break;
        case LEAST_FREQUENT:
            for(int i = 0; i < cache->linesPerSet; i++){
                if(!theSet.lines[i].valid){
                    continue;
                }
                if(theSet.lines[i].tag == tag){
                    theSet.lines[i].meta++;
                }
            }
            break;
        default:
            return;
    }
}

bool probeCache(const ULL address, const cache_t *cache){
    ULL tag = cacheTag(address, cache);
    ULL set = cacheSet(address, cache);
    set_t theSet = cache->sets[set];
    for(int i = 0; i < cache->linesPerSet; i++){
        if(theSet.lines[i].valid && theSet.lines[i].tag == tag){
            return true;
        }
    }
    return false;
}

void allocateCache(const ULL address, const cache_t *cache){
    ULL set = cacheSet(address, cache);
    set_t theSet = cache->sets[set];
    int inserted = 0;
    for(int i = 0; i < cache->linesPerSet; i++){
        if(theSet.lines[i].valid){
            if(FIFO == cache->policy){
                theSet.lines[i].meta++;
            }
            continue;
        }
        if(!inserted){
            theSet.lines[i].valid = 1;
            theSet.lines[i].blockAddr = addr2Block(address, cache);
            theSet.lines[i].tag = cacheTag(address, cache);
            inserted = 1;
        }
        if(FIFO == cache->policy){
            theSet.lines[i].meta = 0;
        }
    }
}

bool availCache(const ULL address, const cache_t *cache){
    ULL set = cacheSet(address, cache);
    set_t theSet = cache->sets[set];
    for(int i = 0; i < cache->linesPerSet; i++){
        if(!theSet.lines[i].valid){
            return true;
        }
    }
    return false;
}

void evictCache(int set, int way, cache_t *cache){
    memset(&(cache->sets[set].lines[way]), 0, sizeof(line_t));
}

ULL victimCache(const ULL address, cache_t *cache){
    ULL set = cacheSet(address, cache);
    set_t theSet = cache->sets[set];
    ULL toEvict = theSet.lines[0].blockAddr;
    unsigned int way = 0;
    int i;
    switch(cache->policy){
        case LEAST_RECENT:
        case FIFO:
            for(i = 1; i < cache->linesPerSet; i++){
                if(theSet.lines[way].meta < theSet.lines[i].meta){
                    way = i;
                    toEvict = theSet.lines[i].blockAddr;
                }
            }
            evictCache(set, way, cache);
            break;
        case LEAST_FREQUENT:
            for(i = 1; i < cache->linesPerSet; i++){
                if(theSet.lines[way].meta >= theSet.lines[i].meta){
                    way = i;
                    toEvict = theSet.lines[i].blockAddr;
                }
            }
            evictCache(set, way, cache);
            break;
        case RANDOM:
            if(1 <= getrandom(&way, sizeof(int), 0)){
                way %= cache->linesPerSet;
                assert(way >= 0 && way < (unsigned int)cache->linesPerSet);
                toEvict = theSet.lines[way].blockAddr;
            }else{
                way = 0;
            }
            evictCache(set, way, cache);
            break;
    }
    return toEvict;
}

void flushCache(const ULL address, cache_t *cache){
    ULL set = cacheSet(address, cache);
    ULL tag = cacheTag(address, cache);
    set_t theSet = cache->sets[set];
    for(int i = 0; i < cache->linesPerSet; i++){
        if(theSet.lines[i].valid && theSet.lines[i].tag == tag){
            theSet.lines[i].valid = 0;
            return;
        }
    }
}

void operateCache(const ULL address, cache_t *cache){
    if(probeCache(address, cache)){
        accessCache(address, cache);
        cache->hitCount++;
        //printf("hit\n");
        return;
    }
    else if(availCache(address, cache)){
        allocateCache(address, cache);
        accessCache(address, cache);
        cache->missCount++;
        //printf("miss 0x%llx\n", addr2Block(address, cache));
        return;
    }
    else{
        /*ULL victim = */victimCache(address, cache);
        allocateCache(address, cache);
        accessCache(address, cache);
        cache->missCount++;
        cache->evictionCount++;
        //printf("evict 0x%llx, insert 0x%llx\n",
        //       addr2Block(victim, cache), addr2Block(address, cache));
        return;
    }
}


void printSummary(const cache_t *cache){
    printf("\n%s hits: %d misses: %d evicts: %d\n", cache->name, cache->hitCount,
           cache->missCount, cache->evictionCount);
}

void cacheDeallocate(cache_t *cache){
    printSummary(cache);
    int sets = (1 << cache->setBits);
    for(int i = 0; i < sets; i++){
        free(cache->sets[i].lines);
    }
    free(cache->sets);
    //maybe a memory leak?
    //memory leak *sometimes*
    //caller must ensure *cache is free'd
    //uncommenting throws an invalid free error though
    //free(cache);
}

void cacheSetup(cache_t* cache, int setBits, int linesPer,
                int blockBits, int policy, char* name){
    if(!cache){
        cache = (cache_t*) calloc(1, sizeof(cache_t));
    }
    if(0 > policy || policy > 3){
        return;
    }
    cache->setBits = setBits;
    cache->linesPerSet = linesPer;
    cache->blockBits = blockBits;
    cache->policy = policy;
    cache->name = name;
    cache->hitCount = 0;
    cache->missCount = 0;
    cache->evictionCount = 0;
    cache->sets = (set_t*) calloc((1 << setBits), sizeof(set_t));
    for(int i = 0; i < (1 << setBits); i++) {
        cache->sets[i].lines = (line_t *) calloc(linesPer, sizeof(line_t));
    }
}

}
