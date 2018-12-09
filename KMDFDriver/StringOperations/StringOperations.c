#include "StringOperations.h"
#define BUFFER_SIZE 1024

#pragma INITCODE
VOID CharTest()
{
	KdPrint(("Char字串测试--Start \n"));

	PCHAR s1 = "abc123";        //CHAR ANSI
	KdPrint(("%x,%s\n", s1, s1));

	WCHAR* s2 = L"abc123";     //WCHAR UNICODE //PWSTR
	KdPrint(("%x,%S\n", s2, s2));

	KdPrint(("Char字串测试--End \n"));
	KdBreakPoint();
	//_asm int 3

}

//ANSI_STRING
//UNICODE_STRING
//字符串初始化测试
#define way1
#pragma INITCODE
VOID StringInitTest()
{
	KdPrint(("初始化字串测试--Start \n"));
	ANSI_STRING  AnsiString1 = { 0 };
	UNICODE_STRING UnicodeString1 = { 0 };

#ifdef way1
	//方法1：申请内存并未其赋值
	//AnsiString1.Buffer="AnsiString1字符串";
	AnsiString1.Buffer = (PCHAR)ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE, MEM_TAG);
	strcpy(AnsiString1.Buffer, "AnsiString1字符串");
	AnsiString1.Length = (USHORT)strlen(AnsiString1.Buffer);
	AnsiString1.MaximumLength = BUFFER_SIZE;

	//UnicodeString1.Buffer =  L"UnicodeString1字符串";
	UnicodeString1.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE, MEM_TAG);
	wcscpy(UnicodeString1.Buffer, L"UnicodeString1字符串");
	UnicodeString1.Length = (USHORT)wcslen(UnicodeString1.Buffer) * 2 - 1;
	UnicodeString1.MaximumLength = BUFFER_SIZE;

	//释放字串
	ExFreePool(AnsiString1.Buffer);
	ExFreePool(UnicodeString1.Buffer);

#elif way2
	//方法2 用RTL函数初始化
	//初始化ANSI_STRING,UNICODE_STRING字符串
	//初始化
	RtlInitAnsiString(&AnsiString1,"AnsiString1字符串");	 
	RtlInitUnicodeString(&UnicodeString1,L"UnicodeString1字符串");

	//释放
	RtlFreeAnsiString(&AnsiString1);
	RtlFreeUnicodeString(&UnicodeString1);

#endif // way1

	//打印ASCII字符用 %Z
	KdPrint(("%x AnsiString1:   %Z\n", &AnsiString1, &AnsiString1));
	//打印UNICODE字符用 %wZ 中文字符会被截断
	KdPrint(("%x UnicodeString1:%wZ\n", &UnicodeString1, &UnicodeString1));

	KdPrint(("初始化字串测试--END \n"));

	KdBreakPoint();
}

//D、字符串复制，比较，（大小写，整数和字串）相互转换
#pragma INITCODE
VOID StringCopyTest()
{
	KdPrint(("\n-------------------字串复制测试开始---------------D \n"));

	UNICODE_STRING strSource;
	RtlInitUnicodeString(&strSource, L"string copy test!");

	UNICODE_STRING strDestination = { 0 };
	strDestination.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE, MEM_TAG);
	strDestination.MaximumLength = BUFFER_SIZE;

	//将初始化strSource拷贝到strDestination
	RtlCopyUnicodeString(&strDestination, &strSource);//strDestination=strSource


	KdPrint(("字串1:%wZ\n", &strSource));
	KdPrint(("字串2:%wZ\n", &strDestination));
	if ( RtlEqualUnicodeString(&strSource, &strDestination, TRUE))
	{
		KdPrint(("2个 字串相等\r\n"));
	}
	//销毁UnicodeString2
	//注意!调用过RtlInitUnicodeString 初始化的字串!UnicodeString1不用销毁, 

	//RtlFreeUnicodeString(&strSource); 加上此行会蓝屏
	RtlFreeUnicodeString(&strDestination);
	KdPrint(("\n-------------------字串复制测试结束--------------- \n"));
}


