#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Variable.h"

namespace SE
{
	#define FILESTREAM_BUFFER_SIZE 4096
	#define STREAM_MAX_STRING_LENGTH (4*1024) // 4 kB

	class ReadStream;
	class WriteStream;
	struct Variant;
	struct VariantTypeHandle;
	class ISerializable;
	class ScriptingObject;
	template<typename T>
	class ScriptingObjectReference;
	template<typename T>
	class SoftObjectReference;
	template<typename T>
	class AssetRef;
	template<typename T>
	class WeakAssetRef;
	template<typename T>
	class SoftAssetRef;

	/**
	 * 所有数据流（内存流、文件流等）的基类
	 */
	class SE_API_RUNTIME Stream
	{
	protected:

		bool _hasError;

		Stream() : _hasError(false)
		{
		}

	public:
		virtual ~Stream()
		{
		}

	public:

		/**
		 * 如果在读取写入流期间发生错误，则返回 true
		 * @return
		 */
		virtual bool HasError() const
		{
			return _hasError;
		}

		/**
		 * 如果可以从该流中读取数据，则返回 true。
		 * @return
		 */
		virtual bool CanRead()
		{
			return false;
		}

		/**
		 * 如果可以将数据写入该流缓冲区，则返回 true。
		 * @return
		 */
		virtual bool CanWrite()
		{
			return false;
		}

	public:

		/**
		 * 刷新流缓冲区
		 */
		virtual void Flush() = 0;

		/**
		 * 关闭流缓冲区
		 */
		virtual void Close() = 0;

		/**
		 * 获取流缓冲区的长度
		 * @return
		 */
		virtual uint32 GetLength() = 0;

		/**
		 * 获取流缓冲区中的当前位置
		 * @return
		 */
		virtual uint32 GetPosition() = 0;

		/**
		 * 在流缓冲区中设置新位置
		 * @param seek
		 */
		virtual void SetPosition(uint32 seek) = 0;
	};
}
