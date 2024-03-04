#include "Jitter_CodeGen_RV64.h"
#include <stdexcept>

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
#if 1
    assert(symbol->m_type == SYM_RELATIVE128);

	uint32 totalOffset = symbol->m_valueLow + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Addi(dstReg, g_baseRegister, totalOffset);
#else
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
#endif
}

void CCodeGen_RV64::LoadTemporary128AddressInRegister(CRV64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
#if 1
    assert(symbol->m_type == SYM_TEMPORARY128);

	uint32 totalOffset = symbol->m_stackLocation + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Addi(dstReg, CRV64Assembler::xSP, totalOffset);
#else
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
#endif
}

void CCodeGen_RV64::LoadTemporary256ElementAddressInRegister(CRV64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
#if 1
    assert(symbol->m_type == SYM_TEMPORARY256);

	uint32 totalOffset = symbol->m_stackLocation + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Addi(dstReg, CRV64Assembler::xSP, totalOffset);
#else
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
#endif
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
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
void CCodeGen_RV64::Emit_Md_MemMemRVV(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
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

    uint32 width = MDOP::Config.m_width;
    uint32 lumop_rs2_vs2 = 0;
    uint32 sumop_rs2_vs2 = 0;
    uint32 vm = 1; // unmasked
    uint32 lmop = 4; // sign-extended unit-stride
    uint32 smop = 0; // unit-stride
    uint32 nf = 0;

    m_assembler.Addiw(tmpReg, CRV64Assembler::zero, MDOP::Config.m_vl);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (MDOP::Config.m_sew<<2));
    m_assembler.Vloadv(src1Reg, src1AddrReg, width, lumop_rs2_vs2, vm, lmop, nf);
    ((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
    m_assembler.Vstorev(dstReg, dstAddrReg, width, sumop_rs2_vs2, vm, smop, nf);
    //m_assembler.Break();
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMem1S(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
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

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
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

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
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
void CCodeGen_RV64::Emit_Md_MemMemMemRVV(const STATEMENT& statement)
{
    assert(m_thead_extentions);
    //assert(0);
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;
    //auto dstReg = CRV64Assembler::q0;
    //auto src1Reg = CRV64Assembler::q1;
    //auto src2Reg = CRV64Assembler::q2;
    auto dstReg = CRV64Assembler::v0;
    auto src1Reg = CRV64Assembler::v1;
    auto src2Reg = CRV64Assembler::v2;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
    //((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto tmp1Reg = GetNextTempRegister();
    auto tmp2Reg = GetNextTempRegister();

    uint32 width = MDOP::Config.m_width;
    uint32 lumop_rs2_vs2 = 0;
    uint32 sumop_rs2_vs2 = 0;
    uint32 vm = 1; // unmasked
    uint32 lmop = 4; // sign-extended unit-stride
    uint32 smop = 0; // unit-stride
    uint32 nf = 0;

    m_assembler.Addiw(tmp1Reg, CRV64Assembler::zero, MDOP::Config.m_vl);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), (MDOP::Config.m_sew<<2));
    m_assembler.Vloadv(src1Reg, src1AddrReg, width, lumop_rs2_vs2, vm, lmop, nf);
    m_assembler.Vloadv(src2Reg, src2AddrReg, width, lumop_rs2_vs2, vm, lmop, nf);
    ((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);
    m_assembler.Vstorev(dstReg, dstAddrReg, width, sumop_rs2_vs2, vm, smop, nf);
    //m_assembler.Break();
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_MemMemMem(const STATEMENT& statement)
{
    //assert(0);
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;
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

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;

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

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;

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

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;

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

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;

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

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //((m_assembler).*(MDSHIFTOP::OpReg()))(dstReg, src1Reg, src2->m_valueLow);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    auto tmp1Reg = GetNextTempRegister();
    ((m_assembler).*(MDSHIFTOP::OpReg()))(dstAddrReg, src1AddrReg, src2->m_valueLow, tmp1Reg);
}

template <typename MDSHIFTOP>
void CCodeGen_RV64::Emit_Md_Shift_MemMemCstRVV(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto dstReg = CRV64Assembler::q0;
    //auto src1Reg = CRV64Assembler::q1;

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto dstReg = GetNextTempRegisterMd();
    auto src1Reg = GetNextTempRegisterMd();

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);

    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
    //((m_assembler).*(MDSHIFTOP::OpReg()))(dstReg, src1Reg, src2->m_valueLow);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    uint32 width = MDSHIFTOP::Config.m_width;
    uint32 lumop_rs2_vs2 = 0;
    uint32 sumop_rs2_vs2 = 0;
    uint32 vm = 1; // unmasked
    uint32 lmop = 4; // sign-extended unit-stride
    uint32 smop = 0; // unit-stride
    uint32 nf = 0;

    auto tmp1Reg = GetNextTempRegister();
    m_assembler.Addiw(tmp1Reg, CRV64Assembler::zero, MDSHIFTOP::Config.m_vl);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), (MDSHIFTOP::Config.m_sew<<2));
    m_assembler.Vloadv(src1Reg, src1AddrReg, width, lumop_rs2_vs2, vm, lmop, nf);
    ((m_assembler).*(MDSHIFTOP::OpReg()))(dstReg, src1Reg, src2->m_valueLow);
    m_assembler.Vstorev(dstReg, dstAddrReg, width, sumop_rs2_vs2, vm, smop, nf);

    //((m_assembler).*(MDSHIFTOP::OpReg()))(dstAddrReg, src1AddrReg, src2->m_valueLow, tmp1Reg);
}

void CCodeGen_RV64::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    //auto dstAddrReg = CRV64Assembler::r0;
    //auto src1AddrReg = CRV64Assembler::r1;
    //auto tmpReg = CRV64Assembler::q0;

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    //m_assembler.Vld1_32x4(tmpReg, src1AddrReg);
    //m_assembler.Vst1_32x4(tmpReg, dstAddrReg);

    if (m_thead_extentions) {
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
        auto valueReg = GetNextTempRegisterMd();
        m_assembler.Vlwv(valueReg, src1AddrReg, 0);
        m_assembler.Vswv(valueReg, dstAddrReg, 0);
        return;
    }

    auto tmpReg = GetNextTempRegister();
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

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    if (m_thead_extentions) {
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
        auto dstReg = GetNextTempRegisterMd();
        auto src1Reg = GetNextTempRegisterMd();
        auto src2Reg = GetNextTempRegisterMd();
        m_assembler.Vlwv(src1Reg, src1AddrReg, 0);
        m_assembler.Vlwv(src2Reg, src2AddrReg, 0);
        m_assembler.Vfdivvv(dstReg, src1Reg, src2Reg, 0);
        m_assembler.Vswv(dstReg, dstAddrReg, 0);
        //m_assembler.Break();
        return;
    }

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
    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;

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
    auto offsetRegister = CRV64Assembler::a0;
    auto dstAddrReg = CRV64Assembler::x11;
    auto src1AddrReg = CRV64Assembler::x12;
    auto src2Register = PrepareSymbolRegisterUse(src2, CRV64Assembler::a3);

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
    auto src1AddrReg = PrepareSymbolRegisterUseRef(src1, CRV64Assembler::x10);
    auto dstAddrReg = CRV64Assembler::x11;

    //auto dstReg = CRV64Assembler::q0;

    LoadMemory128AddressInRegister(dstAddrReg, dst);

    //m_assembler.Vld1_32x4(dstReg, src1AddrReg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    if (m_thead_extentions) {
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
        auto valueReg = GetNextTempRegisterMd();
        m_assembler.Vlwv(valueReg, src1AddrReg, 0);
        m_assembler.Vswv(valueReg, dstAddrReg, 0);
        return;
    }

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

    ADDSUB_IMM_PARAMS addSubImmParams;
    if(indexSymbol->IsConstant() && TryGetAddSubImmParams(indexSymbol->m_valueLow, addSubImmParams))
    //if(uint8 immediate = 0, shiftAmount = 0;
    //    indexSymbol->IsConstant() && TryGetAluImmediateParams(indexSymbol->m_valueLow, immediate, shiftAmount))
    {
        m_assembler.Addi(dstRegister, refRegister, addSubImmParams.imm);
    }
    else
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
    auto src1AddrIdxReg = CRV64Assembler::x11;
    auto dstAddrReg = CRV64Assembler::x12;

    //LoadRefIndexAddress(src1AddrIdxReg, src1, CRV64Assembler::r0, src2, CRV64Assembler::r3, scale);
    LoadRefIndexAddress(src1AddrIdxReg, src1, CRV64Assembler::x10, src2, CRV64Assembler::x13, scale);
    LoadMemory128AddressInRegister(dstAddrReg, dst);

    //m_assembler.Vld1_32x4(dstReg, src1AddrIdxReg);
    //m_assembler.Vst1_32x4(dstReg, dstAddrReg);

    if (m_thead_extentions) {
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
        auto valueReg = GetNextTempRegisterMd();
        m_assembler.Vlwv(valueReg, src1AddrIdxReg, 0);
        m_assembler.Vswv(valueReg, dstAddrReg, 0);
        return;
    }

    auto tmpReg = GetNextTempRegister();
    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrIdxReg, i);
        m_assembler.Str(tmpReg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_StoreAtRef_VarMem(const STATEMENT& statement)
{
    if (m_thead_extentions) {
        auto src1 = statement.src1->GetSymbol().get();
        auto src2 = statement.src2->GetSymbol().get();

        auto src1AddrReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
        auto src2AddrReg = GetNextTempRegister64();

        auto src2Reg = GetNextTempRegisterMd();

        LoadMemory128AddressInRegister(src2AddrReg, src2);

        auto tmpReg = GetNextTempRegister64();
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
        m_assembler.Vlwv(src2Reg, src2AddrReg, 0);
        m_assembler.Vswv(src2Reg, src1AddrReg, 0);

        return;
    }

    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    //auto src1AddrReg = PrepareSymbolRegisterUseRef(src1, CRV64Assembler::r0);
    //auto src2AddrReg = CRV64Assembler::r1;
    auto src1AddrReg = PrepareSymbolRegisterUseRef(src1, CRV64Assembler::x10);
    auto src2AddrReg = CRV64Assembler::x11;

    //auto src2Reg = CRV64Assembler::q0;

    LoadMemory128AddressInRegister(src2AddrReg, src2);

    //m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
    //m_assembler.Vst1_32x4(src2Reg, src1AddrReg);

    if (m_thead_extentions) {
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
        auto valueReg = GetNextTempRegisterMd();
        m_assembler.Vlwv(valueReg, src2AddrReg, 0);
        m_assembler.Vswv(valueReg, src1AddrReg, 0);
        return;
    }

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
    auto src1AddrIdxReg = CRV64Assembler::x11;
    auto valueAddrReg = CRV64Assembler::x12;

    //LoadRefIndexAddress(src1AddrIdxReg, src1, CRV64Assembler::r0, src2, CRV64Assembler::r3, scale);
    LoadRefIndexAddress(src1AddrIdxReg, src1, CRV64Assembler::x10, src2, CRV64Assembler::x13, scale);
    LoadMemory128AddressInRegister(valueAddrReg, src3);

    //m_assembler.Vld1_32x4(valueReg, valueAddrReg);
    //m_assembler.Vst1_32x4(valueReg, src1AddrIdxReg);

    if (m_thead_extentions) {
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
        auto valueReg = GetNextTempRegisterMd();
        m_assembler.Vlwv(valueReg, valueAddrReg, 0);
        m_assembler.Vswv(valueReg, src1AddrIdxReg, 0);
        return;
    }

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
    auto dstAddrReg = CRV64Assembler::x10;
    auto src2AddrReg = CRV64Assembler::x12;
    auto tmpReg = CRV64Assembler::a3;
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
    auto dstAddrReg = CRV64Assembler::x10;
    //auto tmpReg = GetNextTempRegister();
    auto src1Reg = PrepareSymbolRegisterUse(src1, CRV64Assembler::a1);

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
    auto dstAddrReg = CRV64Assembler::x10;
    auto src1Reg = CRV64Assembler::a1;

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
    auto dstAddrReg = CRV64Assembler::x10;
    auto src1Reg = CRV64Assembler::a1;

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
    if (m_thead_extentions) {
        auto dst = statement.dst->GetSymbol().get();
        auto src1 = statement.src1->GetSymbol().get();

        auto dstAddrReg = GetNextTempRegister64();
        auto src1AddrReg = GetNextTempRegister64();
        auto dstReg = GetNextTempRegisterMd();
        auto tmpReg = GetNextTempRegister64();

        LoadMemory128AddressInRegister(dstAddrReg, dst);
        LoadMemory128AddressInRegister(src1AddrReg, src1);

        auto cst1Reg = GetNextTempRegisterMd();
        auto cst2Reg = GetNextTempRegisterMd();

        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
        m_assembler.Ldr_Pc(cst1Reg, g_fpClampMask1, tmpReg);
        m_assembler.Vlwv(dstReg, src1AddrReg, 0);
        //LoadMemoryFpSingleInRegisterRVV(result2Reg, src1);
        m_assembler.Vminvv(dstReg, dstReg, cst1Reg, 0);
        m_assembler.Ldr_Pc(cst2Reg, g_fpClampMask2, tmpReg);
        m_assembler.Vminuvv(dstReg, dstReg, cst2Reg, 0);
        m_assembler.Vswv(dstReg, dstAddrReg, 0);
        //StoreRegisterInMemoryFpSingleRVV(dst, result2Reg);

        return;
    }

    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto cst1Reg = GetNextTempRegister();
    auto cst2Reg = GetNextTempRegister();
    auto tmpReg = GetNextTempRegister();

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);

    m_assembler.Li(cst1Reg, 0x7F7FFFFF);
    m_assembler.Li(cst2Reg, 0xFF7FFFFF);

    for (int i=0; i<16; i+=4) {
        m_assembler.Lw(tmpReg, src1AddrReg, i);
        m_assembler.Smin_1s(tmpReg, tmpReg, cst1Reg);
        m_assembler.Umin_1s(tmpReg, tmpReg, cst2Reg);
        m_assembler.Str(tmpReg, dstAddrReg, i);
    }
}

