#include "processprotected.h"
#include "hook.h"

#pragma  INITCODE
JMPCODE		oldCode;//��������ǰ5�ֽ� �Ա�ָ�
PJMPCODE	pcur;
BOOL		ishook = FALSE;

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) //TYPEDEF LONG NTSTATUS
{
	ULONG	cur, old;
	JMPCODE JmpCode;

	cur = GetNt_CurAddr();//A
	old = GetNt_OldAddr();//C
	if (cur != old)
	{   //hook
		ishook = TRUE;
		//����ǰ5�ֽ�
		pcur = (PJMPCODE)(cur);//��ʼ��ָ��
		oldCode.E9 = pcur->E9;//1�ֽ�
		oldCode.JMPADDR = pcur->JMPADDR;//4�ֽ�
		//

		JmpCode.E9 = 0xE9;
		JmpCode.JMPADDR = old - cur - 5;
		KdPrint(("Ҫд��ĵ�ַ%X", JmpCode.JMPADDR));
		//д��JMP   C-A-5=B //ʵ��Ҫд���ַ
		__asm //ȥ��ҳ�汣��
		{
			cli
			mov eax, cr0
			and eax, not 10000h //and eax,0FFFEFFFFh
			mov cr0, eax

		}

		pcur->E9 = 0xE9;//jmp
		pcur->JMPADDR = JmpCode.JMPADDR;//Ҫ��ת���ĵ�ַ

		__asm //�ָ�ҳ����
		{
			mov eax, cr0
			or eax, 10000h //or eax,not 0FFFEFFFFh
			mov cr0, eax
			sti
		}

		KdPrint(("NtOpenProcess��HOOK��"));
	}

	//ע����ǲ����
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE���IRP������
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE���IRP������
	pDriverObject->MajorFunction[IRP_MJ_READ] = ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE���IRP������
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE���IRP������
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE���IRP������
	CreateMyDevice(pDriverObject);//������Ӧ���豸
	pDriverObject->DriverUnload = DriverUnload;
	return (1);
}

#pragma PAGECODE
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pDev;//����ȡ��Ҫɾ���豸����
	UNICODE_STRING symLinkName; // 

	UnHook();

	if (ishook)
	{//unhook
		__asm //ȥ��ҳ�汣��
		{
			cli
			mov eax, cr0
			and eax, not 10000h //and eax,0FFFEFFFFh
			mov cr0, eax

		}

		pcur->E9 = oldCode.E9;//1�ֽ�
		pcur->JMPADDR = oldCode.JMPADDR;//4�ֽ�

		__asm //�ָ�ҳ����
		{
			mov eax, cr0
			or eax, 10000h //or eax,not 0FFFEFFFFh
			mov cr0, eax
			sti
		}
	} //end unhook

	//ȡ������������
	//ɾ����������
	RtlInitUnicodeString(&symLinkName, MY_SYMLINKNAME);
	IoDeleteSymbolicLink(&symLinkName);
	//ɾ���豸
	pDev = pDriverObject->DeviceObject;
	IoDeleteDevice(pDev); 

	KdPrint(("�����ɹ���ж��...OK-----------"));
}

#pragma PAGECODE
NTSTATUS ddk_DispatchRoutine_CONTROL(IN PDEVICE_OBJECT pDevobj, IN PIRP pIrp)
{
	ULONG info;
	//�õ���ǰջָ��
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG mf = stack->MajorFunction;//����IRP
	switch (mf)
	{
		case IRP_MJ_DEVICE_CONTROL:
		{
			KdPrint(("Enter myDriver_DeviceIOControl\n"));

			//�õ�IOCTL��
			ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
			switch (code)
			{
				case add_code:
				{
					int a, b;
					KdPrint(("add_code 1111111111111111111\n"));
					//��������ʽIOCTL
					//��ȡ���������� a,b		
					int * InputBuffer = (int*)pIrp->AssociatedIrp.SystemBuffer;
					_asm
					{
						mov eax, InputBuffer
						mov ebx, [eax]
						mov a, ebx
						mov ebx, [eax + 4]
						mov b, ebx
					}
					KdPrint(("a=%d,b=%d \n", a, b));

					a = a + b;
					//C�������㷵���������û���
					//�������������
					int* OutputBuffer = (int*)pIrp->AssociatedIrp.SystemBuffer;
					_asm
					{
						mov eax, a
						mov ebx, OutputBuffer
						mov[ebx], eax //bufferet=a+b
					}
					KdPrint(("a+b=%d \n", a));

					//����ʵ�ʲ����������������
					info = 4;
					break;
				}
				case hook_code:
				{  
					//��buffer��ȡMyPid 
					//��ȡ���������� a,b		
					int * InputBuffer = (int*)pIrp->AssociatedIrp.SystemBuffer;
					_asm
					{
						mov eax, InputBuffer
						mov ebx, [eax]
						mov MyPID, ebx
					}

					int* OutputBuffer = (int*)pIrp->AssociatedIrp.SystemBuffer;
					_asm
					{
						mov eax, 1
						mov ebx, OutputBuffer
						mov[ebx], eax //
					}
					info = 4;
					Hook();
					break;
				}
				case unhook_code:
				{
					UnHook();
					break;
				}
				case sub_code:
				{
					break;
				}
			}//end code switch
			break;
		}
		case IRP_MJ_CREATE:
		{
			break;
		}
		case IRP_MJ_CLOSE:
		{
			break;
		}
		case IRP_MJ_READ:
		{
			break;
		}
	}

	//����Ӧ��IPR���д���
	pIrp->IoStatus.Information	= info;//���ò������ֽ���Ϊ0��������ʵ������
	pIrp->IoStatus.Status		= STATUS_SUCCESS;//���سɹ�

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);//ָʾ��ɴ�IRP
	KdPrint(("�뿪��ǲ����\n"));//������Ϣ
	return STATUS_SUCCESS; //���سɹ�
}