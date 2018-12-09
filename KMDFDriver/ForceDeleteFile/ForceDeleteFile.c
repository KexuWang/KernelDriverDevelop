/*
�ļ�ɾ���Ĺ���
ɾ���ļ�ʱ���ȵ�NTFS.sys�ķ������� -> NtfsSetDispositionInfo -> MmFlushImageSection.
MmFlushImageSection()������������ļ������SECTION_OBJECT_POINter�ṹ��
�������ǲ���Ϊ��(Ҳ���Ǽ������ļ���û������),û�еĻ���ֱ�ӷ���TRUE��

ǿ��ɾ���ļ�
�Լ�����IRP��Ȼ����NTFS.sys����IRP���������ļ������ԣ���ɾ���ļ���
����1:
����SECTION_OBJECT_POINter�ṹ��ı�����Ϊ0������ MmFlushImageSection() Ϊ����TRUE����ʾ��ɾ����
����2:
HOOK NTFS.sys�ĵ�����е�MmFlushImageSection()��������HOOK�����м���ǲ�������Ҫɾ�����ļ����ǵĻ�ֱ�ӷ���TRUEҲ�С�
*/
#include <ntddk.h>

#define NT_DEVICE_NAME	L"\\Device\\MyForceDeleteFile"
#define DOS_DEVICE_NAME L"\\??\\MyForceDeleteFile"


VOID UnloadDriver(IN PDRIVER_OBJECT DriverObject)
{
	KdPrint(("unload driver"));
	PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
	UNICODE_STRING uniSymLink;

	RtlInitUnicodeString(&uniSymLink, DOS_DEVICE_NAME);
	IoDeleteSymbolicLink(&uniSymLink);
	IoDeleteDevice(deviceObject);
}


HANDLE SForceIoOpenFile(IN PCWSTR FileName, IN ACCESS_MASK DesiredAccess, IN ULONG ShareAccess )
{
	NTSTATUS ntStatus;
	UNICODE_STRING uniFileName;
	OBJECT_ATTRIBUTES objectAttributes;
	HANDLE ntFileHandle;
	IO_STATUS_BLOCK ioStatus;

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		return 0;
	}

	RtlInitUnicodeString(&uniFileName, FileName);

	InitializeObjectAttributes(&objectAttributes, 
							   &uniFileName,
							   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
							   NULL, NULL);

	ntStatus = IoCreateFile(&ntFileHandle,
							DesiredAccess,
							&objectAttributes,
							&ioStatus,
							0,
							FILE_ATTRIBUTE_NORMAL,
							ShareAccess,
							FILE_OPEN,
							0,
							NULL,
							0,
							0,
							NULL,
							IO_NO_PARAMETER_CHECKING);

	if (!NT_SUCCESS(ntStatus))
	{
		return 0;
	}

	return ntFileHandle;
}

//IO�������
NTSTATUS SForceSetFileCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	Irp->UserIosb->Status		= Irp->IoStatus.Status;
	Irp->UserIosb->Information	= Irp->IoStatus.Information;

	KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);

	IoFreeIrp(Irp);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

