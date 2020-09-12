#ifndef REPORT_H_
#define REPORT_H_


const float phasedev_lim = 0.05;
const float CNo_lim = 35.0;
const float dopp_lim = 30.0;
const float dfreq_lim = 100.0;



struct report_stat
{
	recursive_mean mCNo;
	recursive_mean mdopp;
	recursive_mean mpsrdev;
	recursive_mean mphasedev;
	recursive_mean ddopp;
	recursive_mean mchiprate;

	int phasedev_counter;
	int CNo_counter;
	int coherence_counter;
};


struct SBAS_message
{
	int ok;
	int lost;
	int corrupted;
	int critical_corrupted;
	int wrongSisnet;
	int error_ok;
	int error_lost;
	int error_corrupt;
};




void time_to_string (int time, char *buf);

void print_sm (FILE *file, SBAS_message sm);
void print_lc (FILE *file, lost_counter lc);
void print_lc_total (FILE *file, lost_counter lc[6]);
void print_mean (FILE *file, report_stat rs);

void init_rs (report_stat &rs);
void init_sm (SBAS_message &sm);
void report_init_sm();
void report_init_lc();
void report_init_rs();

void set_sm (int SBAS, SBAS_message &sm);
void set_rs (range_message rm, report_stat &rs);
void report_set_sm(int SBAS_gen1,int SBAS_gen2,int SBAS_sat1,int SBAS_sat2);
void report_set_lc (float time, float SBAS, float phasedev, float CNo, float delta_dopp, float delta_chip_dopp);
void report_set_rs (range_message rm_gen1, range_message rm_gen2, range_message rm_sat1, range_message rm_sat2);

void report_out (FILE *oFile2);



#endif /* REPORT_H_ */
