#ifndef __SLP_SDIO_H__
#define __SLP_SDIO_H__
#include "sdio_int.h"
#include "wcn_glb.h"

union CP_SLP_CTL_REG {
	unsigned char reg;
	struct {
		unsigned char cp_slp_ctl:1;  /* 0:wakeup, 1:sleep */
		unsigned char rsvd:7;
	} bit;
};

static inline
int ap_wakeup_cp(void)
{
	return sprdwcn_bus_aon_writeb(REG_CP_SLP_CTL, 0);
}
int slp_allow_sleep(void);
int slp_pub_int_RegCb(void);

#endif
