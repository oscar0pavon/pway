#ifndef SHM_H
#define SHM_H

#include <stdint.h>

/* CPU-rendering counterpart to egl.h. init_shm() allocates the buffer pool at
 * pway->width/height and must be called instead of init_egl(), never both -
 * they are alternative ways to get pixels onto pway_surface. */
void init_shm();

void shm_resize(int width, int height);

/* Returns a pointer to a free buffer's pixel memory (packed 0xXXRRGGBB rows,
 * left-to-right, top-to-bottom) and writes its stride in bytes to *stride.
 * Returns NULL if every buffer in the pool is still owned by the compositor -
 * the caller must try again on a later loop iteration once a release event
 * has been processed, not block waiting for one. */
uint32_t* shm_get_buffer(int *stride);

/* Attaches the buffer last handed out by shm_get_buffer(), marks only the
 * given rect damaged, and commits. x/y/width/height are in buffer
 * (pixel) coordinates. */
void shm_commit(int x, int y, int width, int height);

#endif
