#pragma once

#include <map>
#include <vector>
#include "Stream.h"
#include "Literal128.h"

class CRV64Assembler
{
public:

    enum REGISTER32
    {
        zero, ra, sp, gp,
        tp, t0, t1, t2,
        s0, s1, a0, a1,
        a2, a3, a4, a5,
        a6, a7, s2, s3,
        s4, s5, s6, s7,
        s8, s9, s10, s11,
        t3, t4, t5, t6, fp=8
    };

    enum REGISTER64
    {
        x0,  x1,  x2,  x3,
        x4,  x5,  x6,  x7,
        x8,  x9,  x10, x11,
        x12, x13, x14, x15,
        x16, x17, x18, x19,
        x20, x21, x22, x23,
        x24, x25, x26, x27,
        x28, x29, x30, x31, xRA=1, xSP=2, xFP=8, xZR=0
    };

    enum REGISTERMD
    {
        v0,  v1,  v2,  v3,
        v4,  v5,  v6,  v7,
        v8,  v9,  v10, v11,
        v12, v13, v14, v15,
        v16, v17, v18, v19,
        v20, v21, v22, v23,
        v24, v25, v26, v27,
        v28, v29, v30, v31
    };

    enum CONDITION
    {
        CONDITION_EQ,
        CONDITION_NE,
        CONDITION_CS,
        CONDITION_CC,
        CONDITION_MI,
        CONDITION_PL,
        CONDITION_VS,
        CONDITION_VC,
        CONDITION_HI,
        CONDITION_LS,
        CONDITION_GE,
        CONDITION_LT,
        CONDITION_GT,
        CONDITION_LE,
        CONDITION_AL,
        CONDITION_NV
    };

    typedef unsigned int LABEL;

    virtual    ~CRV64Assembler() = default;

    void    SetStream(Framework::CStream*);

    LABEL    CreateLabel();
    void     ClearLabels();
    void     MarkLabel(LABEL);
    void     ResolveLabelReferences();
    void     ResolveLiteralReferences();

    void    Clamp_si32(REGISTER64, REGISTER64);
    void    Clamp_si16(REGISTER32, REGISTER32);
    void    Clamp_si8(REGISTER32, REGISTER32);

    void    Clamp_ui32(REGISTER64, REGISTER64);
    void    Clamp_ui16(REGISTER32, REGISTER32);
    void    Clamp_ui8(REGISTER32, REGISTER32);

