#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include "kzik.h"
#include "siggen.h"
#include "status.h"
#include "config.h"



const unsigned short Crc16Table[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};


// начальное состояние регистров для формирования PRN
const unsigned char InitPRN[][2] =
{
    {0x46,2}, {0xA1,2}, {0xB7,0}, {0x9A,0}, {0x8F,3},       	// 120-124
    {0x3E,2}, {0xE6,3}, {0xCF,1}, {0x5A,3}, {0xA8,2},       	// 125-129
    {0xE1,0}, {0x69,1}, {0x50,1}, {0xD9,3}, {0xC6,1},       	// 130-134
    {0x8E,2}, {0xE0,1}, {0x07,2}, {0x28,1},                		// 135-138
    {0305,0}, {0253,3}, {0011,3}, {0244,3}, {0312,2},       	// 139-143
    {0060,2}, {0160,3}, {0035,0}, {0355,0}, {0335,0},       	// 144-148
    {0254,2}, {0041,1}, {0142,0}, {0241,3}, {0104,3},       	// 149-153
    {0351,1}, {0374,3}, {0107,0}, {0153,2}, {0142,3},       	// 154-158
    {0223,2}, {0302,3}, {0036,1}, {0335,3}, {0262,3},       	// 159-163
    {0170,3}, {0173,3}, {0201,0}, {0235,1}, {0337,3},       	// 164-168
    {0270,3}, {0134,0}, {0224,2}, {0060,3},                 	// 169-172
    {0362,1}, {0254,3}, {0110,1}, {0242,0}, {0142,2},		// 173-177
    {0017,2}, {0070,2}, {0101,1}, {0055,1}, {0166,3},		// 178-182
    {0215,0}, {0003,2}, {0054,3}, {0265,3}, {0071,1},		// 183-187
    {0350,3}, {0307,0}, {0272,0}, {0364,1}, {0022,3},		// 188-192
    {0050,2}, {0207,3}, {0347,3}, {0305,2}, {0140,1},		// 193-197
    {0363,2}, {0327,1}, {0147,0}, {0206,2}, {0045,2},		// 198-202
    {0076,1}, {0204,1}, {0357,3}, {0330,2}, {0263,1},		// 203-207
    {0036,3}, {0353,1}, {0331,3}							// 208-210
};

const unsigned char StartBin[4]={0xAA,0x55,0x55,0xAA};
const double ChipScale = 75*pow(2.0,-48);
const double ChipRampScale = 75*pow(2.0,-50);
const double FreqScale = 300*pow(2.0,-48);
const double FreqRampScale = 300*pow(2.0,-50);
const unsigned char MsgSize = 36;
const double FreqIncr = 0.000001;
const int TIMEOUT_MSG = 2000;
const int TIMEOUT_INIT = 120000;
const double Vc = 299792458.0;


GusSigGen::GusSigGen(unsigned char PRN, double Delay, double StartFreq, double StartChipRate)
{
    LogLen=0;
    ExtCommandCode=0;
    CurrentCommand=0;
    op.Freq = 0.0;
    op.Chip = 0.0;

    op.Coherent = false;
    #ifdef COHERENT_FREQCHIP
    op.Coherent = true;
    #endif

    double dFreq = 0.0;
    op.newFreq = StartFreq;
    op.newChip = StartChipRate;
    op.PRN = PRN;
    op.Delay = Delay/Vc;
    log_send(LOG_INFO,"siggen start: PRN=%d, Delay=%.2f, Freq=%.2f, ChipRate=%.2f, Coherent=%d",op.PRN,op.Delay,op.newFreq,op.newChip,op.Coherent);
}


GusSigGen::~GusSigGen()
{
    close(fd);
}


int GusSigGen::get_fd()
{
    return fd;
}


// проверка PRN
bool GusSigGen::GetIsPrnValid(unsigned char PRN)
{
    int length = sizeof(InitPRN)/2;
    if (PRN<120 || PRN>120+length-1)
    {
        log_send(LOG_ERR, "ERR: unavailable PRN #%d\n",PRN);
        return false;
    }
    return true;
}


