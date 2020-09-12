#include <stdio.h>
#include <string.h>
#include <iostream>		// windows for FILE
#include <stdlib.h>
#include <math.h>

#include "recursive_mean.h"
#include "gnss.h"


gnss::gnss()
{
    for (int i=0; i<gpslen; i++)
        sat[i].prn = i+1;
    sat[30].prn = 123;
    sat[31].prn = 125;
    sat[32].prn = 140;

    time = 0;
    visible = 0;
    mean_cn0 = 0.0;

    ext_db_flag = false;
    //read_dbcn0();
    noise = 0.0;
}



void gnss::set (int prn, double cn0, double el)
{
    for (int i=0; i<satlen; i++) {
        if (sat[i].prn == prn) {
            sat[i].cn0 = cn0;
            sat[i].el = el;
        }
    }
}



void gnss::reset (int _time)
{
	time = _time;
    for (int i=0; i<satlen; i++) {
    	sat[i].cn0 = 0;
    	sat[i].el = 0;
    }
}



void gnss::output(FILE *oFile)
{
    fprintf(oFile,"%d",time);
    for (int i=0; i<satlen; i++) {
        fprintf(oFile,", %d",sat[i].prn);
        if (sat[i].cn0==0.0)
            fprintf(oFile,",0");
        else
            fprintf(oFile,",%.1f",sat[i].cn0);
        if (sat[i].el==0.0)
            fprintf(oFile,",0");
        else
            fprintf(oFile,",%.1f",sat[i].el);
    }
    fprintf(oFile,"\n");
}



int gnss::fill (char *buf, int _time)
{
	char tmp[100];
    int prn;
    double cn0, el;

    char *c1 = strchr(buf,'(');         // сдвигаем строку к первому КА
	if (c1-buf != 30)					// отметаем ситуацию, когда по какой-то причине идет не та строка
    {
		//printf("\n\nc1=%d buf=%d >> %s\n",c1,buf,buf);
		return 0;
    }

    bool first = true;

    // проходим строку, пока не закончатся КА
    while (c1!=NULL)
    {
        c1++;                           // избавляемся от скобки

        strncpy(tmp,c1,3);
        tmp[3]='\0';
        prn = atoi(tmp);

        // отсечь данные с приемника Земля
        if (first) {
            if (prn>=120)
                return 0;
            first = false;
            reset(_time);
        }

        char *c2 = strchr(c1,',')+2;
        strncpy(tmp,c2,10);
        tmp[10]='\0';
        cn0 = atof(tmp);

        strncpy(tmp,c2+7,10);
        tmp[10]='\0';
        el = atof(tmp);

        set(prn,cn0,el);

        c1 = strchr(c1,'(');            // следующий КА
    }
    //calc_mean();
    //calc_noise();
    return 1;
}



void gnss::calc_mean()
{
	mean_cn0 = 0;
	visible = 0;

    for (int i=0; i<gpslen; i++)
    	if (sat[i].cn0 != 0.0)
    	{
    		visible++;
    		mean_cn0 += sat[i].cn0;
    	}

    if (visible)
    	mean_cn0 = mean_cn0/visible;

    //printf("\nmean_cn0=%.1f (%d)",mean_cn0,visible);
}



// вычисление среднего отношения сигнал/шум для каждого угла возвышения
void gnss::calibrate()
{
	float step = 90.0/ellen;

	for (int i=0; i<gpslen; i++)
	    if (sat[i].cn0 != 0.0)
	    {
	    	int j_el = (int)(sat[i].el/step);
	    	db_cn0[j_el].add(sat[i].cn0);
	    }
}



void gnss::dbcn0_output()
{
	FILE *oFile;
	oFile = fopen("cn0_db.log","w");
	if (!oFile)
	{
		printf("error: can't open cn0_db output file\n");
		return;
	}

	float step = 90.0/ellen;
	for (int i=0; i<ellen; i++)
	{
		int left = i*step;
		int right = left+step;
		fprintf(oFile,"\n%2d-%2d  %4.1f  %4.1f  %4.1f  %d",left,right,db_cn0[i].val,db_cn0[i].min_val,db_cn0[i].max_val,db_cn0[i].counter);
	}
}



