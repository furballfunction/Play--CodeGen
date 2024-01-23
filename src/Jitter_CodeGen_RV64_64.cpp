#include "Jitter_CodeGen_RV64.h"

using namespace Jitter;

uint32 CCodeGen_RV64::GetMemory64Offset(CSymbol* symbol) const
{
    switch(symbol->m_type)
    {
    case SYM_RELATIVE64:
        return symbol->m_valueLow;
        break;
    case SYM_TEMPORARY64:
        return symbol->m_stackLocation;
        break;
    default:
        assert(false);
        return 0;
        break;
    }
}

void CCodeGen_RV64::LoadMemory64InRegister(CRV64Assembler::REGISTER64 registerId, CSymbol* src)
{
    switch(src->m_type)
    {
    case SYM_RELATIVE64:
        assert((src->m_valueLow & 0x07) == 0x00);
        m_assembler.Ld(registerId, g_baseRegister, src->m_valueLow);
        break;
    case SYM_TEMPORARY64:
        assert((src->m_stackLocation & 0x07) == 0x00);
        m_assembler.Ld(registerId, CRV64Assembler::xSP, src->m_stackLocation);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::StoreRegisterInMemory64(CSymbol* dst, CRV64Assembler::REGISTER64 registerId)
{
    switch(dst->m_type)
    {
    case SYM_RELATIVE64:
        assert((dst->m_valueLow & 0x07) == 0x00);
        m_assembler.Str(registerId, g_baseRegister, dst->m_valueLow);
        break;
    case SYM_TEMPORARY64:
        assert((dst->m_stackLocation & 0x07) == 0x00);
        m_assembler.Str(registerId, CRV64Assembler::xSP, dst->m_stackLocation);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::LoadConstant64InRegister(CRV64Assembler::REGISTER64 registerId, uint64 constant)
{
    /*if(constant == 0)
    {
        m_assembler.Movz(registerId, 0, 0);
        return;
    }
    //TODO: Check if "movn" could be used to load constants
    bool loaded = false;
    static const uint64 masks[4] =
    {
        0x000000000000FFFFULL,
        0x00000000FFFF0000ULL,
        0x0000FFFF00000000ULL,
        0xFFFF000000000000ULL
    };
    for(unsigned int i = 0; i < 4; i++)
    {
        if((constant & masks[i]) != 0)
        {
            if(loaded)
            {
                m_assembler.Movk(registerId, constant >> (i * 16), i);
            }
            else
            {
                m_assembler.Movz(registerId, constant >> (i * 16), i);
            }
            loaded = true;
        }
    }
    assert(loaded);*/
    m_assembler.Li(registerId, constant);
}

void CCodeGen_RV64::LoadMemory64LowInRegister(CRV64Assembler::REGISTER32 registerId, CSymbol* symbol)
{
    switch(symbol->m_type)
    {
    case SYM_RELATIVE64:
        m_assembler.Lw(registerId, g_baseRegister, symbol->m_valueLow + 0);
        break;
    case SYM_TEMPORARY64:
        m_assembler.Lw(registerId, CRV64Assembler::xSP, symbol->m_stackLocation + 0);
        break;
    default:
        assert(false);
        break;
    }
}

void CCodeGen_RV64::LoadMemory64HighInRegister(CRV64Assembler::REGISTER32 registerId, CSymbol* symbol)
{
    switch(symbol->m_type)
    {
    case SYM_RELATIVE64:
        m_assembler.Lw(registerId, g_baseRegister, symbol->m_valueLow + 4);
        break;
    case SYM_TEMPORARY64:
        m_assembler.Lw(registerId, CRV64Assembler::xSP, symbol->m_stackLocation + 4);
        break;
    default:
        assert(false);
        break;
    }
}

void CCodeGen_RV64::LoadSymbol64InRegister(CRV64Assembler::REGISTER64 registerId, CSymbol* symbol)
{
    switch(symbol->m_type)
    {
    case SYM_RELATIVE64:
    case SYM_TEMPORARY64:
        LoadMemory64InRegister(registerId, symbol);
        break;
    case SYM_CONSTANT64:
        LoadConstant64InRegister(registerId, symbol->GetConstant64());
        break;
    default:
        assert(false);
        break;
    }
}

void CCodeGen_RV64::StoreRegistersInMemory64(CSymbol* symbol, CRV64Assembler::REGISTER32 regLo, CRV64Assembler::REGISTER32 regHi)
{
    if(GetMemory64Offset(symbol) < 0x100)
    {
        switch(symbol->m_type)
        {
        case SYM_RELATIVE64:
            m_assembler.Stp(regLo, regHi, g_baseRegister, symbol->m_valueLow);
            break;
        case SYM_TEMPORARY64:
            m_assembler.Stp(regLo, regHi, CRV64Assembler::xSP, symbol->m_stackLocation);
            break;
        default:
            assert(false);
            break;
        }
    }
    else
    {
        switch(symbol->m_type)
        {
        case SYM_RELATIVE64:
            m_assembler.Str(regLo, g_baseRegister, symbol->m_valueLow + 0);
            m_assembler.Str(regHi, g_baseRegister, symbol->m_valueLow + 4);
            break;
        case SYM_TEMPORARY64:
            m_assembler.Str(regLo, CRV64Assembler::xSP, symbol->m_stackLocation + 0);
            m_assembler.Str(regHi, CRV64Assembler::xSP, symbol->m_stackLocation + 4);
            break;
        default:
            assert(false);
            break;
        }
    }
}

void CCodeGen_RV64::Emit_ExtLow64VarMem64(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    LoadMemory64LowInRegister(dstReg, src1);
    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_ExtHigh64VarMem64(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    LoadMemory64HighInRegister(dstReg, src1);
    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_MergeTo64_Mem64AnyAny(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto regLo = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    auto regHi = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

    StoreRegistersInMemory64(dst, regLo, regHi);
}

void CCodeGen_RV64::Emit_LoadFromRef_64_MemVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto dstReg = GetNextTempRegister64();

    m_assembler.Ld(dstReg, addressReg, 0);

    StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_RV64::Emit_LoadFromRef_64_MemVarAny(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert(scale == 1);

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto dstReg = GetNextTempRegister64();

    if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex >= -2048) && (scaledIndex <= 2047))
    {
        m_assembler.Ld(dstReg, addressReg, scaledIndex);
    }
    else
    {
        auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        //m_assembler.Ldr(dstReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), (scale == 8));
        auto tmpReg = GetNextTempRegister64();
        //auto indexReg = PrepareSymbolRegisterUseRef(src2, GetNextTempRegister64());
        if (scale == 8) {
            //assert(0);
            m_assembler.Slli(tmpReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), 3);
            m_assembler.Add(tmpReg, tmpReg, addressReg);
        } else {
            m_assembler.Add(tmpReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg));
        }
        m_assembler.Ld(dstReg, tmpReg, 0);
    }

    StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_RV64::Emit_StoreAtRef_64_VarAny(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto valueReg = GetNextTempRegister64();

    LoadSymbol64InRegister(valueReg, src2);
    m_assembler.Str(valueReg, addressReg, 0);
}

void CCodeGen_RV64::Emit_StoreAtRef_64_VarAnyAny(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    auto src3 = statement.src3->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert(scale == 1);

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto valueReg = GetNextTempRegister64();

    LoadSymbol64InRegister(valueReg, src3);

    if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex >= -2048) && (scaledIndex <= 2047))
    {
        //m_assembler.Str(valueReg, addressReg, scaledIndex);
        m_assembler.Sd(addressReg, valueReg, scaledIndex);
    }
    else
    {
        auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        //m_assembler.Str(valueReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), (scale == 8));
        auto tmpReg = GetNextTempRegister64();
        //auto indexReg = PrepareSymbolRegisterUseRef(src2, GetNextTempRegister64());
        if (scale == 8) {
            //assert(0);
            m_assembler.Slli(tmpReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), 3);
            m_assembler.Add(tmpReg, tmpReg, addressReg);
        } else {
            m_assembler.Add(tmpReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg));
        }
        m_assembler.Sd(tmpReg, valueReg, 0);
    }
}

