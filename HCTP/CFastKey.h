#pragma once
#include "stdafx.h"

//�ֽڶ���λ����8
#define CFK_BYTE_ALIGN 0x00000008
//�ֶ�����λ�� /8 = >>3
#define CFK_BYTE_BITDIV 3
//�ֽڶ������룺00000111
#define CFK_BYTE_BITMASK 0xFFFFFFF8
#define CFK_MAX_SIZE (int)0x003F

class CFastKey
{
	int nSizeSrc; //Դ�ߴ�
	char szData[CFK_MAX_SIZE + CFK_BYTE_ALIGN+3];
public:
	CFastKey();
	CFastKey(const CFastKey& cKey);
	CFastKey(const string& strKey);
	CFastKey(const char* pKey);
	const char* Key()const { return szData; }
	void Init(const char* pKey, size_t ullSize);
	//CFastKey& operator=(const CFastKey& cKey);
	bool operator()(const CFastKey& _Left, const CFastKey& _Right) const;
};