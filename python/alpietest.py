import datetime
import os
import sys
import time
from math import sqrt

import bitstring
import matplotlib.pyplot as plt
import numpy as np
import scipy.io as sio

import functions

def DrawPic(filepath):
    fp = open(filepath, 'rb')
    s = bitstring.ConstBitStream(filename=filepath)
    a = s.read('bin')

    fp.seek(0)  # Go to beginning
    b = fp.read()

    Matrix = np.zeros((512, 1024), dtype=int)
    n = 0
    count = 0
    Triggercnt = np.array([])

    C = '110'
    EF = '1110'
    DL = '00'
    DS = '01'
    CT = '1011'
    CH = '1010'

    while n < len(b):
        if(int.from_bytes(b[n:n+2], byteorder='big') == 21923):
            count = count+1
            j = 3
            while j<(len(b)-n-2)/2:
                if(int.from_bytes(b[n+j*2:n+j*2+2], byteorder='big') == 60304):
                    break
                else:
                    j = j+1
            Triggercnt = np.append(Triggercnt, [int.from_bytes(
                b[n+2:n+4], byteorder='big')*65536+int.from_bytes(b[n+4:n+6], byteorder='big')])
            s = n*8+48
            while (s >= n*8+48 and s < (n+j*2)*8-7):
                if(a[s:s+4] == CH):
                    s = s+16
                elif(a[s:s+3] == C):
                    region = int(a[s+3:s+8], 2)
                    s = s+8
                elif(a[s:s+4] == CT):
                    if(a[s+8:s+12] == '0000'):
                        s = s+16
                    else:
                        s = s+8
                elif(a[s:s+2] == DL):
                    encoder = int(a[s+2:s+6], 2)
                    addr = int(a[s+6:s+16], 2)
                    Matrix[16*region+encoder,
                           addr] = Matrix[16*region+encoder, addr]+1
                    for i in range(1, min(8, 1024-addr)):
                        Matrix[16*region+encoder, addr+i] = Matrix[16 *
                                                                   region+encoder, addr+i]+int(a[s+24-i], 2)
                    s = s+24
                elif(a[s:s+2] == DS):
                    encoder = int(a[s+2:s+6], 2)
                    addr = int(a[s+6:s+16], 2)
                    Matrix[16*region+encoder,
                           addr] = Matrix[16*region+encoder, addr]+1
                    s = s+16
                elif(a[s:s+4] == EF):
                    s = s+16
                else:
                    s = s+8
            n = n+j+1
        else:
            n = n+1

    Matrix0 = np.zeros((1024, 512), dtype=int)

    for J in range(1, 513):
        for K in range(1, 257):
            Matrix0[J*2-2, 2*K-2] = Matrix[J-1, 4*K-4]
            Matrix0[J*2-2, 2*K-1] = Matrix[J-1, 4*K-1]
        for K in range(1, 257):
            Matrix0[J*2-1, 2*K-2] = Matrix[J-1, 4*K-3]
            Matrix0[J*2-1, 2*K-1] = Matrix[J-1, 4*K-2]

    Matrix0 = Matrix0.T

    plt.imshow(Matrix0)
    plt.colorbar()
    ind = filepath.rindex('\\')
    plt.title(filepath[ind+1:-4])
    print('hit map shown in figure')
    # plt.show()

def main():
    siTcp = functions.Functions()
    # siTcp.ResetDAQ()
    # print('DAQ Reseted! Wait 5 Seconds until ReConnection')
    # time.sleep(5)
    # siTcp = functions.Functions()
    siTcp.InitALPIDE()
    siTcp.alpideregtest()
    siTcp.ReadReg(0x1)
    siTcp.ReadReg(0x4)
    siTcp.ReadReg(0x5)
    siTcp.StartPLL()
    filepath = siTcp.DigitalPulse()
    time.sleep(1)
    filepath = siTcp.InternalTrigger()
    # filepath = siTcp.DigitalPulse()
    # filepath = siTcp.ExternalTrigger()
    # filepath = siTcp.AnaloguePulse(30)
    # DrawPic(filepath)
    siTcp.CloseSock()

if __name__ == "__main__":
    main()
