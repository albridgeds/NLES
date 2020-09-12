#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import datetime as dt
import pandas as pd
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import time
from sys import platform
from optparse import OptionParser
import report_download as rd
import report_plot as rp
import report_common as rc


windows_test = True if platform == 'win32' else False
os_path = '' if windows_test else './'
out_path = '' if windows_test else '/mnt/shares/kzik_report/'
fontPath = 'C:\Windows\Fonts\lucon.ttf' if windows_test else '/usr/share/fonts/ttf/dejavu/DejaVuSansMono.ttf'


# индексы столбцов
'''ind_tod = 0
ind_az = 2
ind_el = 3
ind_pow1 = 13
ind_pow2 = 14
ind_pow3 = 15
ind_temp1 = 16
ind_temp2 = 17
ind_temp3 = 18'''


# расчет процента измерений за сутки
def procent(msg):
    return float(msg)/86400*100


# преобразование времени
def tod2hms(tod):
    hour = int(tod/3600)
    min = int((tod-hour*3600)/60)
    sec = tod - hour*3600 - min*60
    return hour, min, sec


# чтение данных из файла
def read_data(namefile):
    data = list()
    if not os.access(namefile, os.F_OK):
        print("Can't open " + namefile)
        # return 0
    elif os.stat(namefile).st_size == 0:
        print("Empty file" + namefile)
        # return 0
    else:
        print("open " + namefile)
        data = pd.read_csv(namefile, header=None)
        data = data.set_index(0)
    return data


# чтение данных из файла
def read_data_tle(namefile):
    data = read_data(namefile)
    if len(data) == 0:
        data = pd.DataFrame([[0,0,0]])      # чтобы не поломались графики
        return data

    tod = []
    for stime in data.index:
        hour = int(stime[11:13])
        min = int(stime[14:16])
        sec = int(stime[17:19])
        tod.append((hour*3600 + min*60 + sec + 3*3600) % 86400)
    data.index = tod
    data.sort_index(inplace=True)
    data.columns = ['az','el']
    return data


#
# создание отчета
#
def create_report(num, date, force, test):

    # cоздаем отчет на КЗиК, загружаем его, читаем данные
    ifile = rc.kzik_name[num] + '-' + date.strftime("%y%m%d") + '.log'
    kzik_getlog = './kzik_getlog -y %d -m %d -d %d' % (date.year, date.month, date.day)
    rd.execute_ssh(rc.kzik_ip[num], 'user', 'user', commands=[kzik_getlog])
    rd.download_ssh(rc.kzik_ip[num], 'user', 'user',
                    remote_file='./'+ifile, local_file=os_path+ifile,
                    force=True, remove=True)
    data = read_data(ifile)

    # удаляем ненужный локальный файл
    if not test:
        print("remove", ifile)
        os.remove(ifile)

    # отрицательную мощность заменить на ноль (1, чтобы было видно на графике с точками)
    data[[13,14,15]] = data[[13,14,15]].replace(-22.2, 1)

    # уровень пилот-сигнала
    data['pilot_ref'] = 100 + data[10]
    data['pilot'] = 100 + data[10] + 2*data[11]

    # опорное время tle из geo.txt
    rd.download_ssh('192.168.31.150', 'user', 'user',
                    './tle/data/geo.txt', os_path+'tmp_geo.txt',
                    force=True)
    tle_reftime = get_tle_reftime(num, os_path+'tmp_geo.txt')

    # время обновления geo.txt на 31.150
    tle_mtime = rd.get_mtime('192.168.31.150', 'user', 'user', './tle/data/geo.txt')

    # программа наведения антенны на КЗиК
    track_path = './track_kzik%d.csv' % (num + 1)
    rd.download_ssh(rc.kzik_ip[num], 'user', 'user', track_path, os_path + 'tmp_tle.csv', force=True)
    data_tle = read_data_tle('./tmp_tle.csv')

    # объединяем в единый DataFrame
    data = data.join(data_tle, how='outer')

    # время обновления программы наведения на КЗиК
    kzik_mtime = rd.get_mtime(rc.kzik_ip[num], 'user', 'user', track_path)

    # время обновления программы на БЗК1
    bzk1_mtime = rd.get_mtime(rc.bzk_ip[num][0], 'user', rc.bzk_pass[num][0], './tle_params.out')
    bzk2_mtime = rd.get_mtime(rc.bzk_ip[num][1], 'user', rc.bzk_pass[num][1], './tle_params.out')

    # curs_date = check_curs_date(kzik_num)
    # print(track_mtime, tle_reftime, tle_mtime)

    # графики
    rp.init(size=(4, 13))
    rp.plot_universal(data[['pilot_ref', 'pilot']], (2, 0), (1, 1), u'Пилот-сигнал', [u'Опорный', u'Комбинация'])
    rp.plot_universal(data[['az', 2]], (4, 0), (2, 1), u'Азимут', [u'Программа', u'Датчик'])
    rp.plot_universal(data[['el', 3]], (7, 0), (2, 1), u'Угол места', [u'Программа', u'Датчик'])
    rp.plot_universal(data[[13, 14, 15]], (10, 0), (2, 1), u'Мощность', [u'УМ1', u'УМ2', u'УМ3'], (0, 100))
    rp.plot_universal(data[[16, 17, 18]], (13, 0), (2, 1), u'Температура', [u'УМ1', u'УМ2', u'УМ3'], (0, 100))

    '''if len(data) != 0:
        plot_az(data[0], data[2], data_tle[0], data_tle[1])
        plot_el(data[0], data[3], data_tle[0], data_tle[2])
        plot_power(data[0], data[13], data[14], data[15])
        plot_temp(data[0], data[16], data[17], data[18])
    else:
        print('no data in %s' % ifile)'''

    ofile = os_path + 'tmp_rfdev' + str(num+1) + '.png'
    rp.save(ofile)

    print_header(ofile, num, date, tle_reftime, tle_mtime, kzik_mtime, bzk1_mtime, bzk2_mtime)
    # !!!надо создавать соответствующий отчет для отсутствующих данных


