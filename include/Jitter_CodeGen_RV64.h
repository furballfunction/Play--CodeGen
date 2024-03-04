#pragma once

#include <deque>
#include "Jitter_CodeGen.h"
#include "RV64Assembler.h"

namespace Jitter
{
    class CCodeGen_RV64 : public CCodeGen
    {
    public:
                   CCodeGen_RV64();
        virtual    ~CCodeGen_RV64() = default;

        void            CheckMachine();

        void            SetGenerateRelocatableCalls(bool);

        void            GenerateCode(const StatementList&, unsigned int) override;
        void            SetStream(Framework::CStream*) override;
        void            RegisterExternalSymbols(CObjectFile*) const override;
        unsigned int    GetAvailableRegisterCount() const override;
        unsigned int    GetAvailableMdRegisterCount() const override;
        bool            Has128BitsCallOperands() const override;
        bool            CanHold128BitsReturnValueInRegisters() const override;
        bool            SupportsExternalJumps() const override;
        uint32          GetPointerSize() const override;

    private:
        typedef std::map<uint32, CRV64Assembler::LABEL> LabelMapType;
        typedef void (CCodeGen_RV64::*ConstCodeEmitterType)(const STATEMENT&);

        struct ADDSUB_IMM_PARAMS
        {
            int16                                       imm = 0;
        };

        struct LOGICAL_IMM_PARAMS
        {
            uint16                                      imm = 0;
        };

        struct PARAM_STATE
        {
            bool prepared = false;
            unsigned int index = 0;
            uint32 spillOffset = 0;
        };

        typedef std::function<void (PARAM_STATE&)> ParamEmitterFunction;
        typedef std::deque<ParamEmitterFunction> ParamStack;

        enum
        {
            MAX_REGISTERS = 9,
        };

        enum
        {
            MAX_MDREGISTERS = 20,
        };

        enum MAX_PARAM_REGS
        {
            MAX_PARAM_REGS = 8,
        };

        enum MAX_TEMP_REGS
        {
            MAX_TEMP_REGS = 7,
        };

        enum MAX_TEMP_MD_REGS
        {
            MAX_TEMP_MD_REGS = 4,
        };

        struct CONSTMATCHER
        {
            OPERATION               op;
            MATCHTYPE               dstType;
            MATCHTYPE               src1Type;
            MATCHTYPE               src2Type;
            MATCHTYPE               src3Type;
            ConstCodeEmitterType    emitter;
        };

        static uint32    GetMaxParamSpillSize(const StatementList&);

        CRV64Assembler::REGISTER32    GetNextTempRegister();
        CRV64Assembler::REGISTER64    GetNextTempRegister64();
        CRV64Assembler::REGISTERMD    GetNextTempRegisterMd();

        uint32    GetMemory64Offset(CSymbol*) const;

        void    LoadMemoryInRegister(CRV64Assembler::REGISTER32, CSymbol*);
        void    StoreRegisterInMemory(CSymbol*, CRV64Assembler::REGISTER32);

        void    LoadMemory64InRegister(CRV64Assembler::REGISTER64, CSymbol*);
        void    StoreRegisterInMemory64(CSymbol*, CRV64Assembler::REGISTER64);

        void    LoadConstantInRegister(CRV64Assembler::REGISTER32, uint32);
        void    LoadConstant64InRegister(CRV64Assembler::REGISTER64, uint64);

        void    LoadMemory64LowInRegister(CRV64Assembler::REGISTER32, CSymbol*);
        void    LoadMemory64HighInRegister(CRV64Assembler::REGISTER32, CSymbol*);

        void    LoadSymbol64InRegister(CRV64Assembler::REGISTER64, CSymbol*);

        void    StoreRegistersInMemory64(CSymbol*, CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32);

        void    LoadMemoryReferenceInRegister(CRV64Assembler::REGISTER64, CSymbol*);
        void    StoreRegisterInTemporaryReference(CSymbol*, CRV64Assembler::REGISTER64);

        void    LoadMemoryFpSingleInRegister(CRV64Assembler::REGISTERMD, CSymbol*);
        void    StoreRegisterInMemoryFpSingle(CSymbol*, CRV64Assembler::REGISTERMD);

        void    LoadMemoryFpSingleInRegisterRVV(CRV64Assembler::REGISTERMD, CSymbol*);
        void    StoreRegisterInMemoryFpSingleRVV(CSymbol*, CRV64Assembler::REGISTERMD);
        void    LoadMemory128InRegister(CRV64Assembler::REGISTERMD, CSymbol*);
        void    StoreRegisterInMemory128(CSymbol*, CRV64Assembler::REGISTERMD);

        void    LoadMemory128AddressInRegister(CRV64Assembler::REGISTER64, CSymbol*, uint32 = 0);
        void    LoadRelative128AddressInRegister(CRV64Assembler::REGISTER64, CSymbol*, uint32);
        void    LoadTemporary128AddressInRegister(CRV64Assembler::REGISTER64, CSymbol*, uint32);

        void    LoadTemporary256ElementAddressInRegister(CRV64Assembler::REGISTER64, CSymbol*, uint32);

