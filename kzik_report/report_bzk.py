#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import datetime as dt
import pandas as pd
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from PIL import Image, ImageDraw, ImageFont
from sys import platform
from optparse import OptionParser
import paramiko
import xml.etree.cElementTree as ET

import report_download as rd
import report_plot as rp
import report_common as rc


mpl.use('Agg')

windows_test = True if platform == 'win32' else False
os_path = '' if windows_test else './'
out_path = '' if windows_test else '/mnt/shares/kzik_report/'
fontPath = 'C:\Windows\Fonts\lucon.ttf' if windows_test else '/usr/share/fonts/ttf/dejavu/DejaVuSansMono.ttf'

font = ImageFont.truetype(fontPath, 16)
text_color = (0, 0, 0)

kzik_name = ["kzik1 (5v)", "kzik2 (5b)"]
bzk_name = ["bzk11", "bzk12", "bzk21", "bzk22"]

# индексы столбцов в лог-файле
idx_tod = 0
idx_st_siggen = 6
idx_st_rec = 9
idx_st_sisnet = 11
idx_cn_sdcm = 26
idx_cn_egnos = 33
idx_dopp_sdcm = 27
idx_dopp_egnos = 34
idx_chiprate = 42
idx_sg_freq = 45
idx_sg_chiprate = 46
idx_psr_sdcm = 28
idx_psr_egnos = 35

# отступ текста от края страницы
X0 = 120
Y0 = 20


# Вывод пустой таблички в случае, если нет данных ни по одному БЗК
def header_not_data(ofile, kzik, date):
    img = Image.open(ofile)
    draw = ImageDraw.Draw(img)
    print_header(draw, kzik, date)
    draw.text((X0, Y0 + 120), u"Данных нет", text_color, font=font)
    draw.text((X0, Y0 + 220), u"Данных нет", text_color, font=font)
    img.save(ofile)


# печать данных по БЗК
def text_bzk(draw, bzk_num, msg_stat, delta, n):

    X = X0+200          # начальное смещение текста по горизонтали
    Y = Y0+80           # начальное смещение текста по вертикали
    if bzk_num == 1:    # для БЗК №2
        X += 250

    no_data = u'      Н/Д'
    if not len(msg_stat):
        draw.text((X, Y), no_data, text_color, font=font)
        draw.text((X, Y + 40), no_data, text_color, font=font)
        draw.text((X, Y + 60), no_data, text_color, font=font)
        draw.text((X, Y + 80), no_data, text_color, font=font)
        draw.text((X, Y + 120), no_data, text_color, font=font)
        draw.text((X, Y + 140), no_data, text_color, font=font)
        draw.text((X, Y + 160), no_data, text_color, font=font)
        draw.text((X, Y + 200), no_data, text_color, font=font)
        return

    def procent(val):
        return float(val) / 86400 * 100

    def stat2string(val):
        return '%5d (%6.2f%%)' % (val, procent(val))

    sisnet = stat2string(msg_stat.loc['ok', 'sisnet'])
    sat1_ok = stat2string(msg_stat.loc['ok', 'sdcm'])
    sat1_no = stat2string(msg_stat.loc['loss', 'sdcm'])
    sat1_bad = stat2string(msg_stat.loc['error', 'sdcm'])
    sat2_ok = stat2string(msg_stat.loc['ok', 'control'])
    sat2_no = stat2string(msg_stat.loc['loss', 'control'])
    sat2_bad = stat2string(msg_stat.loc['error', 'control'])

    draw.text((X, Y), sisnet, text_color, font=font)
    draw.text((X, Y+40), sat1_ok, text_color, font=font)
    draw.text((X, Y+60), sat1_no, text_color, font=font)
    draw.text((X, Y+80), sat1_bad, text_color, font=font)
    draw.text((X, Y+120), sat2_ok, text_color, font=font)
    draw.text((X, Y+140), sat2_no, text_color, font=font)
    draw.text((X, Y+160), sat2_bad, text_color, font=font)

    if n == 0:
        draw.text((X, Y + 200), no_data, text_color, font=font)
    else:
        dopp = '%.1f (%.1f%%)' % (delta, procent(n))
        draw.text((X, Y + 200), dopp, text_color, font=font)


