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

	//�������в���10��Ԫ��
	KdPrint(("Begin insert to link list"));
	for (i = 0; i < 10; i++)
	{
		pData = (PMYDATASTRUCT)ExAllocatePoolWithTag(PagedPool, sizeof(MYDATASTRUCT), MEM_TAG);
		pData->number = i;
		InsertHeadList(&linkListHead, &pData->ListEntry);
	}

	//��������ȡ��������ʾ                                                  
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

	//ע��ַ�����
	pDriverObject->DriverUnload = DriverUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = MyDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]	= MyDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE]	= MyDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ]	= MyDispatchRoutine;

	//���������豸����
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
	//�����豸
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

	//������������
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
	IoCompleteRequest(pIrp, IO_NO_INCREMENT); // ����IO�����������÷��Ѿ����IO����
	KdPrint(("Leave MyDispatchRotine\n"));
	return status;
}





































