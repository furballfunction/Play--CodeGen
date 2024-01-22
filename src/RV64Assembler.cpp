#include <stdio.h>
#include <assert.h>
#include <stdexcept>
#include "RV64Assembler.h"
#include "LiteralPool.h"

#define SIGN_EXTEND_12(v) ((v & 0xFFF) | ((v & 0x800) ? 0xFFFFF000 : 0))
#define SIGN_EXTEND_12_INT16(v) ((v & 0xFFF) | ((v & 0x800) ? 0xF000 : 0))

void CRV64Assembler::SetStream(Framework::CStream* stream)
{
    m_stream = stream;
}

CRV64Assembler::LABEL CRV64Assembler::CreateLabel()
{
    return m_nextLabelId++;
}

void CRV64Assembler::ClearLabels()
{
    m_labels.clear();
}

void CRV64Assembler::MarkLabel(LABEL label)
{
    m_labels[label] = static_cast<size_t>(m_stream->Tell());
}

void CRV64Assembler::CreateBranchLabelReference(LABEL label, CONDITION condition)
{
    LABELREF reference;
    reference.offset    = static_cast<size_t>(m_stream->Tell());
    reference.condition = condition;
    m_labelReferences.insert(std::make_pair(label, reference));
}

void CRV64Assembler::CreateBranchLabelReference(LABEL label, CONDITION condition, REGISTER32 src1Reg, REGISTER32 src2Reg)
{
    LABELREF reference;
    reference.offset    = static_cast<size_t>(m_stream->Tell());
    reference.condition = condition;
    reference.src1Reg = src1Reg;
    reference.src2Reg = src2Reg;
    m_labelReferences.insert(std::make_pair(label, reference));
}

void CRV64Assembler::CreateCompareBranchLabelReference(LABEL label, CONDITION condition, REGISTER32 cbRegister)
{
    LABELREF reference;
    reference.offset     = static_cast<size_t>(m_stream->Tell());
    reference.condition  = condition;
    reference.cbz        = true;
    reference.cbRegister = cbRegister;
    m_labelReferences.insert(std::make_pair(label, reference));
}

void CRV64Assembler::CreateCompareBranchLabelReference(LABEL label, CONDITION condition, REGISTER64 cbRegister)
{
    LABELREF reference;
    reference.offset     = static_cast<size_t>(m_stream->Tell());
    reference.condition  = condition;
    reference.cbz64      = true;
    reference.cbRegister = static_cast<REGISTER32>(cbRegister);
    m_labelReferences.insert(std::make_pair(label, reference));
}

void CRV64Assembler::ResolveLabelReferences()
{
    for(const auto& labelReferencePair : m_labelReferences)
    {
        auto label(m_labels.find(labelReferencePair.first));
        if(label == m_labels.end())
        {
            throw std::runtime_error("Invalid label.");
        }
        const auto& labelReference = labelReferencePair.second;
        size_t labelPos = label->second;
        //int offset = static_cast<int>(labelPos - labelReference.offset) / 4;
        int offset = static_cast<int>(labelPos - labelReference.offset);

        m_stream->Seek(labelReference.offset, Framework::STREAM_SEEK_SET);
        if (uint32 space = m_stream->Read32(); space!=0) {
            printf("ResolveLabelReferences no space\n");
            assert(space==0);
        }
        m_stream->Seek(labelReference.offset, Framework::STREAM_SEEK_SET);
        if(labelReference.condition == CONDITION_AL)
        {
            /*uint32 opcode = 0x14000000;
            opcode |= (offset & 0x3FFFFFF);*/

            uint32 opcode = 0x00000063;
            opcode |= ((offset & 0x1E) >> 1) << 8;
            opcode |= ((offset & 0x800) >> 11) << 7;
            opcode |= ((offset & 0x1000) >> 12) << 31;
            opcode |= ((offset & 0x7E0) >> 5) << 25;
            opcode |= (xZR << 15);
            opcode |= (xZR << 20);
            WriteWord(opcode);
            //assert(0);
        }
        else
        {
            if(labelReference.cbz || labelReference.cbz64)
            {
                assert((labelReference.condition == CONDITION_EQ) || (labelReference.condition == CONDITION_NE));
                uint32 opcode = [&labelReference]() -> uint32
                    {
                        uint32 sf = labelReference.cbz64 ? 0x80000000 : 0;
                        switch(labelReference.condition)
                        {
                        case CONDITION_EQ:
                            //assert(0);
                            return 0x00000063;
                            //return 0x34000000 | sf;
                            break;
                        case CONDITION_NE:
                            //assert(0);
                            return 0x00001063;
                            //return 0x35000000 | sf;
                            break;
                        default:
                            assert(false);
                            return 0U;
                        }
                    }();
                /*opcode |= (offset & 0x7FFFF) << 5;
                opcode |= labelReference.cbRegister;*/
                //opcode |= (offset & 0x7FFFF) << 5;
                /*opcode |= (offset & 0x1E) << 7;
                opcode |= ((offset & 0x800) >> 11) << 7;
                opcode |= ((offset & 0x7E0) >> 11) << 7;
                opcode |= (0 << 7);*/

                opcode |= ((offset & 0x1E) >> 1) << 8;
                opcode |= ((offset & 0x800) >> 11) << 7;
                opcode |= ((offset & 0x1000) >> 12) << 31;
                opcode |= ((offset & 0x7E0) >> 5) << 25;
                opcode |= (xZR << 15);
                opcode |= (labelReference.cbRegister << 20);
                WriteWord(opcode);
            }
            else
            {
                /*uint32 opcode = 0x54000000;
                opcode |= (offset & 0x7FFFF) << 5;
                opcode |= labelReference.condition;
                WriteWord(opcode);*/

                bool flip = false;
                uint32 opcode = 0;
                if (labelReference.condition == CRV64Assembler::CONDITION_NE) {
                    // bne
                    opcode = 0x00001063;
                } else if (labelReference.condition == CRV64Assembler::CONDITION_EQ) {
                    // beq
                    opcode = 0x00000063;
                } else if (labelReference.condition == CRV64Assembler::CONDITION_GE) {
                    // Signed GE=1 : src1Reg >= src2Reg
                    // bge
                    opcode = 0x00005063;
                    // bgeu
                    //opcode = 0x00007063;
                } else if (labelReference.condition == CRV64Assembler::CONDITION_LE) {
                    // Signed LE=1 : src1Reg <= src2Reg
                    // bge
                    opcode = 0x00005063;
                    // bgeu
                    //opcode = 0x00007063;
                    flip = true;
                } else if (labelReference.condition == CRV64Assembler::CONDITION_GT) {
                    // Unknown Signed GT=1 : src1Reg > src2Reg
                    // bge
                    //opcode = 0x00005063;
                    // blt
                    opcode = 0x00004063;
                    // bgeu
                    //opcode = 0x00007063;
                    flip = true;
                } else if (labelReference.condition == CRV64Assembler::CONDITION_LT) {
                    // Unknown Signed LT=1 : src1Reg < src2Reg
                    // bge
                    //opcode = 0x00005063;
                    // blt
                    opcode = 0x00004063;
                    // bgeu
                    //opcode = 0x00007063;
                } else if (labelReference.condition == CRV64Assembler::CONDITION_CS) {
                    // Unsigned CS=1 : src1Reg >= src2Reg
                    // bge
                    //opcode = 0x00005063;
                    // bgeu
                    opcode = 0x00007063;
                } else if (labelReference.condition == CRV64Assembler::CONDITION_CC) {
                    // Unsigned CC=1 : src1Reg < src2Reg
                    // bge
                    //opcode = 0x00005063;
                    // bgeu
                    //opcode = 0x00007063;
                    // bltu
                    opcode = 0x00006063;
                } else if (labelReference.condition == CRV64Assembler::CONDITION_HI) {
                    // Unsigned HI=1 : src1Reg > src2Reg
                    // bge
                    //opcode = 0x00005063;
                    // bgeu
                    //opcode = 0x00007063;
                    // bltu
                    opcode = 0x00006063;
                    flip = true;
                } else if (labelReference.condition == CRV64Assembler::CONDITION_LS) {
                    // Unsigned LS=1 : src1Reg <= src2Reg
                    // bge
                    //opcode = 0x00005063;
                    // bgeu
                    opcode = 0x00007063;
                    // bltu
                    //opcode = 0x00006063;
                    flip = true;
                } else {
                    assert(0);
                }

                opcode |= ((offset & 0x1E) >> 1) << 8;
                opcode |= ((offset & 0x800) >> 11) << 7;
                opcode |= ((offset & 0x1000) >> 12) << 31;
                opcode |= ((offset & 0x7E0) >> 5) << 25;
                if (flip) {
                    opcode |= (labelReference.src2Reg << 15);
                    opcode |= (labelReference.src1Reg << 20);
                } else {
                    opcode |= (labelReference.src1Reg << 15);
                    opcode |= (labelReference.src2Reg << 20);
                }
                WriteWord(opcode);
            }
        }
    }
    m_stream->Seek(0, Framework::STREAM_SEEK_END);
    m_labelReferences.clear();
}

void CRV64Assembler::ResolveLiteralReferences()
{
    // test this
    if(m_literal128Refs.empty()) return;

    CLiteralPool literalPool(m_stream);
    literalPool.AlignPool();

    for(const auto& literalRef : m_literal128Refs)
    {
        auto literalPos = static_cast<uint32>(literalPool.GetLiteralPosition(literalRef.value));
        m_stream->Seek(literalRef.offset, Framework::STREAM_SEEK_SET);
        auto offset = literalPos - literalRef.offset;
        assert((offset & 0x03) == 0);
        assert(offset < 0x100000);
        offset /= 4;
        m_stream->Write32(0x9C000000 | static_cast<uint32>(offset << 5) | literalRef.rt);
    }
    m_literal128Refs.clear();
    m_stream->Seek(0, Framework::STREAM_SEEK_END);
}

void CRV64Assembler::Clamp_si32(REGISTER64 tmp1Reg, REGISTER64 tmp2Reg) {
    auto tmp1Reg32 = static_cast<CRV64Assembler::REGISTER32>(tmp1Reg);
    auto tmp2Reg32 = static_cast<CRV64Assembler::REGISTER32>(tmp2Reg);

    //Sub(tmp2Reg32, wZR, 1, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg32, wZR, -1);
    Slli(tmp2Reg32, tmp2Reg32, 1);
    Lsr(tmp2Reg32, tmp2Reg32, 1);

    uint16 offset = 4*4;
    // bge
    uint32 opcode = 0x00005063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp2Reg << 15);
    opcode |= (tmp1Reg << 20);
    WriteWord(opcode);

    //Sub(tmp1Reg32, wZR, 1, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg32, wZR, -1);
    Slli(tmp1Reg32, tmp1Reg32, 1);
    Lsr(tmp1Reg32, tmp1Reg32, 1);



    //Sub(tmp2Reg32, wZR, 1, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg32, wZR, -1);
    Slli(tmp2Reg32, tmp2Reg32, 1);
    Lsr(tmp2Reg32, tmp2Reg32, 1);

    opcode = 0x00004013;
    opcode |= (tmp2Reg <<  7);
    opcode |= (tmp2Reg << 15);
    opcode |= (-1 << 20);
    WriteWord(opcode);

    offset = 5*4;
    // bge
    opcode = 0x00005063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp1Reg << 15);
    opcode |= (tmp2Reg << 20);
    WriteWord(opcode);

    //Sub(tmp1Reg32, wZR, 1, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg32, wZR, -1);
    Slli(tmp1Reg32, tmp1Reg32, 1);
    Lsr(tmp1Reg32, tmp1Reg32, 1);

    opcode = 0x00004013;
    opcode |= (tmp1Reg <<  7);
    opcode |= (tmp1Reg << 15);
    opcode |= (-1 << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Clamp_ui32(REGISTER64 tmp1Reg, REGISTER64 tmp2Reg) {
    auto tmp1Reg32 = static_cast<CRV64Assembler::REGISTER32>(tmp1Reg);
    auto tmp2Reg32 = static_cast<CRV64Assembler::REGISTER32>(tmp2Reg);

    uint16 offset = 2*4;
    // bge
    uint32 opcode = 0x00005063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp1Reg << 15);
    opcode |= (xZR << 20);
    WriteWord(opcode);

    //Add(tmp1Reg32, wZR, 0, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg32, wZR, 0);

    //Sub(tmp2Reg32, wZR, 1, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg32, wZR, -1);
    Lsr(tmp2Reg, tmp2Reg, 32);

    offset = 4*3;
    // bgeu
    opcode = 0x00007063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp2Reg << 15);
    opcode |= (tmp1Reg << 20);
    WriteWord(opcode);

    //Sub(tmp1Reg32, wZR, 1, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg32, wZR, -1);
    Lsr(tmp1Reg, tmp1Reg, 32);
}

