
#ifndef SIGGEN_H_
#define SIGGEN_H_
#include <math.h>
#define SIGGEN_DEBUG 0


enum {
    SGCOM_RESET,            // программная перезагрузка генератора
    SGCOM_INIT,             // инициализация генератора для формирования сигнала с нужными параметрами
    SGCOM_START,            // начало выдачи сигнала 70 МГЦ (после получения этой команды начинается калибровка генератора, которая может длиться до 15 секунд)
    SGCOM_FREQCHIPSET,
    SGCOM_MAX
};


enum {
    EXTCOM_MANFREQ,
    EXTCOM_MANCHIP,
    EXTCOM_MANFREQCHIP,
    EXTCOM_AUTOFREQ,
    EXTCOM_AUTOFREQCHIP,
    EXTCOM_MAX
};


class GusSigGen
{
    int fd;
    unsigned char log[255];
    unsigned int LogLen;
    int CurrentCommand;
    int ExtCommandCode;

    struct OperationParams
    {
        unsigned char PRN;              // PRN дальномерного кода
        double Chip;                    // скорость модуляции дальномерного кода в Мбит/с
        double Freq;                    // частота формируемого сигнала в МГц
        double newChip;                 // временное значение скорости (используется, пока нет подтверждения от генератора)
        double newFreq;                 // новое значение частоты (используется, пока нет подтверждения от генератора)

        double Delay;                   // задержка по коду в секундах
        unsigned short InitPhase;      	// Uplink Range Code Chip Sub-Phase
        unsigned short InitChip;        // Uplink Range Code Chip Counter
        unsigned short InitSymbol;      // Uplink Range Symbol Counter
        unsigned char SymbolParity;
        bool Coherent;                  // когерентный режим
    } op;

#pragma pack(1)
    struct SigGenMessage
    {
        unsigned short SubPhase;           // Uplink Range Code Chip Sub-Phase
        unsigned short ChipCounter;        // Uplink Range Code Chip Counter
        unsigned short SymbolCounter;      // Uplink Range Symbol Counter
        unsigned char  SwStatus;           // Switch Status
        unsigned short ErStatus;           // Error Status
        unsigned char  HwStatus;           // Hardware Status
        unsigned char  Zero;               // For future use. Set to zero.
        unsigned int   CmRstCounter;       // Reset Command Second Epoch Counter
        unsigned int   HwRstCounter;       // Hardware Reset Second Epoch Counter
        unsigned short FwVersion;          // Firmware Version Number
        unsigned short FpgaVersion;        // FPGA Version Number
        unsigned char  State;              // Signal Generator State (ERROR-RESET-INITIALIZED-CALIBRATION-OPERATIONAL)
    } sgm;

    struct SigGenCommand
    {
        unsigned char StartBin[4];
        unsigned char FreqID;
        unsigned char Identifier;
        unsigned char ControlCommand;
        unsigned char SymbolRate;
        unsigned char SubChip;
        unsigned char CodeAdvance[2];
        unsigned char SymbolAdvance[2];
        unsigned char InitI[2];
        unsigned char InitQ[2];
        unsigned char ChipRate[6];
        unsigned char ChipRateRamp[2];
        unsigned char Frequency[6];
        unsigned char FrequencyRamp[3];
        unsigned short CRC;
    };
#pragma pack()

    struct SigGenState
    {
        bool TxInhibit;                     // всегда при инициализации
        bool IfSwitch;                      // выключена выдача сигнала на IF
        bool CwMode;                        // включен режим излучения без модуляции
        bool CmpDataError;                  // ошибка поступления данных с СУ
        bool UDataNComp;                    // update data not complete (command)
        bool SDataNComp;                    // status data not complete (message)
        bool ParityError;
        bool FramingError;
        bool OverrunError;
        bool DataSyncError;
        bool DataCrcError;
        bool InvFieldValue;                 // недопустимое значение поля команды
        bool InvRangeFields;                // поле range в сообщении генератора имеет недействительное значение
        bool Ref10Mhz;                      // нет внешнего сигнала 10 МГц
        bool ClockFault;
        bool RfFault;
        bool BpskMode;                      // включен режим излучения QPSK вместо BPSK
        bool SymbolsPerSecond;              // включен режим скорость модуляции 1000 бит/с вместо 500 бит/с
        bool Operational;                   // состояние генератора не равно OPERATIONAL (всегда при инициализации)
        bool Ref1PPS;                       // нет внешнего сигнала 1PPS
    } sgs;

    enum {SGS_ERROR, SGS_RESET, SGS_INITIALIZED, SGS_CALIBRATION, SGS_OPERATIONAL, SGS_MAX};

    bool GetIsPrnValid(unsigned char PRN);
    bool SetInitDelay(double Delay);
    unsigned short Crc16 (unsigned char *pcBlock, unsigned int len);

    bool OpenCom();
    void CloseCom();

    bool SearchLog(unsigned char* bufrd,unsigned int NumByte);      // передача прочитанных символов и их количества
    void ReadLog();
    bool DecodeFlags();
    void ErrorHandle ();
    void PrintLog();

    double CodeDelay();
    void CommandManager();
    bool WriteCommand (int CommandID);
    void ChipFreqSet (SigGenCommand &sgc, double Freq, double FreqRamp, double Chip, double ChipRamp);
    bool SendCommand(struct SigGenCommand sgc);

public:
    GusSigGen(unsigned char PRN, double Delay, double StartFreq, double StartChipRate);
    ~GusSigGen();
    int init();
    int get_fd();
    int readmsg(int nread);
    int ExtCommand(int CommandID);
    int ExtCommand(int CommandID, float Param);
    int ExtCommand(int CommandID, float Param1, float Param2);
    static void timeout();
    int ManFreqSet(double NewDopp);
    int ManChipSet(double NewDopp);
};

void sg_callback(int fd, int nread, void *obj);

#endif /* SIGGEN_H_ */

