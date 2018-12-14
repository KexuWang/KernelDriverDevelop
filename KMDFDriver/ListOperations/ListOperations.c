#include "ListOperations.h"


#define DEVICENAME L"\\Device\\MyListOperationsDevice"
#define	SYM_LINK_NAME L"\\??\\MyListOperationsSymLink"


typedef struct _MYDATASTRUCT
{
	ULONG number;
	LIST_ENTRY ListEntry;
} MYDATASTRUCT, *PMYDATASTRUCT;

#pragma INITCODE
void LinkListTest()
{
	LIST_ENTRY linkListHead;
	InitializeListHead(&linkListHead);

	PMYDATASTRUCT pData;
	ULONG i = 0;

	//在链表中插入10个元素
	KdPrint(("Begin insert to link list"));
	for (i = 0; i < 10; i++)
	{
		pData = (PMYDATASTRUCT)ExAllocatePoolWithTag(PagedPool, sizeof(MYDATASTRUCT), MEM_TAG);
		pData->number = i;
		InsertHeadList(&linkListHead, &pData->ListEntry);
	}

	//从链表中取出，并显示                                                  
	KdPrint(("Begin remove from link list\n"));
	while (!IsListEmpty(&linkListHead))
	{
		PLIST_ENTRY pEntry = RemoveTailList(&linkListHead);
		pData = CONTAINING_RECORD(pEntry,
			MYDATASTRUCT,
			ListEntry);
		KdPrint(("%d\n", pData->number));
		ExFreePool(pData);

	}
}


#pragma PAGEDCODE
void DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("Entry DriverUnload\n"));
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, SYM_LINK_NAME);
	IoDeleteSymbolicLink(&symLinkName);
	IoDeleteDevice(pDriverObject->DeviceObject);
}

#pragma INITCODE
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("Enter DriverEntry\n"));

	//注册分发函数
	pDriverObject->DriverUnload = DriverUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = MyDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]	= MyDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE]	= MyDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ]	= MyDispatchRoutine;

	//创建驱动设备对象
	status = CreateDevice(pDriverObject);

	LinkListTest();
	KdPrint(("DriverEntry end\n"));
	return status;
}


#pragma INITCODE
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("Entry CreateDevice\n"));
	//创建设备
	PDEVICE_OBJECT pDevObj;
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, DEVICENAME);
	status = IoCreateDevice(pDriverObject,0,&devName,FILE_DEVICE_UNKNOWN,0, TRUE, &pDevObj);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Error in CreateDevice\n"));
		return status;
	}
	pDevObj->Flags |= DO_BUFFERED_IO;

	//创建符号链接
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, SYM_LINK_NAME);
	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
	}
	return status;

}

#pragma PAGEDCODE
NTSTATUS MyDispatchRoutine(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("Entry MyDispatchRoutine\n"));
	
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;//bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT); // 告诉IO管理器，调用方已经完成IO请求
	KdPrint(("Leave MyDispatchRotine\n"));
	return status;
}





































