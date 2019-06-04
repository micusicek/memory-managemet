#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

//////////////////////////////////////////////////
////////////////////////////////////////////////// globals
//////////////////////////////////////////////////

char *activityFilename = "memory.dat";

// see https://www.geeksforgeeks.org/how-to-find-size-of-array-in-cc-without-using-sizeof-operator/
// WARNING: works for arrays only, not pointers!
#define LEN(x) (*(&(x) + 1) - (x))

#define ACTIVITY_MAX_COUNT 1024
#define JOB_MAX_COUNT 1024
#define MEMORY_PAGE_MAX_COUNT 20
#define SWAP_PAGE_MAX_COUNT 1024

typedef struct {
    int jobId;
    char action; // memory action, see run for valid values
    int pageId;  // page that was affected by activity
} Activity;

typedef struct {
    int id;
    int state; // C(reated), T(erminated), K(illed), - for no process
} Job;

typedef struct {
    int jobId;
    int virtualPageId;
    int dirty;
    // add usage data for swapping algorithms
} MemoryPage;

typedef struct {
    int jobId;
    int virtualPageId;
} SwapPage;

typedef struct {
    Job j[JOB_MAX_COUNT];
    MemoryPage m[MEMORY_PAGE_MAX_COUNT];
    SwapPage s[SWAP_PAGE_MAX_COUNT];
} KernelData;

//////////////////////////////////////////////////
////////////////////////////////////////////////// functions
//////////////////////////////////////////////////

void parseJobActivity(Activity *a, char *s)
{
    s = strdup(s);
    // job ID
    char *found = strsep(&s, " ");
    if(!found) {
        printf("parseJobActivity: expected job id\n");
        exit(EXIT_FAILURE);
    }
    errno = 0;
    a->jobId = strtol(found, NULL, 10);
    if(errno) {
        printf(
            "parseJobActivity: first token cannot be parsed as int: %s\n",
            strerror(errno)
        );
        exit(EXIT_FAILURE);
    }

    // action
    found = strsep(&s, " ");
    if(!found) {
        printf("parseJobActivity: expected activity\n");
        exit(EXIT_FAILURE);
    }
    // printf("parseJobActivity: found [%s]\n", found);
    a->action = found[0];

    // page ID (if needed)
    if((a->action == 'C') || (a->action == 'T')) {
        a->pageId = -1;
        return;
    }
    found = strsep(&s, " ");
    if(!found) {
        printf("parseJobActivity: expected page id\n");
        exit(EXIT_FAILURE);
    }
    errno = 0;
    a->pageId = strtol(found, NULL, 10);
    if(errno) {
        printf(
            "parseJobActivity: third token cannot be parsed as int: %s\n",
            strerror(errno)
        );
        exit(EXIT_FAILURE);
    }
}

void readActivity(char *filename, Activity *a, int aCount) {
    FILE *f = fopen(filename, "r");
    if(f == NULL) {
        printf(
            "ERROR: cannot open file [%s] for reading: %s\n",
            filename,
            strerror(errno)
        );
        exit(EXIT_FAILURE);
    }

    char line[1024];
    int i = 0;

    while(fgets(line, sizeof(line), f) != NULL) {
        parseJobActivity(&a[i], line);
        i++;
        if(i >= aCount) {
            printf("readActivity: FATAL: too many entries [%d]\n", aCount);
            exit(EXIT_FAILURE);
        }
    }
}

void activityInitialize(Activity *activities, int aLen) {
    for(int i = 0; i < aLen; i++) {
        Activity *a = &activities[i];
        a->jobId  = -1;
        a->action = '-';
        a->pageId = -1;
    }
}

void printActivity(Activity *a)
{
    printf(
        "jobId [%d] action [%c]",
        a->jobId,
        a->action
    );
    if(a->pageId != -1) {
        printf(" page [%d]\n", a->pageId);
    } else {
        printf("\n");
    }
}

void memoryInitialize(MemoryPage *m)
{
    m->jobId         = -1;
    m->virtualPageId = -1;
    m->dirty         = 0;
}