// установка начальной задержки ПСП
bool GusSigGen::SetInitDelay(double Delay)
{
    if (Delay<0 || Delay>=0.9999999999999999)			// максимально допустимое значение - 0.99999999999999994 сек;
    {
		log_send(LOG_ERR, "ERR: siggen: incorrect init signal delay %f\n",Delay);
		return false;
    }
    op.InitSymbol = (unsigned short)(Delay*1000/2);
    double Left = Delay-(double)(op.InitSymbol)/1000*2;
    op.SymbolParity = (unsigned char)(Left*1000);
    Left -= (double)(op.SymbolParity)/1000;
    op.InitChip = (unsigned short)(Left*1000*1023);
    Left -= (double)(op.InitChip)/1000/1023;
    op.InitPhase = nearbyint(Left*1000*1023*255);

    if (op.InitChip<0 || op.InitChip>1022 ||
    	op.InitSymbol<0 || op.InitSymbol>499 ||
    	op.InitPhase<0 || op.InitPhase>255 ||
    	op.SymbolParity<0 || op.SymbolParity>1)
    {
		log_send(LOG_ERR, "ERR: siggen: incorrect init delay parameters: %d,%d,%d",op.InitSymbol,op.InitChip,op.InitPhase);
		return false;
    }
    return true;
}


// инициализация генератора
int GusSigGen::init()
{
    if(status_change_r(SYS_GEN,STATUS_INIT) == -1){
        log_send(LOG_ERR, "ERR: status_change_r failed");
        exit(1);
    }
    log_send(LOG_INFO, "Start SigGen initialization (PRN=%d)",op.PRN);

    if (!GetIsPrnValid(op.PRN))						// проверяем, не выходит ли заданный PRN за рамки допустимых
    {
        status_change_r(SYS_GEN,STATUS_ERR);
        exit(1);
    }
    
    if (!SetInitDelay(op.Delay))
    {
    	status_change_r(SYS_GEN,STATUS_ERR);
    	exit(1);
    }

    if (!OpenCom())
    {
        log_send(LOG_ERR, "ERR: GusSigGen::init: OpenCom failed");
        status_change_r(SYS_GEN,STATUS_ERR);
        exit(1);
    }

    WriteCommand(SGCOM_RESET);
    sleep(1);
    WriteCommand(SGCOM_INIT);
    sleep(1);
    tcflush(fd,TCIFLUSH);
    WriteCommand(SGCOM_START);
    while(1)
    {
    	int ret = readmsg(1);
    	if (ret==-1) return -1;
    	if (sgs.Operational==0) break;             // завершаем цикл инициализации, когда получаем с генератора флаг "operational"
    }

    event_reg_fd(fd,sg_callback,this);
    if(status_change_r(SYS_GEN, STATUS_OK) == -1){
        log_send(LOG_ERR, "ERR: status_change_r failed");
        exit(1);
    }

	ExtCommand(EXTCOM_MANFREQCHIP,op.newFreq,op.newChip);
	int ret = readmsg(1);
    log_send(LOG_INFO, "SigGen initialization finished");
    return 0;
}


// подключение по СОМ-порту
bool GusSigGen::OpenCom()
{
    struct termios term;
    bool result = true;

    fd = open(GENERATORFD,O_RDWR|O_NOCTTY);
    if (fd<0)
    {
        log_send(LOG_ERR, "ERR: GusSigGen::OpenCom: can't open siggen port %s",GENERATORFD);
        return false;
    }
    if (cfsetispeed(&term,B19200))
        if (cfsetospeed(&term,B19200))
            result=false;
    term.c_cflag = 0x80000bbe;
    term.c_iflag = 0x11;
    term.c_lflag = 0xaa0;
    term.c_line = 0x0;
    term.c_oflag = 0x4;
    term.c_cc[VTIME] = 1;
    term.c_cc[VMIN] = MsgSize;
    if (tcsetattr(fd,TCSAFLUSH,&term)) result=false;
    return result;
}