# доделать
def bzk_table():
    table = pd.DataFrame()


#
# печать заголовка
#
def print_header(draw, kzik, date):
    X = X0          # начальное смещение текста по горизонтали
    Y = Y0          # начальное смещение текста по вертикали

    header_date = u"Дата: " + date.strftime("%d.%m.%Y")
    kzik_num = u"Отчет КЗиК №" + str(kzik+1)

    # текст заголовка
    if kzik == 0:
        sat_name = u"KA \"Луч-5В\" (PRN 140)"
        header_bzk = [u"_____БЗК11_____", u"_____БЗК12_____"]
        header_prn = ["PRN 140", "PRN 123"]
    elif kzik == 1:
        sat_name = u"KA \"Луч-5Б\" (PRN 125)"
        header_bzk = [u"_____БЗК21_____", u"_____БЗК22_____"]
        header_prn = ["PRN 125", "PRN 123"]
    else:
        sat_name = u"KA ???"
        header_bzk = [u"_____БЗК?1_____", u"_____БЗК?2_____"]
        header_prn = ["PRN ???", "PRN ???"]

    # вывод заголовка по КЗиК
    draw.text((X, Y), header_date, text_color, font=font)
    draw.text((X, Y + 20), kzik_num, text_color, font=font)
    draw.text((X, Y + 40), sat_name, text_color, font=font)
    # draw.text((X+130, Y + 40), prn, text_color, font=font)
    draw.text((X+200, Y + 60), header_bzk[0] + " " * 10 + header_bzk[1], text_color, font=font)

    # вывод заголовка по БЗК
    draw.text((X, Y + 80), u"Sisnet получено:   ", text_color, font=font)
    draw.text((X, Y + 120), header_prn[0] + u" принято:", text_color, font=font)
    draw.text((X, Y + 140), header_prn[0] + u" потеряно:", text_color, font=font)
    draw.text((X, Y + 160), header_prn[0] + u" искажено:", text_color, font=font)
    draw.text((X, Y + 200), header_prn[1] + u" принято:", text_color, font=font)
    draw.text((X, Y + 220), header_prn[1] + u" потеряно:", text_color, font=font)
    draw.text((X, Y + 240), header_prn[1] + u" искажено:", text_color, font=font)
    draw.text((X, Y + 280), u"Сдвиг частоты:", text_color, font=font)


def tod2hms(tod):
    hour = int(tod/3600)
    minute = int((tod-hour*3600)/60)
    sec = tod - hour*3600 - minute*60
    return hour, minute, sec


def check_losses(time, value, log, fill):
    loss_flag = False
    loss_start = 0

    if log:
        f = open(os_path+log, 'w')

    for t in range(len(time)):
        # начало разрыва
        if value[t] > 0 and loss_flag is False:
            loss_flag = True
            loss_start = t
        # конец разрыва
        if value[t] == 0 and loss_flag is True:
            loss_flag = False
            dtime = time[t] - time[loss_start]
            [hh, mm, ss] = tod2hms(time[loss_start])

            if log:
                if dtime == 1:
                    print('Loss: %02d:%02d:%02d' % (hh, mm, ss))
                    f.write('\nLoss: %02d:%02d:%02d' % (hh, mm, ss))
                else:
                    [hh1, mm1, ss1] = tod2hms(time[t]-1)
                    print('Loss: %02d:%02d:%02d - %02d:%02d:%02d  (%d sec)' % (hh, mm, ss, hh1, mm1, ss1, dtime))
                    f.write('\nLoss: %02d:%02d:%02d - %02d:%02d:%02d  (%d sec)' % (hh, mm, ss, hh1, mm1, ss1, dtime))

            if fill:
                if dtime > 500:
                    for i in np.arange(loss_start, t, 500):
                        #value[i] = 0       # !!! предупреждение "A value is trying to be set on a copy of a slice from a DataFrame"
                        value.loc[i] = 0
    if log:
        f.close()
    return value


