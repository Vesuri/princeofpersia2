set $instructionProfile=@PROFILE@
break pop2TimingWindowComplete
commands
 silent
 if $instructionProfile
  monitor aprof stop @RUN@/cadence.prof
 end
 printf "PROFILE C2P passes=%lu rects=%lu pixels=%lu unbatched=%lu\n",g_c2pPasses-$startPasses,g_c2pRects-$startRects,g_c2pPixels-$startPixels,g_c2pUnbatched-$startUnbatched
 dump binary memory @RUN@/profile-end-screen.bin g_integrationScreen g_integrationScreen+307200
 set $completeBuffer=display.pixels_[display.pending_ ? (display.front_^1) : display.front_]
 dump binary memory @RUN@/profile-end-planes.bin $completeBuffer $completeBuffer+196608
 printf "PROFILE INPUT right_reads=%lu\n",g_integrationRightReads
 set $context=*(unsigned long*)(g_coreA5-0x5014)
 dump binary memory @RUN@/profile-end-context.bin $context $context+0x2200
 dump binary memory @RUN@/profile-end-a5.bin g_coreA5-g_coreBelow g_coreA5
 printf "PROFILE COMPLETE fields=%lu ticks=%lu frames=%lu\n",g_scaffoldFields,inputTicks,g_timingCount
 set $i=0
 while $i<24
  if g_coreSegments[$i][0]
   printf "PROFILE ENDSEG %u %08lx %08lx\n",$i,g_coreSegments[$i][0],g_coreSegments[$i][1]
  end
  set $i=$i+1
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
 if g_integrationStage==4
  printf "PROFILE ROUTE C2P frames=%lu passes=%lu rects=%lu pixels=%lu unbatched=%lu\n",g_timingCount-1,g_c2pPasses-$startPasses,g_c2pRects-$startRects,g_c2pPixels-$startPixels,g_c2pUnbatched-$startUnbatched
 end
 printf "CLOCK FAULT %lu\n",g_clockFault
 printf "CORE RESTORE stage=%lu error=%lu vector=%lu heap=%lu zone=%lu dma=%u/%u irq=%u/%u view=%u vbi=%u\n",g_scaffoldStage,g_scaffoldError,g_coreVectorRestored,g_coreHeapOK,g_coreZoneBytes,g_savedDMA,g_actualDMA,g_savedIRQ,g_actualIRQ,g_viewRestored,g_vectorRestored
 printf "CORE EXCEPTION vector=%lu pc=%08lx\n",g_coreException,g_coreFaultPC
 printf "INTEGRATION INPUT right_reads=%lu\n",g_integrationRightReads
 printf "INTEGRATION SPACE events=%lu\n",g_integrationSpaceEvents
 printf "INTEGRATION ESC reads=%lu\n",g_integrationEscReads
 printf "INTEGRATION AUDIT code=%lu input=%lu\n",g_coreCodeAudit,g_inputVectorRestored
 continue
end
break pop2ScaffoldRestored
commands
 silent
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
 printf "PROFILE MEMORY stage=%lu free_fast=%lu largest_fast=%lu free_chip=%lu\n",g_integrationStage,g_profileFreeFast,g_profileLargestFast,g_profileFreeChip
 printf "INTEGRATION STAGE %lu fields=%lu right=%lu kind=%lu\n",g_integrationStage,g_scaffoldFields,g_integrationRightAt,g_integrationDeathKind
 if g_integrationStage==2
  printf "INTEGRATION TITLE phase=%lu cue=%lu\n",g_integrationTitlePhase,g_integrationTitleCue
  dump binary memory @RUN@/title-screen.bin g_integrationScreen g_integrationScreen+307200
  set $colors=*(unsigned long*)g_integrationColors
  dump binary memory @RUN@/title-colors.bin $colors $colors+2056
 end
 if g_integrationStage==3
  dump binary memory @RUN@/first-room-screen.bin g_integrationScreen g_integrationScreen+307200
  set $colors=*(unsigned long*)g_integrationColors
  dump binary memory @RUN@/first-room-colors.bin $colors $colors+2056
  set $context=*(unsigned long*)(g_coreA5-0x5014)
  dump binary memory @RUN@/first-room-context.bin $context $context+0x2200
  dump binary memory @RUN@/first-room-a5.bin g_coreA5-g_coreBelow g_coreA5
  dump binary memory @RUN@/first-room-planes.bin g_displayPlanes g_displayPlanes+196608
  printf "INTEGRATION DISPLAY planes=%08lx copper=%08lx parity=%u frames=%lu\n",g_displayPlanes,g_displayCopper,g_displayParity,g_displayFrames
  dump binary memory @RUN@/first-room-copper.bin g_displayCopper g_displayCopper+2236
  set $i=0
  while $i<24
   if g_coreSegments[$i][0]
    printf "PROFILE SEG %u %08lx %08lx\n",$i,g_coreSegments[$i][0],g_coreSegments[$i][1]
   end
   set $i=$i+1
  end
  maint info sections
  set $startPasses=g_c2pPasses
  set $startRects=g_c2pRects
  set $startPixels=g_c2pPixels
  set $startUnbatched=g_c2pUnbatched
  printf "PROFILE START fields=%lu ticks=%lu rows=%lu right_reads=%lu\n",g_scaffoldFields,inputTicks,g_timingCount,g_integrationRightReads
  if $instructionProfile
   monitor aprof start
  end
 end
 if g_integrationStage==4
  set $context=*(unsigned long*)(g_coreA5-0x5014)
  dump binary memory @RUN@/death-context.bin $context $context+0x2200
  dump binary memory @RUN@/death-a5.bin g_coreA5-g_coreBelow g_coreA5
  dump binary memory @RUN@/death-screen.bin g_integrationScreen g_integrationScreen+307200
  dump binary memory @RUN@/death-planes.bin g_displayPlanes g_displayPlanes+196608
 end
 continue
end
continue