// отправка команды
bool GusSigGen::SendCommand (struct SigGenCommand sgc)
{
    for (int i=0;i<4;i++)
        sgc.StartBin[i]=StartBin[i];                    // Syncrosymbols
    sgc.FreqID = 1;
    sgc.CRC = Crc16(&sgc.StartBin[0],MsgSize-2);

    printf("Command send: ");
    for (int i=0;i<MsgSize;i++)
	printf("%02X",sgc.StartBin[i]);
    printf("\n");

    int n = write(fd,sgc.StartBin,MsgSize);
    if (n < MsgSize)
    {
        log_send(LOG_WARNING, "WRN: siggen write failed (written %d of %d)",n,MsgSize);
        return false;
    }
    return true;
}


// вычисление crc16
unsigned short GusSigGen::Crc16 (unsigned char *pcBlock, unsigned int len)
{
    unsigned short crc = 0xFFFF;
    while (len--)
        crc = (crc<<8) ^ Crc16Table[(crc>>8) ^ *pcBlock++];
    return crc;
}


// команда установки несущей и татовой частоты
void GusSigGen::ChipFreqSet (SigGenCommand &sgc, double Freq, double FreqRamp, double Chip, double ChipRamp)
{
    double dParam;
    int64_t ulParam;

    union {
        unsigned char uc6[6];
        int64_t ui64;
    } uCI;

    dParam = Freq/FreqScale;
    ulParam = (int64_t)dParam;
    if (dParam-ulParam>=0.5) ulParam++;
    uCI.ui64 = ulParam;
    for (int i=0; i<6; i++)
        sgc.Frequency[i]=uCI.uc6[i];

    dParam = FreqRamp/FreqRampScale;
    ulParam = (int64_t)dParam;
    if (dParam-ulParam>=0.5) ulParam++;
    uCI.ui64 = ulParam;
    for (int i=0; i<3; i++)
        sgc.FrequencyRamp[i]=uCI.uc6[i];

    dParam = Chip/ChipScale;
    ulParam = (int64_t)dParam;
    if (dParam-ulParam>=0.5) ulParam++;
    uCI.ui64 = ulParam;
    for (int i=0; i<6; i++)
        sgc.ChipRate[i]=uCI.uc6[i];

    dParam = ChipRamp/ChipRampScale;
    ulParam = (int64_t)dParam;
    if (dParam-ulParam>=0.5) ulParam++;
    uCI.ui64 = ulParam;
    for (int i=0; i<2; i++)
        sgc.ChipRateRamp[i]=uCI.uc6[i];
}


// запись команды
bool GusSigGen::WriteCommand (int CommandID)
{
    SigGenCommand sgc;
    bzero(&sgc,sizeof(sgc));

    switch (CommandID)
    {
    case SGCOM_RESET:                       	// Reset
        sgc.Identifier = 0x10;
        break;

    case SGCOM_INIT:                        	// Initialize
        sgc.Identifier = 0x02;
        sgc.SubChip = op.InitPhase;
        sgc.CodeAdvance[0] = op.InitChip&0xFF;
        sgc.CodeAdvance[1] = op.InitChip>>8;
        sgc.SymbolAdvance[0] = op.InitSymbol&0xFF;
        sgc.SymbolAdvance[1] = (op.InitSymbol>>8)|(op.SymbolParity<<7);
        sgc.InitI[0]=InitPRN[op.PRN-120][0];
        sgc.InitI[1]=InitPRN[op.PRN-120][1];
        break;

    case SGCOM_START:                       	// Start
        sgc.Identifier = 0x01;
        sgc.ControlCommand = 0x21;          	// 00100001
        break;


    case SGCOM_FREQCHIPSET:              	// коррекция частоты и скорости передачи данных
        sgc.Identifier = 0x04;
        log_send(LOG_INFO, "write command FREQCHIPSET, new freq = %f Hz, new chiprate = %f Hz",op.newFreq,op.newChip);
        ChipFreqSet (sgc, 70.0+op.newFreq*0.000001, 0.0 , 1.023+op.newChip*0.000001, 0.0);
        break;

    default:
        log_send(LOG_ERR, "ERR: unavailable siggen command ID: %d",CommandID);
        return false;
    }

    if (!SendCommand(sgc)) {
        log_send(LOG_WARNING, "WRN: can't write siggen command (ID %d)",CommandID);
        return false;
    }
    return true;
}


