#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import datetime
import time
import sys
from sys import platform
import paramiko


def tod2hms(tod):
    hour = int(tod/3600)
    min = int((tod-hour*3600)/60)
    sec = tod - hour*3600 - min*60
    return hour, min, sec


# чтение данных из файла 
def read_data_tle(namefile):
    data = read_data(namefile)
    n = len(data[0])
    tod = [0]*n
    for i in range(n):
        stime = data[0][i]
        hour = int(stime[11:13])
        min = int(stime[14:16])
        sec = int(stime[17:19])
        tod[i] = (hour*3600 + min*60 + sec + 3*3600) % 86400
    data[0] = tod
    data = data.sort(0)
    return data


def check_tle_time(kziknum):
    print('Check TLE file date')
    filename = download_tle()
    satname = ['LUCH 5V','LUCH 5B']
    f = open(filename, 'r')
    line = f.readline()
    while line:
        if line.find(satname[kziknum]) != -1:
            line = f.readline()
            year = int(line[18:20])
            yday = int(line[20:23])
            tod = float(line[24:30])/1000000
            break
        line = f.readline()
    [h, m, s] = tod2hms(int(86400*tod))
    rep_time = datetime.datetime(2000+year, 1, 1, h, m, s) + datetime.timedelta(days=yday-1, hours=3)
    print(rep_time)

    change_time = get_file_time('192.168.0.222', 'user', 'user', '~/tle/data/geo.txt')
    print(time.strftime("%Y-%m-%d %H:%M:%S", change_time))
    return rep_time, change_time


def get_file_time(host, user, secret, filepass):
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(hostname=host, username=user, password=secret, port=22)

    stdin, stdout, stderr = client.exec_command('ls --full-time ' + filepass)
    data = stdout.read().split()
    str_date = data[5] + ' ' + data[6][0:8]
    print(str_date)
    create_time = time.strptime(str_date, '%Y-%m-%d %H:%M:%S')

    client.close()
    return create_time



def main():
    tle_rtime, tle_ctime = check_tle_time(0)
    f = open('tle.log', 'a')
    stime1 = time.strftime("%Y-%m-%d %H:%M:%S")
    stime2 = tle_rtime
    stime3 = time.strftime("%Y-%m-%d %H:%M:%S", tle_ctime)
    line = '\n%s  %s  %s' % (stime1, stime2, stime3)
    f.write(line)


if __name__ == "__main__":
    main()
