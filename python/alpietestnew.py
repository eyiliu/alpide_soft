import datetime
import os
import sys
import time

import functionsnew

def main():
    siTcp = functionsnew.Functionsnew()
    siTcp.InitALPIDE()
    siTcp.SetFrameNumber(10)
    siTcp.SetInTrigGap(20)
    siTcp.StartWorking(1)
    siTcp.CloseSock()

if __name__ == "__main__":
    main()
