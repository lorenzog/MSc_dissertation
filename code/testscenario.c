#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "scenario.c"
#include "evolution.c"

static const int CONDS = 10;

/******************* object handling ****************/


static void print_object(struct object_t *obj) {
	/*
	printf("Obj x: %f y: %f len: %f depth: %f\n", 
			obj->x, obj->y, obj->length, obj->depth);
			*/
	printf("Obj start_x: %f, start_y: %f, end_x: %f, end_y: %f\n", 
			obj->start_x, obj->start_y, obj->end_x, obj->end_y);
}

static void print_sensor(struct sensor_t *s) {
	printf("Sensor pos: %f angle: %f\n", s->pos, s->angle);
}

static void print_scenario(struct scenario_t *s) {
	printf("Scenario obj1:\n");
	print_object(s->obj1);
	printf("Scenario obj2:\n");
	print_object(s->obj2);
	printf("Scenario sensor:\n");
	print_sensor(s->sensor);
}

/*
static void print_status(struct condition_t *now) {
	printf("Current sensor pos: %f, angle: %f, status: %f\n",
			now->sensor_pos, now->sensor_angle, now->sensor_status);
}
*/

int main ( int argc, char **argv ) {
	assert(argc == 3);

	/* random seed gen */
	int seed = strtol(argv[1], NULL, 10);
	init_gen_rand(seed);
	printf("seed: %d\n", seed);

	char* tmpfile = argv[2];

	/* scenario */
	struct scenario_t *scenario = gen_scenario();

	print_scenario(scenario);

	/* strategy */ 
	//char strategy[STRATEGY_MAX_LENGTH];
	//gen_strategy(strategy, STRATEGY_MAX_LENGTH);
	char *strategy = gen_strategy();
	/*
	strategy[0] = ROTATE_RIGHT;
	strategy[1] = OBJECT;
	strategy[2] = ROTATE_RIGHT;
	strategy[3] = NON_OBJECT;
	strategy[4] = '\0';
	*/

	int len = get_input_neurones(strategy);

	/* show strategy */
	printf("Strategy printout:\n");
	int i;
	for ( i = 0 ; i < STRATEGY_MAX_LENGTH ; i++ )
		printf("%d", strategy[i]);
	printf("\n");
	printf("strategy length: %d\n", len);


	/* run strategy */

	/* where to store data for memory */
	fann_type dest[len];
	if ( run_strategy_mem(strategy, scenario, dest, len) < 0 ) {
		fprintf(stderr, "Problems ahead\n");
		return -1;
	}

	/* show input neurones */
	for ( i = 0 ; i < len ; i++ ) 
		printf("%f\n", dest[i]);
	printf("\n");

	/* cleanup */
	destroy_scenario(scenario);
	free(strategy);

	return 0;

}
