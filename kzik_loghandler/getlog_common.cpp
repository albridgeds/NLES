#include <iostream>		// windows for FILE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <zlib.h>

#include "mytime.h"
#include "getlog_common.h"



void buf2float (char *buf, const char name[], int len, double &value)
{
	char *tmp;
	tmp = strstr(buf,name);
	if (tmp)
	{
		char tmp2[100];
		int namelen = strlen(name);
		strncpy(tmp2,tmp+namelen,len);
		tmp2[len]='\0';
		value = atof(tmp2);
	}
}


void buf2int (char *buf, const char name[], int len, int &value)
{
	char *tmp;
	tmp = strstr(buf,name);
	if (tmp)
	{
		char tmp2[100];
		int namelen = strlen(name);
		strncpy(tmp2,tmp+namelen,len);
		tmp2[len]='\0';
		value = atoi(tmp2);
	}
}



void compact_float_print (FILE* outFile, double fval, int accuracy)
{
	if (accuracy==0)
		fprintf(outFile,"%d,",(int)fval);
	else
	{
		if (fval==0.0)
			fprintf(outFile,"0,");		// ��� ����� ��������� �������
		else if (accuracy==1)
			fprintf(outFile,"%.1f,",fval);
		else if (accuracy==2)
			fprintf(outFile,"%.2f,",fval);
		else if (accuracy==3)
			fprintf(outFile,"%.3f,",fval);
	}
}


/////////////////////////////////////////////////////////////////////


bool no_input_time(my_time start_t, my_time end_t)
{
	if (start_t.year != -1) return false;
	if (start_t.month != -1) return false;
	if (start_t.day != -1) return false;
	if (end_t.day != -1) return false;
	if (start_t.hour != -1) return false;
	if (end_t.hour != -1) return false;
	return true;
}



FILE *create_bzk_outfile (my_time cur_t, const char *end, bool standard_infile, char *ifilename)
{
	int day = cur_t.day;
	int month = cur_t.month;
	int year = cur_t.year - (cur_t.year/100)*100;
	int bzknum = 0;
	char refname[maxonamelen];

	if (standard_infile)
#ifndef WINDOWS_DEBUG
		gethostname(refname,maxonamelen);		// ����� ��������� ��� �����, �� ������� �������� ���������
#else
		return 0;								// � windows �� ����� �������� �� ����������� ������
		//strcpy(refname,"bzk00");				// ��������� ��������
#endif
	else strcpy(refname,ifilename);				// ����� ��������� ��� �������� �����

	// �������� ����� ���� "bzk12..."    		!!! �� �������� � ������� (���� ./zzz) ��� ���� ���� � ������ �����
	if (!strncmp(refname,"bzk",2))
	{
		char tmp[2];
		int num1, num2;
		strncpy(tmp,refname+3,1);
		num1 = atoi(tmp);
		strncpy(tmp,refname+4,1);
		num2 = atoi(tmp);
		if (num1>0 && num1<=3 && num2>0 && num2<=2)
			bzknum = num1*10+num2;
	}

	char ofilename[maxonamelen];
	sprintf(ofilename,"bzk%02d-%02d%02d%02d%s",bzknum,year,month,day,end);
	FILE *oFile = fopen(ofilename,"w");
	if (!oFile)
	{
		printf("error: can't create output file %s\n",ofilename);
		return 0;
	}
	printf("\nbzk %s output file: %s\n",end,ofilename);
	return oFile;
}


FILE *create_outfile (my_time cur_t, const char *end, char *hostname)
{
	int day = cur_t.day;
	int month = cur_t.month;
	int year = cur_t.year - (cur_t.year/100)*100;

	char ofilename[maxonamelen];
	sprintf(ofilename,"%s-%02d%02d%02d%s",hostname,year,month,day,end);
	FILE *oFile = fopen(ofilename,"w");
	if (!oFile)
	{
		printf("error: can't create output file %s\n",ofilename);
		return 0;
	}
	printf("\n%s %s output file: %s\n",hostname,end,ofilename);
	return oFile;
}



int getStandardFileName (char *baseName, char *resultName, int step)
{
	if (step<0 || step>10)
	{
		//printf("\ngetStandardFileName error: wrong step %d\n",step);
		return -1;
	}

	if (step==0)
		strcpy(resultName,baseName);
		//sprintf(resultName,"%s",baseName);
	else if (step==1)
		sprintf(resultName,"%s.%d",baseName,step);
	else
		sprintf(resultName,"%s.%d.gz",baseName,step);
	return 0;
}



