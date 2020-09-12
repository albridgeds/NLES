#ifndef LOST_COUNTER_H_
#define LOST_COUNTER_H_


const int cl_size = 86400;


class lost_counter
{
	public:
	lost_counter(int _logic, float _limit);
	void init(int logic, float limit);
	void set (int time, float value);
	void copy (lost_counter in);

	int start[cl_size];
	int end[cl_size];
	int duration[cl_size];
	float parameter[cl_size];

	int counter;
	int read_counter;

	private:
        int set_start(int time);
        int set_end(int time);
	bool cur_status;
	int logic;
	float limit;
};



#endif /* LOST_COUNTER_H_ */
