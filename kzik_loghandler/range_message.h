#ifndef RANGE_MESSAGE_H_
#define RANGE_MESSAGE_H_



struct range_message
{
	double CNo;
	double dopp;
	double psr;
	double psrdev;
	double phase;
	double phasedev;
	int locktime;

	double psrrate;
	float delay;
	bool parity;
	int SBAS;
};



void rm_reset (range_message &rm);
void rm_set (char *buf, range_message &rm);
void print_rm(FILE* outFile,range_message &rm);


#endif /* RANGE_MESSAGE_H_ */