// функция SearchLog
// копит данные, приходящие с генератора, в массив log[]
// если готово целиком сообщение, возвращает true, иначе false
bool GusSigGen::SearchLog (unsigned char *bufrd, unsigned int NumByte)
{
    unsigned int shift = 0;

    if(LogLen+NumByte>2*MsgSize)    		// проверяем, не будет ли переполнения буфера с учетом новых байт
    {
        LogLen=0;                   		// "обнуляем буфер" - ставим указатель на нулевой элемент
        log_send(LOG_WARNING, "WRN: siggen: log overflow");
        shift = NumByte-2*MsgSize;  		// сдвиг, с которым берем байты из буфера (если буфер не переполнен, то это 0)
        NumByte = 2*MsgSize;        		// если буфер переполнен, то берем столько байт, чтобы гарантированно получить сообщение целиком
    }

    for (unsigned int j=0; j<NumByte; j++)
        log[LogLen+j] = bufrd[shift+j];     	// переписываем байты в буфер
    LogLen+=NumByte;                        	// увеличиваем длину буфера

    if (LogLen<MsgSize)                     	// проверяем, достаточно ли в буфере байт для целого сообщения
        return false;
		
    if (NumByte==MsgSize && log[0]==0xAA && log[1]==0x55 && log[2]==0x55 && log[3]==0xAA)   // самый простой случай - работаем в синхронизме, никаких сдвигов не нужно
    {
        ReadLog();
        LogLen=0;
        return true;
    }

    unsigned int k;                         	// указатель на начало сообщения в буфере
    for (k=3; k<LogLen; k++)                	// перебираем буфер с конца до начала (если пришло подрят несколько сообщений, берем последнее)
        if (log[k-3]==0xAA && log[k-2]==0x55 && log[k-1]==0x55 && log[k]==0xAA)     // ищем в буфере синхросимволы генератора
        {
            if (k!=3)                       	// если сообщение не находится в начале буфера, его надо сдвинуть в начало
            {
                for (unsigned int i=0; i<LogLen-k+3; i++)
                    log[i]=log[i+k-3];      	// смещаем сообщение в начало буфера
                LogLen=LogLen-k+3;          	// уточняем длину буфера
            }

            if (LogLen<MsgSize) return false;   // если символов в буфере после сдвига недостаточно, продолжать нельзя

            ReadLog();                          // если все сложилось удачно и дошли сюда, обрабатываем сообщение

            for (unsigned int i=MsgSize; i<LogLen; i++)
                log[i-MsgSize]=log[i];      	// смещаем байты, идущие за декодированным сообщением, в начало буфера
            LogLen-=MsgSize;                	// уточняем длину буфера

            return true;
        }

    return false;
}


// чтение ответа генератора
void GusSigGen::ReadLog()
{
    bool bRead=false;
    union
    {
        unsigned char b[2];
        unsigned short d;
    } ucs;

    for(int i=0;i<2;i++)
        ucs.b[i]=log[MsgSize-2+i];  		// записываем байты контрольной суммы в виде USHORT

    // если контрольная сумма не совпадает
    if (Crc16(&log[0],MsgSize-2)!=ucs.d)
    {
    	log_send(LOG_WARNING, "WRN: siggen message wrong CS (calc=%x, msg=%x)",Crc16(&log[0],MsgSize-2),ucs.d);
    	return;
    }

    // сообщение должно относиться к L1
    if (log[4]!=1) return;

    union
    {
        unsigned char b[MsgSize];
        SigGenMessage sgm;
    } ucm;
    for (unsigned int i=0; i<MsgSize; i++)
        ucm.b[i]=log[i+5];      	// переписываем буфер в временную структуру
    sgm = ucm.sgm;              	// копируем временную структуру в структуру сообщения
    bRead=true;                 	// сообщение генератора готово для обработки
}



