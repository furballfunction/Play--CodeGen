#include <algorithm>
#include <stdexcept>
#include "Jitter_CodeGen_RV64.h"
#include "BitManip.h"

#include <sys/utsname.h>
#include <stdio.h>
#include <string.h>

using namespace Jitter;

CRV64Assembler::REGISTER32    CCodeGen_RV64::g_registers[MAX_REGISTERS] =
{
    CRV64Assembler::s3,
    CRV64Assembler::s4,
    CRV64Assembler::s5,
    CRV64Assembler::s6,
    CRV64Assembler::s7,
    CRV64Assembler::s8,
    CRV64Assembler::s9,
    CRV64Assembler::s10,
    CRV64Assembler::s11,
};

CRV64Assembler::REGISTERMD    CCodeGen_RV64::g_registersMd[MAX_MDREGISTERS] =
{
    //v0-v3 are used for work values, v7-v15 need to be preserved
    CRV64Assembler::v4,  CRV64Assembler::v5,  CRV64Assembler::v6,  CRV64Assembler::v7,
    CRV64Assembler::v16, CRV64Assembler::v17, CRV64Assembler::v18, CRV64Assembler::v19,
    CRV64Assembler::v20, CRV64Assembler::v21, CRV64Assembler::v22, CRV64Assembler::v23,
    CRV64Assembler::v24, CRV64Assembler::v25, CRV64Assembler::v26, CRV64Assembler::v27,
    CRV64Assembler::v28, CRV64Assembler::v29, CRV64Assembler::v30, CRV64Assembler::v31,
};

CRV64Assembler::REGISTER32    CCodeGen_RV64::g_tempRegisters[MAX_TEMP_REGS] =
{
    CRV64Assembler::t0,
    CRV64Assembler::t1,
    CRV64Assembler::t2,
    CRV64Assembler::t3,
    CRV64Assembler::t4,
    CRV64Assembler::t5,
    CRV64Assembler::t6
};

CRV64Assembler::REGISTER64    CCodeGen_RV64::g_tempRegisters64[MAX_TEMP_REGS] =
{
    CRV64Assembler::x5,
    CRV64Assembler::x6,
    CRV64Assembler::x7,
    CRV64Assembler::x28,
    CRV64Assembler::x29,
    CRV64Assembler::x30,
    CRV64Assembler::x31
};

CRV64Assembler::REGISTERMD    CCodeGen_RV64::g_tempRegistersMd[MAX_TEMP_MD_REGS] =
{
    CRV64Assembler::v0,
    CRV64Assembler::v1,
    CRV64Assembler::v2,
    CRV64Assembler::v3,
};

CRV64Assembler::REGISTER32    CCodeGen_RV64::g_paramRegisters[MAX_PARAM_REGS] =
{
    CRV64Assembler::a0,
    CRV64Assembler::a1,
    CRV64Assembler::a2,
    CRV64Assembler::a3,
    CRV64Assembler::a4,
    CRV64Assembler::a5,
    CRV64Assembler::a6,
    CRV64Assembler::a7,
};

CRV64Assembler::REGISTER64    CCodeGen_RV64::g_paramRegisters64[MAX_PARAM_REGS] =
{
    CRV64Assembler::x10,
    CRV64Assembler::x11,
    CRV64Assembler::x12,
    CRV64Assembler::x13,
    CRV64Assembler::x14,
    CRV64Assembler::x15,
    CRV64Assembler::x16,
    CRV64Assembler::x17,
};

CRV64Assembler::REGISTER64    CCodeGen_RV64::g_baseRegister = CRV64Assembler::x18;

const LITERAL128 CCodeGen_RV64::g_fpClampMask1(0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF);
const LITERAL128 CCodeGen_RV64::g_fpClampMask2(0xFF7FFFFF, 0xFF7FFFFF, 0xFF7FFFFF, 0xFF7FFFFF);

static bool isMask(uint32 value)
{
    return value && (((value + 1) & value) == 0);
}

static bool isShiftedMask(uint32 value)
{
    return value && isMask((value - 1) | value);
}

template <typename AddSubOp>
void CCodeGen_RV64::Emit_AddSub_VarAnyVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
    ((m_assembler).*(AddSubOp::OpReg()))(dstReg, src1Reg, src2Reg);
    CommitSymbolRegister(dst, dstReg);
}

template <typename AddOp>
void CCodeGen_RV64::Emit_Add_VarVarCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(src2->m_type == SYM_CONSTANT);

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());

    ADDSUB_IMM_PARAMS addSubImmParams;
    if(TryGetAddSubImmParams(src2->m_valueLow, addSubImmParams))
    {
        ((m_assembler).*(AddOp::OpImm()))(dstReg, src1Reg, addSubImmParams.imm);
    }
    else
    {
        auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        ((m_assembler).*(AddOp::OpReg()))(dstReg, src1Reg, src2Reg);
    }

    CommitSymbolRegister(dst, dstReg);
}

template <typename SubOp>
void CCodeGen_RV64::Emit_Sub_VarVarCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(src2->m_type == SYM_CONSTANT);

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());

    ADDSUB_IMM_PARAMS addSubImmParams;
    if(TryGetAddSubImmParams(-static_cast<int32>(src2->m_valueLow), addSubImmParams))
    {
        ((m_assembler).*(SubOp::OpImmRev()))(dstReg, src1Reg, addSubImmParams.imm);
    }
    else
    {
        auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        ((m_assembler).*(SubOp::OpReg()))(dstReg, src1Reg, src2Reg);
    }

    CommitSymbolRegister(dst, dstReg);
}

template <typename ShiftOp>
void CCodeGen_RV64::Emit_Shift_VarAnyVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
    ((m_assembler).*(ShiftOp::OpReg()))(dstReg, src1Reg, src2Reg);
    CommitSymbolRegister(dst, dstReg);
}

template <typename ShiftOp>
void CCodeGen_RV64::Emit_Shift_VarVarCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(src2->m_type == SYM_CONSTANT);

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    ((m_assembler).*(ShiftOp::OpImm()))(dstReg, src1Reg, src2->m_valueLow);
    CommitSymbolRegister(dst, dstReg);
}

template <typename LogicOp>
void CCodeGen_RV64::Emit_Logic_VarAnyVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
    ((m_assembler).*(LogicOp::OpReg()))(dstReg, src1Reg, src2Reg);
    CommitSymbolRegister(dst, dstReg);
}

template <typename LogicOp>
void CCodeGen_RV64::Emit_Logic_VarVarCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(src2->m_type == SYM_CONSTANT);
    assert(src2->m_valueLow != 0);

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());

    LOGICAL_IMM_PARAMS logicalImmParams;
    if(TryGetLogicalImmParams(src2->m_valueLow, logicalImmParams))
    {
        /*((m_assembler).*(LogicOp::OpImm()))(dstReg, src1Reg,
            logicalImmParams.n, logicalImmParams.immr, logicalImmParams.imms);*/
        ((m_assembler).*(LogicOp::OpImm()))(dstReg, src1Reg, logicalImmParams.imm);
    }
    else
    {
        auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        ((m_assembler).*(LogicOp::OpReg()))(dstReg, src1Reg, src2Reg);
    }
    CommitSymbolRegister(dst, dstReg);
}

template <bool isSigned>
void CCodeGen_RV64::Emit_Mul_Tmp64AnyAny(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(dst->m_type == SYM_TEMPORARY64);

    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
    auto dstReg = GetNextTempRegister64();

    if(isSigned)
    {
        m_assembler.Smull(dstReg, src1Reg, src2Reg);
        //m_assembler.Smull(resLoReg, src1Reg, src2Reg, resHiReg);
        //m_assembler.Smull(dstReg, src1Reg, src2Reg, CRV64Assembler::xZR);
        //auto resTmp1 = GetNextTempRegister64();
        //auto resTmp2 = GetNextTempRegister64();
        //m_assembler.Smull(dstReg, src1Reg, src2Reg, resTmp1, resTmp2);
    }
    else
    {
        auto resTmp1 = GetNextTempRegister64();
        auto resTmp2 = GetNextTempRegister64();
        //m_assembler.Umull(dstReg, src1Reg, src2Reg);
        //m_assembler.Umull(resLoReg, src1Reg, src2Reg, resHiReg);
        //m_assembler.Umull(dstReg, src1Reg, src2Reg, CRV64Assembler::xZR);
        m_assembler.Umull(dstReg, src1Reg, src2Reg, resTmp1, resTmp2);
        //m_assembler.Umull(dstReg, src1Reg, src2Reg);
    }

    //m_assembler.Mov(dstReg, resLoReg);
    //m_assembler.Mov(dstReg, resHiReg);
    m_assembler.Str(dstReg, CRV64Assembler::xSP, dst->m_stackLocation);
    //m_assembler.Str(CRV64Assembler::xZR, CRV64Assembler::xSP, dst->m_stackLocation);
    //m_assembler.Str(resHiReg, CRV64Assembler::xSP, dst->m_stackLocation);
    //m_assembler.Str(resHiReg, CRV64Assembler::xSP, dst->m_stackLocation+4);
}

template <bool isSigned>
void CCodeGen_RV64::Emit_Div_Tmp64AnyAny(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(dst->m_type == SYM_TEMPORARY64);

    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
    auto resReg = GetNextTempRegister();
    auto modReg = GetNextTempRegister();

    if(isSigned)
    {
        m_assembler.Sdiv(resReg, src1Reg, src2Reg);
    }
    else
    {
        m_assembler.Udiv(resReg, src1Reg, src2Reg);
    }

    m_assembler.Msub(modReg, resReg, src2Reg, src1Reg);

    m_assembler.Str(resReg, CRV64Assembler::xSP, dst->m_stackLocation + 0);
    m_assembler.Str(modReg, CRV64Assembler::xSP, dst->m_stackLocation + 4);
}

