/*
* test_c60_reloc.h
*
* Specification of C6x-specific relocation handler unit tests.
*
* Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
*
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the
* distribution.
*
* Neither the name of Texas Instruments Incorporated nor the names of
* its contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#ifndef _TEST_C60_RELOC_H_
#define _TEST_C60_RELOC_H_
#include "c60_elf32.h"
#include <cxxtest/TestSuite.h>

extern "C"
{
extern void unit_c60_reloc_do(C60_RELOC_TYPE r_type, uint8_t* address,
                     uint32_t addend, uint32_t symval, uint32_t pc,
                     uint32_t base_pointer, int wrong_endian, int32_t dsbt_index);

extern void unit_c60_rel_unpack_addend(C60_RELOC_TYPE r_type,
                                       uint8_t* address,
                                       uint32_t* addend);

extern int unit_c60_rel_overflow(C60_RELOC_TYPE r_type, int32_t reloc_value);

}

class C60_TestRelocDo : public CxxTest::TestSuite
{
  public:
    void test_R_C6000_ABS32();
    void test_R_C6000_ABS16();
    void test_R_C6000_ABS8();
    void test_R_C6000_PCR_S21();
    void test_R_C6000_PCR_S12();
    void test_R_C6000_PCR_S10();
    void test_R_C6000_PCR_S7();
    void test_R_C6000_ABS_S16();
    void test_R_C6000_ABS_L16();
    void test_R_C6000_ABS_H16();
    void test_R_C6000_SBR_U15_B();
    void test_R_C6000_SBR_U15_H();
    void test_R_C6000_SBR_U15_W();
    void test_R_C6000_SBR_S16();
    void test_R_C6000_SBR_L16_B();
    void test_R_C6000_SBR_L16_H();
    void test_R_C6000_SBR_L16_W();
    void test_R_C6000_SBR_H16_B();
    void test_R_C6000_SBR_H16_H();
    void test_R_C6000_SBR_H16_W();
    void test_R_C6000_DSBT_INDEX();
};

class C60_TestRelOverflow : public CxxTest::TestSuite
{
  public:
    void test_R_C6000_ABS16();
    void test_R_C6000_ABS8();
    void test_R_C6000_PCR_S21();
    void test_R_C6000_PCR_S12();
    void test_R_C6000_PCR_S10();
    void test_R_C6000_PCR_S7();
    void test_R_C6000_SBR_S16();
    void test_R_C6000_ABS_S16();
    void test_R_C6000_SBR_U15_B();
    void test_R_C6000_SBR_U15_H();
    void test_R_C6000_SBR_U15_W();
    void test_R_C6000_DSBT_INDEX();
};

#endif /* _TEST_C60_RELOC_H_ */