#
# вывод графика состояния
#
def plot_state(x, y1, y2, y3, bzk):
    grid_size = (15, 2)
    ax = plt.subplot2grid(grid_size, (3, bzk), rowspan=2, colspan=1)

    y1 = check_losses(x, y1, False, True)
    y2 = check_losses(x, y2, False, True)
    y3 = check_losses(x, y3, 'losses_siggen.log', True)

    # выделяем моменты, когда программа БЗК вообще не работала (нет записей в лог-файле)
    x0 = np.arange(0.0, 86400.0)
    y0 = np.ones(86400)
    y0[1:86400:2] = 0
    for i in x:
        y0[i] = 0

    y0 = y0 * 2
    y1 = y1 * 0.5
    y3 = y3 * 0.75

    plt.plot(x0/3600, y0, label='bzk', color='grey')
    plt.plot(x/3600, y2, label='receiver', color='blue')
    plt.plot(x/3600, y3, label='sisnet', color='orange')
    plt.plot(x/3600, -y1, label='siggen', color='red')
    plt.ylim(-1.5, 1.5)
    plt.xlim(0, 24)
    plt.legend(loc='best')
    major_ticks = np.arange(0, 25, 2)
    ax.set_xticks(major_ticks)
    plt.yticks([-1, 0, 1], ['Fault', 'OK', 'Loose'])
    plt.grid()


#
# формирование графика сигнал/шум
#
def plot_cn0(x, y1, y2, bzk):
    grid_size = (15, 2)
    ax = plt.subplot2grid(grid_size, (6, bzk), rowspan=2, colspan=1)
    plt.plot(x/3600, y2, label=u"Контроль", color='orange')
    plt.plot(x/3600, y1, label=u"СДКМ", color='blue')
    plt.ylim(20, 50)
    plt.xlim(0, 24)
    plt.title(u"Сигнал/шум (дБГц)")
    plt.legend(loc='best')
    major_ticks = np.arange(0, 25, 2)
    ax.set_xticks(major_ticks)
    plt.grid()


#
# формирование графика частоты
#
def plot_doppler(x, y1, y2, y3, bzk):
    grid_size = (15, 2)
    ax = plt.subplot2grid(grid_size, (9, bzk), rowspan=2, colspan=1)
    plt.plot(x/3600, -y3, label=u"СДКМкод", color='green')
    plt.plot(x/3600, y2, label=u"Контроль", color='orange')
    plt.plot(x/3600, y1, label=u"СДКМ", color='blue')
    plt.ylim(-200, 200)
    plt.xlim(0, 24)
    plt.title(u"Допплер (Гц)")
    plt.legend(loc='best')
    major_ticks = np.arange(0, 25, 2)
    ax.set_xticks(major_ticks)
    plt.grid()


#
# формирование графика частоты формирователя
#
def plot_siggen(x, y1, y2, bzk):
    grid_size = (15, 2)
    ax = plt.subplot2grid(grid_size, (12, bzk), rowspan=1, colspan=1)
    plt.plot(x/3600, y2*10, label=u"код х10", color='green')
    plt.plot(x/3600, y1, label=u"частота", color='blue')
    plt.title(u"Siggen (Гц)")
    plt.legend(loc='best')
    plt.xlim(0, 24)
    major_ticks = np.arange(0, 25, 2)
    ax.set_xticks(major_ticks)
    y_max = max(max(abs(y1)), max(abs(y2)))
    y_lim = np.ceil(y_max/1000)*1000
    plt.ylim(-y_lim, y_lim)
    ax.set_yticks(np.arange(-y_lim, y_lim+100, y_lim))
    plt.grid()