void CCodeGen_RV64::Emit_Md_MakeSz_VarMem(const STATEMENT& statement)
{
    // pack is negative signed and is zero into 8 bits SSSSZZZZ
    if (m_thead_extentions) {
        auto dst = statement.dst->GetSymbol().get();
        auto src1 = statement.src1->GetSymbol().get();

        //m_nextTempRegisterMd = 0;

        auto src1AddrReg = GetNextTempRegister64();
        auto tmpReg = GetNextTempRegister64();
        auto tmp2Reg = GetNextTempRegister64();
        auto tmpSignReg = GetNextTempRegister64();
        auto tmpZeroReg = GetNextTempRegister64();

        LoadMemory128AddressInRegister(src1AddrReg, src1);

        auto dstReg = PrepareSymbolRegisterDef(dst, CRV64Assembler::a0);

        auto cstReg = CRV64Assembler::v8;
        auto signReg = CRV64Assembler::v6;
        auto zeroReg = CRV64Assembler::v7;
        auto src1Reg = CRV64Assembler::v9;

        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);

        // vlw.v v9, (a1)
        m_assembler.Vlwv(src1Reg, src1AddrReg, 0);

        // vlw.v v8, (a3)
        LITERAL128 litZero(0x0000000000000000UL, 0x0000000000000000UL);
        m_assembler.Ldr_Pc(cstReg, litZero, tmpReg);

        // vmflt.vv v6, v9, v8
#if 1
        m_assembler.Vsrlvi(signReg, src1Reg, 31, 0);
#else
        m_assembler.Vmfltvv(signReg, src1Reg, cstReg, 0);
#endif
        // vmfeq.vv v7, v9, v8
        m_assembler.Vmfeqvv(zeroReg, src1Reg, cstReg, 0);

        //vsw.v v6, (a1)
        //vsw.v v7, (a2)
        //vsw.v v0, (a3)

        // addi a4, zero, 0
        //m_assembler.Addi(tmpSignReg, CRV64Assembler::xZR, 0);
        // addi a5, zero, 0
        //m_assembler.Addi(tmpZeroReg, CRV64Assembler::xZR, 0);
        m_assembler.Addi(static_cast<CRV64Assembler::REGISTER64>(dstReg), CRV64Assembler::xZR, 0);

#if 0
        //vmfeq.vv v1, v1, v3
        //vsll.vv v0, v1, v9

        // vext.x.v a4, v0, zero
        m_assembler.Vextxv(tmpReg, zeroReg, CRV64Assembler::xZR);
        // addi a0, zero, 1
        m_assembler.Addi(tmp2Reg, CRV64Assembler::xZR, 1);
        // vext.x.v a5, v0, a0
        m_assembler.Vextxv(tmp2Reg, zeroReg, tmp2Reg);
        // or a4, a4, a5
        m_assembler.Or(tmp2Reg, CRV64Assembler::xZR, 1);
        // addi a0, zero, 2
        m_assembler.Addi(tmp2Reg, CRV64Assembler::xZR, 1);
        // vext.x.v a5, v0, a0
        m_assembler.Vextxv(tmpReg, zeroReg, CRV64Assembler::xZR);
        // or a4, a4, a5
        m_assembler.Or(tmp2Reg, CRV64Assembler::xZR, 1);
        // addi a0, zero, 3
        m_assembler.Addi(tmp2Reg, CRV64Assembler::xZR, 1);
        // vext.x.v a5, v0, a0
        m_assembler.Vextxv(tmpReg, zeroReg, CRV64Assembler::xZR);
        // or a4, a4, a5
        m_assembler.Or(tmp2Reg, CRV64Assembler::xZR, 1);
#else
#define VECTORSHIFT 1
#if VECTORSHIFT
        LITERAL128 litSll(3, 2, 1, 0);
        m_assembler.Ldr_Pc(cstReg, litSll, tmpReg);
        // vsll.vv v0, v1, v9
        m_assembler.Vsllvv(zeroReg, zeroReg, cstReg, 0);
        m_assembler.Vaddvi(cstReg, cstReg, 4, 0);
        m_assembler.Vsllvv(signReg, signReg, cstReg, 0);
        //m_assembler.Vsllvi(signReg, signReg, 4, 0);
#endif
        for (int i=0; i<4; i++) {
            // addi a6, zero, 0
            m_assembler.Addi(tmp2Reg, CRV64Assembler::xZR, 3-i);
            // vext.x.v a0, v6, a6
            m_assembler.Vextxv(tmpReg, signReg, tmp2Reg);
            // vext.x.v a1, v7, a6
            m_assembler.Vextxv(tmp2Reg, zeroReg, tmp2Reg);

#if VECTORSHIFT
#else
            if (i>0) {
                // slli a0, a0, 1
                m_assembler.Slli(tmpReg, tmpReg, i);
                // slli a1, a1, 1
                m_assembler.Slli(tmp2Reg, tmp2Reg, i);
            }
#endif

            // or a4, a4, a0
            //m_assembler.Or(tmpSignReg, tmpSignReg, tmpReg);
            // or a5, a5, a1
            //m_assembler.Or(tmpZeroReg, tmpZeroReg, tmp2Reg);

            m_assembler.Or(static_cast<CRV64Assembler::REGISTER64>(dstReg), static_cast<CRV64Assembler::REGISTER64>(dstReg), tmpReg);
            m_assembler.Or(static_cast<CRV64Assembler::REGISTER64>(dstReg), static_cast<CRV64Assembler::REGISTER64>(dstReg), tmp2Reg);
        }
#endif
        //m_assembler.Addi(static_cast<CRV64Assembler::REGISTER64>(dstReg), tmpSignReg, 0);
        //m_assembler.Addi(static_cast<CRV64Assembler::REGISTER64>(dstReg), tmpZeroReg, 0);
        //m_assembler.Slli(dstReg, dstReg, 4);
        //m_assembler.Or(static_cast<CRV64Assembler::REGISTER64>(dstReg), static_cast<CRV64Assembler::REGISTER64>(dstReg), tmpZeroReg);
        CommitSymbolRegister(dst, dstReg);
        return;
    }

    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    //auto src1AddrReg = CRV64Assembler::r0;
    //auto cstAddrReg = CRV64Assembler::r1;
    //auto src1Reg = CRV64Assembler::q0;
    //auto signReg = CRV64Assembler::q1;
    //auto zeroReg = CRV64Assembler::q2;
    //auto cstReg = CRV64Assembler::q3;
    auto src1AddrReg = CRV64Assembler::x10;
    auto cstAddrReg = CRV64Assembler::x11;

    //LITERAL128 lit1(0x0004080C1014181CUL, 0xFFFFFFFFFFFFFFFFUL);
    //LITERAL128 lit2(0x8040201008040201UL, 0x0000000000000000UL);

    LoadMemory128AddressInRegister(src1AddrReg, src1);
    //m_assembler.Vld1_32x4(src1Reg, src1AddrReg);

    //auto dstReg = PrepareSymbolRegisterDef(dst, CRV64Assembler::r0);
    auto dstReg = PrepareSymbolRegisterDef(dst, CRV64Assembler::a0);

    //m_assembler.Vcltz_I32(signReg, src1Reg);
    auto tmpReg = GetNextTempRegister();
    auto tmpMdReg = GetNextTempRegisterMd();
    //auto zeroReg = GetNextTempRegisterMd();
    //m_assembler.Fmv_1s(zeroReg, CRV64Assembler::wZR);
    auto oneReg = GetNextTempRegister();
    m_assembler.Li(oneReg, 0x00000001);
    auto outReg = GetNextTempRegister();
    //m_assembler.Add(outReg, CRV64Assembler::wZR, CRV64Assembler::wZR);
    m_assembler.Addw(outReg, CRV64Assembler::zero, CRV64Assembler::zero);
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
        opcode |= (CRV64Assembler::zero << 15);
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
        opcode |= (CRV64Assembler::zero << 15);
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
    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    if (m_thead_extentions) {
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
        m_assembler.Vlwv(CRV64Assembler::v8, src2AddrReg, 0);
        m_assembler.Vlwv(CRV64Assembler::v9, src1AddrReg, 0);
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 16);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 0);
        //m_assembler.Vnsrlvi(CRV64Assembler::v2, CRV64Assembler::v8, 16, 0);
        m_assembler.Vnsrlvi(CRV64Assembler::v2, CRV64Assembler::v8, 0, 0);
        m_assembler.Vsbv(CRV64Assembler::v2, dstAddrReg, 0);
        return;
    }

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
    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    LoadMemory128AddressInRegister(src1AddrReg, src1);
    LoadMemory128AddressInRegister(src2AddrReg, src2);

    if (m_thead_extentions) {
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
        m_assembler.Vlwv(CRV64Assembler::v8, src2AddrReg, 0);
        m_assembler.Vlwv(CRV64Assembler::v9, src1AddrReg, 0);
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
        m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 4);
        //m_assembler.Vnsrlvi(CRV64Assembler::v2, CRV64Assembler::v8, 16, 0);
        m_assembler.Vnsrlvi(CRV64Assembler::v2, CRV64Assembler::v8, 0, 0);
        m_assembler.Vshv(CRV64Assembler::v2, dstAddrReg, 0);
        return;
    }

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
    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;

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
    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;

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
    auto dstAddrReg = CRV64Assembler::x10;
    auto src1AddrReg = CRV64Assembler::x11;
    auto src2AddrReg = CRV64Assembler::x12;

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
    auto dstLoAddrReg = CRV64Assembler::x10;
    auto dstHiAddrReg = CRV64Assembler::x11;
    auto src1AddrReg = CRV64Assembler::x12;
    auto src2AddrReg = CRV64Assembler::x13;
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