void CRV64Assembler::Clamp_si16(REGISTER32 tmp1Reg, REGISTER32 tmp2Reg) {
    //Add(tmp2Reg, wZR, 0x7f, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg, wZR, 0x7f);
    Slli(tmp2Reg, tmp2Reg, 8);
    //Add(tmp2Reg, tmp2Reg, 0xff, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg, tmp2Reg, 0xff);

    uint16 offset = 4*4;
    // bge
    uint32 opcode = 0x00005063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp2Reg << 15);
    opcode |= (tmp1Reg << 20);
    WriteWord(opcode);

    //Add(tmp1Reg, wZR, 0x7f, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, wZR, 0x7f);
    Slli(tmp1Reg, tmp1Reg, 8);
    //Add(tmp1Reg, tmp1Reg, 0xff, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, tmp1Reg, 0xff);



    //Add(tmp2Reg, wZR, 0x7f, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg, wZR, 0x7f);
    Slli(tmp2Reg, tmp2Reg, 8);
    //Add(tmp2Reg, tmp2Reg, 0xff, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg, tmp2Reg, 0xff);

    opcode = 0x00004013;
    opcode |= (tmp2Reg <<  7);
    opcode |= (tmp2Reg << 15);
    opcode |= (-1 << 20);
    WriteWord(opcode);

    offset = 5*4;
    // bge
    opcode = 0x00005063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp1Reg << 15);
    opcode |= (tmp2Reg << 20);
    WriteWord(opcode);

    //Add(tmp1Reg, wZR, 0x7f, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, wZR, 0x7f);
    Slli(tmp1Reg, tmp1Reg, 8);
    //Add(tmp1Reg, tmp1Reg, 0xff, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, tmp1Reg, 0xff);

    opcode = 0x00004013;
    opcode |= (tmp1Reg <<  7);
    opcode |= (tmp1Reg << 15);
    opcode |= (-1 << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Clamp_ui16(REGISTER32 tmp1Reg, REGISTER32 tmp2Reg) {
    uint16 offset = 2*4;
    // bge
    uint32 opcode = 0x00005063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp1Reg << 15);
    opcode |= (wZR << 20);
    WriteWord(opcode);

    //Add(tmp1Reg, wZR, 0, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, wZR, 0);

    //Add(tmp2Reg, wZR, 0xff, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg, wZR, 0xff);
    Slli(tmp2Reg, tmp2Reg, 8);
    //Add(tmp2Reg, tmp2Reg, 0xff, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg, tmp2Reg, 0xff);

    offset = 4*4;
    // bgeu
    opcode = 0x00007063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp2Reg << 15);
    opcode |= (tmp1Reg << 20);
    WriteWord(opcode);

    //Add(tmp1Reg, wZR, 0xff, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, wZR, 0xff);
    Slli(tmp1Reg, tmp1Reg, 8);
    //Add(tmp1Reg, tmp1Reg, 0xff, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, tmp1Reg, 0xff);
}

void CRV64Assembler::Clamp_si8(REGISTER32 tmp1Reg, REGISTER32 tmp2Reg) {
    //Add(tmp2Reg, wZR, 0x7f, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg, wZR, 0x7f);

    uint16 offset = 2*4;
    // bge
    uint32 opcode = 0x00005063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp2Reg << 15);
    opcode |= (tmp1Reg << 20);
    WriteWord(opcode);

    //Add(tmp1Reg, wZR, 0x7f, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, wZR, 0x7f);


    //Add(tmp2Reg, wZR, 0x7f, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg, wZR, 0x7f);

    opcode = 0x00004013;
    opcode |= (tmp2Reg <<  7);
    opcode |= (tmp2Reg << 15);
    opcode |= (-1 << 20);
    WriteWord(opcode);

    offset = 3*4;
    // bge
    opcode = 0x00005063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp1Reg << 15);
    opcode |= (tmp2Reg << 20);
    WriteWord(opcode);

    //Add(tmp1Reg, wZR, 0x7f, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, wZR, 0x7f);

    opcode = 0x00004013;
    opcode |= (tmp1Reg <<  7);
    opcode |= (tmp1Reg << 15);
    opcode |= (-1 << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Clamp_ui8(REGISTER32 tmp1Reg, REGISTER32 tmp2Reg) {
    uint16 offset = 2*4;
    // bge
    uint32 opcode = 0x00005063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp1Reg << 15);
    opcode |= (wZR << 20);
    WriteWord(opcode);
    //Add(tmp1Reg, wZR, 0, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, wZR, 0);

    //Add(tmp2Reg, wZR, 0xff, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp2Reg, wZR, 0xff);
    offset = 8;
    // bgeu
    opcode = 0x00007063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (tmp2Reg << 15);
    opcode |= (tmp1Reg << 20);
    WriteWord(opcode);

    //Add(tmp1Reg, wZR, 0xff, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(tmp1Reg, wZR, 0xff);
}

void CRV64Assembler::Add_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4EA08400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Add_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    Lw(tmp1Reg, src1AddrReg, 0);
    Lw(tmp2Reg, src2AddrReg, 0);
    //Add(tmp1Reg, tmp1Reg, tmp2Reg);
    Addw(tmp1Reg, tmp1Reg, tmp2Reg);
    Str(tmp1Reg, dstAddrReg, 0);

    Lw(tmp1Reg, src1AddrReg, 4);
    Lw(tmp2Reg, src2AddrReg, 4);
    //Add(tmp1Reg, tmp1Reg, tmp2Reg);
    Addw(tmp1Reg, tmp1Reg, tmp2Reg);
    Str(tmp1Reg, dstAddrReg, 4);

    Lw(tmp1Reg, src1AddrReg, 8);
    Lw(tmp2Reg, src2AddrReg, 8);
    //Add(tmp1Reg, tmp1Reg, tmp2Reg);
    Addw(tmp1Reg, tmp1Reg, tmp2Reg);
    Str(tmp1Reg, dstAddrReg, 8);

    Lw(tmp1Reg, src1AddrReg, 12);
    Lw(tmp2Reg, src2AddrReg, 12);
    //Add(tmp1Reg, tmp1Reg, tmp2Reg);
    Addw(tmp1Reg, tmp1Reg, tmp2Reg);
    Str(tmp1Reg, dstAddrReg, 12);
    //WriteWord(0xffffffff);
}

void CRV64Assembler::Add_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    //assert(0);
    for (int i=0; i<16; i+=2) {
        Ldrh(tmp1Reg, src1AddrReg, i);
        Ldrh(tmp2Reg, src2AddrReg, i);
        //Add(tmp1Reg, tmp1Reg, tmp2Reg);
        Addw(tmp1Reg, tmp1Reg, tmp2Reg);
        Strh(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Uqadd_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg32, REGISTER32 tmp2Reg32)
{
    auto tmp1Reg = static_cast<CRV64Assembler::REGISTER64>(tmp1Reg32);
    auto tmp2Reg = static_cast<CRV64Assembler::REGISTER64>(tmp2Reg32);
    //assert(0);
    for (int i=0; i<16; i+=4) {
        // lwu
        WriteLoadStoreOpImm(0x00006003, i, src1AddrReg, tmp1Reg);
        WriteLoadStoreOpImm(0x00006003, i, src2AddrReg, tmp2Reg);
        Add(tmp1Reg, tmp1Reg, tmp2Reg);

        Clamp_ui32(tmp1Reg, tmp2Reg);
        Str(tmp1Reg32, dstAddrReg, i);
    }
}

void CRV64Assembler::Sqadd_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg32, REGISTER32 tmp2Reg32)
{
    auto tmp1Reg = static_cast<CRV64Assembler::REGISTER64>(tmp1Reg32);
    auto tmp2Reg = static_cast<CRV64Assembler::REGISTER64>(tmp2Reg32);
    //assert(0);
    for (int i=0; i<16; i+=4) {
        // lw
        WriteLoadStoreOpImm(0x00002003, i, src1AddrReg, tmp1Reg);
        WriteLoadStoreOpImm(0x00002003, i, src2AddrReg, tmp2Reg);
        Add(tmp1Reg, tmp1Reg, tmp2Reg);

        Clamp_si32(tmp1Reg, tmp2Reg);

        Str(tmp1Reg32, dstAddrReg, i);
    }
}