#
# формирование графика дальности
#
def plot_range(x, y1, y2, bzk):
    grid_size = (15, 2)
    ax = plt.subplot2grid(grid_size, (14, bzk), rowspan=1, colspan=1)
    plt.plot(x/3600, y2, label=u"Контроль", color='orange')
    plt.plot(x/3600, y1, label=u"СДКМ", color='blue')
    plt.ylim(3.8e7, 4.2e7)
    plt.xlim(0, 24)
    plt.title(u"Псевдодальность (м)")
    # plt.legend(loc='best')
    major_ticks = np.arange(0, 25, 2)
    ax.set_xticks(major_ticks)
    plt.grid()


#
# расчет доплера
#
def calc_dopp(data_bzk, name):
    if len(data_bzk) == 0:
        print("%s no doppler data" % name)
        return 0, 0
    dopp = data_bzk[27]
    chiprate = data_bzk[42]
    delta = []
    for i in range(len(dopp)-5):
        if dopp[i+5] != 0.0 and dopp[i+4] != 0.0 and dopp[i+3] != 0.0 and dopp[i+2] != 0.0 and dopp[i+1] != 0.0 and dopp[i] != 0.0:
            delta.append(chiprate[i+5]+dopp[i+5])
    n = len(delta)
    if n != 0:
        mdelta = sum(delta)/n
        print("%s doppler delta = %.2f (%d epochs)" % (name, mdelta, n))
        return mdelta, n
    else:
        print("%s no valid doppler data" % name)
        return 0, 0


#
# расчет среднего отношения сигнал/шум
#
def calc_cn0(data_bzk, bzknum):
    if len(data_bzk) == 0:
        print("%s no cn0 data" % bzk_name[bzknum])
        return 0
    cn0 = data_bzk[26]
    _cn0 = cn0[cn0 != 0]
    n = len(_cn0)
    if n != 0:
        m_cn0 = sum(_cn0)/n
        print("%s mean cn0 = %.2f (%d epochs)" % (bzk_name[bzknum], m_cn0, n))
        return m_cn0
    else:
        print("%s no valid cn0 data" % bzk_name[bzknum])
        return 0


#
# статистика по доставке сообщений
#
def calc_stat(data_bzk):

    stat = pd.DataFrame()
    if len(data_bzk) == 0:
        return stat

    for col in [9, 10, 11]:
        stat.loc['ok', col] = len(data_bzk[data_bzk[col] == 0])
        stat.loc['loss', col] = len(data_bzk[data_bzk[col] > 0])
        stat.loc['error', col] = len(data_bzk[data_bzk[col] < 0])
        # разделить error на отсекаемые и пропускаемые приемником (для сертификации)?
    stat = stat.astype('int32')
    stat.columns = ['sdcm', 'control', 'sisnet']
    # print(stat)
    return stat


#
# печать графиков по БЗК
#
def plot_bzk_data(data_bzk, bzk_num):
    if len(data_bzk):
        rp.plot_universal(data_bzk[[6, 9, 11]], (3, bzk_num), (2, 1))
        rp.plot_universal(data_bzk[[33, 26]], (6, bzk_num), (2, 1), u'Сигнал/шум, дБГц', [u'Контроль', u'СДКМ'])
        rp.plot_universal(data_bzk[[42, 34, 27]], (9, bzk_num), (2, 1), u'Допплер, Гц', [u'СДКМкод', 'Контроль', u'СДКМ'])
        rp.plot_universal(data_bzk[[45, 46]], (12, bzk_num), (1, 1), u'Генератор, Гц', [u'Частота', u'Код х10'])
        rp.plot_universal(data_bzk[[28, 35]], (14, bzk_num), (1, 1), u'Псевдодальность')


