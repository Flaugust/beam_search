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

static int candidatas_same_merged(CANDIDATES candidates[], int len)
{
	int i = 0;
	int j = 0;
	for (i = 0; i < len - 1; i++) {
		for (j = i + 1; j < len;) {
			if (0 == strcmp(candidates[i].prefix_set_next, candidates[j].prefix_set_next)) {
				candidates[i].probs_b_cur += candidates[j].probs_b_cur;
				candidates[i].probs_nb_cur += candidates[j].probs_nb_cur;
				for (int k = j; k < len - 1; k++) {
					strcpy(candidates[k].prefix_set_next, candidates[k + 1].prefix_set_next);
					candidates[k].probs_b_cur = candidates[k + 1].probs_b_cur;
					candidates[k].probs_nb_cur = candidates[k + 1].probs_nb_cur;
				}
				len--;
			} else {
				j++;
			}
			candidates[i].probs_set_next = candidates[i].probs_b_cur + candidates[i].probs_nb_cur;
		}
	}
	return len;
}

static int candidatas_sorted(CANDIDATES candidates[], int len)
{
	float tmp_prob = 0.f;
    char *tmp_char;
	tmp_char = (char *)malloc(len);                  VALIDATE_PTR(tmp_char);
	int tmp_idx = 0;
	float tmp_probs_b = 0.f;
	float tmp_probs_nb = 0.f;
	for (int i = 0; i < len - 1; i++) {
	    for (int j = 1 + i; j < len; j++) {
	        if (candidates[i].probs_set_next < candidates[j].probs_set_next) {
	            tmp_prob = candidates[i].probs_set_next;
	            candidates[i].probs_set_next = candidates[j].probs_set_next;
	            candidates[j].probs_set_next = tmp_prob;
				strcpy(tmp_char, candidates[i].prefix_set_next);
				strcpy(candidates[i].prefix_set_next, candidates[j].prefix_set_next);
				strcpy(candidates[j].prefix_set_next, tmp_char);
/*				tmp_char = candidates[i].prefix_set_next;
	            candidates[i].prefix_set_next = candidates[j].prefix_set_next;
	            candidates[j].prefix_set_next[j] = tmp_char;*/
				tmp_probs_b = candidates[i].probs_b_cur;
				candidates[i].probs_b_cur = candidates[j].probs_b_cur;
				candidates[j].probs_b_cur = tmp_probs_b;
				tmp_probs_nb = candidates[i].probs_nb_cur;
				candidates[i].probs_nb_cur = candidates[j].probs_nb_cur;
				candidates[j].probs_nb_cur = tmp_probs_nb;
	        }
	    }
	}
	free(tmp_char);
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

void print_hex(const char* str)
{
    unsigned char ch;
    while(ch=*str++)
    {
        printf("%02x",ch);
    }
    printf("\n");
}

PREFIX_LIST prefix_list[BEAM_SIZE];
PREFIX_LIST *ctc_beam_search_decoder(float          *probs_seq,
							   		int            probs_len,
							  		int            T,
							   		int            blank_id,
							   		float          prune)
{
    int i, j, index;
	int prefix_len = 0;
	float score = 0.0;
	float probs_nb_prev[BEAM_SIZE] = {0.0};
	float probs_b_prev[BEAM_SIZE] = {0.0};
	float probs_set_prev[BEAM_SIZE] = {0.0};
	unsigned char lable[N_GRAMS];

	CANDIDATES candidates[BEAM_SIZE * (PROBS_LEN + 1)];

	for(i = 0; i < BEAM_SIZE; i++) {
		probs_b_prev[i] = 1.0;
	};

	FILE *fp;
    if ((fp = fopen("/home/fengli/workspace/beam_search/beam_search/beam_index/tools/lm4bitassigned.bin", "r")) == NULL) {
        printf("Cannot read file\n");
        return NULL;
    }

	//read bin file
	float unk_prob;
    LM_DATA lm_data;
	ReadLmData(fp, &lm_data, &unk_prob);

    for (int t = 0; t < T; t++) {
        int cutoff_len = 0;
		unsigned char probs_idx[PROBS_LEN];
		for (i = 1; i < PROBS_LEN + 1; i++) {
			probs_idx[i - 1] = i;
		}
		float probs_seq_cutoff[PROBS_LEN] = {0.0};
		unsigned char probs_idx_cutoff[PROBS_LEN] = {0};

		sorted(probs_seq + t * probs_len, probs_idx, probs_len);

		cutoff_len = sequence_new(probs_seq_cutoff, probs_idx_cutoff, probs_seq + t * probs_len, probs_idx, probs_len, prune);

		for (i = 0; i < BEAM_SIZE * (PROBS_LEN + 1); i++) {
			memset(candidates[i].prefix_set_next, 0, PREFIX_CHAR_LENGTH);
			candidates[i].probs_b_cur = 0.0;
			candidates[i].probs_nb_cur = 0.0;
			candidates[i].probs_set_next = 0.0;
		}

		if (t == 0)
			prefix_len = 1;
		else
			prefix_len = BEAM_SIZE;

		int cnt = 0;
        for (i = 0; i < prefix_len; i++) {

			int l_len = strlen(prefix_list[i].prefix_set_prev);
            for (index = 0; index < cutoff_len; index++) {

				unsigned char c_idx;
				c_idx = probs_idx_cutoff[index];
                float prob_c = probs_seq_cutoff[index];
				if (c_idx == blank_id) {
					candidates[i * cutoff_len + index + cnt].probs_b_cur = prob_c * (probs_b_prev[i] + probs_nb_prev[i]);
					strncpy(candidates[i * cutoff_len + index + cnt].prefix_set_next, prefix_list[i].prefix_set_prev, l_len);
                } else {
                    unsigned char l_plus[PREFIX_CHAR_LENGTH];
                    memset(l_plus, '\0', sizeof(l_plus));

					strncpy(l_plus, prefix_list[i].prefix_set_prev, l_len);
					l_plus[l_len] = c_idx;

					int lp_len = l_len + 1;
					unsigned char last_char;
					if (l_len > 0) {
						last_char = prefix_list[i].prefix_set_prev[l_len - 1];
					} else {
						last_char = prefix_list[i].prefix_set_prev[0];
					}

					if (c_idx == last_char) {
						if (probs_b_prev[i] != 0 && probs_nb_prev[i] != 0) {
							strncpy(candidates[i * cutoff_len + index + cnt].prefix_set_next, l_plus, lp_len);
							candidates[i * cutoff_len + index + cnt].prefix_set_next[l_len] = c_idx;
							candidates[i * cutoff_len + index + cnt].probs_nb_cur = prob_c * probs_b_prev[i];
							cnt++;
							strncpy(candidates[i * cutoff_len + index + cnt].prefix_set_next, prefix_list[i].prefix_set_prev, l_len);
							candidates[i * cutoff_len + index + cnt].probs_nb_cur = prob_c * probs_nb_prev[i];
						} else if (probs_nb_prev[i] != 0 && probs_b_prev[i] == 0) {
							strncpy(candidates[i * cutoff_len + index + cnt].prefix_set_next, prefix_list[i].prefix_set_prev, l_len);
							candidates[i * cutoff_len + index + cnt].probs_nb_cur = prob_c * probs_nb_prev[i];
						} else if (probs_nb_prev[i] == 0 && probs_b_prev[i] != 0) {
							strncpy(candidates[i * cutoff_len + index + cnt].prefix_set_next, l_plus, lp_len);
							candidates[i * cutoff_len + index + cnt].prefix_set_next[l_len] = c_idx;
							candidates[i * cutoff_len + index + cnt].probs_nb_cur = prob_c * probs_b_prev[i];
						}
					} else {
                        strncpy(candidates[i * cutoff_len + index + cnt].prefix_set_next, l_plus, lp_len);
						candidates[i * cutoff_len + index + cnt].prefix_set_next[l_len] = c_idx;
						unsigned char lable[N_GRAMS];
						memset(lable, 0, N_GRAMS * sizeof(char));
						if (lp_len > 0) {
							if (lp_len < N_GRAMS) {
								lable[0] = START_CHAR;
								memcpy(lable + 1, l_plus, lp_len);
								score = ext_scoring_func(lm_data, (lp_len + 1), lable, unk_prob) * lp_len;
							} else {
								memcpy(lable, l_plus + (lp_len - N_GRAMS), N_GRAMS);
								score = ext_scoring_func(lm_data, N_GRAMS, lable, unk_prob) * lp_len;
							}
						} else {
							score = 1.0;
						}
						candidates[i * cutoff_len + index + cnt].probs_nb_cur = score * prob_c * (probs_b_prev[i] + probs_nb_prev[i]);
                    }
                }
            }
        }
		int len = prefix_len * cutoff_len + cnt;

		/*遍历重复字符串，将重复字符串进行合并，合并位置选择最小索引值处，后面字符串依次前移，
		并根据上一次是否为blank修改probs_nb_prev（将重复字符串对应索引值非blank结尾的概率相加）
		及probs_b_prev（重复字符串对应索引值blank结尾的概率值相加）的值。*/

		int new_len = candidatas_same_merged(candidates, len);

		candidatas_sorted(candidates, new_len);

		//根据概率大小进行排序，前缀字符串，索引值等跟随概率排序进行变换位置

		for (i = 0; i < BEAM_SIZE; i++) {
			strcpy(prefix_list[i].prefix_set_prev, candidates[i].prefix_set_next);
			probs_nb_prev[i] = candidates[i].probs_nb_cur;
			probs_b_prev[i] = candidates[i].probs_b_cur;
			probs_set_prev[i] = candidates[i].probs_set_next;
		}
    }

	for (i = 0; i < BEAM_SIZE; i++) {
		if (probs_set_prev[i] > 0.0 && strlen(prefix_list[i].prefix_set_prev) >= 1) {
			prefix_list[i].beam_result = fastlogf(probs_set_prev[i]);
		} else {
			prefix_list[i].beam_result = -INFINITY;
		}
	}

	free(lm_data.data_buffer);
	fclose(fp);
    return prefix_list;
}
