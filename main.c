#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

void print_hex(const char* str)
{
    unsigned char ch;
    while(ch=*str++)
    {
        printf("%02x",ch);
    }
    printf("\n");
}
/*
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
//	float score[BEAM_SIZE] = {0.0};
	float *score;

    unsigned char *prefix_set_prev[BEAM_SIZE];
    for (i = 0; i < BEAM_SIZE; i++) {
        prefix_set_prev[i] = (unsigned char *)malloc(PREFIX_CHAR_LENGTH * sizeof(char));
        memset(prefix_set_prev[i], 0, PREFIX_CHAR_LENGTH * sizeof(char));
    }

    start = clock();
    score = ctc_beam_search_decoder(fp, prefix_set_prev, probs_seq, PROBS_LEN, 68, 65, 0);
    finish = clock();
    for (i = 0; i < BEAM_SIZE; i++) {
		printf("%.15f\n", score[i]);
		print_hex(prefix_set_prev[i]);
	}
    printf("%f seconds\n", (double)(finish - start) / CLOCKS_PER_SEC);
	for (i = 0; i < BEAM_SIZE; i++) {
        free(prefix_set_prev[i]);
    }
    fclose(fp);
}
*/

int main()
{
    FILE *fp;

    if ((fp = fopen("/home/fengli/workspace/beam_search/beam_search/beam_index/lib/beam_search/to_fengli/2020_04_14/lm.bin", "r")) == NULL) {
//    if ((fp = fopen("/home/fengli/workspace/beam_search/beam_search/beam_index/lib/beam_search/lm1.bin", "r")) == NULL) {
        printf("Cannot read file\n");
        return -1;
    }

	int i = 0;
	float *score;

    unsigned char *prefix_set_prev[BEAM_SIZE];
    for (i = 0; i < BEAM_SIZE; i++) {
        prefix_set_prev[i] = (unsigned char *)malloc(PREFIX_CHAR_LENGTH * sizeof(char));
        memset(prefix_set_prev[i], 0, PREFIX_CHAR_LENGTH * sizeof(char));
    }

    score = ctc_beam_search_decoder(fp, prefix_set_prev, probs_seq, PROBS_LEN, 68, 65, 0);
    for (i = 0; i < BEAM_SIZE; i++) {
    	print_hex(prefix_set_prev[i]);
		printf("%.15f\n", score[i]);
	}
	for (i = 0; i < BEAM_SIZE; i++) {
        free(prefix_set_prev[i]);
    }
    fclose(fp);
}
