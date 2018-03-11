#include"stdafx.h"
#include"PE.h"
#include"afxwin.h"
#include<Windows.h>
#include<iostream>
typedef unsigned long DWORD;
using namespace std;
/*
Purpose:PE文件的内存偏移与文件偏移相互转换,不考虑系统为对齐填充偏移转换
szFileName:文件名
dwAddr:需要转换的偏移值
bFile2RVA:是否是文件偏移到内存偏移的转换，1 - dwAddr代表的是文件偏移，此函数返回内存偏移
    0 - dwAddr代表的是内存偏移，此函数返回文件偏移
返回值：相对应的偏移值,失败返回-1
总结：根据文件头信息判断地址在哪个节区范围，再由节的文件/内存偏移+地址相对区段基址的偏移（固定）
*/

DWORD AddressConvert(char szFileName[], DWORD dwAddr, BOOL bFile2RVA)
{
 char *lpBase = NULL;
 DWORD dwRet = -1;
 //1.首先将文件读入内存
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
第一个参数stream为文件指针
第二个参数offset为偏移量，正数表示正向偏移，负数表示负向偏移
第三个参数origin设定从文件的哪里开始偏移,可能取值为：SEEK_CUR(1)、 SEEK_END(2) 或 SEEK_SET(0).
fseek(fp,-100L,2);把stream指针退回到离文件结尾100字节处。
 */
 DWORD dwFileSize = ftell(fp);
 //#用于得到文件位置指针当前位置相对于文件首的偏移字节数。
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
用于接收数据的内存地址
size
要读的每个数据项的字节数，单位是字节
count
要读count个数据项，每个数据项size个字节.
stream
输入流
*/
//#原来是读取了当前位置偏移个，即当前位置前的内容。所以最后1字节没有读取。
 fclose(fp);

 //2.读取该文件的信息（文件内存对齐方式以及区块数量，并将区块表指针指向区块表第一个区块头）
 PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpBase;
 PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((unsigned long)lpBase + pDosHeader->e_lfanew);
 
 DWORD dwMemAlign = pNtHeader->OptionalHeader.SectionAlignment;
 DWORD dwFileAlign = pNtHeader->OptionalHeader.FileAlignment;
 int dwSecNum = pNtHeader->FileHeader.NumberOfSections;
 PIMAGE_SECTION_HEADER pSecHeader = (PIMAGE_SECTION_HEADER)((char *)lpBase + pDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));
 DWORD dwHeaderSize = 0;
 
 if(!bFile2RVA)  // 内存偏移转换为文件偏移
 { cout<<"转换为文件偏移"<<endl;
  //看需要转移的偏移是否在PE头内，如果在则两个偏移相同
  dwHeaderSize = pNtHeader->OptionalHeader.SizeOfHeaders;
  if(dwAddr <= dwHeaderSize)
  {
   delete lpBase;
   //#内存回收，申请内存需要一个指针变量，内存回收后指针变量还要置null
   lpBase = NULL;
   //#指针置null
   return dwAddr;
  }
  else //不再PE头里，查看该地址在哪个区块中
  {
   for(int i = 0; i < dwSecNum; i++)
   {
    DWORD dwSecSize = pSecHeader[i].Misc.VirtualSize;
	//#节头里的偏移是内存偏移，即将后面的节放置内存中文件的多少偏移处
    if((dwAddr >= pSecHeader[i].VirtualAddress) && (dwAddr <= pSecHeader[i].VirtualAddress + dwSecSize))
    {
     //3.找到该该偏移，则文件偏移 = 该区块的文件偏移 + （该偏移 - 该区块的内存偏移）
		
     dwRet = pSecHeader[i].PointerToRawData + dwAddr - pSecHeader[i].VirtualAddress;
	}else{cout<<"未找到对应节"<<endl;return 0;}
   }
  }
 }
 else // 文件偏移转换为内存偏移
 {
  dwHeaderSize = pNtHeader->OptionalHeader.SizeOfHeaders;
  //看需要转移的偏移是否在PE头内，如果在则两个偏移相同
  if(dwAddr <= dwHeaderSize)
  {
   delete lpBase;
   lpBase = NULL;
   return dwAddr;
  }
  else//不再PE头里，查看该地址在哪个区块中
  {
   for(int i = 0; i < dwSecNum; i++)
   {
    DWORD dwSecSize = pSecHeader[i].Misc.VirtualSize;
    if((dwAddr >= pSecHeader[i].PointerToRawData) && (dwAddr <= pSecHeader[i].PointerToRawData + dwSecSize))
    {//#节头中也存有文件偏移PointerToRawData
     //3.找到该该偏移，则内存偏移 = 该区块的内存偏移 + （该偏移 - 该区块的文件偏移）
     dwRet = pSecHeader[i].VirtualAddress + dwAddr - pSecHeader[i].PointerToRawData;
    }
   }
  }
 }
 
 //5.释放内存
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
		MessageBox(NULL, _T("加载Shell.dll模块失败，请确保程序的完整性！"), _T("提示"), MB_OK);
	}
	LPFNREGISTER Min =(LPFNREGISTER)GetProcAddress(hShell, "Min");
	cout<<"min_memaddr:  "<<Min<<endl;
	FreeLibrary(hShell);
	if (hShell == NULL)
	{
		MessageBox(NULL, _T("加载Shell.dll模块失败，请确保程序的完整性！"), _T("提示"), MB_OK);
	}
	DWORD Min_fileaddr=AddressConvert("dlltest.dll", 0x42000, 0);
	cout<<"min_fileaddr:  "<<Min_fileaddr;
	system("pause");

}