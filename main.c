#include <stdio.h>
#include <string.h>
#include <time.h>
#include "decoders.h"
#include "config.h"
/*
#define BEAM_SIZE 3
#define PROBS_LEN 65
#define S_LEN     200
#define PREFIX_CHAR_LENGTH 200
#define N_GRAMS   4
#define UNK       65
#define S         67
*/
extern float probs_seq[];

int main(int argc, char *argv[])
{
    FILE *fp;
    clock_t start, finish;

    if (argc < 2) {
        printf("Please input a bin file\n");
        return -1;
    }
    char file_name[20];
    char path_buffer[256];
    strncpy(path_buffer, argv[1], sizeof(path_buffer));
    if ((fp = fopen(path_buffer, "r")) == NULL) {
        printf("Cannot read file [%s]\n", path_buffer);
        return -1;
    }

	int i = 0;
	float *score;
    start = clock();
    score = ctc_beam_search_decoder(fp, probs_seq, PROBS_LEN, 68, 65, 0);
    finish = clock();
    for (i = 0; i < BEAM_SIZE; i++) {
		printf("%.15f\n", score[i]);
	}
    printf("%f seconds\n", (double)(finish - start) / CLOCKS_PER_SEC);
    fclose(fp);
}