#define LOGIC_CONST_MATCHERS(LOGICOP_CST, LOGICOP) \
    { LOGICOP_CST,          MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,    MATCH_NIL, &CCodeGen_RV64::Emit_Logic_VarVarCst<LOGICOP>        }, \
    { LOGICOP_CST,          MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,    MATCH_NIL, &CCodeGen_RV64::Emit_Logic_VarAnyVar<LOGICOP>        },

CCodeGen_RV64::CONSTMATCHER CCodeGen_RV64::g_constMatchers[] =
{
    { OP_NOP,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Nop                                 },
    { OP_BREAK,          MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Break                               },

    { OP_MOV,            MATCH_REGISTER,       MATCH_REGISTER,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Mov_RegReg                          },
    { OP_MOV,            MATCH_REGISTER,       MATCH_MEMORY,         MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Mov_RegMem                          },
    { OP_MOV,            MATCH_REGISTER,       MATCH_CONSTANT,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Mov_RegCst                          },
    { OP_MOV,            MATCH_MEMORY,         MATCH_REGISTER,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Mov_MemReg                          },
    { OP_MOV,            MATCH_MEMORY,         MATCH_MEMORY,         MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Mov_MemMem                          },
    { OP_MOV,            MATCH_MEMORY,         MATCH_CONSTANT,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Mov_MemCst                          },

    { OP_MOV,            MATCH_REG_REF,        MATCH_MEM_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Mov_RegRefMemRef                    },
    { OP_MOV,            MATCH_MEM_REF,        MATCH_REG_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Mov_MemRefRegRef                    },

    { OP_NOT,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Not_VarVar                          },
    { OP_LZC,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Lzc_VarVar                          },

    { OP_RELTOREF,       MATCH_VAR_REF,        MATCH_CONSTANT,       MATCH_ANY,           MATCH_NIL,      &CCodeGen_RV64::Emit_RelToRef_VarCst                     },

    { OP_ADDREF,         MATCH_VAR_REF,        MATCH_VAR_REF,        MATCH_ANY,           MATCH_NIL,      &CCodeGen_RV64::Emit_AddRef_VarVarAny                    },

    { OP_ISREFNULL,      MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_ANY,           MATCH_NIL,      &CCodeGen_RV64::Emit_IsRefNull_VarVar                    },

    { OP_LOADFROMREF,    MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_LoadFromRef_VarVar                  },
    { OP_LOADFROMREF,    MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL,      &CCodeGen_RV64::Emit_LoadFromRef_VarVarAny               },

    { OP_LOADFROMREF,    MATCH_VAR_REF,        MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_LoadFromRef_Ref_VarVar              },
    { OP_LOADFROMREF,    MATCH_VAR_REF,        MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL,      &CCodeGen_RV64::Emit_LoadFromRef_Ref_VarVarAny           },

    { OP_LOAD8FROMREF,   MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Load8FromRef_MemVar                 },
    { OP_LOAD8FROMREF,   MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL,      &CCodeGen_RV64::Emit_Load8FromRef_MemVarAny              },

    { OP_LOAD16FROMREF,  MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Load16FromRef_MemVar                },
    { OP_LOAD16FROMREF,  MATCH_VARIABLE,       MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL,      &CCodeGen_RV64::Emit_Load16FromRef_MemVarAny             },

    { OP_STOREATREF,     MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY32,         MATCH_NIL,      &CCodeGen_RV64::Emit_StoreAtRef_VarAny                   },
    { OP_STOREATREF,     MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY32,         MATCH_ANY32,    &CCodeGen_RV64::Emit_StoreAtRef_VarAnyAny                },

    { OP_STORE8ATREF,    MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY,           MATCH_NIL,      &CCodeGen_RV64::Emit_Store8AtRef_VarAny                  },
    { OP_STORE8ATREF,    MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY,           MATCH_ANY,      &CCodeGen_RV64::Emit_Store8AtRef_VarAnyAny               },

    { OP_STORE16ATREF,   MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY,           MATCH_NIL,      &CCodeGen_RV64::Emit_Store16AtRef_VarAny                 },
    { OP_STORE16ATREF,   MATCH_NIL,            MATCH_VAR_REF,        MATCH_ANY,           MATCH_ANY,      &CCodeGen_RV64::Emit_Store16AtRef_VarAnyAny              },

    { OP_PARAM,          MATCH_NIL,            MATCH_CONTEXT,        MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Param_Ctx                           },
    { OP_PARAM,          MATCH_NIL,            MATCH_REGISTER,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Param_Reg                           },
    { OP_PARAM,          MATCH_NIL,            MATCH_MEMORY,         MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Param_Mem                           },
    { OP_PARAM,          MATCH_NIL,            MATCH_CONSTANT,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Param_Cst                           },
    { OP_PARAM,          MATCH_NIL,            MATCH_MEMORY64,       MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Param_Mem64                         },
    { OP_PARAM,          MATCH_NIL,            MATCH_CONSTANT64,     MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Param_Cst64                         },
//#if SUPPORT_128
    { OP_PARAM,          MATCH_NIL,            MATCH_REGISTER128,    MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Param_Reg128                        },
    { OP_PARAM,          MATCH_NIL,            MATCH_MEMORY128,      MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Param_Mem128                        },
//#endif

    { OP_CALL,           MATCH_NIL,            MATCH_CONSTANTPTR,    MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_RV64::Emit_Call                                },

    { OP_RETVAL,         MATCH_REGISTER,       MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_RetVal_Reg                          },
    { OP_RETVAL,         MATCH_TEMPORARY,      MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_RetVal_Tmp                          },
    { OP_RETVAL,         MATCH_MEMORY64,       MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_RetVal_Mem64                        },
    { OP_RETVAL,         MATCH_REGISTER128,    MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_RetVal_Reg128                       },
    { OP_RETVAL,         MATCH_MEMORY128,      MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_RetVal_Mem128                       },

    { OP_EXTERNJMP,      MATCH_NIL,            MATCH_CONSTANTPTR,    MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_ExternJmp                           },
    { OP_EXTERNJMP_DYN,  MATCH_NIL,            MATCH_CONSTANTPTR,    MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_ExternJmpDynamic                    },

    { OP_JMP,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::Emit_Jmp                                 },

    { OP_CONDJMP,        MATCH_NIL,            MATCH_ANY,            MATCH_VARIABLE,      MATCH_NIL,      &CCodeGen_RV64::Emit_CondJmp_AnyVar                      },
    { OP_CONDJMP,        MATCH_NIL,            MATCH_VARIABLE,       MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_RV64::Emit_CondJmp_VarCst                      },

    { OP_CONDJMP,        MATCH_NIL,            MATCH_VAR_REF,        MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_RV64::Emit_CondJmp_Ref_VarCst                  },

    { OP_CMP,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      MATCH_NIL,      &CCodeGen_RV64::Emit_Cmp_VarAnyVar                       },
    { OP_CMP,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_RV64::Emit_Cmp_VarVarCst                       },

    { OP_SLL,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      MATCH_NIL,      &CCodeGen_RV64::Emit_Shift_VarAnyVar<SHIFTOP_LSL>        },
    { OP_SRL,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      MATCH_NIL,      &CCodeGen_RV64::Emit_Shift_VarAnyVar<SHIFTOP_LSR>        },
    { OP_SRA,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      MATCH_NIL,      &CCodeGen_RV64::Emit_Shift_VarAnyVar<SHIFTOP_ASR>        },

    { OP_SLL,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_RV64::Emit_Shift_VarVarCst<SHIFTOP_LSL>        },
    { OP_SRL,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_RV64::Emit_Shift_VarVarCst<SHIFTOP_LSR>        },
    { OP_SRA,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_RV64::Emit_Shift_VarVarCst<SHIFTOP_ASR>        },

    LOGIC_CONST_MATCHERS(OP_AND, LOGICOP_AND)
    LOGIC_CONST_MATCHERS(OP_OR,  LOGICOP_OR )
    LOGIC_CONST_MATCHERS(OP_XOR, LOGICOP_XOR)

    { OP_ADD,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      MATCH_NIL,      &CCodeGen_RV64::Emit_AddSub_VarAnyVar<ADDSUBOP_ADD>      },
    { OP_ADD,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_RV64::Emit_Add_VarVarCst<ADDSUBOP_ADD>         },
    { OP_SUB,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      MATCH_NIL,      &CCodeGen_RV64::Emit_AddSub_VarAnyVar<ADDSUBOP_SUB>      },
    { OP_SUB,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      MATCH_NIL,      &CCodeGen_RV64::Emit_Sub_VarVarCst<ADDSUBOP_SUB>         },

    { OP_MUL,            MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_RV64::Emit_Mul_Tmp64AnyAny<false>              },
    { OP_MULS,           MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_RV64::Emit_Mul_Tmp64AnyAny<true>               },

    { OP_DIV,            MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_RV64::Emit_Div_Tmp64AnyAny<false>              },
    { OP_DIVS,           MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           MATCH_NIL,      &CCodeGen_RV64::Emit_Div_Tmp64AnyAny<true>               },

    { OP_LABEL,          MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      &CCodeGen_RV64::MarkLabel                                },

    { OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL,      nullptr                                                     },
};

