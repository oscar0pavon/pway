#define _GNU_SOURCE
#include "shm.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "pway.h"
#include "wayland.h"

/* Two buffers: while one is attached and waiting on the compositor's release
 * event, the other is free to draw into. That release is the only throttle
 * this backend has - there is no frame callback - so it also caps how many
 * frames the app can ever get ahead of the compositor. */
#define SHM_BUFFER_COUNT 2

typedef struct ShmBuffer{
  struct wl_buffer *buffer;
  uint32_t *data;
  bool busy;
}ShmBuffer;

static ShmBuffer shm_buffers[SHM_BUFFER_COUNT];
static int shm_fd = -1;
static uint8_t *pool_data = NULL;
static size_t pool_size = 0;
static struct wl_shm_pool *shm_pool = NULL;
static int shm_width, shm_height, shm_stride;

/* Index into shm_buffers of the buffer handed out by the last shm_get_buffer()
 * call, until shm_commit() consumes it. -1 means nothing is checked out. */
static int current_buffer = -1;

static int create_anonymous_file(size_t size){
  int fd = memfd_create("pway-shm", MFD_CLOEXEC);
  if(fd < 0){
    printf("Can't create anonymous shm file: %s\n", strerror(errno));
    return -1;
  }

  if(ftruncate(fd, size) < 0){
    printf("Can't size shm file: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  return fd;
}

static void buffer_release(void *data, struct wl_buffer *wl_buffer){
  for(int i = 0; i < SHM_BUFFER_COUNT; i++){
    if(shm_buffers[i].buffer == wl_buffer){
      shm_buffers[i].busy = false;
      return;
    }
  }
}

static const struct wl_buffer_listener buffer_listener = {
  .release = buffer_release
};

/* The surface is fully opaque XRGB8888 output, so telling the compositor
 * that up front lets it skip blending pway's window against whatever is
 * behind it. */
static void set_opaque_region(int width, int height){
  struct wl_region *region = wl_compositor_create_region(wayland.compositor);
  wl_region_add(region, 0, 0, width, height);
  wl_surface_set_opaque_region(pway_surface, region);
  wl_region_destroy(region);
}

static void destroy_pool(){
  for(int i = 0; i < SHM_BUFFER_COUNT; i++){
    if(shm_buffers[i].buffer){
      wl_buffer_destroy(shm_buffers[i].buffer);
      shm_buffers[i].buffer = NULL;
      shm_buffers[i].data = NULL;
      shm_buffers[i].busy = false;
    }
  }

  if(shm_pool){
    wl_shm_pool_destroy(shm_pool);
    shm_pool = NULL;
  }

  if(pool_data){
    munmap(pool_data, pool_size);
    pool_data = NULL;
  }

  if(shm_fd >= 0){
    close(shm_fd);
    shm_fd = -1;
  }

  current_buffer = -1;
}

static bool create_pool(int width, int height){
  shm_stride = width * 4;
  size_t buffer_size = (size_t)shm_stride * (size_t)height;
  pool_size = buffer_size * SHM_BUFFER_COUNT;

  shm_fd = create_anonymous_file(pool_size);
  if(shm_fd < 0)
    return false;

  pool_data = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if(pool_data == MAP_FAILED){
    printf("Can't mmap shm pool: %s\n", strerror(errno));
    close(shm_fd);
    shm_fd = -1;
    pool_data = NULL;
    return false;
  }

  shm_pool = wl_shm_create_pool(wayland.shm, shm_fd, pool_size);

  for(int i = 0; i < SHM_BUFFER_COUNT; i++){
    off_t offset = (off_t)buffer_size * i;

    shm_buffers[i].buffer = wl_shm_pool_create_buffer(shm_pool, offset,
        width, height, shm_stride, WL_SHM_FORMAT_XRGB8888);
    shm_buffers[i].data = (uint32_t*)(pool_data + offset);
    shm_buffers[i].busy = false;

    wl_buffer_add_listener(shm_buffers[i].buffer, &buffer_listener, NULL);
  }

  shm_width = width;
  shm_height = height;

  return true;
}

void init_shm(){
  create_pool(pway->width, pway->height);
  set_opaque_region(pway->width, pway->height);
}

/* Called from the same place pway_egl_resize() would be - deferred to the
 * top of the app's draw(), so no buffer from the old pool is still checked
 * out via shm_get_buffer() when this runs. */
void shm_resize(int width, int height){
  if(width == shm_width && height == shm_height)
    return;

  destroy_pool();
  create_pool(width, height);
  set_opaque_region(width, height);
}

uint32_t* shm_get_buffer(int *stride){
  if(current_buffer != -1){
    *stride = shm_stride;
    return shm_buffers[current_buffer].data;
  }

  for(int i = 0; i < SHM_BUFFER_COUNT; i++){
    if(!shm_buffers[i].busy){
      current_buffer = i;
      *stride = shm_stride;
      return shm_buffers[i].data;
    }
  }

  /* Both buffers are still owned by the compositor. The caller should leave
   * its redraw flag set and retry after the next round of event dispatch
   * has had a chance to process a release event. */
  return NULL;
}

void shm_commit(int x, int y, int width, int height){
  if(current_buffer == -1)
    return;

  wl_surface_attach(pway_surface, shm_buffers[current_buffer].buffer, 0, 0);
  wl_surface_damage_buffer(pway_surface, x, y, width, height);
  wl_surface_commit(pway_surface);

  shm_buffers[current_buffer].busy = true;
  current_buffer = -1;
}
