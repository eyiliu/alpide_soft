import select
import socket
import struct
import threading
import time
import datetime
import os
from logging import INFO, StreamHandler, getLogger

import sitcpy
from sitcpy.rbcp import Rbcp, RbcpBusError, RbcpError, RbcpTimeout

LOGGER = getLogger(__name__)
HANDLER = StreamHandler()
HANDLER.setLevel(INFO)
LOGGER.setLevel(INFO)
LOGGER.addHandler(HANDLER)


class Functionsnew(object):

    def __init__(self):
        try:
            self._rbcp = Rbcp("192.168.10.16", 4660)
            self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._sock.connect(("192.168.10.16",24))
        except Exception:
                LOGGER.error("Internal Device Connection Error %s %s ",
                        "192.168.10.1*", 24)

    def CloseSock(self):
        self._sock.close()

    def ResetDAQ(self):
        self._rbcp.write(0x00000000,b'\xff')
        time.sleep(5)

    def ChipID(self,ID):
        self._rbcp.write(0x10000001,(ID & 0xff).to_bytes(1,byteorder='big'))

    def WriteReg(self,addr,data):
        self._rbcp.write(0x10000002,(addr & 0xffff).to_bytes(2,byteorder='big'))
        self._rbcp.write(0x10000004,(data & 0xffff).to_bytes(2,byteorder='big'))
        self._rbcp.write(0x00000000,b'\x9c')

    def ReadReg(self,addr):
        self._rbcp.write(0x10000002,(addr & 0xffff).to_bytes(2,byteorder='big'))
        self._rbcp.write(0x00000000,b'\x4e')
        print('Read Count:')
        print(''.join('{:02x}'.format(x) for x in self._rbcp.read(0x1000000d,1)))
        print('Read Data:')
        print(''.join('{:02x}'.format(x) for x in self._rbcp.read(0x1000000e,2)))

    def Broadcast(self,data):
        self._rbcp.write(0x10000006,(data & 0xff).to_bytes(1,byteorder='big'))
        self._rbcp.write(0x00000000,b'\x50')    

    def SetFrameDuration(self,length):
        self._rbcp.write(0x10000007,(length & 0xffff).to_bytes(2,byteorder='big'))
        self.GetFrameDuration()

    def GetFrameDuration(self): 
        print('Current FrameDuration:')
        print(''.join('{:02x}'.format(x) for x in self._rbcp.read(0x10000007,2)))

    def SetFramePhase(self,phase):
        self._rbcp.write(0x10000009,(phase & 0xffff).to_bytes(2,byteorder='big'))
        self.GetFramePhase()

    def GetFramePhase(self): 
        print('Current FramePhase:')
        print(''.join('{:02x}'.format(x) for x in self._rbcp.read(0x10000009,2)))

    def SetInTrigGap(self,gap):
        self._rbcp.write(0x1000000b,(gap & 0xffff).to_bytes(2,byteorder='big'))
        self.GetInTrigGap()

    def GetInTrigGap(self): 
        print('Current InTrigGap:')
        print(''.join('{:02x}'.format(x) for x in self._rbcp.read(0x1000000b,2)))

    def SetFPGAMode(self,mode):
        self._rbcp.write(0x10000010,(mode & 0xff).to_bytes(1,byteorder='big'))

    def SetFrameNumber(self,number):
        self._rbcp.write(0x10000011,(number & 0xff).to_bytes(1,byteorder='big'))
        self.GetFrameNumber()

    def GetFrameNumber(self): 
        print('Current FrameNumber:')
        print(''.join('{:02x}'.format(x) for x in self._rbcp.read(0x10000011,1)))

    def InitALPIDE(self):
        self.ChipID(0x10)
        self.Broadcast(0xD2)
        self.WriteReg(0x10,0x70)
        self.WriteReg(0x4,0x10)
        self.WriteReg(0x5,0x28)
        self.WriteReg(0x601,0x75)
        self.WriteReg(0x602,0x93)
        self.WriteReg(0x603,0x56)
        self.WriteReg(0x604,0x32)
        self.WriteReg(0x605,0xFF)
        self.WriteReg(0x606,0x0)
        self.WriteReg(0x607,0x39)
        self.WriteReg(0x608,0x0)
        self.WriteReg(0x609,0x0)
        self.WriteReg(0x60A,0x0)
        self.WriteReg(0x60B,0x32)
        self.WriteReg(0x60C,0x40)
        self.WriteReg(0x60D,0x40)
        self.WriteReg(0x60E,0x32)
        self.WriteReg(0x701,0x400)
        self.WriteReg(0x487,0xFFFF)
        self.WriteReg(0x500,0x0)
        self.WriteReg(0x500,0x1)
        self.WriteReg(0x1,0x3C)
        self.Broadcast(0x63)
        self.StartPLL()

    def StartPLL(self):
        self.WriteReg(0x14,0x008d)
        self.WriteReg(0x15,0x0088)
        self.WriteReg(0x14,0x0085)
        self.WriteReg(0x14,0x0185)
        self.WriteReg(0x14,0x0085)

    def StartWorking(self,trigmode=0):   #trigmode=0 external trigger;trigmode=1 internal trigger
        self.WriteReg(0x487,0xFFFF)
        self.WriteReg(0x500,0x0)
        self.WriteReg(0x487,0xFFFF)
        self.WriteReg(0x500,0x1)
        self.WriteReg(0x4,0x10)
        self.WriteReg(0x5,4)   #strobe duration / 25 ns     
        self.WriteReg(0x1,0x3D)
        self.Broadcast(0x63)

        self.SetFrameDuration(200)
        self.SetFPGAMode(0x1+trigmode*2)
        print('start frame')

        time.sleep(1)

        stopevt = threading.Event()
        threadRead = None
        threadRead = readThread(self._rbcp,self._sock,stopevt,'Frame')
        
        print('start acquire')
        threadRead.start()

        time.sleep(2)
        self.SetFPGAMode(trigmode*2)

        time.sleep(1)

        stopevt.set()
        filepath = threadRead.join()

        return filepath

