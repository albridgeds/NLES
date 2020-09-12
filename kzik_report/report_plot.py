#!/usr/bin/python3
# -*- coding: utf-8 -*-
import pandas as pd
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
mpl.use('Agg')


#
# инициализируем лист отчета
#
def init(size, grid=(15, 1)):
    # создаем чистый лист
    plt.cla()
    plt.clf()
    f = plt.gcf()
    f.set_size_inches(size)
    # определяем размер подписей на графиках
    plt.rc('font', size=8)
    plt.rc('axes', titlesize=12)
    plt.rc('axes', labelsize=10)
    plt.rc('xtick', labelsize=8)
    plt.rc('ytick', labelsize=8)
    plt.rc('legend', fontsize=8)
    global gl_grid
    gl_grid = grid


#
# универсальный график
#
def plot_universal(data, pos=(0, 0), span=(1, 1), title='', labels=[], ylim=None, how='points'):
    # colors = ['blue', 'orange', 'green']
    ax = plt.subplot2grid(gl_grid, pos, rowspan=span[0], colspan=span[1])
    t = data.index/3600

    plt.xlim(0, 24)
    ax.set_xticks(range(0, 25, 2))

    if ylim:
        plt.ylim(ylim)
    # убрать выбросы на нулях, чтобы не портили масштаб там, где он не задан жестко
    else:
        data = data.where(data != 0)

    i = 0
    for col in data:
        label = labels[i] if len(labels)>i else ''
        i += 1
        plt.plot(t, data[col], '.', markersize=1, label=label)      # точки акцентируют внимание на пропусках
        # plt.plot(t, data[col], label=label)                       # линии акцентируют внимание на выбросах
        # plt.plot(t, data[col], label=label, color=colors[i])

    plt.title(title)
    if len(labels):
        plt.legend(loc='best')
    plt.grid()


def plot_table(draw, x0, y0, table):





def save(ofile):
    print('save figure to', ofile)
    plt.savefig(ofile, format='png', dpi=100)  # сохраняем полученные изображения в файл


if __name__ == "__main__":
    init((3, 4))
