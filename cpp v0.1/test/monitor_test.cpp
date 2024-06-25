#include "V4L2DeviceMonitor.h"
#include <iostream>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "include/rk-camera-module.h"
#include <vector>

#define RKMODULE_GET_MODULE_INFO	\
	_IOR('V', BASE_VIDIOC_PRIVATE + 0, struct rkmodule_inf)
#define RKMODULE_GET_HDMI_MODE       \
	_IOR('V', BASE_VIDIOC_PRIVATE + 34, __u32)



typedef enum{
    HDMI_RX = 0,
    HDMI_CSI,
    HDMI_BT1120
}Hdmi_Type_E;

const int kMaxDevicePathLen = 256;
const char* kDevicePath = "/dev/";
constexpr char kPrefix[] = "video";
constexpr int kPrefixLen = sizeof(kPrefix) - 1;
constexpr char kCsiPrefix[] = "v4l-subdev";
constexpr int kCsiPrefixLen = sizeof(kCsiPrefix) - 1;
//constexpr int kDevicePrefixLen = sizeof(kDevicePath) + kPrefixLen + 1;
constexpr char kHdmiNodeName[] = "rk_hdmirx";
constexpr char kCsiPreSubDevModule[] = "HDMI-MIPI";
constexpr int kCsiPreSubDevModuleLen = sizeof(kCsiPreSubDevModule) - 1;
constexpr char kCsiPreBusInfo[] = "platform:rkcif-mipi-lvds";

static void v4l2_err(const char* function, int line, const char* message, const char* arg1 = "", const char* arg2 = "") {
    std::cerr << function << ":" << line << " " << message << " " << arg1 << " " << arg2 << std::endl;
}


typedef struct {
   /*  hdmi采集类型 */
    Hdmi_Type_E nHdmiType;
    /* hdmi id */
    int nId;
    /* v4l2设备节点 */
    std::string strVideoNode;
     /* v4l2子设备设备节点 */
    std::string strSubdevNode;
    /* 事件监视器 */
    CV4L2DeviceMonitor *pDevMonitor;
}Hdmi_Dev_S;





typedef struct {
    int nHdmiRxNum;
    int nHdmiCsiNum;
    int nHdmiBtNum;
    std::vector<Hdmi_Dev_S>* pHdmiRxDevicesVec;
    std::vector<Hdmi_Dev_S>* pHdmiCsiDevicesVec;
    std::vector<Hdmi_Dev_S>* pHdmiBtDevicesVec;
}HdmiIn_DevicesCtx_S;

CV4L2DeviceMonitor * pMonitorInstance;