        CRV64Assembler::REGISTER32    PrepareSymbolRegisterDef(CSymbol*, CRV64Assembler::REGISTER32);
        CRV64Assembler::REGISTER32    PrepareSymbolRegisterUse(CSymbol*, CRV64Assembler::REGISTER32);
        void                             CommitSymbolRegister(CSymbol*, CRV64Assembler::REGISTER32);

        CRV64Assembler::REGISTER64    PrepareSymbolRegisterDefRef(CSymbol*, CRV64Assembler::REGISTER64);
        CRV64Assembler::REGISTER64    PrepareSymbolRegisterUseRef(CSymbol*, CRV64Assembler::REGISTER64);
        void                             CommitSymbolRegisterRef(CSymbol*, CRV64Assembler::REGISTER64);

        CRV64Assembler::REGISTERMD    PrepareSymbolRegisterDefMd(CSymbol*, CRV64Assembler::REGISTERMD);
        CRV64Assembler::REGISTERMD    PrepareSymbolRegisterUseMd(CSymbol*, CRV64Assembler::REGISTERMD);
        void                             CommitSymbolRegisterMd(CSymbol*, CRV64Assembler::REGISTERMD);

        CRV64Assembler::REGISTER32    PrepareParam(PARAM_STATE&);
        CRV64Assembler::REGISTER64    PrepareParam64(PARAM_STATE&);
        void                             CommitParam(PARAM_STATE&);
        void                             CommitParam64(PARAM_STATE&);

        bool TryGetAddSubImmParams(uint32, ADDSUB_IMM_PARAMS&);
        bool TryGetAddSub64ImmParams(uint64, ADDSUB_IMM_PARAMS&);
        bool TryGetLogicalImmParams(uint32, LOGICAL_IMM_PARAMS&);

        void LoadRefIndexAddress(CRV64Assembler::REGISTER64, CSymbol*, CRV64Assembler::REGISTER64, CSymbol*, CRV64Assembler::REGISTER64, uint8);

        //SHIFTOP ----------------------------------------------------------
        struct SHIFTOP_BASE
        {
            typedef void (CRV64Assembler::*OpImmType)(CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32, uint8);
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32);
        };

        struct SHIFTOP_ASR : public SHIFTOP_BASE
        {
            static OpImmType    OpImm()    { return &CRV64Assembler::Asr; }
            static OpRegType    OpReg()    { return &CRV64Assembler::Asrv; }
        };

        struct SHIFTOP_LSL : public SHIFTOP_BASE
        {
            static OpImmType    OpImm()    { return &CRV64Assembler::Lsl; }
            static OpRegType    OpReg()    { return &CRV64Assembler::Lslv; }
        };

        struct SHIFTOP_LSR : public SHIFTOP_BASE
        {
            static OpImmType    OpImm()    { return &CRV64Assembler::Lsr; }
            static OpRegType    OpReg()    { return &CRV64Assembler::Lsrv; }
        };

