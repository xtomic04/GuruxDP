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

Simple example
=========================== 
Before use you must set following device parameters. 
Parameters are manufacturer spesific.


```C++

All default parameters are given in constructor.
// Is used Logican Name or Short Name referencing.
CGXDLMSClient client(true);

```
