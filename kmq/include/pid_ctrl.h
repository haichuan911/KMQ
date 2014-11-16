#ifndef _PIDCTRL_H_
#define _PIDCTRL_H_

#include <string>
#include <unistd.h>
#include "decr.h"


KMQ_DECLARATION_START

class pid_ctrl_t
{
public:
    // checkInstance����true��ԭ��
    enum EPidCtrlResult {
        PCR_PERMISSION_DENIED, // �����̵�Ȩ�޲���
        PCR_RUNNING_ALREADY,   // �����Ѿ�����
        TOTAL_PCR
    };

    /**
    * init
    * @param exe   execute name argv[0]
    * @param name  һ����������pid�ļ������ơ�
    *              ����У���pid�ļ���Ϊ"<exe>_<name>.pid";
    *              ���ΪNULL�����ļ���Ϊ"<exe>.pid"
    */
    pid_ctrl_t(const char* exe, const char* name = NULL, const char* rootdir = NULL);
    virtual ~pid_ctrl_t();

    /**
    * ����Ƿ�����ͬ��ʵ����������
    * @param reason  (out)������ֵΪtrueʱ������ԭ��; ����ֵΪfalseʱ��Ч
    * @return ������ڣ�����true; ����false
    */
    bool exists(EPidCtrlResult& reason);

    /**
    * ��ĳ���ź���ɱ�������еĽ���ʵ��
    * @param sig   ��Ҫ���ݸ������еĽ���ʵ�����ź���
    * @return true:�ɹ�; false:û��Ȩ�޴����ź���sig
    */
    bool kill(int sig);
    void savePid();
    void delPidFile();

private:
    std::string getPidFilename();
    bool readPid(pid_t& pid);

    std::string m_pidfilename;
    bool m_inited;
};

}

#endif
