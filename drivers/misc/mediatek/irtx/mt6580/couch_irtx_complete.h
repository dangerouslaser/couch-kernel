/* SPDX-License-Identifier: GPL-2.0 */
#ifndef COUCH_IRTX_COMPLETE_H
#define COUCH_IRTX_COMPLETE_H

/* A retained count from an earlier transfer is not completion evidence.
 * Require a reset observation and enough time for all padded DMA samples. */
static inline bool couch_irtx_complete(u32 sent, bool *saw_zero,
				       s64 elapsed_us, u32 expected_us)
{
	if (!sent)
		*saw_zero = true;
	return *saw_zero && sent == 1 && elapsed_us >= expected_us;
}
/* Keep the first valid observation; later minimum-duration waiting must not
 * overwrite the measurement of hardware completion. */
static inline s64 couch_irtx_first_complete(u32 sent, bool saw_zero,
					  s64 first_us, s64 elapsed_us)
{
	return sent == 1 && saw_zero && first_us < 0 ? elapsed_us : first_us;
}
#endif
