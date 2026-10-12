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
printf "INPUT RESTORE vector=%lu\n",g_inputVectorRestored
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
while $i<g_coreCount && $i<4096
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
printf "FILE VERIFY files=%lu bytes=%lu\n",g_fileVerifiedFiles,g_fileVerifiedBytes
if g_fileVolumeQueries != 0
 printf "FILE VOLUMES count=%lu\n",g_fileVolumeQueries
 set $i=0
 while $i<g_fileVolumeQueries && $i<128
  printf "FILE VOLUME index=%u blocks=%lu used=%lu blockbytes=%lu created=%lu modified=%lu locked=%lu\n",$i,g_fileVolume[$i][0],g_fileVolume[$i][1],g_fileVolume[$i][2],g_fileVolume[$i][3],g_fileVolume[$i][4],g_fileVolume[$i][5]
  set $i=$i+1
 end
end
if g_replayBegin != 0
 printf "FILE REPLAY completed=%lu count=%lu size=%lu\n",g_replayCompleted,g_replayCount,g_replaySize
 dump binary memory @RUN@/file-replay-out.bin g_replayBegin g_replayBegin+g_replaySize
end
if g_fileProbePB != 0
 printf "FILE WRITE mode=%lu result=%ld\n",g_fileProbeWriteMode,g_fileProbeWriteResult
 printf "FILE PROBE open=%ld read=%ld close=%ld windows=%lu fields=%lu active=%u actual=%lu\n",g_fileProbeResults[0],g_fileProbeResults[1],g_fileProbeResults[2],g_systemWindows,g_systemWindowFields,g_systemWindowActive,*(unsigned long*)(g_fileProbePB+40)
 dump binary memory @RUN@/file-probe.bin g_fileProbeBuffer g_fileProbeBuffer+16
end
if g_fileProbeFCB != 0
 printf "FCB PROBE open=%ld query=%ld close=%ld\n",g_fileProbeFCBResults[0],g_fileProbeFCBResults[1],g_fileProbeFCBResults[2]
 dump binary memory @RUN@/file-fcb.bin g_fileProbeFCB g_fileProbeFCB+128
 set $name=*(unsigned long*)(g_fileProbeFCB+18)
 dump binary memory @RUN@/file-fcb-name.bin $name $name+32
end
if g_specProbeCount > 11
 printf "SPEC PROBE INVALID COUNT %lu\n",g_specProbeCount
 quit 1
end
if g_specProbeCount != 0
 set $i=0
 while $i<g_specProbeCount
  printf "SPEC PROBE index=%u result=%d\n",$i,g_specProbeResults[$i]
  set $i=$i+1
 end
 dump binary memory @RUN@/file-specs.bin g_specProbeOutputs[0] g_specProbeOutputs[0]+g_specProbeCount*70
end
if g_resourceProbeHandle != 0
 printf "RESOURCE CODE NAME address=%08lx\n",g_resourceProbeNames[3]
 printf "RESOURCE PROBE shape=%08lx option=%08lx refs=%u/%u/%u/%u\n",g_resourceProbeHandle,g_resourceProbeOption,g_resourceProbeRefs[0],g_resourceProbeRefs[1],g_resourceProbeRefs[2],g_resourceProbeRefs[3]
 set $shape=*(unsigned long*)g_resourceProbeHandle
 dump binary memory @RUN@/resource-shape.bin $shape $shape+g_resourceProbeSizes[0]
 set $option=*(unsigned long*)g_resourceProbeOption
 dump binary memory @RUN@/resource-option.bin $option $option+g_resourceProbeSizes[1]
 dump binary memory @RUN@/resource-info.bin g_resourceProbeInfo g_resourceProbeInfo+264
end
detach
quit 0
