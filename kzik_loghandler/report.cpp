#include <iostream>		// windows for FILE
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "range_message.h"
#include "recursive_mean.h"
#include "lost_counter.h"
#include "report.h"


lost_counter lc_lmsg_sat1(1,0);
lost_counter lc_cmsg_sat1(2,0);
lost_counter lc_phase_sat1(1,phasedev_lim);
lost_counter lc_CNo_sat1(2,CNo_lim);
lost_counter lc_dopp_sat1(1,dopp_lim);
lost_counter lc_dfreq_sat1(1,dfreq_lim);

report_stat rs_gen1, rs_gen2, rs_sat1, rs_sat2;
SBAS_message sm_gen1, sm_gen2, sm_sat1, sm_sat2;



void report_out (FILE *oFile2)
{
	printf("\n\nstart report_out\n");
	if (!oFile2)
	{
		printf("report out: no oFile2 exist\n");
		return;
	}

	fprintf(oFile2,"SBAS message control\n\n");

	fprintf(oFile2,"\nReceiver 'Ground' standard:\n");
	print_sm(oFile2,sm_gen1);
	fprintf(oFile2,"\nReceiver 'Ground' narrow:\n");
	print_sm(oFile2,sm_gen2);
	fprintf(oFile2,"\nReceiver 'Space' standard:\n");
	print_sm(oFile2,sm_sat1);
	fprintf(oFile2,"\nReceiver 'Space' narrow:\n");
	print_sm(oFile2,sm_sat2);

	fprintf(oFile2,"_________________________________________________\n\n");
	fprintf(oFile2,"Statistic                                 Mean      Minimum    Maximum     Limit\n\n");
	fprintf(oFile2,"Receiver 'Ground' standard:\n");
	print_mean(oFile2,rs_gen1);
	fprintf(oFile2,"Receiver 'Ground' narrow:\n");
	print_mean(oFile2,rs_gen2);
	fprintf(oFile2,"Receiver 'Space' standard:\n");
	print_mean(oFile2,rs_sat1);
	fprintf(oFile2,"Receiver 'Space' narrow:\n");
	print_mean(oFile2,rs_sat2);


	// Вывод в отчет временных интервалы нарушений в текстовом виде - не очень удобно, графики намного нагляднее
	/*printf("\ncreating text report (lc)\n");
	fprintf(oFile2,"_________________________________________________\n\n");
	fprintf(oFile2,"Приемник 'Космос' стандартный:\n\n");

	lost_counter lc_total[6];
	lc_total[0].copy(lc_lmsg_sat1);
	lc_total[1].copy(lc_cmsg_sat1);
	lc_total[2].copy(lc_phase_sat1);
	lc_total[3].copy(lc_CNo_sat1);
	lc_total[4].copy(lc_dopp_sat1);
	lc_total[5].copy(lc_dfreq_sat1);

	//print_lc_total(oFile2,lc_lmsg_sat1,lc_cmsg_sat1,lc_phase_sat1,lc_CNo_sat1);
	print_lc_total(oFile2,lc_total);

	//fprintf(oFile2,"интервалы потери данных\n\n");
	//print_lc(oFile2,lc_lmsg_sat1);
	//fprintf(oFile2,"интервалы скачков фазы\n\n");
	//print_lc(oFile2,lc_phase_sat1);
	//fprintf(oFile2,"интервалы снижения сигнал/шум\n\n");
	//print_lc(oFile2,lc_CNo_sat1);*/

	printf("report_out finished\n");
}






void print_sm (FILE *file, SBAS_message sm)
{
	float total = sm.ok + sm.lost + sm.corrupted + sm.critical_corrupted;

    fprintf(file,"- OK                                       %5d (%5.2f%%)\n",sm.ok, static_cast<float>(sm.ok)/total*100);
    fprintf(file,"- lost                                     %5d (%5.2f%%)\n",sm.lost,(float)sm.lost/total*100);
    fprintf(file,"- corrupted                                %5d (%5.2f%%)\n",sm.corrupted,(float)sm.corrupted/total*100);
	fprintf(file,"- critical corrupted (no parity error)     %5d\n",sm.critical_corrupted);
	//fprintf(file,"- в том числе с неверной КС на входе: %5d\n\n",sm.wrongSisnet);
	//fprintf(file,"Потеряно не менее %d подряд:          %5d (%5.2f%)\n\n",maxsimlost,sm.simlost,(float)sm.simlost/total*100);
	//fprintf(file,"Ошибок БЗК при успешном приеме:       %5d\n",sm.error_ok);
	//fprintf(file,"Ошибок БЗК при потере:                %5d\n",sm.error_lost);
	//fprintf(file,"Ошибок БЗК при искажении:             %5d\n\n\n",sm.error_corrupt);
}