        //LOGICOP ----------------------------------------------------------
        struct LOGICOP_BASE
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32);
            typedef void (CRV64Assembler::*OpImmType)(CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32, int16);
        };

        struct LOGICOP_AND : public LOGICOP_BASE
        {
            static OpRegType    OpReg()    { return &CRV64Assembler::Andw; }
            static OpImmType    OpImm()    { return &CRV64Assembler::Andiw; }
        };

        struct LOGICOP_OR : public LOGICOP_BASE
        {
            static OpRegType    OpReg()    { return &CRV64Assembler::Orw; }
            static OpImmType    OpImm()    { return &CRV64Assembler::Oriw; }
        };

        struct LOGICOP_XOR : public LOGICOP_BASE
        {
            static OpRegType    OpReg()    { return &CRV64Assembler::Xorw; }
            static OpImmType    OpImm()    { return &CRV64Assembler::Xoriw; }
        };

        //ADDSUBOP ----------------------------------------------------------
        struct ADDSUBOP_BASE
        {
            typedef void (CRV64Assembler::*OpImmType)(CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32, int16 imm);
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32);
        };

        struct ADDSUBOP_ADD : public ADDSUBOP_BASE
        {
            static OpImmType    OpImm()       { return &CRV64Assembler::Addiw; }
            static OpRegType    OpReg()       { return &CRV64Assembler::Addw; }
        };

        struct ADDSUBOP_SUB : public ADDSUBOP_BASE
        {
            static OpRegType    OpReg()       { return &CRV64Assembler::Subw; }
            static OpImmType    OpImmRev()    { return &CRV64Assembler::Addiw; }
        };

        //SHIFT64OP ----------------------------------------------------------
        struct SHIFT64OP_BASE
        {
            typedef void (CRV64Assembler::*OpImmType)(CRV64Assembler::REGISTER64, CRV64Assembler::REGISTER64, uint8);
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTER64, CRV64Assembler::REGISTER64, CRV64Assembler::REGISTER64);
        };

        struct SHIFT64OP_ASR : public SHIFT64OP_BASE
        {
            static OpImmType    OpImm()    { return &CRV64Assembler::Asr; }
            static OpRegType    OpReg()    { return &CRV64Assembler::Asrv; }
        };

        struct SHIFT64OP_LSL : public SHIFT64OP_BASE
        {
            static OpImmType    OpImm()    { return &CRV64Assembler::Lsl; }
            static OpRegType    OpReg()    { return &CRV64Assembler::Lslv; }
        };

        struct SHIFT64OP_LSR : public SHIFT64OP_BASE
        {
            static OpImmType    OpImm()    { return &CRV64Assembler::Lsr; }
            static OpRegType    OpReg()    { return &CRV64Assembler::Lsrv; }
        };

        //FPUOP ----------------------------------------------------------
        struct FPUOP_BASE2
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD);
        };

        struct FPUOP_BASE3
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD);
        };

        struct FPUOP_ADD : public FPUOP_BASE3
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fadd_1s; }
        };

        struct FPUOP_SUB : public FPUOP_BASE3
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fsub_1s; }
        };

        struct FPUOP_MUL : public FPUOP_BASE3
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fmul_1s; }
        };

        struct FPUOP_DIV : public FPUOP_BASE3
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fdiv_1s; }
        };

        struct FPUOP_MIN : public FPUOP_BASE3
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fmin_1s; }
        };

        struct FPUOP_MAX : public FPUOP_BASE3
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fmax_1s; }
        };

        struct FPUOP_ABS : public FPUOP_BASE2
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fabs_1s; }
        };

        struct FPUOP_NEG : public FPUOP_BASE2
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fneg_1s; }
        };

        struct FPUOP_SQRT : public FPUOP_BASE2
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fsqrt_1s; }
        };

        //MDOP -----------------------------------------------------------
        struct MDOP_BASE2_MEMRVV
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD);
        };

        struct MDOP_BASE2
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD);
        };

        struct MDOP_BASE2_MEM
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTER64, CRV64Assembler::REGISTER64, CRV64Assembler::REGISTER32);
        };

        struct MDOP_BASE2_1S
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD);
        };

        struct MDOP_BASE2_IR1S
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTER32, CRV64Assembler::REGISTERMD);
        };

        struct MDOP_BASE2_SR1I
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTER32);
        };

        struct MDOP_BASE3_MEMRVV
        {
            CRV64Assembler::VConfig Config;
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD);
        };

        struct MDOP_BASE3
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD);
        };

        struct MDOP_BASE3_MEM
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTER64, CRV64Assembler::REGISTER64, CRV64Assembler::REGISTER64, CRV64Assembler::REGISTER32, CRV64Assembler::REGISTER32);
        };

        struct MDOP_BASE3_1S
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD);
        };

        struct MDOP_BASE3_IR1S
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTER32, CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD);
        };

        struct MDOP_SHIFT
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD, uint8);
        };

        struct MDOP_SHIFT_MEMRVV
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTERMD, CRV64Assembler::REGISTERMD, uint8);
        };

        struct MDOP_SHIFT_MEM
        {
            typedef void (CRV64Assembler::*OpRegType)(CRV64Assembler::REGISTER64, CRV64Assembler::REGISTER64, uint8, CRV64Assembler::REGISTER32);
        };

        struct MDOP_ADDB_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Add_16b; }
        };

        struct MDOP_ADDH_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Add_8h; }
        };

        struct MDOP_ADDW_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Add_4s; }
        };


        struct MDOP_ADDB : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Add_16b; }
        };

        struct MDOP_ADDH : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Add_8h; }
        };

        struct MDOP_ADDW : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Add_4s; }
        };

        struct MDOP_ADDB_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Add_16b_Mem; }
        };

        struct MDOP_ADDH_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Add_8h_Mem; }
        };

        struct MDOP_ADDW_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Add_4s_Mem; }
        };

        struct MDOP_ADDBUS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqadd_16b; }
        };

        struct MDOP_ADDHUS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqadd_8h; }
        };

        struct MDOP_ADDWUS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqadd_4s; }
        };

        struct MDOP_ADDBSS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sqadd_16b; }
        };

        struct MDOP_ADDHSS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sqadd_8h; }
        };

        struct MDOP_ADDWSS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sqadd_4s; }
        };

        struct MDOP_ADDBUS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Uqadd_16b_Mem; }
        };

        struct MDOP_ADDHUS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Uqadd_8h_Mem; }
        };

        struct MDOP_ADDWUS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Uqadd_4s_Mem; }
        };

        struct MDOP_ADDBSS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Sqadd_16b_Mem; }
        };

        struct MDOP_ADDHSS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Sqadd_8h_Mem; }
        };

        struct MDOP_ADDWSS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Sqadd_4s_Mem; }
        };

        struct MDOP_SUBB_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Sub_16b_Mem; }
        };

        struct MDOP_SUBH_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Sub_8h_Mem; }
        };

        struct MDOP_SUBW_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Sub_4s_Mem; }
        };

        struct MDOP_SUBBUS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Uqsub_16b_Mem; }
        };

        struct MDOP_SUBHUS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Uqsub_8h_Mem; }
        };

        struct MDOP_SUBWUS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Uqsub_4s_Mem; }
        };

        struct MDOP_SUBHSS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Sqsub_8h_Mem; }
        };

        struct MDOP_SUBWSS_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Sqsub_4s_Mem; }
        };

        struct MDOP_ADDBUS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqadd_16b; }
        };

        struct MDOP_ADDHUS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqadd_8h; }
        };

        struct MDOP_ADDWUS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqadd_4s; }
        };

        struct MDOP_ADDBSS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sqadd_16b; }
        };

        struct MDOP_ADDHSS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sqadd_8h; }
        };

        struct MDOP_ADDWSS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sqadd_4s; }
        };

        struct MDOP_SUBB_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sub_16b; }
        };

        struct MDOP_SUBH_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sub_8h; }
        };

        struct MDOP_SUBW_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sub_4s; }
        };

        struct MDOP_SUBBUS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqsub_16b; }
        };

        struct MDOP_SUBHUS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqsub_8h; }
        };

        struct MDOP_SUBWUS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqsub_4s; }
        };

        struct MDOP_SUBHSS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sqsub_8h; }
        };

        struct MDOP_SUBWSS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sqsub_4s; }
        };

        /*struct MDOP_SUBB_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sub_16b; }
        };

        struct MDOP_SUBH_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sub_8h; }
        };

        struct MDOP_SUBW_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sub_4s; }
        };*/

        struct MDOP_SUBB : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sub_16b; }
        };

        struct MDOP_SUBH : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sub_8h; }
        };

        struct MDOP_SUBW : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sub_4s; }
        };

        struct MDOP_SUBBUS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqsub_16b; }
        };

        struct MDOP_SUBHUS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqsub_8h; }
        };

        struct MDOP_SUBWUS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Uqsub_4s; }
        };

        struct MDOP_SUBHSS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sqsub_8h; }
        };

        struct MDOP_SUBWSS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sqsub_4s; }
        };

        struct MDOP_CMPEQB_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmeq_16b; }
        };

        struct MDOP_CMPEQH_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmeq_8h; }
        };

        struct MDOP_CMPEQW_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmeq_4s; }
        };

        struct MDOP_CMPGTB_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmgt_16b; }
        };

        struct MDOP_CMPGTH_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmgt_8h; }
        };

        struct MDOP_CMPGTW_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmgt_4s; }
        };

        struct MDOP_CMPEQB_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Cmeq_16b_Mem; }
        };

        struct MDOP_CMPEQH_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Cmeq_8h_Mem; }
        };

        struct MDOP_CMPEQW_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Cmeq_4s_Mem; }
        };

        struct MDOP_CMPGTB_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Cmgt_16b_Mem; }
        };

        struct MDOP_CMPGTH_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Cmgt_8h_Mem; }
        };

        struct MDOP_CMPGTW_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Cmgt_4s_Mem; }
        };

        struct MDOP_CMPEQB : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmeq_16b; }
        };

        struct MDOP_CMPEQH : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmeq_8h; }
        };

        struct MDOP_CMPEQW : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmeq_4s; }
        };

        struct MDOP_CMPGTB : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmgt_16b; }
        };

        struct MDOP_CMPGTH : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmgt_8h; }
        };

        struct MDOP_CMPGTW : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Cmgt_4s; }
        };

        struct MDOP_MINH : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Smin_8h; }
        };

        struct MDOP_MINW : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Smin_4s; }
        };

        struct MDOP_MINH_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Smin_8h; }
        };

        struct MDOP_MINW_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Smin_4s; }
        };

        struct MDOP_MINH_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Smin_8h_Mem; }
        };

        struct MDOP_MINW_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Smin_4s_Mem; }
        };

        struct MDOP_MAXH : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Smax_8h; }
        };

        struct MDOP_MAXW : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Smax_4s; }
        };

        struct MDOP_MAXH_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Smax_8h; }
        };

        struct MDOP_MAXW_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Smax_4s; }
        };

        struct MDOP_MAXH_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Smax_8h_Mem; }
        };

        struct MDOP_MAXW_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Smax_4s_Mem; }
        };

        struct MDOP_ADDS_1S : public MDOP_BASE3_1S
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fadd_1s; }
        };

        struct MDOP_SUBS_1S : public MDOP_BASE3_1S
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fsub_1s; }
        };

        struct MDOP_MULS_1S : public MDOP_BASE3_1S
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fmul_1s; }
        };

        struct MDOP_DIVS_1S : public MDOP_BASE3_1S
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fdiv_1s; }
        };

        struct MDOP_ADDS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fadd_4s; }
        };

        struct MDOP_SUBS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fsub_4s; }
        };

        struct MDOP_MULS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fmul_4s; }
        };

        struct MDOP_DIVS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fdiv_4s; }
        };

        struct MDOP_ADDS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fadd_4s; }
        };

        struct MDOP_SUBS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fsub_4s; }
        };

        struct MDOP_MULS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fmul_4s; }
        };

        struct MDOP_DIVS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fdiv_4s; }
        };

        struct MDOP_ABSS_1S : public MDOP_BASE2_1S
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fabs_1s; }
        };

        struct MDOP_MINS_1S : public MDOP_BASE3_1S
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fmin_1s; }
        };

        struct MDOP_MAXS_1S : public MDOP_BASE3_1S
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fmax_1s; }
        };

        struct MDOP_ABSS_MEMRVV : public MDOP_BASE2_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fabs_4s; }
        };

        struct MDOP_MINS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fmin_4s; }
        };

        struct MDOP_MAXS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fmax_4s; }
        };

        struct MDOP_ABSS : public MDOP_BASE2
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fabs_4s; }
        };

        struct MDOP_MINS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fmin_4s; }
        };

        struct MDOP_MAXS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fmax_4s; }
        };

        struct MDOP_CMPLTS_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fcmlt_4s; }
        };

        struct MDOP_CMPLTS_REV_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fcmgt_4s; }
        };

        struct MDOP_CMPLTS_1S : public MDOP_BASE3_IR1S
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fcmlt_1s; }
        };

        struct MDOP_CMPGTS : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fcmgt_4s; }
        };

        struct MDOP_TOSINGLE_MEMRVV : public MDOP_BASE2_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Vfcvtfxv_4s; }
        };

        struct MDOP_TOWORD_MEMRVV : public MDOP_BASE2_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Vfcvtxfv_4s; }
        };

        struct MDOP_TOSINGLE_SR1I : public MDOP_BASE2_SR1I
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fcvtsw_1s; }
        };

        struct MDOP_TOWORD_IR1S : public MDOP_BASE2_IR1S
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fcvtws_1s; }
        };

        struct MDOP_TOSINGLE : public MDOP_BASE2
        {
            /*static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Scvtf_4s; }*/
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Vfcvtfxv_4s; }
        };

        struct MDOP_TOWORD : public MDOP_BASE2
        {
            /*static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Fcvtzs_4s; }*/
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Vfcvtxfv_4s; }
        };

        struct MDOP_AND_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::And_16b_Mem; }
        };

        struct MDOP_OR_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Orr_16b_Mem; }
        };

        struct MDOP_XOR_MEM : public MDOP_BASE3_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Eor_16b_Mem; }
        };

        struct MDOP_NOT_MEM : public MDOP_BASE2_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Mvn_16b_Mem; }
        };

        struct MDOP_AND_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::And_16b; }
        };

        struct MDOP_OR_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Orr_16b; }
        };

        struct MDOP_XOR_MEMRVV : public MDOP_BASE3_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Eor_16b; }
        };

        struct MDOP_NOT_MEMRVV : public MDOP_BASE2_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Mvn_16b; }
        };

        struct MDOP_AND : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::And_16b; }
        };

        struct MDOP_OR : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Orr_16b; }
        };

        struct MDOP_XOR : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Eor_16b; }
        };

        struct MDOP_NOT : public MDOP_BASE2
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Mvn_16b; }
        };

        struct MDOP_UNPACK_LOWER_BH : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Zip1_16b; }
        };

        struct MDOP_UNPACK_LOWER_HW : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Zip1_8h; }
        };

        struct MDOP_UNPACK_LOWER_WD : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Zip1_4s; }
        };

        struct MDOP_UNPACK_UPPER_BH : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e8_16_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Zip2_16b; }
        };

        struct MDOP_UNPACK_UPPER_HW : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Zip2_8h; }
        };

        struct MDOP_UNPACK_UPPER_WD : public MDOP_BASE3
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Zip2_4s; }
        };

        struct MDOP_CMPLTZW : public MDOP_BASE2
        {
            static OpRegType OpReg() { return &CRV64Assembler::Cmltz_4s; }
        };

        struct MDOP_CMPEQZS : public MDOP_BASE2
        {
            static OpRegType OpReg() { return &CRV64Assembler::Fcmeqz_4s; }
        };

        struct MDOP_SLLH_MEM : public MDOP_SHIFT_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Shl_8h_Mem; }
        };

        struct MDOP_SLLW_MEM : public MDOP_SHIFT_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Shl_4s_Mem; }
        };

        struct MDOP_SRLH_MEM : public MDOP_SHIFT_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Ushr_8h_Mem; }
        };

        struct MDOP_SRLW_MEM : public MDOP_SHIFT_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Ushr_4s_Mem; }
        };

        struct MDOP_SRAH_MEM : public MDOP_SHIFT_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Sshr_8h_Mem; }
        };

        struct MDOP_SRAW_MEM : public MDOP_SHIFT_MEM
        {
            static OpRegType OpReg() { return &CRV64Assembler::Sshr_4s_Mem; }
        };

        struct MDOP_SLLH_MEMRVV : public MDOP_SHIFT_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Shl_8h; }
        };

        struct MDOP_SLLW_MEMRVV : public MDOP_SHIFT_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Shl_4s; }
        };

        struct MDOP_SRLH_MEMRVV : public MDOP_SHIFT_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Ushr_8h; }
        };

        struct MDOP_SRLW_MEMRVV : public MDOP_SHIFT_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Ushr_4s; }
        };

        struct MDOP_SRAH_MEMRVV : public MDOP_SHIFT_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sshr_8h; }
        };

        struct MDOP_SRAW_MEMRVV : public MDOP_SHIFT_MEMRVV
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sshr_4s; }
        };

        struct MDOP_SLLH : public MDOP_SHIFT
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Shl_8h; }
        };

        struct MDOP_SLLW : public MDOP_SHIFT
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Shl_4s; }
        };

        struct MDOP_SRLH : public MDOP_SHIFT
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Ushr_8h; }
        };

        struct MDOP_SRLW : public MDOP_SHIFT
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Ushr_4s; }
        };

        struct MDOP_SRAH : public MDOP_SHIFT
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e16_8_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sshr_8h; }
        };

        struct MDOP_SRAW : public MDOP_SHIFT
        {
            static constexpr CRV64Assembler::VConfig Config = CRV64Assembler::VConfig_e32_4_Vector;
            static OpRegType OpReg() { return &CRV64Assembler::Sshr_4s; }
        };

        uint16    GetSavedRegisterList(uint32);
        void      Emit_Prolog(const StatementList&, uint32);
        void      Emit_Epilog();

        CRV64Assembler::LABEL GetLabel(uint32);
        void                     MarkLabel(const STATEMENT&);

        void    Emit_Nop(const STATEMENT&);
        void    Emit_Break(const STATEMENT&);

        void    Emit_Mov_RegReg(const STATEMENT&);
        void    Emit_Mov_RegMem(const STATEMENT&);
        void    Emit_Mov_RegCst(const STATEMENT&);
        void    Emit_Mov_MemReg(const STATEMENT&);
        void    Emit_Mov_MemMem(const STATEMENT&);
        void    Emit_Mov_MemCst(const STATEMENT&);

        void    Emit_Mov_RegRefMemRef(const STATEMENT&);
        void    Emit_Mov_MemRefRegRef(const STATEMENT&);

        void    Emit_Not_VarVar(const STATEMENT&);
