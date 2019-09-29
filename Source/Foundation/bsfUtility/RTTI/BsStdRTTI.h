//************************************ bs::framework - Copyright 2018 Marko Pintera **************************************//
//*********** Licensed under the MIT license. See LICENSE.md for full terms. This notice is not to be removed. ***********//
#pragma once

#include "Prerequisites/BsPrerequisitesUtil.h"
#include "Reflection/BsRTTIPlain.h"

namespace bs
{
	struct RTTIFieldInfo;
	
	/** @cond RTTI */
	/** @addtogroup RTTI-Impl-Utility
	*  @{
	*/

	/**
	 * RTTIPlainType for std::vector.
	 *
	 * @see		RTTIPlainType
	 */
	template<class T> struct RTTIPlainType<std::vector<T, StdAlloc<T>>>
	{
		enum { id = TID_Vector }; enum { hasDynamicSize = 1 };

		/** @copydoc RTTIPlainType::toMemory */
		static uint32_t toMemory(const std::vector<T, StdAlloc<T>>& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			return rtti_write_with_size_header(stream, [&data, &stream]()
			{
				uint32_t size = 0;

				auto numElements = (uint32_t)data.size();
				size += rtti_write(numElements, stream);

				for (const auto& item : data)
					size += rtti_write(item, stream);

				return size;
			});
		}

		/** @copydoc RTTIPlainType::fromMemory */
		static uint32_t fromMemory(std::vector<T, StdAlloc<T>>& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			uint32_t size = 0;
			rtti_read(size, stream);

			uint32_t numElements;
			rtti_read(numElements, stream);

			data.clear();
			for (uint32_t i = 0; i < numElements; i++)
			{
				T element;
				rtti_read(element, stream);

				data.push_back(element);
			}

			return size;
		}

		/** @copydoc RTTIPlainType::getDynamicSize */
		static uint32_t getDynamicSize(const std::vector<T, StdAlloc<T>>& data)
		{
			uint64_t dataSize = sizeof(uint32_t) * 2;

			for (const auto& item : data)
				dataSize += rtti_size(item);

			assert(dataSize <= std::numeric_limits<uint32_t>::max());

			return (uint32_t)dataSize;
		}
	};

	/**
	 * RTTIPlainType for std::list.
	 *
	 * @see		RTTIPlainType
	 */
	template<class T> struct RTTIPlainType<std::list<T, StdAlloc<T>>>
	{
		enum { id = TID_List }; enum { hasDynamicSize = 1 };

		/** @copydoc RTTIPlainType::toMemory */
		static uint32_t toMemory(const std::list<T, StdAlloc<T>>& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			return rtti_write_with_size_header(stream, [&data, &stream]()
			{
				uint32_t size = 0;

				auto numElements = (uint32_t)data.size();
				size += rtti_write(numElements, stream);

				for (const auto& item : data)
					size += rtti_write(item, stream);

				return size;
			});
		}

		/** @copydoc RTTIPlainType::fromMemory */
		static uint32_t fromMemory(std::list<T, StdAlloc<T>>& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			uint32_t size = 0;
			rtti_read(size, stream);

			uint32_t numElements;
			rtti_read(numElements, stream);

			for (uint32_t i = 0; i < numElements; i++)
			{
				T element;
				rtti_read(element, stream);

				data.push_back(element);
			}

			return size;
		}

		/** @copydoc RTTIPlainType::getDynamicSize */
		static uint32_t getDynamicSize(const std::list<T, StdAlloc<T>>& data)
		{
			uint64_t dataSize = sizeof(uint32_t) * 2;

			for (const auto& item : data)
				dataSize += rtti_size(item);

			assert(dataSize <= std::numeric_limits<uint32_t>::max());

			return (uint32_t)dataSize;
		}
	};