CCodeGen_RV64::CCodeGen_RV64()
{
    CheckMachine();
    const auto copyMatchers =
        [this](const CONSTMATCHER* constMatchers)
        {
            for(auto* constMatcher = constMatchers; constMatcher->emitter != nullptr; constMatcher++)
            {
                MATCHER matcher;
                matcher.op       = constMatcher->op;
                matcher.dstType  = constMatcher->dstType;
                matcher.src1Type = constMatcher->src1Type;
                matcher.src2Type = constMatcher->src2Type;
                matcher.src3Type = constMatcher->src3Type;
                matcher.emitter  = std::bind(constMatcher->emitter, this, std::placeholders::_1);
                m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
            }
        };

    copyMatchers(g_constMatchers);
    copyMatchers(g_64ConstMatchers);
    copyMatchers(g_fpuConstMatchers);

    // Only MD Mem is supported
    if (false && m_thead_extentions) {
        copyMatchers(g_mdConstMatchersRVV);
    } else {
        copyMatchers(g_mdConstMatchersMem);
    }
}

void CCodeGen_RV64::CheckMachine() {
    m_thead_extentions = false;
    struct utsname osInfo{};
    uname(&osInfo);
    printf("osInfo.nodename: %s\n", osInfo.nodename);
    if (strcmp(osInfo.nodename, "lpi4a") == 0) {
        printf("lpi4a detected using THEAD extentions\n");
        m_thead_extentions = true;
    }
}

void CCodeGen_RV64::SetGenerateRelocatableCalls(bool generateRelocatableCalls)
{
    m_generateRelocatableCalls = generateRelocatableCalls;
}

unsigned int CCodeGen_RV64::GetAvailableRegisterCount() const
{
    return MAX_REGISTERS;
}

unsigned int CCodeGen_RV64::GetAvailableMdRegisterCount() const
{
#if SUPPORT_128
    return MAX_MDREGISTERS;
#else
    return 0;
#endif
}

bool CCodeGen_RV64::Has128BitsCallOperands() const
{
    return true;
}

bool CCodeGen_RV64::CanHold128BitsReturnValueInRegisters() const
{
#if SUPPORT_128
    return true;
#else
    return true;
#endif
}

bool CCodeGen_RV64::SupportsExternalJumps() const
{
    return true;
}

uint32 CCodeGen_RV64::GetPointerSize() const
{
    return 8;
}

void CCodeGen_RV64::SetStream(Framework::CStream* stream)
{
    m_stream = stream;
    m_assembler.SetStream(stream);
}

void CCodeGen_RV64::RegisterExternalSymbols(CObjectFile* objectFile) const
{

}

void CCodeGen_RV64::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
    m_nextTempRegister = 0;
    m_nextTempRegisterMd = 0;

    //Align stack size (must be aligned on 16 bytes boundary)
    stackSize = (stackSize + 0xF) & ~0xF;

    m_registerSave = GetSavedRegisterList(GetRegisterUsage(statements));

    Emit_Prolog(statements, stackSize);

    for(const auto& statement : statements)
    {
        bool found = false;
        auto begin = m_matchers.lower_bound(statement.op);
        auto end = m_matchers.upper_bound(statement.op);

        for(auto matchIterator(begin); matchIterator != end; matchIterator++)
        {
            const MATCHER& matcher(matchIterator->second);
            if(!SymbolMatches(matcher.dstType, statement.dst)) continue;
            if(!SymbolMatches(matcher.src1Type, statement.src1)) continue;
            if(!SymbolMatches(matcher.src2Type, statement.src2)) continue;
            if(!SymbolMatches(matcher.src3Type, statement.src3)) continue;
            matcher.emitter(statement);
            found = true;
            break;
        }
        assert(found);
        if(!found)
        {
            throw std::runtime_error("No suitable emitter found for statement.");
        }
    }

    Emit_Epilog(stackSize);
    m_assembler.Ret();

    m_assembler.ResolveLabelReferences();
    m_assembler.ClearLabels();
    m_assembler.ResolveLiteralReferences();
    m_labels.clear();
}

uint32 CCodeGen_RV64::GetMaxParamSpillSize(const StatementList& statements)
{
    uint32 maxParamSpillSize = 0;
    uint32 currParamSpillSize = 0;
    for(const auto& statement : statements)
    {
        switch(statement.op)
        {
        case OP_PARAM:
        case OP_PARAM_RET:
            {
                CSymbol* src1 = statement.src1->GetSymbol().get();
                switch(src1->m_type)
                {
                case SYM_REGISTER128:
                    assert(false);
                    currParamSpillSize += 16;
                    break;
                default:
                    break;
                }
            }
            break;
        case OP_CALL:
            maxParamSpillSize = std::max<uint32>(currParamSpillSize, maxParamSpillSize);
            currParamSpillSize = 0;
            break;
        default:
            break;
        }
    }
    return maxParamSpillSize;
}

CRV64Assembler::REGISTER32 CCodeGen_RV64::GetNextTempRegister()
{
    auto result = g_tempRegisters[m_nextTempRegister];
    m_nextTempRegister++;
    //assert((m_nextTempRegister == (m_nextTempRegister % MAX_TEMP_REGS)));
    m_nextTempRegister %= MAX_TEMP_REGS;
    return result;
}

CRV64Assembler::REGISTER64 CCodeGen_RV64::GetNextTempRegister64()
{
    auto result = g_tempRegisters64[m_nextTempRegister];
    m_nextTempRegister++;
    //assert((m_nextTempRegister == (m_nextTempRegister % MAX_TEMP_REGS)));
    m_nextTempRegister %= MAX_TEMP_REGS;
    return result;
}

CRV64Assembler::REGISTERMD CCodeGen_RV64::GetNextTempRegisterMd()
{
    auto result = g_tempRegistersMd[m_nextTempRegisterMd];
    m_nextTempRegisterMd++;
    //assert((m_nextTempRegisterMd == (m_nextTempRegisterMd % MAX_TEMP_MD_REGS)));
    m_nextTempRegisterMd %= MAX_TEMP_MD_REGS;
    return result;
}

