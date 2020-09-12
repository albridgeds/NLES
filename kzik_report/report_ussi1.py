#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import datetime
import pandas as pd
import numpy as np
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt
from PIL import Image, ImageFont, ImageDraw
from sys import platform
from optparse import OptionParser
from ftplib import FTP


test = False
tzone = 3*3600
ls = 18

prn_num = [123,125,140]
sat_name = [u"Луч-5Б", u"Луч-5В"]
orb_slot = ["16W", "95E"]

# шрифты для виндовса и линукса
fontPath = 'C:\Windows\Fonts\lucon.ttf'
os_path = ''
out_path = ''
font = ImageFont.truetype(fontPath, 16)
text_color = (0, 0, 0)

SMALL_SIZE = 8
MEDIUM_SIZE = 10
BIGGER_SIZE = 12
plt.rc('font', size=SMALL_SIZE)
plt.rc('axes', titlesize=BIGGER_SIZE)
plt.rc('axes', labelsize=MEDIUM_SIZE)
plt.rc('xtick', labelsize=SMALL_SIZE)
plt.rc('ytick', labelsize=SMALL_SIZE)
plt.rc('legend', fontsize=SMALL_SIZE)


# расчет процента измерений за сутки
def procent(msg):
    return float(msg)/86400*100


# печать данных по БЗК
def text_bzk(draw, date, total, num_ok, mean_cn0):
    x = 120                     # начальное смещение текста по горизонтали
    y = 20                      # начальное смещение текста по вертикали
    header_date = u"Дата: " + date.strftime("%d.%m.%Y")
    draw.text((x, y), header_date, text_color, font=font)
    draw.text((x, y+20), u"Отчет УССИ", text_color, font=font)

    str_total = '%5d (%6.2f%%)' % (total, procent(total))
    draw.text((x, y + 60), u"Всего отчетов получено:", text_color, font=font)
    draw.text((x + 300, y + 60),  str_total, text_color, font=font)
    draw.text((x, y + 100), u"Не получено сообщений SBAS:", text_color, font=font)
    draw.text((x + 50, y + 270), u"Количество приемников", text_color, font=font)
    draw.text((x + 400, y + 270), u"Средний сигнал/шум (дБГц)", text_color, font=font)

    for i in range(len(prn_num)):
        str_prn = u"          PRN %d" % (prn_num[i])
        loss = total - num_ok[i]
        perc_loss = float(loss)/total*100
        str_loss = '%5d (%6.2f%%)' % (loss, perc_loss)
        draw.text((x, y + 120 + i*20), str_prn, text_color, font=font)
        draw.text((x+300, y + 120 + i*20), str_loss, text_color, font=font)
        str_cn0 = u'%4.1f дБГц' % (mean_cn0[i])
        # draw.text((X, Y + 200 + i * 20), str_prn, text_color, font=font)
        draw.text((x+500, y + 120 + i*20), str_cn0, text_color, font=font)


def plot_state(t, y1, y2, num):

    limy = (max(y1)+20)/10*10          # верхняя граница графика определяется максимальным количеством станций
    prn = "PRN %d" % (prn_num[num])

    t0 = np.arange(0.0, 86400.0)       # чтобы отметить моменты без данных
    y0 = np.zeros(86400)
    no_data = np.where(y1 == -1)
    for i in no_data:
        y0[i] = limy

    grid_size = (15, 2)
    ax = plt.subplot2grid(grid_size, (3+3*num, 0), rowspan=2, colspan=1)
    plt.plot(t0/3600, y0, label=u"no data", color='grey')
    plt.plot(t/3600, y1, label=u"total", color='green')
    plt.plot(t/3600, y2, label=prn, color='orange')
    plt.ylim(0, limy)
    plt.xlim(0, 24)
    plt.title(prn)
    # plt.title(u"Кол-во приемников")
    # plt.legend(loc='best')
    major_ticks = np.arange(0, 25, 2)
    ax.set_xticks(major_ticks)
    plt.grid()


