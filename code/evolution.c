#ifndef _EVOLUTION_C
#define _EVOLUTION_C

#include "evolution.h"

/**********************/
extern inline void dbg(char*);

unsigned int MAX_EPOCHS;
float DESIRED_ERROR;
unsigned int STRATEGY_MAX_LENGTH;
int TRAINING_SESSIONS;
int TESTING_SESSIONS;

/**
 * evaluate strategy on random scenarios through an artificial NN 
 */
static float eval(char* strategy, char* datafile) {
	float fitness = -1.0;
	/* initialised at NULL to start */
	struct scenario_t *train_scenario = NULL, *run_scenario = NULL;
	struct fann *ann;	/* the artificial neural network */
	int i, j;

	FILE *f;

	/* 
	 * the number of input neurons is the number of actions
	 * in a strategy
	 */
	int input_neurones = get_input_neurones(strategy);

	/* where to store the data to be fed to the network as input */
	fann_type results[input_neurones];	
	fann_type *network_output;	/* the network output */
	fann_type expected_results[TESTING_SESSIONS];	/* the expected output */

	/* create neural network 
	 * params: layers, input neurones, hidden neurones, output neurones */
	ann = fann_create_standard(NUM_LAYERS, (unsigned int)input_neurones, 
			(unsigned int)input_neurones+5, NUM_OUTPUT); 

	/* prepare the file for writing */
	f = fopen(datafile, "w+");
	if ( f == NULL ) {
		perror("Unable to open file for writing");
		fann_destroy(ann); 
		return -1;
	}

	/* header for neural network:
	 * first line contains:
	 * training_pairs inputs outputs
	 * second line:
	 * input data
	 * third line:
	 * expected output 
	 */
	fprintf(f, "%d %d %d\n", TRAINING_SESSIONS, get_input_neurones(strategy), NUM_OUTPUT);


	for ( i = 0 ; i < TRAINING_SESSIONS ; i++ ) {
		/* generate a new training scenario */
		train_scenario = gen_scenario();

		/* run strategy */
		if ( run_strategy_mem(strategy, train_scenario, results, input_neurones) < 0 ) {
			/* problem here */
			fprintf(stderr, "Error in running strategy for training\n");
			fclose(f);
			destroy_scenario(train_scenario);
			fann_destroy(ann); 
			return -1;
		}

		/* print collected values in the way the neural network expects
		 * to find them */
		for ( j = 0 ; j < input_neurones ; j++ ) {
			fprintf(f, "%f ", results[j]);
		}
		/* next line is output */
		fprintf(f, "\n%f\n", train_scenario->nearest_object_centre);
	}

	if ( fclose(f) != 0 ) {
		perror("Unable to close file");
		destroy_scenario(train_scenario);
		fann_destroy(ann); 
		return -1;
	}

	/* train NN on results */
	fann_train_on_file(ann, datafile, 
			MAX_EPOCHS, EPOCHS_BETWEEN_REPORTS, DESIRED_ERROR);

	/* done with the training scenario
	 * (could avoid destroying this, but it is for clarity */
	destroy_scenario(train_scenario);

	/*
	 * run the same network through 100 different scenarios
	 * expected value: the center of the nearest object
	 * measure goodness of the answers, do average/sqr err
	 * that's the fitness
	 */
	for ( i = 0 ; i < TESTING_SESSIONS ; i++ ) {

		/* generate a new scenario */
		run_scenario = gen_scenario();

		/* apply strategy to new scenario, save results to memory */
		if ( run_strategy_mem(strategy, run_scenario, results, input_neurones) < 0 ) {
			fprintf(stderr, "Error in running strategy for testing\n");
			return -1;
		}

		/* run the neural network on the new input */
		network_output = fann_run(ann, results); 

		/* compare the network output with the expected value */
		expected_results[i] = fabs(run_scenario->nearest_object_centre - *network_output);

	}

	fann_destroy(ann); 
	destroy_scenario(run_scenario);

	/* TODO future fitness might include length, epochs, etc */
	for ( i = 0 ; i < TESTING_SESSIONS ; i++ )
		fitness += (float)expected_results[i];
	fitness /= TESTING_SESSIONS;

	return fitness;
}

/**
 * generate a strategy
 */
