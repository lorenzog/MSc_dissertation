#include <assert.h>
#include <search.h>

#include "evolution.c"
#include "scenario.c"

int main ( int argc, char **argv ) {
	int i, MAX_POPSIZE;
	ENTRY item1, item2;

	assert(argc == 3);

	/* first arg is random seed */
	int seed = strtol(argv[1], NULL, 10);
	init_gen_rand(seed);

	/* second arg is max population size */
	MAX_POPSIZE = atoi(argv[2]);

	/* initialise population */
	if ( hcreate(MAX_POPSIZE) == 0 ) {
		perror("Unable to allocate population");
		return -1;
	}

	/* testing strategy generation */
	char* strategy1 = gen_strategy();
	char* strategy2 = gen_strategy();
	for ( i = 0 ; i < STRATEGY_MAX_LENGTH ; i++ )
		printf("%d", strategy1[i]);
	printf("\n");
	for ( i = 0 ; i < STRATEGY_MAX_LENGTH ; i++ )
		printf("%d", strategy2[i]);
	printf("\n\n");

	/* put strategies in population */
	item1.key = strdup(strategy1);
	item2.key = strdup(strategy2);
	if ( hsearch(item1, ENTER) == NULL 
			|| hsearch(item2, ENTER) == NULL ) {
		perror("Population limit reached");
		return -1;
	}

	while ( mutate_breed(strategy1, strategy2) >= 0 ) {
		for ( i = 0 ; i < STRATEGY_MAX_LENGTH ; i++ )
			printf("%d", strategy1[i]);
		printf("\n");
		for ( i = 0 ; i < STRATEGY_MAX_LENGTH ; i++ )
			printf("%d", strategy2[i]);
		printf("\n\n");
		/* flip a coin */
		if ( gen_rand32() % 2 == 0 ) {
			char tmp[STRATEGY_MAX_LENGTH];
			bcopy(strategy1, tmp, STRATEGY_MAX_LENGTH);
			bcopy(strategy2, strategy1, STRATEGY_MAX_LENGTH);
			bcopy(tmp, strategy2, STRATEGY_MAX_LENGTH);
		}
	}

	free(strategy1); 
	free(strategy2); 

	hdestroy();

	return 0;
}
