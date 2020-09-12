#define _FILE_OFFSET_BITS 64


#include <iostream>		// windows for FILE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <zlib.h>
#include <math.h>

#include "mytime.h"
#include "getlog_common.h"
#include "range_message.h"
#include "lost_counter.h"
#include "recursive_mean.h"
#include "gnss.h"
#include "report.h"



const int SBASsize = 64;
const int SBASmemsize = 5;


#ifndef WINDOWS_DEBUG
gzFile iFile;
#else
FILE *iFile;
#endif
FILE *oFile, *oFile2, *oFile3;

bool standard_infile = true;
bool out_txt = false;
bool bSyslog = false;
//bool out_gps = false;

char SBAS[SBASmemsize][SBASsize+1];
float SBAStime[SBASmemsize];
int inputFileNum = 0;
char ifilename[maxinamelen];


int calc_SBAS_status (char *tmpSBAS, bool bzk_SBAS_status, bool bzk_parity_flag, float &memtime);
int SBAS_finder (char* tmpSBAS, int &best_dif);
int SBAS_comparator (char* SBAS1, char* SBAS2);
void getSBAS(char *tmpSBAS, char *buf);
void checkSBAS(char *buf, range_message &rm, float time);
//void calc_dopp_chiprate(range_message &rm);
void bzk_log_handler(my_time start_t, my_time end_t);





/////////////////////////////////////////////////////////////////////////////



