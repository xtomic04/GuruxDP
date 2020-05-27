//
// --------------------------------------------------------------------------
//  Gurux Ltd
//
//
//
// Filename:        $HeadURL$
//
// Version:         $Revision$,
//                  $Date$
//                  $Author$
//
// Copyright (c) Gurux Ltd
//
//---------------------------------------------------------------------------
//
//  DESCRIPTION
//
// This file is a part of Gurux Device Framework.
//
// Gurux Device Framework is Open Source software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2 of the License.
// Gurux Device Framework is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// More information of Gurux products: http://www.gurux.org
//
// This code is licensed under the GNU General Public License v2.
// Full text may be retrieved at http://www.gnu.org/licenses/gpl-2.0.txt
//---------------------------------------------------------------------------

#include <stdio.h>
#if defined(_WIN32) || defined(_WIN64)//Windows includes
#include <tchar.h>
#include <conio.h>
#include <Winsock.h> //Add support for sockets
#include <time.h>
#include <process.h>//Add support for threads
#else //Linux includes.
#define closesocket close
#include <stdio.h>
#include <pthread.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/socket.h> //Add support for sockets
#include <unistd.h> //Add support for sockets
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <netdb.h>
#endif

#include "../include/GXDLMSBase.h"

#include "../../development/include/GXTime.h"
#include "../../development/include/GXDate.h"
#include "../../development/include/GXDLMSClient.h"
#include "../../development/include/GXDLMSData.h"
#include "../../development/include/GXDLMSRegister.h"
#include "../../development/include/GXDLMSClock.h"
#include "../../development/include/GXDLMSTcpUdpSetup.h"
#include "../../development/include/GXDLMSProfileGeneric.h"
#include "../../development/include/GXDLMSAutoConnect.h"
#include "../../development/include/GXDLMSIECOpticalPortSetup.h"
#include "../../development/include/GXDLMSActivityCalendar.h"
#include "../../development/include/GXDLMSDemandRegister.h"
#include "../../development/include/GXDLMSRegisterMonitor.h"
#include "../../development/include/GXDLMSActionSchedule.h"
#include "../../development/include/GXDLMSSapAssignment.h"
#include "../../development/include/GXDLMSAutoAnswer.h"
#include "../../development/include/GXDLMSModemConfiguration.h"
#include "../../development/include/GXDLMSMacAddressSetup.h"
#include "../../development/include/GXDLMSModemInitialisation.h"
#include "../../development/include/GXDLMSActionSet.h"
#include "../../development/include/GXDLMSIp4Setup.h"
#include "../../development/include/GXDLMSPushSetup.h"
#include "../../development/include/GXDLMSAssociationLogicalName.h"
#include "../../development/include/GXDLMSAssociationShortName.h"
#include "../../development/include/GXDLMSImageTransfer.h"

using namespace std;
#if defined(_WIN32) || defined(_WIN64)//Windows
TCHAR DATAFILE[FILENAME_MAX];
char IMAGEFILE[FILENAME_MAX];
#else
char DATAFILE[FILENAME_MAX];
char IMAGEFILE[FILENAME_MAX];
#endif
int imageSize;
int Pport;

void ListenerThread(void* pVoid)
{
    CGXByteBuffer reply;
    CGXDLMSBase* server = (CGXDLMSBase*)pVoid;
    sockaddr_in add = { 0 };
    int ret;
    char tmp[10];
    CGXByteBuffer bb;
    bb.Capacity(2048);
#if defined(_WIN32) || defined(_WIN64)//If Windows
    int len;
    int AddrLen = sizeof(add);
    SOCKET socket;
#else //If Linux
    socklen_t len;
    socklen_t AddrLen = sizeof(add);
    int socket;
#endif
    struct sockaddr_in client;
    memset(&client, 0, sizeof(client));
    //Get buffer data
    basic_string<char> senderInfo;
    while (server->IsConnected())
    {
        len = sizeof(client);
        senderInfo.clear();
        socket = accept(server->GetSocket(), (struct sockaddr*)&client, &len);
        server->Reset();
        if (server->IsConnected())
        {
            server->Reset();
            if ((ret = getpeername(socket, (sockaddr*)&add, &AddrLen)) == -1)
            {
                closesocket(socket);
#if defined(_WIN32) || defined(_WIN64)//If Windows
                socket = INVALID_SOCKET;
#else //If Linux
                socket = -1;
#endif
                continue;
                //Notify error.
            }
            senderInfo = inet_ntoa(add.sin_addr);
            senderInfo.append(":");
#if _MSC_VER > 1000
            _ltoa_s(add.sin_port, tmp, 10, 10);
#else
            sprintf(tmp, "%d", add.sin_port);
#endif
            senderInfo.append(tmp);
            while (server->IsConnected())
            {
                //If client is left wait for next client.
                if ((ret = recv(socket, (char*)
                    bb.GetData() + bb.GetSize(),
                    bb.Capacity() - bb.GetSize(), 0)) == -1)
                {
                    //Notify error.
                    server->Reset();
#if defined(_WIN32) || defined(_WIN64)//If Windows
                    closesocket(socket);
                    socket = INVALID_SOCKET;
#else //If Linux
                    close(socket);
                    socket = -1;
#endif
                    break;
                }
                //If client is closed the connection.
                if (ret == 0)
                {
                    server->Reset();
#if defined(_WIN32) || defined(_WIN64)//If Windows
                    closesocket(socket);
                    socket = INVALID_SOCKET;
#else //If Linux
                    close(socket);
                    socket = -1;
#endif
                    break;
                }
                bb.SetSize(bb.GetSize() + ret);
                if (server->m_Trace == GX_TRACE_LEVEL_VERBOSE)
                {
                    printf("RX:\t%s\r\n", bb.ToHexString().c_str());
                }
                if (server->HandleRequest(bb, reply) != 0)
                {
#if defined(_WIN32) || defined(_WIN64)//If Windows
                    closesocket(socket);
                    socket = INVALID_SOCKET;
#else //If Linux
                    close(socket);
                    socket = -1;
#endif
                }
                bb.SetSize(0);
                if (reply.GetSize() != 0)
                {
                    if (server->m_Trace == GX_TRACE_LEVEL_VERBOSE)
                    {
                        printf("TX:\t%s\r\n", reply.ToHexString().c_str());
                    }
                    if (send(socket, (const char*)reply.GetData(), reply.GetSize() - reply.GetPosition(), 0) == -1)
                    {
                        //If error has occured
                        server->Reset();
#if defined(_WIN32) || defined(_WIN64)//If Windows
                        closesocket(socket);
                        socket = INVALID_SOCKET;
#else //If Linux
                        close(socket);
                        socket = -1;
#endif
                    }
                    reply.Clear();
                }
            }
            server->Reset();
        }
    }
}

#if defined(_WIN32) || defined(_WIN64)//If Windows
#else //If Linux
void * UnixListenerThread(void * pVoid)
{
    ListenerThread(pVoid);
    return NULL;
}
#endif

bool CGXDLMSBase::IsConnected()
{
#if defined(_WIN32) || defined(_WIN64)//If Windows
    return m_ServerSocket != INVALID_SOCKET;
#else //If Linux
    return m_ServerSocket != -1;
#endif
}

int CGXDLMSBase::GetSocket()
{
    return (int)m_ServerSocket;
}