//字符串变大写测试
#pragma INITCODE
VOID StringToUpperTest()
{
	KdPrint(("\n-------------------字串转大写测试 开始--------------- \n"));

	UNICODE_STRING strTest;
	RtlInitUnicodeString(&strTest, L"UnicodeString1—Aabc");

	KdPrint(("UnicodeString1 初值:%wZ\n", &strTest));

	//变大写
	RtlUpcaseUnicodeString(&strTest, &strTest, FALSE);

	KdPrint(("UnicodeString1 转换大写后:%wZ\n", &strTest));
	KdPrint(("\n-------------------字串转大写测试 结束--------------- \n"));
}

//字符串与整型相互转化测试
#pragma INITCODE
VOID StringToIntegerTest()
{
	KdPrint(("\n-------------------字串转整数测试 开始--------------- \n"));
	//(1)字符串转换成数字
	//初始化UnicodeString1
	UNICODE_STRING strTest1;
	RtlInitUnicodeString(&strTest1, L"1024");

	ULONG lNumber;
	KdPrint(("待转换字串%wZ \n", &strTest1));
	NTSTATUS nStatus = RtlUnicodeStringToInteger(&strTest1, 10, &lNumber);
	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("字串转换整数成功.结果=%d !\n", lNumber));
	}
	else
	{
		KdPrint(("转换整数失败\n"));
	}

	//(2)数字转换成字符串
	//初始化UnicodeString2
	UNICODE_STRING strTest2 = { 0 };
	strTest2.Buffer			= (PWSTR)ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE, MEM_TAG);
	strTest2.MaximumLength	= BUFFER_SIZE;
	nStatus = RtlIntegerToUnicodeString(200, 2, &strTest2);

	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("数字转换字串成功! 结果:%wZ\n", &strTest2));
	}
	else
	{
		KdPrint(("转换字串 失败!\n"));
	}

	//销毁strTest2
	//注意!调用过RtlInitUnicodeString 初始化的字串!strTest1不用销毁, 
	RtlFreeUnicodeString(&strTest2);
	KdPrint(("\n-------------------字串转整数测试 结束--------------- \n"));
}


//ANSI_STRING字符串<--->UNICODE_STRING字符串
#pragma INITCODE
VOID StringConverTest()
{
	KdPrint(("\n-------------------ANSI_STRING字符串与UNICODE_STRING字符串相互转换测试 开始--------------- \n"));
	//(1)将UNICODE_STRING字符串转换成ANSI_STRING字符串
	//初始化strTest1
	UNICODE_STRING wstrTest1;
	RtlInitUnicodeString(&wstrTest1, L"UnicodeString1");

	ANSI_STRING strTest1;
	NTSTATUS nStatus = RtlUnicodeStringToAnsiString(&strTest1, &wstrTest1, TRUE);

	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("RtlUnicodeStringToAnsiString 转换成功 结果=%Z\n", &strTest1));
	}
	else
	{
		KdPrint(("RtlAnsiStringToUnicodeString 转换失败 !\n"));
	}

	//销毁strTest1
	RtlFreeAnsiString(&strTest1);

	//(2)将ANSI_STRING字符串转换成UNICODE_STRING字符串
	//初始化AnsiString2
	ANSI_STRING strTest2;
	RtlInitString(&strTest2, "AnsiString2");

	UNICODE_STRING wstrTest2;
	//RtlAnsiStringToUnicodeString 申请的 wstrTest2 需要调用 RtlFreeUnicodeString 来释放
	nStatus = RtlAnsiStringToUnicodeString(&wstrTest2, &strTest2, TRUE);

	if (NT_SUCCESS(nStatus))
	{
		KdPrint(("RtlAnsiStringToUnicodeString转换成功 结果=%wZ\n", &wstrTest2));
	}
	else
	{
		KdPrint(("RtlAnsiStringToUnicodeString字串转换失败!\n"));
	}

	RtlFreeUnicodeString(&wstrTest2);
	KdPrint(("\n-------------------ANSI_STRING字符串与UNICODE_STRING字符串相互转换测试 结束--------------- \n"));

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

	//注册其他驱动调用函数入口
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