void CCodeGen_RV64::LoadMemoryInRegister(CRV64Assembler::REGISTER32 registerId, CSymbol* src)
{
    switch(src->m_type)
    {
    case SYM_RELATIVE:
        //assert(0);
        assert((src->m_valueLow & 0x03) == 0x00);
        m_assembler.Lw(registerId, g_baseRegister, src->m_valueLow);
        break;
    case SYM_TEMPORARY:
        m_assembler.Lw(registerId, CRV64Assembler::xSP, src->m_stackLocation);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::StoreRegisterInMemory(CSymbol* dst, CRV64Assembler::REGISTER32 registerId)
{
    switch(dst->m_type)
    {
    case SYM_RELATIVE:
        //assert(0);
        assert((dst->m_valueLow & 0x03) == 0x00);
        m_assembler.Str(registerId, g_baseRegister, dst->m_valueLow);
        break;
    case SYM_TEMPORARY:
        m_assembler.Str(registerId, CRV64Assembler::xSP, dst->m_stackLocation);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::LoadConstantInRegister(CRV64Assembler::REGISTER32 registerId, uint32 constant)
{
    /*if((constant & 0x0000FFFF) == constant)
    {
        m_assembler.Movz(registerId, static_cast<uint16>(constant & 0xFFFF), 0);
    }
    else if((constant & 0xFFFF0000) == constant)
    {
        m_assembler.Movz(registerId, static_cast<uint16>(constant >> 16), 1);
    }
    else if((~constant & 0x0000FFFF) == ~constant)
    {
        m_assembler.Movn(registerId, static_cast<uint16>(~constant & 0xFFFF), 0);
    }
    else if((~constant & 0xFFFF0000) == ~constant)
    {
        m_assembler.Movn(registerId, static_cast<uint16>(~constant >> 16), 1);
    }
    else
    {
        m_assembler.Movz(registerId, static_cast<uint16>(constant & 0xFFFF), 0);
        m_assembler.Movk(registerId, static_cast<uint16>(constant >> 16), 1);
    }*/
    m_assembler.Li(registerId, constant);
}

void CCodeGen_RV64::LoadMemoryReferenceInRegister(CRV64Assembler::REGISTER64 registerId, CSymbol* src)
{
    auto tmpReg = GetNextTempRegister64();
    switch(src->m_type)
    {
    case SYM_REL_REFERENCE:
        //assert(0);
        assert((src->m_valueLow & 0x07) == 0x00);
        /*m_assembler.Li(tmpReg, src->m_valueLow);
        m_assembler.Ldr(registerId, g_baseRegister, tmpReg, false);*/
        //m_assembler.Ldr(registerId, g_baseRegister, src->m_valueLow);
        //m_assembler.Ld(registerId, g_baseRegister, src->m_valueLow);
        if ((src->m_valueLow >= -2048) && (src->m_valueLow <= 2047)) {
            m_assembler.Ld(registerId, g_baseRegister, src->m_valueLow);
        } else {
            m_assembler.Li(tmpReg, src->m_valueLow);
            m_assembler.Add(tmpReg, tmpReg, g_baseRegister);
            m_assembler.Ld(registerId, tmpReg, 0);
        }
        break;
    case SYM_TMP_REFERENCE:
        m_assembler.Ld(registerId, CRV64Assembler::xSP, src->m_stackLocation);
        break;
    default:
        assert(false);
        break;
    }
}

void CCodeGen_RV64::StoreRegisterInTemporaryReference(CSymbol* dst, CRV64Assembler::REGISTER64 registerId)
{
    assert(dst->m_type == SYM_TMP_REFERENCE);
    m_assembler.Str(registerId, CRV64Assembler::xSP, dst->m_stackLocation);
}

CRV64Assembler::REGISTER32 CCodeGen_RV64::PrepareSymbolRegisterDef(CSymbol* symbol, CRV64Assembler::REGISTER32 preferedRegister)
{
    switch(symbol->m_type)
    {
    case SYM_REGISTER:
        assert(symbol->m_valueLow < MAX_REGISTERS);
        return g_registers[symbol->m_valueLow];
        break;
    case SYM_TEMPORARY:
    case SYM_RELATIVE:
        return preferedRegister;
        break;
    default:
        throw std::runtime_error("Invalid symbol type.");
        break;
    }
}

CRV64Assembler::REGISTER32 CCodeGen_RV64::PrepareSymbolRegisterUse(CSymbol* symbol, CRV64Assembler::REGISTER32 preferedRegister)
{
    switch(symbol->m_type)
    {
    case SYM_REGISTER:
        assert(symbol->m_valueLow < MAX_REGISTERS);
        return g_registers[symbol->m_valueLow];
        break;
    case SYM_TEMPORARY:
    case SYM_RELATIVE:
        LoadMemoryInRegister(preferedRegister, symbol);
        return preferedRegister;
        break;
    case SYM_CONSTANT:
        LoadConstantInRegister(preferedRegister, symbol->m_valueLow);
        return preferedRegister;
        break;
    default:
        throw std::runtime_error("Invalid symbol type.");
        break;
    }
}

void CCodeGen_RV64::CommitSymbolRegister(CSymbol* symbol, CRV64Assembler::REGISTER32 usedRegister)
{
    switch(symbol->m_type)
    {
    case SYM_REGISTER:
        assert(usedRegister == g_registers[symbol->m_valueLow]);
        break;
    case SYM_TEMPORARY:
    case SYM_RELATIVE:
        StoreRegisterInMemory(symbol, usedRegister);
        break;
    default:
        throw std::runtime_error("Invalid symbol type.");
        break;
    }
}

CRV64Assembler::REGISTER64 CCodeGen_RV64::PrepareSymbolRegisterDefRef(CSymbol* symbol, CRV64Assembler::REGISTER64 preferedRegister)
{
    switch(symbol->m_type)
    {
        case SYM_REG_REFERENCE:
            assert(symbol->m_valueLow < MAX_REGISTERS);
            return static_cast<CRV64Assembler::REGISTER64>(g_registers[symbol->m_valueLow]);
            break;
        case SYM_TMP_REFERENCE:
        case SYM_REL_REFERENCE:
            return preferedRegister;
            break;
        default:
            throw std::runtime_error("Invalid symbol type.");
            break;
    }
}

CRV64Assembler::REGISTER64 CCodeGen_RV64::PrepareSymbolRegisterUseRef(CSymbol* symbol, CRV64Assembler::REGISTER64 preferedRegister)
{
    switch(symbol->m_type)
    {
        case SYM_REG_REFERENCE:
            assert(symbol->m_valueLow < MAX_REGISTERS);
            return static_cast<CRV64Assembler::REGISTER64>(g_registers[symbol->m_valueLow]);
            break;
        case SYM_TMP_REFERENCE:
        case SYM_REL_REFERENCE:
            LoadMemoryReferenceInRegister(preferedRegister, symbol);
            return preferedRegister;
            break;
        default:
            throw std::runtime_error("Invalid symbol type.");
            break;
    }
}

void CCodeGen_RV64::CommitSymbolRegisterRef(CSymbol* symbol, CRV64Assembler::REGISTER64 usedRegister)
{
    switch(symbol->m_type)
    {
        case SYM_REG_REFERENCE:
            assert(usedRegister == g_registers[symbol->m_valueLow]);
            break;
        case SYM_TMP_REFERENCE:
            StoreRegisterInTemporaryReference(symbol, usedRegister);
            break;
        default:
            throw std::runtime_error("Invalid symbol type.");
            break;
    }
}

CRV64Assembler::REGISTER32 CCodeGen_RV64::PrepareParam(PARAM_STATE& paramState)
{
    assert(!paramState.prepared);
    paramState.prepared = true;
    if(paramState.index < MAX_PARAM_REGS)
    {
        return g_paramRegisters[paramState.index];
    }
    else
    {
        assert(false);
        return g_paramRegisters[0];
    }
}

CRV64Assembler::REGISTER64 CCodeGen_RV64::PrepareParam64(PARAM_STATE& paramState)
{
    assert(!paramState.prepared);
    paramState.prepared = true;
    if(paramState.index < MAX_PARAM_REGS)
    {
        return g_paramRegisters64[paramState.index];
    }
    else
    {
        assert(false);
        return g_paramRegisters64[0];
    }
}

void CCodeGen_RV64::CommitParam(PARAM_STATE& paramState)
{
    assert(paramState.prepared);
    paramState.prepared = false;
    if(paramState.index < MAX_PARAM_REGS)
    {
        //Nothing to do
    }
    else
    {
        assert(false);
    }
    paramState.index++;
}

void CCodeGen_RV64::CommitParam64(PARAM_STATE& paramState)
{
    assert(paramState.prepared);
    paramState.prepared = false;
    if(paramState.index < MAX_PARAM_REGS)
    {
        //Nothing to do
    }
    else
    {
        assert(false);
    }
    paramState.index++;
}

bool CCodeGen_RV64::TryGetAddSubImmParams(uint32 imm, ADDSUB_IMM_PARAMS& params)
{
    int32 signedImm = static_cast<int32>(imm);
    int16 signExtend12 = ((signedImm & 0xFFF) | ((signedImm & 0x800) ? 0xF000 : 0));
    if (signExtend12 == signedImm) {
        params.imm = signExtend12;
        return true;
    }
    return false;
}

bool CCodeGen_RV64::TryGetAddSub64ImmParams(uint64 imm, ADDSUB_IMM_PARAMS& params)
{
    int64 signedImm = static_cast<int64>(imm);
    int16 signExtend12 = ((signedImm & 0xFFF) | ((signedImm & 0x800) ? 0xF000 : 0));
    if (signExtend12 == signedImm) {
        params.imm = signExtend12;
        return true;
    }
    return false;
}

bool CCodeGen_RV64::TryGetLogicalImmParams(uint32 imm, LOGICAL_IMM_PARAMS& params)
{
    if ((imm & 0x1f) == imm) {
        params.imm = static_cast<uint16>(imm);
        return true;
    }
    return false;
}

uint32 CCodeGen_RV64::GetSavedRegisterList(uint32 registerUsage)
{
    uint32 registerSave = 0;
    for(unsigned int i = 0; i < MAX_REGISTERS; i++)
    {
        if((1 << i) & registerUsage)
        {
            registerSave |= (1 << (g_registers[i]));
        }
    }
    registerSave |= (1 << (g_baseRegister));
    return registerSave;
}

#define SIGN_EXTEND_12_INT16(v) ((v & 0xFFF) | ((v & 0x800) ? 0xF000 : 0))

#define NEW_CALL 0

void CCodeGen_RV64::Emit_Prolog(const StatementList& statements, uint32 stackSize)
{
    int stackOffset = -16;
#if NEW_CALL
    for(uint32 i = 0; i < 32; i++) {
        if(m_registerSave & (1 << i)) {
            stackOffset -= 8;
        }
    }
    m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xSP, stackOffset);
    m_assembler.Sd(CRV64Assembler::xSP, CRV64Assembler::xRA, (-stackOffset)-8);
    m_assembler.Sd(CRV64Assembler::xSP, CRV64Assembler::xFP, (-stackOffset)-16);
    m_assembler.Addi(CRV64Assembler::xFP, CRV64Assembler::xSP, -stackOffset);
#endif
    
    uint32 maxParamSpillSize = GetMaxParamSpillSize(statements);
    stackOffset = -16;
#if NEW_CALL
#else
    m_assembler.Sd(CRV64Assembler::xSP, CRV64Assembler::xFP, -16);
    m_assembler.Sd(CRV64Assembler::xSP, CRV64Assembler::xRA, -8);
#endif
    //m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xSP, -16);
    //m_assembler.Sd(CRV64Assembler::xSP, CRV64Assembler::xFP, 0);
    //m_assembler.Sd(CRV64Assembler::xSP, CRV64Assembler::xRA, 8);
    //m_assembler.Stp_PreIdx(CRV64Assembler::xFP, CRV64Assembler::xRA, CRV64Assembler::xSP, -16);

    //Preserve saved registers
    for(uint32 i = 0; i < 32; i++)
    {
        if(m_registerSave & (1 << i))
        {
            auto reg = static_cast<CRV64Assembler::REGISTER64>(i);
            stackOffset -= 8;
#if NEW_CALL
            m_assembler.Sd(CRV64Assembler::xSP, reg, stackOffset);
            //m_assembler.Sd(CRV64Assembler::xFP, reg, (-stackOffset)-24);
#else
            m_assembler.Sd(CRV64Assembler::xSP, reg, stackOffset);
#endif
            //m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xSP, -8);
            //m_assembler.Sd(CRV64Assembler::xSP, reg, 0);
            //auto reg0 = static_cast<CRV64Assembler::REGISTER64>((i * 2) + 0);
            //auto reg1 = static_cast<CRV64Assembler::REGISTER64>((i * 2) + 1);
            //m_assembler.Stp_PreIdx(reg0, reg1, CRV64Assembler::xSP, -16);
        }
    }
#if NEW_CALL
    m_assembler.Addi(CRV64Assembler::xFP, CRV64Assembler::xSP, 0);
#else
    m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xSP, stackOffset);

    //m_assembler.Mov_Sp(CRV64Assembler::xFP, CRV64Assembler::xSP);
    m_assembler.Addi(CRV64Assembler::xFP, CRV64Assembler::xSP, 0);
#endif
    uint32 totalStackAlloc = stackSize + maxParamSpillSize;
    int64 signedReverseTotalStackAlloc = -static_cast<int64>(totalStackAlloc);
    int16 signedReverse12TotalStackAlloc = SIGN_EXTEND_12_INT16(signedReverseTotalStackAlloc);
    assert((signedReverse12TotalStackAlloc == signedReverseTotalStackAlloc));
    m_paramSpillBase = stackSize;
    if(totalStackAlloc != 0)
    {
        //assert(0);
        m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xSP, signedReverse12TotalStackAlloc);
    }
    m_assembler.Mov(g_baseRegister, CRV64Assembler::x10);
}

void CCodeGen_RV64::Emit_Epilog(uint32 stackSize2)
{
    int stackSize = -16;
#if NEW_CALL
    for(uint32 i = 0; i < 32; i++) {
        if(m_registerSave & (1 << i)) {
            stackSize -= 8;
        }
    }
    //m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xFP, stackOffset);
    //m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xFP, stackOffset);
    //m_assembler.Beq(CRV64Assembler::xSP, CRV64Assembler::xFP, 4*2);
    //m_assembler.Break();
    m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xFP, 0);
#else
    //m_assembler.Mov_Sp(CRV64Assembler::xSP, CRV64Assembler::xFP);
    m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xFP, 0);
