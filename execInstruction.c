XInstruction *inst = ctx.input_list.front();
    XReference result;

    bool success = inst->exec(this, result);

    if (success) {
        uint32_t value, svalue;

        switch (result.type) {
            case RT_Reg: {
                int regId = result.address;

                getRegValue(regId, value);
                svalue = (result.bitSize < BS_32)? signExtend(value, result.bitSize, BS_32) : value;

                printf("%s = 0x%X %d %u\n", xreg[regId], value, (int)svalue, value);
                break;
            }
            case RT_Mem: {
                uint32_t value;

                readMem(result.address, value, result.bitSize);
                svalue = signExtend(value, result.bitSize, BS_32);

                printf("%s [0x%X] = 0x%X %d %u\n", sizeDirectiveToString(result.bitSize), result.address, value, (int)svalue, value);
                break;
            }
            default: {}
        }

    } else {
        return false;
    }
    
    return true;
