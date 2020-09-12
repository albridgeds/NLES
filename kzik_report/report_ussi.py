#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import datetime as dt
import pandas as pd
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from PIL import Image, ImageFont, ImageDraw
from sys import platform
from optparse import OptionParser
from ftplib import FTP
mpl.use('Agg')


ls = 18
prn_num = []

# настройки для windows и linux
win_flag = True if platform == "win32" else False
font_path = 'C:\Windows\Fonts\lucon.ttf' if win_flag else '/usr/share/fonts/ttf/dejavu/DejaVuSansMono.ttf'
os_path = '' if win_flag else './'
out_path = '' if win_flag else '/mnt/shares/kzik_report/'


# расчет процента измерений за сутки
def percent(msg):
    return float(msg)/86400*100


# печать данных по БЗК
def text_ussi(draw, date, total, num_ok, mean_cn0):
    font = ImageFont.truetype(font_path, 16)
    text_color = (0, 0, 0)
    x = 120                     # начальное смещение текста по горизонтали
    y = 20                      # начальное смещение текста по вертикали
    header_date = u"Дата: " + date.strftime("%d.%m.%Y")
    draw.text((x, y), header_date, text_color, font=font)
    draw.text((x, y+20), u"Отчет УССИ", text_color, font=font)

    str_total = '%5d (%6.2f%%)' % (total, percent(total))
    draw.text((x, y + 60), u"Всего отчетов:", text_color, font=font)
    draw.text((x + 300, y + 60),  str_total, text_color, font=font)
    draw.text((x, y + 100), u"Получено сообщений SBAS (процент пропусков):", text_color, font=font)
    draw.text((x + 50, y + 270), u"Количество приемников", text_color, font=font)
    draw.text((x + 400, y + 270), u"Средний сигнал/шум (дБГц)", text_color, font=font)

    for i in range(len(prn_num)):
        str_prn = u"          PRN %d" % (prn_num[i])
        perc_loss = float(total - num_ok[i])/total*100
        str_loss = '%5d (%6.2f%%)' % (num_ok[i], perc_loss)
        draw.text((x, y + 120 + i*20), str_prn, text_color, font=font)
        draw.text((x+300, y + 120 + i*20), str_loss, text_color, font=font)
        str_cn0 = u'%4.1f дБГц' % (mean_cn0[i])
        draw.text((x+500, y + 120 + i*20), str_cn0, text_color, font=font)


# обработка данных из файла
def sbas_handler(filename):

    pd.set_option('display.max_columns', None)
    pd.set_option('display.expand_frame_repr', False)

    if not os.access(filename, os.F_OK):
        print("Can't open", filename)
        # return []
        # return None
        return pd.DataFrame()
    if os.stat(filename).st_size == 0:
        print("Empty file", filename)
        # return []
        # return None
        return pd.DataFrame()

    print('read', filename)

    # считываем первую строку, чтобы определить PRN в файле
    data = pd.read_csv(filename, header=None, sep=';', nrows=1)

    cols = ['time', 'unix_time', 'ussi']
    dtype_dict = {1: np.uint32}
    prn_len = int(np.ceil(len(data.columns) - 3) / 3)
    for i in range(prn_len):
        prn_num.append(data.iloc[0, 3 + i * 3])
        cols.append('cn0' + str(i + 1))
        dtype_dict[4 + i * 3] = np.float32
    print('prn:', prn_num)

    # считываем только нужные столбцы, ограничиваем разрядность данных
    data = pd.read_csv(filename, header=None, sep=';', dtype=dtype_dict, parse_dates=[0], na_values=0,
                       usecols=[0,1,2,4,7,10])
    data.columns = cols

    # переводим unix_time в datetime по ШВ gps
    print('unix time to gps time')
    data['unix_time'] = pd.to_datetime(data['unix_time'], unit='s', origin='unix')
    data['unix_time'] = data['unix_time'] + pd.Timedelta(hours=5, seconds=-ls)
    
    # удаляем данные за другие даты
    print('remove wrong datetime')
    filedate = data.loc[0, 'time'].date()
    data = data[data['unix_time'].dt.date == filedate]

    # удаляем отрицательную и слишком большую задержку
    print('remove wrong delay')
    data['delay'] = (data['time'] - data['unix_time']).dt.seconds
    # print(data.head())
    # print(data.info())
    # print(data['delay'].describe())
    data = data[data['delay'] > 0]    # тут иногда получалась ошибка "Process finished with exit code -1073741819 (0xC0000005)"
    data = data[data['delay'] < 60]

    # удаляем дубликаты
    '''print('\nDuplicates:')
    # print(data[data.duplicated()])
    # print(data.head())'''

    # сводная таблица по prn
    sbas_table = data.pivot_table(index='unix_time', values=(['cn01', 'cn02', 'cn03']), aggfunc=('count', 'mean'))
    sbas_table.columns = [[prn_num[0], prn_num[0], prn_num[1], prn_num[1], prn_num[2], prn_num[2]],
                          ['count', 'cn0', 'count', 'cn0', 'count', 'cn0']]
    sbas_table['total'] = data.pivot_table(index='unix_time', values='ussi', aggfunc='count')['ussi']
    #print(sbas_table.info())
    #print(sbas_table)
    # удаляем, чтобы не забивать память
    del data
    return sbas_table


