#include <ntddk.h>
#include <wdf.h>
DRIVER_INITIALIZE DriverEntry;


// 卸载驱动程序
VOID DriverUnload(_In_ PDRIVER_OBJECT driver)
{
	DbgPrint("My first driver is unloading…");
}

//驱动程序的入口函数
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{

	DbgPrint("Hello world! My first driver Init Success!");

	// 设置一个卸载函数便于这个函数能退出。
	DriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;

}