CCodeGen_RV64::CONSTMATCHER CCodeGen_RV64::g_mdConstMatchersMem[] =
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

CCodeGen_RV64::CONSTMATCHER CCodeGen_RV64::g_mdConstMatchersMemRVV[] =
{
    { OP_MD_ADD_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_ADDB_MEMRVV> },
    { OP_MD_ADD_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_ADDH_MEMRVV> },
    { OP_MD_ADD_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_ADDW_MEMRVV> },

    { OP_MD_SUB_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_SUBB_MEMRVV> },
    { OP_MD_SUB_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_SUBH_MEMRVV> },
    { OP_MD_SUB_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_SUBW_MEMRVV> },

    { OP_MD_ADDUS_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_ADDBUS_MEMRVV> },
    { OP_MD_ADDUS_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_ADDHUS_MEMRVV> },
    { OP_MD_ADDUS_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_ADDWUS_MEMRVV> },

    { OP_MD_ADDSS_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_ADDBSS_MEMRVV> },
    { OP_MD_ADDSS_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_ADDHSS_MEMRVV> },
    { OP_MD_ADDSS_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_ADDWSS_MEMRVV> },

    { OP_MD_SUBUS_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_SUBBUS_MEMRVV> },
    { OP_MD_SUBUS_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_SUBHUS_MEMRVV> },
    { OP_MD_SUBUS_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_SUBWUS_MEMRVV> },

    { OP_MD_SUBSS_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_SUBHSS_MEMRVV> },
    { OP_MD_SUBSS_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_SUBWSS_MEMRVV> },

    { OP_MD_CLAMP_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL,       MATCH_NIL, &CCodeGen_RV64::Emit_Md_ClampS_MemMem },

#if 1
    { OP_MD_CMPEQ_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_CMPEQB_MEMRVV> },
    { OP_MD_CMPEQ_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_CMPEQH_MEMRVV> },
    { OP_MD_CMPEQ_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_CMPEQW_MEMRVV> },

    { OP_MD_CMPGT_B, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_CMPGTB_MEMRVV> },
    { OP_MD_CMPGT_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_CMPGTH_MEMRVV> },
    { OP_MD_CMPGT_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_CMPGTW_MEMRVV> },
#endif
    { OP_MD_MIN_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_MINH_MEMRVV> },
    { OP_MD_MIN_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_MINW_MEMRVV> },

    { OP_MD_MAX_H, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_MAXH_MEMRVV> },
    { OP_MD_MAX_W, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_MAXW_MEMRVV> },

    { OP_MD_ADD_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_ADDS_MEMRVV> },
    { OP_MD_SUB_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_SUBS_MEMRVV> },
	{ OP_MD_MUL_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_MULS_MEMRVV> },
    { OP_MD_DIV_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_DIVS_MEMRVV> },

    { OP_MD_ABS_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL,       MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemRVV<MDOP_ABSS_MEMRVV>      },

    { OP_MD_MIN_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_MINS_MEMRVV> },
    { OP_MD_MAX_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_MAXS_MEMRVV> },

    { OP_MD_CMPLT_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_CMPLTS_MEMRVV> },
    { OP_MD_CMPGT_S, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_CMPLTS_REV_MEMRVV>    },

    { OP_MD_AND, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_AND_MEMRVV> },
    { OP_MD_OR,  MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_OR_MEMRVV>  },
    { OP_MD_XOR, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemMemRVV<MDOP_XOR_MEMRVV> },
    { OP_MD_NOT, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL,       MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemRVV<MDOP_NOT_MEMRVV>    },

    { OP_MD_SLLH, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCstRVV<MDOP_SLLH_MEMRVV> },
    { OP_MD_SLLW, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCstRVV<MDOP_SLLW_MEMRVV> },

    { OP_MD_SRLH, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCstRVV<MDOP_SRLH_MEMRVV> },
    { OP_MD_SRLW, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCstRVV<MDOP_SRLW_MEMRVV> },

    { OP_MD_SRAH, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCstRVV<MDOP_SRAH_MEMRVV> },
    { OP_MD_SRAW, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_MemMemCstRVV<MDOP_SRAW_MEMRVV> },

    { OP_MD_SRL256, MATCH_VARIABLE128, MATCH_MEMORY256, MATCH_VARIABLE, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Srl256_MemMemVar },
    { OP_MD_SRL256, MATCH_VARIABLE128, MATCH_MEMORY256, MATCH_CONSTANT, MATCH_NIL, &CCodeGen_RV64::Emit_Md_Srl256_MemMemCst },

    { OP_MD_MAKESZ, MATCH_VARIABLE, MATCH_MEMORY128, MATCH_NIL, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MakeSz_VarMem },

    { OP_MD_TOSINGLE,        MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemRVV<MDOP_TOSINGLE_MEMRVV> },
    { OP_MD_TOWORD_TRUNCATE, MATCH_MEMORY128, MATCH_MEMORY128, MATCH_NIL, MATCH_NIL, &CCodeGen_RV64::Emit_Md_MemMemRVV<MDOP_TOWORD_MEMRVV>   },

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

void CCodeGen_RV64::LoadMemoryFpSingleInRegisterRVV(CRV64Assembler::REGISTERMD reg, CSymbol* symbol)
{
    auto tmpReg = GetNextTempRegister64();
    switch(symbol->m_type)
    {
    case SYM_FP_REL_SINGLE:
        m_assembler.Addi(tmpReg, g_baseRegister, symbol->m_valueLow);
        //m_assembler.Vlwv(reg, tmpReg, 0);
        m_assembler.Vlev(reg, tmpReg, 0);
        break;
    case SYM_FP_TMP_SINGLE:
        m_assembler.Addi(tmpReg, CRV64Assembler::xSP, symbol->m_stackLocation);
        //m_assembler.Vlwv(reg, CRV64Assembler::xSP, 0);
        m_assembler.Vlev(reg, CRV64Assembler::xSP, 0);
        break;
    default:
        assert(false);
        break;
    }
}

void CCodeGen_RV64::StoreRegisterInMemoryFpSingleRVV(CSymbol* symbol, CRV64Assembler::REGISTERMD reg)
{
    auto tmpReg = GetNextTempRegister64();
    switch(symbol->m_type)
    {
    case SYM_FP_REL_SINGLE:
        m_assembler.Addi(tmpReg, g_baseRegister, symbol->m_valueLow);
        //m_assembler.Vswv(reg, tmpReg, 0);
        m_assembler.Vsev(reg, tmpReg, 0);
        break;
    case SYM_FP_TMP_SINGLE:
        m_assembler.Addi(tmpReg, CRV64Assembler::xSP, symbol->m_stackLocation);
        //m_assembler.Vswv(reg, tmpReg, 0);
        m_assembler.Vsev(reg, tmpReg, 0);
        break;
    default:
        assert(false);
        break;
    }
}

void CCodeGen_RV64::LoadMemory128InRegister(CRV64Assembler::REGISTERMD dstReg, CSymbol* symbol)
{
    auto tmpReg = GetNextTempRegister64();
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		//m_assembler.Ldr_1q(dstReg, g_baseRegister, symbol->m_valueLow);
        m_assembler.Addi(tmpReg, g_baseRegister, symbol->m_valueLow);
        //m_assembler.Vlwv(dstReg, tmpReg, 0);
        m_assembler.Vlev(dstReg, tmpReg, 0);
		break;
	case SYM_TEMPORARY128:
		//m_assembler.Ldr_1q(dstReg, CRV64Assembler::xSP, symbol->m_stackLocation);
        m_assembler.Addi(tmpReg, CRV64Assembler::xSP, symbol->m_stackLocation);
        //m_assembler.Vlwv(dstReg, tmpReg, 0);
        m_assembler.Vlev(dstReg, tmpReg, 0);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_RV64::StoreRegisterInMemory128(CSymbol* symbol, CRV64Assembler::REGISTERMD srcReg)
{
    auto tmpReg = GetNextTempRegister64();
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		//m_assembler.Str_1q(srcReg, g_baseRegister, symbol->m_valueLow);
        m_assembler.Addi(tmpReg, g_baseRegister, symbol->m_valueLow);
        //m_assembler.Vswv(srcReg, tmpReg, 0);
        m_assembler.Vsev(srcReg, tmpReg, 0);
		break;
	case SYM_TEMPORARY128:
		//m_assembler.Str_1q(srcReg, CRV64Assembler::xSP, symbol->m_stackLocation);
        m_assembler.Addi(tmpReg, CRV64Assembler::xSP, symbol->m_stackLocation);
        //m_assembler.Vswv(srcReg, tmpReg, 0);
        m_assembler.Vsev(srcReg, tmpReg, 0);
		break;
	default:
		assert(0);
		break;
	}
}

/*void CCodeGen_RV64::LoadMemory128AddressInRegister(CRV64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
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
	assert(totalOffset < 0x1000);
	m_assembler.Add(dstReg, g_baseRegister, totalOffset, CRV64Assembler::ADDSUB_IMM_SHIFT_LSL0);
}

void CCodeGen_RV64::LoadTemporary128AddressInRegister(CRV64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_TEMPORARY128);

	uint32 totalOffset = symbol->m_stackLocation + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Add(dstReg, CRV64Assembler::xSP, totalOffset, CRV64Assembler::ADDSUB_IMM_SHIFT_LSL0);
}

void CCodeGen_RV64::LoadTemporary256ElementAddressInRegister(CRV64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_TEMPORARY256);

	uint32 totalOffset = symbol->m_stackLocation + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Add(dstReg, CRV64Assembler::xSP, totalOffset, CRV64Assembler::ADDSUB_IMM_SHIFT_LSL0);
}*/

CRV64Assembler::REGISTERMD CCodeGen_RV64::PrepareSymbolRegisterDefMd(CSymbol* symbol, CRV64Assembler::REGISTERMD preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(symbol->m_valueLow < MAX_MDREGISTERS);
		return g_registersMd[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CRV64Assembler::REGISTERMD CCodeGen_RV64::PrepareSymbolRegisterUseMd(CSymbol* symbol, CRV64Assembler::REGISTERMD preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(symbol->m_valueLow < MAX_MDREGISTERS);
		return g_registersMd[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		LoadMemory128InRegister(preferedRegister, symbol);
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_RV64::CommitSymbolRegisterMd(CSymbol* symbol, CRV64Assembler::REGISTERMD usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(usedRegister == g_registersMd[symbol->m_valueLow]);
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		StoreRegisterInMemory128(symbol, usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
    auto tmpReg = GetNextTempRegister();

    uint32 width = MDOP::Config.m_width;
    uint32 lumop_rs2_vs2 = 0;
    uint32 sumop_rs2_vs2 = 0;
    uint32 vm = 1; // unmasked
    uint32 lmop = 4; // sign-extended unit-stride
    uint32 smop = 0; // unit-stride
    uint32 nf = 0;

    m_assembler.Addiw(tmpReg, CRV64Assembler::zero, MDOP::Config.m_vl);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (MDOP::Config.m_sew<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());

    ((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

    auto tmpReg = GetNextTempRegister();

    uint32 width = MDOP::Config.m_width;
    uint32 lumop_rs2_vs2 = 0;
    uint32 sumop_rs2_vs2 = 0;
    uint32 vm = 1; // unmasked
    uint32 lmop = 4; // sign-extended unit-stride
    uint32 smop = 0; // unit-stride
    uint32 nf = 0;

    m_assembler.Addiw(tmpReg, CRV64Assembler::zero, MDOP::Config.m_vl);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (MDOP::Config.m_sew<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());

    ((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

template <typename MDOP>
void CCodeGen_RV64::Emit_Md_VarVarVarRev(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

    auto tmpReg = GetNextTempRegister();

    uint32 width = MDOP::Config.m_width;
    uint32 lumop_rs2_vs2 = 0;
    uint32 sumop_rs2_vs2 = 0;
    uint32 vm = 1; // unmasked
    uint32 lmop = 4; // sign-extended unit-stride
    uint32 smop = 0; // unit-stride
    uint32 nf = 0;

    m_assembler.Addiw(tmpReg, CRV64Assembler::zero, MDOP::Config.m_vl);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (MDOP::Config.m_sew<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());

    ((m_assembler).*(MDOP::OpReg()))(dstReg, src2Reg, src1Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

template <typename MDSHIFTOP>
void CCodeGen_RV64::Emit_Md_Shift_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

    auto tmpReg = GetNextTempRegister();

    uint32 width = MDSHIFTOP::Config.m_width;
    uint32 lumop_rs2_vs2 = 0;
    uint32 sumop_rs2_vs2 = 0;
    uint32 vm = 1; // unmasked
    uint32 lmop = 4; // sign-extended unit-stride
    uint32 smop = 0; // unit-stride
    uint32 nf = 0;

    m_assembler.Addiw(tmpReg, CRV64Assembler::zero, MDSHIFTOP::Config.m_vl);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (MDSHIFTOP::Config.m_sew<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());

    ((m_assembler).*(MDSHIFTOP::OpReg()))(dstReg, src1Reg, src2->m_valueLow);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_ClampS_VarVar(const STATEMENT& statement)
{
    //assert(false);

    auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e32<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto cst1Reg = GetNextTempRegisterMd();
	auto cst2Reg = GetNextTempRegisterMd();

	//m_assembler.Ldr_Pc(cst1Reg, g_fpClampMask1);
	//m_assembler.Ldr_Pc(cst2Reg, g_fpClampMask2);

	//m_assembler.Smin_4s(dstReg, src1Reg, cst1Reg);
	//m_assembler.Umin_4s(dstReg, dstReg, cst2Reg);

    m_assembler.Ldr_Pc(cst1Reg, g_fpClampMask1, tmpReg);
    //m_assembler.Vlwv(dstReg, src1AddrReg, 0);
    //LoadMemoryFpSingleInRegisterRVV(result2Reg, src1);
    m_assembler.Vminvv(dstReg, src1Reg, cst1Reg, 0);
    m_assembler.Ldr_Pc(cst2Reg, g_fpClampMask2, tmpReg);
    m_assembler.Vminuvv(dstReg, dstReg, cst2Reg, 0);
    //m_assembler.Vswv(dstReg, dstAddrReg, 0);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_MakeSz_VarVar(const STATEMENT& statement)
{
    //assert(false);
    //m_assembler.Break();
    auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_nextTempRegisterMd = 0;

    auto tmpReg = GetNextTempRegister64();

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	//auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
    auto src1Reg = PrepareSymbolRegisterUseMd(src1, CRV64Assembler::v9);

	//auto signReg = GetNextTempRegisterMd();
	//auto zeroReg = GetNextTempRegisterMd();
	//auto cstReg = GetNextTempRegisterMd();

	//assert(zeroReg == signReg + 1);

    //m_nextTempRegisterMd = 0;

    auto tmp2Reg = GetNextTempRegister64();
    auto tmpSignReg = GetNextTempRegister64();
    auto tmpZeroReg = GetNextTempRegister64();

    /*auto cstReg = CRV64Assembler::v8;
    auto signReg = CRV64Assembler::v6;
    auto zeroReg = CRV64Assembler::v7;
    //auto src1Reg = CRV64Assembler::v9;*/

    auto cstReg = CRV64Assembler::v2;
    auto signReg = CRV64Assembler::v0;
    auto zeroReg = CRV64Assembler::v1;

    // vlw.v v8, (a3)
    LITERAL128 litZero(0x0000000000000000UL, 0x0000000000000000UL);
    m_assembler.Ldr_Pc(cstReg, litZero, tmpReg);

    // vmflt.vv v6, v9, v8
    m_assembler.Vsrlvi(signReg, src1Reg, 31, 0);
    // vmfeq.vv v7, v9, v8
    m_assembler.Vmfeqvv(zeroReg, src1Reg, cstReg, 0);

    //vsw.v v6, (a1)
    //vsw.v v7, (a2)
    //vsw.v v0, (a3)

    // addi a4, zero, 0
    //m_assembler.Addi(tmpSignReg, CRV64Assembler::xZR, 0);
    // addi a5, zero, 0
    //m_assembler.Addi(tmpZeroReg, CRV64Assembler::xZR, 0);
    m_assembler.Addi(static_cast<CRV64Assembler::REGISTER64>(dstReg), CRV64Assembler::xZR, 0);


    LITERAL128 litSll(3, 2, 1, 0);
    m_assembler.Ldr_Pc(cstReg, litSll, tmpReg);
    // vsll.vv v0, v1, v9
    m_assembler.Vsllvv(zeroReg, zeroReg, cstReg, 0);
    m_assembler.Vaddvi(cstReg, cstReg, 4, 0);
    m_assembler.Vsllvv(signReg, signReg, cstReg, 0);
    //m_assembler.Vsllvi(signReg, signReg, 4, 0);

    for (int i=0; i<4; i++) {
        // addi a6, zero, 0
        m_assembler.Addi(tmp2Reg, CRV64Assembler::xZR, 3-i);
        // vext.x.v a0, v6, a6
        m_assembler.Vextxv(tmpReg, signReg, tmp2Reg);
        // vext.x.v a1, v7, a6
        m_assembler.Vextxv(tmp2Reg, zeroReg, tmp2Reg);

        m_assembler.Or(static_cast<CRV64Assembler::REGISTER64>(dstReg), static_cast<CRV64Assembler::REGISTER64>(dstReg), tmpReg);
        m_assembler.Or(static_cast<CRV64Assembler::REGISTER64>(dstReg), static_cast<CRV64Assembler::REGISTER64>(dstReg), tmp2Reg);
    }

    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_Mov_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	assert(!dst->Equals(src1));
	
    auto tmp1Reg = GetNextTempRegister();
    m_assembler.Addiw(tmp1Reg, CRV64Assembler::zero, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), 8);

    m_assembler.Mov(g_registersMd[dst->m_valueLow], g_registersMd[src1->m_valueLow]);
}

void CCodeGen_RV64::Emit_Md_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

    auto tmp1Reg = GetNextTempRegister();
    m_assembler.Addiw(tmp1Reg, CRV64Assembler::zero, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), 8);

    LoadMemory128InRegister(g_registersMd[dst->m_valueLow], src1);
}

void CCodeGen_RV64::Emit_Md_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

    auto tmp1Reg = GetNextTempRegister();
    m_assembler.Addiw(tmp1Reg, CRV64Assembler::zero, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), 8);

    StoreRegisterInMemory128(dst, g_registersMd[src1->m_valueLow]);
}

void CCodeGen_RV64::Emit_Md_Mov_MemMem_RVV(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpReg = GetNextTempRegisterMd();

    auto tmp1Reg = GetNextTempRegister();
    m_assembler.Addiw(tmp1Reg, CRV64Assembler::zero, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), 8);

    LoadMemory128InRegister(tmpReg, src1);
	StoreRegisterInMemory128(dst, tmpReg);
}

void CCodeGen_RV64::Emit_Md_LoadFromRef_VarVar(const STATEMENT& statement)
{
    //assert(false);
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

    auto tmp1Reg = GetNextTempRegister();
    m_assembler.Addiw(tmp1Reg, CRV64Assembler::zero, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), 8);

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());

    m_assembler.Vlev(dstReg, addressReg, 0);
    //m_assembler.Ldr_1q(dstReg, addressReg, 0);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_LoadFromRef_VarVarAny(const STATEMENT& statement)
{
    //assert(false);
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

    auto tmp1Reg = GetNextTempRegister();
    m_assembler.Addiw(tmp1Reg, CRV64Assembler::zero, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), 8);

    auto src1AddrIdxReg = GetNextTempRegister64();
    LoadRefIndexAddress(src1AddrIdxReg, src1, CRV64Assembler::x10, src2, CRV64Assembler::x13, scale);

    //auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());

	/*if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x10000))
	{
		m_assembler.Ldr_1q(dstReg, addressReg, scaledIndex);
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
		m_assembler.Ldr_1q(dstReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), (scale == 0x10));
	}*/

    m_assembler.Vlev(dstReg, src1AddrIdxReg, 0);

    CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_StoreAtRef_VarVar(const STATEMENT& statement)
{
    //assert(false);
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

    auto tmp1Reg = GetNextTempRegister();
    m_assembler.Addiw(tmp1Reg, CRV64Assembler::zero, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), 8);

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
	auto valueReg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());

    m_assembler.Vsev(valueReg, addressReg, 0);
	//m_assembler.Str_1q(valueReg, addressReg, 0);
}

void CCodeGen_RV64::Emit_Md_StoreAtRef_VarAnyVar(const STATEMENT& statement)
{
    //assert(false);
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	auto src3 = statement.src3->GetSymbol().get();
	uint8 scale = static_cast<uint8>(statement.jmpCondition);

	assert(scale == 1);

    auto tmp1Reg = GetNextTempRegister();
    m_assembler.Addiw(tmp1Reg, CRV64Assembler::zero, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), 8);

    auto src1AddrIdxReg = GetNextTempRegister64();
    LoadRefIndexAddress(src1AddrIdxReg, src1, CRV64Assembler::x10, src2, CRV64Assembler::x13, scale);

    //auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
	auto valueReg = PrepareSymbolRegisterUseMd(src3, GetNextTempRegisterMd());

	/*if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex < 0x10000))
	{
		m_assembler.Str_1q(valueReg, addressReg, scaledIndex);
	}
	else
	{
		auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
		m_assembler.Str_1q(valueReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), (scale == 0x10));
	}*/

    m_assembler.Vsev(valueReg, src1AddrIdxReg, 0);
}

void CCodeGen_RV64::Emit_Md_MovMasked_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->Equals(src1));

	auto mask = static_cast<uint8>(statement.jmpCondition);

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);

    /*auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());*/

    auto src1Reg = PrepareSymbolRegisterUseMd(src1, CRV64Assembler::v2);
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, CRV64Assembler::v3);
    assert(src1Reg==CRV64Assembler::v2);
    assert(src2Reg==CRV64Assembler::v3);

    auto maskReg = CRV64Assembler::v0;
    m_assembler.Vmvvi(maskReg, mask, 0);

    auto cstReg = CRV64Assembler::v1;
    //LITERAL128 litSll(3, 2, 1, 0);
    LITERAL128 litSll(0, 1, 2, 3);
    m_assembler.Ldr_Pc(cstReg, litSll, tmpReg);
    m_assembler.Vsrlvv(maskReg, maskReg, cstReg, 0);
    m_assembler.Vandvi(maskReg, maskReg, 1, 0);
    m_assembler.Vmergevvm(src1Reg, src1Reg, src2Reg, 0);

#if 0
    auto slide1Reg = GetNextTempRegisterMd();
    auto slide2Reg = GetNextTempRegisterMd();

    m_assembler.Vmvvi(slide1Reg, 0, 0);

    for(unsigned int i = 0; i < 4; i++)
	{
        m_assembler.Addi(tmpReg, CRV64Assembler::xZR, ((mask & (1 << i)) ? 1 : 0));
        m_assembler.Vextxv(tmpReg, src2Reg, tmpReg);
        m_assembler.Vslide1upvx(slide2Reg, slide1Reg, tmpReg, 0);
        m_assembler.Vmvvv(slide1Reg, slide2Reg, 0);
#if 0
		if(mask & (1 << i))
		{
			//m_assembler.Ins_1s(src1Reg, i, src2Reg, i);
            m_assembler.Addi(tmpReg, CRV64Assembler::xZR, i);
            m_assembler.Vextxv(tmpReg, src2Reg, tmpReg);
            //m_assembler.Vmvsx(src1Reg, tmpReg); should set the correct element
            m_assembler.Vslide1upvx(slide2Reg, slide1Reg, tmpReg, 0);
            m_assembler.Vmvvv(slide1Reg, slide2Reg, 0);
		}
#endif
	}
#endif

    //This is only valid if dst == src1
	CommitSymbolRegisterMd(dst, src1Reg);
}