#
# создание отчета
#
def create_report(kzik_num, date, force, test, tle_correction):

    ofile = form_tmpfile(kzik_num)
    rp.init(size=(9, 13), grid=(15, 2))

    msg_stat = []
    dopp_val = [0, 0]
    dopp_n = [0, 0]

    for i in range(2):
        # загрузка лог-файла
        ifile = download_logs(kzik_num, i, date, force, test)
        # чтение файла
        data_bzk = rc.read_data(ifile)
        # работа с данными, если они есть
        if len(data_bzk):
            # подготовка столбцов
            data_bzk[46] = data_bzk[46]*10
            data_bzk[42] = -data_bzk[42].where(abs(data_bzk[42]) < 10000)
            # вывод графиков
            plot_bzk_data(data_bzk, i)
            # статистика по сдвигу частоты (переделать!)
            dopp_val[i], dopp_n[i] = calc_dopp(data_bzk, rc.bzk_name[kzik_num][i])

        # статистика по доставке сообщений
        msg_stat.append(calc_stat(data_bzk))
    rp.save(ofile)

    # расчет среднего сигнал/шум
    # m_cn01 = calc_cn0(data_bzk1, kzik_num*2)
    # m_cn02 = calc_cn0(data_bzk2, kzik_num*2+1)

    # печать статистики
    img = Image.open(ofile)
    draw = ImageDraw.Draw(img)
    print_header(draw, kzik_num, date)
    #text_bzk(draw, 1, msg_ok1, msg_no1, msg_bad1, delta1, n1)
    #text_bzk(draw, 2, msg_ok2, msg_no2, msg_bad2, delta2, n2)

    for i in range(2):
        text_bzk(draw, i, msg_stat[i], dopp_val[i], dopp_n[i])
    img.save(ofile)

    '''
    # итоговая дневная статистика для записи в файл
    daystat_sisnet = msg_ok1[2] if msg_ok1[2] > msg_ok2[2] else msg_ok2[2]
    daystat_sat = msg_ok1[0] if msg_ok1[0] > msg_ok2[0] else msg_ok2[0]
    daystat_ddopp = delta1 if msg_ok1[2] > msg_ok2[2] else delta2
    daystat_cn0 = m_cn01 if msg_ok1[2] > msg_ok2[2] else m_cn02     # сделать здесь средний сигнал-шум !!!

    config_mtime, freq_corr = read_tle_config(kzik_num)
    # print(config_mtime)
    # print(freq_corr)
    if tle_correction:
        tle_correct(config_mtime, kzik_num, freq_corr, daystat_sat, daystat_ddopp, date)
    save_daystat(daystat_sisnet, daystat_sat, daystat_ddopp, daystat_cn0, kzik_num, date, freq_corr)
    '''


#
# запись в файл процента доставки
#
def save_daystat(sisnet, satrec, ddopp, cn0, kzik_num, date, freq_corr):
    year = date.year
    filename = os_path + 'kzik' + str(kzik_num+1) + '_' + str(year) + '.log'

    columns = ['date', 'sisnet', 'satrec', 'ddopp', 'cn0', 'freq_corr']
    new_row = pd.DataFrame([[pd.to_datetime(date), sisnet, satrec, round(ddopp,1), round(cn0,1), freq_corr]],
                           columns=columns)
    new_row = new_row.set_index('date')

    if os.path.exists(filename):
        data = pd.read_csv(filename, sep='\t', index_col='date', parse_dates=True, encoding='utf-8')
        try:
            data = data.drop(new_row.index)
        except:
            print('add stat record')
        else:
            print('replace stat record')
        data = data.append(new_row)
        data = data.sort_index()
    else:
        print('create new dataframe')
        data = new_row

    # сохраняем обновленые данные в (тот же) файл
    data.to_csv(filename, sep='\t', date_format='%Y-%m-%d')


