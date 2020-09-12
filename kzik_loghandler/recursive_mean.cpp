#include "recursive_mean.h"


recursive_mean::recursive_mean()
{
	init();
}

void recursive_mean::init()
{
	val = 0;
	counter = 0;
}

float recursive_mean::get ()
{
	return val;
}

float recursive_mean::get (float &min, float &max)
{
	min = min_val;
	max = max_val;
	return val;
}

void recursive_mean::add (float newval)
{
	if (newval==0.0) return;

	if (!counter)
	{
		val = newval;
		min_val = newval;
		max_val = newval;
		counter++;
		return;
	}

	val = (newval + val*counter)/(counter+1);
	counter++;

	if (newval>max_val)
	max_val = newval;
	if (newval<min_val)
	min_val = newval;
}