// Функция DecodeFlags
// Вызывается из функции readmsg в случае успешного приема сообщения в ReadLog
// Выставляет флаги ошибок на основе анализа значения полей SwStatus,ErStatus,HwStatus
// В отличие от сообщения генератора, здесь '0' ВСЕГДА означает отсутствие
// ошибок или правильное значение идентификатора
bool GusSigGen::DecodeFlags ()
{
    bzero(&sgs,sizeof(sgs));    					// обнуляем структуру флагов ошибок (для экономии времени)
    if (sgm.SwStatus==0 && sgm.ErStatus==0 && sgm.HwStatus==0xC1)   	// нормальная работа, не нужно проверять все флаги
        return false;                                               	// сообщаем, что обработка ошибок не нужна

    if (sgm.SwStatus!=0)
    {
        sgs.TxInhibit       = (bool)(sgm.SwStatus&0x1);
        sgs.IfSwitch        = (bool)(sgm.SwStatus&0x2);
        sgs.CwMode          = (bool)(sgm.SwStatus&0x4);
    }
    if (sgm.ErStatus!=0)
    {
        sgs.CmpDataError    = (bool)(sgm.ErStatus&0x001);
        sgs.UDataNComp      = (bool)(sgm.ErStatus&0x002);
        sgs.SDataNComp      = (bool)(sgm.ErStatus&0x004);
        sgs.ParityError     = (bool)(sgm.ErStatus&0x008);
        sgs.FramingError    = (bool)(sgm.ErStatus&0x010);
        sgs.OverrunError    = (bool)(sgm.ErStatus&0x020);
        sgs.DataSyncError   = (bool)(sgm.ErStatus&0x040);
        sgs.DataCrcError    = (bool)(sgm.ErStatus&0x080);
        sgs.InvFieldValue   = (bool)(sgm.ErStatus&0x100);
        sgs.InvRangeFields  = (bool)(sgm.ErStatus&0x200);
    }
    if (sgm.HwStatus!=0xC1)
    {
        sgs.Ref10Mhz        =!(bool)(sgm.HwStatus&0x01);
        sgs.ClockFault      = (bool)(sgm.HwStatus&0x02);
        sgs.RfFault         = (bool)(sgm.HwStatus&0x04);
        sgs.BpskMode        = (bool)(sgm.HwStatus&0x08);
        sgs.SymbolsPerSecond= (bool)(sgm.HwStatus&0x10);
        sgs.Operational     =!(bool)(sgm.HwStatus&0x40);
        sgs.Ref1PPS         =!(bool)(sgm.HwStatus&0x80);
    }
    return true;        // сообщаем, что нужна обработка ошибок
}


// обработка сообщений об ошибках
void GusSigGen::ErrorHandle()
{
    // флаг ошибки, чтобы не делать status_change много раз, если есть несколько ошибок
    bool bError = false;	

    // если оборудование неисправно, сразу выставляем флаг ошибки
    if (sgm.State==SGS_ERROR)
    {
      log_send(LOG_ERR, "ERR: siggen state: error");
      bError = true;
    }

    // если находимся в процессе инициализации, сообщения об ошибках в лог не пишутся и не обрабатываются
    if (sgm.State==SGS_RESET)
      log_send(LOG_WARNING, "WRN: siggen state: reset");

    if (sgm.State==SGS_CALIBRATION)
      log_send(LOG_WARNING, "WRN: siggen state: calibration");
  
    if (sgm.State==SGS_INITIALIZED)
      log_send(LOG_WARNING, "WRN: siggen state: initialized");
    
    if (sgs.TxInhibit)
        log_send(LOG_WARNING, "WRN: siggen switch: TxInhibit");

    if (sgs.IfSwitch)
    	log_send(LOG_ERR, "ERR: siggen switch: IfSwitch");

    if (sgs.CwMode)
        log_send(LOG_ERR, "ERR: siggen switch: CwMode");

    if (sgs.CmpDataError)
        log_send(LOG_ERR, "ERR: siggen error: CMP Data error");
    
    if (sgs.UDataNComp)
        log_send(LOG_WARNING, "WRN: siggen error: update data not complete at 1PPS");

    if (sgs.SDataNComp)
        log_send(LOG_WARNING, "WRN: siggen error: status data not complete at 1PPS");

    if (sgs.ParityError)
        log_send(LOG_WARNING, "WRN: siggen error: parity error");

    if (sgs.FramingError)
        log_send(LOG_WARNING, "WRN: siggen error: framing error");

    if (sgs.OverrunError)
    	log_send(LOG_WARNING, "WRN: siggen error: overrun error");

    if (sgs.DataSyncError)
       	log_send(LOG_WARNING, "WRN: siggen error: data sync error");

    if (sgs.DataCrcError)
    	log_send(LOG_WARNING, "WRN: siggen error: data CRC error");

    if (sgs.InvFieldValue)
       	log_send(LOG_WARNING, "WRN: siggen error: invalid field value");

    if (sgs.InvRangeFields)
    	log_send(LOG_WARNING, "WRN: siggen error: invalid range fields");

    if (sgs.Ref10Mhz)
        log_send(LOG_ERR, "ERR: siggen status: no reference 10MHz");

    if (sgs.ClockFault)
      	log_send(LOG_ERR, "ERR: siggen status: clock fault");
    
    if (sgs.RfFault)
       	log_send(LOG_ERR, "ERR: siggen status: RF fault");
    
    if (sgs.BpskMode)
       	log_send(LOG_ERR, "ERR: siggen status: QPSK mode");

    if (sgs.SymbolsPerSecond)
        log_send(LOG_ERR, "ERR: siggen status: wrong SPS");

    if (sgs.Operational)
		log_send(LOG_ERR, "ERR: siggen status: not operational");

    if (sgs.Ref1PPS)
        log_send(LOG_ERR, "ERR: siggen status: no reference 1PPS");

    if (bError) {
        log_send(LOG_WARNING, "WRN: siggen failed");
        log_send(LOG_ERR, "ERR: reporting to server and exit with return 1");
        status_change_r(SYS_GEN, STATUS_ERR);
        exit(1);
    }
}


