#pragma once
#include <NTDDK.h>

#define PAGEDCODE	code_seg("PAGE")
#define LOCKEDCODE	code_seg()
#define INITCODE	code_seg("INIT")

#define PAGEDDATA	data_seg("PAGE")
#define LOCKEDDATA	data_seg()
#define INITDATA	data_seg("INIT")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

#define MEM_TAG 'MyTt'

//typedef struct _DEVICE_EXTENSION {
//	PDEVICE_OBJECT pDevice;
//	UNICODE_STRING ustrDeviceName;	//设备名称
//	UNICODE_STRING ustrSymLinkName;	//符号链接名
//} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// 函数声明
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject);
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject);
NTSTATUS MyDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);