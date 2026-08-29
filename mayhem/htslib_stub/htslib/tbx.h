/*
 * Minimal STUB of htslib's <htslib/tbx.h> — additive, lives only under mayhem/.
 *
 * kent's lib/linefile.c unconditionally #includes "htslib/tbx.h" to support the
 * tabix (bgzip + .tbi index) code path. The real htslib is a large dependency
 * tree (zlib/bzip2/liblzma/libcurl) and is not needed by this integration:
 * the fuzz harness drives linefile via lineFileOnString() (an in-memory buffer),
 * which NEVER takes the tabix path. This header supplies exactly the types and
 * function declarations linefile.c references so it compiles unmodified; the
 * bodies (mayhem/htslib_stub.c) abort if ever called, which they are not for a
 * string-backed lineFile.
 *
 * Nothing here is derived from htslib source; it is a hand-written compile shim.
 */
#ifndef MAYHEM_HTSLIB_TBX_STUB_H
#define MAYHEM_HTSLIB_TBX_STUB_H

#include <stddef.h>

/* kstring_t: linefile.c only ever touches the ->s (char*) member. */
typedef struct kstring_t {
    size_t l, m;
    char *s;
} kstring_t;

/* Opaque handles — linefile.c stores these as void* in struct lineFile and only
 * passes them back to the (stubbed) htslib functions below. */
typedef struct htsFile htsFile;
typedef struct tbx_t tbx_t;
typedef struct hts_itr_t hts_itr_t;

#ifndef HTS_IDX_REST
#define HTS_IDX_REST (-2)
#endif

/* Function declarations used directly or via the ti_* macros in linefile.h.
 * (In real htslib several of these are macros; declaring them as functions is
 * fine here because they are never invoked at run time.) */
htsFile   *hts_open(const char *fn, const char *mode);
int        hts_close(htsFile *fp);
tbx_t     *tbx_index_load(const char *fn);
tbx_t     *tbx_index_load3(const char *fn, const char *fnidx, int flags);
void       tbx_destroy(tbx_t *tbx);
int        tbx_name2id(tbx_t *tbx, const char *ss);
hts_itr_t *tbx_itr_queryi(const tbx_t *tbx, int tid, long beg, long end);
void       tbx_itr_destroy(hts_itr_t *iter);
int        tbx_itr_next(htsFile *fp, tbx_t *tbx, hts_itr_t *iter, void *r);

#endif /* MAYHEM_HTSLIB_TBX_STUB_H */
