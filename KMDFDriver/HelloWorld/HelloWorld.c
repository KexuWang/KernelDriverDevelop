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

	// 这是我们的内核模块的入口，可以在这里写入我们想写的东西。
	// 我在这里打印一句话。因为”Hello,world” 常常被高手耻笑，所以
	// 我们打印一点别的。
	DbgPrint("Hello world! My first driver Init Success!");

	// 设置一个卸载函数便于这个函数能退出。
	DriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;

}
