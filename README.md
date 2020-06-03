See An [Gurux](http://www.gurux.org/ "Gurux") for an overview.

Join the Gurux Community or follow [@Gurux](https://twitter.com/guruxorg "@Gurux") for project updates.

GuruxDLMS.cpp library is a high-performance ANSI C++ component that helps you to read you DLMS/COSEM compatible electricity, gas or water meters. We have try to make component so easy to use that you do not need understand protocol at all.

For more info check out [Gurux.DLMS](http://www.gurux.fi/index.php?q=Gurux.DLMS "Gurux.DLMS").

We are updating documentation on Gurux web page. 

Read should read [DLMS/COSEM FAQ](http://www.gurux.org/index.php?q=DLMSCOSEMFAQ) first to get started. Read Instructions for making your own [meter reading application](http://www.gurux.org/index.php?q=DLMSIntro) or build own 
DLMS/COSEM [meter/simulator/proxy](http://www.gurux.org/index.php?q=OwnDLMSMeter).

If you have problems you can ask your questions in Gurux [Forum](http://www.gurux.org/forum).

You can use any connection (TCP, serial, PLC) library you want to.
Gurux.DLMS classes only parse the data.

Before start
=========================== 

If you find an issue, please report it here:
http://www.gurux.fi/fi/project/issues/gurux.dlms.cpp

We have made makefile for linux environment. You should go development folder and create lib and obj directories.
Then run make. gurux_dlms_cpp static library is made.

For Client example you should also create bin and obj -directories.
Change Host name, port and DLMS settings for example. Run make and you are ready to test.



Server
Windows
Po zkompilování projektu GuruxDLMSServerExample vznikne ve složce GuruxDLMSServerExample\VS\Debug soubor GuruxDLMSServerExample.exe, který je spustitelný. Při spuštění tohoto souboru, server běží ve výchozím nastavení na portu 4060. Server lze spustit i z příkazového řádku se vstupním parametrem -p, kterým se nastavuje číslo portu, na kterém server poběží. Pro spuštění serveru z příkazového řádku je nutné vytvořit ze souboru GuruxDLMSServerExample.exe zástupce a ten následně spustit. Pro spuštění více serverů je možné použít dávkový soubor (.bat)
serverLoop.bat
____________________________________________________
FOR /L %%i IN (4060,1,4061) DO (
    
	start GuruxDLMSServerExample.exe.lnk -p %%i
	timeout 1
)
____________________________________________________
RaspberryPi (linux)
Ve složce Development je třeba vytvořit složky bin a obj. Následně ve složce Development příkazem make provézt kompilaci. Dále ve složce GuruxDLMSServerExample je třeba vytvořit složky bin a obj a příkazem make se provede kompilace projektu. Ve složce bin se vytvoří soubor gurux.dlms.server.bin. Ten lze spustit také se vstupním parametrem -p. Příklad spuštění serveru: ./gurux.dlms.server.bin -p 4060. 
Pro spuštění více serveru lze použít dávkové soubory (.bat) 
serverLoopPi.bat
____________________________________________________
FOR /L %%i IN (4060,1,4061) DO (  
	start /B pripojeni.bat %%i
	timeout 1
)
____________________________________________________
pripojeni.bat
____________________________________________________
plink -batch -ssh pi@192.168.10.208 -pw raspberry cd /home/guruxServer/GURUX/GuruxDLMSServerExample/bin/; ./gurux.dlms.server.bin -p %1
____________________________________________________
Soubor serverLoopPi.bat je spustitelný. V souboru pripojeni.bat je třeba nastavit ip adresu RaspberryPi, dále uživatelské jméno a heslo (výchozí nastavení: pi/raspberry). Pro využití těchto dávkových soubor, je nutno mít na PC program plink (který je součástí instalace programu Putty).
Klient
Po kompilaci projektu GuruxDLMSClientExample vznikne ve složce GuruxDLMSClientExample\VS\Debug soubor GuruxDLMSClientExample.exe, který je spustitelný z příkazového řádku. Povinným vstupním parametrem (-h) je ip adresa serveru. Příklad spuštění: GuruxDLMSClientExample.exe -h 192.168.10.208

