break *pop2CoreStopped
continue
if $pc == (unsigned long)&pop2_exception_4
 continue
end
if $pc != (unsigned long)&pop2CoreStopped
 echo CORE / WRONG STOP\n
 quit 1
end
printf "CORE STOP trap=%04lx segment=%lu offset=%04lx count=%lu\n",g_coreStopTrap,g_coreStopSegment,g_coreStopOffset,g_coreCount
printf "CORE REASON %s\n",g_coreReason
printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
printf "CORE EXCEPTION vector=%lu pc=%08lx\n",g_coreException,g_coreFaultPC
printf "CORE STRS %08lx\n",g_coreStrs
set $r=0
while $r<g_coreResourceCount && $r<32
 printf "CORE RESOURCE type=%08lX id=%lu handle=%08lx\n",g_coreResources[$r][0],g_coreResources[$r][1],g_coreResources[$r][2]
 set $r=$r+1
end
set $id=0
while $id<7
 printf "CORE IDENTITY selector=%08lX error=%04lX value=%08lX\n",g_coreIdentity[$id][0],g_coreIdentity[$id][1]&65535,g_coreIdentity[$id][2]
 set $id=$id+1
end
printf "CORE SYSENV error=%lu data=",g_coreSysEnvError
set $b=0
while $b<16
 printf "%02X",g_coreSysEnv[$b]
 set $b=$b+1
end
printf "\n"
set $i=0
while $i<g_coreCount && $i<128
 printf "CODE %02lu+%04lX trap=%04lX\n",g_coreTrace[$i][0],g_coreTrace[$i][1],g_coreTrace[$i][2]
 set $i=$i+1
end
if g_coreA5 != 0
 printf "CORE A5 %08lx below=%lu initial=%08lx\n",g_coreA5,g_coreBelow,g_coreInitial
 dump binary memory @RUN@/a5-initial.bin g_coreInitial g_coreInitial+g_coreBelow
 dump binary memory @RUN@/a5-core.bin g_coreA5-g_coreBelow g_coreA5
 dump binary memory @RUN@/a5-above.bin g_coreA5 g_coreA5+8176
 set $stub=*(unsigned long*)(g_coreA5+0x68)
 if $stub != 0 && g_coreCount >= 19
  printf "CORE STUB %08lx oldload=%08lx oldexit=%08lx\n",$stub,&pop2_old_loadseg,&pop2_old_exit
  dump binary memory @RUN@/runtime-stubs.bin $stub $stub+24
 end
end
if g_coreSegments[1][0] != 0
 printf "CORE SEGMENT id=1 base=%08lx size=%lu\n",g_coreSegments[1][0],g_coreSegments[1][1]
 dump binary memory @RUN@/CODE_1.bin g_coreSegments[1][0] g_coreSegments[1][0]+g_coreSegments[1][1]
end
if g_coreSegments[19][0] != 0
 printf "CORE SEGMENT id=19 base=%08lx size=%lu\n",g_coreSegments[19][0],g_coreSegments[19][1]
 dump binary memory @RUN@/CODE_19.bin g_coreSegments[19][0] g_coreSegments[19][0]+g_coreSegments[19][1]
end
if g_coreSegments[21][0] != 0
 printf "CORE SEGMENT id=21 base=%08lx size=%lu\n",g_coreSegments[21][0],g_coreSegments[21][1]
 dump binary memory @RUN@/CODE_21.bin g_coreSegments[21][0] g_coreSegments[21][0]+g_coreSegments[21][1]
end
if g_coreSegments[22][0] != 0
 printf "CORE SEGMENT id=22 base=%08lx size=%lu\n",g_coreSegments[22][0],g_coreSegments[22][1]
 dump binary memory @RUN@/CODE_22.bin g_coreSegments[22][0] g_coreSegments[22][0]+g_coreSegments[22][1]
end
if g_coreSegments[2][0] != 0
 printf "CORE SEGMENT id=2 base=%08lx size=%lu\n",g_coreSegments[2][0],g_coreSegments[2][1]
 dump binary memory @RUN@/CODE_2.bin g_coreSegments[2][0] g_coreSegments[2][0]+g_coreSegments[2][1]
end
detach
quit 0
