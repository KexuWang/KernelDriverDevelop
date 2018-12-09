#include "StringOperations.h"
#define BUFFER_SIZE 1024

#pragma INITCODE
VOID CharTest()
{
	KdPrint(("Char�ִ�����--Start \n"));

	PCHAR s1 = "abc123";        //CHAR ANSI
	KdPrint(("%x,%s\n", s1, s1));

	WCHAR* s2 = L"abc123";     //WCHAR UNICODE //PWSTR
	KdPrint(("%x,%S\n", s2, s2));

	KdPrint(("Char�ִ�����--End \n"));
	KdBreakPoint();
	//_asm int 3

}

//ANSI_STRING
//UNICODE_STRING
//�ַ�����ʼ������
#define way1
#pragma INITCODE
VOID StringInitTest()
{
	KdPrint(("��ʼ���ִ�����--Start \n"));
	ANSI_STRING  AnsiString1 = { 0 };
	UNICODE_STRING UnicodeString1 = { 0 };

#ifdef way1
	//����1�������ڴ沢δ�丳ֵ
	//AnsiString1.Buffer="AnsiString1�ַ���";
	AnsiString1.Buffer = (PCHAR)ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE, MEM_TAG);
	strcpy(AnsiString1.Buffer, "AnsiString1�ַ���");
	AnsiString1.Length = (USHORT)strlen(AnsiString1.Buffer);
	AnsiString1.MaximumLength = BUFFER_SIZE;

	//UnicodeString1.Buffer =  L"UnicodeString1�ַ���";
	UnicodeString1.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE, MEM_TAG);
	wcscpy(UnicodeString1.Buffer, L"UnicodeString1�ַ���");
	UnicodeString1.Length = (USHORT)wcslen(UnicodeString1.Buffer) * 2 - 1;
	UnicodeString1.MaximumLength = BUFFER_SIZE;

	//�ͷ��ִ�
	ExFreePool(AnsiString1.Buffer);
	ExFreePool(UnicodeString1.Buffer);

#elif way2
	//����2 ��RTL������ʼ��
	//��ʼ��ANSI_STRING,UNICODE_STRING�ַ���
	//��ʼ��
	RtlInitAnsiString(&AnsiString1,"AnsiString1�ַ���");	 
	RtlInitUnicodeString(&UnicodeString1,L"UnicodeString1�ַ���");

	//�ͷ�
	RtlFreeAnsiString(&AnsiString1);
	RtlFreeUnicodeString(&UnicodeString1);

#endif // way1

	//��ӡASCII�ַ��� %Z
	KdPrint(("%x AnsiString1:   %Z\n", &AnsiString1, &AnsiString1));
	//��ӡUNICODE�ַ��� %wZ �����ַ��ᱻ�ض�
	KdPrint(("%x UnicodeString1:%wZ\n", &UnicodeString1, &UnicodeString1));

	KdPrint(("��ʼ���ִ�����--END \n"));

	KdBreakPoint();
}

//D���ַ������ƣ��Ƚϣ�����Сд���������ִ����໥ת��
#pragma INITCODE
VOID StringCopyTest()
{
	KdPrint(("\n-------------------�ִ����Ʋ��Կ�ʼ---------------D \n"));

	UNICODE_STRING strSource;
	RtlInitUnicodeString(&strSource, L"string copy test!");

	UNICODE_STRING strDestination = { 0 };
	strDestination.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE, MEM_TAG);
	strDestination.MaximumLength = BUFFER_SIZE;

	//����ʼ��strSource������strDestination
	RtlCopyUnicodeString(&strDestination, &strSource);//strDestination=strSource


	KdPrint(("�ִ�1:%wZ\n", &strSource));
	KdPrint(("�ִ�2:%wZ\n", &strDestination));
	if ( RtlEqualUnicodeString(&strSource, &strDestination, TRUE))
	{
		KdPrint(("2�� �ִ����\r\n"));
	}
	//����UnicodeString2
	//ע��!���ù�RtlInitUnicodeString ��ʼ�����ִ�!UnicodeString1��������, 

	//RtlFreeUnicodeString(&strSource); ���ϴ��л�����
	RtlFreeUnicodeString(&strDestination);
	KdPrint(("\n-------------------�ִ����Ʋ��Խ���--------------- \n"));
}


