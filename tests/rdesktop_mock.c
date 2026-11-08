#include <cgreen/mocks.h>
#include "../rdesktop.h"

void rd_create_ui(void)
{
  mock();
}


void generate_random(uint8 * random)
{
  mock(random);
}

RD_BOOL g_pending_resize_defer;
uint32 vc_chunk_size = 1600; /* Default virtual-channel fragment size. */

RD_BOOL rd_certcache_mkdir(void)
{
  return mock();
}

void hexdump(unsigned char *p, unsigned int len)
{
  mock(p, len);
}
