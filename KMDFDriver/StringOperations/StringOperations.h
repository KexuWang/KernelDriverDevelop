#pragma once
#include <NTDDK.h>

#define PAGEDCODE	code_seg("PAGE")	//分页内存：Windows规定有些虚拟内存可以交换到文件中，这类内存被称为分页内存
#define LOCKEDCODE	code_seg()			//非分页内存：有些虚拟内存永远不会交换到文件中，这些内存叫非分页内存
#define INITCODE	code_seg("INIT")	//初始化完毕之后就会被释放。即某个例程需要在初始化的时候“载入”内存，然后可以从内存中“卸载”掉

#define PAGEDDATA	data_seg("PAGE")
#define LOCKEDDATA	data_seg()
#define INITDATA	data_seg("INIT")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))
#define MEM_TAG 'MyTt'