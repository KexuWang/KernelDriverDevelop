/*
文件删除的过程
删除文件时会先到NTFS.sys的分派例程 -> NtfsSetDispositionInfo -> MmFlushImageSection.
MmFlushImageSection()函数会检查这个文件对象的SECTION_OBJECT_POINter结构，
看里面是不是为空(也就是检查这个文件在没在运行),没有的话，直接返回TRUE。

强制删除文件
自己构造IRP，然后向NTFS.sys发送IRP，先设置文件的属性，再删除文件。
方法1:
设置SECTION_OBJECT_POINter结构里的变量都为0。这样 MmFlushImageSection() 为返回TRUE，表示能删除。
方法2:
HOOK NTFS.sys的导入表中的MmFlushImageSection()函数，在HOOK函数中检查是不是我们要删除的文件，是的话直接返回TRUE也行。
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

//IO完成例程
NTSTATUS SForceSetFileCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	Irp->UserIosb->Status		= Irp->IoStatus.Status;
	Irp->UserIosb->Information	= Irp->IoStatus.Information;

	KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);

	IoFreeIrp(Irp);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

//去掉文件只读属性
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
										 NULL);//这个文件句柄是在哪个进程的句柄表中

	if (!NT_SUCCESS(ntStatus))
	{
		return FALSE;
	}

	//event1
	KeInitializeEvent(&event1, SynchronizationEvent, FALSE);

	//设置文件的属性
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

	// IrpSp I/O堆栈单元
	irpSp = IoGetNextIrpStackLocation(Irp);	//IoGetNextIrpStackLocation 返回在给定的IRP中低一层驱动使用的 I/O stack location 的指针
	irpSp->MajorFunction	= IRP_MJ_SET_INFORMATION;
	irpSp->DeviceObject		= DeviceObject;
	irpSp->FileObject		= fileObject;
	irpSp->Parameters.SetFile.Length				= sizeof(FILE_BASIC_INFORMATION);
	irpSp->Parameters.SetFile.FileInformationClass	= FileBasicInformation;
	irpSp->Parameters.SetFile.FileObject			= fileObject;

	//注册一个IO完成例程，这个完成例程将会在调用此函数的驱动的下一层驱动完成IRP指定的操作请求时被调用
	IoSetCompletionRoutine(Irp,
						   SForceSetFileCompletion,
						   &event1,
						   TRUE,
						   TRUE,
						   TRUE);

	//调用这个设备对象的驱动对象，并且 IO_STACK_LOCATION 会指向下一个，也就是刚刚设置的
	//如果没有文件系统驱动建立的设备对象没有Attacked的话，就调用文件系统驱动的IRP_MJ_SET_INFORMATION分派例程
	IoCallDriver(DeviceObject, Irp);
					

	//会调用NTFS.sys驱动的NtfsFsdSetInformation例程，再会进入NtfsSetBasicInfo()函数，最后它会设置代表此文件的FCB（文件
	//控制块结构的一些信息，用来设置代表此文件的属性。最后不知道在哪里会调用IoCompleteRequest,它会依次调用先前设置的回调函数
	//回调函数会释放刚分配的IRP和设置事件对象为受信状态。
	KeWaitForSingleObject(&event1, Executive, KernelMode, TRUE, NULL);//一等到事件对象变成受信状态就会继续向下执行。

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

	SForceStripFileAttributes(FileHandle); //去掉只读属性，才能删除只读文件

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

	//如果NTFS.sys驱动建立的设备对象上没有附加的设备对象的话，就返回NTFS.sys建立的设备对象
	//否则返回的是这个设备对象的highest level设备对象。
	DeviceObject = IoGetRelatedDeviceObject(fileObject);

	Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);//如果没有附加，StackSize为7
	if (Irp == NULL)
	{
		ObDereferenceObject(fileObject);
		return FALSE;
	}

	KeInitializeEvent(&event1, SynchronizationEvent, FALSE);
	FileInformation.DeleteFile = TRUE;//设置删除文件

	Irp->AssociatedIrp.SystemBuffer = &FileInformation;
	Irp->UserEvent					= &event1;
	Irp->UserIosb					= &ioStatus;
	Irp->Tail.Overlay.OriginalFileObject = fileObject;
	Irp->Tail.Overlay.Thread		= (PETHREAD)KeGetCurrentThread();
	Irp->RequestorMode				= KernelMode;

	irpSp = IoGetNextIrpStackLocation(Irp); //得到文件系统NTFS.sys驱动的设备 IO_STACK_LOCATION (I/O堆栈 )
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

	//再加上下面这三行代码 ，MmFlushImageSection 函数通过这个结构来检查是否可以删除文件。
	pSectionObjectPointer = fileObject->SectionObjectPointer;
	pSectionObjectPointer->ImageSectionObject	= 0;
	pSectionObjectPointer->DataSectionObject	= 0;

	//这里会依次进入NTFS.sys驱动的 NtfsFsdSetInformation例程 -> NtfsSetDispositionInfo（）-> MmFlushImageSection()
	//MmFlushImageSection（）这函数是用来检查 FILE_OBJECT 对象的 SECTION_OBJECT_POINTER 结构的变量，检查这个文件
	//在内存有没有被映射。也就是有没有执行。如果上面那样设置了，也就是说文件可以删除了。我们也可以HOOK NTFS.sys导入表中的
	//的MmFlushImageSection（），来检查这个文件对象是不是我们要删除 的，是的话，返回TRUE就行了。
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
	// 重点在这
	hFileHandle = SForceIoOpenFile(L"\\??\\c:\\test.exe",
								  FILE_READ_ATTRIBUTES,
								  FILE_SHARE_DELETE); //得到文件句柄

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