class readThread(threading.Thread):
    def __init__(self, rbcp, sock, stopevt=None, filetype='DigitalPulse', iTHR=0x32, charge=10):
        threading.Thread.__init__(self)
        self._stopevt = stopevt
        self._rbcp = rbcp
        self._sock = sock
        self._filetype = filetype
        self._return = None
        self._iTHR = iTHR
        self._charge = charge

    def ReadThreadFunc(self):
        max_buf = 8*1024*1024
        if(not self._filetype == 'Init'):
            fileoutput = self.CreateFile()

        while(not self._stopevt.isSet()):
            rdlist = [self._sock]
            readable,_,_ = select.select(rdlist, [], [], 0.01)
            if self._sock in readable:
                recvbuf = self._sock.recv(max_buf)
                fileoutput['fp'].write(recvbuf)
            else:
                recvbuf = []
                
        while(not len(recvbuf) == 0):
            rdlist = [self._sock]
            readable,_,_ = select.select(rdlist, [], [], 0.05)
            if self._sock in readable:
                recvbuf = self._sock.recv(max_buf)
                fileoutput['fp'].write(recvbuf)
            else:
                recvbuf = []
        
        if(not self._filetype == 'Init'):
            fileoutput['fp'].close()
            self._return = fileoutput['filepath']

        return True

    def CreateFile(self):
        directory = os.path.dirname(__file__) + "\\outputdata\\"
        if not os.path.exists(directory):
            os.makedirs(directory)
        curDateTime = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
        if(self._filetype == 'AnaloguePulse'):
            charge = self._charge * 10
            filename = self._filetype + '_iTHR0x%x' % self._iTHR + \
                '_charge%d_' % charge + curDateTime + ".dat"
        else:
            filename = self._filetype + '_' + curDateTime + ".dat"
        filepath = directory + filename
        fp = open(filepath, 'ab+')
        print('file:',filepath)
        return {'fp': fp,
                'filepath': filepath}

    def run(self):
        self.ReadThreadFunc()

    def join(self):
        threading.Thread.join(self)
        return self._return