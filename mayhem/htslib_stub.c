/*
 * Stub bodies for the htslib symbols kent's lib/linefile.c references.
 * Additive (lives under mayhem/). See mayhem/htslib_stub/htslib/tbx.h for why.
 *
 * The fuzz harness and the KAT probe drive lineFile via lineFileOnString()
 * (an in-memory string), which never enters the tabix/bgzf code path, so none
 * of these are reached. They abort loudly if that assumption is ever violated,
 * rather than silently returning bogus handles.
 */
#include "htslib/tbx.h"
#include <stdio.h>
#include <stdlib.h>

static void mayhem_htslib_unreachable(const char *fn)
{
    fprintf(stderr, "mayhem htslib stub: %s called but tabix path is unsupported "
                    "in this in-memory harness\n", fn);
    abort();
}

htsFile *hts_open(const char *fn, const char *mode)
{ (void)fn; (void)mode; mayhem_htslib_unreachable("hts_open"); return NULL; }

int hts_close(htsFile *fp)
{ (void)fp; mayhem_htslib_unreachable("hts_close"); return -1; }

tbx_t *tbx_index_load(const char *fn)
{ (void)fn; mayhem_htslib_unreachable("tbx_index_load"); return NULL; }

tbx_t *tbx_index_load3(const char *fn, const char *fnidx, int flags)
{ (void)fn; (void)fnidx; (void)flags; mayhem_htslib_unreachable("tbx_index_load3"); return NULL; }

void tbx_destroy(tbx_t *tbx)
{ (void)tbx; mayhem_htslib_unreachable("tbx_destroy"); }

int tbx_name2id(tbx_t *tbx, const char *ss)
{ (void)tbx; (void)ss; mayhem_htslib_unreachable("tbx_name2id"); return -1; }

hts_itr_t *tbx_itr_queryi(const tbx_t *tbx, int tid, long beg, long end)
{ (void)tbx; (void)tid; (void)beg; (void)end; mayhem_htslib_unreachable("tbx_itr_queryi"); return NULL; }

void tbx_itr_destroy(hts_itr_t *iter)
{ (void)iter; mayhem_htslib_unreachable("tbx_itr_destroy"); }

int tbx_itr_next(htsFile *fp, tbx_t *tbx, hts_itr_t *iter, void *r)
{ (void)fp; (void)tbx; (void)iter; (void)r; mayhem_htslib_unreachable("tbx_itr_next"); return -1; }
