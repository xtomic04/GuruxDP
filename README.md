# Server

## Windows

Po zkompilování projektu **GuruxDLMSServerExample** vznikne ve složce **GuruxDLMSServerExample\VS\Debug** soubor **GuruxDLMSServerExample.exe,** který je spustitelný. V té samé složce je nutné vytvořit adresář **logs**, kde se budou ukládat logy. Při spuštění tohoto souboru, server běží ve výchozím nastavení na portu 4060. Server lze spustit i z příkazového řádku se vstupním parametrem **-p,** kterým se nastavuje číslo portu, na kterém server poběží. Pro spuštění serveru z příkazového řádku je nutné vytvořit ze souboru **GuruxDLMSServerExample.exe** zástupce a ten následně spustit. Pro spuštění více serverů je možné použít dávkový soubor (.bat)

**serverLoop.bat**
 
 \_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_

 FOR /L %%i IN (4060,1,4061) DO (

 start GuruxDLMSServerExample.exe.lnk -p %%i
 
 timeout 1
 
 )

 \_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_

## RaspberryPi (linux)

Ve složce **Development** je třeba vytvořit složky **bin** a **obj.** Následně ve složce **Development** příkazem **make** provézt kompilaci. Dále ve složce **GuruxDLMSServerExample** je třeba vytvořit složky **bin** a **obj** a příkazem **make** se provede kompilace projektu. Ve složce **bin** se vytvoří soubor **gurux.dlms.server.bin**. V té samé složce je nutné vytvořit adresář **logs**, kde se budou ukládat logy. Ten lze spustit také se vstupním parametrem **-p.** Příklad spuštění serveru: **./gurux.dlms.server.bin -p 4060**.

Pro spuštění více serveru lze použít dávkové soubory (.bat)

**serverLoopPi.bat**

\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_

 FOR /L %%i IN (4060,1,4061) DO (
 
 start /B pripojeni.bat %%i
 
 timeout 1
 
 )

 \_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_

**pripojeni.bat**

\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_

plink -batch -ssh pi@192.168.10.208 -pw raspberry cd /home/guruxServer/GURUX/GuruxDLMSServerExample/bin/; ./gurux.dlms.server.bin -p %1

 \_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_\_

Soubor **serverLoopPi.bat** je spustitelný. V souboru **pripojeni.bat** je třeba nastavit ip adresu RaspberryPi, dále uživatelské jméno a heslo (výchozí nastavení: pi/raspberry). Pro využití těchto dávkových soubor, je nutno mít na PC program **plink** (který je součástí instalace programu **Putty** ).

# Klient

Po kompilaci projektu **GuruxDLMSClientExample** vznikne ve složce **GuruxDLMSClientExample\VS\Debug** soubor **GuruxDLMSClientExample.exe,** který je spustitelný z příkazového řádku. V té samé složce je nutné vytvořit adresář **logs**, kde se budou ukládat logy. Povinným vstupním parametrem ( **-h** )je ip adresa serveru. Příklad spuštění: **GuruxDLMSClientExample.exe -h 192.168.10.208**

Klient podporuje další vstupní parametry pro nastavení testování:

N – nastavení počtu serverů, na které se bude klient dotazovat

D – nastavení délky testování (v minutách)

T – časové rozmezí mezi dotazováním na servery (v sekundách)
 
C – výběr scénáře (1-5)


Pro práci s klientem a serverem na platformě windows je nutné zvolit vyvojové prostředí pro kompilaci zdrojového kódu. Jednou z možností je využiti vyvojového prostředí Microsoft Visual Studio.


