# Модуль управления генератором навигационного сигнала (siggen)
Модуль работает в состве ПО земной станции спутниковой связи
Управление генератором производится через СОМ-порт по бинарному протоколу


### Задачи
Инициализация работы генератора сигналов (подключение по СОМ-порту, установка начальных параметров сигнала, подача команд, запускающих формирование сигнала)  
Управление параметрами формируемого сигнала по командам, поступающим из внешних модулей
Слежение за параметрами работы генератора, обработка ошибок, запись сообщений о парамерах работы в лог-файл и выдача во внешний модуль для 


### Язык, ОС, основные используемые библиотеки
С++  
Linux  
sys.h, fcntl.h, termios.h


Проект завершен