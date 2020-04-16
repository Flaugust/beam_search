#ifndef __EXT_SCORING_FUNC_H__
#define __EXT_SCORING_FUNC_H__

#include "decoders.h"

typedef struct {
	int grams_len_per[N_GRAMS];
	unsigned char *data_buffer;
	int data_size;
}LM_DATA;

extern int ReadLmData(FILE *fp, LM_DATA *lm_data);
extern float ext_scoring_func(LM_DATA lm_data, int n_grams, const unsigned char *lable, unsigned char unk);

#endif