void CCodeGen_RV64::Emit_Md_Expand_VarReg(const STATEMENT& statement)
{
    //assert(false);
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e32<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());

	//m_assembler.Dup_4s(dstReg, g_registers[src1->m_valueLow]);
    m_assembler.Vmvvx(dstReg, static_cast<CRV64Assembler::REGISTER64>(g_registers[src1->m_valueLow]), 0);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_Expand_VarMem(const STATEMENT& statement)
{
    //assert(false);
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e32<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = GetNextTempRegister();

	LoadMemoryInRegister(src1Reg, src1);

	//m_assembler.Dup_4s(dstReg, src1Reg);

    /*auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);*/

    m_assembler.Vmvvx(dstReg, static_cast<CRV64Assembler::REGISTER64>(src1Reg), 0);

    CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_Expand_VarCst(const STATEMENT& statement)
{
    //assert(false);
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e32<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = GetNextTempRegister();

    /*auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);*/

    if(src1->m_valueLow == 0) // check if it can fit in 5 bit signed imm
	{
		//m_assembler.Eor_16b(dstReg, dstReg, dstReg);
        m_assembler.Vmvvi(dstReg, 0, 0);
	}
	else
	{
		LoadConstantInRegister(src1Reg, src1->m_valueLow);
		//m_assembler.Dup_4s(dstReg, src1Reg);
        m_assembler.Li(tmpReg, src1->m_valueLow);
        m_assembler.Vmvvx(dstReg, tmpReg, 0);
	}

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_PackHB_VarVarVar(const STATEMENT& statement)
{
    //assert(false);
    auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	/*auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());*/

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e16<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, CRV64Assembler::v1);
	auto src1Reg = PrepareSymbolRegisterUseMd(src2, CRV64Assembler::v2);
	auto src2Reg = PrepareSymbolRegisterUseMd(src1, CRV64Assembler::v3);
    assert(dstReg == CRV64Assembler::v1);
    assert(src1Reg == CRV64Assembler::v2);
    assert(src2Reg == CRV64Assembler::v3);

    /*if(dstReg == src1Reg)
	{
		auto tmpReg = GetNextTempRegisterMd();
		m_assembler.Xtn1_8b(tmpReg, src2Reg);
		m_assembler.Xtn2_16b(tmpReg, src1Reg);
		m_assembler.Mov(dstReg, tmpReg);
	}
	else
	{
		m_assembler.Xtn1_8b(dstReg, src2Reg);
		m_assembler.Xtn2_16b(dstReg, src1Reg);
	}*/

    /*auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);
    m_assembler.Vlwv(CRV64Assembler::v8, src2AddrReg, 0);
    m_assembler.Vlwv(CRV64Assembler::v9, src1AddrReg, 0);
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 16);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 0);
    //m_assembler.Vnsrlvi(CRV64Assembler::v2, CRV64Assembler::v8, 16, 0);
    m_assembler.Vnsrlvi(CRV64Assembler::v2, CRV64Assembler::v8, 0, 0);
    m_assembler.Vsbv(CRV64Assembler::v2, dstAddrReg, 0);*/

    //auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 16);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 0);
    m_assembler.Vnsrlvi(dstReg, src1Reg, 0, 0);

    CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_PackWH_VarVarVar(const STATEMENT& statement)
{
    //assert(false);
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	/*auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());*/

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e32<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, CRV64Assembler::v1);
	auto src1Reg = PrepareSymbolRegisterUseMd(src2, CRV64Assembler::v2);
	auto src2Reg = PrepareSymbolRegisterUseMd(src1, CRV64Assembler::v3);
    assert(dstReg == CRV64Assembler::v1);
    assert(src1Reg == CRV64Assembler::v2);
    assert(src2Reg == CRV64Assembler::v3);

    /*if(dstReg == src1Reg)
	{
		auto tmpReg = GetNextTempRegisterMd();
		m_assembler.Xtn1_4h(tmpReg, src2Reg);
		m_assembler.Xtn2_8h(tmpReg, src1Reg);
		m_assembler.Mov(dstReg, tmpReg);
	}
	else
	{
		m_assembler.Xtn1_4h(dstReg, src2Reg);
		m_assembler.Xtn2_8h(dstReg, src1Reg);
	}*/

    //auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 4);
    m_assembler.Vnsrlvi(dstReg, src1Reg, 0, 0);

    CommitSymbolRegisterMd(dst, dstReg);
}

template <uint32 offset>
void CCodeGen_RV64::Emit_Md_UnpackBH_VarVarVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 16);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e8<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, CRV64Assembler::v2);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, CRV64Assembler::v1);
    assert(dstReg == CRV64Assembler::v2);
    assert(src1Reg == CRV64Assembler::v1);

    auto tmpMdReg = CRV64Assembler::v0;

    m_assembler.Vwaddvx(dstReg, src1Reg, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, (CRV64Assembler::VSEW::e16<<2));

    m_assembler.Vsllvi(tmpMdReg, dstReg, 8, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 16);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e8<<2));

    auto src2Reg = PrepareSymbolRegisterUseMd(src2, CRV64Assembler::v1);
    assert(src2Reg == CRV64Assembler::v1);

    m_assembler.Vwaddvx(dstReg, src2Reg, CRV64Assembler::xZR, 0);

    m_assembler.Vorvv(dstReg, dstReg, tmpMdReg, 0);

    CommitSymbolRegisterMd(dst, dstReg);
}

