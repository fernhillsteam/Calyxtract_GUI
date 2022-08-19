# -*- coding: utf-8 -*-
"""
Created on Mon Dec  9 19:23:25 2019

@author: DELL bPC
"""

import serial.tools.list_ports
print(1)
ports=list(serial.tools.list_ports.comports())
print(ports)
for p in ports:
    print(p)