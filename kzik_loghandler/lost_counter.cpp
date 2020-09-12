#include "lost_counter.h"


lost_counter::lost_counter(int _logic, float _limit)
{
	init(_logic,_limit);
}


void lost_counter::init(int _logic, float _limit)
{
	counter = -1;
	start[0] = 0;
	end[0] = 0;
	duration[0] = 0;
	cur_status = false;
	logic = _logic;
	limit = _limit;
}

void lost_counter::copy (lost_counter in)
{
	counter = in.counter;
	for (int i=0; i<cl_size; i++)
	{
		start[i] = in.start[i];
		end[i] = in.end[i];
		duration[i] = in.duration[i];
		parameter[i] = in.parameter[i];
	}
}



// logic
// 0 - ��������� ������������� ���������� ��������
// 1 - ������������ �������, ����� �������� ��������� ������
// 2 - ������������ �������, ����� �������� �� ��������� ������
void lost_counter::set (int time, float value)
{
	bool trigger;
	if (logic==1)
		trigger = value>limit;		// trigger=true ���� �������� ������ �������
	if (logic==2)
		trigger = value<limit;		// trigger=true ���� �������� ������ �������

	if (trigger && !cur_status)		// ���� ������ ������� �����������, � ������ �� �����������, ��������� ��������
	{
        if(!set_start(time))
            parameter[counter] = value;
	}
	if (!trigger && cur_status)		// ���� ������ ������� �� �����������, � ������ �����������, ��������� ��������
	{
		set_end(time);
	}

	if (cur_status)
	{
		if (logic==1 && value>parameter[counter])
            parameter[counter] = value;
		if (logic==2 && value<parameter[counter])
            parameter[counter] = value;
	}
}


int lost_counter::set_start(int time)
{
	counter++;
	cur_status = true;
	if (counter >= cl_size)
	{
		if (counter == cl_size)
            return -1;
	}
	start[counter] = time;
	end[counter] = 86399;
	duration[counter] = 86400-time;

    return 0;
}


int lost_counter::set_end(int time)
{
	cur_status = false;
	if (counter >= cl_size)
        return -1;
	end[counter] = time-1;
	duration[counter] = time - start[counter];
    return 0;
}
