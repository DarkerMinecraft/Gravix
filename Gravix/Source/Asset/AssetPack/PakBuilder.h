#pragma once

#include <filesystem>

namespace Gravix 
{

	enum CompressionFlags 
	{
		Zstd, 
		Brotli
	};

	struct PakHeader 
	{
		char     Signature[4]; // "GPAK"
		uint32_t Version;
		uint32_t AssetCount;
		uint64_t TOCOffset;
		CompressionFlags Compression;
	};


	class PakBuilder 
	{

	};

}