template <uint32 offset>
void CCodeGen_RV64::Emit_Md_UnpackUpperBH_VarVarVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 16);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e8<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, CRV64Assembler::v2);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, CRV64Assembler::v0);
    assert(dstReg == CRV64Assembler::v2);
    assert(src1Reg == CRV64Assembler::v0);

    auto tmpMdReg = CRV64Assembler::v3;
    auto tmp1MdReg = CRV64Assembler::v1;

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 2);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e64<<2));

    m_assembler.Vslide1downvx(tmp1MdReg, src1Reg, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 16);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e8<<2));

    m_assembler.Vwaddvx(dstReg, tmp1MdReg, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, (CRV64Assembler::VSEW::e16<<2));

    m_assembler.Vsllvi(tmp1MdReg, dstReg, 8, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 16);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e8<<2));

    auto src2Reg = PrepareSymbolRegisterUseMd(src2, CRV64Assembler::v0);
    assert(src2Reg == CRV64Assembler::v0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 2);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e64<<2));

    m_assembler.Vslide1downvx(tmpMdReg, src2Reg, CRV64Assembler::xZR, 0);
    m_assembler.Vmvvv(CRV64Assembler::v0, tmpMdReg, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e8<<2));

    //m_assembler.Vwadduvx(dstReg, CRV64Assembler::v0, CRV64Assembler::xZR, 0);
    m_assembler.Vwadduvx(dstReg, CRV64Assembler::v0, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 16);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e8<<2));

    //m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
    //m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, (CRV64Assembler::VSEW::e16<<2));
    m_assembler.Vorvv(dstReg, dstReg, tmp1MdReg, 0);
    //m_assembler.Vmvvv(dstReg, tmp1MdReg, 0);

    CommitSymbolRegisterMd(dst, dstReg);
}