	/**
	 * RTTIPlainType for std::set.
	 *
	 * @see		RTTIPlainType
	 */
	template<class T> struct RTTIPlainType<std::set<T, std::less<T>, StdAlloc<T>>>
	{
		enum { id = TID_Set }; enum { hasDynamicSize = 1 };

		/** @copydoc RTTIPlainType::toMemory */
		static uint32_t toMemory(const std::set<T, std::less<T>, StdAlloc<T>>& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			return rtti_write_with_size_header(stream, [&data, &stream]()
			{
				uint32_t size = 0;

				auto numElements = (uint32_t)data.size();
				size += rtti_write(numElements, stream);

				for (const auto& item : data)
					size += rtti_write(item, stream);

				return size;
			});
		}

		/** @copydoc RTTIPlainType::fromMemory */
		static uint32_t fromMemory(std::set<T, std::less<T>, StdAlloc<T>>& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			uint32_t size = 0;
			rtti_read(size, stream);

			uint32_t numElements;
			rtti_read(numElements, stream);

			for (uint32_t i = 0; i < numElements; i++)
			{
				T element;
				rtti_read(element, stream);
				data.insert(element);
			}

			return size;
		}

		/** @copydoc RTTIPlainType::getDynamicSize */
		static uint32_t getDynamicSize(const std::set<T, std::less<T>, StdAlloc<T>>& data)
		{
			uint64_t dataSize = sizeof(uint32_t) * 2;

			for (const auto& item : data)
				dataSize += rtti_size(item);

			assert(dataSize <= std::numeric_limits<uint32_t>::max());

			return (uint32_t)dataSize;
		}
	};

	/**
	 * RTTIPlainType for std::map.
	 *
	 * @see		RTTIPlainType
	 */
	template<class Key, class Value> struct RTTIPlainType<std::map<Key, Value, std::less<Key>, StdAlloc<std::pair<const Key, Value>>>>
	{
		enum { id = TID_Map }; enum { hasDynamicSize = 1 };

		/** @copydoc RTTIPlainType::toMemory */
		static uint32_t toMemory(const std::map<Key, Value, std::less<Key>, StdAlloc<std::pair<const Key, Value>>>& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			return rtti_write_with_size_header(stream, [&data, &stream]()
			{
				uint32_t size = 0;

				auto numElements = (uint32_t)data.size();
				size += rtti_write(numElements, stream);

				for (const auto& item : data)
				{
					size += rtti_write(item.first, stream);
					size += rtti_write(item.second, stream);
				}

				return size;
			});
		}

		/** @copydoc RTTIPlainType::fromMemory */
		static uint32_t fromMemory(std::map<Key, Value, std::less<Key>, StdAlloc<std::pair<const Key, Value>>>& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			uint32_t size = 0;
			rtti_read(size, stream);

			uint32_t numElements;
			rtti_read(numElements, stream);

			for (uint32_t i = 0; i < numElements; i++)
			{
				Key key;
				rtti_read(key, stream);

				Value value;
				rtti_read(value, stream);

				data[key] = value;
			}

			return size;
		}

		/** @copydoc RTTIPlainType::getDynamicSize */
		static uint32_t getDynamicSize(const std::map<Key, Value, std::less<Key>, StdAlloc<std::pair<const Key, Value>>>& data)
		{
			uint64_t dataSize = sizeof(uint32_t) * 2;

			for (const auto& item : data)
			{
				dataSize += rtti_size(item.first);
				dataSize += rtti_size(item.second);
			}

			assert(dataSize <= std::numeric_limits<uint32_t>::max());

			return (uint32_t)dataSize;
		}
	};

	/**
	 * RTTIPlainType for std::unordered_map.
	 *
	 * @see		RTTIPlainType
	 */
	template<class Key, class Value>
	struct RTTIPlainType<std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, StdAlloc<std::pair<const Key, Value>>>>
	{
		enum { id = TID_UnorderedMap }; enum { hasDynamicSize = 1 };

		typedef std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, StdAlloc<std::pair<const Key, Value>>> MapType;

		/** @copydoc RTTIPlainType::toMemory */
		static uint32_t toMemory(const MapType& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			return rtti_write_with_size_header(stream, [&data, &stream]()
			{
				uint32_t size = 0;

				auto numElements = (uint32_t)data.size();
				size += rtti_write(numElements, stream);

				for (const auto& item : data)
				{
					size += rtti_write(item.first, stream);
					size += rtti_write(item.second, stream);
				}

				return size;
			});
		}

