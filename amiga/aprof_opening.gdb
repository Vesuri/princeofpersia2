set $instructionProfile=@PROFILE@
set $phase=0
break pop2OpeningPhase
commands
 silent
 if $instructionProfile
  eval "monitor aprof stop @RUN@/opening-%d.prof", $phase
 end
 printf "OPENING PHASE %lu frames=%lu fields=%lu health=%u\n",$phase,g_openingCount,g_scaffoldFields,*(unsigned short*)(g_coreA5-0x50b4)
 set $phase=$phase+1
 if $instructionProfile
  monitor aprof start
 end
 continue
end
break pop2UIOverlayShown
commands
 silent
 printf "INTEGRATION ALERT id=%lu\n",g_uiOverlay
 detach
 quit 1
end
break pop2CoreStopped
commands
 silent
 printf "CORE STOP trap=%04lx segment=%lu offset=%04lx\n",g_coreStopTrap,g_coreStopSegment,g_coreStopOffset
 printf "CORE REASON %s\n",g_coreReason
 printf "CLOCK FAULT %lu\n",g_clockFault
 printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
 printf "CORE EXCEPTION vector=%lu pc=%08lx\n",g_coreException,g_coreFaultPC
 printf "INTEGRATION INPUT right_reads=%lu\n",g_integrationRightReads
 printf "INTEGRATION SPACE events=%lu\n",g_integrationSpaceEvents
 printf "INTEGRATION AUDIT code=%lu input=%lu\n",g_coreCodeAudit,g_inputVectorRestored
 continue
end
break pop2ScaffoldRestored
commands
 silent
 printf "OPENING RECORDED rows=%lu phases=%lu\n",g_openingCount,g_openingPhases
 if g_openingCount
  dump binary memory @RUN@/opening-frames.bin &g_openingRows[0][0] &g_openingRows[g_openingCount][0]
 end
 printf "TIMING RECORDED rows=%lu\n",g_timingCount
 if g_timingCount
  dump binary memory @RUN@/timing-frames.bin &g_timingRows[0][0] &g_timingRows[g_timingCount][0]
 end
 dump binary memory @RUN@/timing-samples.bin &g_timingSamples[0] &g_timingSamples[0x1005]
 printf "TIMING SETTERS rows=%lu\n",g_timingSetCount
 if g_timingSetCount
  dump binary memory @RUN@/timing-set.bin &g_timingSetRows[0][0] &g_timingSetRows[g_timingSetCount][0]
 end
 echo INTEGRATION RESTORED\n
 detach
 quit 0
end
break pop2IntegrationStage
commands
 silent
 printf "INTEGRATION STAGE %lu fields=%lu right=%lu kind=%lu\n",g_integrationStage,g_scaffoldFields,g_integrationRightAt,g_integrationDeathKind
 if g_integrationStage==3
  dump binary memory @RUN@/first-room-screen.bin g_integrationScreen g_integrationScreen+307200
  set $colors=*(unsigned long*)g_integrationColors
  dump binary memory @RUN@/first-room-colors.bin $colors $colors+2056
  dump binary memory @RUN@/first-room-a5.bin g_coreA5-g_coreBelow g_coreA5
  set $context=*(unsigned long*)(g_coreA5-0x5014)
  dump binary memory @RUN@/first-room-context.bin $context $context+0x2200
  dump binary memory @RUN@/jt.bin g_coreA5+32 g_coreA5+32+8*1018
  printf "PROFILE A5 %08lx\n",g_coreA5
  set $i=0
  while $i<24
   if g_coreSegments[$i][0]
    printf "PROFILE SEG %u %08lx %08lx\n",$i,g_coreSegments[$i][0],g_coreSegments[$i][1]
   end
   set $i=$i+1
  end
  maint info sections
  printf "PROFILE START fields=%lu ticks=%lu right_reads=%lu\n",g_scaffoldFields,inputTicks,g_integrationRightReads
  if $instructionProfile
   monitor aprof start
  end
 end
 if g_integrationStage==4
  if $instructionProfile
   eval "monitor aprof stop @RUN@/opening-%d.prof", $phase
  end
  printf "PROFILE STOP fields=%lu ticks=%lu frames=%lu phases=%lu\n",g_scaffoldFields,inputTicks,g_openingCount,g_openingPhases
  set $i=0
  while $i<24
   if g_coreSegments[$i][0]
    printf "PROFILE ENDSEG %u %08lx %08lx\n",$i,g_coreSegments[$i][0],g_coreSegments[$i][1]
   end
   set $i=$i+1
  end
  dump binary memory @RUN@/death-screen.bin g_integrationScreen g_integrationScreen+307200
  dump binary memory @RUN@/death-a5.bin g_coreA5-g_coreBelow g_coreA5
  set $context=*(unsigned long*)(g_coreA5-0x5014)
  dump binary memory @RUN@/death-context.bin $context $context+0x2200
 end
 continue
end
continue