template <uint32 offset>
void CCodeGen_RV64::Emit_Md_UnpackHW_VarVarVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e16<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, CRV64Assembler::v2);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, CRV64Assembler::v1);
    assert(dstReg == CRV64Assembler::v2);
    assert(src1Reg == CRV64Assembler::v1);

    auto tmpMdReg = CRV64Assembler::v0;

    m_assembler.Vwaddvx(dstReg, src1Reg, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, (CRV64Assembler::VSEW::e32<<2));

    m_assembler.Vsllvi(tmpMdReg, dstReg, 16, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e16<<2));

    auto src2Reg = PrepareSymbolRegisterUseMd(src2, CRV64Assembler::v1);
    assert(src2Reg == CRV64Assembler::v1);

    m_assembler.Vwaddvx(dstReg, src2Reg, CRV64Assembler::xZR, 0);

    m_assembler.Vorvv(dstReg, dstReg, tmpMdReg, 0);

    CommitSymbolRegisterMd(dst, dstReg);
}

template <uint32 offset>
void CCodeGen_RV64::Emit_Md_UnpackUpperHW_VarVarVar(const STATEMENT& statement)
{
    /*auto srcVL = 16;
    auto srcVSEW = CRV64Assembler::VSEW::e8;
    auto dstVL = 8;
    auto dstVSEW = CRV64Assembler::VSEW::e16;*/
    auto srcVL = 8;
    auto srcVSEW = CRV64Assembler::VSEW::e16;
    auto dstVL = 4;
    auto dstVSEW = CRV64Assembler::VSEW::e32;
    auto shift = 16;

    auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, srcVL);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (srcVSEW<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, CRV64Assembler::v2);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, CRV64Assembler::v0);
    assert(dstReg == CRV64Assembler::v2);
    assert(src1Reg == CRV64Assembler::v0);

    auto tmpMdReg = CRV64Assembler::v3;
    auto tmp1MdReg = CRV64Assembler::v1;

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 2);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e64<<2));

    m_assembler.Vslide1downvx(tmp1MdReg, src1Reg, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, srcVL);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (srcVSEW<<2));

    m_assembler.Vwaddvx(dstReg, tmp1MdReg, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, dstVL);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, (dstVSEW<<2));

    m_assembler.Vsllvi(tmp1MdReg, dstReg, shift, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, srcVL);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (srcVSEW<<2));

    auto src2Reg = PrepareSymbolRegisterUseMd(src2, CRV64Assembler::v0);
    assert(src2Reg == CRV64Assembler::v0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 2);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e64<<2));

    m_assembler.Vslide1downvx(tmpMdReg, src2Reg, CRV64Assembler::xZR, 0);
    m_assembler.Vmvvv(CRV64Assembler::v0, tmpMdReg, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, srcVL/2);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (srcVSEW<<2));

    //m_assembler.Vwadduvx(dstReg, CRV64Assembler::v0, CRV64Assembler::xZR, 0);
    m_assembler.Vwadduvx(dstReg, CRV64Assembler::v0, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, srcVL);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (srcVSEW<<2));

    //m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
    //m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, (CRV64Assembler::VSEW::e16<<2));
    m_assembler.Vorvv(dstReg, dstReg, tmp1MdReg, 0);
    //m_assembler.Vmvvv(dstReg, tmp1MdReg, 0);

    CommitSymbolRegisterMd(dst, dstReg);
}

