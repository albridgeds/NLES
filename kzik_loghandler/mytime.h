#ifndef MYTIME_H_
#define MYTIME_H_

class my_time {
public:

	time_t dtime;			// standard time

	int hour;
	int min;
	int sec;
	int yday;
	int day;
	int month;
	int year;
	float ftime;			// time_t with microseconds
	float tod;				// time of day (seconds)

	int local;
	int syslog_year;

	void init();
	void set(int hour, int min, int sec, int day, int month, int year);
	void set_start();
	void set_end(my_time start_t);
	int set_log(char *buf);
	void shift(time_t t, int dt);
	void print_time(const char *str);
	void print_time();
	void print_date(const char *str);
	void print_date();
	void print_full(const char *str);
	void print_full();
	void set_syslog(int year);
	bool get_syslog();

	char * string_date();
	char * string_time();

	my_time();
	my_time(int _local);
	virtual ~my_time();

private:
	int get_timedate (char *buf, float &msec);
	int get_timedate_syslog (char *buf, float &msec);
	int get_timedate_origin (char *buf, float &msec);
	void get_date (char *buf, int &day, int &month, int &year);
	void get_time (char *buf, int &hour, int &min, int &sec, float &msec);
	void date2struct (struct tm *std_tm);
	void struct2date (struct tm *std_tm);
	bool bSyslog;
};

#endif /* MYTIME_H_ */
