typedef struct BlockHeader {
    int size;
    short int padding;
    char marked;
    char allocated;
    unsigned long finalizer_address;
} BlockHeader;

// Interface
int memInitialize(unsigned long size);
void *memAllocate(unsigned long size, void (*finalize)(void *));
void memDump(void);

static void GC_garbageCollector(void);
static void GC_mark(void *start, void *end);
static int GC_markIfPointerToAllocatedBlock(unsigned long ptr);
static void GC_markRegisters(void);
static void GC_sweep(void);
static void GC_coalesce(void);


static void dumpHeap(void);
static void dumpStack(void);
static void dumpRegisters(void);
static void dumpGlobalMemory(void);

extern unsigned long getSP();
extern unsigned long getFP();
extern unsigned long getRBX();
extern unsigned long getRSI();
extern unsigned long getRDI();