template <uint32 offset>
void CCodeGen_RV64::Emit_Md_UnpackWD_VarVarVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e32<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, CRV64Assembler::v2);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, CRV64Assembler::v1);
    assert(dstReg == CRV64Assembler::v2);
    assert(src1Reg == CRV64Assembler::v1);

    auto tmpMdReg = CRV64Assembler::v0;

    m_assembler.Vwaddvx(dstReg, src1Reg, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 2);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, (CRV64Assembler::VSEW::e64<<2));

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 32);
    m_assembler.Vsllvx(tmpMdReg, dstReg, tmpReg, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e32<<2));

    auto src2Reg = PrepareSymbolRegisterUseMd(src2, CRV64Assembler::v1);
    assert(src2Reg == CRV64Assembler::v1);

    m_assembler.Vwaddvx(dstReg, src2Reg, CRV64Assembler::xZR, 0);

    m_assembler.Vorvv(dstReg, dstReg, tmpMdReg, 0);

    CommitSymbolRegisterMd(dst, dstReg);
}

template <uint32 offset>
void CCodeGen_RV64::Emit_Md_UnpackUpperWD_VarVarVar(const STATEMENT& statement)
{
    /*auto srcVL = 16;
    auto srcVSEW = CRV64Assembler::VSEW::e8;
    auto dstVL = 8;
    auto dstVSEW = CRV64Assembler::VSEW::e16;*/
    auto srcVL = 4;
    auto srcVSEW = CRV64Assembler::VSEW::e32;
    auto dstVL = 2;
    auto dstVSEW = CRV64Assembler::VSEW::e64;
    auto shift = 32;

    auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, srcVL);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (srcVSEW<<2));

    auto dstReg = PrepareSymbolRegisterDefMd(dst, CRV64Assembler::v2);
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, CRV64Assembler::v0);
    assert(dstReg == CRV64Assembler::v2);
    assert(src1Reg == CRV64Assembler::v0);

    auto tmpMdReg = CRV64Assembler::v3;
    auto tmp1MdReg = CRV64Assembler::v1;

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 2);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e64<<2));

    m_assembler.Vslide1downvx(tmp1MdReg, src1Reg, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, srcVL);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (srcVSEW<<2));

    m_assembler.Vwaddvx(dstReg, tmp1MdReg, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, dstVL);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, (dstVSEW<<2));

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, shift);
    m_assembler.Vsllvx(tmp1MdReg, dstReg, tmpReg, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, srcVL);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (srcVSEW<<2));

    auto src2Reg = PrepareSymbolRegisterUseMd(src2, CRV64Assembler::v0);
    assert(src2Reg == CRV64Assembler::v0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 2);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e64<<2));

    m_assembler.Vslide1downvx(tmpMdReg, src2Reg, CRV64Assembler::xZR, 0);
    m_assembler.Vmvvv(CRV64Assembler::v0, tmpMdReg, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, srcVL/2);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (srcVSEW<<2));

    //m_assembler.Vwadduvx(dstReg, CRV64Assembler::v0, CRV64Assembler::xZR, 0);
    m_assembler.Vwadduvx(dstReg, CRV64Assembler::v0, CRV64Assembler::xZR, 0);

    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, srcVL);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (srcVSEW<<2));

    //m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 8);
    //m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, (CRV64Assembler::VSEW::e16<<2));
    m_assembler.Vorvv(dstReg, dstReg, tmp1MdReg, 0);
    //m_assembler.Vmvvv(dstReg, tmp1MdReg, 0);

    CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_RV64::Emit_MergeTo256_MemVarVar(const STATEMENT& statement)
{
    //assert(false);
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY256);

	auto dstLoAddrReg = GetNextTempRegister64();
	auto dstHiAddrReg = GetNextTempRegister64();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e32<<2));

    auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());

	LoadTemporary256ElementAddressInRegister(dstLoAddrReg, dst, 0x00);
	LoadTemporary256ElementAddressInRegister(dstHiAddrReg, dst, 0x10);

	//m_assembler.St1_4s(src1Reg, dstLoAddrReg);
	//m_assembler.St1_4s(src2Reg, dstHiAddrReg);

    //auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);

    m_assembler.Vswv(src1Reg, dstLoAddrReg, 0);
    m_assembler.Vswv(src2Reg, dstHiAddrReg, 0);
}

void CCodeGen_RV64::Emit_Md_Srl256_VarMemCst(const STATEMENT& statement)
{
    //assert(false);
    auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY256);
	assert(src2->m_type == SYM_CONSTANT);

    uint32 width = CRV64Assembler::VWidth::VectorByte;
    uint32 lumop_rs2_vs2 = 0;
    uint32 sumop_rs2_vs2 = 0;
    uint32 vm = 1; // unmasked
    //uint32 lmop = 4; // sign-extended unit-stride
    uint32 lmop = 0; // zero-extended unit-stride
    uint32 smop = 0; // unit-stride
    uint32 nf = 0;

    int vl = 16; // load 5 elements, slide down by the 2 bit offset (multiples of 4)
    auto sew = CRV64Assembler::VSEW::e8;

    auto tmp1Reg = GetNextTempRegister64();
    m_assembler.Addi(tmp1Reg, CRV64Assembler::xZR, vl);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmp1Reg), (sew<<2));

    /*auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, tmpReg, 8);*/

    auto src1AddrReg = GetNextTempRegister64();
	//auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
    auto dstReg = PrepareSymbolRegisterDefMd(dst, CRV64Assembler::v0);

	uint32 offset = (src2->m_valueLow & 0x7F) / 8;
    //assert((offset&0xFFFFFFFC) == offset);
	LoadTemporary256ElementAddressInRegister(src1AddrReg, src1, (offset&0xFFFFFFFF));

	//m_assembler.Ld1_4s(dstReg, src1AddrReg);

    //m_assembler.Vlwv(dstReg, src1AddrReg, 0);
    m_assembler.Vloadv(dstReg, src1AddrReg, width, lumop_rs2_vs2, vm, lmop, nf);
    //m_assembler.Break();

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_RV64::Emit_Md_Srl256_VarMemVar(const STATEMENT& statement)
{
    //assert(false);
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY256);

	auto offsetRegister = GetNextTempRegister();
	auto src1AddrReg = GetNextTempRegister64();

    auto tmpReg = GetNextTempRegister64();
    m_assembler.Addi(tmpReg, CRV64Assembler::xZR, 4);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e32<<2));

    auto src2Register = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());

	LoadTemporary256ElementAddressInRegister(src1AddrReg, src1, 0);

	//Compute offset and modify address
	/*LOGICAL_IMM_PARAMS logicalImmParams;
	FRAMEWORK_MAYBE_UNUSED bool result = TryGetLogicalImmParams(0x7F, logicalImmParams);
	assert(result);
	//m_assembler.And(offsetRegister, src2Register, logicalImmParams.n, logicalImmParams.immr, logicalImmParams.imms);
	m_assembler.Lsr(offsetRegister, offsetRegister, 3);
	m_assembler.Add(src1AddrReg, src1AddrReg, static_cast<CRV64Assembler::REGISTER64>(offsetRegister));*/

	//m_assembler.Ld1_4s(dstReg, src1AddrReg);

    //auto tmpReg = GetNextTempRegister();
    m_assembler.Li(tmpReg, 0x7f);
    //m_assembler.And(offsetRegister, src2Register, tmpReg);
    m_assembler.Andw(offsetRegister, src2Register, static_cast<CRV64Assembler::REGISTER32>(tmpReg));
    m_assembler.Lsr(offsetRegister, offsetRegister, 3);
    m_assembler.Add(src1AddrReg, src1AddrReg, static_cast<CRV64Assembler::REGISTER64>(offsetRegister));

    //m_assembler.Addi(static_cast<CRV64Assembler::REGISTER64>(tmpReg), CRV64Assembler::xZR, 4);
    //m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), 8);
    m_assembler.Addi(static_cast<CRV64Assembler::REGISTER64>(tmpReg), CRV64Assembler::xZR, 16);
    m_assembler.Vsetvli(CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(tmpReg), (CRV64Assembler::VSEW::e8<<2));

    m_assembler.Vlev(dstReg, src1AddrReg, 0);
    //m_assembler.Break();

    CommitSymbolRegisterMd(dst, dstReg);
}

