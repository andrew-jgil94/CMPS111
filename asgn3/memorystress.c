/*
 * memorystress.c
 *
 * Consume lots of memory.  The skew parameter is there to allow for skewed access to memory.
 *
 * Author: Ethan L. Miller (elm@ucsc.edu)
 *         Baskin School of Engineering
 *         University of California, Santa Cruz
 */

#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_REGIONS               16
#define MEMORY_SIZE_MIN         10
#define MEMORY_SIZE_MAX         2000
#define ACCESSES_MIN            1000LL
#define ACCESSES_MAX            1000000000LL

const char * prog = NULL;

uint64_t
rand64 ()
{
    uint64_t r1 = (uint64_t)random () & 0xfffffffULL;
    uint64_t r2 = (uint64_t)random () & 0xfffffffULL;
    uint64_t r3 = (uint64_t)random () & 0xfffffffULL;
    return ((r1 << 36ULL) ^ (r2 << 18ULL) ^ (r3));
}

void
error_and_exit ()
{
    fprintf (stderr, "Usage: %s <memory size in MB (%d-%d)> <accesses (%lld-%lld)> [skew level 0-9]\n", prog,
             MEMORY_SIZE_MIN, MEMORY_SIZE_MAX, ACCESSES_MIN, ACCESSES_MAX);
    exit (1);
}

int
main (int argc, const char * argv[])
{
    long        skew_level = 0;
    unsigned    memory_size_in_mb;
    int64_t     size_in_words;
    int64_t     n_accesses = 0LL;
    int64_t     i;
    uint64_t *  m;
    uint64_t    r, loc, total_weight, w, region;
    uint64_t    region_weight[N_REGIONS];
    double      region_w[N_REGIONS];
    uint64_t    reads_per_region[N_REGIONS];
    uint64_t    writes_per_region[N_REGIONS];
    uint64_t    size_per_region;
    double      weight;
    uint64_t    all = 0ULL;
    unsigned    seed = time (NULL);

    prog = argv[0];

    if (argc < 3 || argc > 4) {
        error_and_exit ();
    }

    memory_size_in_mb = strtol (argv[1], NULL, 0);
    if (memory_size_in_mb < MEMORY_SIZE_MIN || memory_size_in_mb > MEMORY_SIZE_MAX) {
        error_and_exit ();
    }
    size_in_words = memory_size_in_mb * 1024 * 1024 / sizeof (uint64_t);
    size_per_region = size_in_words / N_REGIONS;

    n_accesses = strtoll (argv[2], NULL, 0);
    if (n_accesses < ACCESSES_MIN || n_accesses > ACCESSES_MAX) {
        error_and_exit ();
    }

    if (argc == 4) {
        skew_level = strtol (argv[3], NULL, 0);
        if (skew_level < 0 || skew_level > 9) {
            error_and_exit ();
        }
    }

    srandom (seed);

    /* Create N_REGIONS regions, with potentially different weights.  While calculations
     * are done using FP math, the actual region weight 
     */
    for (i = 0, weight = 1.0, total_weight = 0ULL; i < N_REGIONS; ++i) {
        total_weight += (region_weight[i] = (int64_t)((double)size_per_region * weight));
        region_w[i] = weight;
        weight *= (1.0 + (double)skew_level * 0.1);
    }

    if ((m = (uint64_t *)malloc (memory_size_in_mb * 1024LL * 1024LL)) == NULL) {
        fprintf (stderr, "%s: unable to allocate %d MB of memory!\n", prog, memory_size_in_mb);
        error_and_exit ();
    }
    memset (m, 0, size_in_words * sizeof (uint64_t));
    memset (reads_per_region, 0, sizeof (reads_per_region));
    memset (writes_per_region, 0, sizeof (writes_per_region));

    for (i = 0; i < n_accesses; ++i) {
        do {
            r = rand64 ();
        } while (r > (0xffffffffffffffffULL - total_weight));
        loc = rand64 () % total_weight;
        for (w = 0ULL, region = 0; region < N_REGIONS; ++region) {
            if (loc < w + region_weight[region]) {
                break;
            }
            w += region_weight[region];
        }
        loc -= w;
        loc = (int64_t)((double)loc / region_w[region]) + region * size_per_region;
        if (rand64() & 1) {
            m[loc] ^= rand64 ();
            writes_per_region[region]++;
        }
        all ^= m[loc];
        reads_per_region[region]++;
    }
    /* Use the value of all to ensure the access loop isn't optimized out */
    srandom ((unsigned)(all & 0xfffffff));
    printf ("%6s  %8s  %8s  %5s\n", "Region", "Reads", "Writes", "RW%");
    for (region = 0; region < N_REGIONS; ++region) {
        printf ("%6lu  %8lu  %8lu  %4.1f%%\n", region,
                reads_per_region[region], writes_per_region[region],
                100.0 * (double)writes_per_region[region] / (double)reads_per_region[region]);
    }
}
