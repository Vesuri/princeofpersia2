// Opt-in comparisons against the existing full service path. No game decisions
// execute during the fixture; restore the port, scheduler and stop identities.
extern "C" {
uint32_t g_quickDisplayFirstFailure[5]={};
uint32_t g_quickDisplayEpochRetries=0;
uint32_t g_quickDisplayChecks=0,g_quickDisplayRejects=0,g_quickDisplayErrors=0,g_quickDisplayFaults=0;
}
static void quickDisplayFixture() {
    const uint16_t traps[]={0xa8a8,0xa8a9,0xa8a7,0xa8aa,0xa8ab,0xa8ae,0xaa14,0xaa15,0xab1d};
    uint32_t oldTrap=g_coreStopTrap,oldSeg=g_coreStopSegment,oldOff=g_coreStopOffset;
    uint8_t getArgs[8]={};uint32_t port=0,rr[15]={},consumed=0;
    sl(getArgs,(uint32_t)&port);
    if(!NativeDisplay::dispatch(0xa874,rr,getArgs,consumed) || !validMemory(port,108)) {
        ++g_quickDisplayFaults;return;
    }
    uint8_t savedPort[52];__builtin_memcpy(savedPort,(void*)(port+36),52);
    for(uint16_t trap:traps) {
        uint32_t pc=0;
        for(unsigned seg=0;seg<24 && !pc;++seg)
            for(uint32_t off=0;off+2<=g_coreSegments[seg][1];off+=2)
                if(w((uint8_t*)g_coreSegments[seg][0]+off)==trap){pc=g_coreSegments[seg][0]+off;break;}
        if(!pc){++g_quickDisplayFaults;continue;}
        auto entry=g_pop2QuickTraps[trap&0xfff];
        for(unsigned variant=0;variant<4;++variant)for(unsigned cc=0;cc<32;++cc) {
            uint8_t args[96]={},data[32]={},beforeArgs[32],beforeData[32],expectedArgs[32],expectedData[32];
            uint8_t fastFrame[8]={},fullFrame[8]={},beforePort[52],expectedPort[52];uint32_t fast[15],full[15];
            for(unsigned i=0;i<15;++i)fast[i]=full[i]=0x12340000+i;
            const int16_t bounds[4][8]={{-2,-3,10,12,4,5,14,18},{0,0,0,0,1,1,2,2},
                {-10,-10,-5,-5,5,5,10,10},{-32768,-32768,32767,32767,-1,-1,1,1}};
            for(unsigned i=0;i<8;++i)sw(data+i*2,bounds[variant][i]);
            if(trap==0xa8a8 || trap==0xa8a9){sw(args,3);sw(args+2,2);sl(args+4,(uint32_t)data);}
            else if(trap==0xa8a7){for(unsigned i=0;i<4;++i)sw(args+i*2,bounds[variant][3-i]);sl(args+8,(uint32_t)data);}
            else if(trap==0xa8aa || trap==0xa8ab){sl(args,(uint32_t)data+16);sl(args+4,(uint32_t)data+8);sl(args+8,(uint32_t)data);}
            else if(trap==0xa8ae)sl(args,(uint32_t)data);
            else if(trap==0xab1d){sl(args,l((uint8_t*)port+2));fast[0]=full[0]=0x4000f;}
            else {for(unsigned i=0;i<6;++i)data[i]=(variant&1)?255:0;sl(args,(uint32_t)data);}
            sl(args+64,0x12345678);sl(args+68,0x23456780);
            displayBatchFrame=(uint32_t)args+64;displayBatchParent=0x12345678;displayBatchReturn=0x23456780;
            timerCount=vblCount=0;quickServicesValid=false;quickFixtureAudioPending=0;
            sw(fastFrame,cc);sw(fullFrame,cc);sl(fastFrame+2,pc);sl(fullFrame+2,pc);
            __builtin_memcpy(beforeArgs,args,32);__builtin_memcpy(beforeData,data,32);
            __builtin_memcpy(beforePort,(void*)(port+36),52);
            uint32_t result=0;
            // The real entry may reject an interrupt crossing its readiness
            // check. Require an observed field change and untouched guest state
            // before retrying; every case must still obtain a positive entry.
            for(unsigned attempt=0;attempt<8;++attempt) {
                uint32_t field=g_scaffoldFields;
                result=entry(fast,fastFrame,args);
                if(result || quickDisplayFailure.reason || field==g_scaffoldFields)break;
                bool unchanged=true;
                for(unsigned i=0;i<15;++i)if(fast[i]!=full[i])unchanged=false;
                for(unsigned i=0;i<8;++i)if(fastFrame[i]!=fullFrame[i])unchanged=false;
                for(unsigned i=0;i<32;++i)if(args[i]!=beforeArgs[i] || data[i]!=beforeData[i])unchanged=false;
                for(unsigned i=0;i<52;++i)if(((uint8_t*)port)[36+i]!=beforePort[i])unchanged=false;
                if(!unchanged)break;
                ++g_quickDisplayEpochRetries;
            }
            __builtin_memcpy(expectedArgs,args,32);__builtin_memcpy(expectedData,data,32);
            __builtin_memcpy(expectedPort,(void*)(port+36),52);
            __builtin_memcpy((void*)(port+36),beforePort,52);
            __builtin_memcpy(args,beforeArgs,32);__builtin_memcpy(data,beforeData,32);
            uint32_t mismatch=(!result?1:0)|(quickDisplayFailure.reason?2:0);
            bool ok=result && !quickDisplayFailure.reason;
            sl(fastFrame+2,l(fastFrame+2)+2);
            uint32_t returned=pop2Dispatch(full,fullFrame,args);
            if(didStop)mismatch|=4;
            if(returned!=(uint32_t)args+result-1)mismatch|=8;
            ok=ok && !didStop && returned==(uint32_t)args+result-1;
            for(unsigned i=0;i<15;++i)if(fast[i]!=full[i]){ok=false;mismatch|=16;}
            for(unsigned i=0;i<8;++i)if(fastFrame[i]!=fullFrame[i]){ok=false;mismatch|=32;}
            for(unsigned i=0;i<32;++i)if(args[i]!=expectedArgs[i] || data[i]!=expectedData[i]){ok=false;mismatch|=64;}
            for(unsigned i=0;i<52;++i)if(((uint8_t*)port)[36+i]!=expectedPort[i]){ok=false;mismatch|=128;}
            if(ok)++g_quickDisplayChecks;else {
                if(!g_quickDisplayFaults){g_quickDisplayFirstFailure[0]=trap;g_quickDisplayFirstFailure[1]=variant;g_quickDisplayFirstFailure[2]=cc;g_quickDisplayFirstFailure[3]=result;g_quickDisplayFirstFailure[4]=mismatch;}
                ++g_quickDisplayFaults;
            }
            didStop=false;quickDisplayFailure={};
        }
        for(unsigned fault=1;fault<=14;++fault) {
            uint8_t args[96]={},frame[8]={};uint32_t regs[15]={};uint8_t* usp=args;
            regs[0]=trap==0xab1d?0x4000f:0;sl(frame+2,pc);
            sl(args+64,0x12345678);sl(args+68,0x23456780);
            displayBatchFrame=(uint32_t)args+64;displayBatchParent=0x12345678;displayBatchReturn=0x23456780;
            timerCount=vblCount=0;quickServicesValid=false;quickFixtureAudioPending=0;
            if(fault==1)sw(frame,0x2000);
            if(fault==2)usp=stack+126;
            if(fault==3)usp=stack+65534;
            if(fault==4)sl(frame+2,0);
            if(fault==5)didStop=true;
            if(fault==6)g_timerCallbackActive=1;
            if(fault==7)++displayBatchParent;
            if(fault==8)++displayBatchReturn;
            if(fault==9)usp=args+72;
            if(fault==10)g_clockFault=1;
            if(fault==11){vblCount=1;vblPass.pending=1;}
            if(fault==12){timerCount=1;timers[0]={args,{g_scaffoldFields-1,0},true};}
            if(fault==13)quickFixtureAudioPending=1;
            if(fault==14)displayBatchFrame=0;
            uint32_t before[15];__builtin_memcpy(before,regs,sizeof regs);
            bool ok=entry(regs,frame,usp)==0 && !quickDisplayFailure.reason;
            for(unsigned i=0;i<15;++i)if(before[i]!=regs[i])ok=false;
            if(ok)++g_quickDisplayRejects;else ++g_quickDisplayFaults;
            didStop=false;g_timerCallbackActive=0;g_clockFault=0;vblPass.pending=false;vblPass.active=false;
            quickDisplayFailure={};
        }
        // These bodies reject invalid data through NativeDisplay. Once failed,
        // changing the arguments must not turn a deferred stop into a success.
        if(trap==0xaa14 || trap==0xaa15)for(unsigned changedOwner=0;changedOwner<2;++changedOwner) {
            uint8_t args[96]={},frame[8]={};uint32_t regs[15]={};
            sl(frame+2,pc);sl(args+64,0x12345678);sl(args+68,0x23456780);
            displayBatchFrame=(uint32_t)args+64;displayBatchParent=0x12345678;displayBatchReturn=0x23456780;
            timerCount=vblCount=0;quickServicesValid=false;quickFixtureAudioPending=0;
            bool ok=entry(regs,frame,args)==0 && quickDisplayFailure.reason;
            const char* error=quickDisplayFailure.reason;
            sl(args,(uint32_t)args+32); // Valid black input if the body were retried.
            if(changedOwner)quickDisplayFailure.pc+=2;
            uint32_t returned=pop2Dispatch(regs,frame,args);
            ok=ok && didStop && !quickDisplayFailure.reason && returned==(uint32_t)args &&
                l(frame+2)==(uint32_t)pop2_mac_exit && g_coreStopTrap==trap;
            if(!changedOwner)ok=ok && g_coreReason==error;
            else ok=ok && g_coreReason==quickDisplayOwnerError;
            if(ok)++g_quickDisplayErrors;else ++g_quickDisplayFaults;
            didStop=false;quickDisplayFailure={};
        }
    }
    __builtin_memcpy((void*)(port+36),savedPort,52);
    g_coreStopTrap=oldTrap;g_coreStopSegment=oldSeg;g_coreStopOffset=oldOff;
}
