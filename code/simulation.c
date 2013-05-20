#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/* random number generation library */
#include "SFMT.h"

#include "evolution.h"


inline void usage(char* progname) {
	printf("Usage: %s <pop size> <random seed> <temp datafile> <# generations> ", progname);
	printf("<max # epochs> <desired error> <strategy max length> ");
	printf("<strategy starting length> <training sessions> <testing sessions>\n");
}

int main ( int argc, char **argv ) {
	int max_popsize, seed, generations, strategy_max_len,
		strategy_starting_len, training_sessions, testing_sessions;
	unsigned int max_epochs;
	float desired_error;
	char* datafile;

	if ( argc != 11 ) {
		usage(argv[0]);
		return -1;
	}

	/* first arg is max population size */
	max_popsize = atoi(argv[1]);

	/* second arg is random seed */
	seed = strtol(argv[2], NULL, 10);
	init_gen_rand(seed);
	printf("Random seed: %d\n", seed);

	/* third arg is temporary datafile, test-open for reading and writing */
	{
		FILE *f;
		datafile = argv[3];
		f = fopen(datafile, "w+");
		if ( f == NULL ) {
			perror("Error in opening file for reading and writing");
			return -1;
		}
		/* test passed; will re-open later */
		fclose(f);
	}

	/* fourth arg is no. of generations */
	generations = atoi(argv[4]);
	if ( generations <= 0 ) {
		fprintf(stderr,"Negative generations?\n");
		return -1;
	}

	/* fifth arg is no. of epochs */
	max_epochs = atoi(argv[5]);

	/* sixth arg is desired error for neural network */
	desired_error = atof(argv[6]);

	/* seventh arg is strategy max length */
	strategy_max_len = atoi(argv[7]);
	
	/* eighth arg is strategy starting length */
	strategy_starting_len = atoi(argv[8]);

	training_sessions = atoi(argv[9]);
	testing_sessions = atoi(argv[10]);

	/* initialise population */
	if ( hcreate(max_popsize) == 0 ) {
		perror("Unable to allocate population");
		hdestroy();
		return -1;
	}

	/* run the evolutionary algorithm */
	evolve(datafile, generations, max_epochs, desired_error, strategy_max_len,
			strategy_starting_len, training_sessions, testing_sessions);

	/* remove the population table */
	hdestroy();

	return 0;

}