void CRV64Assembler::Sqadd_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i+=2) {
        // lh
        WriteLoadStoreOpImm(0x00001003, i, src1AddrReg, tmp1Reg);
        WriteLoadStoreOpImm(0x00001003, i, src2AddrReg, tmp2Reg);
        //Ldrb(tmp1Reg, src1AddrReg, i);
        //Ldrb(tmp2Reg, src2AddrReg, i);
        //Add(tmp1Reg, tmp1Reg, tmp2Reg);
        Addw(tmp1Reg, tmp1Reg, tmp2Reg);

        Clamp_si16(tmp1Reg, tmp2Reg);

        Strh(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Uqadd_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    //assert(0);
    for (int i=0; i<16; i+=2) {
        Ldrh(tmp1Reg, src1AddrReg, i);
        Ldrh(tmp2Reg, src2AddrReg, i);
        //Add(tmp1Reg, tmp1Reg, tmp2Reg);
        Addw(tmp1Reg, tmp1Reg, tmp2Reg);

        Clamp_ui16(tmp1Reg, tmp2Reg);

        Strh(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Add_16b_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    //assert(0);
    for (int i=0; i<16; i++) {
        Ldrb(tmp1Reg, src1AddrReg, i);
        Ldrb(tmp2Reg, src2AddrReg, i);
        //Add(tmp1Reg, tmp1Reg, tmp2Reg);
        Addw(tmp1Reg, tmp1Reg, tmp2Reg);
        Strb(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Uqadd_16b_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg) {
    //assert(0);
    for (int i=0; i<16; i++) {
        Ldrb(tmp1Reg, src1AddrReg, i);
        Ldrb(tmp2Reg, src2AddrReg, i);
        //Add(tmp1Reg, tmp1Reg, tmp2Reg);
        Addw(tmp1Reg, tmp1Reg, tmp2Reg);

        Clamp_ui8(tmp1Reg, tmp2Reg);

        Strb(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Sqadd_16b_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg) {
    // 5 instructions with armv7 neon, 396 instructions in risc-c without vector instructions
    //assert(0);
    for (int i=0; i<16; i++) {
        // lb
        WriteLoadStoreOpImm(0x00000003, i, src1AddrReg, tmp1Reg);
        WriteLoadStoreOpImm(0x00000003, i, src2AddrReg, tmp2Reg);
        //Ldrb(tmp1Reg, src1AddrReg, i);
        //Ldrb(tmp2Reg, src2AddrReg, i);
        //Add(tmp1Reg, tmp1Reg, tmp2Reg);
        Addw(tmp1Reg, tmp1Reg, tmp2Reg);

        Clamp_si8(tmp1Reg, tmp2Reg);

        Strb(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Add_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E608400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Add_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E208400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::And_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E201C00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::And_16b_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i++) {
        Ldrb(tmp1Reg, src1AddrReg, i);
        Ldrb(tmp2Reg, src2AddrReg, i);
        //And(tmp1Reg, tmp1Reg, tmp2Reg);
        Andw(tmp1Reg, tmp1Reg, tmp2Reg);
        Strb(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Asr(REGISTER32 rd, REGISTER32 rn, uint8 sa)
{
    /*uint32 imms = 0x1F;
    uint32 immr = sa & 0x1F;
    WriteLogicalOpImm(0x13000000, 0, immr, imms, rn, rd);*/

    // sraiw
    WriteLogicalOpImm(0x4000501B, (sa & 0x1F), rn, rd);
    // srai
    //WriteLogicalOpImm(0x40005013, sa, rn, rd);
}

void CRV64Assembler::Asr(REGISTER64 rd, REGISTER64 rn, uint8 sa)
{
    /*uint32 imms = 0x3F;
    uint32 immr = sa & 0x3F;
    WriteLogicalOpImm(0x93400000, 0, immr, imms, rn, rd);*/

    // srai
    WriteLogicalOpImm(0x40005013, sa, rn, rd);
}

void CRV64Assembler::Asrv(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
    //WriteDataProcOpReg2(0x1AC02800, rm, rn, rd);
    // sra?
    //WriteDataProcOpReg2(0x40005033, rm, rn, rd);
    // sraw
    WriteDataProcOpReg2(0x4000503B, rm, rn, rd);
}

void CRV64Assembler::Asrv(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm)
{
    //WriteDataProcOpReg2(0x9AC02800, rm, rn, rd);
    // sra?
    //WriteDataProcOpReg2(0x40005013, rm, rn, rd);
    // sra
    WriteDataProcOpReg2(0x40005033, rm, rn, rd);
}

void CRV64Assembler::B(LABEL label)
{
    //assert(0);
    CreateBranchLabelReference(label, CONDITION_AL);
    WriteWord(0);
}

void CRV64Assembler::B_offset(uint32 offset)
{
    assert(0);
    assert((offset & 0x3) == 0);
    offset /= 4;
    assert(offset < 0x40000000);
    uint32 opcode = 0x14000000;
    opcode |= offset;
    WriteWord(opcode);
}

void CRV64Assembler::Bl(uint32 offset)
{
    assert(0);
    assert((offset & 0x3) == 0);
    offset /= 4;
    assert(offset < 0x40000000);
    uint32 opcode = 0x94000000;
    opcode |= offset;
    WriteWord(opcode);
}

void CRV64Assembler::Br(REGISTER64 rn)
{
    assert(0);
    /*uint32 opcode = 0xD61F0000;
    opcode |= (rn << 5);
    WriteWord(opcode);*/

    // jalr
    uint32 opcode = 0x00000067;
    opcode |= (xZR << 7);
    opcode |= (rn << 15);
    opcode |= (0 << 20);
    WriteWord(opcode);
}

void CRV64Assembler::BCc(CONDITION condition, LABEL label)
{
    assert(0);
    CreateBranchLabelReference(label, condition);
    WriteWord(0);
}

void CRV64Assembler::BCc(CONDITION condition, LABEL label, REGISTER32 src1Reg, REGISTER32 src2Reg)
{
    //assert(0);
    CreateBranchLabelReference(label, condition, src1Reg, src2Reg);
    WriteWord(0);
}

void CRV64Assembler::Blr(REGISTER64 rn)
{
    /*uint32 opcode = 0xD63F0000;
    opcode |= (rn << 5);
    WriteWord(opcode);*/

    // auipc
    /*uint32 opcode = 0x00000017;
    opcode |= (xRA << 7);
    opcode |= (0 << 12);
    WriteWord(opcode);*/

    // jalr
    uint32 opcode = 0x00000067;
    opcode |= (xRA << 7);
    opcode |= (rn << 15);
    opcode |= (0 << 20);
    static int count = 0;
    count++;
    WriteWord(opcode);
}

void CRV64Assembler::Cbnz(REGISTER32 rt, LABEL label)
{
    //assert(0);
    CreateCompareBranchLabelReference(label, CONDITION_NE, rt);
    WriteWord(0);
}

void CRV64Assembler::Cbnz(REGISTER64 rt, LABEL label)
{
    //assert(0);
    CreateCompareBranchLabelReference(label, CONDITION_NE, rt);
    WriteWord(0);
}

void CRV64Assembler::Cbz(REGISTER32 rt, LABEL label)
{
    //assert(0);
    CreateCompareBranchLabelReference(label, CONDITION_EQ, rt);
    WriteWord(0);
}

void CRV64Assembler::Cbz(REGISTER64 rt, LABEL label)
{
    //assert(0);
    CreateCompareBranchLabelReference(label, CONDITION_EQ, rt);
    WriteWord(0);
}

void CRV64Assembler::Clz(REGISTER32 rd, REGISTER32 rn)
{
    // clzw requires Zbb
    uint32 opcode = 0x6000101B;
    opcode |= (rd << 7);
    opcode |= (rn << 15);
    WriteWord(opcode);

}

void CRV64Assembler::Cmeq_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E208C00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Cmeq_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E608C00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Cmeq_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6EA08C00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Cmgt_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E203400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Cmgt_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E603400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Cmgt_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4EA03400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Cmltz_4s(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x4EA0A800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Sltiu(REGISTER32 rd, REGISTER32 rn, uint16 imm)
{
    // ? maybe should shift this to compare only 32 bits
    // sltiu rd, rn, imm
    uint32 opcode = 0x00003013;
    opcode |= (rd << 7);
    opcode |= (rn  << 15);
    opcode |= (imm  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::XorSltu(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
    // not equal
    // xor  rd, rn, rm
    uint32 opcode = 0x00004033;
    opcode |= (rd << 7);
    opcode |= (rn  << 15);
    opcode |= (rm  << 20);
    WriteWord(opcode);

    // sltu rd, x0, rd
    opcode = 0x00003033;
    opcode |= (rd << 7);
    opcode |= (wZR  << 15);
    opcode |= (rd  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::XorSltu(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm)
{
    // not equal
    // xor  rd, rn, rm
    uint32 opcode = 0x00004033;
    opcode |= (rd << 7);
    opcode |= (rn  << 15);
    opcode |= (rm  << 20);
    WriteWord(opcode);

    // sltu rd, x0, rd
    opcode = 0x00003033;
    opcode |= (rd << 7);
    opcode |= (wZR  << 15);
    opcode |= (rd  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Sltu(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
    // ? maybe should shift this to compare only 32 bits
    // sltu rd, rn, rm
    uint32 opcode = 0x00003033;
    opcode |= (rd << 7);
    opcode |= (rn  << 15);
    opcode |= (rm  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Slt(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
    // slt
    uint32 opcode = 0x00002033;
    opcode |= (rd << 7);
    opcode |= (rn  << 15);
    opcode |= (rm  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Cmp(REGISTER32 rn, REGISTER32 rm)
{
    assert(0);
    uint32 opcode = 0x6B000000;
    opcode |= (wZR << 0);
    opcode |= (rn  << 5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Cmp(REGISTER64 rn, REGISTER64 rm)
{
    assert(0);
    uint32 opcode = 0xEB000000;
    opcode |= (wZR << 0);
    opcode |= (rn  << 5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Cset(REGISTER32 rd, CONDITION condition)
{
    assert(0);
    uint32 opcode = 0x1A800400;
    opcode |= (rd  << 0);
    opcode |= (wZR << 5);
    opcode |= ((condition ^ 1) << 12);	//Inverting lsb inverts condition
    opcode |= (wZR << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Dup_4s(REGISTERMD rd, REGISTER32 rn)
{
    assert(0);
    uint32 opcode = 0x4E040C00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Eor_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E201C00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Eor_16b_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i++) {
        Ldrb(tmp1Reg, src1AddrReg, i);
        Ldrb(tmp2Reg, src2AddrReg, i);
        //Eor(tmp1Reg, tmp1Reg, tmp2Reg);
        Xorw(tmp1Reg, tmp1Reg, tmp2Reg);
        Strb(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Fabs_1s(REGISTERMD rd, REGISTERMD rn)
{
    /*uint32 opcode = 0x1E20C000;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);*/

    // fsgnjx.s
    uint32 opcode = 0x20002053;
    opcode |= (rd <<  7);
    opcode |= (rn << 15);
    opcode |= (rn << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fabs_4s(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x4EA0F800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Fadd_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    /*uint32 opcode = 0x1E202800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    uint16 roundingType = 0x7; // math extra info
    // fadd.s
    uint32 opcode = 0x00000053;
    opcode |= (rd <<  7);
    opcode |= (roundingType << 12);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fadd_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    assert(0);
}

void CRV64Assembler::Fadd_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E20D400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Fcmeqz_4s(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x4EA0D800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Fcmge_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E20E400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}


void CRV64Assembler::Fcmlt_1s(REGISTER32 rd, REGISTERMD rn, REGISTERMD rm)
{
    // flt.s
    uint32 opcode = 0xA0001053;
    opcode |= (rd <<  7);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fcmgt_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6EA0E400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Fcmltz_4s(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x4EA0E800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Fcmp_1s(REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x1E202000;
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Fcvtzs_1s(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    /*uint32 opcode = 0x5EA1B800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);*/

    uint16 roundingType = 0x7; // math extra info
    // fcvt.w.s
    uint32 opcode = 0xC0000053;
    opcode |= (rd <<  7);
    opcode |= (roundingType << 12);
    opcode |= (rn << 15);
    WriteWord(opcode);
    WriteWord(0xffffffff);
}

void CRV64Assembler::Fcvtws_1s(REGISTER32 rd, REGISTERMD rn)
{
    //uint16 roundingType = 0x7; // math extra info
    uint16 roundingType = 0x1; // round to zero
    // fcvt.w.s
    uint32 opcode = 0xC0000053;
    opcode |= (rd <<  7);
    opcode |= (roundingType << 12);
    opcode |= (rn << 15);
    WriteWord(opcode);
    //WriteWord(0xffffffff);
}

void CRV64Assembler::Fcvtzs_4s(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x4EA1B800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Fdiv_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    /*uint32 opcode = 0x1E201800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    uint16 roundingType = 0x7; // math extra info
    // fdiv.s
    uint32 opcode = 0x18000053;
    opcode |= (rd <<  7);
    opcode |= (roundingType << 12);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fdiv_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E20FC00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Fmov_1s(REGISTERMD rd, uint8 imm)
{
    assert(0);
    uint32 opcode = 0x1E201000;
    opcode |= (rd <<   0);
    opcode |= (imm << 13);
    WriteWord(opcode);
}

void CRV64Assembler::Fmv_1s(REGISTERMD rd, REGISTER32 rs)
{
    // fmv.s.x
    uint32 opcode = 0xF0000053;
    opcode |= (rd <<  7);
    opcode |= (rs << 15);
    WriteWord(opcode);
}

void CRV64Assembler::Fmv_x_w(REGISTER32 rd, REGISTERMD rs)
{
    // fmv.x.w
    uint32 opcode = 0xE0000053;
    opcode |= (rd <<  7);
    opcode |= (rs << 15);
    WriteWord(opcode);
}

void CRV64Assembler::Fmul_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    /*uint32 opcode = 0x1E200800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    uint16 roundingType = 0x7; // math extra info
    // fmul.s
    uint32 opcode = 0x10000053;
    opcode |= (rd <<  7);
    opcode |= (roundingType << 12);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fmul_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E20DC00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Fclass_1s(REGISTER32 rd, REGISTERMD rn)
{
    // fclass.s
    uint32 opcode = 0xE0001053;
    opcode |= (rd <<  7);
    opcode |= (rn << 15);
    WriteWord(opcode);
}

void CRV64Assembler::Flt_1s(REGISTER32 rd, REGISTERMD rn, REGISTERMD rm)
{
    // flt.s
    uint32 opcode = 0xA0001053;
    opcode |= (rd <<  7);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fle_1s(REGISTER32 rd, REGISTERMD rn, REGISTERMD rm)
{
    // fle.s
    uint32 opcode = 0xA0000053;
    opcode |= (rd <<  7);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Feq_1s(REGISTER32 rd, REGISTERMD rn, REGISTERMD rm)
{
    // feq.s
    uint32 opcode = 0xA0002053;
    opcode |= (rd <<  7);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fmax_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    /*uint32 opcode = 0x1E204800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    // fmax.s
    uint32 opcode = 0x28001053;
    opcode |= (rd <<  7);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fmax_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E20F400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Fneg_1s(REGISTERMD rd, REGISTERMD rn)
{
    /*assert(0);
    uint32 opcode = 0x1E214000;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);*/

    // fsgnjn.s
    uint32 opcode = 0x20001053;
    opcode |= (rd <<  7);
    opcode |= (rn << 15);
    opcode |= (rn << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fmin_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    /*uint32 opcode = 0x1E205800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    // fmin.s
    uint32 opcode = 0x28000053;
    opcode |= (rd <<  7);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fmin_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4EA0F400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Fsqrt_1s(REGISTERMD rd, REGISTERMD rn)
{
    /*uint32 opcode = 0x1E21C000;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);*/


    uint16 roundingType = 0x7; // math extra info
    // fsqrt.s
    uint32 opcode = 0x58000053;
    opcode |= (rd <<  7);
    opcode |= (roundingType << 12);
    opcode |= (rn << 15);
    WriteWord(opcode);
}

void CRV64Assembler::Fsub_1s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    /*uint32 opcode = 0x1E203800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    uint16 roundingType = 0x7; // math extra info
    // fsub.s
    uint32 opcode = 0x08000053;
    opcode |= (rd <<  7);
    opcode |= (roundingType << 12);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Fsub_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4EA0D400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Ins_1s(REGISTERMD rd, uint8 index1, REGISTERMD rn, uint8 index2)
{
    assert(0);
    assert(index1 < 4);
    assert(index2 < 4);
    index1 &= 0x3;
    index2 &= 0x3;
    uint32 opcode = 0x6E040400;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (index2 << 13);
    opcode |= (index1 << 19);
    WriteWord(opcode);
}

void CRV64Assembler::Ins_1d(REGISTERMD rd, uint8 index, REGISTER64 rn)
{
    assert(0);
    assert(index < 2);
    index &= 0x1;
    uint8 imm5 = (index << 4) | 0x8;
    uint32 opcode = 0x4E001C00;
    opcode |= (rd << 0);
    opcode |= (rn << 5);
    opcode |= (imm5 << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Ld1_4s(REGISTERMD rt, REGISTER64 rn)
{
    assert(0);
    uint32 opcode = 0x4C407800;
    opcode |= (rt <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Ldp_PostIdx(REGISTER64 rt, REGISTER64 rt2, REGISTER64 rn, int32 offset)
{
    /*assert((offset & 0x07) == 0);
    int32 scaledOffset = offset / 8;
    assert(scaledOffset >= -64 && scaledOffset <= 63);
    uint32 opcode = 0xA8C00000;
    opcode |= (rt  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rt2 << 10);
    opcode |= ((scaledOffset & 0x7F) << 15);
    WriteWord(opcode);*/

    {
        //int32 scaledOffset = offset / 8;
        int32 scaledOffset = offset;
        assert(scaledOffset >= -64 && scaledOffset <= 63);

        scaledOffset = 0;
        /*uint32 opcode = 0x00003003;
        opcode |= (rt  <<  7);
        opcode |= (rn  <<  15);
        opcode |= (scaledOffset << 20);
        WriteWord(opcode);*/
        Ld(rt, rn, scaledOffset);
    }
    {
        //int32 scaledOffset = offset / 8;
        int32 scaledOffset = offset;
        assert(scaledOffset >= -64 && scaledOffset <= 63);

        scaledOffset = 8;
        /*uint32 opcode = 0x00003003;
        opcode |= (rt2 <<  7);
        opcode |= (rn  <<  15);
        opcode |= (scaledOffset << 20);
        WriteWord(opcode);*/
        Ld(rt2, rn, scaledOffset);

        scaledOffset = offset;
        //add rn, rn, scaledOffset
        uint32 opcode = 0x00000013;
        opcode |= (rn  <<  7);
        opcode |= (rn  <<  15);
        opcode |= (scaledOffset << 20);
        WriteWord(opcode);
    }
}
#if 0
void CRV64Assembler::Ldr(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
    /*assert((offset & 0x03) == 0);
    uint32 scaledOffset = offset / 4;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0xB9400000, scaledOffset, rn, rt);*/

    assert((offset & 0x03) == 0);
    //uint32 scaledOffset = offset / 4;
    uint32 scaledOffset = offset;
    assert(scaledOffset < 0x1000);
    // maybe should be lwu?
    // lw
    WriteLoadStoreOpImm(0x00002003, scaledOffset, rn, rt);
}

void CRV64Assembler::Ldr(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
    /*uint32 opcode = 0xB8606800;
    opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= scaled ? (1 << 12) : 0;
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    //WriteWord(0xffffffff);
    //assert(!scaled);

    if (scaled) {
        Lsl(rm, rm, 2);
    }
    Add(rn, rn, rm);
    // maybe should be lwu?
    // lw
    WriteLoadStoreOpImm(0x00002003, 0, rn, rt);
    Sub(rn, rn, rm);
    if (scaled) {
        Lsr(rm, rm, 2);
    }
    /*uint32 opcode = 0x00002003;
    opcode |= (rt  <<  7);
    opcode |= (rn  <<  15);
    opcode |= (rm  <<  20);
    WriteWord(opcode);*/
}

void CRV64Assembler::Ldr(REGISTER64 rt, REGISTER64 rn, uint32 offset)
{
    /*assert((offset & 0x07) == 0);
    uint32 scaledOffset = offset / 8;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0xF9400000, scaledOffset, rn, rt);*/

    assert((offset & 0x07) == 0);
    //uint32 scaledOffset = offset / 8;
    uint32 scaledOffset = offset;
    assert(scaledOffset < 0x1000);
    // ld
    WriteLoadStoreOpImm(0x00003003, scaledOffset, rn, rt);
}

void CRV64Assembler::Ldr(REGISTER64 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
    /*uint32 opcode = 0xF8606800;
    opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= scaled ? (1 << 12) : 0;
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    if (scaled) {
        //assert(0);
        Lsl(rm, rm, 3);
    }
    Add(rn, rn, rm);
    // ld
    WriteLoadStoreOpImm(0x00003003, 0, rn, rt);
    Sub(rn, rn, rm);
    if (scaled) {
        Lsr(rm, rm, 3);
    }
}
#endif
void CRV64Assembler::Ldrb(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
    /*uint32 scaledOffset = offset;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0x39400000, scaledOffset, rn, rt);*/

    //uint32 scaledOffset = offset / 4;
    int32 scaledOffset = offset;
    //assert(scaledOffset < 0x1000);
    assert((scaledOffset >= -16));
    assert((scaledOffset <= 15));
    // lbu
    WriteLoadStoreOpImm(0x00004003, scaledOffset, rn, rt);
}

void CRV64Assembler::Ldrb(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
    /*uint32 opcode = 0x38606800;
    opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= scaled ? (1 << 12) : 0;
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    /*if (scaled) {
        Lsl(rm, rm, 2);
    }*/
    Add(rn, rn, rm);
    // lbu
    WriteLoadStoreOpImm(0x00004003, 0, rn, rt);
    Sub(rn, rn, rm);
    /*if (scaled) {
        Lsr(rm, rm, 2);
    }*/
}

void CRV64Assembler::Ldrh(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
    /*assert((offset & 0x01) == 0);
    uint32 scaledOffset = offset / 2;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0x79400000, scaledOffset, rn, rt);*/

    //uint32 scaledOffset = offset / 4;
    int32 scaledOffset = offset;
    //assert(scaledOffset < 0x1000);
    assert((scaledOffset >= -16));
    assert((scaledOffset <= 15));
    // lhu
    WriteLoadStoreOpImm(0x00005003, scaledOffset, rn, rt);
}

void CRV64Assembler::Ldrh(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
    /*uint32 opcode = 0x78606800;
    opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= scaled ? (1 << 12) : 0;
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    if (scaled) {
        Lsl(rm, rm, 1);
    }
    Add(rn, rn, rm);
    // lhu
    WriteLoadStoreOpImm(0x00005003, 0, rn, rt);
    Sub(rn, rn, rm);
    if (scaled) {
        Lsr(rm, rm, 1);
    }
}

void CRV64Assembler::Ldrhs(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
    /*assert((offset & 0x01) == 0);
    uint32 scaledOffset = offset / 2;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0x79400000, scaledOffset, rn, rt);*/

    //uint32 scaledOffset = offset / 4;
    int32 scaledOffset = offset;
    //assert(scaledOffset < 0x1000);
    assert((scaledOffset >= -16));
    assert((scaledOffset <= 15));
    // lh
    WriteLoadStoreOpImm(0x00001003, scaledOffset, rn, rt);
}

void CRV64Assembler::Ldr_Pc(REGISTER64 rt, uint32 offset)
{
    assert(0);
    assert((offset & 0x03) == 0);

    /*uint32 scaledOffset = offset / 4;
    assert(scaledOffset < 0x40000);
    uint32 opcode = 0x58000000;
    opcode |= (rt <<  0);
    opcode |= scaledOffset << 5;
    WriteWord(opcode);*/

    // wrong
    // auipc
    uint32 opcode = 0x00000017;
    opcode |= (rt << 7);
    opcode |= (offset << 12);
    WriteWord(opcode);
}

void CRV64Assembler::Ldr_Pc(REGISTERMD rt, const LITERAL128& literal)
{
    assert(0);
    LITERAL128REF literalRef;
    literalRef.offset = static_cast<size_t>(m_stream->Tell());
    literalRef.value = literal;
    literalRef.rt = rt;
    m_literal128Refs.push_back(literalRef);
    WriteWord(0);
}

void CRV64Assembler::Ldr_1s(REGISTERMD rt, REGISTER64 rn, uint32 offset)
{
    /*assert((offset & 0x03) == 0);
    uint32 scaledOffset = offset / 4;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0xBD400000, scaledOffset, rn, rt);*/

    assert((offset & 0x03) == 0);
    //uint32 scaledOffset = offset / 4;
    uint32 scaledOffset = offset;
    //assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0x00002007, scaledOffset, rn, rt);
}

void CRV64Assembler::Ldr_1q(REGISTERMD rt, REGISTER64 rn, uint32 offset)
{
    assert(0);
    assert((offset & 0x0F) == 0);
    uint32 scaledOffset = offset / 0x10;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0x3DC00000, scaledOffset, rn, rt);
}

void CRV64Assembler::Ldr_1q(REGISTERMD rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
    assert(0);
    uint32 opcode = 0x3CE04800;
    opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= scaled ? (1 << 12) : 0;
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Lsl(REGISTER32 rd, REGISTER32 rn, uint8 sa)
{
    /*uint32 imms = 0x1F - (sa & 0x1F);
    uint32 immr = -sa & 0x1F;
    WriteLogicalOpImm(0x53000000, 0, immr, imms, rn, rd);*/

    // slliw
    WriteLogicalOpImm(0x0000101B, (sa & 0x1F), rn, rd);
    // slli
    //WriteLogicalOpImm(0x00001013, sa, rn, rd);
}

void CRV64Assembler::Lsl(REGISTER64 rd, REGISTER64 rn, uint8 sa)
{
    /*uint32 imms = 0x3F - (sa & 0x3F);
    uint32 immr = -sa & 0x3F;
    WriteLogicalOpImm(0xD3400000, 0, immr, imms, rn, rd);*/

    // slli
    WriteLogicalOpImm(0x00001013, sa, rn, rd);
}

void CRV64Assembler::Lslv(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
    //WriteDataProcOpReg2(0x1AC02000, rm, rn, rd);
    // slliw?
    //WriteDataProcOpReg2(0x00001033, rm, rn, rd);
    // sllw
    WriteDataProcOpReg2(0x0000103B, rm, rn, rd);
}

void CRV64Assembler::Lslv(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm)
{
    //WriteDataProcOpReg2(0x9AC02000, rm, rn, rd);
    // slli?
    //WriteDataProcOpReg2(0x00001013, rm, rn, rd);
    // sll
    WriteDataProcOpReg2(0x00001033, rm, rn, rd);
}

void CRV64Assembler::Lsr(REGISTER32 rd, REGISTER32 rn, uint8 sa)
{
    /*assert(0);
    uint32 imms = 0x1F;
    uint32 immr = sa & 0x1F;
    WriteLogicalOpImm(0x53000000, 0, immr, imms, rn, rd);*/

    // srliw
    WriteLogicalOpImm(0x0000501B, (sa & 0x1F), rn, rd);
    // srli
    //WriteLogicalOpImm(0x00005013, sa, rn, rd);
}

void CRV64Assembler::Lsr(REGISTER64 rd, REGISTER64 rn, uint8 sa)
{
    /*assert(0);
    uint32 imms = 0x3F;
    uint32 immr = sa & 0x3F;
    WriteLogicalOpImm(0xD3400000, 0, immr, imms, rn, rd);*/

    // srli
    WriteLogicalOpImm(0x00005013, sa, rn, rd);
}

void CRV64Assembler::Lsrv(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
    //WriteDataProcOpReg2(0x1AC02400, rm, rn, rd);
    // srlw
    WriteDataProcOpReg2(0x0000503B, rm, rn, rd);
}

void CRV64Assembler::Lsrv(REGISTER64 rd, REGISTER64 rn, REGISTER64 rm)
{
    //WriteDataProcOpReg2(0x9AC02400, rm, rn, rd);
    // srl
    WriteDataProcOpReg2(0x00005033, rm, rn, rd);
}

void CRV64Assembler::Mov(REGISTER32 rd, REGISTER32 rm)
{
    /*uint32 opcode = 0x2A0003E0;
    opcode |= (rd <<  0);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    //addwi rd, rm, 0
    uint32 opcode = 0x0000001B;
    opcode |= (rd  <<  7);
    opcode |= (rm  <<  15);
    opcode |= (0 << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Mov(REGISTER64 rd, REGISTER64 rm)
{
    /*uint32 opcode = 0xAA0003E0;
    opcode |= (rd <<  0);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    //addi rd, rm, 0
    uint32 opcode = 0x00000013;
    opcode |= (rd  <<  7);
    opcode |= (rm  <<  15);
    opcode |= (0 << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Mov(REGISTERMD rd, REGISTERMD rn)
{
    Orr_16b(rd, rn, rn);
}

void CRV64Assembler::Mov_Sp(REGISTER64 rd, REGISTER64 rn)
{
    /*uint32 opcode = 0x91000000;
    opcode |= (rd << 0);
    opcode |= (rn << 5);
    WriteWord(opcode);*/

    //add rd, rn, 0
    uint32 opcode = 0x00000013;
    opcode |= (rd  <<  7);
    opcode |= (rn  <<  15);
    opcode |= (0 << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Slli(CRV64Assembler::REGISTER32 rd, CRV64Assembler::REGISTER32 rn, uint32 rm)
{
    // slli
    //uint32 opcode = 0x00001013;
    // slliw
    uint32 opcode = 0x0000101b;
    opcode |= (rd  <<  7);
    opcode |= (rn  <<  15);
    opcode |= (rm  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Li(CRV64Assembler::REGISTER32 registerId, uint32 constant)
{
    if ((constant & 0x000007FF) == constant) {
        Addiw(registerId, wZR, constant);
    /*} else if (((int32)constant)>=-2048 && ((int32)constant)<=-1) {
        // Need to clear the upper 32 bits?
        Addiw(registerId, wZR, constant);*/
    } else if ((constant & 0xFFFFF000) == constant) {
        Lui(static_cast<CRV64Assembler::REGISTER64>(registerId), constant);
    } else if (constant & 0x800) {
        // 12th bit set
        int32 signedLower12 = SIGN_EXTEND_12(constant);
        Lui(static_cast<CRV64Assembler::REGISTER64>(registerId), ((constant-signedLower12) & 0xfffff000));
        Addiw(registerId, registerId, signedLower12);
    } else {
        Lui(static_cast<CRV64Assembler::REGISTER64>(registerId), (constant & 0xfffff000));
        Addiw(registerId, registerId, (constant & 0xFFF));
    }
}

void CRV64Assembler::Li(CRV64Assembler::REGISTER64 registerId, uint64 constant)
{
    if ((constant & 0x000007FF) == constant) {
        Addi(registerId, xZR, constant);
    } else if (((int32)constant)>=-2048 && ((int32)constant)<=-1) {
        Addi(registerId, xZR, constant);
    } else if ((constant & 0xFFFFF000) == constant) {
        Lui(static_cast<CRV64Assembler::REGISTER64>(registerId), constant);
    } else if (((constant & 0xFFFFFFFF) == constant) && (constant & 0x800)) {
        // 12th bit set
        int32 signedLower12 = SIGN_EXTEND_12(constant);
        Lui(static_cast<CRV64Assembler::REGISTER64>(registerId), ((constant-signedLower12) & 0xfffff000));
        Addi(registerId, registerId, signedLower12);
    } else if ((constant & 0xFFFFFFFF) == constant) {
        Lui(static_cast<CRV64Assembler::REGISTER64>(registerId), (constant & 0xfffff000));
        Addi(registerId, registerId, (constant & 0xFFF));
    } else if ((constant & 0xFFFFFFFF00000000) == constant) {
        // Upper 32 bits
        uint32 constantUpper32 = constant >> 32;
        int32 signedLower12 = SIGN_EXTEND_12(constantUpper32);
        int32 signedUpper20 = (constantUpper32 - signedLower12) & 0xFFFFF000;
        Lui(registerId, signedUpper20);
        Addi(registerId, registerId, signedLower12);
        Slli(registerId, registerId, 31);
        Slli(registerId, registerId, 1);
    } else {
        // Upper 32 bits
        uint32 constantUpper32 = constant >> 32;
        int32 signedLower12 = SIGN_EXTEND_12(constantUpper32);
        int32 signedUpper20 = (constantUpper32 - signedLower12) & 0xFFFFF000;
        Lui(registerId, signedUpper20);
        Addi(registerId, registerId, signedLower12);
        // Lower 32 bits
        Slli(registerId, registerId, 8);
        Addi(registerId, registerId, ((constant>>24) & 0xff));
        Slli(registerId, registerId, 8);
        Addi(registerId, registerId, ((constant>>16) & 0xff));
        Slli(registerId, registerId, 8);
        Addi(registerId, registerId, ((constant>>8) & 0xff));
        Slli(registerId, registerId, 8);
        Addi(registerId, registerId, (constant & 0xff));
    }
}

void CRV64Assembler::Movn(REGISTER32 rd, uint16 imm, uint8 pos)
{
    assert(0);
    assert(pos < 2);
    WriteMoveWideOpImm(0x12800000, pos, imm, rd);
}

void CRV64Assembler::Movk(REGISTER32 rd, uint16 imm, uint8 pos)
{
    assert(0);
    assert(pos < 2);
    WriteMoveWideOpImm(0x72800000, pos, imm, rd);
}

void CRV64Assembler::Movk(REGISTER64 rd, uint16 imm, uint8 pos)
{
    assert(0);
    assert(pos < 4);
    WriteMoveWideOpImm(0xF2800000, pos, imm, rd);
}

void CRV64Assembler::Movz(REGISTER32 rd, uint16 imm, uint8 pos)
{
    assert(0);
    assert(pos < 2);
    WriteMoveWideOpImm(0x52800000, pos, imm, rd);
}

void CRV64Assembler::Movz(REGISTER64 rd, uint16 imm, uint8 pos)
{
    assert(0);
    assert(pos < 4);
    WriteMoveWideOpImm(0xD2800000, pos, imm, rd);
}

void CRV64Assembler::Msub(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm, REGISTER32 ra)
{
    // MSUB rd, rn, rm, ra rd = ra − rn × rm

    /*uint32 opcode = 0x1B008000;
    opcode |= (rd << 0);
    opcode |= (rn << 5);
    opcode |= (ra << 10);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    // mul
    //uint32 opcode = 0x02000033;
    // mulw
    uint32 opcode = 0x0200003B;
    // mulh
    //uint32 opcode = 0x02001033;
    // mulhsu
    //uint32 opcode = 0x02002033;
    // mulhu
    //uint32 opcode = 0x02003033;
    opcode |= (rd  <<  7);
    opcode |= (rn  << 15);
    opcode |= (rm  << 20);
    WriteWord(opcode);

    // sub
    //opcode = 0x40000033;
    // subw
    opcode = 0x4000003B;
    opcode |= (rd <<  7);
    opcode |= (ra << 15);
    opcode |= (rd << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Mvn(REGISTER32 rd, REGISTER32 rm)
{
    /*uint32 opcode = 0x2A200000;
    opcode |= (rd  << 0);
    opcode |= (wZR << 5);
    opcode |= (rm  << 16);
    WriteWord(opcode);*/

    Mov(rd, rm);

    //Xori(rd, rd, -1);
    // xori
    uint32 opcode = 0x00004013;
    opcode |= (rd <<  7);
    opcode |= (rd << 15);
    opcode |= (-1 << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Mvn_16b(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x6E205800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Mvn_16b_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER32 tmp1Reg)
{
    for (int i=0; i<16; i++) {
        Ldrb(tmp1Reg, src1AddrReg, i);
        Mvn(tmp1Reg, tmp1Reg);
        Strb(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Orn_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4EE01C00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Orr_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4EA01C00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Orr_16b_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i++) {
        Ldrb(tmp1Reg, src1AddrReg, i);
        Ldrb(tmp2Reg, src2AddrReg, i);
        //Orr(tmp1Reg, tmp1Reg, tmp2Reg);
        Orw(tmp1Reg, tmp1Reg, tmp2Reg);
        Strb(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Ret(REGISTER64 rn)
{
    /*uint32 opcode = 0xD65F0000;
    opcode |= (rn << 5);
    WriteWord(opcode);*/

    // jalr
    uint32 opcode = 0x00000067;
    opcode |= (xZR << 7);
    opcode |= (rn << 15);
    opcode |= (0 << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Scvtf_1s(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    /*uint32 opcode = 0x5E21D800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);*/

    uint16 roundingType = 0x7; // math extra info
    // fcvt.s.w
    uint32 opcode = 0xD0000053;
    opcode |= (rd <<  7);
    opcode |= (roundingType << 12);
    opcode |= (rn << 15);
    WriteWord(opcode);
    WriteWord(0xffffffff);
}

void CRV64Assembler::Fcvtsw_1s(REGISTERMD rd, REGISTER32 rn)
{
    uint16 roundingType = 0x7; // math extra info
    // fcvt.s.w
    uint32 opcode = 0xD0000053;
    opcode |= (rd <<  7);
    opcode |= (roundingType << 12);
    opcode |= (rn << 15);
    WriteWord(opcode);
}

void CRV64Assembler::Scvtf_4s(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x4E21D800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Sdiv(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
    /*uint32 opcode = 0x1AC00C00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    // div
    //uint32 opcode = 0x02004033;
    // divw
    uint32 opcode = 0x0200403B;
    opcode |= (rd  <<  7);
    opcode |= (rn  << 15);
    opcode |= (rm  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Shl_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, uint8 shmt, REGISTER32 tmp1Reg) {
    for (int i=0; i<16; i+=4) {
        Lw(tmp1Reg, src1AddrReg, i);
        Lsl(tmp1Reg, tmp1Reg, shmt & 0x1f);
        Str(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Shl_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, uint8 shmt, REGISTER32 tmp1Reg) {
    for (int i=0; i<16; i+=2) {
        // should be signed or unsigned?
        // lh
        WriteLoadStoreOpImm(0x00001003, i, src1AddrReg, tmp1Reg);
        Lsl(tmp1Reg, tmp1Reg, shmt & 0xf);
        Strh(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Shl_4s(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
    assert(0);
    uint8 immhb = (sa & 0x1F) + 32;
    uint32 opcode = 0x4F005400;
    opcode |= (rd    <<  0);
    opcode |= (rn    <<  5);
    opcode |= (immhb << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Shl_8h(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
    assert(0);
    uint8 immhb = (sa & 0xF) + 16;
    uint32 opcode = 0x4F005400;
    opcode |= (rd    <<  0);
    opcode |= (rn    <<  5);
    opcode |= (immhb << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Smin_1s(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
    uint16 offset = 2*4;
    // bge
    uint32 opcode = 0x00005063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (rm << 15);
    opcode |= (rn << 20);
    WriteWord(opcode);

    //Add(rd, rm, 0, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(rd, rm, 0);
}

void CRV64Assembler::Umin_1s(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
    uint16 offset = 2*4;
    // bgeu
    uint32 opcode = 0x00007063;
    opcode |= ((offset & 0x1E) >> 1) << 8;
    opcode |= ((offset & 0x800) >> 11) << 7;
    opcode |= ((offset & 0x1000) >> 12) << 31;
    opcode |= ((offset & 0x7E0) >> 5) << 25;
    opcode |= (rm << 15);
    opcode |= (rn << 20);
    WriteWord(opcode);

    //Add(rd, rm, 0, ADDSUB_IMM_SHIFT_LSL0);
    Addiw(rd, rm, 0);
}

void CRV64Assembler::Smax_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4EA06400;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Smax_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E606400;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Smin_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4EA06C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Smin_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E606C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}



void CRV64Assembler::Smax_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i+=4) {
        Lw(tmp1Reg, src1AddrReg, i);
        Lw(tmp2Reg, src2AddrReg, i);

        uint16 offset = 2*4;
        // bge
        uint32 opcode = 0x00005063;
        opcode |= ((offset & 0x1E) >> 1) << 8;
        opcode |= ((offset & 0x800) >> 11) << 7;
        opcode |= ((offset & 0x1000) >> 12) << 31;
        opcode |= ((offset & 0x7E0) >> 5) << 25;
        opcode |= (tmp1Reg << 15);
        opcode |= (tmp2Reg << 20);
        WriteWord(opcode);

        //Add(tmp1Reg, tmp2Reg, 0, ADDSUB_IMM_SHIFT_LSL0);
        Addiw(tmp1Reg, tmp2Reg, 0);

        Str(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Smax_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i+=2) {
        // lh
        WriteLoadStoreOpImm(0x00001003, i, src1AddrReg, tmp1Reg);
        WriteLoadStoreOpImm(0x00001003, i, src2AddrReg, tmp2Reg);

        uint16 offset = 2*4;
        // bge
        uint32 opcode = 0x00005063;
        opcode |= ((offset & 0x1E) >> 1) << 8;
        opcode |= ((offset & 0x800) >> 11) << 7;
        opcode |= ((offset & 0x1000) >> 12) << 31;
        opcode |= ((offset & 0x7E0) >> 5) << 25;
        opcode |= (tmp1Reg << 15);
        opcode |= (tmp2Reg << 20);
        WriteWord(opcode);

        //Add(tmp1Reg, tmp2Reg, 0, ADDSUB_IMM_SHIFT_LSL0);
        Addiw(tmp1Reg, tmp2Reg, 0);

        Strh(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Smin_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i+=4) {
        Lw(tmp1Reg, src1AddrReg, i);
        Lw(tmp2Reg, src2AddrReg, i);

        uint16 offset = 2*4;
        // bge
        uint32 opcode = 0x00005063;
        opcode |= ((offset & 0x1E) >> 1) << 8;
        opcode |= ((offset & 0x800) >> 11) << 7;
        opcode |= ((offset & 0x1000) >> 12) << 31;
        opcode |= ((offset & 0x7E0) >> 5) << 25;
        opcode |= (tmp2Reg << 15);
        opcode |= (tmp1Reg << 20);
        WriteWord(opcode);

        //Add(tmp1Reg, tmp2Reg, 0, ADDSUB_IMM_SHIFT_LSL0);
        Addiw(tmp1Reg, tmp2Reg, 0);

        Str(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Smin_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i+=2) {
        // lh
        WriteLoadStoreOpImm(0x00001003, i, src1AddrReg, tmp1Reg);
        WriteLoadStoreOpImm(0x00001003, i, src2AddrReg, tmp2Reg);

        uint16 offset = 2*4;
        // bge
        uint32 opcode = 0x00005063;
        opcode |= ((offset & 0x1E) >> 1) << 8;
        opcode |= ((offset & 0x800) >> 11) << 7;
        opcode |= ((offset & 0x1000) >> 12) << 31;
        opcode |= ((offset & 0x7E0) >> 5) << 25;
        opcode |= (tmp2Reg << 15);
        opcode |= (tmp1Reg << 20);
        WriteWord(opcode);

        //Add(tmp1Reg, tmp2Reg, 0, ADDSUB_IMM_SHIFT_LSL0);
        Addiw(tmp1Reg, tmp2Reg, 0);

        Strh(tmp1Reg, dstAddrReg, i);
    }
}



void CRV64Assembler::Smull(REGISTER64 rd, REGISTER32 rn, REGISTER32 rm) {
    // mul
    uint32 opcode = 0x02000033;
    // mulw
    //uint32 opcode = 0x0200003B;
    // mulh
    //uint32 opcode = 0x02001033;
    // mulhsu
    //uint32 opcode = 0x02002033;
    // mulhu
    //uint32 opcode = 0x02003033;
    opcode |= (rd  <<  7);
    opcode |= (rn  << 15);
    opcode |= (rm  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Smull(REGISTER64 rd, REGISTER32 rn, REGISTER32 rm, REGISTER64 rt1, REGISTER64 rt2) {
    Addiw(static_cast<REGISTER32>(rt1), rn, 0);
    Addiw(static_cast<REGISTER32>(rt2), rm, 0);

    // mul
    uint32 opcode = 0x02000033;
    // mulw
    //uint32 opcode = 0x0200003B;
    // mulh
    //uint32 opcode = 0x02001033;
    // mulhsu
    //uint32 opcode = 0x02002033;
    // mulhu
    //uint32 opcode = 0x02003033;
    opcode |= (rd  <<  7);
    opcode |= (rt1 << 15);
    opcode |= (rt2 << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Sshr_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, uint8 shmt, REGISTER32 tmp1Reg) {
    for (int i=0; i<16; i+=4) {
        Lw(tmp1Reg, src1AddrReg, i);
        Asr(tmp1Reg, tmp1Reg, shmt & 0x1f);
        Str(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Sshr_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, uint8 shmt, REGISTER32 tmp1Reg) {
    for (int i=0; i<16; i+=2) {
        // should be signed or unsigned?
        // lh
        WriteLoadStoreOpImm(0x00001003, i, src1AddrReg, tmp1Reg);
        Asr(tmp1Reg, tmp1Reg, shmt & 0xf);
        Strh(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Sshr_4s(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
    assert(0);
    uint8 immhb = (32 * 2) - (sa & 0x1F);
    uint32 opcode = 0x4F000400;
    opcode |= (rd    <<  0);
    opcode |= (rn    <<  5);
    opcode |= (immhb << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Sshr_8h(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
    assert(0);
    uint8 immhb = (16 * 2) - (sa & 0xF);
    uint32 opcode = 0x4F000400;
    opcode |= (rd    <<  0);
    opcode |= (rn    <<  5);
    opcode |= (immhb << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Sqadd_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4EA00C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Sqadd_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E600C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Sqadd_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E200C00;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Sqsub_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4EA02C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Sqsub_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E602C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::St1_4s(REGISTERMD rt, REGISTER64 rn)
{
    assert(0);
    uint32 opcode = 0x4C007800;
    opcode |= (rt <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Stp(REGISTER32 rt, REGISTER32 rt2, REGISTER64 rn, int32 offset)
{
    /*assert((offset & 0x03) == 0);
    int32 scaledOffset = offset / 4;
    assert(scaledOffset >= -64 && scaledOffset <= 63);
    uint32 opcode = 0x29000000;
    opcode |= (rt  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rt2 << 10);
    opcode |= ((scaledOffset & 0x7F) << 15);
    WriteWord(opcode);*/

    assert((offset & 0x03) == 0);
    //int32 scaledOffset = offset / 4;
    int32 scaledOffset = offset;
    assert(scaledOffset >= -64 && scaledOffset <= 63);

    uint32 opcode = 0x00002023;
    opcode |= (rt  <<  20);
    opcode |= (rn  <<  15);
    opcode |= ((scaledOffset & 0x1F) << 7);
    opcode |= ((scaledOffset & 0xFE0) << 20);
    WriteWord(opcode);

    scaledOffset += 4;
    opcode = 0x00002023;
    opcode |= (rt2 <<  20);
    opcode |= (rn  <<  15);
    opcode |= ((scaledOffset & 0x1F) << 7);
    opcode |= ((scaledOffset & 0xFE0) << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Stp_PreIdx(REGISTER64 rt, REGISTER64 rt2, REGISTER64 rn, int32 offset)
{
    /*assert((offset & 0x07) == 0);
    int32 scaledOffset = offset / 8;
    assert(scaledOffset >= -64 && scaledOffset <= 63);
    uint32 opcode = 0xA9800000;
    opcode |= (rt  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rt2 << 10);
    opcode |= ((scaledOffset & 0x7F) << 15);
    WriteWord(opcode);*/

    assert((offset & 0x07) == 0);
    {
        //int32 scaledOffset = offset / 8;
        int32 scaledOffset = offset;
        assert(scaledOffset >= -64 && scaledOffset <= 63);

        //add rn, rn, scaledOffset
        uint32 opcode = 0x00000013;
        opcode |= (rn  <<  7);
        opcode |= (rn  <<  15);
        opcode |= (scaledOffset << 20);
        WriteWord(opcode);

        scaledOffset = 0;
        opcode = 0x00003023;
        opcode |= (rt  <<  20);
        opcode |= (rn  <<  15);
        opcode |= ((scaledOffset & 0x1F) << 7);
        opcode |= ((scaledOffset & 0xFE0) << 20);
        WriteWord(opcode);
    }
    {
        //int32 scaledOffset = offset / 8;
        int32 scaledOffset = offset;
        assert(scaledOffset >= -64 && scaledOffset <= 63);

        scaledOffset = 8;
        uint32 opcode = 0x00003023;
        opcode |= (rt2 <<  20);
        opcode |= (rn  <<  15);
        opcode |= ((scaledOffset & 0x1F) << 7);
        opcode |= ((scaledOffset & 0xFE0) << 20);
        WriteWord(opcode);
    }
}

void CRV64Assembler::Str(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
    /*assert((offset & 0x03) == 0);
    uint32 scaledOffset = offset / 4;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0xB9000000, scaledOffset, rn, rt);*/

    assert((offset & 0x03) == 0);
    //uint32 scaledOffset = offset / 4;
    uint32 scaledOffset = offset;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0x00002023, scaledOffset, rn, rt);
}

void CRV64Assembler::Str(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
    /*uint32 opcode = 0xB8206800;
    opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= scaled ? (1 << 12) : 0;
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    //assert(!scaled);

    if (scaled) {
        Lsl(rm, rm, 2);
    }
    Add(rn, rn, rm);
    WriteLoadStoreOpImm(0x00002023, 0, rn, rt);
    Sub(rn, rn, rm);
    if (scaled) {
        Lsr(rm, rm, 2);
    }

    /*uint32 opcode = 0x00002023;
    opcode |= (rt << 7);
    opcode |= (rn << 15);
    opcode |= (rm << 20);
    WriteWord(opcode);*/
}

void CRV64Assembler::Str(REGISTER64 rt, REGISTER64 rn, uint32 offset)
{
    /*assert((offset & 0x07) == 0);
    uint32 scaledOffset = offset / 8;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0xF9000000, scaledOffset, rn, rt);*/
    assert((offset & 0x07) == 0);
    //uint32 scaledOffset = offset / 8;
    uint32 scaledOffset = offset;
    assert(scaledOffset < 0x1000);
    // sd
    WriteLoadStoreOpImm(0x00003023, scaledOffset, rn, rt);
}

void CRV64Assembler::Str(REGISTER64 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
    /*uint32 opcode = 0xF8206800;
    opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= scaled ? (1 << 12) : 0;
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    if (scaled) {
        //assert(0);
        Lsl(rm, rm, 3);
    }
    Add(rn, rn, rm);
    WriteLoadStoreOpImm(0x00003023, 0, rn, rt);
    Sub(rn, rn, rm);
    if (scaled) {
        Lsr(rm, rm, 3);
    }
}

void CRV64Assembler::Strb(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
    /*uint32 scaledOffset = offset;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0x39000000, scaledOffset, rn, rt);*/

    //uint32 scaledOffset = offset / 4;
    int32 scaledOffset = offset;
    //assert(scaledOffset < 0x1000);
    assert((scaledOffset >= -16));
    assert((scaledOffset <= 15));
    // sb
    WriteLoadStoreOpImm(0x00000023, scaledOffset, rn, rt);
}

void CRV64Assembler::Strb(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
    /*uint32 opcode = 0x38206800;
    opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= scaled ? (1 << 12) : 0;
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    /*if (scaled) {
        Lsl(rm, rm, 2);
    }*/
    Add(rn, rn, rm);
    // sb
    WriteLoadStoreOpImm(0x00000023, 0, rn, rt);
    Sub(rn, rn, rm);
    /*if (scaled) {
        //Lsr(rm, rm, 2);
    }*/
}

void CRV64Assembler::Strh(REGISTER32 rt, REGISTER64 rn, uint32 offset)
{
    /*assert((offset & 0x01) == 0);
    uint32 scaledOffset = offset / 2;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0x79000000, scaledOffset, rn, rt);*/

    //uint32 scaledOffset = offset / 4;
    int32 scaledOffset = offset;
    //assert(scaledOffset < 0x1000);
    assert((scaledOffset >= -16));
    assert((scaledOffset <= 15));
    //assert((scaledOffset >= -2048));
    //assert((scaledOffset <= 2047));
    // sh
    WriteLoadStoreOpImm(0x00001023, scaledOffset, rn, rt);
}

void CRV64Assembler::Strh(REGISTER32 rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
    /*uint32 opcode = 0x78206800;
    opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= scaled ? (1 << 12) : 0;
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    if (scaled) {
        Lsl(rm, rm, 1);
    }
    Add(rn, rn, rm);
    // sh
    WriteLoadStoreOpImm(0x00001023, 0, rn, rt);
    Sub(rn, rn, rm);
    if (scaled) {
        Lsr(rm, rm, 1);
    }
}

void CRV64Assembler::Str_1s(REGISTERMD rt, REGISTER64 rn, uint32 offset)
{
    /*assert((offset & 0x03) == 0);
    uint32 scaledOffset = offset / 4;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0xBD000000, scaledOffset, rn, rt);*/

    assert((offset & 0x03) == 0);
    //uint32 scaledOffset = offset / 4;
    uint32 scaledOffset = offset;
    //assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0x00002027, scaledOffset, rn, rt);
}

void CRV64Assembler::Str_1q(REGISTERMD rt, REGISTER64 rn, uint32 offset)
{
    assert(0);
    assert((offset & 0x0F) == 0);
    uint32 scaledOffset = offset / 0x10;
    assert(scaledOffset < 0x1000);
    WriteLoadStoreOpImm(0x3D800000, scaledOffset, rn, rt);
}

void CRV64Assembler::Str_1q(REGISTERMD rt, REGISTER64 rn, REGISTER64 rm, bool scaled)
{
    assert(0);
    uint32 opcode = 0x3CA04800;
    opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= scaled ? (1 << 12) : 0;
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Sub_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i+=4) {
        Lw(tmp1Reg, src1AddrReg, i);
        Lw(tmp2Reg, src2AddrReg, i);
        //Sub(tmp1Reg, tmp1Reg, tmp2Reg);
        Subw(tmp1Reg, tmp1Reg, tmp2Reg);
        Str(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Uqsub_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg32, REGISTER32 tmp2Reg32)
{
    auto tmp1Reg = static_cast<CRV64Assembler::REGISTER64>(tmp1Reg32);
    auto tmp2Reg = static_cast<CRV64Assembler::REGISTER64>(tmp2Reg32);
    for (int i=0; i<16; i+=4) {
        // lwu
        WriteLoadStoreOpImm(0x00006003, i, src1AddrReg, tmp1Reg);
        WriteLoadStoreOpImm(0x00006003, i, src2AddrReg, tmp2Reg);
        Sub(tmp1Reg, tmp1Reg, tmp2Reg);
        Clamp_ui32(tmp1Reg, tmp2Reg);
        Str(tmp1Reg32, dstAddrReg, i);
    }
}

void CRV64Assembler::Sqsub_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg32, REGISTER32 tmp2Reg32)
{
    auto tmp1Reg = static_cast<CRV64Assembler::REGISTER64>(tmp1Reg32);
    auto tmp2Reg = static_cast<CRV64Assembler::REGISTER64>(tmp2Reg32);
    for (int i=0; i<16; i+=4) {
        // lw
        WriteLoadStoreOpImm(0x00002003, i, src1AddrReg, tmp1Reg);
        WriteLoadStoreOpImm(0x00002003, i, src2AddrReg, tmp2Reg);
        Sub(tmp1Reg, tmp1Reg, tmp2Reg);
        Clamp_si32(tmp1Reg, tmp2Reg);
        Str(tmp1Reg32, dstAddrReg, i);
    }
}

void CRV64Assembler::Sub_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i+=2) {
        Ldrh(tmp1Reg, src1AddrReg, i);
        Ldrh(tmp2Reg, src2AddrReg, i);
        //Sub(tmp1Reg, tmp1Reg, tmp2Reg);
        Subw(tmp1Reg, tmp1Reg, tmp2Reg);
        Strh(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Sqsub_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i+=2) {
        // lh
        WriteLoadStoreOpImm(0x00001003, i, src1AddrReg, tmp1Reg);
        WriteLoadStoreOpImm(0x00001003, i, src2AddrReg, tmp2Reg);
        //Sub(tmp1Reg, tmp1Reg, tmp2Reg);
        Subw(tmp1Reg, tmp1Reg, tmp2Reg);
        Clamp_si16(tmp1Reg, tmp2Reg);
        Strh(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Uqsub_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i+=2) {
        Ldrh(tmp1Reg, src1AddrReg, i);
        Ldrh(tmp2Reg, src2AddrReg, i);
        //Sub(tmp1Reg, tmp1Reg, tmp2Reg);
        Subw(tmp1Reg, tmp1Reg, tmp2Reg);
        Clamp_ui16(tmp1Reg, tmp2Reg);
        Strh(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Sub_16b_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg)
{
    for (int i=0; i<16; i++) {
        Ldrb(tmp1Reg, src1AddrReg, i);
        Ldrb(tmp2Reg, src2AddrReg, i);
        //Sub(tmp1Reg, tmp1Reg, tmp2Reg);
        Subw(tmp1Reg, tmp1Reg, tmp2Reg);
        Strb(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Uqsub_16b_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, REGISTER64 src2AddrReg, REGISTER32 tmp1Reg, REGISTER32 tmp2Reg) {
    for (int i=0; i<16; i++) {
        Ldrb(tmp1Reg, src1AddrReg, i);
        Ldrb(tmp2Reg, src2AddrReg, i);
        //Sub(tmp1Reg, tmp1Reg, tmp2Reg);
        Subw(tmp1Reg, tmp1Reg, tmp2Reg);
        Clamp_ui8(tmp1Reg, tmp2Reg);
        Strb(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Sub_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6EA08400;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Sub_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E608400;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Sub_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E208400;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Tbl(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E002000;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Tst(REGISTER32 rn, REGISTER32 rm)
{
    assert(0);
    uint32 opcode = 0x6A000000;
    opcode |= (wZR << 0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Tst(REGISTER64 rn, REGISTER64 rm)
{
    assert(0);
    uint32 opcode = 0xEA000000;
    opcode |= (wZR << 0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Uaddlv_16b(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x6E303800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    WriteWord(opcode);
}

void CRV64Assembler::Udiv(REGISTER32 rd, REGISTER32 rn, REGISTER32 rm)
{
    /*uint32 opcode = 0x1AC00800;
    opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);*/

    // div
    //uint32 opcode = 0x02004033;
    // divw
    //uint32 opcode = 0x0200403B;
    // divu
    //uint32 opcode = 0x02005033;
    // divuw
    uint32 opcode = 0x0200503B;
    opcode |= (rd  <<  7);
    opcode |= (rn  << 15);
    opcode |= (rm  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Umin_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6EA06C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Umov_1s(REGISTER32 rd, REGISTERMD rn, uint8 index)
{
    assert(0);
    assert(index < 4);
    uint8 imm5 = 0x4 | ((index & 3) << 3);
    uint32 opcode = 0x0E003C00;
    opcode |= (rd   <<  0);
    opcode |= (rn   <<  5);
    opcode |= (imm5 << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Umull(REGISTER64 rd, REGISTER32 rn, REGISTER32 rm)
{
    // mul
    uint32 opcode = 0x02000033;
    // mulw
    //uint32 opcode = 0x0200003B;
    // mulh
    //uint32 opcode = 0x02001033;
    // mulhsu
    //uint32 opcode = 0x02002033;
    // mulhu
    //uint32 opcode = 0x02003033;
    opcode |= (rd  <<  7);
    opcode |= (rn  << 15);
    opcode |= (rm  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Umull(REGISTER64 rd, REGISTER32 rn, REGISTER32 rm, REGISTER64 rt1, REGISTER64 rt2)
{
    Lsl(rt1, static_cast<REGISTER64>(rn), 32);
    //Lsr(rt1, rt1, 32);
    Lsl(rt2, static_cast<REGISTER64>(rm), 32);
    //Lsr(rt2, rt2, 32);

    // mul
    //uint32 opcode = 0x02000033;
    // mulw
    //uint32 opcode = 0x0200003B;
    // mulh
    //uint32 opcode = 0x02001033;
    // mulhsu
    //uint32 opcode = 0x02002033;
    // mulhu
    uint32 opcode = 0x02003033;
    opcode |= (rd  <<  7);
    opcode |= (rt1  << 15);
    opcode |= (rt2  << 20);
    WriteWord(opcode);
}

void CRV64Assembler::Uqadd_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6EA00C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Uqadd_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E600C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Uqadd_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E200C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Uqsub_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6EA02C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Uqsub_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E602C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Uqsub_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x6E202C00;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Ushr_4s_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, uint8 shmt, REGISTER32 tmp1Reg) {
    for (int i=0; i<16; i+=4) {
        Lw(tmp1Reg, src1AddrReg, i);
        Lsr(tmp1Reg, tmp1Reg, shmt & 0x1f);
        Str(tmp1Reg, dstAddrReg, i);
    }
}

void CRV64Assembler::Ushr_8h_Mem(REGISTER64 dstAddrReg, REGISTER64 src1AddrReg, uint8 shmt, REGISTER32 tmp1Reg) {
    for (int i=0; i<16; i+=2) {
        // should be signed or unsigned?
        // lhu
        WriteLoadStoreOpImm(0x00005003, i, src1AddrReg, tmp1Reg);
        Lsr(tmp1Reg, tmp1Reg, shmt & 0xf);
        Strh(tmp1Reg, dstAddrReg, i);
    }
    //WriteWord(0xffffffff);
}

void CRV64Assembler::Ushr_4s(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
    assert(0);
    uint8 immhb = (32 * 2) - (sa & 0x1F);
    uint32 opcode = 0x6F000400;
    opcode |= (rd    <<  0);
    opcode |= (rn    <<  5);
    opcode |= (immhb << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Ushr_8h(REGISTERMD rd, REGISTERMD rn, uint8 sa)
{
    assert(0);
    uint8 immhb = (16 * 2) - (sa & 0xF);
    uint32 opcode = 0x6F000400;
    opcode |= (rd    <<  0);
    opcode |= (rn    <<  5);
    opcode |= (immhb << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Xtn1_4h(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x0E612800;
    opcode |= (rd << 0);
    opcode |= (rn << 5);
    WriteWord(opcode);
}

void CRV64Assembler::Xtn1_8b(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x0E212800;
    opcode |= (rd << 0);
    opcode |= (rn << 5);
    WriteWord(opcode);
}

void CRV64Assembler::Xtn2_8h(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x4E612800;
    opcode |= (rd << 0);
    opcode |= (rn << 5);
    WriteWord(opcode);
}

void CRV64Assembler::Xtn2_16b(REGISTERMD rd, REGISTERMD rn)
{
    assert(0);
    uint32 opcode = 0x4E212800;
    opcode |= (rd << 0);
    opcode |= (rn << 5);
    WriteWord(opcode);
}

void CRV64Assembler::Zip1_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E803800;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Zip1_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E403800;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Zip1_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E003800;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Zip2_4s(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E807800;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Zip2_8h(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E407800;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

void CRV64Assembler::Zip2_16b(REGISTERMD rd, REGISTERMD rn, REGISTERMD rm)
{
    assert(0);
    uint32 opcode = 0x4E007800;
    opcode |= (rd  <<  0);
    opcode |= (rn  <<  5);
    opcode |= (rm  << 16);
    WriteWord(opcode);
}

/*
 * Pseudo Instructions
 */

void CRV64Assembler::Lwu(REGISTER32 rd, REGISTER64 rs1, int32 imm) {
    Lwu(static_cast<REGISTER64>(rd), rs1, imm);
}

void CRV64Assembler::Lw(REGISTER32 rd, REGISTER64 rs1, int32 imm) {
    Lw(static_cast<REGISTER64>(rd), rs1, imm);
}

void CRV64Assembler::Xorw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2) {
    Xor(static_cast<REGISTER64>(rd), static_cast<REGISTER64>(rs1), static_cast<REGISTER64>(rs2));
}

void CRV64Assembler::Orw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2) {
    Or(static_cast<REGISTER64>(rd), static_cast<REGISTER64>(rs1), static_cast<REGISTER64>(rs2));
}

void CRV64Assembler::Andw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2) {
    And(static_cast<REGISTER64>(rd), static_cast<REGISTER64>(rs1), static_cast<REGISTER64>(rs2));
}

void CRV64Assembler::Xoriw(REGISTER32 rd, REGISTER32 rs1, int16 imm) {
    Xori(static_cast<REGISTER64>(rd), static_cast<REGISTER64>(rs1), imm);
}

void CRV64Assembler::Oriw(REGISTER32 rd, REGISTER32 rs1, int16 imm) {
    Ori(static_cast<REGISTER64>(rd), static_cast<REGISTER64>(rs1), imm);
}

void CRV64Assembler::Andiw(REGISTER32 rd, REGISTER32 rs1, int16 imm) {
    Andi(static_cast<REGISTER64>(rd), static_cast<REGISTER64>(rs1), imm);
}

/*
 * R Format Shift Instructions:
 */

// RV32I
void CRV64Assembler::Sll(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2)
{
    WriteR(0x00001033, rd, rs1, rs2);
}

// RV32I
void CRV64Assembler::Srl(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2)
{
    WriteR(0x00005033, rd, rs1, rs2);
}

// RV32I
void CRV64Assembler::Sra(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2)
{
    WriteR(0x40005033, rd, rs1, rs2);
}

// RV64I
void CRV64Assembler::Sllw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2)
{
    WriteR(0x0000103B, rd, rs1, rs2);
}

// RV64I
void CRV64Assembler::Srlw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2)
{
    WriteR(0x0000503B, rd, rs1, rs2);
}

// RV64I
void CRV64Assembler::Sraw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2)
{
    WriteR(0x4000503B, rd, rs1, rs2);
}

/*
 * R Format Arithmetic Instructions:
 */

// RV32I
void CRV64Assembler::Add(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2)
{
    WriteR(0x00000033, rd, rs1, rs2);
}

// RV32I
void CRV64Assembler::Sub(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2)
{
    WriteR(0x40000033, rd, rs1, rs2);
}

// RV64I
void CRV64Assembler::Addw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2)
{
    WriteR(0x0000003B, rd, rs1, rs2);
}

// RV64I
void CRV64Assembler::Subw(REGISTER32 rd, REGISTER32 rs1, REGISTER32 rs2)
{
    WriteR(0x4000003B, rd, rs1, rs2);
}

/*
 * R Format Logical Instructions:
 */

// RV32I
void CRV64Assembler::Xor(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2)
{
    WriteR(0x00004033, rd, rs1, rs2);
}

// RV32I
void CRV64Assembler::Or(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2)
{
    WriteR(0x00006033, rd, rs1, rs2);
}

// RV32I
void CRV64Assembler::And(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2)
{
    WriteR(0x00007033, rd, rs1, rs2);
}

/*
 * R Format Compare Instructions:
 */

// RV32I
void CRV64Assembler::Slt(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2)
{
    WriteR(0x00002033, rd, rs1, rs2);
}

// RV32I
void CRV64Assembler::Sltu(REGISTER64 rd, REGISTER64 rs1, REGISTER64 rs2)
{
    WriteR(0x00003033, rd, rs1, rs2);
}

void CRV64Assembler::WriteR(uint32 opcode, uint32 rd, uint32 rs1, uint32 rs2)
{
    opcode |= (rd  <<  7);
    opcode |= (rs1 << 15);
    opcode |= (rs2 << 20);
    WriteWord(opcode);
}

/*
 * I Format Shift Instructions:
 */

// RV32I
void CRV64Assembler::Slli(REGISTER64 rd, REGISTER64 rs1, uint8 shamt)
{
    WriteIShiftDouble(0x00001013, rd, rs1, shamt);
}

// RV32I
void CRV64Assembler::Srli(REGISTER64 rd, REGISTER64 rs1, uint8 shamt)
{
    WriteIShiftDouble(0x00005013, rd, rs1, shamt);
}

// RV32I
void CRV64Assembler::Srai(REGISTER64 rd, REGISTER64 rs1, uint8 shamt)
{
    WriteIShiftDouble(0x40005013, rd, rs1, shamt);
}

// RV64I
void CRV64Assembler::Slliw(REGISTER32 rd, REGISTER32 rs1, uint8 shamt)
{
    WriteIShiftWord(0x0000103B, rd, rs1, shamt);
}

// RV64I
void CRV64Assembler::Srliw(REGISTER32 rd, REGISTER32 rs1, uint8 shamt)
{
    WriteIShiftWord(0x0000501B, rd, rs1, shamt);
}

// RV64I
void CRV64Assembler::Sraiw(REGISTER32 rd, REGISTER32 rs1, uint8 shamt)
{
    WriteIShiftWord(0x4000501B, rd, rs1, shamt);
}

// RV64I
void CRV64Assembler::WriteIShiftDouble(uint32 opcode, uint32 rd, uint32 rs1, uint32 shamt)
{
    // check shamt fits in unsigned 6 bit
    assert((shamt & 0x3F) == shamt);
    assert((shamt <= 63));
    opcode |= (rd  <<  7);
    opcode |= (rs1 << 15);
    opcode |= ((shamt & 0x1F) << 20);
    WriteWord(opcode);
}

// RV32I
void CRV64Assembler::WriteIShiftWord(uint32 opcode, uint32 rd, uint32 rs1, uint32 shamt)
{
    // check shamt fits in unsigned 4 bit
    assert((shamt & 0xF) == shamt);
    assert((shamt <= 15));
    opcode |= (rd  <<  7);
    opcode |= (rs1 << 15);
    opcode |= ((shamt & 0xF) << 20);
    WriteWord(opcode);
}

/*
 * I Format Arithmetic Instructions:
 */

// RV32I
void CRV64Assembler::Addi(REGISTER64 rd, REGISTER64 rs1, int16 imm)
{
    WriteI(0x00000013, rd, rs1, imm);
}

// RV64I
void CRV64Assembler::Addiw(REGISTER32 rd, REGISTER32 rs1, int16 imm)
{
    WriteI(0x0000001B, rd, rs1, imm);
}

/*
 * I Format Logical Instructions:
 */

// RV32I
void CRV64Assembler::Xori(REGISTER64 rd, REGISTER64 rs1, int16 imm)
{
    WriteI(0x00004013, rd, rs1, imm);
}

// RV32I
void CRV64Assembler::Ori(REGISTER64 rd, REGISTER64 rs1, int16 imm)
{
    WriteI(0x00006013, rd, rs1, imm);
}

// RV32I
void CRV64Assembler::Andi(REGISTER64 rd, REGISTER64 rs1, int16 imm)
{
    WriteI(0x00007013, rd, rs1, imm);
}

/*
 * I Format Compare Instructions:
 */

// RV32I
void CRV64Assembler::Slti(REGISTER64 rd, REGISTER64 rs1, int16 imm)
{
    WriteI(0x00002013, rd, rs1, imm);
}

// RV32I
void CRV64Assembler::Sltiu(REGISTER64 rd, REGISTER64 rs1, uint16 imm)
{
    WriteI(0x00003013, rd, rs1, imm);
}

/*
 * I Format Load Instructions:
 */

// RV32I
void CRV64Assembler::Lb(REGISTER64 rd, REGISTER64 rs1, int32 imm)
{
    WriteI(0x00000003, rd, rs1, imm);
}

// RV32I
void CRV64Assembler::Lh(REGISTER64 rd, REGISTER64 rs1, int32 imm)
{
    WriteI(0x00001003, rd, rs1, imm);
}

// RV32I
void CRV64Assembler::Lw(REGISTER64 rd, REGISTER64 rs1, int32 imm)
{
    WriteI(0x00002003, rd, rs1, imm);
}

// RV32I
void CRV64Assembler::Lbu(REGISTER64 rd, REGISTER64 rs1, int32 imm)
{
    WriteI(0x00004003, rd, rs1, imm);
}

// RV32I
void CRV64Assembler::Lhu(REGISTER64 rd, REGISTER64 rs1, int32 imm)
{
    WriteI(0x00005003, rd, rs1, imm);
}

// RV64I
void CRV64Assembler::Ld(REGISTER64 rd, REGISTER64 rs1, int32 imm)
{
    WriteI(0x00003003, rd, rs1, imm);
}

// RV64I
void CRV64Assembler::Lwu(REGISTER64 rd, REGISTER64 rs1, int32 imm)
{
    WriteI(0x00006003, rd, rs1, imm);
}

/*
 * I Format Jump Instructions:
 */

// RV32I
void CRV64Assembler::Jalr(REGISTER64 rd, REGISTER64 rs1, int32 imm)
{
    WriteI(0x00000067, rd, rs1, imm);
}

void CRV64Assembler::WriteI(uint32 opcode, uint32 rd, uint32 rs1, int32 imm)
{
    // check imm fits in signed 12 bit
    assert(SIGN_EXTEND_12(imm) == imm);
    assert((imm >= -2048) && (imm <= 2047));
    opcode |= (rd  <<  7);
    opcode |= (rs1 << 15);
    opcode |= ((imm & 0xFFF) << 20);
    WriteWord(opcode);
}

/*
 * S Format Store Instructions:
 */

// RV32I
void CRV64Assembler::Sb(REGISTER64 rs1, REGISTER64 rs2, int32 imm)
{
    WriteS(0x00000023, rs1, rs2, imm);
}

// RV32I
void CRV64Assembler::Sh(REGISTER64 rs1, REGISTER64 rs2, int32 imm)
{
    WriteS(0x00001023, rs1, rs2, imm);
}

// RV32I
void CRV64Assembler::Sw(REGISTER64 rs1, REGISTER64 rs2, int32 imm)
{
    WriteS(0x00002023, rs1, rs2, imm);
}

// RV64I
void CRV64Assembler::Sd(REGISTER64 rs1, REGISTER64 rs2, int32 imm)
{
    WriteS(0x00003023, rs1, rs2, imm);
}

void CRV64Assembler::WriteS(uint32 opcode, uint32 rs1, uint32 rs2, int32 imm)
{
    // check imm fits in signed 12 bit
    assert(SIGN_EXTEND_12(imm) == imm);
    assert((imm >= -2048) && (imm <= 2047));
    opcode |= (rs2 << 20);
    opcode |= (rs1 << 15);
    opcode |= ((imm & 0x1F) << 7);
    opcode |= ((imm & 0xFE0) << 20);
    WriteWord(opcode);
}

/*
 * B Format Branch Instructions:
 */

// RV32I
void CRV64Assembler::Beq(REGISTER64 rs1, REGISTER64 rs2, int32 imm)
{
    WriteB(0x00000063, rs1, rs2, imm);
}

// RV32I
void CRV64Assembler::Bne(REGISTER64 rs1, REGISTER64 rs2, int32 imm)
{
    WriteB(0x00001063, rs1, rs2, imm);
}

// RV32I
void CRV64Assembler::Blt(REGISTER64 rs1, REGISTER64 rs2, int32 imm)
{
    WriteB(0x00004063, rs1, rs2, imm);
}

// RV32I
void CRV64Assembler::Bge(REGISTER64 rs1, REGISTER64 rs2, int32 imm)
{
    WriteB(0x00005063, rs1, rs2, imm);
}

// RV32I
void CRV64Assembler::Bltu(REGISTER64 rs1, REGISTER64 rs2, int32 imm)
{
    WriteB(0x00006063, rs1, rs2, imm);
}

// RV32I
void CRV64Assembler::Bgeu(REGISTER64 rs1, REGISTER64 rs2, int32 imm)
{
    WriteB(0x00007063, rs1, rs2, imm);
}

void CRV64Assembler::WriteB(uint32 opcode, uint32 rs1, uint32 rs2, int32 imm)
{
    // check imm fits in signed? 12 bit with the first bit missing (only multiples of 2)
    assert((imm & 0x1FFE) == imm);
    assert((imm >= -4096) && (imm <= 4095) && ((imm & 0x1) == 0));
    opcode |= ((imm & 0x1E) >> 1) << 8;
    opcode |= ((imm & 0x800) >> 11) << 7;
    opcode |= ((imm & 0x1000) >> 12) << 31;
    opcode |= ((imm & 0x7E0) >> 5) << 25;
    opcode |= (rs1 << 15);
    opcode |= (rs2 << 20);
    WriteWord(opcode);
}

/*
 * U Format Arithmetic Instructions:
 */

// RV32I
void CRV64Assembler::Lui(REGISTER64 rd, int32 imm)
{
    WriteU(0x00000037, rd, imm);
}

// RV32I
void CRV64Assembler::Auipc(REGISTER64 rd, int32 imm)
{
    WriteU(0x00000017, rd, imm);
}

void CRV64Assembler::WriteU(uint32 opcode, uint32 rd, int32 imm)
{
    // check imm fits in signed/unsigned (doesnt need to sign extend) upper 20 bits of a 32 bit value
    assert((imm & 0xFFFFF000) == imm);
    assert((imm >= -2147483648) && (imm <= 2147483647) && ((imm & 0xFFF) == 0));
    opcode |= (rd  <<  7);
    //opcode |= (imm << 12);
    opcode |= ((imm & 0xFFFFF000));
    WriteWord(opcode);
}

/*
 * J Format Jump Instructions:
 */

// RV32I
void CRV64Assembler::Jal(REGISTER64 rd, int32 imm)
{
    WriteJ(0x0000006F, rd, imm);
}

void CRV64Assembler::WriteJ(uint32 opcode, uint32 rd, int32 imm)
{
    // check imm fits in signed 20 bit with the first bit missing (only multiples of 2)
    assert((imm & 0xFFFFE) == imm);
    assert((imm >= -524288) && (imm <= 524287) && ((imm & 0x1) == 0));
    opcode |= (rd << 7);
    opcode |= ((imm & 0x7FE) >> 1) << 21;
    opcode |= ((imm & 0x800) >> 11) << 20;
    opcode |= ((imm & 0xFF000) >> 12) << 12;
    opcode |= ((imm & 0x100000) >> 20) << 31;
    WriteWord(opcode);
}

void CRV64Assembler::WriteAddSubOpImm(uint32 opcode, uint32 shift, uint32 imm, uint32 rn, uint32 rd, bool flip)
{
    /*assert(imm < 0x1000);
    opcode |= (rd << 0);
    opcode |= (rn << 5);
    opcode |= ((imm & 0xFFF) << 10);
    opcode |= (shift << 22);
    WriteWord(opcode);*/

    assert(shift == 0);
    //assert(imm < 0x1000);

    assert((imm & 0xfff) == imm);

    int32 simm = (int32)imm;
    if (flip) {
        simm = -(int32)imm;
    }

    assert((simm >= -2048) && (simm <= 2047));

    //add rn, rn, imm
    //uint32 opcode = 0x00000013;
    opcode |= (rd  <<  7);
    opcode |= (rn  <<  15);
    opcode |= (simm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::WriteDataProcOpReg2(uint32 opcode, uint32 rm, uint32 rn, uint32 rd)
{
    /*opcode |= (rd <<  0);
    opcode |= (rn <<  5);
    opcode |= (rm << 16);
    WriteWord(opcode);*/
    opcode |= (rd <<  7);
    opcode |= (rn <<  15);
    opcode |= (rm << 20);
    WriteWord(opcode);
}

void CRV64Assembler::WriteLogicalOpImm(uint32 opcode, uint32 n, uint32 immr, uint32 imms, uint32 rn, uint32 rd)
{
    assert(0);
    opcode |= (rd << 0);
    opcode |= (rn << 5);
    opcode |= (imms << 10);
    opcode |= (immr << 16);
    opcode |= (n << 22);
    WriteWord(opcode);
}

void CRV64Assembler::WriteLogicalOpImm(uint32 opcode, uint32 shamt, uint32 rn, uint32 rd)
{
    /*if (shamt>0x1F) {
        shamt = 0x0;
    }*/
    /*if (shamt>0x1F) {
        uint32 opcode2 = opcode;
        opcode2 |= (rd << 7);
        opcode2 |= (rn << 15);
        opcode2 |= (0x1F << 20);
        WriteWord(opcode2);
        shamt -= 0x1F;
    }*/
    if ((opcode&0x8) == 0x8) {
        //assert((shamt & 0x1f) == shamt);
        shamt = (shamt & 0x1f);
    } else {
        //assert((shamt & 0x3f) == shamt);
        shamt = (shamt & 0x3f);
    }
    opcode |= (rd << 7);
    opcode |= (rn << 15);
    opcode |= (shamt << 20);
    WriteWord(opcode);
}

void CRV64Assembler::WriteLoadStoreOpImm(uint32 opcode, uint32 imm, uint32 rn, uint32 rt)
{
    /*opcode |= (rt << 0);
    opcode |= (rn << 5);
    opcode |= (imm << 10);
    WriteWord(opcode);*/

    int32 simm = imm;
    assert((simm >= -2048) && (simm <= 2047));

    // should be 0x7f?
    if ((opcode&0x7f)==0x03) {
        opcode |= (rt << 7);
        opcode |= (rn << 15);
        opcode |= (imm << 20);
        WriteWord(opcode);
    } else if ((opcode&0x7f)==0x23) {
        opcode |= (rt << 20);
        opcode |= (rn << 15);
        //opcode |= (imm << 7);
        opcode |= ((imm & 0x1F) << 7);
        opcode |= ((imm & 0xFE0) << 20);
        WriteWord(opcode);
    } else if ((opcode&0x7f)==0x07) {
        opcode |= (rt << 7);
        opcode |= (rn << 15);
        opcode |= (imm << 20);
        WriteWord(opcode);
    } else if ((opcode&0x7f)==0x27) {
        opcode |= (rt << 20);
        opcode |= (rn << 15);
        //opcode |= (imm << 7);
        opcode |= ((imm & 0x1F) << 7);
        opcode |= ((imm & 0xFE0) << 20);
        WriteWord(opcode);
    } else {
        assert(0);
    }
}

void CRV64Assembler::WriteMoveWideOpImm(uint32 opcode, uint32 hw, uint32 imm, uint32 rd)
{
    assert(0);
    opcode |= (rd << 0);
    opcode |= (imm << 5);
    opcode |= (hw << 21);
    WriteWord(opcode);
}

void CRV64Assembler::WriteWord(uint32 value)
{
    //assert((value != 0));
    m_stream->Write32(value);
}
