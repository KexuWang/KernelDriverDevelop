#pragma once
#include <NTDDK.h>
#include "ctl_code.h"
#include <windef.h>

#define MY_DEVICENAME	L"\\Device\\ProcessProtected_Device"
#define MY_SYMLINKNAME	L"\\??\\MyLinkForProcessProtectedDevice"
#define INITCODE code_seg("INIT")	
#define PAGECODE code_seg("PAGE")

typedef struct _ServiceDescriptorTable {
	PVOID ServiceTableBase;			//System Service Dispatch Table �Ļ���ַ  
	PVOID ServiceCounterTable;		//������ SSDT ��ÿ�����񱻵��ô����ļ����������������һ����sysenter ���¡� 
	unsigned int NumberOfServices;	//�� ServiceTableBase �����ķ������Ŀ��  
	PVOID ParamTableBase;			//����ÿ��ϵͳ��������ֽ�����Ļ���ַ-ϵͳ��������� 
}*PServiceDescriptorTable;

extern PServiceDescriptorTable KeServiceDescriptorTable;

#pragma pack(1)
typedef struct _JMPCODE
{
	BYTE E9;
	ULONG JMPADDR;//+8 88881234=B
}JMPCODE, *PJMPCODE;
#pragma pack( )

VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject);								//ǰ��˵�� ж������
NTSTATUS ddk_DispatchRoutine_CONTROL(IN PDEVICE_OBJECT pDevobj, IN PIRP pIrp);	//��ǲ����


#pragma INITCODE 
NTSTATUS CreateMyDevice(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;/*�������ش����豸*/

	//�����豸����
	UNICODE_STRING devName;
	UNICODE_STRING symLinkName; // 
	RtlInitUnicodeString(&devName, MY_DEVICENAME);

	//�����豸
	status = IoCreateDevice(pDriverObject,
							0,
							&devName,
							FILE_DEVICE_UNKNOWN,
							0, TRUE,
							&pDevObj);
	if (!NT_SUCCESS(status))
	{
		if (status == STATUS_INSUFFICIENT_RESOURCES)
		{
			KdPrint(("��Դ���� STATUS_INSUFFICIENT_RESOURCES"));
		}
		if (status == STATUS_OBJECT_NAME_EXISTS)
		{
			KdPrint(("ָ������������"));
		}
		if (status == STATUS_OBJECT_NAME_COLLISION)
		{
			KdPrint(("//�������г�ͻ"));
		}
		KdPrint(("�豸����ʧ��...++++++++"));
		return status;
	}
	KdPrint(("�豸�����ɹ�...++++++++"));

	pDevObj->Flags |= DO_BUFFERED_IO;

	//������������
	RtlInitUnicodeString(&symLinkName, MY_SYMLINKNAME);
	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status)) /*status����0*/
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	return STATUS_SUCCESS;
}




ULONG GetNt_CurAddr() //��ȡ��ǰSSDT_NtOpenProcess�ĵ�ǰ��ַ
{
	LONG *SSDT_Adr, SSDT_NtOpenProcess_Cur_Addr, t_addr;
	KdPrint(("�����ɹ���������.............................\n"));
	//��ȡSSDT��������ֵΪ0xbe�ĺ���
	//poi(poi(KeServiceDescriptorTable)+0xbe*4)
	t_addr = (LONG)KeServiceDescriptorTable->ServiceTableBase;
	KdPrint(("��ǰServiceTableBase��ַΪ%x \n", t_addr));
	SSDT_Adr = (PLONG)(t_addr + 0xbe * 4);
	KdPrint(("��ǰt_addr+0xbe*4=%x \n", SSDT_Adr));
	SSDT_NtOpenProcess_Cur_Addr = *SSDT_Adr;
	KdPrint(("��ǰSSDT_NtOpenProcess_Cur_Addr��ַΪ%x \n", SSDT_NtOpenProcess_Cur_Addr));
	return SSDT_NtOpenProcess_Cur_Addr;
}

ULONG GetNt_OldAddr()
{
	UNICODE_STRING Old_NtOpenProcess;
	ULONG Old_Addr;
	RtlInitUnicodeString(&Old_NtOpenProcess, L"NtOpenProcess");
	Old_Addr = (ULONG)MmGetSystemRoutineAddress(&Old_NtOpenProcess);//ȡ��NtOpenProcess�ĵ�ַ
	KdPrint(("ȡ��ԭ����NtOpenProcess�ĵ�ַΪ %x", Old_Addr));
	return Old_Addr;

}