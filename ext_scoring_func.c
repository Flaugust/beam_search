#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "decoders.h"
#include "ext_scoring_func.h"

#define FOUND    0
#define NO_FOUND -1
#define PROB      0
#define BACK_PROB 1

typedef struct {
    float prob;
    float back_prob;
    unsigned char *str;
}INFO_GRAMS;

int ReadLmData(FILE *fp, LM_DATA *lm_data, float *unk_prob)
{
    fseek(fp, 4, SEEK_SET);
    int num = N_GRAMS;
    int i = 0;
    int len[N_GRAMS];
    lm_data->data_size = 0;
    for (i = 0; i < N_GRAMS; i++) {
        fread(&(lm_data->grams_len_per[i]), 4, 1, fp);
        lm_data->data_size += lm_data->grams_len_per[i] * 12;    //2个float,1个char(4字节补齐)，grams_len_per是n_grams中的条数
    }
    lm_data->data_buffer = (unsigned char *)malloc(lm_data->data_size);
    fread(lm_data->data_buffer, sizeof(char), lm_data->data_size, fp);

    *unk_prob = *(float *)(lm_data->data_buffer + 24);
    return 0;
}

static int match_lable(LM_DATA lm_data, int n_grams, const unsigned char *lable, int state, float *prob, float unk_prob)
{
    int n = 0, i = 0, flag = 0;
    unsigned char grams_lable[N_GRAMS];
    INFO_GRAMS info_grams;
    unsigned char *start_addr = lm_data.data_buffer;

    for (i = 0; i < n_grams - 1; i++) {
        start_addr += lm_data.grams_len_per[i] * 12;  //2个float,1个char(4字节补齐)
    }
    while(lm_data.grams_len_per[n_grams-1]) {
        info_grams.prob = *(float *)start_addr;
        start_addr += sizeof(float);
        info_grams.back_prob = *(float *)start_addr;
        start_addr += sizeof(float);
        info_grams.str = start_addr;
        start_addr += 4;

		if (info_grams.str[0] == 65) {
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

float ext_scoring_func(LM_DATA lm_data, int n_grams, const unsigned char *lable, float unk_prob)
{
    float tmp_prob, tmp_back_prob;

    if ( n_grams == 0)
    {
        printf("error\n");
        return(0);
    }

    if (match_lable(lm_data, n_grams, lable, PROB, &tmp_prob, unk_prob) == FOUND) {
        return(tmp_prob);
    }

    tmp_prob =
    ((match_lable(lm_data, n_grams - 1, lable, BACK_PROB, &tmp_back_prob, unk_prob) == FOUND) ? tmp_back_prob : 1.0 )
    * ext_scoring_func(lm_data, n_grams - 1, lable + 1, unk_prob);

    return tmp_prob;
}
