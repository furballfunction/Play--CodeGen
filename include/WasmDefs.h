#pragma once

namespace Wasm
{
	enum
	{
		BINARY_MAGIC = 0x6D736100,
		BINARY_VERSION = 0x01,
	};

	enum SECTION_ID : uint8
	{
		SECTION_ID_TYPE = 0x01,
		SECTION_ID_IMPORT = 0x02,
		SECTION_ID_FUNCTION = 0x03,
		SECTION_ID_EXPORT = 0x07,
		SECTION_ID_CODE = 0x0A,
	};

	enum IMPORT_EXPORT_TYPE
	{
		IMPORT_EXPORT_TYPE_FUNCTION = 0x00,
		IMPORT_EXPORT_TYPE_TABLE = 0x01,
		IMPORT_EXPORT_TYPE_MEMORY = 0x02,
	};

	enum TYPE_CODE
	{
		TYPE_I32 = 0x7F,
		TYPE_I64 = 0x7E
	};

	enum INST_PREFIX
	{
		INST_PREFIX_SIMD = 0xFD,
	};

	enum INST_CODE
	{
		INST_BLOCK = 0x02,
		INST_IF = 0x04,
		INST_ELSE = 0x05,
		INST_BR = 0x0C,
		INST_BR_IF = 0x0D,
		INST_END = 0x0B,
		INST_CALL_INDIRECT = 0x11,
		INST_LOCAL_GET = 0x20,
		INST_LOCAL_SET = 0x21,
		INST_I32_LOAD = 0x28,
		INST_I64_LOAD = 0x29,
		INST_F32_LOAD = 0x2A,
		INST_I32_LOAD8_U = 0x2D,
		INST_I32_LOAD16_U = 0x2F,
		INST_I32_STORE = 0x36,
		INST_I64_STORE = 0x37,
		INST_F32_STORE = 0x38,
		INST_I32_STORE8 = 0x3A,
		INST_I32_STORE16 = 0x3B,
		INST_BLOCKTYPE_VOID = 0x40,
		INST_I32_CONST = 0x41,
		INST_I64_CONST = 0x42,
		INST_I32_EQZ = 0x45,
		INST_I32_EQ = 0x46,
		INST_I32_NE = 0x47,
		INST_I32_LT_S = 0x48,
		INST_I32_LT_U = 0x49,
		INST_I32_GT_S = 0x4A,
		INST_I32_GT_U = 0x4B,
		INST_I32_LE_S = 0x4C,
		INST_I32_LE_U = 0x4D,
		INST_I32_GE_S = 0x4E,
		INST_I32_GE_U = 0x4F,
		INST_I64_EQ = 0x51,
		INST_I64_NE = 0x52,
		INST_I64_LT_S = 0x53,
		INST_I64_LT_U = 0x54,
		INST_I64_GT_S = 0x55,
		INST_I64_GT_U = 0x56,
		INST_I64_LE_S = 0x57,
		INST_I64_LE_U = 0x58,
		INST_I64_GE_S = 0x59,
		INST_I64_GE_U = 0x5A,
		INST_I32_ADD = 0x6A,
		INST_I32_SUB = 0x6B,
		INST_I32_DIV_S = 0x6D,
		INST_I32_DIV_U = 0x6E,
		INST_I32_REM_S = 0x6F,
		INST_I32_REM_U = 0x70,
		INST_I32_AND = 0x71,
		INST_I32_OR = 0x72,
		INST_I32_XOR = 0x73,
		INST_I32_SHL = 0x74,
		INST_I32_SHR_S = 0x75,
		INST_I32_SHR_U = 0x76,
		INST_I64_ADD = 0x7C,
		INST_I64_SUB = 0x7D,
		INST_I64_MUL = 0x7E,
		INST_I64_AND = 0x83,
		INST_I64_OR = 0x84,
		INST_I64_SHL = 0x86,
		INST_I64_SHR_S = 0x87,
		INST_I64_SHR_U = 0x88,
		INST_F32_ADD = 0x92,
		INST_F32_MUL = 0x94,
		INST_I32_WRAP_I64 = 0xA7,
		INST_I32_TRUNC_F32_S = 0xA8,
		INST_I64_EXTEND_I32_S = 0xAC,
		INST_I64_EXTEND_I32_U = 0xAD,
		INST_F32_CONVERT_I32_S = 0xB2,
	};

	enum INST_CODE_SIMD
	{
		INST_V128_LOAD = 0x00,
		INST_V128_STORE = 0x0B,
		INST_I32x4_SPLAT = 0x11,
		INST_V128_NOT = 0x4D,
		INST_V128_AND = 0x4E,
		INST_V128_OR = 0x50,
		INST_V128_XOR = 0x51,
		INST_I8x16_ADD = 0x6E,
		INST_I8x16_ADD_SAT_S = 0x6F,
		INST_I8x16_ADD_SAT_U = 0x70,
	};
}