#endif
    int stackOffset = 0;
    //Restore saved registers
    for(int32 i = 31; i >= 0; i--)
    {
        if(m_registerSave & (1 << i))
        {
            auto reg = static_cast<CRV64Assembler::REGISTER64>(i);
#if NEW_CALL
            m_assembler.Ld(reg, CRV64Assembler::xSP, stackOffset);
            //m_assembler.Ld(reg, CRV64Assembler::xFP, (-stackOffset)-stackSize-24);
#else
            m_assembler.Ld(reg, CRV64Assembler::xSP, stackOffset);
#endif
            stackOffset += 8;
            //m_assembler.Ld(reg, CRV64Assembler::xSP, 0);
            //m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xSP, 8);
            //auto reg0 = static_cast<CRV64Assembler::REGISTER64>((i * 2) + 0);
            //auto reg1 = static_cast<CRV64Assembler::REGISTER64>((i * 2) + 1);
            //m_assembler.Ldp_PostIdx(reg0, reg1, CRV64Assembler::xSP, 16);
        }
    }
#if NEW_CALL
    m_assembler.Ld(CRV64Assembler::xRA, CRV64Assembler::xSP, stackOffset+8);
    m_assembler.Ld(CRV64Assembler::xFP, CRV64Assembler::xSP, stackOffset);
    //m_assembler.Ld(CRV64Assembler::xRA, CRV64Assembler::xSP, (-stackSize)-8);
    //m_assembler.Ld(CRV64Assembler::xFP, CRV64Assembler::xSP, (-stackSize)-16);
    m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xSP, stackOffset+16);
    m_assembler.Break();
#else
    m_assembler.Ld(CRV64Assembler::xFP, CRV64Assembler::xSP, stackOffset);
    m_assembler.Ld(CRV64Assembler::xRA, CRV64Assembler::xSP, stackOffset+8);
    m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xSP, stackOffset+16);
#endif
    //m_assembler.Ld(CRV64Assembler::xFP, CRV64Assembler::xSP, 0);
    //m_assembler.Ld(CRV64Assembler::xRA, CRV64Assembler::xSP, 8);
    //m_assembler.Addi(CRV64Assembler::xSP, CRV64Assembler::xSP, 16);
    //m_assembler.Ldp_PostIdx(CRV64Assembler::xFP, CRV64Assembler::xRA, CRV64Assembler::xSP, 16);
}

CRV64Assembler::LABEL CCodeGen_RV64::GetLabel(uint32 blockId)
{
    CRV64Assembler::LABEL result;
    auto labelIterator(m_labels.find(blockId));
    if(labelIterator == m_labels.end())
    {
        result = m_assembler.CreateLabel();
        m_labels[blockId] = result;
    }
    else
    {
        result = labelIterator->second;
    }
    return result;
}

void CCodeGen_RV64::MarkLabel(const STATEMENT& statement)
{
    auto label = GetLabel(statement.jmpBlock);
    m_assembler.MarkLabel(label);
}

void CCodeGen_RV64::Emit_Nop(const STATEMENT&)
{
    m_assembler.Addi(CRV64Assembler::xZR, CRV64Assembler::xZR, 0);
}

void CCodeGen_RV64::Emit_Break(const STATEMENT& statement)
{
    m_assembler.Break();
}

void CCodeGen_RV64::Emit_Mov_RegReg(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    m_assembler.Mov(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow]);
}

void CCodeGen_RV64::Emit_Mov_RegMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    LoadMemoryInRegister(g_registers[dst->m_valueLow], src1);
}

void CCodeGen_RV64::Emit_Mov_RegCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    assert(dst->m_type  == SYM_REGISTER);
    assert(src1->m_type == SYM_CONSTANT);

    LoadConstantInRegister(g_registers[dst->m_valueLow], src1->m_valueLow);
}

void CCodeGen_RV64::Emit_Mov_MemReg(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    assert(src1->m_type == SYM_REGISTER);

    StoreRegisterInMemory(dst, g_registers[src1->m_valueLow]);
}

void CCodeGen_RV64::Emit_Mov_MemMem(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto tmpReg = GetNextTempRegister();
    LoadMemoryInRegister(tmpReg, src1);
    StoreRegisterInMemory(dst, tmpReg);
}

void CCodeGen_RV64::Emit_Mov_MemCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    assert(src1->m_type == SYM_CONSTANT);

    auto tmpReg = GetNextTempRegister();
    LoadConstantInRegister(tmpReg, src1->m_valueLow);
    StoreRegisterInMemory(dst, tmpReg);
}

void CCodeGen_RV64::Emit_Mov_RegRefMemRef(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    assert(dst->m_type == SYM_REG_REFERENCE);

    LoadMemoryReferenceInRegister(static_cast<CRV64Assembler::REGISTER64>(g_registers[dst->m_valueLow]), src1);
}

void CCodeGen_RV64::Emit_Mov_MemRefRegRef(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    assert(src1->m_type == SYM_REG_REFERENCE);

    StoreRegisterInTemporaryReference(dst, static_cast<CRV64Assembler::REGISTER64>(g_registers[src1->m_valueLow]));
}

void CCodeGen_RV64::Emit_Not_VarVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    //m_assembler.Mvn(dstReg, src1Reg);
    m_assembler.Xoriw(dstReg, src1Reg, -1);
    CommitSymbolRegister(dst, dstReg);
}

#if 0
void CCodeGen_RV64::Emit_CLZ(CRV64Assembler::REGISTER64 r, CRV64Assembler::REGISTER64 n, CRV64Assembler::REGISTER64 shift, CRV64Assembler::REGISTER64 tmp, CRV64Assembler::REGISTER64 sixtyfour) {
    CRV64Assembler::REGISTER32 tmp32 = static_cast<CRV64Assembler::REGISTER32>(tmp);
    auto l1 = m_assembler.CreateLabel();
    auto l2 = m_assembler.CreateLabel();
    auto l3 = m_assembler.CreateLabel();
    auto l4 = m_assembler.CreateLabel();
    auto l5 = m_assembler.CreateLabel();
    auto l6 = m_assembler.CreateLabel();
    auto l7 = m_assembler.CreateLabel();
    m_assembler.Srli (tmp,n,32);
    m_assembler.Li   (shift,48);

    //m_assembler.Bnez (tmp,1f);
    m_assembler.BCc(CRV64Assembler::CONDITION_NE, l1, tmp32, CRV64Assembler::wZR);
    m_assembler.Addi (shift,shift,-32);

    m_assembler.MarkLabel(l1);
    m_assembler.Srl  (tmp,n,shift);
    m_assembler.Addi (shift,shift,8);

    //m_assembler.Bnez (tmp,1f);
    m_assembler.BCc(CRV64Assembler::CONDITION_NE, l2, tmp32, CRV64Assembler::wZR);
    m_assembler.Addi (shift,shift,-16);

    m_assembler.MarkLabel(l2);
    m_assembler.Srl  (tmp,n,shift);
    m_assembler.Addi (shift,shift,4);

    //m_assembler.Bnez (tmp,1f);
    m_assembler.BCc(CRV64Assembler::CONDITION_NE, l3, tmp32, CRV64Assembler::wZR);
    m_assembler.Addi (shift,shift,-8);

    m_assembler.MarkLabel(l3);
    m_assembler.Srl  (tmp,n,shift);
    m_assembler.Addi (shift,shift,2);

    //m_assembler.Bnez (tmp,1f);
    m_assembler.BCc(CRV64Assembler::CONDITION_NE, l4, tmp32, CRV64Assembler::wZR);
    m_assembler.Addi (shift,shift,-4);

    m_assembler.MarkLabel(l4);
    m_assembler.Srl  (tmp,n,shift);
    m_assembler.Addi (shift,shift,1);

    //m_assembler.Bnez (tmp,1f);
    m_assembler.BCc(CRV64Assembler::CONDITION_NE, l5, tmp32, CRV64Assembler::wZR);
    m_assembler.Addi (shift,shift,-2);

    m_assembler.MarkLabel(l5);
    m_assembler.Srl  (tmp,n,shift);
    m_assembler.Li   (sixtyfour,64);

    //m_assembler.Bnez (tmp,1f);
    m_assembler.BCc(CRV64Assembler::CONDITION_NE, l6, tmp32, CRV64Assembler::wZR);
    m_assembler.Addi (shift,shift,-1);

    m_assembler.MarkLabel(l6);
    //m_assembler.Beqz (n,1f);
    m_assembler.BCc(CRV64Assembler::CONDITION_EQ, l7, static_cast<CRV64Assembler::REGISTER32>(n), CRV64Assembler::wZR);
    m_assembler.Addi (shift,shift,1);

    m_assembler.MarkLabel(l7);
    m_assembler.Sub  (r,sixtyfour,shift);
    //m_assembler.Ret  ();
}
#endif

