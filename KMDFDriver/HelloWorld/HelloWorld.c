#include <ntddk.h>
#include <wdf.h>
DRIVER_INITIALIZE DriverEntry;


// ж����������
VOID DriverUnload(_In_ PDRIVER_OBJECT driver)
{
	DbgPrint("My first driver is unloading��");
}

//�����������ں���
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{

	// �������ǵ��ں�ģ�����ڣ�����������д��������д�Ķ�����
	// ���������ӡһ�仰����Ϊ��Hello,world�� ���������ֳ�Ц������
	// ���Ǵ�ӡһ���ġ�
	DbgPrint("Hello world! My first driver Init Success!");

	// ����һ��ж�غ�����������������˳���
	DriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;

}