#if 0
        void    Emit_CLZ(CRV64Assembler::REGISTER64 r, CRV64Assembler::REGISTER64 n, CRV64Assembler::REGISTER64 shift, CRV64Assembler::REGISTER64 tmp, CRV64Assembler::REGISTER64 sixtyfour);
#endif
        void    Emit_Lzc_VarVar(const STATEMENT&);

        void    Emit_Mov_Mem64Mem64(const STATEMENT&);
        void    Emit_Mov_Mem64Cst64(const STATEMENT&);

        void    Emit_ExtLow64VarMem64(const STATEMENT&);
        void    Emit_ExtHigh64VarMem64(const STATEMENT&);
        void    Emit_MergeTo64_Mem64AnyAny(const STATEMENT&);

        void    Emit_RelToRef_VarCst(const STATEMENT&);
        void    Emit_AddRef_VarVarAny(const STATEMENT&);
        void    Emit_IsRefNull_VarVar(const STATEMENT&);
        void    Emit_LoadFromRef_VarVar(const STATEMENT&);
        void    Emit_LoadFromRef_VarVarAny(const STATEMENT&);
        void    Emit_LoadFromRef_Ref_VarVar(const STATEMENT&);
        void    Emit_LoadFromRef_Ref_VarVarAny(const STATEMENT&);
        void    Emit_StoreAtRef_VarAny(const STATEMENT&);
        void    Emit_StoreAtRef_VarAnyAny(const STATEMENT&);

        void    Emit_Load8FromRef_MemVar(const STATEMENT&);
        void    Emit_Load8FromRef_MemVarAny(const STATEMENT&);
        void    Emit_Store8AtRef_VarAny(const STATEMENT&);
        void    Emit_Store8AtRef_VarAnyAny(const STATEMENT&);

        void    Emit_Load16FromRef_MemVar(const STATEMENT&);
        void    Emit_Load16FromRef_MemVarAny(const STATEMENT&);
        void    Emit_Store16AtRef_VarAny(const STATEMENT&);
        void    Emit_Store16AtRef_VarAnyAny(const STATEMENT&);

        void    Emit_LoadFromRef_64_MemVar(const STATEMENT&);
        void    Emit_LoadFromRef_64_MemVarAny(const STATEMENT&);
        void    Emit_StoreAtRef_64_VarAny(const STATEMENT&);
        void    Emit_StoreAtRef_64_VarAnyAny(const STATEMENT&);

        void    Emit_Param_Ctx(const STATEMENT&);
        void    Emit_Param_Reg(const STATEMENT&);
        void    Emit_Param_Mem(const STATEMENT&);
        void    Emit_Param_Cst(const STATEMENT&);
        void    Emit_Param_Mem64(const STATEMENT&);
        void    Emit_Param_Cst64(const STATEMENT&);
        void    Emit_Param_Reg128(const STATEMENT&);
        void    Emit_Param_Mem128(const STATEMENT&);