# формирование графика сигнал/шум
def plot_cn0(t, y1, num):
    limy = 50
    prn = u"PRN %d" % (prn_num[num])
    grid_size = (15, 2)
    ax = plt.subplot2grid(grid_size, (3+3*num, 1), rowspan=2, colspan=1)
    plt.plot(t/3600, y1, label=prn, color='blue')
    plt.ylim(20, limy)
    plt.xlim(0, 24)
    plt.title(prn)
    # plt.title(u"Средний сигнал/шум (дБГц)")
    # plt.legend(loc='best')
    major_ticks = np.arange(0, 25, 2)
    ax.set_xticks(major_ticks)
    plt.grid()


# расчет среднего отношения сигнал/шум
def calc_cn0(cn0):
    if len(cn0) == 0:
        print("no cn0 data")
        return 0
    _cn0 = cn0[cn0 > 0]
    n = len(_cn0)
    if n != 0:
        m_cn0 = sum(_cn0)/n
        # print("mean cn0 = %.2f (%d epochs)" % (m_cn0, N))
        return m_cn0
    else:
        print("no valid cn0 data")
        return 0



# создание отчета (графики + текст)
def create_report(savefile, date, total_count, count, mcn0):
    plt.cla()               # создаем чистый лист
    plt.clf()
    f = plt.gcf()
    f.set_size_inches(9, 13)

    time = np.arange(0.0, 86400.0)
    len_prn = len(prn_num)
    num_ok = [None]*len_prn
    m_cn0 = [None]*len_prn
    len_total = len(total_count[total_count != -1])  # количество секунд, для которых есть данные с УССИ

    for i in range(len_prn):
        num_ok[i] = len(count[:, i][count[:, i] > 0])       # количество секунд, когда сигнал видит хотя бы одна станция
        m_cn0[i] = calc_cn0(mcn0[:, i])
        plot_state(time, total_count, count[:, i], i)
        plot_cn0(time, mcn0[:, i], i)
    plt.savefig(savefile, format='png', dpi=100)            # сохраняем полученные изображения в файл

    # печать статистики
    img = Image.open(savefile)
    draw = ImageDraw.Draw(img)
    text_bzk(draw, date, len_total, num_ok,  m_cn0)
    img.save(savefile)


# получение выборки для текущей секунды
def get_sub_data(data, index0, n):
    data_len = len(data)

    if index0+n >= data_len:
        n = data_len - index0 - 1
        # print('get_sub_data iN:',index0,data_len,iN)

    t0 = data.loc[index0, 1]                     # время, которое обрабатываем в этом цикле
    data_t = data.loc[index0:index0+n]         # формируем выборку из ближайших строк (для скорости)
    index1 = data_t.loc[data_t[1] > t0].index     # указатель на строку со следующей секундой
    data_t = data_t[data_t[1] == t0]            # только строки с правильным временем

    if len(index1) == 0:                        # если не нашли следующую секунду
        index1 = index0 + n + 1
        if index1 >= data_len:                  # либо закончился файл
            index1 = -1
        else:                                   # либо не хватило выборки (скорее всего зацепим в следующей итерации)
            print('get_sub_data index:', t0, index0, index1, n)
    else:
        index1 = index1[0]
    return data_t, index1


# количество приемников в первой эпохе
def get_rec_number(data):
    t0 = data.iloc[0, 1]
    data_t = data[data[1] == t0]
    rn = len(data_t.index)
    rn = int(rn*2.5)                # д.б. в минимум в два раза больше количества приемников
    if rn < 100:
        rn = 100
    return rn


