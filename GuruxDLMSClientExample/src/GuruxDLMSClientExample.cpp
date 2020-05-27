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

// GuruxDLMSClientExample.cpp : Defines the entry point for the Gurux DLMS Client example.
#if defined(_WIN32) || defined(_WIN64)//Windows
#include "../include/getopt.h"
#endif
#include "../include/communication.h"
#include <ctime>
#include "../../development/include/GXDLMSTcpUdpSetup.h"



static void ShowHelp()
{
	printf("GuruxDlmsSample reads data from the DLMS/COSEM device.\r\n");
	printf("GuruxDlmsSample -h [Meter IP Address] -p [Meter Port No] -c 16 -s 1 -r SN\r\n");
	printf(" -h \t host name or IP address.\r\n");
	printf(" -p \t port number or name (Example: 1000).\r\n");
	printf(" -S [COM1:9600:8None1]\t serial port.");
	printf(" -i IEC is a start protocol.\r\n");
	printf(" -a \t Authentication (None, Low, High).\r\n");
	printf(" -P \t Password for authentication.\r\n");
	printf(" -c \t Client address. (Default: 16)\r\n");
	printf(" -s \t Server address. (Default: 1)\r\n");
	printf(" -n \t Server address as serial number.\r\n");
	printf(" -r [sn, sn]\t Short name or Logican Name (default) referencing is used.\r\n");
	printf(" -w WRAPPER profile is used. HDLC is default.\r\n");
	printf(" -t Trace messages.\r\n");
	printf(" -g \"0.0.1.0.0.255:1; 0.0.1.0.0.255:2\" Get selected object(s) with given attribute index.\r\n");
	printf("Example:\r\n");
	printf("Read LG device using TCP/IP connection.\r\n");
	printf("GuruxDlmsSample -r SN -c 16 -s 1 -h [Meter IP Address] -p [Meter Port No]\r\n");
	printf("Read LG device using serial port connection.\r\n");
	printf("GuruxDlmsSample -r SN -c 16 -s 1 -sp COM1 -i\r\n");
	printf("Read Indian device using serial port connection.\r\n");
	printf("GuruxDlmsSample -S COM1 -c 16 -s 1 -a Low -P [password]\r\n");
}

#if defined(_WIN32) || defined(_WIN64)//Windows
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	{
#if defined(_WIN32) || defined(_WIN64)//Windows
		//Use user locale settings. This is causing errors in some Linux distros. Example Fedora
		std::locale::global(std::locale(""));
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			// Tell the user that we could not find a usable WinSock DLL.
			return 1;
		}