void CCodeGen_RV64::Emit_Lzc_VarVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstRegister = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Register = PrepareSymbolRegisterUse(src1, GetNextTempRegister());

    auto set32Label = m_assembler.CreateLabel();
    auto startCountLabel = m_assembler.CreateLabel();
    auto doneLabel = m_assembler.CreateLabel();

    // s.ext.w
    m_assembler.Mov(dstRegister, src1Register);
    m_assembler.BCc(CRV64Assembler::CONDITION_EQ, set32Label, dstRegister, CRV64Assembler::zero);
    m_assembler.BCc(CRV64Assembler::CONDITION_GE, startCountLabel, dstRegister, CRV64Assembler::zero);

    //reverse:
    m_assembler.Mvn(dstRegister, dstRegister);
    m_assembler.BCc(CRV64Assembler::CONDITION_EQ, set32Label, dstRegister, CRV64Assembler::zero);

    //startCount:
    m_assembler.MarkLabel(startCountLabel);

    if (m_thead_extentions) {
        m_assembler.Th_ff1(dstRegister, dstRegister);
        m_assembler.Addiw(dstRegister, dstRegister, -32);
    } else {
        m_assembler.Clz(dstRegister, dstRegister);
    }

    m_assembler.Addiw(dstRegister, dstRegister, -1);
    m_assembler.BCc(CRV64Assembler::CONDITION_AL, doneLabel, dstRegister, dstRegister);

    //set32:
    m_assembler.MarkLabel(set32Label);
    LoadConstantInRegister(dstRegister, 0x1F);

    //done
    m_assembler.MarkLabel(doneLabel);

    CommitSymbolRegister(dst, dstRegister);
}

void CCodeGen_RV64::Emit_RelToRef_VarCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    assert(src1->m_type == SYM_CONSTANT);

    auto dstReg = PrepareSymbolRegisterDefRef(dst, GetNextTempRegister64());

    ADDSUB_IMM_PARAMS addSubImmParams;
    if(TryGetAddSubImmParams(src1->m_valueLow, addSubImmParams))
    {
        m_assembler.Addi(dstReg, g_baseRegister, addSubImmParams.imm);
    }
    else
    {
        assert(false);
    }

    CommitSymbolRegisterRef(dst, dstReg);
}

void CCodeGen_RV64::Emit_AddRef_VarVarAny(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDefRef(dst, GetNextTempRegister64());
    auto src1Reg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

    m_assembler.Add(dstReg, src1Reg, static_cast<CRV64Assembler::REGISTER64>(src2Reg));

    CommitSymbolRegisterRef(dst, dstReg);
}

void CCodeGen_RV64::Emit_IsRefNull_VarVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());

    //m_assembler.XorSltu(static_cast<CRV64Assembler::REGISTER64>(dstReg), addressReg, CRV64Assembler::xZR);
    //m_assembler.Sltiu(static_cast<CRV64Assembler::REGISTER64>(dstReg), static_cast<CRV64Assembler::REGISTER64>(dstReg), 1);
    m_assembler.Sltiu(static_cast<CRV64Assembler::REGISTER64>(dstReg), addressReg, 1);


    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_LoadFromRef_VarVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());

    m_assembler.Lw(dstReg, addressReg, 0);

    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_LoadFromRef_VarVarAny(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert((scale == 1) || (scale == 4));

    //assert((scale == 1));

    auto valueReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());

    if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex >= -2048) && (scaledIndex <= 2047))
    {
        m_assembler.Lw(valueReg, addressReg, scaledIndex);
    }
    else
    {
        auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        //m_assembler.Ldr(valueReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), (scale == 4));
        auto tmpReg = GetNextTempRegister64();
        //auto indexReg = PrepareSymbolRegisterUseRef(src2, GetNextTempRegister64());
        if (scale == 4) {
            m_assembler.Slli(tmpReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), 2);
            m_assembler.Add(tmpReg, tmpReg, addressReg);
        } else {
            m_assembler.Add(tmpReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg));
        }
        m_assembler.Lw(valueReg, tmpReg, 0);
    }

    CommitSymbolRegister(dst, valueReg);
}

void CCodeGen_RV64::Emit_LoadFromRef_Ref_VarVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDefRef(dst, GetNextTempRegister64());
    auto src1Reg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());

    m_assembler.Ld(dstReg, src1Reg, 0);

    CommitSymbolRegisterRef(dst, dstReg);
}

void CCodeGen_RV64::Emit_LoadFromRef_Ref_VarVarAny(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert(scale == 8);

    auto valueReg = PrepareSymbolRegisterDefRef(dst, GetNextTempRegister64());
    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());

    if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex >= -2048) && (scaledIndex <= 2047))
    {
        m_assembler.Ld(valueReg, addressReg, scaledIndex);
    }
    else
    {
        auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        //m_assembler.Ldr(valueReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), (scale == 8));
        auto tmpReg = GetNextTempRegister64();
        //auto indexReg = PrepareSymbolRegisterUseRef(src2, GetNextTempRegister64());
        if (scale == 8) {
            //assert(0);
            m_assembler.Slli(tmpReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), 3);
            m_assembler.Add(tmpReg, tmpReg, addressReg);
        } else {
            m_assembler.Add(tmpReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg));
        }
        m_assembler.Ld(valueReg, tmpReg, 0);
    }

    CommitSymbolRegisterRef(dst, valueReg);
}

void CCodeGen_RV64::Emit_Load8FromRef_MemVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());

    //m_assembler.Ldrb(dstReg, addressReg, 0);
    m_assembler.Lbu(static_cast<CRV64Assembler::REGISTER64>(dstReg), addressReg, 0);

    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_Load8FromRef_MemVarAny(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert(scale == 1);

    auto valueReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());

    if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex >= -2048) && (scaledIndex <= 2047))
    {
        //m_assembler.Ldrb(valueReg, addressReg, scaledIndex);
        m_assembler.Lbu(static_cast<CRV64Assembler::REGISTER64>(valueReg), addressReg, scaledIndex);
    }
    else
    {
        auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        //m_assembler.Ldrb(valueReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), (scale == 1));
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Add(tmpReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg));
        m_assembler.Lbu(static_cast<CRV64Assembler::REGISTER64>(valueReg), tmpReg, 0);
    }

    CommitSymbolRegister(dst, valueReg);
}

void CCodeGen_RV64::Emit_Load16FromRef_MemVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());

    //m_assembler.Ldrh(dstReg, addressReg, 0);
    m_assembler.Lhu(static_cast<CRV64Assembler::REGISTER64>(dstReg), addressReg, 0);

    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_Load16FromRef_MemVarAny(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert(scale == 1);

    auto valueReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());

    if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex >= -2048) && (scaledIndex <= 2047))
    {
        //m_assembler.Ldrh(valueReg, addressReg, scaledIndex);
        m_assembler.Lhu(static_cast<CRV64Assembler::REGISTER64>(valueReg), addressReg, scaledIndex);
    }
    else
    {
        auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        //m_assembler.Ldrh(valueReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), false);
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Add(tmpReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg));
        m_assembler.Lhu(static_cast<CRV64Assembler::REGISTER64>(valueReg), tmpReg, 0);
    }

    CommitSymbolRegister(dst, valueReg);
}

void CCodeGen_RV64::Emit_StoreAtRef_VarAny(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto valueReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

    //m_assembler.Str(valueReg, addressReg, 0);
    m_assembler.Sw(addressReg, static_cast<CRV64Assembler::REGISTER64>(valueReg), 0);
}

void CCodeGen_RV64::Emit_StoreAtRef_VarAnyAny(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    auto src3 = statement.src3->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert((scale == 1) || (scale == 4));

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto valueReg = PrepareSymbolRegisterUse(src3, GetNextTempRegister());

    if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex >= -2048) && (scaledIndex <= 2047))
    {
        //m_assembler.Str(valueReg, addressReg, scaledIndex);
        m_assembler.Sw(addressReg, static_cast<CRV64Assembler::REGISTER64>(valueReg), scaledIndex);
    }
    else
    {
        auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        //m_assembler.Str(valueReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), (scale == 4));
        auto tmpReg = GetNextTempRegister64();
        if (scale == 4) {
            m_assembler.Slli(tmpReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), 2);
            m_assembler.Add(tmpReg, tmpReg, addressReg);
        } else {
            m_assembler.Add(tmpReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg));
        }
        m_assembler.Sw(tmpReg, static_cast<CRV64Assembler::REGISTER64>(valueReg), 0);
    }
}

void CCodeGen_RV64::Emit_Store8AtRef_VarAny(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto valueReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

    //m_assembler.Strb(valueReg, addressReg, 0);
    m_assembler.Sb(addressReg, static_cast<CRV64Assembler::REGISTER64>(valueReg), 0);
}