#if !SUPPORT_128
        void    Emit_ParamRet_Tmp128(const STATEMENT&);
#endif

        void    Emit_Call(const STATEMENT&);
        void    Emit_RetVal_Reg(const STATEMENT&);
        void    Emit_RetVal_Tmp(const STATEMENT&);
        void    Emit_RetVal_Mem64(const STATEMENT&);
        void    Emit_RetVal_Reg128(const STATEMENT&);
        void    Emit_RetVal_Mem128(const STATEMENT&);

        void    Emit_ExternJmp(const STATEMENT&);
        void    Emit_ExternJmpDynamic(const STATEMENT&);

        void    Emit_Jmp(const STATEMENT&);

        void    Emit_CondJmp(const STATEMENT&);
        void    Emit_CondJmp(const STATEMENT& statement, CRV64Assembler::REGISTER32 src1Reg, CRV64Assembler::REGISTER32 src2Reg);
        void    Emit_CondJmp_AnyVar(const STATEMENT&);
        void    Emit_CondJmp_VarCst(const STATEMENT&);

        void    Emit_CondJmp_Ref_VarCst(const STATEMENT&);

        void    Cmp_GetFlag(CRV64Assembler::REGISTER32, Jitter::CONDITION);
        bool    Cmp_GetFlag(CRV64Assembler::REGISTER32, Jitter::CONDITION, CRV64Assembler::REGISTER32 src1Reg, int16 imm);
        void    Cmp_GetFlag(CRV64Assembler::REGISTER32, Jitter::CONDITION, CRV64Assembler::REGISTER32 src1Reg, CRV64Assembler::REGISTER32 src2Reg);
        bool    Cmp_GetFlag(CRV64Assembler::REGISTER32, Jitter::CONDITION, CRV64Assembler::REGISTER64 src1Reg, int16 imm);
        void    Cmp_GetFlag(CRV64Assembler::REGISTER32, Jitter::CONDITION, CRV64Assembler::REGISTER64 src1Reg, CRV64Assembler::REGISTER64 src2Reg);
        void    Cmp_GetFlag(CRV64Assembler::REGISTER32, Jitter::CONDITION, CRV64Assembler::REGISTERMD src1Reg, CRV64Assembler::REGISTERMD src2Reg);
        void    Emit_Cmp_VarAnyVar(const STATEMENT&);
        void    Emit_Cmp_VarVarCst(const STATEMENT&);

        void    Emit_Add64_MemMemMem(const STATEMENT&);
        void    Emit_Add64_MemMemCst(const STATEMENT&);

        void    Emit_Sub64_MemAnyMem(const STATEMENT&);
        void    Emit_Sub64_MemMemCst(const STATEMENT&);

        void    Emit_Cmp64_VarAnyMem(const STATEMENT&);
        void    Emit_Cmp64_VarMemCst(const STATEMENT&);

        void    Emit_And64_MemMemMem(const STATEMENT&);

        //ADDSUB
        template <typename> void    Emit_AddSub_VarAnyVar(const STATEMENT&);
        template <typename> void    Emit_Add_VarVarCst(const STATEMENT&);
        template <typename> void    Emit_Sub_VarVarCst(const STATEMENT&);

        //SHIFT
        template <typename> void    Emit_Shift_VarAnyVar(const STATEMENT&);
        template <typename> void    Emit_Shift_VarVarCst(const STATEMENT&);

        //LOGIC
        template <typename> void    Emit_Logic_VarAnyVar(const STATEMENT&);
        template <typename> void    Emit_Logic_VarVarCst(const STATEMENT&);

        //MUL
        template <bool> void Emit_Mul_Tmp64AnyAny(const STATEMENT&);

        //DIV
        template <bool> void Emit_Div_Tmp64AnyAny(const STATEMENT&);

        //SHIFT64
        template <typename> void    Emit_Shift64_MemMemVar(const STATEMENT&);
        template <typename> void    Emit_Shift64_MemMemCst(const STATEMENT&);

        //FPU
        template <typename> void    Emit_Fpu_MemMem(const STATEMENT&);
        template <typename> void    Emit_Fpu_MemMemMem(const STATEMENT&);

        void    Emit_Fp_Cmp_AnyMemMem(const STATEMENT&);
        void    Emit_Fp_Rcpl_MemMem(const STATEMENT&);
        void    Emit_Fp_Rsqrt_MemMem(const STATEMENT&);
        void    Emit_Fp_Clamp_MemMem(const STATEMENT&);
        void    Emit_Fp_Mov_MemSRelI32(const STATEMENT&);
        void    Emit_Fp_ToIntTrunc_MemMem(const STATEMENT&);
        void    Emit_Fp_LdCst_TmpCst(const STATEMENT&);