#endif
		int ret, ret2, scenario, timeSchedule;
		//Remove trace file if exists.
		remove("trace.txt");
		remove("LogFile.txt");
		DLMS_SERVICE_TYPE UDP = DLMS_SERVICE_TYPE_TCP;
		GX_TRACE_LEVEL trace = GX_TRACE_LEVEL_INFO;
		bool useLogicalNameReferencing = true;
		int clientAddress = 16, serverAddress = 1;
		DLMS_AUTHENTICATION authentication = DLMS_AUTHENTICATION_NONE;
		DLMS_INTERFACE_TYPE interfaceType = DLMS_INTERFACE_TYPE_WRAPPER;
		char* password = NULL;
		//char* password = "heslo";
		char* p, * p2, * readObjects = NULL, * obisCodes = NULL, * obisCodes2 = NULL;
		int index, a, b, c, d, e, f;
		int opt = 0;
		int port = 4060;
		char* address = "localhost";
		char* serialPort = NULL;
		bool iec = false;
		while ((opt = getopt(argc, argv, "h:p:c:s:r:it:a:wP:g:S:n:gn:")) != -1)
		{
			switch (opt)
			{
			case 'w':
				interfaceType = DLMS_INTERFACE_TYPE_WRAPPER;
				break;
			case 'r':
				if (strcmp("sn", optarg) == 0)
				{
					useLogicalNameReferencing = false;
				}
				else if (strcmp("ln", optarg) == 0)
				{
					useLogicalNameReferencing = 1;
				}
				else
				{
					printf("Invalid reference option.\n");
					return 1;
				}
				break;
			case 'h':
				//Host address.
				address = optarg;
				break;
			case 't':
				//Trace.
				if (strcmp("Error", optarg) == 0)
					trace = GX_TRACE_LEVEL_ERROR;
				else  if (strcmp("Warning", optarg) == 0)
					trace = GX_TRACE_LEVEL_WARNING;
				else  if (strcmp("Info", optarg) == 0)
					trace = GX_TRACE_LEVEL_INFO;
				else  if (strcmp("Verbose", optarg) == 0)
					trace = GX_TRACE_LEVEL_VERBOSE;
				else  if (strcmp("Off", optarg) == 0)
					trace = GX_TRACE_LEVEL_OFF;
				else
				{
					printf("Invalid trace option '%s'. (Error, Warning, Info, Verbose, Off)", optarg);
					return 1;
				}
				break;
			case 'p':
				//Port.
				//port = atoi(optarg);
				break;
			case 'P':
				password = optarg;
				break;
			case 'i':
				//IEC.
				iec = 1;
				break;
			case 'g':
				//Get (read) selected objects.
				p = optarg;
				do
				{
					if (p != optarg)
					{
						++p;
					}
#if defined(_WIN32) || defined(_WIN64)//Windows
					if ((ret = sscanf_s(p, "%d.%d.%d.%d.%d.%d:%d", &a, &b, &c, &d, &e, &f, &index)) != 7)
#else
					if ((ret = sscanf(p, "%d.%d.%d.%d.%d.%d:%d", &a, &b, &c, &d, &e, &f, &index)) != 7)
#endif
					{
						ShowHelp();
						return 1;
					}
				} while ((p = strchr(p, ',')) != NULL);
				readObjects = optarg;
				break;
			case 'S':
				serialPort = optarg;
				break;
			case 'a':
				if (strcmp("None", optarg) == 0)
				{
					authentication = DLMS_AUTHENTICATION_NONE;
				}
				else if (strcmp("Low", optarg) == 0)
				{
					authentication = DLMS_AUTHENTICATION_LOW;
				}
				else if (strcmp("High", optarg) == 0)
				{
					authentication = DLMS_AUTHENTICATION_HIGH;
				}
				else if (strcmp("HighMd5", optarg) == 0)
				{
					authentication = DLMS_AUTHENTICATION_HIGH_MD5;
				}
				else if (strcmp("HighSha1", optarg) == 0)
				{
					authentication = DLMS_AUTHENTICATION_HIGH_SHA1;
				}
				else if (strcmp("HighGmac", optarg) == 0)
				{
					authentication = DLMS_AUTHENTICATION_HIGH_GMAC;
				}
				else if (strcmp("HighSha256", optarg) == 0)
				{
					authentication = DLMS_AUTHENTICATION_HIGH_SHA256;
				}
				else
				{
					printf("Invalid Authentication option. (None, Low, High, HighMd5, HighSha1, HighGmac, HighSha256)\n");
					return 1;
				}
				break;
			case 'o':
				break;
			case 'c':
				clientAddress = atoi(optarg);
				break;
			case 's':
				serverAddress = atoi(optarg);
				break;
			case 'n':
				serverAddress = CGXDLMSClient::GetServerAddress(atoi(optarg));
				break;
			case '?':
			{
				if (optarg[0] == 'c') {
					printf("Missing mandatory client option.\n");
				}
				else if (optarg[0] == 's') {
					printf("Missing mandatory server option.\n");
				}
				else if (optarg[0] == 'h') {
					printf("Missing mandatory host name option.\n");
				}
				else if (optarg[0] == 'p') {
					printf("Missing mandatory port option.\n");
				}
				else if (optarg[0] == 'r') {
					printf("Missing mandatory reference option.\n");
				}
				else if (optarg[0] == 'a') {
					printf("Missing mandatory authentication option.\n");
				}
				else if (optarg[0] == 'S') {
					printf("Missing mandatory Serial port option.\n");
				}
				else if (optarg[0] == 'g') {
					printf("Missing mandatory OBIS code option.\n");
				}
				else
				{
					ShowHelp();
					return 1;
				}
			}
			break;
			default:
				ShowHelp();
				return 1;
			}
		}

		//--------------------------------------------------ProgramStarts----------------------------------------
		int numberOfServers, testDuration = 60;
		int i, lost = 0, spojeniCelkem = 0, chybaBehemCteni = 0, spatneSpojeni = 0;
		float chybovost = 100, OBIScelkem = 0, OBISuspech = 0, OBISscenar = 0, OBISNeuspech = 0;
		std::string OBIS;
		FILE* file;
		FILE* logFile;
		char buf[0x100];
		CGXByteBuffer tmp;
		std::string newOBISCode;


		printf("\nZadej pocet serveru:\n");
		scanf_s("%d", &numberOfServers);

		numberOfServers = numberOfServers + port;

		printf("Zadej delku trvani testu (minuty)\n");
		scanf_s("%d", &testDuration);

		printf("Zadej rozmezi zasilani dotazu (sekundy)\n");
		scanf_s("%d", &timeSchedule);

		printf("Zadej scenar\n\n");
		printf("1: scenar pro cteni jednoho OBIS kodu (1)\n");
		printf("2: scenar pro cteni jednoho OBIS kodu vice hodnot (1)\n");
		printf("3: scenar pro cteni OBIS kodu napeti (10) \n");
		printf("4: scenar pro nacteni 64 OBIS kodu (64)\n");
		printf("5: precteni vsech obis kodu\n\n");
		
		do {
			scanf_s("%d", &scenario);
			if (scenario < 1 || scenario > 5)
			{
				printf("Neplatna volba.\n");
			}
		} while (scenario < 1 || scenario > 5);

		time_t start = time(0);
		std::cout << asctime(localtime(&start));

		
		//scenario pro vyber OBIS kodu
		//if (scenario != 5) {

		switch (scenario) {

			case 1: obisCodes = "1.0.1.8.0.255:2";
				OBISscenar = 1;
				break;

			case 2: newOBISCode = "0.0.1.0.0.255"; //"0.0.25.0.0.255";
				OBISscenar = 1;
				break;

			case 3:	obisCodes = "1.0.1.8.0.255:2,1.0.1.8.1.255:2,1.0.1.8.2.255:2,1.0.1.8.3.255:2,1.0.1.8.4.255:2,1.0.2.8.0.255:2,1.0.2.8.1.255:2,1.0.2.8.2.255:2,1.0.2.8.3.255:2,1.0.2.8.4.255:2";
				OBISscenar = 10;
				break;

			case 4: obisCodes = "0.0.96.1.1.255:2,0.0.96.13.0.255:2,0.0.96.13.1.255:2,0.0.96.3.10.255:2,0.1.96.3.10.255:2,0.2.96.3.10.255:2,0.0.96.14.0.255:2,0.0.96.3.10.255:2,1.0.1.7.0.255:2,1.0.2.7.0.255:2,1.0.1.8.0.255:2,1.0.1.8.1.255:2,1.0.1.8.2.255:2,1.0.1.8.3.255:2,1.0.1.8.4.255:2,1.0.2.8.0.255:2,1.0.2.8.1.255:2,1.0.2.8.2.255:2,1.0.2.8.3.255:2,1.0.2.8.4.255:2,1.0.1.8.139.255:2,1.0.3.8.0.255:2,1.0.3.8.1.255:2,1.0.3.8.2.255:2,1.0.3.8.3.255:2,1.0.3.8.4.255:2,1.0.4.8.0.255:2,1.0.4.8.1.255:2,1.0.4.8.2.255:2,1.0.4.8.3.255:2,1.0.4.8.0.255:2,1.0.1.7.0.255:2,1.0.21.7.0.255:2,1.0.41.7.0.255:2,1.0.61.7.0.255:2,1.0.2.7.0.255:2,1.0.22.7.0.255:2,1.0.42.7.0.255:2,1.0.62.7.0.255:2,0.0.0.2.2.255:2,0.0.13.0.0.255:2,0.0.13.0.1.255:2,1.0.1.6.0.255:2,1.0.2.6.0.255:2,1.0.8.8.0.255:2,1.0.7.8.0.255:2,1.0.6.8.0.255:2,1.0.5.8.0.255:2,1.0.1.7.0.130:2,1.0.1.7.0.133:2,1.0.1.7.0.166:2,1.0.1.7.0.200:2,1.0.1.7.0.131:2,1.0.1.7.0.134:2,1.0.1.7.0.167:2,1.0.1.7.0.201:2,1.0.2.7.0.130:2,1.0.2.7.0.133:2,1.0.2.7.0.166:2,1.0.2.7.0.200:2,1.0.2.7.0.131:2,1.0.2.7.0.134:2,1.0.2.7.0.167:2,1.0.2.7.0.201:2";
				OBISscenar = 64;
				break;
		}
		if ((scenario != 2) && (scenario != 5)) {
			//Get (read) selected objects.
			p = obisCodes;
			do
			{
				if (p != obisCodes)
				{
					++p;
				}
#if defined(_WIN32) || defined(_WIN64)//Windows
				if ((ret = sscanf_s(p, "%d.%d.%d.%d.%d.%d:%d", &a, &b, &c, &d, &e, &f, &index)) != 7)
#else
					if ((ret = sscanf(p, "%d.%d.%d.%d.%d.%d:%d", &a, &b, &c, &d, &e, &f, &index)) != 7)
#endif
				{
					//ShowHelp();
					//return 1;
				}
			} while ((p = strchr(p, ',')) != NULL);
			readObjects = obisCodes;
		}

		logFile = fopen("SuperLog.txt", "a");
		if (logFile == NULL)
		{
			printf("\nSoubor se nepovedlo otevrit\n");
			getchar();
			exit(1);
		}
			//smycka pro jedno spojeni
		for (;;) 
		{

				//smycka pro pripojeni k nekolika serverum
				for (port = 4060; port < numberOfServers; port++) {
					//vytvoreni souboru pro logovani
					snprintf(buf, sizeof(buf), "dataExport%d.txt", port);
					file = fopen(buf, "a");
					if (file == NULL)
					{
						printf("\nSoubor se nepovedlo otevrit\n");
						getchar();
						exit(1);
					}
					spatneSpojeni = 0;
					chybaBehemCteni = 0;
					spojeniCelkem++;
					printf("\n\n");
					std::time_t result = std::time(nullptr);
					//cas zahajeni prenosu
					printf("\nZahajeni prenosu spojeni.\n");
					std::cout << std::asctime(std::localtime(&result));
					printf("_______________________________________________________");
					printf("\n");

					printf("Port: %d", port);

					printf("\n");
					printf("_______________________________________________________");
					printf("\n");
					fprintf(file, "\nZahajeni prenosu: %s", std::asctime(std::localtime(&result)));
					fprintf(logFile, "\nZahajeni prenosu: %s", std::asctime(std::localtime(&result)));
					fprintf(file, "Port: %d\n", port);
					fprintf(logFile, "Port: %d\n", port);
					CGXDLMSSecureClient cl(useLogicalNameReferencing, clientAddress, serverAddress, authentication, password, interfaceType);
					//Sifrovani prenosu
					/*
					cl.GetCiphering()->SetSecurity(DLMS_SECURITY_ENCRYPTION);
					//std::string hex = "142434445464748";
					std::string hex = "4D4D4D0000BC614E";
					GXHelpers::HexToBytes(hex , tmp);
					cl.GetCiphering()->SetSystemTitle(tmp);
					hex = "000102030405060708090A0B0C0D0E0F";
					GXHelpers::HexToBytes(hex, tmp);
					cl.GetCiphering()->SetBlockCipherKey(tmp);
					hex = "D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF";
					GXHelpers::HexToBytes(hex, tmp);
					cl.GetCiphering()->SetAuthenticationKey(tmp);
					*/
					CGXCommunication comm(&cl, 5000, trace);




					if (port != 0 || address != NULL)
					{
						if (port == 0)
						{
							printf("Missing mandatory port option.\n");
							return 1;
						}
						if (address == NULL)
						{
							printf("Missing mandatory host option.\n");
							return 1;
						}

						if ((ret = comm.Connect(address, port)) != 0)									//Pripojeni
						{
							printf("Connect failed %s.\r\n", CGXDLMSConverter::GetErrorMessage(ret));
							printf("Nepodarilo se navazat spojeni.\r\n");
							lost++;
							spatneSpojeni = 1;
							fprintf(file, "Nepodarilo se navazat spojeni\n");
							fprintf(logFile, "Nepodarilo se navazat spojeni\n");
							//return 1;
							goto KONEC;
						}
					}
					else
					{
						printf("Missing mandatory connection information for TCP/IP or serial port connection.\n");
						return 1;
					}
					OBIScelkem = OBIScelkem + OBISscenar;
					//Vypis vsech obis kodu
					if (scenario == 5) {
						ret = comm.ReadAll();
						if (ret != 0) {
							printf("Chyba behem cteni.\r\n");
							fprintf(file, "Chyba behem cteni: %s\n", CGXDLMSConverter::GetErrorMessage(ret));
							fprintf(logFile, "Chyba behem cteni: %s\n", CGXDLMSConverter::GetErrorMessage(ret));
							lost++;
						}
						else {
							fprintf(file, "Uspesne precteny vsechny OBIS kody\n");
							fprintf(logFile, "Uspesne precteny vsechny OBIS kody\n");
						}
					}
					else
					{

						if ((ret = comm.InitializeConnection()) == 0 && (ret = comm.GetAssociationView()) == 0)
						{
							if (scenario == 2) {
								CGXDLMSObject* obj = cl.GetObjects().FindByLN(DLMS_OBJECT_TYPE_ALL, newOBISCode);
								ret = comm.GetObjectWithoutIndex(obj);
								OBISuspech++;
							}
							else {
								std::string str;
								std::string value;
								char buff[200];
								p = readObjects;
								do
								{
									if (p != readObjects)
									{
										++p;
									}
									str.clear();
									p2 = strchr(p, ':');
									++p2;
#if defined(_WIN32) || defined(_WIN64)//Windows
									sscanf_s(p2, "%d", &index);
#else
									sscanf(p2, "%d", &index);
#endif
									str.append(p, p2 - p);
									OBIS = str;
									std::cout << "\n\nOBIS kod: " << OBIS << "\n";
									fprintf(file, "OBIS kod: %s\n", OBIS.c_str());
									fprintf(logFile, "OBIS kod: %s\n", OBIS.c_str());
									CGXDLMSObject* obj = cl.GetObjects().FindByLN(DLMS_OBJECT_TYPE_ALL, str);
									value.clear();
									if (obj == NULL)
									{
										printf("OBIS neexistuje");
										fprintf(file, "OBIS neexistuje\n");
										fprintf(logFile, "OBIS neexistuje\n");
									}
									else {
										if ((ret = comm.Read(obj, index, value)) != DLMS_ERROR_CODE_OK)
										{
#if _MSC_VER > 1000				

											sprintf_s(buff, 100, "Error! Index: %d %s\r\n", index, CGXDLMSConverter::GetErrorMessage(ret));
											fprintf(file, "Error! Index: %d %s\n", index, CGXDLMSConverter::GetErrorMessage(ret));
											fprintf(logFile, "Error! Index: %d %s\n", index, CGXDLMSConverter::GetErrorMessage(ret));
#else
											sprintf(buff, "Error! Index: %d read failed: %s\r\n", index, CGXDLMSConverter::GetErrorMessage(ret));
#endif
											comm.WriteValue(GX_TRACE_LEVEL_ERROR, buff);
											chybaBehemCteni = 1;
											//Continue reading.

										}
										else
										{
#if _MSC_VER > 1000
											//vypis hodnot obis kodu
											fprintf(file, "Index: %d Value: %s\n", index, value.c_str());
											fprintf(logFile, "Index: %d Value: %s\n", index, value.c_str());
											sprintf_s(buff, 100, "Index: %d Value: ", index);
#else
											sprintf(buff, "Index: %d Value: ", index);
#endif
											comm.WriteValue(trace, buff);
											comm.WriteValue(trace, value.c_str());
											comm.WriteValue(trace, "\r\n");
											OBISuspech++;

										}
									}

								} while ((p = strchr(p, ',')) != NULL);
							}
						}
						else {
							if (spatneSpojeni == 0) {	//pokud dojde k chybe po pripojeni a pred GetAsscociationView

								printf("Chyba behem cteni.\r\n");
								fprintf(file, "Chyba behem cteni]\n");
								fprintf(logFile, "Chyba behem cteni\n");
								lost++;
							}
						}
					if (chybaBehemCteni != 0 && spatneSpojeni == 0) {
						printf("Chyba behem cteni.\r\n");
						fprintf(file, "Chyba behem cteni]\n");
						fprintf(logFile, "Chyba behem cteni\n");
						lost++;
					}

					}
					//Close connection.
					comm.Close();
					KONEC:
					time_t stop = time(0);
					printf("\nUkonceni prenosu.\n");
					std::cout << asctime(localtime(&stop));
					fprintf(file, "Ukonceni prenosu: %s\n", std::asctime(localtime(&stop)));
					fprintf(logFile, "Ukonceni prenosu: %s\n", std::asctime(localtime(&stop)));
					//Close file.
					fclose(file);
				}

				//std::cout << asctime(localtime(&start));
				//std::cout << difftime(stop, start);
				time_t stop = time(0);
				if (difftime(stop, start) > testDuration)
					break;
				Sleep(timeSchedule * 1000);

				//konec smycky pro spojeni
		}

		OBISNeuspech = OBIScelkem - OBISuspech;
		if (OBIScelkem != 0) {

			chybovost = (1 - (OBISuspech / OBIScelkem))*100;
		}
		
		fprintf(logFile, "Celkovy pocet spojeni: %d\n", spojeniCelkem);
		fprintf(logFile, "Predpokladany pocet OBIS kodu: %.0f\n", OBIScelkem);
		fprintf(logFile, "Pocet prijatych OBIS kodu: %0.f\n", OBISuspech);
		fprintf(logFile, "Uspesnost %0.f/%0.f\n", OBIScelkem, OBISuspech);
		fprintf(logFile, "Chybovost: %.3f\n", chybovost);
		fprintf(logFile, "Celkovy pocet neuspesnych navazani spojeni se serverem: %d\n\n", lost);

		fclose(logFile);
		
		printf("\n\nCelkovy pocet spojeni: %d\n", spojeniCelkem);
		printf("Predpokladany pocet OBIS kodu: %.0f\n", OBIScelkem);
		printf("Pocet prijatych OBIS kodu: %0.f\n", OBISuspech);
		printf("Uspesnost %0.f/%0.f\n",OBIScelkem, OBISuspech);
		printf("Chybovost: %.3f\n", chybovost);
		printf("Celkovy pocet neuspesnych navazani spojeni se serverem: %d\n\n", lost);

	}
#if defined(_WIN32) || defined(_WIN64)//Windows
	WSACleanup();
#if _MSC_VER > 1400
	_CrtDumpMemoryLeaks();
#endif
#endif
	return 0;
}

