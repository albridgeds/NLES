#define _FILE_OFFSET_BITS 64

#include <iostream>		// windows for FILE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <zlib.h>
#include <sys/stat.h>

#include "mytime.h"
#include "getlog_common.h"


bool standard_infile = true;
bool bSyslog = false;


char ifilename[maxinamelen];



void kzik_log_handler (my_time start_t, my_time end_t);



int main (int argc, char **argv)
{
	my_time start_t, end_t;

    const char* short_options = "i:y:m:d:n:h:f:ps";
    const struct option long_options[] = {
		{"input",	required_argument,0,'i'},
		{"year",	required_argument,0,'y'},
		{"month",	required_argument,0,'m'},
		{"day",		required_argument,0,'d'},
		{"endday",	required_argument,0,'n'},
		{"hour",	required_argument,0,'h'},
		{"endhour",	required_argument,0,'f'},
		{"help",	no_argument,0,'p'},
		{"syslog",	no_argument,0,'s'},
		{NULL,0,NULL,0}
    };

    int opt;
    int option_index;
    while ((opt = getopt_long(argc,argv,short_options,long_options,&option_index)) != -1)
    {
	switch(opt) {
	case 'i':
		if (strlen(optarg) > maxinamelen)
		{
			printf("error: file name length shouldn't be longer than %d symbols\n",maxinamelen);
			return 0;
		}
		strcpy(ifilename,optarg);
		standard_infile = false;
		break;
	case 'y':
		start_t.year = atoi(optarg);
		break;
	case 'm':
		start_t.month = atoi(optarg);
		break;
	case 'd':
		start_t.day = atoi(optarg);
		break;
	case 'n':
		end_t.day = atoi(optarg);
		break;
	case 'h':
		start_t.hour = atoi(optarg);
		break;
	case 'f':
		end_t.hour = atoi(optarg);
		break;
	case 'p':
	    printf("getlog input options:\n");
	    printf("--input (-i): name of input file (%s by default)\n",ifilename);
	    printf("--output (-o): name of output file (bzk##_mmdd.log by default)\n");
	    printf("--year (-y): start year (current by default)\n");
	    printf("--month (-m): start month (current by default)\n");
	    printf("--day (-d): start day (current by default)\n");
	    printf("--endday (-n): endday (current by default)\n");
	    printf("--hour (-h): start hour (0:00 by default)\n");
	    printf("--endhour (-f): end hour (23:59 by default)\n");
	    return 0;
	 case 's':
	    bSyslog = true;
        break;

	default:
	    break;
        }
    }

    if (!standard_infile && no_input_time(start_t,end_t))
	{
		printf("\nNo time specified, read the whole file");
		start_t.set(0,0,0,1,1,2000);
		end_t.set(23,59,59,31,12,2030);
	}
	else
	{
		start_t.set_start();
		end_t.set_end(start_t);
	}
	start_t.print_full("start time");
	end_t.print_full("end time");

    kzik_log_handler(start_t,end_t);

    printf("\nhandling finished\n\n");
    return 1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////


void kzik_log_handler(my_time start_t, my_time end_t)
{
	char baseInputFileName[maxinamelen];
	strcpy(baseInputFileName,"/var/log/kzik");

	#ifndef WINDOWS_DEBUG
	gzFile iFile;
	#else
	FILE *iFile;
	#endif
	FILE *oFile;

	my_time cur_t;
	if (bSyslog)
	{
		strcpy(baseInputFileName,"/var/log/syslog");

		struct stat file_stat;
		stat(baseInputFileName,&file_stat);
		struct tm *timeinfo;
		timeinfo = localtime(&file_stat.st_mtime);
		int year = timeinfo->tm_year + 1900;

		start_t.set_syslog(year);
		end_t.set_syslog(year);
		cur_t.set_syslog(year);
	}

	// поиск нужного файла
	int inputFileNum = 0;
	if (!standard_infile)					// если используется не стандартный файл, то проверяем только его
		iFile = start_nonstandart_infile(start_t,end_t,ifilename);
	else									// если используются стандартные файлы, то проверяем все возможные
		iFile = start_standart_infile(start_t,end_t,baseInputFileName,inputFileNum);
	if (!iFile)
	{
		printf("\n\nstart file error, exit program\n");
		return;
	}

	// имя хоста (нужно для определения названия выходного файла)
	char hostname[10];
	getHostname(iFile,hostname,bSyslog);
	printf("\nhostname = <%s>",hostname);

	double azimuth,elevation;
	int signal_level,path;
	int mbt1_freq,mbt2_freq;
	double mbt1_att,mbt2_att,beacon_freq,beacon_ref,beacon_ss,amp_att,amp_pow1,amp_pow2,amp_pow3,amp_temp1,amp_temp2,amp_temp3;

	azimuth=elevation=0.0;
	signal_level=path=0;
	mbt1_freq=mbt2_freq=0;
	mbt1_att=mbt2_att=beacon_freq=beacon_ref=beacon_ss=amp_att=amp_pow1=amp_pow2=amp_pow3=amp_temp1=amp_temp2=amp_temp3=0.0;

	float time = 0.0;
	int date = 0;
	int mem_date = 0;
	int mem_time = 0;

	const int bufsize = 10000;
	char buf[bufsize];

	// начало просмотра
    while (1)
    {
#ifndef WINDOWS_DEBUG
		if (!gzgets(iFile, buf, bufsize))
		{
			if (gzeof(iFile))
			{
				gzclose(iFile);
#else
		if (!fgets(buf, bufsize, iFile))
		{
			if (feof(iFile))				// если закончился текущий файл
			{
				fclose(iFile);
#endif
				cur_t.print_time("end of file reached\n");
				if (standard_infile)		// если работаем со стандартным файлом, пробуем более новый лог
				{
					iFile = openNextStandardFile(baseInputFileName,inputFileNum);
					if (!iFile) break;
					continue;
				}
				else break;				// если работаем не со стандартным файлом, то просто завершаем работу
			}
			else						// если какая-то непонятная ошибка чтения
			{
				printf("\nError: can't read input file");
				break;
			}
		}

		// текущее время
		cur_t.set_log(buf);
		time = cur_t.tod;
		date = cur_t.yday;

		// останавливаем цикл при выходе за интервал времени
		if (cur_t.dtime>end_t.dtime)
		{
			cur_t.print_time("end of test period found (time)");
			fclose(oFile);
			break;
		}

		// новый файл при изменении даты
		if (date!=mem_date && date>=start_t.yday)
		{
			if (oFile) fclose(oFile);
			oFile = create_outfile(cur_t,".log",hostname);
			mem_date = date;
		}

		// вывод результатов каждые полторы минуты (данные обновляются редко, а чаще не успевают придти данные с усилителей)
		if (abs((int)time-mem_time)>=120 && cur_t.dtime>=start_t.dtime)
		{
			fprintf(oFile,"%6d,%d,\t%6.2f,%5.2f,%3d,\t%5d,%5d,%5.2f,%5.2f,\t%7.2f,%6.1f,%5.2f,\t%5.2f,%5.1f,%5.1f,%5.1f,%5.1f,%5.1f,%5.1f\n",
							(int)time,path,
							azimuth,elevation,signal_level,
							mbt1_freq, mbt2_freq, mbt1_att, mbt2_att,
							beacon_freq, beacon_ref, beacon_ss,
							amp_att, amp_pow1, amp_pow2, amp_pow3, amp_temp1, amp_temp2, amp_temp3);
			azimuth = 0.0;
			elevation = 0.0;
			signal_level = 0;
			path = 0;
			mbt1_att = mbt2_att = 0.0;
			beacon_freq = beacon_ref = beacon_ss = 0.0;
			amp_att = amp_pow1 = amp_pow2 = amp_pow3 = amp_temp1 = amp_temp2 = amp_temp3 = 0.0;
			mem_time = (int)time;
		}

		// анализ лога
		buf2int(buf,"Switch path ",1,path);

		char *tmp;
		tmp = strstr(buf,"rotator");
		if (tmp) {
			buf2float(buf,"azumuth=",6,azimuth);
			buf2float(buf,"elevation=",5,elevation);
			buf2int(buf,"sig_level=",3,signal_level);
		}

		tmp = strstr(buf,"BUC#1");
		if (tmp) {
			buf2int(buf,"freq=",5,mbt1_freq);
			buf2float(buf,"att=",5,mbt1_att);
		}

		tmp = strstr(buf,"BUC#2");
		if (tmp){
			buf2int(buf,"freq=",5,mbt2_freq);
			buf2float(buf,"att=",5,mbt2_att);
		}

		tmp = strstr(buf,"beacon");
		if (tmp) {
			buf2float(buf,"freq=",7,beacon_freq);
			buf2float(buf,"ref=",6,beacon_ref);
			buf2float(buf,"ss=",5,beacon_ss);
		}

		tmp = strstr(buf,"amplifier");
		if (tmp)
		{
			buf2float(buf,"att=",5,amp_att);
			buf2float(buf,"pow1=",5,amp_pow1);
			buf2float(buf,"pow1=",5,amp_pow1);
			buf2float(buf,"pow2=",5,amp_pow2);
			buf2float(buf,"pow3=",5,amp_pow3);
			buf2float(buf,"temp1=",5,amp_temp1);
			buf2float(buf,"temp2=",5,amp_temp2);
			buf2float(buf,"temp3=",5,amp_temp3);
		}
    }
}

