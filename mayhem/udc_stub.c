/*
 * Stub bodies for the udc (URL data cache) symbols kent's lib/linefile.c
 * references. Additive (lives under mayhem/).
 *
 * lib/linefile.c calls udcIsLocal/udcFileMayOpen/... only when opening an
 * http(s)/ftp URL. Linking the real lib/udc.c would drag in net.c + https.c and
 * the whole OpenSSL/socket stack purely as dead code — this in-memory PSL harness
 * (and the KAT probe) build their lineFile with lineFileOnString(), which NEVER
 * assigns lf->udcFile, so none of the udc code path is reachable. We therefore
 * omit udc.c from the build and satisfy linefile.c's references with these stubs,
 * which abort loudly if the URL path is ever taken (it is not).
 *
 * Including the real kent headers guarantees the signatures match exactly.
 */
#include "common.h"
#include "udc.h"
#include <stdio.h>
#include <stdlib.h>

static void mayhem_udc_unreachable(const char *fn)
{
    fprintf(stderr, "mayhem udc stub: %s called but URL input is unsupported "
                    "in this in-memory harness\n", fn);
    abort();
}

boolean udcIsLocal(char *url)
{ (void)url; mayhem_udc_unreachable("udcIsLocal"); return FALSE; }

struct udcFile *udcFileMayOpen(char *url, char *cacheDir)
{ (void)url; (void)cacheDir; mayhem_udc_unreachable("udcFileMayOpen"); return NULL; }

void udcFileClose(struct udcFile **pFile)
{ (void)pFile; mayhem_udc_unreachable("udcFileClose"); }

char *udcReadLine(struct udcFile *file)
{ (void)file; mayhem_udc_unreachable("udcReadLine"); return NULL; }

void udcSeek(struct udcFile *file, bits64 offset)
{ (void)file; (void)offset; mayhem_udc_unreachable("udcSeek"); }

bits64 udcTell(struct udcFile *file)
{ (void)file; mayhem_udc_unreachable("udcTell"); return 0; }
