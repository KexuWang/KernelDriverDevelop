#pragma once
#include <NTDDK.h>

#define PAGEDCODE	code_seg("PAGE")	//��ҳ�ڴ棺Windows�涨��Щ�����ڴ���Խ������ļ��У������ڴ汻��Ϊ��ҳ�ڴ�
#define LOCKEDCODE	code_seg()			//�Ƿ�ҳ�ڴ棺��Щ�����ڴ���Զ���ύ�����ļ��У���Щ�ڴ�зǷ�ҳ�ڴ�
#define INITCODE	code_seg("INIT")	//��ʼ�����֮��ͻᱻ�ͷš���ĳ��������Ҫ�ڳ�ʼ����ʱ�����롱�ڴ棬Ȼ����Դ��ڴ��С�ж�ء���

#define PAGEDDATA	data_seg("PAGE")
#define LOCKEDDATA	data_seg()
#define INITDATA	data_seg("INIT")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))
#define MEM_TAG 'MyTt'