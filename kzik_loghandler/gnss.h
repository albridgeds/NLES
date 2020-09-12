#ifndef GNSS_H_
#define GNSS_H_


const int satlen = 33;
const int gpslen = 30;
const int ellen = 18;


class satelite
{
    public:
    int prn;
    double cn0;
    double el;
    recursive_mean db_cn0[ellen];

    satelite()
    {
    	prn = 0;
        cn0 = 0.0;
        el = 0.0;
    }
};



class gnss
{
    public:
    int time;
    //static const int satlen = 33;
    //static const int gpslen = 30;
    satelite sat[satlen];

    //static const int ellen = 18;
    recursive_mean db_cn0[ellen];
    float ext_cn0[gpslen][ellen];
    bool ext_db_flag;
    float noise;

    int visible;
    double mean_cn0;

    gnss();
    void set (int prn, double cn0, double el);
    void reset (int _time);
    void output(FILE *oFile);
    void calc_mean();
    int fill (char *buf, int _time);
    void calibrate();
    void dbcn0_output();
    void calibrate2();
    void dbcn0_output2();
    void calc_noise();
    void read_dbcn0();
};


#endif /* GNSS_H_ */
