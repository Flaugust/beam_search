#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ext_scoring_func.h"
#include "decoders.h"
#include "config.h"

static inline float fastlogf2 (float x)
{
    union { float f; unsigned int i; } vx = { x };
    union { unsigned int i; float f; } mx = { (vx.i & 0x007FFFFF) | (0x7e << 23) };
    float y = vx.i;
    y *= 1.0f / (1 << 23);
    return y - 124.22544637f - 1.498030302f * mx.f - 1.72587999f / (0.3520887068f + mx.f);
}

static inline float fastlogf (float x)
{
    return 0.69314718f * fastlogf2 (x);
}

static int candidatas_same_merged(unsigned char **string, float *probs_nb, float *probs_b, int len, float *probs)
{
	int i = 0;
	int j = 0;
	for (i = 0; i < len - 1; i++) {
		for (j = i + 1; j < len;) {
			if (0 == strcmp(string[i], string[j])) {
				probs_b[i] += probs_b[j];
				probs_nb[i] += probs_nb[j];
				for (int k = j; k < len - 1; k++) {
					strcpy(string[k], string[k + 1]);
					probs_b[k] = probs_b[k + 1];
					probs_nb[k] = probs_nb[k + 1];
				}
				len--;
			} else {
				j++;
			}
			probs[i] = probs_b[i] + probs_nb[i];
		}
	}
	return len;
}

static int sequence_new(float *new_probs, unsigned char *new_index, float *probs, char *probs_idx, int probs_len, float prune)
{
	int i = 0;
	int cutoff_len = 0;
	for (i = 0; i < probs_len; i++) {
		if (probs[i] >= prune) {
			new_probs[cutoff_len] = probs[i]; //probs_cutoff：存放大于prun
			new_index[cutoff_len] = probs_idx[i];  //probs_idx:存放大于prune的概率值对应的索引值
			cutoff_len++; //cutoff_len：统计大于prune的个数
        }
    }
	return cutoff_len;
}

static void candidatas_sorted(float *probs, unsigned char **string, float *probs_b, float *probs_nb, int len)
{
	float tmp_prob = 0.f;
    char *tmp_char;
	int tmp_idx = 0;
	float tmp_probs_b = 0.f;
	float tmp_probs_nb = 0.f;
	for (int i = 0; i < len - 1; i++) {
	    for (int j = 1 + i; j < len; j++) {
	        if (probs[i] < probs[j]) {
	            tmp_prob = probs[i];
	            probs[i] = probs[j];
	            probs[j] = tmp_prob;
	            tmp_char = string[i];
	            string[i] = string[j];
	            string[j] = tmp_char;
				tmp_probs_b = probs_b[i];
				probs_b[i] = probs_b[j];
				probs_b[j] = tmp_probs_b;
				tmp_probs_nb = probs_nb[i];
				probs_nb[i] = probs_nb[j];
				probs_nb[j] = tmp_probs_nb;
	        }
	    }
	}
}

static void sorted(float *probs, unsigned char *probs_idx, int probs_len)
{
	float tmp_prob = 0.f;
	int tmp_idx = 0;
	for (int i = 0; i < probs_len - 1; i++) {
		for (int j = 1 + i; j < probs_len; j++) {
			if (probs[i] < probs[j]) {
				tmp_prob = probs[i];
				probs[i] = probs[j];
				probs[j] = tmp_prob;
				tmp_idx = probs_idx[i];
				probs_idx[i] = probs_idx[j];
				probs_idx[j] = tmp_idx;
			}
		}
	}
}