// вычисление среднего отношения сигнал/шум для каждого угла возвышения
void gnss::calibrate2()
{
	float step = 90.0/ellen;

	for (int i=0; i<gpslen; i++)
	    if (sat[i].cn0 != 0.0)
	    {
	    	int j = (int)(sat[i].el/step);
	    	sat[i].db_cn0[j].add(sat[i].cn0);
	    }
}



void gnss::dbcn0_output2()
{
	FILE *oFile;
	oFile = fopen("cn0_db2.log","w");
	if (!oFile)
	{
		printf("error: can't open cn0_db output file\n");
		return;
	}

	//float step = 90.0/ellen;
	for (int i=0; i<gpslen; i++)
	{
		fprintf(oFile,"\n%2d",i+1);
		for (int j=0; j<ellen; j++)
		{
			//int left = i*step;
			//int right = left+step;
			//fprintf(oFile,"\n%2d-%2d  %4.1f  %4.1f  %4.1f  %d",left,right,db_cn0[i].val,db_cn0[i].min_val,db_cn0[i].max_val,db_cn0[i].counter);
			fprintf(oFile," %4.1f",sat[i].db_cn0[j].val);
		}
	}
}



void gnss::calc_noise()
{
	recursive_mean mean_dcn0;
	float step = 90.0/ellen;

	for (int i=0; i<gpslen; i++)					// перебираем все видимые спутники
	{
		float cn0 = sat[i].cn0;
		float el = sat[i].el;
		if (cn0 == 0.0)
			continue;

		int j1 = (int)(el/step);					// номер интервала по углу месту из БД (значение из БД актуально для середины интервала)
		float cn01 = ext_cn0[i][j1];				// середина текущего интервала
		if (cn01 == 0.0)							// если в базе нет данных по нужному интервалу, сразу прекращаем
			continue;

		float el1 = j1*step+step/2;					// середина текущего интервала

		int j2 = j1;								// номер ближайшего соседнего интервала
		if (el<el1 && j1!=0)
			j2 = j1-1;
		else if (el>el1 && j1!=ellen-1)
			j2 = j1+1;

		float cn02 = ext_cn0[i][j2];				// середина соседнего интервала
		if (cn02 == 0.0)
			continue;

		float k = fabs(el-el1)/step;				// коэффициент, определяющий положение значения в интервале
		float interp_cn0 = cn01 + k*(cn02-cn01);	// интерполированное значение сигнал/шум для текущего угла места
		float dcn0 = interp_cn0-cn0;				// отклонение текущего значения от значения из БД
		mean_dcn0.add(dcn0);						// учытиваем полученное значение при вычиследнии среднего
	}
	//printf("\nmean_dcn0=%.1f",mean_dcn0.val);
	noise = mean_dcn0.val;
}



void gnss::read_dbcn0()
{
	FILE *iFile;
	iFile = fopen("cn0_db2.log","r");
	if (!iFile)
	{
		printf("error: can't open cn0_db input file\n");
		return;
	}

	const int bufsize = 128;
	char buf[100];
	//for (int i=0; i<gpslen; i++)
	while(1)
	{
		if (!fgets(buf, bufsize, iFile))
		{
			if (feof(iFile))
			{
				fclose(iFile);
				return;
			}
		}

		char tmp[8];
		strncpy(tmp,buf,2);
		tmp[2]='\0';
		int i = atoi(tmp);
		//printf("\n%d ",i);

		for (int j=0; j<ellen; j++)
		{
			strncpy(tmp,buf+2+j*5,5);
			tmp[5]='\0';
			ext_cn0[i][j] = atof(tmp);
			//printf(" %4.1f",ext_cn0[i][j]);
		}
	}
	ext_db_flag = true;
}