static char* gen_strategy(int starting_len) {
	int i, num_cmds;

	char* strategy = malloc(STRATEGY_MAX_LENGTH);

	/* zeroes the strategy */
	memset(strategy, 0, STRATEGY_MAX_LENGTH);

	/* number of full commands (action + condition) in this strategy 
	 * (the last byte is reserved for \0) */
	num_cmds = starting_len / 2;

	/* check for minimum length again */
	num_cmds == 0 ? num_cmds = 1 : num_cmds;

	for ( i = 0 ; i < num_cmds*2 ;  ) {
		/* all enums start from 1 */
		strategy[i++] = gen_rand32() % NUM_ACTIONS + 1;
		strategy[i++] = gen_rand32() % NUM_CONDITIONS + 1;
	}

	return strategy;
}

inline static void print_strategy(char* strategy) {
	int i;
	for ( i = 0 ; i < STRATEGY_MAX_LENGTH ; i++ )
		printf("%d", strategy[i]);
	printf("\n");
}

static void mutate(char* strategy) {
	dbg("mutate\n");
	size_t strategy_len = strlen(strategy);

	/* the mutation locus
	 * could be inside the current genotype or outside */
	int locus = gen_rand32() % STRATEGY_MAX_LENGTH;

	/* if the locus is outside the current genotype
	 * AND if there's still enough space (last byte is for null-terminating it) */
	if ( locus > strategy_len && strategy_len < STRATEGY_MAX_LENGTH-3 ) {
		/* add new gene (action+condition) */
		strategy[strategy_len] = gen_rand32() % NUM_ACTIONS + 1;
		strategy[strategy_len+1] = gen_rand32() % NUM_CONDITIONS + 1;
	} else {
		/* mutate locally */

		/* avoid mutating outside the current genotype;
		 * chose a locus within the current individual */
		if ( locus > strategy_len )
			locus = gen_rand32() % strategy_len;

		/* flip a coin: remove a (action+condition) gene or mutate? */
		/* note: also check that the locus corresponds to an action and not a
		 * condition */
		if ( gen_rand32() % 2 == 0 && locus % 2 == 0 ) {
			/* removing a gene (action+condition) */
			do {
				strategy[locus] = strategy[locus+2];
				strategy[locus+1] = strategy[locus+3];
				locus += 2;
			} while ( strategy[locus] != '\0' );
		} else {
			/* mutating an existing gene */
			if ( locus % 2 == 0 ) {
				/* mutate action */
				enum action_e new_action;
				do {
					new_action = gen_rand32() % NUM_ACTIONS + 1;
				} while ( strategy[locus] == new_action );
				strategy[locus] = new_action;
			} else {
				/* mutate condition */
				enum condition_e new_condition;
				do {
					new_condition = gen_rand32() % NUM_CONDITIONS + 1;
				} while ( strategy[locus] == new_condition );
				strategy[locus] = new_condition;
			}
		}
	}
}

static void cross_breed(char* winner, char* loser) {
	dbg("cross/breed\n");
	size_t winner_len = strlen(winner);
	size_t loser_len = strlen(loser);
	/* chose a locus within the shorter string */
	unsigned int locus, i;
	
	int do_copy = 0;
	
	/* if the loser is already near-full, must copy and not add */
	/* important: keep -3 (because it could happen to make the loser bigger, etc
	 * */
	if ( loser_len >= STRATEGY_MAX_LENGTH-3 )
		do_copy = 1;

	if ( do_copy == 0 && gen_rand32() % 2 == 0 ) {

		/* adding a gene from the winner to the end of the loser or
		 * to a random position in the loser */

		/* pick a gene from the winner */
		locus = gen_rand32() % winner_len;
		enum action_e action;
		enum condition_e condition;

		if ( locus % 2 == 0 ) {
			/* if it's even it's an action:
			 * must copy the subsequent condition (locus+1)
			 */
			action = winner[locus];
			condition = winner[locus+1];
		} else {
			/* it's odd -> a condition: 
			 * must copy the previous action (locus-1) 
			 */
			action = winner[locus-1];
			condition = winner[locus];
		}

		/* pick a random position in the loser */
		int dest = gen_rand32() % loser_len;
		/* make sure it's an action (even locus) */
		dest = dest % 2 == 0 ? dest : dest+1;

		/* count backwards */
		for ( i = loser_len ; i > dest ; i-- ) {
			loser[i+2] = loser[i];
		}
		loser[dest] = action;
		loser[dest+1] = condition;
	} else {
		/* copying a gene (action+condition) from winner to loser */
		locus = gen_rand32() % winner_len;

		/* copy the action OR condition from the winner to the loser;
		 * since all genotypes have the same structure (action-condition)
		 * it is guaranteed that the same kind of gene will be copied */
		loser[locus] = winner[locus];
	}
}