//�ַ������д����
#pragma INITCODE
VOID StringToUpperTest()
{
	KdPrint(("\n-------------------�ִ�ת��д���� ��ʼ--------------- \n"));

	UNICODE_STRING strTest;
	RtlInitUnicodeString(&strTest, L"UnicodeString1��Aabc");

	KdPrint(("UnicodeString1 ��ֵ:%wZ\n", &strTest));

	//���д
	RtlUpcaseUnicodeString(&strTest, &strTest, FALSE);

	KdPrint(("UnicodeString1 ת����д��:%wZ\n", &strTest));
	KdPrint(("\n-------------------�ִ�ת��д���� ����--------------- \n"));
}

//�ַ����������໥ת������
#pragma INITCODE
VOID StringToIntegerTest()
{
	KdPrint(("\n-------------------�ִ�ת�������� ��ʼ--------------- \n"));
	//(1)�ַ���ת��������
	//��ʼ��UnicodeString1
	UNICODE_STRING strTest1;
	RtlInitUnicodeString(&strTest1, L"1024");

	ULONG lNumber;
	KdPrint(("��ת���ִ�%wZ \n", &strTest1));
	NTSTATUS nStatus = RtlUnicodeStringToInteger(&strTest1, 10, &lNumber);
	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("�ִ�ת�������ɹ�.���=%d !\n", lNumber));
	}
	else
	{
		KdPrint(("ת������ʧ��\n"));
	}

	//(2)����ת�����ַ���
	//��ʼ��UnicodeString2
	UNICODE_STRING strTest2 = { 0 };
	strTest2.Buffer			= (PWSTR)ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE, MEM_TAG);
	strTest2.MaximumLength	= BUFFER_SIZE;
	nStatus = RtlIntegerToUnicodeString(200, 2, &strTest2);

	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("����ת���ִ��ɹ�! ���:%wZ\n", &strTest2));
	}
	else
	{
		KdPrint(("ת���ִ� ʧ��!\n"));
	}

	//����strTest2
	//ע��!���ù�RtlInitUnicodeString ��ʼ�����ִ�!strTest1��������, 
	RtlFreeUnicodeString(&strTest2);
	KdPrint(("\n-------------------�ִ�ת�������� ����--------------- \n"));
}


//ANSI_STRING�ַ���<--->UNICODE_STRING�ַ���
#pragma INITCODE
VOID StringConverTest()
{
	KdPrint(("\n-------------------ANSI_STRING�ַ�����UNICODE_STRING�ַ����໥ת������ ��ʼ--------------- \n"));
	//(1)��UNICODE_STRING�ַ���ת����ANSI_STRING�ַ���
	//��ʼ��strTest1
	UNICODE_STRING wstrTest1;
	RtlInitUnicodeString(&wstrTest1, L"UnicodeString1");

	ANSI_STRING strTest1;
	NTSTATUS nStatus = RtlUnicodeStringToAnsiString(&strTest1, &wstrTest1, TRUE);

	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("RtlUnicodeStringToAnsiString ת���ɹ� ���=%Z\n", &strTest1));
	}
	else
	{
		KdPrint(("RtlAnsiStringToUnicodeString ת��ʧ�� !\n"));
	}

	//����strTest1
	RtlFreeAnsiString(&strTest1);

	//(2)��ANSI_STRING�ַ���ת����UNICODE_STRING�ַ���
	//��ʼ��AnsiString2
	ANSI_STRING strTest2;
	RtlInitString(&strTest2, "AnsiString2");

	UNICODE_STRING wstrTest2;
	//RtlAnsiStringToUnicodeString ����� wstrTest2 ��Ҫ���� RtlFreeUnicodeString ���ͷ�
	nStatus = RtlAnsiStringToUnicodeString(&wstrTest2, &strTest2, TRUE);

	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("RtlAnsiStringToUnicodeStringת���ɹ� ���=%wZ\n", &wstrTest2));
	}
	else
	{
		KdPrint(("RtlAnsiStringToUnicodeString�ִ�ת��ʧ��!\n"));
	}

	RtlFreeUnicodeString(&wstrTest2);
	KdPrint(("\n-------------------ANSI_STRING�ַ�����UNICODE_STRING�ַ����໥ת������ ����--------------- \n"));

}

#pragma PAGEDCODE
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("Enter DriverUnload\r\n"));
}

#pragma INITCODE
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	KdPrint(("Enter DriverEntry\n"));

	//ע�������������ú������
	pDriverObject->DriverUnload = DriverUnload;

	CharTest();
	StringConverTest();
	StringCopyTest();
	StringInitTest();
	StringToIntegerTest();
	StringToUpperTest();

	KdPrint(("DriverEntry end\n"));
	return STATUS_SUCCESS;
}

