Ground uplink station (GUS) monitoring & control software

| project            | description        | Language, libraries       |
| :----------------- | :----------------- | :------------------------ |
| siggen | navigation signal generator (Novatel WAAS GUS SigGen) management (Linux, COM, original binary protocol) | С++: sys.h, fcntl.h, termios.h |
| kzik_loghandler | GUS log files processing. Output: csv file. | C++: getopt.h, zlib.h, time.h, math.h |
| kzik_report | creates dayly reports based on GUS log files. Input: csv; output: png image (signal charachteristics plots and statistics, SDCM data delivery statistics | Python: numpy, pandas, matplotlib, datetime, optparse, paramiko, xml, socket |
