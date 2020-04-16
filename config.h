#ifndef __CONFIG_H__
#define __CONFIG_H__

#define VALIDATE_PTR(ptr) if(NULL == ptr) { printf("%s: allocation failed\n", #ptr); return -1;}
#define BEAM_SIZE          4
#define PROBS_LEN          65
#define S_LEN              50
#define PREFIX_CHAR_LENGTH 50
#define N_GRAMS            4
#define UNK                65
#define START_CHAR         67

typedef struct {
	unsigned char prefix_set_next;
	float probs_set_next;
	float probs_b_next;
	float probs_nb_next;
}CANDIDATES_[PROBS_LEN*BEAM_SIZE+BEAM_SIZE];

#endif