def tle_correct(config_mtime, kzik_num, freq_corr, satrec, ddopp, date):
    # запуск коррекции частоты, если набрано достаточно суточной статистики
    if satrec < 80000:
        print('\nNo tle_config correction can be made: not enough measurements')
        return

    if abs(ddopp) < 10:
        print('\nNo tle_config correction can be made: too small correction')
        return

    # коррекция была недавно
    cur_time = dt.datetime.now()
    ddate = cur_time - config_mtime
    print(cur_time, ddate)
    if ddate < dt.timedelta(hours=23):
        print('\ntle_config correction was made recently, no new correction required')
        return

    # коррекция была позже отчета по БЗК
    # report_time = dt.datetime.strptime(date, '%Y-%m-%d')
    # print(report_time)
    if date < config_mtime:
        print('\ntle_config correction was made after the date of report, no new correction required')
        return

    new_freq = freq_corr + ddopp
    print('\nPerform tle_config correction for kzik%d from %d to %d' % (kzik_num + 1, freq_corr, new_freq))
    # edit_tle_config(kzik_num, new_freq)
    # arg = ['./tle_config.py', '-k' + str(kzik_num+1), '-d' + str(ddopp), '-t' + str(date)]
    # call(arg)


#
# чтение текущего значения коррекции частоты
#
def read_tle_config(kzik_num):

    local_filename = os_path + 'local_tle.xml'

    # копируем файл с сервера в локальное хранилище
    if windows_test:
        config_mtime = dt.datetime.today()
    else:
        config_mtime = download_tle_file('192.168.0.222', 'user', 'user', '/etc/tle/config.xml', local_filename, True)
        print(config_mtime)

    if not os.path.exists(local_filename):
        return config_mtime, np.nan

    # читаем локальный файл
    tree = ET.ElementTree(file=local_filename)
    root = tree.getroot()

    # находим нужный КЗиК
    for kzik in root.iter('station'):
        if kzik.attrib.get('id') == str(kzik_num+1):
            child = kzik.find('correction')
            freq = float(child.attrib.get('doppler'))
            # print('current doppler correction is', freq)
            return config_mtime, freq


#
# запись нового значения коррекции частоты
#
def edit_tle_config(kzik_num, new_freq):
    local_filename = os_path + 'local_tle.xml'

    # читаем локальный файл
    tree = ET.ElementTree(file=local_filename)
    root = tree.getroot()
    # находим нужный КЗиК
    for kzik in root.iter('station'):
        if kzik.attrib.get('id') == kzik_num+1:
            # заменяем атрибут коррекция частоты
            child = kzik.find('correction')
            child.attrib['doppler'] = str(new_freq)
            # print('change doppler correction to', new_freq)

    # сохраняем локальный файл
    tree = ET.ElementTree(root)
    with open(local_filename, "w") as f:
        if windows_test:
            tree.write(f, encoding='unicode')
        else:
            tree.write(f)

    # переписываем файл на сервере исправленным локальным файлом
    if not windows_test:
        upload_file('192.168.0.222', 'user', 'user', local_filename, '/home/user/config1.xml', '/etc/tle/config.xml')


def download_tle_file(host, user, secret, remote_file, local_file, get_date=False):
    print('download remote file to local storage')
    # подключение по ssh
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(hostname=host, username=user, password=secret, port=22)
    # копирование файла
    sftp = ssh.open_sftp()
    sftp.get(remote_file, local_file)
    sftp.close()

    if get_date:
        # дата изменения исходного файла
        stdin, stdout, stderr = ssh.exec_command('ls --full-time ' + remote_file)
        data = stdout.read().split()
        str_date = data[5].decode('UTF-8') + ' ' + data[6].decode('UTF-8')[0:8]
        file_date = dt.datetime.strptime(str_date, '%Y-%m-%d %H:%M:%S')
        ssh.close()
        return file_date
    ssh.close()
    return