/*
 * mutate or cross-breed strategies
 */
static int mutate_breed(char* winner, char* loser) {
	ENTRY item;

	size_t winner_len, loser_len;
	winner_len = strlen(winner);
	loser_len = strlen(loser);

	/* might loop forever if population is already full-1 (?) */
	do {
		if ( genrand_real3() > PROB_MUT ) {
			/* mutate */
			mutate(loser);
		} else {
			/* cross-breed */
			cross_breed(winner, loser);
		}

		/* be sure that the new individual has not been already evaluated */
		item.key = strdup(loser);
	} while ( hsearch(item, FIND) != NULL );
	dbg("new individual found\n");

	/* insert new item into hash table */
	if ( hsearch(item, ENTER) == NULL ) {
		perror("Population limit reached");
		return -1;
	}
	return 0;
}

/*
 * main evolutionary algorithm, using a variant of the microbial GA
 */
void evolve (char* datafile, int generations, unsigned int max_epochs, float desired_error, 
		int strategy_max_len, int strategy_starting_len, 
		int training_sessions, int testing_sessions) {

	/* set global variables */
	DESIRED_ERROR = desired_error;
	/* must be even, last byte is \0 for terminating string */
	STRATEGY_MAX_LENGTH = strategy_max_len % 2 == 0 
		? strategy_max_len+1 : strategy_max_len;
	MAX_EPOCHS = max_epochs;

	TRAINING_SESSIONS = training_sessions;
	TESTING_SESSIONS = testing_sessions;

	char *strategy1, *strategy2;
	float fit1 = -1.0, fit2 = -1.0;
	ENTRY item1, item2;
	int winner;

	if ( strategy_starting_len > strategy_max_len ) {
		fprintf(stderr,"Starting length bigger than max length\n");
		return;
	}

	/* generate two random strategies (allocate mem)*/
	strategy1 = gen_strategy(strategy_starting_len);
	strategy2 = gen_strategy(strategy_starting_len);
	
	/* put strategies in population */
	item1.key = strdup(strategy1);
	item2.key = strdup(strategy2);
	if ( hsearch(item1, ENTER) == NULL 
			|| hsearch(item2, ENTER) == NULL ) {
		perror("Population limit reached");
		return;
	}

	winner = 0;
	/* evaluate their fitness */
	do {
		/* avoid checking already-checked strategies */
		if ( winner != 1 )
			fit1 = eval(strategy1, datafile);
		if ( winner != 2 )
			fit2 = eval(strategy2, datafile);

		if ( generations-- == 0 )
				break;

		printf("Strategy1 (fitness %f): ", fit1);
		print_strategy(strategy1);
		printf("Strategy2 (fitness %f): ", fit2);
		print_strategy(strategy2);

		if ( fit1 < 0 || fit2 < 0 )
			/* problem in memory allocation, etc */
			return;

		/* technically is not a fitness, but an error measure */
		if ( fit1 < fit2 ) {
			winner = 1;
			/* mutate or breed */
			if ( mutate_breed(strategy1, strategy2) < 0 )
				break;
		} else {
			winner = 2;
			/* note: it does mutate if they are equivalent.. */
			if ( mutate_breed(strategy2, strategy1) < 0 )
				break;
		}
		printf("\n");
	} while ( 1 );	/* break if generations == 0 */

	/* print the current best strategy upon quit*/
	printf("Current best strategy: ");
	if ( winner == 1 ) 
		print_strategy(strategy1);
	else if ( winner == 2 ) 
		print_strategy(strategy2);
	else
		printf("No strategy\n");

	free(strategy1);
	free(strategy2);
}


#endif