# выбор из файла нужных данных с учетом сдвига времени (кажется это вообще не нужно)
def get_timezone_data(filename, part):
    if not os.access(filename, os.F_OK):
        print("Can't open " + filename)
        return []
    if os.stat(filename).st_size == 0:
        print("Empty file" + filename)
        return []

    fdata = pd.read_csv(filename, header=None, sep=';')
    t = fdata.loc[0, 1]                  # первое встреченное в файле время unix
    t0 = t - (t-1555351217) % 86400       # время начала суток (на случай, если файл не полный)
    t2 = t0 + 86400                     # время концка суток
    t1 = t0 + 86400 - tzone             # граница интервала с учетом сдвига времени

    if part == 1:
        data = fdata[fdata[1] > t1-1]   # начало интервала
        data = data[data[1] < t2]       # отсечь сообщения из будущего (есть такой баг у Пети)
    else:
        data = fdata[fdata[1] < t1]     # конец интервала
        data = data[data[1] >= t0]       # отсечь сообщения из прошлого (а вдруг и такой тоже есть?)
    return data


# обработка данных из файла
def sbas_handler(data):

    # запоминаем PRN в файле
    for i in np.arange(3, len(data.iloc[0]), 3):
        prn_num.append(data.iloc[0, i])
    print(prn_num)

    index = 0
    n = 86400
    if test:
        n = 1800                       # в режиме теста обрабатываем только пол часа

    rn = get_rec_number(data)
    len_prn = len(prn_num)
    total_count = np.zeros(86400)-1
    count = np.zeros((86400, len_prn))
    mcn0 = np.zeros((86400, len_prn))

    for i in range(n):
        if index == -1:                 # остановка цикла при завершении данных
            break

        data_t, index = get_sub_data(data, index, rn)
        t = (data_t.head(1)[1] - 1555351217 - 3600) % 86400
        # t = datetime2tod(data_t.iloc[0,0])
        total_count[t] = len(data_t)

        # счетчик текущего времени (просто для ощущения контроля над процессом)
        if (i % 3600) == 0:
            now = datetime.datetime.now()
            print("i=%5d  index=%7d  time=%5d  receivers=%3d  now=%s" % (i, index, t, total_count[t], now.isoformat()))
            # print("i=%5d  index=%7d  time=%5d  receivers=%3d   date=%s" % (i, index, t, total_count[t], data_t.head(1)[0].to_string()))

        # перебираем все имеющиеся prn
        for j in range(len_prn):
            k = 4+3*j                                             # столбец со значением сигнал/шум для данной prn
            cn0 = data_t[data_t.iloc[:, k] > 37]                     # все значения cn0 за данную секунду выше порога
            count[t, j] = len(cn0)
            if len(cn0):
                mcn0[t, j] = cn0[k].mean()
#            print("prn#%d : %.1f  (%d)" % (prn_num[j],mcn0,len(cn0)))
#        print('tod = %5d  %2d %2d %2d' % (t,count[t,0],count[t,1],count[t,2]))
    return [total_count, count, mcn0]


def tod2hms(tod):
    hour = int(tod/3600)
    min = int((tod-hour*3600)/60)
    sec = tod - hour*3600 - min*60
    return hour, min, sec


def datetime2tod(dt):
    tod = dt.hour*3600 + dt.minute*60 + dt.second
    return tod


def check_losses(value, log, fill):
    loss_flag = False
    loss_start = 0

    if log:
        f = open(os_path+log, 'w')

    for t in range(86400):
        # начало разрыва
        if value[t] <= 0 and loss_flag is False:
            loss_flag = True
            loss_start = t

        # конец разрыва
        if value[t] > 0 and loss_flag is True:
            loss_flag = False
            dtime = t - loss_start
            [hh, mm, ss] = tod2hms(loss_start)

            if log:
                if dtime == 1:
                    # print('Loss: %02d:%02d:%02d' % (hh,mm,ss))
                    f.write('\nLoss: %02d:%02d:%02d' % (hh, mm, ss))
                else:
                    [hh1, mm1, ss1] = tod2hms(t-1)
                    # print('Loss: %02d:%02d:%02d - %02d:%02d:%02d  (%d sec)' % (hh,mm,ss,hh1,mm1,ss1,dtime))
                    f.write('\nLoss: %02d:%02d:%02d - %02d:%02d:%02d  (%d sec)' % (hh, mm, ss, hh1, mm1, ss1, dtime))
            if fill:
                if dtime > 500:
                    for i in np.arange(loss_start, t, 500):
                        value[i] = 0
    if log:
        f.close()
    return value