void CCodeGen_RV64::Emit_Store8AtRef_VarAnyAny(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    auto src3 = statement.src3->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert(scale == 1);

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto valueReg = PrepareSymbolRegisterUse(src3, GetNextTempRegister());

    if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex >= -2048) && (scaledIndex <= 2047))
    {
        //m_assembler.Strb(valueReg, addressReg, scaledIndex);
        m_assembler.Sb(addressReg, static_cast<CRV64Assembler::REGISTER64>(valueReg), scaledIndex);
    }
    else
    {
        auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        //m_assembler.Strb(valueReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), (scale == 1));
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Add(tmpReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg));
        m_assembler.Sb(tmpReg, static_cast<CRV64Assembler::REGISTER64>(valueReg), 0);
    }
}

void CCodeGen_RV64::Emit_Store16AtRef_VarAny(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto valueReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

    //m_assembler.Strh(valueReg, addressReg, 0);
    m_assembler.Sh(addressReg, static_cast<CRV64Assembler::REGISTER64>(valueReg), 0);
}

void CCodeGen_RV64::Emit_Store16AtRef_VarAnyAny(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();
    auto src3 = statement.src3->GetSymbol().get();
    uint8 scale = static_cast<uint8>(statement.jmpCondition);

    assert(scale == 1);

    auto addressReg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());
    auto valueReg = PrepareSymbolRegisterUse(src3, GetNextTempRegister());

    if(uint32 scaledIndex = (src2->m_valueLow * scale); src2->IsConstant() && (scaledIndex >= -2048) && (scaledIndex <= 2047))
    {
        //m_assembler.Strh(valueReg, addressReg, scaledIndex);
        m_assembler.Sh(addressReg, static_cast<CRV64Assembler::REGISTER64>(valueReg), scaledIndex);
    }
    else
    {
        auto indexReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        //m_assembler.Strh(valueReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg), (scale == 2));
        auto tmpReg = GetNextTempRegister64();
        m_assembler.Add(tmpReg, addressReg, static_cast<CRV64Assembler::REGISTER64>(indexReg));
        m_assembler.Sh(tmpReg, static_cast<CRV64Assembler::REGISTER64>(valueReg), 0);
    }
}

void CCodeGen_RV64::Emit_Param_Ctx(const STATEMENT& statement)
{
    FRAMEWORK_MAYBE_UNUSED auto src1 = statement.src1->GetSymbol().get();

    assert(src1->m_type == SYM_CONTEXT);

    m_params.push_back(
        [this] (PARAM_STATE& paramState)
        {
            auto paramReg = PrepareParam64(paramState);
            //assert(0);
            m_assembler.Mov(paramReg, g_baseRegister);
            CommitParam64(paramState);
        }
    );
}

void CCodeGen_RV64::Emit_Param_Reg(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();

    assert(src1->m_type == SYM_REGISTER);

    m_params.push_back(
        [this, src1] (PARAM_STATE& paramState)
        {
            auto paramReg = PrepareParam(paramState);
            m_assembler.Mov(paramReg, g_registers[src1->m_valueLow]);
            CommitParam(paramState);
        }
    );
}

void CCodeGen_RV64::Emit_Param_Mem(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();

    m_params.push_back(
        [this, src1] (PARAM_STATE& paramState)
        {
            auto paramReg = PrepareParam(paramState);
            LoadMemoryInRegister(paramReg, src1);
            CommitParam(paramState);
        }
    );
}

void CCodeGen_RV64::Emit_Param_Cst(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();

    m_params.push_back(
        [this, src1] (PARAM_STATE& paramState)
        {
            auto paramReg = PrepareParam(paramState);
            LoadConstantInRegister(paramReg, src1->m_valueLow);
            CommitParam(paramState);
        }
    );
}

void CCodeGen_RV64::Emit_Param_Mem64(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();

    m_params.push_back(
        [this, src1] (PARAM_STATE& paramState)
        {
            auto paramReg = PrepareParam64(paramState);
            LoadMemory64InRegister(paramReg, src1);
            CommitParam64(paramState);
        }
    );
}

void CCodeGen_RV64::Emit_Param_Cst64(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();

    m_params.push_back(
        [this, src1] (PARAM_STATE& paramState)
        {
            auto paramReg = PrepareParam64(paramState);
            LoadConstant64InRegister(paramReg, src1->GetConstant64());
            CommitParam64(paramState);
        }
    );
}

void CCodeGen_RV64::Emit_Param_Reg128(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();

    m_params.push_back(
        [this, src1] (PARAM_STATE& paramState)
        {
            auto paramReg = PrepareParam64(paramState);
            uint32 paramBase = m_paramSpillBase + paramState.spillOffset;
            m_assembler.Addi(paramReg, CRV64Assembler::xSP, paramBase);
            m_assembler.Str_1q(g_registersMd[src1->m_valueLow], CRV64Assembler::xSP, paramBase);

            // need to test this
            assert(false);

            //m_assembler.WriteWord(0);

            paramState.spillOffset += 0x10;
            CommitParam64(paramState);
        }
    );
}

void CCodeGen_RV64::Emit_Param_Mem128(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();

    m_params.push_back(
        [this, src1] (PARAM_STATE& paramState)
        {
            auto paramReg = PrepareParam64(paramState);
            LoadMemory128AddressInRegister(paramReg, src1);
            CommitParam64(paramState);
        }
    );
}

void CCodeGen_RV64::Emit_Call(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(src1->m_type == SYM_CONSTANTPTR);
    assert(src2->m_type == SYM_CONSTANT);

    unsigned int paramCount = src2->m_valueLow;
    PARAM_STATE paramState;

    for(unsigned int i = 0; i < paramCount; i++)
    {
        auto emitter(m_params.back());
        m_params.pop_back();
        emitter(paramState);
    }

    if(m_generateRelocatableCalls)
    {
        if(m_externalSymbolReferencedHandler)
        {
            auto position = m_stream->GetLength();
            m_externalSymbolReferencedHandler(src1->GetConstantPtr(), position, CCodeGen::SYMBOL_REF_TYPE::ARMV8_PCRELATIVE);
        }
        m_assembler.Bl(0);
    }
    else
    {
        auto fctAddressReg = GetNextTempRegister64();
        LoadConstant64InRegister(fctAddressReg, src1->GetConstantPtr());
        m_assembler.Blr(fctAddressReg);
    }
}

void CCodeGen_RV64::Emit_RetVal_Reg(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    assert(dst->m_type == SYM_REGISTER);
    m_assembler.Mov(g_registers[dst->m_valueLow], CRV64Assembler::a0);
}

void CCodeGen_RV64::Emit_RetVal_Tmp(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    assert(dst->m_type == SYM_TEMPORARY);
    StoreRegisterInMemory(dst, CRV64Assembler::a0);
}

void CCodeGen_RV64::Emit_RetVal_Mem64(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    StoreRegisterInMemory64(dst, CRV64Assembler::x10);
}

void CCodeGen_RV64::Emit_RetVal_Reg128(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();

    /*m_assembler.Ins_1d(g_registersMd[dst->m_valueLow], 0, CRV64Assembler::x0);
    m_assembler.Ins_1d(g_registersMd[dst->m_valueLow], 1, CRV64Assembler::x1);*/
    //m_assembler.Addi(CRV64Assembler::x0, CRV64Assembler::x0, 0);
    //m_assembler.Addi(CRV64Assembler::x1, CRV64Assembler::x1, 0);
    //m_assembler.WriteWord(0);
}

void CCodeGen_RV64::Emit_RetVal_Mem128(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();

    auto dstAddrReg = GetNextTempRegister64();

    LoadMemory128AddressInRegister(dstAddrReg, dst);
    m_assembler.Str(CRV64Assembler::x10, dstAddrReg, 0);
    m_assembler.Str(CRV64Assembler::x11, dstAddrReg, 8);
    //m_assembler.WriteWord(0);
}

void CCodeGen_RV64::Emit_ExternJmp(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();

    assert(src1->m_type == SYM_CONSTANTPTR);

    m_assembler.Mov(g_paramRegisters64[0], g_baseRegister);
    Emit_Epilog(0);
    //m_assembler.Break();

    if(m_generateRelocatableCalls)
    {
        if(m_externalSymbolReferencedHandler)
        {
            auto position = m_stream->GetLength();
            m_externalSymbolReferencedHandler(src1->GetConstantPtr(), position, CCodeGen::SYMBOL_REF_TYPE::ARMV8_PCRELATIVE);
        }
        m_assembler.B_offset(0);
    }
    else
    {
        auto fctAddressReg = GetNextTempRegister64();
        LoadConstant64InRegister(fctAddressReg, src1->GetConstantPtr());
        m_assembler.Jalr(CRV64Assembler::xZR, fctAddressReg, 0);
    }
}

void CCodeGen_RV64::Emit_ExternJmpDynamic(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();

    assert(src1->m_type == SYM_CONSTANTPTR);

    m_assembler.Mov(g_paramRegisters64[0], g_baseRegister);
    Emit_Epilog(0);
    //m_assembler.Break();
    auto fctAddressReg = GetNextTempRegister64();

    //auto position = m_stream->GetLength();

    //assert(position<=0xfff);

    m_assembler.Auipc(fctAddressReg, 0);
    m_assembler.Ld(fctAddressReg, fctAddressReg, 12);
    m_assembler.Jalr(CRV64Assembler::xZR, fctAddressReg, 0);

    auto position = m_stream->GetLength();

    //Write target function address
    if(m_externalSymbolReferencedHandler)
    {
        //auto position = m_stream->GetLength();
        m_externalSymbolReferencedHandler(src1->GetConstantPtr(), position, CCodeGen::SYMBOL_REF_TYPE::NATIVE_POINTER);
    }

    m_stream->Write64(src1->GetConstantPtr());
}