#ifndef WINDOWS_DEBUG
gzFile openNextStandardFile(char *baseName, int &filenum)
#else
FILE *openNextStandardFile(char *baseName, int &filenum)
#endif
{
	char iFileName[maxinamelen];
	filenum--;
	int res = getStandardFileName(baseName,iFileName,filenum);
	if (res)
	{
		//printf("\nlast file finished");
		return 0;
	}

#ifndef WINDOWS_DEBUG
	gzFile file = gzopen64(iFileName,"r");
#else
	FILE *file = fopen(iFileName,"r");
#endif
	if (!file)
	{
		int err_code = errno;
		printf("error: can't open next standard input file: %s (%s)\n",iFileName,strerror(err_code));
		return 0;
	}
	printf("read next input file %s\n",iFileName);
	return file;
}



#ifndef WINDOWS_DEBUG
gzFile start_standart_infile (my_time start_t, my_time end_t, char *baseName, int &filenum)
{
	gzFile file;
#else
FILE *start_standart_infile (my_time start_t, my_time end_t, char *baseName, int &filenum)
{
	FILE *file;
#endif

	bool found = false;
	char iFileName[300];

	printf("\n\nlooking for start of test period\n");
	for (filenum=0; filenum<10; filenum++)
	{
		int res = getStandardFileName(baseName,iFileName,filenum);
		if (res)
			return 0;

#ifndef WINDOWS_DEBUG
file = gzopen64(iFileName,"r");
#else
file = fopen(iFileName,"r");
#endif
		if (!file)
		{
			int err_code = errno;
			printf("\nerror: can't open standard input file: %s (%s)",iFileName,strerror(err_code));
			return 0;
		}

		printf("\nread input file %s",iFileName);

		// ����� ���� ������, �������������� ������� ������, ����� ������� �������� ������ ��� ������
		my_time shift_t;
		if (start_t.get_syslog()) shift_t.set_syslog(start_t.syslog_year);
		shift_t.shift(start_t.dtime,-5);
		shift_t.print_full("shifted time ");

		res = start_seek(file,shift_t,end_t);
		if (res==-1)
		{
			printf("\nstart_seek error, exit program");
			return 0;
		}
		if (res==3)
		{
			printf("\nstart of required interval found");
			break;
		}
		if (res==2 && found)
		{
			printf("\nonly end of required interval may be found, stop seek");
			break;
		}
		if (res==2 && !found)
		{
			found = true;
			printf("\nend of required interval found, looking for previous file");
			continue;
		}
		if (res==5 && found)
		{
			filenum -= 2;
			printf("\nend of required interval found");
			continue;
		}
		if (res==5 && !found)
		{
			printf("\ninterval not found, exit program");
			return 0;
		}
	}
	return file;
}



#ifndef WINDOWS_DEBUG
gzFile start_nonstandart_infile (my_time start_t, my_time end_t, char *ifilename)
{
	gzFile file = gzopen64(ifilename,"r");
#else
FILE *start_nonstandart_infile (my_time start_t, my_time end_t, char *ifilename)
{
	FILE *file = fopen(ifilename,"r");
#endif

	if (!file)
	{
		int err_code = errno;
		printf("error: can't open nonstandard input file: %s (%s)\n",ifilename,strerror(err_code));
		return 0;
	}
	printf("\n\nread input file %s\n",ifilename);

	// ����� ���� ������, �������������� ������� ������, ����� ������� �������� ������ ��� ������
	my_time shift_t;
	if (start_t.get_syslog()) shift_t.set_syslog(start_t.syslog_year);
	shift_t.shift(start_t.dtime,-5);
	shift_t.print_full("shifted time ");
	int res = start_seek(file,shift_t,end_t);
	if (res==-1)
	{
		printf("start_seek error, exit program\n");
		return 0;
	}
	if (res<2 || res>4)
	{
		printf("\ninterval not found, exit program");
		return 0;
	}
	return file;
}



/////////////////////////////////////////////////////////////////////////////



// ��������� �������� ������������ ���������� ��������� ������� ������������ �����:
// ������� 1: ��������� �������� ������� ������ ������ ����� - ���� �������� ���������� ����
// ������� 2: ������ ����� ����� ������ ���������� ��������� ������� - ����� ������������� �������� ���������� ����
// ������� 3: ��������� �������� ������� ��������� ����� ������ �����, ������� ������ ��������� � ���� �����
// ������� 4: ����� ����� ����� ������ ���������� ��������� ������� - ���� ������ ��������� � ���� �����, ����� ����� ������������� �������� ��������� ����
// ������� 5: ��������� �������� ������� ����� ��������� ����� - ���� �������� ��������� ����

#ifndef WINDOWS_DEBUG
int start_seek (gzFile file, my_time start_time, my_time end_time)
#else
int start_seek (FILE *file, my_time start_time, my_time end_time)
#endif
{
    printf("looking for start of test period\n");
    const int bufsize = 10000;
    char buf[bufsize];
    char *cres;
    int res;

#ifndef WINDOWS_DEBUG
	cres = gzgets(file,buf,bufsize);
	if (!cres) {
		if (gzeof(file)) {
#else
	cres = fgets(buf,bufsize,file);
	if (!cres) {
		if (feof(file)) {
#endif
			printf("file is empty, going to previous file\n");
			return 1;
		}
		int err_code = errno;
		printf("first line check: error in gzgets (%s)\n",strerror(err_code));
		return -1;
    }

	my_time cur_time;
	if (start_time.get_syslog()) cur_time.set_syslog(start_time.syslog_year);
	cur_time.set_log(buf);
	cur_time.print_full("first line\n");
	printf("buf= %s\n",buf);

	if (cur_time.dtime > end_time.dtime)
	{
		printf("case 1: file is too late\n\n");
		return 1;
	}

	if (cur_time.dtime > start_time.dtime)
	{
		printf("case 2: file start inside required interval\n\n");
		return 2;
	}

	int seek_leap = 100000;
	int max_seek_leap = 100000000;
	int min_seek_leap = 1000;
	int step = 1;

	// step==1 - ����������� �������� ������ �� �������� (����� ����� ����� ��� ���������)
	// step==2 - ������ ����� ���������, ������ ����� ������ ��������� �� ��������
	// step==3 - ����� �� ������� �����, �������� ����� �� ��������
	// step==4 - ������ ����� ����� �����, �������� ������ ��� �������
	// step==5 - ������ ����� ������ ���������, �������� ������ ��� �������

	int old_date;

	while(1)
	{
		if (step<4)				// если нужно делать скачки (грубый поиск)
		{

#ifndef WINDOWS_DEBUG
			res = gzseek64(file,seek_leap,SEEK_CUR);
#else
			res = fseek(file,seek_leap,SEEK_CUR);
#endif
			if (res==-1)
			{
				int err_code = errno;
				printf("end line check (seek_leap=%d): error in fseek (%s)\n",seek_leap,strerror(err_code));
				return -1;
			}

#ifndef WINDOWS_DEBUG
			cres = gzgets(file,buf,bufsize);
			if (cres==NULL) {
				if (gzeof(file)) {
#else
			cres = fgets(buf,bufsize,file);
			if (cres==NULL) {
				if (feof(file)) {
#endif
					//printf("end line check (step=%d, seek_leap=%d, res=%d): eof in pre- gzgets\n",step,seek_leap,res);
					printf("  cur_date=xxxxxxxx\n");
					if (abs(seek_leap)>min_seek_leap)
					seek_leap = -abs(seek_leap)/2;			// уменьшаем шаг
					cur_time.print_full("step->3");
					step = 3;
					continue;
				}
				//int err_code = errno;
				//printf("end line check (step=%d, seek_leap=%d, res=%d): error in pre- gzgets (%s)\n",step,seek_leap,res,strerror(err_code));
				return -1;
			}
		}

#ifndef WINDOWS_DEBUG
		cres = gzgets(file,buf,bufsize);
		if (cres==NULL) {
			if (gzeof(file)) {

#else
		cres = fgets(buf,bufsize,file);
		if (cres==NULL) {
			if (feof(file)) {
#endif
				if (step==4 || step==5)		// окончание файла до начала интервала
				{
					cur_time.print_full("end line");
					return 5;
				}

				printf("end line check (step=%d, seek_leap=%d, res=%d): eof in gzgets\n",step,seek_leap,res);
				if (abs(seek_leap)>min_seek_leap)
					seek_leap = -abs(seek_leap)/2;			// уменьшаем шаг
				cur_time.print_full("step->3");
				step = 3;
				continue;
			}
			int err_code = errno;
			printf("end line check (step=%d, seek_leap=%d, res=%d): error in gzgets (%s)\n",step,seek_leap,res,strerror(err_code));
			return -1;
		}

		int res = cur_time.set_log(buf);
			if (res) continue;
		if (old_date != cur_time.yday)
		{
			cur_time.print_full("");
			printf("step=%d  seek_leap=%d",step,seek_leap);
			old_date = cur_time.yday;
		}


		// если получили первую оценку после выхода за пределы файла
		if (step==3)
		{
			// отсекаем вариант 5: требуемый интервал времени позже окончания файла - смотрим следующий файл
			if (cur_time.dtime < start_time.dtime)
			{
				cur_time.print_full("step->5");
				step = 5;
				continue;
			}

			// если интервал все же лежит внутри файла, продолжаем движение назад
			cur_time.print_full("step->2");
			step = 2;
			continue;
		}


		// если точно попали в нужную дату и время, заканчиваем поиск
		if (cur_time.dtime == start_time.dtime)
		{
			cur_time.print_full("start found (exit 3)\n");
			return 3;
		}

		// если движемся вперед без скачков и оказались позднее начала старта
		if (step==4 || step==5)
		{
			if (cur_time.dtime > start_time.dtime)
			{
				if (cur_time.dtime < end_time.dtime)
				{
					cur_time.print_full("only middle can be found (exit 3)\n");
					return 3;																// найдено начало реального интервала внутри искомого
				}
				else
				{
					cur_time.print_full("interval cannot be found (exit 5)\n");
					return 5;																// иначе искомый интервал не может быть найден
				}
			}

		}

		// ���� ��������� ���� ������ �������� �������, �� ����������� ������ ����� � �������� ��������� ������ � ����������� ���������
		if (step < 4)
		{
			int dtime = start_time.dtime - cur_time.dtime;
			if (dtime>0 && dtime<7200)
			{
				cur_time.print_full("close to start, moving slowly (step->5)");
				step = 5;
				continue;
			}
		}

		// ���� ������� ����� ������ ��������
		if (cur_time.dtime > start_time.dtime)
		{
			if (step<4)
			{
				//printf("     move backward (step %d->2)",step);
				step=2;
			}
			if (abs(seek_leap)>min_seek_leap)
				seek_leap = -abs(seek_leap)/2;			// �������� ����� � ����������� ����
			else
				seek_leap = -abs(seek_leap);
		}

		// ���� ������� ����� ������ ��������
		else
		{
			if (step==1)
			{
				if (abs(seek_leap)<max_seek_leap)
					seek_leap = abs(seek_leap)*2;		// ���� �������� ������ �� ������ �����, ����������� ���
			}
			else
			{
				if (abs(seek_leap)>min_seek_leap)
					seek_leap = abs(seek_leap)/2;		// ���� ���� �������� �� ������ �����, �������� ������ � ����������� ����
				else
				{
					//printf("     close to start, moving slowly (step %d->5)",step);
					step=5;								// ���� ��� ��� �����������, �������� ������ ���������
				}
			}
		}
	}
    return -1;
}


// ��� ����� �� �������� �����
// ������ ���� ������ - ����� ������ ���� �� ������� ��������� �������, �� ���-�� �� ����������� � ������ ftell/fseek
#ifndef WINDOWS_DEBUG
void getHostname(gzFile file, char *hostname, bool bSyslog)
#else
void getHostname(FILE *file, char *hostname, bool bSyslog)
#endif
{
    const int bufsize = 10000;
    char buf[bufsize];
    char *cres;

#ifndef WINDOWS_DEBUG
	cres = gzgets(file,buf,bufsize);
	if (!cres) {
		if (gzeof(file)) {
#else
	cres = fgets(buf,bufsize,file);
	if (!cres) {
		if (feof(file)) {
#endif
			printf("\ngetHostname: file is empty");
			return;
		}
		int err_code = errno;
		printf("\ngetHostname: error in gzgets (%s)\n",strerror(err_code));
		return;
    }

	if (bSyslog)
		strncpy(hostname,buf+16,5);
	else
		strncpy(hostname,buf+24,5);
	//printf("\nhost = <%s>\n",hostname);
}