void CCodeGen_RV64::Emit_Add64_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = GetNextTempRegister64();
    auto src1Reg = GetNextTempRegister64();
    auto src2Reg = GetNextTempRegister64();

    LoadMemory64InRegister(src1Reg, src1);
    LoadMemory64InRegister(src2Reg, src2);
    m_assembler.Add(dstReg, src1Reg, src2Reg);
    StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_RV64::Emit_Add64_MemMemCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = GetNextTempRegister64();
    auto src1Reg = GetNextTempRegister64();

    LoadMemory64InRegister(src1Reg, src1);
    auto constant = src2->GetConstant64();

    ADDSUB_IMM_PARAMS addSubImmParams;
    if(TryGetAddSub64ImmParams(static_cast<int64>(constant), addSubImmParams))
    {
        m_assembler.Addi(dstReg, src1Reg, addSubImmParams.imm);
    }
    else
    {
        auto src2Reg = GetNextTempRegister64();
        LoadConstant64InRegister(src2Reg, constant);
        m_assembler.Add(dstReg, src1Reg, src2Reg);
    }

    StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_RV64::Emit_Sub64_MemAnyMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = GetNextTempRegister64();
    auto src1Reg = GetNextTempRegister64();
    auto src2Reg = GetNextTempRegister64();

    LoadSymbol64InRegister(src1Reg, src1);
    LoadMemory64InRegister(src2Reg, src2);
    m_assembler.Sub(dstReg, src1Reg, src2Reg);
    StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_RV64::Emit_Sub64_MemMemCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = GetNextTempRegister64();
    auto src1Reg = GetNextTempRegister64();

    LoadMemory64InRegister(src1Reg, src1);
    auto constant = src2->GetConstant64();

    ADDSUB_IMM_PARAMS addSubImmParams;
    if(TryGetAddSub64ImmParams(-static_cast<int64>(constant), addSubImmParams))
    {
        m_assembler.Addi(dstReg, src1Reg, addSubImmParams.imm);
    }
    else
    {
        auto src2Reg = GetNextTempRegister64();
        LoadConstant64InRegister(src2Reg, constant);
        m_assembler.Sub(dstReg, src1Reg, src2Reg);
    }

    StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_RV64::Cmp_GetFlag(CRV64Assembler::REGISTER32 registerId, Jitter::CONDITION condition, CRV64Assembler::REGISTER64 src1Reg, CRV64Assembler::REGISTER64 src2Reg)
{
    switch(condition)
    {
    case CONDITION_EQ:
        //m_assembler.XorSltu(static_cast<CRV64Assembler::REGISTER64>(registerId), src1Reg, src2Reg);
        //m_assembler.Sltiu(static_cast<CRV64Assembler::REGISTER64>(registerId), static_cast<CRV64Assembler::REGISTER64>(registerId), 1);
        m_assembler.Sub(static_cast<CRV64Assembler::REGISTER64>(registerId), src1Reg, src2Reg);
        m_assembler.Sltiu(static_cast<CRV64Assembler::REGISTER64>(registerId), static_cast<CRV64Assembler::REGISTER64>(registerId), 1);
        break;
    case CONDITION_NE:
        //m_assembler.XorSltu(static_cast<CRV64Assembler::REGISTER64>(registerId), src1Reg, src2Reg);
        m_assembler.Sub(static_cast<CRV64Assembler::REGISTER64>(registerId), src1Reg, src2Reg);
        m_assembler.Sltu(static_cast<CRV64Assembler::REGISTER64>(registerId), CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(registerId));
        break;
    case CONDITION_LT:
        m_assembler.Slt(static_cast<CRV64Assembler::REGISTER64>(registerId), src1Reg, src2Reg);
        break;
    case CONDITION_LE:
        m_assembler.Slt(static_cast<CRV64Assembler::REGISTER64>(registerId), src2Reg, src1Reg);
        m_assembler.Sltiu(static_cast<CRV64Assembler::REGISTER64>(registerId), static_cast<CRV64Assembler::REGISTER64>(registerId), 1);
        break;
    case CONDITION_GT:
        m_assembler.Slt(static_cast<CRV64Assembler::REGISTER64>(registerId), src2Reg, src1Reg);
        break;
    case CONDITION_BL:
        m_assembler.Sltu(static_cast<CRV64Assembler::REGISTER64>(registerId), src1Reg, src2Reg);
        break;
    case CONDITION_BE:
        m_assembler.Sltu(static_cast<CRV64Assembler::REGISTER64>(registerId), src2Reg, src1Reg);
        m_assembler.Sltiu(static_cast<CRV64Assembler::REGISTER64>(registerId), static_cast<CRV64Assembler::REGISTER64>(registerId), 1);
        break;
    case CONDITION_AB:
        m_assembler.Sltu(static_cast<CRV64Assembler::REGISTER64>(registerId), src2Reg, src1Reg);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::Emit_Cmp64_VarAnyMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = GetNextTempRegister64();
    auto src2Reg = GetNextTempRegister64();

    LoadMemory64InRegister(src1Reg, src1);
    LoadMemory64InRegister(src2Reg, src2);
    Cmp_GetFlag(dstReg, statement.jmpCondition, src1Reg, src2Reg);
    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_Cmp64_VarMemCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(src2->m_type == SYM_CONSTANT64);

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = GetNextTempRegister64();

    LoadMemory64InRegister(src1Reg, src1);
    uint64 src2Cst = src2->GetConstant64();

    /*ADDSUB_IMM_PARAMS addSubImmParams;
    if(TryGetAddSub64ImmParams(src2Cst, addSubImmParams))
    {
        m_assembler.Cmp(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
    }
    else if(TryGetAddSub64ImmParams(-static_cast<int64>(src2Cst), addSubImmParams))
    {
        m_assembler.Cmn(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
    }
    else
    {
        auto src2Reg = GetNextTempRegister64();
        LoadConstant64InRegister(src2Reg, src2Cst);
        m_assembler.Cmp(src1Reg, src2Reg);
    }*/

    auto src2Reg = GetNextTempRegister64();
    LoadConstant64InRegister(src2Reg, src2Cst);

    Cmp_GetFlag(dstReg, statement.jmpCondition, src1Reg, src2Reg);
    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_And64_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = GetNextTempRegister64();
    auto src1Reg = GetNextTempRegister64();
    auto src2Reg = GetNextTempRegister64();

    LoadMemory64InRegister(src1Reg, src1);
    LoadMemory64InRegister(src2Reg, src2);
    m_assembler.And(dstReg, src1Reg, src2Reg);
    StoreRegisterInMemory64(dst, dstReg);
}

template <typename Shift64Op>
void CCodeGen_RV64::Emit_Shift64_MemMemVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = GetNextTempRegister64();
    auto src1Reg = GetNextTempRegister64();
    auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

    LoadMemory64InRegister(src1Reg, src1);
    ((m_assembler).*(Shift64Op::OpReg()))(dstReg, src1Reg, static_cast<CRV64Assembler::REGISTER64>(src2Reg));
    StoreRegisterInMemory64(dst, dstReg);
}

