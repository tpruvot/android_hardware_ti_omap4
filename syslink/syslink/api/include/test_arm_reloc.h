/*
 *  Syslink-IPC for TI OMAP Processors
 *
 *  Copyright (c) 2008-2010, Texas Instruments Incorporated
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _TEST_ARM_RELOC_H_
#define _TEST_ARM_RELOC_H_
#include "arm_elf32.h"
#include <cxxtest/TestSuite.h>

extern "C"
{
extern void unit_arm_reloc_do(ARM_RELOC_TYPE r_type, uint8_t* address,
                     uint32_t addend, uint32_t symval, uint32_t pc,
                     uint32_t base_pointer, int wrong_endian);

extern void unit_arm_rel_unpack_addend(ARM_RELOC_TYPE r_type,
                                       uint8_t* address,
                                       uint32_t* addend);

extern int unit_arm_rel_overflow(ARM_RELOC_TYPE r_type, int32_t reloc_value);

extern void unit_arm_rel_mask_for_group(ARM_RELOC_TYPE r_type,
                                        int32_t* reloc_val);
}



class ARM_TestRelocDo : public CxxTest::TestSuite
{
  public:
    void test_R_ARM_PC24();
    void test_R_ARM_JUMP24();
    void test_R_ARM_ABS32();
    void test_R_ARM_ABS16();
    void test_R_ARM_ABS8();
    void test_R_ARM_THM_CALL();
    void test_R_ARM_THM_JUMP11();
    void test_R_ARM_THM_JUMP8();
    void test_R_ARM_THM_PC8();
    void test_R_ARM_THM_ABS5();
    void test_R_ARM_THM_JUMP6();
    void test_R_ARM_ABS12();
    void test_R_ARM_THM_JUMP19();
    void test_R_ARM_PREL31();
    void test_R_ARM_MOVW_ABS_NC();
    void test_R_ARM_MOVT_ABS();
    void test_R_ARM_MOVW_PREL_NC();
    void test_R_ARM_MOVT_PREL();
    void test_R_ARM_THM_MOVW_ABS_NC();
    void test_R_ARM_THM_MOVT_ABS();
    void test_R_ARM_THM_MOVW_PREL_NC();
    void test_R_ARM_THM_MOVT_PREL();
    void test_R_ARM_ABS32_NOI();
    void test_R_ARM_REL32_NOI();
    void test_R_ARM_THM_PC12();
    void test_R_ARM_ALU_PC_G0_NC();
    void test_R_ARM_LDR_PC_G0();
    void test_R_ARM_LDRS_PC_G0();
    void test_R_ARM_LDC_PC_G0();
};

class ARM_TestRelUnpackAddend : public CxxTest::TestSuite
{
  public:
    void test_R_ARM_PC24();
    void test_R_ARM_THM_JUMP19();
    void test_R_ARM_ABS32();
    void test_R_ARM_ABS16();
    void test_R_ARM_ABS8();
    void test_R_ARM_THM_JUMP11();
    void test_R_ARM_THM_JUMP8();
    void test_R_ARM_THM_ABS5();
    void test_R_ARM_THM_CALL();
    void test_R_ARM_THM_JUMP6();
    void test_R_ARM_THM_PC8();
    void test_R_ARM_THM_JUMP24();
    void test_R_ARM_PREL31();
    void test_R_ARM_MOVW_ABS_NC();
    void test_R_ARM_THM_MOVW_ABS_NC();
    void test_R_ARM_ABS12();
    void test_R_ARM_THM_PC12();
};


class ARM_TestRelOverflow : public CxxTest::TestSuite
{
  public:
    void test_R_ARM_PC24();
    void test_R_ARM_ABS12();
    void test_R_ARM_THM_CALL();
    void test_R_ARM_THM_JUMP19();
    void test_R_ARM_ABS16();
    void test_R_ARM_ABS8();
    void test_R_ARM_PREL31();
};

class ARM_TestGroupRelocations : public CxxTest::TestSuite
{
  public:
    void test_RelMaskForGroup();
};
#endif /* _TEST_ARM_RELOC_H_ */
