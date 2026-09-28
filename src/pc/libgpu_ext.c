// Replacements for PSY-Z libgpu functions that are wrong on 64-bit hosts,
// routed in with the linker's --wrap.

#include <libgpu.h>

int get_tim_addr(unsigned int* addr, TIM_IMAGE* img);

// PSY-Z advances with u_long* arithmetic, doubling the stride on 64-bit, so a
// chain of TIMs skips every other one. TIM lengths are in 32-bit words.
static unsigned int* s_Tim;

int __wrap_OpenTIM(u_long* addr) {
    s_Tim = (unsigned int*)addr;
    return 0;
}

TIM_IMAGE* __wrap_ReadTIM(TIM_IMAGE* img) {
    int len = get_tim_addr(s_Tim, img);

    if (len == -1) {
        return NULL;
    }
    s_Tim += len;
    return img;
}
