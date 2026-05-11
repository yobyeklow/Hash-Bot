#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef _WIN32
#include <windows.h>
static uint64_t now_ms(void) {
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (uli.QuadPart / 10000ull) - 11644473600000ull;
}
#else
#include <dlfcn.h>
#include <time.h>
static uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}
#endif

typedef intptr_t cl_intptr_t;
typedef uintptr_t cl_uintptr_t;
typedef cl_uintptr_t cl_bitfield;
typedef cl_bitfield cl_device_type;
typedef cl_bitfield cl_mem_flags;
typedef cl_bitfield cl_command_queue_properties;
typedef uint32_t cl_uint;
typedef int32_t cl_int;
typedef uint64_t cl_ulong;
typedef uint8_t cl_bool;
typedef struct _cl_platform_id *cl_platform_id;
typedef struct _cl_device_id *cl_device_id;
typedef struct _cl_context *cl_context;
typedef struct _cl_command_queue *cl_command_queue;
typedef struct _cl_mem *cl_mem;
typedef struct _cl_program *cl_program;
typedef struct _cl_kernel *cl_kernel;
typedef struct _cl_event *cl_event;
typedef cl_uint cl_platform_info;
typedef cl_uint cl_device_info;
typedef cl_uint cl_program_build_info;

#define CL_SUCCESS 0
#define CL_FALSE 0
#define CL_TRUE 1
#define CL_DEVICE_TYPE_GPU (1u << 2)
#define CL_MEM_READ_ONLY (1u << 2)
#define CL_MEM_WRITE_ONLY (1u << 1)
#define CL_MEM_COPY_HOST_PTR (1u << 5)
#define CL_PROGRAM_BUILD_LOG 0x1183
#define CL_DEVICE_NAME 0x102B
#define CL_DEVICE_VENDOR 0x102F
#define CL_DEVICE_MAX_COMPUTE_UNITS 0x1002
#define CL_DEVICE_MAX_WORK_GROUP_SIZE 0x1004
#define CL_PLATFORM_NAME 0x0902

