#ifndef V4L2_DEVICE_MONITOR_H
#define V4L2_DEVICE_MONITOR_H

#include <functional>
#include <thread>
#include <string>
#include <linux/videodev2.h>

typedef enum{
    MONITOR_VDEV = 0,
    MONITOR_VSUBDEV,
}Monitor_Type_E;

class CV4L2DeviceMonitor {
public:
    using TimingCallback = std::function<void(const v4l2_dv_timings&)>;
    /* for whole v4l2 pipeline devices like mipi-hdmi bt-hdmi */
    CV4L2DeviceMonitor(const std::string& videoNode, const std::string& subdevNode);
    /* for v4l2 device like HDMIrx that only have video node  */
    CV4L2DeviceMonitor(const std::string& videoNode);
    ~CV4L2DeviceMonitor();
    
    void run();
    void stop();
    void setCallback(TimingCallback v4l2Callback);
    bool getRuningStatus(){return m_bRunning;};
    const std::string& getmVideoNode() const {
        return m_videoNode;
    }

    const std::string& getSubdevNode() const {
        return m_subdevNode;
    }
private:
    void monitorLoop();
    void handleEvent(int nFd,v4l2_event stEv);
    void subscribeEvent(int fd);
    
    std::string m_videoNode;
    std::string m_subdevNode;
    int nEventFd;
    int nVideoFd;
    TimingCallback m_fnCallback;
    std::thread m_monitorThread;
    v4l2_dv_timings m_stCurTimings;
    Monitor_Type_E m_eMonitorTpye;
    bool m_bRunning;
};

#endif // V4L2_DEVICE_MONITOR_H