CCodeGen_RV64::CONSTMATCHER CCodeGen_RV64::g_mdConstMatchersRVV[] =
{
#if 1
    { OP_MD_ADD_B,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_ADDB>                  },
    { OP_MD_ADD_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_ADDH>                  },
    { OP_MD_ADD_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_ADDW>                  },

    { OP_MD_ADDUS_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_ADDBUS>                },
    { OP_MD_ADDUS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_ADDHUS>                },
    { OP_MD_ADDUS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_ADDWUS>                },

    { OP_MD_ADDSS_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_ADDBSS>                },
    { OP_MD_ADDSS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_ADDHSS>                },
    { OP_MD_ADDSS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_ADDWSS>                },

    { OP_MD_SUB_B,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_SUBB>                  },
    { OP_MD_SUB_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_SUBH>                  },
    { OP_MD_SUB_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_SUBW>                  },

    { OP_MD_SUBUS_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_SUBBUS>                },
    { OP_MD_SUBUS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_SUBHUS>                },
    { OP_MD_SUBUS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_SUBWUS>                },

    { OP_MD_SUBSS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_SUBHSS>                },
    { OP_MD_SUBSS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_SUBWSS>                },

    { OP_MD_CLAMP_S,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_ClampS_VarVar                         },

    { OP_MD_CMPEQ_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_CMPEQB>                },
    { OP_MD_CMPEQ_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_CMPEQH>                },
    { OP_MD_CMPEQ_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_CMPEQW>                },

    { OP_MD_CMPGT_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_CMPGTB>                },
    { OP_MD_CMPGT_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_CMPGTH>                },
    { OP_MD_CMPGT_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_CMPGTW>                },

    { OP_MD_MIN_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_MINH>                  },
    { OP_MD_MIN_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_MINW>                  },

    { OP_MD_MAX_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_MAXH>                  },
    { OP_MD_MAX_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_MAXW>                  },

    { OP_MD_ADD_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_ADDS>                  },
    { OP_MD_SUB_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_SUBS>                  },
    { OP_MD_MUL_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_MULS>                  },
    { OP_MD_DIV_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_DIVS>                  },

    { OP_MD_ABS_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVar<MDOP_ABSS>                     },
    { OP_MD_MIN_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_MINS>                  },
    { OP_MD_MAX_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_MAXS>                  },

    { OP_MD_CMPLT_S,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVarRev<MDOP_CMPGTS>             },
    { OP_MD_CMPGT_S,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_CMPGTS>                },

    { OP_MD_AND,                MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_AND>                   },
    { OP_MD_OR,                 MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_OR>                    },
    { OP_MD_XOR,                MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVar<MDOP_XOR>                   },
    { OP_MD_NOT,                MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVar<MDOP_NOT>                      },

    { OP_MD_SLLH,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_VarVarCst<MDOP_SLLH>            },
    { OP_MD_SLLW,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_VarVarCst<MDOP_SLLW>            },

    { OP_MD_SRLH,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_VarVarCst<MDOP_SRLH>            },
    { OP_MD_SRLW,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_VarVarCst<MDOP_SRLW>            },

    { OP_MD_SRAH,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_VarVarCst<MDOP_SRAH>            },
    { OP_MD_SRAW,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_RV64::Emit_Md_Shift_VarVarCst<MDOP_SRAW>            },

    { OP_MD_SRL256,             MATCH_VARIABLE128,    MATCH_MEMORY256,      MATCH_VARIABLE,         MATCH_NIL, &CCodeGen_RV64::Emit_Md_Srl256_VarMemVar                      },
    { OP_MD_SRL256,             MATCH_VARIABLE128,    MATCH_MEMORY256,      MATCH_CONSTANT,         MATCH_NIL, &CCodeGen_RV64::Emit_Md_Srl256_VarMemCst                      },

    { OP_MD_MAKESZ,             MATCH_VARIABLE,       MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_MakeSz_VarVar                         },

    { OP_MD_TOSINGLE,           MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVar<MDOP_TOSINGLE>                 },
    { OP_MD_TOWORD_TRUNCATE,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVar<MDOP_TOWORD>                   },

    { OP_LOADFROMREF,           MATCH_VARIABLE128,    MATCH_VAR_REF,        MATCH_NIL,              MATCH_NIL,         &CCodeGen_RV64::Emit_Md_LoadFromRef_VarVar            },
    { OP_LOADFROMREF,           MATCH_VARIABLE128,    MATCH_VAR_REF,        MATCH_ANY32,            MATCH_NIL,         &CCodeGen_RV64::Emit_Md_LoadFromRef_VarVarAny         },
    { OP_STOREATREF,            MATCH_NIL,            MATCH_VAR_REF,        MATCH_VARIABLE128,      MATCH_NIL,         &CCodeGen_RV64::Emit_Md_StoreAtRef_VarVar             },
    { OP_STOREATREF,            MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY32,            MATCH_VARIABLE128, &CCodeGen_RV64::Emit_Md_StoreAtRef_VarAnyVar          },

    { OP_MD_MOV_MASKED,         MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_MovMasked_VarVarVar                   },

    { OP_MD_EXPAND,             MATCH_VARIABLE128,    MATCH_REGISTER,       MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_Expand_VarReg                         },
    { OP_MD_EXPAND,             MATCH_VARIABLE128,    MATCH_MEMORY,         MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_Expand_VarMem                         },
    { OP_MD_EXPAND,             MATCH_VARIABLE128,    MATCH_CONSTANT,       MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_Expand_VarCst                         },

    { OP_MD_PACK_HB,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_PackHB_VarVarVar                      },
    { OP_MD_PACK_WH,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_PackWH_VarVarVar                      },

    /*{ OP_MD_UNPACK_LOWER_BH,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVarRev<MDOP_UNPACK_LOWER_BH>    },
    { OP_MD_UNPACK_LOWER_HW,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVarRev<MDOP_UNPACK_LOWER_HW>    },
    { OP_MD_UNPACK_LOWER_WD,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVarRev<MDOP_UNPACK_LOWER_WD>    },

    { OP_MD_UNPACK_UPPER_BH,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVarRev<MDOP_UNPACK_UPPER_BH>    },
    { OP_MD_UNPACK_UPPER_HW,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVarRev<MDOP_UNPACK_UPPER_HW>    },
    { OP_MD_UNPACK_UPPER_WD,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_VarVarVarRev<MDOP_UNPACK_UPPER_WD>    },*/

    { OP_MD_UNPACK_LOWER_BH,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackBH_VarVarVar<0>                 },
    { OP_MD_UNPACK_LOWER_HW,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackHW_VarVarVar<0>                 },
    { OP_MD_UNPACK_LOWER_WD,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackWD_VarVarVar<0>                 },

    { OP_MD_UNPACK_UPPER_BH,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackUpperBH_VarVarVar<8>            },
    { OP_MD_UNPACK_UPPER_HW,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackUpperHW_VarVarVar<8>            },
    { OP_MD_UNPACK_UPPER_WD,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_Md_UnpackUpperWD_VarVarVar<8>            },

    { OP_MOV,                   MATCH_REGISTER128,    MATCH_REGISTER128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_Mov_RegReg                            },
    { OP_MOV,                   MATCH_REGISTER128,    MATCH_MEMORY128,      MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_Mov_RegMem                            },
    { OP_MOV,                   MATCH_MEMORY128,      MATCH_REGISTER128,    MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_Mov_MemReg                            },
    { OP_MOV,                   MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,              MATCH_NIL, &CCodeGen_RV64::Emit_Md_Mov_MemMem_RVV                        },

    { OP_MERGETO256,            MATCH_MEMORY256,      MATCH_VARIABLE128,    MATCH_VARIABLE128,      MATCH_NIL, &CCodeGen_RV64::Emit_MergeTo256_MemVarVar                     },

    { OP_MOV,                   MATCH_NIL,            MATCH_NIL,            MATCH_NIL,              MATCH_NIL, nullptr                                                          },
#endif
};
