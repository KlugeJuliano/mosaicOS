#include <l4/sys/ipc.h>
#include <l4/sys/utcb.h>
#include <l4/re/c/video/goos.h>
#include <l4/re/c/rm.h>
#include <l4/re/env.h>

l4_utcb_t *rust_l4_utcb(void) {
    return l4_utcb();
}

l4_msgtag_t rust_l4_ipc_reply_and_wait(l4_utcb_t *utcb, l4_msgtag_t tag,
                                        l4_umword_t *label, l4_timeout_t timeout) {
    return l4_ipc_reply_and_wait(utcb, tag, label, timeout);
}

l4_msgtag_t rust_l4_ipc_call(l4_cap_idx_t object, l4_utcb_t *utcb, l4_msgtag_t tag,
                              l4_timeout_t timeout) {
    return l4_ipc_call(object, utcb, tag, timeout);
}

int rust_l4re_video_goos_info(l4re_video_goos_t goos,
                              l4re_video_goos_info_t *ginfo) {
    return l4re_video_goos_info(goos, ginfo);
}

int rust_l4re_video_goos_refresh(l4re_video_goos_t goos, int x, int y, int w, int h) {
    return l4re_video_goos_refresh(goos, x, y, w, h);
}

int rust_l4re_rm_attach(void **start, unsigned long size, l4re_rm_flags_t flags,
                         l4re_ds_t mem, l4re_rm_offset_t offs,
                         unsigned char align) {
    return l4re_rm_attach(start, size, flags, mem, offs, align);
}

L4_CV l4_cap_idx_t rust_l4re_env_get_cap(const char *name) {
    return l4re_env_get_cap(name);
}