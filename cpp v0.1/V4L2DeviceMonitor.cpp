
#include "V4L2DeviceMonitor.h"

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <cstring>

CV4L2DeviceMonitor::CV4L2DeviceMonitor(const std::string& videoNode, const std::string& subdevNode)
    : m_videoNode(videoNode), m_subdevNode(subdevNode),  m_bRunning(false) {

        m_eMonitorTpye = MONITOR_VSUBDEV;
    }

CV4L2DeviceMonitor::CV4L2DeviceMonitor(const std::string& videoNode)
    : m_videoNode(videoNode),m_bRunning(false) {
        m_eMonitorTpye = MONITOR_VDEV;
    }


CV4L2DeviceMonitor::~CV4L2DeviceMonitor() {
    stop();
}

void CV4L2DeviceMonitor::run() {
    if(m_eMonitorTpye == MONITOR_VDEV)
        std::cout<<"Video node monitor vidoenode:"<<m_videoNode.c_str()<<""<<" start running "<<std::endl;
    else  
        std::cout<<"Video node monitor subdevnode:"<<m_subdevNode.c_str()<<" videonode:"<<m_videoNode.c_str()<<""<<" start running "<<std::endl;
    m_bRunning = true;   
    m_monitorThread = std::thread(&CV4L2DeviceMonitor::monitorLoop, this);
}

void CV4L2DeviceMonitor::stop() {
    if (m_bRunning) {
        m_bRunning = false;
        if (m_monitorThread.joinable()) {
            m_monitorThread.join();
        }
    }
}
 
void CV4L2DeviceMonitor::monitorLoop() {
        
    if(m_eMonitorTpye == MONITOR_VDEV){
        nEventFd = open(m_videoNode.c_str(), O_RDWR);
        nVideoFd = nEventFd;
        if (nVideoFd < 0) {
            std::cerr << "vdev monitor Failed to open video or subdev node:" <<"videonode path:"
            <<m_videoNode<< std::endl;
        }
        return;
    }else{
        nVideoFd  = open(m_videoNode.c_str(), O_RDWR);
        nEventFd  = open(m_subdevNode.c_str(), O_RDWR);
        if (nVideoFd < 0 || nEventFd < 0) {
            std::cerr << "subvdev monitor Failed to open video or subdev node " <<"video node path:"
                <<m_videoNode<<" "<<"subvdev node path: "<<m_subdevNode<< std::endl;
            return;
        }
    }


    subscribeEvent(nEventFd);

    // Get initial timing
    v4l2_dv_timings initialTimings;
    if (ioctl(nEventFd, VIDIOC_G_DV_TIMINGS, &initialTimings) == 0) {
        m_fnCallback(initialTimings);
    } else {
        std::cerr << "Failed to get initial DV timings" << std::endl;
    }

    struct pollfd fds;
    fds.fd = nEventFd;
    fds.events = POLLPRI;
   
    while (m_bRunning) {
        int ret = poll(&fds, 1, 1000);

        if (ret > 0) {
            if (fds.revents & POLLPRI) {
                struct v4l2_event stEvent;
                if (ioctl(nEventFd, VIDIOC_DQEVENT, &stEvent) == 0){
                    if (stEvent.type == V4L2_EVENT_SOURCE_CHANGE) {
                        std::cout<<"video device source change"<<std::endl;
                        handleEvent(nEventFd,stEvent);
                    }else{
                        std::cout<<"unkown event type"<<std::endl;
                    }
                }else{
                     std::cout<<"event dq err"<<std::endl;
                }
            }else{
                std::cout<<"unsupported poll event do nothing"<<std::endl;
            }
           
        }
    }

    close(nEventFd);
    close(nVideoFd);
}

void CV4L2DeviceMonitor::handleEvent(int nFd,v4l2_event stEv) {
    
    if (stEv.type == V4L2_EVENT_SOURCE_CHANGE) {
        v4l2_dv_timings m_stCurTimings;
        if (ioctl(nFd, VIDIOC_G_DV_TIMINGS, &m_stCurTimings) == 0) {
            if(m_fnCallback)
                m_fnCallback(m_stCurTimings);
        } else {
            std::cerr << "Failed to get DV timings" << std::endl;
        }
    }
   
}

void CV4L2DeviceMonitor::subscribeEvent(int fd) {
    struct v4l2_event_subscription sub;
    std::memset(&sub, 0, sizeof(sub));
    sub.type = V4L2_EVENT_SOURCE_CHANGE;

    if (ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &sub) < 0) {
        std::cerr << "Failed to subscribe to event :"+ std::to_string(fd)<< std::endl;
    }
}

void CV4L2DeviceMonitor::setCallback(TimingCallback v4l2EventCallback) {
    if(m_fnCallback == nullptr)
        m_fnCallback = v4l2EventCallback;
    return;
}

