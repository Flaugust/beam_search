#ifndef __DECODERS_H__
#define __DECODERS_H__

extern float *ctc_beam_search_decoder(FILE *fp, float *probs_seq, int probs_len, int T, int blank_id, float prune);

#endif