//ȥ���ļ�ֻ������
BOOLEAN SForceStripFileAttributes(IN HANDLE FileHandle)
{
	NTSTATUS ntStatus =		STATUS_SUCCESS;
	PFILE_OBJECT			fileObject;
	PDEVICE_OBJECT			DeviceObject;
	FILE_BASIC_INFORMATION	FileInformation;
	PIRP				Irp;
	KEVENT				event1;
	IO_STATUS_BLOCK		ioStatus;
	PIO_STACK_LOCATION	irpSp;

	//fileObject
	ntStatus = ObReferenceObjectByHandle(FileHandle,
										 DELETE,
										 *IoFileObjectType,
										 KernelMode,
										 &fileObject,
										 NULL);//����ļ���������ĸ����̵ľ������

	if (!NT_SUCCESS(ntStatus))
	{
		return FALSE;
	}

	//event1
	KeInitializeEvent(&event1, SynchronizationEvent, FALSE);

	//�����ļ�������
	memset(&FileInformation, 0, 0x28);
	FileInformation.FileAttributes = FILE_ATTRIBUTE_NORMAL;

	//Irp
	DeviceObject = IoGetRelatedDeviceObject(fileObject);
	Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
	if (Irp == NULL)
	{
		ObDereferenceObject(fileObject);
		return FALSE;
	}
	Irp->AssociatedIrp.SystemBuffer = &FileInformation;
	Irp->UserEvent					= &event1;
	Irp->UserIosb					= &ioStatus;
	Irp->Tail.Overlay.OriginalFileObject = fileObject;
	Irp->Tail.Overlay.Thread		= (PETHREAD)KeGetCurrentThread();
	Irp->RequestorMode				= KernelMode;

	// IrpSp I/O��ջ��Ԫ
	irpSp = IoGetNextIrpStackLocation(Irp);	//IoGetNextIrpStackLocation �����ڸ�����IRP�е�һ������ʹ�õ� I/O stack location ��ָ��
	irpSp->MajorFunction	= IRP_MJ_SET_INFORMATION;
	irpSp->DeviceObject		= DeviceObject;
	irpSp->FileObject		= fileObject;
	irpSp->Parameters.SetFile.Length				= sizeof(FILE_BASIC_INFORMATION);
	irpSp->Parameters.SetFile.FileInformationClass	= FileBasicInformation;
	irpSp->Parameters.SetFile.FileObject			= fileObject;

	//ע��һ��IO������̣����������̽����ڵ��ô˺�������������һ���������IRPָ���Ĳ�������ʱ������
	IoSetCompletionRoutine(Irp,
						   SForceSetFileCompletion,
						   &event1,
						   TRUE,
						   TRUE,
						   TRUE);

	//��������豸������������󣬲��� IO_STACK_LOCATION ��ָ����һ����Ҳ���Ǹո����õ�
	//���û���ļ�ϵͳ�����������豸����û��Attacked�Ļ����͵����ļ�ϵͳ������IRP_MJ_SET_INFORMATION��������
	IoCallDriver(DeviceObject, Irp);
					

	//�����NTFS.sys������NtfsFsdSetInformation���̣��ٻ����NtfsSetBasicInfo()����������������ô�����ļ���FCB���ļ�
	//���ƿ�ṹ��һЩ��Ϣ���������ô�����ļ������ԡ����֪������������IoCompleteRequest,�������ε�����ǰ���õĻص�����
	//�ص��������ͷŸշ����IRP�������¼�����Ϊ����״̬��
	KeWaitForSingleObject(&event1, Executive, KernelMode, TRUE, NULL);//һ�ȵ��¼�����������״̬�ͻ��������ִ�С�

	ObDereferenceObject(fileObject);

	return TRUE;
}