# запись в файл процента доставки
def save_daystat(total_count, count, mcn0, kzik_num, date):
    year = date.year
    filename = os_path + 'kzik' + str(kzik_num) + '_' + str(year) + '.log'

    if os.path.exists(filename):
        data = pd.read_csv(filename, sep='\t', encoding='utf-8')
    else:
        print('create new dataframe')
        # data = pd.DataFrame(columns=['date','sisnet','satrec','ddopp','cn0','ussi_total','ussi_rec','ussi_mcn0'])
        data = pd.DataFrame(columns=['date', 'sisnet', 'satrec', 'ddopp', 'cn0', 'ussi_total', 'ussi_rec', 'ussi_mcn0'])

    ussi_total = len(total_count[total_count > 0])
    ussi_rec = len(count[:, kzik_num][count[:, kzik_num] > 0])
    ussi_mcn0 = calc_cn0(mcn0[:, kzik_num])

    a = data[data['date'] == str(date)]
    if not a.empty:                                 # заменяем данные, если они уже есть
        print('replace stat record')
        idx = a.index
        data.at[idx, 'ussi_total'] = ussi_total
        data.at[idx, 'ussi_rec'] = ussi_rec
        data.at[idx, 'ussi_mcn0'] = ussi_mcn0
    else:                                           # вставляем данные в нужное место
        a = data[data['date'] < str(date)]
        if a.empty:
            idx = 0
        else:
            idx = max(a.index) + 1
        df = pd.DataFrame([date, 0, 0, 0, 0, ussi_total, ussi_rec, ussi_mcn0]).T
        df = df.rename(columns=dict(zip(df.columns, data.columns)))
        data = pd.concat([data[:idx], df, data[idx:]])
        data = data.reset_index(drop=True)

    # сохраняем обновленые данные в (тот же) файл
    data.to_csv(filename, sep='\t', index=None)


# выбор из файла нужных данных с учетом сдвига времени
def get_data(filename):
    if not os.access(filename, os.F_OK):
        print("Can't open " + filename)
        return []
    if os.stat(filename).st_size == 0:
        print("Empty file" + filename)
        return []

    # задаем типы данных, чтобы ограничить объем и время обработки
    dtype_dict = {1: np.int32}
    for i in [3, 6, 9]:
        dtype_dict[i] = np.int16
        dtype_dict[i+1] = np.float16
        dtype_dict[i+2] = np.int16

    header = ['datetime', 'unix_time', 'rinex_id', 'prn1', 'cn1', 'el1', 'prn2', 'cn2', 'el2', 'prn3', 'cn3', 'el3']
    data = pd.read_csv(filename, header=None, sep=';', dtype=dtype_dict, parse_dates=[0], names=header)
    # print(data.info(memory_usage='deep'))

    # переводим время в другой формат
    data['tod'] = data['datetime'].apply(datetime2tod)

    # выделяем название станции из rinex_id (пока не получилось проще)
    def set_ussi_name(name):
        return name[0:4]
    data['name'] = data['rinex_id'].apply(set_ussi_name)

    # выделяем только интересующие станции в зоне двойного покрытия
    # data2 = data[(data['name'] == 'MENK') | (data['name'] == 'BRKN') | (data['name'] == 'RSDN')].reset_index()
    return data[['tod', 'name', 'rinex_id', 'cn1', 'cn2', 'cn3']]