    void    Add_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Add_8h(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Add_16b(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Add_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Add_8h_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Add_16b_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    And_16b(REGISTERMD, REGISTERMD, REGISTERMD);

    void    And_16b_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    Asr(REGISTER32, REGISTER32, uint8);
    void    Asr(REGISTER64, REGISTER64, uint8);
    void    Asrv(REGISTER32, REGISTER32, REGISTER32);
    void    Asrv(REGISTER64, REGISTER64, REGISTER64);
    void    B(LABEL);
    void    B_offset(uint32);
    void    Bl(uint32);
    void	Br(REGISTER64);
    void    BCc(CONDITION, LABEL);
    void    BCc(CONDITION, LABEL, REGISTER32, REGISTER32);
    void    Blr(REGISTER64);
    void    Cbnz(REGISTER32, LABEL);
    void    Cbnz(REGISTER64, LABEL);
    void    Cbz(REGISTER32, LABEL);
    void    Cbz(REGISTER64, LABEL);
    void    Th_ff0(REGISTER32 rd, REGISTER32 rs1);
    void    Th_ff1(REGISTER32 rd, REGISTER32 rs1);
    void    Clz(REGISTER32, REGISTER32);
    void    Cmeq_16b_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Cmeq_8h_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Cmeq_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Cmgt_16b_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Cmgt_8h_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Cmgt_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Cmeq_16b(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Cmeq_8h(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Cmeq_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Cmgt_16b(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Cmgt_8h(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Cmgt_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Cmltz_4s(REGISTERMD, REGISTERMD);
    void    Cmp(REGISTER32, REGISTER32);
    void    Cmp(REGISTER64, REGISTER64);
    void    Cset(REGISTER32, CONDITION);
    void    Sltiu(REGISTER32 rd, REGISTER32 rn, uint16 imm);
    void    XorSltu(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm);
    void    XorSltu(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm);
    void    Sltu(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm);
    void    Slt(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm);
    void    Slli(CRV64Assembler::REGISTER32 rd, CRV64Assembler::REGISTER32 rs, uint32 rm);
    void    Li(CRV64Assembler::REGISTER32 registerId, uint32 constant);
    void    Li(CRV64Assembler::REGISTER64 registerId, uint64 constant);
    void    Dup_4s(REGISTERMD, REGISTER32);

    void    Eor_16b(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Eor_16b_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    Fabs_1s(REGISTERMD, REGISTERMD);
    void    Fabs_4s(REGISTERMD, REGISTERMD);
    void    Fadd_1s(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Fadd_4s(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Fadd_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    Fcmeqz_4s(REGISTERMD, REGISTERMD);
    void    Fcmge_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Fcmgt_4s(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Fcmlt_1s(REGISTER32, REGISTERMD, REGISTERMD);

    void    Fcmltz_4s(REGISTERMD, REGISTERMD);
    void    Fcmp_1s(REGISTERMD, REGISTERMD);
    void    Fcvtzs_1s(REGISTERMD, REGISTERMD);
    void    Fcvtws_1s(REGISTER32, REGISTERMD);
    void    Fcvtzs_4s(REGISTERMD, REGISTERMD);
    void    Fdiv_1s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Fdiv_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Fmov_1s(REGISTERMD, uint8);
    void    Fmv_1s(REGISTERMD rd, REGISTER32 rs);
    void    Fmv_x_w(REGISTER32 rd, REGISTERMD rs);
    void    Fmul_1s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Fmul_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Fclass_1s(REGISTER32 rd, REGISTERMD rn);
    void    Flt_1s(REGISTER32 rd, REGISTERMD rn, REGISTERMD rm);
    void    Fle_1s(REGISTER32 rd, REGISTERMD rn, REGISTERMD rm);
    void    Feq_1s(REGISTER32 rd, REGISTERMD rn, REGISTERMD rm);
    void    Fmax_1s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Fmax_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Fneg_1s(REGISTERMD, REGISTERMD);
    void    Fmin_1s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Fmin_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Fsqrt_1s(REGISTERMD, REGISTERMD);
    void    Fsub_1s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Fsub_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Ins_1s(REGISTERMD, uint8, REGISTERMD, uint8);
    void    Ins_1d(REGISTERMD, uint8, REGISTER64);

    void    Ld1_4s(REGISTERMD, REGISTER64);
    void    Ldp_PostIdx(REGISTER64, REGISTER64, REGISTER64, int32);
    void    Ldrb(REGISTER32, REGISTER64, uint32);
    //void    Ldrb(REGISTER32, REGISTER64, REGISTER64, bool);
    void    Ldrh(REGISTER32, REGISTER64, uint32);
    //void    Ldrh(REGISTER32, REGISTER64, REGISTER64, bool);
    void    Ldrhs(REGISTER32, REGISTER64, uint32);
    void    Ldr_Pc(REGISTER64, uint32);
    void    Ldr_Pc(REGISTERMD, const LITERAL128&, REGISTER64);
    void    Ldr_1s(REGISTERMD, REGISTER64, uint32);
    void    Ldr_1q(REGISTERMD, REGISTER64, uint32);
    void    Ldr_1q(REGISTERMD, REGISTER64, REGISTER64, bool);
    void    Lsl(REGISTER32, REGISTER32, uint8);
    void    Lsl(REGISTER64, REGISTER64, uint8);
    void    Lslv(REGISTER32, REGISTER32, REGISTER32);
    void    Lslv(REGISTER64, REGISTER64, REGISTER64);
    void    Lsr(REGISTER32, REGISTER32, uint8);
    void    Lsr(REGISTER64, REGISTER64, uint8);
    void    Lsrv(REGISTER32, REGISTER32, REGISTER32);
    void    Lsrv(REGISTER64, REGISTER64, REGISTER64);
    void    Mov(REGISTER32, REGISTER32);
    void    Mov(REGISTER64, REGISTER64);
    void    Mov(REGISTERMD, REGISTERMD);
    void    Mov_Sp(REGISTER64, REGISTER64);
    void    Movn(REGISTER32, uint16, uint8);
    void    Movk(REGISTER32, uint16, uint8);
    void    Movk(REGISTER64, uint16, uint8);
    void    Movz(REGISTER32, uint16, uint8);
    void    Movz(REGISTER64, uint16, uint8);
    void    Msub(REGISTER32, REGISTER32, REGISTER32, REGISTER32);
    void    Mvn(REGISTER32, REGISTER32);

    void    Mvn_16b(REGISTERMD, REGISTERMD);

    void    Mvn_16b_Mem(REGISTER64, REGISTER64, REGISTER32);

    void    Orn_16b(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Orr_16b(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Orr_16b_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    Ret(REGISTER64 = xRA);
    void    Scvtf_1s(REGISTERMD, REGISTERMD);
    void    Fcvtsw_1s(REGISTERMD rd, REGISTER32 rn);
    void    Scvtf_4s(REGISTERMD, REGISTERMD);
    void    Sdiv(REGISTER32, REGISTER32, REGISTER32);

    void    Shl_4s(REGISTERMD, REGISTERMD, uint8);
    void    Shl_8h(REGISTERMD, REGISTERMD, uint8);

    void    Shl_4s_Mem(REGISTER64, REGISTER64, uint8, REGISTER32);
    void    Shl_8h_Mem(REGISTER64, REGISTER64, uint8, REGISTER32);

    void    Smin_1s(REGISTER32, REGISTER32, REGISTER32);
    void    Umin_1s(REGISTER32, REGISTER32, REGISTER32);

    void    Smax_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Smax_8h(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Smin_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Smin_8h(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Smax_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Smax_8h_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Smin_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Smin_8h_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    Smull(REGISTER64, REGISTER32, REGISTER32);
    void    Smull(REGISTER64, REGISTER32, REGISTER32, REGISTER64, REGISTER64);

    void    Sshr_4s(REGISTERMD, REGISTERMD, uint8);
    void    Sshr_8h(REGISTERMD, REGISTERMD, uint8);

    void    Sshr_4s_Mem(REGISTER64, REGISTER64, uint8, REGISTER32);
    void    Sshr_8h_Mem(REGISTER64, REGISTER64, uint8, REGISTER32);

    void    Sqadd_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Sqadd_8h(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Sqadd_16b(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Sqadd_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Sqadd_8h_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Sqadd_16b_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    Sqsub_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Sqsub_8h(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Sqsub_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Sqsub_8h_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    St1_4s(REGISTERMD, REGISTER64);
    void    Stp(REGISTER32, REGISTER32, REGISTER64, int32);
    void    Stp_PreIdx(REGISTER64, REGISTER64, REGISTER64, int32);
    void    Str(REGISTER32, REGISTER64, uint32);
    //void    Str(REGISTER32, REGISTER64, REGISTER64, bool);
    void    Str(REGISTER64, REGISTER64, uint32);
    //void    Str(REGISTER64, REGISTER64, REGISTER64, bool);
    void    Strb(REGISTER32, REGISTER64, uint32);
    //void    Strb(REGISTER32, REGISTER64, REGISTER64, bool);
    void    Strh(REGISTER32, REGISTER64, uint32);
    //void    Strh(REGISTER32, REGISTER64, REGISTER64, bool);
    void    Str_1s(REGISTERMD, REGISTER64, uint32);
    void    Str_1q(REGISTERMD, REGISTER64, uint32);
    void    Str_1q(REGISTERMD, REGISTER64, REGISTER64, bool);

    void    Sub_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Sub_8h(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Sub_16b(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Sub_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Sub_8h_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Sub_16b_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    Tbl(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Tst(REGISTER32, REGISTER32);
    void    Tst(REGISTER64, REGISTER64);
    void    Uaddlv_16b(REGISTERMD, REGISTERMD);
    void    Udiv(REGISTER32, REGISTER32, REGISTER32);
    void    Umin_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Umov_1s(REGISTER32, REGISTERMD, uint8);
    void    Umull(REGISTER64, REGISTER32, REGISTER32);
    void    Umull(REGISTER64, REGISTER32, REGISTER32, REGISTER64, REGISTER64);

    void    Uqadd_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Uqadd_8h(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Uqadd_16b(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Uqadd_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Uqadd_8h_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Uqadd_16b_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    Uqsub_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Uqsub_8h(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Uqsub_16b(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Uqsub_4s_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Uqsub_8h_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);
    void    Uqsub_16b_Mem(REGISTER64, REGISTER64, REGISTER64, REGISTER32, REGISTER32);

    void    Ushr_4s(REGISTERMD, REGISTERMD, uint8);
    void    Ushr_8h(REGISTERMD, REGISTERMD, uint8);

    void    Ushr_4s_Mem(REGISTER64, REGISTER64, uint8, REGISTER32);
    void    Ushr_8h_Mem(REGISTER64, REGISTER64, uint8, REGISTER32);

    void    Xtn1_4h(REGISTERMD, REGISTERMD);
    void    Xtn1_8b(REGISTERMD, REGISTERMD);
    void    Xtn2_8h(REGISTERMD, REGISTERMD);
    void    Xtn2_16b(REGISTERMD, REGISTERMD);
    void    Zip1_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Zip1_8h(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Zip1_16b(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Zip2_4s(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Zip2_8h(REGISTERMD, REGISTERMD, REGISTERMD);
    void    Zip2_16b(REGISTERMD, REGISTERMD, REGISTERMD);

    void    Smin_1s_RVV(REGISTER32, REGISTER32, REGISTER32);
    void    Umin_1s_RVV(REGISTER32, REGISTER32, REGISTER32);

    void    Amominw(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    Min(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);

    void    Vlswv(REGISTERMD vd, REGISTER64 rs1, REGISTER64 rs2, int vm);
    void    Vlwv(REGISTERMD vd, REGISTER64 rs1, int vm);
    void    Vswv(REGISTERMD vs3, REGISTER64 rs1, int vm);
    void    Vlhv(REGISTERMD vd, REGISTER64 rs1, int vm);
    void    Vshv(REGISTERMD vs3, REGISTER64 rs1, int vm);
    void    Vlbv(REGISTERMD vd, REGISTER64 rs1, int vm);
    void    Vsbv(REGISTERMD vs3, REGISTER64 rs1, int vm);
    void    Vflwv(REGISTERMD vd, REGISTER64 rs1, int vm);
    void    Vfswv(REGISTERMD vs3, REGISTER64 rs1, int vm);
    void    Vsetvli(REGISTER64 rd, REGISTER64 rs1, int vtypei);
    void    Vsetvli(REGISTER64 rd, REGISTER64 rs1, int vtypei, int count);
    void    Vextxv(REGISTER64 rd, REGISTERMD vs1, REGISTER64 rs1);
    void    Vmvsx(REGISTERMD vd, REGISTER64 rs1);
    void    Vmaxvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vminvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vminvx(REGISTERMD vd, REGISTERMD vs2, REGISTER64 rs1, int vm);
    void    Vminuvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vminuvx(REGISTERMD vd, REGISTERMD vs2, REGISTER64 rs1, int vm);
    void    Vaddvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vsubvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vssubuvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vssubvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vsadduvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vsaddvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vsllvi(REGISTERMD vd, REGISTERMD vs2, int16 imm, int vm);
    void    Vsrlvi(REGISTERMD vd, REGISTERMD vs2, int16 imm, int vm);
    void    Vsravi(REGISTERMD vd, REGISTERMD vs2, int16 imm, int vm);
    void    Vmseqvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vmsltvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vmulhuvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vmulvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vandvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vxorvi(REGISTERMD vd, REGISTERMD vs2, int16 imm, int vm);
    void    Vxorvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vorvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vfaddvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vfsubvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vfmulvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vnsrlvi(REGISTERMD vd, REGISTERMD vs2, int16 imm, int vm);
    void    Vmfltvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vmfeqvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vsllvv(REGISTERMD vd, REGISTERMD vs2, REGISTERMD vs1, int vm);
    void    Vaddvi(REGISTERMD vd, REGISTERMD vs2, int16 imm, int vm);

    void    Lwu(REGISTER32 rd, REGISTER64 rs1, int32 imm);
    void    Lw(REGISTER32 rd, REGISTER64 rs1, int32 imm);
    void    Xorw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2);
    void    Orw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2);
    void    Andw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2);
    void    Xoriw(REGISTER32 rd, REGISTER32 rs1, int16 imm);
    void    Oriw(REGISTER32 rd, REGISTER32 rs1, int16 imm);
    void    Andiw(REGISTER32 rd, REGISTER32 rs1, int16 imm);

    void    Sll(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    Srl(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    Sra(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    Sllw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2);
    void    Srlw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2);
    void    Sraw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2);
    void    Add(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    Sub(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    Addw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2);
    void    Subw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2);
    void    Xor(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    Or(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    And(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    Slt(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    Sltu(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2);
    void    Slli(REGISTER64 rd, REGISTER64 rs1, uint8 shamt);
    void    Srli(REGISTER64 rd, REGISTER64 rs1, uint8 shamt);
    void    Srai(REGISTER64 rd, REGISTER64 rs1, uint8 shamt);
    void    Slliw(REGISTER32 rd, REGISTER32 rs1, uint8 shamt);
    void    Srliw(REGISTER32 rd, REGISTER32 rs1, uint8 shamt);
    void    Sraiw(REGISTER32 rd, REGISTER32 rs1, uint8 shamt);
    void    Addi(REGISTER64 rd, REGISTER64 rs1, int16 imm);
    void    Addiw(REGISTER32 rd, REGISTER32 rs1, int16 imm);
    void    Xori(REGISTER64 rd, REGISTER64 rs1, int16 imm);
    void    Ori(REGISTER64 rd, REGISTER64 rs1, int16 imm);
    void    Andi(REGISTER64 rd, REGISTER64 rs1, int16 imm);
    void    Slti(REGISTER64 rd, REGISTER64 rs1, int16 imm);
    void    Sltiu(REGISTER64 rd, REGISTER64 rs1, uint16 imm);
    void    Lb(REGISTER64 rd, REGISTER64 rs1, int32 imm);
    void    Lh(REGISTER64 rd, REGISTER64 rs1, int32 imm);
    void    Lw(REGISTER64 rd, REGISTER64 rs1, int32 imm);
    void    Lbu(REGISTER64 rd, REGISTER64 rs1, int32 imm);
    void    Lhu(REGISTER64 rd, REGISTER64 rs1, int32 imm);
    void    Ld(REGISTER64 rd, REGISTER64 rs1, int32 imm);
    void    Lwu(REGISTER64 rd, REGISTER64 rs1, int32 imm);
    void    Jalr(REGISTER64 rd, REGISTER64 rs1, int32 imm);
    void    Sb(REGISTER64 rs1, REGISTER64 rs2, int32 imm);
    void    Sh(REGISTER64 rs1, REGISTER64 rs2, int32 imm);
    void    Sw(REGISTER64 rs1, REGISTER64 rs2, int32 imm);
    void    Sd(REGISTER64 rs1, REGISTER64 rs2, int32 imm);
    void    Beq(REGISTER64 rs1, REGISTER64 rs2, int32 imm);
    void    Bne(REGISTER64 rs1, REGISTER64 rs2, int32 imm);
    void    Blt(REGISTER64 rs1, REGISTER64 rs2, int32 imm);
    void    Bge(REGISTER64 rs1, REGISTER64 rs2, int32 imm);
    void    Bltu(REGISTER64 rs1, REGISTER64 rs2, int32 imm);
    void    Bgeu(REGISTER64 rs1, REGISTER64 rs2, int32 imm);
    void    Lui(REGISTER64 rd, int32 imm);
    void    Auipc(REGISTER64 rd, int32 imm);
    void    Jal(REGISTER64 rd, int32 imm);

    void    Break();

    void    SetTHeadExtentions(bool available) {m_thead_extentions = available;}

private:
    struct LABELREF
    {
        size_t offset = 0;
        bool cbz = false;
        bool cbz64 = false;
        REGISTER32 cbRegister = a0;
        REGISTER32 src1Reg = a0;
        REGISTER32 src2Reg = a0;
        CONDITION condition;
    };

    struct LITERAL128REF
    {
        size_t offset = 0;
        uint32 rt = 0;
        uint32 rtmp = 0;
        LITERAL128 value = LITERAL128(0, 0);
    };

    typedef std::map<LABEL, size_t> LabelMapType;
    typedef std::multimap<LABEL, LABELREF> LabelReferenceMapType;

    typedef std::vector<LITERAL128REF> Literal128ArrayType;

    void    CreateBranchLabelReference(LABEL, CONDITION);
    void    CreateBranchLabelReference(LABEL, CONDITION, REGISTER32, REGISTER32);
    void    CreateCompareBranchLabelReference(LABEL, CONDITION, REGISTER32);
    void    CreateCompareBranchLabelReference(LABEL, CONDITION, REGISTER64);

    void    WriteR(uint32 opcode, uint32 rd, uint32 rs1, uint32 rs2);
    void    WriteIShiftDouble(uint32 opcode, uint32 rd, uint32 rs1, uint32 shamt);
    void    WriteIShiftWord(uint32 opcode, uint32 rd, uint32 rs1, uint32 shamt);
    void    WriteI(uint32 opcode, uint32 rd, uint32 rs1, int32 imm);
    void    WriteS(uint32 opcode, uint32 rs1, uint32 rs2, int32 imm);
    void    WriteB(uint32 opcode, uint32 rs1, uint32 rs2, int32 imm);
    void    WriteU(uint32 opcode, uint32 rd, int32 imm);
    void    WriteJ(uint32 opcode, uint32 rd, int32 imm);

    void    WriteAddSubOpImm(uint32, uint32 shift, uint32 imm, uint32 rn, uint32 rd, bool flip=false);
    void    WriteDataProcOpReg2(uint32, uint32 rm, uint32 rn, uint32 rd);
    void    WriteLogicalOpImm(uint32, uint32 n, uint32 immr, uint32 imms, uint32 rn, uint32 rd);
    void    WriteLogicalOpImm(uint32 opcode, uint32 shamt, uint32 rn, uint32 rd);
    void    WriteLoadStoreOpImm(uint32, uint32 imm, uint32 rn, uint32 rt);
    void    WriteMoveWideOpImm(uint32, uint32 hw, uint32 imm, uint32 rd);
    void    WriteWord(uint32);
private:

    unsigned int             m_nextLabelId = 1;
    LabelMapType             m_labels;
    LabelReferenceMapType    m_labelReferences;
    Literal128ArrayType      m_literal128Refs;

    Framework::CStream*    m_stream = nullptr;

    bool    m_thead_extentions = false;
};
