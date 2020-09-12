#include <iostream>		// windows for FILE
#include <stdio.h>
#include <zlib.h>
#include <time.h>
#include "mytime.h"
#include "range_message.h"
#include "getlog_common.h"



void rm_set (char *buf, range_message &rm)
{
	buf2float(buf,"CNo=",5,rm.CNo);
	buf2float(buf,"dopp=",7,rm.dopp);
	buf2float(buf,"psr=",16,rm.psr);
	buf2float(buf,"psrdev=",5,rm.psrdev);
	buf2float(buf,"phase=",16,rm.phase);
	buf2float(buf,"phasedev=",5,rm.phasedev);
	buf2int(buf,"locktime=",9,rm.locktime);
}


void rm_reset (range_message &rm)
{
	rm.CNo = 0.0;
	rm.dopp = 0.0;
	rm.psr = 0.0;
	rm.psrdev = 0.0;
	rm.phase = 0.0;
	rm.phasedev = 0.0;
	rm.locktime = 0;

	rm.parity = true;
	rm.psrrate = 0.0;
	rm.delay = 0.0;
	rm.SBAS = 1;
}


void print_rm(FILE* oFile,range_message &rm)
{
	compact_float_print(oFile,rm.CNo,2);
	compact_float_print(oFile,rm.dopp,2);
	compact_float_print(oFile,rm.psr,2);
	compact_float_print(oFile,rm.psrdev,3);
	compact_float_print(oFile,rm.phase,2);
	compact_float_print(oFile,rm.phasedev,3);
	fprintf(oFile,"%d,\t",rm.locktime);
}
