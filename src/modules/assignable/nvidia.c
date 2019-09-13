#include "/opt/cuda/include/nvml.h"

#define MAX_GPUS 32

static nvmlDevice_t nvml_handles[MAX_GPUS];