typedef cl_int (*PFN_clGetPlatformIDs)(cl_uint, cl_platform_id *, cl_uint *);
typedef cl_int (*PFN_clGetPlatformInfo)(cl_platform_id, cl_platform_info, size_t, void *, size_t *);
typedef cl_int (*PFN_clGetDeviceIDs)(cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint *);
typedef cl_int (*PFN_clGetDeviceInfo)(cl_device_id, cl_device_info, size_t, void *, size_t *);
typedef cl_context (*PFN_clCreateContext)(const cl_intptr_t *, cl_uint, const cl_device_id *, void *, void *, cl_int *);
typedef cl_command_queue (*PFN_clCreateCommandQueue)(cl_context, cl_device_id, cl_command_queue_properties, cl_int *);
typedef cl_mem (*PFN_clCreateBuffer)(cl_context, cl_mem_flags, size_t, void *, cl_int *);
typedef cl_program (*PFN_clCreateProgramWithSource)(cl_context, cl_uint, const char **, const size_t *, cl_int *);
typedef cl_int (*PFN_clBuildProgram)(cl_program, cl_uint, const cl_device_id *, const char *, void *, void *);
typedef cl_int (*PFN_clGetProgramBuildInfo)(cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *);
typedef cl_kernel (*PFN_clCreateKernel)(cl_program, const char *, cl_int *);
typedef cl_int (*PFN_clSetKernelArg)(cl_kernel, cl_uint, size_t, const void *);
typedef cl_int (*PFN_clEnqueueWriteBuffer)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (*PFN_clEnqueueReadBuffer)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (*PFN_clEnqueueNDRangeKernel)(cl_command_queue, cl_kernel, cl_uint, const size_t *, const size_t *, const size_t *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (*PFN_clFinish)(cl_command_queue);
typedef cl_int (*PFN_clReleaseMemObject)(cl_mem);
typedef cl_int (*PFN_clReleaseKernel)(cl_kernel);
typedef cl_int (*PFN_clReleaseProgram)(cl_program);
typedef cl_int (*PFN_clReleaseCommandQueue)(cl_command_queue);
typedef cl_int (*PFN_clReleaseContext)(cl_context);

typedef struct {
    PFN_clGetPlatformIDs clGetPlatformIDs;
    PFN_clGetPlatformInfo clGetPlatformInfo;
    PFN_clGetDeviceIDs clGetDeviceIDs;
    PFN_clGetDeviceInfo clGetDeviceInfo;
    PFN_clCreateContext clCreateContext;
    PFN_clCreateCommandQueue clCreateCommandQueue;
    PFN_clCreateBuffer clCreateBuffer;
    PFN_clCreateProgramWithSource clCreateProgramWithSource;
    PFN_clBuildProgram clBuildProgram;
    PFN_clGetProgramBuildInfo clGetProgramBuildInfo;
    PFN_clCreateKernel clCreateKernel;
    PFN_clSetKernelArg clSetKernelArg;
    PFN_clEnqueueWriteBuffer clEnqueueWriteBuffer;
    PFN_clEnqueueReadBuffer clEnqueueReadBuffer;
    PFN_clEnqueueNDRangeKernel clEnqueueNDRangeKernel;
    PFN_clFinish clFinish;
    PFN_clReleaseMemObject clReleaseMemObject;
    PFN_clReleaseKernel clReleaseKernel;
    PFN_clReleaseProgram clReleaseProgram;
    PFN_clReleaseCommandQueue clReleaseCommandQueue;
    PFN_clReleaseContext clReleaseContext;
} cl_api;

static cl_api cl;

#ifdef _WIN32
#define LIB_LOAD(f) ((void*)GetProcAddress((HMODULE)lib, f))
#define LIB_OPEN "OpenCL.dll"
static void *lib_open(void) { return LoadLibraryA(LIB_OPEN); }
#else
#define LIB_LOAD(f) dlsym(lib, f)
#define LIB_OPEN "libOpenCL.so"
static void *lib_open(void) {
    void *h = dlopen("libOpenCL.so", RTLD_NOW);
    if (!h) h = dlopen("libOpenCL.so.1", RTLD_NOW);
    return h;
}
#endif

static void fatal_cl(const char *what, cl_int err) {
    fprintf(stderr, "fatal: %s returned %d\n", what, err);
    exit(2);
}

static void load_opencl(void) {
    void *lib = lib_open();
    if (!lib) { fprintf(stderr, "OpenCL runtime not found\n"); exit(2); }
    cl.clGetPlatformIDs = (PFN_clGetPlatformIDs)LIB_LOAD("clGetPlatformIDs");
    cl.clGetPlatformInfo = (PFN_clGetPlatformInfo)LIB_LOAD("clGetPlatformInfo");
    cl.clGetDeviceIDs = (PFN_clGetDeviceIDs)LIB_LOAD("clGetDeviceIDs");
    cl.clGetDeviceInfo = (PFN_clGetDeviceInfo)LIB_LOAD("clGetDeviceInfo");
    cl.clCreateContext = (PFN_clCreateContext)LIB_LOAD("clCreateContext");
    cl.clCreateCommandQueue = (PFN_clCreateCommandQueue)LIB_LOAD("clCreateCommandQueue");
    cl.clCreateBuffer = (PFN_clCreateBuffer)LIB_LOAD("clCreateBuffer");
    cl.clCreateProgramWithSource = (PFN_clCreateProgramWithSource)LIB_LOAD("clCreateProgramWithSource");
    cl.clBuildProgram = (PFN_clBuildProgram)LIB_LOAD("clBuildProgram");
    cl.clGetProgramBuildInfo = (PFN_clGetProgramBuildInfo)LIB_LOAD("clGetProgramBuildInfo");
    cl.clCreateKernel = (PFN_clCreateKernel)LIB_LOAD("clCreateKernel");
    cl.clSetKernelArg = (PFN_clSetKernelArg)LIB_LOAD("clSetKernelArg");
    cl.clEnqueueWriteBuffer = (PFN_clEnqueueWriteBuffer)LIB_LOAD("clEnqueueWriteBuffer");
    cl.clEnqueueReadBuffer = (PFN_clEnqueueReadBuffer)LIB_LOAD("clEnqueueReadBuffer");
    cl.clEnqueueNDRangeKernel = (PFN_clEnqueueNDRangeKernel)LIB_LOAD("clEnqueueNDRangeKernel");
    cl.clFinish = (PFN_clFinish)LIB_LOAD("clFinish");
    cl.clReleaseMemObject = (PFN_clReleaseMemObject)LIB_LOAD("clReleaseMemObject");
    cl.clReleaseKernel = (PFN_clReleaseKernel)LIB_LOAD("clReleaseKernel");
    cl.clReleaseProgram = (PFN_clReleaseProgram)LIB_LOAD("clReleaseProgram");
    cl.clReleaseCommandQueue = (PFN_clReleaseCommandQueue)LIB_LOAD("clReleaseCommandQueue");
    cl.clReleaseContext = (PFN_clReleaseContext)LIB_LOAD("clReleaseContext");
    if (!cl.clGetPlatformIDs) fatal_cl("clGetPlatformIDs", -1);
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void hex_to_bytes(const char *hex, uint8_t *out, int len) {
    for (int i = 0; i < len; i++) {
        int hi = hexval(hex[i * 2]);
        int lo = hexval(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) { fprintf(stderr, "bad hex\n"); exit(2); }
        out[i] = (uint8_t)((hi << 4) | lo);
    }
}

static uint64_t parse_u64(const char *s) {
    char *end;
    uint64_t v = strtoull(s, &end, 10);
    if (*end) { fprintf(stderr, "bad integer: %s\n", s); exit(2); }
    return v;
}

// Pre-compute base keccak state with challenge absorbed + padding,
// nonce lanes (4-7) zeroed. Per-hash we only XOR nonce into lane 7.
static void prepare_base_state(const uint8_t challenge[32], uint64_t base_state[25]) {
    memset(base_state, 0, 25 * sizeof(uint64_t));
    // Absorb challenge (32 bytes = 4 lanes, little-endian)
    for (int i = 0; i < 4; i++) {
        int j = i * 8;
        base_state[i] = ((uint64_t)challenge[j]) |
                        ((uint64_t)challenge[j+1] << 8) |
                        ((uint64_t)challenge[j+2] << 16) |
                        ((uint64_t)challenge[j+3] << 24) |
                        ((uint64_t)challenge[j+4] << 32) |
                        ((uint64_t)challenge[j+5] << 40) |
                        ((uint64_t)challenge[j+6] << 48) |
                        ((uint64_t)challenge[j+7] << 56);
    }
    // Nonce lanes (4-7) stay zero — XOR nonce into lane 7 per hash
    // Padding: 0x01 at byte 64 = lane 8 byte 0
    base_state[8] ^= 0x01UL;
    // Padding: 0x80 at byte 135 = lane 16 byte 7
    base_state[16] ^= 0x8000000000000000UL;
}

static void prepare_diff_words(const uint8_t difficulty[32], uint32_t diff_words[8]) {
    for (int i = 0; i < 8; i++) {
        diff_words[i] = ((uint32_t)difficulty[i*4] << 24) |
                        ((uint32_t)difficulty[i*4+1] << 16) |
                        ((uint32_t)difficulty[i*4+2] << 8) |
                        ((uint32_t)difficulty[i*4+3]);
    }
}

static const char *kernel_source =
"__constant ulong keccakf_rndc[24] = {\n"
"  0x0000000000000001UL, 0x0000000000008082UL, 0x800000000000808aUL,\n"
"  0x8000000080008000UL, 0x000000000000808bUL, 0x0000000080000001UL,\n"
"  0x8000000080008081UL, 0x8000000000008009UL, 0x000000000000008aUL,\n"
"  0x0000000000000088UL, 0x0000000080008009UL, 0x000000008000000aUL,\n"
"  0x000000008000808bUL, 0x800000000000008bUL, 0x8000000000008089UL,\n"
"  0x8000000000008003UL, 0x8000000000008002UL, 0x8000000000000080UL,\n"
"  0x000000000000800aUL, 0x800000008000000aUL, 0x8000000080008081UL,\n"
"  0x8000000000008080UL, 0x0000000080000001UL, 0x8000000080008008UL };\n"
"__constant int keccakf_rotc[24] = {\n"
"  1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14,\n"
"  27, 41, 52, 62, 9, 19, 28, 38, 49, 58, 11, 25 };\n"
"__constant int keccakf_piln[24] = {\n"
"  10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4,\n"
"  15, 23, 19, 13, 12, 2, 20, 14, 22, 9, 6, 1 };\n"
"\n"
"uint bswap32(uint v) {\n"
"  return (v & 0x000000ffU) << 24 | (v & 0x0000ff00U) << 8 |\n"
"         (v & 0x00ff0000U) >> 8 | (v & 0xff000000U) >> 24;\n"
"}\n"
"\n"
"__kernel void mine(\n"
"  __constant const ulong *base_state,\n"
"  __constant const uint *diff_words,\n"
"  ulong start_nonce,\n"
"  ulong iter_per_thread,\n"
"  __global int *found_flag,\n"
"  __global ulong *found_nonce)\n"
"{\n"
"  ulong gid = (ulong)get_global_id(0);\n"
"  ulong global_size = (ulong)get_global_size(0);\n"
"  ulong nonce = start_nonce + gid;\n"
"  ulong end_nonce = start_nonce + gid + iter_per_thread * global_size;\n"
"\n"
"  while (*found_flag == 0 && nonce < end_nonce) {\n"
"    ulong st[25];\n"
"    for (int i = 0; i < 25; i++) st[i] = base_state[i];\n"
"\n"
"    // XOR nonce into lane 7 (bytes 56-63 of 64-byte input = last 8 bytes of uint256 nonce)\n"
"    st[7] ^= ((nonce >> 56) & 0xFFUL) | ((nonce >> 48) & 0xFFUL) << 8 |\n"
"             ((nonce >> 40) & 0xFFUL) << 16 | ((nonce >> 32) & 0xFFUL) << 24 |\n"
"             ((nonce >> 24) & 0xFFUL) << 32 | ((nonce >> 16) & 0xFFUL) << 40 |\n"
"             ((nonce >> 8)  & 0xFFUL) << 48 | (nonce & 0xFFUL) << 56;\n"
"\n"
"    // Keccak-f permutation (24 rounds)\n"
"    ulong bc[5], t;\n"
"    for (int r = 0; r < 24; r++) {\n"
"      for (int i = 0; i < 5; i++)\n"
"        bc[i] = st[i] ^ st[i+5] ^ st[i+10] ^ st[i+15] ^ st[i+20];\n"
"      for (int i = 0; i < 5; i++) {\n"
"        t = bc[(i+4)%5] ^ ((bc[(i+1)%5] << 1) | (bc[(i+1)%5] >> 63));\n"
"        for (int j = 0; j < 25; j += 5) st[j+i] ^= t;\n"
"      }\n"
"      t = st[1];\n"
"      for (int i = 0; i < 24; i++) {\n"
"        int j = keccakf_piln[i];\n"
"        bc[0] = st[j];\n"
"        st[j] = ((t << keccakf_rotc[i]) | (t >> (64 - keccakf_rotc[i])));\n"
"        t = bc[0];\n"
"      }\n"
"      for (int j = 0; j < 25; j += 5) {\n"
"        bc[0] = st[j]; bc[1] = st[j+1]; bc[2] = st[j+2]; bc[3] = st[j+3]; bc[4] = st[j+4];\n"
"        for (int i = 0; i < 5; i++)\n"
"          st[j+i] = bc[i] ^ ((~bc[(i+1)%5]) & bc[(i+2)%5]);\n"
"      }\n"
"      st[0] ^= keccakf_rndc[r];\n"
"    }\n"
"\n"
"    // Squeeze first 256 bits (lanes 0-3), compare as big-endian 32-bit words\n"
"    uint w0 = bswap32((uint)st[0]), w1 = bswap32((uint)(st[0] >> 32));\n"
"    uint w2 = bswap32((uint)st[1]), w3 = bswap32((uint)(st[1] >> 32));\n"
"    uint w4 = bswap32((uint)st[2]), w5 = bswap32((uint)(st[2] >> 32));\n"
"    uint w6 = bswap32((uint)st[3]), w7 = bswap32((uint)(st[3] >> 32));\n"
"\n"
"    int less = 0;\n"
"    if (w0 < diff_words[0]) less = 1;\n"
"    else if (w0 == diff_words[0]) {\n"
"      if (w1 < diff_words[1]) less = 1;\n"
"      else if (w1 == diff_words[1]) {\n"
"        if (w2 < diff_words[2]) less = 1;\n"
"        else if (w2 == diff_words[2]) {\n"
"          if (w3 < diff_words[3]) less = 1;\n"
"          else if (w3 == diff_words[3]) {\n"
"            if (w4 < diff_words[4]) less = 1;\n"
"            else if (w4 == diff_words[4]) {\n"
"              if (w5 < diff_words[5]) less = 1;\n"
"              else if (w5 == diff_words[5]) {\n"
"                if (w6 < diff_words[6]) less = 1;\n"
"                else if (w6 == diff_words[6]) {\n"
"                  if (w7 < diff_words[7]) less = 1;\n"
"                }\n"
"              }\n"
"            }\n"
"          }\n"
"        }\n"
"      }\n"
"    }\n"
"\n"
"    if (less) {\n"
"      if (atomic_cmpxchg(found_flag, 0, 1) == 0) {\n"
"        *found_nonce = nonce;\n"
"      }\n"
"      return;\n"
"    }\n"
"    nonce += global_size;\n"
"  }\n"
"}\n";

int main(int argc, char **argv) {
    const char *challenge_hex = NULL;
    const char *difficulty_hex = NULL;
    size_t batch_size = 1u << 20;
    size_t local_size = 256;
    uint64_t start_nonce = 0;
    uint64_t cutoff_ms = 0;
    uint64_t progress_ms = 1000;
    uint32_t platform_index = 0;
    uint32_t device_index = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--challenge") && i + 1 < argc) challenge_hex = argv[++i];
        else if (!strcmp(argv[i], "--difficulty") && i + 1 < argc) difficulty_hex = argv[++i];
        else if (!strcmp(argv[i], "--batch-size") && i + 1 < argc) batch_size = (size_t)parse_u64(argv[++i]);
        else if (!strcmp(argv[i], "--local-size") && i + 1 < argc) local_size = (size_t)parse_u64(argv[++i]);
        else if (!strcmp(argv[i], "--start") && i + 1 < argc) start_nonce = parse_u64(argv[++i]);
        else if (!strcmp(argv[i], "--cutoff-ms") && i + 1 < argc) cutoff_ms = parse_u64(argv[++i]);
        else if (!strcmp(argv[i], "--progress-ms") && i + 1 < argc) progress_ms = parse_u64(argv[++i]);
        else if (!strcmp(argv[i], "--platform-index") && i + 1 < argc) platform_index = (uint32_t)parse_u64(argv[++i]);
        else if (!strcmp(argv[i], "--device-index") && i + 1 < argc) device_index = (uint32_t)parse_u64(argv[++i]);
    }

    if (!challenge_hex || !difficulty_hex) {
        fprintf(stderr, "usage: gpu-miner --challenge HEX --difficulty HEX [--start N] [--batch-size N] [--local-size N] [--cutoff-ms N] [--progress-ms N]\n");
        return 2;
    }

    if (strlen(challenge_hex) != 64 || strlen(difficulty_hex) != 64) {
        fprintf(stderr, "challenge and difficulty must be 64 hex chars\n");
        return 2;
    }

    uint8_t challenge[32], difficulty[32];
    hex_to_bytes(challenge_hex, challenge, 32);
    hex_to_bytes(difficulty_hex, difficulty, 32);

    // Pre-compute base state and difficulty words on host
    uint64_t base_state[25];
    uint32_t diff_words[8];
    prepare_base_state(challenge, base_state);
    prepare_diff_words(difficulty, diff_words);

    load_opencl();

    // Find platform
    cl_uint platform_count = 0;
    cl_int err = cl.clGetPlatformIDs(0, NULL, &platform_count);
    if (err != CL_SUCCESS || platform_count == 0) fatal_cl("clGetPlatformIDs", err ? err : -1);
    cl_platform_id *platforms = (cl_platform_id *)calloc(platform_count, sizeof(*platforms));
    err = cl.clGetPlatformIDs(platform_count, platforms, NULL);
    if (err != CL_SUCCESS) fatal_cl("clGetPlatformIDs(list)", err);
    if (platform_index >= platform_count) { fprintf(stderr, "platform-index out of range\n"); return 2; }
    cl_platform_id platform = platforms[platform_index];

    // Find device
    cl_uint device_count = 0;
    err = cl.clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &device_count);
    if (err != CL_SUCCESS || device_count == 0) { fprintf(stderr, "no OpenCL GPU found\n"); return 2; }
    cl_device_id *devices = (cl_device_id *)calloc(device_count, sizeof(*devices));
    err = cl.clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, device_count, devices, NULL);
    if (err != CL_SUCCESS) fatal_cl("clGetDeviceIDs(list)", err);
    if (device_index >= device_count) { fprintf(stderr, "device-index out of range\n"); return 2; }
    cl_device_id device = devices[device_index];

    char device_name[256] = {0};
    char device_vendor[256] = {0};
    size_t size = 0;
    cl.clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, &size);
    cl.clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(device_vendor), device_vendor, &size);

    size_t max_wg = 0;
    cl.clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_wg), &max_wg, NULL);
    if (local_size > max_wg && max_wg > 0) local_size = max_wg;
    if (batch_size % local_size != 0) batch_size = ((batch_size + local_size - 1) / local_size) * local_size;

    cl_context context = cl.clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) fatal_cl("clCreateContext", err);
    cl_command_queue queue = cl.clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) fatal_cl("clCreateCommandQueue", err);

    const char *sources[] = { kernel_source };
    cl_program program = cl.clCreateProgramWithSource(context, 1, sources, NULL, &err);
    if (err != CL_SUCCESS) fatal_cl("clCreateProgramWithSource", err);
    err = cl.clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size = 0;
        cl.clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *)calloc(log_size + 1, 1);
        if (log) {
            cl.clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
            fprintf(stderr, "Build log:\n%s\n", log);
            free(log);
        }
        fatal_cl("clBuildProgram", err);
    }

    cl_kernel kernel = cl.clCreateKernel(program, "mine", &err);
    if (err != CL_SUCCESS) fatal_cl("clCreateKernel", err);

    // Create __constant buffers for base_state (25 ulongs = 200 bytes) and diff_words (8 uints = 32 bytes)
    cl_mem base_state_buf = cl.clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 25 * sizeof(uint64_t), base_state, &err);
    if (err != CL_SUCCESS) fatal_cl("clCreateBuffer(base_state)", err);
    cl_mem diff_words_buf = cl.clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 8 * sizeof(uint32_t), diff_words, &err);
    if (err != CL_SUCCESS) fatal_cl("clCreateBuffer(diff_words)", err);
    cl_mem found_flag_buf = cl.clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_int), NULL, &err);
    if (err != CL_SUCCESS) fatal_cl("clCreateBuffer(flag)", err);
    cl_mem found_nonce_buf = cl.clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_ulong), NULL, &err);
    if (err != CL_SUCCESS) fatal_cl("clCreateBuffer(nonce)", err);

    // Set fixed kernel args
    err = cl.clSetKernelArg(kernel, 0, sizeof(base_state_buf), &base_state_buf);
    err |= cl.clSetKernelArg(kernel, 1, sizeof(diff_words_buf), &diff_words_buf);
    err |= cl.clSetKernelArg(kernel, 4, sizeof(found_flag_buf), &found_flag_buf);
    err |= cl.clSetKernelArg(kernel, 5, sizeof(found_nonce_buf), &found_nonce_buf);
    if (err != CL_SUCCESS) fatal_cl("clSetKernelArg(fixed)", err);

    size_t global_size = batch_size;
    if (global_size < local_size) global_size = local_size;
    if (global_size % local_size != 0)
        global_size = ((global_size + local_size - 1) / local_size) * local_size;

    // Target batch hashes for ~8 seconds on RTX 3060 (~40 MH/s keccak256)
    uint64_t target_batch_hashes = 320000000ULL;
    uint64_t iter_per_thread = target_batch_hashes / (uint64_t)global_size;
    if (iter_per_thread < 1) iter_per_thread = 1;
    uint64_t batch_hashes = (uint64_t)global_size * iter_per_thread;

    uint64_t started = now_ms();
    uint64_t last_progress = started;
    uint64_t next_nonce = start_nonce;
    uint64_t total_hashes = 0;
    uint8_t found = 0;
    uint64_t found_nonce = 0;

    while (1) {
        if (cutoff_ms && now_ms() >= cutoff_ms) {
            printf("{\"type\":\"expired\",\"hashes\":\"%" PRIu64 "\"}\n", total_hashes);
            fflush(stdout);
            break;
        }

        cl_int flag = 0;
        cl_ulong nonce_result = 0;

        err = cl.clEnqueueWriteBuffer(queue, found_flag_buf, CL_TRUE, 0, sizeof(flag), &flag, 0, NULL, NULL);
        err |= cl.clEnqueueWriteBuffer(queue, found_nonce_buf, CL_TRUE, 0, sizeof(nonce_result), &nonce_result, 0, NULL, NULL);
        if (err != CL_SUCCESS) fatal_cl("clEnqueueWriteBuffer(reset)", err);

        // Set dynamic args: start_nonce (arg 2), iter_per_thread (arg 3)
        err = cl.clSetKernelArg(kernel, 2, sizeof(next_nonce), &next_nonce);
        err |= cl.clSetKernelArg(kernel, 3, sizeof(iter_per_thread), &iter_per_thread);
        if (err != CL_SUCCESS) fatal_cl("clSetKernelArg(dynamic)", err);

        err = cl.clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
        if (err != CL_SUCCESS) fatal_cl("clEnqueueNDRangeKernel", err);

        err = cl.clFinish(queue);
        if (err != CL_SUCCESS) fatal_cl("clFinish", err);

        total_hashes += batch_hashes;

        err = cl.clEnqueueReadBuffer(queue, found_flag_buf, CL_TRUE, 0, sizeof(flag), &flag, 0, NULL, NULL);
        if (err != CL_SUCCESS) fatal_cl("clEnqueueReadBuffer(flag)", err);

        if (flag) {
            err = cl.clEnqueueReadBuffer(queue, found_nonce_buf, CL_TRUE, 0, sizeof(found_nonce), &found_nonce, 0, NULL, NULL);
            if (err != CL_SUCCESS) fatal_cl("clEnqueueReadBuffer(nonce)", err);
            found = 1;
            break;
        }

        next_nonce += batch_hashes;

        uint64_t now = now_ms();
        if (now - last_progress >= progress_ms) {
            uint64_t elapsed_ms = now_ms() - started;
            printf("{\"type\":\"progress\",\"hashes\":\"%" PRIu64 "\",\"nonce\":\"%" PRIu64 "\",\"elapsed_ms\":\"%" PRIu64 "\",\"device\":\"%s / %s\",\"batch_size\":\"%zu\",\"local_size\":\"%zu\"}\n",
                   total_hashes, next_nonce, elapsed_ms, device_vendor, device_name, global_size, local_size);
            fflush(stdout);
            last_progress = now;
        }
    }

    if (found) {
        printf("{\"type\":\"found\",\"solution_nonce\":\"%" PRIu64 "\",\"hashes\":\"%" PRIu64 "\",\"device\":\"%s / %s\"}\n",
               found_nonce, total_hashes, device_vendor, device_name);
        fflush(stdout);
    }

    cl.clReleaseMemObject(found_nonce_buf);
    cl.clReleaseMemObject(found_flag_buf);
    cl.clReleaseMemObject(diff_words_buf);
    cl.clReleaseMemObject(base_state_buf);
    cl.clReleaseKernel(kernel);
    cl.clReleaseProgram(program);
    cl.clReleaseCommandQueue(queue);
    cl.clReleaseContext(context);
    free(devices);
    free(platforms);
    return found ? 0 : 1;
}
