#ifndef V4L2_DEVICE_SCANNER_H
#define V4L2_DEVICE_SCANNER_H


#include <vector>
#include <string>
#include <mutex>
#include "V4L2DeviceMonitor.h"

typedef enum{
    HDMI_RX = 0,
    HDMI_CSI,
    HDMI_BT1120
}Hdmi_Type_E;

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



class V4L2DeviceScanner {
private:
    static V4L2DeviceScanner* m_pScannerInstance;
    static std::mutex m_Mutex;
public:
    static V4L2DeviceScanner* getInstance();
    static void deleteInstance();

      /*********************
    * @brief  扫描所有Hdmi设备
    * @param  void
    * @return int
    * @author     lk
    * @date       2024/06/20
    ********************/
    int scanAllHdmiDevices();


    /*********************
    * @brief  获取HdMiRx 设备容器
    * @param  void
    * @return vector<Hdmi_Dev_S>&
    * @author     lk
    * @date       2024/06/20
    ********************/
    std::vector<Hdmi_Dev_S>& getHdmiRxDevicesVec() const;
     /*********************
    * @brief  获取HdMiCsi 设备容器
    * @param  void
    * @return vector<Hdmi_Dev_S>&
    * @author     lk
    * @date       2024/06/20
    ********************/
    std::vector<Hdmi_Dev_S>& getHdmiCsiDevicesVec() const;
     /*********************
    * @brief  获取HdMi-Bt 设备容器
    * @param  void
    * @return vector<Hdmi_Dev_S>&
    * @author     lk
    * @date       2024/06/20
    ********************/
    std::vector<Hdmi_Dev_S>& getHdmiBtDevicesVec() const;

    /*********************
    * @brief  获取HdMiRx数量
    * @param  void
    * @return int
    * @author     lk
    * @date       2024/06/20
    ********************/
    int getHdmiRxNum() const;
    /*********************
    * @brief  获取HdMiCsi数量
    * @param  void
    * @return int
    * @author     lk
    * @date       2024/06/20
    ********************/
    int getHdmiCsiNum() const;
     /*********************
    * @brief  获取HDMI-bT数量
    * @param  void
    * @return int
    * @author     lk
    * @date       2024/06/20
    ********************/
    int getHdmiBtNum() const;

     /*********************
    * @brief  扫描HDMIRX设备
    * @param  void
    * @return int
    * @author     lk
    * @date       2024/06/20
    ********************/
    int scanHdmiRxDevices();
     /*********************
    * @brief  扫描HDMICSI设备
    * @param  void
    * @return int
    * @author     lk
    * @date       2024/06/20
    ********************/
    int scanHdmiCsiDevices();



private:
    /* 禁止外部构造 */
    V4L2DeviceScanner();
    /* 禁止外部析构 */
    ~V4L2DeviceScanner();
    /* 禁用复制构造函数 */
    V4L2DeviceScanner(const V4L2DeviceScanner&) = delete;
    /* 禁止外部赋值 */
    V4L2DeviceScanner& operator=(const V4L2DeviceScanner&) = delete;
    

    std::vector<Hdmi_Dev_S> *m_pHdmiRxDevicesVec;
    std::vector<Hdmi_Dev_S> *m_pHdmiCsiDevicesVec;
    std::vector<Hdmi_Dev_S> *m_pHdmiBtDevicesVec;

    int m_nHdmiRxNum = 0;
    int m_nHdmiCsiNum = 0;
    int m_nHdmiBtNum = 0;


};
#endif 
