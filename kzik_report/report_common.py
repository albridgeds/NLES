#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import datetime as dt
import pandas as pd


sat_name = [u'Луч-5В', u'Луч-5Б']
kzik_name = ['kzik1', 'kzik2']
kzik_ip = ['192.168.31.10', '192.168.31.20']

bzk_name = [['bzk11', 'bzk12'],
            ['bzk21', 'bzk22']]
bzk_ip = [['192.168.31.11', '192.168.31.12'],
          ['192.168.31.21', '192.168.31.22']]
bzk_pass = [['*****', '*****'],
            ['*****', '*****']]

test_data = 'aaa'


#
# чтение данных файла
#
def read_data(filename):
    data = list()
    if not os.access(filename, os.F_OK):
        print("Open " + filename + " - FAIL: no file")
        #return None
    elif os.stat(filename).st_size == 0:
        print("Open " + filename + " - FAIL: empty file")
        #return None
    else:
        print("Open " + filename + " - OK")
        data = pd.read_csv(filename, header=None)
        data = data.set_index(0)
    return data



def str2date(s_date):
    year = 2000 + int(s_date[0:2])
    month = int(s_date[2:4])
    day = int(s_date[4:6])
    return dt.date(year, month, day)


def test():
    print('report_common test')


if __name__ == "__main__":
    test()