def upload_file(host, user, secret, local_file, remote_file, final_file=''):
    print('upload local file to remote home')
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        ssh.connect(hostname=host, username=user, password=secret, port=22)
    except:
        print('no connection to', host)
        ssh.close()
        return
    else:
        # поскольку нет возможности копирования под суперпользователем, записываем файл во временную папку на сервере
        sftp = ssh.open_sftp()
        sftp.put(local_file, remote_file)
        sftp.close()
        if final_file:
            # переписываем файл через sudo (может не понадобиться, если файл будет полностью доступен для user)
            stdin, stdout, stderr = ssh.exec_command('sudo mv %s %s' % (remote_file, final_file), get_pty=True)
            stdin.write(secret + '\r\n')
            stdin.flush()
            data = stdout.read()        # не понимаю почему, но без этой строчки не работает (м.б. слишком быстро закрывает соединение?)
        ssh.close()


# имя выходного файла
def form_savefile(date, ext):
    name = out_path + "kzik_" + date.strftime("%y%m%d") + ".png"
    return name


# имя входного файла
def form_openfile(pref, date):
    name = pref + "-" + date.strftime("%y%m%d") + '.log'
    return name


# имя входного файла
def form_tmpfile(kzik_num):
    name = os_path + 'tmp_bzk' + str(kzik_num+1) + '.png'
    return name


#
# загрузка лог-файлов с БЗК
#
def download_logs(kziknum, bzknum, rep_date, force, test):

    ifile = rc.bzk_name[kziknum][bzknum] + '-' + rep_date.strftime("%y%m%d") + '.log'

    # проверка существования файла здесь, чтобы не собирать лог-файл на БЗК
    if not force:
        if os.path.exists(ifile):
            if os.path.getsize(ifile):
                print(ifile, 'exist, do not reload')
                return ifile

    bzk_getlog = './bzk_getlog -y %d -m %d -d %d' % (rep_date.year, rep_date.month, rep_date.day)
    rd.execute_ssh(rc.bzk_ip[kziknum][bzknum], 'user', rc.bzk_pass[kziknum][bzknum], commands=[bzk_getlog])
    rd.download_ssh(rc.bzk_ip[kziknum][bzknum], 'user', rc.bzk_pass[kziknum][bzknum],
                    remote_file='./' + ifile, local_file=os_path + ifile,
                    force=True, remove=test)
    return ifile


def main():
    parser = OptionParser()
    parser.add_option('-f', '--force', dest='force', action='store_true', default=False, help="force logfile reload")
    parser.add_option('-d', '--date', dest='date', action='store', default=0, help="specific date (example for 29 May 2018: 180529)")
    parser.add_option('-n', '--night', dest='night', action='store', default=0, help="end of period (example for 4 June 2018: 180604)")
    parser.add_option('-y', '--yesterday', dest='yesterday', action='store_true', default=False, help="quick yesterday date")
    parser.add_option('-l', '--log', dest='log', action='store_true', default=False, help="save stat to log")
    parser.add_option('-t', '--test', dest='test', action='store_true', default=False, help="test mode - don't remove file")
    parser.add_option('-c', '--corr', dest='tle_correction', action='store_true', default=False, help="tle frequency correction")
    parser.add_option('-k', '--kziknum', dest='kziknum', action='store', default=0, help="specific kzik number (1 & 2 by default)")
    parser.add_option('-b', '--bzknum', dest='bzkofkzik', action='store', default=0, help="specific bzk number (1 & 2 by default)")
    options, args = parser.parse_args()

    rep_date = dt.date.today()
    if options.yesterday:
        rep_date = dt.date.today() - dt.timedelta(days=1)
    elif options.date:
        rep_date = dt.datetime.strptime(options.date, '%y%m%d').date()
    options.kziknum = int(options.kziknum)

    for kziknum in range(2):
        # print('kziknum=', kziknum)
        if options.kziknum and kziknum != options.kziknum-1:
            continue

        print('\n_______________________________')
        print('KZiK%d BZK report for %s' % (kziknum+1, rep_date))

        # print("\nstart %s" % kzik_name[j])
        create_report(kziknum, rep_date, options.force, options.test, options.tle_correction)


if __name__ == "__main__":
    main()
