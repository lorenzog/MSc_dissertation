#ifndef _SCENARIO_H
#define _SCENARIO_H

#include <string.h>
#include <strings.h>
#include <math.h>

/* random number generation library */
#include "SFMT.h"

/* FANN library */
#include "floatfann.h"

static const float SENSOR_ANGULARSTEP = 0.1;
static const float SENSOR_LATERALSTEP = 0.1;
static const float SENSOR_SKIPSTEP = 0.1;

static const float SENSOR_CONE = 0.1; 	

static const float OBJECT_DEPTH = 0.05;

struct object_t {
	float start_x, start_y, end_x, end_y;	/* for ease of computation */
};

struct sensor_t {
	float angle;	
	float pos;
};

struct scenario_t {
	struct object_t *obj1;
	struct object_t *obj2;

	struct sensor_t *sensor;
	float nearest_object_centre;
};

struct condition_t {
	float sensor_pos;
	float sensor_angle;
	float sensor_status;
};

enum action_e { MOVE_LEFT = 1, MOVE_RIGHT, ROTATE_LEFT, ROTATE_RIGHT, SKIP_LEFT, SKIP_RIGHT };

/* move until the object appears/disappears/or landmark movement, 
 * i.e. move 'a little bit' and then check
 */
enum condition_e { NON_OBJECT = 1, OBJECT };
static const int NUM_ACTIONS = 6;
static const int NUM_CONDITIONS = 2;


struct scenario_t *gen_scenario();
void destroy_scenario(struct scenario_t *scenario);
void init_conditions(struct condition_t *now);

inline int get_input_neurones(char* strategy);

int run_strategy_disk(char* strategy, struct scenario_t *scenario, 
		char* filename, int len);
int run_strategy_mem(char* strategy, struct scenario_t *scenario, 
		fann_type* dest, int len);

#endif	
