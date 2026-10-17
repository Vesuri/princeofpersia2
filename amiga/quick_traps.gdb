break pop2QuickProbeDone
commands
 silent
 printf "BLOCK MOVE checks=%lu\n",g_blockMoveChecks
 printf "QUICK CONTROLS calls=%lu rejected=%lu failures=%lu\n",g_quickProbeCalls,g_quickProbeRejected,g_quickProbeFailures
 printf "QUICK DEADLINES rejected=%lu\n",g_quickDeadlineRejected
 printf "QUICK ZONE rejected=%lu\n",g_quickZoneRejected
 printf "QUICK CALLBACK checks=%lu rejected=%lu\n",g_quickCallbackChecks,g_quickCallbackRejected
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
 printf "CORE STOP trap=%04lx segment=%lu offset=%04lx count=%lu\n",g_coreStopTrap,g_coreStopSegment,g_coreStopOffset,g_coreCount
 printf "CORE REASON %s\n",g_coreReason
 printf "QUICK GUARD checks=%lu faults=%lu\n",g_quickGuardChecks,g_quickGuardFault
 printf "QUICK AUDIO MODE %ld\n",quickFixtureAudioPending
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
 echo INTEGRATION RESTORED\n
 detach
 quit 0
end
break pop2IntegrationStage
commands
 silent
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
 end
 if g_integrationStage==4
  set $context=*(unsigned long*)(g_coreA5-0x5014)
  dump binary memory @RUN@/death-context.bin $context $context+0x2200
  dump binary memory @RUN@/death-a5.bin g_coreA5-g_coreBelow g_coreA5
  dump binary memory @RUN@/death-screen.bin g_integrationScreen g_integrationScreen+307200
 end
 continue
end
continue
