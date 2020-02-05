#ifndef OBJREADER_H
#define OBJREADER_H

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <map>
#include <string>
#include <algorithm>
#include <locale>
#include "Vertex.h"
#include "LightHelper.h"


class MtlReader;

class ObjReader
{
public:
	struct ObjPart
	{
		ObjPart() : material() {}
		~ObjPart() = default;

		Material material;							// 材质
		std::vector<VertexPosNormalTex> vertices;	// 顶点集合
		std::vector<WORD> indices16;				// 顶点数不超过65535时使用
		std::vector<DWORD> indices32;				// 顶点数超过65535时使用
		std::wstring texStrDiffuse;					// 漫射光纹理文件名，需为相对路径，在mbo必须占260字节
	};

	ObjReader() : vMin(), vMax() {}
	~ObjReader() = default;

	// 指定.mbo文件的情况下，若.mbo文件存在，优先读取该文件
	// 否则会读取.obj文件
	// 若.obj文件被读取，且提供了.mbo文件的路径，则会根据已经读取的数据创建.mbo文件
	bool Read(const wchar_t* mboFileName, const wchar_t* objFileName);
	
	bool ReadObj(const wchar_t* objFileName);
	bool ReadMbo(const wchar_t* mboFileName);
	bool WriteMbo(const wchar_t* mboFileName);
public:
	std::vector<ObjPart> objParts;
	DirectX::XMFLOAT3 vMin, vMax;					// AABB盒双顶点
private:
	void AddVertex(const VertexPosNormalTex& vertex, DWORD vpi, DWORD vti, DWORD vni);

	// 缓存有v/vt/vn字符串信息
	std::unordered_map<std::wstring, DWORD> vertexCache;
};

class MtlReader
{
public:
	bool ReadMtl(const wchar_t* mtlFileName);


public:
	std::map<std::wstring, Material> materials;
	std::map<std::wstring, std::wstring> mapKdStrs;
};


#endif