		/** @copydoc RTTIPlainType::fromMemory */
		static uint32_t fromMemory(MapType& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			uint32_t size = 0;
			rtti_read(size, stream);

			uint32_t numElements;
			rtti_read(numElements, stream);

			for (uint32_t i = 0; i < numElements; i++)
			{
				Key key;
				rtti_read(key, stream);

				Value value;
				rtti_read(value, stream);

				data[key] = value;
			}

			return size;
		}

		/** @copydoc RTTIPlainType::getDynamicSize */
		static uint32_t getDynamicSize(const MapType& data)
		{
			uint64_t dataSize = sizeof(uint32_t) * 2;

			for (const auto& item : data)
			{
				dataSize += rtti_size(item.first);
				dataSize += rtti_size(item.second);
			}

			assert(dataSize <= std::numeric_limits<uint32_t>::max());

			return (uint32_t)dataSize;
		}
	};

	/**
	 * RTTIPlainType for std::unordered_set.
	 *
	 * @see		RTTIPlainType
	 */
	template<class Key>
	struct RTTIPlainType<std::unordered_set<Key, std::hash<Key>, std::equal_to<Key>, StdAlloc<Key>>>
	{
		enum { id = TID_UnorderedSet }; enum { hasDynamicSize = 1 };

		typedef std::unordered_set<Key, std::hash<Key>, std::equal_to<Key>, StdAlloc<Key>> MapType;

		/** @copydoc RTTIPlainType::toMemory */
		static uint32_t toMemory(const MapType& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			return rtti_write_with_size_header(stream, [&data, &stream]()
			{
				uint32_t size = 0;

				auto numElements = (uint32_t)data.size();
				size += rtti_write(numElements, stream);

				for (const auto& item : data)
					size += rtti_write(item, stream);

				return size;
			});
		}

		/** @copydoc RTTIPlainType::fromMemory */
		static uint32_t fromMemory(MapType& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			uint32_t size = 0;
			rtti_read(size, stream);

			uint32_t numElements;
			rtti_read(numElements, stream);

			for (uint32_t i = 0; i < numElements; i++)
			{
				Key key;
				rtti_read(key, stream);

				data.insert(key);
			}

			return size;
		}

		/** @copydoc RTTIPlainType::getDynamicSize */
		static uint32_t getDynamicSize(const MapType& data)
		{
			uint64_t dataSize = sizeof(uint32_t) * 2;

			for (const auto& item : data)
			{
				dataSize += rtti_size(item);
			}

			assert(dataSize <= std::numeric_limits<uint32_t>::max());

			return (uint32_t)dataSize;
		}
	};

	/**
	 * RTTIPlainType for std::pair.
	 *
	 * @see		RTTIPlainType
	 */
	template<class A, class B> struct RTTIPlainType<std::pair<A, B>>
	{
		enum { id = TID_Pair }; enum { hasDynamicSize = 1 };

		/** @copydoc RTTIPlainType::toMemory */
		static uint32_t toMemory(const std::pair<A, B>& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			return rtti_write_with_size_header(stream, [&data, &stream]()
			{
				uint32_t size = 0;
				size += rtti_write(data.first, stream);
				size += rtti_write(data.second, stream);

				return size;
			});
		}

		/** @copydoc RTTIPlainType::fromMemory */
		static uint32_t fromMemory(std::pair<A, B>& data, Bitstream& stream, const RTTIFieldInfo& fieldInfo, bool compress)
		{
			uint32_t size = 0;
			rtti_read(size, stream);
			rtti_read(data.first, stream);
			rtti_read(data.second, stream);

			return size;
		}

		/** @copydoc RTTIPlainType::getDynamicSize */
		static uint32_t getDynamicSize(const std::pair<A, B>& data)
		{
			uint64_t dataSize = sizeof(uint32_t);
			dataSize += rtti_size(data.first);
			dataSize += rtti_size(data.second);

			assert(dataSize <= std::numeric_limits<uint32_t>::max());

			return (uint32_t)dataSize;
		}
	};

	/** @} */
	/** @endcond */
}
