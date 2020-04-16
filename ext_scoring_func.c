#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "decoders.h"

#define FOUND    0
#define NO_FOUND -1
#define PROB      0
#define BACK_PROB 1

typedef struct {
	float prob;
	float back_prob;
	unsigned char str[N_GRAMS];
}INFO_GRAMS;

static int match_lable(LM_DATA lm_data, int n_grams, const unsigned char *lable, int state, float *prob, unsigned char unk)
{
    int n = 0, i = 0, flag = 0;
    float back_prob = 0.0, prev_prob = 0.0, unk_prob = 0.0;
    unsigned char grams_lable[N_GRAMS];
    INFO_GRAMS info_grams;

    int start_addr = 0;
    for (i = 0; i < n_grams - 1; i++) {
        start_addr += lm_data.grams_len_per[i] * (8 + (i + 1));
    }
    while(lm_data.grams_len_per[n_grams-1]) {
        memcpy(&info_grams.prob, lm_data.data_buffer + start_addr, sizeof(float));
        start_addr += sizeof(float);
//        memcpy(info_grams.str, lm_data.data_buffer + start_addr, n_grams);
		strncpy(info_grams.str, lm_data.data_buffer + start_addr, n_grams);
        start_addr += n_grams;
        memcpy(&info_grams.back_prob, lm_data.data_buffer + start_addr, sizeof(float));
        start_addr += sizeof(float);

        if (info_grams.str[0] == unk) {
            unk_prob = info_grams.prob;
        }
        int cnt = 0;
        for (i = 0; i < n_grams; i++) {
            if (info_grams.str[i] == lable[i]){
                cnt++;
            }
        }
        if (state == PROB) {
            if (cnt == n_grams) {
                flag = 1;
                *prob = info_grams.prob;
                return FOUND;
            }
        } else if (state == BACK_PROB) {
            if (cnt == n_grams) {
                flag = 1;
                *prob = info_grams.back_prob;
                return FOUND;
            }
        }
        lm_data.grams_len_per[n_grams-1]--;
    }
    if (flag == 0 && state == PROB) {
        if (n_grams == 1) {
            *prob = unk_prob;
            return FOUND;
        } else {
            return NO_FOUND;
        }

    } else if (flag == 0 && state == BACK_PROB) {
        return NO_FOUND;
    }
}

float ext_scoring_func(LM_DATA lm_data, int n_grams, const unsigned char *lable, unsigned char unk)
{
	float tmp_prob, tmp_back_prob;

    if ( n_grams == 0)
    {
        printf("error\n");
        return(0);
    }

    if (match_lable(lm_data, n_grams, lable, PROB, &tmp_prob, unk) == FOUND) {
        return(tmp_prob);
    }

    tmp_prob =
    ((match_lable(lm_data, n_grams - 1, lable, BACK_PROB, &tmp_back_prob, unk) == FOUND) ? tmp_back_prob : 1.0 )
    * ext_scoring_func(lm_data, n_grams - 1, lable + 1, unk);

    return tmp_prob;
}
