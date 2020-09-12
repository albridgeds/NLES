#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import datetime as dt
import time
import sys
from sys import platform
import paramiko
import socket


#
# подключение по ssh с обработкой ошибок
#
def ssh_connect(host, user, secret):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        ssh.connect(hostname=host, username=user, password=secret, port=22, timeout=5)
    except paramiko.AuthenticationException:
        print('ERR: connection to host %s failed (authentication error)' % host)
        return None
    except paramiko.SSHException as sshException:
        print('ERR: connection to host %s failed (%s)' % (host, sshException))
        return None
    except socket.error:
        print('ERR: connection to host %s failed (no route to host)' % host)
        return None
    return ssh


#
# скачать файл
#
def download_ssh(host, user, secret, remote_file, local_file, remove=False, force=False):

    if not force:
        if os.path.exists(local_file):
            if os.path.getsize(local_file):
                print(local_file, 'exist, do not reload')
                return

    print('download %s from %s to local storage' % (remote_file, host))
    ssh = ssh_connect(host, user, secret)
    if not ssh:
        return

    sftp = ssh.open_sftp()
    try:
        sftp.get(remote_file, local_file)
    except FileNotFoundError:
        print('file not found')
    else:
        if remove:
            sftp.remove(remote_file)
    sftp.close()
    ssh.close()


#
# последовательное выполнение команд
#
def execute_ssh(host, user, secret, commands=[]):
    ssh = ssh_connect(host, user, secret)
    if not ssh:
        return

    # перебираем команды
    for com in commands:
        print('send command <<' + com + '>> to ' + host)
        errdata = ''
        ssh_transp = ssh.get_transport()
        chan = ssh_transp.open_session()
        chan.setblocking(0)
        chan.exec_command(com)
        while True:
            # while chan.recv_ready():
            # outdata = str(chan.recv(100))
            # print(outdata)
            while chan.recv_stderr_ready():
                errdata += str(chan.recv_stderr(100))
            if chan.exit_status_ready():  # завершена команда
                break
            time.sleep(0.001)
        retcode = chan.recv_exit_status()
        # ssh_transp.close()
        print(errdata)
    ssh.close()


#
# время изменения файла
#
def get_mtime(host, user, secret, remote_file):
    ssh = ssh_connect(host, user, secret)
    if not ssh:
        return None
    sftp = ssh.open_sftp()
    try:
        utime = sftp.stat(remote_file).st_mtime
        mtime = dt.datetime.fromtimestamp(utime)
    except:
        mtime = None
    sftp.close()
    ssh.close()
    return mtime


def test():
    print('report_download test')
    res = get_mtime('192.168.31.13', 'user', 'user', './rotato.py')
    print(res)


if __name__ == "__main__":
    test()
