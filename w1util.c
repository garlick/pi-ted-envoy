#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "w1.h"

int main (int argc, char *argv[])
{
	double val;

	if (argc != 2) {
		fprintf (stderr, "Usage: w1util addr\n");
		exit (1);
	}
	val = w1_therm_get (argv[1]);
	if (val != NAN)
		printf ("%f\n", c2f (val));
	exit (0);
}