void memoryPrint(MemoryPage *memoryPages, int size)
{
    int isMemoryEmpty = 1;
    for(int i = 0; i < size; i++) {
        MemoryPage *m = &memoryPages[i];
        if(m->jobId == -1) {
            continue;
        }
        isMemoryEmpty = 0;
        printf(
            "\tpage [%d] job id [%d] virtual page id [%d] dirty [%d]\n",
            i,
            m->jobId,
            m->virtualPageId,
            m->dirty
        );
    }
    if(isMemoryEmpty) {
        printf("\tmemory is empty\n");
    }
}

void swapInitialize(SwapPage *s)
{
    s->jobId         = -1;
    s->virtualPageId = -1;
}

void swapPrint(KernelData *k)
{
    int isSwapEmpty = 1;
    for(int i = 0; i < LEN(k->s); i++) {
        SwapPage *s = &k->s[i];
        if(s->jobId == -1) {
            continue;
        }
        isSwapEmpty = 0;
        printf(
            "\tswap page [%d] job id [%d] virtual page id [%d]\n",
            i,
            s->jobId,
            s->virtualPageId
        );
    }
    if(isSwapEmpty) {
        printf("\tswap is empty\n");
    }
}

int swapFind(KernelData *k, int jobId, int virtualPageId)
{
    for(int i = 0; i < LEN(k->s); i++) {
        SwapPage *s = &k->s[i];
        if((s->jobId == jobId) && (s->virtualPageId == virtualPageId)) {
            return i;
        }
    }

    return -1;
}

int swapFindMemoryPage(KernelData *k, MemoryPage *m)
{
    return swapFind(k, m->jobId, m->virtualPageId);
}

int swapDestroy(KernelData *k, int jobId, int virtualPageId)
{
    int destroyedSomething = 0;

    for(int i = 0; i < LEN(k->s); i++) {
        SwapPage *s = &k->s[i];
        if((s->jobId == jobId) && (s->virtualPageId == virtualPageId)) {
            swapInitialize(s);
            destroyedSomething = 1;
        }
    }

    return destroyedSomething ? 0 : -1;
}

void swapDestroyMemoryPage(KernelData *k, MemoryPage *m)
{
    for(int i = 0; i < LEN(k->s); i++) {
        SwapPage *s = &k->s[i];
        if((s->jobId == m->jobId) && (s->virtualPageId == m->virtualPageId)) {
            swapInitialize(s);
        }
    }
}

void swapDestroyJobPages(KernelData *k, int jobId)
{
    for(int i = 0; i < LEN(k->s); i++) {
        SwapPage *s = &k->s[i];
        if(s->jobId == jobId) {
            swapInitialize(s);
        }
    }
}

int swapSelectFIFO(MemoryPage *m, int size)
{
    printf("FATAL: swap select FIFO not implemented yet\n");
}

int swapSelectLRU(MemoryPage *m, int size)
{
    printf("FATAL: swap select LRU not implemented yet\n");
}

int swapSelectRandom(MemoryPage *m, int size)
{
    return rand() % size;
}

int swapOut(KernelData *k)
{
    // if there's clean page that's also in swap, use it
    for(int i = 0; i < LEN(k->m); i++) {
        MemoryPage *m = &k->m[i];
        // is this page clean and in swap?
        if(!m->dirty && (swapFindMemoryPage(k, m) != -1)) {
            // it's already in swap and it wasn't changed so we can use it
            return i;
        }
    }

    // pick any page and use it (swap out existing data first)
    int mIndex = swapSelectRandom(k->m, LEN(k->m));
    MemoryPage *m = &k->m[mIndex];

    for(int i = 0; i < LEN(k->s); i++) {
        SwapPage *s = &k->s[i];
        if(s->jobId == -1) {
            s->jobId = m->jobId;
            s->virtualPageId = m->virtualPageId;
            break;
        }
    }

    memoryInitialize(m); // refurbish :)

    return mIndex;
}

