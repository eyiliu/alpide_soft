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
            while 1:
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
    plt.show()

import argparse

import argparse
import os
import sys
def main(argv=None):
    parser = argparse.ArgumentParser(description="ALPIDE test, SiTCP command")
    parser.add_argument("-v", "--verbosity", action="count", default=0,
                        help="increase output verbosity")
    pulse_g = parser.add_mutually_exclusive_group(required=True)
    pulse_g.add_argument("-d", "--DigitalPulse", action="store_true",
                         help="DigitalPulse")
    pulse_g.add_argument("-a", "--AnaloguePulse", type=int, default=-1,
                         metavar = "N",
                         help="AnaloguePulse")
    parser.add_argument("-e", "--ExternalTrigger", action="store_true",
                        help="ExternalTrigger")
    parser.add_argument("-s", "--sleep", type=int, default=1,
                        help="sleep seeconds")
    parser.add_argument("-i", "--ipaddress", type=str, default="192.168.10.16",
                        metavar = "ip",
                        help="IP address of SiTCP device")
    parser.add_argument("-t", "--tcp", type=int, default=24,
                        help="tcp port, data channel, sock")
    parser.add_argument("-u", "--udp", type=int, default=4660,
                        help="udp port, control channel, rbcp")
    parser.add_argument("--datafolder", type=str,
                        default=os.path.dirname(__file__) + "\\outputdata\\",
                        metavar = "file_path",)
    
    if argv is None:
        argv = sys.argv[1:]
    args = parser.parse_args(argv)

    siTcp = functions.Functions()
    siTcp.InitALPIDE()

    if args.DigitalPulse is True:
        filepath = siTcp.DigitalPulse()
    if args.AnaloguePulse > 0:
        # filepath = siTcp.AnaloguePulse(30)
        filepath = siTcp.AnaloguePulse(args.AnaloguePulse)        
    # siTcp.ReadReg(0x1)
    time.sleep(args.sleep)
    if args.ExternalTrigger is True:
        filepath = siTcp.ExternalTrigger()

    DrawPic(filepath)
    siTcp.CloseSock()

if __name__ == "__main__":
    main()
