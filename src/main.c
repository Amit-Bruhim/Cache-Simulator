
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char uchar;

typedef struct cache_line_s
{
    uchar valid;
    uchar frequency;
    long int tag;
    uchar *block;
} cache_line_t;

typedef struct cache_s
{
    uchar s;
    uchar t;
    uchar b;
    uchar E;
    cache_line_t **cache;
} cache_t;

// declaring functions
int getSet(uchar s, uchar b, long off);
int getOffset(uchar b, long int off);
int getTag(uchar s, uchar b, long int off);

// initialize the cache by the given parameters
cache_t initialize_cache(uchar s, uchar t, uchar b, uchar E)
{
    // allocate memory and assign the values
    cache_t *cacheMemory = malloc(sizeof(cache_t));
    cacheMemory->s = s;
    cacheMemory->t = t;
    cacheMemory->b = b;
    cacheMemory->E = E;
    // allocate memory for each set and the for each line
    cacheMemory->cache = malloc(sizeof(cache_line_t *) * (1 << s));
    for (int i = 0; i < (1 << s); i++)
    {
        cacheMemory->cache[i] = malloc(sizeof(cache_line_t) * E);
        // allocate the memory for the blocks (and initialize the valid bit)
        for (int j = 0; j < E; j++)
        {
            cacheMemory->cache[i][j].valid = 0;
            cacheMemory->cache[i][j].block = calloc((1 << b), 1);
            cacheMemory->cache[i][j].frequency = 0;
            cacheMemory->cache[i][j].tag = 0;
        }
    }
    return *cacheMemory;
}

// read one byte from the cache that match start[off] 
uchar read_byte(cache_t cache, uchar *start, long int off)
{
    int set = getSet(cache.s, cache.b, off);
    int tag = getTag(cache.s, cache.b, off);
    int offset = getOffset(cache.b, off);
    
    // search for the byte
    for (int i = 0; i < cache.E; i++)
    {
        if (cache.cache[set][i].valid)
        {
            // hit
            if (cache.cache[set][i].tag == tag)
            {
                cache.cache[set][i].frequency++;
                return cache.cache[set][i].block[offset];
            }
        }
    }

    // miss
    int isFull = 0;
    int line = 0;
    int j;

    // if there is an empty line - write the block there.
    for (j = 0; j < cache.E; j++)
    {
        if (!cache.cache[set][j].valid)
        {
            break;
        }
        else if (j == cache.E - 1)
        {
            isFull++;
        }
    }
    if (!isFull)
    {
        line = j;
    }

    else
    {
        // find the minimum frequency
        int min_lfu = cache.cache[set][0].frequency;
        for (int k = 0; k < cache.E; k++)
        {
            if (cache.cache[set][k].frequency < min_lfu)
            {
                min_lfu = cache.cache[set][k].frequency;
            }
        }
        // find the first line with the lfu
        int l;
        for (l = 0; l < cache.E; l++)
        {
            if (cache.cache[set][l].frequency == min_lfu)
            {
                break;
            }
        }
        line = l;
    }

    // copy the block to the cache
    int startingIndex = off - offset;
    for (int m = 0; m < (1 << cache.b); m++)
    {
        cache.cache[set][line].block[m] = start[startingIndex + m];
    }
    cache.cache[set][line].valid = 1;
    cache.cache[set][line].frequency = 1;
    cache.cache[set][line].tag = tag;
    return cache.cache[set][line].block[offset];
}

// write a byte to the cache and the memory
void write_byte(cache_t cache, uchar *start, long int off, uchar new)
{
    // get the set, tag and initialize the line.
    start[off] = new;
    int set = getSet(cache.s, cache.b, off);
    int tag = getTag(cache.s, cache.b, off);
    int isFull = 0;
    int line = 0;
    int j;

    // if there is an empty line - write the byte there.
    for (j = 0; j < cache.E; j++)
    {
        if (!cache.cache[set][j].valid)
        {
            break;
        }
        else if (j == cache.E - 1)
        {
            isFull++;
        }
    }
    if (!isFull)
    {
        line = j;
    }

    // if there isn't an empty line, choose the last one who was read.
    else
    {
        // find the minimum frequency
        int min_lfu = cache.cache[set][0].frequency;
        for (int k = 0; k < cache.E; k++)
        {
            if (cache.cache[set][k].frequency < min_lfu)
            {
                min_lfu = cache.cache[set][k].frequency;
            }
        }
        // find the first line with the lfu
        int l;
        for (l = 0; l < cache.E; l++)
        {
            if (cache.cache[set][l].frequency == min_lfu)
            {
                break;
            }
        }
        line = l;
    }

    // write the byte
    cache.cache[set][line].valid = 1;
    cache.cache[set][line].frequency++;
    cache.cache[set][line].tag = tag;
    cache.cache[set][line].block[getOffset(cache.b, off)] = new;
}

// return the set
int getSet(uchar s, uchar b, long int off)
{
    int mask1 = (1 << s) - 1;
    int mask2 = off >> b;
    return mask1 & mask2;
}

// return the tag
int getTag(uchar s, uchar b, long int off)
{
    return off >> (s + b);
}

// get offset
int getOffset(uchar b, long int off)
{
    int mask1 = (1 << b) - 1;
    return mask1 & off;
}

// print the cache
void print_cache(cache_t cache)
{
    int S = 1 << cache.s;
    int B = 1 << cache.b;

    for (int i = 0; i < S; i++)
    {
        printf("\033[0;32mSet %d\033[0m\n", i);
        for (int j = 0; j < cache.E; j++)
        {
            printf("%1d %d 0x%0*lx ", cache.cache[i][j].valid,
                   cache.cache[i][j].frequency, cache.t, cache.cache[i][j].tag);
            for (int k = 0; k < B; k++)
            {
                printf("%02x ", cache.cache[i][j].block[k]);
            }
            puts("");
        }
    }
}

// main function
int main()
{
    int n;
    printf("\033[0;32mSize of data: \033[0m");
    scanf("%d", &n);
    uchar *mem = malloc(n);
    printf("\033[0;32mInput data >> \033[0m");
    for (int i = 0; i < n; i++)
        scanf("%hhd", mem + i);

    int s, t, b, E;
    printf("\033[0;32ms t b E: \033[0m");
    scanf("%d %d %d %d", &s, &t, &b, &E);
    cache_t cache = initialize_cache(s, t, b, E);

    while (1)
    {
        scanf("%d", &n);
        if (n < 0)
            break;
        read_byte(cache, mem, n);
    }

    puts("");
    print_cache(cache);

    free(mem);
}