BOOLEAN SForceDeleteFile( IN HANDLE FileHandle )
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PFILE_OBJECT fileObject;
	PDEVICE_OBJECT DeviceObject;
	PIRP Irp;
	KEVENT event1;
	FILE_DISPOSITION_INFORMATION FileInformation;
	IO_STATUS_BLOCK ioStatus;
	PIO_STACK_LOCATION irpSp;
	PSECTION_OBJECT_POINTERS pSectionObjectPointer; ////////////////////

	SForceStripFileAttributes(FileHandle); //ȥ��ֻ�����ԣ�����ɾ��ֻ���ļ�

	ntStatus = ObReferenceObjectByHandle(FileHandle,
										 DELETE,
										 *IoFileObjectType,
										 KernelMode,
										 &fileObject,
										 NULL);

	if (!NT_SUCCESS(ntStatus))
	{
		return FALSE;
	}

	//���NTFS.sys�����������豸������û�и��ӵ��豸����Ļ����ͷ���NTFS.sys�������豸����
	//���򷵻ص�������豸�����highest level�豸����
	DeviceObject = IoGetRelatedDeviceObject(fileObject);

	Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);//���û�и��ӣ�StackSizeΪ7
	if (Irp == NULL)
	{
		ObDereferenceObject(fileObject);
		return FALSE;
	}

	KeInitializeEvent(&event1, SynchronizationEvent, FALSE);
	FileInformation.DeleteFile = TRUE;//����ɾ���ļ�

	Irp->AssociatedIrp.SystemBuffer = &FileInformation;
	Irp->UserEvent					= &event1;
	Irp->UserIosb					= &ioStatus;
	Irp->Tail.Overlay.OriginalFileObject = fileObject;
	Irp->Tail.Overlay.Thread		= (PETHREAD)KeGetCurrentThread();
	Irp->RequestorMode				= KernelMode;

	irpSp = IoGetNextIrpStackLocation(Irp); //�õ��ļ�ϵͳNTFS.sys�������豸 IO_STACK_LOCATION (I/O��ջ )
	irpSp->MajorFunction	= IRP_MJ_SET_INFORMATION;
	irpSp->DeviceObject		= DeviceObject;
	irpSp->FileObject		= fileObject;
	irpSp->Parameters.SetFile.Length				= sizeof(FILE_DISPOSITION_INFORMATION);
	irpSp->Parameters.SetFile.FileInformationClass	= FileDispositionInformation;
	irpSp->Parameters.SetFile.FileObject			= fileObject;

	IoSetCompletionRoutine(Irp,
						   SForceSetFileCompletion,
						   &event1,
						   TRUE,
						   TRUE,
						   TRUE);

	//�ټ������������д��� ��MmFlushImageSection ����ͨ������ṹ������Ƿ����ɾ���ļ���
	pSectionObjectPointer = fileObject->SectionObjectPointer;
	pSectionObjectPointer->ImageSectionObject	= 0;
	pSectionObjectPointer->DataSectionObject	= 0;

	//��������ν���NTFS.sys������ NtfsFsdSetInformation���� -> NtfsSetDispositionInfo����-> MmFlushImageSection()
	//MmFlushImageSection�����⺯����������� FILE_OBJECT ����� SECTION_OBJECT_POINTER �ṹ�ı������������ļ�
	//���ڴ���û�б�ӳ�䡣Ҳ������û��ִ�С�����������������ˣ�Ҳ����˵�ļ�����ɾ���ˡ�����Ҳ����HOOK NTFS.sys������е�
	//��MmFlushImageSection���������������ļ������ǲ�������Ҫɾ�� �ģ��ǵĻ�������TRUE�����ˡ�
	IoCallDriver(DeviceObject, Irp);
	
	KeWaitForSingleObject(&event1, Executive, KernelMode, TRUE, NULL);
	ObDereferenceObject(fileObject);

	return TRUE;
}

NTSTATUS DriverEntry( 
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	UNICODE_STRING uniDeviceName;
	UNICODE_STRING uniSymLink;
	NTSTATUS ntStatus;
	PDEVICE_OBJECT deviceObject = NULL;
	HANDLE hFileHandle;

	KdPrint(("DriverEntry\r\n"));
	RtlInitUnicodeString(&uniDeviceName, NT_DEVICE_NAME);
	RtlInitUnicodeString(&uniSymLink, DOS_DEVICE_NAME);

	KdBreakPoint();
	ntStatus = IoCreateDevice(DriverObject,
							  0x100u,
							  &uniDeviceName,
							  FILE_DEVICE_UNKNOWN,
							  FILE_DEVICE_SECURE_OPEN,
							  TRUE,
							  &deviceObject);

	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("IoCreateDevice error\r\n"));
		return ntStatus;
	}

	ntStatus = IoCreateSymbolicLink(&uniSymLink, &uniDeviceName);
	if (!NT_SUCCESS(ntStatus))
	{
		KdPrint(("IoCreateSymbolicLink error\r\n"));
		IoDeleteDevice(deviceObject);
		return ntStatus;
	}

	DriverObject->DriverUnload = UnloadDriver;

	//
	// �ص�����
	hFileHandle = SForceIoOpenFile(L"\\??\\c:\\test.exe",
								  FILE_READ_ATTRIBUTES,
								  FILE_SHARE_DELETE); //�õ��ļ����

	if (hFileHandle != NULL)
	{
		SForceDeleteFile(hFileHandle);
		ZwClose(hFileHandle);
	}
	else
	{
		KdPrint(("delete file ok"));
	}
	
	return STATUS_SUCCESS;
}