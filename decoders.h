#ifndef __DECODERS_H__
#define __DECODERS_H__

#include "config.h"

typedef struct {
    unsigned char prefix_set_next[PREFIX_CHAR_LENGTH];
    float probs_set_next;
    float probs_b_cur;
    float probs_nb_cur;
}CANDIDATES;

typedef struct {
    unsigned char prefix_set_prev[PREFIX_CHAR_LENGTH];
    float beam_result;
}PREFIX_LIST;


extern void print_hex(const char* str);
extern PREFIX_LIST *ctc_beam_search_decoder(float *probs_seq, int probs_len, int T, int blank_id, float prune);

#endif