int main (int argc, char **argv)
{
	my_time start_t, end_t;

	const char* short_options = "i:y:m:d:n:h:f:pts";
	const struct option long_options[] = {
		{"input",	required_argument,0,'i'},
		{"year",	required_argument,0,'y'},
		{"month",	required_argument,0,'m'},
		{"day",		required_argument,0,'d'},
		{"endday",	required_argument,0,'n'},
		{"hour",	required_argument,0,'h'},
		{"endhour",	required_argument,0,'f'},
        {"txt",		no_argument,0,'t'},				// create text file
        {"gps",		no_argument,0,'g'},				// create gps file
		{"help",	no_argument,0,'p'},
		{"syslog",	no_argument,0,'s'},
		{NULL,0,NULL,0}
	};

	int opt, option_index;
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

	case 't':
		out_txt = true;
		break;

	/*case 'g':
		out_gps = true;
		break;*/

	case 'p':
		printf("getlog input options:\n");
		printf("--input (-i): name of input file (%s by default)\n",ifilename);
		printf("--year (-y): start year (current by default)\n");
		printf("--month (-m): start month (current by default)\n");
		printf("--day (-d): start day (current by default)\n");
		printf("--endday (-n): end day (current by default)\n");
		printf("--hour (-h): start hour (0:00 by default)\n");
		printf("--endhour (-f): end hour (23:59 by default)\n");
		printf("--txt (-t): create short report text file\n");
		//printf("--gps (-g): create gps log file\n");
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



	bzk_log_handler(start_t,end_t);
	printf("\n\nhandling finished\n");

	if (out_txt)
		report_out(oFile2);

    /*if (out_gps)
    {
    	gps.dbcn0_output();
    	gps.dbcn0_output2();
    }*/
	return 1;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void bzk_log_handler(my_time start_t, my_time end_t)
{
	char baseInputFileName[] = "/var/log/bzk";

	const int bufsize = 10000;
	char buf[bufsize];
//int res;
//int counter=0;

    char hostname[64];
    strcpy(hostname,"host0");

	char *tmp;
	float time = 0.0;
	int date = 0;
	my_time cur_t;
	
	int SBAScount = 0;
	float SiggenTime = 0.0;
	float SBASdelay=0.0;
	double rectimeoffset=0.0;
    int SBAS_timeout=0, siggen_error=0;
	double sig_delay=0.0, sig_freq=0.0, sig_chiprate=0.0;

	range_message rm_gen1, rm_gen2, rm_sat1, rm_sat2;
	rm_reset(rm_gen1);
	rm_reset(rm_gen2);
	rm_reset(rm_sat1);
	rm_reset(rm_sat2);

	gnss gps;
	gps.read_dbcn0();

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
	getHostname(iFile,hostname,bSyslog);

	if (out_txt)
	{
		report_init_sm();
		report_init_lc();
	}

    for (int i=0; i<SBASmemsize; i++)
    {
        SBAStime[i]=0.0;
    }

	int mem_date=0;
	int mem_time=0;
	printf("\nstart handling the log\n");

	while (1)
	{
		// считываем строку и обрабатываем возможные ошибки
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
				cur_t.print_time("end of file reached");
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
				printf("Error: can't read input file\n");
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
			break;
		}

		// новый файл при изменении даты
        if (date!=mem_date && date>=start_t.yday)
		{
        	if (out_txt)
            {
				if (oFile2)
				{
					report_out(oFile2);
					fclose(oFile2);					// gzclose????????
				}
				//oFile2 = create_bzk_outfile(cur_t,".txt",standard_infile,ifilename);
				oFile2 = create_outfile(cur_t,".txt",hostname);
				report_init_sm();
				report_init_lc();
				report_init_rs();
			}
            /*if (out_gps)
            {
				if (oFile3) fclose(oFile3);			// gzclose????????
				oFile3 = create_bzk_outfile(cur_t,".gps",standard_infile,ifilename);
            }*/
           	if (oFile) fclose(oFile);				// gzclose????????
            //oFile = create_bzk_outfile(cur_t,".log",standard_infile,ifilename);
            oFile = create_outfile(cur_t,".log",hostname);
		}
		mem_date = date;

        // сообщение при изменении часа
		if (mem_time != cur_t.hour)
		{
			cur_t.print_time("new hour");
			mem_time = cur_t.hour;
		}


		// ОБРАБОТКА СТРОЧКИ ЛОГА
		// приход нового сообщения SBAS на БЗК
		tmp = strstr(buf,"SBAS stored in buffer");
		if (tmp)
		{
			getSBAS(SBAS[SBAScount],tmp);				// записываем новое сообщение sisnet на место самого старого в буфере
//printf("\n!!!sisnet <<%s>>",SBAS[SBAScount]);
            SBAStime[SBAScount] = time;
            SBASdelay = time - SiggenTime;
		    if (SBASdelay>86390)
				SBASdelay=86400-SBASdelay;
			if (SBAS[SBAScount][2]!='F' || SBAS[SBAScount][3]!='C')		// проверка на случай, если после sisnet-таймаута успело придти сообщение из центра
				SBAS_timeout = 0;
		}

		if(strstr(buf,"siggen switch: IfSwitch"))			// ошибка генератора
			siggen_error = 1;
		if(strstr(buf,"Starting up bzk"))					// перезапуск программы БЗК
			siggen_error = -1;

		// ошибка КС в сообщении SBAS
		if (strstr(buf,"gen std message parity failure"))
			rm_gen1.parity = false;
		if(strstr(buf,"gen nrw message parity failure"))
			rm_gen2.parity = false;
		if(strstr(buf,"space std message parity failure"))
			rm_sat1.parity = false;
		if(strstr(buf,"space nrw message parity failure"))
			rm_sat2.parity = false;
		if(strstr(buf,"space egnos nrw message parity failure"))
			rm_sat2.parity = false;

		// сообщения SBAS
		if (strstr(buf,"gen std sbas frame received at"))
			checkSBAS(buf,rm_gen1,time);
		if (strstr(buf,"gen nrw sbas frame received at"))
			checkSBAS(buf,rm_gen2,time);
		if (strstr(buf,"space std sbas frame received at"))
			checkSBAS(buf,rm_sat1,time);
		if (strstr(buf,"space nrw sbas frame received at"))
			checkSBAS(buf,rm_sat2,time);
		if (strstr(buf,"egnos nrw sbas frame received at"))
			checkSBAS(buf,rm_sat2,time);

		// ошибки sisnet
        if(strstr(buf, "sisnet message timeout occured"))	// после этого БЗК генерит нулевое сообщение (при этом чуть позже еще может придти sisnet)
			SBAS_timeout = 1;
		if(strstr(buf, "sisnet_receive_data: read failed"))	// разорвано соединение с сервером, БЗК в этом случае вылетает
			SBAS_timeout = -1;

		// параметры радиосигнала (сообщение RANGE приемника)
		if(strstr(buf,"gensignal std:"))
			rm_set(buf,rm_gen1);
		if(strstr(buf,"gensignal nrw:"))
			rm_set(buf,rm_gen2);
		if(strstr(buf,"satsignal std:"))
			rm_set(buf,rm_sat1);
		if(strstr(buf,"satsignal nrw:"))
			rm_set(buf,rm_sat2);
		if(strstr(buf,"egnos nrw:"))
			rm_set(buf,rm_sat2);

		// отклонение шкалы времени приемника
		buf2float(buf,"space time offset=",20,rectimeoffset);

		// результаты усредненного расчета скорости кода
		buf2float(buf,"psrrate_gen = ",13,rm_gen1.psrrate);
		buf2float(buf,"psrrate_sat = ",13,rm_sat1.psrrate);
		buf2float(buf,"psrrate_egnos = ",13,rm_sat2.psrrate);

		// данные по спутникам gps
		if(strstr(buf,"satelites being observed"))
		{
#ifndef WINDOWS_DEBUG
			if (!gzgets(iFile, buf, bufsize)) {
				if (gzeof(iFile))
#else
			if (!fgets(buf, bufsize, iFile)) {
				if (feof(iFile))
#endif
					printf("End of file reached\n");
		        else printf("Error: can't read input file\n");
				break;
            }
			if (gps.fill(buf,(int)time))
			{
				gps.calc_mean();
				gps.calc_noise();
			}


		    /*if (out_gps && oFile3)						// отсечь 5 секунд предыдущих суток, когда файл еще не создан
		    //{
		    	//gps.output(oFile3);
		    	gps.calibrate();
		    	gps.calibrate2();
		    //}*/
		}

		// приход сообщения с генератора (всегда синхронно с ШВ GPS), заканчиваем обработку текущей секунды
		tmp = strstr(buf,"siggen:");
		if (tmp)
		{
			buf2float(buf,"delay=",10,sig_delay);
			buf2float(buf,"dFreq=",10,sig_freq);
			buf2float(buf,"dChip=",10,sig_chiprate);

            if (out_txt)
            {
             	//report_set_lc(time,(float)SBAS_sat1,rm_sat1.phasedev,rm_sat1.CNo,rm_sat1.delta_dopp,fabs(rm_sat1.dopp+psrrate_sat));
            	report_set_rs(rm_gen1,rm_gen2,rm_sat1,rm_sat2);
            }

			// вывод данных в лог (за исключением первых секунд, в течение которых набирается статистика)
			if (cur_t.dtime>=start_t.dtime)
			{
				fprintf(oFile,"%d,\t",(int)time);
				compact_float_print(oFile,SBASdelay,3);
				compact_float_print(oFile,rm_gen1.delay,3);
				compact_float_print(oFile,rm_gen2.delay,3);
				compact_float_print(oFile,rm_sat1.delay,3);
				compact_float_print(oFile,rm_sat2.delay,3);

				fprintf(oFile,"\t%d,%d,%d,%d,%d,%d,\t",siggen_error,rm_gen1.SBAS,rm_gen2.SBAS,rm_sat1.SBAS,rm_sat2.SBAS,SBAS_timeout);

				print_rm(oFile,rm_gen1);
				print_rm(oFile,rm_gen2);
				print_rm(oFile,rm_sat1);
				print_rm(oFile,rm_sat2);

				fprintf(oFile,"%e,%.1f,%.1f,%.1f\t",rectimeoffset,rm_gen1.psrrate,rm_sat1.psrrate,rm_sat2.psrrate);
				fprintf(oFile,"%d,%.1f,%.1f\t",(int)sig_delay,sig_freq,sig_chiprate);
				fprintf(oFile,"%.1f,%d,%.1f\n",gps.mean_cn0,gps.visible,gps.noise);
			}

			// увеличиваем указатель на новое сообщение (пригодится в следующем цикле)
			SBAScount++;					
			if (SBAScount>=SBASmemsize) SBAScount=0;

			SiggenTime = time;

			// обнуляем переменные для нового цикла
			SBASdelay = 0.0;
			SBAS_timeout = 0;
			siggen_error = 0;
			rectimeoffset = 0.0;
			sig_delay = sig_freq = sig_chiprate = 0.0;

			rm_reset(rm_gen1);
			rm_reset(rm_gen2);
			rm_reset(rm_sat1);
			rm_reset(rm_sat2);
		}
	}
}



///////////////////////////////////////////////////////////////////


/*void calc_dopp_chiprate(range_message &rm)
{
	if (rm.dopp==0.0 || rm.psrrate==0.0)
		return;
	rm.dopp_chiprate = rm.dopp - rm.psrrate;
}*/


void checkSBAS(char *buf, range_message &rm, float time)
{
	float memtime;
	char tmpSBAS[SBASsize+1];

	getSBAS(tmpSBAS,buf+20);
	bool flag = true;
	if (strstr(buf,"wrong"))
		flag = false;
	rm.SBAS = calc_SBAS_status (tmpSBAS,flag,rm.parity,memtime);
	if (memtime!=0.0)
		rm.delay = time - memtime;
	if (rm.delay<0)
		rm.delay+=86400;
}


// ищет совпадение текущего сообщения SBAS с записанными в памяти
// возвращает номер ячейки совпавшего сообщения
// определяет частичное совпадение при искажении не более 10 символов
// при отсутствии совпадений возвращает -1
int SBAS_finder (char *curSBAS, int &best_dif)
{
	best_dif = 1000;
	int best_compare = -1; 

	for (int i=0; i<SBASmemsize; i++)
	{
		int dif = SBAS_comparator(curSBAS,SBAS[i]);
		if (dif<best_dif)
		{
			best_dif = dif;
			best_compare = i;
		}
	}
	return best_compare;
}


// возвращает значение равное числу символов, отличающихся двух сообщениях SBAS
int SBAS_comparator (char *SBAS1, char *SBAS2)
{
	int counter = 0;
	for (int i=0; i<64; i++)
	if (SBAS1[i]!=SBAS2[i])
		counter++;
	return counter;
}


void getSBAS(char *tmpSBAS, char *buf)
{
	char *tmp = strrchr(buf,':');
	strncpy(tmpSBAS,tmp+2,SBASsize);
	tmpSBAS[SBASsize] = '\0';
}


int calc_SBAS_status (char *tmpSBAS, bool bzk_SBAS_status, bool bzk_parity_flag, float &memtime)
{
	int dif;
	int mem_pointer = SBAS_finder(tmpSBAS, dif);
	
	// если не найдено предыдущее сообщение (такое бывает в начале лог-файла)
	if (mem_pointer==-1)
		memtime = 0.0;
	else
		memtime = SBAStime[mem_pointer];

	if (bzk_SBAS_status)		// верное сообщение
		return 0;
	else
	{
		if (bzk_parity_flag)	// неверное сообщение, верная КС => критично: искаженная информация, пропуск ошибки потребителем!
		{
			if (strncmp(tmpSBAS,"C6",2) && strncmp(tmpSBAS,"53",2) && strncmp(tmpSBAS,"9A",2))		// дополнительная проверка: если неправильная преамбула, потребитель отбросит сообщение
				return -1;
			if (strncmp(tmpSBAS+3,"FC",2))															// дополнительная проверка: КЗиК сформировал пустое сообщение из-за несвоевременного прихода Sisnet
				return -1;
			printf("\ncalc_SBAS_status ERROR: wrong message, right parity - %s",tmpSBAS);
			return -90;
		}
		return -1;				// неверное сообщение, искаженное при передаче, но потребитель его отбросит
	}
}