// MD RVV
        //MD
        template <typename> void    Emit_Md_VarVar(const STATEMENT&);
        template <typename> void    Emit_Md_VarVarVar(const STATEMENT&);
        template <typename> void    Emit_Md_VarVarVarRev(const STATEMENT&);
        template <typename> void    Emit_Md_Shift_VarVarCst(const STATEMENT&);

        void    Emit_Md_ClampS_VarVar(const STATEMENT&);
        void    Emit_Md_MakeSz_VarVar(const STATEMENT&);

        void    Emit_Md_Mov_RegReg(const STATEMENT&);
        void    Emit_Md_Mov_RegMem(const STATEMENT&);
        void    Emit_Md_Mov_MemReg(const STATEMENT&);
        void    Emit_Md_Mov_MemMem_RVV(const STATEMENT&);

        void    Emit_Md_LoadFromRef_VarVar(const STATEMENT&);
        void    Emit_Md_LoadFromRef_VarVarAny(const STATEMENT&);
        void    Emit_Md_StoreAtRef_VarVar(const STATEMENT&);
        void    Emit_Md_StoreAtRef_VarAnyVar(const STATEMENT&);

        void    Emit_Md_MovMasked_VarVarVar(const STATEMENT&);
        void    Emit_Md_Expand_VarReg(const STATEMENT&);
        void    Emit_Md_Expand_VarMem(const STATEMENT&);
        void    Emit_Md_Expand_VarCst(const STATEMENT&);

        void    Emit_Md_PackHB_VarVarVar(const STATEMENT&);
        void    Emit_Md_PackWH_VarVarVar(const STATEMENT&);

        template <uint32> void					Emit_Md_UnpackBH_VarVarVar(const STATEMENT&);
        template <uint32> void					Emit_Md_UnpackUpperBH_VarVarVar(const STATEMENT&);
        template <uint32> void					Emit_Md_UnpackHW_VarVarVar(const STATEMENT&);
        template <uint32> void					Emit_Md_UnpackUpperHW_VarVarVar(const STATEMENT&);
        template <uint32> void					Emit_Md_UnpackWD_VarVarVar(const STATEMENT&);
        template <uint32> void					Emit_Md_UnpackUpperWD_VarVarVar(const STATEMENT&);

        void    Emit_MergeTo256_MemVarVar(const STATEMENT&);
        void    Emit_Md_Srl256_VarMemVar(const STATEMENT&);
        void    Emit_Md_Srl256_VarMemCst(const STATEMENT&);