int swapIn(KernelData *k, int jobId, int virtualPageId)
{
    int sIndex = swapFind(k, jobId, virtualPageId);
    if(sIndex == -1) {
        return -1;
    }

    int mIndex = swapOut(k);
    MemoryPage *m = &k->m[mIndex];
    SwapPage   *s = &k->s[sIndex];
    m->jobId = s->jobId;
    m->virtualPageId = s->virtualPageId;

    return mIndex;
}

void jobsPrint(Job *jobs, int size)
{
    int isJobListEmpty = 1;
    for(int i = 0; i< size; i++) {
        Job *j = &jobs[i];
        if(j->id == -1) {
            continue;
        }
        isJobListEmpty = 0;
        printf("\tjob id [%d] state [%c]\n", j->id, j->state);
    }
    if(isJobListEmpty) {
        printf("\tno jobs\n");
    }
}

int jobIndex(KernelData *k, int id)
{
    for(int i = 0; i < LEN(k->j); i++) {
        Job *j = &k->j[i];
        if((j->id == id) && (j->state == 'C')) {
            return i;
        }
    }

    return -1;
}

void jobMustNotExist(KernelData *k, int id)
{
    if(jobIndex(k, id) != -1) {
        printf("FATAL: job id [%d] is already running\n", id);
        exit(EXIT_FAILURE);
    }
}

// end the job, state should be one of T(erminate), K(ill)
int jobEnd(KernelData *k, int jobId, char state)
{
    int jIndex = jobIndex(k, jobId);
    if(jIndex == -1) {
        return -1;
    }

    k->j[jIndex].state = state;

    // free job memory
    for(int i = 0; i < LEN(k->m); i++) {
        MemoryPage *m = &k->m[i];
        if(m->jobId == jobId) {
            memoryInitialize(m);
        }
    }

    swapDestroyJobPages(k, jobId);

    return jIndex;
}

void kernelInitialize(KernelData *k)
{
    for(int i = 0; i < LEN(k->j); i++) {
        Job *j   = &k->j[i];
        j->id    = -1;
        j->state = '-';
    }
    for(int i = 0; i < LEN(k->m); i++) {
        MemoryPage *m = &k->m[i];
        memoryInitialize(m);
    }
    for(int i = 0; i < LEN(k->s); i++) {
        SwapPage *s      = &k->s[i];
        s->jobId         = -1;
        s->virtualPageId = -1;
    }
}

void kernelPrint(KernelData *k)
{
    printf("kernelPrint: jobs:\n");
    jobsPrint(k->j, LEN(k->j));
    printf("kernelPrint: memory:\n");
    memoryPrint(k->m, LEN(k->m));
    printf("kernelPrint: swap:\n");
    swapPrint(k);
}

int runCreate(Activity *a, KernelData *k) {
    jobMustNotExist(k, a->jobId);

    for(int i = 0; i < LEN(k->j); i++) {
        Job *j = &k->j[i];
        if(j->id == -1) {
            // add job to job table
            j->id    = a->jobId;
            j->state = 'C';
            return 0;
        }

    }

    // if we get here there is no more space in job table
    kernelPrint(k);
    printf("Too many jobs\n");
    exit(EXIT_FAILURE);
}

int runTerminate(Activity *a, KernelData *k) {
    if(jobEnd(k, a->jobId, 'T') == -1) {
        printf("WARN: action refers to a non existent job\n");
        return -1;
    }
    return 0;
}

int runAllocate(Activity *a, KernelData *k) {
    if(jobIndex(k, a->jobId) == -1) {
        printf("WARN: action refers to non existent job\n");
        return -1;
    }

    // allocate memory for job
    for(int i = 0; i < LEN(k->m); i++) {
        MemoryPage *m = &k->m[i];
        if(m->jobId == -1) {
            m->jobId         = a->jobId;
            m->virtualPageId = a->pageId;
            m->dirty         = 0;
            return 0;
        }
    }

    // swap a page out and use it
    // TODO: make swap selector a parameter (FIFO, LRU, Random)
    int mIndex       = swapOut(k);
    MemoryPage *m    = &k->m[mIndex];
    m->jobId         = a->jobId;
    m->virtualPageId = a->pageId;
    m->dirty         = 0;

    return 0;
}