// обработка команды, поступающей из другого модуля (без параметров)
int GusSigGen::ExtCommand (int ExtCommandID)
{
    if (status_get(SYS_GEN) != STATUS_OK) {			// если инициализируется или неисправен, команды не принимать
		log_send(LOG_ERR, "ERR: external commands not permitted at current siggen status");
		return -1;
    }
    if (ExtCommandID<0 && ExtCommandID>=SGCOM_MAX) {
        log_send(LOG_ERR, "ERR: unavailable external siggen command code %d",ExtCommandID);
        return -1;
    }
    if(!WriteCommand(ExtCommandID)) 
		return -1;          	// если команда не проходит сразу, повторять ее не будем, сразу пишем об ошибке
    CurrentCommand=ExtCommandID;
    return 0;
}


// обработка команды, поступающей из другого модуля (один параметр)
int GusSigGen::ExtCommand (int ExtCommandID, float Param)
{
    if (status_get(SYS_GEN) != STATUS_OK) {			// если инициализируется или неисправен, команды не принимать
		log_send(LOG_ERR, "ERR: commands are not permitted at current siggen status");
		return -1;
    }
    if (ExtCommandID<0 && ExtCommandID>=EXTCOM_MAX) {
        log_send(LOG_ERR, "ERR: unavailable external siggen command code %d",ExtCommandID);
        return -1;
    }

    int CommandID;
    if (ExtCommandID==EXTCOM_MANFREQ)
    {
		if (abs(Param)>5000.0)
		{
			log_send(LOG_ERR, "ERR: new Doppler value %f is out of range [-5000,5000] Hz",Param);
			return 0;
		}
		CommandID=SGCOM_FREQCHIPSET;
		op.newFreq = Param;
		if (op.Coherent)
			op.newChip = op.newFreq/1540;
    }
    else if (ExtCommandID==EXTCOM_MANCHIP)
    {
		if (abs(Param)>5000.0)
		{
			log_send(LOG_ERR, "ERR: new chip rate value %f is out of range [-5000,5000] Hz",Param);
			return 0;
		}
		CommandID=SGCOM_FREQCHIPSET;
		op.newFreq = op.Freq;
		op.newChip = Param/1540;
    }
    else if (ExtCommandID==EXTCOM_AUTOFREQ)
    {
		float limit=2000.0;
		if (abs(Param)>limit)
		{
			log_send(LOG_ERR, "ERR: frequency correction value %f is out of range [-%.1f,%.1f] Hz",Param,limit,limit);
			return 0;
		}
		CommandID=SGCOM_FREQCHIPSET;
		op.newFreq = op.Freq + Param;
		if (op.Coherent)
			op.newChip = op.newFreq/1540;
    }
    else
    {
		log_send(LOG_ERR,"ERR: ExtCommand: wrong external command ID %d", ExtCommandID);
		return 0;
    }

    if(!WriteCommand(CommandID)) return -1;          	// если команда не проходит, пишем об ошибке
    CurrentCommand=CommandID;
    return 0;
}