// MD MEM
        //MDOP
        template <typename> void				Emit_Md_MemMem1S(const STATEMENT&);
        template <typename> void				Emit_Md_MemMemIR1S(const STATEMENT&);
        template <typename> void				Emit_Md_MemMemSR1I(const STATEMENT&);
        template <typename> void				Emit_Md_MemMemRVV(const STATEMENT&);
        template <typename> void				Emit_Md_MemMem(const STATEMENT&);
        template <typename> void				Emit_Md_MemMemMem1S(const STATEMENT&);
        template <typename> void				Emit_Md_MemMemMemIR1S(const STATEMENT&);
        template <typename> void				Emit_Md_MemMemMemRVV(const STATEMENT&);
        template <typename> void				Emit_Md_MemMemMem(const STATEMENT&);
        template <typename> void				Emit_Md_MemMemMemRev1S(const STATEMENT&);
        template <typename> void				Emit_Md_MemMemMemRevIR1S(const STATEMENT&);
        template <typename> void				Emit_Md_MemMemMemRev(const STATEMENT&);
        template <typename> void				Emit_Md_Shift_MemMemCst(const STATEMENT&);
        template <typename> void				Emit_Md_Shift_MemMemCstRVV(const STATEMENT&);

        void									Emit_Md_Mov_MemMem(const STATEMENT&);
        void									Emit_Md_DivS_MemMemMem(const STATEMENT&);

        void									Emit_Md_Srl256_MemMemVar(const STATEMENT&);
        void									Emit_Md_Srl256_MemMemCst(const STATEMENT&);

        void									Emit_Md_LoadFromRef_MemVar(const STATEMENT&);
        void									Emit_Md_LoadFromRef_MemVarAny(const STATEMENT&);
        void									Emit_Md_StoreAtRef_VarMem(const STATEMENT&);
        void									Emit_Md_StoreAtRef_VarAnyMem(const STATEMENT&);

        void									Emit_Md_MovMasked_MemMemMem(const STATEMENT&);
        void									Emit_Md_Expand_MemReg(const STATEMENT&);
        void									Emit_Md_Expand_MemMem(const STATEMENT&);
        void									Emit_Md_Expand_MemCst(const STATEMENT&);

        void									Emit_Md_ClampS_MemMem(const STATEMENT&);
        void									Emit_Md_MakeSz_VarMem(const STATEMENT&);

        void									Emit_Md_PackHB_MemMemMem(const STATEMENT&);
        void									Emit_Md_PackWH_MemMemMem(const STATEMENT&);

        template <uint32> void					Emit_Md_UnpackBH_MemMemMem(const STATEMENT&);
        template <uint32> void					Emit_Md_UnpackHW_MemMemMem(const STATEMENT&);
        template <uint32> void					Emit_Md_UnpackWD_MemMemMem(const STATEMENT&);

        void									Emit_MergeTo256_MemMemMem(const STATEMENT&);
