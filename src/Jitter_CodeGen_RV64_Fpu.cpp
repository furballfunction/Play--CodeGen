#include "Jitter_CodeGen_RV64.h"

using namespace Jitter;

void CCodeGen_RV64::Cmp_GetFlag(CRV64Assembler::REGISTER32 registerId, Jitter::CONDITION condition, CRV64Assembler::REGISTERMD src1Reg, CRV64Assembler::REGISTERMD src2Reg)
{
    switch(condition)
    {
    case CONDITION_EQ:
        m_assembler.Feq_1s(registerId, src1Reg, src2Reg);
        break;
    case CONDITION_NE:
        assert(0);
        break;
    case CONDITION_LT:
        assert(0);
        break;
    case CONDITION_LE:
        assert(0);
        break;
    case CONDITION_GT:
        assert(0);
        break;
    case CONDITION_BL:
        m_assembler.Flt_1s(registerId, src1Reg, src2Reg);
        break;
    case CONDITION_BE:
        m_assembler.Fle_1s(registerId, src1Reg, src2Reg);
        break;
    case CONDITION_AB:
        // maybe?
        //assert(0);
        m_assembler.Flt_1s(registerId, src2Reg, src1Reg);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::LoadMemoryFpSingleInRegister(CRV64Assembler::REGISTERMD reg, CSymbol* symbol)
{
    switch(symbol->m_type)
    {
    case SYM_FP_REL_SINGLE:
        m_assembler.Ldr_1s(reg, g_baseRegister, symbol->m_valueLow);
        break;
    case SYM_FP_TMP_SINGLE:
        m_assembler.Ldr_1s(reg, CRV64Assembler::xSP, symbol->m_stackLocation);
        break;
    default:
        assert(false);
        break;
    }
}

void CCodeGen_RV64::StoreRegisterInMemoryFpSingle(CSymbol* symbol, CRV64Assembler::REGISTERMD reg)
{
    switch(symbol->m_type)
    {
    case SYM_FP_REL_SINGLE:
        m_assembler.Str_1s(reg, g_baseRegister, symbol->m_valueLow);
        break;
    case SYM_FP_TMP_SINGLE:
        m_assembler.Str_1s(reg, CRV64Assembler::xSP, symbol->m_stackLocation);
        break;
    default:
        assert(false);
        break;
    }
}

template <typename FPUOP>
void CCodeGen_RV64::Emit_Fpu_MemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegisterMd();

    LoadMemoryFpSingleInRegister(src1Reg, src1);
    ((m_assembler).*(FPUOP::OpReg()))(dstReg, src1Reg);
    StoreRegisterInMemoryFpSingle(dst, dstReg);
}

template <typename FPUOP>
void CCodeGen_RV64::Emit_Fpu_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegisterMd();
    auto src2Reg = GetNextTempRegisterMd();

    LoadMemoryFpSingleInRegister(src1Reg, src1);
    LoadMemoryFpSingleInRegister(src2Reg, src2);
    ((m_assembler).*(FPUOP::OpReg()))(dstReg, src1Reg, src2Reg);
    StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_RV64::Emit_Fp_Cmp_AnyMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = GetNextTempRegisterMd();
    auto src2Reg = GetNextTempRegisterMd();

    LoadMemoryFpSingleInRegister(src1Reg, src1);
    LoadMemoryFpSingleInRegister(src2Reg, src2);
    Cmp_GetFlag(dstReg, statement.jmpCondition, src1Reg, src2Reg);
    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_Fp_Rcpl_MemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegisterMd();
    auto oneReg = GetNextTempRegisterMd();
    auto tmpReg = GetNextTempRegister();

    m_assembler.Li(tmpReg, 0x3f800000);	//Loads 1.0f
    m_assembler.Fmv_1s(oneReg, tmpReg);
    LoadMemoryFpSingleInRegister(src1Reg, src1);
    m_assembler.Fdiv_1s(dstReg, oneReg, src1Reg);
    StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_RV64::Emit_Fp_Rsqrt_MemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegisterMd();
    auto oneReg = GetNextTempRegisterMd();
    auto tmpReg = GetNextTempRegister();

    m_assembler.Li(tmpReg, 0x3f800000);	//Loads 1.0f
    m_assembler.Fmv_1s(oneReg, tmpReg);
    LoadMemoryFpSingleInRegister(src1Reg, src1);
    m_assembler.Fsqrt_1s(src1Reg, src1Reg);
    m_assembler.Fdiv_1s(dstReg, oneReg, src1Reg);
    StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_RV64::Emit_Fp_Clamp_MemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto resultReg = GetNextTempRegisterMd();
    //auto cst1Reg = GetNextTempRegisterMd();
    //auto cst2Reg = GetNextTempRegisterMd();
    auto cst1Reg = GetNextTempRegister();
    auto cst2Reg = GetNextTempRegister();
    auto tmpReg = GetNextTempRegister();

    //m_assembler.Ldr_Pc(cst1Reg, g_fpClampMask1);
    //m_assembler.Ldr_Pc(cst2Reg, g_fpClampMask2);
    m_assembler.Li(cst1Reg, 0x7F7FFFFF);
    m_assembler.Li(cst2Reg, 0xFF7FFFFF);

    //LoadMemoryFpSingleInRegister(resultReg, src1);
    //m_assembler.Smin_4s(resultReg, resultReg, cst1Reg);
    //m_assembler.Umin_4s(resultReg, resultReg, cst2Reg);

    LoadMemoryFpSingleInRegister(resultReg, src1);
    m_assembler.Fmv_x_w(tmpReg, resultReg);

    m_assembler.Smin_1s(tmpReg, tmpReg, cst1Reg);
    m_assembler.Umin_1s(tmpReg, tmpReg, cst2Reg);
    m_assembler.Fmv_1s(resultReg, tmpReg);
    StoreRegisterInMemoryFpSingle(dst, resultReg);
}

