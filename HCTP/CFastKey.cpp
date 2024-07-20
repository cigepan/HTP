#include "CFastKey.h"

CFastKey::CFastKey()
{
	Init("MAP_DEFAULT", strlen("MAP_DEFAULT"));
}
CFastKey::CFastKey(const CFastKey& cKey)
{
	Init(cKey.szData, cKey.nSizeSrc);
}
CFastKey::CFastKey(const string& strKey)
{
	Init(strKey.c_str(), strKey.size());
}
CFastKey::CFastKey(const char* pKey)
{
	Init(pKey,strlen(pKey));
}
void CFastKey::Init(const char* pKey, size_t ullSize)
{
	if (ullSize < 0) { return; } //异常值
	nSizeSrc = min((int)ullSize, CFK_MAX_SIZE); //限定最大长度
	int nSizeAlign = nSizeSrc + 2;  //预留2个字节的结束符用于兼容其他类型编码
	nSizeAlign = (nSizeAlign & CFK_BYTE_BITMASK) + CFK_BYTE_ALIGN; //8字节补齐
	memset(szData, 0, nSizeAlign);
	memcpy(szData, pKey, nSizeSrc);
}
//CFastKey& CFastKey::operator=(const CFastKey& cKey) //创建新KEY
//{
//	Init(cKey.szData, cKey.nSizeSrc);
//	return *this;
//}
//键值比较函数重载：8字节对齐与64位比较，效率比普通字符串map提升4倍以上
bool CFastKey::operator()(const CFastKey& _Left, const CFastKey& _Right) const
{
	if (_Left.nSizeSrc != _Right.nSizeSrc) { return (_Left.nSizeSrc < _Right.nSizeSrc); } //尺寸比较，长度分类
	unsigned long long* pCmpLeft = (unsigned long long*)_Left.szData; //用于8字节比较的指针
	unsigned long long* pCmpRight = (unsigned long long*)_Right.szData; //用于8字节比较的指针
	unsigned long long* pCmpEnd = (unsigned long long*)(_Left.szData + _Left.nSizeSrc);
	while (pCmpLeft < pCmpEnd)
	{
		if (*(pCmpLeft) != *(pCmpRight))
			return (*(pCmpLeft) < *(pCmpRight));
		pCmpLeft++; pCmpRight++;
	}
	return false;// (0 > memcmp(_Left.ptrData, _Right.ptrData, _Left.nSizeSrc));
}