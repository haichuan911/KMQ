/**
 * File: pid_ctrl_t.cpp
 * User:
 * ͨ��PID�ļ����ƽ��̵��������˳�
 */
#include "pid_ctrl.h"
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
KMQ_DECLARATION_START


/**
 * ��ʼ��
 * @param exe   �����������Դ�argv[0]
 * @param name  һ����������pid�ļ������ơ�
 *              ����У���pid�ļ���Ϊ"<exe>_<name>.pid";
 *              ���ΪNULL�����ļ���Ϊ"<exe>.pid"
 */
pid_ctrl_t::pid_ctrl_t(const char* exe, const char* name, const char* basedir) : m_inited(false)
{
    if ( basedir != NULL) {
        m_pidfilename = basedir;
    } else {
        m_pidfilename = "";
    }
    if (exe == NULL)
        return;
    else
        m_pidfilename += exe;

    if (name != NULL)
        m_pidfilename += string("_") + name + ".pid";
    m_inited = true;
}

pid_ctrl_t::~pid_ctrl_t()
{
}

// �ϳ�pid�ļ���
string
pid_ctrl_t::getPidFilename()
{
    if (!m_inited) return "";

    return m_pidfilename;
}

/**
 * ��ȡpid��ֵ
 */
bool
pid_ctrl_t::readPid(pid_t& pid)
{
    string fpid = getPidFilename();
    if (fpid == "") return false;
    FILE *fp = fopen(fpid.c_str(), "r");
    if (fp == NULL)
        return false;

    char pbuf[100];
    if(fgets(pbuf,100,fp) == NULL) {
        fclose(fp);
        return false;
    }

    fclose(fp);
    pid = atoi(pbuf);

    return true;
}

/**
 * ����Ƿ�����ͬ��ʵ����������
 * @param reason  (out)������ֵΪtrueʱ������ԭ��; ����ֵΪfalseʱ��Ч
 * @return ������ڣ�����true; ����false
 */
bool
pid_ctrl_t::exists(EPidCtrlResult& reason)
{
    if (!m_inited) return false;

    // ��pid
    pid_t pid;
    if (!readPid(pid)) return false;

    // ���pid��Ӧ�Ľ����Ƿ����
    if (::kill(pid, 0) == -1) {
        switch(errno) {
        case EINVAL:
        case ESRCH:
            // no such process, that's ok
            break;
        case EPERM:
            reason = PCR_PERMISSION_DENIED;
            return true;
        }
    } else {
        reason = PCR_RUNNING_ALREADY;
        return true;
    }

    return false;
}

/**
 * ����pid�ļ�
 */
void
pid_ctrl_t::savePid()
{
    if (!m_inited) return;

    string fpid = this->getPidFilename();
    if (fpid == "") return;
    FILE *fp = fopen(fpid.c_str(), "w");
    if (fp == NULL)
        return;

    pid_t pid = getpid();
    fprintf(fp,"%d",pid);
    fclose(fp);
}

void
pid_ctrl_t::delPidFile()
{
    if (!m_inited) return;

    string fpid = getPidFilename();
    if (fpid == "") return;

    remove(fpid.c_str());
}

/**
 * ��ĳ���ź���ɱ�������еĽ���ʵ��
 * @param sig   ��Ҫ���ݸ������еĽ���ʵ�����ź���
 * @return true:�ɹ�; false:û��Ȩ�޴����ź���sig
 */
bool
pid_ctrl_t::kill(int sig)
{
    if (!m_inited) {
        return true;
    }

    // ��pid
    pid_t pid;
    if (!readPid(pid)) {
        return true;
    }


    if (::kill(pid, sig) == -1) {
        switch (errno) {
        case EINVAL:
        case ESRCH:
            // no such process, that's ok
            printf("no such process\n");
            return true;
        case EPERM:
            // permission denied
            printf("permission denied\n");
            return false;
        }
    }
    //delete the pid file
    delPidFile();
    return true;
}


}
