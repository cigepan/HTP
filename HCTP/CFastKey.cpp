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
	if (ullSize < 0) { return; } //�쳣ֵ
	nSizeSrc = min((int)ullSize, CFK_MAX_SIZE); //�޶���󳤶�
	int nSizeAlign = nSizeSrc + 2;  //Ԥ��2���ֽڵĽ��������ڼ����������ͱ���
	nSizeAlign = (nSizeAlign & CFK_BYTE_BITMASK) + CFK_BYTE_ALIGN; //8�ֽڲ���
	memset(szData, 0, nSizeAlign);
	memcpy(szData, pKey, nSizeSrc);
}
//CFastKey& CFastKey::operator=(const CFastKey& cKey) //������KEY
//{
//	Init(cKey.szData, cKey.nSizeSrc);
//	return *this;
//}
//��ֵ�ȽϺ������أ�8�ֽڶ�����64λ�Ƚϣ�Ч�ʱ���ͨ�ַ���map����4������
bool CFastKey::operator()(const CFastKey& _Left, const CFastKey& _Right) const
{
	if (_Left.nSizeSrc != _Right.nSizeSrc) { return (_Left.nSizeSrc < _Right.nSizeSrc); } //�ߴ�Ƚϣ����ȷ���
	unsigned long long* pCmpLeft = (unsigned long long*)_Left.szData; //����8�ֽڱȽϵ�ָ��
	unsigned long long* pCmpRight = (unsigned long long*)_Right.szData; //����8�ֽڱȽϵ�ָ��
	unsigned long long* pCmpEnd = (unsigned long long*)(_Left.szData + _Left.nSizeSrc);
	while (pCmpLeft < pCmpEnd)
	{
		if (*(pCmpLeft) != *(pCmpRight))
			return (*(pCmpLeft) < *(pCmpRight));
		pCmpLeft++; pCmpRight++;
	}
	return false;// (0 > memcmp(_Left.ptrData, _Right.ptrData, _Left.nSizeSrc));
}