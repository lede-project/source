
#include "pcie_dbg.h"

int pcie_hexdump(char *name, char *buf, int len)
{
	int i, count;
	unsigned int *p;

	count = len / 32;
	count += 1;
	PCIE_INFO("hexdump %s hex(len=%d):\n", name, len);
	for (i = 0; i < count; i++) {
		p = (unsigned int *)(buf + i * 32);
		PCIE_INFO("mem[0x%04x] 0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,"
			  "0x%08x,0x%08x,0x%08x,\n",
			  i * 32, p[0], p[1], p[2], p[3], p[4], p[5],
			  p[6], p[7]);
	}

	return 0;
}
