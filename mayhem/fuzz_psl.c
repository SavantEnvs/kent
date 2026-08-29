/*
 * In-process libFuzzer harness for kent's PSL alignment-format parser.
 *
 * PSL is UCSC's tab-separated pairwise-alignment format (the output of BLAT/isPCR
 * and many pipeline tools). kent parses it in lib/psl.c: a line is chopped on
 * whitespace into 21 (psl) or 23 (pslx) columns and the numeric fields
 * (match/misMatch counts, blockCount, the comma-separated blockSizes / qStarts /
 * tStarts arrays, ...) are converted with the sqlNum/sqlList helpers. That parser
 * and its conversions are the memory-safety attack surface for an untrusted .psl.
 *
 * We drive the SAME code in-process with ZERO file I/O by wrapping the fuzz bytes
 * in an in-memory lineFile (lineFileOnString) — no /tmp, no /dev/shm, no reliance
 * on cwd (SPEC net-new §3).
 *
 * Memory model — why the *Lm (local-memory) loaders:
 *   The public pslNext()/pslLoad() allocate every field with the GLOBAL allocator
 *   (needMem/cloneString) and rely on sqlUnsigned() calling errAbort() to *exit the
 *   process* on a malformed field. Under a fuzzer we must instead RECOVER from that
 *   errAbort (via errCatch, otherwise every bad line looks like a crash) — but the
 *   longjmp abandons the half-built struct, so the global loaders leak on nearly
 *   every malformed input, and that LSan noise drowns real bugs. pslLoadLm()/
 *   pslxLoadLm() allocate EVERYTHING from a caller-owned localmem pool instead, so a
 *   single lmCleanup() after each input reclaims the whole parse — including any
 *   allocations the errAbort longjmp left behind. Same parser code, no error-path
 *   leak. (This mirrors the "call the library's own release API per iteration"
 *   guidance rather than disabling leak detection.)
 *
 * We replicate pslNext()'s own read loop (lineFileNextReal + chopLine + the 21/23
 * column dispatch) so the exercised code is identical, only routed through the Lm
 * loaders. A line whose column count is neither 21 nor 23 is skipped (pslNext would
 * errAbort; skipping just lets the rest of the input keep exercising the parser).
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "linefile.h"
#include "localmem.h"
#include "psl.h"
#include "errCatch.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* lineFileOnString takes the buffer, tokenizes in place (zTerm), never
     * reallocates it and does NOT free it on close — so hand it a private,
     * NUL-terminated heap copy and free it ourselves afterwards (the idiom kent
     * uses in asParse.c: lineFileClose then freez(&dupe)). */
    char *buf = malloc(size + 1);
    if (buf == NULL)
        return 0;
    if (size > 0)
        memcpy(buf, data, size);
    buf[size] = '\0';

    struct lineFile *lf = lineFileOnString("fuzz.psl", TRUE, buf);
    struct lm *lm = lmInit(0);

    struct errCatch *errCatch = errCatchNew();
    if (errCatchStart(errCatch))
        {
        char *line;
        while (lineFileNextReal(lf, &line))
            {
            /* chopLine mutates its buffer; copy into the pool so the source line
             * (inside lf->buf) is untouched for the next read, and so the copy is
             * reclaimed by lmCleanup along with everything else. */
            int lineSize = strlen(line);
            char *chopBuf = lmAlloc(lm, lineSize + 1);
            memcpy(chopBuf, line, lineSize + 1);

            char *words[32];
            int wordCount = chopLine(chopBuf, words);
            if (wordCount == 21)
                pslLoadLm(words, lm);
            else if (wordCount == 23)
                pslxLoadLm(words, lm);
            /* else: not a PSL record — keep parsing the remaining lines. */
            }
        }
    errCatchEnd(errCatch);
    errCatchFree(&errCatch);

    lmCleanup(&lm);       /* frees the whole parse, incl. anything errAbort abandoned */
    lineFileClose(&lf);   /* frees lf + lf->fileName, but NOT buf */
    free(buf);
    return 0;
}
