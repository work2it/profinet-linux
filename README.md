<h1>Демо-проект реализации profinet-device на языке программирования С в среде Eclipse под Ubuntu 22.04</h1>
Разработано и проверено на виртуальной машине под Oracle VirtualBox.<br>
Порядок установки VirtualBox, Ubuntu 22.04 и Eclipse IDE for c/c++ developers опускаем..<br><br>
Далее:
<li>Клонировать проект с github</li>
<li>Запустить Eclipse с правами супер-пользователя</li>
<li>Открыть, построить и запустить проект (при необходимости, изменить настройки в файле config.ini)</li>
<li>Установить GSDML-файл из папки gsd в Simatic Manager</li>
<li>В каталог будет добавлено устройство Virtual Profinet Device</li>
<img width="228" height="234" alt="image" src="https://github.com/user-attachments/assets/bcc5ceaa-2099-404f-9048-84ced8e6d14a" />
<li>Добавить устройство в конфигурацию PLC</li>
<img width="134" height="120" alt="image" src="https://github.com/user-attachments/assets/0c8144d6-c017-4914-a40e-2009c18aaf30" />
<li>Задать имя px-device, снять признак назначения адреса из PLC</li>
<img width="427" height="362" alt="image" src="https://github.com/user-attachments/assets/7d5de1f3-4327-4117-81ca-c7cae493d164" />
<li>Добавить в устройство по одному модулю ввода и вывода</li>
<img width="480" height="115" alt="image" src="https://github.com/user-attachments/assets/9bb2625a-588c-42f1-b82e-8f18c00f7a9a" />
<li>Загрузить конфигурацию в PLC и запустить</li>
<li>В VAT-таблицу добавить соответствующие адреса ввода и вывода</li>
<li>Виртуальное устройство копирует данные модуля вывода на модуль ввода</li>
<img width="350" height="90" alt="image" src="https://github.com/user-attachments/assets/c07ebc7d-a421-4962-a5ad-0e50f7e42481" />
<br>
Программа использует стандартный udp-порт siemens 34964 и исходящий порт 50000. Необходимо разрешить их в настройках фаервола или вообще отключить его.
<br><br>

<a target=blank href='https://yoomoney.ru/to/4100119354498534'>благодарность</a>

