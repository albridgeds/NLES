#ifndef RECURSIVE_MEAN_H_
#define RECURSIVE_MEAN_H_


class recursive_mean
{
	public:
	recursive_mean();
	void init();
	float get ();
	float get (float &min, float &max);
	void add (float newval);

	float val;
	float min_val;
	float max_val;
	int counter;
};



#endif /* RECURSIVE_MEAN_H_ */
