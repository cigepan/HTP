#pragma once
#include "CMyData.h"
#include "pool.hpp"
using namespace std;
using namespace mongocxx;
using namespace v_noabi;
using namespace bsoncxx::document;
using namespace bsoncxx::array;
using namespace bsoncxx::builder::stream;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;
using bsoncxx::type;

//enum class eMdbOpType //操作方式
//{
//	MOT_INSERT = 0,
//	MOT_DELETE,
//	MOT_UPDATE,
//	MOT_ENDF
//};
enum eMdbTable //操作表
{
	MDB_DEFAULT= 0,
	MDB_MKT_TICK,
	MDB_MKT_LINEK,
	MDB_TRD_ACCOUNT,
	MDB_TRD_TRADE,
	MDB_TRD_ORDER,
	MDB_TRD_ORDERLOG,
	MDB_TRD_POSITION,
	MDB_CONTRACT,
	MDB_RUNTIME,
	MDB_FOLLOW,
	MDB_TARGET,
	MDB_TARGET_PLAN,
	MDB_BASIC,
	MDB_RISKCTRL,
	MDB_TASK,
	MDB_HTS,
	MDB_LOG,
	MDB_EMAIL,
	MDB_ENDEF
};
const char g_szMdbTbl[MDB_ENDEF][32] = {
	"MDB_DEFAULT", "MKT_TICK", "MDB_LINEK",
	"TRD_ACCOUNT", "TRD_TRADE","TRD_ORDER","TRD_ORDERLOG","TRD_POSITION",
	"MDB_CONTRACT","MDB_RUNTIME","MDB_FOLLOW","MDB_TARGET","TARGET_PLAN",
	"MDB_BASIC","MDB_RISKCTRL","MDB_TASK","MDB_HTS", "MDB_LOG","MDB_EMAIL"
};

class CMdbClient
{
	instance m_instance;
public:
	map<CFastKey, pool*, CFastKey> m_mapPool;
	CMdbClient()
	{
		m_mapPool.clear();
	}
	~CMdbClient()
	{
		pool* pPool = NULL;
		for (const auto& it : m_mapPool)
		{
			if (it.second != pPool)
			{
				pPool = it.second;
				delete pPool;
			}
		}
	}
	CHtpMsg AddClient(const string& strUrl, const vector<string>& vecDB)
	{
		try 
		{
			pool* pPool = new pool(uri(strUrl.c_str()));
			for (const auto& it : vecDB)
			{
				m_mapPool[it] = pPool;
			}
		}
		catch (const std::exception& e)
		{
			return CHtpMsg(string(e.what()).append(strUrl), HTP_CID_FAIL|1010);
		}
		return CHtpMsg(HTP_CID_OK);
	}

	//CHtpMsg find(const string& strDB, const string& strColl, const document& key,const options::find opt, vector<view>& vecView)
	//{
	//	if (m_mapPool.find(strDB) == m_mapPool.end())
	//	{
	//		return CHtpMsg(string("CMdbClient::find m_mapDB == null ")
	//			.append(strDB).append("\\").append(strColl), 1010);
	//	}
	//	try
	//	{
	//		//opt.cursor_type(mongocxx::cursor::type::k_tailable); //可尾游标
	//		//opt.no_cursor_timeout();
	//		pool::entry acEntry = m_mapPool[strDB]->acquire();
	//		auto& cursor = (*acEntry)[strDB.c_str()][strColl.c_str()].find(key.view(), opt);
	//		if (cursor.begin() != cursor.end())
	//		{
	//			for (auto& vw : cursor)
	//			{
	//				vecView.push_back(vw);
	//			}
	//		}
	//	}
	//	catch (const std::exception& e)
	//	{
	//		return CHtpMsg(string(strDB).append("\\").append(strColl).append(":").append(e.what()), 1012);
	//	}
	//	return CHtpMsg();
	//}
	CHtpMsg remove(const string& strDB, const string& strColl, const document& key)
	{
		if (m_mapPool.find(strDB) == m_mapPool.end())
		{
			return CHtpMsg(string("CMdbClient::remove m_mapDB == null ")
				.append(strDB).append("\\").append(strColl), HTP_CID_FAIL | 1010);
		}
		try
		{
			pool::entry acEntry = m_mapPool[strDB]->acquire();
			(*acEntry)[strDB.c_str()][strColl.c_str()].delete_many(key.view());
		}
		catch (const std::exception& e)
		{
			return CHtpMsg(string(strDB).append("\\").append(strColl).append(":").append(e.what()), HTP_CID_FAIL | 1012);
		}
		return CHtpMsg(HTP_CID_OK);
	}
	CHtpMsg update(const string& strDB, const string& strColl, const document& key, const document& doc, const options::update& opt)
	{
		if (m_mapPool.find(strDB) == m_mapPool.end())
		{
			return CHtpMsg(string("CMdbClient::update m_mapDB == null ")
				.append(strDB).append("\\").append(strColl), HTP_CID_FAIL|1010);
		}
		try
		{
			pool::entry acEntry = m_mapPool[strDB]->acquire();
			(*acEntry)[strDB.c_str()][strColl.c_str()].update_one(key.view(), doc.view(), opt);
		}
		catch (const std::exception& e)
		{
			return CHtpMsg(string(strDB).append("\\").append(strColl).append(":").append(e.what()), HTP_CID_FAIL|1012);
		}
		return CHtpMsg(HTP_CID_OK);
	}
	CHtpMsg Insert(const string& strDB, const string& strColl, document& doc)
	{
		if (m_mapPool.find(strDB) == m_mapPool.end())
		{
			return CHtpMsg(string("CMdbClient::Insert m_mapDB == null ")
				.append(strDB).append("\\").append(strColl), HTP_CID_FAIL|1010);
		}
		try
		{
			pool::entry acEntry = m_mapPool[strDB]->acquire();
			(*acEntry)[strDB.c_str()][strColl.c_str()].insert_one(doc.view());
		}
		catch (const std::exception& e)
		{
			return CHtpMsg(string(strDB).append("\\").append(strColl).append(":").append(e.what()), HTP_CID_FAIL|1013);
		}
		return CHtpMsg(HTP_CID_OK);
	}
	//CHtpMsg aggregate(const string& strDB, const string& strColl, const pipeline& pipe, const options::aggregate& opt, vector<view>& vecView)
	//{
	//	if (m_mapPool.find(strDB) == m_mapPool.end())
	//	{
	//		return CHtpMsg(string("CMdbClient::aggregate m_mapDB == null ")
	//			.append(strDB).append("\\").append(strColl), 1010);
	//	}
	//	try
	//	{
	//		pool::entry acEntry = m_mapPool[strDB]->acquire();
	//		auto cursor = (*acEntry)[strDB.c_str()][strColl.c_str()].aggregate(pipe, opt);
	//		if (cursor.begin() != cursor.end())
	//		{
	//			for (auto&& vw : cursor)
	//			{
	//				vecView.push_back(vw);
	//			}
	//		}
	//	}
	//	catch (const std::exception& e)
	//	{
	//		return CHtpMsg(string(strDB).append("\\").append(strColl).append(":").append(e.what()), HTP_CID_FAIL|1014);
	//	}
	//	return CHtpMsg(HTP_CID_OK);
	//}

};