void print_mean (FILE *file, report_stat rs)
{
	fprintf(file,"- signal to noise                   %10.2f  %10.2f  %10.2f      %5.2f       %5d\n",rs.mCNo.val,rs.mCNo.min_val,rs.mCNo.max_val,CNo_lim,rs.CNo_counter);
	fprintf(file,"- frequency                         %10.1f  %10.1f  %10.1f\n",rs.mdopp.val,rs.mdopp.min_val,rs.mdopp.max_val);
	fprintf(file,"- chiprate                          %10.1f  %10.1f  %10.1f\n",rs.mchiprate.val,rs.mchiprate.min_val,rs.mchiprate.max_val);
	fprintf(file,"- frequency-chiprate                %10.1f  %10.1f  %10.1f\n",rs.ddopp.val,rs.ddopp.min_val,rs.ddopp.max_val);
	fprintf(file,"- pseudorange deviation             %10.2f  %10.2f  %10.2f\n",rs.mpsrdev.val,rs.mpsrdev.min_val,rs.mpsrdev.max_val);
	fprintf(file,"- phase deviation                   %10.3f  %10.3f  %10.3f      %5.3f\n\n",rs.mphasedev.val,rs.mphasedev.min_val,rs.mphasedev.max_val,phasedev_lim);
}




void print_lc (FILE *file, lost_counter lc)
{
	fprintf(file," # |  Начало  | Окончание | Длительность (c)\n");

	int size = cl_size;
	if (lc.counter<size) size=lc.counter+1;

	char start[20], end[20];
	for (int i=0; i<size; i++)
	{
		time_to_string (lc.start[i],start);
		time_to_string (lc.end[i],end);
		fprintf(file,"%2d | %s | %s  | %5d\n",i,start,end,lc.duration[i]);
	}
}



void print_lc_total (FILE *file, lost_counter lc[6])
{
	const int lcs = 6;
	int size[lcs];
	int counter[lcs];
	char start[20], end[20];

	for(int i=0; i<lcs; i++)
	{
		counter[i] = 0;
		size[i] = cl_size;
		if (lc[i].counter<cl_size)
			size[i] = lc[i].counter;
	}

	int iii=0;
	int last_start=0;
	char message[6][100];

	strcpy(message[0],"Потерянное сообщение");
	strcpy(message[1],"пскаженное сообщение");
	strcpy(message[2],"Высокая девиация фазы");
	strcpy(message[3],"Низкий сигнал/шум");
	//strcpy(message[4],"Высокий допплер");
    strcpy(message[4],"Большое изменение допплера");
	strcpy(message[5],"Высокая разница частоты и скорости кода");

	while(1)
	{

		// завершение цикла по ...
		int sum = 0;
		for(int i=0; i<lcs; i++)
		{
			if (counter[i]>size[i])
				sum++;
		}
		if (sum == lcs)
			break;

		// завершение цикла по окончанию значений старта интервала
		sum = 0;
		for(int i=0; i<lcs; i++)
		{
			if (lc[i].start==0 && counter[i]>0)
				sum++;
		}
		if (sum == lcs)
			break;

		// завершение цикла по превышению количества операций
		if (iii++>lcs*cl_size)
			break;


		int min_start = 86400;
		int pointer = 0;
		for(int i=0; i<lcs; i++)
		{
			int cnt_i = counter[i];
			//int cnt_p = counter[pointer];
			//if (lc[i].start[cnt_i]<lc[pointer].start[cnt_p] && counter[i]<size[i])
			if (lc[i].start[cnt_i]<=min_start && counter[i]<=size[i])
			{
				min_start = lc[i].start[cnt_i];
				pointer = i;
			}
		}

		int cnt = counter[pointer];

		if (lc[pointer].start[cnt]-last_start>100)
			fprintf(file,"\n");
		last_start=lc[pointer].start[cnt];
		time_to_string (lc[pointer].start[cnt],start);
		time_to_string (lc[pointer].end[cnt],end);
		fprintf(file,"%s | %s |  %5d |  %s (%02d/%02d)",start,end,lc[pointer].duration[cnt],message[pointer],cnt,size[pointer]);

		if (pointer>1) fprintf(file,"  | %.3f",lc[pointer].parameter[cnt]);
            fprintf(file,"\n");

		counter[pointer]++;
	}
}




void report_init_sm()
{
	init_sm(sm_gen1);
	init_sm(sm_gen2);
	init_sm(sm_sat1);
	init_sm(sm_sat2);
}


