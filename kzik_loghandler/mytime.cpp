#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mytime.h"


my_time::my_time(int _local) {
	init();
	local = _local;
}

my_time::my_time() {
	init();
	local = 0;
}

my_time::~my_time() {

}



void my_time::init()
{
	time(&dtime);

	year = -1;
	month = -1;
	day = -1;
	hour = -1;
	min = -1;
	sec = -1;

	bSyslog = false;
}


void my_time::set(int _hour, int _min, int _sec, int _day, int _month, int _year)
{
	if (_hour != -1)
		hour = _hour;
	if (_min != -1)
		min = _min;
	if (_sec != -1)
		sec = _sec;
	if (_day != -1)
		day = _day;
	if (_month != -1)
		month = _month;
	if (_year != -1)
		year = _year;

	time_t tmp_time;
	time(&tmp_time);
	struct tm *timeinfo;
	timeinfo = localtime(&tmp_time);
	date2struct(timeinfo);

	dtime = mktime(timeinfo);
	yday = timeinfo->tm_yday;
}



void my_time::set_start()
{
	struct tm *std_tm;
	std_tm = localtime(&dtime);

	if (year!=-1)
		std_tm->tm_year = year-1900;

	if (month!=-1)
		std_tm->tm_mon = month-1;

	if (day!=-1)
		std_tm->tm_mday = day;

	if (hour!=-1)
		std_tm->tm_hour = hour;
	else
		std_tm->tm_hour = 0;

	std_tm->tm_min = 0;
	std_tm->tm_sec = 0;

	dtime = mktime(std_tm);
	if (local)
	{
		dtime -= 3600*local;
		std_tm = localtime(&dtime);
	}
	ftime = (float)dtime;
	struct2date(std_tm);
}



void my_time::set_end(my_time start_t)
{
	struct tm *std_tm;
	std_tm = localtime(&dtime);

	std_tm->tm_year = start_t.year-1900;
	std_tm->tm_mon = start_t.month-1;

	if (day!=-1)
		std_tm->tm_mday = day;
	else
		std_tm->tm_mday = start_t.day;

	if (hour!=-1)
		std_tm->tm_hour = start_t.hour;
	else
		std_tm->tm_hour = 23;

	std_tm->tm_min = 59;
	std_tm->tm_sec = 59;

	dtime = mktime(std_tm);
	if (local)
	{
		dtime -= 3600*local;
		std_tm = localtime(&dtime);
	}
	ftime = (float)dtime;
	struct2date(std_tm);
}




int my_time::set_log(char *buf)
{
	float msec;
	if (get_timedate(buf,msec))
		return 1;

	time_t tmp_time;
	time(&tmp_time);
	struct tm *std_tm;
	std_tm = localtime(&tmp_time);
	date2struct(std_tm);

	dtime = mktime(std_tm);
	if (local)
	{
		dtime -= 3600*local;
		std_tm = localtime(&dtime);
		struct2date(std_tm);
	}
	yday = std_tm->tm_yday;
	ftime = dtime + msec;
	tod = hour*3600 + min*60 + sec + msec;
	return 0;
}



void my_time::date2struct (struct tm *std_tm)
{
	std_tm->tm_hour = hour;
	std_tm->tm_min = min;
	std_tm->tm_sec = sec;
	std_tm->tm_mday = day;
	std_tm->tm_mon = month-1;
	std_tm->tm_year = year-1900;
}


void my_time::struct2date (struct tm *std_tm)
{
	hour = std_tm->tm_hour;
	min = std_tm->tm_min;
	sec = std_tm->tm_sec;
	day = std_tm->tm_mday;
	month = std_tm->tm_mon + 1;
	year = std_tm->tm_year + 1900;
	yday = std_tm->tm_yday;
}




int my_time::get_timedate (char *buf, float &msec)
{
	if (bSyslog)
		return get_timedate_syslog(buf,msec);
	else
		return get_timedate_origin(buf,msec);
}


int my_time::get_timedate_syslog (char *buf, float &msec)
{
    char tmp2 [100];

	strncpy(tmp2,buf,3);
	tmp2[3] = '\0';
	if (!strcmp(tmp2,"Jan"))
		month = 1;
	else if (!strcmp(tmp2,"Feb"))
		month = 2;
	else if (!strcmp(tmp2,"Mar"))
		month = 3;
	else if (!strcmp(tmp2,"Apr"))
		month = 4;
	else if (!strcmp(tmp2,"May"))
		month = 5;
	else if (!strcmp(tmp2,"Jun"))
		month = 6;
	else if (!strcmp(tmp2,"Jul"))
		month = 7;
	else if (!strcmp(tmp2,"Aug"))
		month = 8;
	else if (!strcmp(tmp2,"Sep"))
		month = 9;
	else if (!strcmp(tmp2,"Oct"))
		month = 10;
	else if (!strcmp(tmp2,"Nov"))
		month = 11;
	else if (!strcmp(tmp2,"Dec"))
		month = 12;
	else
		month = 0;

	year = syslog_year;
	msec = 0.0;

	strncpy(tmp2,buf+4,2);
	tmp2[2]='\0';
	day = atoi(tmp2);
	strncpy(tmp2,buf+7,2);
	tmp2[2]='\0';
	hour = atoi(tmp2);
	strncpy(tmp2,buf+10,2);
	tmp2[2]='\0';
	min = atoi(tmp2);
	strncpy(tmp2,buf+13,2);
	tmp2[2]='\0';
	sec = atoi(tmp2);

    if (hour>23 || min>59 || sec>59 || hour<0 || min<0 || sec<0) {
        printf("\nWARNING: wrong time %s (%d:%d:%d.%.3f)\n",strncpy(tmp2,buf+11,8),hour,min,sec,msec);
        return 1;
    }
    if (day>31 || month>12 || year>2030 || day<0 || month<0 || year<2000) {
        printf("WARNING: wrong date %s (%d-%d-%d)\n",strncpy(tmp2,buf,10),day,month,year);
        return 1;
    }
    return 0;
}