// обработка команды, поступающей из другого модуля (два параметра)
int GusSigGen::ExtCommand (int ExtCommandID, float Param1, float Param2)
{
    if (status_get(SYS_GEN) != STATUS_OK) {			// если инициализируется или неисправен, команды не принимать
		log_send(LOG_ERR, "ERR: commands are not permitted at current siggen status");
		return -1;
    }
    if (ExtCommandID<0 && ExtCommandID>=EXTCOM_MAX) {
        log_send(LOG_ERR, "ERR: unavailable external siggen command code %d",ExtCommandID);
        return -1;
    }

    int CommandID;
    if (ExtCommandID==EXTCOM_MANFREQCHIP)
    {
		float limit=5000.0;
		if (abs(Param1)>limit)
		{
			log_send(LOG_ERR, "ERR: new Doppler value %f is out of range [%.1f,%.1f] Hz",Param1,-limit,limit);
			return 0;
		}
		if (abs(Param2)>limit)
		{
			log_send(LOG_ERR, "ERR: new chip rate value %f is out of range [%.1f,%.1f] Hz",Param2,-limit,limit);
			return 0;
		}
		CommandID=SGCOM_FREQCHIPSET;
		op.newFreq = Param1;
		op.newChip = Param2/1540;
    }
    else if (ExtCommandID==EXTCOM_AUTOFREQCHIP)
    {
		float limit=2000.0;
		if (abs(Param1)>limit)
		{
			log_send(LOG_ERR, "ERR: frequency correction value %f is out of range [%.1f,%.1f] Hz",Param1,-limit,limit);
			return 0;
		}
		if (abs(Param2)>limit)
		{
			log_send(LOG_ERR, "ERR: chip rate correction value %f is out of range [%.1f,%.1f] Hz",Param2,-limit,limit);
			return 0;
		}
		CommandID=SGCOM_FREQCHIPSET;
		op.newFreq = op.Freq + Param1;
		op.newChip = op.Chip + Param2/1540;
    }
    else
    {
		log_send(LOG_ERR,"ERR: ExtCommand: wrong external command ID %d", ExtCommandID);
		return 0;
    }

    if(!WriteCommand(CommandID)) return -1;          	// если команда не проходит сразу, повторять ее не будем, сразу пишем об ошибке
    CurrentCommand=CommandID;
    return 0;
}


// эта функция проверяет, понял ли генератор предыдущую команду, и посылает новую команду на генератор
void GusSigGen::CommandManager()
{
    // проверяем выполнение прошлой команды (если она еще не выполнена)
    if (CurrentCommand)
    {
        if (sgs.UDataNComp||sgs.ParityError||sgs.FramingError||sgs.OverrunError||sgs.DataSyncError||sgs.DataCrcError||sgs.InvFieldValue) {
            log_send(LOG_WARNING, "WRN: siggen doesn't understand last command ID %d (ErStatus=%x)",CurrentCommand,sgm.ErStatus);
        }
        else
        {
            log_send(LOG_INFO, "siggen command ID %d applied",CurrentCommand);
			if (CurrentCommand==SGCOM_FREQCHIPSET) {
				op.Freq = op.newFreq;
				op.Chip = op.newChip;
			}
            CurrentCommand=0;
        }
    }
}


// вычисление задержки ПСП в секундах
double GusSigGen::CodeDelay()
{
    int DSymbol = sgm.SymbolCounter&0x7FFF;
    int DEpoch = sgm.SymbolCounter>>15;
    int DChip = sgm.ChipCounter;
    int DPhase = sgm.SubPhase;
    double Ptime = (double)(DSymbol)/1000*2 + (double)(DEpoch)/1000 + (double)(DChip)/1023/1000 + (double)(DPhase)/1023/1000/65536;
    if (Ptime>0.5) Ptime-=1.0;
    return Ptime;
}


