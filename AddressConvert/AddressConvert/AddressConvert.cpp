#include"stdafx.h"
#include"PE.h"
#include"afxwin.h"
#include<Windows.h>
#include<iostream>
typedef unsigned long DWORD;
using namespace std;
/*
Purpose:PE�ļ����ڴ�ƫ�����ļ�ƫ���໥ת��,������ϵͳΪ�������ƫ��ת��
szFileName:�ļ���
dwAddr:��Ҫת����ƫ��ֵ
bFile2RVA:�Ƿ����ļ�ƫ�Ƶ��ڴ�ƫ�Ƶ�ת����1 - dwAddr��������ļ�ƫ�ƣ��˺��������ڴ�ƫ��
    0 - dwAddr��������ڴ�ƫ�ƣ��˺��������ļ�ƫ��
����ֵ�����Ӧ��ƫ��ֵ,ʧ�ܷ���-1
�ܽ᣺�����ļ�ͷ��Ϣ�жϵ�ַ���ĸ�������Χ�����ɽڵ��ļ�/�ڴ�ƫ��+��ַ������λ�ַ��ƫ�ƣ��̶���
*/

DWORD AddressConvert(char szFileName[], DWORD dwAddr, BOOL bFile2RVA)
{
 char *lpBase = NULL;
 DWORD dwRet = -1;
 //1.���Ƚ��ļ������ڴ�
 if(szFileName[0] == 0)
 {
  return -1;
 }

 FILE *fp = fopen("dlltest.dll", "rb");
 if(fp == 0)
 {
  cout<<"open dlltest.dll fail!"<<endl;
  return -1;
 }

 fseek(fp, 0, SEEK_END);
 /*##
��һ������streamΪ�ļ�ָ��
�ڶ�������offsetΪƫ������������ʾ����ƫ�ƣ�������ʾ����ƫ��
����������origin�趨���ļ������￪ʼƫ��,����ȡֵΪ��SEEK_CUR(1)�� SEEK_END(2) �� SEEK_SET(0).
fseek(fp,-100L,2);��streamָ���˻ص����ļ���β100�ֽڴ���
 */
 DWORD dwFileSize = ftell(fp);
 //#���ڵõ��ļ�λ��ָ�뵱ǰλ��������ļ��׵�ƫ���ֽ�����
 if(dwFileSize == 0)
 {
  return -1;
 }

 lpBase = new char[dwFileSize];
 memset(lpBase, 0, dwFileSize);
 fseek(fp, 0, SEEK_SET);
 fread(lpBase, 1, dwFileSize, fp);
 /*##
buffer
���ڽ������ݵ��ڴ��ַ
size
Ҫ����ÿ����������ֽ�������λ���ֽ�
count
Ҫ��count�������ÿ��������size���ֽ�.
stream
������
*/
//#ԭ���Ƕ�ȡ�˵�ǰλ��ƫ�Ƹ�������ǰλ��ǰ�����ݡ��������1�ֽ�û�ж�ȡ��
 fclose(fp);

 //2.��ȡ���ļ�����Ϣ���ļ��ڴ���뷽ʽ�Լ��������������������ָ��ָ��������һ������ͷ��
 PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpBase;
 PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((unsigned long)lpBase + pDosHeader->e_lfanew);
 
 DWORD dwMemAlign = pNtHeader->OptionalHeader.SectionAlignment;
 DWORD dwFileAlign = pNtHeader->OptionalHeader.FileAlignment;
 int dwSecNum = pNtHeader->FileHeader.NumberOfSections;
 PIMAGE_SECTION_HEADER pSecHeader = (PIMAGE_SECTION_HEADER)((char *)lpBase + pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));
 DWORD dwHeaderSize = 0;
 
 if(!bFile2RVA)  // �ڴ�ƫ��ת��Ϊ�ļ�ƫ��
 { cout<<"ת��Ϊ�ļ�ƫ��"<<endl;
  //����Ҫת�Ƶ�ƫ���Ƿ���PEͷ�ڣ������������ƫ����ͬ
  dwHeaderSize = pNtHeader->OptionalHeader.SizeOfHeaders;
  if(dwAddr <= dwHeaderSize)
  {
   delete lpBase;
   //#�ڴ���գ������ڴ���Ҫһ��ָ��������ڴ���պ�ָ�������Ҫ��null
   lpBase = NULL;
   //#ָ����null
   return dwAddr;
  }
  else //����PEͷ��鿴�õ�ַ���ĸ�������
  {
   for(int i = 0; i < dwSecNum; i++)
   {
    DWORD dwSecSize = pSecHeader[i].Misc.VirtualSize;
	//#��ͷ���ƫ�����ڴ�ƫ�ƣ���������Ľڷ����ڴ����ļ��Ķ���ƫ�ƴ�
    if((dwAddr >= pSecHeader[i].VirtualAddress) && (dwAddr <= pSecHeader[i].VirtualAddress + dwSecSize))
    {
     //3.�ҵ��ø�ƫ�ƣ����ļ�ƫ�� = ��������ļ�ƫ�� + ����ƫ�� - ��������ڴ�ƫ�ƣ�
		
     dwRet = pSecHeader[i].PointerToRawData + dwAddr - pSecHeader[i].VirtualAddress;
	}else{cout<<"δ�ҵ���Ӧ��"<<endl;return 0;}
   }
  }
 }
 else // �ļ�ƫ��ת��Ϊ�ڴ�ƫ��
 {
  dwHeaderSize = pNtHeader->OptionalHeader.SizeOfHeaders;
  //����Ҫת�Ƶ�ƫ���Ƿ���PEͷ�ڣ������������ƫ����ͬ
  if(dwAddr <= dwHeaderSize)
  {
   delete lpBase;
   lpBase = NULL;
   return dwAddr;
  }
  else//����PEͷ��鿴�õ�ַ���ĸ�������
  {
   for(int i = 0; i < dwSecNum; i++)
   {
    DWORD dwSecSize = pSecHeader[i].Misc.VirtualSize;
    if((dwAddr >= pSecHeader[i].PointerToRawData) && (dwAddr <= pSecHeader[i].PointerToRawData + dwSecSize))
    {//#��ͷ��Ҳ�����ļ�ƫ��PointerToRawData
     //3.�ҵ��ø�ƫ�ƣ����ڴ�ƫ�� = ��������ڴ�ƫ�� + ����ƫ�� - ��������ļ�ƫ�ƣ�
     dwRet = pSecHeader[i].VirtualAddress + dwAddr - pSecHeader[i].PointerToRawData;
    }
   }
  }
 }
 
 //5.�ͷ��ڴ�
 delete lpBase;
 lpBase = NULL;
 return dwRet;
}
typedef DWORD (CALLBACK *LPFNREGISTER)(int,int);
void main()
{
	cout<<"load dlltest.dll"<<endl;
	HMODULE hShell = LoadLibrary(TEXT("dlltest.dll"));
	if (hShell == NULL)
	{
		MessageBox(NULL, _T("����Shell.dllģ��ʧ�ܣ���ȷ������������ԣ�"), _T("��ʾ"), MB_OK);
	}
	LPFNREGISTER Min =(LPFNREGISTER)GetProcAddress(hShell, "Min");
	cout<<"min_memaddr:  "<<Min<<endl;
	FreeLibrary(hShell);
	if (hShell == NULL)
	{
		MessageBox(NULL, _T("����Shell.dllģ��ʧ�ܣ���ȷ������������ԣ�"), _T("��ʾ"), MB_OK);
	}
	DWORD Min_fileaddr=AddressConvert("dlltest.dll", 0x42000, 0);
	cout<<"min_fileaddr:  "<<Min_fileaddr;
	system("pause");

}