float *ctc_beam_search_decoder(FILE           *fp,
							   unsigned char  **prefix_set_prev,
                               float          *probs_seq,
							   int            probs_len,
							   int            T,
							   int            blank_id,
							   float          prune)
{
    int i = 0;
    int j = 0;
    int index = 0;
	int prefix_len = 0;
	float score = 0.0;

	unsigned char lable[N_GRAMS];

	float probs_set_next[BEAM_SIZE * (PROBS_LEN + 1)];
	for(i = 0; i < BEAM_SIZE * (PROBS_LEN + 1); i++) {
		probs_set_next[i] = 1.0;
	}
	float probs_set_prev[BEAM_SIZE] = {0.f};

	int idx_set_next[BEAM_SIZE * (PROBS_LEN + 1)] = {0};
	int idx_set_prev[BEAM_SIZE] = {0};

	unsigned char *prefix_set_next[BEAM_SIZE * (PROBS_LEN + 1)];
	for (i = 0; i < BEAM_SIZE * (PROBS_LEN + 1); i++) {
		prefix_set_next[i] = (unsigned char *)malloc(PREFIX_CHAR_LENGTH * sizeof(char));
	}

	float probs_b_prev[BEAM_SIZE * (PROBS_LEN + 1)];
	for(i = 0; i < BEAM_SIZE * (PROBS_LEN + 1); i++) {
		probs_b_prev[i] = 1.0;
	};
	float probs_nb_prev[BEAM_SIZE * (PROBS_LEN + 1)] = {0.0};

	unsigned char *l;
	l = (unsigned char *)malloc(PREFIX_CHAR_LENGTH * sizeof(char));

    unsigned char start_character = UNK + 2;

    for (int t = 0; t < T; t++) {

		float probs_b_cur[PROBS_LEN * (BEAM_SIZE + 1)];
		memset(probs_b_cur, 0.0, BEAM_SIZE * (PROBS_LEN + 1) * sizeof(float));
		float probs_nb_cur[PROBS_LEN*BEAM_SIZE + PROBS_LEN];
		memset(probs_nb_cur, 0.0, BEAM_SIZE * (PROBS_LEN + 1) * sizeof(float));
        int cutoff_len = 0;
		unsigned char probs_idx[PROBS_LEN];
		for (i = 1; i < PROBS_LEN + 1; i++) {
			probs_idx[i - 1] = i;
		}
		float probs_seq_cutoff[PROBS_LEN] = {0.f};
		unsigned char probs_idx_cutoff[PROBS_LEN] = {0};

		for (i = 0; i < BEAM_SIZE * (PROBS_LEN + 1); i++) {
			memset(prefix_set_next[i], 0, PREFIX_CHAR_LENGTH * sizeof(char));
		}

		sorted(probs_seq + t * probs_len, probs_idx, probs_len);

		cutoff_len = sequence_new(probs_seq_cutoff, probs_idx_cutoff, probs_seq + t * probs_len, probs_idx, probs_len, prune);

		if (t == 0)
			prefix_len = 1;
		else
			prefix_len = BEAM_SIZE;

		int cnt = 0;
        for (i = 0; i < prefix_len; i++) {

			memset(l, 0, PREFIX_CHAR_LENGTH * sizeof(char));
			strcpy(l, prefix_set_prev[i]);
			int l_len = strlen(l);

            for (index = 0; index < cutoff_len; index++) {

				unsigned char c_idx[2];
                c_idx[0] = probs_idx_cutoff[index];
				c_idx[1] = 0;
                float prob_c = probs_seq_cutoff[index];
                if (c_idx[0] == blank_id) {
					probs_b_cur[i * cutoff_len + index + cnt] = prob_c * (probs_b_prev[i] +
							probs_nb_prev[i]);
					strcpy(prefix_set_next[i * cutoff_len + index + cnt], l);
                } else {
                    unsigned char l_plus[S_LEN];
                    memset(l_plus, '\0', sizeof(l_plus));

					strcpy(l_plus, l);
					strcat(l_plus, c_idx);

					int lp_len = strlen(l_plus);
					unsigned char last_char[2];
					if (l_len > 0) {
						last_char[0] = l[l_len - 1];
					} else {
						last_char[0] = l[0];
					}
					last_char[1] = 0;

					if (0 == strcmp(c_idx, last_char)) {

						if (probs_b_prev[i] != 0 && probs_nb_prev[i] != 0) {
							strcpy(prefix_set_next[i * cutoff_len + index + cnt], l_plus);
							probs_nb_cur[i * cutoff_len + index + cnt] = prob_c * probs_b_prev[i];
							cnt++;
							strcpy(prefix_set_next[i * cutoff_len + index + cnt], l);
							probs_nb_cur[i * cutoff_len + index + cnt] = prob_c * probs_nb_prev[i];
						} else if (probs_nb_prev[i] != 0 && probs_b_prev[i] == 0) {
							strcpy(prefix_set_next[i * cutoff_len + index + cnt], l);
							probs_nb_cur[i * cutoff_len + index + cnt] = prob_c * probs_nb_prev[i];
						} else if (probs_nb_prev[i] == 0 && probs_b_prev[i] != 0) {
							strcpy(prefix_set_next[i * cutoff_len + index + cnt], l_plus);
							probs_nb_cur[i * cutoff_len + index + cnt] = prob_c * probs_b_prev[i];
						}
					} else {
                        strcpy(prefix_set_next[i * cutoff_len + index + cnt], l_plus);

						memset(lable, 0, N_GRAMS * sizeof(char));
						if (lp_len > 0) {
							if (lp_len < N_GRAMS) {
								lable[0] = start_character;
								memcpy(lable + 1, l_plus, lp_len);
								score = ext_scoring_func(fp, (lp_len + 1), lable, UNK) * lp_len;
							} else {
								memcpy(lable, l_plus + (lp_len - N_GRAMS), N_GRAMS);
								score = ext_scoring_func(fp, N_GRAMS, lable, UNK) * lp_len;
							}
						} else {
							score = 1.0;
						}
						probs_nb_cur[i * cutoff_len + index + cnt] = score * prob_c * (probs_b_prev[i] + probs_nb_prev[i]);
                    }
                }
            }
        }
		int len = prefix_len * cutoff_len + cnt;

		/*遍历重复字符串，将重复字符串进行合并，合并位置选择最小索引值处，后面字符串依次前移，
		并根据上一次是否为blank修改probs_nb_prev（将重复字符串对应索引值非blank结尾的概率相加）
		及probs_b_prev（重复字符串对应索引值blank结尾的概率值相加）的值。*/
		memset(probs_set_next, 0.0, len*sizeof(float));
		int new_len = candidatas_same_merged(prefix_set_next, probs_nb_cur, probs_b_cur, len, probs_set_next);

		candidatas_sorted(probs_set_next, prefix_set_next, probs_b_cur, probs_nb_cur, len);

		//根据概率大小进行排序，前缀字符串，索引值等跟随概率排序进行变换位置

		memset(probs_nb_prev, 0, BEAM_SIZE);
		memset(probs_b_prev, 0, BEAM_SIZE);

		for (i = 0; i < BEAM_SIZE; i++) {
			strcpy(prefix_set_prev[i], prefix_set_next[i]);
			probs_nb_prev[i] = probs_nb_cur[i];
			probs_b_prev[i] = probs_b_cur[i];
			probs_set_prev[i] = probs_set_next[i];
			idx_set_prev[i] = idx_set_next[i];
		}
    }
	static float beam_result[BEAM_SIZE] = {0.f};

	for (i = 0; i < BEAM_SIZE; i++) {
		if (probs_set_prev[i] > 0.0 && strlen(prefix_set_prev[i]) >= 1) {
			beam_result[i] = fastlogf(probs_set_prev[i]);
		} else {
			beam_result[i] = -INFINITY;
		}
	}
	free(l);
	for (i = 0; i < BEAM_SIZE * PROBS_LEN; i++) {
		free(prefix_set_next[i]);
	}
    return beam_result;
}