def plt_init():
    SMALL_SIZE = 8
    MEDIUM_SIZE = 10
    BIGGER_SIZE = 12
    plt.rc('font', size=SMALL_SIZE)
    plt.rc('axes', titlesize=BIGGER_SIZE)
    plt.rc('axes', labelsize=MEDIUM_SIZE)
    plt.rc('xtick', labelsize=SMALL_SIZE)
    plt.rc('ytick', labelsize=SMALL_SIZE)
    plt.rc('legend', fontsize=SMALL_SIZE)
    plt.cla()  # создаем чистый лист
    plt.clf()
    f = plt.gcf()
    f.set_size_inches(9, 13)


def create_report(savefile, date, sbas_table):
    plt_init()

    time = np.arange(0.0, 86399.0)
    len_prn = len(prn_num)
    num_ok = [None]*len_prn
    m_cn0 = [None]*len_prn
    total_count = len(sbas_table)                 # количество секунд, для которых есть данные с УССИ

    for i in range(len_prn):
        # print('plot for prn', i)
        plot_cn0(time, sbas_table[prn_num[i]]['cn0'], i)
        plot_count(time, sbas_table[prn_num[i]]['count'], sbas_table['total'], i)
        num_ok[i] = (sbas_table[prn_num[i]]['count']>0).sum()
        m_cn0[i] = sbas_table[prn_num[i]]['cn0'].mean()
    print('total count:', total_count)
    print('count:', num_ok)
    print('mean cn0:', np.round(m_cn0, 1))
    save_daystat(total_count, num_ok, m_cn0, date)

    plt.savefig(savefile, format='png', dpi=100)        # сохраняем полученные изображения в файл
    img = Image.open(savefile)
    draw = ImageDraw.Draw(img)
    text_ussi(draw, date, total_count, num_ok,  m_cn0)   # печать статистики
    img.save(savefile)


def set_xaxis(ax, x0):
    x1 = x0.date()
    x2 = x1 + dt.timedelta(days=1)
    ax.set_xlim(x1, x2)
    ax.xaxis_date()
    date_format = mdates.DateFormatter('%H')
    ax.xaxis.set_major_formatter(date_format)
    locator = mdates.HourLocator(interval=2)
    ax.xaxis.set_major_locator(locator)
    ax.grid()


# формирование графика сигнал/шум
def plot_cn0(t, y1, num):
    prn = u"PRN %d" % (prn_num[num])
    grid_size = (15, 2)
    ax = plt.subplot2grid(grid_size, (3+3*num, 1), rowspan=2, colspan=1)
    # plt.plot(y1, label=prn, color='blue')
    plt.plot(y1, label=prn, color='blue', linestyle='', marker='.', markersize=1)
    plt.ylim(20, 50)
    plt.title(prn)
    set_xaxis(ax, y1.index[0])


