#include "Jitter_CodeGen_RV64.h"

// ???
uint64 m_stackLevel = 0;

using namespace Jitter;

void CCodeGen_RV64::LoadMemory128AddressInRegister(CRV64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
    switch(symbol->m_type)
    {
    case SYM_RELATIVE128:
        LoadRelative128AddressInRegister(dstReg, symbol, offset);
        break;
    case SYM_TEMPORARY128:
        LoadTemporary128AddressInRegister(dstReg, symbol, offset);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::LoadRelative128AddressInRegister(CRV64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
    assert(symbol->m_type == SYM_RELATIVE128);

    uint32 totalOffset = symbol->m_valueLow + offset;

    uint8 immediate = 0;
    uint8 shiftAmount = 0;
    /*if(TryGetAluImmediateParams(totalOffset, immediate, shiftAmount))
    {
        m_assembler.Add(dstReg, g_baseRegister, CRV64Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
    }
    else*/
    {
        LoadConstant64InRegister(dstReg, totalOffset);
        m_assembler.Add(dstReg, g_baseRegister, dstReg);
    }
}

void CCodeGen_RV64::LoadTemporary128AddressInRegister(CRV64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
    assert(symbol->m_type == SYM_TEMPORARY128);

    uint32 totalOffset = symbol->m_stackLocation + m_stackLevel + offset;

    uint8 immediate = 0;
    uint8 shiftAmount = 0;
    /*if(TryGetAluImmediateParams(totalOffset, immediate, shiftAmount))
    {
        m_assembler.Add(dstReg, CRV64Assembler::xSP, CRV64Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
    }
    else*/
    {
        LoadConstant64InRegister(dstReg, totalOffset);
        m_assembler.Add(dstReg, CRV64Assembler::xSP, dstReg);
    }
}

void CCodeGen_RV64::LoadTemporary256ElementAddressInRegister(CRV64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
    assert(symbol->m_type == SYM_TEMPORARY256);

    uint32 totalOffset = symbol->m_stackLocation + m_stackLevel + offset;

    uint8 immediate = 0;
    uint8 shiftAmount = 0;
    /*if(TryGetAluImmediateParams(totalOffset, immediate, shiftAmount))
    {
        m_assembler.Add(dstReg, CRV64Assembler::xSP, CRV64Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
    }
    else*/
    {
        LoadConstant64InRegister(dstReg, totalOffset);
        m_assembler.Add(dstReg, CRV64Assembler::xSP, dstReg);
    }
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    //auto dstReg = CRV64Assembler::q0;
    //auto src1Reg = CRV64Assembler::q1;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto tmp1Reg = GetNextTempRegister();
    ((m_assembler).*(MDOP::OpReg()))(dstAddrReg, src1AddrReg, tmp1Reg);
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMem1S(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    //auto dstReg = CRV64Assembler::q0;
    //auto src1Reg = CRV64Assembler::q1;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto tmpReg = GetNextTempRegister();
    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegisterMd();

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Fmv_1s(src1Reg, tmpReg);
        ((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
        m_assembler.Str_1s(dstReg, dstAddrReg, i);
    }
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMemIR1S(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    //auto dstReg = CRV64Assembler::q0;
    //auto src1Reg = CRV64Assembler::q1;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto tmpReg = GetNextTempRegister();
    auto dstReg = GetNextTempRegister();
    auto src1Reg = GetNextTempRegisterMd();

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Fmv_1s(src1Reg, tmpReg);
        ((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
        m_assembler.Str(dstReg, dstAddrReg, i);
    }
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMemSR1I(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    //auto dstReg = CRV64Assembler::q0;
    //auto src1Reg = CRV64Assembler::q1;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegister();

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(src1Reg, src1AddrReg, i);
        ((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
        m_assembler.Str_1s(dstReg, dstAddrReg, i);
    }
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMemMem(const STATEMENT& statement)
{
    //assert(0);
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;
    //auto dstReg = CRV64Assembler::q0;
    //auto src1Reg = CRV64Assembler::q1;
    //auto src2Reg = CRV64Assembler::q2;
    //auto dstReg = CRV64Assembler::v0;
    //auto src1Reg = CRV64Assembler::v1;
    //auto src2Reg = CRV64Assembler::v2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
    //((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto tmp1Reg = GetNextTempRegister();
    auto tmp2Reg = GetNextTempRegister();

    ((m_assembler).*(MDOP::OpReg()))(dstAddrReg, src1AddrReg, src2AddrReg, tmp1Reg, tmp2Reg);
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMemMem1S(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    auto tmpReg = GetNextTempRegister();
    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegisterMd();
    auto src2Reg = GetNextTempRegisterMd();

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Fmv_1s(src1Reg, tmpReg);
        m_assembler.Lw(tmpReg, src2AddrReg, i);
        m_assembler.Fmv_1s(src2Reg, tmpReg);
        ((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);
        m_assembler.Str_1s(dstReg, dstAddrReg, i);
    }
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMemMemIR1S(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    auto tmpReg = GetNextTempRegister();
    auto dstReg = GetNextTempRegister();
    auto src1Reg = GetNextTempRegisterMd();
    auto src2Reg = GetNextTempRegisterMd();

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Fmv_1s(src1Reg, tmpReg);
        m_assembler.Lw(tmpReg, src2AddrReg, i);
        m_assembler.Fmv_1s(src2Reg, tmpReg);
        ((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);
        m_assembler.Str(dstReg, dstAddrReg, i);
    }
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMemMemRev(const STATEMENT& statement)
{
    assert(0);
#if 0
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::r0;
    auto src1AddrReg = CRV64Assembler::r1;
    auto src2AddrReg = CRV64Assembler::r2;
    auto dstReg = CRV64Assembler::q0;
    auto src1Reg = CRV64Assembler::q1;
    auto src2Reg = CRV64Assembler::q2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
    ((m_assembler).*(MDOP::OpReg()))(dstReg, src2Reg, src1Reg);
    m_assembler.Vst1_32x4(dstReg, dstAddrReg);
#endif
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMemMemRev1S(const STATEMENT& statement)
{
    assert(0);
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    auto tmpReg = GetNextTempRegister();
    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegisterMd();
    auto src2Reg = GetNextTempRegisterMd();

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Fmv_1s(src1Reg, tmpReg);
        m_assembler.Lw(tmpReg, src2AddrReg, i);
        m_assembler.Fmv_1s(src2Reg, tmpReg);
        ((m_assembler).*(MDOP::OpReg()))(dstReg, src2Reg, src1Reg);
        m_assembler.Str_1s(dstReg, dstAddrReg, i);
    }
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMemMemRevIR1S(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    auto tmpReg = GetNextTempRegister();
    auto dstReg = GetNextTempRegister();
    auto src1Reg = GetNextTempRegisterMd();
    auto src2Reg = GetNextTempRegisterMd();

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Fmv_1s(src1Reg, tmpReg);
        m_assembler.Lw(tmpReg, src2AddrReg, i);
        m_assembler.Fmv_1s(src2Reg, tmpReg);
        ((m_assembler).*(MDOP::OpReg()))(dstReg, src2Reg, src1Reg);
        m_assembler.Str(dstReg, dstAddrReg, i);
    }
}

template <typename MDSHIFTOP>
void CCodeGen_RV64::Emit_Md_Shift_MemMemCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto dstReg = CRV64Assembler::q0;
    //auto src1Reg = CRV64Assembler::q1;

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //((m_assembler).*(MDSHIFTOP::OpReg()))(dstReg, src1Reg, src2->m_valueLow);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto tmp1Reg = GetNextTempRegister();
    ((m_assembler).*(MDSHIFTOP::OpReg()))(dstAddrReg, src1AddrReg, src2->m_valueLow, tmp1Reg);
}

void CCodeGen_RV64::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto tmpReg = CRV64Assembler::q0;

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto tmpReg = GetNextTempRegister();

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    //m_assembler.Vld1_32x4(tmpReg, src1AddrReg);
    //m_assembler.Vst1_32x4(tmpReg, dstAddrReg);

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Str(tmpReg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_DivS_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    auto tmpReg = GetNextTempRegister();
    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegisterMd();
    auto src2Reg = GetNextTempRegisterMd();

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Fmv_1s(src1Reg, tmpReg);
        m_assembler.Lw(tmpReg, src2AddrReg, i);
        m_assembler.Fmv_1s(src2Reg, tmpReg);
        m_assembler.Fdiv_1s(dstReg, src1Reg, src2Reg);
        m_assembler.Str_1s(dstReg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_Srl256_MemMemCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(src1->m_type == SYM_TEMPORARY256);
    assert(src2->m_type == SYM_CONSTANT);

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto dstReg = CRV64Assembler::q0;
    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;

    uint32 offset = (src2->m_valueLow & 0x7F) / 8;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadTemporary256ElementAddressInRegister(src1AddrReg, src1, offset);

    //m_assembler.Vld1_32x4_u(dstReg, src1AddrReg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto tmpReg = GetNextTempRegister();
    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Str(tmpReg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_Srl256_MemMemVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(src1->m_type == SYM_TEMPORARY256);

    //auto offsetRegister = CRV64Assembler::r0;
    //auto dstAddrReg = CRV64Assembler::r1;
    //auto src1AddrReg = CRV64Assembler::r2;
    //auto src2Register = PrepareSymbolRegisterUse(src2, CRV64Assembler::r3);
    auto offsetRegister = CRV64Assembler::w0;
    auto dstAddrReg = CRV64Assembler::x1;
    auto src1AddrReg = CRV64Assembler::x2;
    auto src2Register = PrepareSymbolRegisterUse(src2, CRV64Assembler::w3);

    //auto dstReg = CRV64Assembler::q0;

    //auto offsetShift = CRV64Assembler::MakeConstantShift(CRV64Assembler::SHIFT_LSR, 3);

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadTemporary256ElementAddressInRegister(src1AddrReg, src1, 0);

    //Compute offset and modify address
    //m_assembler.And(offsetRegister, src2Register, CRV64Assembler::MakeImmediateAluOperand(0x7F, 0));
    //m_assembler.Mov(offsetRegister, CRV64Assembler::MakeRegisterAluOperand(offsetRegister, offsetShift));
    //m_assembler.Add(src1AddrReg, src1AddrReg, offsetRegister);

    auto tmpReg = GetNextTempRegister();
    m_assembler.Li(tmpReg, 0x7f);
    //m_assembler.And(offsetRegister, src2Register, tmpReg);
    m_assembler.Andw(offsetRegister, src2Register, tmpReg);
    m_assembler.Lsr(offsetRegister, offsetRegister, 3);
    m_assembler.Add(src1AddrReg, src1AddrReg, static_cast<CRV64Assembler::REGISTER64>(offsetRegister));

    //m_assembler.Vld1_32x4_u(dstReg, src1AddrReg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Str(tmpReg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_LoadFromRef_MemVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    //auto src1AddrReg = PrepareSymbolRegisterUseRef(src1, CRV64Assembler::r0);
    //auto dstAddrReg = CRV64Assembler::r1;
    auto src1AddrReg = PrepareSymbolRegisterUseRef(src1, CRV64Assembler::x0);
    auto dstAddrReg = CRV64Assembler::x1;

    //auto dstReg = CRV64Assembler::q0;

    LoadMemory128AddressInRegister(dstAddrReg, dst);

    //m_assembler.Vld1_32x4(dstReg, src1AddrReg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto tmpReg = GetNextTempRegister();
    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Str(tmpReg, dstAddrReg, i);
    }
}

// 32 bit index maybe?
void CCodeGen_RV64::LoadRefIndexAddress(CRV64Assembler::REGISTER64 dstRegister, CSymbol* refSymbol, CRV64Assembler::REGISTER64 refRegister, CSymbol* indexSymbol, CRV64Assembler::REGISTER64 indexRegister, uint8 scale)
{
    assert(scale == 1);

    refRegister = PrepareSymbolRegisterUseRef(refSymbol, refRegister);

    /*if(uint8 immediate = 0, shiftAmount = 0;
        indexSymbol->IsConstant() && TryGetAluImmediateParams(indexSymbol->m_valueLow, immediate, shiftAmount))
    {
        m_assembler.Add(dstRegister, refRegister, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
    }
    else*/
    {
        //auto indexReg = PrepareSymbolRegisterUse(indexSymbol, indexRegister);
        //m_assembler.Add(dstRegister, refRegister, indexReg);
        auto indexReg = PrepareSymbolRegisterUse(indexSymbol, static_cast<CRV64Assembler::REGISTER32>(indexRegister));
        m_assembler.Add(dstRegister, refRegister, static_cast<CRV64Assembler::REGISTER64>(indexReg));
    }
}

void CCodeGen_RV64::Emit_Md_LoadFromRef_MemVarAny(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert(scale == 1);

    //auto src1AddrIdxReg = CRV64Assembler::r1;
    //auto dstAddrReg = CRV64Assembler::r2;
    //auto dstReg = CRV64Assembler::q0;
    auto src1AddrIdxReg = CRV64Assembler::x1;
    auto dstAddrReg = CRV64Assembler::x2;

    //LoadRefIndexAddress(src1AddrIdxReg, src1, CRV64Assembler::r0, src2, CRV64Assembler::r3, scale);
    LoadRefIndexAddress(src1AddrIdxReg, src1, CRV64Assembler::x0, src2, CRV64Assembler::x3, scale);
    LoadMemory128AddressInRegister(dstAddrReg, dst);

    //m_assembler.Vld1_32x4(dstReg, src1AddrIdxReg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto tmpReg = GetNextTempRegister();
    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrIdxReg, i);
        m_assembler.Str(tmpReg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_StoreAtRef_VarMem(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    //auto src1AddrReg = PrepareSymbolRegisterUseRef(src1, CRV64Assembler::r0);
    //auto src2AddrReg = CRV64Assembler::r1;
    auto src1AddrReg = PrepareSymbolRegisterUseRef(src1, CRV64Assembler::x0);
    auto src2AddrReg = CRV64Assembler::x1;

    //auto src2Reg = CRV64Assembler::q0;

    LoadMemory128AddressInRegister(src2AddrReg, src2);

    //m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
    //m_assembler.Vst1_32x4(src2Reg, src1AddrReg);

    auto tmpReg = GetNextTempRegister();
    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src2AddrReg, i);
        m_assembler.Str(tmpReg, src1AddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_StoreAtRef_VarAnyMem(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    auto src3 = statement.src3->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert(scale == 1);

    //auto src1AddrIdxReg = CRV64Assembler::r1;
    //auto valueAddrReg = CRV64Assembler::r2;
    //auto valueReg = CRV64Assembler::q0;
    auto src1AddrIdxReg = CRV64Assembler::x1;
    auto valueAddrReg = CRV64Assembler::x2;

    //LoadRefIndexAddress(src1AddrIdxReg, src1, CRV64Assembler::r0, src2, CRV64Assembler::r3, scale);
    LoadRefIndexAddress(src1AddrIdxReg, src1, CRV64Assembler::x0, src2, CRV64Assembler::x3, scale);
    LoadMemory128AddressInRegister(valueAddrReg, src3);

    //m_assembler.Vld1_32x4(valueReg, valueAddrReg);
    //m_assembler.Vst1_32x4(valueReg, src1AddrIdxReg);

    auto tmpReg = GetNextTempRegister();
    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, valueAddrReg, i);
        m_assembler.Str(tmpReg, src1AddrIdxReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_MovMasked_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    FRAMEWORK_MAYBE_UNUSED auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(dst->Equals(src1));

    auto mask = static_cast<uint8>(statement.jmpCondition);

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src2AddrReg = CRV64Assembler::r2;
    //auto tmpReg = CRV64Assembler::r3;
    //auto dstReg = CRV64Assembler::q0;
    //auto src2Reg = CRV64Assembler::q2;
    auto dstAddrReg = CRV64Assembler::x0;
    auto src2AddrReg = CRV64Assembler::x2;
    auto tmpReg = CRV64Assembler::w3;
    //auto dstReg = CRV64Assembler::q0;
    //auto src2Reg = CRV64Assembler::q2;
    //auto dstRegLo = static_cast<CRV64Assembler::DOUBLE_REGISTER>(dstReg + 0);
    //auto dstRegHi = static_cast<CRV64Assembler::DOUBLE_REGISTER>(dstReg + 1);
    //auto src2RegLo = static_cast<CRV64Assembler::DOUBLE_REGISTER>(src2Reg + 0);
    //auto src2RegHi = static_cast<CRV64Assembler::DOUBLE_REGISTER>(src2Reg + 1);

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    /*m_assembler.Vld1_32x4(dstReg, dstAddrReg);
    m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
    for(unsigned int i = 0; i < 4; i++)
    {
        if(mask & (1 << i))
        {
            m_assembler.Vmov(tmpReg, (i & 2) ? src2RegHi : src2RegLo, (i & 1));
            m_assembler.Vmov((i & 2) ? dstRegHi : dstRegLo, tmpReg, (i & 1));
        }
    }*/
    for(unsigned int i = 0; i < 4; i++)
    {
        if(mask & (1 << i))
        {
            m_assembler.Lw(tmpReg, src2AddrReg, i*4);
            m_assembler.Str(tmpReg, dstAddrReg, i*4);
        }
    }

    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

void CCodeGen_RV64::Emit_Md_Expand_MemReg(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto tmpReg = CRV64Assembler::q0;
    auto dstAddrReg = CRV64Assembler::x0;
    //auto tmpReg = GetNextTempRegister();
    auto src1Reg = PrepareSymbolRegisterUse(src1, CRV64Assembler::w1);

    LoadMemory128AddressInRegister(dstAddrReg, dst);

    //m_assembler.Vdup(tmpReg, g_registers[src1->m_valueLow]);
    //m_assembler.Vst1_32x4(tmpReg, dstAddrReg);

    for (int i=0; i<16; i+=4) {
        m_assembler.Str(src1Reg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_Expand_MemMem(const STATEMENT& statement)
{
    // untested
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1Reg = CRV64Assembler::r1;
    //auto tmpReg = CRV64Assembler::q0;
    auto dstAddrReg = CRV64Assembler::x0;
    auto src1Reg = CRV64Assembler::w1;

    LoadMemoryInRegister(src1Reg, src1);
    LoadMemory128AddressInRegister(dstAddrReg, dst);

    //m_assembler.Vdup(tmpReg, src1Reg);
    //m_assembler.Vst1_32x4(tmpReg, dstAddrReg);

    for (int i=0; i<16; i+=4) {
        m_assembler.Str(src1Reg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_Expand_MemCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1Reg = CRV64Assembler::r1;
    //auto tmpReg = CRV64Assembler::q0;
    auto dstAddrReg = CRV64Assembler::x0;
    auto src1Reg = CRV64Assembler::w1;

    LoadMemory128AddressInRegister(dstAddrReg, dst);

    /*if(src1->m_valueLow == 0)
    {
        m_assembler.Veor(tmpReg, tmpReg, tmpReg);
    }
    else*/
    {
        LoadConstantInRegister(src1Reg, src1->m_valueLow);
        //m_assembler.Vdup(tmpReg, src1Reg);
    }

    //m_assembler.Vst1_32x4(tmpReg, dstAddrReg);*/

    for (int i=0; i<16; i+=4) {
        m_assembler.Str(src1Reg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_ClampS_MemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto cstAddrReg = CRV64Assembler::r2;
    //auto dstReg = CRV64Assembler::q0;
    //auto cst0Reg = CRV64Assembler::q1;
    //auto cst1Reg = CRV64Assembler::q2;
    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto cst1Reg = GetNextTempRegister();
    auto cst2Reg = GetNextTempRegister();
    auto tmpReg = GetNextTempRegister();

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);

    m_assembler.Li(cst1Reg, 0x7F7FFFFF);
    m_assembler.Li(cst2Reg, 0xFF7FFFFF);

    //m_assembler.Adrl(cstAddrReg, g_fpClampMask1);
    //m_assembler.Vld1_32x4(cst0Reg, cstAddrReg);
    //m_assembler.Adrl(cstAddrReg, g_fpClampMask2);
    //m_assembler.Vld1_32x4(cst1Reg, cstAddrReg);

    //m_assembler.Vld1_32x4(dstReg, src1AddrReg);
    //m_assembler.Vmin_I32(dstReg, dstReg, cst0Reg);
    //m_assembler.Vmin_U32(dstReg, dstReg, cst1Reg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Smin_1s(tmpReg, tmpReg, cst1Reg);
        m_assembler.Umin_1s(tmpReg, tmpReg, cst2Reg);
        m_assembler.Str(tmpReg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_MakeSz_VarMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    //auto src1AddrReg = CRV64Assembler::r0;
    //auto cstAddrReg = CRV64Assembler::r1;
    //auto src1Reg = CRV64Assembler::q0;
    //auto signReg = CRV64Assembler::q1;
    //auto zeroReg = CRV64Assembler::q2;
    //auto cstReg = CRV64Assembler::q3;
    auto src1AddrReg = CRV64Assembler::x0;
    auto cstAddrReg = CRV64Assembler::x1;

    //LITERAL128 lit1(0x0004080C1014181CUL, 0xFFFFFFFFFFFFFFFFUL);
    //LITERAL128 lit2(0x8040201008040201UL, 0x0000000000000000UL);

    LoadMemory128AddressInRegister(src1AddrReg, src1);
    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);

    //auto dstReg = PrepareSymbolRegisterDef(dst, CRV64Assembler::r0);
    auto dstReg = PrepareSymbolRegisterDef(dst, CRV64Assembler::w0);

    //m_assembler.Vcltz_I32(signReg, src1Reg);
    auto tmpReg = GetNextTempRegister();
    auto tmpMdReg = GetNextTempRegisterMd();
    //auto zeroReg = GetNextTempRegisterMd();
    //m_assembler.Fmv_1s(zeroReg, CRV64Assembler::wZR);
    auto oneReg = GetNextTempRegister();
    m_assembler.Li(oneReg, 0x00000001);
    auto outReg = GetNextTempRegister();
    //m_assembler.Add(outReg, CRV64Assembler::wZR, CRV64Assembler::wZR);
    m_assembler.Addw(outReg, CRV64Assembler::wZR, CRV64Assembler::wZR);
    //auto one2Reg = GetNextTempRegister();
    //m_assembler.Li(one2Reg, 0x1);
    auto signReg = GetNextTempRegister();

    //auto negMaskReg = GetNextTempRegister();
    //m_assembler.Li(negMaskReg, 0xf);

    auto zeroReg = GetNextTempRegister();
    auto zeroMaskReg = GetNextTempRegister();
    m_assembler.Li(zeroMaskReg, 0x18);

    auto signBitReg = GetNextTempRegister();
    m_assembler.Li(signBitReg, 0x80000000);

    //for (int i=0; i<4; i++)
    for (int i=4; i-->0; )
    {
        //m_assembler.Mov(signReg, oneReg);
        m_assembler.Lwu(tmpReg, src1AddrReg, i*4);

        m_assembler.Andw(signReg, tmpReg, signBitReg);

        m_assembler.Fmv_1s(tmpMdReg, tmpReg);
        //m_assembler.Flt_1s(tmpReg, tmpMdReg, zeroReg);
        m_assembler.Fclass_1s(tmpReg, tmpMdReg);

        //m_assembler.And(signReg, tmpReg, negMaskReg);
        //m_assembler.And(zeroReg, tmpReg, zeroMaskReg);

        //m_assembler.Andw(signReg, tmpReg, negMaskReg);
        m_assembler.Andw(zeroReg, tmpReg, zeroMaskReg);

        //m_assembler.Lsr(tmpReg, tmpReg, 1);
        //m_assembler.And(signReg, tmpReg, one2Reg);
        //m_assembler.Lsr(tmpReg, tmpReg, 1);
        //m_assembler.And(signReg, tmpReg, one2Reg);
        //m_assembler.Lsr(tmpReg, tmpReg, 1);
        //m_assembler.And(signReg, tmpReg, one2Reg);

        m_assembler.Lsl(oneReg, oneReg, 4);

        int offset = 4*2;
        // beq
        uint32 opcode = 0x00000063;
        // blt
        //uint32 opcode = 0x00004063;
        // bltu
        //uint32 opcode = 0x00006063;
        opcode |= ((offset & 0x1E) >> 1) << 8;
        opcode |= ((offset & 0x800) >> 11) << 7;
        opcode |= ((offset & 0x1000) >> 12) << 31;
        opcode |= ((offset & 0x7E0) >> 5) << 25;
        opcode |= (CRV64Assembler::wZR << 15);
        opcode |= (signReg << 20);
        //m_assembler.WriteWord(opcode);
        m_assembler.Beq(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(signReg), offset);

        // or
        opcode = 0x00006033;
        opcode |= (outReg <<  7);
        opcode |= (outReg << 15);
        opcode |= (oneReg << 20);
        //m_assembler.WriteWord(opcode);
        m_assembler.Orw(outReg, outReg, oneReg);

        m_assembler.Lsr(oneReg, oneReg, 4);

        offset = 4*2;
        // beq
        opcode = 0x00000063;
        // blt
        //uint32 opcode = 0x00004063;
        // bltu
        //uint32 opcode = 0x00006063;
        opcode |= ((offset & 0x1E) >> 1) << 8;
        opcode |= ((offset & 0x800) >> 11) << 7;
        opcode |= ((offset & 0x1000) >> 12) << 31;
        opcode |= ((offset & 0x7E0) >> 5) << 25;
        opcode |= (CRV64Assembler::wZR << 15);
        opcode |= (zeroReg << 20);
        //m_assembler.WriteWord(opcode);
        m_assembler.Beq(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(zeroReg), offset);

        // or
        opcode = 0x00006033;
        opcode |= (outReg <<  7);
        opcode |= (outReg << 15);
        opcode |= (oneReg << 20);
        //m_assembler.WriteWord(opcode);
        m_assembler.Orw(outReg, outReg, oneReg);

        m_assembler.Lsl(oneReg, oneReg, 1);
    }

    m_assembler.Mov(dstReg, outReg);

    /*m_assembler.Vceqz_F32(zeroReg, src1Reg);

    m_assembler.Adrl(cstAddrReg, lit1);
    m_assembler.Vld1_32x4(cstReg, cstAddrReg);
    m_assembler.Adrl(cstAddrReg, lit2);
    m_assembler.Vtbl(
        static_cast<CRV64Assembler::DOUBLE_REGISTER>(signReg),
        static_cast<CRV64Assembler::DOUBLE_REGISTER>(signReg),
        static_cast<CRV64Assembler::DOUBLE_REGISTER>(cstReg));
    m_assembler.Vld1_32x4(cstReg, cstAddrReg);
    m_assembler.Vand(signReg, signReg, cstReg);
    m_assembler.Vpaddl_I8(zeroReg, signReg);
    m_assembler.Vpaddl_I16(cstReg, zeroReg);
    m_assembler.Vpaddl_I32(signReg, cstReg);
    m_assembler.Vmov(dstReg, static_cast<CRV64Assembler::DOUBLE_REGISTER>(signReg), 0);*/

    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_PackHB_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto src2AddrReg = CRV64Assembler::r2;
    //auto dstReg = CRV64Assembler::q0;
    //auto src1Reg = CRV64Assembler::q1;
    //auto src2Reg = CRV64Assembler::q2;
    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
    //m_assembler.Vmovn_I16(static_cast<CRV64Assembler::DOUBLE_REGISTER>(dstReg + 1), src1Reg);
    //m_assembler.Vmovn_I16(static_cast<CRV64Assembler::DOUBLE_REGISTER>(dstReg + 0), src2Reg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    /*auto tmpReg = GetNextTempRegister();
    for (int i=0; i<8; i++) {
        m_assembler.Ldrb(tmpReg, src1AddrReg, i*2);
        m_assembler.Strb(tmpReg, dstAddrReg, i+8);
    }
    for (int i=0; i<8; i++) {
        m_assembler.Ldrb(tmpReg, src2AddrReg, i*2);
        m_assembler.Strb(tmpReg, dstAddrReg, i);
    }*/

    auto tmp1Reg = GetNextTempRegister64();
    auto tmp2Reg = GetNextTempRegister64();
    //for (int i=0; i<8; i++)
    for (int i=8; i-->0; )
    {
        m_assembler.Lsl(tmp2Reg, tmp2Reg, 8);
        m_assembler.Ldrb(static_cast<CRV64Assembler::REGISTER32>(tmp1Reg), src1AddrReg, i*2);
        uint32 opcode = 0x00006033;
        opcode |= (tmp2Reg <<  7);
        opcode |= (tmp2Reg << 15);
        opcode |= (tmp1Reg << 20);
        //m_assembler.WriteWord(opcode);
        m_assembler.Or(tmp2Reg, tmp2Reg, tmp1Reg);
    }
    m_assembler.Str(tmp2Reg, dstAddrReg, 8);
    //for (int i=0; i<8; i++)
    for (int i=8; i-->0; )
    {
        m_assembler.Lsl(tmp2Reg, tmp2Reg, 8);
        m_assembler.Ldrb(static_cast<CRV64Assembler::REGISTER32>(tmp1Reg), src2AddrReg, i*2);
        uint32 opcode = 0x00006033;
        opcode |= (tmp2Reg <<  7);
        opcode |= (tmp2Reg << 15);
        opcode |= (tmp1Reg << 20);
        //m_assembler.WriteWord(opcode);
        m_assembler.Or(tmp2Reg, tmp2Reg, tmp1Reg);
    }
    m_assembler.Str(tmp2Reg, dstAddrReg, 0);
}

void CCodeGen_RV64::Emit_Md_PackWH_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto src2AddrReg = CRV64Assembler::r2;
    //auto dstReg = CRV64Assembler::q0;
    //auto src1Reg = CRV64Assembler::q1;
    //auto src2Reg = CRV64Assembler::q2;
    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
    //m_assembler.Vmovn_I32(static_cast<CRV64Assembler::DOUBLE_REGISTER>(dstReg + 1), src1Reg);
    //m_assembler.Vmovn_I32(static_cast<CRV64Assembler::DOUBLE_REGISTER>(dstReg + 0), src2Reg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    /*auto tmpReg = GetNextTempRegister();
    for (int i=0; i<4; i++) {
        m_assembler.Ldrh(tmpReg, src1AddrReg, i*4);
        m_assembler.Strh(tmpReg, dstAddrReg, (i*2)+8);
    }
    for (int i=0; i<4; i++) {
        m_assembler.Ldrh(tmpReg, src2AddrReg, i*4);
        m_assembler.Strh(tmpReg, dstAddrReg, i*2);
    }*/

    auto tmp1Reg = GetNextTempRegister64();
    auto tmp2Reg = GetNextTempRegister64();
    //for (int i=0; i<8; i++)
    for (int i=4; i-->0; )
    {
        m_assembler.Lsl(tmp2Reg, tmp2Reg, 16);
        m_assembler.Ldrh(static_cast<CRV64Assembler::REGISTER32>(tmp1Reg), src1AddrReg, i*4);
        uint32 opcode = 0x00006033;
        opcode |= (tmp2Reg <<  7);
        opcode |= (tmp2Reg << 15);
        opcode |= (tmp1Reg << 20);
        //m_assembler.WriteWord(opcode);
        m_assembler.Or(tmp2Reg, tmp2Reg, tmp1Reg);
    }
    m_assembler.Str(tmp2Reg, dstAddrReg, 8);
    //for (int i=0; i<8; i++)
    for (int i=4; i-->0; )
    {
        m_assembler.Lsl(tmp2Reg, tmp2Reg, 16);
        m_assembler.Ldrh(static_cast<CRV64Assembler::REGISTER32>(tmp1Reg), src2AddrReg, i*4);
        uint32 opcode = 0x00006033;
        opcode |= (tmp2Reg <<  7);
        opcode |= (tmp2Reg << 15);
        opcode |= (tmp1Reg << 20);
        //m_assembler.WriteWord(opcode);
        m_assembler.Or(tmp2Reg, tmp2Reg, tmp1Reg);
    }
    m_assembler.Str(tmp2Reg, dstAddrReg, 0);
}

template <uint32 offset>
void CCodeGen_RV64::Emit_Md_UnpackBH_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto src2AddrReg = CRV64Assembler::r2;
    //auto src1Reg = CRV64Assembler::d0;
    //auto src2Reg = CRV64Assembler::d1;
    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1, offset);
    LoadMemory128AddressInRegister(src2AddrReg, src2, offset);

    //Warning: VZIP modifies both registers
    //m_assembler.Vld1_32x2(src1Reg, src2AddrReg);
    //m_assembler.Vld1_32x2(src2Reg, src1AddrReg);
    //m_assembler.Vzip_I8(src1Reg, src2Reg);
    //m_assembler.Vst1_32x4(static_cast<CRV64Assembler::QUAD_REGISTER>(src1Reg), dstAddrReg);

    auto tmpReg = GetNextTempRegister();
    for (int i=0; i<8; i++)
    {
        m_assembler.Ldrb(tmpReg, src2AddrReg, i);
        m_assembler.Strb(tmpReg, dstAddrReg, i*2);
        m_assembler.Ldrb(tmpReg, src1AddrReg, i);
        m_assembler.Strb(tmpReg, dstAddrReg, (i*2)+1);
    }
}

template <uint32 offset>
void CCodeGen_RV64::Emit_Md_UnpackHW_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto src2AddrReg = CRV64Assembler::r2;
    //auto src1Reg = CRV64Assembler::d0;
    //auto src2Reg = CRV64Assembler::d1;
    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1, offset);
    LoadMemory128AddressInRegister(src2AddrReg, src2, offset);

    //Warning: VZIP modifies both registers
    //m_assembler.Vld1_32x2(src1Reg, src2AddrReg);
    //m_assembler.Vld1_32x2(src2Reg, src1AddrReg);
    //m_assembler.Vzip_I16(src1Reg, src2Reg);
    //m_assembler.Vst1_32x4(static_cast<CRV64Assembler::QUAD_REGISTER>(src1Reg), dstAddrReg);

    auto tmpReg = GetNextTempRegister();
    for (int i=0; i<4; i++)
    {
        m_assembler.Ldrh(tmpReg, src2AddrReg, i*2);
        m_assembler.Strh(tmpReg, dstAddrReg, i*4);
        m_assembler.Ldrh(tmpReg, src1AddrReg, i*2);
        m_assembler.Strh(tmpReg, dstAddrReg, (i*4)+2);
    }
}

template <uint32 offset>
void CCodeGen_RV64::Emit_Md_UnpackWD_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto src2AddrReg = CRV64Assembler::r2;
    //auto src1Reg = CRV64Assembler::d0;
    //auto src2Reg = CRV64Assembler::d2;
    auto dstAddrReg = CRV64Assembler::x0;
    auto src1AddrReg = CRV64Assembler::x1;
    auto src2AddrReg = CRV64Assembler::x2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1, offset);
    LoadMemory128AddressInRegister(src2AddrReg, src2, offset);

    //Warning: VZIP modifies both registers
    //m_assembler.Vld1_32x2(src1Reg, src2AddrReg);
    //m_assembler.Vld1_32x2(src2Reg, src1AddrReg);
    //m_assembler.Vzip_I32(static_cast<CRV64Assembler::QUAD_REGISTER>(src1Reg), static_cast<CRV64Assembler::QUAD_REGISTER>(src2Reg));
    //m_assembler.Vst1_32x4(static_cast<CRV64Assembler::QUAD_REGISTER>(src1Reg), dstAddrReg);

    auto tmpReg = GetNextTempRegister();
    for (int i=0; i<2; i++)
    {
        m_assembler.Lwu(tmpReg, src2AddrReg, i*4);
        m_assembler.Str(tmpReg, dstAddrReg, i*8);
        m_assembler.Lwu(tmpReg, src1AddrReg, i*4);
        m_assembler.Str(tmpReg, dstAddrReg, (i*8)+4);
    }
}

void CCodeGen_RV64::Emit_MergeTo256_MemMemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(dst->m_type == SYM_TEMPORARY256);

    //auto dstLoAddrReg = CRV64Assembler::r0;
    //auto dstHiAddrReg = CRV64Assembler::r1;
    //auto src1AddrReg = CRV64Assembler::r2;
    //auto src2AddrReg = CRV64Assembler::r3;
    //auto src1Reg = CRV64Assembler::q0;
    //auto src2Reg = CRV64Assembler::q1;
    auto dstLoAddrReg = CRV64Assembler::x0;
    auto dstHiAddrReg = CRV64Assembler::x1;
    auto src1AddrReg = CRV64Assembler::x2;
    auto src2AddrReg = CRV64Assembler::x3;
    //auto src1Reg = CRV64Assembler::q0;
    //auto src2Reg = CRV64Assembler::q1;

    LoadTemporary256ElementAddressInRegister(dstLoAddrReg, dst, 0x00);
    LoadTemporary256ElementAddressInRegister(dstHiAddrReg, dst, 0x10);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
    //m_assembler.Vst1_32x4(src1Reg, dstLoAddrReg);
    //m_assembler.Vst1_32x4(src2Reg, dstHiAddrReg);

    auto tmpReg = GetNextTempRegister();
    for (int i=0; i<16; i+=4) {
        m_assembler.Lwu(tmpReg, src1AddrReg, i);
        m_assembler.Str(tmpReg, dstLoAddrReg, i);
    }
    for (int i=0; i<16; i+=4) {
        m_assembler.Lwu(tmpReg, src2AddrReg, i);
        m_assembler.Str(tmpReg, dstHiAddrReg, i);
    }
}

CCodeGen_RV64::CONSTMATCHER CCodeGen_RV64::g_mdConstMatchers[] =
{
    { OP_MD_ADD_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_ADDB_MEM> },
    { OP_MD_ADD_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_ADDH_MEM> },
    { OP_MD_ADD_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_ADDW_MEM> },

    { OP_MD_SUB_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_SUBB_MEM> },
    { OP_MD_SUB_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_SUBH_MEM> },
    { OP_MD_SUB_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_SUBW_MEM> },

    { OP_MD_ADDUS_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_ADDBUS_MEM> },
    { OP_MD_ADDUS_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_ADDHUS_MEM> },
    { OP_MD_ADDUS_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_ADDWUS_MEM> },

    { OP_MD_ADDSS_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_ADDBSS_MEM> },
    { OP_MD_ADDSS_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_ADDHSS_MEM> },
    { OP_MD_ADDSS_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_ADDWSS_MEM> },

    { OP_MD_SUBUS_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_SUBBUS_MEM> },
    { OP_MD_SUBUS_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_SUBHUS_MEM> },
    { OP_MD_SUBUS_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_SUBWUS_MEM> },

    { OP_MD_SUBSS_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_SUBHSS_MEM> },
    { OP_MD_SUBSS_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_SUBWSS_MEM> },

    { OP_MD_CLAMP_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL,       MATCH_NIL, &CCodeGen_RV64::Emit_Md_ClampS_MemMem },

#if 1
    { OP_MD_CMPEQ_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_CMPEQB_MEM> },
    { OP_MD_CMPEQ_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_CMPEQH_MEM> },
    { OP_MD_CMPEQ_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_CMPEQW_MEM> },

    { OP_MD_CMPGT_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_CMPGTB_MEM> },
    { OP_MD_CMPGT_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_CMPGTH_MEM> },
    { OP_MD_CMPGT_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_CMPGTW_MEM> },
#endif
    { OP_MD_MIN_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_MINH_MEM> },
    { OP_MD_MIN_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_MINW_MEM> },

    { OP_MD_MAX_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_MAXH_MEM> },
    { OP_MD_MAX_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_MAXW_MEM> },

    { OP_MD_ADD_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem1S<MDOP_ADDS_1S> },
    { OP_MD_SUB_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem1S<MDOP_SUBS_1S> },
    { OP_MD_MUL_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem1S<MDOP_MULS_1S> },
    { OP_MD_DIV_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_DivS_MemMemMem       },

    { OP_MD_ABS_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL,       MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMem1S<MDOP_ABSS_1S>      },

    { OP_MD_MIN_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem1S<MDOP_MINS_1S> },
    { OP_MD_MAX_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem1S<MDOP_MAXS_1S> },

    { OP_MD_CMPLT_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemIR1S<MDOP_CMPLTS_1S> },
    { OP_MD_CMPGT_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRevIR1S<MDOP_CMPLTS_1S>    },

    { OP_MD_AND, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_AND_MEM> },
    { OP_MD_OR,  MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_OR_MEM>  },
    { OP_MD_XOR, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMem<MDOP_XOR_MEM> },
    { OP_MD_NOT, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL,       MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMem<MDOP_NOT_MEM>    },

    { OP_MD_SLLH, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCst<MDOP_SLLH_MEM> },
    { OP_MD_SLLW, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCst<MDOP_SLLW_MEM> },

    { OP_MD_SRLH, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCst<MDOP_SRLH_MEM> },
    { OP_MD_SRLW, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCst<MDOP_SRLW_MEM> },

    { OP_MD_SRAH, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCst<MDOP_SRAH_MEM> },
    { OP_MD_SRAW, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCst<MDOP_SRAW_MEM> },

    { OP_MD_SRL256, MATCH_VARIABLE128, MATCH_MEMORY256, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Srl256_MemMemVar },
    { OP_MD_SRL256, MATCH_VARIABLE128, MATCH_MEMORY256, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Srl256_MemMemCst },

    { OP_MD_MAKESZ, MATCH_VARIABLE, MATCH_MEMORY128, MATCH_NIL, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MakeSz_VarMem },

    { OP_MD_TOSINGLE,        MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemSR1I<MDOP_TOSINGLE_SR1I> },
    { OP_MD_TOWORD_TRUNCATE, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemIR1S<MDOP_TOWORD_IR1S>   },

    { OP_MOV, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Mov_MemMem },

    { OP_LOADFROMREF, MATCH_MEMORY128, MATCH_VAR_REF, MATCH_NIL,       MATCH_NIL,       &CCodeGen_RV64::Emit_Md_LoadFromRef_MemVar    },
    { OP_LOADFROMREF, MATCH_MEMORY128, MATCH_VAR_REF, MATCH_ANY32,     MATCH_NIL,       &CCodeGen_RV64::Emit_Md_LoadFromRef_MemVarAny },
    { OP_STOREATREF,  MATCH_NIL,       MATCH_VAR_REF, MATCH_MEMORY128, MATCH_NIL,       &CCodeGen_RV64::Emit_Md_StoreAtRef_VarMem     },
    { OP_STOREATREF,  MATCH_NIL,       MATCH_VAR_REF, MATCH_ANY32,     MATCH_MEMORY128, &CCodeGen_RV64::Emit_Md_StoreAtRef_VarAnyMem  },

    { OP_MD_MOV_MASKED, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MovMasked_MemMemMem },

    { OP_MD_EXPAND, MATCH_MEMORY128, MATCH_REGISTER, MATCH_NIL, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Expand_MemReg },
    { OP_MD_EXPAND, MATCH_MEMORY128, MATCH_MEMORY,   MATCH_NIL, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Expand_MemMem },
    { OP_MD_EXPAND, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Expand_MemCst },

    { OP_MD_PACK_HB, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_PackHB_MemMemMem },
    { OP_MD_PACK_WH, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_PackWH_MemMemMem },

    { OP_MD_UNPACK_LOWER_BH, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackBH_MemMemMem<0> },
    { OP_MD_UNPACK_LOWER_HW, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackHW_MemMemMem<0> },
    { OP_MD_UNPACK_LOWER_WD, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackWD_MemMemMem<0> },

    { OP_MD_UNPACK_UPPER_BH, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackBH_MemMemMem<8> },
    { OP_MD_UNPACK_UPPER_HW, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackHW_MemMemMem<8> },
    { OP_MD_UNPACK_UPPER_WD, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackWD_MemMemMem<8> },

    { OP_MERGETO256, MATCH_MEMORY256, MATCH_VARIABLE128, MATCH_VARIABLE128, MATCH_NIL, &CCodeGen_RV64::Emit_MergeTo256_MemMemMem },

    { OP_MOV, MATCH_NIL, MATCH_NIL, MATCH_NIL, MATCH_NIL, nullptr },

};