int findAllHdmiRxDevices(std::vector<Hdmi_Dev_S>* pHdmiRxDevicesVec){
    int nRet;
    int nHdmiRXNum;
    int nVideoFd;
    std::string strCsiNum = "";
    struct dirent* de;
    DIR* devdir = opendir(kDevicePath);
    if(devdir == 0) {
        printf("%s: cannot open %s! Exiting threadloop\n", __FUNCTION__, kDevicePath);
        return -1;
    }
    while ((de = readdir(devdir)) != 0){
        char strV4l2DevicePath[kMaxDevicePathLen];
	    char strV4l2DeviceDriver[16];
	    char strGadgetVideo[100] = {0};
       /* 先判断是不是HDMIRX */
       /* 找到/dev/videoX */
       if(!strncmp(kPrefix, de->d_name, kPrefixLen)){
           //排除uvc视频节点
            sprintf(strGadgetVideo, "/sys/class/video4linux/%s/function_name", de->d_name);
            if (access(strGadgetVideo, F_OK) == 0) {
                printf("/dev/%s is uvc gadget device, don't open it!\n", de->d_name);
                continue;
            }
            snprintf(strV4l2DevicePath, kMaxDevicePathLen,"%s%s", kDevicePath, de->d_name);
            printf("ready to open %s\n",strV4l2DevicePath);
            /* 打开video节点 */
            nVideoFd = open(strV4l2DevicePath, O_RDWR);
            if(nVideoFd < 0){
                printf("[%s %d] fail to open %s\n", __FUNCTION__, __LINE__, strV4l2DevicePath);
                continue;
            } else {
                struct v4l2_capability cap;
                nRet = ioctl(nVideoFd, VIDIOC_QUERYCAP, &cap);
			    if (nRet < 0) {
			    	printf("VIDIOC_QUERYCAP Failed, error: %s\n", strerror(errno));
			    	close(nVideoFd);
                    nVideoFd = -1;
                    continue;
                }
                snprintf(strV4l2DeviceDriver, 16,"%s",cap.driver);
                /* 找到HDMI Video名字 */
                if(!strncmp(kHdmiNodeName, strV4l2DeviceDriver, sizeof(kHdmiNodeName)-1)){
                    printf("find hdmirx device\n");
                    printf( "VIDIOC_QUERYCAP driver=%s,%s\n", cap.driver,strV4l2DeviceDriver);
		            printf("VIDIOC_QUERYCAP card=%s\n", cap.card);
		            printf("VIDIOC_QUERYCAP version=%d\n", cap.version);
		            printf("VIDIOC_QUERYCAP capabilities=0x%08x,0x%08x\n", cap.capabilities,V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
		            printf("VIDIOC_QUERYCAP device_caps=0x%08x\n", cap.device_caps);
                    Hdmi_Dev_S newHdmiRxDevice;
                    newHdmiRxDevice.nHdmiType = HDMI_RX;
                    newHdmiRxDevice.nId = nHdmiRXNum;
                    newHdmiRxDevice.strVideoNode = strV4l2DevicePath;
                    /* 添加到vector */
                    pHdmiRxDevicesVec->push_back(newHdmiRxDevice); 
                    nHdmiRXNum++;
                }

            }
       }

    }
    printf("find total %d hdmirx devices\n",nHdmiRXNum);
    return nHdmiRXNum;
}

int findCsiVideoDevice(int nCsiNum){
    int nRet;
    int nVideoFd = -1;
    std::string strMinVideoPath = "zzzzzzzz";
    std::string strCsiNum = "";
    struct dirent* de;
    const std::string strVdevPrefix = "/dev/video";
    DIR* devdir = opendir(kDevicePath);
    if (devdir == nullptr) {
        v4l2_err(__FUNCTION__, __LINE__, "cannot open", kDevicePath);
        return -1;
    }
    while ((de = readdir(devdir)) != nullptr) {
        char strV4l2DevicePath[kMaxDevicePathLen];
	    char strGadgetVideo[100] = {0};
        if (strncmp(kPrefix, de->d_name, kPrefixLen) == 0){ 
            sprintf(strGadgetVideo, "/sys/class/video4linux/%s/function_name", de->d_name);
            if (access(strGadgetVideo, F_OK) == 0) {
                printf("/dev/%s is uvc gadget device, don't open it!", de->d_name);
                continue;
            }
            snprintf(strV4l2DevicePath, kMaxDevicePathLen,"%s%s", kDevicePath, de->d_name);
            printf("ready to open %s\n",strV4l2DevicePath);
            /* 打开video节点 */
            nVideoFd = open(strV4l2DevicePath, O_RDWR);
            if(nVideoFd < 0){
                printf("[%s %d] fail to open %s\n", __FUNCTION__, __LINE__, strV4l2DevicePath);
                continue;
            } else {
                struct v4l2_capability cap;
                memset(&cap, 0, sizeof(struct v4l2_capability));
                nRet = ioctl(nVideoFd, VIDIOC_QUERYCAP, &cap);
                if (nRet < 0) {
                    v4l2_err(__FUNCTION__, __LINE__, "VIDIOC_QUERYCAP Failed", strV4l2DevicePath,strerror(errno));
                    close(nVideoFd);
                    continue;
                } else {
                    printf("VIDIOC_QUERYCAP %s cap.bus_info=%s\n", strV4l2DevicePath, cap.bus_info);
                }
                char strStandardBusInfo[kMaxDevicePathLen];
                char strCurBusInfo[kMaxDevicePathLen];
                if(nCsiNum>0)
                    snprintf(strStandardBusInfo, kMaxDevicePathLen,"%s%d", kCsiPreBusInfo, nCsiNum);
                else
                    snprintf(strStandardBusInfo, kMaxDevicePathLen,"%s", kCsiPreBusInfo);
                printf("standard csi bus info:%s\n", cap.bus_info);
                snprintf(strCurBusInfo, 32,"%s",cap.bus_info);
                if (strcmp(strStandardBusInfo, strCurBusInfo) == 0) {
                    printf("find video devices equals current csi bus type:%s\n",strV4l2DevicePath);
                    if (strMinVideoPath.compare(strV4l2DevicePath) > 0) {
                        strMinVideoPath = strV4l2DevicePath;
                        if (nVideoFd > -1) {
                            close(nVideoFd);
                        }
                          
                    } else {
                        close(nVideoFd);
                    }
                } else {
                    close(nVideoFd);
                }
            }
        }

    }
    if(strMinVideoPath.compare("zzzzzzzz") == 0){
        printf("no csi device video device find\n");
        return -1;
    }
    else {
        printf("find csi device video device path: %s\n",strMinVideoPath.c_str());
        std::string strIndex = strMinVideoPath.substr(strVdevPrefix.size());
        int nVdevIndex = std::stoi(strIndex);
        printf("get csi device %d\n",nVdevIndex);
        return nVdevIndex;
    }
}
int findHdmiCsiDevices(std::vector<Hdmi_Dev_S>* pHdmiCsiDevices){
    int nRet;
    int nHdmiCsiNum;
    int nSubVdevFd;
    std::string strCsiNum = "";
    struct dirent* de;
    DIR* devdir = opendir(kDevicePath);
    if(devdir == 0) {
        printf("%s: cannot open %s! Exiting threadloop\n", __FUNCTION__, kDevicePath);
        return -1;
    }
    while ((de = readdir(devdir)) != 0){
        if (strncmp(kCsiPrefix, de->d_name, kCsiPrefixLen) == 0){
            std::string strV4l2SubDevPath = std::string(kDevicePath) + de->d_name;
            printf("ready to open %s\n",strV4l2SubDevPath.c_str());
            nSubVdevFd =  open(strV4l2SubDevPath.c_str(), O_RDWR);
            if (nSubVdevFd < 0) {
                printf("[%s %d] fail to open %s\n", __FUNCTION__, __LINE__, strV4l2SubDevPath.c_str());
                continue;
            } else {
                printf("[%s %d] fail to open %s\n", __FUNCTION__, __LINE__, strV4l2SubDevPath.c_str());
                uint32_t nIshdmi = 0;
                nRet = ioctl(nSubVdevFd, RKMODULE_GET_HDMI_MODE, &nIshdmi);
                if (nRet < 0 || !nIshdmi) {
                    v4l2_err(__FUNCTION__, __LINE__, "RKMODULE_GET_HDMI_MODE Failed or Is not a bridge hdmi subdev", strV4l2SubDevPath.c_str(), strerror(errno));
                    close(nSubVdevFd);
                    continue;
                }
                struct rkmodule_inf stModuleInfo;
                memset(&stModuleInfo, 0, sizeof(struct rkmodule_inf));
                nRet = ioctl(nSubVdevFd, RKMODULE_GET_MODULE_INFO, &stModuleInfo);
                if (nRet < 0) {
                    close(nSubVdevFd);
                    continue;
                }
                v4l2_err(__FUNCTION__, __LINE__, "sensor name:", stModuleInfo.base.sensor);
                v4l2_err(__FUNCTION__, __LINE__, "module name:", stModuleInfo.base.module);
                if (strstr(stModuleInfo.base.module, kCsiPreSubDevModule)) {
                    strCsiNum = stModuleInfo.base.module[kCsiPreSubDevModuleLen];
                    v4l2_err(__FUNCTION__, __LINE__, "csiNum=", strCsiNum.c_str());
                    const std::string strVdevPrefix = "/dev/video";
                    
                    int strVdevIndex = findCsiVideoDevice(std::stoi(strCsiNum));
                    if(strVdevIndex < 0){
                        v4l2_err(__FUNCTION__, __LINE__, "no match csi video device");
                        continue;
                    }
                    /* 拼接字符串 */
                    std::string strVideoPath = strVdevPrefix+std::to_string(strVdevIndex);
                    Hdmi_Dev_S newHdmiCsiDevice;
                    newHdmiCsiDevice.nHdmiType = HDMI_CSI;
                    newHdmiCsiDevice.nId = nHdmiCsiNum;
                    newHdmiCsiDevice.strVideoNode = strVideoPath;
                    newHdmiCsiDevice.strSubdevNode = strV4l2SubDevPath;
                    /* 添加到vector */
                    pHdmiCsiDevices->push_back(newHdmiCsiDevice); 
                    nHdmiCsiNum++;
                } else {
                    close(nSubVdevFd);
                    continue;
                }
            }
        }
    }
    return nHdmiCsiNum;
}


int main(int argc,char **argv) {

    HdmiIn_DevicesCtx_S HdmiInCtx;

    HdmiInCtx.pHdmiRxDevicesVec = new std::vector<Hdmi_Dev_S>();
    HdmiInCtx.pHdmiCsiDevicesVec = new std::vector<Hdmi_Dev_S>();
    HdmiInCtx.pHdmiBtDevicesVec = new std::vector<Hdmi_Dev_S>();
    findAllHdmiRxDevices(HdmiInCtx.pHdmiRxDevicesVec);
    findHdmiCsiDevices(HdmiInCtx.pHdmiCsiDevicesVec);

    for (auto& dev : *HdmiInCtx.pHdmiRxDevicesVec) {
        
        dev.pDevMonitor = new CV4L2DeviceMonitor(dev.strVideoNode);
        if(dev.pDevMonitor == nullptr)
            std::cout<<"worong new a monitor"<<std::endl;
        dev.pDevMonitor->setCallback([=](const v4l2_dv_timings& timings) {
        std::cout <<dev.pDevMonitor->getmVideoNode()<<" Timing changed: " << timings.bt.width << "x" << timings.bt.height << std::endl;    
        });
        dev.pDevMonitor->run();
        std::cout<<"hdmirx device video node:"<<dev.strVideoNode.c_str()<<" start monitor job"<<std::endl;
    }

    for (auto& dev : *HdmiInCtx.pHdmiCsiDevicesVec) {
        dev.pDevMonitor = new CV4L2DeviceMonitor(dev.strVideoNode, dev.strSubdevNode);
        /* set timing print callback */
        dev.pDevMonitor->setCallback([=](const v4l2_dv_timings& timings) {
        std::cout <<dev.pDevMonitor->getSubdevNode()<<" "<<dev.pDevMonitor->getmVideoNode()<<" Timing changed: " << timings.bt.width << "x" << timings.bt.height << std::endl;
        });
        dev.pDevMonitor->run();
        std::cout<<"hdmicsi device video node:"<<dev.strVideoNode.c_str()<<"with subdev node:"<<dev.strSubdevNode.c_str()<<" start monitor job"<<std::endl;
    }
   
    std::this_thread::sleep_for(std::chrono::seconds(120));
    for (auto& dev : *HdmiInCtx.pHdmiRxDevicesVec) {
        dev.pDevMonitor->stop();
        delete dev.pDevMonitor;
    }
    for(auto& dev : *HdmiInCtx.pHdmiCsiDevicesVec){
        dev.pDevMonitor->stop();
        delete dev.pDevMonitor;
    }
    
    delete [] HdmiInCtx.pHdmiRxDevicesVec;
    delete [] HdmiInCtx.pHdmiCsiDevicesVec;
    delete [] HdmiInCtx.pHdmiBtDevicesVec;
    
    return 0;
}