# формирование графика кол-во приемников
def plot_count(t, y1, y2, num):
    prn = u"PRN %d" % (prn_num[num])
    grid_size = (15, 2)
    ax = plt.subplot2grid(grid_size, (3+3*num, 0), rowspan=2, colspan=1)
    # plt.plot(y2, label='total', color='orange')
    # plt.plot(y1, label=prn, color='blue')
    plt.plot(y2, label='total', color='orange', linestyle='', marker='.', markersize=1)
    plt.plot(y1, label=prn, color='blue', linestyle='', marker='.', markersize=1)
    ylim = round(max(y2), -1) + 10
    plt.ylim(0, ylim)
    plt.title(prn)
    # plt.legend(loc='best')
    set_xaxis(ax, y1.index[0])


# запись в файл процента доставки
def save_daystat(total_count, count, cn0, date):
    year = date.year
    filename = os_path + 'ussi_' + str(year) + '.log'
    new_row = pd.DataFrame([[pd.to_datetime(date), total_count]], columns=['date', 'total_count'])
    for i in range(len(prn_num)):
        sprn = str(prn_num[i])
        new_row[sprn + '_count'] = count[i]
        new_row[sprn + '_cn0'] = cn0[i].round(1)
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


# имя выходного файла
def form_savefile(date, ext):
    if ext:
        name = './tmp_ussi.png'
    else:
        name = out_path + "sbas_" + date.strftime("%y%m%d") + ".png"
    return name


#
# скачать с ftp файл на нужную дату
#
def download_ifile(date, force):
    filename = os_path + "sbas_" + date.strftime("%y%m%d") + '.log'

    if not force:
        if os.path.exists(filename):
            if os.path.getsize(filename):
                print(filename, 'exist, do not reload')
                return filename

    ftp = FTP('172.20.0.55')
    # ftp = FTP('192.168.202.120')
    print(ftp)
    print(ftp.login('ftp_nsm', 'H6743N'))
    ftp.cwd(str(date.year))
    ftp.cwd(str(date.strftime("%m")))
    ifilename = str(date.strftime("%d")) + '.log'

    with open(filename, 'wb') as f:
        print('download file ' + ifilename + ' as ' + filename)
        try:
            ftp.retrbinary('RETR ' + ifilename, f.write)
        except:
            filename = ''
        f.close
    ftp.quit()
    return filename


# удалить файл
def delete_file(filename, opttest):
    if opttest or platform == "win32":
        return
    print("remove files")
    command = "rm " + filename
    print(command)
    os.system(command)


def main():
    parser = OptionParser()
    parser.add_option('-f', '--force', dest='force', action='store_true', default=False, help="force logfile reload")
    parser.add_option('-d', '--date', dest='date', action='store', default=0, help="specific date (example: 180329)")
    parser.add_option('-y', '--yesterday', dest='yesterday', action='store_true', default=False, help="quick yesterday date")
    parser.add_option('-l', '--log', dest='log', action='store_true', default=False, help="save stat to log")
    parser.add_option('-t', '--test', dest='test', action='store_true', default=False, help="test mode - don't remove file")
    parser.add_option('-e', '--external', dest='ext', action='store_true', default=False, help="external call")
    parser.add_option('-i', '--input', dest='input', action='store', default='', help="input file name")
    options, args = parser.parse_args()

    rep_date = dt.date.today()
    if options.yesterday:
        rep_date = dt.date.today() - dt.timedelta(days=1)
    elif options.date:
        rep_date = dt.datetime.strptime(options.date, '%y%m%d').date()

    print('\n__________________________')
    print('USSI report for %s' % rep_date)

    if options.input:
        ifilename = options.input
    else:
        ifilename = download_ifile(rep_date, options.force)
    if not ifilename:
        print('Error downloading file, exit programm')
        return

    ofilename = form_savefile(rep_date, options.ext)

    sbas_table = sbas_handler(ifilename)
    if sbas_table.empty:
    #if not sbas_table:
        print('\nNo sbas_table, exit program')
        return
    delete_file(ifilename, options.test)
    create_report(ofilename, rep_date, sbas_table)


if __name__ == "__main__":
    main()