template <typename Shift64Op>
void CCodeGen_RV64::Emit_Shift64_MemMemCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(src2->m_type == SYM_CONSTANT);

    auto dstReg = GetNextTempRegister64();
    auto src1Reg = GetNextTempRegister64();

    LoadMemory64InRegister(src1Reg, src1);
    ((m_assembler).*(Shift64Op::OpImm()))(dstReg, src1Reg, src2->m_valueLow);
    StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_RV64::Emit_Mov_Mem64Mem64(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    LoadMemory64InRegister(tmpReg, src1);
    StoreRegisterInMemory64(dst, tmpReg);
}

void CCodeGen_RV64::Emit_Mov_Mem64Cst64(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    LoadConstant64InRegister(tmpReg, src1->GetConstant64());
    StoreRegisterInMemory64(dst, tmpReg);
}

CCodeGen_RV64::CONSTMATCHER CCodeGen_RV64::g_64ConstMatchers[] =
{
    { OP_EXTLOW64,       MATCH_VARIABLE,       MATCH_MEMORY64,       MATCH_NIL,           MATCH_NIL, &CCodeGen_RV64::Emit_ExtLow64VarMem64                    },
    { OP_EXTHIGH64,      MATCH_VARIABLE,       MATCH_MEMORY64,       MATCH_NIL,           MATCH_NIL, &CCodeGen_RV64::Emit_ExtHigh64VarMem64                   },

    { OP_MERGETO64,      MATCH_MEMORY64,       MATCH_ANY,            MATCH_ANY,           MATCH_NIL, &CCodeGen_RV64::Emit_MergeTo64_Mem64AnyAny               },

    { OP_LOADFROMREF,    MATCH_MEMORY64,       MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL, &CCodeGen_RV64::Emit_LoadFromRef_64_MemVar               },
    { OP_LOADFROMREF,    MATCH_MEMORY64,       MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL, &CCodeGen_RV64::Emit_LoadFromRef_64_MemVarAny            },

    { OP_STOREATREF,     MATCH_NIL,            MATCH_VAR_REF,        MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_RV64::Emit_StoreAtRef_64_VarAny                },
    { OP_STOREATREF,     MATCH_NIL,            MATCH_VAR_REF,        MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_RV64::Emit_StoreAtRef_64_VarAny                },

    { OP_STOREATREF,     MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY32,         MATCH_MEMORY64,   &CCodeGen_RV64::Emit_StoreAtRef_64_VarAnyAny      },
    { OP_STOREATREF,     MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY32,         MATCH_CONSTANT64, &CCodeGen_RV64::Emit_StoreAtRef_64_VarAnyAny      },

    { OP_ADD64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_RV64::Emit_Add64_MemMemMem                     },
    { OP_ADD64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_RV64::Emit_Add64_MemMemCst                     },

    { OP_SUB64,          MATCH_MEMORY64,       MATCH_ANY,            MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_RV64::Emit_Sub64_MemAnyMem                     },
    { OP_SUB64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_RV64::Emit_Sub64_MemMemCst                     },

    { OP_CMP64,          MATCH_VARIABLE,       MATCH_ANY,            MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_RV64::Emit_Cmp64_VarAnyMem                     },
    { OP_CMP64,          MATCH_VARIABLE,       MATCH_ANY,            MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_RV64::Emit_Cmp64_VarMemCst                     },

    { OP_AND64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_RV64::Emit_And64_MemMemMem                     },

    { OP_SLL64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_VARIABLE,      MATCH_NIL, &CCodeGen_RV64::Emit_Shift64_MemMemVar<SHIFT64OP_LSL>    },
    { OP_SRL64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_VARIABLE,      MATCH_NIL, &CCodeGen_RV64::Emit_Shift64_MemMemVar<SHIFT64OP_LSR>    },
    { OP_SRA64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_VARIABLE,      MATCH_NIL, &CCodeGen_RV64::Emit_Shift64_MemMemVar<SHIFT64OP_ASR>    },

    { OP_SLL64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT,      MATCH_NIL, &CCodeGen_RV64::Emit_Shift64_MemMemCst<SHIFT64OP_LSL>    },
    { OP_SRL64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT,      MATCH_NIL, &CCodeGen_RV64::Emit_Shift64_MemMemCst<SHIFT64OP_LSR>    },
    { OP_SRA64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT,      MATCH_NIL, &CCodeGen_RV64::Emit_Shift64_MemMemCst<SHIFT64OP_ASR>    },

    { OP_MOV,            MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_NIL,           MATCH_NIL, &CCodeGen_RV64::Emit_Mov_Mem64Mem64                      },
    { OP_MOV,            MATCH_MEMORY64,       MATCH_CONSTANT64,     MATCH_NIL,           MATCH_NIL, &CCodeGen_RV64::Emit_Mov_Mem64Cst64                      },

    { OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL, nullptr                                                     },
};
