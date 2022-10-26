/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/

#include "sw_nal_unit.h"
#ifdef TEST_DATA
#include "enctrace.h"
#endif

/*------------------------------------------------------------------------------
  nal_unit
------------------------------------------------------------------------------*/
void nal_unit(struct buffer *b, struct nal_unit *nal_unit)
{
    COMMENT(b, "forbidden_zero_bit")
    put_bit(b, 0, 1);

    COMMENT(b, "nal_unit_type")
    put_bit(b, nal_unit->type, 6);

    COMMENT(b, "nuh_layer_id")
    put_bit(b, 0, 6);

    COMMENT(b, "nuh_temporal_id_plus1\n")
    put_bit(b, nal_unit->temporal_id + 1, 3);
}

void HevcNalUnitHdr(struct buffer *stream, enum nal_type nalType, true_e byteStream)
{
    struct nal_unit nal_unit_val;
    if (byteStream == ENCHW_YES) {
        put_bits_startcode(stream);
    }

    nal_unit_val.type = nalType;
    nal_unit_val.temporal_id = 0;
    nal_unit(stream, &nal_unit_val);
}

/*------------------------------------------
      H264 functions
--------------------------------------------*/
void H264NalUnitHdr(struct buffer *b, i32 nalRefIdc, enum nal_type nalType, true_e byteStream)
{
    if (byteStream == ENCHW_YES) {
        put_bits_startcode(b);
    }

    put_bit(b, 0, 1);
    COMMENT(b, "forbidden_zero_bit");

    put_bit(b, nalRefIdc, 2);
    COMMENT(b, "nal_ref_idc");

    put_bit(b, (i32)nalType, 5);
    COMMENT(b, "nal_unit_type");
}