// чтение сообщения генератора
int GusSigGen::readmsg(int nread)
{
    unsigned char buf[1000];

    unsigned int res = read(fd,buf,1000);
    if (res<0)
    {
        log_send(LOG_ERR, "ERR: GusSigGen::readmsg: can't read siggen data");
        return -1;
    }

    if (SearchLog(buf,res))
    {
        DecodeFlags();
        ErrorHandle();
        CommandManager();
    }

    float dFreq = op.Freq;		// отклонение частоты несущей в Гц
    float dChip = op.Chip*1540;		// отклонение скорости кода в Гц
    struct bzk_stat_t *bzk_stat;
    bzk_stat = status_get_addr();
    bzk_stat->sgDelay = CodeDelay()*Vc;
    bzk_stat->sgFreq = dFreq;
    bzk_stat->sgChip = dChip;
    log_send(LOG_INFO, "siggen: delay=%d dFreq=%.2f dChip=%.2f",(int)bzk_stat->sgDelay,dFreq,dChip);

    #if SIGGEN_DEBUG
	printf("\nSigGen message received:  ");
	for (int i=0;i<MsgSize;i++) printf(" %02X",log[i]);
	printf("\nHardware reset counter: \t%d s",sgm.HwRstCounter);
	printf("\nSoftware reset counter: \t%d s",sgm.CmRstCounter);

	if (status_get(SYS_GEN) == STATUS_OK) 		printf("\nSigGen status: ok\n");
	if (status_get(SYS_GEN) == STATUS_ERR) 		printf("\nSigGen status: error\n");
	if (status_get(SYS_GEN) == STATUS_INIT) 	printf("\nSigGen status: init\n");
	if (status_get(SYS_GEN) == STATUS_INIT1) 	printf("\nSigGen status: init1\n");
	if (status_get(SYS_GEN) == STATUS_INIT2) 	printf("\nSigGen status: init2\n");
	if (status_get(SYS_GEN) == STATUS_UNDEFINED) 	printf("\nSigGen status: unknown\n");

	if (status_get(SYS_GEN) == STATUS_OK)
	{
		int DSymbol = sgm.SymbolCounter&0x7FFF;
		int DEpoch = sgm.SymbolCounter>>15;
		int DChip = sgm.ChipCounter;
		int DPhase = sgm.SubPhase;
		printf("\nInit delay: \t%3d ms, \t%d symbol, \t%3d chip, \t%5d subchip",op.InitSymbol,op.SymbolParity,op.InitChip,op.InitPhase);
		printf("\nReal Delay: \t%3d ms, \t%d symbol, \t%3d chip, \t%5d subchip",DSymbol,DEpoch,DChip,DPhase);
		double Ptime = (double)(DSymbol)/1000*2 + (double)(DEpoch)/1000 + (double)(DChip)/1023/1000 + (double)(DPhase)/1023/1000/65536;
		double Delta = Ptime - op.Delay;
		if (Delta>0.9) Delta-=1.0;
		if (Delta<-0.9) Delta+=1.0;
		printf("\nDELAY=%20.18f(s) \tPTIME=%20.18f(s) \tDELTA=%20.18f(s)\n",op.Delay,Ptime,Delta);
	}
    #endif
    return 0;
}


void sg_callback(int fd, int nread, void *obj)
{
    int res=((GusSigGen*)obj)->readmsg(nread);
    if (res==-1)
    {
        log_send(LOG_ERR, "ERR: sg_callback: SigGen::readmsg() failed");
        log_send(LOG_ERR, "ERR: sg_callback: report and exit");
        status_change_r(SYS_GEN,STATUS_ERR);
        exit(1);
    } 
    else
        if (status_get(SYS_GENBOARD) == STATUS_OK && status_get(SYS_GEN) == STATUS_INIT)
            if(status_change_r(SYS_GEN, STATUS_OK) == -1){
                log_send(LOG_ERR, "ERR: status_change_r failed");
                exit(1);
            }
}


// прекращение работы модуля по таймауту
void GusSigGen::timeout()
{
    log_send(LOG_ERR, "ERR: Siggen message timeout occured");
    log_send(LOG_ERR, "ERR: this is the critical error, performing report end exit");
    status_change_r(SYS_GEN, STATUS_ERR);
    exit(1);
}
