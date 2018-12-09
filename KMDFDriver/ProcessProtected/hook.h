/*
1: kd> u nt!ZwOpenProcess
nt!ZwOpenProcess:
83e9386c b8be000000      mov     eax,0BEh
83e93871 8d542404        lea     edx,[esp+4]
83e93875 9c              pushfd
83e93876 6a08            push    8
83e93878 e8b1190000      call    nt!KiSystemService (83e9522e)
83e9387d c21000          ret     10h
1: kd> dd KeServiceDescriptorTable
83fba9c0  83ec16f0 00000000 00000191 83ec1d38
83fba9d0  00000000 00000000 00000000 00000000
83fba9e0  83f2d493 00000000 03b770bf 0000002e
83fba9f0  00000011 00000100 5385d2ba d717548f
83fbaa00  83ec16f0 00000000 00000191 83ec1d38
83fbaa10  b2575000 00000000 00000339 b257602c
83fbaa20  00000000 00000000 83fbaa24 00000240
83fbaa30  00000240 a17fb508 00000003 00000000��
??0x83ec16f0+0xbe*4
1: kd> dd 0x83ec19e8
83ec19e8  840d3531 8408e8f1 8408e03e 83fcfdc0
83ec19f8  840d11ba 840363ca 84055cc8 840d048a
83ec1a08  840d1e88 8408e155 8408dc29 8415aedb
83ec1a18  84143879 84144b11 8403a231 8408b7fc
83ec1a28  8414342a 8414314a 841434e2 84143202
83ec1a38  8404d05c 840159ba 8403003f 8414526c
83ec1a48  84145332 840d1c41 840e8c3c 840b4b48
83ec1a58  84155afa 84155f3d 83f14c6a 840d6473
1: kd> u 840d3531
nt!NtOpenProcess:
840d3531 8bff            mov     edi,edi
840d3533 55              push    ebp
840d3534 8bec            mov     ebp,esp
840d3536 51              push    ecx
840d3537 51              push    ecx
840d3538 64a124010000    mov     eax,dword ptr fs:[00000124h]
840d353e 8a803a010000    mov     al,byte ptr [eax+13Ah]
840d3544 8b4d14          mov     ecx,dword ptr [ebp+14h]
*/

#pragma once
#include <NTDDK.h> 


BOOL	ssdthook_flag=FALSE;
ULONG	RealNtOpenProcessAddress; 
HANDLE	MyPID; 


// A�������Լ����ں˺����������滻��Ӧ���ں˺���)

// ����һ��NtOpenProcess��ԭ��
typedef NTSTATUS __stdcall NTOPENPROCESS 
( 
 OUT PHANDLE ProcessHandle, 
 IN ACCESS_MASK AccessMask, 
 IN POBJECT_ATTRIBUTES ObjectAttributes, 
 IN PCLIENT_ID ClientId 
 );

NTOPENPROCESS* RealNtOpenProcess; 

PEPROCESS  EP;

// �Զ����NtOpenProcess���� ZwOpenProcess
#pragma PAGECODE
NTSTATUS __stdcall MyNtOpenProcess( 
	OUT     PHANDLE ProcessHandle, 
	IN     ACCESS_MASK DesiredAccess, 
	IN     POBJECT_ATTRIBUTES ObjectAttributes, 
	IN     PCLIENT_ID ClientId ) 
{ 
	NTSTATUS     status;
	HANDLE       PID; 

	KdPrint(("++++++++++++Entry MyNtOpenProcess int   ++++++++++++++\n"));  

	status = (NTSTATUS)RealNtOpenProcess( ProcessHandle, DesiredAccess, ObjectAttributes, ClientId );

	if( (ClientId != NULL) ) 
	{ 
		PID = ClientId->UniqueProcess; 	 
		KdPrint(( "------------------------- PID=%d--------------\n",(int*)PID ));

		// ����Ǳ�������PID����ܾ����ʣ������������Ϊ�� 
		if(PID == MyPID) 
		{ 
			KdPrint(("���������� MyPID=%d \n",(int)MyPID));
			ProcessHandle = NULL;		//����ǹؼ�
			status = STATUS_ACCESS_DENIED;	//�������ֵ 
			//PsLookupProcessByProcessId((ULONG)PID,&EP);
			EP=PsGetCurrentProcess();			 
			KdPrint((" ACESS Process Name  --:%s--   \n",(PTSTR)((ULONG)EP+0x174)));
		} 
	} 

	return status;
} 

//HOOK ��������
#pragma PAGECODE
VOID Hook() 
{ 
	ssdthook_flag = TRUE;//���ñ�HOOK��־
	KdPrint(("++++HOOK START ++++-\n"));
	LONG *ssdt_index_be,SSDT_NtOpenProcess_Cur_Addr, serviceTableBaseAddr;

	KdPrint(("�����ɹ���������.............................\n"));
	//��ȡSSDT��������ֵΪ0xbe�ĺ���
	//poi(poi(KeServiceDescriptorTable)+0xbe*4)
	serviceTableBaseAddr=(LONG)KeServiceDescriptorTable->ServiceTableBase;
	ssdt_index_be =(PLONG)( serviceTableBaseAddr + 0xbe*4 );

	SSDT_NtOpenProcess_Cur_Addr=*ssdt_index_be;
	RealNtOpenProcessAddress = *ssdt_index_be;
	RealNtOpenProcess = ( NTOPENPROCESS *)RealNtOpenProcessAddress; 

	KdPrint(( "��ʵ��NtOpenProcess��ַ: %x\n",	(int) RealNtOpenProcessAddress )); 
	KdPrint((" α��NTOpenProcess��ַ: %x\n",	(int)MyNtOpenProcess ));  


	__asm //ȥ��ҳ�汣��
	{
		cli
		mov eax,cr0
		and eax,not 10000h //and eax,0FFFEFFFFh
		mov cr0,eax
	}
	*ssdt_index_be = (ULONG)MyNtOpenProcess; //SSDT HOOK
	__asm 
	{ 
		mov	eax, cr0 
		or	eax, 10000h 
		mov	cr0, eax 
		sti 
	}   
	return;
} 

//UnHook��������
#pragma PAGECODE
VOID UnHook() 
{ 
	ULONG ssdt_Index_be;
	ssdt_Index_be = (ULONG)KeServiceDescriptorTable->ServiceTableBase + 0xbe * 4;
	if (ssdthook_flag)
	{
		ssdthook_flag = FALSE;
		__asm
		{
			cli
			mov     eax, cr0
			and     eax, not 10000h
			mov     cr0, eax
		}

		// ��ԭSSDT 
		*((ULONG*)ssdt_Index_be) = (ULONG)RealNtOpenProcessAddress;

		__asm
		{
			mov     eax, cr0
			or eax, 10000h
			mov     cr0, eax
			sti
		}
		KdPrint(("UnHook��ԭSSDT OK \n"));
	}

	return;
}