int CGXDLMSBase::StartServer(int port)
{
    int ret;
    if ((ret = StopServer()) != 0)
    {
        return ret;
    }
    m_ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (!IsConnected())
    {
        //socket creation.
        return -1;
    }
    int fFlag = 1;
    if (setsockopt(m_ServerSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&fFlag, sizeof(fFlag)) == -1)
    {
        //setsockopt.
        return -1;
    }
    sockaddr_in add = { 0 };
    add.sin_port = htons(port);
    add.sin_addr.s_addr = htonl(INADDR_ANY);
#if defined(_WIN32) || defined(_WIN64)//Windows includes
    add.sin_family = AF_INET;
#else
    add.sin_family = AF_INET;
#endif
    if ((ret = ::bind(m_ServerSocket, (sockaddr*)&add, sizeof(add))) == -1)
    {
        //bind;
        return -1;
    }
    if ((ret = listen(m_ServerSocket, 1)) == -1)
    {
        //socket listen failed.
        return -1;
    }
#if defined(_WIN32) || defined(_WIN64)//Windows includes
    m_ReceiverThread = (HANDLE)_beginthread(ListenerThread, 0, (LPVOID)this);
#else
    ret = pthread_create(&m_ReceiverThread, NULL, UnixListenerThread, (void *)this);
#endif
    return ret;
}

int CGXDLMSBase::StopServer()
{
    if (IsConnected())
    {
#if defined(_WIN32) || defined(_WIN64)//Windows includes
        closesocket(m_ServerSocket);
        m_ServerSocket = INVALID_SOCKET;
        if (m_ReceiverThread != INVALID_HANDLE_VALUE)
        {
            int ret = ::WaitForSingleObject(m_ReceiverThread, 5000);
            m_ReceiverThread = INVALID_HANDLE_VALUE;
        }
#else
        close(m_ServerSocket);
        m_ServerSocket = -1;
        void *res;
        pthread_join(m_ReceiverThread, (void **)&res);
        free(res);
#endif
    }
    return 0;
}

int GetIpAddress(std::string& address)
{
    int ret;
    struct hostent *phe;
    char ac[80];
    if ((ret = gethostname(ac, sizeof(ac))) == 0)
    {
        phe = gethostbyname(ac);
        if (phe == 0)
        {
            ret = -1;
        }
        else
        {
            struct in_addr* addr = (struct in_addr*)phe->h_addr_list[0];
            address = inet_ntoa(*addr);
        }
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////
//Add Logical Device Name. 123456 is meter serial number.
///////////////////////////////////////////////////////////////////////
// COSEM Logical Device Name is defined as an octet-string of 16 octets.
// The first three octets uniquely identify the manufacturer of the device and it corresponds
// to the manufacturer's identification in IEC 62056-21.
// The following 13 octets are assigned by the manufacturer.
//The manufacturer is responsible for guaranteeing the uniqueness of these octets.
CGXDLMSData* AddLogicalDeviceName(CGXDLMSObjectCollection& items, unsigned long sn)
{
    char buff[17];
#if defined(_WIN32) || defined(_WIN64)//Windows
    sprintf_s(buff, "GRX%.13d", sn);
#else
    sprintf(buff, "GRX%.13d", sn);
#endif
    CGXDLMSVariant id;
    id.Add((const char*)buff, 16);
    CGXDLMSData* ldn = new CGXDLMSData("0.0.42.0.0.255");
    ldn->SetValue(id);
    items.push_back(ldn);
    return ldn;
}

/*
* Add firmware version.
*/
void AddFirmwareVersion(CGXDLMSObjectCollection& items)
{
    CGXDLMSVariant version;
    version = "Gurux FW 0.0.1";
    CGXDLMSData* fw = new CGXDLMSData("1.0.0.2.0.255");
    fw->SetValue(version);
    items.push_back(fw);
}

/*
* Add Electricity ID 1.
*/
void AddElectricityID1(CGXDLMSObjectCollection& items, unsigned long sn)
{
    char buff[17];
#if defined(_WIN32) || defined(_WIN64)//Windows
    sprintf_s(buff, "GRX%.13d", sn);
#else
    sprintf(buff, "GRX%.13d", sn);
#endif
    CGXDLMSVariant id;
    id.Add((const char*)buff, 16);
    CGXDLMSData* d = new CGXDLMSData("1.1.0.0.0.255");
    d->SetValue(id);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_STRING));
    items.push_back(d);
}


/*
* Add Electricity ID 2.
*/
void AddElectricityID2(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id2(sn);
    CGXDLMSData* d = new CGXDLMSData("1.1.0.0.1.255");
    d->SetValue(id2);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);
}
/*
* Add Electricity MyOBIS.
*/
void AddElectricityID01(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id01(sn);
	CGXDLMSData* d = new CGXDLMSData("1.0.1.8.0.255");
	d->SetValue(id01);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);
    
}

void AddElectricityID02(CGXDLMSObjectCollection& items, unsigned long sn)
{
	CGXDLMSVariant id02(sn);
	CGXDLMSData* d = new CGXDLMSData("1.0.1.8.1.255");
	d->SetValue(id02);
	d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
	items.push_back(d);
}
void AddElectricityID03(CGXDLMSObjectCollection& items, unsigned long sn)
{
	CGXDLMSVariant id03(sn);
	CGXDLMSData* d = new CGXDLMSData("1.0.1.8.2.255");
	d->SetValue(id03);
	d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
	items.push_back(d);
}
void AddElectricityID04(CGXDLMSObjectCollection& items, unsigned long sn)
{
	CGXDLMSVariant id2(sn);
	CGXDLMSData* d = new CGXDLMSData("1.0.1.8.3.255");
	d->SetValue(id2);
	d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
	items.push_back(d);
}
void AddElectricityID05(CGXDLMSObjectCollection& items, unsigned long sn)
{
	CGXDLMSVariant id2(sn);
	CGXDLMSData* d = new CGXDLMSData("1.0.1.8.4.255");
	d->SetValue(id2);
	d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
	items.push_back(d);
}
void AddElectricityID06(CGXDLMSObjectCollection& items, unsigned long sn)
{
	CGXDLMSVariant id2(sn);
	CGXDLMSData* d = new CGXDLMSData("1.0.2.8.0.255");
	d->SetValue(id2);
	d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
	items.push_back(d);
}
void AddElectricityID07(CGXDLMSObjectCollection& items, unsigned long sn)
{
	CGXDLMSVariant id2(sn);
	CGXDLMSData* d = new CGXDLMSData("1.0.2.8.1.255");
	d->SetValue(id2);
	d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
	items.push_back(d);
}
void AddElectricityID08(CGXDLMSObjectCollection& items, unsigned long sn)
{
	CGXDLMSVariant id2(sn);
	CGXDLMSData* d = new CGXDLMSData("1.0.2.8.2.255");
	d->SetValue(id2);
	d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
	items.push_back(d);
}
void AddElectricityID09(CGXDLMSObjectCollection& items, unsigned long sn)
{
	CGXDLMSVariant id2(sn);
	CGXDLMSData* d = new CGXDLMSData("1.0.2.8.3.255");
	d->SetValue(id2);
	d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
	items.push_back(d);
}
void AddElectricityID10(CGXDLMSObjectCollection& items, unsigned long sn)
{
	CGXDLMSVariant id2(sn);
	CGXDLMSData* d = new CGXDLMSData("1.0.2.8.4.255");
	d->SetValue(id2);
	d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
	items.push_back(d);
}

                                                                                                        ////////////////////////////////////////////////////////