void CCodeGen_RV64::Emit_Jmp(const STATEMENT& statement)
{
    m_assembler.B(GetLabel(statement.jmpBlock));
}

void CCodeGen_RV64::Emit_CondJmp(const STATEMENT& statement)
{
    auto label = GetLabel(statement.jmpBlock);

    switch(statement.jmpCondition)
    {
    case CONDITION_EQ:
        m_assembler.BCc(CRV64Assembler::CONDITION_EQ, label);
        break;
    case CONDITION_NE:
        m_assembler.BCc(CRV64Assembler::CONDITION_NE, label);
        break;
    case CONDITION_BL:
        m_assembler.BCc(CRV64Assembler::CONDITION_CC, label);
        break;
    case CONDITION_BE:
        m_assembler.BCc(CRV64Assembler::CONDITION_LS, label);
        break;
    case CONDITION_AB:
        m_assembler.BCc(CRV64Assembler::CONDITION_HI, label);
        break;
    case CONDITION_AE:
        m_assembler.BCc(CRV64Assembler::CONDITION_CS, label);
        break;
    case CONDITION_LT:
        m_assembler.BCc(CRV64Assembler::CONDITION_LT, label);
        break;
    case CONDITION_LE:
        m_assembler.BCc(CRV64Assembler::CONDITION_LE, label);
        break;
    case CONDITION_GT:
        m_assembler.BCc(CRV64Assembler::CONDITION_GT, label);
        break;
    case CONDITION_GE:
        m_assembler.BCc(CRV64Assembler::CONDITION_GE, label);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::Emit_CondJmp(const STATEMENT& statement, CRV64Assembler::REGISTER32 src1Reg, CRV64Assembler::REGISTER32 src2Reg)
{
    auto label = GetLabel(statement.jmpBlock);

    switch(statement.jmpCondition)
    {
    case CONDITION_EQ:
        m_assembler.BCc(CRV64Assembler::CONDITION_EQ, label, src1Reg, src2Reg);
        break;
    case CONDITION_NE:
        m_assembler.BCc(CRV64Assembler::CONDITION_NE, label, src1Reg, src2Reg);
        break;
    case CONDITION_BL:
        m_assembler.BCc(CRV64Assembler::CONDITION_CC, label, src1Reg, src2Reg);
        break;
    case CONDITION_BE:
        m_assembler.BCc(CRV64Assembler::CONDITION_LS, label, src1Reg, src2Reg);
        break;
    case CONDITION_AB:
        m_assembler.BCc(CRV64Assembler::CONDITION_HI, label, src1Reg, src2Reg);
        break;
    case CONDITION_AE:
        m_assembler.BCc(CRV64Assembler::CONDITION_CS, label, src1Reg, src2Reg);
        break;
    case CONDITION_LT:
        m_assembler.BCc(CRV64Assembler::CONDITION_LT, label, src1Reg, src2Reg);
        break;
    case CONDITION_LE:
        m_assembler.BCc(CRV64Assembler::CONDITION_LE, label, src1Reg, src2Reg);
        break;
    case CONDITION_GT:
        m_assembler.BCc(CRV64Assembler::CONDITION_GT, label, src1Reg, src2Reg);
        break;
    case CONDITION_GE:
        m_assembler.BCc(CRV64Assembler::CONDITION_GE, label, src1Reg, src2Reg);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::Emit_CondJmp_AnyVar(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
    Emit_CondJmp(statement, src1Reg, src2Reg);
}

void CCodeGen_RV64::Emit_CondJmp_VarCst(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    assert(src2->m_type == SYM_CONSTANT);

    if((src2->m_valueLow == 0) && ((statement.jmpCondition == CONDITION_NE) || (statement.jmpCondition == CONDITION_EQ)))
    {
        auto label = GetLabel(statement.jmpBlock);

        switch(statement.jmpCondition)
        {
        case CONDITION_EQ:
            m_assembler.Cbz(src1Reg, label);
            break;
        case CONDITION_NE:
            m_assembler.Cbnz(src1Reg, label);
            break;
        default:
            assert(false);
            break;
        }
    }
    else
    {
        /*ADDSUB_IMM_PARAMS addSubImmParams;
        if(TryGetAddSubImmParams(src2->m_valueLow, addSubImmParams))
        {
            m_assembler.Cmp(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
        }
        else if(TryGetAddSubImmParams(-static_cast<int32>(src2->m_valueLow), addSubImmParams))
        {
            m_assembler.Cmn(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
        }
        else
        {
            auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
            m_assembler.Cmp(src1Reg, src2Reg);
        }*/

        auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        Emit_CondJmp(statement, src1Reg, src2Reg);
    }
}

void CCodeGen_RV64::Emit_CondJmp_Ref_VarCst(const STATEMENT& statement)
{
    auto src1 = statement.src1->GetSymbol().get();
    FRAMEWORK_MAYBE_UNUSED auto src2 = statement.src2->GetSymbol().get();

    auto src1Reg = PrepareSymbolRegisterUseRef(src1, GetNextTempRegister64());

    assert(src2->m_type == SYM_CONSTANT);
    assert(src2->m_valueLow == 0);
    assert((statement.jmpCondition == CONDITION_NE) || (statement.jmpCondition == CONDITION_EQ));

    auto label = GetLabel(statement.jmpBlock);
    switch(statement.jmpCondition)
    {
        case CONDITION_EQ:
            m_assembler.Cbz(src1Reg, label);
            break;
        case CONDITION_NE:
            m_assembler.Cbnz(src1Reg, label);
            break;
        default:
            assert(false);
            break;
    }
}

void CCodeGen_RV64::Cmp_GetFlag(CRV64Assembler::REGISTER32 registerId, Jitter::CONDITION condition)
{
    switch(condition)
    {
    case CONDITION_EQ:
        m_assembler.Cset(registerId, CRV64Assembler::CONDITION_EQ);
        break;
    case CONDITION_NE:
        m_assembler.Cset(registerId, CRV64Assembler::CONDITION_NE);
        break;
    case CONDITION_LT:
        m_assembler.Cset(registerId, CRV64Assembler::CONDITION_LT);
        break;
    case CONDITION_LE:
        m_assembler.Cset(registerId, CRV64Assembler::CONDITION_LE);
        break;
    case CONDITION_GT:
        m_assembler.Cset(registerId, CRV64Assembler::CONDITION_GT);
        break;
    case CONDITION_BL:
        m_assembler.Cset(registerId, CRV64Assembler::CONDITION_CC);
        break;
    case CONDITION_BE:
        m_assembler.Cset(registerId, CRV64Assembler::CONDITION_LS);
        break;
    case CONDITION_AB:
        m_assembler.Cset(registerId, CRV64Assembler::CONDITION_HI);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::Cmp_GetFlag(CRV64Assembler::REGISTER32 registerId, Jitter::CONDITION condition, CRV64Assembler::REGISTER32 src1Reg, CRV64Assembler::REGISTER32 src2Reg)
{
    switch(condition)
    {
    case CONDITION_EQ:
        //m_assembler.XorSltu(registerId, src1Reg, src2Reg);
        //m_assembler.Sltiu(registerId, registerId, 1);
        m_assembler.Subw(registerId, src1Reg, src2Reg);
        m_assembler.Sltiu(registerId, registerId, 1);
        break;
    case CONDITION_NE:
        //m_assembler.XorSltu(registerId, src1Reg, src2Reg);
        m_assembler.Subw(registerId, src1Reg, src2Reg);
        m_assembler.Sltu(static_cast<CRV64Assembler::REGISTER64>(registerId), CRV64Assembler::xZR, static_cast<CRV64Assembler::REGISTER64>(registerId));
        break;
    case CONDITION_LT:
        m_assembler.Slt(registerId, src1Reg, src2Reg);
        break;
    case CONDITION_LE:
        m_assembler.Slt(registerId, src2Reg, src1Reg);
        m_assembler.Sltiu(registerId, registerId, 1);
        break;
    case CONDITION_GT:
        m_assembler.Slt(registerId, src2Reg, src1Reg);
        break;
    case CONDITION_BL:
        m_assembler.Sltu(registerId, src1Reg, src2Reg);
        break;
    case CONDITION_BE:
        m_assembler.Sltu(registerId, src2Reg, src1Reg);
        m_assembler.Sltiu(registerId, registerId, 1);
        break;
    case CONDITION_AB:
        m_assembler.Sltu(registerId, src2Reg, src1Reg);
        break;
    default:
        assert(0);
        break;
    }
}

void CCodeGen_RV64::Emit_Cmp_VarAnyVar(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
    auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
    Cmp_GetFlag(dstReg, statement.jmpCondition, src1Reg, src2Reg);
    CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_RV64::Emit_Cmp_VarVarCst(const STATEMENT& statement)
{
    auto dst = statement.dst->GetSymbol().get();
    auto src1 = statement.src1->GetSymbol().get();
    auto src2 = statement.src2->GetSymbol().get();

    assert(src2->m_type == SYM_CONSTANT);

    auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
    auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());

    ADDSUB_IMM_PARAMS addSubImmParams;
    /*if(TryGetAddSubImmParams(src2->m_valueLow, addSubImmParams))
    {
        assert(0);
        m_assembler.Cmp(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
    }
    else if(TryGetAddSubImmParams(-static_cast<int32>(src2->m_valueLow), addSubImmParams))
    {
        assert(0);
        m_assembler.Cmn(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
    }
    else
    {
        auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
        m_assembler.Cmp(src1Reg, src2Reg);
    }*/

    auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

    //assert(0);
    Cmp_GetFlag(dstReg, statement.jmpCondition, src1Reg, src2Reg);
    CommitSymbolRegister(dst, dstReg);
}
