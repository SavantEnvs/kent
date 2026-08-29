/*
 * Known-answer-test probe for the kent PSL parser — the behavioral oracle.
 *
 * Built by mayhem/build.sh with the project's NORMAL flags (no sanitizer, no
 * -gdwarf-3): a clean, dynamically-linked binary. mayhem/test.sh runs it and
 * greps its stdout for EXACT parsed field values. Because it is dynamically
 * linked, verify-repo's sabotage shim (LD_PRELOAD constructor that _exit(0)s
 * every non-system executable) neuters it -> it prints nothing -> the greps
 * miss -> test.sh FAILS. That is what makes the oracle behavioral rather than
 * a liveness/exit-code check.
 *
 * The fixed input is one canonical 21-column PSL record with hand-computed
 * fields; we assert the parser recovers each one (counts, names, coordinates,
 * strand, block count, and individual block-array elements).
 */
#include <stdio.h>
#include "common.h"
#include "linefile.h"
#include "psl.h"

/* match misMatch repMatch nCount qNumIns qBaseIns tNumIns tBaseIns strand
 * qName qSize qStart qEnd tName tSize tStart tEnd blockCount
 * blockSizes qStarts tStarts   (trailing commas as kent writes them) */
static const char *KAT_PSL =
    "30\t2\t0\t0\t1\t5\t1\t10\t+\t"
    "qSeq\t100\t5\t42\tchr1\t1000\t200\t242\t2\t"
    "20,10,\t5,30,\t200,232,\n";

int main(void)
{
    char *dup = cloneString(KAT_PSL);
    struct lineFile *lf = lineFileOnString("kat.psl", TRUE, dup);
    struct psl *psl = pslNext(lf);
    if (psl == NULL)
        {
        fprintf(stderr, "KAT: pslNext returned NULL\n");
        return 2;
        }

    printf("match=%u\n", psl->match);
    printf("misMatch=%u\n", psl->misMatch);
    printf("qNumInsert=%u\n", psl->qNumInsert);
    printf("tBaseInsert=%d\n", psl->tBaseInsert);
    printf("strand=%s\n", psl->strand);
    printf("qName=%s\n", psl->qName);
    printf("qSize=%u\n", psl->qSize);
    printf("tName=%s\n", psl->tName);
    printf("tStart=%d\n", psl->tStart);
    printf("tEnd=%d\n", psl->tEnd);
    printf("blockCount=%u\n", psl->blockCount);
    printf("blockSize0=%u\n", psl->blockSizes[0]);
    printf("blockSize1=%u\n", psl->blockSizes[1]);
    printf("tStart0=%u\n", psl->tStarts[0]);
    printf("tStart1=%u\n", psl->tStarts[1]);

    pslFree(&psl);
    lineFileClose(&lf);
    return 0;
}
