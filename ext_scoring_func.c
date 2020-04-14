#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

#define FOUND    0
#define NO_FOUND -1
#define PROB      0
#define BACK_PROB 1

struct {
	int context_len;
	int start;
}info_grams[10];

static int match_lable(FILE *fp, int n_grams, const unsigned char *lable, int state, float *prob, unsigned char unk)
{
    int n = 0, i = 0, flag = 0;
    float back_prob = 0.0, prev_prob = 0.0, unk_prob = 0.0;
    unsigned char *grams_lable, tmp;

    grams_lable = (unsigned char *)malloc(n_grams);    VALIDATE_PTR(grams_lable);

    fseek(fp, 0, SEEK_SET);
    fread(&n, 4, 1, fp);
    int num = n_grams;
    int k = 0;
    while(num) {
        fread(&info_grams[k].context_len, 4, 1, fp);
        num--;
        k++;
    }
    int start = (n + 1) * 4;
    for (i = 0; i < n_grams - 1; i++) {
        start += info_grams[i].context_len * (8 + (i + 1));
    }
    fseek(fp, start, SEEK_SET);
    while(info_grams[n_grams-1].context_len) {
        fread(&prev_prob, 4, 1, fp);
        for (i = 0; i < n_grams; i++) {
            fread(&grams_lable[i], 1, 1, fp);
        }
        fread(&back_prob, 4, 1, fp);
        if (grams_lable[0] == unk) {
            unk_prob = prev_prob;
        }
        int cnt = 0;
        for (i = 0; i < n_grams; i++) {
            if (grams_lable[i] == lable[i]){
                cnt++;
            }
        }
        if (state == PROB) {
            if (cnt == n_grams) {
                flag = 1;
                *prob = prev_prob;
                free(grams_lable);
                return FOUND;
            }
        } else if (state == BACK_PROB) {
            if (cnt == n_grams) {
                flag = 1;
                *prob = back_prob;
                free(grams_lable);
                return FOUND;
            }
        }
        info_grams[n_grams-1].context_len--;
    }
    if (flag == 0 && state == PROB) {
        if (n_grams == 1) {
            *prob = unk_prob;
            free(grams_lable);
            return FOUND;
        } else {
            free(grams_lable);
            return NO_FOUND;
        }

    } else if (flag == 0 && state == BACK_PROB) {
        free(grams_lable);
        return NO_FOUND;
    }
}

float ext_scoring_func(FILE *fp, int n_grams, const unsigned char *lable, unsigned char unk)
{
	float tmp_prob, tmp_back_prob;

    if ( n_grams == 0)
    {
        printf("error\n");
        return(0);
    }

    if (match_lable(fp, n_grams, lable, PROB, &tmp_prob, unk) == FOUND) {
        return(tmp_prob);
    }

    tmp_prob =
    ((match_lable(fp, n_grams - 1, lable, BACK_PROB, &tmp_back_prob, unk) == FOUND) ? tmp_back_prob : 1.0 )
    * ext_scoring_func(fp, n_grams - 1, lable + 1, unk);

    return tmp_prob;
}
