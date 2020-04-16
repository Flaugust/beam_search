#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "decoders.h"
#include "config.h"

extern float probs_seq[];

int main()
{
    int i = 0;
	float *score;
    PREFIX_LIST *prefix_list;

    prefix_list = ctc_beam_search_decoder(probs_seq, PROBS_LEN, 68, 65, 0);
    for (i = 0; i < BEAM_SIZE; i++) {
    	print_hex(prefix_list[i].prefix_set_prev);
		printf("%.15f\n", prefix_list[i].beam_result);
	}
}