void report_init_lc()
{
	lc_lmsg_sat1.init(1,0);
	lc_cmsg_sat1.init(2,0);
	lc_phase_sat1.init(1,phasedev_lim);
	lc_CNo_sat1.init(2,CNo_lim);
	lc_dopp_sat1.init(1,dopp_lim);
	lc_dfreq_sat1.init(1,dfreq_lim);
}


void report_init_rs()
{
	init_rs(rs_gen1);
	init_rs(rs_gen2);
	init_rs(rs_sat1);
	init_rs(rs_sat2);
}


void set_sm(int SBAS, SBAS_message &sm)
{
	switch (SBAS)
	{
	case 0:
		sm.ok++;
		break;
	case 1:
		sm.lost++;
		break;
	case -1:
		sm.corrupted++;
		break;
	case -90:
		sm.critical_corrupted++;
		break;
	}
}


void report_set_sm (int SBAS_gen1,int SBAS_gen2,int SBAS_sat1,int SBAS_sat2)
{
	set_sm(SBAS_gen1,sm_gen1);
	set_sm(SBAS_gen2,sm_gen2);
	set_sm(SBAS_sat1,sm_sat1);
	set_sm(SBAS_sat2,sm_sat2);
}


void report_set_lc (float time, float SBAS, float phasedev, float CNo, float delta_dopp, float delta_chip_dopp)
{
	lc_lmsg_sat1.set(time,SBAS);
	lc_cmsg_sat1.set(time,SBAS);
	lc_phase_sat1.set(time,phasedev);
	lc_CNo_sat1.set(time,CNo);
	lc_dopp_sat1.set(time,delta_dopp);
    lc_dfreq_sat1.set(time,delta_chip_dopp); 	// chiprate ~= -dopp
}


void set_rs (range_message rm, report_stat &rs)
{
	if (rm.CNo != 0.0)
		rs.mCNo.add(rm.CNo);
	if (rm.dopp != 0.0)
		rs.mdopp.add(rm.dopp);
	if (rm.psrdev != 0.0)
		rs.mpsrdev.add(rm.psrdev);
	if (rm.phasedev != 0.0)
		rs.mphasedev.add(rm.phasedev);
	//if (rm.delta_dopp != 0.0)
	//	rs.ddopp.add(rm.delta_dopp);
	//if (rm.dopp_chiprate != 0.0)
	//	rs.ddopp.add(rm.dopp_chiprate);
	if (rm.psrrate != 0.0 && fabs(rm.psrrate)<1000000.0)
		rs.mchiprate.add(rm.psrrate);

	if (rm.phasedev > phasedev_lim)
		rs.phasedev_counter++;
	if (rm.CNo < CNo_lim && rm.CNo!=0.0)
		rs.CNo_counter++;

	if (rm.dopp!=0.0 && rm.psrrate!=0.0 && fabs(rm.psrrate)<1000000.0)
	{
		float dopp_chiprate = rm.dopp - rm.psrrate;
		rs.ddopp.add(dopp_chiprate);
	}
}


void report_set_rs (range_message rm_gen1, range_message rm_gen2, range_message rm_sat1, range_message rm_sat2)
{
	set_rs(rm_gen1,rs_gen1);
	set_rs(rm_gen2,rs_gen2);
	set_rs(rm_sat1,rs_sat1);
	set_rs(rm_sat2,rs_sat2);

	set_sm(rm_gen1.SBAS,sm_gen1);
	set_sm(rm_gen2.SBAS,sm_gen2);
	set_sm(rm_sat1.SBAS,sm_sat1);
	set_sm(rm_sat2.SBAS,sm_sat2);
}


void init_rs (report_stat &rs)
{
	rs.mCNo.init();
	rs.mdopp.init();
	rs.mpsrdev.init();
	rs.mphasedev.init();
	rs.ddopp.init();
	rs.mchiprate.init();

	rs.phasedev_counter = 0;
	rs.CNo_counter = 0;
	rs.coherence_counter = 0;
}


void init_sm (SBAS_message &sm)
{
	sm.ok = 0;
	sm.lost = 0;
	//sm.simlost = 0;
	sm.corrupted = 0;
	sm.critical_corrupted = 0;
	sm.wrongSisnet = 0;
	sm.error_ok = 0;
	sm.error_lost = 0;
	sm.error_corrupt = 0;
	//sm.curlostcounter=0;
}


void time_to_string (int time, char *buf)
{
	int hour = time/3600;
	int min = (time-hour*3600)/60;
	int sec = time - hour*3600 - min*60;
	sprintf(buf,"%02d:%02d:%02d",hour,min,sec);
}


