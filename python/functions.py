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


class Functions(object):

    def __init__(self):
        try:
            self._rbcp = Rbcp("192.168.10.16", 4660)
            self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._sock.connect(("192.168.10.16",24))
        except Exception:
                LOGGER.error("Internal Device Connection Error %s %s ",
                        "192.168.10.16", 24)

    def CloseSock(self):
        self._sock.close()

    def ChipID(self,ID):
        self._rbcp.write(0x40000000,(ID & 0xff).to_bytes(1,byteorder='big'))

    def WriteReg(self,addr,data):
        self._rbcp.write(0x00000000+addr,(data & 0xff).to_bytes(1,byteorder='big'))
        self._rbcp.write(0x00010000+addr,((data & 0xff00)>>8).to_bytes(1,byteorder='big'))

    def ReadReg(self,addr):
        self._rbcp.write(0x50000000+addr*16,b'\x00')
        print(''.join('{:02x}'.format(x) for x in self._rbcp.read(0x60000000+addr*16,2)))

    def Broadcast(self,data):
        self._rbcp.write(0x10000000+data,b'\x00')

    def StartExtTrig(self):
        self._rbcp.write(0x60000000,b'\x00')
        
    def StopExtTrig(self):
        self._rbcp.write(0x70000000,b'\x00')

    def SetEventNum(self,num):
        self._rbcp.write(0x90000000+num,b'\x00')

    def SetTrigDelay(self,delay):
        self._rbcp.write(0x80000000,(delay & 0xff).to_bytes(1,byteorder='big'))

    def Pulse(self):
        self._rbcp.write(0x20000000,b'\x00')

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
        self.WriteReg(0x1,0x3D)
        self.Broadcast(0x63)


    def DigitalPulse(self):
        for region in range(0,32):
            if(region == 0):
                self.WriteReg(0x487,0xFFFF)
                self.WriteReg(0x500,0x3)
                self.WriteReg(0x4,0x0)
                self.WriteReg(0x5,0x28)                
                self.WriteReg(0x7,0x32)
                self.WriteReg(0x8,0x3E8)
            
            if(region == 22):
                self.WriteReg(0x488,0x0)                
                self.WriteReg(0xB408,0xFFFF)
            else:
                self.WriteReg(0x488,0x0)
                self.WriteReg(0x408 | (region << 11), 0xFFFF)
                self.WriteReg(0xB408,0x0)

            self.WriteReg(0x1,0x3D)
            self.Pulse()

            if(region == 0):
                stopevt = threading.Event()
                threadRead = readThread(self._rbcp,self._sock,stopevt,'DigitalPulse')
                threadRead.start()
            elif(region == 31):
                stopevt.set()
                filepath = threadRead.join()

        return filepath

    def ExternalTrigger(self):        
        self.WriteReg(0x487,0xFFFF)
        self.WriteReg(0x500,0x0)
        self.WriteReg(0x487,0xFFFF)
        self.WriteReg(0x500,0x1)
        self.WriteReg(0x4,0x10)
        self.WriteReg(0x5,0x4)     
        self.WriteReg(0x1,0x3D)
        self.Broadcast(0x63)

        self.SetTrigDelay(0)
        self.SetEventNum(10000)
        self.StartExtTrig()

        stopevt = threading.Event()
        threadRead = readThread(self._rbcp,self._sock,stopevt,'ExtTrig')
        threadRead.start()

        time.sleep(10)
        
        stopevt.set()
        filepath = threadRead.join()

        return filepath

    def AnaloguePulse(self,charge):
        for region in range(0,32):
            if(region == 0):
                self.WriteReg(0x487,0xFFFF)
                self.WriteReg(0x500,0x3)
                self.WriteReg(0x605,0xaa)
                self.WriteReg(0x606,0xaa-charge)
                self.WriteReg(0x4,0x20)
                self.WriteReg(0x5,0x190)                
                self.WriteReg(0x7,0x0)
                self.WriteReg(0x8,0x7D0)
            
            if(region == 22):
                self.WriteReg(0x488,0x0)                
                self.WriteReg(0xB408,0xFFFF)
            else:
                self.WriteReg(0x488,0x0)
                self.WriteReg(0x408 | (region << 11), 0xFFFF)
                self.WriteReg(0xB408,0x0)

            self.WriteReg(0x1,0x3D)
            self.Pulse()

            if(region == 0):
                stopevt = threading.Event()
                threadRead = None
                threadRead = readThread(self._rbcp,self._sock,stopevt,'AnaloguePulse')
                threadRead.start()
            elif(region == 31):
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
            readable,_,_ = select.select(rdlist, [], [], 0.01)
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
        print('open:',filepath)
        return {'fp': fp,
                'filepath': filepath}

    def run(self):
        self.ReadThreadFunc()

    def join(self):
        threading.Thread.join(self)
        return self._return