// END

        static CONSTMATCHER    g_constMatchers[];
        static CONSTMATCHER    g_64ConstMatchers[];
        static CONSTMATCHER    g_fpuConstMatchers[];
        static CONSTMATCHER    g_mdConstMatchersMem[];
        static CONSTMATCHER    g_mdConstMatchersMemRVV[];
        static CONSTMATCHER    g_mdConstMatchersRVV[];

        static CRV64Assembler::REGISTER32    g_registers[MAX_REGISTERS];
        static CRV64Assembler::REGISTERMD    g_registersMd[MAX_MDREGISTERS];
        static CRV64Assembler::REGISTER32    g_tempRegisters[MAX_TEMP_REGS];
        static CRV64Assembler::REGISTER64    g_tempRegisters64[MAX_TEMP_REGS];
        static CRV64Assembler::REGISTERMD    g_tempRegistersMd[MAX_TEMP_MD_REGS];
        static CRV64Assembler::REGISTER32    g_paramRegisters[MAX_PARAM_REGS];
        static CRV64Assembler::REGISTER64    g_paramRegisters64[MAX_PARAM_REGS];
        static CRV64Assembler::REGISTER64    g_baseRegister;

        static const LITERAL128    g_fpClampMask1;
        static const LITERAL128    g_fpClampMask2;

        Framework::CStream*    m_stream = nullptr;
        CRV64Assembler         m_assembler;
        LabelMapType           m_labels;
        ParamStack             m_params;
        uint32                 m_nextTempRegister = 0;
        uint32                 m_nextTempRegisterMd = 0;
        uint16                 m_registerSave = 0;
        uint32                 m_paramSpillBase = 0;

        bool    m_generateRelocatableCalls = false;
        bool    m_thead_extentions = false;
    };
};