int my_time::get_timedate_origin (char *buf, float &msec)
{
    char tmp2 [100];
    strncpy(tmp2,buf,4);
    tmp2[4]='\0';
    year = atoi(tmp2);
    strncpy(tmp2,buf+5,2);
    tmp2[2]='\0';
    month = atoi(tmp2);
    strncpy(tmp2,buf+8,2);
    tmp2[2]='\0';
    day = atoi(tmp2);
    strncpy(tmp2,buf+11,2);
    tmp2[2]='\0';
    hour = atoi(tmp2);
    strncpy(tmp2,buf+14,2);
    tmp2[2]='\0';
    min = atoi(tmp2);
    strncpy(tmp2,buf+17,2);
    tmp2[2]='\0';
    sec = atoi(tmp2);
    strncpy(tmp2,buf+20,3);
    tmp2[3]='\0';
    msec = atof(tmp2)/1000;

    if (hour>23 || min>59 || sec>59 || hour<0 || min<0 || sec<0) {
        printf("\nWARNING: wrong time %s (%d:%d:%d.%.3f)\n",strncpy(tmp2,buf+11,8),hour,min,sec,msec);
        return 1;
    }
    if (day>31 || month>12 || year>2030 || day<0 || month<0 || year<2000) {
        printf("WARNING: wrong date %s (%d-%d-%d)\n",strncpy(tmp2,buf,10),day,month,year);
        return 1;
    }
    return 0;
}



void my_time::get_date (char *buf, int &day, int &month, int &year)
{
    char tmp2 [100];
    strncpy(tmp2,buf,4);
    tmp2[4]='\0';
    year = atoi(tmp2);
    strncpy(tmp2,buf+5,2);
    tmp2[2]='\0';
    month = atoi(tmp2);
    strncpy(tmp2,buf+8,2);
    tmp2[2]='\0';
    day = atoi(tmp2);

    if (day>31 || month>12 || year>2030 || day<0 || month<0 || year<2000) {
        printf("WARNING: wrong date %s (%d-%d-%d)\n",strncpy(tmp2,buf,10),day,month,year);
    }
}


void my_time::get_time (char *buf, int &hour, int &min, int &sec, float &msec)
{
    char tmp2 [100];
    strncpy(tmp2,buf+11,2);
    tmp2[2]='\0';
    hour = atoi(tmp2);
    strncpy(tmp2,buf+14,2);
    tmp2[2]='\0';
    min = atoi(tmp2);
    strncpy(tmp2,buf+17,2);
    tmp2[2]='\0';
    sec = atoi(tmp2);
    strncpy(tmp2,buf+20,3);
    tmp2[3]='\0';
    msec = atof(tmp2)/1000;

    if (hour>23 || min>59 || sec>59 || hour<0 || min<0 || sec<0) {
        printf("\nWARNING: wrong time %s (%d:%d:%d.%.3f)\n",strncpy(tmp2,buf+11,8),hour,min,sec,msec);
    }
}


void my_time::shift(time_t t, int dt)
{
	dtime = t + dt;

	struct tm *std_tm;
	std_tm = localtime(&dtime);
	struct2date(std_tm);

	tod = hour*3600 + min*60 + sec;
}


void my_time::print_full()
{
	print_date();
	print_time();
}


void my_time::print_full(const char *str)
{
	printf("\n ");
	print_date();
	print_time();
	printf(" %s",str);
}


void my_time::print_time(const char *str)
{
	printf("\n ");
	print_time();
	printf(" %s",str);
}

void my_time::print_time()
{
	printf(" %02d:%02d:%02d ",hour,min,sec);
}

void my_time::print_date(const char *str)
{
	printf("\n ");
	print_date();
	printf(" %s",str);
}

void my_time::print_date()
{
	printf(" %02d/%02d/%02d ",day,month,year);
}


char * my_time::string_date()
{
	static char c[8];
	sprintf(c,"%02d/%02d/%02d",day,month,year);
	return c;
}

char * my_time::string_time()
{
	static char c[8];
	sprintf(c,"%02d:%02d:%02d",hour,min,sec);
	return c;
}

void my_time::set_syslog(int _year)
{
	bSyslog = true;
	syslog_year = _year;
}



bool my_time::get_syslog()
{
	return bSyslog;
}

