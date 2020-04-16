#ifndef __DECODERS_H__
#define __DECODERS_H__

#include "config.h"

typedef struct {
	int grams_len_per[N_GRAMS];
	unsigned char *data_buffer;
	int data_size;
}LM_DATA;

extern float *ctc_beam_search_decoder(FILE *fp, unsigned char **prefix_set_prev, float *probs_seq, int probs_len, int T, int blank_id, float prune);

#endif

