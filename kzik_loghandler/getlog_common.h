#ifndef GETLOG_COMMON
#define GETLOG_COMMON


#define WINDOWS_DEBUG

const unsigned int maxinamelen = 256;
const unsigned int maxonamelen = 256;
//const char baseInputFileName[] = "/var/log/bzk";


int getStandardFileName (char *baseName, char *resultName, int step);
FILE *create_bzk_outfile (my_time cur_t, const char *end, bool standard_infile, char *ifilename);
FILE *create_outfile (my_time cur_t, const char *end, char *hostname);
bool no_input_time(my_time start_t, my_time end_t);

void buf2float (char *buf, const char name[], int len, double &value);
void buf2int (char *buf, const char name[], int len, int &value);
void compact_float_print (FILE* outFile, double fval, int accuracy);


#ifndef WINDOWS_DEBUG
	int start_seek (gzFile file, my_time start_time, my_time end_time);
	gzFile openNextStandardFile(char *baseName, int &filenum);
	gzFile start_nonstandart_infile (my_time start_t, my_time end_t, char *ifilename);
	gzFile start_standart_infile (my_time start_t, my_time end_t, char *baseName, int &filenum);
	void getHostname(gzFile file, char *hostname, bool bSyslog);
#else
	int start_seek (FILE *file, my_time start_time, my_time end_time);
	FILE *openNextStandardFile(char *baseName, int &filenum);
	FILE *start_nonstandart_infile (my_time start_t, my_time end_t, char *ifilename);
	FILE *start_standart_infile (my_time start_t, my_time end_t, char *baseName, int &filenum);
	void getHostname(FILE *file, char *hostname, bool bSyslog);
#endif


#endif