def create_ussi_report(data, ussi):
    print('\nОтчет по', ussi)
    # создаем лист отчета
    plt.cla()
    plt.clf()
    f = plt.gcf()
    f.set_size_inches(13, 9)

    rec_stat = np.zeros((3, 4), dtype=int)

    # по каждому приемнику
    for i in range(3):
        rinex_id = ussi + str(i+1)

        # формируем непрерывную ленту времени
        data_u = pd.DataFrame(range(86400))
        data_u.columns = ['tod']
        data_u = data_u.merge(data[data['rinex_id'] == rinex_id], how='left')
        #print(data_u)

        rec_stat[i][0] = data_u['name'].count()
        rec_stat[i][1] = data_u[data_u['cn1'] != 0]['cn1'].count()
        rec_stat[i][2] = data_u[data_u['cn2'] != 0]['cn2'].count()
        rec_stat[i][3] = data_u[data_u['cn3'] != 0]['cn3'].count()

        # рисуем графики
        plot_cn02(data_u['tod'], data_u['cn1'], i, 0)
        plot_cn02(data_u['tod'], data_u['cn2'], i, 1)
        plot_cn02(data_u['tod'], data_u['cn3'], i, 2)

    print(rec_stat)
    outfile = ussi+'.png'
    plt.savefig(outfile, format='png', dpi=100)            # сохраняем полученные изображения в файл

    # печать статистики
    img = Image.open(outfile)
    draw = ImageDraw.Draw(img)
    x = 100
    draw.text((550, 20), u"Отчет по УССИ " + ussi, text_color, font=font)
    draw.text((x, 80), u"Всего сообщений:", text_color, font=font)
    draw.text((x, 100), u"по PRN 123:", text_color, font=font)
    draw.text((x, 120), u"по PRN 125:", text_color, font=font)
    draw.text((x, 140), u"по PRN 140:", text_color, font=font)
    for rec in range(3):
        x = 400+rec*250
        draw.text((x-20, 60), ussi + str(rec+1), text_color, font=font)
        draw.text((x, 80), str(rec_stat[rec][0]).rjust(5,' '), text_color, font=font)
        draw.text((x, 100), str(rec_stat[rec][1]).rjust(5,' '), text_color, font=font)
        draw.text((x, 120), str(rec_stat[rec][2]).rjust(5,' '), text_color, font=font)
        draw.text((x, 140), str(rec_stat[rec][3]).rjust(5,' '), text_color, font=font)
    img.save(outfile)


# формирование графика сигнал/шум
def plot_cn02(t, cn0, rec, signal):
    limy = 50
    prn = u"PRN %d" % (prn_num[signal])
    grid_size = (10, 3)
    ax = plt.subplot2grid(grid_size, (2+3*signal, rec), rowspan=2, colspan=1)
    plt.plot(t/3600, cn0, label=prn, color='blue')
    plt.ylim(20, limy)
    plt.xlim(0, 24)
    plt.title(prn)
    # plt.title(u"Средний сигнал/шум (дБГц)")
    # plt.legend(loc='best')
    major_ticks = np.arange(0, 25, 2)
    ax.set_xticks(major_ticks)
    plt.grid()


def main():
    data = get_data('10.log')
    # print(data)

    # data_table = data.pivot_table(index=['name','rinex_id'], values=['cn1','cn2','cn3'], aggfunc=['count','mean'])
    # print(data_table)

    # data_grouped = data.groupby(by='rinex_id')
    # print(data_grouped['cn1'].count())

    ussi_list = ['MENK', 'BRKN', 'RSDN', 'SVPL', 'SVEK']
    # ussi_list = ['MENK', 'BRKN', 'RSDN', 'SVPL', 'SVEK', 'ASTR', 'ARHG', 'SAMR']
    for ussi in ussi_list:
        create_ussi_report(data[data['name'] == ussi], ussi)


if __name__ == "__main__":
    main()
