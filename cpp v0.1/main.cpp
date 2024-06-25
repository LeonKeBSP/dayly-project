#include "V4L2DeviceMonitor.h"
#include "V4L2DeviceScanner.h"
#include <iostream>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "include/rk-camera-module.h"
#include <vector>



int main(int argc,char **argv) {

    V4L2DeviceScanner* pVdevScannerInstance = V4L2DeviceScanner::getInstance();


    int nTotalDev = pVdevScannerInstance->scanAllHdmiDevices();
    printf("total %d HDMI devices in currunt systrm ",nTotalDev);
    for (auto& dev : pVdevScannerInstance->getHdmiRxDevicesVec()) {
        
        dev.pDevMonitor = new CV4L2DeviceMonitor(dev.strVideoNode);
        if(dev.pDevMonitor == nullptr)
            std::cout<<"worong new a monitor"<<std::endl;
        dev.pDevMonitor->setCallback([=](const v4l2_dv_timings& timings) {
        std::cout <<dev.pDevMonitor->getmVideoNode()<<" Timing changed: " << timings.bt.width << "x" << timings.bt.height << std::endl;    
        });
        dev.pDevMonitor->run();
        std::cout<<"hdmirx device video node:"<<dev.strVideoNode.c_str()<<" start monitor job"<<std::endl;
    }

    for (auto& dev :  pVdevScannerInstance->getHdmiCsiDevicesVec()) {
        dev.pDevMonitor = new CV4L2DeviceMonitor(dev.strVideoNode, dev.strSubdevNode);
        /* set timing print callback */
        dev.pDevMonitor->setCallback([=](const v4l2_dv_timings& timings) {
        std::cout <<dev.pDevMonitor->getSubdevNode()<<" "<<dev.pDevMonitor->getmVideoNode()<<" Timing changed: " << timings.bt.width << "x" << timings.bt.height << std::endl;
        });
        dev.pDevMonitor->run();
        std::cout<<"hdmicsi device video node:"<<dev.strVideoNode.c_str()<<"with subdev node:"<<dev.strSubdevNode.c_str()<<" start monitor job"<<std::endl;
    }
   
    std::this_thread::sleep_for(std::chrono::seconds(120));
    for (auto& dev : pVdevScannerInstance->getHdmiRxDevicesVec()) {
        dev.pDevMonitor->stop();
        delete dev.pDevMonitor;
    }
    for(auto& dev : pVdevScannerInstance->getHdmiCsiDevicesVec()){
        dev.pDevMonitor->stop();
        delete dev.pDevMonitor;
    }
    
    pVdevScannerInstance->deleteInstance();
    
    return 0;
}