def datetime2string(input_dt):
    #return u"Н/Д" if input_dt == 0 else input_dt.strftime("%d.%m.%Y %H:%M:%S")
    return u"        -Н/Д-" if not input_dt else input_dt.strftime("%d.%m.%Y %H:%M:%S")

#
# печать заголовка
#
def print_header(ofile, kzik, date, tle_reftime, tle_mtime, kzik_mtime, bzk1_mtime, bzk2_mtime):
    # шрифты
    font = ImageFont.truetype(fontPath, 16)
    text_color = (0, 0, 0)

    img = Image.open(ofile)
    draw = ImageDraw.Draw(img)

    header_date = u"Дата: " + date.strftime("%d.%m.%Y")
    s_cur_time = u"Время отчета:       " + dt.datetime.now().strftime("%d.%m.%Y %H:%M:%S")

    kzik_num = u"Отчет КЗиК №" + str(kzik+1)
    if kzik == 0:
        sat_name = u"KA \"Луч-5В\" (PRN 140)"
    elif kzik == 1:
        sat_name = u"KA \"Луч-5Б\" (PRN 125)"
    else:
        sat_name = u"KA ???"

    # вывод заголовка по КЗиК
    x = 0           # начальное смещение текста по горизонтали
    y = 20          # начальное смещение текста по вертикали
    draw.text((x, y), header_date, text_color, font=font)
    draw.text((x, y + 20), kzik_num, text_color, font=font)
    draw.text((x, y + 40), sat_name, text_color, font=font)
    draw.text((x, y + 80), s_cur_time, text_color, font=font)
    draw.text((x, y + 100), u"Опорное время TLE:  " + datetime2string(tle_reftime), text_color, font=font)
    draw.text((x, y + 120), u"Обновление TLE:     " + datetime2string(tle_mtime), text_color, font=font)
    draw.text((x, y + 140), u"Обновление ЦУ КЗиК: " + datetime2string(kzik_mtime), text_color, font=font)
    draw.text((x, y + 160), u"Обновление ЦУ БЗК1: " + datetime2string(bzk1_mtime), text_color, font=font)
    draw.text((x, y + 180), u"Обновление ЦУ БЗК2: " + datetime2string(bzk2_mtime), text_color, font=font)

    img.save(ofile)


#
# опорное время tle из файла geo.txt
#
def get_tle_reftime(kziknum, filename):
    print('Check TLE file date')
    satname = ['LUCH 5V','LUCH 5B']
    try:
        f = open(filename, 'r')
    except:
        print('can\'t open geo.txt')
        return 0

    line = f.readline()
    while line:
        if line.find(satname[kziknum]) != -1:
            line = f.readline()
            break
        line = f.readline()
    f.close()

    year = int(line[18:20])
    yday = int(line[20:23])
    tod = float(line[24:30]) / 1000000
    [h, m, s] = tod2hms(int(86400*tod))
    rep_time = dt.datetime(2000+year, 1, 1, h, m, s) + dt.timedelta(days=yday-1, hours=3)
    return rep_time


# проверка даты создания последнего отчета для ЦУРС
def check_curs_date(kziknum):
    print('Check curs (empty)')


def str2date(s_date):
    year = 2000 + int(s_date[0:2])
    month = int(s_date[2:4])
    day = int(s_date[4:6])
    return dt.date(year, month, day)


def main():

    parser = OptionParser()
    parser.add_option('-f', '--force', dest='force', action='store_true', default=False, help="force logfile reload")
    parser.add_option('-d', '--date', dest='date', action='store', default=0, help="specific date (example for 29 May 2018: 180529)")
    parser.add_option('-y', '--yesterday', dest='yesterday', action='store_true', default=False, help="quick yesterday date")
    parser.add_option('-l', '--log', dest='log', action='store_true', default=False, help="save stat to log")
    parser.add_option('-t', '--test', dest='test', action='store_true', default=False, help="test mode - don't remove file")
    parser.add_option('-k', '--kziknum', dest='kziknum', action='store', default='1', help="specific kzik number (all by default)")
    parser.add_option('-e', '--external', dest='ext', action='store_true', default=False, help="external call")
    options, args = parser.parse_args()

    rep_date = dt.date.today()
    if options.yesterday:
        rep_date = dt.date.today() - dt.timedelta(days=1)
    elif options.date:
        rep_date = str2date(options.date)
    kziknum = int(options.kziknum) - 1
    print('\nKZiK%d RF_dev report for %s' % (kziknum+1, rep_date))

    create_report(kziknum, rep_date, options.force, options.test)

if __name__ == "__main__":
    main()