void CCodeGen_RV64::Emit_Fp_Mov_MemSRelI32(const STATEMENT& statement)
{
    //assert(false);
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    assert(src1->m_type == SYM_FP_REL_INT32);

    auto dstReg = GetNextTempRegisterMd();
    //auto src1Reg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegister();

    //m_assembler.Ldr_1s(src1Reg, g_baseRegister, src1->m_valueLow);
    //m_assembler.Scvtf_1s(dstReg, src1Reg);

    m_assembler.Lw(src1Reg, g_baseRegister, src1->m_valueLow);
    m_assembler.Fcvtsw_1s(dstReg, src1Reg);

    StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_RV64::Emit_Fp_ToIntTrunc_MemMem(const STATEMENT& statement)
{
    //assert(false);
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegisterMd();

    LoadMemoryFpSingleInRegister(src1Reg, src1);
    //m_assembler.Fcvtzs_1s(dstReg, src1Reg);

    auto tmpReg = GetNextTempRegister();
    m_assembler.Fcvtws_1s(tmpReg, src1Reg);
    m_assembler.Fmv_1s(dstReg, tmpReg);

    StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_RV64::Emit_Fp_LdCst_TmpCst(const STATEMENT& statement)
{
    //assert(false);
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    assert(dst->m_type  == SYM_FP_TMP_SINGLE);
    assert(src1->m_type == SYM_CONSTANT);

    auto tmpReg = GetNextTempRegister();

    LoadConstantInRegister(tmpReg, src1->m_valueLow);
    m_assembler.Str(tmpReg, CRV64Assembler::xSP, dst->m_stackLocation);
}

CCodeGen_RV64::CONSTMATCHER CCodeGen_RV64::g_fpuConstMatchers[] =
{
    { OP_FP_ADD,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    MATCH_NIL, &CCodeGen_RV64::Emit_Fpu_MemMemMem<FPUOP_ADD>    },
    { OP_FP_SUB,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    MATCH_NIL, &CCodeGen_RV64::Emit_Fpu_MemMemMem<FPUOP_SUB>    },
    { OP_FP_MUL,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    MATCH_NIL, &CCodeGen_RV64::Emit_Fpu_MemMemMem<FPUOP_MUL>    },
    { OP_FP_DIV,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    MATCH_NIL, &CCodeGen_RV64::Emit_Fpu_MemMemMem<FPUOP_DIV>    },

    { OP_FP_CMP,            MATCH_ANY,                    MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    MATCH_NIL, &CCodeGen_RV64::Emit_Fp_Cmp_AnyMemMem            },

    { OP_FP_MIN,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    MATCH_NIL, &CCodeGen_RV64::Emit_Fpu_MemMemMem<FPUOP_MIN>    },
    { OP_FP_MAX,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    MATCH_NIL, &CCodeGen_RV64::Emit_Fpu_MemMemMem<FPUOP_MAX>    },

    { OP_FP_RCPL,           MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 MATCH_NIL, &CCodeGen_RV64::Emit_Fp_Rcpl_MemMem              },
    { OP_FP_SQRT,           MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 MATCH_NIL, &CCodeGen_RV64::Emit_Fpu_MemMem<FPUOP_SQRT>      },
    { OP_FP_RSQRT,          MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 MATCH_NIL, &CCodeGen_RV64::Emit_Fp_Rsqrt_MemMem             },

    { OP_FP_CLAMP,          MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 MATCH_NIL, &CCodeGen_RV64::Emit_Fp_Clamp_MemMem             },

    { OP_FP_ABS,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 MATCH_NIL, &CCodeGen_RV64::Emit_Fpu_MemMem<FPUOP_ABS>       },
    { OP_FP_NEG,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 MATCH_NIL, &CCodeGen_RV64::Emit_Fpu_MemMem<FPUOP_NEG>       },

    { OP_MOV,               MATCH_MEMORY_FP_SINGLE,       MATCH_RELATIVE_FP_INT32,    MATCH_NIL,                 MATCH_NIL, &CCodeGen_RV64::Emit_Fp_Mov_MemSRelI32           },
    { OP_FP_TOINT_TRUNC,    MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 MATCH_NIL, &CCodeGen_RV64::Emit_Fp_ToIntTrunc_MemMem        },

    { OP_FP_LDCST,          MATCH_TEMPORARY_FP_SINGLE,    MATCH_CONSTANT,             MATCH_NIL,                 MATCH_NIL, &CCodeGen_RV64::Emit_Fp_LdCst_TmpCst             },

    { OP_MOV,               MATCH_NIL,                    MATCH_NIL,                  MATCH_NIL,                 MATCH_NIL, nullptr                                             },
};