int runRead(Activity *a, KernelData *k) {
    if(jobIndex(k, a->jobId) == -1) {
        printf("WARN: action refers to non existent job\n");
        return -1;
    }

    // prepare to maybe mark it read later
    for(int i = 0; i < LEN(k->m); i++) {
        MemoryPage *m = &k->m[i];
        if((m->jobId == a->jobId) && (m->virtualPageId == a->pageId)) {
            // maybe later mark it as read
            return 0;
        }
    }

    // might be in swap
    int mIndex = swapIn(k, a->jobId, a->pageId);
    if(mIndex != -1) {
        // mark read later maybe?
        return 0;
    }

    // page does not exist, BAD job!
    jobEnd(k, a->jobId, 'K');

    return 0;
}

int runWrite(Activity *a, KernelData *k) {
    if(jobIndex(k, a->jobId) == -1) {
        printf("WARN: action refers to non existent job\n");
        return -1;
    }

    // mark page dirty
    for(int i = 0; i < LEN(k->m); i++) {
        MemoryPage *m = &k->m[i];
        if((m->jobId == a->jobId) && (m->virtualPageId == a->pageId)) {
            m->dirty = 1;
            swapDestroyMemoryPage(k, m);
            return 0;
        }
    }

    // might be in swap
    int mIndex = swapIn(k, a->jobId, a->pageId);
    if(mIndex != -1) {
        // we're writing so it's dirty
        k->m[mIndex].dirty = 1;
        return 0;
    }

    // page does not exist, BAD job!
    jobEnd(k, a->jobId, 'K');

    return 0;
}

int runFree(Activity *a, KernelData *k) {
    if(jobIndex(k, a->jobId) == -1) {
        printf("WARN: action refers to non existent job\n");
        return -1;
    }

    int deletedSomething = 0;

    // free page
    for(int i = 0; i < LEN(k->m); i++) {
        MemoryPage *m = &k->m[i];
        if((m->jobId == a->jobId) && (m->virtualPageId == a->pageId)) {
            memoryInitialize(m);
            deletedSomething = 1;
            break;
        }
    }

    // free page in swap
    if(swapDestroy(k, a->jobId, a->pageId) != -1) {
        deletedSomething = 1;
    }

    if(!deletedSomething) {
        // page does not exist, BAD job!
        jobEnd(k, a->jobId, 'K');
    }

    return 0;
}

void run(Activity *activities, int aCount)
{
    KernelData *k = malloc(sizeof(KernelData));
    kernelInitialize(k);

    int i;
    Activity *a;

    for(
        i = 0, a = &activities[i];
        (i < aCount) && (a->jobId != -1);
        i++, a = &activities[i]
    ) {
        Activity *a = &activities[i];
        printf("run: ****************************************************\n");
        printf("run: activity: ");
        printActivity(a);
        int runResult;
        switch(a->action) {
        case 'C':
            runResult = runCreate(a, k);
            break;
        case 'T':
            runResult = runTerminate(a, k);
            break;
        case 'A':
            runResult = runAllocate(a, k);
            break;
        case 'R':
            runResult = runRead(a, k);
            break;
        case 'W':
            runResult = runWrite(a, k);
            break;
        case 'F':
            runResult = runFree(a, k);
            break;
        default:
            printf("run: unknown action %c\n", a->action);
            exit(EXIT_FAILURE);
        }
        if(runResult != -1) {
            kernelPrint(k);
        }
    }

    if(k != NULL) { free(k); }
}

//////////////////////////////////////////////////
////////////////////////////////////////////////// main
//////////////////////////////////////////////////

int main(void)
{
    // seed the random number gennerator (for swapSelectRandom)
    srand(time(0));

    Activity a[ACTIVITY_MAX_COUNT];

    activityInitialize(a, LEN(a));

    readActivity(activityFilename, a, LEN(a));

    run(a, LEN(a));

    return 0;
}