void AddElectricityID11(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("0.0.96.1.1.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID12(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("0.0.96.13.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID13(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("0.0.96.13.1.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID14(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("0.0.96.3.10.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID15(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("0.1.96.3.10.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID16(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("0.2.96.3.10.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID17(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("0.0.96.14.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID18(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("0.0.96.3.10.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID19(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.7.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID20(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.7.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID21(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.8.139.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID22(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.3.8.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID23(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.3.8.1.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID24(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.3.8.2.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID25(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.3.8.3.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID26(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.3.8.4.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID27(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.4.8.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID28(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.4.8.1.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID29(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.4.8.2.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID30(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.4.8.3.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID31(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.4.8.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID32(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.7.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID33(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.21.7.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID34(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.41.7.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID35(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.61.7.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID36(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.7.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID37(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.22.7.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID38(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.42.7.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID39(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.62.7.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID40(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("0.0.0.2.2.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID41(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("0.0.13.0.1.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID42(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.6.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID43(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.6.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID44(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.8.8.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID45(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.7.8.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID46(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.6.8.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID47(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.5.8.0.255");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID48(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.7.0.130");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID49(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.7.0.133");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID50(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.7.0.166");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID51(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.7.0.200");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID52(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.7.0.131");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID53(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.7.0.134");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID54(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.7.0.167");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID55(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.1.7.0.201");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID56(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.7.0.130");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID57(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.7.0.133");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID58(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.7.0.166");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID59(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.7.0.200");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID60(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.7.0.131");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID61(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.7.0.134");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID62(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.7.0.167");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}

void AddElectricityID63(CGXDLMSObjectCollection& items, unsigned long sn)
{
    CGXDLMSVariant id02(sn);
    CGXDLMSData* d = new CGXDLMSData("1.0.2.7.0.201");
    d->SetValue(id02);
    d->GetAttributes().push_back(CGXDLMSAttribute(2, DLMS_DATA_TYPE_UINT32));
    items.push_back(d);

}



                                                                                                        ///////////////////////////////////////////////////////
/*
* Add Auto connect object.
*/
void AddAutoConnect(CGXDLMSObjectCollection& items)
{
    CGXDLMSAutoConnect* pAC = new CGXDLMSAutoConnect();
    pAC->SetMode(AUTO_CONNECT_MODE_AUTO_DIALLING_ALLOWED_ANYTIME);
    pAC->SetRepetitions(10);
    pAC->SetRepetitionDelay(60);
    //Calling is allowed between 1am to 6am.
    pAC->GetCallingWindow().push_back(std::make_pair(CGXTime(1, 0, 0, -1), CGXTime(6, 0, 0, -1)));
    pAC->GetDestinations().push_back("www.gurux.org");
    items.push_back(pAC);
}

/*
* Add Activity Calendar object.
*/
void AddActivityCalendar(CGXDLMSObjectCollection& items)
{
    CGXDLMSActivityCalendar* pActivity = new CGXDLMSActivityCalendar();
    pActivity->SetCalendarNameActive("Active");
    pActivity->GetSeasonProfileActive().push_back(new CGXDLMSSeasonProfile("Summer time", CGXDate(-1, 3, 31), ""));
    pActivity->GetWeekProfileTableActive().push_back(new CGXDLMSWeekProfile("Monday", 1, 1, 1, 1, 1, 1, 1));
    CGXDLMSDayProfile *aDp = new CGXDLMSDayProfile();
    aDp->SetDayId(1);
    CGXDateTime now = CGXDateTime::Now();
    CGXTime time = now;
    aDp->GetDaySchedules().push_back(new CGXDLMSDayProfileAction(time, "test", 1));
    pActivity->GetDayProfileTableActive().push_back(aDp);
    pActivity->SetCalendarNamePassive("Passive");
    pActivity->GetSeasonProfilePassive().push_back(new CGXDLMSSeasonProfile("Winter time", CGXDate(-1, 10, 30), ""));
    pActivity->GetWeekProfileTablePassive().push_back(new CGXDLMSWeekProfile("Tuesday", 1, 1, 1, 1, 1, 1, 1));

    CGXDLMSDayProfile* passive = new CGXDLMSDayProfile();
    passive->SetDayId(1);
    passive->GetDaySchedules().push_back(new CGXDLMSDayProfileAction(time, "0.0.1.0.0.255", 1));
    pActivity->GetDayProfileTablePassive().push_back(passive);
    CGXDateTime dt(CGXDateTime::Now());
    pActivity->SetTime(dt);
    items.push_back(pActivity);
}

/*
* Add Optical Port Setup object.
*/
void AddOpticalPortSetup(CGXDLMSObjectCollection& items)
{
    CGXDLMSIECOpticalPortSetup* pOptical = new CGXDLMSIECOpticalPortSetup();
    pOptical->SetDefaultMode(DLMS_OPTICAL_PROTOCOL_MODE_DEFAULT);
    pOptical->SetProposedBaudrate(DLMS_BAUD_RATE_9600);
    pOptical->SetDefaultBaudrate(DLMS_BAUD_RATE_300);
    pOptical->SetResponseTime(DLMS_LOCAL_PORT_RESPONSE_TIME_200_MS);
    pOptical->SetDeviceAddress("Gurux");
    pOptical->SetPassword1("Gurux1");
    pOptical->SetPassword2("Gurux2");
    pOptical->SetPassword5("Gurux5");
    items.push_back(pOptical);
}

/*
* Add Demand Register object.
*/
void AddDemandRegister(CGXDLMSObjectCollection& items)
{
    CGXDLMSDemandRegister* pDr = new CGXDLMSDemandRegister("1.0.31.4.0.255");
    pDr->SetCurrentAvarageValue(10);
    pDr->SetLastAvarageValue(20);
    pDr->SetStatus(1);
    pDr->SetStartTimeCurrent(CGXDateTime::Now());
    pDr->SetCaptureTime(CGXDateTime::Now());
    pDr->SetPeriod(10);
    pDr->SetNumberOfPeriods(1);
    items.push_back(pDr);
}

/*
* Add Register Monitor object.
*/
void AddRegisterMonitor(CGXDLMSObjectCollection& items, CGXDLMSRegister* pRegister)
{
    CGXDLMSRegisterMonitor* pRm = new CGXDLMSRegisterMonitor("0.0.16.1.0.255");
    CGXDLMSVariant threshold;
    vector<CGXDLMSVariant> thresholds;
    threshold.Add("Gurux1", 6);
    thresholds.push_back(threshold);
    threshold.Clear();
    threshold.Add("Gurux2", 6);
    thresholds.push_back(threshold);
    pRm->SetThresholds(thresholds);
    CGXDLMSMonitoredValue mv;
    mv.Update(pRegister, 2);
    pRm->SetMonitoredValue(mv);
    CGXDLMSActionSet * action = new CGXDLMSActionSet();
    string ln;
    pRm->GetLogicalName(ln);
    action->GetActionDown().SetLogicalName(ln);
    action->GetActionDown().SetScriptSelector(1);
    pRm->GetLogicalName(ln);
    action->GetActionUp().SetLogicalName(ln);
    action->GetActionUp().SetScriptSelector(1);
    pRm->GetActions().push_back(action);
    items.push_back(pRm);
}

/*
* Add action schedule object.
*/
void AddActionSchedule(CGXDLMSObjectCollection& items)
{
    CGXDLMSActionSchedule* pActionS = new CGXDLMSActionSchedule();
    pActionS->SetExecutedScriptLogicalName("1.2.3.4.5.6");
    pActionS->SetExecutedScriptSelector(1);
    pActionS->SetType(DLMS_SINGLE_ACTION_SCHEDULE_TYPE1);
    pActionS->GetExecutionTime().push_back(CGXDateTime::Now());
    items.push_back(pActionS);
}

/*
* Add SAP Assignment object.
*/
void AddSapAssignment(CGXDLMSObjectCollection& items)
{
    CGXDLMSSapAssignment* pSap = new CGXDLMSSapAssignment();
    std::map<int, basic_string<char> > list;
    list[1] = "Gurux";
    list[16] = "Gurux-2";
    pSap->SetSapAssignmentList(list);
    items.push_back(pSap);
}

/**
* Add Auto Answer object.
*/
void AddAutoAnswer(CGXDLMSObjectCollection& items)
{
    CGXDLMSAutoAnswer* pAa = new CGXDLMSAutoAnswer();
    pAa->SetMode(AUTO_CONNECT_MODE_EMAIL_SENDING);
    pAa->GetListeningWindow().push_back(std::pair<CGXDateTime, CGXDateTime>(CGXDateTime(-1, -1, -1, 6, -1, -1, -1), CGXDateTime(-1, -1, -1, 8, -1, -1, -1)));
    pAa->SetStatus(AUTO_ANSWER_STATUS_INACTIVE);
    pAa->SetNumberOfCalls(0);
    pAa->SetNumberOfRingsInListeningWindow(1);
    pAa->SetNumberOfRingsOutListeningWindow(2);
    items.push_back(pAa);
}

/*
* Add Modem Configuration object.
*/
void AddModemConfiguration(CGXDLMSObjectCollection& items)
{
    CGXDLMSModemConfiguration* pMc = new CGXDLMSModemConfiguration();
    pMc->SetCommunicationSpeed(DLMS_BAUD_RATE_38400);
    CGXDLMSModemInitialisation init;
    vector<CGXDLMSModemInitialisation> initialisationStrings;
    init.SetRequest("AT");
    init.SetResponse("OK");
    init.SetDelay(0);
    initialisationStrings.push_back(init);
    pMc->SetInitialisationStrings(initialisationStrings);
    items.push_back(pMc);
}

/**
* Add MAC Address Setup object.
*/
void AddMacAddressSetup(CGXDLMSObjectCollection& items)
{
    CGXDLMSMacAddressSetup* pMac = new CGXDLMSMacAddressSetup();
    pMac->SetMacAddress("00:11:22:33:44:55:66");
    items.push_back(pMac);
}

/**
* Add IP4 setup object.
*/
CGXDLMSIp4Setup* AddIp4Setup(CGXDLMSObjectCollection& items, std::string& address)
{
    CGXDLMSIp4Setup* pIp4 = new CGXDLMSIp4Setup();
    pIp4->SetIPAddress(address);
    items.push_back(pIp4);
    return pIp4;
}

/*
* Generic initialize for all servers.
*/
int CGXDLMSBase::Init(int port, GX_TRACE_LEVEL trace)
{
    int ret;
    Pport = port;
    m_Trace = trace;
    if ((ret = StartServer(port)) != 0)
    {
        return ret;
    }
    //Get local IP address.
    std::string address;
    GetIpAddress(address);
    
    unsigned long sn = 123456;
	unsigned long volt = 230;
    CGXDLMSData* ldn = AddLogicalDeviceName(GetItems(), sn);
	
    //Add firmaware.
    AddFirmwareVersion(GetItems());
    AddElectricityID1(GetItems(), sn);
    AddElectricityID2(GetItems(), sn);
	AddElectricityID01(GetItems(), volt);
	AddElectricityID02(GetItems(), volt);
	AddElectricityID03(GetItems(), volt);
	AddElectricityID04(GetItems(), volt);
	AddElectricityID05(GetItems(), volt);
	AddElectricityID06(GetItems(), volt);
	AddElectricityID07(GetItems(), volt);
	AddElectricityID08(GetItems(), volt);
	AddElectricityID09(GetItems(), volt);
	AddElectricityID10(GetItems(), volt);

    AddElectricityID11(GetItems(), volt);
    AddElectricityID12(GetItems(), volt);
    AddElectricityID13(GetItems(), volt);
    AddElectricityID14(GetItems(), volt);
    AddElectricityID15(GetItems(), volt);
    AddElectricityID16(GetItems(), volt);
    AddElectricityID17(GetItems(), volt);
    AddElectricityID18(GetItems(), volt);
    AddElectricityID19(GetItems(), volt);
    AddElectricityID20(GetItems(), volt);
    AddElectricityID21(GetItems(), volt);
    AddElectricityID22(GetItems(), volt);
    AddElectricityID23(GetItems(), volt);
    AddElectricityID24(GetItems(), volt);
    AddElectricityID25(GetItems(), volt);
    AddElectricityID26(GetItems(), volt);
    AddElectricityID27(GetItems(), volt);
    AddElectricityID28(GetItems(), volt);
    AddElectricityID29(GetItems(), volt);
    AddElectricityID30(GetItems(), volt);
    AddElectricityID31(GetItems(), volt);
    AddElectricityID32(GetItems(), volt);
    AddElectricityID33(GetItems(), volt);
    AddElectricityID34(GetItems(), volt);
    AddElectricityID35(GetItems(), volt);
    AddElectricityID36(GetItems(), volt);
    AddElectricityID37(GetItems(), volt);
    AddElectricityID38(GetItems(), volt);
    AddElectricityID39(GetItems(), volt);
    AddElectricityID40(GetItems(), volt);
    AddElectricityID41(GetItems(), volt);
    AddElectricityID42(GetItems(), volt);
    AddElectricityID43(GetItems(), volt);
    AddElectricityID44(GetItems(), volt);
    AddElectricityID45(GetItems(), volt);
    AddElectricityID46(GetItems(), volt);
    AddElectricityID47(GetItems(), volt);
    AddElectricityID48(GetItems(), volt);
    AddElectricityID49(GetItems(), volt);
    AddElectricityID50(GetItems(), volt);
    AddElectricityID51(GetItems(), volt);
    AddElectricityID52(GetItems(), volt);
    AddElectricityID53(GetItems(), volt);
    AddElectricityID54(GetItems(), volt);
    AddElectricityID55(GetItems(), volt);
    AddElectricityID56(GetItems(), volt);
    AddElectricityID57(GetItems(), volt);
    AddElectricityID58(GetItems(), volt);
    AddElectricityID59(GetItems(), volt);
    AddElectricityID60(GetItems(), volt);
    AddElectricityID61(GetItems(), volt);
    AddElectricityID62(GetItems(), volt);
    AddElectricityID63(GetItems(), volt);


    //Add Last avarage.
    CGXDLMSRegister* pRegister = new CGXDLMSRegister("1.1.21.25.0.255");
    //Set access right. Client can't change Device name.
    pRegister->SetAccess(2, DLMS_ACCESS_MODE_READ);
    GetItems().push_back(pRegister);
    //Add default clock. Clock's Logical Name is 0.0.1.0.0.255.
    CGXDLMSClock* pClock = new CGXDLMSClock();
    CGXDateTime begin(-1, 9, 1, -1, -1, -1, -1);
    pClock->SetBegin(begin);
    CGXDateTime end(-1, 3, 1, -1, -1, -1, -1);
    pClock->SetEnd(end);
    pClock->SetTimeZone(CGXDateTime::GetCurrentTimeZone());
    pClock->SetDeviation(CGXDateTime::GetCurrentDeviation());
    GetItems().push_back(pClock);
    ///////////////////////////////////////////////////////////////////////
    //Add profile generic (historical data) object.
    CGXDLMSProfileGeneric* profileGeneric = new CGXDLMSProfileGeneric("1.0.99.1.0.255");
    //Set capture period to 60 second.
    profileGeneric->SetCapturePeriod(60);
    profileGeneric->SetSortMethod(DLMS_SORT_METHOD_FIFO);
    profileGeneric->SetSortObject(pClock);
    //Add colums.
    //Set saved attribute index.
    CGXDLMSCaptureObject * capture = new CGXDLMSCaptureObject(2, 0);
    profileGeneric->GetCaptureObjects().push_back(std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*>(pClock, capture));
    //Set saved attribute index.
    capture = new CGXDLMSCaptureObject(2, 0);
    profileGeneric->GetCaptureObjects().push_back(std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*>(pRegister, capture));
    GetItems().push_back(profileGeneric);

    // Create 10 000 rows for profile generic file.
    // In example profile generic we have two columns.
    // Date time and integer value.
    int rowCount = 10000;
    CGXDateTime tm = CGXDateTime::Now();
    tm.AddMinutes(-tm.GetValue().tm_min);
    tm.AddSeconds(-tm.GetValue().tm_sec);
    tm.AddHours(-(rowCount - 1));

#if defined(_WIN32) || defined(_WIN64)//Windows
    FILE* f;
    _tfopen_s(&f, DATAFILE, _T("w"));
#else
    FILE* f = fopen(DATAFILE, "w");
#endif
    for (int pos = 0; pos != rowCount; ++pos)
    {
        fprintf(f, "%s;%d\n", tm.ToString().c_str(), pos + 1);
        tm.AddHours(1);
    }
    fclose(f);
    //Maximum row count.
    profileGeneric->SetEntriesInUse(rowCount);
    profileGeneric->SetProfileEntries(rowCount);

    ///////////////////////////////////////////////////////////////////////
    //Add Auto connect object.
    AddAutoConnect(GetItems());

    ///////////////////////////////////////////////////////////////////////
    //Add Activity Calendar object.
    AddActivityCalendar(GetItems());

    ///////////////////////////////////////////////////////////////////////
    //Add Optical Port Setup object.
    AddOpticalPortSetup(GetItems());
    ///////////////////////////////////////////////////////////////////////
    //Add Demand Register object.
    AddDemandRegister(GetItems());

    ///////////////////////////////////////////////////////////////////////
    //Add Register Monitor object.
    AddRegisterMonitor(GetItems(), pRegister);

    ///////////////////////////////////////////////////////////////////////
    //Add action schedule object.
    AddActionSchedule(GetItems());

    ///////////////////////////////////////////////////////////////////////
    //Add SAP Assignment object.
    AddSapAssignment(GetItems());

    ///////////////////////////////////////////////////////////////////////
    //Add Auto Answer object.
    AddAutoAnswer(GetItems());

    ///////////////////////////////////////////////////////////////////////
    //Add Modem Configuration object.
    AddModemConfiguration(GetItems());

    ///////////////////////////////////////////////////////////////////////
    //Add Mac Address Setup object.
    AddMacAddressSetup(GetItems());
    ///////////////////////////////////////////////////////////////////////
    //Add IP4 Setup object.
    CGXDLMSIp4Setup* pIp4 = AddIp4Setup(GetItems(), address);

    ///////////////////////////////////////////////////////////////////////
    //Add Push Setup object.
    CGXDLMSPushSetup* pPush = new CGXDLMSPushSetup();
    pPush->SetDestination(address);
    GetItems().push_back(pPush);

    // Add push object itself. This is needed to tell structure of data to
    // the Push listener.
    pPush->GetPushObjectList().push_back(std::pair<CGXDLMSObject*, CGXDLMSCaptureObject>(pPush, CGXDLMSCaptureObject(2, 0)));
    //Add logical device name.
    pPush->GetPushObjectList().push_back(std::pair<CGXDLMSObject*, CGXDLMSCaptureObject>(ldn, CGXDLMSCaptureObject(2, 0)));
    //Add 0.0.25.1.0.255 Ch. 0 IPv4 setup IP address.
    pPush->GetPushObjectList().push_back(std::pair<CGXDLMSObject*, CGXDLMSCaptureObject>(pIp4, CGXDLMSCaptureObject(3, 0)));

    ///////////////////////////////////////////////////////////////////////
    //Add image transfer object.
    CGXDLMSImageTransfer* image = new CGXDLMSImageTransfer();
    GetItems().push_back(image);
    ///////////////////////////////////////////////////////////////////////
    //Server must initialize after all objects are added.
    ret = Initialize();
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    return DLMS_ERROR_CODE_OK;
}

CGXDLMSObject* CGXDLMSBase::FindObject(
    DLMS_OBJECT_TYPE objectType,
    int sn,
    std::string& ln)
{
    return NULL;
}

/**
* Return data using start and end indexes.
*
* @param p
*            ProfileGeneric
* @param index
* @param count
* @return Add data Rows
*/
void GetProfileGenericDataByEntry(CGXDLMSProfileGeneric* p, long index, long count)
{
    int len, month = 0, day = 0, year = 0, hour = 0, minute = 0, second = 0, value = 0;
    // Clear old data. It's already serialized.
    p->GetBuffer().clear();
    if (count != 0)
    {
#if defined(_WIN32) || defined(_WIN64)//Windows
        FILE* f;
        _tfopen_s(&f, DATAFILE, _T("r"));
#else
        FILE* f = fopen(DATAFILE, "r");
#endif
        if (f != NULL)
        {
#if defined(_WIN32) || defined(_WIN64)//Windows
            while ((len = fscanf_s(f, "%d/%d/%d %d:%d:%d;%d", &month, &day, &year, &hour, &minute, &second, &value)) != -1)
#else
            while ((len = fscanf(f, "%d/%d/%d %d:%d:%d;%d", &month, &day, &year, &hour, &minute, &second, &value)) != -1)
#endif
            {
                // Skip row
                if (index > 0)
                {
                    --index;
                }
                else if (len == 7)
                {
                    if (p->GetBuffer().size() == count)
                    {
                        break;
                    }
                    CGXDateTime tm(2000 + year, month, day, hour, minute, second, 0, 0x8000);
                    std::vector<CGXDLMSVariant> row;
                    row.push_back(tm);
                    row.push_back(value);
                    p->GetBuffer().push_back(row);
                }
                if (p->GetBuffer().size() == count)
                {
                    break;
                }
            }
            fclose(f);
        }
    }
}

/**
* Find start index and row count using start and end date time.
*
* @param start
*            Start time.
* @param end
*            End time
* @param index
*            Start index.
* @param count
*            Item count.
*/
void GetProfileGenericDataByRange(CGXDLMSValueEventArg* e)
{
    int len, month = 0, day = 0, year = 0, hour = 0, minute = 0, second = 0, value = 0;
    CGXDLMSVariant start, end;
    CGXByteBuffer bb;
    bb.Set(e->GetParameters().Arr[1].byteArr, e->GetParameters().Arr[1].size);
    CGXDLMSClient::ChangeType(bb, DLMS_DATA_TYPE_DATETIME, start);
    bb.Clear();
    bb.Set(e->GetParameters().Arr[2].byteArr, e->GetParameters().Arr[2].size);
    CGXDLMSClient::ChangeType(bb, DLMS_DATA_TYPE_DATETIME, end);
#if defined(_WIN32) || defined(_WIN64)//Windows
    FILE* f;
    _tfopen_s(&f, DATAFILE, _T("r"));
#else
    FILE* f = fopen(DATAFILE, "r");
#endif
    if (f != NULL)
    {
#if defined(_WIN32) || defined(_WIN64)//Windows
        while ((len = fscanf_s(f, "%d/%d/%d %d:%d:%d;%d", &month, &day, &year, &hour, &minute, &second, &value)) != -1)
#else
        while ((len = fscanf(f, "%d/%d/%d %d:%d:%d;%d", &month, &day, &year, &hour, &minute, &second, &value)) != -1)
#endif
        {
            CGXDateTime tm(2000 + year, month, day, hour, minute, second, 0, 0x8000);
            if (tm.CompareTo(end.dateTime) > 0)
            {
                // If all data is read.
                break;
            }
            if (tm.CompareTo(start.dateTime) < 0)
            {
                // If we have not find first item.
                e->SetRowBeginIndex(e->GetRowBeginIndex() + 1);
            }
            e->SetRowEndIndex(e->GetRowEndIndex() + 1);
        }
        fclose(f);
    }
}

/**
* Get row count.
*
* @return
*/
int GetProfileGenericDataCount() {
    int rows = 0;
    int ch;
#if defined(_WIN32) || defined(_WIN64)//Windows
    FILE* f;
    _tfopen_s(&f, DATAFILE, _T("r"));
#else
    FILE* f = fopen(DATAFILE, "r");
#endif
    if (f != NULL)
    {
        while ((ch = fgetc(f)) != EOF)
        {
            if (ch == '\n')
            {
                ++rows;
            }
        }
        fclose(f);
    }
    return rows;
}


/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
void CGXDLMSBase::PreRead(std::vector<CGXDLMSValueEventArg*>& args)
{
    CGXDLMSVariant value;
    CGXDLMSObject* pObj;
    int ret, index;
    DLMS_OBJECT_TYPE type;
    std::string ln;
    for (std::vector<CGXDLMSValueEventArg*>::iterator it = args.begin(); it != args.end(); ++it)
    {
        //Let framework handle Logical Name read.
        if ((*it)->GetIndex() == 1)
        {
            continue;
        }
        //Get attribute index.
        index = (*it)->GetIndex();
        pObj = (*it)->GetTarget();
        //Get target type.
        type = pObj->GetObjectType();
        if (type == DLMS_OBJECT_TYPE_PROFILE_GENERIC)
        {
            CGXDLMSProfileGeneric* p = (CGXDLMSProfileGeneric*)pObj;
            // If buffer is read and we want to save memory.
            if (index == 7)
            {
                // If client wants to know EntriesInUse.
                p->SetEntriesInUse(GetProfileGenericDataCount());
            }
            else if (index == 2)
            {
                // Read rows from file.
                // If reading first time.
                if ((*it)->GetRowEndIndex() == 0)
                {
                    if ((*it)->GetSelector() == 0)
                    {
                        (*it)->SetRowEndIndex(GetProfileGenericDataCount());
                    }
                    else if ((*it)->GetSelector() == 1)
                    {
                        // Read by entry.
                        GetProfileGenericDataByRange((*it));
                    }
                    else if ((*it)->GetSelector() == 2)
                    {
                        // Read by range.
                        unsigned int begin = (*it)->GetParameters().Arr[0].ulVal;
                        (*it)->SetRowBeginIndex(begin);
                        (*it)->SetRowEndIndex((*it)->GetParameters().Arr[1].ulVal);
                        // If client wants to read more data what we have.
                        int cnt = GetProfileGenericDataCount();
                        if ((*it)->GetRowEndIndex() > cnt)
                        {
                            (*it)->SetRowEndIndex(cnt);
                            if ((*it)->GetRowEndIndex() < 0)
                            {
                                (*it)->SetRowEndIndex(0);
                            }
                        }
                    }
                }
                long count = (*it)->GetRowEndIndex() - (*it)->GetRowBeginIndex() + 1;
                // Read only rows that can fit to one PDU.
                if ((*it)->GetRowEndIndex() - (*it)->GetRowBeginIndex() > (*it)->GetRowToPdu())
                {
                    count = (*it)->GetRowToPdu();
                }
                GetProfileGenericDataByEntry(p, (*it)->GetRowBeginIndex(), count);
            }
            continue;
        }
        //Framework will handle Association objects automatically.
        if (type == DLMS_OBJECT_TYPE_ASSOCIATION_LOGICAL_NAME ||
            type == DLMS_OBJECT_TYPE_ASSOCIATION_SHORT_NAME ||
            //Framework will handle profile generic automatically.
            type == DLMS_OBJECT_TYPE_PROFILE_GENERIC)
        {
            continue;
        }
        DLMS_DATA_TYPE ui, dt;
        (*it)->GetTarget()->GetUIDataType(index, ui);
        (*it)->GetTarget()->GetDataType(index, dt);
        //Update date and time of clock object.
        if (type == DLMS_OBJECT_TYPE_CLOCK && index == 2)
        {
            CGXDateTime tm = CGXDateTime::Now();
            ((CGXDLMSClock*)pObj)->SetTime(tm);
            continue;
        }
        else if (type == DLMS_OBJECT_TYPE_REGISTER_MONITOR)
        {
            CGXDLMSRegisterMonitor* pRm = (CGXDLMSRegisterMonitor*)pObj;
            if (index == 2)
            {
                //Initialize random seed.
                srand((unsigned int)time(NULL));
                pRm->GetThresholds().clear();
                pRm->GetThresholds().push_back(rand() % 100 + 1);
                continue;
            }
        }
        else
        {
            CGXDLMSVariant null;
            CGXDLMSValueEventArg e(pObj, index);
            ret = ((IGXDLMSBase*)pObj)->GetValue(m_Settings, e);
            if (ret != DLMS_ERROR_CODE_OK)
            {
                //TODO: Show error.
                continue;
            }
            //If data is not assigned and value type is unknown return number.
            DLMS_DATA_TYPE tp = e.GetValue().vt;
            if (tp == DLMS_DATA_TYPE_INT8 ||
                tp == DLMS_DATA_TYPE_INT16 ||
                tp == DLMS_DATA_TYPE_INT32 ||
                tp == DLMS_DATA_TYPE_INT64 ||
                tp == DLMS_DATA_TYPE_UINT8 ||
                tp == DLMS_DATA_TYPE_UINT16 ||
                tp == DLMS_DATA_TYPE_UINT32 ||
                tp == DLMS_DATA_TYPE_UINT64)
            {
                //Initialize random seed.
                srand((unsigned int)time(NULL));
                value = rand() % 100 + 1;
                value.vt = tp;
                e.SetValue(value);
            }
        }
    }
}

void CGXDLMSBase::PostRead(std::vector<CGXDLMSValueEventArg*>& args)
{
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
void CGXDLMSBase::PreWrite(std::vector<CGXDLMSValueEventArg*>& args)
{
    std::string ln;
    for (std::vector<CGXDLMSValueEventArg*>::iterator it = args.begin(); it != args.end(); ++it)
    {
        if (m_Trace > GX_TRACE_LEVEL_WARNING)
        {
            (*it)->GetTarget()->GetLogicalName(ln);
            printf("Writing: %s \r\n", ln.c_str());
            ln.clear();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
void CGXDLMSBase::PostWrite(std::vector<CGXDLMSValueEventArg*>& args)
{
}

//In this example we wait 5 seconds before image is verified or activated.
time_t imageActionStartTime;

void HandleImageTransfer(CGXDLMSValueEventArg* e)
{
    CGXDLMSImageTransfer* i = (CGXDLMSImageTransfer*)e->GetTarget();
    //Image name and size to transfer
    FILE *f;
    if (e->GetIndex() == 1)
    {
        i->SetImageTransferStatus(DLMS_IMAGE_TRANSFER_STATUS_NOT_INITIATED);
        if (e->GetParameters().Arr.size() != 2)
        {
            e->SetError(DLMS_ERROR_CODE_UNMATCH_TYPE);
            return;
        }
        imageSize = e->GetParameters().Arr[1].ToInteger();
        char *p = strrchr(IMAGEFILE, '\\');
        ++p;
        *p = '\0';
#if defined(_WIN32) || defined(_WIN64)//If Windows
        strncat_s(IMAGEFILE, (char*)e->GetParameters().Arr[0].byteArr, (int)e->GetParameters().Arr[0].GetSize());
        strcat_s(IMAGEFILE, ".bin");
#else
        strncat(IMAGEFILE, (char*)e->GetParameters().Arr[0].byteArr, (int)e->GetParameters().Arr[0].GetSize());
        strcat(IMAGEFILE, ".bin");
#endif
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)//If Windows or Linux
        printf("Updating image %s Size: %d\n", IMAGEFILE, imageSize);
#endif
#if defined(_WIN32) || defined(_WIN64)//If Windows
        fopen_s(&f, IMAGEFILE, "wb");
#else
        f = fopen(IMAGEFILE, "wb");
#endif
        if (!f)
        {
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)//If Windows or Linux
            printf("Unable to open file %s\n", IMAGEFILE);
#endif
            e->SetError(DLMS_ERROR_CODE_HARDWARE_FAULT);
            return;
        }
        fclose(f);
    }
    //Transfers one block of the Image to the server
    else if (e->GetIndex() == 2)
    {
        if (e->GetParameters().Arr.size() != 2)
        {
            e->SetError(DLMS_ERROR_CODE_UNMATCH_TYPE);
            return;
        }
        i->SetImageTransferStatus(DLMS_IMAGE_TRANSFER_STATUS_INITIATED);
#if defined(_WIN32) || defined(_WIN64)//If Windows
        fopen_s(&f, IMAGEFILE, "ab");
#else
        f = fopen(IMAGEFILE, "ab");
#endif
        if (!f)
        {
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)//If Windows or Linux
            printf("Unable to open file %s\n", IMAGEFILE);
#endif
            e->SetError(DLMS_ERROR_CODE_HARDWARE_FAULT);
            return;
        }

        int ret = fwrite(e->GetParameters().Arr[1].byteArr, 1, (int)e->GetParameters().Arr[1].GetSize(), f);
        fclose(f);
        if (ret != e->GetParameters().Arr[1].GetSize())
        {
            e->SetError(DLMS_ERROR_CODE_UNMATCH_TYPE);
        }
        imageActionStartTime = time(NULL);
        return;
    }
    //Verifies the integrity of the Image before activation.
    else if (e->GetIndex() == 3)
    {
        i->SetImageTransferStatus(DLMS_IMAGE_TRANSFER_STATUS_VERIFICATION_INITIATED);
#if defined(_WIN32) || defined(_WIN64)//If Windows
        fopen_s(&f, IMAGEFILE, "rb");
#else
        f = fopen(IMAGEFILE, "rb");
#endif
        if (!f)
        {
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)//If Windows or Linux
            printf("Unable to open file %s\n", IMAGEFILE);
#endif
            e->SetError(DLMS_ERROR_CODE_HARDWARE_FAULT);
            return;
        }
        fseek(f, 0L, SEEK_END);
        int size = (int)ftell(f);
        fclose(f);
        if (size != imageSize)
        {
            i->SetImageTransferStatus(DLMS_IMAGE_TRANSFER_STATUS_VERIFICATION_FAILED);
            e->SetError(DLMS_ERROR_CODE_OTHER_REASON);
        }
        else
        {
            //Wait 5 seconds before image is verified.
            if (time(NULL) - imageActionStartTime < 5)
            {
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)//If Windows or Linux
                printf("Image verification is on progress.\n");
#endif
                e->SetError(DLMS_ERROR_CODE_TEMPORARY_FAILURE);
            }
            else
            {
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)//If Windows or Linux
                printf("Image is verificated");
#endif
                i->SetImageTransferStatus(DLMS_IMAGE_TRANSFER_STATUS_VERIFICATION_SUCCESSFUL);
                imageActionStartTime = time(NULL);
            }
        }
    }
    //Activates the Image.
    else if (e->GetIndex() == 4)
    {
        i->SetImageTransferStatus(DLMS_IMAGE_TRANSFER_STATUS_ACTIVATION_INITIATED);
        //Wait 5 seconds before image is activated.
        if (time(NULL) - imageActionStartTime < 5)
        {
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)//If Windows or Linux
            printf("Image activation is on progress.\n");
#endif
            e->SetError(DLMS_ERROR_CODE_TEMPORARY_FAILURE);
        }
        else
        {
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)//If Windows or Linux
            printf("Image is activated.");
#endif
            i->SetImageTransferStatus(DLMS_IMAGE_TRANSFER_STATUS_ACTIVATION_SUCCESSFUL);
            imageActionStartTime = time(NULL);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
void CGXDLMSBase::PreAction(std::vector<CGXDLMSValueEventArg*>& args)
{
    for (std::vector<CGXDLMSValueEventArg*>::iterator it = args.begin(); it != args.end(); ++it)
    {
        if ((*it)->GetTarget()->GetObjectType() == DLMS_OBJECT_TYPE_IMAGE_TRANSFER)
        {
            HandleImageTransfer(*it);
        }
    }
}

void Capture(CGXDLMSProfileGeneric* pg)
{
    std::vector<std::string> values;
    std::string value;
    unsigned char first = 1;
    int cnt = GetProfileGenericDataCount();
    FILE* f;
#if defined(_WIN32) || defined(_WIN64)//Windows
    _tfopen_s(&f, DATAFILE, _T("a"));
#else
    f = fopen(DATAFILE, "a");
#endif
    for (std::vector<std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*> >::iterator it = pg->GetCaptureObjects().begin();
        it != pg->GetCaptureObjects().end(); ++it)
    {
        if (first)
        {
            first = 0;
        }
        else
        {
            fprintf(f, ";");
            values.clear();
        }
        if (it->first->GetObjectType() == DLMS_OBJECT_TYPE_CLOCK && it->second->GetAttributeIndex() == 2)
        {
            value = CGXDateTime::Now().ToString();
        }
        else
        {
            // TODO: Read value here example from the meter if it's not
            // updated automatically.
            it->first->GetValues(values);
            value = values.at(it->second->GetAttributeIndex() - 1);
            if (value == "")
            {
                char tmp[20];
                // Generate random value here.
#if defined(_WIN32) || defined(_WIN64)//If Windows
                sprintf_s(tmp, "%d", ++cnt);
#else
                sprintf(tmp, "%d", ++cnt);
#endif
                value = tmp;
            }
        }
        fprintf(f, "%s", value.c_str());
    }
    fprintf(f, "\n");
    fclose(f);
}

void HandleProfileGenericActions(CGXDLMSValueEventArg* it)
{
    CGXDLMSProfileGeneric* pg = (CGXDLMSProfileGeneric*)it->GetTarget();
    if (it->GetIndex() == 1)
    {
        FILE* f;
        // Profile generic clear is called. Clear data.
#if defined(_WIN32) || defined(_WIN64)//Windows
        _tfopen_s(&f, DATAFILE, _T("w"));
#else
        f = fopen(DATAFILE, "w");
#endif
        fclose(f);
    }
    else if (it->GetIndex() == 2)
    {
        // Profile generic Capture is called.
    }
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
void CGXDLMSBase::PostAction(std::vector<CGXDLMSValueEventArg*>& args)
{
    for (std::vector<CGXDLMSValueEventArg*>::iterator it = args.begin(); it != args.end(); ++it)
    {
        if ((*it)->GetTarget()->GetObjectType() == DLMS_OBJECT_TYPE_PROFILE_GENERIC)
        {
            HandleProfileGenericActions(*it);
        }
    }
}


bool CGXDLMSBase::IsTarget(
    unsigned long int serverAddress,
    unsigned long clientAddress)
{
    return true;
}

DLMS_SOURCE_DIAGNOSTIC CGXDLMSBase::ValidateAuthentication(
    DLMS_AUTHENTICATION authentication,
    CGXByteBuffer& password)
{
    if (authentication == DLMS_AUTHENTICATION_NONE)
    {
        //Uncomment this if authentication is always required.
        //return DLMS_SOURCE_DIAGNOSTIC_AUTHENTICATION_MECHANISM_NAME_REQUIRED;
    }

    if (authentication == DLMS_AUTHENTICATION_LOW)
    {
        CGXByteBuffer expected;
        std::string name = "0.0.40.0.0.255";
        if (GetUseLogicalNameReferencing())
        {
            CGXDLMSAssociationLogicalName* ln =
                (CGXDLMSAssociationLogicalName*)GetItems().FindByLN(
                    DLMS_OBJECT_TYPE_ASSOCIATION_LOGICAL_NAME, name);
            expected = ln->GetSecret();
        }
        else
        {
            CGXDLMSAssociationShortName* sn =
                (CGXDLMSAssociationShortName*)GetItems().FindByLN(
                    DLMS_OBJECT_TYPE_ASSOCIATION_SHORT_NAME, name);
            expected = sn->GetSecret();
        }
        if (expected.Compare(password.GetData(), password.GetSize()))
        {
            return DLMS_SOURCE_DIAGNOSTIC_NONE;
        }
        return DLMS_SOURCE_DIAGNOSTIC_AUTHENTICATION_FAILURE;
    }
    // Other authentication levels are check on phase two.
    return DLMS_SOURCE_DIAGNOSTIC_NONE;
}

DLMS_ACCESS_MODE CGXDLMSBase::GetAttributeAccess(CGXDLMSValueEventArg* arg)
{
    // Only read is allowed
    if (arg->GetSettings()->GetAuthentication() == DLMS_AUTHENTICATION_NONE)
    {
        return DLMS_ACCESS_MODE_READ;
    }
    // Only clock write is allowed.
    if (arg->GetSettings()->GetAuthentication() == DLMS_AUTHENTICATION_LOW)
    {
        if (arg->GetTarget()->GetObjectType() == DLMS_OBJECT_TYPE_CLOCK)
        {
            return DLMS_ACCESS_MODE_READ_WRITE;
        }
        return DLMS_ACCESS_MODE_READ;
    }
    // All writes are allowed.
    return DLMS_ACCESS_MODE_READ_WRITE;
}

/**
* Get method access mode.
*
* @param arg
*            Value event argument.
* @return Method access mode.
* @throws Exception
*             Server handler occurred exceptions.
*/
DLMS_METHOD_ACCESS_MODE CGXDLMSBase::GetMethodAccess(CGXDLMSValueEventArg* arg)
{
    // Methods are not allowed.
    if (arg->GetSettings()->GetAuthentication() == DLMS_AUTHENTICATION_NONE)
    {
        return DLMS_METHOD_ACCESS_MODE_NONE;
    }
    // Only clock and profile generic methods are allowed.
    if (arg->GetSettings()->GetAuthentication() == DLMS_AUTHENTICATION_LOW)
    {
        if (arg->GetTarget()->GetObjectType() == DLMS_OBJECT_TYPE_CLOCK ||
            arg->GetTarget()->GetObjectType() == DLMS_OBJECT_TYPE_PROFILE_GENERIC)
        {
            return DLMS_METHOD_ACCESS_MODE_ACCESS;
        }
        return DLMS_METHOD_ACCESS_MODE_NONE;
    }
    return DLMS_METHOD_ACCESS_MODE_ACCESS;
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
void CGXDLMSBase::Connected(
    CGXDLMSConnectionEventArgs& connectionInfo)
{
    if (m_Trace > GX_TRACE_LEVEL_WARNING)
    {
        FILE* file;
        char buf[0x100];
        snprintf(buf, sizeof(buf), "dataExport%d.txt", Pport);
        file = fopen(buf, "a");


        printf("Connected.\r\n");
        std::time_t result = std::time(nullptr);
        //cas zahajeni prenosu
        printf("Cas pripojeni.\n");
        std::cout << std::asctime(std::localtime(&result)); 
        
        fprintf(file, "\nConnected.\n");
        fprintf(file, "Cas pripojeni.\n");
        fprintf(file, "%s", std::asctime(std::localtime(&result)));
        fclose(file);
        printf("\r\n");



    }
}

void CGXDLMSBase::InvalidConnection(
    CGXDLMSConnectionEventArgs& connectionInfo)
{
    if (m_Trace > GX_TRACE_LEVEL_WARNING)
    {
        printf("InvalidConnection.\r\n");
    }
}
/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////
void CGXDLMSBase::Disconnected(
    CGXDLMSConnectionEventArgs& connectionInfo)
{
    if (m_Trace > GX_TRACE_LEVEL_WARNING)
    {
        FILE* file;
        char buf[0x100];
        snprintf(buf, sizeof(buf), "dataExport%d.txt", Pport);
        file = fopen(buf, "a");

        printf("Disconnected.\r\n");
        std::time_t result = std::time(nullptr);
        //cas zahajeni prenosu
        printf("Cas odpojeni.\n");
        std::cout << std::asctime(std::localtime(&result));
        
        fprintf(file, "\nDisconnected.\n");
        fprintf(file, "Cas odpojeni.\n");
        fprintf(file, "%s", std::asctime(std::localtime(&result)));
        fclose(file);
        printf("\r\n");
    }
}

void CGXDLMSBase::PreGet(
    std::vector<CGXDLMSValueEventArg*>& args)
{
    for (std::vector<CGXDLMSValueEventArg*>::iterator it = args.begin(); it != args.end(); ++it)
    {
        if ((*it)->GetTarget()->GetObjectType() == DLMS_OBJECT_TYPE_PROFILE_GENERIC)
        {
            CGXDLMSProfileGeneric* pg = (CGXDLMSProfileGeneric*)(*it)->GetTarget();
            Capture(pg);
            (*it)->SetHandled(true);
        }
    }
}

void CGXDLMSBase::PostGet(
    std::vector<CGXDLMSValueEventArg*>& args)
{

}