#pragma once

#include <mpack/mpack.h>

enum class CppType : uint8_t {
	I8,
	U8,
	I16,
	U16,
	I32,
	U32,
	I64,
	U64,
	F32,
	F64,
	Bool,
	Object,
	ObjectPtr,
	NestedArray
};

enum class MsgType {
	Request, Response, Event, Unknown
};

inline MsgType toType(std::string_view s) {
	if (s == "request")
		return MsgType::Request;
	if (s == "response")
		return MsgType::Response;
	if (s == "event")
		return MsgType::Event;
	return MsgType::Unknown;
}

inline const char* fromType(MsgType t) {
	switch (t) {
	case MsgType::Request:
		return "request";
	case MsgType::Response:
		return "response";
	case MsgType::Event:
		return "event";
	default:
		return "unknown";
	}
}

struct MPackHeader {
	mpack_type_t type;
	uint32_t countOrLength;
	int8_t extType;
};

class MPackObject {
public:
	void read(mpack_reader_t &reader, int depth = 0) {
		if (depth > 32) {
			mpack_reader_flag_error(&r, mpack_error_too_big);
			return;
		}

		MPackHeader header { };
		if (!readHeader(reader, h) || h.type != mpack_type_map) {
			mpack_reader_flag_error(&reader, mpack_error_type);
			return;
		}

		for (size_t i = 0; i < header.countOrLength; i++) {
			MPackHeader keyHeader { };
			if (!readHeader(reader, keyHeader)
					|| keyHeader.type != mpack_type_str) {
				mpack_reader_flag_error(&reader, mpack_error_type);
				return;
			}

			char *key = new char[keyHeader.countOrLength + 1];
			if (keyHeader.countOrLength)
				mpack_read_bytes(&reader, key, keyHeader.countOrLength);
			key[keyHeader.countOrLength] = '\0';
			mpack_done_str(&reader);

			if (isObject(key)) {
				MPackObject *object = getObject(key);
				if (object != nullptr) {
					object->read(reader, depth + 1);
					continue;
				} else {
					mpack_reader_flag_error(&reader, mpack_error_invalid);
					return;
				}
			}

			if (isArray(key)) {
				MPackHeader arrayHeader { };
				if (!readHeader(reader, keyHeader)
						|| keyHeader.type != mpack_type_array) {
					mpack_reader_flag_error(&reader, mpack_error_type);
					return;
				}

				CppType arrayType;
				void *arr = getArray(key, arrayHeader.countOrLength, arrayType);

				bool success = readArray(reader, depth + 1, key, arrayType, arr,
						arrayHeader.countOrLength);

				if (!success) {
					return;
				}
			}

			delete[] key;
		}
	}

	virtual void write(mpack_writer_t &writer, int depth = 0) = 0;

protected:
	static inline bool ok(mpack_reader_t &r) {
		return mpack_reader_error(&r) == mpack_ok;
	}

	static bool readHeader(mpack_reader_t &r, MPackHeader &h) {
		mpack_tag_t t = mpack_read_tag(&r);
		if (!ok(r)) {
			return;
		}

		h.type = mpack_tag_type(&t);
		h.countOrLength = 0;
		h.extType = 0;

		switch (h.type) {
		case mpack_type_map:
			h.countOrLength = mpack_tag_map_count(&t);
			break;
		case mpack_type_array:
			h.countOrLength = mpack_tag_array_count(&t);
			break;
		case mpack_type_str:
			h.countOrLength = mpack_tag_str_length(&t);
			break;
		case mpack_type_bin:
			h.countOrLength = mpack_tag_bin_length(&t);
			break;
		case mpack_type_ext:
			h.countOrLength = mpack_tag_ext_length(&t);
			h.extType = mpack_tag_ext_type(&t);
			break;
		default:
			break;
		}
		return true;
	}

	static bool readMap(const MPackHeader &h) {
		for (uint32_t i = 0; i < h.countOrLength && ok(reader); i++) {
			MPackHeader kh { };
			if (!readHeader(reader, kh) || kh.type != mpack_type_str) {
				mpack_reader_flag_error(&reader, mpack_error_type);
				return T { };
			}

			char *key = new char[kh.countOrLength + 1];
			if (kh.countOrLength)
				mpack_read_bytes(&r, key, kh.countOrLength);
			key[kh.countOrLength] = '\0';
			mpack_done_str (&r);
		}
	}

	bool readArray(mpack_reader_t &reader, int depth, const char *name,
			const CppType &type, void *array, const size_t &size) {

		for (size_t i = 0; i < size; i++) {
			switch (type) {
			case CppType::I8: {
				int8_t *arr = (int8_t*) array;
				arr[i] = mpack_expect_i8(&reader);
			}
				break;

			case CppType::U8: {
				uint8_t *arr = (uint8_t*) array;
				arr[i] = mpack_expect_u8(&reader);
			}
				break;

			case CppType::I16: {
				int16_t *arr = (int16_t*) array;
				arr[i] = mpack_expect_i16(&reader);
			}
				break;

			case CppType::U16: {
				uint16_t *arr = (uint16_t*) array;
				arr[i] = mpack_expect_u16(&reader);
			}
				break;

			case CppType::I32: {
				int32_t *arr = (int32_t*) array;
				arr[i] = mpack_expect_i32(&reader);
			}
				break;

			case CppType::U32: {
				uint32_t *arr = (uint32_t*) array;
				arr[i] = mpack_expect_u32(&reader);
			}
				break;

			case CppType::I64: {
				int64_t *arr = (int64_t*) array;
				arr[i] = mpack_expect_i64(&reader);
			}
				break;

			case CppType::U64: {
				uint64_t *arr = (uint64_t*) array;
				arr[i] = mpack_expect_u64(&reader);
			}
				break;

			case CppType::F32: {
				float *arr = (float*) array;
				arr[i] = mpack_expect_float(&reader);
			}
				break;

			case CppType::F64: {
				double *arr = (double*) array;
				arr[i] = mpack_expect_double(&reader);
			}
				break;

			case CppType::Bool: {
				bool *arr = (bool*) array;
				arr[i] = mpack_expect_bool(&reader);
			}
				break;

			case CppType::Object: {
				MPackObject **arr = (MPackObject**) array;
				MPackObject *obj = getObject(name);
				obj->read(reader, depth + 1);
				arr[i] = obj;
			}
				break;

			case CppType::NestedArray:
				return false;

			default:
				mpack_reader_flag_error(&reader, mpack_error_type);
				return false;
			}
		}

		return mpack_reader_error(&reader) == mpack_ok;
	}

private:
	virtual bool isObject(const char *name) = 0;
	virtual bool isArray(const char *name) = 0;

	virtual MPackObject* getObject(const char *name) = 0;
	virtual void* getArray(const char *name, size_t count,
			mpack_type_t &outType) = 0;

	template<typename T>
	virtual bool setMPackValue(const char *name, T value) = 0;
};

virtual class Payload: public MPackObject {
};

class Message: public MPackObject {
private:
	Message read(mpack_reader_t &reader) override
	{
		MPackHeader header;
		readHeader(reader, header);

	}

public:
	MsgType msgType;
	char *opCode;
	Payload *payload = nullptr;
};
