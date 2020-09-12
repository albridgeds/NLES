#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import datetime
import pandas as pd
from PIL import Image
from sys import platform
from optparse import OptionParser
from subprocess import call, Popen, PIPE
import paramiko


windows_test = True if platform == 'win32' else False
os_path = '' if windows_test else './'
out_path = '' if windows_test else './'

bzk_ip = ["192.168.31.2", "192.168.31.3", "192.168.31.12", "192.168.31.13"]
bzk_pass = ["user", "sdcm_nles#140", "sdcm_nles#125", "user"]


# имя выходного файла
def out_kzik_name(date, kziknum=0):
    name = "kzik"
    if kziknum:
        name = name + str(kziknum)
    name = name + '_' + date.strftime("%y%m%d") + ".png"
    return name


# имя входного файла
def tmp_kzik_name(kzik_num):
    name = 'tmp_kzik' + str(kzik_num) + '.png'
    return name


def tmp_bzk_name(kzik_num):
    name = 'tmp_bzk' + str(kzik_num) + '.png'
    return name


def tmp_rfdev_name(kzik_num):
    name = 'tmp_rfdev' + str(kzik_num) + '.png'
    return name


# соединение отчетов по двум КЗиК в одну картинку
def merge_kzik1(fn_bzk, fn_rfdev, fn_kzik):
    img1 = Image.open(os_path + fn_bzk)
    img2 = Image.open(os_path + fn_rfdev)
    img = Image.new('RGB', (1200, 1200))
    img.paste(img1, (-50, 0))
    img.paste(img2, (800, 0))
    img.save(os_path + fn_kzik, 'PNG')


def merge_kzik2(fn_bzk1, fn_bzk2, fn_rfdev1, fn_rfdev2, fn_kzik):
    img11 = Image.open(os_path + fn_bzk1)
    img12 = Image.open(os_path + fn_rfdev1)
    img21 = Image.open(os_path + fn_bzk2)
    img22 = Image.open(os_path + fn_rfdev2)
    img21 = img21.crop((50, 0, 1000, 1200))
    img = Image.new('RGB', (2400, 1200))
    img.paste(img11, (-50, 0))
    img.paste(img12, (800, 0))
    img.paste(img21, (1200, 0))
    img.paste(img22, (2000, 0))
    img.save(os_path + fn_kzik, 'PNG')


def str2date(str):
    year = 2000 + int(str[0:2])
    month = int(str[2:4])
    day = int(str[4:6])
    return datetime.date(year, month, day)


def mount_disk():
    if platform == "win32":
        return
    print('mount shared disk')
    command = 'mount.cifs //192.168.0.211/Share/kzik_report ' + out_path
    print(command)
    os.system(command)


def upload_file(host, user, secret, local_file, remote_file):
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
        ssh.close()


def main():

    parser = OptionParser()
    parser.add_option('-f', '--force', dest='force', action='store_true', default=False, help="force logfile reload")
    parser.add_option('-d', '--date', dest='date', action='store', default=0, help="specific date (example for 29 May 2018: 180529)")
    parser.add_option('-n', '--night', dest='night', action='store', default=0, help="end of period (example for 4 June 2018: 180604)")
    parser.add_option('-y', '--yesterday', dest='yesterday', action='store_true', default=False, help="quick yesterday date")
    parser.add_option('-l', '--log', dest='log', action='store_true', default=False, help="save stat to log")
    parser.add_option('-t', '--test', dest='test', action='store_true', default=False, help="test mode - don't remove file")
    parser.add_option('-u', '--ussi', dest='ussi', action='store_true', default=False, help="add ussi report")
    parser.add_option('-b', '--bzk', dest='bzk', action='store_true', default=False, help="add bzk report")
    parser.add_option('-r', '--rfdev', dest='rfdev', action='store_true', default=False, help="add RF equipment report")
    parser.add_option('-k', '--kziknum', dest='kziknum', action='store', default='0', help="specific kzik number (all by default)")
    options, args = parser.parse_args()

    start_date = datetime.date.today()
    if options.yesterday:
        start_date = datetime.date.today() - datetime.timedelta(1)
    elif options.date:
        start_date = str2date(options.date)
    # print('\n\nKzik report for %s' % (start_date))

    end_date = start_date
    if options.night:
        end_date = str2date(options.night)

    delta = end_date - start_date
    # print(start_date, end_date)
    if delta < datetime.timedelta(0) or delta > datetime.timedelta(30):
        print("ERR: wrong dates")
        return
    daterange = pd.date_range(start=start_date, end=end_date)

    kziknum = int(options.kziknum)

    # перебираем даты
    for rep_date in daterange:
        print('\n\nLaunch reports for %s' % rep_date)

        # отчет по двум БЗК
        if options.bzk:
            # print("\nStart BZK report")
            for j in range(2):
                if kziknum != 0 and j != kziknum - 1:
                    continue
                arg = ['./report_bzk.py', '-d' + rep_date.strftime("%y%m%d"), '-k' + str(j+1)]
                if options.test:
                    arg.append('-t')
                if options.force:
                    arg.append('-f')
                arg.append('-c')            # автоматическая коррекция поправки к частоте в конфигурационном файле tle
                call(arg)

        # отчет по ВЧ-части КЗиК
        if options.rfdev:
            # print("\nStart RF_DEV report")
            for j in range(2):
                if kziknum != 0 and j != kziknum - 1:
                    continue
                arg = ['./report_rfdev.py', '-e', '-d' + rep_date.strftime("%y%m%d"), '-k' + str(j+1)]
                if options.test:
                    arg.append('-t')
                if options.force:
                    arg.append('-f')
                call(arg)

        # объединение БЗК и ВЧ-части
        if options.bzk and options.rfdev:
            print('\nMerge KZiK reports')
            out_filename = out_kzik_name(rep_date, kziknum)
            if kziknum == 0:
                merge_kzik2(tmp_bzk_name(2), tmp_bzk_name(1), tmp_rfdev_name(2), tmp_rfdev_name(1), out_filename)
            else:
                merge_kzik1(tmp_bzk_name(kziknum), tmp_rfdev_name(kziknum), out_filename)
            upload_file(bzk_ip[2], 'user', bzk_pass[2], os_path+out_filename, '/home/user/buffer/reports/'+out_filename)

        if options.ussi:
            # print("\nStart USSI report")
            arg = ['./report_ussi.py', '-d' + rep_date.strftime("%y%m%d")]
            if options.test:
                arg.append('-t')
            if options.force:
                arg.append('-f')
            call(arg)


if __name__ == "__main__":
    main()
